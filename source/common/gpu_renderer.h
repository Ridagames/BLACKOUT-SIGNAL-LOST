#ifndef BLACKOUT_GPU_RENDERER_H
#define BLACKOUT_GPU_RENDERER_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
bool GpuRenderer_Init(void);
bool GpuRenderer_Render(const uint32_t *rgbaOverlay, int frameId);
#ifdef __cplusplus
}
#endif
#endif
