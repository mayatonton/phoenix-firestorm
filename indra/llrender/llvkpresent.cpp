/**
* @file llvkpresent.cpp
* @brief AYAstorm r42 Vulkan loader — PE thread (single submitter) / timeline semaphore
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
#include <limits>
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
    std::atomic<uint64_t> sTimelineNext{0};                       // 次に採番する値(enqueue 時 ++ )
    std::mutex              sPESlotMutex;
    std::condition_variable sPESlotCv;

    std::mutex              sPEQueueMutex;
    std::condition_variable sPEQueueCv;
    std::deque<PEJob>       sPEJobs;
    bool                    sPEBusy          = false;
    bool                    sPEStopRequested = false;
    std::thread             sPEThread;
    std::mutex              sPEAbandonedMutex;
    std::vector<uint64_t>   sPEAbandonedTimelineValues;   // 非 device-lost で submit 失敗した frame/producer/async の値(永久未 signal・waitTimeline 脱出用)
    void                    recordAbandonedTimelineValue(uint64_t v);   // 定義は gpuTimelineValue 後
} // namespace

namespace LLVKLoaderInternal
{

    U64 vkMonoUs()
    {
        return (U64)std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
} // namespace LLVKLoaderInternal

namespace
{
    std::atomic<U64>        sPresentIdCounter{0};

    constexpr size_t DISPLAY_TIMING_MAPPING_LIMIT = 4096;

    struct DisplayTimingPresentMapping
    {
        U64 scene_id = 0;
    };

    std::mutex sDisplayTimingMutex;
    std::unordered_map<U32, DisplayTimingPresentMapping> sDisplayTimingMappings;
    std::unordered_set<U32> sDisplayTimingCompletedIds;
    U64 sDisplayTimingLastActualTime = 0;
    U64 sDisplayTimingLastFreshTime = 0;
    U64 sDisplayTimingLastSceneId = 0;
    bool sDisplayTimingLastSceneValid = false;
    bool sDisplayTimingIdExhausted = false;
    LLVKLoaderInternal::DisplayTimingIntervalStats sDisplayTimingIntervalStats;

    U32 nextDisplayTimingPresentId()
    {
        // `0` is reserved by VK_GOOGLE_display_timing. Do not recycle a
        // 32-bit wire ID: on wrap, stop tagging rather than risk mapping a
        // delayed completion to a different scene.
        if (sDisplayTimingIdExhausted)
        {
            return 0;
        }
        const U64 serial = sPresentIdCounter.fetch_add(1, std::memory_order_relaxed) + 1;
        if (serial > (U64)std::numeric_limits<U32>::max())
        {
            sDisplayTimingIdExhausted = true;
            return 0;
        }
        return (U32)serial;
    }

    void rememberDisplayTimingPresent(U32 present_id, U64 scene_id)
    {
        std::lock_guard<std::mutex> guard(sDisplayTimingMutex);
        if (sDisplayTimingMappings.size() >= DISPLAY_TIMING_MAPPING_LIMIT ||
            sDisplayTimingMappings.find(present_id) != sDisplayTimingMappings.end())
        {
            ++sDisplayTimingIntervalStats.mapping_dropped;
            return;
        }
        sDisplayTimingMappings.emplace(present_id, DisplayTimingPresentMapping{scene_id});
    }

    void forgetDisplayTimingPresent(U32 present_id)
    {
        std::lock_guard<std::mutex> guard(sDisplayTimingMutex);
        sDisplayTimingMappings.erase(present_id);
    }

    void noteDisplayTimingQueryError(VkResult result)
    {
        std::lock_guard<std::mutex> guard(sDisplayTimingMutex);
        ++sDisplayTimingIntervalStats.history_query_errors;
        sDisplayTimingIntervalStats.last_history_query_result = (S32)result;
    }

    void pollDisplayTimingLocked(VkSwapchainKHR swapchain)
    {
        if (!sDisplayTimingEnabled || vkGetPastPresentationTimingGOOGLE == nullptr ||
            swapchain == VK_NULL_HANDLE)
        {
            return;
        }
        U32 count = 0;
        VkResult result = vkGetPastPresentationTimingGOOGLE(sDevice, swapchain, &count, nullptr);
        if (result != VK_SUCCESS)
        {
            noteDisplayTimingQueryError(result);
            return;
        }
        if (count == 0)
        {
            return;
        }
        std::vector<VkPastPresentationTimingGOOGLE> timings(count);
        result = vkGetPastPresentationTimingGOOGLE(sDevice, swapchain, &count, timings.data());
        if (result != VK_SUCCESS)
        {
            noteDisplayTimingQueryError(result);
            return;
        }
        std::lock_guard<std::mutex> guard(sDisplayTimingMutex);
        for (U32 i = 0; i < count; ++i)
        {
            const VkPastPresentationTimingGOOGLE& timing = timings[i];
            if (timing.presentID == 0 || timing.actualPresentTime == 0 ||
                !sDisplayTimingCompletedIds.insert(timing.presentID).second)
            {
                continue;
            }
            if (sDisplayTimingCompletedIds.size() > DISPLAY_TIMING_MAPPING_LIMIT)
            {
                // This set only suppresses duplicate history delivery. If an
                // old entry is seen again after reset it has no mapping and is
                // therefore counted as unknown, never as fresh/duplicate.
                sDisplayTimingCompletedIds.clear();
            }

            ++sDisplayTimingIntervalStats.actual_count;
            sDisplayTimingIntervalStats.present_margin_ns.push_back(timing.presentMargin);
            if (sDisplayTimingLastActualTime != 0 && timing.actualPresentTime > sDisplayTimingLastActualTime)
            {
                ++sDisplayTimingIntervalStats.actual_interval_count;
                sDisplayTimingIntervalStats.actual_interval_ns += timing.actualPresentTime - sDisplayTimingLastActualTime;
            }
            if (timing.actualPresentTime > sDisplayTimingLastActualTime)
            {
                sDisplayTimingLastActualTime = timing.actualPresentTime;
            }

            const auto it = sDisplayTimingMappings.find(timing.presentID);
            if (it == sDisplayTimingMappings.end())
            {
                ++sDisplayTimingIntervalStats.unknown_scene_count;
                sDisplayTimingLastSceneValid = false;
                continue;
            }
            const U64 scene_id = it->second.scene_id;
            sDisplayTimingMappings.erase(it);
            if (!sDisplayTimingLastSceneValid)
            {
                ++sDisplayTimingIntervalStats.unknown_scene_count;
                sDisplayTimingLastSceneId = scene_id;
                sDisplayTimingLastSceneValid = true;
                continue;
            }
            if (scene_id == sDisplayTimingLastSceneId)
            {
                ++sDisplayTimingIntervalStats.duplicate_count;
                continue;
            }
            ++sDisplayTimingIntervalStats.fresh_count;
            if (sDisplayTimingLastFreshTime != 0 && timing.actualPresentTime > sDisplayTimingLastFreshTime)
            {
                ++sDisplayTimingIntervalStats.fresh_interval_count;
                sDisplayTimingIntervalStats.fresh_interval_ns += timing.actualPresentTime - sDisplayTimingLastFreshTime;
            }
            if (timing.actualPresentTime > sDisplayTimingLastFreshTime)
            {
                sDisplayTimingLastFreshTime = timing.actualPresentTime;
            }
            sDisplayTimingLastSceneId = scene_id;
        }
    }

} // namespace

namespace LLVKLoaderInternal
{
    void resetDisplayTimingHistory()
    {
        std::lock_guard<std::mutex> guard(sDisplayTimingMutex);
        sDisplayTimingMappings.clear();
        sDisplayTimingCompletedIds.clear();
        sDisplayTimingLastActualTime = 0;
        sDisplayTimingLastFreshTime = 0;
        sDisplayTimingLastSceneId = 0;
        sDisplayTimingLastSceneValid = false;
        sDisplayTimingIntervalStats = DisplayTimingIntervalStats{};
    }

    DisplayTimingIntervalStats collectDisplayTimingIntervalStats()
    {
        std::lock_guard<std::mutex> guard(sDisplayTimingMutex);
        DisplayTimingIntervalStats stats = std::move(sDisplayTimingIntervalStats);
        stats.enabled = sDisplayTimingEnabled;
        stats.pending_mappings = (U32)sDisplayTimingMappings.size();
        sDisplayTimingIntervalStats = DisplayTimingIntervalStats{};
        return stats;
    }
} // namespace LLVKLoaderInternal

namespace
{

    struct PEJobReleaser
    {
        PEJob&   mJob;
        VkResult mLastSr       = VK_ERROR_UNKNOWN;
        bool     mSlotDone     = false;
        bool     mSyncDone     = false;
        bool     mFenceDecided = false;

        explicit PEJobReleaser(PEJob& job) : mJob(job) {}

        void finishSlot(U32 state)
        {
            if (!mJob.is_frame || mSlotDone)
            {
                return;
            }
            {
                std::lock_guard<std::mutex> lk(sPESlotMutex);
                sPESlotState[mJob.slot].store(state);
            }
            sPESlotCv.notify_all();
            mSlotDone = true;
        }

        void finishSync(VkResult r)
        {
            if (mJob.sync == nullptr || mSyncDone)
            {
                return;
            }
            {
                std::lock_guard<std::mutex> lk(mJob.sync->m);
                mJob.sync->done   = true;
                mJob.sync->result = r;
                mJob.sync->cv.notify_all();
            }
            mSyncDone = true;
        }

        void failOneshotFence()
        {
            if (!mJob.is_oneshot || mFenceDecided)
            {
                return;
            }
            {
                std::lock_guard<std::mutex> lk(sPEFailedMutex);
                sPEFailedOneShotValues.push_back(mJob.timeline_value);
            }
            mFenceDecided = true;
        }

        void markFenceLive()
        {
            mFenceDecided = true;
        }

        ~PEJobReleaser()
        {
            finishSlot(PE_SLOT_FAILED);
            finishSync(mLastSr);
            failOneshotFence();
        }
    };

    void peExecute(PEJob& job)
    {
        PEJobReleaser rel(job);
        if (sVkDeviceLost.load(std::memory_order_acquire))
        {
            rel.finishSlot(PE_SLOT_FAILED);
            rel.failOneshotFence();
            static bool s_lost_skip_logged = false;
            if (!s_lost_skip_logged)
            {
                s_lost_skip_logged = true;
                LL_WARNS("Vulkan") << "PresentEngine: device lost — all further submits skipped"
                                   << " (first skipped: is_frame=" << (job.is_frame ? 1 : 0)
                                   << " is_oneshot=" << (job.is_oneshot ? 1 : 0) << ")" << LL_ENDL;
            }
            rel.finishSync(VK_ERROR_DEVICE_LOST);
            return;
        }
        // signal = [present 用 binary semaphore(有れば)] + [GPU タイムライン(有れば)]。
        // binary は値 0・timeline は job.timeline_value。順序は追加順で単調。
        VkSemaphore signal_sems[2];
        uint64_t    signal_vals[2];
        U32         sig_n = 0;
        if (job.signal_semaphore != VK_NULL_HANDLE)
        {
            signal_sems[sig_n] = job.signal_semaphore;
            signal_vals[sig_n] = 0;
            ++sig_n;
        }
        if (sGpuTimeline != VK_NULL_HANDLE)
        {
            signal_sems[sig_n] = sGpuTimeline;
            signal_vals[sig_n] = job.timeline_value;
            ++sig_n;
        }
        const uint64_t wait_val = 0;
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkTimelineSemaphoreSubmitInfo tsi = {};
        tsi.sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        tsi.waitSemaphoreValueCount   = (job.wait_semaphore != VK_NULL_HANDLE) ? 1u : 0u;
        tsi.pWaitSemaphoreValues      = &wait_val;
        tsi.signalSemaphoreValueCount = sig_n;
        tsi.pSignalSemaphoreValues    = signal_vals;

        VkSubmitInfo si = {};
        si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.pNext                = &tsi;
        si.commandBufferCount   = (job.cmd != VK_NULL_HANDLE) ? 1u : 0u;
        si.pCommandBuffers      = &job.cmd;
        si.waitSemaphoreCount   = (job.wait_semaphore != VK_NULL_HANDLE) ? 1u : 0u;
        si.pWaitSemaphores      = &job.wait_semaphore;
        si.pWaitDstStageMask    = &wait_stage;
        si.signalSemaphoreCount = sig_n;
        si.pSignalSemaphores    = signal_sems;

        if (job.is_async_producer)
        {
            sProdEnqToSubUs += vkMonoUs() - sProdEnqMonoUs.load();
            ++sProdSubCount;
        }
        const auto t0 = std::chrono::steady_clock::now();
        VkResult sr = vkQueueSubmit(sGraphicsQueue, 1, &si, job.fence);
#if LL_DARWIN
        recordDarwinSubmitTrace(job, sr);
#endif
        if (sr == VK_SUCCESS && job.wait_idle)
        {
            vkQueueWaitIdle(sGraphicsQueue);
        }
        sPESubmitUs += (U64)std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0).count();

        rel.mLastSr = sr;
        rel.finishSlot((sr == VK_SUCCESS) ? PE_SLOT_SUBMITTED : PE_SLOT_FAILED);
        if (job.is_oneshot)
        {
            if (sr == VK_SUCCESS)
            {
                rel.markFenceLive();
            }
            else
            {
                rel.failOneshotFence();
            }
        }
        else if (sr != VK_SUCCESS)
        {
            // frame/producer/async の submit 失敗 = この値は永久に signal されない。
            // waitTimeline がハングしないよう abandoned として記録(device-lost 有無に依らず安全側)。
            recordAbandonedTimelineValue(job.timeline_value);
        }

        if (sr != VK_SUCCESS)
        {
            if (sr == VK_ERROR_DEVICE_LOST)
            {
                static bool s_bc_dumped = false;
                if (!s_bc_dumped)
                {
                    s_bc_dumped = true;
                    dumpCheckpointsOnDeviceLost();
                    dumpDeviceFaultOnDeviceLost();
                }
#if LL_DARWIN
                dumpDarwinSubmitTraceOnDeviceLost("queue-submit");
#endif
                sVkDeviceLost.store(true, std::memory_order_release);
            }
            LL_WARNS("Vulkan") << "PresentEngine submit failed sr=" << (S32)sr
                               << " frame=" << (job.is_frame ? 1 : 0) << LL_ENDL;
        }
        else
        {
            for (PEPresentTarget& t : job.presents)
            {
                if (t.swapchain == VK_NULL_HANDLE)
                {
                    continue;
                }
                VkPresentInfoKHR present_info = {};
                present_info.sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                present_info.swapchainCount = 1;
                present_info.pSwapchains    = &t.swapchain;
                present_info.pImageIndices  = &t.image_index;
                if (t.wait_semaphore != VK_NULL_HANDLE)
                {
                    present_info.waitSemaphoreCount = 1;
                    present_info.pWaitSemaphores    = &t.wait_semaphore;
                }
                // ============================================================================
                // 3-OS MAINTAINERS — PLEASE VERIFY / IMPLEMENT ON YOUR PLATFORM:
                // This VK_KHR_present_wait path replaces the graphics driver's FIFO vsync
                // BUSY-WAIT (which pegs a whole CPU core spinning on clock_gettime) with a real
                // SLEEP via vkWaitForPresentKHR. Verified on Linux+NVIDIA (present thread CPU
                // 93% -> 2%, fps unchanged, tear-free). It is gated on sPresentWaitEnabled so it
                // silently no-ops where the extension is absent.
                //   * WINDOWS maintainer: NVIDIA/AMD/Intel Windows drivers expose
                //     VK_KHR_present_wait — confirm it is enabled (device-creation block in
                //     initVulkan) and that the present thread stops spinning; add a Win32
                //     fallback (e.g. driver knob / waitable swapchain) if the driver still spins.
                //   * macOS (MoltenVK) maintainer: VK_KHR_present_wait support is
                //     MoltenVK-version dependent. If unsupported, keep the no-op fallback and
                //     add a Metal-side present-pacing (CAMetalLayer / MTLDrawable) sleep so
                //     macOS does not busy-wait either.
                // Do NOT ship a platform that busy-waits vsync — return the core.
                // ============================================================================
                const bool prs_aux = (t.swapchain != sSwapchain);
                VkPresentIdKHR present_id_info = {};
                VkPresentTimeGOOGLE present_time = {};
                VkPresentTimesInfoGOOGLE display_timing_info = {};
                uint64_t this_present_id = 0;
                U32 display_timing_present_id = 0;
                const bool present_wait_active = sPresentWaitEnabled &&
                                                 sActivePresentMode == VK_PRESENT_MODE_FIFO_KHR;
                if (!prs_aux && sDisplayTimingEnabled)
                {
                    display_timing_present_id = nextDisplayTimingPresentId();
                    if (present_wait_active)
                    {
                        this_present_id = display_timing_present_id;
                    }
                    if (display_timing_present_id != 0)
                    {
                        present_time.presentID = display_timing_present_id;
                        // Zero asks the implementation to choose its normal
                        // presentation time. The returned actualPresentTime is
                        // the only signal counted as display completion.
                        present_time.desiredPresentTime = 0;
                        display_timing_info.sType = VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE;
                        display_timing_info.swapchainCount = 1;
                        display_timing_info.pTimes = &present_time;
                        display_timing_info.pNext = present_info.pNext;
                        present_info.pNext = &display_timing_info;
                    }
                }
                if (this_present_id == 0 && present_wait_active)
                {
                    this_present_id = sPresentIdCounter.fetch_add(1, std::memory_order_relaxed) + 1;
                }
                if (present_wait_active && this_present_id != 0)
                {
                    present_id_info.sType          = VK_STRUCTURE_TYPE_PRESENT_ID_KHR;
                    present_id_info.swapchainCount = 1;
                    present_id_info.pPresentIds    = &this_present_id;
                    present_id_info.pNext          = present_info.pNext;
                    present_info.pNext             = &present_id_info;
                }
                const auto p0 = std::chrono::steady_clock::now();
                VkResult pr;
                {
                    std::lock_guard<std::mutex> lk(sSwapchainAccessMutex);
                    const auto p1 = std::chrono::steady_clock::now();
                    sPEPrsLockUs += (U64)std::chrono::duration_cast<std::chrono::microseconds>(p1 - p0).count();
                    pr = vkQueuePresentKHR(sGraphicsQueue, &present_info);
                    if (!prs_aux && display_timing_present_id != 0 &&
                        (pr == VK_SUCCESS || pr == VK_SUBOPTIMAL_KHR))
                    {
                        rememberDisplayTimingPresent(display_timing_present_id, t.scene_id);
                    }
                    else if (!prs_aux && display_timing_present_id != 0)
                    {
                        forgetDisplayTimingPresent(display_timing_present_id);
                    }
                    if (!prs_aux && sDisplayTimingEnabled)
                    {
                        // Keep this query under the same main-swapchain lock
                        // as vkQueuePresentKHR so recreate cannot invalidate
                        // the handle while MoltenVK returns delayed history.
                        pollDisplayTimingLocked(t.swapchain);
                    }
                    (prs_aux ? sPEPrsAuxUs : sPEPrsMainUs) +=
                        (U64)std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::steady_clock::now() - p1).count();
                }
                sPEPresentUs += (U64)std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - p0).count();
                if (!prs_aux)
                {
                    sMainPresentCallCount.fetch_add(1, std::memory_order_relaxed);
                    if (pr == VK_SUCCESS || pr == VK_SUBOPTIMAL_KHR)
                    {
                        sMainPresentAcceptedCount.fetch_add(1, std::memory_order_relaxed);
                    }
                }
                if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR)
                {
                    sRecreateReasonMask.fetch_or((pr == VK_SUBOPTIMAL_KHR)
                                                     ? RECREATE_REASON_PRS_SUBOPTIMAL
                                                     : RECREATE_REASON_PRS_OUT_OF_DATE);
                    sSwapchainRecreatePending = true;
                }
                else if (pr == VK_ERROR_DEVICE_LOST)
                {
#if LL_DARWIN
                    dumpDarwinSubmitTraceOnDeviceLost("queue-present");
#endif
                    sVkDeviceLost.store(true, std::memory_order_release);
                }
                else if (present_wait_active && pr == VK_SUCCESS && this_present_id != 0)
                {
                    sMainPresentWaitAttemptCount.fetch_add(1, std::memory_order_relaxed);
                    const auto w0 = std::chrono::steady_clock::now();
                    VkResult wr = vkWaitForPresentKHR(sDevice, t.swapchain, this_present_id, 100000000ull);
                    (prs_aux ? sPEPwAuxUs : sPEPwMainUs) +=
                        (U64)std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::steady_clock::now() - w0).count();
                    if (!prs_aux)
                    {
                        if (wr == VK_SUCCESS)
                        {
                            sMainPresentDoneCount.fetch_add(1, std::memory_order_relaxed);
                        }
                        else if (wr == VK_TIMEOUT)
                        {
                            sMainPresentWaitTimeoutCount.fetch_add(1, std::memory_order_relaxed);
                        }
                        else
                        {
                            sMainPresentWaitErrorCount.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                    if (wr == VK_ERROR_DEVICE_LOST)
                    {
#if LL_DARWIN
                        dumpDarwinSubmitTraceOnDeviceLost("wait-for-present");
#endif
                        sVkDeviceLost.store(true, std::memory_order_release);
                    }
                }
            }
        }

        rel.finishSync(sr);
    }

    void peThreadMain()
    {
#if LL_LINUX
        pthread_setname_np(pthread_self(), "aya-present");
#endif
        for (;;)
        {
            PEJob job;
            {
                std::unique_lock<std::mutex> lk(sPEQueueMutex);
                sPEQueueCv.wait(lk, [] { return sPEStopRequested || !sPEJobs.empty(); });
                if (sPEJobs.empty())
                {
                    return;
                }
                job = std::move(sPEJobs.front());
                sPEJobs.pop_front();
                sPEBusy = true;
            }
            peExecute(job);
            {
                std::lock_guard<std::mutex> lk(sPEQueueMutex);
                sPEBusy = false;
            }
            sPEQueueCv.notify_all();
        }
    }
} // namespace

namespace LLVKLoaderInternal
{

    // GPU 完了タイムラインの現在値(= 完了済みの最大 submit 値)。任意スレッドから race 無く可。
    uint64_t gpuTimelineValue()
    {
        uint64_t v = 0;
        vkGetSemaphoreCounterValue(sDevice, sGpuTimeline, &v);
        return v;
    }
} // namespace LLVKLoaderInternal

namespace
{

    // submit 失敗(非 device-lost)で永久に signal されない値を記録。記録時に counter を越えた
    // 既済値を prune して集合を有界化(失敗は稀・集合は小)。
    void recordAbandonedTimelineValue(uint64_t v)
    {
        const uint64_t cur = gpuTimelineValue();
        std::lock_guard<std::mutex> lk(sPEAbandonedMutex);
        size_t w = 0;
        for (size_t r = 0; r < sPEAbandonedTimelineValues.size(); ++r)
        {
            const uint64_t a = sPEAbandonedTimelineValues[r];
            if (a > cur)
            {
                sPEAbandonedTimelineValues[w++] = a;
            }
        }
        sPEAbandonedTimelineValues.resize(w);
        sPEAbandonedTimelineValues.push_back(v);
    }

    bool timelineValueAbandoned(uint64_t v)
    {
        std::lock_guard<std::mutex> lk(sPEAbandonedMutex);
        for (uint64_t a : sPEAbandonedTimelineValues)
        {
            if (a == v)
            {
                return true;
            }
        }
        return false;
    }
} // namespace

namespace LLVKLoaderInternal
{

    // GPU タイムラインが値 v に到達するまで待つ(= 該当 submit の GPU 完了待ち)。
    // 任意スレッドから race 無く可。device-lost もしくは当該 submit 失敗(永久未 signal)で脱出。v==0 は待ち不要。
    void waitTimeline(uint64_t v)
    {
        if (v == 0 || sGpuTimeline == VK_NULL_HANDLE)
        {
            return;
        }
        VkSemaphoreWaitInfo wi = {};
        wi.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        wi.semaphoreCount = 1;
        wi.pSemaphores    = &sGpuTimeline;
        wi.pValues        = &v;
        while (vkWaitSemaphores(sDevice, &wi, 50000000ull) == VK_TIMEOUT)
        {
            if (sVkDeviceLost.load(std::memory_order_acquire))
            {
                return;
            }
            if (timelineValueAbandoned(v))
            {
                return;
            }
        }
    }

    // enqueue 時に単調な timeline 値を採番し job に載せて返す。
    // 採番と push を同一ロックで行うため 値順 == 提出順 == timeline signal 順(単調)。
    uint64_t peEnqueue(PEJob&& job)
    {
        if (!sPERunning)
        {
            const uint64_t v = ++sTimelineNext;
            job.timeline_value = v;
            peExecute(job);
            return v;
        }
        uint64_t v;
        {
            std::lock_guard<std::mutex> lk(sPEQueueMutex);
            v = ++sTimelineNext;
            job.timeline_value = v;
            sPEJobs.push_back(std::move(job));
        }
        sPEQueueCv.notify_one();
        return v;
    }

    VkResult peSubmitBlocking(VkCommandBuffer cmd, VkFence fence, bool wait_idle)
    {
        PESyncPoint sync;
        PEJob job;
        job.cmd       = cmd;
        job.fence     = fence;
        job.wait_idle = wait_idle;
        job.sync      = &sync;
        if (!sPERunning)
        {
            job.timeline_value = ++sTimelineNext;
            peExecute(job);
            return sync.result;
        }
        {
            std::lock_guard<std::mutex> lk(sPEQueueMutex);
            job.timeline_value = ++sTimelineNext;
            sPEJobs.push_back(std::move(job));
        }
        sPEQueueCv.notify_one();
        std::unique_lock<std::mutex> lk(sync.m);
        sync.cv.wait(lk, [&] { return sync.done; });
        return sync.result;
    }

    void peDrain()
    {
        if (!sPERunning)
        {
            return;
        }
        std::unique_lock<std::mutex> lk(sPEQueueMutex);
        sPEQueueCv.wait(lk, [] { return sPEJobs.empty() && !sPEBusy; });
    }

    U32 peQueueDepth()
    {
        std::lock_guard<std::mutex> lk(sPEQueueMutex);
        return (U32)sPEJobs.size();
    }

    void peStart()
    {
        const char* e = getenv("AYASTORM_MT_THREADS");
        sPEThreaded = (e == nullptr) || (atoi(e) > 1);
        const char* p = getenv("AYASTORM_MT_PE");
        if (p != nullptr && atoi(p) == 0)
        {
            sPEThreaded = false;
        }
        if (!sPEThreaded)
        {
            return;
        }
        sPEStopRequested = false;
        sPERunning       = true;
        sPEThread        = std::thread(peThreadMain);
    }

    void peStop()
    {
        if (!sPERunning)
        {
            return;
        }
        {
            std::lock_guard<std::mutex> lk(sPEQueueMutex);
            sPEStopRequested = true;
        }
        sPEQueueCv.notify_all();
        if (sPEThread.joinable())
        {
            sPEThread.join();
        }
        sPERunning  = false;
        sPEThreaded = false;
    }
} // namespace LLVKLoaderInternal


bool peThreaded()
{
    return sPERunning;
}

bool immediatePresentActive()
{
    return sActivePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR;
}

} // namespace LLVKLoader
