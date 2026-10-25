/*
 * BLACKOUT: SIGNAL LOST — point d'entrée initial PS3
 *
 * Ceci est un point de départ minimal pour l'intégration au SDK.
 * Le rendu 3D, les contrôles et la boucle de jeu seront ajoutés après
 * validation de la chaîne de compilation sur la cible PS3.
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /*
     * Sortie utile pour les builds/debuggers qui capturent stdout.
     * Une interface graphique PS3 doit ensuite être initialisée via
     * les APIs PSL1GHT / RSX ou une bibliothèque compatible.
     */
    puts("BLACKOUT: SIGNAL LOST");
    puts("PS3 homebrew prototype - build scaffold");
    puts("Next milestone: RSX renderer + controller input.");

    return EXIT_SUCCESS;
}
