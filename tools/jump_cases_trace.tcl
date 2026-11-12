# openMSX: Simon's jump per game pass in forced rooms (run by tools/run_jump_cases.py).
# Variables set by the runner: jc_out, jc_cases = list of {name stage room x y dirmask high walk rom1E6A}.
# For each case: force the room (D000, D002, D001, C800.. cleared, C000 = 4), settle, hold the direction for `walk` passes,
# then (X, Y) are written to C427/C425 and UP (32, the MSX jump key) is held 4 passes; the direction stays held 120 passes.
# Per hook of 4028: case, n, time, C003, C420, C421, C425, C427, C428, C42B, C42E -> OUT/jump.txt
set throttle off
set pause off
set power on
set jc_held 0
set jc_f 0
set jc_idx 0
set jc_phase title
set jc_cnt 0
set jc_l 0
set jc_title 0
set jc_log [open [file join $jc_out jump.txt] w]
proc jc_rd {a} { return [debug read memory $a] }
proc jc_press {m} {
    global jc_held
    if {$jc_held} { keymatrixup 8 $jc_held; set jc_held 0 }
    if {$m} { keymatrixdown 8 $m; set jc_held $m }
}
proc jc_force {} {
    global jc_cases jc_idx
    set c [lindex $jc_cases $jc_idx]
    debug write memory 0xD000 [lindex $c 1]
    debug write memory 0xD002 [lindex $c 8]
    debug write memory 0xD001 [lindex $c 2]
    for {set a 0} {$a < 896} {incr a} { debug write memory [expr {0xC800 + $a}] 0 }
    for {set a 0} {$a < 1024} {incr a} { debug write memory [expr {0xD700 + $a}] 0 }
    debug write memory 0xC000 4
}
proc jc_hook {} {
    global jc_f jc_idx jc_phase jc_cnt jc_l jc_title jc_cases jc_log
    incr jc_f
    set c0 [jc_rd 0xC000]
    if {$jc_phase eq "title"} {
        if {$c0 == 5} {
            set jc_phase load; set jc_l 0; jc_press 0; jc_force
        } else {
            incr jc_title
            if {$jc_title % 120 == 0} { jc_press 1 }
            if {$jc_title % 120 == 3} { jc_press 0 }
            if {$jc_title > 40000} { puts $jc_log "TITLE TIMEOUT"; close $jc_log; exit }
        }
        return
    }
    debug write memory 0xC415 32
    debug write memory 0xC410 5
    set c [lindex $jc_cases $jc_idx]
    lassign $c name stage room x y dir high walk rom1e6a frames
    if {$jc_phase eq "load"} {
        incr jc_l
        if {$c0 == 5 && $jc_l > 3} { set jc_phase settle; set jc_cnt 0 } elseif {$jc_l > 600} { puts $jc_log "LOADFAIL $name"; set jc_phase settle; set jc_cnt 0 }
        return
    }
    incr jc_cnt
    if {$jc_phase eq "settle"} {
        for {set a 0} {$a < 896} {incr a} { debug write memory [expr {0xC800 + $a}] 0 }
        if {$jc_cnt == 30} { debug write memory 0xC427 $x; debug write memory 0xC425 $y; debug write memory 0xC431 [expr {$high == 1 ? 16 : 0}] }
        if {$jc_cnt == 40 && $dir != 32 && $dir != 64} { jc_press $dir }
        if {$jc_cnt == 40 + $walk} {
            debug write memory 0xC427 $x; debug write memory 0xC425 $y
            jc_press [expr {($dir == 64 || $high == 2) ? $dir : ($dir | 32)}]
            set jc_phase run; set jc_cnt 0
        }
        return
    }
    # run: n = 0 is the pass that holds the first jump key
    debug write memory 0xC431 [expr {$high == 1 ? 16 : 0}]
    for {set a 0} {$a < 896} {incr a} { debug write memory [expr {0xC800 + $a}] 0 }
    puts $jc_log [format "%s %d t=%.4f c003=%d c420=%d c421=%d y=%d x=%d c428=%d c42b=%d c42e=%d c006=%d" $name $jc_cnt [machine_info time] [jc_rd 0xC003] [jc_rd 0xC420] [jc_rd 0xC421] [jc_rd 0xC425] [jc_rd 0xC427] [jc_rd 0xC428] [jc_rd 0xC42B] [jc_rd 0xC42E] [jc_rd 0xC006]]
    if {$jc_cnt == 4} { jc_press $dir }
    if {$jc_cnt >= $frames} {
        incr jc_idx
        if {$jc_idx >= [llength $jc_cases]} { jc_press 0; close $jc_log; set f [open [file join $::jc_out done.txt] w]; puts $f ok; close $f; exit }
        jc_press 0
        set jc_phase load; set jc_l 0; jc_force
    }
}
debug set_bp 0x4028 {} {jc_hook}
