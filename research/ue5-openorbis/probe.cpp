#include <cstdio>
#include <orbis/libkernel.h>

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    std::puts("UE5-OPENORBIS PROBE");
    std::puts("C++ runtime reached main().");
    std::puts("OpenOrbis headers and C++ standard library are linked.");
    return 0;
}
