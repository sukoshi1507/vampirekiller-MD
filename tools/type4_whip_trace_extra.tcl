# Fouet et contact du type 4 (chauve-souris), a ajouter apres tools/vertical_links_trace.tcl
# (lance par tools/run_type4_whip_trace.py). t4.csv : entree de 84A7 (box : D, E = X, Y de
# l'objet, L, H de la classe), 7DD4 (hit : le fouet a touche l'objet IX), 847A (contact), 4638
# (energie), avec C427, C425, C42C, C420, C416, C422, C415.
proc key_snap {label} {}
set t4_w [open [file join $vk_out t4.csv] w]
puts $t4_w "frame,ev,pc,ixh,ixl,type,objy,objx,d,e,l,h,c427,c425,c42c,c420,c416,c422,c415"
proc t4_reg {i} { return [debug read {CPU regs} $i] }
proc t4_log {ev} {
    global t4_w vk_frame
    # CPU regs: 0 A,1 F,2 B,3 C,4 D,5 E,6 H,7 L,... IX at 16/17 (IXH, IXL)
    set ixh [t4_reg 16]; set ixl [t4_reg 17]
    set ix [expr {$ixh * 256 + $ixl}]
    set ty [debug read memory $ix]
    set oy [debug read memory [expr {$ix + 3}]]
    set ox [debug read memory [expr {$ix + 5}]]
    puts $t4_w "$vk_frame,$ev,[vk_pc],$ixh,$ixl,$ty,$oy,$ox,[t4_reg 4],[t4_reg 5],[t4_reg 7],[t4_reg 6],[vk_rd 0xC427],[vk_rd 0xC425],[vk_rd 0xC42C],[vk_rd 0xC420],[vk_rd 0xC416],[vk_rd 0xC422],[vk_rd 0xC415]"
    flush $t4_w
}
# 84A7: whip box entry (D = X, E = Y of the object, L, H of the class).
debug set_bp 0x84A7 {} {t4_log box}
# 7DD4: the whip touched an object (C800 loop of 7DAD).
debug set_bp 0x7DD4 {} {t4_log hit}
# 847A: contact box entry (class params in H, L).
debug set_bp 0x847A {} {t4_log contact}
# 4638: energy lowered by a contact.
debug set_bp 0x4638 {} {t4_log energy}
