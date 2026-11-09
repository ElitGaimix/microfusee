# MicroFusée

Ce repository regroupe différents projets de microfusées.

## Organisation

Ce projet se divise en deux parties :
- La partie au sol : dans le dossier `controllers`, vous retrouverez tout ce qui concerne la réception au sol (code électronique ainsi que logiciel).
- Les fusées : dans le dossier `Rockets`, divisé par versions (NLR1, NLR2, etc.), vous trouverez le projet OpenRocket, les différentes modélisations 3D, l'électronique embarquée et le code.

## Etat actuelle du projet
Actuellement le projet avance du coté de la communication radio entre la fusée et l'ordinateur au sol, on utilise un récépteur elrs brancher a un stm32 (pour l'instant une devboard nucleoF401RE) et un éméteur RadioMaster nano ranger tx brancher au pc, l'objectif est de créer un pont serial (déjà validé) et un protocole de communication entre les deux appareils (En cours)

## Sous-module NLR

Le dépôt [NLR](https://github.com/ElitGaimix/NLR) est référencé dans
`Rockets/nlr1/nlr`. Après un nouveau clonage, utilisez
`git submodule update --init --recursive` pour récupérer les sous-modules.

Sur le PC de développement actuel, `Rockets/nlr1/nlr` est une jonction Windows
vers `Rockets/nlr1/code/stm32/nlr1` : le code reste dans son dossier d'origine.
Cette jonction est locale ; GitHub affiche un lien vers le dépôt NLR au chemin
`Rockets/nlr1/nlr`.

## Contributions

Les contributions sont très limitées, voire fermées, et réservées à des amis proches. Toutefois, si vous avez une remarque concernant un mauvais code ou une mauvaise conception, vous pouvez ouvrir une issue. Je serais ravi de recevoir des avis à ce sujet.
