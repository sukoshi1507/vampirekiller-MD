# Posture contact trace (hall room 0): per-frame Simon + objects, 847A box results,
# reactive jump over right-side zombies (distance list), then crouch until a contact.
# Extra script for the plan harness scratchpad/transition/harness.tcl (run through
# scratchpad/fidelity/run_fid.py OUT PLAN tools/contact_posture_trace.tcl), plan:
#   hold R 1500 s0 / hold R 1500 s1 / hold R 1500 s2 / wait 30 / gotox 128 300 g128 / wait 3000
# Keys: UP (row 8 bit 5) jumps (C006 bit 5, 6B91), DOWN crouches. Outputs: posture.csv,
# box.csv (breakpoint 84A5: C420, C425, C427, D/E = object X/Y, A, H, hit = A < H),
# posture_ev.txt. research/enemy-behavior.md, "Contact selon la posture de Simon".
proc key_snap {label} {}
set pz_csv [open [file join $vk_out posture.csv] w]
puts $pz_csv "frame,d000,d001,c420,c421,x,y,c415,c42d,o0,o1,o2,phase,keys"
set pz_box [open [file join $vk_out box.csv] w]
puts $pz_box "frame,c420,c425,c427,objx,objy,a,h,hit"
set pz_ev [open [file join $vk_out posture_ev.txt] w]
set pz_phase 0
set pz_jumps {23 30 18 26 34}
set pz_jidx 0
set pz_keyt 0
set pz_keys 0
set pz_e0 -1
set pz_target -1
proc pz_hex {a n} {
    set s ""
    for {set i 0} {$i < $n} {incr i} { append s [format %02X [vk_rd [expr {$a + $i}]]] }
    return $s
}
proc pz_log {msg} {
    global pz_ev vk_frame
    puts $pz_ev "frame=$vk_frame x=[vk_rd 0xC427] y=[vk_rd 0xC425] c420=[vk_rd 0xC420] c415=[vk_rd 0xC415] $msg"
    flush $pz_ev
}
proc pz_keydown {m n} {
    global pz_keys pz_keyt
    if {$pz_keys} { keymatrixup 8 $pz_keys }
    set pz_keys $m
    set pz_keyt $n
    if {$m} { keymatrixdown 8 $m }
}
proc pz_frame {} {
    global pz_csv vk_frame vk_act_kind pz_phase pz_jumps pz_jidx pz_keyt pz_keys pz_e0 pz_target
    set d0 [vk_rd 0xD000]
    set d1 [vk_rd 0xD001]
    set x [vk_rd 0xC427]
    set y [vk_rd 0xC425]
    set e [vk_rd 0xC415]
    puts $pz_csv "$vk_frame,$d0,$d1,[vk_rd 0xC420],[vk_rd 0xC421],$x,$y,$e,[vk_rd 0xC42D],[pz_hex 0xC800 6],[pz_hex 0xC880 6],[pz_hex 0xC900 6],$pz_phase,$pz_keys"
    if {$vk_frame % 200 == 0} { flush $pz_csv }
    if {$pz_keyt > 0} {
        incr pz_keyt -1
        if {$pz_keyt == 0} { keymatrixup 8 $pz_keys; set pz_keys 0 }
    }
    if {$pz_e0 >= 0 && $e != $pz_e0} { pz_log "ENERGY $pz_e0 -> $e phase=$pz_phase" }
    set pz_e0 $e
    if {!($vk_act_kind eq "wait" && $d0 == 1 && $d1 == 0)} { return }
    if {$pz_phase == 0} {
        if {$pz_jidx >= [llength $pz_jumps]} {
            set pz_phase 2
            pz_log "CROUCH start"
            pz_keydown 64 100000
            return
        }
        if {[vk_rd 0xC420] != 0 || [vk_rd 0xC42D] != 0} { return }
        set dist [lindex $pz_jumps $pz_jidx]
        foreach b {0xC800 0xC880 0xC900} {
            if {[vk_rd $b] != 1} { continue }
            set ox [vk_rd [expr {$b + 5}]]
            if {$ox > $x && $ox - $x <= $dist} {
                set pz_target $b
                pz_log "JUMP dist=$dist obj=[format %04X $b] ox=$ox oy=[vk_rd [expr {$b + 3}]]"
                pz_keydown 32 6
                set pz_phase 1
                return
            }
        }
        return
    }
    if {$pz_phase == 1} {
        set ox [vk_rd [expr {$pz_target + 5}]]
        set m [vk_rd 0xC420]
        if {[vk_rd $pz_target] != 1 || $ox + 12 < $x} {
            if {$m == 0 || $m == 5} {
                pz_log "PASSED target ox=$ox c42d=[vk_rd 0xC42D]"
                incr pz_jidx
                set pz_phase 0
            }
        }
        if {$m == 5} {
            pz_log "HIT during jump attempt ox=$ox"
            incr pz_jidx
            set pz_phase 0
        }
        return
    }
    if {$pz_phase == 2} {
        if {[vk_rd 0xC42D] != 0} {
            pz_log "CROUCH contact"
            pz_keydown 0 0
            set pz_phase 3
        }
    }
}
debug set_bp 0x4028 {} {pz_frame}
proc pz_boxlog {} {
    global pz_box vk_frame
    if {[vk_rd 0x847A] != 0x3E || [vk_rd 0x847B] != 0x05} { return }
    set a [debug read {CPU regs} 0]
    set h [debug read {CPU regs} 6]
    set hit [expr {$a < $h ? 1 : 0}]
    puts $pz_box "$vk_frame,[vk_rd 0xC420],[vk_rd 0xC425],[vk_rd 0xC427],[debug read {CPU regs} 4],[debug read {CPU regs} 5],$a,$h,$hit"
    flush $pz_box
}
debug set_bp 0x84A5 {} {pz_boxlog}
