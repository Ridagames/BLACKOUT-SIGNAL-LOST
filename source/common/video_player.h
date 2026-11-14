#ifndef BLACKOUT_VIDEO_PLAYER_H
#define BLACKOUT_VIDEO_PLAYER_H
#include <cstdint>
#include <vector>
#include "orbis_avplayer.h"

class VideoPlayer {
public:
    VideoPlayer() = default;
    ~VideoPlayer();
    static bool globalInit();
    bool open(const char *path);
    bool update();
    void stop();
    const uint32_t *pixels() const { return m_pixels.empty() ? nullptr : m_pixels.data(); }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool hasFrame() const { return m_hasFrame; }
private:
    void convertNV12(const uint8_t *yPlane, const uint8_t *uvPlane, int width, int height, int pitch);
    SceAvPlayerHandle m_handle = nullptr;
    std::vector<uint32_t> m_pixels;
    int m_width = 0;
    int m_height = 0;
    bool m_hasFrame = false;
};
#endif
