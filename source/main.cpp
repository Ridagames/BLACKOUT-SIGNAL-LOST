
#include <stdlib.h>
#include <sysutil/sysutil.h>
#include <io/pad.h>

static bool running = true;

static void sysCallback(u64 status, u64 param, void *userdata)
{
    (void)param;
    (void)userdata;

    if (status == SYSUTIL_EXIT_GAME)
    {
        running = false;
    }
}

int main()
{
    sysUtilRegisterCallback(0, sysCallback, NULL);

    ioPadInit(7);

    while (running)
    {
        sysUtilCheckCallback();
    }

    ioPadEnd();

    return 0;
}
