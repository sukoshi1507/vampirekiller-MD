# Vérification de la ROM et escaliers — 26 octobre 2026

## ROM

L'archive `Vampire Killer (Japan, Europe) (En).zip` fournie par l'utilisateur contient une ROM de 131072 octets dont le SHA-256 est `99eabaaeb2b6c02220e131043d5a6401d0d8c5f413d89204547adcc04d65e7e2`. Cette empreinte est exactement celle attendue par `tools/probe.py`. Il n'est plus nécessaire d'utiliser la variante Alt 3. La ROM n'est pas redistribuée dans ce projet.

## Correction de la démonstration d'escalier

Dans la salle 1, utiliser A+HAUT pour placer Simon sur la zone expérimentale, puis HAUT/BAS pour déplacer X et Y conjointement. La pente de démonstration monte vers la droite. La séquence alterne des poses de marche déjà disponibles, plutôt qu'une pose de saut figée.

IMPORTANT : la pente et la zone sont des artifices de test, **pas** des coordonnées/animations reconstruites depuis la ROM. Les escaliers authentiques exigent encore le décodage de leurs tables et motifs. La touche B est détectée et le prototype du fouet possède une fenêtre d'attaque, mais les sprites originaux du fouet ne sont pas encore extraits.

## Vérifications

`sh tests/run_native.sh` : PASS (tests gameplay, physique, fouet). Compilation SGDK et rendu BlastEm non exécutés ici. La ROM compilée précédemment dans l'archive n'intègre pas ces modifications et doit être régénérée avec SGDK.
