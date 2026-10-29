# Captures for the HUD item box (8EE6) and the shield pose (7618), to append after
# tools/vertical_links_trace.tcl (launched by tools/run_hud_shield_capture.py).
# snap LABEL writes LABEL.png (screenshot -raw), LABEL.vram (128 KB), LABEL.pal
# (VDP palette), LABEL.regs (VDP registers) and LABEL.ram (C400-C7FF); state.csv logs C701, C441, C42E/C42F.
proc key_snap {label} {
    global vk_out
    catch {screenshot -raw [file join $vk_out $label.png]}
    foreach {device ext len} {VRAM vram 131072 {VDP palette} pal 32 {VDP regs} regs 64} {
        set f [open [file join $vk_out $label.$ext] w]
        fconfigure $f -translation binary
        puts -nonewline $f [debug read_block $device 0 $len]
        close $f
    }
    set f [open [file join $vk_out $label.ram] w]
    fconfigure $f -translation binary
    puts -nonewline $f [debug read_block memory 0xC400 0x400]
    close $f
    vk_log_ev "SNAP $label c701=[vk_rd 0xC701] c441=[vk_rd 0xC441] c42e=[vk_rd 0xC42E] c42f=[vk_rd 0xC42F] c420=[vk_rd 0xC420] c422=[vk_rd 0xC422]"
}
set h_csv [open [file join $vk_out state.csv] w]
puts $h_csv "frame,c000,d000,d001,x,y,c420,c422,c42c,c42e,c42f,c701,c441,c419,c5e5,c5e6,c003"
# The first VBLANK with Simon crouched (C420 = 2) for 8 frames is snapped as crouch_auto.
set h_crouch 0
proc h_frame {} {
    global h_csv vk_frame h_crouch
    if {[vk_rd 0xC420] == 2} { incr h_crouch; if {$h_crouch == 8} { key_snap crouch_auto } } else { set h_crouch 0 }
    set r {}
    foreach a {C000 D000 D001 C427 C425 C420 C422 C42C C42E C42F C701 C441 C419 C5E5 C5E6 C003} { lappend r [vk_rd 0x$a] }
    puts $h_csv "$vk_frame,[join $r ,]"
    if {$vk_frame % 200 == 0} { flush $h_csv }
}
debug set_bp 0x4028 {} {h_frame}
