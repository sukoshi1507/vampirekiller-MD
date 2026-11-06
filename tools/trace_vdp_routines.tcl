# openMSX breakpoints on the bank-0 VDP routines found in research/z80-vdp-io-analysis.md.
# Usage: set vk_output {/absolute/output/dir} before this file, then run openMSX with
# -control stdio -machine C-BIOS_MSX2 -cart <rom> -script <file>.
# The logged C value is meaningful only at routine entries where C is set by the routine
# itself (cmd_setup_9B at 4917, data_loop_98 at 4698, vram_addr_set at 46BB).
set vdpf [open [file join $vk_output vdp-trace.txt] w]
proc vk_vdp_log {tag} {
    global vdpf
    puts $vdpf "[machine_info time] $tag A=[debug read {CPU regs} 0] C=[debug read {CPU regs} 3] B=[debug read {CPU regs} 2]"
}
# CPU regs indexes (openMSX 21): 0=A 1=F 2=B 3=C 4=D 5=E 6=H 7=L.
debug set_bp 0x4855 {} {vk_vdp_log status_read}
debug set_bp 0x4917 {} {vk_vdp_log cmd_setup_9B}
debug set_bp 0x4698 {} {vk_vdp_log data_loop_98}
debug set_bp 0x46BB {} {vk_vdp_log vram_addr_set}
debug set_bp 0x4875 {} {vk_vdp_log cmd_setup_alt}
