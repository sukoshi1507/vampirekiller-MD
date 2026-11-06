# Stage 6 : chauve-souris 07, statue 0A et sa boule, pics du plafond, objets de salle

Code : `sgdk/src/enemies.c` (`batStep`, `statueStep`, `fireballStep`, `type8Channel`), `sgdk/src/crushers.c` (pics, C580),
`actors.c` (tuiles, sprites, contact des pics). Tuiles : `tools/build_stage6_enemies_md.py`, sur des instantanés VRAM
d'une trace openMSX non versionnée (`scratchpad/stage6/out/R2`). Bougies, blocs, clés dorées et coffres :
`tools/build_candles_md.py` (`stage > 6` aux trois endroits, `WALL_STAGES = 7`). Tests : `tests/test_crushers.c`,
`tests/enemy_integration.c`, scénario `tests/md_scenarios/zx_stage6_enemies.txt`.

Forcer le stage 6 dans openMSX : `D000` = 6, `D002` = 1 (table `5E6A`, octet de fichier `1E6A`, indice 6 : 1 ; la valeur 2
ne vaut qu'à partir du stage 7), `D001` = salle, `C000` = 4. Le stage a 6 salles (0 à 5) ; D001 = 6 ou 7 charge des
restes sans objet.

## Objets vus (trace, 500 images par salle, Simon immobile ; salle 5 : 480 images)

| Salle | Drapeau (85A6) | Objets | Fixes |
|---|---|---|---|
| 0 | 10 | 07 (canal `9DC3`) | clé 17 (112, 96) |
| 1 | 00 | trois pics `C580` (`8F9A`) | clé 17 (192, 144) |
| 2 | 00 | 0A en (192, 80), boules D700 de type 3 | clé 17 (64, 64), coffre 19 (32, 160), contenu 1A |
| 3 | A0 | 08 (canal `9DD5`) | clé 17 (208, 144) |
| 4 | 20 | 08 | - |
| 5 | 00 | script de boss (CE01 de 0 à 3), objet 13h (porté, `research/stage6-boss.md`) | - |

Les clés et coffres (4 objets 0x17, 1 objet 0x19) sont dans `candles_md.c`. Le bit 7 du drapeau (salle 3) ne montre rien
en trace, comme aux stages 4 et 5. Aucun objet 06 au stage 6.

## Prouvé (ROM et trace)

**Objet 07, chauve-souris** (init `B061`, pas `B07B`, pose `B0B5`, canal `9DC3`, table `9DCD` : 1C 1C 1C 48 deux fois).
Même code `9D9F`, même garde `9E16` que l'objet 08 : `type8Channel` sert aux deux. Elle nait en X 10h ou F0h, Y de
Simon - 8, puis : but revu à la première passe et toutes les 16 passes (`+C` : 16, 15, ...), vy = -100h si Simon + 1 est
au-dessus d'elle, +100h si elle est à plus de 18h au-dessus de Simon + 1, sinon 0 ; vx = 120h vers Simon ; pose 16 + (`+C`
& 4 ? 1 : 0), +2 vers la droite (motifs A0, A8, B0, B8). Points de vie 2 (`+D` en trace).
- Trace R2, Simon (232, 128) : X 17, 18, 19, 20, 21, 22, 23, 25 (fractions comprises) à chaque passe depuis la
  création ; le port donne les mêmes valeurs (scénario, 9 relevés), et le premier contact (degré 2, énergie 32 à 30)
  arrive à X 220, comme la trace (images 1205-1206).
- Trace R2, images 1226 à 1240 : la chauve-souris monte (vy -100h, Simon en Y 112), Y 119 à 113, X 233 à 239 avec leurs
  fractions ; images 1266-1268 : Simon en Y 128, elle repart vers le bas (vy +100h, Y 104 puis 105). Test natif
  (`enemy_integration.c`) : mêmes valeurs, passe par passe.

**Objet 08 aux salles 3 et 4** : même code qu'aux stages 4 et 5 (`A4FB`, `A51B`, tuiles 40, 48, 50, 58 identiques octet
à octet à `stage4_enemies_md.c`). Trace R2, salle 3, Simon (232, 128) : X 18, 21, 23, 26, 28 et Y 116, 112, 109, 106,
103 aux cinq premières passes, identiques en scénario.

**Objet 0A, statue** (init `A222`, pas `A22E`, pose `A2A8`) : immobile, tourne vers Simon (pose 4 à gauche, 5 à droite,
mêmes motifs C0/C8 des deux côtés). Cycle en passes : état 0 pendant `+10` passes (8 au début, puis 28h - 8 x D012), état
1 pendant 18h passes avec les yeux (motif C4/CC) en couleur 8 quand le compteur est impair avant sa baisse, tir à la
dernière, état 2 pendant 12h passes, second tir, retour à l'état 0. Points de vie 8 (`+D` en trace).
- Trace R2 images 1845 à 1944 : 7 passes à `+10` de 7 à 1, état 1 à 1860 (`+11` = 24), tir à 1908, second tir à 1944.
  Test natif : mêmes comptes (32 passes jusqu'au premier tir, 18 jusqu'au second, 40 jusqu'à l'état 1).
- Boule : objet D700 de type 3 (`9F6D`, `A08E`[10]), créée en (X statue, Y statue - 14h), vx +-400h (`+B` bit 0), déplacée
  dans la même passe, 4 px par passe : X 196 (200, 204, ... 240), supprimée à X >= F1 (`99E5`). Trace S3 (Simon à
  gauche) : X 188, 184, ... (vx -400h). Le port suit la trace en scénario (196, 200, 204, 240, puis plus rien).
- Sprites : statue SAT (X - 8, Y - 32) et (X - 8, Y - 16) ; boule motif F0 couleur 8 en (X - 4, Y - 12) ; chauve-souris
  (X - 7, Y - 15).

**Pics du plafond** (`8F9A`, `8FEA`, `85A6`) : trois enregistrements de 8 octets en `C580`, créés tant que `C580` vaut 0
en stage 6 salle 1 (table `8FBD`) : (état 1, Y 60h, X 3Ch, masque 0, compteur 0, limite 0Bh), (1, 60h, 7Ch, 0, 1, 0Ah),
(1, 60h, BCh, 1, 2, 0Bh). À chaque passe : compteur + 1 ; état 1 : Y + 4 sauf si masque & compteur, état 2 : Y - 4 une
passe sur quatre ; à la limite en pas, l'état passe de 1 à 2 ou de 2 à 1. Image 32 x 16 copiée en (X, Y - 4) par `4946`
depuis la page 1 de la VRAM en (80h, 70h) : 4 rangées de chaine, corps, pointes.
- Trace R1 (frames 782 à 880) : Y par passe 64, 68, ... 8C pour le premier (retournement au 11e pas), puis 88 tout de
  suite, 84 quatre passes après ; mêmes cycles en trace R3 (41 passes en montée, 11 en descente). `test_crushers.c`
  reproduit ces valeurs.
- Contact (`85A6`, après le pas, gardes C420 = 6, C42D, C43A) : (C425 - 1Ch - Y) < 8 et (C427 - X) < 20h. Dégât 8 en
  montée (trace S1 : Simon X 70, énergie 32 à 24, Y 84h), 16 en descente (trace S2 : Simon X 196, 32 à 16, Y 80h). Simon
  est repoussé à gauche (`C43C` n'est pas écrit : valeur du dernier coup, 0 au début). Le scénario reproduit le coup de S2.
- Le port dessine les pics en sprites : chaine à partir de Y 5Ch (le haut est dans le décor de la salle), corps 2 cellules
  16 x 16. Chaine en cellules de 16 rangées, la première rognée à 4, 8 ou 12.

**Décor** : les salles 0 à 5 comparées au MSX (instantané VRAM, couleurs MD `(v + 18) // 36`) : environ 1000 pixels
différents par salle, tous dans les sprites (Simon, ennemis, flammes), le HUD et les deux dessins parasites connus en
(64, 144) et (192, 144). Clés, coffre et blocs conformes. Les pics de la salle 1 ont la même géométrie à l'oeil.

## Déduit (code lu, pas mesuré en jeu)

- Chauve-souris : 200 points et dégât 2 (81CE, paire 02 01 doublée), boîte de contact classe 2 (8, 10h) comme l'objet 08 ;
  fouet : 1 ou 2 avec la chaine, `7DE6` comme pour l'objet 06 ; réarmement par `999F` ; recul dans le sens de vx.
- Statue : 300 points et dégât 2 (paire 03 01), boîte (5, 18h) classe 8378 ; elle recule Simon vers la droite (`+A` = 01).
- Boule : boîte (3, 6) `8318` puis `847A`, dégât 1 (`7F3D`, 462B avec B = 1, ni armure ni doublement), supprimée au
  contact ; le fouet (`7F49`) et les armes lancées (`7F79`) la détruisent pour 100 points (`822A`). Le contact boule - Simon
  n'a pas été tracé.
- Sprites des pics : l'image MSX est une copie bitmap, immédiate ; les sprites du port passent par le tampon d'une passe de
  retard des autres sprites (`actors.c`) : un écart d'une passe en affichage, sans effet sur le contact.

## Non porté, non vérifié

- **Boss de la salle 5** : porté, voir `research/stage6-boss.md` (script `CE01`, objet 13h `BD19`, boules, mort, orbe, passage au
  stage 7).
- Les pics suivent `C434` (objet 09) : sur MSX `85A6` ne le lit pas, le port l'impose via `Enemies_contact`.
- Temps arrêté (objet 0A de la liste d'objets, `D010`) : `8FCF` n'a pas été suivi ; le port ne fige pas les pics.
- Fouet et armes lancées sur chauve-souris, statue et boule : testés en natif seulement, pas en jeu ni en trace.
- Statue avec Simon à gauche : trace S3 pour la boule, port non rejoué.
- Clé, porte et sortie du stage 6 : rejouées (trace K et A, scénario `zy_j_stage6_key_door`), voir `research/stage6-boss.md`.

## Anomalies relevées

- Le brief donnait `D002` = 2 pour le stage 6 ; la table `5E6A` dit 1 (voir plus haut).
- Dans une trace, la colonne `room` suit D001 posé à la main : l'instantané final de la salle 4 (`f03387_r4`) montre la salle
  3 (Simon renvoyé), comme `r5` au stage 5. Les instantanés `f03077_r4_*` montrent bien la salle 4.
- Une lecture trop rapide des relevés `C580` (une ligne sur trente) a fait croire un instant que les pics disparaissaient
  après le rechargement de la salle : ils sont recréés à la passe suivante (`c580w.csv`).
- Les deux dessins parasites en (64, 144) et (192, 144) sont toujours là (stages 4 et 5, non traités ici).
