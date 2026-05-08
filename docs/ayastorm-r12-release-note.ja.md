# AYAstorm r12 — リリース告知文案

GitHub release ページに貼り付ける用の短文案。リリース判断確定後、AYA が公開時に貼り付ける。

機能の詳細はユーザー向けガイド (`doc/3dstream-tag-guide.{ja,en,zh}.md`) と各仕様書に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹する。

---

## AYAstorm r12 — バイノーラル + 会場残響 + stereo→5.1 upmix

r10 → r12 の 1 ジャンプで以下をまとめて提供します。**r11 は独立リリースせず、タグ書式変更を 2 段階に分けない方針** で r12 に丸ごと同梱しました。

### r11 由来 — 配信者が制御するヘッドホン体感 + 会場感

- **`{binaural:on|off}` (短縮形 `bin`)**: lite-HRTF (ITD + 空気吸収による HF 減衰) を per-channel で挿入。詳細 → [tag-guide §7.1](../doc/3dstream-tag-guide.ja.md#71-binauralonoff-短縮形-bin) / 仕様 `doc/spec_binaural_venue_reverb.md`
- **`{venue:NAME}` (短縮形 `v`)**: `dry` / `room_small` / `room_medium` / `hall_small` / `hall_medium` / `hall_large` / `club` / `cathedral` / `outdoor` の 9 種会場残響プリセット。詳細 → [tag-guide §7.2](../doc/3dstream-tag-guide.ja.md#72-venuename-短縮形-v)
- **`{wetgain:N}` (短縮形 `wg`)**: 0.0–2.0 の wet 倍率。全 IR が unity-gain 正規化済なので venue を切り替えても retune 不要。詳細 → [tag-guide §7.3](../doc/3dstream-tag-guide.ja.md#73-wetgainn-短縮形-wg)
- **配信者主導モデル**: この 3 キーは root prim Description が真理。listener 側 Preferences UI は新設しません。例外的な救援用に `Stream3DBinauralRender` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` の sentinel debug 設定のみ提供。詳細 → [tag-guide §7.4](../doc/3dstream-tag-guide.ja.md#74-配信者主導モデル) / [§12.2](../doc/3dstream-tag-guide.ja.md#122-debug-settings-詳細チューニング)

### r12 単独 — stereo 配信を 5.1 placement に展開

- **`{upmix:on|off}` タグ**: viewer 内 DSP で 2ch → 6ch (FL/FR/C/Ls/Rs/LFE) を生成。SL 配信ソフトの大半が stereo 止まりという実情に対し、r10 で作った 6 spk placement を **stereo 配信のままでも体験可能** にします。詳細 → [tag-guide §8](../doc/3dstream-tag-guide.ja.md#8-stereo51-upmix-r12) / 仕様 `doc/spec_stereo_upmix.md`
- **アルゴリズムは固定 (NG1)**: DPL2 系 matrix decode + 帯域分離 (LFE LPF / center bleed 除去 / rear decorrelation)。`{upmix:dpl2|logic7|...}` のような選択肢は導入しません (= 表現の不確定性を増やさない方針)。
- **5.1 native は auto bypass**: source ch ≥ 6 の場合 `{upmix:on}` でも自動 bypass + chat 通知 1 回。同じ Description を stereo / 5.1 native 両方の素材で使い回せます。
- **default は `off` (opt-in)**: r10 挙動を保つため、配信者がタグで明示的に opt-in。

### r12 共通の改善

- **タグ短縮形**: `binaural`/`venue`/`wetgain` を `bin`/`v`/`wg` に、9 venue 値にも 1〜2 字エイリアス (`d`/`rs`/`rm`/`hs`/`hm`/`hl`/`cl`/`ct`/`od`)。長形式と完全等価。SL Description 127-byte 制限に収まりやすくなります。詳細 → [tag-guide §4.5](../doc/3dstream-tag-guide.ja.md#45-キー名venue-値の短縮形-r12)
- **配信者向け LSL 拡張** (`doc/lsl/aya_3dstream_setup.lsl`): r11 タグ (binaural/venue/wetgain) と r12 タグ (upmix) を menu/dialog から設定可能。出力は **常に短縮形** で書き出します。
- **macOS ビルド復活**: r10.x では Linux/Windows のみとしていた macOS ビルドを r12 で復活。Mac ユーザーは r10 → r12 で移行してください。

### 既存配置の扱い

r8 / r9 / r10 で既に置いた全プリムは **タグ無改修で動作** します。r11/r12 機能は配信者が opt-in する設計です (defaults: `binaural=off` / `venue=dry` / `wetgain=1.0` / `upmix=off` = r10 挙動)。

### 既知の制約

- `hall_medium` / `hall_large` / `cathedral` は CPU 重め (r10 比 +7.7〜+10.2pp)。低スペック機では `room_small` / `room_medium` / `hall_small` 推奨。詳細 → [tag-guide §7.2](../doc/3dstream-tag-guide.ja.md#72-venuename-短縮形-v) CPU 表
- listener 側 venue / binaural の一般 UI は提供しません (配信者主導モデル)。
- SOFA 個人 HRTF / Steam Audio integration / VenueReverb CPU 最適化 / air absorption 客観 FFT 測定 / 公開 README は r13+ 持ち越し。

### IR ライセンス

bundled venue IR は OpenAIR 系 (CC-BY 4.0)。出典は `app_settings/venue_ir/CREDITS.md` を参照。

### ドキュメント

- ユーザー向けガイド: `doc/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md`
- r11 仕様: `doc/spec_binaural_venue_reverb.md`
- r12 仕様: `doc/spec_stereo_upmix.md`
- r12 実装記録: `docs/ayastorm-r12-stereo-upmix.md`
- ロードマップ: `docs/ayastorm-stream3d-roadmap.md`
