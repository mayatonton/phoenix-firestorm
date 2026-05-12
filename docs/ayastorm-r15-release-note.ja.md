# AYAstorm r15 — リリース告知文案

GitHub release ページに貼り付ける用の文案。**r15 は r12.1 (= r10→r12.1 累積リリース) からの 3 段 (r13 + r14 + r15) を 1 ジャンプで束ねた次の公開リリース** で、音響表現章の最後の柱 (タグベース occlusion) と、新章「視覚的リアリティ」の第 1+2 弾 (空気の体積 + 光線) を一括同梱します。

機能の詳細はユーザー向けガイド (`doc/3dstream-tag-guide.{ja,en,zh}.md`) と各仕様書 / ロードマップに常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r15 — タグベース occlusion + 視覚的リアリティ章 第 1+2 弾 (空気の体積 + 光線)

r12.1 → r15 の 1 ジャンプで以下をまとめて提供します。**r13 / r14 はそれぞれ独立リリースしておらず**、音響表現章を r13 で締めて視覚的リアリティ章を r14+r15 で立ち上げる流れを 1 リリースに集約しました。

### r13 由来 — タグベース occlusion (実形状 mesh raycast) + UI / 起動系 fix

#### r13 の柱: `[ayastorm:occlude]` 静的 occlusion (会場運営 / 建設者向け)

3 つ目のタグ family を新設。**壁・扉・床・天井プリム** に `[ayastorm:occlude]` を書くと、AYAstorm はそのプリムを「音を遮るもの」として扱い、リスナー位置と音源位置を結ぶ線分が **プリムの実形状 (path cut / hollow / mesh の三角形メッシュ)** を貫くと **音量減衰 + 低域偏重のローパス着色** で「壁の向こう」感を出します。

- **書式**: `[ayastorm:occlude]` 単体で default (`direct:0.7 reverb:0.5`)、`[ayastorm:occlude{direct:0.9}{reverb:0.7}]` で per-prim 値指定。詳細 → [tag-guide §16](../doc/3dstream-tag-guide.ja.md#16-静的-occlusion-ayastormocclude-r13) / 仕様 `doc/spec_obb_occlusion.md` / 実装記録 `docs/ayastorm-r13-occlusion.md`
- **何が遮蔽されるか**: `[3dstream:...]` / `[3dstream-stereo:...]` の 3D 定位ストリーム + `llPlaySound` / 添付音 / 子プリム効果音 (世界 SFX)。2D ストリーム / Voice / UI 音は対象外。
- **実プリム形状で判定**: Path Cut で開けた切れ目を抜ける音は通る / Hollow の中空内側を通る音は遮られない / mesh プリムは正確な形状で判定。OBB で粗く pre-cull (~95% reject) → Möller-Trumbore 三角形 raycast の 2 段判定で精度と CPU を両立 (詳細 → [tag-guide §16.2](../doc/3dstream-tag-guide.ja.md#162-動作モデル))。
- **複数プリムの集計**: 直接音 / 残響は **掛け合わせ** で蓄積 — 例: `direct=0.7` のプリム 2 枚を抜けると実効 `1 - (1-0.7)² ≈ 0.91`、「壁が増えるほどこもる」直感どおり。
- **推奨セット** (材質イメージ): 石壁 `0.9/0.7` / 木壁 `0.7/0.5` (default) / カーテン `0.6/0.4` / ガラス `0.3/0.2` / 装飾 `0.1/0.05`。詳細 → [tag-guide §16.4](../doc/3dstream-tag-guide.ja.md#164-推奨セット-材質イメージ--値)
- **動く扉も追従**: `refreshOccluders` が毎 tick 位置 / 回転 / スケールを再取得するので、LSL でアニメーションする扉 / 乗り物 / 移動プリムに書いても自動追従。専用「door タグ」は不要。build floater で **選択中のプリム** は Path Cut / Hollow / Sculpt スライダー操作中もシアン可視化と音側 raycast がライブ追従します (編集ウィンドウ閉じ待ち不要)。
- **配信者主導モデルとの直交性**: 既存の `[3dstream:...]{venue:NAME}` (配信者表現) と `[ayastorm:occlude]` (建設者物理) は意図的に独立。会場演出と現実建築が一致しない場合 (洞窟風プリムなのに `venue:cathedral` 等) でも viewer 側で警告 / 自動補正は出しません。

#### 同梱 debug settings (occlusion 系)

`[ayastorm:occlude]` を支える live-tuning 用 settings (一般 UI 無し、debug settings 経由):

| キー | 既定 | 目的 |
|---|---|---|
| `Stream3DOcclusion` | `-1` (= 有効) | master sentinel。`0` で全 occlude タグを無視 (smoothing 経由で素通しに戻る、cliff 無し) |
| `Stream3DOccluderRange` | `64.0` m | 距離 cull。listener-source 距離が超えれば raycast skip。`0` で常時 raycast |
| `Stream3DOcclusionRampMs` | `250.0` ms | 遮蔽の direct/reverb 値の crossfade 時間。`0` で binary jump (debug 用) |
| `Stream3DShowOccluders` | off | 登録済 occluder の **シアン三角形メッシュ** (半透明 fill + wireframe、raycast 実形状そのもの) を debug overlay 表示 (Alt+Shift+O、master と独立)。選択中プリムは編集中ライブ追従 |

詳細 → [tag-guide §16.6-§16.8](../doc/3dstream-tag-guide.ja.md#166-距離-cull-stream3doccluderrange--64m)

#### r13 同梱の独立修正

- **起動時 OS unresponsive dialog の根本対策** (commits `f336d43abc` / `5c3487ff06`): r11 P10 で導入した URL 事前解決 (libcurl HEAD pre-resolve) が `https://` URL に対して同期 3 秒待機し、login 直後 N 個の `[3dstream:url=https://…]` タグ付き prim が同時到着するシーンで N × 3s ブロック → OS「応答なし」dialog 発火 → の連鎖を起こしていた件を、**完全非同期 worker thread 経由 API** (`LLStream3DUrlResolve::submit/poll/cancel/shutdown`) に作り直して根治。main thread の curl 同期ブロックは完全消滅。詳細 → 実装記録 `docs/ayastorm-r13-occlusion.md` §5.4
- **chat font live-apply fix** (`d66bdb74fc`): LL-style chat (FS 旧式表示) で `ChatFontSize` / `PlainTextChatHistory` の変更が次の発話まで反映されない不具合を修正。**3dstream とは独立した chat UI 修正** で、occlusion 系の作業中に併発したものを cherry-pick で同梱。
- **V3 skin の on-screen chat console 初期表示揃え** (`617716ced8`): V3 skin だけ `FSUseNearbyChatConsole` の default が `0` で初期 OFF になっていたのを他 skin (firestorm / phoenix / text / hybrid) と揃えて `1` (= 初期 ON) に統一。新規インストール / skin 切替時の挙動が他 skin と一貫します。

#### r13 同梱の独立機能追加

- **`[parcelhide]` 高度ゲート** (`1dfa52d0e9`): parcel description のタグを `[parcelhide:{altitude:1000-2000,3000-4000}]` のように書くと、自分の高度 (Z) が指定範囲のいずれかに入っているときだけ非表示を発火させられます。両端 inclusive、ハイフン区切り、カンマ区切りで複数範囲。撮影用途の skybox 階だけ非表示にする等の運用が可能。`parcelhide` 単独 (引数なし) の従来動作はそのまま。
- **他住民所有オブジェクトの IM を一括無視 (`FSIgnoreObjectIM`)** (`e99d7c9abf`): ベンダー広告 / 釣り告知など、他住民が rez したオブジェクトからの IM を黙って捨てるグローバルスイッチ。**自分が所有する HUD / rezzer からの IM は素通り** (`permYouOwner()` 判定)。Preferences → Notifications → People にチェックボックスを追加、default OFF。

### r14 由来 — 視覚的リアリティ章 第 1 弾: 空気の体積 (volumetric atmosphere)

音響表現章 (r8〜r13) を締めて、視覚的リアリティ章を r14 で立ち上げます。**章の本丸は「写真を撮るに値する空気と空間」**「物質が見せる本来の色・空気・雰囲気」。LUT / Tone での「写真風 look」誤魔化しや AAA 的な暗がり依存はこの章で明示的に拒否し、scene-referred な物理計算を内側で組み直す方針です (章 thesis → `docs/ayastorm-visual-realism-roadmap.md` §1)。

r14 は A 軸 (大気・空気) の第 1 弾として、**距離と高度に応じて空気が体積として見える** ところを取りに行きました。

- **マスタースイッチ `AYAVisualRealismEnabled`** (default TRUE): 視覚的リアリティ章 (r14 以降) 全体を 1 本でまとめて ON/OFF。OFF で r13 までと同じ見え方に戻ります。軸 / 機能ごとの個別 cvar は本章では量産しません (`feedback_prefer_defaults_over_config.md`)。
- **altitude density (高度減衰の物理化)**: 高度に応じて大気密度が指数で減衰する scale-height モデルを `calcAtmosphericVars` 内に挿入。高所からの遠景・低空の霞みが「色付きフィルター」状から自然な体積減衰に変わります。詳細 → `docs/ayastorm-r14-volumetric-atmosphere.md` §3-§4
- **scene-referred 積分** (sky shader 内側、`skyV.glsl` の haze 色合成を blue / haze 2 系統に分割して linear 空間で積分): 朝・夕の暖色 / 青空 / 雲被り時の色相が、preset 値 (Blue Density / Haze Density / Haze Horizon) を「物理パラメタ」として再解釈して積み直されます。preset 互換 (朝 preset は朝らしい暖色、夕は夕らしい、を維持) は保ったまま、空気感の物理的な妥当性が上がる構成です。
- **WindLight preset は失われない**: 朝・夕・夜・region 個別 preset 等の数値はそのまま input。estate operator / 配信者 / AYA 自身の preset 資産は無効化しません。過去 SS との完全一致は取れませんが、master switch OFF で r13 までの見え方に戻せる設計を本章で維持します。
- **deferred → HDR scene buffer → tonemap → LDR の骨格は保つ**: Linden 本家との merge 経路を維持し、AYAstorm 単独 fork のメンテ破綻を避けます。書き換えるのは内側 (atmospherics の計算式、sun halo / haze_glow の生成経路) のみ。

#### r14 で意図的にスコープ外にしたもの

- **Preetham (1999) 近似による太陽方向経路長の物理化** (`docs/ayastorm-r14-volumetric-atmosphere.md` §4 P2.b/c): 太陽 disc が消失する副作用が出たため deferred。再設計時に「sun disc 保護」と両立する形で復活させる方針。
- **sun disc HDR boost** (alpha-driven halo の前提誤り、`69cf280f43` で revert): r14 P1 で一度試行したが体感ゼロで unground、r14 中では再着手せず r17 (時間帯色温度 + 雲) と合流予定。

### r15 由来 — 視覚的リアリティ章 第 2 弾: 光線が空間を貫く (godrays)

r14 で「空気が体積として見える」基盤ができたので、r15 ではその空気の中を **光線が走る** 体感を加えます。

- **shadow map driven の screen-space godrays pass を新規追加**: `renderGeomPostDeferred` の atmospherics 直後に挿入する fullscreen pass。既存の cascaded sun shadow を流用 (新規 shadow buffer は追加しません)、視線方向に 16 サンプルの ray-march で「太陽光が遮蔽されていないか」を積分、Mie 前方ピーク phase (`cos^8`) で太陽方向のヴェールに整形してから HDR scene buffer に additive で乗せます。詳細 → `docs/ayastorm-r15-godrays.md`
- **強度は `strength = 0.10` で固定**: tuning cvar は導入しません (master switch 1 本のみで章全体を ON/OFF)。AYA 体感調整で 0.5 → 0.2 → 0.15 → 0.10 と段階的に絞り、太陽方向に控えめなヴェールが乗る・空や地面の色を壊さない、を満たす値に着地。
- **マスタースイッチは r14 と共有**: `AYAVisualRealismEnabled = FALSE` で godrays pass も skip。OFF 時は r13 までの見え方に戻ります。
- **新規 cvar / 新規 UI なし**: 配信者主導モデル (r11+) の流儀をそのまま視覚側にも適用。本リリースに r15 由来の Preferences UI 追加は **なし**。
- **既存 glow / bloom と非衝突**: godrays は HDR scene buffer 上で動き、glow は tonemap 後の bright pass。論理的な合成衝突なし、両立して足し算的に効きます。

#### r15 P1 で判明し永続化した知見

scene buffer に additive (blendFunc `ONE/ONE`) で post-pass を書くときは、shader 側で `frag_color.a = 0.0` を強制する必要があります。alpha=1 を積むと scene buffer の alpha チャネル (doAtmospherics / sky 合成が sky mask として参照) が累積破壊され、tonemap 後に **空が真っ白に潰れる** 症状を起こすため。r15 P1 投入初回でこの症状を観測 → 原因特定 → memory `project_aya_visual_realism_alpha_protect.md` として永続化済 (r16+ 視覚表現章で再利用必須)。

### 既存配置の扱い

- r8 / r9 / r10 / r11 / r12 / r12.1 で既に置いた **全プリムはタグ無改修で動作** します
- r13 の `[ayastorm:occlude]` は新規 opt-in タグなので、これを書かない既存会場の体感は r12.1 と完全同じ (occlusion 系)
- r14 / r15 の視覚的リアリティ章は master switch `AYAVisualRealismEnabled` (default TRUE) で全 viewer に効きますが、**OFF で r13 までの見え方に完全に戻ります**。WindLight preset / Environment preset / region 個別 preset は input として受け取り続けるので preset 資産の無効化はありません。
- r12 で導入した `[3dstream:...]{venue:NAME}` (会場残響) と r13 の `[ayastorm:occlude]` (会場 occlusion) は **意図的に直交設計** — `venue:dry` の建物に occlude を貼ることも、`venue:cathedral` のプリムが occlude されることも問題なく動きます

### 既知の制約

- **occlusion 系** (r13 由来):
  - **同時 occluder 数 256** (`kMaxOccluders` hardcoded)。SL 通常会場 (~100 prim) には十分な余裕。257 個目以降は登録されず `LL_WARNS` がログに出ます。
  - **三角形数上限 2000 / occluder** (`kMaxTrisPerOccluder` hardcoded)。超過する mesh プリムは三角形抽出を諦め bounding OBB のみで判定するフォールバックモードに入ります (`LL_WARNS_ONCE` 出力、§16.8 のシアン非表示でも判別可)。標準 SL prim や建築用 mesh は通常範囲内。
  - **同梱 FMOD 2.03.07 の制約**: 内部実装は viewer 側の OBB pre-cull + Möller-Trumbore 三角形 raycast (同梱 libfmod の `FMOD::Geometry::createGeometry` が動かないため)。ユーザー視点では影響無し。
- **視覚的リアリティ章** (r14 / r15 由来):
  - **shadow detail 依存**: r15 godrays は cascaded sun shadow (RenderShadowDetail ≥ 1) が前提。shadow OFF 設定では godrays pass も実質ゼロ寄与になります (描画破綻ではなく見えなくなるだけ)。
  - **太陽方向 cone 限定**: phase = `cos^8` の指数で、太陽周辺おおむね 30° 以内の view ray でのみ実効寄与。太陽が完全に画面外の方向には godrays は (意図通り) 乗りません。
  - **過去 SS との完全一致は取れない**: r14 の atmospherics 内側書き換えで色の積み方が変わるため、r13 までの screenshot とピクセル一致しません。master switch OFF で r13 までの見え方に戻せます。

### 意図的にスコープ外 (r14 / r15 design decision)

以下は「未実装」ではなく、**入れない方針** として固めた設計判断です (詳細 → [`docs/ayastorm-visual-realism-roadmap.md`](./ayastorm-visual-realism-roadmap.md))。

- **LUT / tonemap / color grade による「写真風 look」誤魔化し**: scene-referred な物理計算の精緻化を取ります。本章で明示的に拒否。
- **AAA 風の暗がり依存**: 明るい場所 / 曇り / 雨でも空気感が出る表現を狙います。暗くするだけの解決策は採用しません。
- **重い per-frame full-screen volumetric ray-march**: 配布負債と GPU 負荷の両面で AYAstorm の流儀 (1 viewer / 3 OS) に合わない。r15 godrays も 16 サンプル shadow-driven の軽量実装。
- **軸 / 機能ごとの個別 debug settings 量産**: master switch 1 本 (`AYAVisualRealismEnabled`) で章全体を ON/OFF (`feedback_prefer_defaults_over_config.md`)。

r16+ で順次積みます:
- **r16 aerial perspective** (距離による色変化): depth-based scattering integration、preset の Distance Multiplier を物理係数として再解釈
- **r17 時間帯色温度 + 雲のリアリティ**: Sun/Ambient color の色温度解釈、雲の volumetric 化 (軽量、heavy raymarch ではない)
- **r18+ 物質色 (B 軸)** / **r20+ カメラ表現 (C 軸)**

### ドキュメント

- ユーザー向けガイド: `doc/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md` (§16 として OBB occlusion 全項を新設)
- r13 仕様: `doc/spec_obb_occlusion.md`
- r13 実装記録 + 設計判断ログ: `docs/ayastorm-r13-occlusion.md`
- r14 spec: `docs/ayastorm-r14-volumetric-atmosphere.md`
- r15 spec: `docs/ayastorm-r15-godrays.md`
- 視覚的リアリティ章ロードマップ: `docs/ayastorm-visual-realism-roadmap.md` (r14-r17 A 軸、r18+ B 軸、r20+ C 軸の長期計画)
- 音響表現章ロードマップ (r13 完結): `docs/ayastorm-stream3d-roadmap.md`
- 描画パフォーマンス調査メモ (議論用たたき台): `docs/ayastorm-render-perf-survey.md` — LL 本体 + Firestorm + AYAstorm 独自層の描画ホットスポットを横串で俯瞰した観測ノート。r13-r15 個別の機能ではなく今後の改修議論用
