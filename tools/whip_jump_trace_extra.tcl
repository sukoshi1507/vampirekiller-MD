# Trace du fouet en plein saut (research/whip-hitbox.md, section « Fouet en saut ») :
# quelles coordonnees de Simon le fouet utilise, pour l'affichage et pour la collision.
# A ajouter apres tools/vertical_links_trace.tcl ; lance par tools/run_whip_jump_trace.py.
# Sorties :
#   wj_frames.csv : une ligne par VBLANK (crochet 4028) : C003, C005, C420, C422, C429,
#     C42C, C42E, C42F, X/Y de Simon (C427/C425), le tampon SAT D600 (32 entrees, hexa,
#     rempli par la passe), la SAT en VRAM (celle de l'image precedente) et les bougies
#     C470-C48F (deux entrees de 16 octets ; +3 compte les coups du fouet, 7FD2).
#   wj_events.csv : instant (us depuis le VBLANK) et PC de : 84D1 (lecture de C425 par la
#     boite du fouet 84A7), 84E7 (lecture de C427 par 84D9), 7DAD / 7F49 / 7FB7 (tests du
#     fouet : objets C800, D700, bougies C470), 6560 (envoi SAT), ecritures de C425, C427,
#     C422 et C42F ; avec C425, C427, C420, C42C, C422, D (X cible), E (Y cible) et IX.
# Action de plan en plus : « whipat Y up|down » arme un appui ESPACE (2 images) au
# VBLANK qui ouvre une passe (C005 = 0) pendant un saut (C420 = 1) quand C425 = Y, a la
# montee (C428 < 9) ou a la descente : la passe lit la touche (4BBB) puis fait le pas
# de saut suivant (6D5D) avant de lancer le coup (7133). Plusieurs triggers possibles.
set wj_csv [open [file join $vk_out wj_frames.csv] w]
puts $wj_csv "frame,c003,c005,c420,c422,c429,c42c,c42e,c42f,x,y,d600,vsat,c470"
set wj_ev [open [file join $vk_out wj_events.csv] w]
puts $wj_ev "frame,us,what,pc,c425,c427,c420,c42c,c422,d,e,ix"
set wj_t0 0.0
proc wj_reg {n} { return [debug read {CPU regs} $n] }
proc wj_log {what} {
    global wj_ev vk_frame wj_t0
    set us [format %.0f [expr {([machine_info time] - $wj_t0) * 1e6}]]
    set ix [format %02X%02X [wj_reg 16] [wj_reg 17]]
    puts $wj_ev "$vk_frame,$us,$what,[vk_pc],[vk_rd 0xC425],[vk_rd 0xC427],[vk_rd 0xC420],[vk_rd 0xC42C],[vk_rd 0xC422],[wj_reg 4],[wj_reg 5],$ix"
}
proc wj_frame {} {
    global wj_csv vk_frame wj_t0
    set wj_t0 [machine_info time]
    set r {}
    foreach a {C003 C005 C420 C422 C429 C42C C42E C42F C427 C425} { lappend r [vk_rd 0x$a] }
    binary scan [debug read_block memory 0xD600 128] H* shadow
    set sat [expr {(([debug read {VDP regs} 11] & 3) << 15) | (([debug read {VDP regs} 5] & 0xFC) << 7)}]
    binary scan [debug read_block VRAM $sat 128] H* hex
    binary scan [debug read_block memory 0xC470 32] H* cand
    puts $wj_csv "$vk_frame,[join $r ,],$shadow,$hex,$cand"
    wj_triggers
    if {$vk_frame % 200 == 0} { flush $wj_csv; flush $::wj_ev }
}
set wj_trig {}
set wj_release 0
proc wj_triggers {} {
    global wj_trig wj_release
    if {$wj_release > 0} {
        incr wj_release -1
        if {$wj_release == 0} { keymatrixup 8 1; wj_note "release" }
    }
    if {![llength $wj_trig] || [vk_rd 0xC005] != 0 || [vk_rd 0xC420] != 1} { return }
    set t [lindex $wj_trig 0]
    set up [expr {[vk_rd 0xC428] < 9}]
    if {[vk_rd 0xC425] != [lindex $t 0]} { return }
    if {([lindex $t 1] eq "up") != $up} { return }
    keymatrixdown 8 1
    set wj_release 2
    set wj_trig [lrange $wj_trig 1 end]
    wj_note "press y=[vk_rd 0xC425] x=[vk_rd 0xC427] c428=[vk_rd 0xC428]"
}
proc wj_note {msg} { vk_log_ev "WHIPAT $msg" }
proc vk_custom_action {w} {
    global wj_trig
    if {[lindex $w 0] ne "whipat"} { return }
    lappend wj_trig [list [lindex $w 1] [lindex $w 2]]
    vk_log_ev "WHIPAT armed [lindex $w 1] [lindex $w 2]"
}
debug set_bp 0x4028 {} {wj_frame}
foreach {pc what} {0x84D1 box_y 0x84E7 box_x 0x7DAD test_c800 0x7F49 test_d700 0x7FB7 test_c470 0x6560 sat_upload} {
    debug set_bp $pc {} [list wj_log $what]
}
foreach a {C425 C427 C422 C42F} {
    debug set_watchpoint write_mem [list 0x$a] {} [list wj_log "write_$a"]
}
rename vk_finish wj_finish_base
proc vk_finish {} {
    global wj_csv wj_ev
    close $wj_csv; close $wj_ev
    wj_finish_base
}
