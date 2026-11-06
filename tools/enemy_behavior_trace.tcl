# Trace openMSX du comportement des ennemis du hall (stage 1) : lecture seule.
# Variables attendues (definies par run_enemy_behavior_trace.py) : eb_out (dossier), eb_plan (lignes).
# Actions : hold KEYS MAX LABEL (fin au changement de salle, a l'immobilite de 45 images, ou a MAX)
#           tap KEYS N LABEL (appui exact de N images)
#           wait N | poke ADDR VAL
# KEYS : combinaison de R L U D SP joints par +, ou NONE.
# Sorties : frames.csv (une ligne par image), sat.txt et objC8.txt / objD7.txt (hex, si changement).
# Point d'arret VBLANK 0x4028 (meme ancrage que tools/vertical_links_trace.tcl).
set throttle off
set pause off
set power on

array set eb_km {R 128 D 64 U 32 L 16 SP 1}
set eb_held 0
set eb_frame 0
set eb_csv [open [file join $eb_out frames.csv] w]
puts $eb_csv "frame,time,d000,d001,c000,c420,x,y,c415,c410,c42d,c422,c42f,held,act,label"
set eb_ev [open [file join $eb_out events.txt] w]
set eb_sat [open [file join $eb_out sat.txt] w]
set eb_o8 [open [file join $eb_out objC8.txt] w]
set eb_od7 [open [file join $eb_out objD7.txt] w]
set eb_last_sat ""
set eb_last_o8 ""
set eb_last_od7 ""

set eb_lines [split $eb_plan "\n"]
set eb_idx 0
set eb_act none
set eb_alabel none
set eb_amax 0
set eb_acnt 0
set eb_stuck 0
set eb_lastx -1
set eb_sd0 0
set eb_sd1 0
set eb_gap 0
set eb_title_ticks 0
set eb_title_done 0

proc eb_rd {addr} { return [debug read memory $addr] }
proc eb_hex {base n} {
    set s ""
    for {set i 0} {$i < $n} {incr i} {
        append s [format %02X [debug read memory [expr {$base + $i}]]]
    }
    return $s
}
proc eb_log {msg} {
    global eb_ev eb_frame
    puts $eb_ev "frame=$eb_frame t=[machine_info time] $msg"
    flush $eb_ev
}
proc eb_mask {spec} {
    global eb_km
    if {$spec eq "NONE"} { return 0 }
    set m 0
    foreach k [split $spec +] { incr m $eb_km($k) }
    return $m
}
proc eb_press {mask} {
    global eb_held
    if {$eb_held} { keymatrixup 8 $eb_held; set eb_held 0 }
    if {$mask} { keymatrixdown 8 $mask; set eb_held $mask }
}
proc eb_next {} {
    global eb_lines eb_idx eb_act eb_alabel eb_amax eb_acnt eb_stuck eb_lastx eb_sd0 eb_sd1
    while {$eb_idx < [llength $eb_lines]} {
        set line [string trim [lindex $eb_lines $eb_idx]]
        incr eb_idx
        if {$line eq "" || [string index $line 0] eq "#"} { continue }
        set w [regexp -all -inline {\S+} $line]
        set kind [lindex $w 0]
        set eb_act $kind
        set eb_acnt 0
        set eb_stuck 0
        set eb_lastx -1
        set eb_sd0 [eb_rd 0xD000]
        set eb_sd1 [eb_rd 0xD001]
        switch -- $kind {
            hold {
                set eb_amax [lindex $w 2]; set eb_alabel [lindex $w 3]
                eb_press [eb_mask [lindex $w 1]]
                eb_log "BEGIN hold [lindex $w 1] $eb_alabel"
                return
            }
            tap {
                set eb_amax [lindex $w 2]; set eb_alabel [lindex $w 3]
                eb_press [eb_mask [lindex $w 1]]
                eb_log "BEGIN tap [lindex $w 1] $eb_alabel"
                return
            }
            wait {
                set eb_amax [lindex $w 1]; set eb_alabel wait
                eb_press 0
                eb_log "BEGIN wait $eb_amax"
                return
            }
            poke {
                eb_press 0
                debug write memory [lindex $w 1] [lindex $w 2]
                eb_log "POKE [lindex $w 1]=[lindex $w 2] (readback [eb_rd [lindex $w 1]])"
            }
            default {
                eb_log "UNKNOWN $line"
            }
        }
    }
    set eb_act done
}
proc eb_dump_changes {} {
    global eb_frame eb_sat eb_o8 eb_od7 eb_last_sat eb_last_o8 eb_last_od7
    set s [eb_hex 0xD600 128]
    if {$s ne $eb_last_sat} { puts $eb_sat "$eb_frame $s"; set eb_last_sat $s }
    set s [eb_hex 0xC800 256]
    if {$s ne $eb_last_o8} { puts $eb_o8 "$eb_frame $s"; set eb_last_o8 $s }
    set s [eb_hex 0xD700 256]
    if {$s ne $eb_last_od7} { puts $eb_od7 "$eb_frame $s"; set eb_last_od7 $s }
}
proc eb_frame_hook {} {
    global eb_csv eb_frame eb_act eb_alabel eb_amax eb_acnt eb_stuck eb_lastx eb_sd0 eb_sd1 eb_gap eb_held eb_title_ticks eb_title_done
    incr eb_frame
    if {$eb_frame > 60000} { eb_log "abort frame limit"; eb_finish }
    set d0 [eb_rd 0xD000]; set d1 [eb_rd 0xD001]; set c0 [eb_rd 0xC000]
    set x [eb_rd 0xC427]; set y [eb_rd 0xC425]; set c420 [eb_rd 0xC420]
    set c415 [eb_rd 0xC415]; set c410 [eb_rd 0xC410]; set c42d [eb_rd 0xC42D]
    set c422 [eb_rd 0xC422]; set c42f [eb_rd 0xC42F]
    set pre "$eb_frame,[machine_info time],$d0,$d1,$c0,$c420,$x,$y,$c415,$c410,$c42d,$c422,$c42f,$eb_held"
    if {!$eb_title_done} {
        if {$c0 == 5} {
            set eb_title_done 1
            eb_log "gameplay C000=5 D000=$d0 D001=$d1 x=$x y=$y"
            eb_press 0
            eb_next
        } else {
            incr eb_title_ticks
            if {$eb_title_ticks % 120 == 0} { eb_press 1 }
            if {$eb_title_ticks % 120 == 3} { eb_press 0 }
            if {$eb_title_ticks > 40000} { eb_log "title timeout"; eb_finish }
            puts $eb_csv "$pre,-,title"
            return
        }
    }
    eb_dump_changes
    if {$eb_act eq "done"} {
        puts $eb_csv "$pre,done,"
        eb_finish
    }
    if {$eb_gap > 0} {
        incr eb_gap -1
        puts $eb_csv "$pre,$eb_act,$eb_alabel"
        if {$eb_gap == 0} { eb_next }
        return
    }
    incr eb_acnt
    if {$x == $eb_lastx} { incr eb_stuck } else { set eb_stuck 0 }
    set eb_lastx $x
    set end 0; set res ""
    switch -- $eb_act {
        hold {
            if {$d0 != $eb_sd0 || $d1 != $eb_sd1} {
                set end 1; set res "CHANGE $eb_sd0/$eb_sd1 -> $d0/$d1 x=$x y=$y"
            } else {
                if {$eb_stuck >= 45} {
                    set end 1; set res "STUCK x=$x y=$y"
                } else {
                    if {$eb_acnt >= $eb_amax} {
                        set end 1; set res "TIMEOUT x=$x y=$y"
                    }
                }
            }
        }
        tap { if {$eb_acnt >= $eb_amax} { set end 1; set res "TAPDONE" } }
        wait { if {$eb_acnt >= $eb_amax} { set end 1; set res "WAITED x=$x y=$y" } }
        default { set end 1; set res "" }
    }
    puts $eb_csv "$pre,$eb_act,$eb_alabel"
    if {$end} {
        eb_log "[string toupper $eb_alabel] $res"
        eb_press 0
        set eb_gap 10
    }
    if {$eb_frame % 300 == 0} { flush $eb_csv }
}
proc eb_finish {} {
    global eb_csv eb_ev eb_sat eb_o8 eb_od7 eb_out eb_frame eb_held
    eb_press 0
    close $eb_csv; close $eb_ev; close $eb_sat; close $eb_o8; close $eb_od7
    set f [open [file join $eb_out done.txt] w]; puts $f "ok $eb_frame"; close $f
    exit
}

debug set_bp 0x4028 {} {eb_frame_hook}
