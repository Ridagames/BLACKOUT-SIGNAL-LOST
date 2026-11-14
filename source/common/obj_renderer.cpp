#include "obj_renderer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

static Color materialColor(const char *name)
{
    // Palette industrielle NEXUS-9 : acier froid, revetement graphite,
    // verre de securite et contamination organique.
    if (std::strcmp(name, "floor") == 0) return {31, 37, 39};
    if (std::strcmp(name, "wall") == 0) return {67, 76, 78};
    if (std::strcmp(name, "ceiling") == 0) return {35, 43, 46};
    if (std::strcmp(name, "metal") == 0) return {112, 125, 129};
    if (std::strcmp(name, "darkmetal") == 0) return {18, 25, 29};
    if (std::strcmp(name, "warning") == 0) return {170, 52, 31};
    if (std::strcmp(name, "light") == 0) return {255, 104, 61};
    if (std::strcmp(name, "glass") == 0) return {22, 52, 62};
    if (std::strcmp(name, "bio") == 0) return {76, 103, 57};
    return {75, 82, 82};
}

static float edge(float ax, float ay, float bx, float by, float px, float py)
{
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

ObjRenderer::ObjRenderer()
    : m_width(1920), m_height(1080), m_loaded(false)
{
    m_error[0] = 0;
    m_pixels.resize((size_t)m_width * m_height);
    m_depth.resize((size_t)m_width * m_height);
}

bool ObjRenderer::Load(const char *path)
{
    m_loaded = false;
    m_vertices.clear();
    m_triangles.clear();
    if (!path) { std::strcpy(m_error, "NULL MODEL PATH"); return false; }
    FILE *file = std::fopen(path, "r");
    if (!file) { std::strcpy(m_error, "OBJ FILE NOT FOUND"); return false; }

    char line[512];
    char material[64] = "wall";
    while (std::fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            Vertex v;
            if (std::sscanf(line + 2, "%f %f %f", &v.x, &v.y, &v.z) == 3)
                m_vertices.push_back(v);
        } else if (std::strncmp(line, "usemtl ", 7) == 0) {
            std::sscanf(line + 7, "%63s", material);
        } else if (line[0] == 'f' && line[1] == ' ') {
            int indices[32];
            int count = 0;
            char *p = line + 2;
            while (*p && count < 32) {
                while (*p == ' ' || *p == '\t') ++p;
                if (!*p || *p == '\n' || *p == '#') break;
                char *end = nullptr;
                long idx = std::strtol(p, &end, 10);
                if (end == p) break;
                if (idx < 0) idx = (long)m_vertices.size() + idx + 1;
                if (idx > 0 && idx <= (long)m_vertices.size())
                    indices[count++] = (int)idx - 1;
                while (*end && *end != ' ' && *end != '\t' && *end != '\n') ++end;
                p = end;
            }
            for (int i = 1; i + 1 < count; ++i) {
                Triangle t = {indices[0], indices[i], indices[i + 1], materialColor(material)};
                m_triangles.push_back(t);
            }
        }
    }
    std::fclose(file);
    if (m_vertices.empty() || m_triangles.empty()) {
        std::strcpy(m_error, "OBJ HAS NO TRIANGLES");
        return false;
    }
    m_loaded = true;
    std::snprintf(m_error, sizeof(m_error), "OBJ OK: %u TRIANGLES",
                  (unsigned)m_triangles.size());
    return true;
}

bool ObjRenderer::IsLoaded() const { return m_loaded; }

const uint32_t *ObjRenderer::Render(float timeSeconds)
{
    const size_t count = (size_t)m_width * m_height;
    const float pulse = 0.82f + 0.18f * std::sin(timeSeconds * 2.1f);
    for (size_t i = 0; i < count; ++i) {
        m_pixels[i] = 0xFF070B0Du;
        m_depth[i] = 100000.0f;
    }
    if (!m_loaded) return m_pixels.data();

    const float cameraX = 0.10f * std::sin(timeSeconds * 0.23f);
    const float cameraY = 0.04f * std::sin(timeSeconds * 0.31f);
    const float yaw = 0.025f * std::sin(timeSeconds * 0.19f);
    const float cy = std::cos(yaw), sy = std::sin(yaw);
    const float focal = 855.0f; // Projection adaptee au rendu natif 1920x1080.
    const float lightX = 0.25f, lightY = 0.82f, lightZ = -0.50f;

    struct Projected { float x, y, z; };
    for (size_t ti = 0; ti < m_triangles.size(); ++ti) {
        const Triangle &t = m_triangles[ti];
        const Vertex &a = m_vertices[t.a], &b = m_vertices[t.b], &c = m_vertices[t.c];
        const Vertex *src[3] = {&a, &b, &c};
        Projected p[3];
        bool visible = true;
        for (int k = 0; k < 3; ++k) {
            const float wx = src[k]->x - cameraX;
            const float wy = src[k]->y - cameraY;
            const float wz = src[k]->z + 5.0f;
            const float vx = wx * cy - wz * sy;
            const float vz = wx * sy + wz * cy;
            if (vz < 0.35f) { visible = false; break; }
            p[k].x = m_width * 0.5f + vx * focal / vz;
            p[k].y = m_height * 0.51f - wy * focal / vz;
            p[k].z = vz;
        }
        if (!visible) continue;

        const float ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
        const float vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
        float nx = uy * vz - uz * vy;
        float ny = uz * vx - ux * vz;
        float nz = ux * vy - uy * vx;
        float nl = std::sqrt(nx*nx + ny*ny + nz*nz);
        if (nl < 0.0001f) continue;
        nx /= nl; ny /= nl; nz /= nl;
        const float diffuse = std::fabs(nx*lightX + ny*lightY + nz*lightZ);
        const float cx = (a.x + b.x + c.x) / 3.0f;
        const float cy = (a.y + b.y + c.y) / 3.0f;
        const float cz = (a.z + b.z + c.z) / 3.0f;
        // Eclairage local de plusieurs plafonniers et voyants de securite.
        const float l1x = cx - 0.0f, l1y = cy - 2.75f, l1z = cz - 5.0f;
        const float l2x = cx - 0.0f, l2y = cy - 2.70f, l2z = cz - 20.0f;
        const float d1 = std::sqrt(l1x*l1x + l1y*l1y + l1z*l1z);
        const float d2 = std::sqrt(l2x*l2x + l2y*l2y + l2z*l2z);
        const float local1 = 0.34f / (1.0f + d1*d1*0.055f);
        const float local2 = 0.26f / (1.0f + d2*d2*0.045f);
        const float shade = std::min(1.18f, (0.22f + 0.43f*diffuse + local1 + local2) * pulse);
        int minX = (int)std::floor(std::min(p[0].x, std::min(p[1].x,p[2].x)));
        int maxX = (int)std::ceil (std::max(p[0].x, std::max(p[1].x,p[2].x)));
        int minY = (int)std::floor(std::min(p[0].y, std::min(p[1].y,p[2].y)));
        int maxY = (int)std::ceil (std::max(p[0].y, std::max(p[1].y,p[2].y)));
        minX = std::max(0, minX); minY = std::max(0, minY);
        maxX = std::min(m_width-1, maxX); maxY = std::min(m_height-1, maxY);
        const float area = edge(p[0].x,p[0].y,p[1].x,p[1].y,p[2].x,p[2].y);
        if (std::fabs(area) < 0.001f) continue;
        int cr = (int)(t.color.r * shade);
        int cg = (int)(t.color.g * shade);
        int cb = (int)(t.color.b * shade);
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                const float px = x + 0.5f, py = y + 0.5f;
                float w0 = edge(p[1].x,p[1].y,p[2].x,p[2].y,px,py) / area;
                float w1 = edge(p[2].x,p[2].y,p[0].x,p[0].y,px,py) / area;
                float w2 = 1.0f - w0 - w1;
                if (w0 < 0 || w1 < 0 || w2 < 0) continue;
                const float invZ = w0/p[0].z + w1/p[1].z + w2/p[2].z;
                if (invZ <= 0) continue;
                const float z = 1.0f / invZ;
                const size_t at = (size_t)y * m_width + x;
                if (z >= m_depth[at]) continue;
                m_depth[at] = z;
                const float fog = std::min(0.82f, std::max(0.0f, (z - 12.0f) / 28.0f));
                const int r = (int)(cr * (1.0f-fog) + 7.0f*fog);
                const int g = (int)(cg * (1.0f-fog) + 11.0f*fog);
                const int bl = (int)(cb * (1.0f-fog) + 14.0f*fog);
                m_pixels[at] = 0xFF000000u | ((uint32_t)std::min(255,std::max(0,r)) << 16) |
                    ((uint32_t)std::min(255,std::max(0,g)) << 8) |
                    (uint32_t)std::min(255,std::max(0,bl));
            }
        }
    }
    return m_pixels.data();
}
