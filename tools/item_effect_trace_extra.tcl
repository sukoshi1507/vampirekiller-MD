# Trace de l'effet d'un objet ramasse (8D30 et sa table 8D3E) sur les ennemis, a ajouter
# apres tools/vertical_links_trace.tcl et tools/simon_blink_trace_extra.tcl (lance par
# tools/run_item_effect_trace.py). Sert a trancher l'effet des livres 10 et 11
# (research/castle-candles.md 9.8) contre l'objet 05 (7806).
# Sorties :
#   items.csv : une ligne par VBLANK (crochet 4028) : salle, Simon, C003, les 7 blocs C800
#     (+0), les 8 blocs D700 (+0), les 8 objets C500 (+0 etat, +1 Y, +2 X, +4 objet), le
#     score C405-C407 (BCD), C43E (eclair), C702, C701, C419 (dernier objet), C417 (coeurs),
#     C415 (energie).
#   items_ev.csv : PC remarquables avec le registre A : 8D30 (objet pris), 8E1D (objet 10),
#     8E26 (objet 11), 8DBB (objet 05), 7806 (destruction de tous les ennemis), 9A3A
#     (destruction d'un bloc C800), 9A1A (d'un bloc D700), 81AB (destruction avec points),
#     509F (demande de son, A = numero), 9418 (prix du marchand), 44EC (points, DE).
#   Le ramassage (8D30) demande 60 instantanes VRAM (b_pre de simon_blink_trace_extra.tcl) ;
#   l'action « snap LABEL » prend un instantane (rendu : tools/analyze_simon_blink.py
#   render_crop, ou tout rendu SCREEN 5 des fichiers shots/fNNNNN.vram/.txt).
proc key_snap {label} {
    vk_log_ev "SNAP $label"
    b_snap
}
set it_csv [open [file join $vk_out items.csv] w]
set it_head "frame,d000,d001,x,y,c003"
for {set i 0} {$i < 7} {incr i} { append it_head ",e$i" }
for {set i 0} {$i < 8} {incr i} { append it_head ",p$i" }
for {set i 0} {$i < 8} {incr i} { append it_head ",o${i}s,o${i}y,o${i}x,o${i}k" }
append it_head ",c405,c406,c407,c43e,c702,c701,c419,c417,c415,c422,c006"
puts $it_csv $it_head
proc it_frame {} {
    global it_csv vk_frame
    set r {}
    foreach a {D000 D001 C427 C425 C003} { lappend r [vk_rd 0x$a] }
    for {set i 0} {$i < 7} {incr i} { lappend r [vk_rd [expr {0xC800 + 0x80 * $i}]] }
    for {set i 0} {$i < 8} {incr i} { lappend r [vk_rd [expr {0xD700 + 0x80 * $i}]] }
    for {set i 0} {$i < 8} {incr i} {
        set b [expr {0xC500 + 16 * $i}]
        foreach o {0 1 2 4} { lappend r [vk_rd [expr {$b + $o}]] }
    }
    foreach a {C405 C406 C407 C43E C702 C701 C419 C417 C415 C422 C006} { lappend r [vk_rd 0x$a] }
    puts $it_csv "$vk_frame,[join $r ,]"
    if {$vk_frame % 200 == 0} { flush $it_csv }
}
debug set_bp 0x4028 {} {it_frame}
set it_ev [open [file join $vk_out items_ev.csv] w]
puts $it_ev "frame,what,pc,a,de,ix,c702,c43e,c405,c406,c407"
proc it_log {what} {
    global it_ev vk_frame
    set a [debug read {CPU regs} 0]
    set de [format %02X%02X [debug read {CPU regs} 4] [debug read {CPU regs} 5]]
    set ix [format %02X%02X [debug read {CPU regs} 16] [debug read {CPU regs} 17]]
    puts $it_ev "$vk_frame,$what,[vk_pc],[format %02X $a],$de,$ix,[vk_rd 0xC702],[vk_rd 0xC43E],[vk_rd 0xC405],[vk_rd 0xC406],[vk_rd 0xC407]"
    flush $it_ev
}
proc it_pick {} {
    global b_pre
    it_log pick_8D30
    set b_pre 60
}
debug set_bp 0x8D30 {} {it_pick}
foreach {pc what} {0x8E1D obj10_8E1D 0x8E26 obj11_8E26 0x8DBB obj05_8DBB 0x7806 wipe_7806
                   0x9A3A kill_C800_9A3A 0x9A1A kill_D700_9A1A 0x81AB kill_pts_81AB
                   0x509F sound_509F 0x9418 price_9418 0x44EC points_44EC} {
    debug set_bp $pc {} [list it_log $what]
}
# vk_finish de vertical_links_trace.tcl, avec la fermeture de items.csv et items_ev.csv.
proc vk_finish {} {
    global vk_csv vk_ev vk_wcsv vk_out it_csv it_ev b_csv b_ev
    vk_press 0
    close $vk_csv; close $vk_ev; close $vk_wcsv
    close $it_csv; close $it_ev; flush $b_csv; flush $b_ev
    set f [open [file join $vk_out done.txt] w]; puts $f ok; close $f
    exit
}
