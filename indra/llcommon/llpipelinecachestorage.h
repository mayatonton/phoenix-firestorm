/**
 * @file llpipelinecachestorage.h
 * @brief AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-5:
 *        VkPipelineCache blob disk persist 機構 + 64 MB 上限 enforcement
 *        (= PSC 持越項目)
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
 *   §9.3 PSO cache 戦略:
 *     - pipeline 1 layout 共有 = 全 PSO が同一 layout = cache hit 率最大
 *     - pCachedData を起動時 disk から load
 *       (= ~/.ayastorm_x64/cache/pipeline_cache.bin)、終了時 save
 *   §12 (PSC) 確定値:
 *     - disk persist path = ~/.ayastorm_x64/cache/pipeline_cache.bin
 *     - 上限 64 MB
 *   既存配線 (= llvkloader.cpp):
 *     - sPipelineCache (line 64) + createPipelineCache() (line 756) は
 *       VkPipelineCacheCreateInfo{} (pInitialData = nullptr) で in-memory only
 *     - disk persist 未実装 = 本 PC-5 で blob 管理 + 64 MB 上限 enforcement の
 *       algorithm 層を追加
 *
 * 本 class は Vulkan device 非依存の bookkeeping algorithm = 実 disk I/O は
 * caller injected callback (FileReader / FileWriter) 経由。これにより file
 * system 未準備な unittest 環境でも blob 管理 + cap enforcement algorithm
 * 単独検証可能 (= PC-3 (α) / PC-4 (α') パターン踏襲 = llcommon 単体配置 +
 * llcommon 既設 LL_ADD_INTEGRATION_TEST framework 経由 unittest)。
 *
 * 64 MB 上限 enforcement 戦略 = (e1) AYA 確定 2026-06-04:
 *   - load 時 (initialize): file size > 64 MB なら blob 破棄 = 0 cache から
 *     再生成 (= stale 大 blob 排除)
 *   - persist 時 (persistToDisk): blob size > 64 MB なら writer 不呼出 +
 *     false return = caller (PC-6 wire up timing) が skip / delete / truncate
 *     戦略を決定
 *   - algorithm 層は size 関連 bool query 提供のみ
 *     (= isWithinLimit / getBlobSize / getMaxSizeBytes)
 *
 * 実 vkGetPipelineCacheData (= sPipelineCache → blob 取出) + 起動時 file load
 * → VkPipelineCacheCreateInfo.pInitialData 投入 / shutdown 時 save の wire up
 * は PC-6 (= 5 cadence 全経路 update site 実装) で llrender / llvkloader 側
 * で実装予定。本 PC-5 段は algorithm + unittest + settings.xml cvar 露出のみ
 * (= AYA 採用 (α'') 2026-06-04)。
 */

#ifndef LL_LLPIPELINECACHESTORAGE_H
#define LL_LLPIPELINECACHESTORAGE_H

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class LLPipelineCacheStorage
{
public:
    using CacheBlob  = std::vector<std::uint8_t>;
    using FileReader = std::function<bool(const std::string& /*path*/, CacheBlob& /*out*/)>;
    using FileWriter = std::function<bool(const std::string& /*path*/, const CacheBlob& /*data*/)>;

    // design 07 §12 (PSC) 確定値
    static constexpr std::uint32_t kDefaultMaxSizeMB = 64;

    LLPipelineCacheStorage(FileReader    reader,
                           FileWriter    writer,
                           std::string   file_path,
                           std::uint32_t max_size_mb = kDefaultMaxSizeMB);
    ~LLPipelineCacheStorage();

    LLPipelineCacheStorage(const LLPipelineCacheStorage&)            = delete;
    LLPipelineCacheStorage& operator=(const LLPipelineCacheStorage&) = delete;

    // file load if exists、size > max なら blob 破棄 (= (e1) load reject)。
    // reader 無効時のみ false return。file 不在 / read 失敗は許容
    // (= empty blob で start、起動初回想定で initialize true return)。
    bool initialize();

    // blob clear + state reset。persistToDisk 呼出は explicit
    // (= shutdown 内では自動 persist しない、caller の責任)。
    void shutdown();

    // caller (PC-6 wire up) が vkGetPipelineCacheData 結果を渡す。
    // 無条件で mCacheBlob 置換。size 制限超過は isWithinLimit() で別途確認、
    // 本メソッドは store 自体は失敗しない (= true return)。
    bool updateBlob(CacheBlob new_blob);

    // disk persist 実行。size > max なら writer 不呼出 + false return
    // (= (e1) persist 時 size check で caller 判断)。size OK なら
    // mWriter 呼出 + その結果 return。
    bool persistToDisk();

    const CacheBlob&   getBlob()         const noexcept { return mCacheBlob; }
    std::size_t        getBlobSize()     const noexcept { return mCacheBlob.size(); }
    std::uint32_t      getBlobSizeMB()   const noexcept;
    std::size_t        getMaxSizeBytes() const noexcept;
    std::uint32_t      getMaxSizeMB()    const noexcept { return mMaxSizeMB; }
    const std::string& getFilePath()     const noexcept { return mFilePath; }
    bool               isWithinLimit()   const noexcept;
    bool               isInitialized()   const noexcept { return mInitialized; }

private:
    FileReader    mReader;
    FileWriter    mWriter;
    std::string   mFilePath;
    std::uint32_t mMaxSizeMB;
    CacheBlob     mCacheBlob;
    bool          mInitialized = false;
};

#endif // LL_LLPIPELINECACHESTORAGE_H
