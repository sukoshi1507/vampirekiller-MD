# Durations of the room change (state 9) for every arrival room: to append after
# tools/vertical_links_trace.tcl; run and analysed by tools/run_room_freeze_trace.py.
# Plan actions: "stage S" (D000 := S - 1, C408 := 1: the stage exit of state 10 loads
# stage S as the door does) and "trans R" (C5B4 := R, C41B := FF: B963 then writes
# D001 := C5B4 (B98C) instead of reading the link table, and state 9 loads room R as for
# any room change; energy and lives are refilled so that the run goes on). rf_events.csv: frame of each 47D4 (display off) and 47C7 (on).
proc key_snap {label} {}
set rf_ev [open [file join $vk_out rf_events.csv] w]
puts $rf_ev "frame,what"
debug set_bp 0x47D4 {} {puts $rf_ev "$vk_frame,off"}
debug set_bp 0x47C7 {} {puts $rf_ev "$vk_frame,on"}
proc vk_custom_action {w} {
    set kind [lindex $w 0]
    debug write memory 0xC415 32
    debug write memory 0xC410 5
    if {$kind eq "trans"} {
        debug write memory 0xC5B4 [lindex $w 1]
        debug write memory 0xC41B 255
    } elseif {$kind eq "stage"} {
        debug write memory 0xD000 [expr {[lindex $w 1] - 1}]
        debug write memory 0xC408 1
    }
}
rename vk_finish rf_finish_base
proc vk_finish {} {
    global rf_ev
    close $rf_ev
    rf_finish_base
}
