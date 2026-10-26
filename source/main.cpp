#include <io/pad.h>
#include <sysutil/sysutil.h>
#include <NoRSX.h>
#include <unistd.h>

static volatile bool running = true;

static void sysCallback(u64 status, u64 param, void *userdata)
{
    (void)param;
    (void)userdata;
    if (status == SYSUTIL_EXIT_GAME)
        running = false;
}

int main()
{
    sysUtilRegisterCallback(0, sysCallback, NULL);

    if (ioPadInit(7) < 0)
        return 1;

    NoRSX gfx(RESOLUTION_AUTO, RESOLUTION_1280x720);
    Font font(JPN, &gfx);
    Image image(&gfx);
    pngData *background = new pngData;

    // The image is packaged in the game's USRDIR directory.
    image.LoadPNG("/dev_hdd0/game/BLSG00001/USRDIR/BLACKOUT_MENU.png", background);

    int selected = 0;
    padInfo padinfo;
    padData paddata;
    bool previousUp = false, previousDown = false, previousCross = false;

    gfx.AppStart();
    while (running && gfx.GetAppStatus())
    {
        sysUtilCheckCallback();
        ioPadGetInfo(&padinfo);
        if (padinfo.status[0])
        {
            ioPadGetData(0, &paddata);
            bool up = paddata.BTN_UP;
            bool down = paddata.BTN_DOWN;
            bool cross = paddata.BTN_CROSS;

            if (up && !previousUp) selected = (selected + 2) % 3;
            if (down && !previousDown) selected = (selected + 1) % 3;

            if (cross && !previousCross)
            {
                if (selected == 2) running = false;
                // Play and Options are placeholders until those screens exist.
            }
            previousUp = up;
            previousDown = down;
            previousCross = cross;
        }

        gfx.Clear(0x000000);
        image.DrawIMG(0, 0, background);
        font.Printf(90, 420, selected == 0 ? COLOR_YELLOW : COLOR_WHITE, 50, "%s JOUER", selected == 0 ? ">" : " ");
        font.Printf(90, 490, selected == 1 ? COLOR_YELLOW : COLOR_WHITE, 50, "%s OPTIONS", selected == 1 ? ">" : " ");
        font.Printf(90, 560, selected == 2 ? COLOR_YELLOW : COLOR_WHITE, 50, "%s QUITTER", selected == 2 ? ">" : " ");
        font.Printf(90, 650, COLOR_WHITE, 25, "FLECHES : NAVIGUER    X : VALIDER");
        gfx.Flip();
        usleep(16000);
    }

    gfx.AppExit();
    delete background;
    ioPadEnd();
    return 0;
}
