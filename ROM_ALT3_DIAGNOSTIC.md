# ROM Alt 3 — contrôle de compatibilité

ROM apportée : `Vampire Killer (Japan, Europe) (En) (Alt 3).rom`, 131072 octets.
SHA-256 : `377d253d285b8a4776de1dc6bacaad9f326e0d9207433269441eac25fa776a4b`.
ROM référencée dans les outils et ressources : `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`.

Comparaison statique des annotations de `research/known.asm` : **83 enregistrements identiques sur 111**. La version Alt 3 n'est donc pas substituable directement à la ROM de référence. Les adresses et données des 28 entrées restantes doivent être revalidées. Les scripts `tools/probe.py` / `tools/extract_player.py` refusent la régénération tant que l'empreinte ne correspond pas : cette protection a été conservée.

La version fournie corrige **l'accessibilité du test d'escalier** et rend le fouet **visuellement distinct** et **diagnostiquable** ; elle ne prétend pas avoir extrait les escaliers ou animations authentiques depuis Alt 3.

## Essais BlastEm

1. Démarrer une partie et entrer dans le château via la cour.
2. **B** : un message `B: ATTAQUE DETECTEE` apparaît temporairement en haut ; le fouet se voit en blanc à hauteur du buste. Si aucun message n'apparaît, vérifier l'assignation de la touche B du port 1 dans BlastEm.
3. **A + DROITE** dans le hall pour aller à la salle de test 1.
4. **A + HAUT** dans cette salle : positionnement sur l'escalier de démonstration, puis **HAUT / BAS** pour monter / descendre. **C** permet de quitter l'escalier en sautant.
5. Le message `B: BLOQUE SUR ESCALIER` signifie que l'entrée B arrive correctement, mais que l'attaque est interdite par le contrôleur durant l'escalade.

Limites : pas d'escalier authentifié issu de la ROM, pas de sprite de fouet authentique, pas de recompilation SGDK ni exécution BlastEm dans cet environnement. Le test natif `tests/run_native.sh` passe.
