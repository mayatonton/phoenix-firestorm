# AYAstorm r13 — リリース告知文案

GitHub release ページに貼り付ける用の文案。r13 は r12.1 (= r10→r12.1 ジャンプ後の最初の公開リリース) からの **小ぶりな増分** で、新タグ family `[ayastorm:occlude]` + UI/起動系の修正 2 件が骨子です。

機能の詳細はユーザー向けガイド (`doc/3dstream-tag-guide.{ja,en,zh}.md`) と仕様書に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r13 — タグベース OBB occlusion + UI / 起動系 fix

### r13 の柱: `[ayastorm:occlude]` 静的 OBB occlusion (会場運営 / 建設者向け)

3 つ目のタグ family を新設。**壁・扉・床・天井プリム** に `[ayastorm:occlude]` を書くと、AYAstorm はそのプリムを「音を遮るもの」として扱い、リスナー位置と音源位置を結ぶ線分が OBB を貫くと **音量減衰 + 低域偏重のローパス着色** で「壁の向こう」感を出します。

- **書式**: `[ayastorm:occlude]` 単体で default (`direct:0.7 reverb:0.5`)、`[ayastorm:occlude{direct:0.9}{reverb:0.7}]` で per-prim 値指定。詳細 → [tag-guide §16](../doc/3dstream-tag-guide.ja.md#16-静的-obb-occlusion-ayastormocclude-r13) / 仕様 `doc/spec_obb_occlusion.md` / 実装記録 `docs/ayastorm-r13-occlusion.md`
- **何が遮蔽されるか**: `[3dstream:...]` / `[3dstream-stereo:...]` の 3D 定位ストリーム + `llPlaySound` / 添付音 / 子プリム効果音 (世界 SFX)。2D ストリーム / Voice / UI 音は対象外。
- **推奨セット** (材質イメージ): 石壁 `0.9/0.7` / 木壁 `0.7/0.5` (default) / カーテン `0.6/0.4` / ガラス `0.3/0.2` / 装飾 `0.1/0.05`。詳細 → [tag-guide §16.4](../doc/3dstream-tag-guide.ja.md#164-推奨セット-材質イメージ--値)
- **動く扉も追従**: `refreshOccluders` が毎 tick 位置 / 回転 / スケールを再取得するので、LSL でアニメーションする扉 / 乗り物 / 移動プリムに書いても自動追従。専用「door タグ」は不要。
- **配信者主導モデルとの直交性**: 既存の `[3dstream:...]{venue:NAME}` (配信者表現) と `[ayastorm:occlude]` (建設者物理) は意図的に独立。会場演出と現実建築が一致しない場合 (洞窟風プリムなのに `venue:cathedral` 等) でも viewer 側で警告 / 自動補正は出しません。

### 同梱 debug settings

`[ayastorm:occlude]` を支える live-tuning 用 settings (一般 UI 無し、debug settings 経由):

| キー | 既定 | 目的 |
|---|---|---|
| `Stream3DOcclusion` | `-1` (= 有効) | master sentinel。`0` で全 occlude タグを無視 (smoothing 経由で素通しに戻る、cliff 無し) |
| `Stream3DOccluderRange` | `64.0` m | 距離 cull。listener-source 距離が超えれば raycast skip。`0` で常時 raycast |
| `Stream3DOcclusionRampMs` | `250.0` ms | OBB 遮蔽の direct/reverb 値の crossfade 時間。`0` で binary jump (debug 用) |
| `Stream3DShowOccluders` | off | 登録済 occluder の OBB ワイヤーフレームを debug overlay 表示 (Alt+Shift+O、master と独立) |

詳細 → [tag-guide §16.6-§16.8](../doc/3dstream-tag-guide.ja.md#166-距離-cull-stream3doccluderrange--64m)

### r13 同梱の独立修正

- **起動時 OS unresponsive dialog の根本対策** (commits `f336d43abc` / `5c3487ff06`): r11 P10 で導入した URL 事前解決 (libcurl HEAD pre-resolve) が `https://` URL に対して同期 3 秒待機し、login 直後 N 個の `[3dstream:url=https://…]` タグ付き prim が同時到着するシーンで N × 3s ブロック → OS「応答なし」dialog 発火 → の連鎖を起こしていた件を、**完全非同期 worker thread 経由 API** (`LLStream3DUrlResolve::submit/poll/cancel/shutdown`) に作り直して根治。main thread の curl 同期ブロックは完全消滅。詳細 → 実装記録 `docs/ayastorm-r13-occlusion.md` §5.4
- **chat font live-apply fix** (`d66bdb74fc`): LL-style chat (FS 旧式表示) で `ChatFontSize` / `PlainTextChatHistory` の変更が次の発話まで反映されない不具合を修正。**3dstream とは独立した chat UI 修正** で、occlusion 系の作業中に併発したものを cherry-pick で同梱。

### 既存配置の扱い

- r8 / r9 / r10 / r11 / r12 / r12.1 で既に置いた **全プリムはタグ無改修で動作** します
- `[ayastorm:occlude]` は新規 opt-in タグなので、これを書かない既存会場の体感は r12.1 と完全同じ
- r12 で導入した `[3dstream:...]{venue:NAME}` (会場残響) と `[ayastorm:occlude]` (会場 occlusion) は **意図的に直交設計** — `venue:dry` の建物に occlude を貼ることも、`venue:cathedral` のプリムが occlude されることも問題なく動きます

### 既知の制約

- **同時 occluder 数 256** (`kMaxOccluders` hardcoded)。SL 通常会場 (~100 prim) には十分な余裕。257 個目以降は登録されず `LL_WARNS` がログに出ます。
- **OBB 近似**: bounding box 単位の遮蔽判定。アーチ / 曲面 / 階段の手すりなどは近似誤差が出るので、必要なら panel 分割で個別タグ。
- **同梱 FMOD 2.03.07 の制約**: 内部実装は viewer 側の segment-vs-OBB slab test (同梱 libfmod の `FMOD::Geometry::createGeometry` が動かないため)。ユーザー視点では影響無し。
- **Steam Audio integration / SOFA 個人 HRTF / occluder の sound transmission curve / per-material preset 表** は r13 では入れず r14+ 検討。viewer-side raycast + 単一 lowpass で「壁の向こう感」は十分出るという r13 design decision。

### ドキュメント

- ユーザー向けガイド: `doc/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md` (§16 として OBB occlusion 全項を新設)
- r13 仕様: `doc/spec_obb_occlusion.md`
- r13 実装記録 + 設計判断ログ: `docs/ayastorm-r13-occlusion.md`
- ロードマップ: `docs/ayastorm-stream3d-roadmap.md`
