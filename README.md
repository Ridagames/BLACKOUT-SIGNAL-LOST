# BLACKOUT: SIGNAL LOST — PS4 Homebrew

Projet homebrew PS4 de survival horror à la première personne.

## Direction du jeu

- Survival horror inspiré des classiques du genre.
- Vue à la première personne.
- Munitions rares : la fuite et la discrétion priment sur le combat.
- Exploration, énigmes, histoire, ambiance sonore et tension.
- Commandes DualShock 4.

## Plateforme et compilation

- Cible : PlayStation 4.
- Toolchain : [OpenOrbis PS4 Toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain), version 0.5.4.
- Langage : C++.
- Compilation : GitHub Actions.
- Sortie attendue : `eboot.bin` et paquet `.pkg`.

La branche de portage est `ps4/port`. La branche `main` et l'ancienne branche PS3 ne sont pas modifiées.

## Compiler

1. Ouvrir l'onglet **Actions** du dépôt.
2. Sélectionner **Build BLACKOUT PS4**.
3. Cliquer sur **Run workflow** et choisir la branche `ps4/port`.
4. Récupérer l'artefact **BLACKOUT-SIGNAL-LOST-PS4** si la compilation réussit.

## État actuel

Cette conversion initialise une base OpenOrbis PS4 et son pipeline de compilation. Ce n'est pas encore le jeu complet : le rendu 3D, le contrôleur, le menu interactif, le niveau, les ennemis, les sons et les sauvegardes restent à développer et tester sur console.

Un PKG compilé n'est pas une garantie de compatibilité avec un firmware donné. L'installation et l'exécution nécessitent un environnement PS4 compatible homebrew.
