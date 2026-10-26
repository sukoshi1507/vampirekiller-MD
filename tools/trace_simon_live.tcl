# openMSX Tcl debugger: read-only Simon animation capture.
# Requires vk_output variable set by the launcher.
set throttle off
set pause off
set power on
set vk_csv [open [file join $vk_output simon-live.csv] w]
puts $vk_csv "tick,time,marker,c000,c001,c42b,c420,c436,lower_c42e,upper_c42f,raw_c422,raw_c423"
set vk_tick_number 0
set vk_marker boot
set vk_last_pair ""
proc vk_mark {name} {
    global vk_marker
    set vk_marker $name
    puts stderr "Simon marker: $name"
}
proc vk_sample {} {
    global vk_csv vk_tick_number vk_marker
    incr vk_tick_number
    set vals {}
    foreach a {0xC000 0xC001 0xC42B 0xC420 0xC436 0xC42E 0xC42F 0xC422 0xC423} {
        lappend vals [debug read memory $a]
    }
    puts $vk_csv "$vk_tick_number,[machine_info time],$vk_marker,[join $vals ,]"
    if {($vk_tick_number % 100) == 0} {flush $vk_csv}
}
proc vk_finish {} {
    global vk_csv vk_output vk_tick_number
    close $vk_csv
    set f [open [file join $vk_output simon-live-complete.txt] w]
    puts $f [openmsx_info version]
    puts $f $vk_tick_number
    close $f
    exit
}
# The game's 0x4028 handler is documented as the interrupt entry point.
# The breakpoint does not mutate cartridge code or memory.
debug set_bp 0x4028 {} {vk_sample}
# Trigger the same title SPACE pulse as the pre-existing validated startup harness.
set vk_title_seen 0
proc vk_begin_game {} {
    global vk_title_seen vk_marker
    if {!$vk_title_seen} {
        set vk_title_seen 1
        set vk_marker title_start
        after time 0.2 {keymatrixdown 8 1}
        after time 0.4 {keymatrixup 8 1}
        after time 0.5 {set vk_marker gameplay_unclassified}
        # Optional experiment: unverified keyboard matrix input.
        # Delayed relative to title-start, clearly labeled in the trace.
        if {[info exists ::vk_probe_row] && [info exists ::vk_probe_col]} {
            after time [expr {0.5 + $::vk_probe_delay}] {
                set ::vk_marker experimental_input_held
                keymatrixdown $::vk_probe_row $::vk_probe_col
            }
            after time [expr {0.5 + $::vk_probe_delay + $::vk_probe_hold}] {
                keymatrixup $::vk_probe_row $::vk_probe_col
                set ::vk_marker experimental_input_released
            }
        }
    }
}
debug set_bp 0x4194 {} {vk_begin_game}
# The gameplay input mapping has not yet been authenticated. An attack is NOT
# synthesized; do not interpret the resulting trace as a whip timeline.
after time 38 {vk_finish}
