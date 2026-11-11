# Species alive in C800/D700 in each forced room of stages 12-18 (run by tools/run_species_trace.py).
# Read-only except room forcing (D000, D002, D001, C000=4), energy C415 and lives C410. Variables: sp_out, sp_rooms, sp_frames, sp_walk.
set throttle off
set pause off
set power on
array set sp_km {R 128 L 16}
set sp_held 0
set sp_f 0
set sp_idx 0
set sp_phase title
set sp_cnt 0
set sp_seen [dict create]
set sp_log [open [file join $sp_out species.txt] w]
set sp_title 0
set sp_tot 0
set sp_rf 0
set sp_wait 0
set sp_l 0
proc sp_rd {a} { return [debug read memory $a] }
proc sp_press {m} {
    global sp_held
    if {$sp_held} { keymatrixup 8 $sp_held; set sp_held 0 }
    if {$m} { keymatrixdown 8 $m; set sp_held $m }
}
proc sp_force {} {
    global sp_rooms sp_idx
    set r [lindex $sp_rooms $sp_idx]
    debug write memory 0xD000 [lindex $r 0]
    debug write memory 0xD002 [lindex $r 2]
    debug write memory 0xD001 [lindex $r 1]
    for {set a 0} {$a < 896} {incr a} { debug write memory [expr {0xC800 + $a}] 0 }
    for {set a 0} {$a < 1024} {incr a} { debug write memory [expr {0xD700 + $a}] 0 }
    debug write memory 0xC000 4
}
proc sp_hook {} {
    global sp_wait sp_rf sp_l sp_tot sp_f sp_idx sp_phase sp_cnt sp_seen sp_log sp_title sp_rooms sp_frames sp_walk sp_out
    incr sp_f
    if {$sp_f % 60 == 0} { flush $sp_log }
    set c0 [sp_rd 0xC000]
    if {$sp_phase eq "title"} {
        if {$c0 == 5} {
            set sp_phase load; set sp_l 0; sp_press 0; sp_force; set sp_cnt 0
        } else {
            incr sp_title
            if {$sp_title % 120 == 0} { sp_press 1 }
            if {$sp_title % 120 == 3} { sp_press 0 }
            if {$sp_title > 40000} { puts $sp_log "TITLE TIMEOUT"; close $sp_log; exit }
        }
        return
    }
    debug write memory 0xC415 32
    debug write memory 0xC410 5
    if {$sp_phase eq "load"} {
        incr sp_l
        if {$c0 == 5 && $sp_l > 3} { set sp_phase run } elseif {$sp_l > 600} { puts $sp_log "LOADFAIL [lindex $sp_rooms $sp_idx]"; set sp_phase run }
        if {$sp_phase eq "load"} { return }
        if {[info exists ::sp_x]} { debug write memory 0xC427 $::sp_x }
    }
    incr sp_cnt; incr sp_tot
    if {$sp_walk} {
        if {($sp_cnt / 24) % 2 == 0} { sp_press 128 } else { sp_press 16 }
    }
    if {$sp_tot >= $sp_frames && ($c0 == 5 || $sp_tot > $sp_frames + 600)} {
        flush $sp_log; puts $sp_log "ENDROOM [lindex $sp_rooms $sp_idx] f=$sp_f"
        incr sp_idx
        if {$sp_idx >= [llength $sp_rooms]} {
            sp_press 0; close $sp_log
            set f [open [file join $sp_out done.txt] w]; puts $f ok; close $f
            exit
        }
        set sp_phase load; set sp_l 0; set sp_cnt 0; set sp_tot 0; sp_press 0; sp_force; return
    }
    set st [sp_rd 0xD000]; set rm [sp_rd 0xD001]
    if {[info exists ::sp_dbg] && $sp_tot % 60 == 0} {
        puts $sp_log "DBG $st $rm t=$sp_tot x=[sp_rd 0xC427] y=[sp_rd 0xC425] c420=[sp_rd 0xC420] c440=[sp_rd 0xC440] c5ac=[sp_rd 0xC5AC] cf0c=[sp_rd 0xCF0C] cf0d=[sp_rd 0xCF0D] cf10=[sp_rd 0xCF10]"
    }
    set fr [lindex $sp_rooms $sp_idx]
    if {$st != [lindex $fr 0] || $rm != [lindex $fr 1]} {
        # Simon left (or died out of) the forced room: force it again, the frame budget goes on.
        # Forcing C000 in the middle of a room change (state 9) crashes the Z80: wait for gameplay (5).
        if {$c0 != 5 && $sp_wait < 600} { incr sp_wait; return }
        set sp_wait 0
        incr sp_rf; if {$sp_rf < 30} { puts $sp_log "REFORCE [lindex $sp_rooms $sp_idx] now $st $rm c0=$c0 tot=$sp_tot cnt=$sp_cnt f=$sp_f"; flush $sp_log }
        set sp_phase load; set sp_l 0; set sp_cnt 0; sp_press 0; sp_force
        return
    }
    for {set i 0} {$i < 7} {incr i} {
        set t [sp_rd [expr {0xC800 + 128*$i}]]
        if {$t} {
            set pk "POSE $st $rm [expr {$t & 127}] [sp_rd [expr {0xC80B + 128*$i}]]"
            if {![dict exists $sp_seen $pk]} { dict set sp_seen $pk 1; puts $sp_log "$pk x=[sp_rd [expr {0xC800 + 128*$i + 4}]] y=[sp_rd [expr {0xC800 + 128*$i + 2}]] frame=$sp_tot" }
            set k "$st $rm C [expr {$t & 127}]"
            if {![dict exists $sp_seen $k]} { dict set sp_seen $k 1; puts $sp_log "$k frame=$sp_tot raw=$t" }
        }
    }
    for {set i 0} {$i < 8} {incr i} {
        set t [sp_rd [expr {0xD700 + 128*$i}]]
        if {$t} {
            set k "$st $rm D $t"
            if {![dict exists $sp_seen $k]} { dict set sp_seen $k 1; puts $sp_log "$k frame=$sp_tot" }
        }
    }
}
debug set_bp 0x4028 {} {sp_hook}
# Watchdog (emulated time): the VBLANK hook should advance every few frames; log when it does not.
set sp_wf -1
proc sp_watch {} {
    global sp_f sp_wf sp_log sp_idx sp_rooms
    if {$sp_f == $sp_wf} {
        puts $sp_log "STALL room=[lindex $sp_rooms $sp_idx] f=$sp_f pc=[format %04X [reg PC]] c000=[debug read memory 0xC000] d000=[debug read memory 0xD000] d001=[debug read memory 0xD001] c420=[debug read memory 0xC420] c410=[debug read memory 0xC410] c415=[debug read memory 0xC415]"
        flush $sp_log
    }
    set sp_wf $sp_f
    after time 4 sp_watch
}
after time 4 sp_watch
