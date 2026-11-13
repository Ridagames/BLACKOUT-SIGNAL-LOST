#include <cstdio>
#include <cstdint>
#include <cstring>
#include <sstream>

#include <orbis/libkernel.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>

#include "graphics.h"
#include "png.h"

#define SCREEN_W 1920
#define SCREEN_H 1080
#define FRAME_DEPTH 4
#define VIDEO_MEMORY_SIZE (32 * 1024 * 1024)

std::stringstream debugLogStream;

static const uint8_t FONT[26][7] = {
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
    {7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};

static void drawText(Scene2D *scene, const char *text, int x, int y, int scale, Color color)
{
    for (size_t i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
        if (c >= 'A' && c <= 'Z') {
            const uint8_t *glyph = FONT[c - 'A'];
            for (int row = 0; row < 7; ++row)
                for (int col = 0; col < 5; ++col)
                    if (glyph[row] & (1 << (4 - col)))
                        scene->DrawRectangle(x + col * scale, y + row * scale, scale, scale, color);
        }
        x += 6 * scale;
    }
}

static void drawMenu(Scene2D *scene, PNG *background, int selected, int screen)
{
    Color black = {8, 10, 14};
    Color white = {235, 239, 242};
    Color red = {190, 28, 35};
    Color muted = {125, 132, 140};
    scene->FrameBufferFill(black);
    background->Draw(scene, 0, 0);

    scene->DrawRectangle(105, 170, 760, 720, black);
    scene->DrawRectangle(105, 170, 8, 720, red);
    drawText(scene, "BLACKOUT", 160, 220, 9, white);
    drawText(scene, "SIGNAL LOST", 165, 315, 4, red);

    if (screen == 0) {
        const char *items[] = {"NOUVELLE PARTIE", "CONTINUER", "OPTIONS", "QUITTER"};
        for (int i = 0; i < 4; ++i) {
            int y = 470 + i * 92;
            if (i == selected) {
                scene->DrawRectangle(145, y - 15, 650, 62, red);
                drawText(scene, items[i], 175, y, 4, white);
            } else {
                drawText(scene, items[i], 175, y, 4, (i == 1 ? muted : white));
            }
        }
        drawText(scene, "FLECHES : NAVIGUER", 160, 835, 2, muted);
        drawText(scene, "X : VALIDER", 160, 875, 2, muted);
    } else if (screen == 1) {
        drawText(scene, "CHAPITRE 01", 165, 485, 5, red);
        drawText(scene, "SIGNAL PERDU", 165, 565, 4, white);
        drawText(scene, "LA NUIT EST TOMBEE", 165, 665, 3, white);
        drawText(scene, "APPUYEZ SUR O POUR RETOUR", 165, 790, 2, muted);
    } else {
        drawText(scene, "OPTIONS", 165, 485, 5, red);
        drawText(scene, "COMMANDES : MANETTE", 165, 590, 3, white);
        drawText(scene, "APPUYEZ SUR O POUR RETOUR", 165, 790, 2, muted);
    }
}

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    Scene2D scene(SCREEN_W, SCREEN_H, FRAME_DEPTH);
    if (!scene.Init(VIDEO_MEMORY_SIZE, 2)) {
        std::printf("BLACKOUT: video initialization failed\n");
        for (;;) sceKernelUsleep(1000000);
    }
    scene.SetActiveFrameBuffer(0);

    if (scePadInit() != 0) {
        std::printf("BLACKOUT: controller initialization failed\n");
        for (;;) sceKernelUsleep(1000000);
    }

    OrbisUserServiceInitializeParams userParams{};
    userParams.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    sceUserServiceInitialize(&userParams);
    int userId = -1;
    sceUserServiceGetInitialUser(&userId);
    int pad = scePadOpen(userId, 0, 0, nullptr);
    if (pad < 0) {
        std::printf("BLACKOUT: controller not available\n");
        for (;;) sceKernelUsleep(1000000);
    }

    PNG background("/app0/USRDIR/BLACKOUT_MENU.png");
    OrbisPadData padData{};
    uint32_t previousButtons = 0;
    int selected = 0;
    int screen = 0;
    int frame = 0;
    bool running = true;

    while (running) {
        scePadReadState(pad, &padData);
        const uint32_t buttons = padData.buttons;
        const uint32_t pressed = buttons & ~previousButtons;
        previousButtons = buttons;

        if (screen == 0) {
            if (pressed & ORBIS_PAD_BUTTON_UP) selected = (selected + 3) % 4;
            if (pressed & ORBIS_PAD_BUTTON_DOWN) selected = (selected + 1) % 4;
            if (pressed & ORBIS_PAD_BUTTON_CROSS) {
                if (selected == 0) screen = 1;
                else if (selected == 2) screen = 2;
                else if (selected == 3) running = false;
            }
        } else if (pressed & ORBIS_PAD_BUTTON_CIRCLE) {
            screen = 0;
        }

        drawMenu(&scene, &background, selected, screen);
        scene.SubmitFlip(frame);
        scene.FrameWait(frame);
        scene.FrameBufferSwap();
        ++frame;
    }
    return 0;
}
