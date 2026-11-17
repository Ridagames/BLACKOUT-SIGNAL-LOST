# Étude technique — Unreal Engine vers PS4 / OpenOrbis

**Branche :** `research/ue5-openorbis`  
**Statut :** phase 0 — audit des dépendances et définition d'un prototype  
**Cible :** PS4 x86-64, homebrew OpenOrbis  
**Important :** aucun code Unreal Engine n'est inclus dans ce dépôt. Ce document ne prétend pas qu'UE5 est déjà porté.

## 1. Résultat de l'audit initial

Le dépôt BLACKOUT possède déjà une branche PS4/OpenOrbis distincte. Son point d'entrée actuel (`source/main.cpp`) est un programme de diagnostic qui démarre puis attend en boucle. Il ne contient pas de runtime Unreal Engine.

L'OpenOrbis PS4 Toolchain fournit une chaîne LLVM/Clang, des en-têtes, des stubs de bibliothèques et des outils de génération d'exécutables/PKG homebrew. Elle ne fournit pas le code d'un moteur de jeu généraliste ni un backend Unreal.

La documentation Epic confirme que l'accès à la documentation et aux éléments de développement PlayStation est réservé aux développeurs ayant satisfait les exigences NDA et obtenu les autorisations console correspondantes. L'accès aux sources publiques d'Unreal Engine ne signifie donc pas que le backend PlayStation propriétaire est disponible.

Références vérifiées le 17 novembre 2026 :
- OpenOrbis Toolchain : https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain
- Documentation Epic — développement PlayStation 4 : https://dev.epicgames.com/documentation/en-us/unreal-engine/development-for-playstation-4-in-unreal-engine

## 2. Ce qu'un vrai portage doit fournir

Un portage ne consiste pas à convertir un projet Unreal en PKG. Il faut rendre le moteur lui-même compatible avec l'ABI, le système et le GPU de la PS4.

| Sous-système | Travail requis |
|---|---|
| Cible de compilation | ABI x86-64 Orbis, compilateur, linker, exceptions, RTTI, TLS et bibliothèques C/C++ |
| Plateforme | types, synchronisation, threads, mémoire virtuelle, fichiers, chemins, horloge et tâches |
| Rendu | RHI Unreal dédié, gestion mémoire GPU, commandes, synchronisation, présentation et shaders |
| Shaders | chaîne de compilation compatible avec les formats et le pilote réellement disponibles sur PS4 |
| Fenêtre / affichage | initialisation vidéo, swapchain et gestion des résolutions |
| Entrées | adaptation DualShock 4 et périphériques |
| Audio | backend audio, mixage, streaming et périphériques |
| Contenu | cuisson des assets, formats, compression et chargement |
| Packaging | exécutable Orbis, modules requis, SFO et PKG |
| Outils | remplacement ou adaptation des outils Editor/Build qui supposent Windows, Visual Studio ou les SDK propriétaires |

Le RHI est le verrou majeur : sans backend graphique opérationnel, le moteur ne peut pas rendre une scène 3D. Une piste concrète existe toutefois dans OpenOrbis 0.5.2 : l'exemple officiel Piglet utilise EGL et OpenGL ES 2.0 (GLES2) pour initialiser un contexte GPU et présenter des images. Ce n'est pas encore un RHI Unreal. Voir `research/ue5-openorbis/PIGLET_RENDERING.md`.

## 3. Stratégie de travail

On ne tentera pas de compiler tout Unreal d'un seul coup. Chaque étape devra produire un résultat vérifiable avant la suivante.

### Étape A — inventaire des sources
- Identifier précisément la version d'Unreal visée.
- Vérifier quels modules du moteur sont disponibles dans les sources auxquelles le projet a légalement accès.
- Établir la liste des dépendances impossibles à satisfaire avec OpenOrbis seul.
- Séparer les modules indépendants de la plateforme des modules Windows/PlayStation.

**Critère de sortie :** inventaire des modules et dépendances avec preuve dans les fichiers source.

### Étape B — preuve de compilation Orbis
- Créer un petit programme C++ isolé.
- Vérifier les headers, la bibliothèque standard, les exceptions, les threads et les allocations.
- Produire un ELF/OELF et un PKG avec OpenOrbis.
- Garder cette preuve séparée du jeu et du moteur.

**Critère de sortie :** compilation reproductible dans GitHub Actions, avec artefacts et journaux.

### Étape C — couche plateforme minimale
- Implémenter uniquement les abstractions système indispensables.
- Tester mémoire, fichiers, temps, threads et contrôleur sur console.
- Ne pas déclarer une API compatible tant qu'un test ne l'a pas démontré.

**Critère de sortie :** tests ciblés réussis sur PS4 réelle.

### Étape D — rendu expérimental
- Étudier les interfaces GPU réellement exposées par l'environnement OpenOrbis.
- Faire un test de présentation et de rendu 3D indépendant d'Unreal.
- Mesurer les limites de formats, shaders, mémoire et synchronisation.
- Décider ensuite si un RHI Unreal peut être écrit sans dépendance propriétaire manquante.

**Critère de sortie :** scène de test 3D stable sur console, pas seulement une fenêtre noire.

### Étape E — intégration moteur
Uniquement si les étapes précédentes réussissent :
- intégrer progressivement les modules portables ;
- remplacer les dépendances de plateforme ;
- connecter le RHI expérimental ;
- compiler les modules par lots ;
- valider les assets et le runtime.

## 4. Blocages connus — à ne pas masquer

1. **Sources et autorisations PlayStation Unreal :** l'accès aux composants console Epic/Sony dépend d'autorisations et d'accords spécifiques. Aucun fichier propriétaire ne sera supposé disponible.
2. **Backend GPU :** la toolchain OpenOrbis est un SDK homebrew, pas un backend Unreal prêt à l'emploi. Sa documentation décrit les bibliothèques et outils fournis et présente encore le support GPU comme un chantier de sa feuille de route.
3. **Unreal Editor :** faire tourner l'éditeur complet sur une PS4 Slim n'est pas l'objectif de cette branche. La cible éventuelle est un runtime de jeu compilé depuis un environnement de build externe.
4. **Capacité de compilation :** GitHub Actions peut compiler du code source accessible ; il ne génère pas les fichiers intermédiaires et les assets cuits qui exigent les outils Unreal si ceux-ci ne sont pas disponibles.
5. **Compatibilité console :** un PKG qui se construit n'est pas une preuve que l'exécutable démarre, rend une image ou fonctionne sur un firmware précis.

## 5. Règles de validation

- Pas de faux résultat : chaque étape indique clairement `NON TESTÉ`, `COMPILÉ` ou `TESTÉ SUR CONSOLE`.
- Pas de remplacement silencieux d'Unreal par un moteur maison.
- Pas de cube de démonstration présenté comme le jeu final.
- Aucun changement sur `main` ni sur `ps4/port` depuis cette branche de recherche.
- Les essais restent isolés dans `research/ue5-openorbis` jusqu'à validation.

## 6. Prochaine tâche technique

Créer le prototype C++ minimal de l'étape B et son workflow CI isolé. Il servira à mesurer exactement ce que la toolchain OpenOrbis sait compiler avant toute tentative d'intégration Unreal.
