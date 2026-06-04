/**
 * @file lluboringbuffer.h
 * @brief AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-4:
 *        per-frame / per-pass cadence UBO ring buffer + grow 機構 (= RB 持越項目、
 *        旧 ID R1 = 設計 review 2026-06-03 §3.1 で ring buffer prefix へ rename)
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * AYAstorm Viewer Source Code
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 *
 * 設計根拠: docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md
 *   §7.2 ring buffer 容量算定:
 *     - 5000 draw 通常 sim: 5000 × 4 × 64 B = 1.28 MB / frame × 3 frame =
 *       3.84 MB → **4 MB ring buffer** (= 起動時 prealloc)
 *     - 20000 draw 過密 sim: 5.12 MB / frame × 3 = 15.36 MB →
 *       **16 MB ring buffer** (= grow 上限)
 *     - cvar `AYARingBufferSizeMB` で起動時 prealloc 容量 default 4 を配信
 *   §7.5 chunk 構造:
 *     - vmaCreateBuffer を 1 frame 単位で 3 段 chunk に分け
 *       (= 4 MB / 3 ≈ 1.33 MB / frame chunk)
 *     - chunk write head が chunk_size 超過 → 次 chunk へ wrap
 *       (= N-2 chunk は GPU 読込完了済保証、FRAMES_IN_FLIGHT=3 triple-
 *       buffering と同形)
 *     - chunk 内枯渇 (= 1 frame で 3 chunk 全消費想定) = ring buffer grow
 *       (= 4 MB → 8 MB → 16 MB)
 *   §7.3 offset alignment:
 *     - `minUniformBufferOffsetAlignment` (= Vulkan 1.3 spec 最大 256、
 *       AMD/NVIDIA 典型 64) で align_up
 *     - Codegen 側 UBO struct size は **256 B safe** padding 出力済
 *
 * 本 class は Vulkan device 非依存の bookkeeping algorithm = 実 VkBuffer 生成
 * / 破棄は caller injected callback (BufferAllocator / BufferDestroyer) 経由。
 * これにより Vulkan device 未初期化な unittest 環境でも ring buffer grow +
 * chunk wrap algorithm 単独検証可能 (= PC-3 (α) パターン踏襲 = llcommon 単体
 * 配置 + llcommon 既設 LL_ADD_INTEGRATION_TEST framework 経由 unittest)。
 *
 * 実 VkBuffer (= VMA `vmaCreateBuffer` HOST_VISIBLE+MAPPED) factory injection
 * は PC-6 (= 5 cadence 全経路 update site 実装) で llrender / llvkloader 側
 * で wire up 予定。本 PC-4 段は algorithm + unittest + settings.xml cvar 露出
 * のみ (= cvar 読込 / Vulkan device 配線は PC-6 持越、AYA 採用 (α') 2026-06-04)。
 */

#ifndef LL_LLUBORINGBUFFER_H
#define LL_LLUBORINGBUFFER_H

#include <cstdint>
#include <functional>

class LLUboRingBuffer
{
public:
    using BufferHandle    = std::uint64_t;
    using BufferAllocator = std::function<BufferHandle(std::uint32_t /*size_bytes*/)>;
    using BufferDestroyer = std::function<void(BufferHandle)>;

    // design 07 §7.2 確定値
    static constexpr std::uint32_t kInitialSizeMB    = 4;
    static constexpr std::uint32_t kMaxSizeMB        = 16;
    // design 07 §7.5 + §8.4 FRAMES_IN_FLIGHT=3 triple-buffering と同形
    static constexpr std::uint32_t kFramesInFlight   = 3;
    // design 07 §7.3 Vulkan 1.3 spec 最大 minUniformBufferOffsetAlignment
    static constexpr std::uint32_t kDefaultAlignment = 256;

    LLUboRingBuffer(BufferAllocator allocator,
                    BufferDestroyer destroyer,
                    std::uint32_t initial_size_mb = kInitialSizeMB,
                    std::uint32_t max_size_mb     = kMaxSizeMB,
                    std::uint32_t alignment       = kDefaultAlignment);
    ~LLUboRingBuffer();

    LLUboRingBuffer(const LLUboRingBuffer&)            = delete;
    LLUboRingBuffer& operator=(const LLUboRingBuffer&) = delete;

    bool initialize();
    void shutdown();

    struct AllocateResult
    {
        BufferHandle  buffer  = 0;
        std::uint32_t offset  = 0;  // ring buffer 先頭からの byte offset (= vkCmdBindDescriptorSets pDynamicOffsets[] 投入経路、design 07 §7.4)
        std::uint32_t size    = 0;  // alignment 切上後 byte 数
        bool          success = false;
        bool          grew    = false; // 本 alloc 内で ring buffer grow が発火した
    };
    AllocateResult allocate(std::uint32_t size_bytes);

    // 次 frame に進む = frame index advance + chunk 状態 reset
    // (design 07 §8.4 = `beginFrame()` 1 回の sFrameIndex 進行で全 cadence
    //  同期 rotate)
    void beginFrame();

    std::uint32_t getInitialSizeMB()         const noexcept { return mInitialSizeMB; }
    std::uint32_t getMaxSizeMB()             const noexcept { return mMaxSizeMB; }
    std::uint32_t getCurrentSizeMB()         const noexcept;
    std::uint32_t getCurrentSizeBytes()      const noexcept { return mCurrentSizeBytes; }
    std::uint32_t getChunkSizeBytes()        const noexcept { return mCurrentSizeBytes / kFramesInFlight; }
    std::uint32_t getAlignment()             const noexcept { return mAlignment; }
    std::uint32_t getFrameIndex()            const noexcept { return mFrameIndex; }
    std::uint32_t getActiveChunk()           const noexcept { return mActiveChunk; }
    std::uint32_t getChunkBytesUsed()        const noexcept { return mChunkBytesUsed; }
    std::uint32_t getChunksUsedThisFrame()   const noexcept { return mChunksConsumedThisFrame; }
    BufferHandle  getBuffer()                const noexcept { return mBuffer; }
    bool          isInitialized()            const noexcept { return mInitialized; }
    bool          isAtMaxSize()              const noexcept;

private:
    bool         tryGrow();
    BufferHandle invokeAllocator(std::uint32_t size_bytes);
    void         destroyCurrentBuffer();

    BufferAllocator mAllocator;
    BufferDestroyer mDestroyer;
    std::uint32_t   mInitialSizeMB;
    std::uint32_t   mMaxSizeMB;
    std::uint32_t   mAlignment;

    BufferHandle    mBuffer                  = 0;
    std::uint32_t   mCurrentSizeBytes        = 0;
    std::uint32_t   mFrameIndex              = 0;
    std::uint32_t   mActiveChunk             = 0;
    std::uint32_t   mChunkBytesUsed          = 0;
    std::uint32_t   mChunksConsumedThisFrame = 1;
    bool            mInitialized             = false;
};

#endif // LL_LLUBORINGBUFFER_H
