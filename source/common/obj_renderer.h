#ifndef BLACKOUT_OBJ_RENDERER_H
#define BLACKOUT_OBJ_RENDERER_H
#include <cstdint>
#include <vector>
#include "graphics.h"

class ObjRenderer {
public:
    ObjRenderer();
    bool Load(const char *path);
    bool IsLoaded() const;
    const uint32_t *Render(float timeSeconds);
    int Width() const { return m_width; }
    int Height() const { return m_height; }
    const char *Error() const { return m_error; }
private:
    struct Vertex { float x, y, z; };
    struct Triangle { int a, b, c; Color color; };
    int m_width, m_height;
    bool m_loaded;
    char m_error[96];
    std::vector<Vertex> m_vertices;
    std::vector<Triangle> m_triangles;
    std::vector<uint32_t> m_pixels;
    std::vector<float> m_depth;
};
#endif
