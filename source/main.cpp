#include <cstdio>
#include <sstream>
#include <orbis/libkernel.h>
#include "log.h"

std::stringstream debugLogStream;

int main(void)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    DEBUGLOG << "BLACKOUT PS4 BOOT DIAGNOSTIC: executable started";
    sceKernelUsleep(2 * 1000000);
    DEBUGLOG << "BLACKOUT PS4 BOOT DIAGNOSTIC: entering idle loop";
    for (;;) {
        sceKernelUsleep(1000000);
    }
    return 0;
}
