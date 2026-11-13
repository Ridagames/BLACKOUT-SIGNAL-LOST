#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <new>
#include <stdint.h>

#include <string>

#include "graphics.h"

Scene2D::~Scene2D()
{
    // Le système libère les ressources vidéo à la fermeture du processus.
}

Scene2D::Scene2D(int w, int h, int pixelDepth)
{
    this->width = w;
    this->height = h;
    this->depth = pixelDepth;
    this->video = -1;
    this->directMemOff = 0;
    this->directMemAllocationSize = 0;
    this->videoMemSP = 0;
    this->videoMem = NULL;
    this->frameBuffers = NULL;
    this->flipQueue = 0;
    this->frameBufferSize = 0;
    this->frameBufferCount = 0;
    this->activeFrameBufferIdx = 0;

    if (w > 0 && h > 0 && pixelDepth > 0)
        this->frameBufferSize = (size_t)w * (size_t)h * (size_t)pixelDepth;
}

bool Scene2D::Init(size_t memSize, int numFrameBuffers)
{
    if (this->width <= 0 || this->height <= 0 || this->depth != 4 ||
        this->frameBufferSize == 0 || numFrameBuffers != 2 ||
        memSize < this->frameBufferSize * (size_t)numFrameBuffers)
        return false;

    this->video = sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN, ORBIS_VIDEO_OUT_BUS_MAIN, 0, 0);
    if (this->video < 0)
        return false;

#ifdef GRAPHICS_USES_FONT
    int rc = sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL);
    if (rc < 0)
        return false;

    rc = FT_Init_FreeType(&this->ftLib);
    if (rc != 0)
        return false;
#endif

    if (!initFlipQueue())
        return false;

    if (!allocateVideoMem(memSize, 0x200000))
        return false;

    if (!allocateFrameBuffers(numFrameBuffers)) {
        deallocateVideoMem();
        return false;
    }

    sceVideoOutSetFlipRate(this->video, 0);
    return true;
}

bool Scene2D::initFlipQueue()
{
    int rc = sceKernelCreateEqueue(&flipQueue, "homebrew flip queue");
    if (rc < 0)
        return false;

    return sceVideoOutAddFlipEvent(flipQueue, this->video, 0) == 0;
}

bool Scene2D::allocateFrameBuffers(int num)
{
    if (num != 2 || !this->videoMem || this->frameBufferSize == 0)
        return false;

    const size_t required = this->frameBufferSize * (size_t)num;
    if (required > this->directMemAllocationSize)
        return false;

    this->frameBuffers = new (std::nothrow) char*[num]();
    if (!this->frameBuffers)
        return false;

    for (int i = 0; i < num; ++i) {
        this->frameBuffers[i] = this->allocateDisplayMem(this->frameBufferSize);
        if (!this->frameBuffers[i]) {
            delete[] this->frameBuffers;
            this->frameBuffers = NULL;
            return false;
        }
    }

    sceVideoOutSetBufferAttribute(&this->attr, 0x80000000, 1, 0,
                                  this->width, this->height, this->width);

    if (sceVideoOutRegisterBuffers(this->video, 0, (void **)this->frameBuffers,
                                   num, &this->attr) != 0) {
        delete[] this->frameBuffers;
        this->frameBuffers = NULL;
        return false;
    }

    this->frameBufferCount = num;
    return true;
}

char *Scene2D::allocateDisplayMem(size_t size)
{
    if (!this->videoMem || size == 0)
        return NULL;

    const uintptr_t base = (uintptr_t)this->videoMem;
    const uintptr_t current = this->videoMemSP;
    const uintptr_t end = base + this->directMemAllocationSize;

    if (current < base || current > end || size > (size_t)(end - current))
        return NULL;

    char *allocatedPtr = (char *)current;
    this->videoMemSP += size;
    return allocatedPtr;
}

bool Scene2D::allocateVideoMem(size_t size, int alignment)
{
    if (size == 0 || alignment <= 0)
        return false;

    this->directMemAllocationSize =
        (size + (size_t)alignment - 1) / (size_t)alignment * (size_t)alignment;

    int rc = sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(),
                                            this->directMemAllocationSize,
                                            alignment, 3, &this->directMemOff);
    if (rc < 0) {
        this->directMemAllocationSize = 0;
        return false;
    }

    rc = sceKernelMapDirectMemory(&this->videoMem, this->directMemAllocationSize,
                                  0x33, 0, this->directMemOff, alignment);
    if (rc < 0) {
        sceKernelReleaseDirectMemory(this->directMemOff, this->directMemAllocationSize);
        this->directMemOff = 0;
        this->directMemAllocationSize = 0;
        this->videoMem = NULL;
        return false;
    }

    this->videoMemSP = (uintptr_t)this->videoMem;
    return true;
}

void Scene2D::deallocateVideoMem()
{
    if (this->directMemAllocationSize != 0) {
        sceKernelReleaseDirectMemory(this->directMemOff, this->directMemAllocationSize);
    }

    this->videoMem = NULL;
    this->videoMemSP = 0;
    this->directMemOff = 0;
    this->directMemAllocationSize = 0;

    delete[] this->frameBuffers;
    this->frameBuffers = NULL;
    this->frameBufferCount = 0;
}

void Scene2D::SetActiveFrameBuffer(int index)
{
    if (index >= 0 && index < this->frameBufferCount)
        this->activeFrameBufferIdx = index;
}

void Scene2D::SubmitFlip(int frameID)
{
    if (this->video >= 0 && this->frameBufferCount > 0)
        sceVideoOutSubmitFlip(this->video, this->activeFrameBufferIdx,
                              ORBIS_VIDEO_OUT_FLIP_VSYNC, frameID);
}

void Scene2D::FrameWait(int frameID)
{
    OrbisKernelEvent evt;
    int count;

    if (this->video < 0 || this->flipQueue == 0)
        return;

    for (;;) {
        OrbisVideoOutFlipStatus flipStatus;
        if (sceVideoOutGetFlipStatus(this->video, &flipStatus) != 0)
            break;

        if (flipStatus.flipArg == frameID)
            break;

        if (sceKernelWaitEqueue(this->flipQueue, &evt, 1, &count, 0) != 0)
            break;
    }
}

void Scene2D::FrameBufferSwap()
{
    if (this->frameBufferCount > 0)
        this->activeFrameBufferIdx = (this->activeFrameBufferIdx + 1) % this->frameBufferCount;
}

void Scene2D::FrameBufferClear()
{
    Color blank = { 255, 255, 255 };
    FrameBufferFill(blank);
}

#ifdef GRAPHICS_USES_FONT
bool Scene2D::InitFont(FT_Face *face, const char *fontPath, int fontSize)
{
    int rc = FT_New_Face(this->ftLib, fontPath, 0, face);
    if (rc < 0)
        return false;

    rc = FT_Set_Pixel_Sizes(*face, 0, fontSize);
    return rc >= 0;
}
#endif

void Scene2D::FrameBufferFill(Color color)
{
    DrawRectangle(0, 0, this->width, this->height, color);
}

void Scene2D::DrawPixel(int x, int y, Color color)
{
    if (!this->frameBuffers || this->activeFrameBufferIdx < 0 ||
        this->activeFrameBufferIdx >= this->frameBufferCount ||
        x < 0 || y < 0 || x >= this->width || y >= this->height)
        return;

    const size_t pixel = (size_t)y * (size_t)this->width + (size_t)x;
    const uint32_t encodedColor = 0x80000000u |
        ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | (uint32_t)color.b;

    ((uint32_t *)this->frameBuffers[this->activeFrameBufferIdx])[pixel] = encodedColor;
}

void Scene2D::DrawRectangle(int x, int y, int w, int h, Color color)
{
    if (w <= 0 || h <= 0)
        return;

    int64_t left = x;
    int64_t top = y;
    int64_t right = (int64_t)x + (int64_t)w;
    int64_t bottom = (int64_t)y + (int64_t)h;

    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > this->width) right = this->width;
    if (bottom > this->height) bottom = this->height;

    if (left >= right || top >= bottom)
        return;

    for (int64_t py = top; py < bottom; ++py)
        for (int64_t px = left; px < right; ++px)
            DrawPixel((int)px, (int)py, color);
}

#ifdef GRAPHICS_USES_FONT
void Scene2D::DrawText(char *txt, FT_Face face, int startX, int startY, Color bgColor, Color fgColor)
{
    int rc;
    int xOffset = 0;
    int yOffset = 0;
    FT_GlyphSlot slot = face->glyph;

    for (int n = 0; n < strlen(txt); n++) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, txt[n]);
        rc = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        if (rc)
            continue;

        rc = FT_Render_Glyph(slot, ft_render_mode_normal);
        if (rc)
            continue;

        if (txt[n] == '\n') {
            xOffset = 0;
            yOffset += slot->bitmap.width * 2;
            continue;
        }

        for (int yPos = 0; yPos < slot->bitmap.rows; yPos++) {
            for (int xPos = 0; xPos < slot->bitmap.width; xPos++) {
                char pixel = slot->bitmap.buffer[(yPos * slot->bitmap.width) + xPos];
                int px = startX + xPos + xOffset + slot->bitmap_left;
                int py = startY + yPos + yOffset - slot->bitmap_top;

                if (px < 0 || py < 0 || px >= this->width || py >= this->height)
                    continue;

                Color finalColor = {
                    (uint8_t)(pixel * fgColor.r / 255),
                    (uint8_t)(pixel * fgColor.g / 255),
                    (uint8_t)(pixel * fgColor.b / 255)
                };

                if (pixel != 0)
                    this->DrawPixel(px, py, finalColor);
            }
        }

        xOffset += slot->advance.x >> 6;
    }
}
#endif
