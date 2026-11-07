# Inventaire des 12 effets demandés — audit de portage, 2026-11-07

Source : ROM vérifiée par `probe.load_rom`, annotations du dépôt, sources C. Les preuves de comportement des stages tardifs manquent ; « porté » ne signifie pas validation de ces stages.

| Objet demandé | ID / ROM | Code C / effet effectivement présent | Lacune et statut |
|---|---|---|---|
| Bottes (vitesse) | `0Ch`, `8DF5–8E03`, `6C44/6C67` | `candles.c:vk_candles_give` fixe `C431` bit 3 ; `gameplay.c:vk_game_hit` emploie `VK_ITEM_FAST_RECOIL`, soit le recul accéléré | **Partiel / dénomination à confirmer** : ce bit accélère le recul, pas la marche ; ne pas annoncer une vitesse de déplacement portée |
| Aile (saut) | `0Dh`, `8DF5–8E03`, `6CE7/6D94` | `candles.c:vk_candles_give` ; `gameplay.c:vk_game_tick_controls`, `player.c:vk_player_tick_jump`, `VK_ITEM_HIGH_JUMP` | **Porté pour saut** ; validation tardive non faite |
| Bouclier anti-projectiles | `04h`, `8DA1`, `8610` | `candles.c:vk_candles_give` pose `VK_ITEM_BIT_04` ; `enemies.c:Enemies_item` préserve l'armure | **Partiel** : interception des projectiles `D700h` non portée |
| Bouclier défense / armure | `03h`, `8D98` | `candles.c:vk_candles_give` pose `VK_ITEM_BIT_03` ; `enemies.c:Enemies_item` active `armor` | **Porté en partie**, selon classes de dégâts prises en charge |
| Croix argentée (arrêt apparitions ~10 s) | `06h`, `8D7C` | `enemies.c:Enemies_item`, `spawnPause` ; prise d'objet dans `candles.c` | **Partiel** : minuterie prise en charge, canaux tardifs `40h/80h` non portés ; durée exacte à comparer à trace |
| Carte F2 (usages limités) | `0Fh`, `8E33`, `C70F` | `candles.c:vk_candles_give`, `vk_candles_use_map` (`C70F=3`) | **Partiel** : inventaire/consommation connus ; rendu et effet global F2 à valider |
| Bougie rouge (révèle les blocs) | `1Eh`, `8D8D` | `candles.c:vk_candles_give` pose `VK_ITEM_BIT_1E` | **Partiel** : effet révélateur tardif non vérifié |
| Bible blanche (prix baissés) | `11h`, `8E26`, `9418` | `candles.c:vk_candles_give` pose `C702` bit 7 ; `wall.c:vk_wall_shop_open` utilise la colonne prix réduits | **Porté pour prix existants**, pas de marchands 12–18 |
| Bible noire (prix montés) | `10h`, `8E1D`, `9418` | `candles.c:vk_candles_give` pose `C702` bit 6 ; `wall.c:vk_wall_shop_open` utilise la colonne prix majorés | **Porté pour prix existants**, pas de marchands 12–18 |
| Objet bleu (invincible + tue au contact) | `09h`, `8DE5` | `enemies.c:Enemies_item`, `killTimer` | **Partiel** : vérifier immunité et contacts dans les ennemis tardifs ; ne pas confondre objets 08h/09h |
| Objet d'invincibilité qui traverse | `08h`, `8DCD` | `enemies.c:Enemies_item`, `guardTimer` | **Partiel** : immunité par minuterie ; passage à travers obstacles/ennemis non démontré par les seules affectations |
| Clé rouge (= 3 clés jaunes) | `12h`, `8E79` | `candles.c:vk_candles_give`, `gold_keys=3`, `triple_key=1`, icône HUD | **Porté pour compteur**, comportements de portes tardives à tester |

## Distinctions importantes

- `0Ch` : bit `C431.3`, accélération **du recul après dégât**, et non vitesse de marche observée. Toute affirmation « bottes = vitesse de Simon » nécessite une autre trace.
- `08h` et `09h` : `guardTimer` et `killTimer` sont deux modes distincts. Leur immunité exacte demande les tests de collisions en conditions réelles.
- `04h` : présence en inventaire ne prouve pas la protection `D700h` ; le code le documente explicitement comme non portée.
- Les prix sont codés dans `wall.c:vk_shop_prices[9][4]`. Les étendre aux stages 12–18 exige la table ROM de chaque marchand et un générateur `--check` ; aucune ligne n'est inventée.
- Aucun fichier de branchement interdit n'a été modifié lors de cet audit.
