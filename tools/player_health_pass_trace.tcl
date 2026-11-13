# Death by enemy contact and fall out of the room traced in ONE session with the same markers (research/player-health.md,
# "Ecart 174 / 182"). Usage: VK_LOG=out.txt openmsx -machine C-BIOS_MSX2 -cart <rom> -script tools/player_health_pass_trace.tcl -control stdio
# Title by SPACE, walk right from 25 s, C415 := 2 at 40 s (contact death), then after the respawn Y := 230 (fall out of the room,
# 76C8). One line per VBLANK ("F": C003, C005, C420, C421, C428, C413, en = nonzero C800 blocks) and one line per write to C005,
# C413 and C420 with the PC ("W"): 4053 sets C005 (pass start), 405C clears it (pass end), 7070 / 76C8 set C420 := 6, 70D9 C413 := 0.
set throttle off
set pause off
set power on
set log [open $::env(VK_LOG) w]
set ::f 0
set ::phase 0
set ::resp 0
proc rd {a} { return [debug read memory $a] }
proc nz {} { set n 0; for {set a 0} {$a<256} {incr a 16} { if {[rd [expr {0xC800+$a}]]} {incr n} }; return $n }
proc pcof {} { return [format %04X [expr {[debug read {CPU regs} 20]*256+[debug read {CPU regs} 21]}]] }
foreach a {C005 C413 C420} {
  debug set_watchpoint write_mem [list 0x$a] {} [format {puts $::log "W %s=[debug read memory 0x%s] pc=[pcof] f=$::f t=[machine_info time]"} $a $a]
}
debug set_bp 0x4194 {} {after time 0.2 {keymatrixdown 8 1}; after time 0.4 {keymatrixup 8 1}}
debug set_bp 0x4028 {} {
  incr ::f
  set c0 [rd 0xC000]
  puts $::log [format "F %d p%d t=%.4f c000=%d c003=%d c005=%d c420=%d c421=%d c425=%d c427=%d c428=%d c413=%dc415=%d en=%s" $::f $::phase [machine_info time] $c0 [rd 0xC003] [rd 0xC005] [rd 0xC420] [rd 0xC421] [rd 0xC425] [rd 0xC427] [rd 0xC428] [rd 0xC413] [rd 0xC415] [nz]]
  if {$::phase == 1 && $c0 == 6} { set ::seen6 1 }
  if {$::phase == 1 && [info exists ::seen6] && $c0 == 5} { incr ::resp; if {$::resp == 240} { set ::phase 2; set ::resp 0; debug write memory 0xC425 230 } }
  if {$::phase == 2 && $c0 == 6} { incr ::resp; if {$::resp > 260} { close $::log; exit } }
}
after time 25 { keymatrixdown 8 128 }
after time 40 { debug write memory 0xC415 2; set ::phase 1 }
after time 41 { keymatrixup 8 128 }
after time 200 { close $log; exit }
