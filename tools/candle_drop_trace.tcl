# Usage : VK_OUT=<dossier> openmsx -machine C-BIOS_MSX2 -cart <rom> -script tools/candle_drop_trace.tcl -control stdio
# Genere par scratchpad/candles/c2.tcl. Documente dans research/candles-and-drops.md.
set throttle off
set pause off
set power on
# Lecture seule. Scenario courtyard identique a c1 + snapshots complets par VBLANK.
# Sorties : state.csv, diff.log (octets changes), snap-c8.bin (C800-CBFF), snap-d6.bin (D600-D7FF), snap-c4.bin (C400-C5FF).
set out $::env(VK_OUT)
set log [open $out/diff.log w]
set csv [open $out/state.csv w]
set sn8 [open $out/snap-c8.bin w]; fconfigure $sn8 -translation binary
set sd6 [open $out/snap-d6.bin w]; fconfigure $sd6 -translation binary
set sc4 [open $out/snap-c4.bin w]; fconfigure $sc4 -translation binary
puts $csv "n,phase,cnt,c000,d000,d001,c427,c425,c410,c411,c415,c418,c420,c42d"
set ::n 0
set ::phase title
set ::cnt 0
set ::lastx -1
set ::prevA ""
set ::prevB ""
set ::prevC ""
proc rd {a} { return [debug read memory $a] }
proc shot {name} { global out; catch {screenshot -raw $out/$name.png} }
proc note {m} { global log n; puts $log "# n=$n $m"; flush $log }
proc diffreg {base len prevname tag} {
    global log n
    upvar #0 $prevname prev
    set cur [debug read_block memory $base $len]
    if {$prev eq ""} { set prev $cur; return }
    if {$cur eq $prev} { return }
    binary scan $cur cu* cv
    binary scan $prev cu* pv
    set i 0
    foreach c $cv p $pv {
        if {$c != $p} {
            puts $log [format "%s n=%d %04X %02X->%02X" $tag $n [expr {$base + $i}] $p $c]
        }
        incr i
    }
    set prev $cur
}
proc sample {} {
    global n csv phase cnt sn8 sd6 sc4
    set v [list $n $phase $cnt]
    foreach a {0xC000 0xD000 0xD001 0xC427 0xC425 0xC410 0xC411 0xC415 0xC418 0xC420 0xC42D} { lappend v [rd $a] }
    puts $csv [join $v ,]
    set b8 [debug read_block memory 0xC800 1024]
    puts -nonewline $sn8 $b8
    puts -nonewline $sd6 [debug read_block memory 0xD600 512]
    puts -nonewline $sc4 [debug read_block memory 0xC400 512]
    diffreg 0xC400 512 ::prevA C4
    diffreg 0xC800 1024 ::prevB C8
    diffreg 0xD600 512 ::prevC D6
}
proc finish {} {
    global log csv out sn8 sd6 sc4
    sample
    close $log; close $csv; close $sn8; close $sd6; close $sc4
    set f [open $out/done.txt w]; puts $f ok; close $f
    exit
}
proc frame {} {
    global n phase cnt lastx
    incr n
    sample
    set c000 [rd 0xC000]; set d000 [rd 0xD000]; set d001 [rd 0xD001]; set x [rd 0xC427]
    incr cnt
    switch $phase {
        title {
            if {$c000 == 5} {
                set phase walk1; set cnt 0; keymatrixdown 8 128; note walk1
            } else {
                if {$n % 10 == 0} { keymatrixdown 8 1 } else {
                    if {$n % 10 == 3} { keymatrixup 8 1 }
                }
            }
        }
        walk1 {
            if {$d000 == 0 && $d001 == 1} {
                keymatrixup 8 128; set phase settle; set cnt 0; note room1-enter; shot room1-enter
            } else {
                if {$x == $lastx && $cnt % 10 == 0} { keymatrixdown 8 128 }
            }
        }
        settle { if {$cnt >= 60 && $c000 == 5} { set phase idle; set cnt 0; note idle } }
        idle { if {$cnt >= 60} { set phase walk2; set cnt 0; keymatrixdown 8 128; note walk2 } }
        walk2 {
            if {$x >= 36} {
                keymatrixup 8 128; set phase whipA; set cnt 0; note whipA; shot before-whipA
            } else {
                if {$x == $lastx && $cnt % 10 == 0} { keymatrixdown 8 128 }
            }
        }
        whipA {
            if {$cnt == 2} { keymatrixdown 8 1 }
            if {$cnt == 6} { keymatrixup 8 1 }
            if {$cnt == 60} { set phase walk3; set cnt 0; keymatrixdown 8 128; note walk3 }
        }
        walk3 {
            if {$x >= 168} {
                keymatrixup 8 128; set phase whip; set cnt 0; note whipB; shot before-whipB
            } else {
                if {$x == $lastx && $cnt % 10 == 0} { keymatrixdown 8 128 }
            }
        }
        whip {
            if {$cnt == 2} { keymatrixdown 8 1 }
            if {$cnt == 6} { keymatrixup 8 1 }
            if {$cnt == 60} { set phase pick; set cnt 0; note pick; shot after-whipB-1s }
        }
        pick {
            if {$cnt <= 45} { keymatrixdown 8 128 } else { keymatrixup 8 128 }
            if {$cnt == 46} { note pickstop }
            if {$cnt >= 200} { finish }
        }
    }
    set lastx $x
    if {$n >= 2500} { finish }
}
debug set_bp 0x4028 {} {frame}
