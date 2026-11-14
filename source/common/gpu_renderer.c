#include "gpu_renderer.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <orbis/libkernel.h>
#include <compat/freegnm.h>
#include <gnm/drawcommandbuffer.h>
#include <gnm/buffer.h>
#include <gnm/dataformat.h>
#include <gnm/depthrendertarget.h>
#include <gnm/gpuaddr/gpuaddr.h>
#include <gnm/rendertarget.h>
#include <gnmdriver.h>
#include <gnm/shader.h>
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
#define SHADOW_SIZE 1024
#define GPU_OVERLAY_BYTES ((size_t)GPU_WIDTH * (size_t)GPU_HEIGHT * 4u)

typedef struct {
    GnmTexture texture;
    GnmSampler sampler;
} UiDescriptorSet;

typedef struct {
    float lightViewProjection[16];
    float model[16];
} ShadowConstants;

typedef struct {
    GnmBuffer constbuf;
} ShadowVsDescriptorSet;

typedef struct {
    MemoryAllocator memory;
    DisplayContext display;
    GnmRenderTarget frameTargets[2];
    GnmDepthRenderTarget depthTarget;
    GnmRenderTarget shadowTarget;
    GnmDepthRenderTarget shadowDepthTarget;
    GnmTexture shadowTexture;
    GnmSampler shadowSampler;
    GnmVsShader *shadowVertexShader;
    GnmPsShader *shadowPixelShader;
    void *shadowFetchMemory;
    ShadowConstants *shadowConstants;
    GnmBuffer shadowConstBuffer;
    uint64_t *shadowBarrierLabel;
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
        .mipfilter = GNM_MIPFILTER_LINEAR,
        .zfilter = GNM_ZFILTER_POINT,
        .minlod = 0,
        .maxlod = 0,
    };
    return true;
}

static bool init_shadow_map(void)
{
    GpaSurfaceProperties surface = {0};
    GpaError gpaerr = gpaFindOptimalSurface(
        &surface, GPA_SURFACE_COLORDISPLAY, 32, 1, false, gnmGpuMode()
    );
    if (gpaerr != GPA_ERR_OK) {
        gpu_status("GPU_SHADOW_SURFACE_LAYOUT_FAILED");
        return false;
    }

    const GnmRenderTargetCreateInfo rtInfo = {
        .width = SHADOW_SIZE,
        .height = SHADOW_SIZE,
        .pitch = 0,
        .numslices = 1,
        .colorfmt = GNM_FMT_R32_FLOAT,
        .colortilemodehint = surface.tilemode,
        .mingpumode = gnmGpuMode(),
        .numsamples = 1,
        .numfragments = 1,
    };
    if (gnmCreateRenderTarget(&s_gpu.shadowTarget, &rtInfo) != GNM_ERROR_OK) {
        gpu_status("GPU_SHADOW_TARGET_CREATE_FAILED");
        return false;
    }

    uint64_t shadowBytes = 0;
    uint32_t shadowAlignment = 0;
    if (gnmRtCalcByteSize(&shadowBytes, &shadowAlignment, &s_gpu.shadowTarget) != GNM_ERROR_OK) {
        gpu_status("GPU_SHADOW_TARGET_SIZE_FAILED");
        return false;
    }
    void *shadowMemory = memalloc_alloc(&s_gpu.memory, shadowBytes, shadowAlignment);
    if (!shadowMemory) {
        gpu_status("GPU_SHADOW_TARGET_MEMORY_FAILED");
        return false;
    }
    gnmRtSetBaseAddr(&s_gpu.shadowTarget, shadowMemory);

    if (!initdepthtarget(
            &s_gpu.shadowDepthTarget, &s_gpu.memory,
            SHADOW_SIZE, SHADOW_SIZE, 0, 1, 1,
            GNM_Z_32_FLOAT, GNM_STENCIL_INVALID, gnmGpuMode())) {
        gpu_status("GPU_SHADOW_DEPTH_FAILED");
        return false;
    }

    const GnmTextureCreateInfo textureInfo = {
        .texturetype = GNM_TEXTURE_2D,
        .width = SHADOW_SIZE,
        .height = SHADOW_SIZE,
        .depth = 1,
        .pitch = gnmRtGetPitch(&s_gpu.shadowTarget),
        .nummiplevels = 1,
        .numslices = 1,
        .format = GNM_FMT_R32_FLOAT,
        .tilemodehint = surface.tilemode,
        .mingpumode = gnmGpuMode(),
        .numfragments = 1,
    };
    if (gnmCreateTexture(&s_gpu.shadowTexture, &textureInfo) != GNM_ERROR_OK) {
        gpu_status("GPU_SHADOW_TEXTURE_CREATE_FAILED");
        return false;
    }
    gnmTexSetBaseAddress(&s_gpu.shadowTexture, shadowMemory);
    s_gpu.shadowSampler = (GnmSampler){
        .xyminfilter = GNM_FILTER_BILINEAR,
        .xymagfilter = GNM_FILTER_BILINEAR,
        .mipfilter = GNM_MIPFILTER_LINEAR,
        .zfilter = GNM_ZFILTER_POINT,
        .minlod = 0,
        .maxlod = 0,
    };

    if (!loadvshader(
            &s_gpu.shadowVertexShader, &s_gpu.memory,
            "/app0/USRDIR/SHADERS/nexus9_shadow.vert.sb") ||
        !loadpshader(
            &s_gpu.shadowPixelShader, &s_gpu.memory,
            "/app0/USRDIR/SHADERS/nexus9_shadow.frag.sb")) {
        gpu_status("GPU_SHADOW_SHADERS_LOAD_FAILED");
        return false;
    }

    const GnmFetchShaderCreateInfo fetchInfo = {
        .regs = &s_gpu.shadowVertexShader->registers,
        .vtxinputs = gnmVsShaderInputSemanticTable(s_gpu.shadowVertexShader),
        .numvtxinputs = s_gpu.shadowVertexShader->numinputsemantics,
        .inputusages = gnmVsShaderInputUsageSlotTable(s_gpu.shadowVertexShader),
        .numinputusages = s_gpu.shadowVertexShader->common.numinputusageslots,
    };
    uint32_t fetchBytes = 0;
    if (gnmFetchShaderCalcSize(&fetchBytes, &fetchInfo) != GNM_ERROR_OK) {
        gpu_status("GPU_SHADOW_FETCH_SIZE_FAILED");
        return false;
    }
    s_gpu.shadowFetchMemory = memalloc_alloc(
        &s_gpu.memory, fetchBytes, GNM_ALIGNMENT_FETCHSHADER_BYTES
    );
    if (!s_gpu.shadowFetchMemory) {
        gpu_status("GPU_SHADOW_FETCH_MEMORY_FAILED");
        return false;
    }
    GnmFetchShaderResults fetchResults = {0};
    if (gnmCreateFetchShader(
            s_gpu.shadowFetchMemory, fetchBytes, &fetchInfo, &fetchResults
        ) != GNM_ERROR_OK) {
        gpu_status("GPU_SHADOW_FETCH_CREATE_FAILED");
        return false;
    }
    gnmVsRegsSetFetchShaderModifier(
        &s_gpu.shadowVertexShader->registers, &fetchResults
    );

    s_gpu.shadowConstants = memalloc_alloc(
        &s_gpu.memory, sizeof(ShadowConstants), GNM_ALIGNMENT_BUFFER_BYTES
    );
    if (!s_gpu.shadowConstants) {
        gpu_status("GPU_SHADOW_CONSTANTS_FAILED");
        return false;
    }
    memset(s_gpu.shadowConstants, 0, sizeof(ShadowConstants));
    pass_model_get_light_matrix(s_gpu.shadowConstants->lightViewProjection);
    s_gpu.shadowConstants->model[0] = 1.0f;
    s_gpu.shadowConstants->model[5] = 1.0f;
    s_gpu.shadowConstants->model[10] = 1.0f;
    s_gpu.shadowConstants->model[15] = 1.0f;
    s_gpu.shadowConstBuffer = gnmCreateConstBuffer(
        s_gpu.shadowConstants, sizeof(ShadowConstants)
    );

    s_gpu.shadowBarrierLabel = memalloc_alloc(
        &s_gpu.memory, sizeof(uint64_t), sizeof(uint64_t)
    );
    if (!s_gpu.shadowBarrierLabel) {
        gpu_status("GPU_SHADOW_SYNC_LABEL_FAILED");
        return false;
    }
    pass_model_set_shadow(s_gpu.shadowTexture, s_gpu.shadowSampler);
    return true;
}

static void draw_shadow_map(void)
{
    const float clearColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    clearcolortarget(&s_gpu.commandBuffer, &s_gpu.shadowTarget, clearColor);
    cleardepthtarget(&s_gpu.commandBuffer, &s_gpu.shadowDepthTarget, 1.0f);

    const GnmPrimitiveSetup primitive = {
        .cullmode = GNM_CULL_NONE,
        .frontface = GNM_FACE_CCW,
        .frontmode = GNM_FILL_SOLID,
        .backmode = GNM_FILL_SOLID,
        .provokemode = GNM_PROVOKINGVTX_FIRST,
    };
    const GnmBlendControl blend = {0};
    const GnmDbRenderControl db = {0};
    const GnmDepthStencilControl depth = {
        .zwrite = true,
        .zfunc = GNM_DEPTH_COMPARE_LESSEQUAL,
        .stencilfunc = GNM_DEPTH_COMPARE_NEVER,
        .stencilbackfunc = GNM_DEPTH_COMPARE_NEVER,
        .separatestencilenable = false,
        .depthenable = true,
        .stencilenable = false,
        .depthboundsenable = false,
    };

    gnmDrawCmdSetPrimitiveSetup(&s_gpu.commandBuffer, &primitive);
    gnmDrawCmdSetBlendControl(&s_gpu.commandBuffer, 0, &blend);
    gnmDrawCmdSetDbRenderControl(&s_gpu.commandBuffer, &db);
    gnmDrawCmdSetDepthStencilControl(&s_gpu.commandBuffer, &depth);
    gnmDrawCmdSetRenderTarget(&s_gpu.commandBuffer, 0, &s_gpu.shadowTarget);
    gnmDrawCmdSetRenderTargetMask(&s_gpu.commandBuffer, 0x1);
    gnmDrawCmdSetDepthRenderTarget(&s_gpu.commandBuffer, &s_gpu.shadowDepthTarget);
    setupviewport(&s_gpu.commandBuffer, 0, 0, SHADOW_SIZE, SHADOW_SIZE, 0.5f, 0.5f);
    gnmDrawCmdSetVsShader(&s_gpu.commandBuffer, &s_gpu.shadowVertexShader->registers, 0);
    gnmDrawCmdSetPsShader(&s_gpu.commandBuffer, &s_gpu.shadowPixelShader->registers);
    gnmDrawCmdSetPointerUserData(
        &s_gpu.commandBuffer, GNM_STAGE_VS, 0, s_gpu.shadowFetchMemory
    );
    gnmDrawCmdSetPrimitiveType(&s_gpu.commandBuffer, GNM_PT_TRILIST);
    gnmDrawCmdSetIndexSize(&s_gpu.commandBuffer, GNM_INDEX_32, GNM_POLICY_LRU);

    for (size_t i = 0; i < uvlen(&s_gpu.model.PrimMeshes); ++i) {
        R_Mesh *mesh = uvdata(&s_gpu.model.PrimMeshes, i);
        gnmDrawCmdSetPointerUserData(
            &s_gpu.commandBuffer, GNM_STAGE_VS, 2, mesh->vertbufs
        );
        ShadowVsDescriptorSet *descriptor = gnmCmdAllocInside(
            &s_gpu.commandBuffer, sizeof(ShadowVsDescriptorSet), 4
        );
        if (!descriptor) continue;
        descriptor->constbuf = s_gpu.shadowConstBuffer;
        gnmDrawCmdSetPointerUserData(
            &s_gpu.commandBuffer, GNM_STAGE_VS, 6, descriptor
        );
        gnmDrawCmdDrawIndex(
            &s_gpu.commandBuffer, mesh->numindices, mesh->indexmem
        );
    }

    gnmDrawCmdEventWriteEop(
        &s_gpu.commandBuffer, GNM_CACHE_FLUSH_AND_INV_TS_EVENT,
        (uint64_t)s_gpu.shadowBarrierLabel, GNM_DATA_SEL_SEND_DATA64, 1
    );
    gnmDrawCmdWaitMem(
        &s_gpu.commandBuffer, GNM_WAIT_REG_MEM_FUNC_EQUAL,
        (uint64_t)s_gpu.shadowBarrierLabel, 1, 0xffffffff
    );
    gnmDrawCmdWaitGraphicsWrite(
        &s_gpu.commandBuffer, GNM_ACQUIRE_TARGET_CB0 | GNM_ACQUIRE_TARGET_DB
    );
}

bool GpuRenderer_Init(void)
{
    memset(&s_gpu, 0, sizeof(s_gpu));
    s_gpu.memory = memalloc_init(
        96u * 1024u * 1024u,
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

    if (!init_shadow_map()) return false;

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
    // Garlic memory is write-combined, so CPU writes are visible to the GPU.

    gnmCmdReset(&s_gpu.commandBuffer);
    *s_gpu.gpuDoneLabel = 9;
    *s_gpu.shadowBarrierLabel = 9;

    gnmDrawCmdWaitUntilSafeForRendering(
        &s_gpu.commandBuffer, s_gpu.display.videohandle, s_gpu.currentFrame
    );

    draw_shadow_map();

    const float clearColor[4] = {0.018f, 0.024f, 0.028f, 1.0f};
    clearcolortarget(
        &s_gpu.commandBuffer,
        &s_gpu.frameTargets[s_gpu.currentFrame], clearColor
    );
    cleardepthtarget(&s_gpu.commandBuffer, &s_gpu.depthTarget, 1.0f);

    *s_gpu.shadowBarrierLabel = 9;
    gnmDrawCmdEventWriteEop(
        &s_gpu.commandBuffer, GNM_CACHE_FLUSH_AND_INV_TS_EVENT,
        (uint64_t)s_gpu.shadowBarrierLabel, GNM_DATA_SEL_SEND_DATA64, 1
    );
    gnmDrawCmdWaitMem(
        &s_gpu.commandBuffer, GNM_WAIT_REG_MEM_FUNC_EQUAL,
        (uint64_t)s_gpu.shadowBarrierLabel, 1, 0xffffffff
    );
    gnmDrawCmdWaitGraphicsWrite(
        &s_gpu.commandBuffer, GNM_ACQUIRE_TARGET_CB0 | GNM_ACQUIRE_TARGET_DB
    );

    pass_model_update();
    pass_model_render(
        &s_gpu.commandBuffer, &s_gpu.model,
        &s_gpu.frameTargets[s_gpu.currentFrame], &s_gpu.depthTarget
    );
    draw_menu_overlay();

    gnmDrawCmdEventWriteEop(
        &s_gpu.commandBuffer, GNM_CACHE_FLUSH_AND_INV_TS_EVENT,
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

    // Une fence GPU qui ne revient jamais ne doit pas figer le homebrew.
    // 20 000 x 100 us = environ 2 secondes maximum avant le mode de secours.
    int fenceWait = 0;
    while (*s_gpu.gpuDoneLabel != 1 && fenceWait < 20000) {
        sceKernelUsleep(100);
        ++fenceWait;
    }
    if (*s_gpu.gpuDoneLabel != 1) {
        gpu_status("GPU_FENCE_TIMEOUT");
        return false;
    }

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
