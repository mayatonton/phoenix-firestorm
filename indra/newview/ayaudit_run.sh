#!/usr/bin/env bash
# AYAudit launcher: starts the viewer with every shader-permutation-sensitive
# and render-target-sensitive master cvar pre-set to ON, so the dependent
# sub-cvars can be observed during the sweep.
#
# Why pre-set instead of mid-session toggle:
# - Shader permutations (HAS_DOF_CHROMA, GODRAYS_FADE, ...) are baked at
#   viewer init, before audit code runs. Mid-session setBOOL does NOT
#   recompile shaders, so sub-cvars stay dark.
# - Render targets (mVelocityMap for MotionBlur, SSR copy targets) are
#   allocated at init based on initial cvar state. Mid-session toggle does
#   not re-allocate, so the dependent post-pass remains skipped.
#
# Usage: ayaudit_run.sh [PROFILE] [SLURL_OVERRIDE]
#   PROFILE = A (default, Morris/Aditi baseline)
#           | B (kustom9/Agni motion-rich)
#           | C (reflection-rich, scene 未確定)
#           | D (depth-rich, scene 未確定)
#   SLURL_OVERRIDE: profile default の SLURL を上書きしたい時のみ
set -euo pipefail

PROFILE="${1:-A}"
SLURL_OVERRIDE="${2:-}"

case "$PROFILE" in
    A)  # baseline: Morris / Aditi / static scene
        AUDIT_PROFILE_NAME="A-morris-aditi-baseline"
        GRID_ARGS=(--grid util.aditi.lindenlab.com)
        LOGIN_ARGS=(--login esforco Resident Pokemon1007)
        DEFAULT_SLURL="secondlife://util.aditi.lindenlab.com/secondlife/Morris/84/117/36"
        ;;
    B)  # motion-rich: 人 + 動きあり / NOTE: avatar 動が noise floor を持ち上げ
        # 検出母集団を縮小するため audit にはマイナス、Kittens Palace で失敗確定済
        AUDIT_PROFILE_NAME="B-agni-motion"
        GRID_ARGS=()  # Agni はデフォルト grid
        LOGIN_ARGS=(--login esforco Resident Pokemon1007)
        DEFAULT_SLURL="secondlife://kustom9/118/96/1003"
        ;;
    C)  # reflection-rich: 静かな反射面景勝地 (水鏡 / ガラス / クローム)
        # 人が少ない場所が必須 (avatar 動が noise floor を持ち上げる)
        AUDIT_PROFILE_NAME="C-agni-reflection-static"
        GRID_ARGS=()  # Agni
        LOGIN_ARGS=(--login esforco Resident Pokemon1007)
        DEFAULT_SLURL="secondlife://Roleplay%20Heaven/66/127/22"
        DOF_MASTER_PIN="0"
        ;;
    D)  # depth-rich: 起動時 DoF ON で DoF HQ chain (HQ/Chroma/ChromaStrength) 覚醒
        # 前景 1-3m + 遠景 50m+ 共存 scene 推奨。autofocus 1Hz bistate で
        # noise floor 上昇リスクあり、mask 値見て採否判断。
        AUDIT_PROFILE_NAME="D-agni-dof-on-depth"
        GRID_ARGS=()  # Agni
        LOGIN_ARGS=(--login esforco Resident Pokemon1007)
        DEFAULT_SLURL="secondlife://Roleplay%20Heaven/66/127/22"
        DOF_MASTER_PIN="1"
        ;;
    *)
        echo "unknown profile: $PROFILE (expected A|B|C|D)" >&2
        exit 2
        ;;
esac
DOF_MASTER_PIN="${DOF_MASTER_PIN:-0}"

SLURL="${SLURL_OVERRIDE:-$DEFAULT_SLURL}"
AUDIT_DIR="${AUDIT_DIR:-/tmp/aya-audit-$PROFILE}"
LOGFILE="${LOGFILE:-/tmp/aya-audit-run-$PROFILE.log}"
VIEWER="${VIEWER:-/home/ishikawa/ayastorm/ayastorm}"

mkdir -p "$AUDIT_DIR"
# Clear stale snapshots
rm -f "$AUDIT_DIR"/*.png "$AUDIT_DIR"/manifest.llsd "$LOGFILE"

# Kill any stray viewer
pkill -f 'do-not-directly-run-ayastorm-bin' 2>/dev/null || true
sleep 1

# Master cvars that gate dependent sub-cvar visibility. Set ON at startup so
# their shader permutations get compiled and render targets allocated.
SETS=(
    # Multi-instance + login plumbing
    --multiple
    --set SLURLPassToOtherInstance 0
    --set RestrainedLove 0
    # Mode: AYAstorm View (1) — getRenderCvar* returns user values, not bd_default
    --set AYAVisualRealismEnabled 1
    # Deferred + GI masters (these are usually default-ON but pin them)
    --set RenderDeferred 1
    --set RenderDeferredSSAO 1
    --set RenderDeferredBlurLight 1
    # Glow master OFF at startup. With glow ON, the chain naturally amplifies
    # animation noise (water, leaves, particles) via smoothstep(minLuminance)
    # threshold + blur kernel, producing a ~40% whole-frame bistate
    # concentrated in bloom region — that drives the baseline noise floor
    # to ~0.40 and forces the mask to drop 40% of pixels (60% stable).
    # With glow OFF, baseline is clean (99.5% stable, floor ~0.005). Glow
    # sub-cvars are still measured cleanly because setShaders() re-fires on
    # RenderGlow toggle and forceMasterOn() re-enables glow during each
    # sub-cvar's low/high sweep pair (per-sub-cvar measurement has some
    # bistate variance during the sweep but baseline & non-glow cvars are
    # clean).
    --set RenderGlow 0
    # Shadow master (S32: 2 = sun+spot shadows on)
    --set RenderShadowDetail 2
    # DoF + Chroma masters (Chroma triggers HAS_DOF_CHROMA shader permutation)
    # NOTE: DoF autofocus oscillates between avatar and background subjects,
    # producing a ~1Hz bistate in the sun-bloom region (sun blur radius changes
    # with focus distance). DoF master is OFF by default (profile A/B/C) to
    # keep noise calibration clean; profile D pins DoF=1 to wake the HQ chain
    # at the cost of autofocus noise (mask may drop, judge by mask value).
    --set RenderDepthOfField "$DOF_MASTER_PIN"
    --set RenderDepthOfFieldChroma 1
    --set RenderDepthOfFieldHighQuality 1
    --set RenderDepthOfFieldFront 1
    # MotionBlur master (allocates mVelocityMap target at init)
    --set RenderMotionBlur 1
    --set RenderMotionBlurSelfAvatar 1
    --set RenderMotionBlurOtherAvatars 1
    # SSR master (allocates SSR copy targets at init)
    --set RenderScreenSpaceReflections 1
    # VolumetricLighting master + directional shader permutation
    # NOTE: Volumetric block is gated by isCinematicMode() AND bd_default
    # short-circuit, so this is currently no-op even ON. Pre-set anyway
    # in case a future change opens the gate.
    --set RenderVolumetricLighting 1
    --set RenderVolumetricLightingDirectional 1
    # AA master (FSAA SMAA path). T2x jitters camera by 0.5px between
    # alternating frames; un-accumulated snapshots land in random parity,
    # producing whole-frame ~0.43 diff between back-to-back frames that
    # poisons noise calibration. Force OFF for audit; T2x itself will be
    # measured as DEAD as a known limitation (documented).
    --set RenderFSAAType 2
    --set RenderSMAAT2x 0
    # Dynamic exposure feeds back the previous frame's luminance into the
    # current frame's tonemap — alternating snapshot frames see alternating
    # exposure states, producing a periodic ~0.40 whole-sky bistate that
    # destroys the noise floor. Force OFF for audit (the per-cvar sweep of
    # exposure-related cvars is not currently in scope; can be added with a
    # separate sweep run).
    --set RenderDynamicExposureEnabled 0
    # Audit
    --ayaudit
    --ayaudit-output "$AUDIT_DIR"
    # Grid + login (profile 依存)
    "${GRID_ARGS[@]}"
    "${LOGIN_ARGS[@]}"
    "$SLURL"
)

cd "$(dirname "$VIEWER")"
nohup "$VIEWER" "${SETS[@]}" > "$LOGFILE" 2>&1 &
echo "PID=$! PROFILE=$AUDIT_PROFILE_NAME AUDIT_DIR=$AUDIT_DIR LOGFILE=$LOGFILE"
