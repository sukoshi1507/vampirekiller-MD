# Trace des effets sonores au coup de fouet : ecritures des variables son (C096, C016, C0A7)
# et des objets (C800, C500, C422, C42F) avec PC de l'ecrivain, plus les ecritures du mapper (6000-7FFF).
# Scenario identique a tools/candle_drop_trace.tcl : fouet a vide (x=36, torche A), fouet a vide (x=100),
# fouet sur la torche B (x=168, cour salle 1). Lecture seule, la ROM n'est pas modifiee.
# Usage : VK_OUT=<dossier> openmsx -machine C-BIOS_MSX2 -cart <rom> -script tools/hit_sound_trace.tcl -control stdio
# Sorties : events.log (une ligne par ecriture), state.csv (etat par image), done.txt.
# Documente dans research/hit-sound.md.
set throttle off
set pause off
set power on
set out $::env(VK_OUT)
set ::log [open $out/events.log w]
set ::csv [open $out/state.csv w]
puts $::csv "n,phase,cnt,c000,d000,d001,c427,c422,c42f,c096,c016,c0a7"
set ::n 0
set ::phase title
set ::cnt 0
set ::lastx -1
proc rd {a} { return [debug read memory $a] }
proc pcval {} { return [format %04X [expr {256 * [debug read {CPU regs} 20] + [debug read {CPU regs} 21]}]] }
proc ev {tag addr val} {
    global ::log ::n ::phase
    puts $::log [format "%s n=%d phase=%s addr=%04X val=%02X pc=%s" $tag $::n $::phase $addr $val [pcval]]
    flush $::log
}
proc note {m} {
    global ::log ::n
    puts $::log "# n=$::n $m"
    flush $::log
}
proc sample {} {
    global ::csv ::n ::phase ::cnt
    set v [list $::n $::phase $::cnt]
    foreach a {0xC000 0xD000 0xD001 0xC427 0xC422 0xC42F 0xC096 0xC016 0xC0A7} { lappend v [rd $a] }
    puts $::csv [join $v ,]
}
proc finish {} {
    global ::log ::csv ::n out
    sample
    close $::log
    close $::csv
    set f [open $out/done.txt w]
    puts $f ok
    close $f
    exit
}
# Watchpoints (un octet a la fois, comme dans les scripts precedents).
debug set_watchpoint write_mem 0xC096 {} {ev C096 $::wp_last_address $::wp_last_value; retlog $::wp_last_value}
debug set_watchpoint write_mem 0xC016 {} {ev C016 $::wp_last_address $::wp_last_value}
debug set_watchpoint write_mem 0xC0A7 {} {ev C0A7 $::wp_last_address $::wp_last_value}
debug set_watchpoint write_mem 0xC422 {} {ev C422 $::wp_last_address $::wp_last_value}
debug set_watchpoint write_mem 0xC42F {} {ev C42F $::wp_last_address $::wp_last_value}
debug set_watchpoint write_mem 0xC800 {} {ev C800 $::wp_last_address $::wp_last_value}
debug set_watchpoint write_mem 0xC500 {} {ev C500 $::wp_last_address $::wp_last_value}
debug set_watchpoint write_mem {0x4000 0x7FFF} {} {ev MAPPER $::wp_last_address $::wp_last_value}
proc retlog {v} {
    # Au point d'ecriture en 517D, la pile contient (hl,de,bc) pousses par 509F : l'adresse de retour est a SP+6.
    global ::log
    set sp [expr {256 * [debug read {CPU regs} 22] + [debug read {CPU regs} 23]}]
    set r [expr {[debug read memory [expr {($sp + 6) & 0xFFFF}]] + 256 * [debug read memory [expr {($sp + 7) & 0xFFFF}]]}]
    puts $::log [format "RET n=%d val=%02X sp=%04X ret_at_sp6=%04X" $::n $v $sp $r]
    flush $::log
}
proc dump_block {n v} {
    global ::out
    set f [open [format "%s/c096-%04d-%02X.bin" $::out $n $v] w]
    fconfigure $f -translation binary
    puts -nonewline $f [debug read_block memory 0x4000 16384]
    close $f
    set g [open [format "%s/c096-%04d-%02X-8000.bin" $::out $n $v] w]
    fconfigure $g -translation binary
    puts -nonewline $g [debug read_block memory 0x8000 16384]
    close $g
}
proc frame {} {
    global ::n ::phase ::cnt ::lastx ::log
    incr ::n
    incr ::cnt
    sample
    set c000 [rd 0xC000]; set d000 [rd 0xD000]; set d001 [rd 0xD001]; set x [rd 0xC427]
    switch $::phase {
        title {
            if {$c000 == 5} {
                set ::phase walk1; set ::cnt 0; keymatrixdown 8 128; note walk1
            } else {
                if {$::n % 10 == 0} { keymatrixdown 8 1 } else {
                    if {$::n % 10 == 3} { keymatrixup 8 1 }
                }
            }
        }
        walk1 {
            if {$d000 == 0 && $d001 == 1} {
                keymatrixup 8 128; set ::phase settle; set ::cnt 0; note room1-enter
            } else {
                if {$x == $::lastx && $::cnt % 10 == 0} { keymatrixdown 8 128 }
            }
        }
        settle { if {$::cnt >= 60 && $c000 == 5} { set ::phase idle; set ::cnt 0; note idle } }
        idle { if {$::cnt >= 60} { set ::phase walk2; set ::cnt 0; keymatrixdown 8 128; note walk2 } }
        walk2 {
            if {$x >= 36} {
                keymatrixup 8 128; set ::phase whipA; set ::cnt 0; note whipA-torchA-x36
            } else {
                if {$x == $::lastx && $::cnt % 10 == 0} { keymatrixdown 8 128 }
            }
        }
        whipA {
            if {$::cnt == 2} { keymatrixdown 8 1 }
            if {$::cnt == 6} { keymatrixup 8 1 }
            if {$::cnt == 60} { set ::phase walk2b; set ::cnt 0; keymatrixdown 8 128; note walk2b }
        }
        walk2b {
            if {$x >= 100} {
                keymatrixup 8 128; set ::phase whipE; set ::cnt 0; note whipE-x100
            } else {
                if {$x == $::lastx && $::cnt % 10 == 0} { keymatrixdown 8 128 }
            }
        }
        whipE {
            if {$::cnt == 2} { keymatrixdown 8 1 }
            if {$::cnt == 6} { keymatrixup 8 1 }
            if {$::cnt == 60} { set ::phase walk3; set ::cnt 0; keymatrixdown 8 128; note walk3 }
        }
        walk3 {
            if {$x >= 168} {
                keymatrixup 8 128; set ::phase whipB; set ::cnt 0; note whipB-torchB-x168
            } else {
                if {$x == $::lastx && $::cnt % 10 == 0} { keymatrixdown 8 128 }
            }
        }
        whipB {
            if {$::cnt == 2} { keymatrixdown 8 1 }
            if {$::cnt == 6} { keymatrixup 8 1 }
            if {$::cnt == 60} { set ::phase pick; set ::cnt 0; note pick }
        }
        pick {
            if {$::cnt <= 45} { keymatrixdown 8 128 } else { keymatrixup 8 128 }
            if {$::cnt == 46} { note pickstop }
            if {$::cnt >= 200} { finish }
        }
    }
    set ::lastx $x
    if {$::n >= 2500} { finish }
}
debug set_bp 0x4028 {} {frame}
