# Run by capture_msx_startup.py with vk_output set to an isolated directory.
# Breakpoints only observe the cartridge; the sole input is the SPACE key.
set throttle off
set pause off
set power on
if {![info exists vk_attract]} {set vk_attract 0}
set vk_trace [open [file join $vk_output [expr {$vk_attract ? "attract-trace.csv" : "trace.csv"}]] w]
puts $vk_trace "time,state,substate,timer,stage,room,display"
set vk_updates [open [file join $vk_output [expr {$vk_attract ? "attract-updates.csv" : "updates.csv"}]] w]
puts $vk_updates "update,time,c003"
set vk_last ""
set vk_count 0
set vk_title_seen 0

proc vk_dump {name} {
    global vk_output
    foreach {device extension} {VRAM vram {VDP palette} pal {VDP regs} regs} {
        save_debuggable $device [file join $vk_output "$name.$extension"]
    }
    set out [open [file join $vk_output "$name.ram"] wb]
    puts -nonewline $out [debug read_block memory 0xC000 16384]
    close $out
}

proc vk_tick {} {
    global vk_trace vk_last vk_attract vk_updates vk_output
    set a [debug read memory 0xC000]
    set b [debug read memory 0xC001]
    set state "$a,$b"
    if {$state ne $vk_last} {
        puts $vk_trace "[machine_info time],$state,[debug read memory 0xC004],[debug read memory 0xD000],[debug read memory 0xD001],[debug read {VDP regs} 1]"
        set vk_last $state
    }
    if {$vk_attract && $a == 2 && $b == 1} {
        close $vk_trace
        close $vk_updates
        set out [open [file join $vk_output attract-complete.txt] w]
        puts $out [openmsx_info version]
        close $out
        exit
    }
}

proc vk_title {} {
    global vk_title_seen vk_attract
    if {!$vk_title_seen} {
        set vk_title_seen 1
        if {!$vk_attract} {
            vk_dump title
            after time 0.2 {keymatrixdown 8 1}
            after time 0.4 {keymatrixup 8 1}
        }
    }
}

proc vk_intro {} {
    global vk_count vk_output vk_updates
    incr vk_count
    puts $vk_updates "$vk_count,[machine_info time],[debug read memory 0xC003]"
    set out [open [file join $vk_output [format "sprites-%03d.bin" $vk_count]] wb]
    puts -nonewline $out [debug read_block VRAM 0xF400 3072]
    close $out
}

proc vk_courtyard {} {
    global vk_count vk_trace vk_updates vk_output
    if {$vk_count > 0} {
        vk_dump courtyard
        close $vk_trace
        close $vk_updates
        set out [open [file join $vk_output complete.txt] w]
        puts $out [openmsx_info version]
        puts $out $vk_count
        close $out
        exit
    }
}

debug set_bp 0x414D {} {vk_tick}
debug set_bp 0x4191 {} {vk_dump logo}
debug set_bp 0x4194 {} {vk_title}
debug set_bp 0x41F6 {} {vk_dump castle}
debug set_bp 0x4200 {} {vk_intro}
debug set_bp 0x4245 {} {vk_courtyard}
after time 35 {exit 2}
