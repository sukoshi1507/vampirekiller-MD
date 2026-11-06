# Trace des appels BIOS GIWRTE (0093) pendant le jeu. Lecture seule, la ROM n'est pas modifiee.
# Variables attendues: vk_output, vk_seconds, vk_keys ("t,row,col,hold" separes par des espaces).
set throttle off
set vk_csv [open [file join $vk_output psg-writes.csv] w]
puts $vk_csv "time,frame,reg,value,caller"
set vk_frame 0
# openMSX VGM recorder (scripts/_vgmrecorder.tcl): ecrit les ecritures PSG au niveau puce.
vgm_rec start PSG MSX-Music
set vk_cnt_bios 0
proc vk_sp {} {
    set sp [expr {256 * [debug read {CPU regs} 22] + [debug read {CPU regs} 23]}]
    return $sp
}
proc vk_caller {} {
    set sp [vk_sp]
    set ret [expr {[debug read memory $sp] + 256 * [debug read memory [expr {($sp + 1) & 0xFFFF}]]}]
    return [format %04X [expr {($ret - 3) & 0xFFFF}]]
}
debug set_bp 0x0093 {} {
    global vk_csv vk_frame vk_cnt_bios
    incr vk_cnt_bios
    puts $vk_csv "[machine_info time],$vk_frame,[debug read {CPU regs} 0],[debug read {CPU regs} 5],[vk_caller]"
}
set vk_state_csv [open [file join $vk_output game-state.csv] w]
puts $vk_state_csv "time,frame,c000,c001,c422,c42e"
set vk_last_state ""
debug set_bp 0x4028 {} {
    global vk_frame vk_state_csv vk_last_state
    incr vk_frame
    set st "[debug read memory 0xC000],[debug read memory 0xC001]"
    if {$st ne $vk_last_state} {
        set vk_last_state $st
        puts $vk_state_csv "[machine_info time],$vk_frame,[debug read memory 0xC000],[debug read memory 0xC001],[debug read memory 0xC422],[debug read memory 0xC42E]"
        flush $vk_state_csv
    }
}
proc vk_key_pulse {row col hold} {
    keymatrixdown $row $col
    after time $hold "keymatrixup $row $col"
}
foreach k $vk_keys {
    lassign [split $k ,] t row col hold
    after time $t "vk_key_pulse $row $col $hold"
}
proc vk_finish {} {
    global vk_csv vk_output vk_frame vk_cnt_bios
    close $vk_csv
    catch {close $vk_state_csv}
    catch {close $vk_mem}
    catch {vgm_rec stop}
    set f [open [file join $vk_output psg-summary.txt] w]
    puts $f "frames=$vk_frame bios_giwrte=$vk_cnt_bios time=[machine_info time]"
    close $f
    exit
}
# Journal des ecritures RAM dans C080-C0BF (variables son), avec PC de l'ecrivain.
if {[info exists vk_mem_watch] && $vk_mem_watch} {
    set vk_mem [open [file join $vk_output mem-writes.csv] w]
    puts $vk_mem "time,frame,addr,value,pc"
    debug set_watchpoint write_mem {0xC080 <= addr && addr <= 0xC0BF} {} {
        global vk_mem vk_frame
        set pc [expr {256 * [debug read {CPU regs} 20] + [debug read {CPU regs} 21]}]
        puts $vk_mem "[machine_info time],$vk_frame,[format %04X $::wp_last_address],[format %02X $::wp_last_value],[format %04X $pc]"
    }
}
after time $vk_seconds {vk_finish}
