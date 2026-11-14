#include <cstdio>
#include <cstring>
#include <cstdint>
#include <orbis/libkernel.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
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
static int diagnosticPadInit = -999;
static int diagnosticUserInit = -999;
static int diagnosticUserResult = -999;
static int diagnosticActiveUser = -1;
static unsigned int diagnosticConnected = 0;

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

#if DIAGNOSTIC_BUILD
static void drawDiagnostics(Scene2D &scene)
{
    char line[96];
    const Color white = {225, 228, 220};
    const Color red = {230, 105, 78};
    const Color green = {120, 210, 145};

    scene.DrawRectangle(1380, 35, 500, 300, {8, 12, 14});
    scene.DrawRectangle(1380, 35, 4, 300, {218, 75, 54});
    drawText(scene, "PAD DIAGNOSTIC", 1400, 52, 2, white);

    std::snprintf(line, sizeof(line), "PAD INIT %d", diagnosticPadInit);
    drawText(scene, line, 1400, 88, 2, diagnosticPadInit == 0 ? green : red);

    std::snprintf(line, sizeof(line), "USER INIT %d", diagnosticUserInit);
    drawText(scene, line, 1400, 114, 2, diagnosticUserInit == 0 || diagnosticUserInit == 0x80960003 ? green : red);

    std::snprintf(line, sizeof(line), "USER RESULT %d", diagnosticUserResult);
    drawText(scene, line, 1400, 140, 2, diagnosticUserResult == 0 ? green : red);

    std::snprintf(line, sizeof(line), "USER ID %d", diagnosticActiveUser);
    drawText(scene, line, 1400, 166, 2, white);

    std::snprintf(line, sizeof(line), "PAD HANDLE %d", lastPadHandle);
    drawText(scene, line, 1400, 192, 2, lastPadHandle >= 0 ? green : red);

    std::snprintf(line, sizeof(line), "READ %d", lastPadReadResult);
    drawText(scene, line, 1400, 218, 2, lastPadReadResult >= 0 ? green : red);

    std::snprintf(line, sizeof(line), "BUTTONS %08X", (unsigned int)lastPadButtons);
    drawText(scene, line, 1400, 244, 2, white);

    std::snprintf(line, sizeof(line), "CONNECTED %u", diagnosticConnected);
    drawText(scene, line, 1400, 270, 2, diagnosticConnected ? green : red);
}

#endif

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

    #if DIAGNOSTIC_BUILD
    drawDiagnostics(scene);
    #endif
    scene.SubmitFlip(frameId);
    scene.FrameWait(frameId);
}

static unsigned int pollPressed(int pad, OrbisPadData &state)
{
    std::memset(&state, 0, sizeof(state));

    if (pad < 0) {
        lastPadReadResult = -1;
        lastPadButtons = 0;
        previousButtons = 0;
        stickRepeat = 0;
        return 0;
    }

    // Lecture unique de l'etat complet de la DualShock 4.
    lastPadReadResult = scePadReadState(pad, &state);
    if (lastPadReadResult < 0) {
        lastPadButtons = 0;
        previousButtons = 0;
        stickRepeat = 0;
        diagnosticConnected = 0;
        return 0;
    }
    diagnosticConnected = (unsigned)state.connected;
    if (!state.connected) {
        lastPadButtons = 0;
        previousButtons = 0;
        stickRepeat = 0;
        return 0;
    }

    const unsigned int directionMask =
        ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN |
        ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT;
    const unsigned int menuMask = directionMask |
        ORBIS_PAD_BUTTON_CROSS | ORBIS_PAD_BUTTON_CIRCLE;

    // Les sticks PS4 renvoient des valeurs de 0 a 255, centre a 128.
    // Zone morte volontaire pour eviter les mouvements parasites.
    const int deadZoneLow = 64;
    const int deadZoneHigh = 192;
    unsigned int current = state.buttons & menuMask;

    // Stick gauche ET stick droit peuvent naviguer dans les menus.
    if (state.leftStick.y < deadZoneLow || state.rightStick.y < deadZoneLow)
        current |= ORBIS_PAD_BUTTON_UP;
    if (state.leftStick.y > deadZoneHigh || state.rightStick.y > deadZoneHigh)
        current |= ORBIS_PAD_BUTTON_DOWN;
    if (state.leftStick.x < deadZoneLow || state.rightStick.x < deadZoneLow)
        current |= ORBIS_PAD_BUTTON_LEFT;
    if (state.leftStick.x > deadZoneHigh || state.rightStick.x > deadZoneHigh)
        current |= ORBIS_PAD_BUTTON_RIGHT;

    unsigned int pressed = current & ~previousButtons;
    const unsigned int heldDirections = current & directionMask;

    // Repeter le mouvement si le stick ou la croix reste maintenu :
    // premier repete apres environ 350 ms, puis toutes les 100 ms.
    if (heldDirections) {
        if (pressed & directionMask) {
            stickRepeat = 0;
        } else {
            ++stickRepeat;
            if (stickRepeat >= 22 && ((stickRepeat - 22) % 6) == 0)
                pressed |= heldDirections;
        }
    } else {
        stickRepeat = 0;
    }

    previousButtons = current;
    lastPadButtons = (int)state.buttons;
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

    PNG background0("/app0/USRDIR/BLACKOUT_MENU_00.png");
    PNG background1("/app0/USRDIR/BLACKOUT_MENU_01.png");
    PNG background2("/app0/USRDIR/BLACKOUT_MENU_02.png");
    PNG background3("/app0/USRDIR/BLACKOUT_MENU_03.png");
    PNG background4("/app0/USRDIR/BLACKOUT_MENU_04.png");
    PNG background5("/app0/USRDIR/BLACKOUT_MENU_05.png");
    PNG *backgrounds[6] = {&background0, &background1, &background2, &background3, &background4, &background5};
    if (!background0.IsLoaded())
        write_status("BLACKOUT_MENU_ANIMATION_LOAD_FAILED");

    // Initialisation et ouverture reprises du flux de controleur de PS MTP Explorer.
    const int padInit = scePadInit();
    OrbisUserServiceInitializeParams userParams = { ORBIS_KERNEL_PRIO_FIFO_LOWEST };
    const int userInit = sceUserServiceInitialize(&userParams);
    int activeUser = -1;
    // Le depot de reference tente GetInitialUser meme si le service etait deja initialise.
    const int userResult = sceUserServiceGetInitialUser(&activeUser);
    int pad = -1;
    diagnosticPadInit = padInit;
    diagnosticUserInit = userInit;
    diagnosticUserResult = userResult;
    diagnosticActiveUser = activeUser;
    if (padInit == 0 && userResult == 0) {
        pad = scePadOpen(activeUser, 0, 0, NULL);
        // Si le handle est deja ouvert par le systeme, recuperer le handle existant.
        if (pad < 0)
            pad = scePadGetHandle(activeUser, ORBIS_PAD_PORT_TYPE_STANDARD, 0);
    }

    lastPadHandle = pad;
    if (pad < 0) {
        char message[128];
        std::snprintf(message, sizeof(message),
            "PAD_OPEN_FAILED init=%d userResult=%d user=%d handle=%d",
            padInit, userResult, activeUser, pad);
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
        diagnosticConnected = (unsigned)state.connected;
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

        if (frameId == 0 || changed || (diagnosticTicks % 4) == 0) {
            scene.SetActiveFrameBuffer(frameId % 2);
            const int animationFrame = (frameId / 8) % 6;
            render(scene, *backgrounds[animationFrame], frameId++);
        }
        sceKernelUsleep(16000);
    }
    return 0;
}
