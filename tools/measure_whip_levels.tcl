# Mesure des niveaux de fouet MSX2 (lecture seule, la ROM n'est pas modifiee).
# Variables fournies par tools/measure_whip_levels.py : vk_output (dossier), vk_level (C416 force, -1 = aucun),
# vk_end (secondes). Chaque tick VBLANK (0x4028) : une ligne dans ticks.csv, 128 octets de SAT et 2 Ko de motifs
# de sprites (VRAM 0xF800) dans sat-pat.bin, 14 registres PSG dans psg.csv.
set throttle off
set pause off
set power on
set vk_tick 0
set vk_mark "idle"
set vk_csv [open [file join $vk_output ticks.csv] w]
puts $vk_csv "tick,mark,c000,c42e,c42f,c427,c425,c416,c436,c096,c016,c007"
set vk_psg [open [file join $vk_output psg.csv] w]
puts $vk_psg "tick,r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13"
set vk_bin [open [file join $vk_output sat-pat.bin] w]
fconfigure $vk_bin -translation binary
proc vk_sample {} {
    global vk_tick vk_mark vk_csv vk_psg vk_bin
    incr vk_tick
    set v {}
    foreach a {0xC000 0xC42E 0xC42F 0xC427 0xC425 0xC416 0xC436 0xC096 0xC016 0xC007} {
        lappend v [debug read memory $a]
    }
    puts $vk_csv "$vk_tick,$vk_mark,[join $v ,]"
    set p {}
    for {set r 0} {$r <= 13} {incr r} { lappend p [debug read "PSG regs" $r] }
    puts $vk_psg "$vk_tick,[join $p ,]"
    puts -nonewline $vk_bin [debug read_block VRAM 0xF600 128]
    puts -nonewline $vk_bin [debug read_block VRAM 0xF800 2048]
}
proc vk_mark {m} { global vk_mark; set vk_mark $m }
debug set_bp 0x4028 {} {vk_sample}
debug set_bp 0x4194 {} {after time 0.2 {keymatrixdown 8 1}; after time 0.4 {keymatrixup 8 1}}
# Niveau force par ecriture de C416 (copie ensuite dans C436 par le jeu).
if {$vk_level >= 0} {
    after time 41.0 "debug write memory 0xC416 $vk_level; vk_mark level$vk_level"
}
# Reveil du jeu (intro) comme dans run_trace.py.
after time 40.0 { keymatrixdown 8 1 }
after time 40.15 { keymatrixup 8 1 }
# Trois coups debout face a droite (SPACE = 1).
after time 42.0 { vk_mark stand-r1; keymatrixdown 8 1 }
after time 42.15 { keymatrixup 8 1; vk_mark idle }
after time 43.5 { vk_mark stand-r2; keymatrixdown 8 1 }
after time 43.65 { keymatrixup 8 1; vk_mark idle }
after time 45.0 { vk_mark stand-r3; keymatrixdown 8 1 }
after time 45.15 { keymatrixup 8 1; vk_mark idle }
# Accroupi (DOWN+SPACE = 65), face a droite.
after time 46.5 { vk_mark crouch-r1; keymatrixdown 8 65 }
after time 46.75 { keymatrixup 8 65; vk_mark idle }
# Marche a gauche (LEFT = 16) puis deux coups debout face a gauche.
after time 47.5 { keymatrixdown 8 16 }
after time 48.0 { keymatrixup 8 16 }
after time 48.2 { vk_mark stand-l1; keymatrixdown 8 1 }
after time 48.35 { keymatrixup 8 1; vk_mark idle }
after time 49.7 { vk_mark stand-l2; keymatrixdown 8 1 }
after time 49.85 { keymatrixup 8 1; vk_mark idle }
# Accroupi face a gauche.
after time 51.2 { vk_mark crouch-l1; keymatrixdown 8 65 }
after time 51.45 { keymatrixup 8 65; vk_mark idle }
# Accroupi face a gauche, second coup.
after time 52.8 { vk_mark crouch-l2; keymatrixdown 8 65 }
after time 53.05 { keymatrixup 8 65; vk_mark idle }
after time [expr {$vk_end}] {
    global vk_csv vk_psg vk_bin vk_output
    close $vk_csv
    close $vk_psg
    close $vk_bin
    set f [open [file join $vk_output done.txt] w]; puts $f ok; close $f
    exit
}
