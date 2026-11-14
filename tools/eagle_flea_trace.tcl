# openMSX : aigle (0F) et puce (23h) du stage 11 dans une salle forcee, Simon cloue a (ef_x, ef_y), une ligne par image (run par tools/run_eagle_flea_trace.py).
# Variables : ef_out, ef_room, ef_d2, ef_x, ef_y, ef_frames, ef_whip (periode du fouet, 0 = aucun), ef_keep (1 = Simon reste cloue apres un coup), c415.
set throttle off
set pause off
set power on
set f 0
set ph title
set tt 0
set held 0
set n 0
set lg [open [file join $ef_out log.txt] w]
set pinned 1
proc rd {a} { return [debug read memory $a] }
proc press {m} { global held; if {$held} { keymatrixup 8 $held; set held 0 }; if {$m} { keymatrixdown 8 $m; set held $m } }
proc force {} {
    global ef_room ef_d2
    debug write memory 0xD000 11
    debug write memory 0xD002 $ef_d2
    debug write memory 0xD001 $ef_room
    for {set a 0} {$a < 896} {incr a} { debug write memory [expr {0xC800 + $a}] 0 }
    for {set a 0} {$a < 1024} {incr a} { debug write memory [expr {0xD700 + $a}] 0 }
    debug write memory 0xC000 4
}
proc hook {} {
    global f ph tt n lg ef_x ef_y ef_frames ef_whip pinned c415
    incr f
    set c0 [rd 0xC000]
    if {$ph eq "title"} {
        if {$c0 == 5} { set ph load; set n 0; press 0; force } else {
            incr tt
            if {$tt % 120 == 0} { press 1 }
            if {$tt % 120 == 3} { press 0 }
        }
        return
    }
    debug write memory 0xC410 5
    if {$ph eq "load"} {
        incr n
        if {$c0 == 5 && $n > 3} { set ph run; set n 0; debug write memory 0xC415 32; set c415 32 } else { return }
    }
    incr n
    if {$pinned} {
        debug write memory 0xC427 $ef_x
        debug write memory 0xC425 $ef_y
    }
    if {$ef_whip > 0 && $n % $ef_whip == 1} { press 1 } elseif {$ef_whip > 0 && $n % $ef_whip == 4} { press 0 }
    set e [rd 0xC415]
    if {$e < $c415} { if {!$::ef_keep} { set pinned 0 }; puts $lg "# DAMAGE at n=$n e=$e" }
    if {$e < 8} { debug write memory 0xC415 32; set e 32; puts $lg "# REFILL n=$n" }
    set c415 $e
    set s "$n sx=[rd 0xC427] sy=[rd 0xC425] c420=[rd 0xC420] e=$e inv=[rd 0xC42D] w=[rd 0xC422]/[rd 0xC42F] f0c=[rd 0xCF0C] f0d=[rd 0xCF0D] f10=[rd 0xCF10] r=[rd 0xC003]"
    for {set i 0} {$i < 7} {incr i} {
        set b [expr {0xC800 + 128*$i}]
        set t [rd $b]
        if {$t} {
            append s [format " | %d t=%02X s=%d y=%d.%d x=%d.%d vx=%d vy=%d m=%d p=%02X c=%d hp=%d fl=%d" $i $t [rd [expr {$b+1}]] [rd [expr {$b+3}]] [rd [expr {$b+2}]] [rd [expr {$b+5}]] [rd [expr {$b+4}]] [expr {[rd [expr {$b+9}]] + 256*[rd [expr {$b+10}]]}] [expr {[rd [expr {$b+7}]] + 256*[rd [expr {$b+8}]]}] [rd [expr {$b+6}]] [rd [expr {$b+11}]] [rd [expr {$b+12}]] [rd [expr {$b+13}]] [rd [expr {$b+14}]]]
        }
    }
    puts $lg $s
    if {$n >= $ef_frames} { close $lg; set d [open [file join $::ef_out done.txt] w]; puts $d ok; close $d; exit }
}
debug set_bp 0x4028 {} {hook}
