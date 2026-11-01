#include <io/pad.h>
#include <sysutil/sysutil.h>
#include <sysmodule/sysmodule.h>
#include <tiny3d.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

static volatile bool running = true;
static FILE *logFile = NULL;

static void logMessage(const char *fmt, ...)
{
    if (!logFile) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(logFile, fmt, args);
    va_end(args);
    fputc('\n', logFile);
    fflush(logFile);
}

/* Both a fresh load and an already-loaded module are successful states. */
static bool moduleOk(int result)
{
    return result == SYSMODULE_LOADED || result == (int)SYSMODULE_ERR_DUPLICATE;
}

static void sysCallback(u64 status, u64 param, void *userdata)
{
    (void)param;
    (void)userdata;
    logMessage("sysutil callback status=%llu", (unsigned long long)status);
    if (status == SYSUTIL_EXIT_GAME) {
        logMessage("SYSUTIL_EXIT_GAME received");
        running = false;
    }
}

/* Minimal 5x7 font: avoids external font assets and keeps the diagnostic build self-contained. */
static const uint8_t glyphs[][5] = {
    {0x7e,0x11,0x11,0x11,0x7e}, {0x7f,0x49,0x49,0x49,0x36},
    {0x3e,0x41,0x41,0x41,0x22}, {0x7f,0x41,0x41,0x22,0x1c},
    {0x7f,0x49,0x49,0x49,0x41}, {0x7f,0x09,0x09,0x09,0x01},
    {0x3e,0x41,0x49,0x49,0x7a}, {0x7f,0x08,0x08,0x08,0x7f},
    {0x00,0x41,0x7f,0x41,0x00}, {0x20,0x40,0x41,0x3f,0x01},
    {0x7f,0x08,0x14,0x22,0x41}, {0x7f,0x40,0x40,0x40,0x40},
    {0x7f,0x02,0x0c,0x02,0x7f}, {0x7f,0x04,0x08,0x10,0x7f},
    {0x3e,0x41,0x41,0x41,0x3e}, {0x7f,0x09,0x09,0x09,0x06},
    {0x3e,0x41,0x51,0x21,0x5e}, {0x7f,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7f,0x01,0x01},
    {0x3f,0x40,0x40,0x40,0x3f}, {0x1f,0x20,0x40,0x20,0x1f},
    {0x3f,0x40,0x38,0x40,0x3f}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43},
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5f,0x00,0x00},
    {0x00,0x06,0x09,0x09,0x06}, {0x00,0x36,0x36,0x00,0x00},
    {0x00,0x08,0x08,0x08,0x08}
};

static int glyphIndex(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c == ' ') return 26;
    if (c == '>') return 27;
    if (c == ':') return 28;
    if (c == '.') return 29;
    if (c == '-') return 30;
    return 26;
}

static void rect(float x, float y, float w, float h, uint32_t color)
{
    tiny3d_SetPolygon(TINY3D_QUADS);
    tiny3d_VertexPos(x, y, 0.0f); tiny3d_VertexColor(color);
    tiny3d_VertexPos(x+w, y, 0.0f); tiny3d_VertexColor(color);
    tiny3d_VertexPos(x+w, y+h, 0.0f); tiny3d_VertexColor(color);
    tiny3d_VertexPos(x, y+h, 0.0f); tiny3d_VertexColor(color);
    tiny3d_End();
}

static void drawText(float x, float y, const char *s, float scale, uint32_t color)
{
    while (*s) {
        int g = glyphIndex(*s++);
        for (int col=0; col<5; ++col)
            for (int row=0; row<7; ++row)
                if (glyphs[g][col] & (1u << row))
                    rect(x + col*scale, y + row*scale, scale, scale, color);
        x += 6*scale;
    }
}

int main(void)
{
    int res;

    res = sysModuleLoad(SYSMODULE_FS);
    logFile = fopen("/dev_hdd0/tmp/blackout_log.txt", "w");
    if (logFile) {
        setvbuf(logFile, NULL, _IONBF, 0);
        logMessage("BLACKOUT diagnostic log started");
        logMessage("stage: main entered");
    }
    logMessage("stage: FS res=%d ok=%d", res, moduleOk(res));

    res = sysModuleLoad(SYSMODULE_SYSUTIL);
    logMessage("stage: SYSUTIL res=%d ok=%d", res, moduleOk(res));
    if (!moduleOk(res)) {
        logMessage("ERROR: SYSUTIL");
        if (logFile) fclose(logFile);
        return 1;
    }

    sysUtilRegisterCallback(0, sysCallback, NULL);
    logMessage("stage: sysUtilRegisterCallback done");

    res = sysModuleLoad(SYSMODULE_IO);
    logMessage("stage: IO res=%d ok=%d", res, moduleOk(res));
    if (!moduleOk(res)) {
        logMessage("ERROR: IO");
        if (logFile) fclose(logFile);
        return 1;
    }

    int padResult = ioPadInit(7);
    logMessage("stage: ioPadInit result=%d", padResult);
    if (padResult < 0) {
        logMessage("ERROR: ioPadInit failed");
        if (logFile) fclose(logFile);
        return 1;
    }

    res = sysModuleLoad(SYSMODULE_GCM_SYS);
    logMessage("stage: GCM_SYS res=%d ok=%d", res, moduleOk(res));
    if (!moduleOk(res)) {
        logMessage("ERROR: GCM_SYS");
        ioPadEnd();
        if (logFile) fclose(logFile);
        return 2;
    }

    logMessage("stage: before tiny3d_Init");
    res = tiny3d_Init(1024*1024);
    logMessage("stage: tiny3d_Init res=%d", res);
    if (res != TINY3D_OK) {
        logMessage("ERROR: tiny3d_Init failed");
        ioPadEnd();
        if (logFile) fclose(logFile);
        return 2;
    }

    int selected = 0;
    padInfo padinfo;
    padData paddata;
    bool prevUp=false, prevDown=false, prevCross=false;
    unsigned long frame = 0;

    while (running) {
        if ((frame % 60) == 0) logMessage("stage: main loop frame=%lu", frame);
        sysUtilCheckCallback();
        ioPadGetInfo(&padinfo);
        if (padinfo.status[0]) {
            ioPadGetData(0, &paddata);
            bool up = (paddata.BTN_UP != 0);
            bool down = (paddata.BTN_DOWN != 0);
            bool cross = (paddata.BTN_CROSS != 0);
            if (up && !prevUp) selected = (selected + 2) % 3;
            if (down && !prevDown) selected = (selected + 1) % 3;
            if (cross && !prevCross && selected == 2) {
                logMessage("stage: quit selected");
                running = false;
            }
            prevUp=up; prevDown=down; prevCross=cross;
        }

        if (frame == 0) logMessage("stage: before first tiny3d frame");
        tiny3d_Clear(0xff080b12, TINY3D_CLEAR_ALL);
        tiny3d_Project2D();
        rect(0, 0, 848, 512, 0xff080b12);
        rect(40, 42, 768, 3, 0xff26364a);
        drawText(48, 75, "BLACKOUT", 6, 0xffe7edf5);
        drawText(48, 140, "SIGNAL LOST", 3, 0xff4ca6ff);
        const char *items[3] = {"JOUER", "OPTIONS", "QUITTER"};
        for (int i=0; i<3; ++i) {
            uint32_t color = (i == selected) ? 0xffffff40 : 0xffd8dee8;
            if (i == selected) rect(48, 220 + i*55, 260, 38, 0xff182b40);
            drawText(62, 228 + i*55, i == selected ? ">" : " ", 4, color);
            drawText(92, 228 + i*55, items[i], 4, color);
        }
        drawText(48, 470, "FLECHES NAVIGUER X VALIDER", 1, 0xff9aa9bc);
        tiny3d_Flip();
        if (frame == 0) logMessage("stage: first tiny3d_Flip returned");
        ++frame;
    }

    logMessage("stage: cleanup");
    ioPadEnd();
    tiny3d_Exit();
    logMessage("stage: normal exit");
    if (logFile) fclose(logFile);
    return 0;
}
