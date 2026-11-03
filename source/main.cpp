#include <sstream>
#include <iostream>
#include <orbis/libkernel.h>
#include "log.h"

// Point d'entrée PS4 OpenOrbis.
// Cette première étape valide le démarrage et la chaîne de compilation.
// Le rendu 3D, la manette et le menu graphique seront ajoutés ensuite.
std::stringstream debugLogStream;

int main(void)
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    DEBUGLOG << "BLACKOUT: SIGNAL LOST";
    DEBUGLOG << "PS4 build - OpenOrbis";
    DEBUGLOG << "Prototype de demarrage initialise.";

    for (;;)
    {
        sceKernelUsleep(1000000);
    }

    return 0;
}
