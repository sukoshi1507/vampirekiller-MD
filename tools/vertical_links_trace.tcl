# Trace openMSX des liens verticaux (HAUT/BAS) et des sorties gauche/droite.
# Usage : run_vertical_links_trace.py ROM.rom OUTDIR PLANFILE [TIMEOUT_S]
# Variables attendues : vk_out (dossier de sortie), vk_plan (lignes d'actions).
# Actions : hold KEYS MAX LABEL | wait N | poke ADDR VAL | gotox X MAX LABEL | snap LABEL (le script
# ajoute doit definir key_snap). Journaux : frames.csv, writes.csv, events.txt.
# Deux poke de suite : mettre un wait 1 entre eux (sinon l'action suivante est sautee).
set throttle off
set pause off
set power on
set vk_frame 0
set vk_csv [open [file join $vk_out frames.csv] w]
puts $vk_csv "frame,time,d000,d001,c000,c420,c421,c41b,x,y,c41c,c41d,c41e,c41f,c42c,held,act"
set vk_ev [open [file join $vk_out events.txt] w]
set vk_wcsv [open [file join $vk_out writes.csv] w]
puts $vk_wcsv "frame,time,pc,name,new_value,d000,d001,c000,c420,c41b,x,y,c41c,c41d,c41e,c41f,c42c,held"
array set vk_keymask {R 128 D 64 U 32 L 16 S 1}
set vk_held 0

proc vk_mask {spec} {
    global vk_keymask
    set m 0
    foreach k [split $spec +] { incr m $vk_keymask($k) }
    return $m
}
proc vk_press {mask} {
    global vk_held
    if {$vk_held} { keymatrixup 8 $vk_held; set vk_held 0 }
    if {$mask} { keymatrixdown 8 $mask; set vk_held $mask }
}
proc vk_rd {addr} { return [debug read memory $addr] }
proc vk_log_ev {msg} {
    global vk_ev vk_frame
    puts $vk_ev "frame=$vk_frame t=[machine_info time] $msg"
    flush $vk_ev
}
proc vk_pc {} {
    return [format %02X%02X [debug read {CPU regs} 20] [debug read {CPU regs} 21]]
}
proc vk_wlog {name} {
    global vk_frame vk_wcsv vk_held
    set v [vk_rd [expr {$name eq "D000" ? 0xD000 : ($name eq "D001" ? 0xD001 : 0xC41B)}]]
    puts $vk_wcsv "$vk_frame,[machine_info time],[vk_pc],$name,$v,[vk_rd 0xD000],[vk_rd 0xD001],[vk_rd 0xC000],[vk_rd 0xC420],[vk_rd 0xC41B],[vk_rd 0xC427],[vk_rd 0xC425],[vk_rd 0xC41C],[vk_rd 0xC41D],[vk_rd 0xC41E],[vk_rd 0xC41F],[vk_rd 0xC42C],$vk_held"
    flush $vk_wcsv
}
debug set_watchpoint write_mem {0xD000} {} {vk_wlog D000}
debug set_watchpoint write_mem {0xD001} {} {vk_wlog D001}
debug set_watchpoint write_mem {0xC41B} {} {vk_wlog C41B}

set vk_plan_lines [split $vk_plan "\n"]
set vk_plan_idx 0
set vk_act_kind none
set vk_act_frames 0
set vk_act_max 0
set vk_act_label ""
set vk_gap 0
set vk_title_ticks 0
set vk_title_done 0
set vk_stuck 0
set vk_last_x -1
set vk_start_d000 0
set vk_start_d001 0

proc vk_next_action {} {
    global vk_plan_lines vk_plan_idx vk_act_kind vk_act_frames vk_act_max vk_act_label vk_stuck vk_last_x vk_start_d000 vk_start_d001 vk_held
    while {$vk_plan_idx < [llength $vk_plan_lines]} {
        set line [string trim [lindex $vk_plan_lines $vk_plan_idx]]
        incr vk_plan_idx
        if {$line eq "" || [string index $line 0] eq "#"} { continue }
        set w [split $line " "]
        set kind [lindex $w 0]
        set vk_act_kind $kind
        set vk_act_frames 0
        set vk_stuck 0
        set vk_last_x -1
        set vk_start_d000 [vk_rd 0xD000]
        set vk_start_d001 [vk_rd 0xD001]
        switch $kind {
            hold {
                set vk_act_max [lindex $w 2]; set vk_act_label [lindex $w 3]
                vk_press [vk_mask [lindex $w 1]]
                vk_log_ev "BEGIN hold [lindex $w 1] $vk_act_label"
            }
            wait { set vk_act_max [lindex $w 1]; set vk_act_label wait; vk_press 0; vk_log_ev "BEGIN wait $vk_act_max" }
            gotox {
                global vk_goto vk_godir
                set vk_goto [lindex $w 1]; set vk_act_max [lindex $w 2]; set vk_act_label [lindex $w 3]
                set cx [vk_rd 0xC427]
                if {$cx < $vk_goto} { set vk_godir 1; vk_press 128 } else { set vk_godir -1; vk_press 16 }
                vk_log_ev "BEGIN gotox $vk_goto from x=$cx $vk_act_label"
            }
            snap {
                set vk_act_max 1; set vk_act_label [lindex $w 1]; vk_press 0
                key_snap [lindex $w 1]
            }
            poke {
                set addr [lindex $w 1]; set val [lindex $w 2]
                vk_press 0
                debug write memory $addr $val
                vk_log_ev "POKE $addr=$val (readback [vk_rd $addr])"
                set vk_act_kind pokedone
                set vk_act_max 1; set vk_act_label poke
                return
            }
            default { set vk_act_max 1; set vk_act_label $kind; vk_press 0 }
        }
        return
    }
    set vk_act_kind done
}

proc vk_frame_hook {} {
    global vk_frame vk_csv vk_act_kind vk_act_frames vk_act_max vk_act_label vk_gap vk_stuck vk_last_x vk_start_d000 vk_start_d001 vk_held vk_plan_idx vk_title_ticks vk_title_done vk_ev
    incr vk_frame
    if {$vk_frame > 40000} { vk_log_ev "abort frame limit"; vk_finish }
    set d0 [vk_rd 0xD000]; set d1 [vk_rd 0xD001]; set c0 [vk_rd 0xC000]
    set x [vk_rd 0xC427]; set y [vk_rd 0xC425]
    set mode [vk_rd 0xC420]; set c21 [vk_rd 0xC421]; set c41b [vk_rd 0xC41B]
    set s1 [vk_rd 0xC41C]; set s2 [vk_rd 0xC41D]; set s3 [vk_rd 0xC41E]; set s4 [vk_rd 0xC41F]; set c2c [vk_rd 0xC42C]
    set pre "$vk_frame,[machine_info time],$d0,$d1,$c0,$mode,$c21,$c41b,$x,$y,$s1,$s2,$s3,$s4,$c2c,$vk_held"
    if {!$vk_title_done} {
        if {$c0 == 5} { set vk_title_done 1; vk_log_ev "gameplay C000=5 D000=$d0 D001=$d1 x=$x y=$y"; set vk_act_kind none; vk_press 0 }
        incr vk_title_ticks
        if {$vk_title_ticks % 120 == 0} { vk_press 1 }
        if {$vk_title_ticks % 120 == 3} { vk_press 0 }
        if {$vk_title_ticks > 40000} { vk_log_ev "title timeout"; vk_finish }
        if {!$vk_title_done} { puts $vk_csv "$pre,title"; return }
        vk_next_action
    }
    set act $vk_act_kind
    if {$act eq "done"} {
        puts $vk_csv "$pre,done"
        if {$vk_frame % 500 == 0} { flush $vk_csv }
        return
    }
    if {$act eq "pokedone"} { vk_next_action; set act $vk_act_kind }
    incr vk_act_frames
    if {$x == $vk_last_x} { incr vk_stuck } else { set vk_stuck 0 }
    set vk_last_x $x
    set end 0; set res ""
    if {$vk_gap > 0} {
        incr vk_gap -1
        if {$vk_gap == 0} { vk_next_action }
    } else {
        switch $act {
            hold {
                if {$d0 != $vk_start_d000 || $d1 != $vk_start_d001} {
                    set end 1; set res "CHANGE $vk_start_d000/$vk_start_d001 -> $d0/$d1 x=$x y=$y mode=$mode c41b=$c41b"
                } else {
                    if {$vk_stuck >= 45} {
                        set end 1; set res "STUCK x=$x y=$y mode=$mode"
                    } else {
                        if {$vk_act_frames >= $vk_act_max} {
                            set end 1; set res "TIMEOUT x=$x y=$y mode=$mode"
                        }
                    }
                }
            }
            gotox {
                global vk_goto vk_godir
                if {($vk_godir > 0 && $x >= $vk_goto) || ($vk_godir < 0 && $x <= $vk_goto)} {
                    set end 1; set res "REACHED x=$x y=$y mode=$mode"
                } else {
                    if {$d0 != $vk_start_d000 || $d1 != $vk_start_d001} {
                        set end 1; set res "CHANGE $vk_start_d000/$vk_start_d001 -> $d0/$d1 x=$x y=$y mode=$mode"
                    } else {
                        if {$vk_stuck >= 45} { set end 1; set res "STUCK x=$x y=$y mode=$mode" } else {
                            if {$vk_act_frames >= $vk_act_max} { set end 1; set res "TIMEOUT x=$x y=$y mode=$mode" }
                        }
                    }
                }
            }
            wait { if {$vk_act_frames >= $vk_act_max} { set end 1; set res "WAITED x=$x y=$y c0=$c0 d=$d0/$d1 mode=$mode c41b=$c41b" } }
            default { set end 1; set res "" }
        }
        if {$end} {
            vk_log_ev "[string toupper $vk_act_label] $res"
            vk_press 0
            set vk_gap 10
        }
    }
    puts $vk_csv "$pre,$act/$vk_act_label"
    if {$vk_frame % 500 == 0} { flush $vk_csv }
    if {$vk_act_kind eq "done"} { vk_finish }
}

proc vk_finish {} {
    global vk_csv vk_ev vk_wcsv vk_out
    vk_press 0
    close $vk_csv; close $vk_ev; close $vk_wcsv
    set f [open [file join $vk_out done.txt] w]; puts $f ok; close $f
    exit
}

debug set_bp 0x4028 {} {vk_frame_hook}
