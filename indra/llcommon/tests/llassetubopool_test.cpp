/**
 * @file llassetubopool_test.cpp
 * @brief LLAssetUboPool grow algorithm tut tests
 *        (r41 Phase 1.C PC-3、(W2) 持越項目)
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
 * 検証対象 (= design/07-vulkan-api-state.md §6.1/§6.3/§7.2):
 *   - 起動時 prealloc N=64 (= 1 物理 pool 確保)
 *   - 全 pool 枯渇 detect 時 = grow chunk 64 で新 1 物理 pool 追加
 *   - reset / shutdown / factory failure / custom params の edge case
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llassetubopool.h"

namespace tut
{
    struct asset_ubo_pool_data
    {
        std::uint64_t                       next_handle = 1;
        std::vector<LLAssetUboPool::PoolHandle> destroyed;

        LLAssetUboPool::PoolFactory makeIncrementingFactory()
        {
            return [this]() -> LLAssetUboPool::PoolHandle
            {
                return next_handle++;
            };
        }
        LLAssetUboPool::PoolDestroyer makeRecordingDestroyer()
        {
            return [this](LLAssetUboPool::PoolHandle h)
            {
                destroyed.push_back(h);
            };
        }
    };

    typedef test_group<asset_ubo_pool_data> asset_ubo_pool_group;
    typedef asset_ubo_pool_group::object    asset_ubo_pool_object;
    tut::asset_ubo_pool_group g_asset_ubo_pool_group("LLAssetUboPool");

    // test<1>: default 値検証 (= 07 §6.1/§6.3/§7.2 確定値 N=64 / grow chunk=64)
    template<> template<>
    void asset_ubo_pool_object::test<1>()
    {
        LLAssetUboPool pool(makeIncrementingFactory(), makeRecordingDestroyer());
        ensure_equals("default prealloc N=64", pool.getPreallocCount(), 64u);
        ensure_equals("default grow chunk=64", pool.getGrowChunkCount(), 64u);
        ensure_equals("constructor: 0 pools",  pool.getPoolCount(),     0u);
        ensure("constructor: not initialized", !pool.isInitialized());
    }

    // test<2>: 起動時 prealloc 1 物理 pool 確保 = capacity 64
    template<> template<>
    void asset_ubo_pool_object::test<2>()
    {
        LLAssetUboPool pool(makeIncrementingFactory(), makeRecordingDestroyer());
        ensure("initialize success",        pool.initialize());
        ensure("initialized flag set",      pool.isInitialized());
        ensure_equals("1 pool after init",  pool.getPoolCount(), 1u);
        ensure_equals("64 capacity",        pool.getTotalAssetCapacity(), 64u);
        ensure_equals("0 used after init",  pool.getUsedAssetCount(), 0u);
    }

    // test<3>: 64 acquire まで pool 1 個で済む (= prealloc 容量内)
    template<> template<>
    void asset_ubo_pool_object::test<3>()
    {
        LLAssetUboPool pool(makeIncrementingFactory(), makeRecordingDestroyer());
        ensure("initialize success", pool.initialize());
        for (std::uint32_t i = 0; i < 64; ++i)
        {
            auto r = pool.acquire();
            ensure("acquire success", r.success);
            ensure_equals("slot index ascending", r.slot_in_pool, i);
        }
        ensure_equals("still 1 pool", pool.getPoolCount(), 1u);
        ensure_equals("64 used",      pool.getUsedAssetCount(), 64u);
    }

    // test<4>: 65 個目 acquire で grow chunk 64 1 個追加 (= 2 pools / capacity 128)
    template<> template<>
    void asset_ubo_pool_object::test<4>()
    {
        LLAssetUboPool pool(makeIncrementingFactory(), makeRecordingDestroyer());
        ensure("initialize success", pool.initialize());
        for (std::uint32_t i = 0; i < 64; ++i) pool.acquire();
        ensure_equals("1 pool before grow", pool.getPoolCount(), 1u);
        auto r = pool.acquire();
        ensure("65th acquire success (grew)",   r.success);
        ensure_equals("2 pools after grow",     pool.getPoolCount(),       2u);
        ensure_equals("65 used",                pool.getUsedAssetCount(),  65u);
        ensure_equals("slot in new pool = 0",   r.slot_in_pool,            0u);
        ensure_equals("total capacity 64+64",   pool.getTotalAssetCapacity(), 128u);
    }

    // test<5>: 多重 grow (= 200 acquire で 4 個 pool、64×4=256 ≥ 200)
    template<> template<>
    void asset_ubo_pool_object::test<5>()
    {
        LLAssetUboPool pool(makeIncrementingFactory(), makeRecordingDestroyer());
        ensure("initialize success", pool.initialize());
        for (std::uint32_t i = 0; i < 200; ++i)
        {
            auto r = pool.acquire();
            ensure("acquire success", r.success);
        }
        ensure_equals("4 pools after 200 acquire", pool.getPoolCount(),         4u);
        ensure_equals("200 used",                  pool.getUsedAssetCount(),    200u);
        ensure_equals("total capacity 256",        pool.getTotalAssetCapacity(),256u);
    }

    // test<6>: resetAllSlots → pool 数維持 + used 0 + 再 acquire 可
    template<> template<>
    void asset_ubo_pool_object::test<6>()
    {
        LLAssetUboPool pool(makeIncrementingFactory(), makeRecordingDestroyer());
        pool.initialize();
        for (std::uint32_t i = 0; i < 100; ++i) pool.acquire();
        ensure_equals("2 pools before reset",         pool.getPoolCount(),      2u);
        pool.resetAllSlots();
        ensure_equals("2 pools after reset",          pool.getPoolCount(),      2u);
        ensure_equals("0 used after reset",           pool.getUsedAssetCount(), 0u);
        auto r = pool.acquire();
        ensure("acquire after reset success",         r.success);
        ensure_equals("1 used after reset + acquire", pool.getUsedAssetCount(), 1u);
    }

    // test<7>: shutdown → 全 pool destroyer 呼出 + state reset + 再 initialize 可
    template<> template<>
    void asset_ubo_pool_object::test<7>()
    {
        LLAssetUboPool pool(makeIncrementingFactory(), makeRecordingDestroyer());
        pool.initialize();
        for (std::uint32_t i = 0; i < 100; ++i) pool.acquire();
        ensure_equals("2 pools before shutdown", pool.getPoolCount(), 2u);
        pool.shutdown();
        ensure_equals("0 pools after shutdown",       pool.getPoolCount(), 0u);
        ensure_equals("2 destroyer calls",            static_cast<std::uint32_t>(destroyed.size()), 2u);
        ensure("not initialized after shutdown",      !pool.isInitialized());
        ensure("re-initialize OK after shutdown",     pool.initialize());
        ensure_equals("1 pool after re-init",         pool.getPoolCount(), 1u);
    }

    // test<8>: factory failure (= 0 handle return) → initialize fail + acquire fail
    template<> template<>
    void asset_ubo_pool_object::test<8>()
    {
        auto failing_factory = []() -> LLAssetUboPool::PoolHandle { return 0; };
        auto noop_destroyer  = [](LLAssetUboPool::PoolHandle) {};
        LLAssetUboPool pool(failing_factory, noop_destroyer);
        ensure("initialize fails on factory==0",   !pool.initialize());
        ensure("not initialized after fail",       !pool.isInitialized());
        ensure_equals("0 pools after failed init", pool.getPoolCount(), 0u);
        auto r = pool.acquire();
        ensure("acquire fails when uninitialized", !r.success);
    }

    // test<9>: custom prealloc / grow chunk = 16 / 8
    template<> template<>
    void asset_ubo_pool_object::test<9>()
    {
        LLAssetUboPool pool(makeIncrementingFactory(), makeRecordingDestroyer(), 16, 8);
        ensure_equals("custom prealloc 16", pool.getPreallocCount(),  16u);
        ensure_equals("custom grow 8",      pool.getGrowChunkCount(), 8u);
        pool.initialize();
        ensure_equals("1 pool after init", pool.getPoolCount(),         1u);
        ensure_equals("capacity 16",       pool.getTotalAssetCapacity(),16u);
        for (std::uint32_t i = 0; i < 16; ++i) pool.acquire();
        ensure_equals("1 pool after 16 acquire", pool.getPoolCount(),   1u);
        pool.acquire();
        ensure_equals("2 pools after 17",        pool.getPoolCount(),   2u);
        ensure_equals("capacity 16+8=24",        pool.getTotalAssetCapacity(), 24u);
    }

    // test<10>: 連続 grow で pool handle が単調増加 (= factory 1 回ずつ呼出証跡)
    template<> template<>
    void asset_ubo_pool_object::test<10>()
    {
        LLAssetUboPool pool(makeIncrementingFactory(), makeRecordingDestroyer());
        pool.initialize();
        auto r1 = pool.acquire();
        for (std::uint32_t i = 1; i < 64; ++i) pool.acquire();
        auto r2 = pool.acquire();
        for (std::uint32_t i = 1; i < 64; ++i) pool.acquire();
        auto r3 = pool.acquire();
        ensure_equals("1st pool handle",  r1.pool, static_cast<LLAssetUboPool::PoolHandle>(1));
        ensure_equals("2nd pool handle",  r2.pool, static_cast<LLAssetUboPool::PoolHandle>(2));
        ensure_equals("3rd pool handle",  r3.pool, static_cast<LLAssetUboPool::PoolHandle>(3));
        ensure_equals("3 pools total",    pool.getPoolCount(), 3u);
    }
}
