#include <cstdio>
#include <orbis/libkernel.h>

// BLACKOUT_DIAGNOSTIC_BUILD:
// Keep the process alive to isolate a crash during normal application exit.

int main()
{
    FILE *log = std::fopen("/data/blackout_diag.txt", "w");
    if (log != nullptr) {
        std::fprintf(log, "BLACKOUT_DIAGNOSTIC_STARTED\n");
        std::fprintf(log, "BLACKOUT_DIAGNOSTIC_STAYING_ALIVE\n");
        std::fflush(log);
        std::fclose(log);
    }

    for (;;) {
        sceKernelUsleep(1000000);
    }

    return 0;
}
