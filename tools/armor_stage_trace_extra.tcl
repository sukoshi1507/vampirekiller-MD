# Armure (objet 03) au changement de stage, a ajouter apres tools/vertical_links_trace.tcl
# (lance par tools/run_armor_stage_trace.py). armor.csv : chaque ecriture de C441, C701 et
# C415 avec le PC, puis C441, C701, C415, C420, D000, D001, C427, C42C.
proc key_snap {label} {}
set ar_w [open [file join $vk_out armor.csv] w]
puts $ar_w "frame,pc,addr,val,c441,c701,c415,c420,d000,d001,x,c42c"
proc ar_wlog {a} {
    global ar_w vk_frame
    puts $ar_w "$vk_frame,[vk_pc],$a,[vk_rd 0x$a],[vk_rd 0xC441],[vk_rd 0xC701],[vk_rd 0xC415],[vk_rd 0xC420],[vk_rd 0xD000],[vk_rd 0xD001],[vk_rd 0xC427],[vk_rd 0xC42C]"
    flush $ar_w
}
foreach a {C441 C701 C415} {
    debug set_watchpoint write_mem [list 0x$a] {} [list ar_wlog $a]
}
