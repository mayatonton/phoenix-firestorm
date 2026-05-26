/**
 * @file llreflectionocclusionworker.cpp
 * @brief AYAstorm CPU perf 章 案 O: Worker A pool 実装。
 *
 * spec: docs/specs/ayastorm-cpu-perf/phase2-case-O-plan.md §2.1 Pass 1 (CPU on worker)
 */

#include "llviewerprecompiledheaders.h"

#include "llreflectionocclusionworker.h"

#include "llmath.h"
#include "llreflectionmap.h"
#include "llayastormperflog.h"

namespace
{
    // Pass 1 (CPU on worker): per-probe action 決定
    // スカラを load3 して局所 LLVector4a に組み直す (snapshot 内に LLVector4a を持たないため)
    ProbeOcclusionActionKind compute_probe_action(const ProbeOcclusionSnapshotEntry& e, const LLVector4a& eye)
    {
        // mRadius * F_SQRT3 + 1 padding (元 llreflectionmap.cpp:358 と同式)
        F32 dist = e.mRadius * F_SQRT3 + 1.f;
        LLVector4a origin;
        origin.load3(e.mOriginXYZ);
        LLVector4a o;
        o.setSub(origin, eye);
        if (o.getLength3().getF32() < dist)
        {
            return ProbeOcclusionActionKind::SKIP_INSIDE_RADIUS;
        }
        if (e.mOcclusionQuery == 0)
        {
            return ProbeOcclusionActionKind::NEED_GEN_QUERY;
        }
        return ProbeOcclusionActionKind::POLL_AND_MAYBE_PUSH;
    }
}

LLReflectionOcclusionWorker::LLReflectionOcclusionWorker()
{
    mThread = std::thread([this]() { thread_main(); });
}

LLReflectionOcclusionWorker::~LLReflectionOcclusionWorker()
{
    {
        std::lock_guard<std::mutex> lk(mMutex);
        mStop = true;
    }
    mCvIn.notify_all();
    if (mThread.joinable())
    {
        mThread.join();
    }
}

void LLReflectionOcclusionWorker::enqueue(ProbeOcclusionSnapshot&& snap)
{
    {
        std::lock_guard<std::mutex> lk(mMutex);
        mInput = std::move(snap);
        mInputReady = true;
    }
    mCvIn.notify_one();
}

bool LLReflectionOcclusionWorker::wait_get_result(ProbeOcclusionResult& out)
{
    std::unique_lock<std::mutex> lk(mMutex);
    if (mFirstFrame)
    {
        // 初回 frame: 結果はまだ存在しない (enqueue 直前)。occlusion は本 frame skip。
        mFirstFrame = false;
        return false;
    }
    mCvOut.wait(lk, [this]() { return mOutputReady || mStop; });
    if (mStop)
    {
        return false;
    }
    out = std::move(mOutput);
    mOutputReady = false;
    return true;
}

void LLReflectionOcclusionWorker::thread_main()
{
    while (true)
    {
        ProbeOcclusionSnapshot snap;
        {
            std::unique_lock<std::mutex> lk(mMutex);
            mCvIn.wait(lk, [this]() { return mInputReady || mStop; });
            if (mStop)
            {
                return;
            }
            snap = std::move(mInput);
            mInputReady = false;
        }

        // Pass 1 compute (CPU only、GL 触らない)
        ProbeOcclusionResult result;
        result.mFrameSeq = snap.mFrameSeq;
        result.mActions.reserve(snap.mEntries.size());
        {
            AYAPERF_ZONE("worker_compute");
            LLVector4a eye;
            eye.load3(snap.mEyeXYZ);
            for (auto& e : snap.mEntries)
            {
                ProbeOcclusionAction a;
                a.mProbe = e.mProbe;
                a.mKind  = compute_probe_action(e, eye);
                result.mActions.push_back(a);
            }
        }

        {
            std::lock_guard<std::mutex> lk(mMutex);
            mOutput = std::move(result);
            mOutputReady = true;
        }
        mCvOut.notify_one();
    }
}
