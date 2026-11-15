# Generateurs et ouverture de porte (tools/run_door_respawn_trace.py), a ajouter apres tools/vertical_links_trace.tcl.
# door.csv : a chaque image (4028), etat de jeu, premier octet de chaque bloc objet C800..CF80 (pas 80h) et de D700..D770.
# spawn.csv : chaque ecriture du premier octet d'un bloc objet, avec le PC de l'ecrivain.
proc key_snap {label} {}
set dr_w [open [file join $vk_out door.csv] w]
puts $dr_w "frame,c000,d000,d001,c5ac,c420,x,y,c701,blocs_C800_CF80,tirs_D700"
proc dr_frame {} {
    global dr_w vk_frame
    set b ""
    for {set a 0xC800} {$a < 0xD000} {incr a 0x80} { append b [format %02X [vk_rd $a]] " " }
    set t ""
    for {set a 0xD700} {$a < 0xD780} {incr a 0x10} { append t [format %02X [vk_rd $a]] " " }
    puts $dr_w "$vk_frame,[vk_rd 0xC000],[vk_rd 0xD000],[vk_rd 0xD001],[vk_rd 0xC5AC],[vk_rd 0xC420],[vk_rd 0xC427],[vk_rd 0xC425],[vk_rd 0xC701],$b,$t"
    if {$vk_frame % 100 == 0} { flush $dr_w }
}
debug set_bp 0x4028 {} {dr_frame}
set dr_s [open [file join $vk_out spawn.csv] w]
puts $dr_s "frame,pc,addr,val,c5ac,c5ae"
proc dr_spawn_w {a} {
    global dr_s vk_frame
    puts $dr_s "$vk_frame,[vk_pc],$a,[vk_rd 0x$a],[vk_rd 0xC5AC],[vk_rd 0xC5AE]"
    flush $dr_s
}
for {set a 0xC800} {$a < 0xD000} {incr a 0x80} {
    debug set_watchpoint write_mem [list $a] {} [list dr_spawn_w [format %04X $a]]
}
