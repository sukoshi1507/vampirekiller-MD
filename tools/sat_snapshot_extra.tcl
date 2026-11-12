# Extra script for tools/run_species_trace.py (SP_EXTRA=tools/sat_snapshot_extra.tcl): SAT ownership as the ROM holds it.
# Every distinct state of a live C800 (7 blocks) / D700 (8 blocks) object is logged to sat_snap.txt in sp_out:
#   SNAP stage:room C|D index type count refs=<SAT references +21+5k> ys=<D638+4*ref> colours=<+25+5k> ce03=<CE03>
# (5F1D / 9F6D write the count at +20, the references at +21, +26, ...; 6445 + 64EC/64E5 copy them to D638..D67C).
set ss_log [open [file join $sp_out sat_snap.txt] w]
set ss_seen [dict create]
rename sp_hook sp_hook_orig
proc ss_snap {} {
    global ss_log ss_seen
    set rm "[debug read memory 0xD000]:[debug read memory 0xD001]"
    foreach {kind base n} {C 0xC800 7 D 0xD700 8} {
        for {set i 0} {$i < $n} {incr i} {
            set b [expr {$base + 128 * $i}]
            set t [debug read memory $b]
            if {!$t} { continue }
            set c [debug read memory [expr {$b + 0x20}]]
            if {$c > 12} { continue }
            set refs {}; set ys {}; set cols {}
            for {set k 0} {$k < $c} {incr k} {
                set r [debug read memory [expr {$b + 0x21 + 5 * $k}]]
                lappend refs $r
                lappend ys [format %02X [debug read memory [expr {0xD638 + 4 * ($r & 31)}]]]
                lappend cols [format %02X [debug read memory [expr {$b + 0x25 + 5 * $k}]]]
            }
            set key "SNAP $rm $kind $i [format %02X $t] $c refs=$refs colours=$cols"
            if {![dict exists $ss_seen $key]} {
                dict set ss_seen $key 1
                puts $ss_log "$key ys=$ys pose=[format %02X [debug read memory [expr {$b + 11}]]] ce03=[debug read memory 0xCE03]"
                flush $ss_log
            }
        }
    }
}
proc sp_hook {} { sp_hook_orig; ss_snap }
