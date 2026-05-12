# r15 P0: godrays 実装注入点調査 (Round 1)

**作成日**: 2026-05-12
**対象**: AYAstorm r15 (godrays = 光線が空間を貫く) の実装注入点同定
**ブランチ**: `feature/aya-r15-godrays-spec-draft`

> 本書は r15 spec (`docs/ayastorm-r15-godrays.md`) §4 P0 の **完了条件** (pass 挿入点 / shadow map availability / sun direction uniform availability / HDR additive 注入経路 / 既存 glow との衝突切り分け) に応える調査記録。実装 (P1) 着手前のスナップショット。

---

## 1. pass 挿入点の同定

### 1.1 frame 全体の主要関数呼び出し順 (LLPipeline / llviewerdisplay 経由)

| 順 | 関数 | 場所 | 役割 |
|---|---|---|---|
| 1 | `generateSunShadow(LLViewerCamera)` | `llviewerdisplay.cpp:1328` | 太陽方向の cascaded shadow map を生成 (4 cascade) |
| 2 | `renderDeferredLighting()` | `llviewerdisplay.cpp:1373` / `pipeline.cpp:9829` | deferred light pass (HDR scene buffer `mRT->screen` に lit color を蓄積) |
| 3 | `renderGeomPostDeferred(camera)` | `pipeline.cpp:4739` | 半透明 / 後段 pass 処理。途中で `doAtmospherics()`、`doWaterHaze()` を pool 順序に合わせて 1 回ずつ起動 |
| 4 | `doAtmospherics()` | `pipeline.cpp:4805` (from #3 内 L4803) / `pipeline.cpp:10327` | 大気 (calcAtmosphericVars 系) の screen-space additive。**まだ HDR、tonemap 前** |
| 5 | `renderFinalize()` | `llviewerdisplay.cpp:1624` / `pipeline.cpp:9345` | post-process orchestrator: SSR → luminance → exposure → tonemap → glow → CAS → DoF → FXAA/SMAA → vignette → present |

godrays は **scene-referred (HDR) 段階で additive** したい (章 thesis: LUT/post で誤魔化さず物理で組む)。

### 1.2 推奨挿入点

**第 1 候補: `doAtmospherics()` の直後** (`pipeline.cpp:4805` 直後)
- 大気 in-scatter が乗った直後の HDR scene buffer (`mRT->screen`) に additive
- `doAtmospherics` 同様、screen-space pass で fullscreen quad → fragment で sun direction にレイマーチ
- ファイル: `pipeline.cpp` の `renderGeomPostDeferred` 内、`done_atmospherics = true;` の直後に `doGodrays()` 追加

**第 2 候補: `renderGeomPostDeferred` の最後** (alpha 描画も含めて全て描いた後)
- alpha や paritcle も含めた最終的な HDR scene に対して godrays
- ただし alpha の rim が godrays の occluder として参加してほしくないので、shadow map ベースの r15 では関係ない
- 第 1 候補で十分

### 1.3 半解像度中間 buffer の必要性
- 既存 SSR / atmospherics / waterhaze は基本 full-res で動いている (mRT->screen に直接書き込み)
- godrays は per-pixel に多段 shadow sample (8-32 steps) を行うため、full-res では bandwidth 負荷高い見込み
- 推奨: `mRT->godrays` を **half-res or quarter-res** で新規追加、godrays pass はそこに write、最後に upsample + additive で `mRT->screen` に乗せる
- 既存例: `mSceneMap` (SSR の中間)、`mLuminanceMap` (exposure 用) など解像度違いの aux buffer は pipeline で既に運用されている

---

## 2. shadow map の availability 確認

### 2.1 既存 cascaded sun shadow map

`generateSunShadow(LLCamera&)` (`pipeline.cpp:11274`) が **既に毎フレーム 4 cascade で sun direction shadow を生成**。

- 提供 API: `LLPipeline::getSunShadowTarget(U32 i)` (`pipeline.h:350`, `i = 0-3`)
- 行列: `mSunShadowMatrix[6]` (`pipeline.h:839`, 4 sun cascade + 2 spot)
- shader uniform 経路:
  - `DEFERRED_SHADOW_MATRIX` (`"shadow_matrix"`, `llshadermgr.h:161`)
  - `DEFERRED_SHADOW_CLIP` (`"shadow_clip"`, L163)
  - `DEFERRED_SHADOW0` 〜 `DEFERRED_SHADOW5` (`"shadowMap0"`-`"shadowMap5"`, L214-L219)
- 既存利用例: `shadowUtil.glsl` に sun shadow sampling helper あり、godrays shader から呼び出し可能

### 2.2 雲の遮蔽の取り扱い (R2 リスク評価)

- SL の雲は WindLight cloud layer = sky dome 上の billboard / projected texture で、 **shadow map には参加しない** (建物 / 地形 / オブジェクトのみ)
- ただし「雲間 godrays」は r15 ねらいの 1 つ
- 対策案 (P1 で判断):
  - (a) shadow map ベースの godrays は地形・建物の隙間用と割り切る、雲間は r17 (雲体積化) まで保留
  - (b) sky shader (`skyV.glsl` / `skyF.glsl`) の cloud alpha を screen-space で再 sample して occluder として参加させる (擬似 cloud shadow projection)
- 一旦 (a) で着手、(b) は P1 で必要性判断 → spec §6 R4 で trade-off 整理する

---

## 3. sun direction uniform の availability

`DEFERRED_SUN_DIR` (`"sun_dir"`, `llshadermgr.h:178`) が **既に plumbed**。

- world-space sun direction は deferred lighting で全 shader に配布されている
- `lightnorm` も並列に存在 (atmospherics / sky で使われている、`skyV.glsl` / `shadowUtil.glsl` / `skyF.glsl` / `cloudsV.glsl`)
- godrays shader では world-space sun direction を view matrix で **screen-space に project** して使う:
  ```
  vec4 sun_screen_clip = projection_matrix * view_matrix * vec4(sun_dir, 0.0);
  vec2 sun_screen_uv   = (sun_screen_clip.xy / sun_screen_clip.w) * 0.5 + 0.5;
  ```
- radial blur 系 godrays: 各 pixel から sun_screen_uv に向けて N steps レイマーチ、各 step で:
  - 深度 sample → world-space 復元 → shadow_matrix で shadow map sample
  - 影で無い (太陽光が届く) なら sun radiance を 1/N で加算

新規 uniform は基本不要 (既存の sun_dir / shadow_matrix / shadowMap0-3 / projection / view を使い回せる)。

---

## 4. HDR scene buffer への additive 注入経路

### 4.1 buffer 状態
- HDR 有効時 (`RenderHDREnabled=TRUE`, GL 4.05+): `mRT->screen` が RGBA16F、tonemap 前まで HDR scene を保持
- `doAtmospherics` も `mRT->screen` に additive 加算する形 (`pipeline.cpp:10327` 参照、内部で fullscreen quad pass)

### 4.2 godrays の additive 経路
1. **godrays pass**: half-res `mRT->godrays` (新規) に shader 出力
2. **upsample + additive**: full-res `mRT->screen` に bilinear upsample + 加算 blend
   - 別 shader か、godrays shader の出力を `glBlendFunc(GL_ONE, GL_ONE)` で `mRT->screen` に直接 blit するか
- 既存 SSR (`copyScreenSpaceReflections`) や atmospherics と同じパターンで実装可能

### 4.3 HDR disabled (LDR only) 経路
- `gammaCorrect(&mRT->screen, &mPostPingMap)` が呼ばれる (`pipeline.cpp:9389`)
- LDR fallback でも godrays は **gammaCorrect 前** に乗せれば物理整合性は維持される
- 実装上は HDR / non-HDR 共通で「`renderGeomPostDeferred` の `doAtmospherics` 直後」に挿入で OK

---

## 5. 既存 glow / bloom との衝突有無

### 5.1 既存 glow / bloom の構成
- `generateGlow(&mPostPingMap)` (`pipeline.cpp:8406`) が `renderFinalize` 内 **tonemap 後** に呼ばれる (L9394)
- 中身: `glowExtractF.glsl` で luminance threshold (`minLuminance`) を超えた pixel を抽出 → blur (`RenderGlowIterations`) → `combineGlow` で additive
- **LDR 空間** (tonemap 済の `mPostPingMap`) で動作

### 5.2 godrays との関係
- godrays は **HDR pre-tonemap** で乗る。tonemap 後 LDR の glow が、godrays 由来で明るくなった pixel を再度 bloom として拾うこと自体は **物理的に自然** (= 明るいものは bloom する)
- 二重に光って overbright になるリスクは P1 着手後の体感調整で判断、godrays intensity uniform を新規 1 本入れて tune できる余地は残す (debug settings として恒久化はしない、`feedback_prefer_defaults_over_config.md`)
- 既存の太陽 halo は `glowExtractF` の高輝度抽出が拾うので、godrays が無くても halo は出ている → godrays は太陽そのものではなく **太陽周辺の空間の光線** を担う、機能的に重複しない

### 5.3 既存太陽 disc / glow pipeline との関係
- r14 P0 (`doc/r14/sun_rendering_survey.md`) で同定: 太陽 disc は `sunDiscF.glsl`、halo は LDR 段の `generateGlow` 経由
- godrays はそれらとは別 pass で、太陽 disc / halo はそのまま温存

---

## 6. 結論 (P1 着手前の決め事)

| # | 項目 | 確定値 |
|---|---|---|
| 1 | pass 挿入点 | `LLPipeline::renderGeomPostDeferred` 内、`doAtmospherics()` 完了直後 (`pipeline.cpp:4805` 直後) に `doGodrays()` を新規追加 |
| 2 | 中間 buffer | `mRT->godrays` を half-res で新規追加 (P1 で full-res でも回るか先に試して bandwidth で苦しければ half-res に落とす) |
| 3 | shadow map | 既存 `getSunShadowTarget(0-3)` + `mSunShadowMatrix` を流用、cascaded で godrays ray-march の各 step を shadow test |
| 4 | sun direction | 既存 `DEFERRED_SUN_DIR` ("sun_dir") + `lightnorm` を流用、shader 内で view/projection で screen-space に project |
| 5 | HDR additive | `mRT->screen` (HDR RGBA16F) に additive blit、`gBlendFunc(GL_ONE, GL_ONE)` |
| 6 | master switch | r14 で配線済 `aya_visual_realism_enabled` を流用、off 時は `doGodrays()` 自体を skip (完全 noop) |
| 7 | 個別 cvar | **godrays intensity 1 本だけ**は P1 着手後に必要性判断、必要なら `RenderGodraysStrength` を入れる。それ以外の tuning cvar (step 数 / decay 等) はソース内定数で持つ方向、`feedback_prefer_defaults_over_config.md` |
| 8 | 雲間 godrays | r15 では shadow map ベース (地形・建物の隙間) のみ。雲は shadow map に参加していないため雲間 godrays は r17 (雲体積化) まで保留 |
| 9 | 3 OS 影響 | GLSL 共通、`.metal` 無し、`#ifdef` 無しで通す方針を維持 |
| 10 | FPS 影響 | full-res 1 pass = 大気 pass と同程度の見込み、half-res 化で更に軽量化可能。実測は P1 着手後 |

---

## 7. P1 着手時の触るファイル (最終)

- `indra/newview/pipeline.h` — `mGodraysMap` (新規 LLRenderTarget), `doGodrays()` 宣言, `releaseGodraysTargets()` / `allocateGodraysTargets()` 補助
- `indra/newview/pipeline.cpp` — `doGodrays()` 実装、`renderGeomPostDeferred` 内 L4805 直後で呼び出し、buffer create/release
- `indra/newview/llviewershadermgr.h` / `.cpp` — `gDeferredGodraysProgram` 追加、`loadShadersDeferred` で load
- `indra/llrender/llshadermgr.h` — 必要なら `DEFERRED_GODRAYS_STRENGTH` 等の uniform enum 追加 (1 本のみ)
- `indra/newview/app_settings/shaders/class1/deferred/godraysV.glsl` (新規) — fullscreen quad vertex
- `indra/newview/app_settings/shaders/class1/deferred/godraysF.glsl` (新規) — screen-space ray-march + shadow sample + sun radiance accumulate
- master switch (`aya_visual_realism_enabled`) は r14 で配線済、追加なし

---

## 8. 残る不確実性 / 次回 P1 で詰める論点

- half-res / full-res の決定 (FPS 計測後)
- N steps (8 / 16 / 32) の決定 (体感 vs GPU 負荷)
- 雲遮蔽の reconstruction (採用案 b: cloud alpha re-sample) は r17 まで保留としたが、もし P1 体感確認で「太陽が雲に半分隠れている時の見え方」が不自然すぎる場合は r15 内で簡易版を検討
- godrays intensity の preset 連動方法 (WindLight の `glow.xyz` のいずれかにマッピングするか、独立に持つか) — preset 互換性を維持しつつ意味づけ
