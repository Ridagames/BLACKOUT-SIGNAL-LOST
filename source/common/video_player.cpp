#include "video_player.h"
#include <cstdlib>
#include <cstring>
#include <orbis/Sysmodule.h>

extern "C" {
static void *blackoutAvAllocate(void *, uint32_t alignment, uint32_t size)
{
    void *ptr = nullptr;
    if (alignment < sizeof(void *)) alignment = sizeof(void *);
    size_t rounded = ((size_t)size + alignment - 1) & ~((size_t)alignment - 1);
    if (posix_memalign(&ptr, alignment, rounded) != 0) return nullptr;
    return ptr;
}
static void blackoutAvFree(void *, void *ptr) { free(ptr); }
static void *blackoutAvTextureAllocate(void *arg, uint32_t alignment, uint32_t size)
{ return blackoutAvAllocate(arg, alignment, size); }
static void blackoutAvTextureFree(void *arg, void *ptr) { blackoutAvFree(arg, ptr); }
}

bool VideoPlayer::globalInit()
{
    int rc = sceSysmoduleLoadModule(ORBIS_SYSMODULE_AV_PLAYER);
    return rc >= 0 || rc == 0x80960003;
}

VideoPlayer::~VideoPlayer() { stop(); }

bool VideoPlayer::open(const char *path)
{
    stop();
    if (!path || !*path) return false;

    SceAvPlayerInitData init;
    std::memset(&init, 0, sizeof(init));
    init.memoryReplacement.allocate = blackoutAvAllocate;
    init.memoryReplacement.deallocate = blackoutAvFree;
    init.memoryReplacement.allocateTexture = blackoutAvTextureAllocate;
    init.memoryReplacement.deallocateTexture = blackoutAvTextureFree;
    init.debugLevel = SCE_AVPLAYER_DBG_NONE;
    init.basePriority = 160;
    init.numOutputVideoFrameBuffers = 4;
    init.autoStart = 1;
    init.defaultLanguage = "en";

    m_handle = sceAvPlayerInit(&init);
    if (!m_handle) return false;
    if (sceAvPlayerAddSource(m_handle, path) < 0) {
        sceAvPlayerClose(m_handle);
        m_handle = nullptr;
        return false;
    }
    // The decoder repeats the local MP4 without returning to a blank frame.
    sceAvPlayerSetLooping(m_handle, 1);
    return true;
}

bool VideoPlayer::update()
{
    if (!m_handle || !sceAvPlayerIsActive(m_handle)) return false;

    SceAvPlayerFrameInfoEx frame;
    std::memset(&frame, 0, sizeof(frame));
    if (sceAvPlayerGetVideoDataEx(m_handle, &frame) != 0 || !frame.pData)
        return false;

    const int w = (int)frame.details.video.width;
    const int h = (int)frame.details.video.height;
    int pitch = (int)frame.details.video.pitch;
    if (pitch <= 0) pitch = w;
    if (w <= 0 || h <= 0 || pitch < w) return false;

    const uint8_t *yPlane = (const uint8_t *)frame.pData;
    const uint8_t *uvPlane = yPlane + (size_t)pitch * h;
    convertNV12(yPlane, uvPlane, w, h, pitch);
    m_hasFrame = true;
    return true;
}

void VideoPlayer::convertNV12(const uint8_t *yPlane, const uint8_t *uvPlane,
                              int w, int h, int pitch)
{
    if (w != m_width || h != m_height) {
        m_width = w;
        m_height = h;
        m_pixels.resize((size_t)w * h);
    }

    for (int y = 0; y < h; ++y) {
        const uint8_t *yr = yPlane + (size_t)y * pitch;
        const uint8_t *uvr = uvPlane + (size_t)(y >> 1) * pitch;
        uint32_t *dst = m_pixels.data() + (size_t)y * w;
        for (int x = 0; x < w; ++x) {
            const int yy = (int)yr[x] - 16;
            const int u = (int)uvr[x & ~1] - 128;
            const int v = (int)uvr[(x & ~1) + 1] - 128;
            const int c = yy < 0 ? 0 : yy;
            int r = (298*c + 409*v + 128) >> 8;
            int g = (298*c - 100*u - 208*v + 128) >> 8;
            int b = (298*c + 516*u + 128) >> 8;
            if (r < 0) r = 0; else if (r > 255) r = 255;
            if (g < 0) g = 0; else if (g > 255) g = 255;
            if (b < 0) b = 0; else if (b > 255) b = 255;
            dst[x] = 0xFF000000u | ((uint32_t)r << 16) |
                     ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}

void VideoPlayer::stop()
{
    if (m_handle) {
        sceAvPlayerStop(m_handle);
        sceAvPlayerClose(m_handle);
        m_handle = nullptr;
    }
    m_pixels.clear();
    m_width = m_height = 0;
    m_hasFrame = false;
}
