/**
 * @file llayaudit.h
 * @brief Headless audit of Cinematic-floater cvars.
 *
 * <FS:AYAstorm r30 P4>
 * Drives a state machine that sweeps every cvar bound in
 * floater_aya_cinematic.xml across a low / high value pair and saves
 * a snapshot after each setting. The resulting PNG set is consumed by
 * an external diff driver to determine which cvars actually affect
 * the rendered frame.
 */
#ifndef LL_AYAUDIT_H
#define LL_AYAUDIT_H

#include "llsingleton.h"
#include "llsd.h"
#include <map>
#include <string>

class LLAYAuditTicker;

class LLAYAudit : public LLSingleton<LLAYAudit>
{
    LLSINGLETON(LLAYAudit);
    ~LLAYAudit();

public:
    static bool isEnabled();

    void onStartupDone();
    void tick();

private:
    enum ECvarType { CV_BOOL, CV_S32, CV_U32, CV_F32 };

    struct CvarSpec
    {
        const char* name;
        ECvarType   type;
        double      low;
        double      high;
        const char* depends;  // master BOOL cvar to force ON during sweep; nullptr if none
    };

    enum EState
    {
        STATE_IDLE,
        STATE_WARMUP,
        STATE_NOISE_CAL,       // take N baseline snapshots to measure animation noise
        STATE_SWEEP_LOW_SET,
        STATE_SWEEP_LOW_SNAP,
        STATE_SWEEP_HIGH_SET,
        STATE_SWEEP_HIGH_SNAP,
        STATE_FINALIZE,
        STATE_QUIT
    };

    static const CvarSpec sSpecs[];
    static const size_t   sSpecCount;

    EState  mState         = STATE_IDLE;
    int     mWaitFrames    = 0;
    size_t  mCvarIdx       = 0;
    int     mNoiseIdx      = 0;
    int     mSnapRepeatIdx = 0;
    LLSD    mManifest;
    std::string mOutputDir;
    std::map<std::string, LLSD> mOriginalValues;
    std::map<std::string, LLSD> mForcedMasters; // depends names that were force-ON

    LLAYAuditTicker* mTicker = nullptr;

    void   ensureOutputDir();
    bool   takeSnapshot(const std::string& filename);
    void   recordOriginal(const CvarSpec& spec);
    void   restoreOriginal(const CvarSpec& spec);
    void   setCvar(const CvarSpec& spec, double value);
    void   forceMasterOn(const char* name);
    void   restoreMaster(const char* name);
    void   writeManifest();
    void   logState(const char* label);
};

#endif // LL_AYAUDIT_H
