#include <cstdio>
#include <orbis/libkernel.h>

int main(void)
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    std::printf("BLACKOUT: SIGNAL LOST\\n");
    std::printf("PS4 HOMEbrew diagnostic build started\\n");

    sceKernelUsleep(2 * 1000000);

    std::printf("Initialization complete. Idle loop active.\\n");

    for (;;) {
        sceKernelUsleep(1000000);
    }

    return 0;
}
