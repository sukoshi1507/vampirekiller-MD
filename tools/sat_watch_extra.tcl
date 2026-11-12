# Extra watchpoints for tools/run_species_trace.py (SP_EXTRA=tools/sat_watch_extra.tcl): who writes CE03 (D700 SAT range
# restriction read by 9FC2) and the SAT colour bytes (+25 + 5k of each C800/D700 block, read by 651B).
# Output: sat_writers.txt in sp_out, one line per distinct (kind, pc, value, object type) with the room.
set sw_log [open [file join $sp_out sat_writers.txt] w]
set sw_seen [dict create]
proc sw_pc {} { return [expr {256 * [debug read {CPU regs} 20] + [debug read {CPU regs} 21]}] }
proc sw_note {kind} {
    global sw_log sw_seen
    set a $::wp_last_address
    set v $::wp_last_value
    if {$kind eq "colour"} {
        set off [expr {$a & 127}]
        if {$off < 0x25 || ($off - 0x25) % 5 != 0 || $off > 0x48} { return }
        set blk [expr {$a & 0xFF80}]
        set t [debug read memory $blk]
        set key "$kind [format %04X [sw_pc]] [expr {$blk >= 0xD700 ? "D" : "C"}] t=$t v=[format %02X $v] off=[format %02X $off]"
    } else {
        set key "$kind [format %04X [sw_pc]] v=[format %02X $v]"
    }
    if {![dict exists $sw_seen $key]} {
        dict set sw_seen $key 1
        puts $sw_log "$key room=[debug read memory 0xD000]:[debug read memory 0xD001] ce00=[debug read memory 0xCE00]"
        flush $sw_log
    }
}
debug set_watchpoint write_mem 0xCE03 {} {sw_note ce03}
foreach base {0xC800 0xC880 0xC900 0xC980 0xCA00 0xCA80 0xCB00 0xCB80 0xD700 0xD780 0xD800 0xD880 0xD900 0xD980 0xDA00 0xDA80} {
    debug set_watchpoint write_mem [list [expr {$base + 0x25}] [expr {$base + 0x48}]] {} {sw_note colour}
}
