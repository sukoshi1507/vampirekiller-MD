# Ajout a vertical_links_trace.tcl pour la descente par le haut (BAS).
# Journalise chaque ecriture de C420 et C421 avec le PC de l'instruction qui ecrit.
proc vk_wlog_mode {addr name} {
    global vk_frame vk_wcsv vk_held
    set v [vk_rd $addr]
    puts $vk_wcsv "$vk_frame,[machine_info time],[vk_pc],$name,$v,[vk_rd 0xD000],[vk_rd 0xD001],[vk_rd 0xC000],[vk_rd 0xC420],[vk_rd 0xC41B],[vk_rd 0xC427],[vk_rd 0xC425],[vk_rd 0xC41C],[vk_rd 0xC41D],[vk_rd 0xC41E],[vk_rd 0xC41F],[vk_rd 0xC42C],$vk_held"
    flush $vk_wcsv
}
proc vk_w420 {} { vk_wlog_mode 0xC420 C420 }
proc vk_w421 {} { vk_wlog_mode 0xC421 C421 }
debug set_watchpoint write_mem {0xC420} {} {vk_w420}
debug set_watchpoint write_mem {0xC421} {} {vk_w421}
# Compteur de pas de l'escalier (C42B) : ecriture journalisee avec le PC.
proc vk_w42b {} { vk_wlog_mode 0xC42B C42B }
debug set_watchpoint write_mem {0xC42B} {} {vk_w42b}
