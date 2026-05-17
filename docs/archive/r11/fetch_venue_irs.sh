#!/usr/bin/env bash
#
# AYAstorm r11 venue IR DL/変換 helper
#
# OpenAIR (CC-BY 4.0) から 8 IR の zip を DL → 解凍。中身を AYA が確認し、
# variant を選んだら `ir_convert <input.wav> <venue_name>` で 48 kHz / 16-bit
# / mono / 3 秒 trim+fade に正規化して `app_settings/venue_ir/<venue>.wav` に
# drop-in する。
#
# spec_binaural_venue_reverb.md §4.4.5 / §13 の bundled IR catalog に対応:
#
#   room_small  → Terry's Typing Room
#   room_medium → Spring Lane Building, U. of York
#   hall_small  → The Dixon Studio Theatre
#   hall_medium → St Andrew's Church
#   hall_large  → Usina del Arte Symphony Hall
#   club        → Genesis 6 Studio Live Room (Drum Set Up)
#   cathedral   → York Minster
#   outdoor     → Koli National Park – Summer
#
# 依存: curl, unzip, sox
#
# 使い方:
#   1) source docs/archive/r11/fetch_venue_irs.sh        # 関数 ir_convert を読み込む
#   2) docs/archive/r11/fetch_venue_irs.sh fetch         # 8 zip を DL + 解凍
#   3) ls ./out_r11_venue_ir/<slug>/             # 中身を確認
#   4) ir_convert ./out_r11_venue_ir/<slug>/<picked>.wav <venue_name>
#                                                # 8 venue 全部を順次
#

set -euo pipefail

OUT_DIR="${OUT_DIR:-./out_r11_venue_ir}"
VENUE_IR_DEST="${VENUE_IR_DEST:-indra/newview/app_settings/venue_ir}"

# venue_name → OpenAIR slug (zip filename = slug.zip)
declare -A IR_SLUG=(
    [room_small]=terrys-typing-room
    [room_medium]=spring-lane-building-university-york
    [hall_small]=dixon-studio-theatre-university-york
    [hall_medium]=st-andrews-church
    [hall_large]=usina-del-arte-symphony-hall
    [club]=genesis-6-studio-live-room-drum-set
    [cathedral]=york-minster
    [outdoor]=koli-national-park-summer
)

OPENAIR_BASE="https://webfiles.york.ac.uk/OPENAIR/IRs"

# 48 kHz / 16-bit / mono / 3 s trim+fade に統一変換して venue_ir/ に drop-in。
# 入力が stereo / B-Format でも sox の -c 1 で 1ch に down-mix される
# (B-Format の場合は W+X+Y+Z 平均になるため、事前に W ch だけを取り出した
# 中間 wav を渡す方が音響的には clean。それは AYA 判断)。
# 末尾 0.05 s の fade は IR 切断時のクリック防止。
ir_convert() {
    if [[ $# -ne 2 ]]; then
        echo "usage: ir_convert <input.wav> <venue_name>" >&2
        return 2
    fi
    local in="$1"
    local name="$2"
    if [[ ! -f "$in" ]]; then
        echo "ERROR: input wav not found: $in" >&2
        return 1
    fi
    if [[ -z "${IR_SLUG[$name]:-}" ]]; then
        echo "ERROR: unknown venue name: $name" >&2
        echo "valid: ${!IR_SLUG[*]}" >&2
        return 1
    fi
    mkdir -p "$VENUE_IR_DEST"
    local out="$VENUE_IR_DEST/${name}.wav"
    sox "$in" -r 48000 -b 16 -c 1 -e signed-integer "$out" \
        trim 0 3 \
        fade 0 -0 0.05
    echo "  $name <- $in"
    echo "    -> $out ($(soxi -d "$out" 2>/dev/null || echo '?') @ $(soxi -r "$out" 2>/dev/null) Hz, $(soxi -b "$out" 2>/dev/null)-bit)"
}

fetch_all() {
    echo "[fetch_venue_irs] checking prerequisites..."
    for tool in curl unzip sox soxi; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            echo "ERROR: required tool not found in PATH: $tool" >&2
            exit 1
        fi
    done

    mkdir -p "$OUT_DIR"
    local zip_dir="$OUT_DIR/zips"
    mkdir -p "$zip_dir"

    for name in "${!IR_SLUG[@]}"; do
        local slug="${IR_SLUG[$name]}"
        local url="$OPENAIR_BASE/$slug/$slug.zip"
        local zip_path="$zip_dir/$slug.zip"
        local extract_dir="$OUT_DIR/$slug"

        if [[ -f "$zip_path" ]]; then
            echo "[fetch] $name ($slug.zip) — already present, skip DL"
        else
            echo "[fetch] $name <- $url"
            curl --fail --location --show-error --silent \
                 -o "$zip_path" "$url"
        fi

        if [[ -d "$extract_dir" ]]; then
            echo "[fetch]   already extracted at $extract_dir, skip unzip"
        else
            mkdir -p "$extract_dir"
            unzip -q -o "$zip_path" -d "$extract_dir"
        fi
    done

    echo
    echo "[fetch_venue_irs] done. inspect each venue dir and pick a WAV variant:"
    for name in "${!IR_SLUG[@]}"; do
        local slug="${IR_SLUG[$name]}"
        echo "  $name: $OUT_DIR/$slug/"
    done
    echo
    echo "preferred variant order:"
    echo "  1) cardioid mono recording (e.g. neumann-km140-*.wav)"
    echo "  2) omnidirectional mono"
    echo "  3) Soundfield W channel only (use sox 'remix 1' to extract)"
    echo "  4) stereo cardioid pair (sox -c 1 averages L+R)"
    echo "  5) B-Format 4ch (last resort, -c 1 averages W+X+Y+Z)"
    echo
    echo "then run for each:"
    echo "  ir_convert <picked-wav-path> <venue_name>"
}

# Allow `source` for ir_convert function only, or `fetch` for the DL run.
if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    case "${1:-}" in
        fetch) fetch_all ;;
        *)     echo "usage: $0 fetch  |  source $0  (then call ir_convert)" >&2
               exit 2 ;;
    esac
fi
