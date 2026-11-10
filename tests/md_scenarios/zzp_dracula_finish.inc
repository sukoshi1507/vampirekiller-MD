# Isolated end proof after the real stage18 door; no full-health fight claim.
poke vk_enemies+162 0
set before_final_score ram32('vk_hud_score')
repeat 500 zzp_dracula_hit1.inc
repeat 700 zzp_dracula_pin.inc
expect ram8('dracula') == 23
repeat 500 zzp_dracula_hit2.inc
until ram8('dracula+544') == 2 and ram8('dracula+529') > 0 max 4000
wait 320
shot zzp18-credits
until front_mode == MODES['COURTYARD'] max 16000
expect ram32('vk_hud_score') >= before_final_score+80000
shot zzp18-ending-courtyard
