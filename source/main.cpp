#include <cstdio>
#include <orbis/libkernel.h>

// BLACKOUT diagnostic build:
// - no graphics engine
// - no controller initialization
// - no PNG loading
// Writes a marker in the app writable data area, waits, then exits cleanly.

int main()
{
    FILE *log = std::fopen("/data/blackout_diag.txt", "w");
    if (log != nullptr) {
        std::fprintf(log, "BLACKOUT_DIAGNOSTIC_STARTED\n");
        std::fflush(log);
    }

    sceKernelUsleep(8000000);

    if (log != nullptr) {
        std::fprintf(log, "BLACKOUT_DIAGNOSTIC_EXITING\n");
        std::fclose(log);
    }

    return 0;
}
