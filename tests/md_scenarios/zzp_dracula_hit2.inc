# Test fixture: reduce boss energy and initialise a real active whip, no opcode patches.
poke health+1 0
poke health+2 0 16
when not(ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0) and ram8('dracula+544') == 0 :: poke vk_game+0 16
when not(ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0) and ram8('dracula+544') == 0 :: poke vk_game+1 160
when not(ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0) and ram8('dracula+544') == 0 :: poke vk_game+2 0
when not(ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0) and ram8('dracula+544') == 0 :: poke vk_game+3 0
when not(ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0) and ram8('dracula+544') == 0 :: poke vk_game+4 0
when not(ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0) and ram8('dracula+544') == 0 :: poke vk_game+26 0
when not(ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0) and ram8('dracula+544') == 0 :: poke vk_game+30 0
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: write vk_game+0 8 (ram8('dracula+5')-20)%256
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: write vk_game+1 8 (ram8('dracula+3')-0)%256
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: poke vk_game+2 0
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: poke vk_game+3 0
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: poke vk_game+4 0
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: poke vk_game+26 0
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: poke vk_game+30 0
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: poke dracula+608 4
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: poke vk_whip 12
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: poke vk_whip+1 0
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: poke vk_whip+2 0
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: write vk_whip+3 8 ram8('vk_game')
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: write vk_whip+4 8 ram8('vk_game+1')
when ram8('dracula') == 23 and ram8('dracula+14') & 1 != 0 :: poke vk_whip+5 0
poke Enemies_god 1
wait 1
