# AYAstorm r15: godrays (光線が空間を貫く)

**作成日**: 2026-05-12
**対象**: AYAstorm `feature/aya-r15-godrays-spec-draft` (P1 実装完了 / Linux 実機 PASS)
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の第 2 弾、r14 (volumetric atmosphere) の自然な延長

> **本書の役割**: r15 個別の **計画 + 実装結果スナップショット**。P0 / P1 完了後に commit hash / 確定値 / 体感 PASS を反映済。
> 章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md`、r14 (前リリース) は `docs/ayastorm-r14-volumetric-atmosphere.md`。

---

## 1. ゴール

r14 で「**空気が体積として見える**」基盤 (depth-driven Beer-Lambert + altitude density + scene-referred 積分) を整えた。r15 ではその空気の中を **光線が走る** 体感を加える。

具体的には:

- **雲間から差す光線** (太陽が雲に部分遮蔽されている時の god ray)
- **木漏れ日 / 窓から差す光線** (任意の遮蔽物が太陽光を切り取った時の volumetric beam)
- **大気中の塵 / 水蒸気で散乱した光線が「線」として見える**

技術的には **shadow map driven の screen-space light shaft** (Crytek 系 radial blur or screen-space ray-march) を analytic 寄りで実装する。SL 既存実装に sun shaft / godrays 系の infrastructure は **存在しない** (P0 事前 grep 確認済み: `sun.?shaft` / `godray` / `light.?shaft` ヒット 0、scatter ヒットは r14 で触った atmosphericsFuncs.glsl のみ)。ゼロから組む。

---

## 2. 設計制約

`docs/ayastorm-visual-realism-roadmap.md` §2 の境界条件をそのまま継承:

- **保つ**: WindLight preset 互換、HDR scene buffer 骨格、PBR shader interface、deferred → tonemap → LDR の流れ
- **書き換える**: 新規 post-process pass を `gPipeline` に追加 (既存 pass の改変は最小限)
- **言い換え**: 既存 sun direction / shadow map / depth buffer を input として受け取り、screen-space で god ray を加算合成。tonemap 入力の HDR scene buffer に対して additive で乗せる

### Master switch
- r14 で導入済の `AYAVisualRealismEnabled` (default TRUE) を流用
- `FALSE` で r14 までの見え方に戻る (godrays pass を skip)
- 軸 / リリース個別 switch は作らない (`feedback_prefer_defaults_over_config.md`)

---

## 3. スコープ

### 含む
- **shadow map driven screen-space godrays pass** を新規追加 — sun direction を screen-space に projection、screen-space radial march で shadow sample を積分、光線として加算
- pass の挿入位置: deferred lighting 後、tonemap 前 (HDR scene buffer に additive)
- master switch `aya_visual_realism_enabled` で pass の skip / 実行を分岐 (skip 時は完全 noop、r14 までと同じ見え方)
- C++ 側 plumbing: `LLPipeline` に godrays pass を新規 register (`renderGodrays` 等)、frame buffer の中継、必要なら quarter-res 中間バッファ
- 新規 shader: vertex (fullscreen quad) + fragment (screen-space ray-march)
- preset との関係: WindLight の `sun_glow_focus` / `sun_glow_size` 等を「godrays の濃さ / 広がり」として再解釈 (新規 uniform 追加は最小限)
- 3 OS (Linux / macOS / Windows) ビルド + 体感確認

### 含まない (→ r16+)
- aerial perspective の精緻化 (色相変化、距離 attenuation の物理化、r16)
- 時間帯色温度の物理化 + 波長依存散乱の物理分離 (r17)
- 雲の体積化 (r17)
- heavy raymarch volumetric (per-frame full-screen ray-march、毎フレーム重い積分)
- 物質側の subsurface scattering (B 軸、r18+)

### 永久 drop
- 軸 / 機能ごとの個別 debug settings (master switch 1 本のみ、godrays 強度を tuning する追加 cvar は入れない方向で P1 着手、必要なら P1 で議論)
- preset を破壊する後方非互換変更
- LUT / color grade による「光線風」誤魔化し
- 重い per-frame full-screen volumetric ray-march (配布負債と GPU 負荷の両面で AYAstorm の流儀 = 1 viewer / 3 OS に合わない)

---

## 4. フェーズ分解

viewer-only の改修。配信側 / SIM 側変更なし。

### P0: 実装箇所調査 + spec 確定 — **完了** (fd027e7475 起票 / 95223d1079 調査)

調査ターゲット (P0 完了条件) と結果:

1. **pass 挿入点の同定** — `LLPipeline::renderGeomPostDeferred` の `doAtmospherics()` 直後に挿入。`done_atmospherics` フラグで 1 frame 1 回ガード済の slot を共有
2. **shadow map の availability 確認** — `shadowUtil.glsl` の `sampleDirectionalShadow()` を `features.hasShadows + HAS_SUN_SHADOW permutation` で attach、既存 cascade (`shadowMap0..3` + `shadow_matrix` + `shadow_clip`) を流用 (追加 buffer なし)
3. **sun direction uniform の availability 確認** — `mTransformedSunDir` (view-space) が `bindDeferredShader()` 経由で `sun_dir` / `moon_dir` に自動 bind。新規 uniform 不要
4. **HDR scene buffer への additive 注入の経路** — `mRT->screen` が currently-bound のまま `doAtmospherics` と同じ流儀で fullscreen triangle を additive blend で描く。FBO 追加なし、quarter-res buffer も不要 (16-sample で full-res 直書きが軽量に成立)
5. **既存 glow / bloom との衝突有無** — godrays は HDR scene buffer 上で動き、glow は tonemap 後の bright pass。**論理的衝突なし**、両立して足し算的に効く

成果物: `docs/archive/r15/godrays_survey.md` (Round 1)。

### P1: 実装 — **完了** (9a703a2480)

**実際に触ったファイル** (P0 見込みからの差分含む):

- `indra/newview/app_settings/shaders/class1/deferred/godraysV.glsl` (新規) — fullscreen triangle、screen UV を fragment に渡す
- `indra/newview/app_settings/shaders/class1/deferred/godraysF.glsl` (新規) — depth 再構成 → 16-sample shadow-driven ray-march → Mie 前方ピーク phase 適用 → additive 出力
  - **配置 `effects/` ではなく `deferred/`** に置いた (P0 見込みからの変更): `shadowUtil.glsl` 系の auto-attach を効かせるため、deferred shader path 配下が必須
- `indra/newview/llviewershadermgr.h` — `gDeferredGodraysProgram` extern 宣言
- `indra/newview/llviewershadermgr.cpp` — declaration + `mShaderList.push_back` + load block (`gHazeProgram` 直後)。`features.isDeferred = true` で `deferredUtil.glsl`、`features.hasShadows = use_sun_shadow` + `HAS_SUN_SHADOW` permutation で `shadowUtil.glsl` を attach
- `indra/newview/pipeline.h` — `void doGodrays();` 宣言
- `indra/newview/pipeline.cpp` — `doGodrays()` 実装、`renderGeomPostDeferred` 内 `doAtmospherics()` 直後から呼び出し

**P0 見込みからの未着手 (= 不要だった)**:
- `indra/llrender/llshadermgr.h/.cpp` — 新規 uniform enum 追加不要 (既存 `sun_dir` / `moon_dir` / `sun_up_factor` / `inv_proj` で完結)
- `createGLBuffers` / `releaseGLBuffers` — FBO 追加不要
- `loadShadersEffects` — `effects/` 配下ではなく `loadShadersDeferred` 相当の chain に組み込んだ

**確定したアルゴリズム値**:
- ray-march samples: **N = 16**
- phase function: `pow(cos(view·sun), **8.0**)` (Mie 前方ピーク)
- strength: **0.10** (AYA 体感調整、0.5 → 0.2 → 0.15 → 0.10 で着地)
- jitter: Bayer-ish hash で隣接 pixel 間のバンディング破り
- cascade 外 sample: `if (p.z <= -shadow_clip.w) continue;` で 0 寄与扱い
  ※ `sampleDirectionalShadow` は cascade 外で 1.0 (lit) を返す surface-shading 仕様。godrays の中空 sample でそのまま使うと過剰加算
- NaN/Inf ガード: `shadow /= weight` の weight=0 fallthrough を防御
- **alpha 保護**: `frag_color.a = 0.0` 必須。ONE/ONE additive で alpha=1.0 を積むと scene buffer の sky mask が破壊され空が真っ白に潰れる (P1 初回投入で観測、原因特定 → memory `project_aya_visual_realism_alpha_protect.md` に永続化)

### P2: 3 OS ビルド + 体感確認 — **着手前**

AYA が Linux フルビルド + 体感確認 (shader-only 高速反映の後、commit 9a703a2480 が C++ も含むため一度 autobuild 必須)。問題なければ macOS / Windows ビルドへ。

### P3: tag / release — **着手前**

`feedback_release_with_user_feedback.md` に従い、完璧を目指さず tag / release してフィードバック収集。

---

## 5. 受け入れ条件

- [x] 既存 WindLight preset (朝・昼・夕・夜) が **読み込めて、preset 切替が機能する** (preset 互換破壊なし) — Linux P1 で preset 切替動作確認、破壊なし
- [x] `AYAVisualRealismEnabled = TRUE` で太陽方向に godrays が見える — Linux P1 で太陽周辺に控えめなヴェールを観測 (strength=0.10)
- [x] `AYAVisualRealismEnabled = FALSE` で r14 までと同じ見え方に戻る (godrays pass skip) — Linux P1 で確認
- [ ] 3 OS でビルド + 起動 + 表現確認 — Linux のみ完了、macOS / Windows は P2 で
- [ ] FPS 影響が ±10% 以内 — Linux 体感では問題なし、定量計測は P2 で
- [x] 描画破綻なし (sky 真っ白 / scene 色破壊なし) — P1 で alpha 保護を入れた後の状態で確認

---

## 6. リスク

| ID | リスク | 状態 |
|---|---|---|
| R1 | godrays pass の FPS 負荷が想定超過 (full-res screen-space ray-march の bandwidth) | P0 で quarter-res / half-res で済むか確認、必要なら downsampled 中間 buffer 採用 |
| R2 | shadow map の precision / range が godrays 用途に不十分 (太陽方向のみ覆っているか、雲遮蔽は反映されるか) | P0 で SL の shadow buffer 構成を調査、不足なら追加 buffer / 既存 buffer の解釈変更で対応 |
| R3 | 既存 glow / bloom と機能が重複・衝突 (太陽 halo は r13 までの glow で生成、新規 godrays と二重に光って overbright) | P0 で挙動切り分け、master switch off 時は r14 までと同じ glow 振る舞い、on 時は godrays が glow を一部担う設計を P1 で検討 |
| R4 | 太陽が雲・地形に完全遮蔽されている時 (太陽が screen 外) の godrays 取り扱い (見えるべきか / 切るべきか) | P0 後に design 判断、screen-space 系は太陽位置が screen 外だと自然に消えるはずだが、雲が画面内にある時の振る舞いは要確認 |
| R5 | 3 OS でビルドが通らない (macOS Metal 等) | r14 と同じく GLSL 共通 / `.metal` 無し / `#ifdef` 無しで通す、P0 で確認 |
| R6 | master switch off 経路で見え方が完全に r14 に戻らない | r14 と同じ流儀: `if (aya_visual_realism_enabled > 0)` を pass 入口で取り、off 時は完全 noop |

---

## 7. 更新履歴

- 2026-05-12 (初版): r14 (volumetric atmosphere) を P2.a refined で完了させ A 軸第 1 弾とした流れを受けて、A 軸第 2 弾の godrays を起票。既存 SL に sun shaft / godrays 系 infrastructure 無し (事前 grep 確認: `sun.?shaft` / `godray` / `light.?shaft` ヒット 0) を確認した上で、ゼロから shadow map driven screen-space pass として組む方針を確定。章全体の位置づけは `docs/ayastorm-visual-realism-roadmap.md` §4 r15、設計制約は §2 を継承
- 2026-05-12 (P0 完了): `docs/archive/r15/godrays_survey.md` (Round 1) で挿入点 (`renderGeomPostDeferred` の `doAtmospherics` 直後) / shadow map 経路 (`sampleDirectionalShadow` 流用) / sun direction (`bindDeferredShader` 自動 bind) / HDR scene buffer additive / glow 非衝突を確定。FBO 追加不要、新規 uniform enum 不要、quarter-res 不要の方針で P1 へ
- 2026-05-12 (P1 完了 / Linux 実機 PASS, commit 9a703a2480): godrays 実装 6 ファイル (`godraysV.glsl` / `godraysF.glsl` / `pipeline.h` / `pipeline.cpp` / `llviewershadermgr.h` / `llviewershadermgr.cpp`)。確定値 N=16 / phase=cos^8 / strength=0.10 / cascade skip / NaN ガード / **alpha 保護 (frag_color.a=0)**。alpha 保護に至るデバッグ中、ONE/ONE additive で alpha=1 を積むと scene buffer の sky mask が破壊されて空が真っ白に潰れる挙動を観測 → memory `project_aya_visual_realism_alpha_protect.md` に永続化 (r14+ 視覚表現章で再利用必須の知見)。3 OS ビルド + tag/release は P2 / P3 で実施
