#!/usr/bin/env bash
# AYAudit analyzer.
#
# Inputs in AUDIT_DIR (per-sample triplets, R=SNAP_REPEAT_COUNT):
#   00_baseline_a_a.png 00_baseline_a_b.png 00_baseline_a_c.png  ... (× N baselines)
#       N (= NOISE_SAMPLES) baseline positions, each captured R times spaced
#       ~0.25s apart. Each triplet is pixel-wise medianed into a denoised
#       baseline; diffs between denoised baselines = animation noise floor
#       (water, leaves, particles).
#   NNN_<cvar>_low_a.png _b.png _c.png        — R-snap triplet at low value.
#   NNN_<cvar>_high_a.png _b.png _c.png       — R-snap triplet at high value.
#
# Cinematic pipeline has an exposure-feedback loop concentrated in the sun-bloom
# region: pixels around the sun alternate between "bloomed" and "not bloomed"
# state across multi-second periods regardless of RenderDynamicExposureEnabled.
# To prevent that flicker from poisoning every per-cvar diff, Phase 0 builds a
# "flicker mask" from baseline variance and Phase 1/2 evaluate diffs only on
# stable (non-flicker) pixels.
#
# Output table:
#   CVAR | diff_frac | AE_px | noise_floor | judgement
#
# Judgement bands (relative to noise floor):
#   SOLID    : diff_frac >= noise_floor * 2.0
#   WIRED    : noise_floor * 1.3 <= diff_frac < noise_floor * 2.0
#   MARGINAL : noise_floor       <= diff_frac < noise_floor * 1.3
#   DEAD     : diff_frac < noise_floor
#
# Usage: ayaudit_analyze.sh [AUDIT_DIR=/tmp/aya-audit]
set -euo pipefail

AUDIT_DIR="${1:-/tmp/aya-audit}"
if [[ ! -d "$AUDIT_DIR" ]]; then
    echo "audit dir not found: $AUDIT_DIR" >&2
    exit 1
fi

MASK_FILE="$AUDIT_DIR/stable_mask.png"

# Compute pixel-difference fractions between two PNGs.
# Always returns the *masked* (stable-pixel-only) fraction first, and the
# *unmasked* (full-frame) fraction second. Without a mask, both are equal.
# Uses fuzz=5% to ignore sub-perceptual dither/quantization noise.
# Returns: "<masked_frac> <masked_AE> <unmasked_frac> <unmasked_AE>" on stdout.
diff_pair() {
    local A="$1" B="$2"
    local AE_U PX_U FRAC_U AE_M PX_M FRAC_M
    AE_U=$(compare -metric AE -fuzz 5% "$A" "$B" null: 2>&1 || true)
    AE_U=$(echo "$AE_U" | grep -oE '^[0-9.]+' | head -1 | cut -d. -f1)
    [[ -z "$AE_U" ]] && AE_U=0
    PX_U=$(identify -format "%[fx:w*h]" "$A" 2>/dev/null || echo 1)
    FRAC_U=$(awk -v a="$AE_U" -v p="$PX_U" 'BEGIN{ if(p<=0){print 0}else{printf "%.5f", a/p} }')

    if [[ -f "$MASK_FILE" ]]; then
        local AM="${A%.png}.masked.png" BM="${B%.png}.masked.png"
        [[ -f "$AM" ]] || convert "$A" "$MASK_FILE" -alpha off -compose multiply -composite "$AM" 2>/dev/null
        [[ -f "$BM" ]] || convert "$B" "$MASK_FILE" -alpha off -compose multiply -composite "$BM" 2>/dev/null
        AE_M=$(compare -metric AE -fuzz 5% "$AM" "$BM" null: 2>&1 || true)
        AE_M=$(echo "$AE_M" | grep -oE '^[0-9.]+' | head -1 | cut -d. -f1)
        [[ -z "$AE_M" ]] && AE_M=0
        PX_M="$STABLE_PX"
        FRAC_M=$(awk -v a="$AE_M" -v p="$PX_M" 'BEGIN{ if(p<=0){print 0}else{printf "%.5f", a/p} }')
    else
        FRAC_M="$FRAC_U"
        AE_M="$AE_U"
    fi
    echo "$FRAC_M $AE_M $FRAC_U $AE_U"
}

# Pixel-wise median a triplet (or any sequence) into a single output PNG.
# Cached: if the medianed file exists, reuse it.
medianize() {
    local prefix="$1"   # e.g. /tmp/aya-audit/00_baseline_a
    local out="${prefix}.med.png"
    local -a inputs=( "${prefix}"_*.png )
    if (( ${#inputs[@]} == 0 )); then
        return 1
    fi
    if [[ ! -f "$out" ]]; then
        convert "${inputs[@]}" -evaluate-sequence median "$out" 2>/dev/null
    fi
    echo "$out"
}

# --- Phase 0a: medianize all triplets up front (idempotent). ---
shopt -s nullglob
MED_PREFIXES=()
for f in "$AUDIT_DIR"/00_baseline_*_a.png "$AUDIT_DIR"/[0-9]*_low_a.png "$AUDIT_DIR"/[0-9]*_high_a.png; do
    [[ -e "$f" ]] || continue
    prefix="${f%_a.png}"
    MED_PREFIXES+=("$prefix")
done
for prefix in "${MED_PREFIXES[@]}"; do
    medianize "$prefix" >/dev/null
done

# --- Phase 0b: build flicker mask from baseline variance. ---
# Pixels whose value range (max - min) across all medianed baselines exceeds
# a threshold are flagged as flickering and excluded from all subsequent diffs.
BASELINES=( "$AUDIT_DIR"/00_baseline_*.med.png )
STABLE_PX=0
if (( ${#BASELINES[@]} >= 2 )); then
    rm -f "$AUDIT_DIR"/*.masked.png  # invalidate stale masked products
    convert "${BASELINES[@]}" -evaluate-sequence max "$AUDIT_DIR/_base_max.png" 2>/dev/null
    convert "${BASELINES[@]}" -evaluate-sequence min "$AUDIT_DIR/_base_min.png" 2>/dev/null
    # Range image: |max - min| per pixel. Use -compose difference (absolute
    # diff); -compose minus is order-dependent and silently produced zeros.
    # Threshold 10% (= 25.5 / 255) catches the bloom flicker without flagging
    # quiet animation noise (~1-2 units of dither).
    convert "$AUDIT_DIR/_base_max.png" "$AUDIT_DIR/_base_min.png" -compose difference -composite \
            -colorspace Gray -threshold 10% "$AUDIT_DIR/_flicker_mask.png" 2>/dev/null
    # Invert: stable_mask is white (255) where stable, black where flickery.
    convert "$AUDIT_DIR/_flicker_mask.png" -negate "$MASK_FILE" 2>/dev/null
    # Count stable pixels (mean of binary mask × area = white pixel count).
    STABLE_PX=$(identify -format "%[fx:mean*w*h]" "$MASK_FILE" 2>/dev/null | awk '{printf "%d", $1}')
    TOTAL_PX=$(identify -format "%[fx:w*h]" "$MASK_FILE" 2>/dev/null)
    if [[ -z "$STABLE_PX" || "$STABLE_PX" -le 0 ]]; then
        echo "warning: stable_mask is empty (all pixels flagged); disabling mask." >&2
        rm -f "$MASK_FILE"
    else
        STABLE_FRAC=$(awk -v s="$STABLE_PX" -v t="$TOTAL_PX" 'BEGIN{printf "%.3f", s/t}')
        printf "Flicker mask: %d / %d pixels stable (%.1f%%)\n" \
               "$STABLE_PX" "$TOTAL_PX" "$(awk -v f="$STABLE_FRAC" 'BEGIN{printf "%.1f", f*100}')"
    fi
fi

# --- Phase 1: noise floor from medianed baselines (masked). ---
NOISE_FLOOR=0
NOISE_DESC="(no baseline samples found; falling back to 0.005)"
if (( ${#BASELINES[@]} >= 2 )); then
    DIFFS=()
    for ((i=0; i<${#BASELINES[@]}; i++)); do
        for ((j=i+1; j<${#BASELINES[@]}; j++)); do
            read -r F _ _ _ < <(diff_pair "${BASELINES[i]}" "${BASELINES[j]}")
            DIFFS+=("$F")
        done
    done
    SORTED=$(printf "%s\n" "${DIFFS[@]}" | sort -g)
    N=${#DIFFS[@]}
    MID=$(( N / 2 ))
    if (( N % 2 == 1 )); then
        NOISE_FLOOR=$(echo "$SORTED" | sed -n "$((MID+1))p")
    else
        A=$(echo "$SORTED" | sed -n "${MID}p")
        B=$(echo "$SORTED" | sed -n "$((MID+1))p")
        NOISE_FLOOR=$(awk -v a="$A" -v b="$B" 'BEGIN{ printf "%.5f", (a+b)/2.0 }')
    fi
    NOISE_DESC=$(printf "(median of %d C(N,2) pairs from %d samples, masked)" "$N" "${#BASELINES[@]}")
fi
NOISE_FLOOR=$(awk -v n="$NOISE_FLOOR" 'BEGIN{ printf "%.5f", (n<0.001 ? 0.001 : n) }')

THR_SOLID=$(awk -v n="$NOISE_FLOOR" 'BEGIN{ printf "%.5f", n*2.0 }')
THR_WIRED=$(awk -v n="$NOISE_FLOOR" 'BEGIN{ printf "%.5f", n*1.3 }')

printf "Noise floor: %s %s\n" "$NOISE_FLOOR" "$NOISE_DESC"
printf "Thresholds: SOLID >= %s, WIRED >= %s, MARGINAL >= %s, DEAD < %s\n\n" \
       "$THR_SOLID" "$THR_WIRED" "$NOISE_FLOOR" "$NOISE_FLOOR"

# --- Phase 2: per-cvar sweep diffs. ---
# JUDGEMENT is classified entirely from the MASKED diff (stable pixels only).
# The unmasked column is shown for human inspection but NOT used for ranking:
# the sun-bloom bistate flicker (see Phase 0b) adds ~0.40 to most unmasked
# diffs randomly, so unmasked numbers near that magnitude carry no signal.
# A cvar that genuinely affects the bloom region will currently appear DEAD
# here because its effect is hidden inside the flickering mask area — fixing
# the bistate at the pipeline level is the long-term remedy.
printf "%-60s %10s %10s %s\n" "CVAR" "masked" "unmasked" "JUDGEMENT"
printf "%-60s %10s %10s %s\n" "------------------------------------------------------------" "----------" "----------" "---------"

for low in $(ls "$AUDIT_DIR" | grep -E '^[0-9]+_.*_low\.med\.png$' | sort); do
    cvar="${low#*_}"; cvar="${cvar%_low.med.png}"
    high="${low%_low.med.png}_high.med.png"
    if [[ ! -f "$AUDIT_DIR/$high" ]]; then
        printf "%-60s %10s %10s %s\n" "$cvar" "n/a" "n/a" "MISSING_HIGH_PAIR"
        continue
    fi

    read -r FM AM FU AU < <(diff_pair "$AUDIT_DIR/$low" "$AUDIT_DIR/$high")

    JUDGE=$(awk -v fm="$FM" -v s="$THR_SOLID" -v w="$THR_WIRED" -v n="$NOISE_FLOOR" \
        'BEGIN{
            if (fm>=s) print "SOLID";
            else if (fm>=w) print "WIRED";
            else if (fm>=n) print "MARGINAL";
            else print "DEAD";
         }')
    printf "%-60s %10s %10s %s\n" "$cvar" "$FM" "$FU" "$JUDGE"
done
