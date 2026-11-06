# Trace du clignotement de Simon (790C : C43A / C434, 78CC : C42D) par rapport a C003.
# A ajouter apres tools/vertical_links_trace.tcl ; lance par tools/run_simon_blink_trace.py.
# Actions de plan en plus :
#   « item OBJ » pose l'objet OBJ (hex, ex. 09) sous Simon dans le premier emplacement libre
#   de C500 (type 84 ramassable, +1 Y = C425 - 10, +2 X = C427 - 8, +4 objet) : le vrai
#   ramassage 7FE2 -> 8D30 s'execute dans la passe suivante.
#   « shots N » : instantanes VRAM pendant N VBLANK (l'action suivante demarre 10 VBLANK
#   plus tard : « shots 20 » puis « item 09 » couvre l'avant et l'apres du ramassage).
# Sorties :
#   blink.csv : une ligne par VBLANK (crochet 4028, avant 414D) : C003, C005, C434, C43A,
#     C42D, C420, D400 (couleur de la 1re ligne du sprite 0 de Simon), D600 (Y du sprite 0),
#     et, lus en VRAM (table des sprites R5/R11), la couleur et l'Y du sprite de Simon
#     (entree de la SAT de meme X et motif que D601/D602, -1 si absente).
#   blink_ev.csv : PC remarquables (414D C003+1, 790C remplissage D400, 78CC test C42D,
#     6560 envoi SAT, 658A envoi des couleurs) et ecritures de C434/C43A/C42D, avec le
#     decalage en microsecondes depuis le dernier VBLANK et D400/D600 a cet instant.
#   shots/fNNNNN.vram/.txt : VRAM (128 Ko), registres et palette du VDP lus au VBLANK
#     NNNNN, donc l'etat de l'image affichee entre les VBLANK NNNNN-1 et NNNNN (les envois
#     6560/658A ont lieu ~1 ms apres le VBLANK) ; pris pendant « shots N », les 16 VBLANK
#     qui suivent le depart d'une minuterie, tant qu'une minuterie vaut 3 ou moins et
#     12 VBLANK apres. Rendu : tools/analyze_simon_blink.py (« screenshot » exige le rendu
#     OpenGL, absent sans affichage).
proc key_snap {label} {}
file mkdir [file join $vk_out shots]
set b_csv [open [file join $vk_out blink.csv] w]
puts $b_csv "frame,c003,c005,c434,c43a,c42d,c420,d000,d001,x,y,d400,d600,vram_col,vram_y,vram_idx,c00f"
set b_ev [open [file join $vk_out blink_ev.csv] w]
puts $b_ev "frame,us,what,pc,c003,c434,c43a,c42d,d400,d600"
set b_t0 0.0
set b_tail 0
set b_pre 0
set b_head 0
set b_on 0
proc b_us {} {
    global b_t0
    return [format %.0f [expr {([machine_info time] - $b_t0) * 1e6}]]
}
proc b_ev {what} {
    global b_ev vk_frame
    puts $b_ev "$vk_frame,[b_us],$what,[vk_pc],[vk_rd 0xC003],[vk_rd 0xC434],[vk_rd 0xC43A],[vk_rd 0xC42D],[vk_rd 0xD400],[vk_rd 0xD600]"
}
proc b_vram_simon {} {
    # SAT (mode 2) = R11 bits 0-1 en A15-A16, R5 & FC en A7-A14 ; couleurs = SAT - 200h.
    set r5 [debug read {VDP regs} 5]
    set r11 [debug read {VDP regs} 11]
    set sat [expr {(($r11 & 3) << 15) | (($r5 & 0xFC) << 7)}]
    set x [vk_rd 0xD601]; set p [vk_rd 0xD602]
    for {set i 0} {$i < 32} {incr i} {
        set a [expr {$sat + 4 * $i}]
        if {[debug read VRAM [expr {$a + 1}]] == $x && [debug read VRAM [expr {$a + 2}]] == $p} {
            return "[debug read VRAM [expr {$sat - 0x200 + 16 * $i}]],[debug read VRAM $a],$i"
        }
    }
    return "-1,-1,-1"
}
proc b_snap {} {
    global vk_out vk_frame
    set base [file join $vk_out shots [format f%05d $vk_frame]]
    set f [open $base.vram w]; fconfigure $f -translation binary -encoding binary
    puts -nonewline $f [debug read_block VRAM 0 131072]; close $f
    set regs ""
    for {set i 0} {$i < 48} {incr i} { append regs "[debug read {VDP regs} $i] " }
    set pal ""
    for {set i 0} {$i < 32} {incr i} { append pal "[debug read {VDP palette} $i] " }
    set f [open $base.txt w]
    puts $f "regs $regs"; puts $f "pal $pal"
    puts $f "x [vk_rd 0xC427] y [vk_rd 0xC425] d [vk_rd 0xD000]/[vk_rd 0xD001]"
    close $f
}
proc b_frame {} {
    global b_csv vk_frame b_t0 b_tail b_pre b_head b_on
    set b_t0 [machine_info time]
    set r {}
    foreach a {C003 C005 C434 C43A C42D C420 D000 D001 C427 C425 D400 D600} { lappend r [vk_rd 0x$a] }
    puts $b_csv "$vk_frame,[join $r ,],[b_vram_simon],[vk_rd 0xC00F]"
    if {$vk_frame % 200 == 0} { flush $b_csv }
    set on 0
    set low 1
    foreach a {C434 C43A C42D} {
        set v [vk_rd 0x$a]
        if {$v} { set on 1 }
        if {$v > 3} { set low 0 }
    }
    if {$on && !$b_on} { set b_head 16 }
    set b_on $on
    if {$on} { set b_tail 12 }
    if {($on && $low) || $b_tail > 0 || $b_pre > 0 || $b_head > 0} { b_snap }
    if {!$on && $b_tail > 0} { incr b_tail -1 }
    if {$b_pre > 0} { incr b_pre -1 }
    if {$b_head > 0} { incr b_head -1 }
}
debug set_bp 0x4028 {} {b_frame}
foreach {pc what} {0x414D c003_inc 0x790C fill_d400 0x78CC test_c42d 0x6560 sat_upload 0x658A col_upload} {
    debug set_bp $pc {} [list b_ev $what]
}
foreach a {C434 C43A C42D} {
    debug set_watchpoint write_mem [list 0x$a] {} [list b_ev "write_$a"]
}
proc vk_custom_action {w} {
    global b_pre
    if {[lindex $w 0] eq "shots"} { set b_pre [lindex $w 1]; return }
    if {[lindex $w 0] ne "item"} { return }
    set obj [scan [lindex $w 1] %x]
    for {set s 0} {$s < 8} {incr s} {
        set a [expr {0xC500 + 16 * $s}]
        if {[vk_rd $a] == 0} { break }
    }
    debug write memory [expr {$a + 1}] [expr {([vk_rd 0xC425] - 10) & 255}]
    debug write memory [expr {$a + 2}] [expr {([vk_rd 0xC427] - 8) & 255}]
    debug write memory [expr {$a + 3}] 0
    debug write memory [expr {$a + 4}] $obj
    debug write memory [expr {$a + 5}] 0
    debug write memory $a 0x84
    vk_log_ev "ITEM [lindex $w 1] slot [format %04X $a] x=[vk_rd 0xC427] y=[vk_rd 0xC425]"
    b_ev "item_[lindex $w 1]"
}
