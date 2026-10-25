#include <NoRSX.h>
#include <io/pad.h>

static const u32 COLOR_BG       = 0x080B10;
static const u32 COLOR_WHITE    = 0xE6E6E6;
static const u32 COLOR_RED      = 0xD62828;
static const u32 COLOR_GRAY     = 0x777777;

int main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;

    ioPadInit(7);

    NoRSX GFX(RESOLUTION_AUTO, RESOLUTION_1280x720);
    Background BG(&GFX);
    Font FONT(JPN, &GFX);

    int selection = 0;
    bool running = true;

    GFX.AppStart();

    while (running && GFX.GetAppStatus())
    {
        padInfo info;
        padData data;

        ioPadGetInfo(&info);

        if (info.status[0])
        {
            ioPadGetData(0, &data);

            if (data.BTN_UP)
                selection = (selection + 2) % 3;

            if (data.BTN_DOWN)
                selection = (selection + 1) % 3;

            if (data.BTN_TRIANGLE || data.BTN_START)
                running = false;

            if (data.BTN_CROSS && selection == 2)
                running = false;
        }

        BG.Mono(COLOR_BG);

        FONT.Printf(90, 100, COLOR_RED, 48,
                    "BLACKOUT");

        FONT.Printf(90, 155, COLOR_WHITE, 28,
                    "SIGNAL LOST");

        FONT.Printf(90, 230, COLOR_GRAY, 22,
                    "SURVIVAL HORROR");

        FONT.Printf(110, 340,
                    selection == 0 ? COLOR_RED : COLOR_WHITE,
                    26, "NOUVELLE PARTIE");

        FONT.Printf(110, 395,
                    selection == 1 ? COLOR_RED : COLOR_WHITE,
                    26, "CHARGER PARTIE");

        FONT.Printf(110, 450,
                    selection == 2 ? COLOR_RED : COLOR_WHITE,
                    26, "QUITTER");

        FONT.Printf(90, 590, COLOR_GRAY, 18,
                    "HAUT / BAS : SELECTION");

        FONT.Printf(90, 620, COLOR_GRAY, 18,
                    "CROIX : VALIDER");

        FONT.Printf(90, 650, COLOR_GRAY, 18,
                    "TRIANGLE : QUITTER");

        GFX.Flip();
    }

    GFX.AppExit();
    ioPadEnd();

    return 0;
}
