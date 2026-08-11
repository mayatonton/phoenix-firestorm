/**
* @file llvkdiag.cpp
* @brief AYAstorm r42 Vulkan loader — device-lost diagnostics (checkpoints / device fault / Darwin submit trace)
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

namespace LLVKLoaderInternal
{

    void gpuCheckpointImpl(const char* label)
    {
        if (!sCheckpointsEnabled)
        {
            return;
        }
        VkCommandBuffer cmd = currentRecordCmd();
        if (cmd != VK_NULL_HANDLE)
        {
            vkCmdSetCheckpointNV(cmd, (const void*)label);
        }
    }

    void dumpCheckpointsOnDeviceLost()
    {
        if (!sCheckpointsEnabled || sGraphicsQueue == VK_NULL_HANDLE)
        {
            return;
        }
        U32 n = 0;
        vkGetQueueCheckpointDataNV(sGraphicsQueue, &n, nullptr);
        if (n == 0)
        {
            LL_WARNS("Vulkan") << "GPU breadcrumb: no checkpoint data on device-lost" << LL_ENDL;
            return;
        }
        std::vector<VkCheckpointDataNV> data(n);
        for (auto& d : data)
        {
            d.sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
            d.pNext = nullptr;
        }
        vkGetQueueCheckpointDataNV(sGraphicsQueue, &n, data.data());
        for (const auto& d : data)
        {
            const char* lbl = (const char*)d.pCheckpointMarker;
            LL_WARNS("Vulkan") << "GPU breadcrumb @devlost: stage=0x" << std::hex << (U32)d.stage << std::dec
                               << " last-reached=" << (lbl ? lbl : "(null)") << LL_ENDL;
        }
    }

    void dumpDeviceFaultOnDeviceLost()
    {
        if (!sDeviceFaultEnabled || sDevice == VK_NULL_HANDLE || vkGetDeviceFaultInfoEXT == nullptr)
        {
            return;
        }
        VkDeviceFaultCountsEXT counts = {};
        counts.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT;
        if (vkGetDeviceFaultInfoEXT(sDevice, &counts, nullptr) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "GPU fault @devlost: count query failed" << LL_ENDL;
            return;
        }
        std::vector<VkDeviceFaultAddressInfoEXT> addrs(counts.addressInfoCount);
        for (auto& a : addrs) { a = {}; }
        std::vector<VkDeviceFaultVendorInfoEXT> vendors(counts.vendorInfoCount);
        for (auto& v : vendors) { v = {}; }
        counts.vendorBinarySize = 0;
        VkDeviceFaultInfoEXT info = {};
        info.sType         = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT;
        info.pAddressInfos = addrs.empty() ? nullptr : addrs.data();
        info.pVendorInfos  = vendors.empty() ? nullptr : vendors.data();
        if (vkGetDeviceFaultInfoEXT(sDevice, &counts, &info) != VK_SUCCESS)
        {
            LL_WARNS("Vulkan") << "GPU fault @devlost: info query failed" << LL_ENDL;
            return;
        }
        LL_WARNS("Vulkan") << "GPU fault @devlost: desc='" << info.description
                           << "' addrs=" << counts.addressInfoCount
                           << " vendors=" << counts.vendorInfoCount << LL_ENDL;
        static const char* addr_type_names[] = {
            "NONE", "READ_INVALID", "WRITE_INVALID", "EXECUTE_INVALID",
            "IP_UNKNOWN", "IP_INVALID", "IP_FAULT"
        };
        for (U32 i = 0; i < counts.addressInfoCount; ++i)
        {
            const U32 t = (U32)addrs[i].addressType;
            LL_WARNS("Vulkan") << "GPU fault addr[" << i << "]: type="
                               << (t < 7 ? addr_type_names[t] : "?") << "(" << t << ")"
                               << " va=0x" << std::hex << addrs[i].reportedAddress
                               << " precision=0x" << addrs[i].addressPrecision << std::dec << LL_ENDL;
        }
        for (U32 i = 0; i < counts.vendorInfoCount; ++i)
        {
            LL_WARNS("Vulkan") << "GPU fault vendor[" << i << "]: '" << vendors[i].description
                               << "' code=0x" << std::hex << vendors[i].vendorFaultCode
                               << " data=0x" << vendors[i].vendorFaultData << std::dec << LL_ENDL;
        }
    }
} // namespace LLVKLoaderInternal

namespace
{

#if LL_DARWIN
    // MoltenVK does not expose device-fault or checkpoint extensions on the
    // current Apple GPU path. Keep a small host-side submit history so a
    // VK_ERROR_DEVICE_LOST report identifies the work that was in flight.
    struct PEDarwinSubmitTrace
    {
        const char* type             = "unknown";
        const char* oneshot_source   = "-";
        VkResult    result           = VK_ERROR_UNKNOWN;
        U64         command_buffer   = 0;
        U64         fence            = 0;
        U64         wait_semaphore   = 0;
        U64         signal_semaphore = 0;
        uint64_t    timeline_value   = 0;
        U32         slot             = 0;
        U32         present_count    = 0;
        bool        wait_idle        = false;
        U64         oneshot_staging_bytes = 0;
    };

    std::mutex                      sPEDarwinSubmitTraceMutex;
    std::deque<PEDarwinSubmitTrace> sPEDarwinSubmitTrace;
    std::atomic<bool>               sPEDarwinDeviceLostTraceDumped{false};

    const char* peJobType(const PEJob& job)
    {
        if (job.is_frame)
        {
            return "frame";
        }
        if (job.is_async_producer)
        {
            return "async-producer";
        }
        if (job.is_oneshot)
        {
            return "one-shot";
        }
        if (job.sync != nullptr)
        {
            return "blocking";
        }
        if (!job.presents.empty())
        {
            return "aux-present";
        }
        return "producer";
    }
} // namespace

namespace LLVKLoaderInternal
{

    void recordDarwinSubmitTrace(const PEJob& job, VkResult result)
    {
        PEDarwinSubmitTrace trace;
        trace.type             = peJobType(job);
        trace.oneshot_source   = job.oneshot_source ? job.oneshot_source : "-";
        trace.result           = result;
        trace.command_buffer   = (U64)(uintptr_t)job.cmd;
        trace.fence            = (U64)(uintptr_t)job.fence;
        trace.wait_semaphore   = (U64)(uintptr_t)job.wait_semaphore;
        trace.signal_semaphore = (U64)(uintptr_t)job.signal_semaphore;
        trace.timeline_value   = job.timeline_value;
        trace.slot             = job.slot;
        trace.present_count    = (U32)job.presents.size();
        trace.wait_idle        = job.wait_idle;
        trace.oneshot_staging_bytes = job.oneshot_staging_bytes;

        std::lock_guard<std::mutex> lk(sPEDarwinSubmitTraceMutex);
        constexpr size_t history_capacity = 16;
        if (sPEDarwinSubmitTrace.size() == history_capacity)
        {
            sPEDarwinSubmitTrace.pop_front();
        }
        sPEDarwinSubmitTrace.push_back(trace);
    }

    void dumpDarwinSubmitTraceOnDeviceLost(const char* trigger)
    {
        if (sPEDarwinDeviceLostTraceDumped.exchange(true))
        {
            return;
        }

        std::deque<PEDarwinSubmitTrace> history;
        {
            std::lock_guard<std::mutex> lk(sPEDarwinSubmitTraceMutex);
            history = sPEDarwinSubmitTrace;
        }
        LL_WARNS("Vulkan") << "VKC-DEVLOST trigger=" << trigger
                           << " submit_history=" << history.size() << LL_ENDL;
        U32 index = 0;
        for (const PEDarwinSubmitTrace& trace : history)
        {
            LL_WARNS("Vulkan") << "VKC-DEVLOST submit[" << index++ << "]: type=" << trace.type
                               << " oneshot_source=" << trace.oneshot_source
                               << " staging_bytes=" << trace.oneshot_staging_bytes
                               << " result=" << (S32)trace.result
                               << " timeline=" << trace.timeline_value
                               << " slot=" << trace.slot
                               << " presents=" << trace.present_count
                               << " wait_idle=" << (trace.wait_idle ? 1 : 0)
                               << " cmd=0x" << std::hex << trace.command_buffer
                               << " fence=0x" << trace.fence
                               << " wait_sem=0x" << trace.wait_semaphore
                               << " signal_sem=0x" << trace.signal_semaphore
                               << std::dec << LL_ENDL;
        }
    }
#endif
} // namespace LLVKLoaderInternal


void gpuCheckpoint(const char* label)
{
    gpuCheckpointImpl(label);
}

} // namespace LLVKLoader
