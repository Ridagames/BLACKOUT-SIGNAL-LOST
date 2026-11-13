#include <cstdio>
#include <orbis/libkernel.h>
#include "common/graphics.h"

// BLACKOUT: first PS4 video-output test.
// Initialize the display, draw a simple screen, and keep presenting frames.

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

    write_status("BLACKOUT_VIDEO_TEST_STARTING");

    Scene2D scene(width, height, 4);
    if (!scene.Init(frameBytes * 2u, 2)) {
        write_status("BLACKOUT_VIDEO_INIT_FAILED");
        for (;;) sceKernelUsleep(1000000);
    }

    const Color background = { 12, 14, 20 };
    const Color accent = { 170, 24, 35 };

    scene.SetActiveFrameBuffer(0);
    scene.FrameBufferFill(background);
    scene.DrawRectangle(0, 0, width, 12, accent);
    scene.DrawRectangle(0, height - 12, width, 12, accent);
    scene.DrawRectangle(80, 120, 8, 840, accent);
    scene.SubmitFlip(0);
    scene.FrameWait(0);

    scene.FrameBufferSwap();
    scene.FrameBufferFill(background);
    scene.DrawRectangle(0, 0, width, 12, accent);
    scene.DrawRectangle(0, height - 12, width, 12, accent);
    scene.DrawRectangle(80, 120, 8, 840, accent);
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
