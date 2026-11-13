file mkdir [file join $vk_out snaps]
proc bf_hex {base n} {
    set s ""
    for {set i 0} {$i < $n} {incr i} { append s [format %02X [vk_rd [expr {$base+$i}]]] }
    return $s
}
set bf_csv [open [file join $vk_out bf.csv] w]
set bf_seen 0
proc bf_snap {label} {
    global vk_out vk_frame
    set base [file join $vk_out snaps "f[format %05d $vk_frame]_$label"]
    set f [open $base.vram w]; fconfigure $f -translation binary -encoding binary
    puts -nonewline $f [debug read_block VRAM 0 131072]; close $f
}
proc bf_frame {} {
    global bf_csv vk_frame bf_seen
    set blocks ""
    set any 0
    set flame 0
    for {set k 0} {$k < 7} {incr k} {
        set b [expr {0xC800 + 128*$k}]
        set t [vk_rd $b]
        if {$t == 0} continue
        if {$t == 0x1E} { set flame 1 }
        if {$t == 0x1E || ($t >= 0x12 && $t <= 0x14)} { set any 1 }
        append blocks [format "k%d:%02X,Y%02X%02X,X%02X%02X,B%02X,C%02X,E%02X,7E%02X,7F%02X,%s " $k $t [vk_rd [expr {$b+3}]] [vk_rd [expr {$b+2}]] [vk_rd [expr {$b+5}]] [vk_rd [expr {$b+4}]] [vk_rd [expr {$b+0xB}]] [vk_rd [expr {$b+0xC}]] [vk_rd [expr {$b+0xE}]] [vk_rd [expr {$b+0x7E}]] [vk_rd [expr {$b+0x7F}]] [bf_hex [expr {$b+0x20}] 8]]
    }
    if {!$any && !$bf_seen} return
    set r11 [debug read {VDP regs} 11]; set r5 [debug read {VDP regs} 5]
    set sat [expr {(($r11 & 3) << 15) | (($r5 & 0xF8) << 7) | 0x200}]
    set col [expr {$sat - 0x200}]
    set out ""
    for {set n 0} {$n < 32} {incr n} {
        set a [expr {$sat + 4*$n}]
        set y [debug read VRAM $a]
        if {$y == 216} break
        set p [debug read VRAM [expr {$a+2}]]
        if {$p < 0x40} continue
        append out [format "%d:%d:%d:%02X:%s/%s " $n $y [debug read VRAM [expr {$a+1}]] $p [format %02X [debug read VRAM [expr {$col+16*$n}]]] [format %02X [debug read VRAM [expr {$col+16*$n+15}]]]]
    }
    set pal ""
    for {set i 0} {$i < 32} {incr i} { append pal [format %02X [debug read {VDP palette} $i]] }
    puts $bf_csv "$vk_frame|[vk_rd 0xD000]|[vk_rd 0xD001]|[vk_rd 0xCE00]|$blocks|$out|$pal"
    flush $bf_csv
    if {$flame && $bf_seen == 0} { set bf_seen 1; bf_snap flame }
}
debug set_bp 0x4028 {} {bf_frame}
proc bf_s {} { global bf_csv vk_frame; puts $bf_csv "S7806 $vk_frame ix=[format %04X [expr {([debug read {CPU regs} 14] << 8) | [debug read {CPU regs} 15]}]]"; flush $bf_csv }
debug set_bp 0x7806 {} {bf_s}
