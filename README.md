# BLACKOUT: SIGNAL LOST — PS3 Homebrew

Prototype de jeu d'horreur/survie à la première personne pour PS3 homebrew (cible HEN).
**État : squelette de projet / première étape de compilation, pas encore un jeu 3D complet.**

## Chaîne technique vérifiée

- SDK : [ps3dev/PSL1GHT](https://github.com/ps3dev/PSL1GHT)
- Environnement complet : [ps3dev/ps3dev](https://github.com/ps3dev/ps3dev)
- Une application PSL1GHT peut produire `.elf` / `.self`; les projets qui définissent une cible de paquet peuvent produire un `.pkg`.
- La compatibilité finale du PKG doit être testée sur une PS3 HEN réelle. GitHub Actions ne peut pas valider le lancement sur console.

## Projet

- `src/main.c` : point d'entrée initial, avec boucle de vie simple.
- `Makefile` : attend les règles PSL1GHT installées dans l'environnement.
- `.github/workflows/build.yml` : lance la compilation dans une image Docker PS3DEV, puis publie les fichiers générés en artefacts.
- `assets/README.md` : conventions pour les ressources du jeu.

## Compiler sur GitHub

1. Crée un dépôt GitHub vide et téléverse le contenu de cette archive.
2. Ouvre **Actions** et active les workflows si GitHub le demande.
3. Lance **Build PS3 Homebrew** avec **Run workflow**, ou pousse un commit sur `main`.
4. Télécharge les artefacts `blackout-ps3-build` depuis l'exécution.

Le workflow est une base CI. L'image de compilation et les cibles Makefile peuvent nécessiter un ajustement si l'image Docker amont ou les règles PSL1GHT changent.

## Compiler localement

Dans un environnement PS3DEV configuré (`PS3DEV` et `PSL1GHT` exportés), exécute :

```sh
make
```

Pour produire un PKG, la cible `pkg` doit être prise en charge par les règles PSL1GHT et le Makefile de l'application. Ne renomme pas simplement un `.elf` en `.pkg`.

## Feuille de route

1. Valider la compilation de base et l'exécution homebrew sur console.
2. Ajouter le rendu 3D et la gestion de la manette avec une bibliothèque graphique PS3 compatible.
3. Ajouter caméra FPS, collisions, lampe torche et première salle.
4. Ajouter sons, portes, objets, sauvegarde et niveaux.
5. Ajouter des ressources originales optimisées pour la mémoire de la PS3.

Un jeu de plusieurs Go exige de vraies ressources (modèles, textures, sons, niveaux) et des tests de performance. Cette archive ne prétend pas déjà les contenir.
