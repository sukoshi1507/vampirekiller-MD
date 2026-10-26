# V4 — Fouet : affichage synchronisé avec l'attaque

## Ajouts

- `vk_whip_phase()` retourne 0 hors phase de contact, 1 pour la portée courte (16 pixels) et 2 pour la portée longue (24 pixels).
- `vk_whip_segments()` retourne respectivement 0, 2 ou 3 segments visibles.
- Le moteur SGDK affiche un nombre de segments cohérent avec la boîte de collision, des motifs distincts pour la poignée, la chaîne et l'extrémité et masque les segments inutilisés.
- L'orientation et la position restent celles mémorisées au début de l'attaque.
- Tests natifs des phases et portées ajoutés.

## Tests

Commande : `sh tests/run_native.sh` — réussite sur l'environnement de développement.

**À faire sur la machine SGDK :** compiler la ROM et vérifier dans BlastEm B à gauche/droite, le passage de 2 à 3 segments et le retour au repos.

## Limites

Le rendu est toujours provisoire : aucun sprite d'attaque, pose de bras ou frame d'escalier supplémentaire ne peut être qualifié d'authentique sur cette seule base. La ROM MSX2 de référence (131072 octets, SHA-256 `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`) a été vérifiée. La trajectoire et l'animation d'escalier V3 sont inchangées.
