# AYAstorm r15: godrays (光線が空間を貫く)

**作成日**: 2026-05-12
**対象**: AYAstorm `feature/aya-r15-godrays` (予定 / 着手前)
**位置づけ**: 視覚的リアリティ章 (`docs/ayastorm-visual-realism-roadmap.md`) §4 A 軸の第 2 弾、r14 (volumetric atmosphere) の自然な延長

> **本書の役割**: r15 個別の **計画スナップショット**。実装着手後に commit hash / 実測値を埋める。
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

### P0: 実装箇所調査 + spec 確定

調査ターゲット (P0 完了条件):

1. **pass 挿入点の同定** — `LLPipeline::renderGeom` / `renderDeferredLighting` / `renderPostProcess` のうちどこに register するか
2. **shadow map の availability 確認** — godrays に使う shadow buffer (sun direction 側) が既に main pass で生成済か、別途追加が必要か
3. **sun direction uniform の availability 確認** — `sun_dir` / `lightnorm` 系 uniform を godrays shader に渡す経路
4. **HDR scene buffer への additive 注入の経路** — どの FBO に対して blit / additive blend するか
5. **既存 glow / bloom との衝突有無** — `glowExtractF.glsl` / glow pass と機能的に重複しないか、合成順序

成果物: `doc/r15/godrays_survey.md` (Round 1)、必要に応じて spec §3 / §4 / §6 を P0 後改訂。

### P1: 実装

P0 で確定した経路に godrays pass を実装。

**触るファイル (見込み)**:
- `indra/newview/pipeline.h` — godrays pass 用 framebuffer / shader メンバ追加
- `indra/newview/pipeline.cpp` — `renderGodrays()` 追加、`renderPostProcess` (or 適切な箇所) から呼び出し
- `indra/newview/llviewershadermgr.h` / `.cpp` — godrays shader object 追加、load 経路
- `indra/llrender/llshadermgr.h` / `.cpp` — 必要なら新規 uniform enum 追加 (sun_dir_screen 等)
- `indra/newview/app_settings/shaders/class1/effects/godraysV.glsl` (新規) — fullscreen quad
- `indra/newview/app_settings/shaders/class1/effects/godraysF.glsl` (新規) — screen-space ray-march、shadow sample 積分
- (master switch は r14 で配線済の `aya_visual_realism_enabled` を流用)

**触る関数**:
- `LLPipeline::createGLBuffers` / `releaseGLBuffers` (FBO 追加)
- `LLPipeline::renderPostProcess` or 適切な host (pass 呼び出し追加)
- `LLViewerShaderMgr::loadShadersEffects` (shader load 追加)

### P2: 3 OS ビルド + 体感確認

AYA が Linux ビルド + 体感確認。問題なければ macOS / Windows ビルドへ。

### P3: tag / release

`feedback_release_with_user_feedback.md` に従い、完璧を目指さず tag / release してフィードバック収集。

---

## 5. 受け入れ条件

- [ ] 既存 WindLight preset (朝・昼・夕・夜) が **読み込めて、preset 切替が機能する** (preset 互換破壊なし)
- [ ] `AYAVisualRealismEnabled = TRUE` で:
  - 雲間 (雲が太陽を部分遮蔽している時) に光線が見える
  - 屋内 / 構造物の隙間から差す光が「線」として見える (空気中に光のすじが描ける)
- [ ] `AYAVisualRealismEnabled = FALSE` で r14 までと同じ見え方に戻る (godrays pass skip)
- [ ] 3 OS でビルド + 起動 + 表現確認
- [ ] FPS 影響が ±10% 以内 (P0 で軽量実装の見込みを立てる、quarter-res 中間バッファ等を許容)

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
