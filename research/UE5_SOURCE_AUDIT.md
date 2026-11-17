# Audit des sources Unreal Engine disponibles — portage PS4/OpenOrbis

**Vérification :** 17 novembre 2026  
**Fork source :** [Ridagames/UnrealEngine](https://github.com/Ridagames/UnrealEngine)  
**Branche source inspectée :** `release`  
**Version déclarée par le dépôt :** Unreal Engine 5.8.4  
**Branche de travail isolée créée dans le fork :** `ridagames/ps4-openorbis-port`

## 1. Accès au vrai moteur confirmé

Le compte GitHub lié à Epic a permis de disposer d'un fork privé du dépôt complet Unreal Engine. L'API GitHub a permis de lire des fichiers du moteur, notamment :

- [README.md](https://github.com/Ridagames/UnrealEngine/blob/release/README.md)
- [Engine/Build/Build.version](https://github.com/Ridagames/UnrealEngine/blob/release/Engine/Build/Build.version)
- [LaunchEngineLoop.cpp](https://github.com/Ridagames/UnrealEngine/blob/release/Engine/Source/Runtime/Launch/Private/LaunchEngineLoop.cpp)
- [RHI.cpp](https://github.com/Ridagames/UnrealEngine/blob/release/Engine/Source/Runtime/RHI/Private/RHI.cpp)
- [OpenGLDrv.cpp](https://github.com/Ridagames/UnrealEngine/blob/release/Engine/Source/Runtime/OpenGLDrv/Private/OpenGLDrv.cpp)
- [OpenGLDrv.Build.cs](https://github.com/Ridagames/UnrealEngine/blob/release/Engine/Source/Runtime/OpenGLDrv/OpenGLDrv.Build.cs)

Le fichier Build.version indique MajorVersion 5, MinorVersion 8, PatchVersion 4.

## 2. Découverte importante : le pilote OpenGL Unreal n'est pas un backend PS4 prêt à l'emploi

Le fichier `OpenGLDrv.Build.cs` déclare explicitement les plateformes prises en charge par ce module :

- Linux
- Android
- LinuxArm64
- groupe Windows pour les configurations concernées

PS4/Orbis n'est pas déclaré. Le code OpenGL comporte aussi des chemins conditionnels Android/EGL, mais cela ne crée pas une intégration Piglet PS4. Il faudra donc créer une plateforme Orbis et un backend RHI dédié, ou adapter proprement les couches concernées ; activer simplement OpenGLDrv dans le Build.cs ne suffit pas.

## 3. Composants console absents des chemins publics testés

Les chemins suivants n'existent pas dans la branche `release` du fork, selon les réponses 404 de l'API GitHub :

- `Engine/Platforms/PS4/Source/Developer/TargetPlatform/PS4TargetPlatform.Build.cs`
- `Engine/Platforms/PS4/Source/Runtime/PS4/PS4.Build.cs`
- `Engine/Platforms/PS4/Source/Runtime/PS4RHI/PS4RHI.Build.cs`
- `Engine/Source/Programs/UnrealBuildTool/Platform/PS4/PS4Platform.cs`

Cela ne prouve pas que tous les composants PS4 sont absents de tous les dépôts ou packages licenciés. Cela prouve que ces chemins attendus ne sont pas disponibles dans cette branche du fork. La documentation Epic indique que l'accès aux sources, SDK et instructions spécifiques à une console nécessite une procédure de développeur/accord distincte.

## 4. Branche de portage isolée

Une branche de travail a été créée dans le fork Unreal Engine :

`ridagames/ps4-openorbis-port`

Elle part de `release`. Aucun fichier du moteur n'a encore été modifié. Le dépôt source Epic ne doit pas être copié dans BLACKOUT-SIGNAL-LOST ni redistribué : les sources Unreal restent dans le fork privé soumis à l'EULA.

## 5. Architecture de portage à implémenter

Le portage doit être fait dans le fork Unreal Engine, puis relié au projet BLACKOUT par une configuration de build. Les sous-systèmes à traiter sont :

1. **UnrealBuildTool** : déclarer Orbis comme cible, toolchain Clang/LLD OpenOrbis, ABI et règles de packaging.
2. **HAL / Core** : plateforme système, mémoire, threads, fichiers, temps, synchronisation, crash/log.
3. **ApplicationCore / Launch** : démarrage du runtime, événements et fenêtre/affichage.
4. **RHI** : module OrbisRHI dédié et gestion des ressources GPU.
5. **Piglet / EGL / GLES** : initialisation de l'affichage et appels graphiques compatibles avec les fonctions réellement disponibles sur la PS4.
6. **Shaders** : chaîne de compilation et formats acceptés par le pilote ; les shaders PSSL précompilés du sample Piglet doivent être traités comme une dépendance distincte, non comme une capacité GLSL automatiquement disponible.
7. **Audio / Input / fichiers** : backend système, contrôleur et accès aux assets.
8. **Packaging** : executable Orbis, SFO, modules et PKG.

## 6. Règles de travail

- Le code Unreal est modifié uniquement dans le fork privé et sur `ridagames/ps4-openorbis-port`.
- Les notes de portage et les scripts propres à BLACKOUT restent dans `research/ue5-openorbis`.
- Ne jamais toucher à `main` ni à `ps4/port` de BLACKOUT.
- Ne jamais prétendre que le moteur compile ou démarre sur PS4 avant preuve réelle.
- Ne pas remplacer Unreal par un moteur alternatif.
