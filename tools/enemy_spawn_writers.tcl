# Journal des ecritures (PC et image) sur les entrees SAT ennemies et sur les objets C800/C880.
# Charge apres tools/enemy_behavior_trace.tcl (variables eb_* deja definies).
set eb_wlog [open [file join $eb_out spawnwriters.txt] w]
puts $eb_wlog "frame,pc_reg20,pc_reg21,addr,value,d000,d001,c000"
proc eb_wpc {} {
    return [format %02X%02X [debug read {CPU regs} 20] [debug read {CPU regs} 21]]
}
proc eb_wlog_write {addr} {
    global eb_wlog eb_frame
    set v [debug read memory $addr]
    puts $eb_wlog "$eb_frame,[eb_wpc],[format %04X $addr],$v,[debug read memory 0xD000],[debug read memory 0xD001],[debug read memory 0xC000]"
}
foreach a {0xD638 0xD63C 0xD648 0xD658 0xC800 0xC880} {
    debug set_watchpoint write_mem [list $a] {} [list eb_wlog_write $a]
}
proc eb_wclose {} { global eb_wlog; close $eb_wlog }
rename eb_finish eb_finish_base
proc eb_finish {} { eb_wclose; eb_finish_base }
