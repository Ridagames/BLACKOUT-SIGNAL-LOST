#include <stdlib.h>
#include <unistd.h>
#include <sysutil/sysutil.h>
#include <io/pad.h>

static volatile bool running = true;

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

    if (ioPadInit(7) < 0)
    {
        return 1;
    }

    while (running)
    {
        sysUtilCheckCallback();
        usleep(1000);
    }

    ioPadEnd();
    return 0;
}
