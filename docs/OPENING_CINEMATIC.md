# BLACKOUT: SIGNAL LOST — Séquence cinématique d'ouverture

> Statut : direction artistique validée / implémentation à réaliser.
> Cette fiche définit la séquence à produire. Elle ne prétend pas que les animations, sons ou effets sont déjà intégrés au jeu.

## Intention

Créer une ouverture digne d'un jeu survival-horror premium : sobre, cinématographique, inquiétante et entièrement cohérente avec l'identité Ridagames. La séquence doit être réalisée avec des éléments originaux du jeu, pas avec un montage de logos statiques.

## Déroulé proposé (environ 14 à 18 secondes)

| Temps | Image et animation | Son |
|---|---|---|
| 0–2 s | Écran noir. Quelques particules fines apparaissent dans un volume sombre. | Silence presque total, souffle grave très discret. |
| 2–6 s | Le symbole Ridagames se révèle en 3D dans la fumée. La caméra effectue un lent travelling avant. | Impact cinématique profond, texture métallique et réverbération. |
| 6–8 s | Le texte « ÉDITÉ PAR RIDAGAMES » apparaît sous le logo, net et lisible. | La musique originale commence à monter. |
| 8–11 s | Transition par extinction lumineuse et interférences contrôlées vers la mention Unreal Engine. | Pulsation électronique et balayage radio. |
| 11–14 s | Le logo officiel Unreal Engine apparaît brièvement, sans le modifier ni le déformer. | Impact court, puis la musique se coupe progressivement. |
| 14–18 s | Noir, grésillement radio, puis révélation du titre « BLACKOUT: SIGNAL LOST ». | Voix radio originale ou bruit de transmission, puis première note du thème du jeu. |

Les durées sont indicatives et seront ajustées au montage final.

## Direction visuelle

- Palette : noir profond, acier froid, gris cendre et accents lumineux blancs/rouge d'alerte.
- Caméra : mouvements lents et maîtrisés, profondeur de champ légère, transitions propres.
- VFX : fumée volumétrique si le budget GPU le permet, particules, poussière, étincelles ponctuelles, aberration chromatique très légère et interférences radio.
- Éclairage : contre-jour, faisceau volumétrique modéré, reflets contrôlés sur le logo.
- Typographie : identité Ridagames cohérente, titres très lisibles et sans surcharge.

## Audio

Composer ou produire une piste originale pour BLACKOUT: SIGNAL LOST :
- nappe ambient sombre ;
- basse cinématique et impacts conçus sur mesure ;
- textures radio et parasites ;
- montée courte, coupure nette et transition vers le thème principal.

Tous les sons et musiques devront être originaux ou disposer d'une licence adaptée. Prévoir un mixage stéréo et un niveau sonore cohérent avec le jeu.

## Mentions et marques

- Mention éditoriale exacte : **ÉDITÉ PAR RIDAGAMES**.
- Unreal Engine doit être présenté comme le moteur utilisé, pas comme l'éditeur du jeu.
- Utiliser les éléments officiels Unreal Engine conformément aux règles de marque d'Epic Games. Ne pas redessiner le logo officiel comme s'il s'agissait d'un logo Ridagames.
- Ridagames conserve sa propre identité visuelle et son logo original.

## Exigences techniques

- Prévoir une séquence d'ouverture rejouable et une option permettant de la passer après la première lecture.
- Éviter les vidéos lourdes si une séquence temps réel offre une qualité équivalente.
- Prévoir un fallback léger pour PS4 Slim sous GoldHEN : résolution, particules, transparences, post-traitement et audio doivent être profilés sur le matériel réel.
- Ne pas supposer qu'Unreal Engine 5 est déjà compatible PS4/GoldHEN. La séquence Unreal ne sera activée sur PS4 qu'après validation du portage du moteur et des outils de compilation.
- Tant que le pipeline Unreal PS4 n'est pas opérationnel, conserver le chemin natif PS4 existant et ne pas casser le build actuel.

## Critères de validation

1. Les deux mentions et le titre sont parfaitement lisibles.
2. Les animations ne provoquent ni saccades majeures ni blocage.
3. La musique et les effets sont synchronisés et mixés proprement.
4. L'ouverture peut être passée sans bloquer le chargement.
5. Une version optimisée est testée sur la PS4 Slim sous GoldHEN avant d'être considérée comme prête.
