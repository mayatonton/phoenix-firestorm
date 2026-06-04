/**
 * @file llassetubopool.h
 * @brief AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-3:
 *        per-asset cadence descriptor pool grow 機構 (= W2 持越項目)
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
 *   §6.1 / §6.3 / §7.2 確定値:
 *     - 1 物理 pool = 64 asset 枠 (= 64 × 3 frame-in-flight = 192 set /
 *       576 UBO / 9408 sampler)
 *     - 起動時 prealloc 1 物理 pool (capacity = 64 asset)
 *     - 全 pool 枯渇 detect 時 = 新 1 物理 pool 追加 (= 各 64 asset 枠 = grow chunk)
 *     - 配列化 `sAssetUboPools[]` で grow 履歴を保持、cleanup は cadence
 *       単位 (= shutdown 時 reverse 順 destroy)
 *
 * 本 class は Vulkan device 非依存の bookkeeping algorithm = 実 pool 生成 /
 * 破棄は caller injected callback (PoolFactory / PoolDestroyer) 経由。
 * これにより Vulkan device 未初期化な unittest 環境でも grow algorithm
 * 単独検証可能 (= Phase 1.C PC-3 AYA 採用 案 (α) = llcommon 単体配置 +
 * llcommon 既設 LL_ADD_INTEGRATION_TEST framework 経由 unittest)。
 *
 * 実 VkDescriptorPool factory injection は PC-6 (= 5 cadence 全経路 update
 * site 実装) で llrender / llvkloader 側で wire up 予定。本 PC-3 段は
 * algorithm + unittest のみ。
 */

#ifndef LL_LLASSETUBOPOOL_H
#define LL_LLASSETUBOPOOL_H

#include <cstdint>
#include <functional>
#include <vector>

class LLAssetUboPool
{
public:
    using PoolHandle    = std::uint64_t;
    using PoolFactory   = std::function<PoolHandle()>;
    using PoolDestroyer = std::function<void(PoolHandle)>;

    static constexpr std::uint32_t kPreallocAssetCount  = 64;
    static constexpr std::uint32_t kGrowChunkAssetCount = 64;

    LLAssetUboPool(PoolFactory   factory,
                   PoolDestroyer destroyer,
                   std::uint32_t prealloc_count = kPreallocAssetCount,
                   std::uint32_t grow_chunk     = kGrowChunkAssetCount);
    ~LLAssetUboPool();

    LLAssetUboPool(const LLAssetUboPool&)            = delete;
    LLAssetUboPool& operator=(const LLAssetUboPool&) = delete;

    bool initialize();
    void shutdown();

    struct AcquireResult
    {
        PoolHandle    pool         = 0;
        std::uint32_t slot_in_pool = 0;
        bool          success      = false;
    };
    AcquireResult acquire();

    void resetAllSlots();

    std::uint32_t getPoolCount()          const noexcept { return static_cast<std::uint32_t>(mPools.size()); }
    std::uint32_t getTotalAssetCapacity() const noexcept;
    std::uint32_t getUsedAssetCount()     const noexcept;
    std::uint32_t getPreallocCount()      const noexcept { return mPreallocCount; }
    std::uint32_t getGrowChunkCount()     const noexcept { return mGrowChunkCount; }
    bool          isInitialized()         const noexcept { return mInitialized; }

private:
    bool growOnce(std::uint32_t capacity);

    PoolFactory                mFactory;
    PoolDestroyer              mDestroyer;
    std::uint32_t              mPreallocCount;
    std::uint32_t              mGrowChunkCount;
    std::vector<PoolHandle>    mPools;
    std::vector<std::uint32_t> mPoolCapacities;
    std::vector<std::uint32_t> mUsedPerPool;
    bool                       mInitialized = false;
};

#endif // LL_LLASSETUBOPOOL_H
