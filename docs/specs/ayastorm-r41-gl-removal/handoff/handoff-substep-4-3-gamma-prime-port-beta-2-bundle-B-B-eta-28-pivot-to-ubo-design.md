# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-α 適用後 UBO 全体設計 pivot handoff

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-prep.md` (起草 → 訂正、Phase 2d-α 着手用)
**直近 reference doc**: `reference-shader-location-map.md` (η-28 Phase 2c 末 + Phase 2d-α 更新済)

---

## §0 本 handoff の目的 (= 次 session 着手地点)

η-28 Phase 2d-α (parse error 5 file 修正) を **適用済 (commit 済 / 未 push / 未 verify)** の状態で、AYA 指示により以下に **pivot** する:

> **「Vulkan エラー潰しを設計無しで継続している現状を止め、UBO 全体設計 doc を起草する」**

次 session では本 handoff の §2 (現 26 UBO 一覧と各起源理由) を **議題 1**、§3 (設計議題リスト) を **議題 2** として AYA さんと議論する。Phase 2d-α 適用済 commit (5 file feat + 3 docs) は **push 保留 / cold launch verify 保留**、UBO 全体設計の方針が固まってから処遇判断する。

---

## §0.5 pivot に至った経緯 (本 session 末でのやりとり要約)

1. 前 session で Phase 2c push 完了 → Phase 2d-α prep doc 起草着手
2. Prep doc §6-A self-verify 中、Agent 2-stage trace で **`LLGLSLShader::uniform*fv()` (`llglslshader.cpp:2166-2554`) に Vulkan UBO redirect 層が無い** ことを発見 (Vulkan path でも `glUniform*` を直接呼んでいる)
3. AYA さんに報告 → 設計原則 2 件と「1 UBO ずつ」方針を受領 (memory 保存: `project_ayastorm_r41_design_principles.md` / `feedback_ubo_migration_one_at_a_time.md`)
4. host C++ UBO redirect 層は **Phase 3 scope** に切出 (prep doc §0.5 で確定)、Phase 2d-α は **GLSL parse pass のみ** に scope 縮小
5. 5 file feat 編集適用 (commit `0587c574da`)
6. reference doc 更新 (commit `9a576884c0`)
7. AYA さんから「**1 UBO ずつ**とあるが、そもそも UBO は何個ある前提か?」の質問 → 「現 26 + Phase 2d 以降増加見込み」と回答
8. AYA さんから「**設計もしないで Vulkan エラーを取ることに躍起になっている**」 + 「UBO 全体設計 doc 起草に切り替え」 + 「**26 個にした理由などを知りたいので handoff して次 session で議論**」の指示
9. 本 handoff 起草 (= 本 doc)

**自己評価**: 私は前 session で「parse error 1 つ消す → 次の error が出る → また消す」を反復し、η-24 から η-28 Phase 2c までで設計無しに UBO を 26 個積み上げた。`feedback_build_only_verified` / `feedback_render_full_trace_first` 両方への違反。

---

## §1 2 大設計原則 (memory `project_ayastorm_r41_design_principles.md` の再掲)

本 handoff 後の UBO 全体設計はこの 2 原則に **必ず** 従う:

### 原則 1: Upstream OpenGL 取り込みやすさ維持

Firestorm / SL upstream は今後も OpenGL ベースで commit が降ってくる。AYAstorm 側の Vulkan 化が **call site API (host C++ の `glUniform*fv()` 呼出箇所、shader 側の uniform 参照箇所) を温存** していないと、upstream 取込時に conflict / migration cost が爆発する。

**具体的要求**:
- `LLGLSLShader::uniform4fv("color", ...)` のような **既存呼出形** はそのまま使えること
- shader 側 `uniform vec4 color;` の参照箇所 (本体 GLSL コード) は **shader 内部の宣言形** だけ変わって本体は不変であること
- UBO 化は **内部の翻訳層** で吸収 (= host C++ 側の uniform 名 → UBO offset の自動 mapping、shader 側の `#ifdef LL_VULKAN_GLSL` で宣言だけ UBO 化)

### 原則 2: Core プロセス分散実現

r41 Vulkan 化の **真の目的** は単なる API 移行ではなく、**CPU multi-core 分散による perf 向上**。UBO 設計はこれを **後で容易に乗せられる構造** でなければならない。

**具体的要求**:
- UBO 更新は **worker thread から実行可能** な形 (= memcpy ベースで OpenGL context 不要)
- batch upload (= 描画 call 直前に dirty UBO だけ単発 upload) で **draw call 間の独立性** を確保
- UBO の **寿命 (per-frame / per-program / per-draw)** が明確で、更新頻度に応じた buffer 配置 (persistent mapped / triple buffered / dynamic) を後で選択可能

### 原則の含意 (= 設計判断時の優先順位)

- **call site API 温存** > 内部実装の綺麗さ
- **寿命分類の明確化** > UBO 個数の節約
- **worker thread 安全性** > driver hint 最適化

---

## §2 現 26 UBO 一覧 + 各起源理由 (= AYA さんが知りたい議題 1)

set=2 帯 (PerDrawUBO / PerProgramUBO) の binding 0-25 を時系列順 (起源 sub-step 順) に列挙。**「なぜ単独 UBO として切り出したか」** = 各 phase で発生していた `non-opaque uniforms outside a block` parse error 対応として、その program 単独に新規 binding を割り当てた、というのが事実上の **唯一の理由**。設計から導いた粒度ではない。

### §2.1 per-draw 帯 (binding 0-1、寿命 = 描画 call 毎)

| binding | UBO 名 | 起源 | 切出理由 |
|---|---|---|---|
| 0 | `PerDrawUBO_LightParams` | η-3 (2025-XX) | r41 initial backbone。`color` / `falloff` / `size` / `light_direction` 等の per-light 値を 1 ブロックに統合。複数 light shader (pointLight / spotLight / multiSpotLight) で **共有** する想定で per-draw set=2 binding=0 に確保 |
| 1 | `PerDrawUBO_MultiLight` | η-23 | multiSpotLight 系で `light[N]` / `light_col[N]` / `light_count` 等の array 群を切出 |

### §2.2 PerProgram 帯 - η-24 期 (binding 2、寿命 = program bind 時 1 回)

| binding | UBO 名 | 起源 | 切出理由 |
|---|---|---|---|
| 2 | `PerProgramUBO_GammaCorrect` | η-24 | gammaCorrectF program の `gamma` plain uniform が parse error → 単独 program 用 UBO 化。η-24 期から **「1 program = 1 UBO」** 方針 (= 設計でなく現場判断) が暗黙成立 |

### §2.3 PerProgram 帯 - η-25 期 (binding 3-5)

| binding | UBO 名 | 起源 | 切出理由 |
|---|---|---|---|
| 3 | `PerProgramUBO_AlphaParams` | η-25 Phase 1a | alpha 系 (alphaF) の `minimum_alpha` 等 |
| 4 | `PerProgramUBO_ColorGrading` | η-25 Phase 1b | exoColorGradeF 等の grading params |
| 5 | `PerProgramUBO_PointLightV` | η-25 Phase 1c | pointLightV 単独 (trans_center 等)、F 側は §2.2 共用想定だったが η-28 Phase 2c で binding=25 に切出済 |

### §2.4 PerProgram 帯 - η-26 期 (binding 6-8)

| binding | UBO 名 | 起源 | 切出理由 |
|---|---|---|---|
| 6 | `PerProgramUBO_ShadowAlphaMaskV` | η-26 Phase 1a | shadowAlphaMaskV の skin/non-skin permutation 共有 |
| 7 | `PerProgramUBO_PostDeferredV` | η-26 Phase 1b | postDeferredV (postDeferredGammaCorrect 系 V) |
| 8 | `PerProgramUBO_FullbrightShinyV` | η-26 Phase 1c | fullbrightShinyV |

### §2.5 PerProgram 帯 - η-27 期 (binding 9-12)

| binding | UBO 名 | 起源 | 切出理由 |
|---|---|---|---|
| 9 | `PerProgramUBO_FxaaF` | η-27 Phase 1a | fxaaF (rcpFrame / rcpFrameOpt 等) |
| 10 | `PerProgramUBO_SpotLightF` | η-27 Phase 1d + 1e-A (+ η-28 Phase 2d-α で field 追加) | spotLightF (proj_origin / proj_n / proj_p / proj_focus / proj_lod / proj_ambient_lod / proj_ambiance / proj_shadow_idx 等)。η-28 Phase 2d-α で `vec3 center` (MULTI_SPOTLIGHT permutation 用) も吸収 (η-28-C type 3 範式) |
| 11 | `PerProgramUBO_PbrAlphaV` | η-27 Phase 1c | pbralphaV non-skin path |
| 12 | `PerProgramUBO_PostDeferredNoDoFF` | η-27 Phase 1b | postDeferredNoDoFF |

### §2.6 PerProgram 帯 - η-28 Phase 2a 期 (binding 13-16)

| binding | UBO 名 | 起源 | 切出理由 |
|---|---|---|---|
| 13 | `PerProgramUBO_FsObjectIdF` | η-28 Phase 2a | FS Object ID picker shader (r21 self picker 由来の AYAstorm 専用 program) |
| 14 | `PerProgramUBO_ShadowCubeV` | η-28 Phase 2a | shadowCubeV (`box_center` / `box_size` の vec3 pair) |
| 15 | `PerProgramUBO_WaterHazeV` | η-28 Phase 2a (η-28-C type 1 範式確立) | waterHazeV + waterHazeF **両 stage 共有** UBO の初例 (`above_water` int 1 個)。Phase 2a self-trace で V 単独 UBO 化では F 側 cascade error 浮上見込みだったため両 stage に同名宣言で予防 |
| 16 | `PerProgramUBO_VisualizeBuffersF` | η-28 Phase 2a | visualizeBuffersF (debug 用 buffer 可視化) |

### §2.7 PerProgram 帯 - η-28 Phase 2b 期 (binding 17-20)

| binding | UBO 名 | 起源 | 切出理由 |
|---|---|---|---|
| 17 | `PerProgramUBO_GodraysF` | η-28 Phase 2b | godraysF |
| 18 | `PerProgramUBO_VolumetricLightF` | η-28 Phase 2b | volumetricLightF |
| 19 | `PerProgramUBO_VelocityAlphaV` | η-28 Phase 2b | velocityAlphaV (`mat4 last_object_matrix`) |
| 20 | `PerProgramUBO_PostDeferredF` | η-28 Phase 2b (η-28-C type 2 範式確立) | postDeferredF + postDeferredHQDoFF **cross-variant 共有**。cvar 切替で実体が 2 file に分かれるが同 program slot として扱われるため共有宣言 |

### §2.8 PerProgram 帯 - η-28 Phase 2c 期 (binding 21-25)

| binding | UBO 名 | 起源 | 切出理由 |
|---|---|---|---|
| 21 | `PerProgramUBO_CofF` | η-28 Phase 2c | cofF (DoF circle of confusion、float × 6: depth_cutoff / norm_cutoff / focal_distance / blur_constant / tan_pixel_angle / magnification) |
| 22 | `PerProgramUBO_BlurLightF` | η-28 Phase 2c | blurLightF (SSAO blur、float `kern[4]` 等) |
| 23 | `PerProgramUBO_WaterF` | η-28 Phase 2c | waterF (`blend_factor` 等) |
| 24 | `PerProgramUBO_PbrTerrainV` | η-28 Phase 2c (η-28-C type 3 範式確立) | pbrterrainV **permutation 共有** (PAINTMAP 有無で 2 permutation、`terrain_texture_transforms[5]` 等) |
| 25 | `PerProgramUBO_PointLightF` | η-28 Phase 2c | pointLightF (`size` / `color` / `falloff` 等、当初 binding=0 PerDrawUBO_LightParams 共用想定だったが Phase 2c で個別必要が判明し切出) |

### §2.9 26 個に至った構造的問題 (= 設計が無い証拠)

**問題 1: 1 program = 1 UBO の暗黙化**
- η-24 で初例として gammaCorrectF 単独 UBO 化したのを起点に、以降 26 phase 全部「parse error 出た program 単独に新 binding 割当」を反復
- 結果: 同種寿命 (per-program load 時 upload) の UBO が 24 個並列、host C++ 側で 24 回 `glBindBufferBase` する未来

**問題 2: 寿命分類が name suffix だけ**
- `<...>V` / `<...>F` suffix は **起源 stage** を示すだけで attach 範囲ではない (reference doc §6-A 注意書きで明示済)
- per-draw / per-program の使い分けは `PerDrawUBO_` / `PerProgramUBO_` 接頭辞のみだが、これは **更新頻度の意図** であって host 側更新コードがそれに従っている保証はない

**問題 3: cross-variant / permutation / stage 跨ぎの 3 形態を後付け吸収**
- η-28-C type 1 (V+F 跨ぎ、waterHaze) / type 2 (cvar variant 跨ぎ、postDeferred) / type 3 (preprocessor permutation 跨ぎ、pbrterrain / spotLight) は **後から発生事例として分類** したもので、最初から設計したわけではない
- 設計があれば「stage / variant / permutation を跨ぐ UBO はこう扱う」を **先に決めて** 各 phase で適用するだけのはず

**問題 4: 既存 set=3 Legacy 帯 (32 個) との未統合**
- reference doc §6-D に列挙された `<Name>UBO_Legacy` は η-6/η-13 期 grouping で **32 個** 存在 (set=3 binding 0-61 散発)
- 「set=2 PerProgramUBO への移行候補」と reference doc §6-D に書いているが、移行判断基準は無い
- 結果: 同種 program param が set=2 (新) と set=3 (旧 Legacy) に **二重存在** する可能性 (個別 trace 未実施)

**問題 5: set=0 (`Frame*`) / set=1 (`MaterialUBO`) との接続曖昧**
- per-frame 寿命 (set=0) は `FrameViewProj` / `FrameLights` / `FrameAtmosphere_Lighting` の 3 個と一部 sampler のみ観測 (reference doc §6-B、要全件棚卸し明記)
- set=1 `MaterialUBO` は per-program material 寿命と説明あるが、set=2 `PerProgramUBO_*` との **役割分担基準が無い**

**問題 6: host C++ redirect 層が無い** (前 session §0.5 で発覚)
- 上記 1-5 の累積結果として、26 個の UBO は **GLSL 側にしか存在しない**
- host C++ `LLGLSLShader::uniform*fv()` (`llglslshader.cpp:2166-2554`) は Vulkan path でも `glUniform*` を呼んでおり、shader 側の UBO 化と完全乖離
- = **Vulkan で実 build しても 26 UBO に値が入らない**

---

## §3 設計議題 (= 次 session の焦点)

§2 を起点に、次 session で AYA さんと以下を決める。優先順位順 (上から決まらないと下が決められない):

### §3.1 寿命分類 (= UBO grouping の第一義基準)

- **per-frame**: frame 開始時 1 回 upload (view/proj matrix / sun direction / atmospherics 等)
- **per-view**: per-frame の派生 (cube map 6 face / shadow map 6 face / probe 等で複数 view を frame 内で巡る場合、view 単位で値が変わる)
- **per-program**: program bind 時 1 回 upload (固定 const に近い param、e.g., gamma / fxaa rcpFrame)
- **per-material**: material 単位 (per-program と区別必要か?既存 set=1 `MaterialUBO` の扱い)
- **per-draw**: 描画 call 毎 (mTransform / per-light params / per-mesh color 等)

**議題**: この 5 分類で過不足ないか / 既存 26 UBO + 32 Legacy + 3 Frame を全部この分類に再 mapping できるか。

### §3.2 host C++ redirect 層の枠組み (= 原則 1「call site API 温存」の実装手段)

prep doc §0.5 で起草した方向:

```
LLGLSLShader::uniform4fv("color", ...) 呼出
  ↓
shader 内部 metadata で "color" → {ubo_index=PerProgramUBO_SpotLightF, offset=64, size=16} を lookup
  ↓
ubo_index 番目の dirty buffer (host memory copy) に memcpy
  ↓
dirty flag を立てる
  ↓
draw call 直前に dirty な UBO だけ batch upload (glBufferSubData / mapped memory write)
```

**議題**:
- metadata の lookup は **shader load 時に GLSL parse → reflection で自動生成** か / **手書き宣言** か
- dirty flag の粒度は UBO 単位か member 単位か (member 単位なら partial update、UBO 単位なら whole upload)
- 既存 `MaterialUBO_Legacy` / `<...>UBO_Legacy` の 32 個も同じ redirect に乗せるか / 別レイヤーか
- worker thread (= 原則 2) は redirect 層のどこで動くか

### §3.3 既存 26 UBO の存置 / 再構成判断基準

§2 で列挙した 26 個を §3.1 寿命分類に再 mapping した結果、

- **そのまま残す**: 寿命分類と現状の binding 割当が整合する UBO
- **統合する**: 同寿命 + 同 program で複数 UBO に分かれているもの (e.g., binding=10 SpotLightF と binding=25 PointLightF を per-program light UBO として統合?)
- **昇格 / 降格する**: per-program だが実は per-frame で十分なもの (e.g., gamma) や、その逆
- **削除する**: 役割重複 (set=3 Legacy と内容が同じもの)

判断は §3.1 寿命分類確定後にしかできない。

### §3.4 Phase 番号体系の再設計

現状の η-X / Phase N の体系は「次に parse error が出た program を順に処理」の連番だが、設計 pivot 後は:

- **Phase α**: UBO 全体設計 doc (本 handoff の発展形) 確定
- **Phase β**: host C++ redirect 層実装 (1 寿命分類ずつ、e.g., per-draw → per-program → per-frame)
- **Phase γ**: GLSL 側 UBO 再構成 (§3.3 判断に従い既存 26 + 32 Legacy を整理)
- **Phase δ**: 残り未対応 program の UBO 化 (現 η-28 Phase 2d 以降が該当)
- **Phase ε**: cold launch verify + Vulkan 実 build 検証

の体系に組み直す必要あり。

### §3.5 Phase 2d-α 適用済 commit の扱い

§2/§3 の設計が固まった後、以下のどれを選ぶか:

- **(A) push して cold launch verify → 結果次第で残置 / revert**: 動作中の 5 program parse fail 解消は事実なので、設計が決まる前でも前進として残せる可能性
- **(B) push 保留のまま残し、設計確定後に再評価**: GLSL 編集のみで host 側は無影響なので、push しなくても害は無い
- **(C) revert する**: 設計と齟齬あるなら綺麗に巻き戻して設計に従って書き直す

§3.1〜3.4 が決まらないと判断できない。

---

## §4 Phase 2d-α 適用済の状態 (= 何が動いていて何が止まっているか)

### §4.1 GLSL 5 file 編集 (commit `0587c574da`)

| file | 編集内容 | 修正対象 Issue |
|---|---|---|
| `class3/deferred/pointLightF.glsl` | 自己 PerDrawUBO_LightParams 宣言削除 + body の `size` → `spot_light_size`、`color` → `spot_light_color` (Vulkan path のみ rename、`#ifdef LL_VULKAN_GLSL` 分岐) | Issue B (`size` undeclared at L2439 in transformed dump、η-28-E 範式: deferredUtil alias scope refinement) |
| `class3/deferred/spotLightF.glsl` | PerProgramUBO_SpotLightF に `vec3 center + float _pad_center` chunk 3 追加、`uniform vec3 center;` を `#ifndef LL_VULKAN_GLSL` で wrap、自己 PerDrawUBO_LightParams 削除、`color.rgb` → `spot_light_color.rgb` rename | Issue E + Issue F (MULTI_SPOTLIGHT permutation で `center` 参照、η-28-C type 3 範式) |
| `class1/deferred/pbrterrainF.glsl` | L47-51 `struct TerrainMix` を `#ifndef TERRAIN_MIX_DEFINED` guard wrap | Issue C (TerrainMix struct redefinition at L1750 in transformed dump、η-28-F 範式: struct redefinition guard wrap) |
| `class1/deferred/pbrterrainUtilF.glsl` | L183-187 同じ guard wrap (addCommonShader 経由で全 frag program に attach されるため) | Issue C 連動 |
| `class1/interface/pbrTerrainBakeF.glsl` | L34 同じ guard wrap (現状 redef 未発生だが将来 common shader 経路の予防) | 予防修正 (η-28-F 範式 future-proofing) |

### §4.2 reference doc 更新 (commit `9a576884c0`)

- L302: binding 10 `PerProgramUBO_SpotLightF` の起源 sub-step に η-28 Phase 2d-α 追加 (vec3 center field 吸収 + η-28-C type 3 範式の実証)
- §6-E (η-28 Phase 2c 末まで存在しなかった節) として η-28-E (deferredUtil alias scope refinement) + η-28-F (struct redefinition guard wrap) 範式追加

### §4.3 prep doc (commit `fd4dbb71aa` + `9acf70acdf`)

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-prep.md` (840 行)
- §0.5 で host C++ UBO redirect 層を Phase 3 scope に切出した経緯記録あり
- 本 pivot で **本 prep doc 自体の前提が変わった** ため、次 session で再評価必要

### §4.4 未実施事項

- push (`git push origin feature/ayastorm-r41-gl-removal`) → AYA 担当だが本 pivot により保留
- AYA cold launch 起動 (deploy → cache clear → run → log 取得)
- Claude self-verify (reference doc §8 cookbook、parse fail 7→2 / link 0 / 5 program SPIR-V PASS 期待)
- Phase 2d-α complete handoff 起草

すべて **本 pivot 後の設計判断 (§3.5) が出るまで保留**。

---

## §5 本 session の commit 一覧 (時系列)

| hash | 種別 | 内容 |
|---|---|---|
| `fd4dbb71aa` | docs | Phase 2d-α prep handoff 起草 (815 行) |
| `9acf70acdf` | docs | Phase 2d-α prep §0.5 新設 + §6-A/§6-E 訂正 (host UBO redirect を Phase 3 scope 切出) |
| `0587c574da` | feat | Phase 2d-α 5 file 適用 (pointLightF / spotLightF / pbrterrainF / pbrterrainUtilF / pbrTerrainBakeF) |
| `9a576884c0` | docs | reference doc 更新 (binding 10 note + η-28-E/F 範式追加) |

(Phase 2c push は前 session 末で完了済、本 session 開始時の起点)

---

## §6 次 session 開始 protocol

### §6.1 必読 file 順

1. **本 handoff doc** (= 本 file) - 全体把握
2. **`reference-shader-location-map.md`** - 既存 26 UBO + 32 Legacy + Frame の確定マップ、特に §6-A〜E
3. **memory `project_ayastorm_r41_design_principles.md`** - 2 大設計原則
4. **memory `feedback_ubo_migration_one_at_a_time.md`** - 「1 UBO ずつ」方針 (ただし本 pivot で解釈見直し必要)
5. **`handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-prep.md`** §0.5 - host C++ redirect 層の前 session 起案 (本 pivot で更に発展させる起点)

### §6.2 議題進行順 (= §3 の優先順位)

1. **§3.1 寿命分類確定** (per-frame / per-view / per-program / per-material / per-draw の 5 分類で妥当か)
2. **§3.2 host C++ redirect 層の枠組み確定** (metadata lookup の自動 vs 手書き / dirty flag 粒度 / worker thread 位置)
3. **§3.3 既存 26 UBO + 32 Legacy + 3 Frame の再 mapping** (= §3.1 を当てはめる作業)
4. **§3.4 Phase 番号体系再設計** (α/β/γ/δ/ε の妥当性)
5. **§3.5 Phase 2d-α 適用済 commit の処遇判断** (push / 保留 / revert の 3 択)

### §6.3 アウトプット成果物 (議論結果の保存先)

- 新規 doc: `ayastorm-r41-ubo-overall-design.md` (live document、本 handoff の §2/§3 を発展させた永続資料)
- memory 更新: `feedback_ubo_migration_one_at_a_time.md` の「1 UBO ずつ」表現を再定義 (§3.1 寿命分類の中で「1 寿命分類ずつ」 or 「1 program ずつ」のどちらに解釈し直すか)
- reference doc 更新: §6 全節の整合確認 (新規 design doc から逆参照を張る)

### §6.4 議論で詰まったとき

- AYA さんが「全 UBO 一覧を見たい」と言われたら → §2 を順に提示
- AYA さんが「なぜ 26 個になった?」と聞かれたら → §2.9 の構造的問題 6 件を提示
- AYA さんが「で、何が間違ってる?」と聞かれたら → §0.5 の自己評価 (parse error 反復で設計無し) を率直に提示
- AYA さんが「Phase 2d-α は要らないのか?」と聞かれたら → §3.5 の 3 択を提示、設計確定後に決まる旨説明

---

## §7 本 handoff の制約 (= 次 session 開始時に Claude が忘れがちな点)

- 本 pivot は **「parse error 潰しを止める」のではない**、「**設計無しの parse error 潰しを止める**」。設計が固まれば parse error 潰し作業は再開する (= Phase δ 相当)
- **既存 26 UBO は全部捨てる前提ではない**。§3.3 で「そのまま残す」判断が出るものは残す
- **AYA さんは Vulkan / GLSL の細部より、設計の全体像 (寿命分類 / call site API 温存 / Core 分散) に焦点がある**。技術詳細を並べすぎないこと
- `feedback_no_scope_shrink` (memory): AYA さん「設計しろ」の literal scope を「Phase 2d-α 適用済の範囲だけ」に縮小しない。**26 個全部** + Legacy 32 個 + Frame 3 個も対象
- `feedback_doubt_self_first` (memory): 「parse error が出たから UBO 切出」の現場判断 reflex に戻りそうになったら自分を疑う

---

## §8 補足: 数値現状 (η-28 Phase 2c 末時点、Phase 2d-α 適用後)

| 帯 | 個数 | 出所 |
|---|---|---|
| set=0 `Frame*` UBO | 3 観測 + sampler 混在 | reference doc §6-B (要全件棚卸し明記) |
| set=1 `MaterialUBO` | 1 観測 + sampler 多数 | reference doc §6-C |
| set=2 `PerDrawUBO_*` | 2 (binding 0-1) | 本 doc §2.1 |
| set=2 `PerProgramUBO_*` | 24 (binding 2-25) | 本 doc §2.2-2.8 |
| set=3 `<Name>UBO_Legacy` | 32 (binding 0-61 散発) | reference doc §6-D |
| **合計 UBO** | **62 + α** (要棚卸し) | |

**= 設計対象は 26 個ではなく 62+ 個**。本 handoff で「26 個」と語っているのは **本 session で議論の起点となった set=2 帯のみ**。次 session では Legacy 32 個 + Frame 3 個も統合議題。

---

**本 handoff は r41 UBO 全体設計確定まで永続参照**。次 session で `ayastorm-r41-ubo-overall-design.md` (新規 live doc) 起案後、本 handoff は **設計 pivot の経緯資料** として as-is 保存 (rebase / 整理しない、`project_fix_double_alpha_block_branch` 範式と同じ)。
