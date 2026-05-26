/**
 * @file llreflectionocclusionworker.h
 * @brief AYAstorm CPU perf 章 案 O: Reflection probe occlusion culling を main thread から worker thread に剥がす。
 *
 * spec: docs/specs/ayastorm-cpu-perf/phase2-case-O-plan.md §4 (Worker A pool infra)
 *
 * 設計: 1 frame lag 非同期式。
 *   - frame N: main が snapshot 作成 → enqueue → worker が CPU 計算開始
 *   - frame N+1: main が wait_get_result で前 frame 結果取得 → GL apply
 * 初回 frame は wait_get_result が false を返し、apply されない (occlusion 効かない 1 frame)。
 *
 * GL 制約: glGenQueries / glBeginQuery / glEndQuery / glGetQueryObjectuiv は main thread 専有。
 *   worker は per-probe CPU 計算 (dist / inside-radius / query==0 check) のみ実施し、
 *   action descriptor (kind enum) を main に返す。main が action 種別に応じて GL を発行。
 */

#pragma once

#include "llvector4a.h"
#include "stdtypes.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

class LLReflectionMap;

// per-probe snapshot input (main → worker)
// NOTE: スカラ格納 (LLVector4a 直接埋め込み禁止)。LLVector4a は alignas(16) で std::vector の
// 既定 allocator が確実な 16B 整列を保証しないため、SSE intrinsic が SIGSEGV する事象を r40 案 O
// 検証時に観測。compute_probe_action 内で load3 して局所 LLVector4a に組み直す。
//
// NOTE: mProbe は raw pointer (非所有)。worker thread は触らず、main thread apply 時に
// mProbes 在籍 validation 経由でのみ deref する。LLPointer (所有) に戻すと probe lifetime を
// 1 frame 延長し、その間に mGroup (raw LLSpatialGroup*) が freed → autoAdjustOrigin で
// dangling deref → SIGSEGV (r40 案 O 検証時に観測)。
struct ProbeOcclusionSnapshotEntry
{
    LLReflectionMap* mProbe;  // non-owning; validate alive in apply via mProbes set
    F32 mOriginXYZ[3];
    F32 mRadius;
    U32 mOcclusionQuery;  // GLuint (current value at snapshot time)
};

struct ProbeOcclusionSnapshot
{
    std::vector<ProbeOcclusionSnapshotEntry> mEntries;
    F32 mEyeXYZ[3];
    U32 mFrameSeq;
};

// worker → main の per-probe action descriptor
enum class ProbeOcclusionActionKind
{
    SKIP_INSIDE_RADIUS,    // eye 内側、main は mOccluded=false 設定のみ
    NEED_GEN_QUERY,        // mOcclusionQuery==0、main は glGenQueries → push query
    POLL_AND_MAYBE_PUSH,   // mOcclusionQuery!=0、main は poll available → 結果次第で read result + push
};

struct ProbeOcclusionAction
{
    LLReflectionMap* mProbe;  // non-owning; main thread must validate via mProbes set before deref
    ProbeOcclusionActionKind mKind;
};

struct ProbeOcclusionResult
{
    std::vector<ProbeOcclusionAction> mActions;
    U32 mFrameSeq;
};

// 専属 1 thread の worker pool。manager 1 つにつき 1 instance (reflection / hero で別 instance)。
class LLReflectionOcclusionWorker
{
public:
    LLReflectionOcclusionWorker();
    ~LLReflectionOcclusionWorker();

    // main thread から呼出。snapshot を move-in、worker thread を kick。
    // 直前の enqueue 結果がまだ取られていない場合 (=異常) は overwrite。
    void enqueue(ProbeOcclusionSnapshot&& snap);

    // main thread から呼出。worker thread の compute 完了を待ち、結果を move-out。
    // 初回 frame (まだ enqueue 1 度もされていない) では false を返し、out は空のまま。
    // それ以降は必ず true を返し out に結果が入る (worker 計算がまだなら短時間 wait)。
    bool wait_get_result(ProbeOcclusionResult& out);

private:
    void thread_main();

    std::thread mThread;
    std::mutex  mMutex;
    std::condition_variable mCvIn;
    std::condition_variable mCvOut;
    bool mInputReady = false;
    bool mOutputReady = false;
    bool mStop = false;
    bool mFirstFrame = true;  // true まで wait_get_result は no-op
    ProbeOcclusionSnapshot mInput;
    ProbeOcclusionResult   mOutput;
};
