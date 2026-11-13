#include <cstdio>
#include <cstring>
#include <orbis/Pad.h>
#include "common/graphics.h"
#include "common/png.h"

static const int SCREEN_W = 1920;
static const int SCREEN_H = 1080;

static void write_status(const char *message)
{
    FILE *log = std::fopen("/data/blackout_diag.txt", "w");
    if (log != nullptr) {
        std::fprintf(log, "%s\n", message);
        std::fflush(log);
        std::fclose(log);
    }
}

static const unsigned char *glyph(char c)
{
    static const unsigned char blank[7] = {0,0,0,0,0,0,0};
    static const unsigned char A[7] = {14,17,17,31,17,17,17};
    static const unsigned char B[7] = {30,17,17,30,17,17,30};
    static const unsigned char C[7] = {14,17,16,16,16,17,14};
    static const unsigned char D[7] = {30,17,17,17,17,17,30};
    static const unsigned char E[7] = {31,16,16,30,16,16,31};
    static const unsigned char I[7] = {31,4,4,4,4,4,31};
    static const unsigned char L[7] = {16,16,16,16,16,16,31};
    static const unsigned char N[7] = {17,25,25,21,19,19,17};
    static const unsigned char O[7] = {14,17,17,17,17,17,14};
    static const unsigned char P[7] = {30,17,17,30,16,16,16};
    static const unsigned char R[7] = {30,17,17,30,20,18,17};
    static const unsigned char S[7] = {15,16,16,14,1,1,30};
    static const unsigned char T[7] = {31,4,4,4,4,4,4};
    static const unsigned char U[7] = {17,17,17,17,17,17,14};
    static const unsigned char V[7] = {17,17,17,17,17,10,4};
    static const unsigned char G[7] = {14,17,16,23,17,17,14};
    static const unsigned char H[7] = {17,17,17,31,17,17,17};
    static const unsigned char M[7] = {17,27,21,21,17,17,17};
    static const unsigned char  zero[7] = {14,17,19,21,25,17,14};
    switch (c) {
        case 'A': return A; case 'B': return B; case 'C': return C;
        case 'D': return D; case 'E': return E; case 'G': return G;
        case 'H': return H; case 'I': return I; case 'L': return L;
        case 'M': return M; case 'N': return N; case 'O': return O;
        case 'P': return P; case 'R': return R; case 'S': return S;
        case 'T': return T; case 'U': return U; case 'V': return V;
        case '0': return zero; default: return blank;
    }
}

static void drawText(Scene2D &scene, const char *text, int x, int y, int scale, Color color)
{
    for (int i = 0; text[i] != '\0'; ++i) {
        const unsigned char *rows = glyph(text[i]);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (rows[row] & (1u << (4 - col)))
                    scene.DrawRectangle(x + (i * 6 + col) * scale,
                                        y + row * scale, scale, scale, color);
            }
        }
    }
}

static void drawButton(Scene2D &scene, int y, const char *label, bool selected)
{
    const Color panel = selected ? Color{45, 19, 17} : Color{8, 10, 12};
    const Color border = selected ? Color{230, 65, 38} : Color{90, 75, 70};
    const Color text = selected ? Color{255, 225, 205} : Color{185, 185, 180};

    scene.DrawRectangle(112, y, 590, 82, panel);
    scene.DrawRectangle(112, y, 7, 82, border);
    scene.DrawRectangle(119, y, 583, 2, border);
    scene.DrawRectangle(119, y + 80, 583, 2, border);
    if (selected)
        scene.DrawRectangle(680, y + 12, 5, 58, border);
    drawText(scene, label, 155, y + 27, 4, text);
}

static void render(Scene2D &scene, PNG &background, int selected, int page, int frameId)
{
    scene.FrameBufferFill({0, 0, 0});
    if (background.IsLoaded())
        background.DrawScaled(&scene, 0, 0, SCREEN_W, SCREEN_H);

    if (page == 0) {
        drawButton(scene, 650, "NOUVELLE PARTIE", selected == 0);
        drawButton(scene, 750, "CONTINUER", selected == 1);
        drawButton(scene, 850, "OPTIONS", selected == 2);
        drawText(scene, "CROIX VALIDER", 120, 985, 3, {225, 225, 215});
        drawText(scene, "ROND RETOUR", 520, 985, 3, {175, 175, 170});
    } else if (page == 1) {
        scene.DrawRectangle(100, 620, 780, 300, {5, 8, 10});
        scene.DrawRectangle(100, 620, 8, 300, {220, 55, 35});
        drawText(scene, "PROLOGUE", 145, 680, 7, {245, 235, 220});
        drawText(scene, "SIGNAL LOST", 145, 770, 5, {220, 65, 45});
        drawText(scene, "DEBUT DE PARTIE A VENIR", 145, 850, 3, {205, 205, 195});
        drawText(scene, "ROND RETOUR", 145, 965, 3, {225, 225, 215});
    } else if (page == 2) {
        scene.DrawRectangle(100, 620, 780, 300, {5, 8, 10});
        scene.DrawRectangle(100, 620, 8, 300, {220, 55, 35});
        drawText(scene, "AUCUNE SAUVEGARDE", 145, 700, 5, {245, 235, 220});
        drawText(scene, "ROND RETOUR", 145, 820, 4, {225, 225, 215});
    } else {
        scene.DrawRectangle(100, 620, 780, 300, {5, 8, 10});
        scene.DrawRectangle(100, 620, 8, 300, {220, 55, 35});
        drawText(scene, "OPTIONS", 145, 670, 6, {245, 235, 220});
        drawText(scene, "SON ACTIVE", 145, 770, 4, {205, 205, 195});
        drawText(scene, "ROND RETOUR", 145, 850, 4, {225, 225, 215});
    }

    scene.SubmitFlip(frameId);
    scene.FrameWait(frameId);
}

int main()
{
    const size_t frameBytes = (size_t)SCREEN_W * (size_t)SCREEN_H * 4u;
    write_status("BLACKOUT_MENU_FULLSCREEN_STARTING");

    Scene2D scene(SCREEN_W, SCREEN_H, 4);
    if (!scene.Init(frameBytes * 2u, 2)) {
        write_status("BLACKOUT_VIDEO_INIT_FAILED");
        for (;;) sceKernelUsleep(1000000);
    }

    PNG background("/app0/USRDIR/BLACKOUT_MENU.png");
    if (!background.IsLoaded())
        write_status("BLACKOUT_MENU_IMAGE_LOAD_FAILED");

    int pad = -1;
    if (scePadInit() == 0)
        pad = scePadOpen(1, 0, 0, NULL);

    int selected = 0;
    int page = 0;
    unsigned int previousButtons = 0;
    int frameId = 0;

    for (;;) {
        OrbisPadData state;
        std::memset(&state, 0, sizeof(state));
        unsigned int pressed = 0;
        if (pad >= 0 && scePadReadState(pad, &state) == 0) {
            pressed = state.buttons & ~previousButtons;
            previousButtons = state.buttons;
        }

        bool changed = false;
        if (page == 0) {
            if (pressed & ORBIS_PAD_BUTTON_UP) {
                selected = (selected + 2) % 3;
                changed = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_DOWN) {
                selected = (selected + 1) % 3;
                changed = true;
            }
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                page = selected + 1;
                changed = true;
                if (selected == 0) write_status("BLACKOUT_NEW_GAME_SELECTED");
                else if (selected == 1) write_status("BLACKOUT_CONTINUE_SELECTED");
                else write_status("BLACKOUT_OPTIONS_OPENED");
            }
        } else if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
            page = 0;
            changed = true;
            write_status("BLACKOUT_MAIN_MENU");
        }

        if (frameId == 0 || changed) {
            scene.SetActiveFrameBuffer(0);
            render(scene, background, selected, page, frameId++);
            scene.FrameBufferSwap();
            render(scene, background, selected, page, frameId++);
            scene.FrameBufferSwap();
        }
        sceKernelUsleep(16000);
    }

    return 0;
}
