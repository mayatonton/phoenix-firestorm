# r30 BD full port Phase 3.5 — AY-only Render* cvar dispatch spec

## 1 目的

`docs/specs/ayastorm-r30-bd-full-port-inventory.md` §3.3 (AY-only cvars 579 件) のうち
**`Render*` prefix を持つ 26 件** が AY 拡張機能 (LUT / Tone / Chroma / Volumetric Lighting /
DoF 拡張 / Motion Blur / SMAA / Resolution Multiplier 等) を駆動する。

BD には対応する cvar が存在しないため、Cinematic mode (mode 2) では
**helper 経由で AY 拡張機能を完全 disable / no-op 値に固定** する必要がある。

本 spec は 26 cvar を grep 結果ベースで category 分けし、Phase 3.7 dispatch impl で
適用する `getRenderCvarXxx("...", BD_NOOP_DEFAULT)` の BD_NOOP 値を確定する。

---

## 2 対象 26 cvar (grep ベース call site あり)

`indra/newview/llfloatersettingsdebug.cpp` (debug floater 全表示) と
`indra/newview/llcontrolavatar.cpp` (avatar control) を除外して
`indra/newview/**/*.{cpp,h}` を grep した結果。

| cvar | 型 | AY default | render-path site | category |
|---|---|---|---|---|
| `RenderChromaStrength` | F32 | 5.0 | pipeline.cpp | A: 色補正 (r14+) |
| `RenderColorBrightness` | F32 | 0.0 | pipeline.cpp, llfloaterpreference.cpp | A: 色補正 (r14+) |
| `RenderColorContrast` | F32 | 1.0 | pipeline.cpp, llfloaterpreference.cpp | A: 色補正 (r14+) |
| `RenderColorGradingLUTIntensity` | F32 | 1.0 | pipeline.cpp | A: 色補正 (r14+) |
| `RenderColorGradingLUTName` | String | `` | pipeline.cpp, llfloaterpreference.cpp | A: 色補正 (r14+) |
| `RenderColorSaturation` | F32 | 1.0 | pipeline.cpp, llfloaterpreference.cpp | A: 色補正 (r14+) |
| `RenderColorTemperature` | F32 | 0.0 | pipeline.cpp, llfloaterpreference.cpp | A: 色補正 (r17 Sun Kelvin) |
| `RenderDebugSH` | Boolean | 0 | **no render use** | C: dead |
| `RenderDepthOfFieldChroma` | Boolean | 1 | llviewercontrol.cpp, llviewershadermgr.cpp | A: DoF 拡張 (P4) |
| `RenderDepthOfFieldFront` | Boolean | 1 | llviewercontrol.cpp, llviewershadermgr.cpp | A: DoF 拡張 (P4) |
| `RenderDepthOfFieldHighQuality` | Boolean | 0 | llviewercontrol.cpp, llviewershadermgr.cpp | A: DoF 拡張 (P4) |
| `RenderJellyDollsAsImpostors` | Boolean | 1 | pipeline.cpp, llvoavatar.cpp | B: impostor 拡張 |
| `RenderMotionBlurOtherAvatars` | Boolean | 1 | lldrawpoolavatar.cpp, lldrawpool.cpp | A: motion blur (r30 P2) |
| `RenderMotionBlurSelfAvatar` | Boolean | 1 | lldrawpoolavatar.cpp, lldrawpool.cpp | A: motion blur (r30 P2) |
| `RenderResolutionMultiplier` | F32 | 1.0 | llviewercontrol.cpp, pipeline.cpp | B: 解像度 multiplier (SL:KB) |
| `RenderSMAAT2x` | Boolean | 0 | pipeline.cpp | A: SMAA T2x AA |
| `RenderSculptSAThreshold` | F32 | 150.0 | llvovolume.cpp | B: SA protection (AY) |
| `RenderShadowSoftness` | F32 | 1.0 | pipeline.cpp | B: shadow softness (AY) |
| `RenderVolumeSAFrameMax` | F32 | 5000.0 | llvovolume.cpp | B: SA protection (AY) |
| `RenderVolumeSAProtection` | Boolean | 0 | llvovolume.cpp | B: SA protection (AY) |
| `RenderVolumeSAThreshold` | F32 | 75.0 | llvovolume.cpp | B: SA protection (AY) |
| `RenderVolumetricLighting` | Boolean | 1 | lldrawpoolalpha.cpp, pipeline.cpp | A: volumetric light (r18) |
| `RenderVolumetricLightingDirectional` | Boolean | 1 | llviewershadermgr.cpp | A: volumetric light (r18) |
| `RenderVolumetricLightingFalloffMultiplier` | F32 | 1.0 | pipeline.cpp | A: volumetric light (r18) |
| `RenderVolumetricLightingMultiplier` | F32 | 50.0 | pipeline.cpp | A: volumetric light (r18) |
| `RenderVolumetricLightingResolution` | U32 | 16 | pipeline.cpp | A: volumetric light (r18) |

### 2.1 内訳

- Category A (r14+ AY 視覚表現 / r30 P2-P5): **18 cvar**
- Category B (AY 性能・解像度系拡張): **7 cvar**
- Category C (dead, 無視可): **1 cvar** (`RenderDebugSH`)

---

## 3 Cinematic mode dispatch 値 (BD_NOOP_DEFAULT)

BD には対応 cvar が無いため、BD で render される結果と同じになる「**機能 OFF / no-op**」値を hardcode する。

### 3.1 Category A (r14+ 視覚表現) — BD 全 disable

| cvar | BD_NOOP value | 理由 |
|---|---|---|
| `RenderChromaStrength` | `0.0f` | chroma 効果無し |
| `RenderColorBrightness` | `0.0f` | brightness 加算ゼロ |
| `RenderColorContrast` | `1.0f` | contrast 無補正 |
| `RenderColorGradingLUTIntensity` | `0.0f` | LUT 適用ゼロ |
| `RenderColorGradingLUTName` | `""` | LUT load 空 |
| `RenderColorSaturation` | `1.0f` | saturation 無補正 |
| `RenderColorTemperature` | `0.0f` | Kelvin shift ゼロ |
| `RenderDepthOfFieldChroma` | `false` | DoF chroma 無効 |
| `RenderDepthOfFieldFront` | `false` | front-of-focus blur 無効 |
| `RenderDepthOfFieldHighQuality` | `false` | BD original DoF quality |
| `RenderMotionBlurOtherAvatars` | `false` | 他 avatar motion blur 無効 |
| `RenderMotionBlurSelfAvatar` | `false` | self motion blur 無効 |
| `RenderSMAAT2x` | `false` | SMAA 無効 (BD は FSAAType で完結) |
| `RenderVolumetricLighting` | `false` | volumetric light 完全 disable |
| `RenderVolumetricLightingDirectional` | `false` | directional volumetric 無効 |
| `RenderVolumetricLightingFalloffMultiplier` | `1.0f` | falloff 無補正 (cvar 自体は no-op gate 内) |
| `RenderVolumetricLightingMultiplier` | `0.0f` | volumetric 強度ゼロ |
| `RenderVolumetricLightingResolution` | `16` | 値据置 (機能 off で無関係) |

### 3.2 Category B (AY 性能拡張) — Cinematic で BD parity 用 default

| cvar | BD_NOOP value | 理由 |
|---|---|---|
| `RenderJellyDollsAsImpostors` | `true` | BD も avatar impostor 機構あり、`true` で BD parity |
| `RenderResolutionMultiplier` | `1.0f` | 解像度 multiplier 無効 (BD は 1.0 相当) |
| `RenderSculptSAThreshold` | `150.0f` | AY 拡張だが Cinematic で BD parity 確保のため AY default 維持 |
| `RenderShadowSoftness` | `1.0f` | shadow softness 無補正 (BD は固定値) |
| `RenderVolumeSAFrameMax` | `5000.0f` | SA protection AY default 維持 |
| `RenderVolumeSAProtection` | `false` | BD には無い保護機構 → off |
| `RenderVolumeSAThreshold` | `75.0f` | SA threshold AY default 維持 |

### 3.3 Category C

- `RenderDebugSH` → 対象外 (render-path call site 無し)

---

## 4 Phase 3.5 deliverable

本 spec の §3.1 / §3.2 表を Phase 3.7 dispatch impl 時に `getRenderCvarXxx("...", BD_NOOP)` の
default 引数として直接参照する。Phase 3.5 自体は spec 確定のみで、コード変更無し。

ただし以下 2 機能群は **Phase 3.1 で既に予測 (predicate) 制御化** 済みなので、
本 spec の dispatch 適用は **冗長 fallback** として動作する (二重 gate)。

- r17 (Sun Kelvin) — `RenderColorTemperature` の上流 gate を `aya_visual_realism() != 1` に flip 済
- r18 (Cloud Volumetric) — `RenderVolumetricLighting*` の上流 gate を flip 済
- master uniform — `aya_master() == 1`

二重 gate は安全側 (= 上流 gate を Phase 3.7 で外しても下流 cvar dispatch で BD parity 維持) として残す。

---

## 5 Phase 3.7 への引継ぎ

Phase 3.7 で `LLPipeline` 以外のクラス (e.g., `LLVOVolume`, `LLDrawPoolAvatar` 等) からも
`LLPipeline::getRenderCvarXxx` を呼ぶことになるが、Phase 3.3 で **static method** として
公開済みのため include `pipeline.h` だけで呼出可能。

Phase 3.7 commit は **cvar group 単位** (色補正 / volumetric / DoF / motion blur / SA protection) で
分割する。1 group 1 commit + 1 build で進行。

---

## 6 自検 (warn rule)

- 「AY-only cvar は AY mode 1 では従来挙動を保持」を厳守 → helper は mode 0/1 で `gSavedSettings` を読む既存挙動を変えない (Phase 3.3 helper 仕様)
- 「Cinematic 用 preset を投入」案は採用しない (`feedback_bd_full_port_only.md` 違反)
- BD_NOOP default は **機能 OFF** = BD には存在しない機能を Cinematic で動かさない、を機械的 bind するだけ → 推論を含まない本線
