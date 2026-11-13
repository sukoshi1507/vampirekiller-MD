# JUMP from the platform and landing back on it (same room). Cases: platform-relative X d, key pressed on pass frame par.
# (derived from platform_fall_trace.tcl) Ledge fall / drop onto the moving platform of the stage 10 room 4 (platform id 1, Y 167, X 129 to 193), one case per
# (Y0, d, parity): Simon is put at (platform X + d, Y0) in state 0 at the pass frame (parity 0) or one frame later, 34 hooks logged
# ("n y x C420 C428 C439 platform-X platform-step"). Compared with the port by tools/platform_fall_compare.py.
# Usage: VK_E6A=<ROM[0x1E6A+10]> VK_LOG=out.txt openmsx -machine C-BIOS_MSX2 -cart <rom> -script tools/platform_fall_trace.tcl -control stdio
# 2026-11-13: 5C80/90B8 steps the platform on the first frame of the pass, 6B39 (8524 probe, 6BAF carry, Simon) on the second.
set throttle off
set pause off
set power on
set ::f 0
set ::ph 0
set ::l 0
set log [open $::env(VK_LOG) w]
proc rd {a} { return [debug read memory $a] }
# cases: {y0 d parity}
set ::cases {}
foreach par {0 1} {
  foreach y0 {168} {
    foreach d $::env(VK_DS) { lappend ::cases [list $y0 $d $par] }
  }
}
set ::ci 0
set ::cs idle
set ::cn 0
set ::last3 -1
debug set_bp 0x4194 {} {after time 0.2 {keymatrixdown 8 1}; after time 0.4 {keymatrixup 8 1}}
debug set_bp 0x4028 {} {
  incr ::f
  set c0 [rd 0xC000]
  if {$::ph == 0 && $c0 == 5} {
    set ::ph 1
    debug write memory 0xD000 10
    debug write memory 0xD002 $::env(VK_E6A)
    debug write memory 0xD001 4
    for {set a 0} {$a < 896} {incr a} { debug write memory [expr {0xC800 + $a}] 0 }
    debug write memory 0xC000 4
    set ::l 0
    return
  }
  if {$::ph == 0} { return }
  incr ::l
  debug write memory 0xC415 32
  debug write memory 0xC410 5
  for {set a 0} {$a < 896} {incr a} { debug write memory [expr {0xC800 + $a}] 0 }
  if {$::ph == 1 && $c0 == 5 && $::l > 400} { set ::ph 2; set ::l 0 }
  if {$::ph != 2} { return }
  set c003 [rd 0xC003]
  set newpass [expr {$c003 != $::last3}]
  set ::last3 $c003
  if {$::ci >= [llength $::cases]} { close $log; exit }
  lassign [lindex $::cases $::ci] y0 d par
  if {$::cs eq "idle"} {
    if {[rd 0xC420] == 0 && [rd 0xC425] == 144 && [rd 0xC599] == 167 && $newpass} {
      set ::cs fire
    }
    return
  }
  if {$::cs eq "fire"} {
    set px [rd 0xC59A]
    set x0 [expr {($px + $d) & 255}]
    debug write memory 0xC427 $x0
    debug write memory 0xC425 168
    set ::cs settle
    set ::cn 0
    return
  }
  if {$::cs eq "settle"} {
    incr ::cn
    if {$::cn >= 12 && $newpass && [rd 0xC420] == 0 && [rd 0xC425] == 168} {
      puts $log [format "CASE %d y0=%d d=%d par=%d x0=%d px=%d dx=%d cnt=%d len=%d" $::ci $y0 $d $par [rd 0xC427] [rd 0xC59A] [rd 0xC59B] [rd 0xC59E] [rd 0xC59C]]
      if {$par == 0} { keymatrixdown 8 [expr {0x20 | $::env(VK_DIR)}]; set ::cs run; set ::cn 0 } else { set ::cs wait1 }
    }
    return
  }
  if {$::cs eq "wait1"} { keymatrixdown 8 [expr {0x20 | $::env(VK_DIR)}]; set ::cs run; set ::cn 0; return }
  if {$::cs eq "run"} {
    incr ::cn
    if {$::cn == 4} { keymatrixup 8 0x20 }
    puts $log [format "%d y=%d x=%d c420=%d c428=%d c439=%d px=%d dx=%d" $::cn [rd 0xC425] [rd 0xC427] [rd 0xC420] [rd 0xC428] [rd 0xC439] [rd 0xC59A] [rd 0xC59B]]
    if {$::cn >= 60} {
      incr ::ci; keymatrixup 8 $::env(VK_DIR)
      set ::cs idle
      debug write memory 0xC425 144
      debug write memory 0xC427 40
      debug write memory 0xC420 0
      debug write memory 0xC428 0
      debug write memory 0xC421 0
      debug write memory 0xC439 0
    }
    return
  }
}
after time 4000 { close $log; exit }
