# Trace du recul au bord d'une salle et des poses d'escalier (a ajouter apres
# tools/vertical_links_trace.tcl ; lance par tools/run_stair_recoil_trace.py).
# state.csv : etat de Simon par VBLANK (C420-C42F, C435, C43C, X, Y, D000/D001, touches).
# wr.csv : ecritures de C000, C420, C421, C423, C42A, C42B, C42E, C42F, C427, C43C avec le PC.
# Coups forces : VK_TRIG (env) est une liste Tcl de {C42B mortel}. Au VBLANK ou Simon est sur
# l'escalier (C420 = 3) avec ce C42B, C420 := 5 (comme le contact 7EC3) ; mortel met d'abord
# C415 := 0. Les declencheurs sont pris dans l'ordre.
proc key_snap {label} {}
set t_csv [open [file join $vk_out state.csv] w]
puts $t_csv "frame,c000,d000,d001,x,y,c426,c420,c421,c423,c42a,c42b,c42c,c42e,c42f,c415,c42d,c43c,c435,held"
proc t_frame {} {
    global t_csv vk_frame vk_held
    set r {}
    foreach a {C000 D000 D001 C427 C425 C426 C420 C421 C423 C42A C42B C42C C42E C42F C415 C42D C43C C435} { lappend r [vk_rd 0x$a] }
    puts $t_csv "$vk_frame,[join $r ,],$vk_held"
    if {$vk_frame % 200 == 0} { flush $t_csv }
}
debug set_bp 0x4028 {} {t_frame}
set t_w [open [file join $vk_out wr.csv] w]
puts $t_w "frame,pc,addr,val,c000,d001,x,y,c420,c423,c42a,c42b"
proc t_wlog {a} {
    global t_w vk_frame
    puts $t_w "$vk_frame,[vk_pc],$a,[vk_rd 0x$a],[vk_rd 0xC000],[vk_rd 0xD001],[vk_rd 0xC427],[vk_rd 0xC425],[vk_rd 0xC420],[vk_rd 0xC423],[vk_rd 0xC42A],[vk_rd 0xC42B]"
    flush $t_w
}
foreach a {C000 C420 C421 C423 C42A C42B C42E C42F C427 C43C} {
    debug set_watchpoint write_mem [list 0x$a] {} [list t_wlog $a]
}
set t_trig [expr {[info exists ::env(VK_TRIG)] ? $::env(VK_TRIG) : {}}]
proc t_hit {} {
    global t_trig
    if {[llength $t_trig] == 0} { return }
    set tr [lindex $t_trig 0]
    if {[vk_rd 0xC420] != 3 || [vk_rd 0xC42B] != [lindex $tr 0]} { return }
    if {[lindex $tr 1]} { debug write memory 0xC415 0 }
    debug write memory 0xC420 5
    vk_log_ev "TRIG hit c42b=[lindex $tr 0] lethal=[lindex $tr 1] x=[vk_rd 0xC427] y=[vk_rd 0xC425] c42e=[vk_rd 0xC42E] c42f=[vk_rd 0xC42F]"
    set t_trig [lrange $t_trig 1 end]
}
debug set_bp 0x4028 {} {t_hit}
