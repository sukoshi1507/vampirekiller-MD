# Stage 11 : ennemis (zombies, puces 0D et 23h, aigle 0F, dragon 0E et son tir, objet 1B)

Code : `sgdk/src/enemies_stage11.inc` (aigle, puce 23h, canal `9DE7`), `enemies_dragon.inc` (dragon, visée `A0E5`, tir), `enemies_ghost.inc` (objet 1B), `enemies.c` (types `ENEMY_TYPE_EAGLE` 18 et
`ENEMY_TYPE_DRAGON` 19, boule visée de `fireballStep`), `enemies_death.inc` (flamme de l'aigle, quatre flammes du dragon), `actors.c` (tuiles), `tools/build_stage11_enemies_md.py` (tuiles tirées
d'instantanés VRAM), `tools/gen_stage11_vectors.py` et `gen_dragon_vectors.py` (vecteurs : routines ROM dans l'émulateur Z80), tests natifs `tests/test_stage11.c`, `test_dragon.c`, scénario `zzo_stage11_enemies.txt`.
Drapeaux `85A6` (`vk_enemy_room_flags[11]` = 81, 00, 40, 40, 40, 80) ; liste fixe : salles 1 et 2 une puce 0D (144, 80) et (112, 112), salle 5 un objet 0Eh (208, 112). Bit 6 = canal `9DE7` (jamais dispatché avant) ; bit 7 : aucun canal.

## Prouvé

- **Zombies (salle 0, bit 0)** : même canal `9CE6` que les stages 2 et 3 ; `9CFC` donne E = B0h pour D000 = 11 (trace R : bloc 01 en Y B0h). Images des motifs 40 à 6C identiques à `vk_enemy_tiles` (formes égales, couleurs 02 et 44).
- **Aigle 0F et puce 23h** (`B1C3` / `B193`, puis `B248`) : 42 vecteurs, **10 920 passes** (état, X, Y, fractions, pose, vx, vy, mobile, `+C`, `+11` des deux objets) : le port est égal à l'émulateur. Trace R : le premier aigle de la salle 2
  (94 passes, vol, arrêt de 8 passes près de Simon, pose 6D/6E/8D) est égal à la simulation passe par passe ; la puce l'est jusqu'au plongeon (état 2 deux passes, puis 3) ; `+C` initial (aléatoire) différent (R).
  Canal : un aigle toutes les 25 x 4 passes (200 images), Y 30h puis 40h (parité de CF0D), X E0h, rien si Simon est en X >= C0h (CF0C = 1) ; première passe faite à la création (X DDh, puce E1h/3Ch comme la trace).
  Règles lues dans la ROM : 8 points de vie, dégâts 4 (paire 04 02), 400 points, boîte 0Ch x 18h (classe 4), 6 entrées SAT, flamme 18h au-dessus (non tracée). Puce 23h : 1 point de vie, 200 points, comme la 0D ; elle part en état 0, mobile.
  Cellules : trace R (poses 6D : 40, 48, 50 ; 6E : 40, 58, 60 ; 8D : 40, 68, 70 ; demi-pixel de X selon la fraction).
- **Dragon 0E** (`AACD`, `A9B7`, états `A9D8`, `AA0B`, `AA37`) : 10 vecteurs, **4 000 passes** (état, `+C`, `+18`, `+19`, huit cibles, huit entrées SAT, tir) et **432 visées `A0E5`** : 0 écart. Le tir (D700 type 1, `9F6D`) en (X de la pièce 0, Y + 12),
  vitesses 16 bits visées sur Simon (vx < 0 seulement), mêmes sites que le tir du stage 10 (boule F0 couleur 8, SAT (X - 4, Y - 13), trace R5). Trace R5 : la tête glisse de 208 à 160 (boîte X 168), le cou en quatre pièces. 12 points de vie, 1000 points, dégâts 6.
  Mort : quatre flammes en (X + 16 k, Y), chacune avec un petit coeur (`9AF3`, lu dans la ROM, non tracé).
- **Objet 1B** (bougies d'objet 15, `D002` = 3) : même routine `B4EA` que 1A et 1C (règles `81CE`, `82CB`, `60E1` identiques, lues). Trace G (salle 0, bougie (160, 144)) : créé à 865 en (160, 160), poses 9B/9C (motifs D0/D8, D4/DC),
  couleurs 8 et 2 (CC) ; trace G5 (salle 5) : pose A1 (A0 + D8, A4 + DC). Bases `B681` : 9B, A1, -, -, -, A1. Le comportement (déjà prouvé pour 1A et 1C) n'a pas de vecteur propre au stage 11.

## Écarts et non porté

- **Objet 03 (stage 10), flamme de mort** : `9B33[03]` = 10h dans la ROM, le port met 0 pour le type FISH3 (`deathOffset`) ; lu, non tracé, non corrigé (hors stage 11).
- Flammes de l'aigle et du dragon, dégâts de contact, fouet sur l'aigle : lus dans la ROM, non mesurés. `+C` aléatoire des objets 0F/23h (R) non reproductible.
- Gigue d'un pixel en X (boule du dragon, aigle) partiellement reproduite. Pas de scénario du coup de fouet sur l'objet 1B.
