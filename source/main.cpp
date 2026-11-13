#include <cstdio>
#include "common/graphics.h"
#include "common/png.h"

static void write_status(const char *message)
{
    FILE *log = std::fopen("/data/blackout_diag.txt", "w");
    if (log != nullptr) {
        std::fprintf(log, "%s\n", message);
        std::fflush(log);
        std::fclose(log);
    }
}

int main()
{
    const int width = 1920;
    const int height = 1080;
    const size_t frameBytes = (size_t)width * (size_t)height * 4u;

    write_status("BLACKOUT_MENU_IMAGE_STARTING");

    Scene2D scene(width, height, 4);
    if (!scene.Init(frameBytes * 2u, 2)) {
        write_status("BLACKOUT_VIDEO_INIT_FAILED");
        for (;;) sceKernelUsleep(1000000);
    }

    // Affiche directement l'image originale fournie pour le menu.
    PNG menu("/app0/USRDIR/BLACKOUT_MENU.png");

    scene.SetActiveFrameBuffer(0);
    scene.FrameBufferFill({ 0, 0, 0 });
    menu.Draw(&scene, 0, 0);
    scene.SubmitFlip(0);
    scene.FrameWait(0);

    scene.FrameBufferSwap();
    scene.FrameBufferFill({ 0, 0, 0 });
    menu.Draw(&scene, 0, 0);
    scene.SubmitFlip(1);
    scene.FrameWait(1);

    write_status("BLACKOUT_MENU_IMAGE_PRESENTED");

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
