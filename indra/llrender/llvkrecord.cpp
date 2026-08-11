/**
* @file llvkrecord.cpp
* @brief AYAstorm r42 Vulkan loader — record-phase thread state / occlusion+timestamp queries / per-draw bind memo
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
* Copyright (C) 2025-2026 Ishikawa AYA (github: mayatonton, mayatonton1994@gmail.com)
*
* このファイルは Ishikawa AYA が新規に作成した独自著作物である。
* 著作権は Ishikawa AYA が保持し、パブリックドメインには置かない。
* All rights reserved by the author except as licensed below.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*
* 光の国のひとたちと共にわたしはここにいる　彩
*/
#include "linden_common.h"
#include "llvkloader.h"
#include "llvkcontract.h"

#include "volk.h"
#include "lldir.h"
#include "llcontrol.h"
#include "llwindow.h"
#include "llimagegl.h"
#include "llglslshader.h"
#include "llrendertarget.h"
#include "llthread.h"

#include <vector>
#include <string>
#include <climits>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <queue>
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <thread>
#include <pthread.h>
#if LL_LINUX
#include <cstdio>
#include <sys/resource.h>
#endif

extern LLControlGroup gSavedSettings;

extern S32 gGLViewport[4];

#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-variable"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "vk_mem_alloc.h"

extern bool gCubeSnapshot;
extern bool gHeroProbeMirrorRender;
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif
#include "llvkloaderinternal.h"

namespace LLVKLoader
{

namespace
{

    thread_local bool sScissorEnabled     = false;
    thread_local S32  sScissorRectGL[4]   = {0, 0, 0, 0};

    thread_local VkPipeline       sLastBoundGraphicsPipeline = VK_NULL_HANDLE;
    thread_local VkPipelineLayout sLastDescLayout   = VK_NULL_HANDLE;
    thread_local VkDescriptorSet  sLastDescSet0     = VK_NULL_HANDLE;
    thread_local VkDescriptorSet  sLastDescSet1     = VK_NULL_HANDLE;
    thread_local VkDescriptorSet  sLastDescSet2     = VK_NULL_HANDLE;
    thread_local VkDescriptorSet  sLastDescSet3     = VK_NULL_HANDLE;
    thread_local U32              sLastDescDynCount = 0;
    thread_local U32              sLastDescOffsets[LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS] = {};
    thread_local VkPipelineLayout sLastMvLayout     = VK_NULL_HANDLE;
    thread_local float            sLastMv[16]       = {};
    thread_local VkViewport       sLastViewport     = {};
    thread_local VkRect2D         sLastScissor      = {};
    thread_local bool             sViewportScissorValid = false;

    thread_local VkCommandBuffer  tMemoCmd = VK_NULL_HANDLE;
    thread_local U64              tMemoEpoch      = 0;

    thread_local VkCommandBuffer  tVBMemoCmd   = VK_NULL_HANDLE;
    thread_local U64              tVBMemoEpoch = 0;
    thread_local VkBuffer         tVBMemoBuf[16] = {};
    thread_local VkDeviceSize     tVBMemoOff[16] = {};

    thread_local VkCommandBuffer  tIBMemoCmd   = VK_NULL_HANDLE;
    thread_local U64              tIBMemoEpoch = 0;
    thread_local VkBuffer         tIBMemoBuf   = VK_NULL_HANDLE;
    thread_local VkDeviceSize     tIBMemoOff   = 0;
    thread_local VkIndexType      tIBMemoType  = VK_INDEX_TYPE_MAX_ENUM;
} // namespace

namespace LLVKLoaderInternal
{

    U32 vkcRaceSelf()
    {
        static std::atomic<U32> s_next{0};
        static thread_local U32 s_id = ++s_next;
        return s_id;
    }
} // namespace LLVKLoaderInternal

namespace
{

    bool vkCmdMemoEnabled()
    {
        static const bool s_enabled = []() -> bool {
            const char* e = getenv("AYASTORM_VKCMD_MEMO");
            return (e == nullptr) || (atof(e) != 0.0);
        }();
        return s_enabled;
    }
} // namespace

namespace LLVKLoaderInternal
{

    VkCommandPool threadCmdPool()
    {
        return t_cmdPool != VK_NULL_HANDLE ? t_cmdPool : sCommandPool;
    }
    void (*sGeoWorkerStopHook)()   = nullptr;
    void (*sBakeWorkerStopHook)()  = nullptr;

    U32 sLastCompletedMonotonic = 0;
    U32 sFrameSubmittedMonotonic[FRAMES_IN_FLIGHT] = { 0, 0, 0 };
    bool sReapForceAll = false;
    bool sProducersQuiesced = false;
    void (*sDeviceLostHook)() = nullptr;
    bool sDeviceLostSignaled = false;
    bool reapReady(U32 enqueue_frame)
    {
        return sReapForceAll || enqueue_frame <= sLastCompletedMonotonic;
    }

    VkCommandBuffer currentRecordCmd()
    {
        if (tRecordCmdOverride != VK_NULL_HANDLE)
        {
            return tRecordCmdOverride;
        }
        return sInFrame ? sCommandBuffers[sFrameIndex] : VK_NULL_HANDLE;
    }
} // namespace LLVKLoaderInternal

namespace
{


    U32 rwDesiredWorkerCount()
    {
        static const U32 s_count = []() -> U32 {
            const char* r = getenv("AYASTORM_MT_RECORD");
            if (r != nullptr && atoi(r) == 0)
            {
                return 0;
            }
            const char* e = getenv("AYASTORM_MT_THREADS");
            if (e != nullptr && atoi(e) <= 1)
            {
                return 0;
            }
            const U32 hw  = (U32)std::thread::hardware_concurrency();
            const U32 cap = (hw > 2) ? (hw - 2) : 1u;
            U32 want = 4u;
            if (e != nullptr)
            {
                const int v = atoi(e) - 1;
                want = (v > 0) ? (U32)v : 1u;
            }
            return llmin(llmin(want, 4u), cap);
        }();
        return s_count;
    }

    struct PendingQueryRelease
    {
        uint32_t index;
        U32      enqueue_frame;
    };
    std::vector<PendingQueryRelease> sPendingOcclusionQueryReleases;
} // namespace

extern thread_local float sCurrentModelviewMatrix[16];

U32 recordWorkerCount()
{
    return rwDesiredWorkerCount();
}

uint32_t acquireOcclusionQueryVk()
{
    if (sOcclusionQueryPool == VK_NULL_HANDLE)
    {
        return 0;
    }
    if (sOcclusionQueryFree.empty())
    {
        static U32 s_exhausted_count = 0;
        ++s_exhausted_count;
        if ((s_exhausted_count & (s_exhausted_count - 1)) == 0)
        {
            LL_WARNS("Vulkan") << "occlusion query pool exhausted (capacity="
                               << sOcclusionQueryCapacity
                               << " pending_release=" << (S32)sPendingOcclusionQueryReleases.size()
                               << " total_misses=" << s_exhausted_count << ")" << LL_ENDL;
        }
        return 0;
    }
    uint32_t index = sOcclusionQueryFree.front();
    sOcclusionQueryFree.pop();
    vkResetQueryPool(sDevice, sOcclusionQueryPool, index, 1);
    return index + 1;
}

void noteDeferredEnqueueThread()
{
    if (!on_main_thread())
    {
        LL_WARNS_ONCE("Vulkan") << "deferred resource free enqueued off main thread" << LL_ENDL;
        llassert(false);
    }
}

void releaseOcclusionQueryVk(uint32_t handle)
{
    noteDeferredEnqueueThread();
    if (handle == 0 || sOcclusionQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    uint32_t index = handle - 1;
    if (index < sOcclusionQueryCapacity)
    {
        PendingQueryRelease pending;
        pending.index         = index;
        pending.enqueue_frame = sMonotonicFrameCount;
        sPendingOcclusionQueryReleases.push_back(pending);
    }
}

void tickDeferredQueryReleaseQueue()
{
    size_t w = 0;
    const size_t n = sPendingOcclusionQueryReleases.size();
    for (size_t r = 0; r < n; ++r)
    {
        PendingQueryRelease& e = sPendingOcclusionQueryReleases[r];
        if (reapReady(e.enqueue_frame))
        {
            sOcclusionQueryFree.push(e.index);
        }
        else
        {
            if (w != r)
            {
                sPendingOcclusionQueryReleases[w] = e;
            }
            ++w;
        }
    }
    sPendingOcclusionQueryReleases.resize(w);
}

void cmdBeginOcclusionQueryVk(VkCommandBuffer cmd, uint32_t handle)
{
    if (handle == 0 || cmd == VK_NULL_HANDLE || sOcclusionQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    vkCmdBeginQuery(cmd, sOcclusionQueryPool, handle - 1, 0);
}

void cmdEndOcclusionQueryVk(VkCommandBuffer cmd, uint32_t handle)
{
    if (handle == 0 || cmd == VK_NULL_HANDLE || sOcclusionQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    vkCmdEndQuery(cmd, sOcclusionQueryPool, handle - 1);
}

bool getOcclusionQueryResultVk(uint32_t handle, bool& available, uint64_t& samples)
{
    available = false;
    samples   = 0;
    if (handle == 0 || sOcclusionQueryPool == VK_NULL_HANDLE)
    {
        return false;
    }
    uint64_t results[2] = { 0, 0 };
    VkResult r = vkGetQueryPoolResults(sDevice, sOcclusionQueryPool, handle - 1, 1,
                                       sizeof(results), results, sizeof(results),
                                       VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    if (r == VK_SUCCESS && results[1] != 0)
    {
        available = true;
        samples   = results[0];
    }
    return true;
}

bool isTimestampSupportedVk()
{
    return sTimestampSupported;
}

uint32_t acquireTimestampPairVk()
{
    if (sTimestampQueryPool == VK_NULL_HANDLE || !sTimestampSupported || sTimestampPairFree.empty())
    {
        return 0;
    }
    uint32_t index = sTimestampPairFree.front();
    sTimestampPairFree.pop();
    vkResetQueryPool(sDevice, sTimestampQueryPool, index * 2, 2);
    return index + 1;
}

void releaseTimestampPairVk(uint32_t handle)
{
    if (handle == 0 || sTimestampQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    uint32_t index = handle - 1;
    if (index < sTimestampPairCapacity)
    {
        sTimestampPairFree.push(index);
    }
}

void cmdWriteTimestampBeginVk(VkCommandBuffer cmd, uint32_t handle)
{
    if (handle == 0 || cmd == VK_NULL_HANDLE || sTimestampQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, sTimestampQueryPool, (handle - 1) * 2);
}

void cmdWriteTimestampEndVk(VkCommandBuffer cmd, uint32_t handle)
{
    if (handle == 0 || cmd == VK_NULL_HANDLE || sTimestampQueryPool == VK_NULL_HANDLE)
    {
        return;
    }
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, sTimestampQueryPool, (handle - 1) * 2 + 1);
}

bool getTimestampElapsedNsVk(uint32_t handle, bool& available, uint64_t& elapsed_ns)
{
    available  = false;
    elapsed_ns = 0;
    if (handle == 0 || sTimestampQueryPool == VK_NULL_HANDLE)
    {
        return false;
    }
    uint32_t index = handle - 1;
    uint64_t results[4] = { 0, 0, 0, 0 };
    VkResult r = vkGetQueryPoolResults(sDevice, sTimestampQueryPool, index * 2, 2,
                                       sizeof(results), results, sizeof(uint64_t) * 2,
                                       VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    if (r == VK_SUCCESS && results[1] != 0 && results[3] != 0)
    {
        uint64_t mask = (sTimestampValidBits >= 64) ? ~0ull : ((1ull << sTimestampValidBits) - 1);
        uint64_t diff = (results[2] - results[0]) & mask;
        elapsed_ns = (uint64_t)((double)diff * (double)sTimestampPeriodNs);
        available  = true;
    }
    return true;
}

thread_local float sCurrentModelviewMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

void pushCurrentModelviewMatrix(const float modelview_matrix[16])
{
    if (modelview_matrix != nullptr)
    {
        std::memcpy(sCurrentModelviewMatrix, modelview_matrix, sizeof(sCurrentModelviewMatrix));
    }
}

const float* getCurrentModelviewMatrix()
{
    return sCurrentModelviewMatrix;
}

void bindVertexBufferVk(VkCommandBuffer cmd_buf, VkBuffer buffer, VkDeviceSize offset, U32 firstBinding)
{
    if (cmd_buf == VK_NULL_HANDLE || buffer == VK_NULL_HANDLE)
    {
        return;
    }
    if (tVBMemoCmd != cmd_buf || tVBMemoEpoch != tCmdRecordEpoch)
    {
        tVBMemoCmd   = cmd_buf;
        tVBMemoEpoch = tCmdRecordEpoch;
        std::memset(tVBMemoBuf, 0, sizeof(tVBMemoBuf));
        std::memset(tVBMemoOff, 0, sizeof(tVBMemoOff));
    }
    if (firstBinding < 16
        && tVBMemoBuf[firstBinding] == buffer
        && tVBMemoOff[firstBinding] == offset)
    {
        ++gVkPerf.vb_skip;
        return;
    }
    if (firstBinding < 16)
    {
        tVBMemoBuf[firstBinding] = buffer;
        tVBMemoOff[firstBinding] = offset;
    }
    ++gVkPerf.vb_bind;
    VkBuffer     buffers[1] = { buffer };
    VkDeviceSize offsets[1] = { offset };
    vkCmdBindVertexBuffers(cmd_buf,
                           firstBinding,
                           1,
                           buffers,
                           offsets);
}

void bindIndexBufferVk(VkCommandBuffer cmd_buf,
                       VkBuffer        buffer,
                       VkDeviceSize    offset,
                       VkIndexType     index_type)
{
    if (cmd_buf == VK_NULL_HANDLE || buffer == VK_NULL_HANDLE)
    {
        return;
    }
    if (tIBMemoCmd != cmd_buf || tIBMemoEpoch != tCmdRecordEpoch)
    {
        tIBMemoCmd   = cmd_buf;
        tIBMemoEpoch = tCmdRecordEpoch;
        tIBMemoBuf   = VK_NULL_HANDLE;
        tIBMemoOff   = 0;
        tIBMemoType  = VK_INDEX_TYPE_MAX_ENUM;
    }
    if (tIBMemoBuf == buffer && tIBMemoOff == offset && tIBMemoType == index_type)
    {
        ++gVkPerf.ib_skip;
        return;
    }
    tIBMemoBuf  = buffer;
    tIBMemoOff  = offset;
    tIBMemoType = index_type;
    ++gVkPerf.ib_bind;
    vkCmdBindIndexBuffer(cmd_buf, buffer, offset, index_type);
}

bool beginShaderDrawOrSkip(LLGLSLShader* shader, U32 render_mode, VkCommandBuffer& out_cmd)
{
    out_cmd = VK_NULL_HANDLE;
    VkDescriptorSet set_to_bind = LLGLSLShader::vkResolvePerCallSetForDraw();
    if (set_to_bind == VK_NULL_HANDLE)
    {
        LLVKContract::drawSkipped(LLVKContract::C_UNKNOWN,
                                  shader != nullptr ? shader->mName : std::string("(no-shader)"));
        return false;
    }
    if (shader == nullptr)
    {
        LLVKContract::drawSkipped(LLVKContract::C_NO_SHADER_OR_LAYOUT, std::string("(no-shader)"));
        return false;
    }
    VkCommandBuffer cmd = getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE)
    {
        LLVKContract::drawSkipped(LLVKContract::C_CMD_NULL, shader->mName);
        return false;
    }
    VkPipeline pipeline = shader->getOrCreateVkPipelineForBoundRT(render_mode);
    if (pipeline == VK_NULL_HANDLE)
    {
        LLVKContract::drawSkipped(LLVKContract::C_PIPELINE_NULL, shader->mName);
        return false;
    }
    if (!isInRenderPassScope())
    {
        LLRenderTarget* bound_rt = LLRenderTarget::getCurrentBoundTarget();
        if (bound_rt == nullptr)
        {
            if (producerSwapchainFallbackShouldSkip())
            {
                return false;
            }
            beginSwapchainRendering();
            if (!isInRenderPassScope())
            {
                LLVKContract::drawSkipped(LLVKContract::C_PASS_SCOPE_FAIL, shader->mName);
                return false;
            }
        }
        else
        {
            static std::atomic<U32> s_rt_resume_count{0};
            const U32 resume_n = s_rt_resume_count.fetch_add(1, std::memory_order_relaxed) + 1;
            if ((resume_n & (resume_n - 1)) == 0)
            {
                LL_WARNS("Vulkan") << "draw with bound RT outside pass scope: resuming"
                                   << " shader='" << shader->mName
                                   << "' count=" << resume_n << LL_ENDL;
            }
            bound_rt->resumeVkDynamicRendering();
            if (!isInRenderPassScope())
            {
                LLVKContract::drawSkipped(LLVKContract::C_PASS_SCOPE_FAIL, shader->mName);
                return false;
            }
        }
    }
    bindGraphicsPipelineOnce(cmd, pipeline);
    {
        const bool vk_screen_space_copy = LLGLSLShader::vkUsePositiveViewport(
            LLRenderTarget::getCurrentBoundTarget() != nullptr,
            LLGLSLShader::vkCaptureRegimeActive());
        setupViewportAndScissor(cmd, vk_screen_space_copy);
    }
    bindDrawDescriptorSetsOnce(cmd,
                               shader->mVkPipelineLayout,
                               getCurrentPerFrameDescriptorSet(),
                               set_to_bind,
                               shader->mVkSet1DynamicCount,
                               LLGLSLShader::sCurPerCallVkDynamicOffsets);
    pushModelviewOnce(cmd,
                      shader->mVkPipelineLayout,
                      getCurrentModelviewMatrix());
    out_cmd = cmd;
    return true;
}

void setRenderViewport(S32 x, S32 y, S32 w, S32 h)
{
    sVkRenderViewport[0] = x;
    sVkRenderViewport[1] = y;
    sVkRenderViewport[2] = w;
    sVkRenderViewport[3] = h;
}

static void memoSyncCmd(VkCommandBuffer cmd)
{
    if (cmd == tMemoCmd && tCmdRecordEpoch == tMemoEpoch)
    {
        return;
    }
    tMemoCmd                   = cmd;
    tMemoEpoch                 = tCmdRecordEpoch;
    sLastBoundGraphicsPipeline = VK_NULL_HANDLE;
    sLastDescLayout            = VK_NULL_HANDLE;
    sLastMvLayout              = VK_NULL_HANDLE;
    sViewportScissorValid      = false;
}

void setupViewportAndScissor(VkCommandBuffer cmd, bool screen_space_copy)
{
    memoSyncCmd(cmd);
    const S32 fb_height = (S32)sCurrentRenderAreaHeight;

    VkViewport viewport = {};
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    if (screen_space_copy)
    {
        viewport.x      = (float)sVkRenderViewport[0];
        viewport.y      = (float)sVkRenderViewport[1];
        viewport.width  = (float)sVkRenderViewport[2];
        viewport.height = (float)sVkRenderViewport[3];
    }
    else
    {
        viewport.x      = (float)sVkRenderViewport[0];
        viewport.y      = (float)(fb_height - sVkRenderViewport[1]);
        viewport.width  = (float)sVkRenderViewport[2];
        viewport.height = -(float)sVkRenderViewport[3];
    }

    S32 sc_x, sc_y, sc_w, sc_h;
    if (sScissorEnabled)
    {
        sc_x = sScissorRectGL[0];
        sc_y = sScissorRectGL[1];
        sc_w = sScissorRectGL[2];
        sc_h = sScissorRectGL[3];
    }
    else
    {
        sc_x = sVkRenderViewport[0];
        sc_y = sVkRenderViewport[1];
        sc_w = sVkRenderViewport[2];
        sc_h = sVkRenderViewport[3];
    }

    VkRect2D scissor = {};
    scissor.offset.x      = llmax(sc_x, 0);
    scissor.offset.y      = llmax(screen_space_copy ? sc_y : fb_height - (sc_y + sc_h), 0);
    scissor.extent.width  = (U32)llmax(sc_w, 0);
    scissor.extent.height = (U32)llmax(sc_h, 0);

    if (vkCmdMemoEnabled()
        && sViewportScissorValid
        && std::memcmp(&viewport, &sLastViewport, sizeof(viewport)) == 0
        && std::memcmp(&scissor, &sLastScissor, sizeof(scissor)) == 0)
    {
        ++gVkPerf.vp_skip;
        return;
    }
    ++gVkPerf.vp_set;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    sLastViewport         = viewport;
    sLastScissor          = scissor;
    sViewportScissorValid = true;
}

void bindGraphicsPipelineOnce(VkCommandBuffer cmd, VkPipeline pipeline)
{
    memoSyncCmd(cmd);
    if (vkCmdMemoEnabled() && pipeline == sLastBoundGraphicsPipeline)
    {
        ++gVkPerf.pipe_skip;
        return;
    }
    ++gVkPerf.pipe_bind;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    sLastBoundGraphicsPipeline = pipeline;
}

void bindDrawDescriptorSetsOnce(VkCommandBuffer cmd, VkPipelineLayout layout,
                                VkDescriptorSet set0, VkDescriptorSet set1,
                                U32 dyn_count, const U32* offsets)
{
    memoSyncCmd(cmd);
    if (vkValidationRequested())
    {
        LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
        std::lock_guard<std::mutex> lk(sSet1BirthMutex);
        if (set1 != VK_NULL_HANDLE)
        {
            Set1BirthInfo& bi = sSet1BirthLedger[(U64)set1];
            bi.path   = gVkPerfSetPath;
            bi.gen    = gVkPerDrawTopologyGen.load(std::memory_order_relaxed);
            bi.frame  = sMonotonicFrameCount;
            bi.shader = (sh != nullptr) ? sh->mName : std::string();
        }
        if (set0 != VK_NULL_HANDLE)
        {
            Set1BirthInfo& bi = sSet1BirthLedger[(U64)set0];
            bi.path   = 99;
            bi.gen    = gVkPerDrawTopologyGen.load(std::memory_order_relaxed);
            bi.frame  = sMonotonicFrameCount;
            bi.shader = (sh != nullptr) ? sh->mName : std::string();
        }
    }
    const U32 pass_bucket = perfPassBucket();
    ++gVkPerf.draws_pass[pass_bucket];
    if (pass_bucket == 1u)
    {
        ++gVkPerf.draws_shadow_map[gVkPerfShadowMapIndex < 7u ? gVkPerfShadowMapIndex : 6u];
        const U32 sctx = gVkPerfShadowCtx < VKPERF_SHCTX_COUNT ? gVkPerfShadowCtx : 0u;
        const U32 ssec = gVkPerfShadowSection < VKPERF_SHSEC_COUNT ? gVkPerfShadowSection : 0u;
        ++gVkPerf.draws_shadow_site[sctx][ssec];
    }
    VkDescriptorSet set2 = VK_NULL_HANDLE;
    VkDescriptorSet set3 = VK_NULL_HANDLE;
    {
        LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
        if (sh != nullptr)
        {
            if (sh->mVkUsesHeapSet)
            {
                set2 = sBindlessHeapSet;
            }
            else if (sh->mVkUsesSkinSet)
            {
                set2 = sEmptySet;
            }
            if (sh->mVkUsesSkinSet)
            {
                set3 = sSkinBaseSet;
            }
        }
    }
    LLGLSLShader::vkVerifyPerCallBindingsAtBind(offsets, dyn_count);

    U32        eff_local[LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS];
    const U32* eff_offsets = offsets;
    U32        eff_count   = dyn_count;
    if (set3 != VK_NULL_HANDLE && dyn_count < LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS)
    {
        if (dyn_count > 0)
        {
            std::memcpy(eff_local, offsets, dyn_count * sizeof(U32));
        }
        eff_local[dyn_count] = skinBaseDynamicOffsetBytes();
        eff_offsets          = eff_local;
        eff_count            = dyn_count + 1;
    }

    if (vkCmdMemoEnabled()
        && layout == sLastDescLayout
        && set0 == sLastDescSet0
        && set1 == sLastDescSet1
        && set2 == sLastDescSet2
        && set3 == sLastDescSet3
        && eff_count == sLastDescDynCount
        && (eff_count == 0 || std::memcmp(eff_offsets, sLastDescOffsets, eff_count * sizeof(U32)) == 0))
    {
        ++gVkPerf.desc_skip;
        return;
    }
    ++gVkPerf.desc_bind;
    VkDescriptorSet sets[4] = { set0, set1, set2, set3 };
    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            layout,
                            0,
                            (set3 != VK_NULL_HANDLE) ? 4u : ((set2 != VK_NULL_HANDLE) ? 3u : 2u),
                            sets,
                            eff_count,
                            eff_count ? eff_offsets : nullptr);
    sLastDescLayout   = layout;
    sLastDescSet0     = set0;
    sLastDescSet1     = set1;
    sLastDescSet2     = set2;
    sLastDescSet3     = set3;
    sLastDescDynCount = eff_count;
    if (eff_count > 0)
    {
        std::memcpy(sLastDescOffsets, eff_offsets, eff_count * sizeof(U32));
    }
}

void pushModelviewOnce(VkCommandBuffer cmd, VkPipelineLayout layout, const float* mv16)
{
    memoSyncCmd(cmd);
    if (vkCmdMemoEnabled() && layout == sLastMvLayout && std::memcmp(mv16, sLastMv, sizeof(sLastMv)) == 0)
    {
        ++gVkPerf.mv_skip;
        return;
    }
    ++gVkPerf.mv_push;
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, mv16);
    sLastMvLayout = layout;
    std::memcpy(sLastMv, mv16, sizeof(sLastMv));
}

bool perFrameMatrixNeedsWrite()
{
    if (!sInitialized || sFrameIndex >= FRAMES_IN_FLIGHT)
    {
        return false;
    }
    return !sMatrixRingHasCurrent[sFrameIndex];
}

void setScissor(S32 x, S32 y, S32 w, S32 h)
{
    sScissorRectGL[0] = x;
    sScissorRectGL[1] = y;
    sScissorRectGL[2] = w;
    sScissorRectGL[3] = h;
    sScissorEnabled   = true;
}

void disableScissor()
{
    sScissorEnabled = false;
}

} // namespace LLVKLoader
