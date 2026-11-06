# openMSX Tcl: lecture seule de zones RAM candidates aux objets (ennemis/plateformes).
# Requiert la variable vk_output (repertoire de sortie) definie par le lanceur.
# Echantillonne a chaque VBLANK (point d'arret 0x4028). N'altere ni ROM ni RAM.
set throttle off
set pause off
set power on
set vk_csv [open [file join $vk_output entities-live.csv] w]
puts $vk_csv "frame,time,marker,c000,c001,c003,c004,c005,c40a,c40c,c410,c415,c418,c425,c427,c002,c006,c007,c00b,c00c,c420_hex,sat_d600_hex,obj_c470_hex"
set vk_frame 0
set vk_marker boot
proc vk_hex {base n} {
    set s ""
    for {set i 0} {$i < $n} {incr i} {
        append s [format %02X [debug read memory [expr {$base + $i}]]]
    }
    return $s
}
proc vk_sample {} {
    global vk_csv vk_frame vk_marker
    incr vk_frame
    set c000 [debug read memory 0xC000]
    set c001 [debug read memory 0xC001]
    set c003 [debug read memory 0xC003]
    set c004 [debug read memory 0xC004]
    set c005 [debug read memory 0xC005]
    set c40a [debug read memory 0xC40A]
    set c40c [debug read memory 0xC40C]
    set c410 [debug read memory 0xC410]
    set c415 [debug read memory 0xC415]
    set c418 [debug read memory 0xC418]
    set c425 [debug read memory 0xC425]
    set c002 [debug read memory 0xC002]
    set c006 [debug read memory 0xC006]
    set c007 [debug read memory 0xC007]
    set c00b [debug read memory 0xC00B]
    set c00c [debug read memory 0xC00C]
    set c427 [debug read memory 0xC427]
    set o420 [vk_hex 0xC420 80]
    set sat  [vk_hex 0xD600 128]
    set o470 [vk_hex 0xC470 64]
    puts $vk_csv "$vk_frame,[machine_info time],$vk_marker,$c000,$c001,$c003,$c004,$c005,$c40a,$c40c,$c410,$c415,$c418,$c425,$c427,$c002,$c006,$c007,$c00b,$c00c,$o420,$sat,$o470"
    if {($vk_frame % 60) == 0} {flush $vk_csv}
}
proc vk_finish {} {
    global vk_csv vk_output vk_frame
    close $vk_csv
    set f [open [file join $vk_output entities-complete.txt] w]
    puts $f $vk_frame
    close $f
    exit
}
debug set_bp 0x4028 {} {vk_sample}
set vk_title_seen 0
proc vk_begin_game {} {
    global vk_title_seen vk_marker
    if {!$vk_title_seen} {
        set vk_title_seen 1
        set vk_marker title_start
        after time 0.2 {keymatrixdown 8 1}
        after time 0.4 {keymatrixup 8 1}
        after time 0.5 {set vk_marker gameplay_idle}
    }
}
debug set_bp 0x4194 {} {vk_begin_game}
after time $::vk_seconds {vk_finish}
