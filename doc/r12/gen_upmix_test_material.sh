#!/usr/bin/env bash
#
# AYAstorm r12 stereo→5.1 upmix 検証材料ジェネレータ
#
# spec_stereo_upmix.md §5 に従い、stereo upmix の効果を主観確認するための
# 3 素材を 30 秒 stereo Vorbis として生成する:
#
#   stereo_voice_30s.ogg       — center 抽出 (= U1)
#   stereo_music_wide_30s.ogg  — rear 展開 (= U2)
#   stereo_subbass_30s.ogg     — LFE 経路 (= U3)
#
# 5.1 native 回帰用 (= U4) は r10 で生成済の sample1_6ch.ogg を流用するの
# でこのスクリプトでは扱わない (spec §5 4 行目)。
#
# 各素材の理屈 (spec §4.3 の DPL2 + bleed 数式に照らして):
#
#   1) stereo_voice_30s: mono dialog を L=R duplicate して完全 centered。
#      C = (L+R)/√2 = √2·V (フル) / FL,FR = L − C·bleed/√2 = 0 (bleed=1) /
#      SL,SR = ±(L−R)/√2 = 0 / LFE = LPF((L+R)/2) ≈ 0 (voice は >80Hz)。
#      → C スピーカーだけが鳴り、その他 5 spk は実質無音。bleed 除去が
#         効いていないと front L/R に phantom center が二重で残る。
#
#   2) stereo_music_wide_30s: 独立 pinknoise × 2 を L/R に配置。L≠R で
#      decorrelated wide stereo。S = (L−R)/√2 がフル → SL/SR が delay-
#      decorrelated で空間的に展開、C = (L+R)/√2 は avg、LFE は LPF((L+R)/2)
#      で低域のみ。rear decorrelation が効いていれば SL/SR が「左右に
#      広がる ambience」として聞こえる。
#
#   3) stereo_subbass_30s: L = 60Hz sine + 880Hz sine、R = 60Hz sine +
#      1320Hz sine。60Hz は L=R で centered (→ C と LFE)、880/1320Hz は
#      L≠R で wide (→ front + rear)。
#         C   = √2·60Hz + (880Hz+1320Hz)/√2  (bass + mid 両方)
#         LFE = LPF(60Hz + (880+1320)/2)     (≈60Hz のみ、>80Hz は遮断)
#         FL  = (880Hz − 1320Hz)/2           (mid のみ、bass は bleed で消える)
#         FR  = (1320Hz − 880Hz)/2           (mid のみ、bass は bleed で消える)
#         SL  = +(880Hz − 1320Hz)/√2 delay'd (mid のみ)
#         SR  = −(880Hz − 1320Hz)/√2 delay'd (mid のみ)
#      → LFE スピーカーから 60Hz だけ、front/rear からは中域のみ
#         (bass が漏れていれば LPF が壊れている)。
#
# 一発スクリプト。生成済み ogg は repo に入れず AYA Icecast にホストする
# 想定なので、出力ディレクトリは repo 外 (default ./out_r12_test_material/)。
#
# 依存: espeak-ng, sox, oggenc (vorbis-tools)
#
# 使い方:
#   doc/r12/gen_upmix_test_material.sh [output_dir]
#

set -euo pipefail

OUT_DIR="${1:-./out_r12_test_material}"

DURATION_SEC=30
SAMPLE_RATE=48000

echo "[gen_upmix_test_material] checking prerequisites..."
for tool in espeak-ng sox oggenc; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ERROR: required tool not found in PATH: $tool" >&2
        exit 1
    fi
done

echo "[gen_upmix_test_material] output directory: $OUT_DIR"
mkdir -p "$OUT_DIR"
TMP_DIR="$(mktemp -d -t r12_upmix_test_material.XXXXXX)"
trap 'rm -rf "$TMP_DIR"' EXIT

# ---- 1) stereo voice 30s (centered) -----------------------------------
# espeak-ng の生成尺は短い (数秒) ので、30s 全尺を埋めるよう repeat する。
# r11 の voice_panning_test と同じテキスト構造を流用 (left/center/right の
# orientation cue が混ざるのは upmix 検証では重要ではないが、speech が単調
# にならず聴感確認が楽になる)。
# mono → -c 2 で L=R duplicate に展開して centered stereo を作る。
echo "[gen_upmix_test_material] (1/3) stereo_voice_30s"
espeak-ng -v en -w "$TMP_DIR/voice_raw.wav" \
    "left ... center ... right ... left ... center ... right"
sox "$TMP_DIR/voice_raw.wav" -r "$SAMPLE_RATE" "$TMP_DIR/voice_resampled.wav"
sox "$TMP_DIR/voice_resampled.wav" "$TMP_DIR/voice_loop_unit.wav" pad 0 0.5
sox "$TMP_DIR/voice_loop_unit.wav" "$TMP_DIR/voice_mono_30s.wav" \
    repeat 9 trim 0 "$DURATION_SEC"
# mono 1ch → stereo 2ch (L=R duplicate)。`remix 1 1` で 1ch をそのまま 2 本に。
sox "$TMP_DIR/voice_mono_30s.wav" -c 2 "$TMP_DIR/stereo_voice_30s.wav" \
    remix 1 1
oggenc --quiet --quality 5 \
    -o "$OUT_DIR/stereo_voice_30s.ogg" \
    "$TMP_DIR/stereo_voice_30s.wav"

# ---- 2) wide stereo music 30s (decorrelated pinknoise) ---------------
# pinknoise を 2 回独立に生成して L / R に振る。sox の synth は /dev/random
# から seed を引くので 2 回呼ぶと自然に decorrelated。混ざった「楽器」では
# なく純粋な broadband pink だが、spec §5 が要求しているのは「L≠R で wide
# な stereo 素材」であり、decorrelated pinknoise は最も理屈の通った wide
# 素材 (= 任意の周波数で L/R が無相関、S = (L−R)/√2 がフル帯域でエネルギー
# を持つ)。聴感的にも「左右に広がるホワイト/ピンクノイズ」として識別容易。
echo "[gen_upmix_test_material] (2/3) stereo_music_wide_30s"
sox -n -c 1 -r "$SAMPLE_RATE" -b 16 -e signed-integer \
    "$TMP_DIR/pink_left.wav" \
    synth "$DURATION_SEC" pinknoise vol 0.5
# 同じプロセスを 2 回呼ぶ。/dev/random は呼び出し毎に異なるバイト列を返す
# ため、出力 wav は decorrelated になる (内部 seed 共有なし)。
sox -n -c 1 -r "$SAMPLE_RATE" -b 16 -e signed-integer \
    "$TMP_DIR/pink_right.wav" \
    synth "$DURATION_SEC" pinknoise vol 0.5
sox -M "$TMP_DIR/pink_left.wav" "$TMP_DIR/pink_right.wav" \
    "$TMP_DIR/stereo_music_wide_30s.wav"
oggenc --quiet --quality 5 \
    -o "$OUT_DIR/stereo_music_wide_30s.ogg" \
    "$TMP_DIR/stereo_music_wide_30s.wav"

# ---- 3) stereo with strong sub bass 30s ------------------------------
# L = 60Hz + 880Hz、R = 60Hz + 1320Hz。60Hz は両 ch 共通なので centered →
# C と LFE に振り分けられる。880/1320Hz は L≠R で wide → front L/R / SL/SR
# に振り分けられる。LFE LPF が 80Hz cutoff で正しく効いていれば LFE スピーカー
# からは 60Hz のみが鳴り、880/1320Hz の高域は LFE には残らない。bleed=1.0
# で front L/R には bass が残らず、中域だけが鳴る。
# vol 0.5 は 2 sine の合成でサンプル飽和を避けるため (個別最大 0.5 → 合成
# 最大 1.0 を狙う計算)。
echo "[gen_upmix_test_material] (3/3) stereo_subbass_30s"
sox -n -c 1 -r "$SAMPLE_RATE" -b 16 -e signed-integer \
    "$TMP_DIR/subbass_left.wav" \
    synth "$DURATION_SEC" sine 60 sine 880 vol 0.5
sox -n -c 1 -r "$SAMPLE_RATE" -b 16 -e signed-integer \
    "$TMP_DIR/subbass_right.wav" \
    synth "$DURATION_SEC" sine 60 sine 1320 vol 0.5
sox -M "$TMP_DIR/subbass_left.wav" "$TMP_DIR/subbass_right.wav" \
    "$TMP_DIR/stereo_subbass_30s.wav"
oggenc --quiet --quality 5 \
    -o "$OUT_DIR/stereo_subbass_30s.ogg" \
    "$TMP_DIR/stereo_subbass_30s.wav"

echo "[gen_upmix_test_material] done."
echo "  outputs:"
echo "    $OUT_DIR/stereo_voice_30s.ogg       (U1 center 抽出)"
echo "    $OUT_DIR/stereo_music_wide_30s.ogg  (U2 rear 展開)"
echo "    $OUT_DIR/stereo_subbass_30s.ogg     (U3 LFE 経路)"
echo "  next: AYA Icecast にアップロードし、prim description の {url} に設定"
echo "        (5.1 native 回帰 = U4 は r10 の sample1_6ch.ogg をそのまま流用する)"
