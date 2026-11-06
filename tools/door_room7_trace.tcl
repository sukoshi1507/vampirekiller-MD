# Ajout a vertical_links_trace.tcl pour la porte de la salle 7 du hall (stage 1).
# Journalise les ecritures de C408 (sortie d'etat 10), C5AC, C5AE, C41C-C41F, C000, C41B
# avec le PC de l'ecrivain, et un etat par image dans frames_door.csv.
set vk_dcsv [open [file join $vk_out frames_door.csv] w]
puts $vk_dcsv "frame,time,d000,d001,c000,c408,c409,c41b,c41c,c41d,c41e,c41f,c5ac,c5ae,x,y,c420,c421,held"
proc vk_wlog_door {addr name} {
    global vk_frame vk_wcsv vk_held
    set v [vk_rd $addr]
    puts $vk_wcsv "$vk_frame,[machine_info time],[vk_pc],$name,$v,[vk_rd 0xD000],[vk_rd 0xD001],[vk_rd 0xC000],[vk_rd 0xC420],[vk_rd 0xC41B],[vk_rd 0xC427],[vk_rd 0xC425],[vk_rd 0xC41C],[vk_rd 0xC41D],[vk_rd 0xC41E],[vk_rd 0xC41F],[vk_rd 0xC42C],$vk_held"
    flush $vk_wcsv
}
proc vk_wd_c408 {} { vk_wlog_door 0xC408 C408 }
proc vk_wd_c5ac {} { vk_wlog_door 0xC5AC C5AC }
proc vk_wd_c5ae {} { vk_wlog_door 0xC5AE C5AE }
proc vk_wd_c41c {} { vk_wlog_door 0xC41C C41C }
proc vk_wd_c41d {} { vk_wlog_door 0xC41D C41D }
proc vk_wd_c41e {} { vk_wlog_door 0xC41E C41E }
proc vk_wd_c41f {} { vk_wlog_door 0xC41F C41F }
proc vk_wd_c000 {} { vk_wlog_door 0xC000 C000 }
debug set_watchpoint write_mem {0xC408} {} {vk_wd_c408}
debug set_watchpoint write_mem {0xC5AC} {} {vk_wd_c5ac}
debug set_watchpoint write_mem {0xC5AE} {} {vk_wd_c5ae}
debug set_watchpoint write_mem {0xC41C} {} {vk_wd_c41c}
debug set_watchpoint write_mem {0xC41D} {} {vk_wd_c41d}
debug set_watchpoint write_mem {0xC41E} {} {vk_wd_c41e}
debug set_watchpoint write_mem {0xC41F} {} {vk_wd_c41f}
debug set_watchpoint write_mem {0xC000} {} {vk_wd_c000}
proc vk_door_frame {} {
    global vk_dcsv vk_frame vk_held
    set s [vk_rd 0xC000]
    puts $vk_dcsv "$vk_frame,[machine_info time],[vk_rd 0xD000],[vk_rd 0xD001],$s,[vk_rd 0xC408],[vk_rd 0xC409],[vk_rd 0xC41B],[vk_rd 0xC41C],[vk_rd 0xC41D],[vk_rd 0xC41E],[vk_rd 0xC41F],[vk_rd 0xC5AC],[vk_rd 0xC5AE],[vk_rd 0xC427],[vk_rd 0xC425],[vk_rd 0xC420],[vk_rd 0xC421],$vk_held"
    if {$vk_frame % 200 == 0} { flush $vk_dcsv }
}
debug set_bp 0x4028 {} {vk_door_frame}
# Vidage de la carte de salle courante (tampon D100, 768 octets) : a chaque changement de
# (D000, D001) et a chaque image multiple de 1000 (fichiers room_<tag>.bin, binaire).
set vk_dump_done {}
set vk_dump_key ""
proc vk_dump_room {tag} {
    global vk_out
    set data [debug read_block memory 0xD100 768]
    set f [open [file join $vk_out "room_$tag.bin"] w]
    fconfigure $f -translation binary -encoding binary
    puts -nonewline $f $data
    close $f
}
proc vk_door_dump {} {
    global vk_frame vk_dump_done vk_dump_key
    set key "[vk_rd 0xD000]_[vk_rd 0xD001]"
    if {$key ne $vk_dump_key && [vk_rd 0xC000] == 5} {
        set vk_dump_key $key
        vk_dump_room s$key\_f$vk_frame
    }
}
debug set_bp 0x4028 {} {vk_door_dump}
