# Trace des dégâts, recul, invulnérabilité, vies et mort (lecture seule sauf les pokes de scénario).
# Usage : VK_OUT=<dossier> VK_SCENARIO=natural|death|gameover openmsx -machine C-BIOS_MSX2 \
#         -cart <rom> -script tools/player_health_trace.tcl -control stdio
# natural : titre passé par SPACE, marche à droite de 25 s à 90 s (contacts ennemi vers 40 s).
# death   : idem, avec C415 (énergie) forcée à 2 à 40 s : un contact amène l'énergie à 0.
# gameover: idem, avec en plus C410 (vies) forcée à 0 : fin de partie puis retour au titre.
# Sortie : frames.csv (octets d'état par VBLANK), writes.csv (écritures avec PC),
#          ram-c400.bin (C400-C5FF par image, enregistrement = 4 octets d'image + 512 octets).
set throttle off
set pause off
set power on
set dir [expr {[info exists ::env(VK_OUT)] ? $::env(VK_OUT) : "."}]
set scenario [expr {[info exists ::env(VK_SCENARIO)] ? $::env(VK_SCENARIO) : "natural"}]
set wcsv [open [file join $dir writes.csv] w]
set fcsv [open [file join $dir frames.csv] w]
set dumpcsv [open [file join $dir ram-c400.bin] w]
fconfigure $dumpcsv -translation binary -encoding binary
puts $wcsv "time,pc,addr,value,c000,c001"
puts $fcsv "frame,time,c000,c001,c410,c411,c413,c408,c409,c40a,c40c,c40d,c41b,c420,c425,c427,c004,c405,c407,c002,c0a7,e600"
set ::frame 0
proc pcof {} {
    set hi [debug read {CPU regs} 20]
    set lo [debug read {CPU regs} 21]
    return [format %04X [expr {$hi*256+$lo}]]
}
foreach a {C408 C409 C40A C40C C40D C410 C411 C413 C41B C420 C405 C407 C000 C001 C004 C002 C0A7 E600 C425 C427 C415 C418 C419 C41A C423 C42D} {
    debug set_watchpoint write_mem [list 0x$a] {} [format {
        puts $wcsv "[machine_info time],[pcof],%s,[debug read memory 0x%s],[debug read memory 0xC000],[debug read memory 0xC001]"
        flush $wcsv
    } $a $a]
}
debug set_bp 0x4028 {} {
    incr ::frame
    set r {}
    foreach a {C000 C001 C410 C411 C413 C408 C409 C40A C40C C40D C41B C420 C425 C427 C004 C405 C407 C002 C0A7 E600} {
        lappend r [debug read memory 0x$a]
    }
    puts $fcsv "$::frame,[machine_info time],[join $r ,]"
    puts -nonewline $dumpcsv [binary format i $::frame][debug read_block memory 0xC400 512]
}
debug set_bp 0x4194 {} {after time 0.2 {keymatrixdown 8 1}; after time 0.4 {keymatrixup 8 1}}
after time 25 { keymatrixdown 8 128 }
if {$scenario eq "death" || $scenario eq "gameover"} {
    after time 40 { debug write memory 0xC415 2 }
}
if {$scenario eq "gameover"} {
    after time 40 { debug write memory 0xC410 0 }
}
after time 90 { keymatrixup 8 128; close $wcsv; close $fcsv; close $dumpcsv; exit }
