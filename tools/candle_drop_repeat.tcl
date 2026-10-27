# Usage : VK_OUT=<dossier> [VK_SHIFT=<n>] openmsx -machine C-BIOS_MSX2 -cart <rom> -script tools/candle_drop_repeat.tcl -control stdio
# Repetition du coup torche B (cour, salle 1) : meme scenario que tools/candle_drop_trace.tcl. VK_SHIFT decale le coup de n trames (0 = coup identique).
# Documente dans research/candles-and-drops.md (section Deuxieme essai).
# Sorties : state.csv (une ligne par trame), diff.log (octets C400-C5FF et C800-CBFF modifies),
#           ram-*.bin (RAM C000-FFFF complete aux instants cles).
set throttle off
set pause off
set power on
set out $::env(VK_OUT)
set ::shift 0
catch {set ::shift $::env(VK_SHIFT)}
set log [open $out/diff.log w]
set csv [open $out/state.csv w]
puts $csv "n,phase,cnt,c000,d000,d001,c427,c417,c419,c422,c42f,c800,c500,c501"
set ::n 0
set ::phase title
set ::cnt 0
set ::lastx -1
set ::prevA ""
set ::prevB ""
set ::c800prev 0
set ::c500prev 0
set ::created800 -1
set ::created500 -1
proc rd {a} { return [debug read memory $a] }
proc note {m} { global log n; puts $log "# n=$n $m"; flush $log }
proc dumpram {name} {
    global out
    set f [open $out/$name.bin w]
    fconfigure $f -translation binary
    puts -nonewline $f [debug read_block memory 0xC000 16384]
    close $f
}
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
    global n csv phase cnt
    set v [list $n $phase $cnt]
    foreach a {0xC000 0xD000 0xD001 0xC427 0xC417 0xC419 0xC422 0xC42F 0xC800 0xC500 0xC501} { lappend v [rd $a] }
    puts $csv [join $v ,]
    diffreg 0xC400 512 ::prevA C4
    diffreg 0xC800 1024 ::prevB C8
}
proc finish {} {
    global log csv out
    sample
    close $log; close $csv
    set f [open $out/done.txt w]; puts $f ok; close $f
    exit
}
proc frame {} {
    global n phase cnt lastx shift created800 created500 c800prev c500prev
    incr n
    sample
    set c000 [rd 0xC000]; set d000 [rd 0xD000]; set d001 [rd 0xD001]; set x [rd 0xC427]
    set c800 [rd 0xC800]; set c500 [rd 0xC500]
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
                keymatrixup 8 128; set phase settle; set cnt 0; note room1-enter
            } else {
                if {$x == $lastx && $cnt % 10 == 0} { keymatrixdown 8 128 }
            }
        }
        settle { if {$cnt >= 60 && $c000 == 5} { set phase idle; set cnt 0; note idle } }
        idle { if {$cnt >= 60} { set phase walk2; set cnt 0; keymatrixdown 8 128; note walk2 } }
        walk2 {
            if {$x >= 36} {
                keymatrixup 8 128; set phase whipA; set cnt 0; note whipA
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
                keymatrixup 8 128; set phase whip; set cnt 0; note whipB
            } else {
                if {$x == $lastx && $cnt % 10 == 0} { keymatrixdown 8 128 }
            }
        }
        whip {
            if {$cnt == 1 + $shift} { dumpram ram-whip-pre; note "whip-pre x=$x c417=[rd 0xC417]" }
            if {$cnt == 2 + $shift} { keymatrixdown 8 1; note whip-down }
            if {$cnt == 6 + $shift} { keymatrixup 8 1 }
            if {$cnt == 60 + $shift} { set phase pick; set cnt 0; note pick }
        }
        pick {
            if {$cnt <= 45} { keymatrixdown 8 128 } else { keymatrixup 8 128 }
            if {$cnt == 46} { note pickstop }
            if {$cnt >= 200} { finish }
        }
    }
    if {$created800 < 0 && $phase eq "whip" && $c800 != 0 && $c800prev == 0} {
        set created800 $n; note "C800 cree type=$c800"; dumpram ram-c800-create
    }
    if {$created500 < 0 && $phase eq "whip" && $c500 != 0 && $c500prev == 0} {
        set created500 $n; note "C500 cree type=$c500"; dumpram ram-c500-create
    }
    set c800prev $c800
    set c500prev $c500
    set lastx $x
    if {$n >= 2500} { finish }
}
debug set_bp 0x4028 {} {frame}
