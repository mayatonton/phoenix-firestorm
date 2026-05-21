/**
 * @file llayaudit.cpp
 * @brief Headless audit of Cinematic-floater cvars.
 * <FS:AYAstorm r30 P4>
 */

#include "llviewerprecompiledheaders.h"

#include "llayaudit.h"

#include "llappviewer.h"
#include "llcontrol.h"
#include "lleventtimer.h"
#include "llfile.h"
#include "llsdserialize.h"
#include "llsnapshotmodel.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

#include <cmath>
#include <cstring>

namespace
{
// Tick period: 50ms, ~20 ticks/sec. Wait counts are in ticks.
constexpr F32 TICK_PERIOD = 0.05f;
// Wait between cvar set and snapshot: ~15 ticks (~0.75s) to let shader
// recompiles, render-to-texture passes and reflection probes settle.
constexpr int SETTLE_TICKS = 15;
// Initial warmup after login state STARTED: give the environment, region,
// textures, mesh, attached avatars and reflection probes time to rez. 10s was
// insufficient — large-region content streaming was still landing in the middle
// of baseline calibration, producing whole-frame "flicker" outliers that
// poisoned the noise floor. 40s lets streaming settle.
constexpr int WARMUP_TICKS = 800; // ~40s
// Noise calibration: take N baseline snapshots GAP ticks apart so the analyzer
// can compute the animation noise floor (water/leaves/clouds) and adjust its
// "WIRED vs DEAD" threshold above that floor. Use enough samples (9) that
// median-of-C(N,2) pairs is robust against 1-2 outlier frames.
constexpr int NOISE_SAMPLES   = 9;
constexpr int NOISE_GAP_TICKS = 20; // ~1s between baseline samples
// Per-sample snapshot repeats. The Cinematic pipeline carries a 1-frame
// exposure-feedback (mLastExposure) loop that persists even when
// RenderDynamicExposureEnabled=0 (only the response curve is gated). The
// effect is a multi-state per-frame flicker — back-to-back snapshots land in
// different stable rendering states, polluting both baseline and sweep diffs.
// Take N snapshots per sample with small gaps so the analyzer can pixel-wise
// median them into a denoised representative before diffing.
constexpr int SNAP_REPEAT_COUNT     = 3;
constexpr int SNAP_REPEAT_GAP_TICKS = 5; // ~0.25s between repeats
// Snapshot resolution.
constexpr S32 SNAP_W = 1280;
constexpr S32 SNAP_H = 720;
}

// Ticker bridges LLEventTimer into LLAYAudit::tick().
class LLAYAuditTicker : public LLEventTimer
{
public:
    LLAYAuditTicker() : LLEventTimer(TICK_PERIOD) {}
    bool tick() override
    {
        LLAYAudit::instance().tick();
        return false; // keep firing
    }
};

// Cvar list — generated from floater_aya_cinematic.xml.
// Booleans sweep 0/1; numeric sweep XML-declared min/max.
// `depends` names a master BOOL cvar that must be ON for this sub-cvar to be
// observable; the audit force-toggles it ON for the duration of the pair and
// restores it after.
const LLAYAudit::CvarSpec LLAYAudit::sSpecs[] = {
    {"RenderDeferred",                                       CV_BOOL, 0, 1,         nullptr},
    {"RenderDeferredSSAO",                                   CV_BOOL, 0, 1,         nullptr},
    {"RenderDeferredBlurLight",                              CV_BOOL, 0, 1,         nullptr},
    {"RenderGlow",                                           CV_BOOL, 0, 1,         nullptr},
    {"RenderAttachedParticles",                              CV_BOOL, 0, 1,         nullptr},
    {"RenderReflectionProbeLevel",                           CV_S32,  0, 3,         nullptr},
    {"RenderShadowDetail",                                   CV_S32,  0, 3,         nullptr},
    {"RenderShadowAutomaticDistance",                        CV_BOOL, 0, 1,         nullptr},
    {"RenderShadowResolutionScale",                          CV_F32,  0.5,  3.0,    nullptr},
    {"RenderShadowBlurSize",                                 CV_F32,  0.0,  4.0,    nullptr},
    {"RenderSSAOFactor",                                     CV_F32,  0.0,  2.0,    "RenderDeferredSSAO"},
    {"RenderSSAOMaxScale",                                   CV_U32,  0,    1000,   "RenderDeferredSSAO"},
    {"RenderSSAOScale",                                      CV_F32,  0.0,  2000.0, "RenderDeferredSSAO"},
    {"RenderGlowIterations",                                 CV_S32,  0,    8,      "RenderGlow"},
    {"RenderGlowMinLuminance",                               CV_F32,  0.0,  4.0,    "RenderGlow"},
    {"RenderGlowMaxExtractAlpha",                            CV_F32,  0.0,  1.0,    "RenderGlow"},
    {"RenderGlowStrength",                                   CV_F32,  0.0,  2.0,    "RenderGlow"},
    {"RenderGlowWarmthAmount",                               CV_F32,  0.0,  2.0,    "RenderGlow"},
    {"RenderGlowWidth",                                      CV_F32,  0.1,  5.0,    "RenderGlow"},
    {"RenderMotionBlur",                                     CV_BOOL, 0, 1,         nullptr},
    {"RenderMotionBlurOtherAvatars",                         CV_BOOL, 0, 1,         "RenderMotionBlur"},
    {"RenderMotionBlurSelfAvatar",                           CV_BOOL, 0, 1,         "RenderMotionBlur"},
    {"RenderMotionBlurStrength",                             CV_S32,  0, 128,       "RenderMotionBlur"},
    {"RenderDepthOfField",                                   CV_BOOL, 0, 1,         nullptr},
    {"RenderDepthOfFieldFront",                              CV_BOOL, 0, 1,         "RenderDepthOfField"},
    {"RenderDepthOfFieldHighQuality",                        CV_BOOL, 0, 1,         "RenderDepthOfField"},
    {"RenderDepthOfFieldChroma",                             CV_BOOL, 0, 1,         "RenderDepthOfField"},
    {"RenderChromaStrength",                                 CV_F32,  0.0,  100.0,  "RenderDepthOfFieldChroma"},
    {"CameraDoFResScale",                                    CV_F32,  0.1,  1.0,    "RenderDepthOfField"},
    {"CameraFNumber",                                        CV_F32,  1.0,  128.0,  "RenderDepthOfField"},
    {"CameraFieldOfView",                                    CV_F32,  10.0, 120.0,  "RenderDepthOfField"},
    {"CameraFocalLength",                                    CV_F32,  10.0, 300.0,  "RenderDepthOfField"},
    {"CameraFocusTransitionTime",                            CV_F32,  0.0,  5.0,    "RenderDepthOfField"},
    {"CameraMaxCoF",                                         CV_F32,  0.1,  50.0,   "RenderDepthOfField"},
    {"RenderFSAAType",                                       CV_U32,  0, 3,         nullptr},
    {"RenderCASSharpness",                                   CV_F32,  0.0,  1.0,    nullptr},
    {"RenderScreenSpaceReflections",                         CV_BOOL, 0, 1,         nullptr},
    {"RenderScreenSpaceReflectionAdaptiveStepMultiplier",    CV_F32,  0.1,  10.0,   "RenderScreenSpaceReflections"},
    {"RenderScreenSpaceReflectionDepthRejectBias",           CV_F32,  0.0,  1.0,    "RenderScreenSpaceReflections"},
    {"RenderScreenSpaceReflectionDistanceBias",              CV_F32,  0.0,  1.0,    "RenderScreenSpaceReflections"},
    {"RenderScreenSpaceReflectionGlossySamples",             CV_S32,  1, 64,        "RenderScreenSpaceReflections"},
    {"RenderScreenSpaceReflectionIterations",                CV_S32,  1, 200,       "RenderScreenSpaceReflections"},
    {"RenderScreenSpaceReflectionRayStep",                   CV_F32,  0.01, 10.0,   "RenderScreenSpaceReflections"},
    {"RenderVolumetricLighting",                             CV_BOOL, 0, 1,         nullptr},
    {"RenderVolumetricLightingDirectional",                  CV_BOOL, 0, 1,         "RenderVolumetricLighting"},
    {"RenderVolumetricLightingFalloffMultiplier",            CV_F32,  0.0,  10.0,   "RenderVolumetricLighting"},
    {"RenderVolumetricLightingMultiplier",                   CV_F32,  0.0,  200.0,  "RenderVolumetricLighting"},
    {"RenderVolumetricLightingResolution",                   CV_U32,  4, 64,        "RenderVolumetricLighting"},
};

const size_t LLAYAudit::sSpecCount = sizeof(sSpecs) / sizeof(sSpecs[0]);

LLAYAudit::LLAYAudit()
{
}

LLAYAudit::~LLAYAudit()
{
}

bool LLAYAudit::isEnabled()
{
    return gSavedSettings.getBOOL("AYAuditMode");
}

void LLAYAudit::onStartupDone()
{
    if (!isEnabled())
    {
        return;
    }
    if (mState != STATE_IDLE)
    {
        return;
    }

    mOutputDir = gSavedSettings.getString("AYAuditOutputDir");
    if (mOutputDir.empty())
    {
        mOutputDir = "/tmp/aya-audit";
    }
    ensureOutputDir();

    LL_INFOS("AYAudit") << "AYAudit starting. Output dir: " << mOutputDir
                        << ". Sweeping " << sSpecCount << " cvars." << LL_ENDL;

    mState       = STATE_WARMUP;
    mWaitFrames  = WARMUP_TICKS;
    mCvarIdx     = 0;
    mNoiseIdx    = 0;
    mManifest    = LLSD::emptyMap();
    mManifest["output_dir"]    = mOutputDir;
    mManifest["snap_width"]    = SNAP_W;
    mManifest["snap_height"]   = SNAP_H;
    mManifest["noise_samples"] = NOISE_SAMPLES;
    mManifest["snap_repeats"]  = SNAP_REPEAT_COUNT;
    mManifest["cvars"]         = LLSD::emptyArray();

    // Spawn ticker. It self-deletes when we stop returning false.
    // We instead let it keep firing — quit will tear the process down.
    if (!mTicker)
    {
        mTicker = new LLAYAuditTicker();
    }
}

void LLAYAudit::tick()
{
    if (mState == STATE_IDLE || mState == STATE_QUIT)
    {
        return;
    }

    if (mWaitFrames > 0)
    {
        --mWaitFrames;
        return;
    }

    switch (mState)
    {
    case STATE_WARMUP:
        logState("WARMUP done -> NOISE_CAL");
        mState    = STATE_NOISE_CAL;
        mNoiseIdx = 0;
        return;

    case STATE_NOISE_CAL:
        if (mNoiseIdx < NOISE_SAMPLES)
        {
            char tag = (char)('a' + mNoiseIdx);
            char rep = (char)('a' + mSnapRepeatIdx);
            std::string fname = llformat("00_baseline_%c_%c.png", tag, rep);
            takeSnapshot(fname);
            ++mSnapRepeatIdx;
            if (mSnapRepeatIdx < SNAP_REPEAT_COUNT)
            {
                mWaitFrames = SNAP_REPEAT_GAP_TICKS;
                return;
            }
            mSnapRepeatIdx = 0;
            ++mNoiseIdx;
            if (mNoiseIdx < NOISE_SAMPLES)
            {
                mWaitFrames = NOISE_GAP_TICKS;
                return;
            }
        }
        logState("NOISE_CAL done -> SWEEP");
        mCvarIdx = 0;
        mState   = STATE_SWEEP_LOW_SET;
        return;

    case STATE_SWEEP_LOW_SET:
        if (mCvarIdx >= sSpecCount)
        {
            mState = STATE_FINALIZE;
            return;
        }
        {
            const CvarSpec& spec = sSpecs[mCvarIdx];
            recordOriginal(spec);
            if (spec.depends)
            {
                forceMasterOn(spec.depends);
            }
            setCvar(spec, spec.low);
            mWaitFrames = SETTLE_TICKS;
            mState      = STATE_SWEEP_LOW_SNAP;
        }
        return;

    case STATE_SWEEP_LOW_SNAP:
        {
            const CvarSpec& spec = sSpecs[mCvarIdx];
            char rep = (char)('a' + mSnapRepeatIdx);
            std::string fname = llformat("%03zu_%s_low_%c.png", mCvarIdx + 1, spec.name, rep);
            takeSnapshot(fname);
            ++mSnapRepeatIdx;
            if (mSnapRepeatIdx < SNAP_REPEAT_COUNT)
            {
                mWaitFrames = SNAP_REPEAT_GAP_TICKS;
                return;
            }
            mSnapRepeatIdx = 0;
            mState = STATE_SWEEP_HIGH_SET;
        }
        return;

    case STATE_SWEEP_HIGH_SET:
        {
            const CvarSpec& spec = sSpecs[mCvarIdx];
            setCvar(spec, spec.high);
            mWaitFrames = SETTLE_TICKS;
            mState      = STATE_SWEEP_HIGH_SNAP;
        }
        return;

    case STATE_SWEEP_HIGH_SNAP:
        {
            const CvarSpec& spec = sSpecs[mCvarIdx];
            char rep = (char)('a' + mSnapRepeatIdx);
            std::string fname = llformat("%03zu_%s_high_%c.png", mCvarIdx + 1, spec.name, rep);
            takeSnapshot(fname);
            ++mSnapRepeatIdx;
            if (mSnapRepeatIdx < SNAP_REPEAT_COUNT)
            {
                mWaitFrames = SNAP_REPEAT_GAP_TICKS;
                return;
            }
            mSnapRepeatIdx = 0;

            LLSD entry;
            entry["cvar"] = spec.name;
            entry["low"]  = spec.low;
            entry["high"] = spec.high;
            entry["depends"]   = spec.depends ? spec.depends : "";
            entry["low_prefix"]  = llformat("%03zu_%s_low",  mCvarIdx + 1, spec.name);
            entry["high_prefix"] = llformat("%03zu_%s_high", mCvarIdx + 1, spec.name);
            entry["repeats"]     = SNAP_REPEAT_COUNT;
            mManifest["cvars"].append(entry);

            restoreOriginal(spec);
            if (spec.depends)
            {
                restoreMaster(spec.depends);
            }
            ++mCvarIdx;
            mState = STATE_SWEEP_LOW_SET;
        }
        return;

    case STATE_FINALIZE:
        writeManifest();
        LL_INFOS("AYAudit") << "AYAudit complete. Manifest written. Quitting." << LL_ENDL;
        mState      = STATE_QUIT;
        mWaitFrames = 20; // give LL_INFOS a moment to flush
        if (LLAppViewer::instance())
        {
            LLAppViewer::instance()->forceQuit();
        }
        return;

    case STATE_QUIT:
    case STATE_IDLE:
        return;
    }
}

void LLAYAudit::ensureOutputDir()
{
    if (mOutputDir.empty()) return;
    if (!LLFile::isdir(mOutputDir))
    {
        LLFile::mkdir(mOutputDir);
    }
}

bool LLAYAudit::takeSnapshot(const std::string& filename)
{
    if (!gViewerWindow)
    {
        LL_WARNS("AYAudit") << "No gViewerWindow; cannot snapshot " << filename << LL_ENDL;
        return false;
    }
    std::string fullpath = mOutputDir + "/" + filename;
    bool ok = gViewerWindow->saveSnapshot(fullpath, SNAP_W, SNAP_H,
                                          /*show_ui=*/false,
                                          /*show_hud=*/false,
                                          /*do_rebuild=*/false,
                                          /*show_balance=*/false,
                                          LLSnapshotModel::SNAPSHOT_TYPE_COLOR,
                                          LLSnapshotModel::SNAPSHOT_FORMAT_PNG);
    LL_INFOS("AYAudit") << (ok ? "[snap] " : "[snap FAIL] ") << fullpath << LL_ENDL;
    return ok;
}

void LLAYAudit::recordOriginal(const CvarSpec& spec)
{
    LLControlVariable* var = gSavedSettings.getControl(spec.name);
    if (!var)
    {
        LL_WARNS("AYAudit") << "Missing cvar: " << spec.name << LL_ENDL;
        return;
    }
    mOriginalValues[spec.name] = var->getValue();
}

void LLAYAudit::restoreOriginal(const CvarSpec& spec)
{
    auto it = mOriginalValues.find(spec.name);
    if (it == mOriginalValues.end()) return;
    LLControlVariable* var = gSavedSettings.getControl(spec.name);
    if (!var) return;
    var->setValue(it->second);
}

void LLAYAudit::forceMasterOn(const char* name)
{
    if (!name) return;
    LLControlVariable* var = gSavedSettings.getControl(name);
    if (!var)
    {
        LL_WARNS("AYAudit") << "Missing master cvar: " << name << LL_ENDL;
        return;
    }
    // Walk the dependency chain: if this master is itself a sub-cvar of
    // another master, force that one ON first. Required for nested gates
    // (e.g. RenderChromaStrength -> RenderDepthOfFieldChroma -> RenderDepthOfField).
    for (size_t i = 0; i < sSpecCount; ++i)
    {
        if (std::strcmp(sSpecs[i].name, name) == 0 && sSpecs[i].depends)
        {
            forceMasterOn(sSpecs[i].depends);
            break;
        }
    }
    // Already forced? leave alone.
    if (mForcedMasters.find(name) != mForcedMasters.end()) return;
    mForcedMasters[name] = var->getValue();
    gSavedSettings.setBOOL(name, true);
}

void LLAYAudit::restoreMaster(const char* name)
{
    if (!name) return;
    auto it = mForcedMasters.find(name);
    if (it == mForcedMasters.end()) return;
    LLControlVariable* var = gSavedSettings.getControl(name);
    if (var)
    {
        var->setValue(it->second);
    }
    mForcedMasters.erase(it);
}

void LLAYAudit::setCvar(const CvarSpec& spec, double value)
{
    switch (spec.type)
    {
    case CV_BOOL:
        gSavedSettings.setBOOL(spec.name, value > 0.5 ? true : false);
        break;
    case CV_S32:
        gSavedSettings.setS32(spec.name, (S32)llround(value));
        break;
    case CV_U32:
        gSavedSettings.setU32(spec.name, (U32)llround(value));
        break;
    case CV_F32:
        gSavedSettings.setF32(spec.name, (F32)value);
        break;
    }
}

void LLAYAudit::writeManifest()
{
    std::string fullpath = mOutputDir + "/manifest.llsd";
    llofstream os(fullpath.c_str());
    if (!os.is_open())
    {
        LL_WARNS("AYAudit") << "Cannot open manifest: " << fullpath << LL_ENDL;
        return;
    }
    LLSDSerialize::toPrettyXML(mManifest, os);
    os.close();
    LL_INFOS("AYAudit") << "Manifest: " << fullpath << LL_ENDL;
}

void LLAYAudit::logState(const char* label)
{
    LL_INFOS("AYAudit") << label << " (idx=" << mCvarIdx << "/" << sSpecCount << ")" << LL_ENDL;
}
