# Sélection des animations de Simon — audit statique Z80

ROM MSX2 : SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`.

## Mécanisme confirmé par les octets Z80

Les **deux moitiés du personnage ont des index indépendants** : `C42E` sélectionne les motifs de la partie basse (table `A281`), `C42F` ceux de la partie haute (`A2D1`). La routine de rendu en banque 0 à `56E1` passe par ces tables; le positionnement emploie les tables `7985` et `79D5` en banque 1. Les compositions précédemment exportées avec `(index,index)` ne sont donc **pas nécessairement des images réelles du jeu** : des associations croisées sont possibles et même déjà attestées pour les poses de marche et de saut documentées dans `research/player-format.md`.

### Éléments de séquence retrouvés

- `6F0B–6F22` et `6F23–6F3B` (banque 1) : deux chemins incrémentent les index bas/haut, suivant des valeurs dérivées de `C42B`. **Transitions relatives observées**, mais pas de chronologie complète sans trace d'exécution.
- `7200–720B` : sélection de l'index inférieur **0 ou 6** selon `C420`.
- `7214–7220` : sélection de l'index supérieur **6 ou 9** selon `C436`.
- `7229–7233` : sélection directe de l'index supérieur **12**.
- `723F–7242` : incrément de l'index supérieur en mémoire (`INC (C42F)` après `LD HL,C42F`).
- `7275–7284` : sélection de l'index supérieur **0 ou 2** selon `C420`.
- `6FB0–6FB8` : sélection explicite de l'index supérieur **2**.
- `7660–7676` : opération simultanée sur l'index inférieur/supérieur (HL chargé depuis `C42E`), ajout conditionnel `+0A` (bas) et `+0F` (haut). **Candidat fort pour changement d'orientation graphique**, pas preuve de cadence temporelle.

Ces opérations confirment un **enchaînement piloté par l'état**, distinct d'une simple boucle `06,07,08,...`. Il serait incorrect de déclarer que la séquence authentique de fouet suit les index 6 à 11, ou de programmer des durées d'image en se fondant seulement sur leur apparence.

## Séquences d'usage dans le portage : éléments établis et limites

| Action | Index lower/upper | Confiance |
|---|---|---|
| Marche droite | `0/0`, `1/1`, `2/2` | déjà documenté dans le portage; cadence complète MSX non tracée |
| Marche gauche | `10/15`, `11/16`, `12/17` | déjà documenté dans le portage; cadence complète MSX non tracée |
| Saut droite | `6/0` | paire documentée, sans preuve des autres états du saut |
| Saut gauche | `16/15` | paire documentée, sans preuve des autres états du saut |
| Attaque / fouet | sélecteur supérieur inclut des valeurs `6`, `9`, `12` sur un chemin d'état; liaison exacte à la touche attaque à tracer | hypothèse fonctionnelle, **non certifiée** |
| Escalier | non déterminé | non identifié |

## Étape nécessaire pour compléter les séquences authentiques

Il faut tracer dynamiquement les écritures de `C42E`, `C42F` ainsi que `C420`, `C436` et `C42B` à chaque interruption, avec l'état des entrées et les banques chargées, sur une émulation MSX2 du jeu original. Le relevé attendu doit enregistrer : *tick, bouton, PC Z80, banque, C42E, C42F, position X/Y et état*. Il permettra de déterminer l'ordre réel, la durée et les transitions d'attaque et d'escalier, sans les deviner.

Le fichier `animation-selector-audit.json` et le script `tools/trace_simon_animation_indices.py` fournissent les références exactes des écritures statiques repérées. **Le gameplay SGDK n'a pas été modifié** dans cette livraison.
