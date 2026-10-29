# Trace du calendrier d'affichage (research/frame-timing.md) : quand chaque element calcule
# dans une passe atteint l'ecran. A ajouter apres tools/vertical_links_trace.tcl ; lance
# par tools/run_frame_timing_trace.py, analyse par tools/analyze_frame_timing.py.
# Action de plan en plus : « item OBJ » (objet OBJ pose sous Simon dans C500, type 84,
# comme tools/simon_blink_trace_extra.tcl ; le vrai ramassage 7FE2 -> 8D30 s'execute).
# Sorties :
#   ft_frames.csv : une ligne par VBLANK (crochet 4028, avant 4B9D et 414D) : C003, C005,
#     C006 (touches nouvelles), C007 (touches tenues), X/Y de Simon (C427/C425), C417
#     (coeurs), C415 (energie), C42D, C43E, R#7 (bordure), l'entree SAT de Simon lue en
#     VRAM (Y, X, motif ; -1 si absente) et la SAT entiere (32 entrees, hexadecimal) : la
#     SAT lue au VBLANK N est celle de l'image N-1 (les envois 6560/658A ont lieu ~1 ms
#     apres le VBLANK).
#   ft_events.csv : evenements avec le decalage en microsecondes depuis le dernier VBLANK :
#     4BBB (lecture des touches), 414D (C003 + 1), 6560 / 658A (envoi SAT / couleurs),
#     depart d'une commande VDP (ecriture de R#46 par le port 9Bh : CMD, DX, DY, NX, NY),
#     CHGCLR (0062, F3EB), 509F (demande de son, A), ecritures de C096 (effet en cours du
#     pilote 8964), de C417, C415 et C42D.
proc key_snap {label} {}
set ft_csv [open [file join $vk_out ft_frames.csv] w]
puts $ft_csv "frame,c003,c005,c006,c007,x,y,c417,c415,c42d,c43e,r7,sat_y,sat_x,sat_p,sat"
set ft_ev [open [file join $vk_out ft_events.csv] w]
puts $ft_ev "frame,us,what,pc,a,b,c,d,e,f"
set ft_t0 0.0
proc ft_us {} {
    global ft_t0
    return [format %.0f [expr {([machine_info time] - $ft_t0) * 1e6}]]
}
proc ft_log {what {rest ""}} {
    global ft_ev vk_frame
    puts $ft_ev "$vk_frame,[ft_us],$what,[vk_pc],$rest"
}
proc ft_reg {n} { return [debug read {VDP regs} $n] }
proc ft_sat_simon {} {
    set sat [expr {(([ft_reg 11] & 3) << 15) | (([ft_reg 5] & 0xFC) << 7)}]
    set x [vk_rd 0xD601]; set p [vk_rd 0xD602]
    for {set i 0} {$i < 32} {incr i} {
        set a [expr {$sat + 4 * $i}]
        if {[debug read VRAM [expr {$a + 2}]] == $p && [debug read VRAM [expr {$a + 1}]] == $x} {
            return "[debug read VRAM $a],[debug read VRAM [expr {$a + 1}]],$p"
        }
    }
    # Pas d'entree de meme X : la premiere entree de meme motif.
    for {set i 0} {$i < 32} {incr i} {
        set a [expr {$sat + 4 * $i}]
        if {[debug read VRAM [expr {$a + 2}]] == $p} {
            return "[debug read VRAM $a],[debug read VRAM [expr {$a + 1}]],$p"
        }
    }
    return "-1,-1,-1"
}
proc ft_frame {} {
    global ft_csv vk_frame ft_t0
    set ft_t0 [machine_info time]
    set r {}
    foreach a {C003 C005 C006 C007 C427 C425 C417 C415 C42D C43E} { lappend r [vk_rd 0x$a] }
    set sat [expr {(([ft_reg 11] & 3) << 15) | (([ft_reg 5] & 0xFC) << 7)}]
    binary scan [debug read_block VRAM $sat 128] H* hex
    puts $ft_csv "$vk_frame,[join $r ,],[ft_reg 7],[ft_sat_simon],$hex"
    if {$vk_frame % 200 == 0} { flush $ft_csv; flush $::ft_ev }
}
proc ft_cpu {r} { return [debug read {CPU regs} $r] }
debug set_bp 0x4028 {} {ft_frame}
foreach {pc what} {0x4BBB keys 0x4BB1 keys_done 0x414D c003_inc 0x6560 sat_upload 0x658A col_upload} {
    debug set_bp $pc {} [list ft_log $what]
}
# CHGCLR : bordure F3EB.
debug set_bp 0x0062 {} {ft_log chgclr "[vk_rd 0xF3EB],[ft_reg 7]"}
# 509F : demande de son (A = numero), registre A = octet haut de AF (index 0).
debug set_bp 0x509F {} {ft_log sound_req "[ft_cpu 0]"}
proc ft_write {name} { ft_log "write_$name" $::wp_last_value }
foreach a {C096 C417 C415 C42D} {
    debug set_watchpoint write_mem [list 0x$a] {} [list ft_write $a]
}
rename vk_finish ft_finish_base
proc vk_finish {} {
    global ft_csv ft_ev
    close $ft_csv; close $ft_ev
    ft_finish_base
}
# Depart d'une commande VDP : ecriture indirecte (port 9Bh) vers R#46, valeur >= 40h.
proc ft_cmd {} {
    set v $::wp_last_value
    set r17 [expr {[ft_reg 17] & 63}]
    if {$v < 0x40 || ($r17 != 46 && $r17 != 47)} { return }
    set dx [expr {[ft_reg 36] | ([ft_reg 37] << 8)}]
    set dy [expr {[ft_reg 38] | ([ft_reg 39] << 8)}]
    set nx [expr {[ft_reg 40] | ([ft_reg 41] << 8)}]
    set ny [expr {[ft_reg 42] | ([ft_reg 43] << 8)}]
    ft_log vdp_cmd "[format %02X $v],$dx,$dy,$nx,$ny,$r17"
}
debug set_watchpoint write_io 0x9B {} {ft_cmd}
proc vk_custom_action {w} {
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
    ft_log "item_[lindex $w 1]"
}
