# MSX trace of the Dracula fight (stage 18 room 9): Simon is driven only through the keyboard matrix (walk, jump UP, whip SPACE).
# Pokes: D000/D001/D002 (room forcing, as species_trace.tcl), C415/C410 (Simon energy, lives) and, if dm_boss4 = 1, C418 = 4
# once at the start to reach the second form quickly. Logs one line per game pass to dm_out/dracula.txt.
set throttle off
set pause off
set power on
set dm_log [open [file join $dm_out dracula.txt] w]
set dm_phase title
set dm_f 0
set dm_cnt 0
set dm_held 0
set dm_title 0
proc dm_rd {a} { return [debug read memory $a] }
proc dm_press {m} {
    global dm_held
    if {$dm_held} { keymatrixup 8 $dm_held; set dm_held 0 }
    if {$m} { keymatrixdown 8 $m; set dm_held $m }
}
proc dm_hook {} {
    global dm_f dm_phase dm_cnt dm_title dm_log dm_boss4 dm_out dm_frames dm_cycle
    incr dm_f
    set c0 [dm_rd 0xC000]
    if {$dm_phase eq "title"} {
        if {$c0 == 5} {
            dm_press 0
            debug write memory 0xD000 18
            debug write memory 0xD002 $::dm_d002
            debug write memory 0xD001 $::dm_room
            for {set a 0} {$a < 896} {incr a} { debug write memory [expr {0xC800 + $a}] 0 }
            for {set a 0} {$a < 1024} {incr a} { debug write memory [expr {0xD700 + $a}] 0 }
            debug write memory 0xC000 4
            set dm_phase load; set dm_cnt 0
        } else {
            incr dm_title
            if {$dm_title % 120 == 0} { dm_press 1 }
            if {$dm_title % 120 == 3} { dm_press 0 }
            if {$dm_title > 40000} { puts $dm_log TITLE_TIMEOUT; close $dm_log; exit }
        }
        return
    }
    debug write memory 0xC415 32
    debug write memory 0xC410 5
    if {$dm_phase eq "load"} {
        incr dm_cnt
        if {$c0 == 5 && $dm_cnt > 3} {
            set dm_phase run; set dm_cnt 0
            if {[info exists ::dm_x]} { debug write memory 0xC427 $::dm_x }
            if {$dm_boss4 == 1} { debug write memory 0xC418 4 }
            if {$dm_boss4 == 2} { debug write memory 0xCE16 1 }
        } elseif {$dm_cnt > 600} { puts $dm_log LOADFAIL; close $dm_log; exit }
        return
    }
    incr dm_cnt
    if {$dm_boss4 == 2 && $dm_cnt == 200} { debug write memory 0xCE16 1 }
    set n [expr {$dm_cnt % $dm_cycle}]
    set dir [expr {(($dm_cnt / $dm_cycle) % 2) == 0 ? 128 : 16}]
    set w $::dm_walk
    if {$n < $w} { dm_press $dir } elseif {$n < $w+4} { dm_press [expr {32 | $dir}] } elseif {$n < $w+6} { dm_press $dir } elseif {$n < $w+30} { dm_press [expr {1 | $dir}] } else { dm_press 0 }
    if {$dm_cnt % 3 == 0} {
        puts $dm_log "t=$dm_cnt c420=[dm_rd 0xC420] y=[dm_rd 0xC425] x=[dm_rd 0xC427] c418=[dm_rd 0xC418] ce00=[dm_rd 0xCE00] ce01=[dm_rd 0xCE01] o0=[dm_rd 0xC800] o0y=[dm_rd 0xC803] o0x=[dm_rd 0xC805] o0f=[dm_rd 0xC80E] c422=[dm_rd 0xC422]"
    }
    if {$dm_cnt == 100} {
        set fd [open [file join $dm_out ram.bin] wb]
        fconfigure $fd -translation binary
        puts -nonewline $fd [debug read_block memory 0xC000 0x4000]
        close $fd
        set fd [open [file join $dm_out cpu.bin] wb]
        fconfigure $fd -translation binary
        puts -nonewline $fd [debug read_block memory 0x0000 0x10000]
        close $fd
    }
    if {$dm_cnt >= $dm_frames} { close $dm_log; exit }
}
debug set_bp 0x4028 {} {dm_hook}
