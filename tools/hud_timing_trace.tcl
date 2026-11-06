# Timing of the HUD item box on a pickup (8D30 -> 8F23, 8F55, 8EE6), to append after
# tools/vertical_links_trace.tcl and tools/hud_shield_capture.tcl (launched by
# tools/run_hud_timing_trace.py).
# box.csv: per VBLANK (4028), C003, C5E5, C5E6, C419, C701, VDP R#2 (shown page) and the
# number of passes of 8A4A since the previous VBLANK.
# box.bin: per VBLANK, the box x B8..F7, y 0C..1B of page 0 then page 1 (SCREEN 5,
# 128 bytes a line: 16 lines of 32 bytes, 512 bytes a page, 1024 bytes a frame).
# calls.csv: each call of 8A4A (pass), 8F2D (object shown alone), 8F65 (C5E6 reached 0)
# and 8EE6 (box drawn again), with the frame, C003 and C5E6.
set hb_csv [open [file join $vk_out box.csv] w]
puts $hb_csv "frame,c000,c003,c5e5,c5e6,c419,c701,r2,passes"
set hb_bin [open [file join $vk_out box.bin] w]
fconfigure $hb_bin -translation binary
set hb_calls [open [file join $vk_out calls.csv] w]
puts $hb_calls "frame,pc,c003,c5e5,c5e6,c419,c701"
set hb_passes 0
proc hb_frame {} {
    global hb_csv hb_bin vk_frame hb_passes
    set r {}
    foreach a {C000 C003 C5E5 C5E6 C419 C701} { lappend r [vk_rd 0x$a] }
    lappend r [debug read {VDP regs} 2]
    puts $hb_csv "$vk_frame,[join $r ,],$hb_passes"
    set hb_passes 0
    foreach page {0 0x8000} {
        for {set y 12} {$y < 28} {incr y} {
            puts -nonewline $hb_bin [debug read_block VRAM [expr {$page + $y * 128 + 0x5C}] 32]
        }
    }
    if {$vk_frame % 200 == 0} { flush $hb_csv; flush $hb_bin }
}
proc hb_call {pc} {
    global hb_calls vk_frame hb_passes
    if {$pc eq "8A4A"} { incr hb_passes }
    puts $hb_calls "$vk_frame,$pc,[vk_rd 0xC003],[vk_rd 0xC5E5],[vk_rd 0xC5E6],[vk_rd 0xC419],[vk_rd 0xC701]"
    flush $hb_calls
}
debug set_bp 0x4028 {} {hb_frame}
foreach pc {8A4A 8F2D 8F65 8EE6} {
    debug set_bp 0x$pc {} [list hb_call $pc]
}
