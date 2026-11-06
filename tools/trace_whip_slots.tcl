# Mesure des emplacements de fouet (C450, C460) pendant une serie de coups espaces.
# Lecture seule : la ROM n'est pas modifiee. Variables fournies par tools/measure_whip_slots.py :
# vk_output (dossier), vk_level (C416 force, -1 = aucun), vk_end (secondes).
# Chaque tick VBLANK (0x4028) : une ligne dans slots.csv avec l'etat d'attaque (C420, C422, C429, C42C,
# C42E, C42F, C436), les 16 premiers octets de chaque emplacement (C450..C45F, C460..C46F), C096, C016,
# C006, C007 et la position de Simon (C427, C425).
# Protocole : par direction (droite, gauche) et par posture (debout = SPACE, accroupi = DOWN+SPACE),
# une serie de coups separes par des intervalles variables (0,25 a 3 s), pour mesurer la fenetre de blocage.
set throttle off
set pause off
set power on
set vk_tick 0
set vk_mark "idle"
set vk_csv [open [file join $vk_output slots.csv] w]
set vk_cols {C420 C422 C429 C42C C42E C42F C436}
for {set a 0xC450} {$a <= 0xC45F} {incr a} { lappend vk_cols [format "%X" $a] }
for {set a 0xC460} {$a <= 0xC46F} {incr a} { lappend vk_cols [format "%X" $a] }
lappend vk_cols C096 C016 C006 C007 C427 C425
puts $vk_csv "tick,mark,[join $vk_cols ,]"
proc vk_sample {} {
    global vk_tick vk_mark vk_csv
    incr vk_tick
    set v {}
    foreach a {0xC420 0xC422 0xC429 0xC42C 0xC42E 0xC42F 0xC436} { lappend v [debug read memory $a] }
    for {set a 0xC450} {$a <= 0xC45F} {incr a} { lappend v [debug read memory $a] }
    for {set a 0xC460} {$a <= 0xC46F} {incr a} { lappend v [debug read memory $a] }
    foreach a {0xC096 0xC016 0xC006 0xC007 0xC427 0xC425} { lappend v [debug read memory $a] }
    puts $vk_csv "$vk_tick,$vk_mark,[join $v ,]"
}
proc vk_mark {m} { global vk_mark; set vk_mark $m }
debug set_bp 0x4028 {} {vk_sample}
debug set_bp 0x4194 {} {after time 0.2 {keymatrixdown 8 1}; after time 0.4 {keymatrixup 8 1}}
# Niveau force par ecriture de C416 (copiee ensuite dans C436 par le jeu).
if {$vk_level >= 0} {
    after time 41.0 "debug write memory 0xC416 $vk_level; vk_mark level$vk_level"
}
# Reveil du jeu (intro), comme dans measure_whip_levels.tcl.
after time 40.0 { keymatrixdown 8 1 }
after time 40.15 { keymatrixup 8 1 }
# Serie de coups : pour chaque mode (mask, nom), intervalles G (secondes entre le debut de deux coups).
# SPACE = 1, LEFT = 16, DOWN = 64. Maintien de 0,15 s pour SPACE.
set vk_gaps {1.5 1.0 0.5 0.25 2.0 0.75 1.25 3.0}
set vk_t 42.0
proc vk_series {name mask} {
    global vk_gaps vk_t
    set k 0
    foreach g $vk_gaps {
        incr k
        set t $vk_t
        set m [format "%s-%d-gap%s" $name $k $g]
        after time $t "vk_mark {$m}; keymatrixdown 8 $mask"
        after time [expr {$t + 0.15}] "keymatrixup 8 $mask; vk_mark {idle}"
        set vk_t [expr {$vk_t + $g}]
    }
    set vk_t [expr {$vk_t + 1.0}]
}
vk_series stand-R 1
vk_series crouch-R 65
# Marche a gauche (LEFT maintenu 0,6 s), puis les memes series face a gauche.
after time [expr {$vk_t}] { vk_mark walk-L; keymatrixdown 8 16 }
after time [expr {$vk_t + 0.6}] { keymatrixup 8 16; vk_mark idle }
set vk_t [expr {$vk_t + 1.5}]
vk_series stand-L 1
vk_series crouch-L 65
after time [expr {$vk_end}] {
    global vk_csv vk_output
    close $vk_csv
    set f [open [file join $vk_output done.txt] w]; puts $f ok; close $f
    exit
}
