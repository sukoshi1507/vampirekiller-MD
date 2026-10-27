# Usage : VK_OUT=<dossier> VK_SCENARIO=still_tap|still_hold|right_jump|right_held|turn_jump openmsx -machine C-BIOS_MSX2 -cart <rom> -script tools/jump_trace.tcl -control stdio
# openMSX: trace de saut de Simon (lecture seule, sauf les touches simulees).
# Variables d'environnement : VK_OUT (dossier), VK_SCENARIO (voir la table ci-dessous).
# Par image (VBLANK, 0x4028) : frames.csv + ram-c400.bin (C400-C5FF, 512 octets).
# Ecritures : writes.csv (PC de l'ecriture) pour C420, C421, C423, C425, C427.
set throttle off
set pause off
set power on
set dir [expr {[info exists ::env(VK_OUT)] ? $::env(VK_OUT) : "."}]
set scenario [expr {[info exists ::env(VK_SCENARIO)] ? $::env(VK_SCENARIO) : "still_tap"}]
set t0 25.0
# Each event: {time_offset action mask}. Mask 32 = UP, 128 = RIGHT, 16 = LEFT, 1 = SPACE.
switch $scenario {
    still_tap   { set events {{0.0 down 32} {0.05 up 32}}; set tend 4.0 }
    still_hold  { set events {{0.0 down 32} {2.0 up 32}}; set tend 4.0 }
    right_jump  { set events {{0.0 down 128} {0.5 down 32} {0.55 up 32} {2.0 up 128}}; set tend 4.0 }
    right_held  { set events {{0.0 down 128} {0.5 down 32} {2.0 up 32} {2.0 up 128}}; set tend 4.0 }
    turn_jump   { set events {{0.0 down 128} {0.8 up 128} {1.2 down 16} {1.6 down 32} {1.65 up 32} {3.0 up 16}}; set tend 5.0 }
    default     { set events {}; set tend 4.0 }
}
set wcsv [open [file join $dir writes.csv] w]
set fcsv [open [file join $dir frames.csv] w]
set dumpcsv [open [file join $dir ram-c400.bin] w]
fconfigure $dumpcsv -translation binary -encoding binary
puts $wcsv "time,pc,addr,value,c000,c001"
puts $fcsv "frame,time,c000,c001,c003,c006,c007,c420,c421,c423,c425,c427,c428,c42b,c42d,c42e,c42f,c431,c439,cff0,c422"
set ::frame 0
proc pcof {} {
    set hi [debug read {CPU regs} 20]
    set lo [debug read {CPU regs} 21]
    return [format %04X [expr {$hi*256+$lo}]]
}
foreach a {C420 C421 C423 C425 C427 C428 C006 C003} {
    debug set_watchpoint write_mem [list 0x$a] {} [format {
        puts $wcsv "[machine_info time],[pcof],%s,[debug read memory 0x%s],[debug read memory 0xC000],[debug read memory 0xC001]"
    } $a $a]
}
debug set_bp 0x4028 {} {
    incr ::frame
    set r {}
    foreach a {C000 C001 C003 C006 C007 C420 C421 C423 C425 C427 C428 C42B C42D C42E C42F C431 C439 CFF0 C422} {
        lappend r [debug read memory 0x$a]
    }
    puts $fcsv "$::frame,[machine_info time],[join $r ,]"
    puts -nonewline $dumpcsv [binary format i $::frame][debug read_block memory 0xC400 512]
}
debug set_bp 0x4194 {} {after time 0.2 {keymatrixdown 8 1}; after time 0.4 {keymatrixup 8 1}}
foreach e $events {
    lassign $e t action mask
    if {$action eq "down"} {
        after time [expr {$t0 + $t}] [list keymatrixdown 8 $mask]
    } else {
        after time [expr {$t0 + $t}] [list keymatrixup 8 $mask]
    }
}
proc vk_finish {} {
    global wcsv fcsv dumpcsv
    close $wcsv; close $fcsv; close $dumpcsv
    exit
}
after time [expr {$t0 + $tend}] {vk_finish}
# Memory snapshot (CPU view, 64 KB) at t0+1.0 s, for disassembly of the jump routines.
after time [expr {$t0 + 1.0}] {
    set df [open [file join $dir mem-cpu.bin] w]
    fconfigure $df -translation binary -encoding binary
    puts -nonewline $df [debug read_block memory 0x0000 0x10000]
    close $df
}
