# AYAstorm r13 — リリース告知文案

GitHub release ページに貼り付ける用の文案。r13 は r12.1 (= r10→r12.1 ジャンプ後の最初の公開リリース) からの **小ぶりな増分** で、新タグ family `[ayastorm:occlude]` (実プリム形状で判定) + UI/起動系の修正 2 件が骨子です。

機能の詳細はユーザー向けガイド (`docs/guides/3dstream-tag-guide.{ja,en,zh}.md`) と仕様書に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r13 — タグベース occlusion (実形状 mesh raycast) + UI / 起動系 fix

### r13 の柱: `[ayastorm:occlude]` 静的 occlusion (会場運営 / 建設者向け)

3 つ目のタグ family を新設。**壁・扉・床・天井プリム** に `[ayastorm:occlude]` を書くと、AYAstorm はそのプリムを「音を遮るもの」として扱い、リスナー位置と音源位置を結ぶ線分が **プリムの実形状 (path cut / hollow / mesh の三角形メッシュ)** を貫くと **音量減衰 + 低域偏重のローパス着色** で「壁の向こう」感を出します。

- **書式**: `[ayastorm:occlude]` 単体で default (`direct:0.7 reverb:0.5`)、`[ayastorm:occlude{direct:0.9}{reverb:0.7}]` で per-prim 値指定。詳細 → [tag-guide §16](../guides/3dstream-tag-guide.ja.md#16-静的-occlusion-ayastormocclude-r13) / 仕様 `docs/specs/spec_obb_occlusion.md` / 実装記録 `docs/ayastorm-r13-occlusion.md`
- **何が遮蔽されるか**: `[3dstream:...]` / `[3dstream-stereo:...]` の 3D 定位ストリーム + `llPlaySound` / 添付音 / 子プリム効果音 (世界 SFX)。2D ストリーム / Voice / UI 音は対象外。
- **実プリム形状で判定**: Path Cut で開けた切れ目を抜ける音は通る / Hollow の中空内側を通る音は遮られない / mesh プリムは正確な形状で判定。OBB で粗く pre-cull (~95% reject) → Möller-Trumbore 三角形 raycast の 2 段判定で精度と CPU を両立 (詳細 → [tag-guide §16.2](../guides/3dstream-tag-guide.ja.md#162-動作モデル))。
- **複数プリムの集計**: 直接音 / 残響は **掛け合わせ** で蓄積 — 例: `direct=0.7` のプリム 2 枚を抜けると実効 `1 - (1-0.7)² ≈ 0.91`、「壁が増えるほどこもる」直感どおり。
- **推奨セット** (材質イメージ): 石壁 `0.9/0.7` / 木壁 `0.7/0.5` (default) / カーテン `0.6/0.4` / ガラス `0.3/0.2` / 装飾 `0.1/0.05`。詳細 → [tag-guide §16.4](../guides/3dstream-tag-guide.ja.md#164-推奨セット-材質イメージ--値)
- **動く扉も追従**: `refreshOccluders` が毎 tick 位置 / 回転 / スケールを再取得するので、LSL でアニメーションする扉 / 乗り物 / 移動プリムに書いても自動追従。専用「door タグ」は不要。build floater で **選択中のプリム** は Path Cut / Hollow / Sculpt スライダー操作中もシアン可視化と音側 raycast がライブ追従します (編集ウィンドウ閉じ待ち不要)。
- **配信者主導モデルとの直交性**: 既存の `[3dstream:...]{venue:NAME}` (配信者表現) と `[ayastorm:occlude]` (建設者物理) は意図的に独立。会場演出と現実建築が一致しない場合 (洞窟風プリムなのに `venue:cathedral` 等) でも viewer 側で警告 / 自動補正は出しません。

### 同梱 debug settings

`[ayastorm:occlude]` を支える live-tuning 用 settings (一般 UI 無し、debug settings 経由):

| キー | 既定 | 目的 |
|---|---|---|
| `Stream3DOcclusion` | `-1` (= 有効) | master sentinel。`0` で全 occlude タグを無視 (smoothing 経由で素通しに戻る、cliff 無し) |
| `Stream3DOccluderRange` | `64.0` m | 距離 cull。listener-source 距離が超えれば raycast skip。`0` で常時 raycast |
| `Stream3DOcclusionRampMs` | `250.0` ms | 遮蔽の direct/reverb 値の crossfade 時間。`0` で binary jump (debug 用) |
| `Stream3DShowOccluders` | off | 登録済 occluder の **シアン三角形メッシュ** (半透明 fill + wireframe、raycast 実形状そのもの) を debug overlay 表示 (Alt+Shift+O、master と独立)。選択中プリムは編集中ライブ追従 |

詳細 → [tag-guide §16.6-§16.8](../guides/3dstream-tag-guide.ja.md#166-距離-cull-stream3doccluderrange--64m)

### r13 同梱の独立修正

- **起動時 OS unresponsive dialog の根本対策** (commits `f336d43abc` / `5c3487ff06`): r11 P10 で導入した URL 事前解決 (libcurl HEAD pre-resolve) が `https://` URL に対して同期 3 秒待機し、login 直後 N 個の `[3dstream:url=https://…]` タグ付き prim が同時到着するシーンで N × 3s ブロック → OS「応答なし」dialog 発火 → の連鎖を起こしていた件を、**完全非同期 worker thread 経由 API** (`LLStream3DUrlResolve::submit/poll/cancel/shutdown`) に作り直して根治。main thread の curl 同期ブロックは完全消滅。詳細 → 実装記録 `docs/ayastorm-r13-occlusion.md` §5.4
- **chat font live-apply fix** (`d66bdb74fc`): LL-style chat (FS 旧式表示) で `ChatFontSize` / `PlainTextChatHistory` の変更が次の発話まで反映されない不具合を修正。**3dstream とは独立した chat UI 修正** で、occlusion 系の作業中に併発したものを cherry-pick で同梱。
- **V3 skin の on-screen chat console 初期表示揃え** (`617716ced8`): V3 skin だけ `FSUseNearbyChatConsole` の default が `0` で初期 OFF になっていたのを他 skin (firestorm / phoenix / text / hybrid) と揃えて `1` (= 初期 ON) に統一。新規インストール / skin 切替時の挙動が他 skin と一貫します。

### r13 同梱の独立機能追加

- **`[parcelhide]` 高度ゲート** (`1dfa52d0e9`): parcel description のタグを `[parcelhide:{altitude:1000-2000,3000-4000}]` のように書くと、自分の高度 (Z) が指定範囲のいずれかに入っているときだけ非表示を発火させられます。両端 inclusive、ハイフン区切り、カンマ区切りで複数範囲。撮影用途の skybox 階だけ非表示にする等の運用が可能。`parcelhide` 単独 (引数なし) の従来動作はそのまま。
- **他住民所有オブジェクトの IM を一括無視 (`FSIgnoreObjectIM`)** (`e99d7c9abf`): ベンダー広告 / 釣り告知など、他住民が rez したオブジェクトからの IM を黙って捨てるグローバルスイッチ。**自分が所有する HUD / rezzer からの IM は素通り** (`permYouOwner()` 判定)。Preferences → Notifications → People にチェックボックスを追加、default OFF。

### 既存配置の扱い

- r8 / r9 / r10 / r11 / r12 / r12.1 で既に置いた **全プリムはタグ無改修で動作** します
- `[ayastorm:occlude]` は新規 opt-in タグなので、これを書かない既存会場の体感は r12.1 と完全同じ
- r12 で導入した `[3dstream:...]{venue:NAME}` (会場残響) と `[ayastorm:occlude]` (会場 occlusion) は **意図的に直交設計** — `venue:dry` の建物に occlude を貼ることも、`venue:cathedral` のプリムが occlude されることも問題なく動きます

### 既知の制約

- **同時 occluder 数 256** (`kMaxOccluders` hardcoded)。SL 通常会場 (~100 prim) には十分な余裕。257 個目以降は登録されず `LL_WARNS` がログに出ます。
- **三角形数上限 2000 / occluder** (`kMaxTrisPerOccluder` hardcoded)。超過する mesh プリムは三角形抽出を諦め bounding OBB のみで判定するフォールバックモードに入ります (`LL_WARNS_ONCE` 出力、§16.8 のシアン非表示でも判別可)。標準 SL prim や建築用 mesh は通常範囲内。
- **同梱 FMOD 2.03.07 の制約**: 内部実装は viewer 側の OBB pre-cull + Möller-Trumbore 三角形 raycast (同梱 libfmod の `FMOD::Geometry::createGeometry` が動かないため)。ユーザー視点では影響無し。

### 意図的にスコープ外 (r13 design decision)

以下は「未実装」ではなく、**入れない方針** として固めた設計判断です (詳細 → [`docs/ayastorm-stream3d-roadmap.md`](./ayastorm-stream3d-roadmap.md))。

- **Steam Audio integration / 回折・反射・共鳴の物理シミュ**: 反射 / 共鳴は r11 convolution venue reverb (9 IR) で先取り済、回折は r13 occlusion の lowpass + 減衰で知覚的に近似済。3 OS バイナリ配布負債と engine 依存を増やしてまで物理シミュを抱える ROI が薄い。
- **SOFA per-source HRTF / 個人 HRTF**: r11 lite-HRTF (ITD + ILD shadow + air abs) で AYAstorm の目指す音響リアリティ閾値は越えた。SOFA は CPU 高 × 個人測定済ユーザ限定で ROI 薄、再配布ライセンス調査負債も回避。
- **occluder sound transmission curve / per-material preset 表**: 実機聴感で material flag と occlusion 値の相関が薄い。tag-guide §16.4 の推奨セット (石壁 0.9/0.7 等) を建設者が選ぶ運用に統一。
- これらは将来必要になれば spike から再着手しますが、現状の r13 で「壁の向こう感」「会場の鳴り」「定位」は十分なリアリティに達したと判断しています。

### ドキュメント

- ユーザー向けガイド: `docs/guides/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md` (§16 として OBB occlusion 全項を新設)
- r13 仕様: `docs/specs/spec_obb_occlusion.md`
- r13 実装記録 + 設計判断ログ: `docs/ayastorm-r13-occlusion.md`
- ロードマップ: `docs/ayastorm-stream3d-roadmap.md`
- 描画パフォーマンス調査メモ (議論用たたき台): `docs/ayastorm-render-perf-survey.md` (`33c3afaf62`) — LL 本体 + Firestorm + AYAstorm 独自層の描画ホットスポットを横串で俯瞰した観測ノート。r13 の機能ではなく今後の改修議論用
