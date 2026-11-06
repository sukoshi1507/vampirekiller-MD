# Trace des ecritures des emplacements de fouet C450 (etat) et C451 (activite), avec le PC de l'ecrivain.
# Lecture seule. Variables attendues : vk_output, vk_level (C416 force, -1 = aucun), vk_end (secondes).
# Scenario : un coup debout face a droite a 42 s, puis un coup accroupi a droite a 60 s.
# PC de l'ecrivain = CPU regs 20 (octet haut) et 21 (octet bas). Sortie : writers.csv.
set throttle off
set pause off
set power on
set vk_csv [open [file join $vk_output writers.csv] w]
puts $vk_csv "time,addr,value,pc,c420,c422,c42c,c42e,c42f,c436,c096"
proc vk_wr {addr} {
    global vk_csv
    set pc [expr {[debug read {CPU regs} 20] * 256 + [debug read {CPU regs} 21]}]
    puts $vk_csv "[machine_info time],[format %X $addr],[debug read memory $addr],[format %04X $pc],[debug read memory 0xC420],[debug read memory 0xC422],[debug read memory 0xC42C],[debug read memory 0xC42E],[debug read memory 0xC42F],[debug read memory 0xC436],[debug read memory 0xC096]"
    flush $vk_csv
}
debug set_watchpoint write_mem {0xC450} {} { vk_wr 0xC450 }
debug set_watchpoint write_mem {0xC451} {} { vk_wr 0xC451 }
debug set_watchpoint write_mem {0xC453} {} { vk_wr 0xC453 }
debug set_watchpoint write_mem {0xC460} {} { vk_wr 0xC460 }
debug set_watchpoint write_mem {0xC461} {} { vk_wr 0xC461 }
debug set_bp 0x4194 {} {after time 0.2 {keymatrixdown 8 1}; after time 0.4 {keymatrixup 8 1}}
if {$vk_level >= 0} {
    after time 41.0 "debug write memory 0xC416 $vk_level"
}
after time 40.0 { keymatrixdown 8 1 }
after time 40.15 { keymatrixup 8 1 }
after time 42.0 { keymatrixdown 8 1 }
after time 42.15 { keymatrixup 8 1 }
after time 60.0 { keymatrixdown 8 65 }
after time 60.25 { keymatrixup 8 65 }
after time $vk_end {
    global vk_csv vk_output
    close $vk_csv
    set f [open [file join $vk_output done.txt] w]; puts $f ok; close $f
    exit
}
