# OpenOrbis / Piglet — audit du rendu GPU

**Date de vérification :** 17 novembre 2026  
**Statut :** API et exemple officiel identifiés ; exécution sur notre PS4 non encore testée.

## Conclusion

OpenOrbis 0.5.2 inclut un exemple officiel de rendu GPU nommé `samples/piglet`, ainsi que des en-têtes EGL/GLES. Ce n'est donc pas exact de dire qu'OpenOrbis ne possède aucune piste GPU exploitable.

La voie identifiée est **Piglet + EGL + OpenGL ES 2.0 (GLES2)**. Ce n'est pas un pilote OpenGL générique que l'on pourrait simplement installer : Piglet est la couche spécifique à la PS4 qui permet à l'application d'utiliser le contexte graphique.

Sources officielles :
- Changelog OpenOrbis v0.5.2 : https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/CHANGELOG.md
- Exemple Piglet : https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/tree/master/samples/piglet
- Implémentation du contexte : https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/samples/piglet/piglet/PigletApplication.cpp
- En-têtes EGL : https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/tree/master/include/EGL
- En-têtes GLES : https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/tree/master/include/GLES2

## Ce que fait l'exemple officiel

L'implémentation Piglet de référence suit ces étapes :

1. Charge les modules système `libScePigletv2VSH.sprx` et `libScePrecompiledShaders.sprx`.
2. Configure Piglet avec `scePigletSetConfigurationVSH`.
3. Crée un affichage EGL avec `eglGetDisplay`, puis initialise EGL.
4. Sélectionne une configuration EGL compatible avec `EGL_OPENGL_ES2_BIT`.
5. Crée la surface de fenêtre et le contexte GLES 2.0.
6. Active le contexte avec `eglMakeCurrent`.
7. Utilise les appels GLES (viewport, clear, textures, buffers, draw calls).
8. Présente chaque image avec `eglSwapBuffers`.

L'exemple affiche une texture animée et montre donc un véritable chemin de rendu GPU, pas uniquement une écriture CPU dans le framebuffer.

## Limites et dépendances critiques

- Le sample dépend de modules système Piglet et de shaders précompilés. Le code les charge depuis le chemin système sandboxé `/common/lib/`.
- Le sample utilise des shaders précompilés au format binaire PSSL via `glShaderBinary`. Il précise que le format utilisé est le format 0 (PSSL).
- Cela signifie que la présence des en-têtes EGL/GLES ne prouve pas qu'on puisse compiler librement des shaders GLSL sur la PS4 avec la toolchain seule.
- Les modules système ne doivent pas être copiés depuis une console ou redistribués dans le dépôt. Le prototype doit s'appuyer sur les composants présents légalement sur la console cible.
- Les paramètres mémoire Piglet du sample sont des valeurs d'exemple, pas encore des valeurs validées pour BLACKOUT.
- La feuille de route OpenOrbis mentionne encore la finalisation du support GPU en v0.7 : il faut donc distinguer l'exemple Piglet fonctionnel de la maturité générale de la toolchain.

## Conséquence pour BLACKOUT / Unreal

Piglet donne une piste concrète pour un futur backend graphique, mais ne constitue pas un RHI Unreal prêt à l'emploi.

Pour Unreal, il faudrait au minimum :
- une implémentation de RHI qui traduit les commandes Unreal vers les appels GLES/Piglet réellement disponibles ;
- une stratégie de shaders compatible avec le format accepté par le pilote PS4 (la chaîne PSSL est un point bloquant à étudier) ;
- la gestion des ressources GPU, buffers, textures, synchronisation et présentation ;
- une validation sur console réelle et firmware cible.

Il ne faut pas supposer qu'un backend OpenGL ES 2.0 suffira aux fonctionnalités modernes attendues par UE5. Il faut d'abord cartographier les fonctions réellement exposées et les limitations du pilote.

## Prochaines preuves à obtenir

| Test | État |
|---|---|
| Présence des headers EGL/GLES dans le SDK | Confirmée par le dépôt officiel |
| Exemple Piglet officiel et séquence d'initialisation | Confirmés par les sources |
| Compilation d'un petit programme qui inclut les headers | À faire dans GitHub Actions |
| Liaison avec les bibliothèques nécessaires | À déterminer depuis le Makefile officiel du sample |
| Démarrage du contexte Piglet sur PS4 HEN | Non testé |
| Affichage d'un triangle avec shader compatible | Non testé |
| Rendu 3D texturé stable sur PS4 Slim | Non testé |
| Faisabilité d'un RHI Unreal au-dessus de Piglet | Non démontrée |

**Décision :** ne pas écrire un faux test graphique qui ne pourrait pas compiler ou démarrer sans les modules système et les shaders requis. La prochaine étape est de reproduire fidèlement les dépendances de build du sample officiel, puis de séparer la vérification de compilation de la validation sur console.
