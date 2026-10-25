#include <NoRSX.h>
#include <io/pad.h>

static const u32 GAME_BG_COLOR = 0x080B10;
static const u32 GAME_TEXT_COLOR = 0xE6E6E6;
static const u32 GAME_ACCENT_COLOR = 0xD62828;

int main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;

    ioPadInit(7);

    NoRSX GFX(RESOLUTION_AUTO, RESOLUTION_1280x720);
    Background BG(&GFX);
    Font FONT(JPN, &GFX);

    bool running = true;
    int selection = 0;

    padInfo info;
    padData data;

    while (running)
    {
        ioPadGetInfo(&info);

        if (info.status[0])
        {
            ioPadGetData(0, &data);

            if (data.BTN_UP)
                selection = 0;

            if (data.BTN_DOWN)
                selection = 1;

            if (data.BTN_CROSS)
            {
                if (selection == 1)
                    running = false;
            }

            if (data.BTN_CIRCLE)
                running = false;
        }

        BG.Mono(GAME_BG_COLOR);

        FONT.Printf(100, 100, GAME_TEXT_COLOR,
                    "BLACKOUT: SIGNAL LOST");

        FONT.Printf(100, 180,
                    selection == 0 ? GAME_ACCENT_COLOR : GAME_TEXT_COLOR,
                    "NOUVELLE PARTIE");

        FONT.Printf(100, 230,
                    selection == 1 ? GAME_ACCENT_COLOR : GAME_TEXT_COLOR,
                    "QUITTER");

        FONT.Printf(100, 650, GAME_TEXT_COLOR,
                    "Prototype - Menu principal");

        GFX.Flip();
    }

    GFX.AppExit();
    ioPadEnd();

    return 0;
}
