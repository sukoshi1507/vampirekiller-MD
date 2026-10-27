# Spawn des ennemis, salle 1 : recherche et logique observée

Statut : **table de spawn ROM non trouvée**. Ce qui est établi : la logique de mouvement et d'animation des deux premiers ennemis de la salle 1, leurs deux spawns observés, et la structure de la machine d'états qui les pilote. Le module `sgdk/src/enemies.c` reproduit la trace image par image (414 images vérifiées). Il ne lit pas la ROM.

Adresses CPU sauf mention contraire. Référence : ROM `99eabaae...` (zip, jamais committée). Trace : `scratchpad/enemies6/frames.txt` (openMSX, 2217 images, RAM `E000-E47F`, `C420-C5FF`, `D600-D67F`, `C000/C001`, `D000`, `C425/C427`).

## 1. Méthode et limites

- **openMSX n'est pas installé** sur cette machine. La trace existante sert de vérité terrain.
- **Mapper** : ROM de 128 KB, banques de 8 KB (16 banques), mapper Konami. Les écritures en `6000`, `8000`, `A000` choisissent la banque de la fenêtre correspondante ; `4000-5FFF` est fixe à la banque 0. Confirmé par `4028-4033` (`LD ($8000),A` puis `LD ($A000),A`).
- **Émulateur Z80 en Python** (scratchpad, non committé : `scratchpad/work/z80.py`, `msx.py`, `runner.py`). Cœur complet (flags documentés), interruption VBLANK synchrone du hook `4028`, HLE du BIOS (`0141` SNSMAT, `0096` RDPSG, `00D8`, `00D5`, `0138`, `000C`). **Non validé** : la machine d'états du titre diverge de la trace. L'état 1 attend `C004` pendant 256 images (`41A0 : DEC (HL) ; RET NZ`, `C004` remis à 0 en `439F`), alors que la trace réelle est en partie de jeu (`C000=5`) à 24 s. Ce divergence n'est pas résolue ; l'émulateur ne sert donc pas encore à des watchpoints fiables.
- **Ce qui n'a pas été fait** : désassemblage complet de `5C92`, `4257` (état 5) et des routines qui écrivent dans la SAT ; watchpoint sur les écritures SAT en openMSX.

## 2. Prouvé par lecture de la ROM

- **Entrées par manette** : `4BBD` appelle `0093` (WRTPSG, registre 15) puis `0096` (RDPSG, registre 14) : c'est la lecture de la manette 1 via le PSG (`CPL ; AND 3F`), pas la matrice clavier. Le chemin `40C5` (atteint si `E600 ≠ 0`) appelle `SNSMAT` (`0141`) avec `A = 7`.
- **Hook 4028** : `4B9D` (entrées ; teste `C002 & 40`) puis `414D` (dispatch) puis `405B` (remise à zéro de `C005`). `C005` sert de garde contre l'exécution imbriquée (`research/known.asm`).
- **Dispatch `414D`** : `C000` est lu comme index dans la table inline à `4161` (14 entrées, appelée par `CALL 406B`) :

  | C000 | Routine | Remarque |
  |---|---|---|
  | 0 | `417D` | titre ; appelle `624C` ; teste `C422` |
  | 1 | `41A0` | attente `C004` (DEC, RET NZ), puis `CALL 7AE7 ; JP 4249` |
  | 2 | `41AB` | `DJNZ` sur `B = C001` |
  | 3 | `41D1` | démarrage musique (écrit `C000=3` en `43B5`) |
  | 4 | `422C` | décrémente `C410` (BCD), voir `research/mechanics-remaining.md` |
  | 5 | `4257` | partie en cours |
  | 6 | `4294` | |
  | 7 | `42B2` | |
  | 8 | `4324` | |
  | 9 | `4377` | |
  | 10 | `4387` | |
  | 11 | `43DA` | |
  | 12 | `43F0` | |
  | 13 | `4414` | |

- **Transitions du titre** (`4391-43C8`) : `LD (C004),0` conditionnel en `439F`. Chemin `43A1-43BC` : si `B = C000` vaut 0, `DJNZ` saute en `43BA` et écrit `C000=1`. Le chemin `43B5` (écriture de `C000=3` et `C001=B`) n'est atteint que si `E600 = 0`, `A & 30 ≠ 0` et `B = 1`. `A` vient de l'appelant (`624C`).
- **Boucle d'attente** : `40B0-40C3` installe le hook (`FD9F = C3`, `FDA0 = 4028`) puis fait `EI ; JR $`. Le programme principal est donc **inactif et piloté par l'interruption** ; la logique de jeu tourne dans le hook.
- **Banque 0E, fenêtre 8000** : la table à `8000` (`8019, 80E7, 81B3, ...`) pointe vers des flux de carte au même format que `5A49` (paires cellule/attribut, `FE`, `FF`, `00`, `7F` suivi d'un troisième octet). Ce ne sont pas des structures d'ennemi ; la table de spawn n'est pas là, au moins pour la salle 0.

## 3. Observé dans la trace (openMSX, `frames.txt`)

Image absolue `n` ; image relative à la salle = `n - 857` (chargement de la salle 1 : `E000` réécrit en 857-858, Simon x=16).

| n | Rel. | Événement | Mesure |
|---|---|---|---|
| 908 | 51 | **Spawn E1 à droite** (4 entrées SAT s14-s17). Image 908 : `Y=E1`, `X=99/A9` (position provisoire) | |
| 909 | 52 | E1 en `Y=A0/B0`, `X=E5` (229) | motifs haut `40,44`, bas `48,4C` |
| 911-1125 | | E1 avance vers la gauche | +2 px toutes les 2 images à partir de l'âge 3, plus +1 px tous les 16 images (âge 17, 33, ...) ; 229 -> 0 en 216 images |
| 923 | 66 | E1 pose bas `50,54` (pose B) | pose B de l'âge 15, blocs de 8 images |
| 1012 | 155 | **Spawn E2 à gauche** (s18-s21). Image 1012 : `Y=E1`, `X=B8` | |
| 1013 | 156 | E2 en `Y=A0/B0`, `X=0A` (10) | motifs haut `58,5C`, bas `60,64` / `68,6C` |
| 1015-1207 | | E2 avance vers la droite | même loi, +1 px supplémentaire à l'âge 15, 31, ... ; 10 -> 216 |
| 1125 | 268 | E1 à `X=0` (dernière position visible) | |
| 1126 | 269 | E1 masqué (`Y=E0`) | pas de pas à cette image : masquage à l'image suivant la limite |
| 1207 | 350 | E2 à `X=D8` (216) | |
| 1208 | 351 | E2 masqué | idem ; `C000=9` et Simon x=10 à cette image (changement d'écran) |
| 1256 | 399 | E1 réapparaît à `X=00` (gauche), Simon x=40 | hors spawns du premier cycle |
| 1360 | 503 | E2 réapparaît à `X=D8` (droite), Simon x=122 | |
| 1474 / 1564 | | E1 masqué (Simon x=160) / E2 masqué (`C000=9`, x=10) | |
| 1585 | 728 | E1 réapparaît à `X=00` (gauche), Simon x=10 | |
| 1694 | 837 | E1 masqué (Simon x=88) | |

Intervalles de réapparition (déjà notés dans `research/enemies-platforms.md`) : 130, 111, 152 images. Non expliqués.

## 4. Pistes non prouvées pour le déclencheur

- **Compteur `C476`** (et `C486`) : il augmente d'environ 1 toutes les 2 images, il est remis à zéro au changement d'écran. Aux spawns, sa valeur est `0x11` (908), `0x45` (1012), `0x28` (1256), `0x5C` (1360), `0x32` (1585). Ce n'est pas un seuil commun : **pas de déclencheur confirmé**.
- **Changements d'écran** : `C000=9` coïncide avec la disparition d'E2 à 1208 et avec celle à 1564. C'est peut-être un nettoyage des ennemis au changement d'écran ; **non vérifié** (la coïncidence est observée sur 2 cas).
- **Minuterie depuis le chargement** : spawn E1 à +51 et E2 à +155 images. Compatible avec une table de délais, pas prouvé.

## 5. Module et tests produits

- `sgdk/src/enemies.h`, `sgdk/src/enemies.c` : logique pure (pas de SGDK, pas de `main.c`). Structure `Enemy` (actif, sortie, type, phase, x, y, dx, âge), `EnemySpawnDef`, `Enemies_update()`, `Enemies_spawn()`, `Enemies_sprites()`. Données `kEnemyRoom1Spawns` = les deux spawns observés (confiance **observé**, pas une table ROM).
- `tests/test_enemies.c` : trajectoire E1 et E2 (valeurs lues dans la trace), limites de spawn et de visibilité. Compilation : `gcc -std=c99 -Wall -Wextra -Werror -pedantic -I sgdk/src tests/test_enemies.c sgdk/src/enemies.c`.
- Validation contre la trace (pilote C dans le scratchpad, non committé) : 414 images, X et motif bas identiques, sauf les images de spawn 908 et 1012 (position provisoire dans la trace, le module donne la position de spawn) et les apparitions après 1256 (hors données).

## 6. Inconnu (à faire)

1. **La table de spawn ROM** : position, type, nombre, déclencheur, délai. Non trouvée.
2. **Le déclencheur** : minuterie, compteur, ou événement de salle. Le candidat `C476` n'est pas confirmé.
3. **Le cycle complet** après le changement d'écran (1256, 1360, 1474, 1585, 1694) : relation avec les spawns du premier cycle.
4. **Géométrie** : les 4 entrées SAT partagent le même X ; taille réelle du sprite (16x32 ou 8x8 empilés) et boîte de contact non vérifiées.
5. **Provenance du décalage +1 px** aux images 16k+925 (E1) et 16k+1027 (E2) : cause inconnue.

Prochaine étape recommandée : une trace openMSX (ou un émulateur corrigé) avec watchpoints sur `D638-D647` (SAT de E1 à 908) et `D648-D65F` (E2 à 1012), pour obtenir le PC des écrivains. Le PC donne la routine de spawn et sa lecture de table. C'est la voie la plus courte vers la table.
