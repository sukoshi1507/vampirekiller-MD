# Sonde (lecture seule) : a chaque ecriture de C800 (type d'objet cree), journal des registres
# CPU (indices numeriques : PC = 20/21, SP = 22/23 d'apres la sonde) et des 8 mots au sommet de
# la pile, entre les images eb_cfrom et eb_cto. Charge apres tools/enemy_behavior_trace.tcl.
set eb_cfrom 3990
set eb_cto 4000
set eb_clog [open [file join $eb_out callers.txt] w]
proc eb_caller_hook {addr} {
    global eb_clog eb_frame eb_cfrom eb_cto
    if {$eb_frame >= $eb_cfrom && $eb_frame <= $eb_cto} {
        set sp [expr {([debug read {CPU regs} 22] << 8) | [debug read {CPU regs} 23]}]
        set s "frame=$eb_frame addr=$addr PC=[format %02X%02X [debug read {CPU regs} 20] [debug read {CPU regs} 21]] SP=[format %04X $sp] stack:"
        for {set i 0} {$i < 16} {incr i} {
            append s " [format %02X [debug read memory [expr {($sp + $i) & 0xFFFF}]]]"
        }
        set regs ""
        for {set i 0} {$i < 20} {incr i} { append regs " r$i=[format %02X [debug read {CPU regs} $i]]" }
        puts $eb_clog "$s |$regs"
        flush $eb_clog
    }
}
debug set_watchpoint write_mem {0xC800} {} {eb_caller_hook 0xC800}
