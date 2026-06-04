/**
 * @file lluboringbuffer_test.cpp
 * @brief LLUboRingBuffer grow + chunk wrap algorithm tut tests
 *        (r41 Phase 1.C PC-4、(RB) 持越項目 = 旧 (R1) rename)
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
 * 検証対象 (= design/07-vulkan-api-state.md §7.2/§7.3/§7.5):
 *   - 起動時 prealloc 4 MB / grow 上限 16 MB
 *   - alignment up (= minUniformBufferOffsetAlignment 256 safe)
 *   - chunk 構造 (FRAMES_IN_FLIGHT=3 と同形)
 *   - chunk wrap (= chunks_used <= 2 まで safe wrap)
 *   - 3 chunk 想定 (= N-1 hazard) で ring buffer grow trigger
 *   - cap at max (= 16 MB 到達後 grow 不可で alloc fail)
 *   - beginFrame() frame index advance + per-frame chunk 状態 reset
 *   - allocator failure / custom params edge case
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../lluboringbuffer.h"

namespace tut
{
    struct ubo_ring_buffer_data
    {
        std::uint64_t                              next_handle = 100;
        std::vector<LLUboRingBuffer::BufferHandle> destroyed;
        std::vector<std::uint32_t>                 allocated_sizes;

        LLUboRingBuffer::BufferAllocator makeIncrementingAllocator()
        {
            return [this](std::uint32_t size_bytes) -> LLUboRingBuffer::BufferHandle
            {
                allocated_sizes.push_back(size_bytes);
                return next_handle++;
            };
        }
        LLUboRingBuffer::BufferDestroyer makeRecordingDestroyer()
        {
            return [this](LLUboRingBuffer::BufferHandle h)
            {
                destroyed.push_back(h);
            };
        }
    };

    typedef test_group<ubo_ring_buffer_data> ubo_ring_buffer_group;
    typedef ubo_ring_buffer_group::object    ubo_ring_buffer_object;
    tut::ubo_ring_buffer_group g_ubo_ring_buffer_group("LLUboRingBuffer");

    // test<1>: default 値検証 (= design 07 §7.2 確定値 4 MB initial / 16 MB max / alignment 256 / FRAMES_IN_FLIGHT 3)
    template<> template<>
    void ubo_ring_buffer_object::test<1>()
    {
        LLUboRingBuffer rb(makeIncrementingAllocator(), makeRecordingDestroyer());
        ensure_equals("kInitialSizeMB=4",      LLUboRingBuffer::kInitialSizeMB,    4u);
        ensure_equals("kMaxSizeMB=16",         LLUboRingBuffer::kMaxSizeMB,        16u);
        ensure_equals("kFramesInFlight=3",     LLUboRingBuffer::kFramesInFlight,   3u);
        ensure_equals("kDefaultAlignment=256", LLUboRingBuffer::kDefaultAlignment, 256u);
        ensure_equals("default initial MB",    rb.getInitialSizeMB(), 4u);
        ensure_equals("default max MB",        rb.getMaxSizeMB(),     16u);
        ensure_equals("default alignment",     rb.getAlignment(),     256u);
        ensure_equals("0 current MB pre-init", rb.getCurrentSizeMB(), 0u);
        ensure("not initialized",              !rb.isInitialized());
        ensure_equals("0 allocator calls",     static_cast<std::uint32_t>(allocated_sizes.size()), 0u);
    }

    // test<2>: initialize → 4 MB buffer alloc + state 反映
    template<> template<>
    void ubo_ring_buffer_object::test<2>()
    {
        LLUboRingBuffer rb(makeIncrementingAllocator(), makeRecordingDestroyer());
        ensure("initialize success",         rb.initialize());
        ensure("initialized flag",           rb.isInitialized());
        ensure_equals("current MB = 4",      rb.getCurrentSizeMB(),    4u);
        ensure_equals("current bytes = 4MB", rb.getCurrentSizeBytes(), 4u * 1024u * 1024u);
        ensure_equals("chunk = 4MB/3",       rb.getChunkSizeBytes(),   (4u * 1024u * 1024u) / 3u);
        ensure_equals("frame index = 0",     rb.getFrameIndex(),       0u);
        ensure_equals("active chunk = 0",    rb.getActiveChunk(),      0u);
        ensure_equals("chunks used = 1",     rb.getChunksUsedThisFrame(), 1u);
        ensure_equals("1 allocator call",    static_cast<std::uint32_t>(allocated_sizes.size()), 1u);
        ensure_equals("allocator size 4MB",  allocated_sizes[0], 4u * 1024u * 1024u);
        ensure("not at max",                 !rb.isAtMaxSize());
    }

    // test<3>: 小サイズ alloc + alignment 反映 (= default 256 align)
    template<> template<>
    void ubo_ring_buffer_object::test<3>()
    {
        LLUboRingBuffer rb(makeIncrementingAllocator(), makeRecordingDestroyer());
        rb.initialize();
        auto a = rb.allocate(64);
        ensure("alloc 1 success",           a.success);
        ensure("no grow",                   !a.grew);
        ensure_equals("a offset = 0",       a.offset, 0u);
        ensure_equals("a size aligned 256", a.size,   256u);
        ensure("buffer non-zero",           a.buffer != 0);
        auto b = rb.allocate(100);
        ensure("alloc 2 success",            b.success);
        ensure_equals("b offset = 256",      b.offset, 256u);
        ensure_equals("b size aligned 256",  b.size,   256u);
        ensure_equals("chunk bytes used 512",rb.getChunkBytesUsed(), 512u);
    }

    // test<4>: 非 default alignment (= 64) 反映
    template<> template<>
    void ubo_ring_buffer_object::test<4>()
    {
        LLUboRingBuffer rb(makeIncrementingAllocator(), makeRecordingDestroyer(), 1, 4, 64);
        ensure_equals("alignment = 64",  rb.getAlignment(), 64u);
        rb.initialize();
        auto a = rb.allocate(1);
        ensure_equals("a aligned 64",    a.size, 64u);
        ensure_equals("a offset 0",      a.offset, 0u);
        auto b = rb.allocate(65);
        ensure_equals("b aligned 128",   b.size, 128u);
        ensure_equals("b offset 64",     b.offset, 64u);
        auto c = rb.allocate(64);
        ensure_equals("c aligned 64",    c.size,   64u);
        ensure_equals("c offset 64+128", c.offset, 192u);
    }

    // test<5>: chunk overflow → next chunk へ safe wrap (chunks_used < kFramesInFlight - 1、grow せず)
    template<> template<>
    void ubo_ring_buffer_object::test<5>()
    {
        // initial 3 MB = chunk_size 1 MB ぴったり (= 256 align 整合)
        LLUboRingBuffer rb(makeIncrementingAllocator(), makeRecordingDestroyer(), 3, 9, 256);
        rb.initialize();
        std::uint32_t chunk_size = rb.getChunkSizeBytes();
        ensure_equals("chunk_size = 1 MB", chunk_size, 1u * 1024u * 1024u);

        // chunk 0 をぴったり埋める
        auto a = rb.allocate(chunk_size);
        ensure("a success",                  a.success);
        ensure("no grow",                    !a.grew);
        ensure_equals("a offset 0",          a.offset, 0u);
        ensure_equals("active chunk 0",      rb.getActiveChunk(), 0u);
        ensure_equals("chunks used 1",       rb.getChunksUsedThisFrame(), 1u);

        // 次 alloc は chunk 1 へ wrap (= chunks_used が 1→2 で safe)
        auto b = rb.allocate(256);
        ensure("b success",                            b.success);
        ensure("no grow on safe wrap",                 !b.grew);
        ensure_equals("active chunk 1",                rb.getActiveChunk(), 1u);
        ensure_equals("chunks used 2",                 rb.getChunksUsedThisFrame(), 2u);
        ensure_equals("b offset = 1 chunk",            b.offset, chunk_size);
        ensure_equals("still 1 allocator call (no grow)",
                      static_cast<std::uint32_t>(allocated_sizes.size()), 1u);
        ensure_equals("still 3 MB",                    rb.getCurrentSizeMB(), 3u);
    }

    // test<6>: chunks_used が kFramesInFlight に到達想定で grow 発火 (= 3 → 6 MB)
    template<> template<>
    void ubo_ring_buffer_object::test<6>()
    {
        LLUboRingBuffer rb(makeIncrementingAllocator(), makeRecordingDestroyer(), 3, 12, 256);
        rb.initialize();
        std::uint32_t chunk_size = rb.getChunkSizeBytes();  // 1 MB
        // chunk 0 / chunk 1 を順次埋める (= safe wrap 1 回)
        rb.allocate(chunk_size);
        rb.allocate(chunk_size);
        ensure_equals("chunks used = 2 pre-grow",       rb.getChunksUsedThisFrame(), 2u);
        ensure_equals("1 allocator call pre-grow",      static_cast<std::uint32_t>(allocated_sizes.size()), 1u);

        // 次 alloc は chunk 2 = N-1 hazard ⇒ grow trigger
        auto c = rb.allocate(256);
        ensure("c success after grow",                  c.success);
        ensure("c grew flag",                           c.grew);
        ensure_equals("now 6 MB",                       rb.getCurrentSizeMB(), 6u);
        ensure_equals("2 allocator calls",              static_cast<std::uint32_t>(allocated_sizes.size()), 2u);
        ensure_equals("2nd allocator size = 6MB",       allocated_sizes[1], 6u * 1024u * 1024u);
        ensure_equals("1 destroyer call (old buffer)",  static_cast<std::uint32_t>(destroyed.size()), 1u);
        // grow 後 state reset = active chunk frame_index%3 = 0、used = 256 (= c の alloc 量)
        ensure_equals("active chunk reset 0",           rb.getActiveChunk(), 0u);
        ensure_equals("chunks used reset 1",            rb.getChunksUsedThisFrame(), 1u);
        ensure_equals("c offset 0",                     c.offset, 0u);
        ensure_equals("chunk_bytes_used = 256",         rb.getChunkBytesUsed(), 256u);
    }

    // test<7>: 多重 grow chain (= 3 → 6 → 12 MB、cap at 12)
    template<> template<>
    void ubo_ring_buffer_object::test<7>()
    {
        LLUboRingBuffer rb(makeIncrementingAllocator(), makeRecordingDestroyer(), 3, 12, 256);
        rb.initialize();

        // 3 → 6 grow
        std::uint32_t cs = rb.getChunkSizeBytes();  // 1 MB
        rb.allocate(cs);
        rb.allocate(cs);
        auto a = rb.allocate(256);
        ensure("a grew (3→6)",        a.grew);
        ensure_equals("now 6 MB",     rb.getCurrentSizeMB(), 6u);

        // 6 → 12 grow
        cs = rb.getChunkSizeBytes();  // 2 MB
        rb.allocate(cs);              // wrap chunk 0 (256 used) → chunk 1
        auto b = rb.allocate(cs);     // chunk 2 想定 = grow trigger
        ensure("b grew (6→12)",       b.grew);
        ensure_equals("now 12 MB",    rb.getCurrentSizeMB(), 12u);
        ensure("at max (= 12 MB)",    rb.isAtMaxSize());

        // 12 MB cap = もう grow できない
        ensure_equals("3 allocator calls total",  static_cast<std::uint32_t>(allocated_sizes.size()), 3u);
        ensure_equals("3rd alloc size = 12 MB",   allocated_sizes[2], 12u * 1024u * 1024u);
        ensure_equals("2 destroyer calls",        static_cast<std::uint32_t>(destroyed.size()), 2u);
    }

    // test<8>: cap at max enforcement (= grow 不可で alloc fail、buffer state 不変)
    template<> template<>
    void ubo_ring_buffer_object::test<8>()
    {
        LLUboRingBuffer rb(makeIncrementingAllocator(), makeRecordingDestroyer(), 3, 3, 256);
        rb.initialize();
        ensure("at max immediately",           rb.isAtMaxSize());
        std::uint32_t cs = rb.getChunkSizeBytes();  // 1 MB

        rb.allocate(cs);  // chunk 0
        rb.allocate(cs);  // wrap → chunk 1
        // 3 chunk 目要求 = grow 必須だが max 到達 → fail
        auto fail = rb.allocate(256);
        ensure("alloc fail at cap",            !fail.success);
        ensure("fail.grew false",              !fail.grew);
        ensure_equals("still 3 MB",            rb.getCurrentSizeMB(), 3u);
        ensure_equals("only 1 allocator call", static_cast<std::uint32_t>(allocated_sizes.size()), 1u);
        ensure_equals("no destroyer call",     static_cast<std::uint32_t>(destroyed.size()), 0u);
    }

    // test<9>: allocator failure (= 0 handle) → initialize fail + alloc fail when not initialized
    template<> template<>
    void ubo_ring_buffer_object::test<9>()
    {
        auto failing_allocator = [](std::uint32_t) -> LLUboRingBuffer::BufferHandle { return 0; };
        auto noop_destroyer    = [](LLUboRingBuffer::BufferHandle) {};
        LLUboRingBuffer rb(failing_allocator, noop_destroyer);
        ensure("initialize fail on allocator==0", !rb.initialize());
        ensure("not initialized",                 !rb.isInitialized());
        ensure_equals("0 buffer",                 rb.getBuffer(),
                      static_cast<LLUboRingBuffer::BufferHandle>(0));
        ensure_equals("0 current size",           rb.getCurrentSizeMB(), 0u);
        auto a = rb.allocate(64);
        ensure("alloc fail when uninitialized",   !a.success);
    }

    // test<10>: shutdown → buffer destroy + state reset + 再 initialize 可
    template<> template<>
    void ubo_ring_buffer_object::test<10>()
    {
        LLUboRingBuffer rb(makeIncrementingAllocator(), makeRecordingDestroyer(), 1, 4, 64);
        ensure_equals("custom initial 1",  rb.getInitialSizeMB(), 1u);
        ensure_equals("custom max 4",      rb.getMaxSizeMB(),     4u);
        ensure_equals("custom alignment",  rb.getAlignment(),     64u);
        rb.initialize();
        ensure_equals("current 1 MB",      rb.getCurrentSizeMB(), 1u);
        rb.allocate(50);
        ensure_equals("1 destroyed pre-shutdown = 0",  static_cast<std::uint32_t>(destroyed.size()), 0u);
        rb.shutdown();
        ensure("not init after shutdown",  !rb.isInitialized());
        ensure_equals("0 current",         rb.getCurrentSizeMB(), 0u);
        ensure_equals("1 destroyer call",  static_cast<std::uint32_t>(destroyed.size()), 1u);
        // 再 initialize 可
        ensure("re-init success",          rb.initialize());
        ensure_equals("re-init 1 MB",      rb.getCurrentSizeMB(), 1u);
        ensure_equals("2 allocator calls", static_cast<std::uint32_t>(allocated_sizes.size()), 2u);
    }

    // test<11>: beginFrame() frame index advance + per-frame chunk 状態 reset
    template<> template<>
    void ubo_ring_buffer_object::test<11>()
    {
        LLUboRingBuffer rb(makeIncrementingAllocator(), makeRecordingDestroyer(), 3, 9, 256);
        rb.initialize();
        ensure_equals("frame index 0 init",  rb.getFrameIndex(),          0u);
        ensure_equals("active chunk 0 init", rb.getActiveChunk(),         0u);

        rb.allocate(256);
        ensure_equals("chunk bytes 256",     rb.getChunkBytesUsed(),      256u);

        rb.beginFrame();
        ensure_equals("frame index 1",       rb.getFrameIndex(),          1u);
        ensure_equals("active chunk 1",      rb.getActiveChunk(),         1u);
        ensure_equals("chunk bytes reset 0", rb.getChunkBytesUsed(),      0u);
        ensure_equals("chunks used reset 1", rb.getChunksUsedThisFrame(), 1u);

        auto a = rb.allocate(256);
        ensure_equals("a offset = chunk 1 start", a.offset, rb.getChunkSizeBytes());

        rb.beginFrame();
        ensure_equals("frame index 2",        rb.getFrameIndex(),  2u);
        ensure_equals("active chunk 2",       rb.getActiveChunk(), 2u);

        rb.beginFrame();
        ensure_equals("frame index 3",        rb.getFrameIndex(),  3u);
        ensure_equals("active chunk wrap 0",  rb.getActiveChunk(), 0u);
    }
}
