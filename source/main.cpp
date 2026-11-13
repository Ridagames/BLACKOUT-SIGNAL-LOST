#include <cstdio>
#include <cstring>
#include <cstdint>
#include <orbis/Pad.h>
#include "common/graphics.h"
#include "common/png.h"

static const int SCREEN_W = 1920;
static const int SCREEN_H = 1080;

enum MenuPage {
    PAGE_MAIN = 0,
    PAGE_NEW_GAME,
    PAGE_CONTINUE,
    PAGE_OPTIONS,
    PAGE_INTRO
};

static bool vibrationEnabled = true;
static int selected = 0;
static int difficulty = 1;
static int page = PAGE_MAIN;
static unsigned int previousButtons = 0;
static int repeatDelay = 0;
static int repeatDirection = 0;

static void write_status(const char *message)
{
    FILE *log = std::fopen("/data/blackout_diag.txt", "w");
    if (log) {
        std::fprintf(log, "%s\n", message);
        std::fflush(log);
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

static void drawText(Scene2D &scene, const char *text, int x, int y, int scale, Color color)
{
    for (int i = 0; text[i] != '\0'; ++i) {
        const unsigned char *rows = glyph(text[i]);
        if (text[i] == ' ') continue;
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (rows[row] & (1u << (4 - col)))
                    scene.DrawRectangle(x + (i * 7 + col) * scale,
                                        y + row * scale, scale, scale, color);
    }
}

static void panel(Scene2D &scene, int x, int y, int w, int h)
{
    scene.DrawRectangle(x, y, w, h, {5, 8, 10});
    scene.DrawRectangle(x, y, 5, h, {184, 42, 29});
    scene.DrawRectangle(x + 5, y, w - 5, 2, {70, 48, 43});
    scene.DrawRectangle(x + 5, y + h - 2, w - 5, 2, {70, 48, 43});
}

static void menuItem(Scene2D &scene, int y, const char *label, bool active, bool disabled = false)
{
    Color mainText = disabled ? Color{92, 94, 94} :
                     active ? Color{255, 231, 214} : Color{184, 188, 187};
    if (active) {
        scene.DrawRectangle(112, y - 13, 690, 72, {38, 15, 14});
        scene.DrawRectangle(112, y - 13, 6, 72, {230, 69, 42});
        scene.DrawRectangle(808, y - 13, 3, 72, {115, 44, 35});
        drawText(scene, ">", 137, y + 7, 4, {255, 100, 65});
    }
    drawText(scene, label, 183, y, 4, mainText);
}

static void drawHeader(Scene2D &scene, const char *section)
{
    drawText(scene, "BLACKOUT", 112, 92, 7, {242, 238, 226});
    drawText(scene, "SIGNAL LOST", 115, 160, 3, {221, 74, 49});
    scene.DrawRectangle(112, 215, 690, 3, {164, 49, 34});
    drawText(scene, section, 112, 255, 3, {174, 178, 174});
}

static void drawMain(Scene2D &scene)
{
    panel(scene, 72, 55, 800, 950);
    drawHeader(scene, "MENU PRINCIPAL");
    menuItem(scene, 405, "NOUVELLE PARTIE", selected == 0);
    menuItem(scene, 535, "CONTINUER", selected == 1);
    menuItem(scene, 665, "OPTIONS", selected == 2);
    scene.DrawRectangle(112, 865, 690, 2, {70, 48, 43});
    drawText(scene, "EXPLORER  -  SURVIVRE  -  COMPRENDRE", 112, 900, 2, {145, 151, 148});
    drawText(scene, "CROIX  VALIDER", 112, 958, 2, {220, 220, 208});
    drawText(scene, "ROND  RETOUR", 420, 958, 2, {170, 174, 170});
}

static void drawNewGame(Scene2D &scene)
{
    panel(scene, 72, 55, 800, 950);
    drawHeader(scene, "NOUVELLE PARTIE");
    drawText(scene, "CHOISIR LA DIFFICULTE", 112, 405, 3, {220, 220, 210});
    const char *names[3] = {"FACILE", "NORMAL", "DIFFICILE"};
    for (int i = 0; i < 3; ++i) {
        int x = 112 + i * 225;
        bool active = difficulty == i;
        scene.DrawRectangle(x, 500, 205, 105, active ? Color{48, 19, 16} : Color{13, 17, 18});
        scene.DrawRectangle(x, 500, 205, 4, active ? Color{226, 68, 42} : Color{72, 76, 73});
        if (active) scene.DrawRectangle(x, 500, 5, 105, {226, 68, 42});
        drawText(scene, names[i], x + 20, 545, 3,
                 active ? Color{255, 225, 205} : Color{160, 165, 161});
    }
    drawText(scene, "GAUCHE / DROITE  CHANGER", 112, 680, 2, {170, 175, 170});
    menuItem(scene, 790, "COMMENCER", selected == 0);
    menuItem(scene, 895, "RETOUR", selected == 1);
    drawText(scene, "CROIX  VALIDER     ROND  RETOUR", 112, 970, 2, {205, 207, 198});
}

static void drawContinue(Scene2D &scene)
{
    panel(scene, 72, 55, 800, 950);
    drawHeader(scene, "CONTINUER");
    drawText(scene, "AUCUNE SAUVEGARDE", 112, 490, 4, {235, 226, 211});
    drawText(scene, "LES PARTIES SAUVEGARDEES", 112, 565, 2, {165, 171, 167});
    drawText(scene, "APPARAITRONT ICI", 112, 600, 2, {165, 171, 167});
    menuItem(scene, 790, "RETOUR", true);
    drawText(scene, "ROND  RETOUR", 112, 958, 2, {205, 207, 198});
}

static void drawOptions(Scene2D &scene)
{
    panel(scene, 72, 55, 800, 950);
    drawHeader(scene, "OPTIONS");
    menuItem(scene, 425, vibrationEnabled ? "VIBRATION  ACTIVE" : "VIBRATION  DESACTIVEE", selected == 0);
    menuItem(scene, 545, "RETOUR", selected == 1);
    drawText(scene, "CROIX OU GAUCHE / DROITE  MODIFIER", 112, 760, 2, {165, 171, 167});
    drawText(scene, "ROND  RETOUR", 112, 958, 2, {205, 207, 198});
}

static void drawIntro(Scene2D &scene)
{
    panel(scene, 72, 55, 800, 950);
    drawHeader(scene, "PROLOGUE");
    drawText(scene, "SIGNAL LOST", 112, 460, 6, {226, 72, 48});
    drawText(scene, "DIFFICULTE SELECTIONNEE", 112, 590, 2, {175, 180, 174});
    const char *names[3] = {"FACILE", "NORMAL", "DIFFICILE"};
    drawText(scene, names[difficulty], 112, 630, 3, {240, 230, 215});
    drawText(scene, "LE PROLOGUE SERA AJOUTE", 112, 760, 2, {165, 171, 167});
    drawText(scene, "DANS LA PROCHAINE ETAPE DU JEU", 112, 795, 2, {165, 171, 167});
    drawText(scene, "ROND  RETOUR AU MENU", 112, 958, 2, {205, 207, 198});
}

static void render(Scene2D &scene, PNG &background, int frameId)
{
    scene.FrameBufferFill({0, 0, 0});
    if (background.IsLoaded())
        background.DrawScaled(&scene, 0, 0, SCREEN_W, SCREEN_H);

    // Contraste sombre a gauche pour garder l'illustration visible.
    scene.DrawRectangle(0, 0, 930, SCREEN_H, {3, 5, 7});
    scene.DrawRectangle(930, 0, 5, SCREEN_H, {82, 30, 24});
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
    if (pad < 0 || scePadReadState(pad, &state) != 0) {
        previousButtons = 0;
        return 0;
    }
    unsigned int pressed = state.buttons & ~previousButtons;
    previousButtons = state.buttons;
    return pressed;
}

static int navigationDirection(const OrbisPadData &state, unsigned int pressed)
{
    int direction = 0;
    if (pressed & ORBIS_PAD_BUTTON_UP) direction = -1;
    else if (pressed & ORBIS_PAD_BUTTON_DOWN) direction = 1;
    else if (state.leftStick.y < 75) direction = -1;
    else if (state.leftStick.y > 180) direction = 1;

    if (direction == 0) {
        repeatDirection = 0;
        repeatDelay = 0;
        return 0;
    }
    if (pressed & (ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN)) {
        repeatDirection = direction;
        repeatDelay = 22;
        return direction;
    }
    if (direction != repeatDirection) {
        repeatDirection = direction;
        repeatDelay = 22;
        return direction;
    }
    if (repeatDelay > 0) {
        --repeatDelay;
        return 0;
    }
    repeatDelay = 6;
    return direction;
}

static void applyVibration(int pad)
{
    if (pad < 0 || !vibrationEnabled) return;
    OrbisPadVibeParam vibe;
    vibe.largeMotor = 0;
    vibe.smallMotor = 90;
    scePadSetVibration(pad, &vibe);
    sceKernelUsleep(40000);
    OrbisPadVibeParam off; off.largeMotor = 0; off.smallMotor = 0;
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

    int padInit = scePadInit();
    int pad = -1;
    if (padInit == 0) {
        // Identifiant du premier utilisateur PS4, pas le nombre 1.
        pad = scePadOpen(0x10000000, 0, 0, NULL);
    }
    if (pad < 0) write_status("BLACKOUT_PAD_OPEN_FAILED");
    else write_status("BLACKOUT_PAD_CONNECTED");

    int frameId = 0;
    for (;;) {
        OrbisPadData state;
        unsigned int pressed = pollPressed(pad, state);
        int direction = navigationDirection(state, pressed);
        bool changed = false;

        if (page == PAGE_MAIN) {
            if (direction != 0) {
                selected = (selected + direction + 3) % 3;
                changed = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                applyVibration(pad);
                if (selected == 0) {
                    page = PAGE_NEW_GAME;
                    selected = 0;
                } else if (selected == 1) {
                    page = PAGE_CONTINUE;
                    selected = 0;
                } else if (selected == 2) {
                    page = PAGE_OPTIONS;
                    selected = 0;
                } else {
                    selected = 0;
                }
                changed = true;
            }
        } else if (page == PAGE_NEW_GAME) {
            if (pressed & ORBIS_PAD_BUTTON_LEFT) difficulty = (difficulty + 2) % 3, changed = true;
            if (pressed & ORBIS_PAD_BUTTON_RIGHT) difficulty = (difficulty + 1) % 3, changed = true;
            if (direction != 0) selected = (selected + direction + 2) % 2, changed = true;
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                applyVibration(pad);
                if (selected == 0) {
                    page = PAGE_INTRO;
                    write_status("BLACKOUT_NEW_GAME_READY");
                } else {
                    page = PAGE_MAIN;
                    selected = 0;
                }
                changed = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
                page = PAGE_MAIN; selected = 0; changed = true;
            }
        } else if (page == PAGE_OPTIONS) {
            if (direction != 0) selected = (selected + direction + 2) % 2, changed = true;
            if ((pressed & ORBIS_PAD_BUTTON_CROSS) ||
                (selected == 0 && (pressed & (ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT)))) {
                if (selected == 0) {
                    vibrationEnabled = !vibrationEnabled;
                    if (vibrationEnabled) applyVibration(pad);
                    else if (pad >= 0) {
                        OrbisPadVibeParam off; off.largeMotor = 0; off.smallMotor = 0;
                        scePadSetVibration(pad, &off);
                    }
                } else {
                    page = PAGE_MAIN; selected = 0;
                }
                changed = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
                page = PAGE_MAIN; selected = 0; changed = true;
            }
        } else {
            if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
                page = PAGE_MAIN; selected = 0; changed = true;
            }
        }

        if (page == PAGE_CONTINUE && (pressed & ORBIS_PAD_BUTTON_CROSS)) {
            page = PAGE_MAIN; selected = 0; changed = true;
        }

        if (frameId == 0 || changed || (frameId % 120) == 0) {
            scene.SetActiveFrameBuffer(frameId % 2);
            render(scene, background, frameId++);
        }
        sceKernelUsleep(16000);
    }
    return 0;
}
