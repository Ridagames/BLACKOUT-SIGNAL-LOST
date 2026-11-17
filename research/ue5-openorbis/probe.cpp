#include <cstdio>
#include <orbis/libkernel.h>

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    std::puts("UE5-OPENORBIS PROBE");
    std::puts("C++ runtime reached main()");
    const int result = sceKernelUsleep(1000);
    std::printf("sceKernelUsleep returned: %d\n", result);
    std::puts("Compile/link smoke test completed.");
    return 0;
}
