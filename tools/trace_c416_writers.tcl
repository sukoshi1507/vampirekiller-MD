# Trace des ecritures de C416 (variable de niveau du fouet) avec les registres CPU, pendant une marche
# automatique. Lecture seule. Variables attendues : vk_output. PC de l'ecrivain = CPU regs 20 (hi) et 21 (lo).
# (valide : 0x70E5 a l'initialisation, cf. research/whip-variants.md).
set throttle off
set pause off
set power on
set vk_csv [open [file join $vk_output writes.csv] w]
set vk_regcsv [open [file join $vk_output regs.csv] w]
puts $vk_csv "time,value,c000,c001,c42e,c42f,c427,c425,c420,c421"
debug set_watchpoint write_mem {0xC416} {} {
    global vk_csv vk_regcsv
    set r {}
    for {set i 0} {$i <= 27} {incr i} { lappend r [debug read {CPU regs} $i] }
    puts $vk_regcsv "[machine_info time],[join $r ,]"
    puts $vk_csv "[machine_info time],[debug read memory 0xC416],[debug read memory 0xC000],[debug read memory 0xC001],[debug read memory 0xC42E],[debug read memory 0xC42F],[debug read memory 0xC427],[debug read memory 0xC425],[debug read memory 0xC420],[debug read memory 0xC421]"
    flush $vk_csv
}
debug set_bp 0x4194 {} {after time 0.2 {keymatrixdown 8 1}; after time 0.4 {keymatrixup 8 1}}
# marche : cycles de 4 s (droite 2 s, gauche 1 s, saut+SPACE, SPACE)
for {set c 0} {$c < 40} {incr c} {
    set t0 [expr {40.0 + 4.0*$c}]
    after time $t0 { keymatrixdown 8 128 }
    after time [expr {$t0+2.0}] { keymatrixup 8 128; keymatrixdown 8 16 }
    after time [expr {$t0+3.0}] { keymatrixup 8 16; keymatrixdown 8 33 }
    after time [expr {$t0+3.3}] { keymatrixup 8 33; keymatrixdown 8 1 }
    after time [expr {$t0+3.5}] { keymatrixup 8 1 }
}
after time 220 { close $vk_csv; close $vk_regcsv; set f [open [file join $vk_output done.txt] w]; puts $f ok; close $f; exit }
