# Trace de l'objet 0A (arret du temps), a ajouter apres tools/vertical_links_trace.tcl
# (lance par tools/run_time_stop_trace.py).
# ts.csv : par VBLANK, C006/C007 (touches nouvelles / tenues), C420, C417 (coeurs, BCD),
# C701, C43B, D010, C098 (pilote son : bit 1 = FB), C415 (energie), C42D, puis pour les 7 blocs C800 :
# +0 (type), +3 (X), +5 (Y), +0B (pose), +7E (figeable).
# tsw.csv : ecritures de C43B, D010, C417, C098 avec le PC.
proc key_snap {label} {}
set ts_csv [open [file join $vk_out ts.csv] w]
set ts_head "frame,d000,d001,x,y,c006,c007,c420,c417,c701,c43b,d010,c098,c415,c42d"
for {set i 0} {$i < 7} {incr i} { append ts_head ",e${i}t,e${i}x,e${i}y,e${i}p,e${i}f" }
puts $ts_csv $ts_head
proc ts_frame {} {
    global ts_csv vk_frame
    set r {}
    foreach a {D000 D001 C427 C425 C006 C007 C420 C417 C701 C43B D010 C098 C415 C42D} { lappend r [vk_rd 0x$a] }
    for {set i 0} {$i < 7} {incr i} {
        set b [expr {0xC800 + 0x80 * $i}]
        foreach o {0 3 5 11 126} { lappend r [vk_rd [expr {$b + $o}]] }
    }
    puts $ts_csv "$vk_frame,[join $r ,]"
    flush $ts_csv
}
debug set_bp 0x4028 {} {ts_frame}
set ts_w [open [file join $vk_out tsw.csv] w]
puts $ts_w "frame,pc,addr,val,c420,c417,c43b,d010"
proc ts_wlog {a} {
    global ts_w vk_frame
    puts $ts_w "$vk_frame,[vk_pc],$a,[vk_rd 0x$a],[vk_rd 0xC420],[vk_rd 0xC417],[vk_rd 0xC43B],[vk_rd 0xD010]"
    flush $ts_w
}
foreach a {C43B D010 C417 C098} {
    debug set_watchpoint write_mem [list 0x$a] {} [list ts_wlog $a]
}
