
#include <NoRSX.h>
#include <io/pad.h>
#include <pngdec/pngdec.h>

static const u32 GAME_TEXT_COLOR = 0xE6E6E6;
static const u32 GAME_ACCENT_COLOR = 0xD62828;
static const u32 GAME_MUTED_COLOR = 0xAAAAAA;

int main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;

    ioPadInit(7);

    NoRSX GFX(RESOLUTION_AUTO, RESOLUTION_1280x720);
    Background BG(&GFX);
    Font FONT(JPN, &GFX);
    Image IMG(&GFX);

    pngData menuBackground = {};

    bool imageLoaded = false;

    IMG.LoadPNG("/app_home/BLACKOUT_MENU.png", &menuBackground);

    if (menuBackground.width > 0 && menuBackground.height > 0)
        imageLoaded = true;

    const char* menuItems[] = {
        "NOUVELLE PARTIE",
        "CHARGER PARTIE",
        "OPTIONS",
        "QUITTER"
    };

    const int menuCount = 4;
    int selection = 0;
    bool running = true;

    padInfo info;
    padData data;

    while (running)
    {
        ioPadGetInfo(&info);

        if (info.status[0])
        {
            ioPadGetData(0, &data);

            if (data.BTN_UP && selection > 0)
                selection--;

            if (data.BTN_DOWN && selection < menuCount - 1)
                selection++;

            if (data.BTN_CIRCLE)
                running = false;

            if (data.BTN_CROSS && selection == 3)
                running = false;
        }

        if (imageLoaded)
        {
            IMG.DrawIMG(0, 0, &menuBackground);
        }
        else
        {
            BG.Mono(0x080B10);
        }

        FONT.Printf(90, 105, GAME_TEXT_COLOR, "BLACKOUT");
        FONT.Printf(95, 145, GAME_MUTED_COLOR,
                    "S I G N A L   L O S T");
        FONT.Printf(95, 185, GAME_MUTED_COLOR,
                    "--------------------------------");

        for (int i = 0; i < menuCount; i++)
        {
            u32 color = (i == selection)
                ? GAME_ACCENT_COLOR
                : GAME_TEXT_COLOR;

            int y = 270 + (i * 55);

            if (i == selection)
                FONT.Printf(95, y, GAME_ACCENT_COLOR, ">");

            FONT.Printf(125, y, color, "%s", menuItems[i]);
        }

        FONT.Printf(95, 535, GAME_MUTED_COLOR,
                    "HAUT / BAS : NAVIGUER");
        FONT.Printf(95, 570, GAME_TEXT_COLOR,
                    "X : SELECTIONNER");
        FONT.Printf(95, 605, GAME_MUTED_COLOR,
                    "O : RETOUR");
        FONT.Printf(95, 675, GAME_MUTED_COLOR,
                    "PROTO­TYPE - VERSION EN DEVELOPPEMENT");

        GFX.Flip();
    }

    GFX.AppExit();
    ioPadEnd();

    return 0;
}
