#include "gpu_renderer.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <orbis/libkernel.h>
#include <compat/freegnm.h>
#include <gnm/drawcommandbuffer.h>
#include <gnm/gnmdriver.h>
#include <gnm/platform.h>
#include <gnm/sampler.h>
#include <gnm/texture.h>

#include "displayctx.h"
#include "gltfmodel.h"
#include "memalloc.h"
#include "misc.h"
#include "pass_model.h"
#include "u/utility.h"

#define GPU_WIDTH 1920
#define GPU_HEIGHT 1080
#define GPU_OVERLAY_BYTES ((size_t)GPU_WIDTH * (size_t)GPU_HEIGHT * 4u)

typedef struct {
    GnmTexture texture;
    GnmSampler sampler;
} UiDescriptorSet;

typedef struct {
    MemoryAllocator memory;
    DisplayContext display;
    GnmRenderTarget frameTargets[2];
    GnmDepthRenderTarget depthTarget;
    GnmCommandBuffer commandBuffer;
    GnmPsShader *uiPixelShader;
    GnmTexture uiTexture;
    GnmSampler uiSampler;
    uint8_t *uiTextureMemory;
    uint64_t *gpuDoneLabel;
    gltfModel model;
    unsigned currentFrame;
    bool initialized;
} GpuRendererState;

static GpuRendererState s_gpu;

static void gpu_status(const char *message)
{
    FILE *f = fopen("/data/blackout_gpu.txt", "w");
    if (f) {
        fprintf(f, "%s\n", message);
        fclose(f);
    }
}

static bool init_ui_texture(void)
{
    s_gpu.uiTextureMemory = memalloc_alloc(
        &s_gpu.memory, GPU_OVERLAY_BYTES, GNM_ALIGNMENT_SHADER_BYTES
    );
    if (!s_gpu.uiTextureMemory) {
        gpu_status("GPU_UI_MEMORY_FAILED");
        return false;
    }
    memset(s_gpu.uiTextureMemory, 0, GPU_OVERLAY_BYTES);

    const GnmTextureCreateInfo info = {
        .texturetype = GNM_TEXTURE_2D,
        .width = GPU_WIDTH,
        .height = GPU_HEIGHT,
        .depth = 1,
        .pitch = GPU_WIDTH,
        .nummiplevels = 1,
        .numslices = 1,
        .format = GNM_FMT_R8G8B8A8_SRGB,
        .tilemodehint = GNM_TM_DISPLAY_LINEAR_GENERAL,
        .mingpumode = GNM_GPU_BASE,
        .numfragments = 1,
    };
    GnmError err = gnmCreateTexture(&s_gpu.uiTexture, &info);
    if (err != GNM_ERROR_OK) {
        gpu_status("GPU_UI_TEXTURE_CREATE_FAILED");
        return false;
    }
    gnmTexSetBaseAddress(&s_gpu.uiTexture, s_gpu.uiTextureMemory);

    s_gpu.uiSampler = (GnmSampler){
        .xyminfilter = GNM_FILTER_BILINEAR,
        .xymagfilter = GNM_FILTER_BILINEAR,
        .mipfilter = GNM_MIPFILTER_NONE,
        .zfilter = GNM_ZFILTER_POINT,
        .minlod = 0,
        .maxlod = 0,
    };
    return true;
}

bool GpuRenderer_Init(void)
{
    memset(&s_gpu, 0, sizeof(s_gpu));
    s_gpu.memory = memalloc_init(
        64u * 1024u * 1024u,
        ORBIS_KERNEL_PROT_CPU_READ | ORBIS_KERNEL_PROT_CPU_RW |
            ORBIS_KERNEL_PROT_GPU_READ | ORBIS_KERNEL_PROT_GPU_WRITE,
        ORBIS_KERNEL_WC_GARLIC
    );

    if (!displayctx_init(&s_gpu.display)) {
        gpu_status("GPU_VIDEOOUT_INIT_FAILED");
        return false;
    }

    for (unsigned i = 0; i < 2; ++i) {
        if (!initcolortarget(
                &s_gpu.frameTargets[i], &s_gpu.memory,
                s_gpu.display.screenw, s_gpu.display.screenh,
                GNM_FMT_R8G8B8A8_SRGB, gnmGpuMode())) {
            gpu_status("GPU_COLOR_TARGET_FAILED");
            return false;
        }
    }

    if (!initdepthtarget(
            &s_gpu.depthTarget, &s_gpu.memory,
            s_gpu.display.screenw, s_gpu.display.screenh, 0, 1, 1,
            GNM_Z_32_FLOAT, GNM_STENCIL_INVALID, gnmGpuMode())) {
        gpu_status("GPU_DEPTH_TARGET_FAILED");
        return false;
    }

    if (!displayctx_setrts(&s_gpu.display, s_gpu.frameTargets, 2)) {
        gpu_status("GPU_REGISTER_BUFFERS_FAILED");
        return false;
    }

    if (!initclearutility(&s_gpu.memory)) {
        gpu_status("GPU_CLEAR_SHADER_FAILED");
        return false;
    }

    pass_model_init(&s_gpu.memory, s_gpu.display.screenw, s_gpu.display.screenh);

    if (!LoadGltfModel(
            &s_gpu.model, "/app0/USRDIR/MODELS/NEXUS9_MENU.gltf",
            &s_gpu.memory, true, false)) {
        gpu_status("GPU_GLTF_MODEL_LOAD_FAILED");
        return false;
    }

    if (!loadpshader(
            &s_gpu.uiPixelShader, &s_gpu.memory,
            "/app0/USRDIR/SHADERS/nexus9_ui.frag.sb")) {
        gpu_status("GPU_UI_SHADER_LOAD_FAILED");
        return false;
    }

    if (!init_ui_texture()) return false;

    const size_t commandMemorySize = GNM_INDIRECT_BUFFER_MAX_BYTESIZE;
    void *commandMemory = memalloc_alloc(
        &s_gpu.memory, commandMemorySize, GNM_ALIGNMENT_BUFFER_BYTES
    );
    if (!commandMemory) {
        gpu_status("GPU_COMMAND_BUFFER_ALLOC_FAILED");
        return false;
    }
    s_gpu.commandBuffer = gnmCmdInit(
        commandMemory, commandMemorySize, NULL, NULL
    );

    s_gpu.gpuDoneLabel = memalloc_alloc(
        &s_gpu.memory, sizeof(uint64_t), sizeof(uint64_t)
    );
    if (!s_gpu.gpuDoneLabel) {
        gpu_status("GPU_SYNC_LABEL_ALLOC_FAILED");
        return false;
    }

    s_gpu.currentFrame = 0;
    s_gpu.initialized = true;
    gpu_status("BLACKOUT_GNM_GPU_RENDERER_READY");
    return true;
}

static void draw_menu_overlay(void)
{
    const GnmPrimitiveSetup primitive = {
        .cullmode = GNM_CULL_NONE,
        .frontface = GNM_FACE_CCW,
        .frontmode = GNM_FILL_SOLID,
        .backmode = GNM_FILL_SOLID,
        .provokemode = GNM_PROVOKINGVTX_FIRST,
    };
    const GnmBlendControl blend = {
        .blendenabled = true,
        .colorfunc = GNM_COMB_DST_PLUS_SRC,
        .colorsrcmult = GNM_BLEND_SRC_ALPHA,
        .colordstmult = GNM_BLEND_ONE_MINUS_SRC_ALPHA,
        .separatealphaenable = false,
    };
    const GnmDbRenderControl db = {0};
    const GnmDepthStencilControl depth = {0};

    gnmDrawCmdSetPrimitiveSetup(&s_gpu.commandBuffer, &primitive);
    gnmDrawCmdSetBlendControl(&s_gpu.commandBuffer, 0, &blend);
    gnmDrawCmdSetDbRenderControl(&s_gpu.commandBuffer, &db);
    gnmDrawCmdSetDepthStencilControl(&s_gpu.commandBuffer, &depth);

    gnmDrawCmdSetEmbeddedVsShader(
        &s_gpu.commandBuffer, GNM_EMBEDDED_VSH_FULLSCREEN, 0
    );
    gnmDrawCmdSetPsShader(
        &s_gpu.commandBuffer, &s_gpu.uiPixelShader->registers
    );

    UiDescriptorSet *set = gnmCmdAllocInside(
        &s_gpu.commandBuffer, sizeof(UiDescriptorSet), 4
    );
    if (!set) return;
    set->texture = s_gpu.uiTexture;
    set->sampler = s_gpu.uiSampler;
    gnmDrawCmdSetPointerUserData(
        &s_gpu.commandBuffer, GNM_STAGE_PS, 0, set
    );

    gnmDrawCmdSetRenderTarget(&s_gpu.commandBuffer, 0,
                              &s_gpu.frameTargets[s_gpu.currentFrame]);
    gnmDrawCmdSetRenderTargetMask(&s_gpu.commandBuffer, 0xf);
    setupviewport(&s_gpu.commandBuffer, 0, 0, GPU_WIDTH, GPU_HEIGHT, 0.5f, 0.5f);
    gnmDrawCmdSetPrimitiveType(&s_gpu.commandBuffer, GNM_PT_RECTLIST);
    gnmDrawCmdDrawIndexAuto(&s_gpu.commandBuffer, 3);
}

bool GpuRenderer_Render(const uint32_t *rgbaOverlay, int frameId)
{
    if (!s_gpu.initialized || !rgbaOverlay) return false;

    memcpy(s_gpu.uiTextureMemory, rgbaOverlay, GPU_OVERLAY_BYTES);
    sceGnmFlushGarlic();

    gnmCmdReset(&s_gpu.commandBuffer);
    *s_gpu.gpuDoneLabel = 9;

    gnmDrawCmdWaitUntilSafeForRendering(
        &s_gpu.commandBuffer, s_gpu.display.videohandle, s_gpu.currentFrame
    );

    const float clearColor[4] = {0.018f, 0.024f, 0.028f, 1.0f};
    clearcolortarget(
        &s_gpu.commandBuffer,
        &s_gpu.frameTargets[s_gpu.currentFrame], clearColor
    );
    cleardepthtarget(&s_gpu.commandBuffer, &s_gpu.depthTarget, 1.0f);

    pass_model_update();
    pass_model_render(
        &s_gpu.commandBuffer, &s_gpu.model,
        &s_gpu.frameTargets[s_gpu.currentFrame], &s_gpu.depthTarget
    );
    draw_menu_overlay();

    gnmDrawCmdEventWriteEop(
        &s_gpu.commandBuffer, GNM_CACHE_FLUSH_TS,
        (uint64_t)s_gpu.gpuDoneLabel, GNM_DATA_SEL_SEND_DATA64, 1
    );

    void *dcbAddress = s_gpu.commandBuffer.beginptr;
    uint32_t dcbSize = (uint32_t)(
        (s_gpu.commandBuffer.cmdptr - s_gpu.commandBuffer.beginptr) *
        sizeof(uint32_t)
    );
    int result = sceGnmSubmitCommandBuffers(
        1, &dcbAddress, &dcbSize, NULL, NULL
    );
    if (result != 0) {
        gpu_status("GPU_SUBMIT_FAILED");
        return false;
    }

    while (*s_gpu.gpuDoneLabel != 1) sceKernelUsleep(100);

    if (!displayctx_flip(&s_gpu.display, s_gpu.currentFrame)) {
        gpu_status("GPU_FLIP_FAILED");
        return false;
    }
    result = sceGnmSubmitDone();
    if (result != 0) {
        gpu_status("GPU_SUBMIT_DONE_FAILED");
        return false;
    }

    s_gpu.currentFrame = (s_gpu.currentFrame + 1) % 2;
    return true;
}
