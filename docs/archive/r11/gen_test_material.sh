#!/usr/bin/env bash
#
# AYAstorm r11 binaural / venue reverb 検証材料ジェネレータ
#
# spec_binaural_venue_reverb.md §5.1 に従い、binaural (lite-HRTF) 検証用の
# 3 素材を 30 秒 mono Vorbis として生成する:
#
#   pink_noise_30s.ogg     — air absorption (HF rolloff) 距離変化チェック
#   voice_panning_test.ogg — 単独 source placement に対する 360° 頭追従
#   click_train_440Hz.ogg  — ITD の sample-level 聴感 (sharp transient)
#
# venue 検証は r10 で生成済の sample1_6ch.ogg (musical content) を流用するの
# でこのスクリプトでは扱わない (§5.2)。
#
# 一発スクリプト。生成済み ogg は repo に入れず AYA Icecast にホストする
# 想定なので、出力ディレクトリは repo 外 (default ./out_r11_test_material/)。
#
# 依存: espeak-ng, sox, oggenc (vorbis-tools)
#
# 使い方:
#   docs/archive/r11/gen_test_material.sh [output_dir]
#

set -euo pipefail

OUT_DIR="${1:-./out_r11_test_material}"

DURATION_SEC=30
SAMPLE_RATE=48000

echo "[gen_test_material] checking prerequisites..."
for tool in espeak-ng sox oggenc; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ERROR: required tool not found in PATH: $tool" >&2
        exit 1
    fi
done

echo "[gen_test_material] output directory: $OUT_DIR"
mkdir -p "$OUT_DIR"
TMP_DIR="$(mktemp -d -t r11_test_material.XXXXXX)"
trap 'rm -rf "$TMP_DIR"' EXIT

# ---- 1) pink noise 30s -------------------------------------------------
# air absorption (HF rolloff) は broadband 素材でないと体感しづらい。
# pinknoise を 30s 連続で出すだけ。
echo "[gen_test_material] (1/3) pink_noise_30s"
sox -n -c 1 -r "$SAMPLE_RATE" -b 16 -e signed-integer "$TMP_DIR/pink_noise_30s.wav" \
    synth "$DURATION_SEC" pinknoise
oggenc --quiet --quality 5 \
    -o "$OUT_DIR/pink_noise_30s.ogg" \
    "$TMP_DIR/pink_noise_30s.wav"

# ---- 2) voice panning test 30s ----------------------------------------
# espeak-ng の生成尺は短い (数秒) ので、30s 全尺を埋めるよう repeat する。
# pad で 1 周期間に短い無音を挟み、avatar を回している間に何度も
# "left/center/right" が再生されるようにする (1 周期あたり pad 込み ~6s)。
echo "[gen_test_material] (2/3) voice_panning_test"
espeak-ng -v en -w "$TMP_DIR/voice_raw.wav" \
    "left ... center ... right ... left ... center ... right"
# espeak-ng は固定 22050 Hz で出すのでまずリサンプル
sox "$TMP_DIR/voice_raw.wav" -r "$SAMPLE_RATE" "$TMP_DIR/voice_resampled.wav"
# 1 周期 = voice + 0.5s 無音、それを 30s 超えるまで繰り返してから trim
sox "$TMP_DIR/voice_resampled.wav" "$TMP_DIR/voice_loop_unit.wav" \
    pad 0 0.5
sox "$TMP_DIR/voice_loop_unit.wav" "$TMP_DIR/voice_panning_test.wav" \
    repeat 9 trim 0 "$DURATION_SEC"
oggenc --quiet --quality 5 \
    -o "$OUT_DIR/voice_panning_test.ogg" \
    "$TMP_DIR/voice_panning_test.wav"

# ---- 3) click train 440Hz @ 1Hz tick rate ----------------------------
# 20ms 440Hz burst → 980ms silence → repeat。30 clicks @ 1Hz = 30s。
# ITD 確認用の sharp transient なのでフェードは入れない (envelope を入れ
# ると onset がボヤけて inter-aural delay が判別しにくくなる)。
# なお spec §5.1 の例コマンド (synth 30 sine 440 ... fade) は連続 sine で
# transient が出ず ITD 検証に不適なため、description 側の "20ms tick × 1Hz"
# 仕様に合わせてここを正解とする。
echo "[gen_test_material] (3/3) click_train_440Hz"
sox -n -c 1 -r "$SAMPLE_RATE" -b 16 -e signed-integer "$TMP_DIR/click_train_440Hz.wav" \
    synth 0.020 sine 440 vol 0.3 \
    pad 0 0.980 \
    repeat 29
oggenc --quiet --quality 5 \
    -o "$OUT_DIR/click_train_440Hz.ogg" \
    "$TMP_DIR/click_train_440Hz.wav"

echo "[gen_test_material] done."
echo "  outputs:"
echo "    $OUT_DIR/pink_noise_30s.ogg"
echo "    $OUT_DIR/voice_panning_test.ogg"
echo "    $OUT_DIR/click_train_440Hz.ogg"
echo "  next: AYA Icecast にアップロードし、prim description の {url} に設定"
echo "        (venue 検証は r10 の sample1_6ch.ogg をそのまま流用する)"
