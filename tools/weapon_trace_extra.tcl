# Trace des armes secondaires (C416 >= 2 : dague, hache, croix ; objet 1E : eau benite),
# a ajouter apres tools/vertical_links_trace.tcl (lance par tools/run_weapon_trace.py).
# Voir research/secondary-weapons.md.
# Actions de plan en plus :
#   « item OBJ » : pose l'objet OBJ (hex) sous Simon dans le premier emplacement libre de C500
#     (type 84, +1 Y = C425 - 10, +2 X = C427 - 8, +4 objet) : le vrai ramassage
#     7FE2 -> 8D30 s'execute a la passe suivante ;
#   « shots N » : instantanes VRAM pendant N VBLANK (shots/fNNNNN.vram et .txt).
# Sorties :
#   weapon.csv : une ligne par VBLANK (crochet 4028) : salle, Simon (X, Y, C420, C42C), C003,
#     C422, C42E, C42F, C416, C436, C417 (coeurs, BCD), C701, C433, les 16 octets des
#     emplacements C450 et C460, la SAT D618-D627 (2 x 2 sprites), les couleurs D460, D470,
#     D480, D490, les 7 blocs C800 (+0, +3 Y, +5 X, +0D, +0E), les 8 bougies C470 (+0, +3),
#     les 8 objets C500 (+0, +4), C006.
#   weapon_ev.csv : PC remarquables (registres A, IX, IY) : lancement, liberation, perte de
#     l'arme, coups, sons.
proc key_snap {label} { w_snap }
file mkdir [file join $vk_out shots]
set w_csv [open [file join $vk_out weapon.csv] w]
set w_head "frame,d000,d001,x,y,c420,c42c,c003,c422,c42e,c42f,c416,c436,c417,c701,c433"
foreach s {a b} { for {set i 0} {$i < 16} {incr i} { append w_head ",$s$i" } }
for {set i 0} {$i < 16} {incr i} { append w_head ",sat$i" }
append w_head ",d460,d470,d480,d490"
for {set i 0} {$i < 7} {incr i} { append w_head ",e${i}s,e${i}y,e${i}x,e${i}hp,e${i}f" }
for {set i 0} {$i < 8} {incr i} { append w_head ",k${i}s,k${i}n" }
for {set i 0} {$i < 8} {incr i} { append w_head ",o${i}s,o${i}k" }
append w_head ",c006,c415"
puts $w_csv $w_head
set w_shots 0
proc w_snap {} {
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
proc w_frame {} {
    global w_csv vk_frame w_shots
    set r {}
    foreach a {D000 D001 C427 C425 C420 C42C C003 C422 C42E C42F C416 C436 C417 C701 C433} { lappend r [vk_rd 0x$a] }
    for {set i 0} {$i < 32} {incr i} { lappend r [vk_rd [expr {0xC450 + $i}]] }
    for {set i 0} {$i < 16} {incr i} { lappend r [vk_rd [expr {0xD618 + $i}]] }
    foreach a {D460 D470 D480 D490} { lappend r [vk_rd 0x$a] }
    for {set i 0} {$i < 7} {incr i} {
        set b [expr {0xC800 + 0x80 * $i}]
        foreach o {0 3 5 13 14} { lappend r [vk_rd [expr {$b + $o}]] }
    }
    for {set i 0} {$i < 8} {incr i} {
        set b [expr {0xC470 + 16 * $i}]
        foreach o {0 3} { lappend r [vk_rd [expr {$b + $o}]] }
    }
    for {set i 0} {$i < 8} {incr i} {
        set b [expr {0xC500 + 16 * $i}]
        foreach o {0 4} { lappend r [vk_rd [expr {$b + $o}]] }
    }
    lappend r [vk_rd 0xC006] [vk_rd 0xC415]
    puts $w_csv "$vk_frame,[join $r ,]"
    if {$vk_frame % 200 == 0} { flush $w_csv }
    if {$w_shots > 0} { w_snap; incr w_shots -1 }
}
debug set_bp 0x4028 {} {w_frame}
set w_ev [open [file join $vk_out weapon_ev.csv] w]
puts $w_ev "frame,what,pc,a,b,ix,iy,c416,c422,c42f,c450,c451,c460,c461,c417"
proc w_log {what} {
    global w_ev vk_frame
    set a [debug read {CPU regs} 0]
    set b [debug read {CPU regs} 2]
    set ix [format %02X%02X [debug read {CPU regs} 16] [debug read {CPU regs} 17]]
    set iy [format %02X%02X [debug read {CPU regs} 18] [debug read {CPU regs} 19]]
    puts $w_ev "$vk_frame,$what,[vk_pc],[format %02X $a],[format %02X $b],$ix,$iy,[vk_rd 0xC416],[vk_rd 0xC422],[vk_rd 0xC42F],[vk_rd 0xC450],[vk_rd 0xC451],[vk_rd 0xC460],[vk_rd 0xC461],[format %02X [vk_rd 0xC417]]"
    flush $w_ev
}
foreach {pc what} {0x71D4 reserve_71D4 0x7133 attack_7133 0x72A6 launch_72A6 0x71AA water_71AA
                   0x74E9 free_74E9 0x74EC free_74EC 0x8E93 lost_8E93 0x74A5 axe_drop_74A5
                   0x8992 drop_8992 0x8D30 pick_8D30 0x509F sound_509F 0x81AB kill_pts_81AB
                   0x9A3E kill_9A3E 0x804C hit_enemy_804C 0x80B9 hit_candle_80B9
                   0x7F99 hit_d700_7F99 0x810A hit_wall_810A 0x7DA0 nowhip_7DA0 0x7DAD whip_7DAD
                   0x5593 gfx_5593 0x7373 boom_back_7373 0x739D boom_ret_739D 0x7359 edge_7359
                   0x7407 water_floor_7407 0x7433 water_end_7433 0x999F rearm_999F} {
    debug set_bp $pc {} [list w_log $what]
}
proc vk_custom_action {w} {
    global w_shots
    if {[lindex $w 0] eq "shots"} { set w_shots [lindex $w 1]; return }
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
    w_log "item_[lindex $w 1]"
}
proc vk_finish {} {
    global vk_csv vk_ev vk_wcsv vk_out w_csv w_ev
    vk_press 0
    close $vk_csv; close $vk_ev; close $vk_wcsv
    close $w_csv; close $w_ev
    set f [open [file join $vk_out done.txt] w]; puts $f ok; close $f
    exit
}
