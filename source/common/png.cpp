#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "graphics.h"
#include "png.h"

PNG::PNG(const char *imagePath)
{
    width = 0;
    height = 0;
    channels = 0;
    img = (uint32_t *)stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
}

PNG::~PNG()
{
    if (img != NULL)
        stbi_image_free(img);
}

bool PNG::IsLoaded() const
{
    return img != NULL && width > 0 && height > 0;
}

void PNG::Draw(Scene2D *scene, int startX, int startY)
{
    if (!IsLoaded())
        return;
    DrawScaled(scene, startX, startY, width, height);
}

void PNG::DrawScaled(Scene2D *scene, int startX, int startY, int drawWidth, int drawHeight)
{
    if (!IsLoaded() || scene == NULL || drawWidth <= 0 || drawHeight <= 0)
        return;

    // Mise a l'echelle plein ecran, sans recadrage de l'image.
    for (int y = 0; y < drawHeight; ++y) {
        const int sourceY = (int)(((int64_t)y * height) / drawHeight);
        for (int x = 0; x < drawWidth; ++x) {
            const int sourceX = (int)(((int64_t)x * width) / drawWidth);
            const uint32_t pixel = img[(size_t)sourceY * (size_t)width + (size_t)sourceX];
            Color color = {
                (uint8_t)(pixel & 0xFF),
                (uint8_t)((pixel >> 8) & 0xFF),
                (uint8_t)((pixel >> 16) & 0xFF)
            };
            scene->DrawPixel(startX + x, startY + y, color);
        }
    }
}
