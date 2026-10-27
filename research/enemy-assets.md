# Ennemis de la salle 1 : graphismes et animation (état)

Statut : **extraction des tuiles non réussie**. Ce document consigne ce qui est prouvé
(chronologie et géométrie SAT), ce qui est déduit, et le blocage restant. Aucun PNG
d'ennemi n'est produit : aucun motif n'a pu être relié à une source ROM, et un PNG non
vérifié serait un asset « candidate » sans preuve.

ROM de référence : SHA-256 `99eabaae...`, chargée via `tools/probe.py`. Adresses CPU
sauf mention contraire.

## 1. Prouvé (trace openMSX, RAM SAT `D600-D67F`)

Source : trace VBLANK (4028), entrées SPACE puis RIGHT, identique à `research/enemies-platforms.md`
(1.2). Fichier brut hors dépôt. Script de lecture : `tools/extract_enemy_timeline.py`.

| Ennemi | Première image | Position | Motifs (haut / bas) |
|---|---|---|---|
| 1 | 908 | X départ `E5`, Y `A0/B0` | haut `40/44` fixe ; bas `48/4C` ↔ `50/54` |
| 2 | 1012 | X départ `0A`, Y `A0/B0` | haut `58/5C` fixe ; bas `68/6C` ↔ `60/64` |

Chronologie (images VBLANK, 60 Hz) de l'ennemi 1 : `909-922` pose `48/4C` (14 images,
spawn compris), puis alternance de 8 images : `50/54` (923-930), `48/4C` (931-938), etc.
Le script `tools/extract_enemy_timeline.py` reproduit cette séquence.

Géométrie SAT : chaque partie du corps est un couple de sprites 16x16 de même X et de même
Y (N et N+4), ce qui suit le même schéma que Simon (couches de 16x16 par partie du corps,
`research/player-format.md`). Ennemi complet : 16 px de large, 32 px de haut
(Y `A0` puis `B0`). Les entrées sont à l'ordre SAT 14-17 (ennemi 1) et 18-21 (ennemi 2).

Vitesse : -1 px par image pour l'ennemi 1 (mesure de `enemies-platforms.md`, 1.2).

## 2. Prouvé mais partiel (VRAM, openMSX, images 900-1060)

- Registres VDP (identiques entre 1000 et 1060) : R0=6, R1=98 (sprites 16x16), R5=239
  (SAT à `F600`), R6=31 (motifs de sprites à `F800`), R11=1.
- Palette VDP lue par `debug read "VDP palette"` : 16 entrées au format ROM (octet 0 =
  `R<<4|B`, octet 1 = `G`), niveaux 0-7. Le format et l'ordre sont cohérents avec
  `tools/extract_rooms.py` (`palette_patch`). Palette de l'image 1030 enregistrée dans
  `scratchpad/enemy-assets/cap1/f1030.txt`.
- Le SAT en VRAM est une **rotation** du SAT RAM (ordre des entrées décalé d'une image sur
  l'autre). Les entrées ennemies sont identiques en contenu. Les entrées de Simon diffèrent
  (VRAM : Y=`DE`, masqué ; RAM : Y=`A2`). Cette différence **n'est pas expliquée**.

## 3. Bloqué : emplacement des motifs ennemis

- Formule `F800 + (N & FC) * 32` (même que `whip/render.py`) : pour N=`40..6C`, l'adresse
  sort de la plage de 64 Ko et tombe en zone vide (`10000`) ; avec masquage 16 bits,
  l'adresse retombe sur les tuiles de fond (`0000-0600`), ce qui est incohérent.
- Le bloc de VRAM `4800-4AFF` change d'une image à l'autre, mais il contient des tuiles de
  **fond** (grille, statue, mur doré), pas des motifs d'ennemis.
- Aucun motif VRAM ne correspond octet pour octet à une tuile de la ROM (recherche de
  32 octets : 3 correspondances seulement, sans rapport avec les ennemis). Les motifs de
  sprites ne sont donc pas copiés tels quels depuis la ROM : ils doivent être générés à
  partir de masques (comme Simon, `research/player-format.md`) ou décompressés (RLE).
- Capture d'écran : `screenshot -raw` d'openMSX fonctionne avec `SDL_VIDEODRIVER=offscreen`
  (320x240, cadre avec bordure). Le personnage visible à gauche ne correspond pas aux
  positions de la SAT RAM. La calibration pixel à pixel a échoué (au plus 43 pixels
  concordants sur 600). Ce point reste **non résolu** et doit être revu avant de juger
  n'importe quel rendu.

## 4. Déduit (non vérifié)

- Les motifs ennemis sont probablement décodés depuis la banque 0B/0C/0D (même chargeur
  que Simon, `56E1`, table `A281/A2D1`) avec masques 1bpp et couleurs : **non vérifié**.
- Le mode écran est SCREEN 4 (`R0` bits M3/M4 à 1, `R1` bits M1/M2 à 0), sprites 16x16 en
  4 bpp. Le format 4 bpp est déduit de `whip-hitbox.md`, non prouvé pour ces motifs.

## 5. Ce qui manque pour finir

1. Trouver la routine qui écrit les motifs ennemis en VRAM (watchpoint sur le port `98`
   ou sur `VRAM pointer` pendant le chargement de la salle 1, entre les images 839 et 908).
2. Confirmer la formule d'adresse des motifs 16x16 (N ≥ 0x40) en comparant avec un rendu
   de référence, ce qui exige une calibration validée sur Simon d'abord.
3. Générer les PNG indexés 4 bits seulement après cette étape, avec manifeste
   (offset ROM, banque, adresse CPU, palette, statut `candidate` ou `decoded`).

## 6. Fichiers

- `tools/extract_enemy_timeline.py` : chronologie des poses à partir d'une trace RAM (aucune ROM lue).
- Données de travail (hors dépôt) : `scratchpad/enemy-assets/` (captures VRAM, registres,
  palette, scripts d'exploration). Les `.bin` VRAM ne sont pas committés.
