#include "graphics.h"

#ifndef PNG_H
#define PNG_H

class PNG {
    int width;
    int height;
    int channels;
    uint32_t *img;

public:
    PNG(const char *imagePath);
    ~PNG();

    bool IsLoaded() const;
    void Draw(Scene2D *scene, int startX, int startY);
    void DrawScaled(Scene2D *scene, int startX, int startY, int drawWidth, int drawHeight);
};

#endif
