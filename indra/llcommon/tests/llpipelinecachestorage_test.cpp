/**
 * @file llpipelinecachestorage_test.cpp
 * @brief LLPipelineCacheStorage disk persist + 64 MB cap algorithm tut tests
 *        (r41 Phase 1.C PC-5、(PSC) 持越項目)
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
 * 検証対象 (= design/07-vulkan-api-state.md §9.3 + §12 PSC):
 *   - default 値 (= 64 MB cap)
 *   - initialize で file load + size > max 時 (e1) load reject
 *   - updateBlob で blob 置換
 *   - persistToDisk で size 関連 bool query (= (e1) cap 超過 false return)
 *   - shutdown + 再 initialize round-trip
 *   - reader / writer null edge case + ctor 0 fallback
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llpipelinecachestorage.h"

#include <map>
#include <utility>

namespace tut
{
    struct pipeline_cache_data
    {
        std::map<std::string, std::vector<std::uint8_t>>      files;
        std::vector<std::pair<std::string, std::size_t>>      write_log;
        std::vector<std::pair<std::string, std::size_t>>      read_log;
        bool                                                  reader_force_fail = false;
        bool                                                  writer_force_fail = false;

        LLPipelineCacheStorage::FileReader makeReader()
        {
            return [this](const std::string& p, std::vector<std::uint8_t>& out) -> bool
            {
                if (reader_force_fail)
                {
                    return false;
                }
                auto it = files.find(p);
                if (it == files.end())
                {
                    read_log.emplace_back(p, 0);
                    return false;
                }
                out = it->second;
                read_log.emplace_back(p, out.size());
                return true;
            };
        }
        LLPipelineCacheStorage::FileWriter makeWriter()
        {
            return [this](const std::string& p, const std::vector<std::uint8_t>& data) -> bool
            {
                if (writer_force_fail)
                {
                    return false;
                }
                files[p] = data;
                write_log.emplace_back(p, data.size());
                return true;
            };
        }
    };

    typedef test_group<pipeline_cache_data> pipeline_cache_group;
    typedef pipeline_cache_group::object    pipeline_cache_object;
    tut::pipeline_cache_group g_pipeline_cache_group("LLPipelineCacheStorage");

    // test<1>: default 値検証 (= design 07 §12 (PSC) 確定値 64 MB cap)
    template<> template<>
    void pipeline_cache_object::test<1>()
    {
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "test.bin");
        ensure_equals("kDefaultMaxSizeMB=64",  LLPipelineCacheStorage::kDefaultMaxSizeMB, 64u);
        ensure_equals("default max MB",        s.getMaxSizeMB(), 64u);
        ensure_equals("default max bytes",     s.getMaxSizeBytes(),
                      static_cast<std::size_t>(64u) * 1024u * 1024u);
        ensure_equals("blob empty pre-init",   s.getBlobSize(),   static_cast<std::size_t>(0u));
        ensure_equals("blob MB 0 pre-init",    s.getBlobSizeMB(), 0u);
        ensure("not initialized",              !s.isInitialized());
        ensure("within limit (empty)",         s.isWithinLimit());
        ensure_equals("file path stored",      s.getFilePath(), std::string("test.bin"));
        ensure_equals("0 reader call",         static_cast<std::size_t>(read_log.size()),  static_cast<std::size_t>(0));
        ensure_equals("0 writer call",         static_cast<std::size_t>(write_log.size()), static_cast<std::size_t>(0));
    }

    // test<2>: initialize で file 不在 → empty blob + initialized + reader 1 回呼出
    template<> template<>
    void pipeline_cache_object::test<2>()
    {
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "missing.bin");
        ensure("initialize success (file 不在許容)", s.initialize());
        ensure("initialized",                  s.isInitialized());
        ensure_equals("blob empty",            s.getBlobSize(), static_cast<std::size_t>(0u));
        ensure_equals("1 reader call",         static_cast<std::size_t>(read_log.size()), static_cast<std::size_t>(1));
        ensure_equals("reader path",           read_log[0].first, std::string("missing.bin"));
    }

    // test<3>: initialize で file 内容 load 成功
    template<> template<>
    void pipeline_cache_object::test<3>()
    {
        std::vector<std::uint8_t> data(1024, 0xABu);
        files["cache.bin"] = data;
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "cache.bin");
        ensure("initialize success",            s.initialize());
        ensure_equals("blob size 1024",         s.getBlobSize(), static_cast<std::size_t>(1024));
        ensure_equals("blob content[0]",        s.getBlob()[0],  static_cast<std::uint8_t>(0xABu));
        ensure_equals("blob content[1023]",     s.getBlob()[1023], static_cast<std::uint8_t>(0xABu));
        ensure("within limit",                  s.isWithinLimit());
    }

    // test<4>: initialize で file size > max → blob 破棄 (= (e1) load reject)
    template<> template<>
    void pipeline_cache_object::test<4>()
    {
        // max_size_mb = 1 で 2 MB blob を load させる
        std::vector<std::uint8_t> oversize(2u * 1024u * 1024u, 0x55u);
        files["big.bin"] = oversize;
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "big.bin", 1u);
        ensure_equals("max 1 MB",              s.getMaxSizeMB(), 1u);
        ensure("initialize success",            s.initialize());
        ensure("initialized",                   s.isInitialized());
        ensure_equals("blob 破棄 (load reject)", s.getBlobSize(), static_cast<std::size_t>(0u));
        ensure("within limit (empty)",          s.isWithinLimit());
    }

    // test<5>: updateBlob で blob 置換
    template<> template<>
    void pipeline_cache_object::test<5>()
    {
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "x.bin");
        s.initialize();
        std::vector<std::uint8_t> new_blob(512, 0xCDu);
        ensure("updateBlob success",            s.updateBlob(new_blob));
        ensure_equals("blob size 512",          s.getBlobSize(), static_cast<std::size_t>(512));
        ensure_equals("blob[0]",                s.getBlob()[0], static_cast<std::uint8_t>(0xCDu));
    }

    // test<6>: isWithinLimit = size ≤ max → true
    template<> template<>
    void pipeline_cache_object::test<6>()
    {
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "x.bin", 1u);
        s.initialize();
        // ぴったり 1 MB
        std::vector<std::uint8_t> blob(1u * 1024u * 1024u, 0x00u);
        s.updateBlob(blob);
        ensure("size == max → within",          s.isWithinLimit());
        ensure_equals("blob MB 1",              s.getBlobSizeMB(), 1u);
    }

    // test<7>: updateBlob 巨大 blob → isWithinLimit false
    template<> template<>
    void pipeline_cache_object::test<7>()
    {
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "x.bin", 1u);
        s.initialize();
        // max + 1 byte
        std::vector<std::uint8_t> blob(1u * 1024u * 1024u + 1u, 0x00u);
        s.updateBlob(blob);
        ensure("over limit → not within",       !s.isWithinLimit());
    }

    // test<8>: persistToDisk size OK → writer 呼出 + true return
    template<> template<>
    void pipeline_cache_object::test<8>()
    {
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "out.bin");
        s.initialize();
        std::vector<std::uint8_t> blob(256, 0xEFu);
        s.updateBlob(blob);
        ensure("persist success",               s.persistToDisk());
        ensure_equals("1 writer call",          static_cast<std::size_t>(write_log.size()), static_cast<std::size_t>(1));
        ensure_equals("writer path",            write_log[0].first, std::string("out.bin"));
        ensure_equals("writer size 256",        write_log[0].second, static_cast<std::size_t>(256));
        ensure_equals("file 反映",               files["out.bin"].size(), static_cast<std::size_t>(256));
    }

    // test<9>: persistToDisk size > max → writer 不呼出 + false return ((e1) cap skip)
    template<> template<>
    void pipeline_cache_object::test<9>()
    {
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "out.bin", 1u);
        s.initialize();
        std::vector<std::uint8_t> oversize(1u * 1024u * 1024u + 1u, 0xFFu);
        s.updateBlob(oversize);
        ensure("persist false at cap exceed",   !s.persistToDisk());
        ensure_equals("0 writer call",          static_cast<std::size_t>(write_log.size()), static_cast<std::size_t>(0));
        ensure("file 不変",                      files.find("out.bin") == files.end());
    }

    // test<10>: persistToDisk writer fail → false return
    template<> template<>
    void pipeline_cache_object::test<10>()
    {
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "out.bin");
        s.initialize();
        s.updateBlob(std::vector<std::uint8_t>(64, 0x11u));
        writer_force_fail = true;
        ensure("persist false on writer fail",  !s.persistToDisk());
    }

    // test<11>: shutdown 後 再 initialize OK
    template<> template<>
    void pipeline_cache_object::test<11>()
    {
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "x.bin");
        s.initialize();
        s.updateBlob(std::vector<std::uint8_t>(128, 0x22u));
        ensure_equals("blob 128 pre-shutdown",  s.getBlobSize(), static_cast<std::size_t>(128));
        s.shutdown();
        ensure("not init after shutdown",       !s.isInitialized());
        ensure_equals("blob 0 after shutdown",  s.getBlobSize(), static_cast<std::size_t>(0));
        ensure("re-init success",               s.initialize());
        ensure("re-initialized",                s.isInitialized());
    }

    // test<12>: reader nullptr → initialize false + 未 init 維持
    template<> template<>
    void pipeline_cache_object::test<12>()
    {
        LLPipelineCacheStorage s(LLPipelineCacheStorage::FileReader{}, makeWriter(), "x.bin");
        ensure("initialize false on reader null", !s.initialize());
        ensure("still not initialized",           !s.isInitialized());
        // persistToDisk も初期化失敗時 false
        ensure("persist false when uninit",       !s.persistToDisk());
    }

    // test<13>: ctor max_size_mb = 0 → kDefaultMaxSizeMB 64 fallback
    template<> template<>
    void pipeline_cache_object::test<13>()
    {
        LLPipelineCacheStorage s(makeReader(), makeWriter(), "x.bin", 0u);
        ensure_equals("max fallback 64 MB",      s.getMaxSizeMB(), 64u);
        ensure_equals("max bytes fallback",      s.getMaxSizeBytes(),
                      static_cast<std::size_t>(64u) * 1024u * 1024u);
    }
}
