#include <cstdio>
#include <cstring>
#include <cstdint>
#include <orbis/Pad.h>
#include "common/graphics.h"
#include "common/png.h"

static const int SCREEN_W = 1920;
static const int SCREEN_H = 1080;

enum MenuPage { PAGE_MAIN, PAGE_NEW_GAME, PAGE_CONTINUE, PAGE_OPTIONS, PAGE_INTRO };

static bool vibrationEnabled = true;
static int selected = 0;
static int difficulty = 1;
static MenuPage page = PAGE_MAIN;
static unsigned int previousButtons = 0;
static int lastPadReadResult = -999;
static int lastPadButtons = 0;
static int lastPadHandle = -1;
static int stickRepeat = 0;

static void write_status(const char *message)
{
    FILE *log = std::fopen("/data/blackout_diag.txt", "w");
    if (log) {
        std::fprintf(log, "%s\n", message);
        std::fclose(log);
    }
}

static const unsigned char *glyph(char c)
{
    static const unsigned char blank[7] = {0,0,0,0,0,0,0};
    static const unsigned char table[37][7] = {
        {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
        {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
        {14,17,16,23,17,17,14}, {17,17,17,31,17,17,17},
        {31,4,4,4,4,4,31}, {7,2,2,2,18,18,12},
        {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17}, {17,25,25,21,19,19,17},
        {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
        {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
        {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
        {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
        {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
        {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
        {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
        {0,0,0,0,0,0,0}
    };
    if (c >= 'A' && c <= 'Z') return table[c - 'A'];
    if (c >= '0' && c <= '9') return table[26 + c - '0'];
    if (c == '-') return table[36];
    return blank;
}

static void drawText(Scene2D &scene, const char *txt, int x, int y, int scale, Color color)
{
    for (int i = 0; txt[i]; ++i) {
        if (txt[i] == ' ') continue;
        const unsigned char *rows = glyph(txt[i]);
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (rows[row] & (1u << (4 - col)))
                    scene.DrawRectangle(x + (i * 7 + col) * scale,
                                        y + row * scale, scale, scale, color);
    }
}

static void drawTextShadow(Scene2D &scene, const char *txt, int x, int y, int scale, Color color)
{
    drawText(scene, txt, x + 3, y + 3, scale, {0, 0, 0});
    drawText(scene, txt, x, y, scale, color);
}

static void drawTitle(Scene2D &scene, const char *section)
{
    drawTextShadow(scene, "BLACKOUT", 112, 116, 6, {238, 235, 224});
    drawText(scene, "SIGNAL LOST", 116, 174, 2, {210, 74, 55});
    scene.DrawRectangle(116, 220, 310, 2, {178, 72, 54});
    drawText(scene, section, 116, 250, 2, {188, 190, 183});
}

static void drawItem(Scene2D &scene, int y, const char *label, bool active, bool disabled = false)
{
    Color c = disabled ? Color{105, 108, 105} :
              active ? Color{247, 239, 224} : Color{177, 181, 176};
    if (active) {
        scene.DrawRectangle(116, y - 5, 3, 35, {218, 75, 54});
        drawText(scene, ">", 137, y + 2, 2, {230, 100, 76});
    }
    drawTextShadow(scene, label, 166, y, 3, c);
}

static void drawMain(Scene2D &scene)
{
    drawTitle(scene, "MENU PRINCIPAL");
    drawItem(scene, 475, "NOUVELLE PARTIE", selected == 0);
    drawItem(scene, 555, "CONTINUER", selected == 1);
    drawItem(scene, 635, "OPTIONS", selected == 2);
    drawText(scene, "EXPLORER   SURVIVRE   COMPRENDRE", 116, 900, 2, {170, 172, 165});
    drawText(scene, "CROIX  VALIDER", 116, 970, 2, {205, 205, 194});
    drawText(scene, "ROND  RETOUR", 370, 970, 2, {160, 165, 160});
}

static void drawNewGame(Scene2D &scene)
{
    drawTitle(scene, "NOUVELLE PARTIE");
    drawText(scene, "DIFFICULTE", 116, 430, 2, {188, 190, 183});
    const char *names[3] = {"FACILE", "NORMAL", "DIFFICILE"};
    int x[3] = {116, 340, 580};
    for (int i = 0; i < 3; ++i) {
        bool active = difficulty == i;
        if (active) scene.DrawRectangle(x[i], 490, 3, 35, {218, 75, 54});
        drawTextShadow(scene, names[i], x[i] + (active ? 18 : 0), 495, 3,
                       active ? Color{247, 239, 224} : Color{155, 160, 155});
    }
    drawText(scene, "GAUCHE / DROITE  CHANGER", 116, 590, 2, {165, 170, 164});
    drawItem(scene, 740, "COMMENCER", selected == 0);
    drawItem(scene, 820, "RETOUR", selected == 1);
    drawText(scene, "CROIX  VALIDER       ROND  RETOUR", 116, 970, 2, {205, 205, 194});
}

static void drawContinue(Scene2D &scene)
{
    drawTitle(scene, "CONTINUER");
    drawTextShadow(scene, "AUCUNE SAUVEGARDE", 116, 485, 3, {236, 228, 213});
    drawText(scene, "LES PARTIES SAUVEGARDEES APPARAITRONT ICI", 116, 540, 2, {165, 170, 164});
    drawItem(scene, 740, "RETOUR", true);
    drawText(scene, "ROND  RETOUR", 116, 970, 2, {205, 205, 194});
}

static void drawOptions(Scene2D &scene)
{
    drawTitle(scene, "OPTIONS");
    drawItem(scene, 475, vibrationEnabled ? "VIBRATION  ACTIVE" : "VIBRATION  DESACTIVEE", selected == 0);
    drawItem(scene, 555, "RETOUR", selected == 1);
    drawText(scene, "CROIX  MODIFIER       ROND  RETOUR", 116, 970, 2, {205, 205, 194});
}

static void drawIntro(Scene2D &scene)
{
    drawTitle(scene, "PROLOGUE");
    drawTextShadow(scene, "SIGNAL LOST", 116, 465, 4, {220, 78, 57});
    drawText(scene, "DIFFICULTE", 116, 575, 2, {170, 174, 168});
    const char *names[3] = {"FACILE", "NORMAL", "DIFFICILE"};
    drawText(scene, names[difficulty], 116, 615, 3, {235, 229, 215});
    drawText(scene, "LE PROLOGUE SERA AJOUTE DANS LA PROCHAINE ETAPE", 116, 760, 2, {165, 170, 164});
    drawText(scene, "ROND  RETOUR AU MENU", 116, 970, 2, {205, 205, 194});
}

static void render(Scene2D &scene, PNG &background, int frameId)
{
    scene.FrameBufferFill({0, 0, 0});
    if (background.IsLoaded())
        background.DrawScaled(&scene, 0, 0, SCREEN_W, SCREEN_H);

    // Aucun panneau opaque : l'illustration d'origine reste visible sur tout l'écran.
    if (page == PAGE_MAIN) drawMain(scene);
    else if (page == PAGE_NEW_GAME) drawNewGame(scene);
    else if (page == PAGE_CONTINUE) drawContinue(scene);
    else if (page == PAGE_OPTIONS) drawOptions(scene);
    else drawIntro(scene);

    scene.SubmitFlip(frameId);
    scene.FrameWait(frameId);
}

static unsigned int pollPressed(int pad, OrbisPadData &state)
{
    std::memset(&state, 0, sizeof(state));
    if (pad < 0) {
        lastPadReadResult = -1;
        previousButtons = 0;
        return 0;
    }

    lastPadReadResult = scePadReadState(pad, &state);
    if (lastPadReadResult != 0) {
        previousButtons = 0;
        return 0;
    }

    const unsigned int menuMask =
        ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN |
        ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT |
        ORBIS_PAD_BUTTON_CROSS | ORBIS_PAD_BUTTON_CIRCLE;
    const unsigned int current = state.buttons & menuMask;
    unsigned int pressed = current & ~previousButtons;
    previousButtons = current;
    lastPadButtons = (int)state.buttons;

    // Secours analogique : les sticks PS4 sont centres autour de 128.
    // Une zone morte evite les mouvements fantomes au repos.
    if (pressed == 0) {
        const int y = (int)state.leftStick.y - 128;
        const int x = (int)state.leftStick.x - 128;
        if (y < -48) {
            if (stickRepeat == 0) { pressed |= ORBIS_PAD_BUTTON_UP; stickRepeat = 9; }
        } else if (y > 48) {
            if (stickRepeat == 0) { pressed |= ORBIS_PAD_BUTTON_DOWN; stickRepeat = 9; }
        } else if (x < -48) {
            if (stickRepeat == 0) { pressed |= ORBIS_PAD_BUTTON_LEFT; stickRepeat = 9; }
        } else if (x > 48) {
            if (stickRepeat == 0) { pressed |= ORBIS_PAD_BUTTON_RIGHT; stickRepeat = 9; }
        } else {
            stickRepeat = 0;
        }
    }
    if (stickRepeat > 0) --stickRepeat;
    return pressed;
}

static void applyVibration(int pad)
{
    if (pad < 0 || !vibrationEnabled) return;
    OrbisPadVibeParam vibe = {};
    vibe.lgMotor = 0;
    vibe.smMotor = 70;
    scePadSetVibration(pad, &vibe);
    sceKernelUsleep(35000);
    OrbisPadVibeParam off = {};
    scePadSetVibration(pad, &off);
}

int main()
{
    const size_t frameBytes = (size_t)SCREEN_W * (size_t)SCREEN_H * 4u;
    write_status("BLACKOUT_MENU_STARTING");

    Scene2D scene(SCREEN_W, SCREEN_H, 4);
    if (!scene.Init(frameBytes * 2u, 2)) {
        write_status("BLACKOUT_VIDEO_INIT_FAILED");
        for (;;) sceKernelUsleep(1000000);
    }

    PNG background("/app0/USRDIR/BLACKOUT_MENU.png");
    if (!background.IsLoaded())
        write_status("BLACKOUT_MENU_IMAGE_LOAD_FAILED");

    const int padInit = scePadInit();
    int pad = -1;
    if (padInit == 0)
        pad = scePadOpen(0x10000000, 0, 0, NULL);

    lastPadHandle = pad;
    if (pad < 0) {
        char message[96];
        std::snprintf(message, sizeof(message), "BLACKOUT_PAD_OPEN_FAILED rc=%d", pad);
        write_status(message);
    } else {
        write_status("BLACKOUT_PAD_CONNECTED");
        OrbisPadData initial = {};
        if (scePadReadState(pad, &initial) == 0)
            previousButtons = initial.buttons & (ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN |
                ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT |
                ORBIS_PAD_BUTTON_CROSS | ORBIS_PAD_BUTTON_CIRCLE);
    }

    int frameId = 0;
    for (;;) {
        OrbisPadData state = {};
        const unsigned int pressed = pollPressed(pad, state);
        static int diagnosticTicks = 0;
        if ((diagnosticTicks++ % 60) == 0 && pad >= 0) {
            char message[128];
            std::snprintf(message, sizeof(message),
                "PAD handle=%d read=%d buttons=%04x connected=%u",
                lastPadHandle, lastPadReadResult, lastPadButtons,
                (unsigned)state.connected);
            write_status(message);
        }
        bool changed = false;

        if (page == PAGE_MAIN) {
            if (pressed & ORBIS_PAD_BUTTON_UP) { selected = (selected + 2) % 3; changed = true; }
            if (pressed & ORBIS_PAD_BUTTON_DOWN) { selected = (selected + 1) % 3; changed = true; }
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                applyVibration(pad);
                if (selected == 0) { page = PAGE_NEW_GAME; selected = 0; }
                else if (selected == 1) { page = PAGE_CONTINUE; selected = 0; }
                else { page = PAGE_OPTIONS; selected = 0; }
                changed = true;
            }
        } else if (page == PAGE_NEW_GAME) {
            if (pressed & ORBIS_PAD_BUTTON_LEFT) { difficulty = (difficulty + 2) % 3; changed = true; }
            if (pressed & ORBIS_PAD_BUTTON_RIGHT) { difficulty = (difficulty + 1) % 3; changed = true; }
            if (pressed & ORBIS_PAD_BUTTON_UP) { selected = (selected + 1) % 2; changed = true; }
            if (pressed & ORBIS_PAD_BUTTON_DOWN) { selected = (selected + 1) % 2; changed = true; }
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                applyVibration(pad);
                if (selected == 0) { page = PAGE_INTRO; write_status("BLACKOUT_NEW_GAME_READY"); }
                else { page = PAGE_MAIN; selected = 0; }
                changed = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) { page = PAGE_MAIN; selected = 0; changed = true; }
        } else if (page == PAGE_OPTIONS) {
            if (pressed & (ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN)) {
                selected = (selected + 1) % 2; changed = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                if (selected == 0) {
                    vibrationEnabled = !vibrationEnabled;
                    if (!vibrationEnabled && pad >= 0) {
                        OrbisPadVibeParam off = {};
                        scePadSetVibration(pad, &off);
                    } else applyVibration(pad);
                } else { page = PAGE_MAIN; selected = 0; }
                changed = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) { page = PAGE_MAIN; selected = 0; changed = true; }
        } else {
            if (pressed & (ORBIS_PAD_BUTTON_CIRCLE | ORBIS_PAD_BUTTON_CROSS)) {
                page = PAGE_MAIN; selected = 0; changed = true;
            }
        }

        if (frameId == 0 || changed) {
            scene.SetActiveFrameBuffer(frameId % 2);
            render(scene, background, frameId++);
        }
        sceKernelUsleep(16000);
    }
    return 0;
}
