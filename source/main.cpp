#include <cstdio>
#include <cstdint>
#include <orbis/libkernel.h>
#include "common/graphics.h"

// BLACKOUT: SIGNAL LOST - first in-game main menu screen.
// Controller navigation will be added after this screen is validated on hardware.

static void write_status(const char *message)
{
    FILE *log = std::fopen("/data/blackout_diag.txt", "w");
    if (log != nullptr) {
        std::fprintf(log, "%s\n", message);
        std::fflush(log);
        std::fclose(log);
    }
}

static const uint8_t *glyph(char c)
{
    static const uint8_t blank[7] = {0,0,0,0,0,0,0};
#define G(name,a,b,c,d,e,f,g) static const uint8_t name[7] = {a,b,c,d,e,f,g}
    G(A,14,17,17,31,17,17,17); G(B,30,17,17,30,17,17,30);
    G(C,14,17,16,16,16,17,14); G(D,30,17,17,17,17,17,30);
    G(E,31,16,16,30,16,16,31); G(F,31,16,16,30,16,16,16);
    G(G,14,17,16,23,17,17,15); G(H,17,17,17,31,17,17,17);
    G(I,14,4,4,4,4,4,14); G(J,7,2,2,2,18,18,12);
    G(K,17,18,20,24,20,18,17); G(L,16,16,16,16,16,16,31);
    G(M,17,27,21,21,17,17,17); G(N,17,25,21,19,17,17,17);
    G(O,14,17,17,17,17,17,14); G(P,30,17,17,30,16,16,16);
    G(Q,14,17,17,17,21,18,13); G(R,30,17,17,30,20,18,17);
    G(S,15,16,16,14,1,1,30); G(T,31,4,4,4,4,4,4);
    G(U,17,17,17,17,17,17,14); G(V,17,17,17,17,17,10,4);
    G(W,17,17,17,21,21,21,10); G(X,17,17,10,4,10,17,17);
    G(Y,17,17,10,4,4,4,4); G(Z,31,1,2,4,8,16,31);
    G(N0,14,17,19,21,25,17,14); G(N1,4,12,4,4,4,4,14);
    G(N2,14,17,1,2,4,8,31); G(N3,30,1,1,14,1,1,30);
    G(N4,2,6,10,18,31,2,2); G(N5,31,16,16,30,1,1,30);
    G(N6,14,16,16,30,17,17,14); G(N7,31,1,2,4,8,8,8);
    G(N8,14,17,17,14,17,17,14); G(N9,14,17,17,15,1,1,14);
    G(DASH,0,0,0,31,0,0,0); G(DOT,0,0,0,0,0,12,12);
    G(COLON,0,12,12,0,12,12,0); G(EXCL,4,4,4,4,4,0,4);
    G(APOS,12,12,8,0,0,0,0);
#undef G
    switch (c) {
        case 'A': return A; case 'B': return B; case 'C': return C;
        case 'D': return D; case 'E': return E; case 'F': return F;
        case 'G': return G; case 'H': return H; case 'I': return I;
        case 'J': return J; case 'K': return K; case 'L': return L;
        case 'M': return M; case 'N': return N; case 'O': return O;
        case 'P': return P; case 'Q': return Q; case 'R': return R;
        case 'S': return S; case 'T': return T; case 'U': return U;
        case 'V': return V; case 'W': return W; case 'X': return X;
        case 'Y': return Y; case 'Z': return Z;
        case '0': return N0; case '1': return N1; case '2': return N2;
        case '3': return N3; case '4': return N4; case '5': return N5;
        case '6': return N6; case '7': return N7; case '8': return N8;
        case '9': return N9; case '-': return DASH; case '.': return DOT;
        case ':': return COLON; case '!': return EXCL; case '\'': return APOS;
        default: return blank;
    }
}

static void drawText(Scene2D &scene, const char *text, int x, int y,
                     int scale, Color color)
{
    for (const char *p = text; *p; ++p) {
        if (*p == ' ') {
            x += 6 * scale;
            continue;
        }

        const uint8_t *rows = glyph(*p);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (rows[row] & (uint8_t)(0x10 >> col))
                    scene.DrawRectangle(x + col * scale, y + row * scale,
                                        scale, scale, color);
            }
        }
        x += 6 * scale;
    }
}

static void drawMenu(Scene2D &scene)
{
    const int width = 1920;
    const int height = 1080;

    const Color background = { 9, 11, 16 };
    const Color panel = { 17, 20, 27 };
    const Color panelLine = { 43, 47, 55 };
    const Color red = { 190, 40, 30 };
    const Color redBright = { 225, 70, 50 };
    const Color white = { 225, 226, 224 };
    const Color muted = { 112, 119, 128 };

    scene.FrameBufferFill(background);

    // Atmospheric background panels and restrained red accents.
    scene.DrawRectangle(0, 0, 14, height, red);
    scene.DrawRectangle(14, 0, 3, height, panelLine);
    scene.DrawRectangle(1700, 0, 220, height, panel);
    scene.DrawRectangle(1700, 0, 4, height, panelLine);
    scene.DrawRectangle(220, 180, 1180, 2, panelLine);
    scene.DrawRectangle(220, 870, 1180, 2, panelLine);
    scene.DrawRectangle(220, 180, 8, 90, red);
    scene.DrawRectangle(220, 870, 8, 70, red);

    drawText(scene, "BLACKOUT", 270, 285, 15, white);
    drawText(scene, "SIGNAL LOST", 278, 420, 7, redBright);
    drawText(scene, "SURVIVAL HORROR", 282, 500, 3, muted);

    // Main menu panel.
    scene.DrawRectangle(270, 610, 760, 300, panel);
    scene.DrawRectangle(270, 610, 760, 3, panelLine);
    scene.DrawRectangle(270, 610, 5, 300, red);

    // First item is highlighted; pad navigation is the next development step.
    scene.DrawRectangle(305, 655, 680, 62, { 39, 24, 29 });
    scene.DrawRectangle(305, 655, 5, 62, redBright);
    scene.DrawRectangle(335, 681, 18, 3, redBright);
    scene.DrawRectangle(335, 681, 3, 18, redBright);
    drawText(scene, "NOUVELLE PARTIE", 380, 674, 4, white);
    drawText(scene, "CONTINUER", 380, 755, 4, muted);
    drawText(scene, "OPTIONS", 380, 835, 4, muted);

    drawText(scene, "MENU PRINCIPAL", 270, 570, 2, muted);
    drawText(scene, "BUILD 0.2", 270, 960, 2, muted);
    drawText(scene, "BLSG 00001", 270, 1000, 2, muted);

    // Subtle right-side signal markings.
    drawText(scene, "NO SIGNAL", 1745, 900, 3, redBright);
    scene.DrawRectangle(1745, 950, 130, 3, red);
    scene.DrawRectangle(1745, 965, 82, 3, panelLine);
    scene.DrawRectangle(1745, 980, 104, 3, panelLine);
}

int main()
{
    const int width = 1920;
    const int height = 1080;
    const size_t frameBytes = (size_t)width * (size_t)height * 4u;

    write_status("BLACKOUT_VIDEO_TEST_STARTING");

    Scene2D scene(width, height, 4);
    if (!scene.Init(frameBytes * 2u, 2)) {
        write_status("BLACKOUT_VIDEO_INIT_FAILED");
        for (;;) sceKernelUsleep(1000000);
    }

    scene.SetActiveFrameBuffer(0);
    drawMenu(scene);
    scene.SubmitFlip(0);
    scene.FrameWait(0);

    scene.FrameBufferSwap();
    drawMenu(scene);
    scene.SubmitFlip(1);
    scene.FrameWait(1);

    write_status("BLACKOUT_VIDEO_INIT_OK_SCREEN_PRESENTED");

    int frameId = 2;
    for (;;) {
        scene.FrameBufferSwap();
        scene.SubmitFlip(frameId);
        scene.FrameWait(frameId);
        ++frameId;
        sceKernelUsleep(16000);
    }

    return 0;
}
