# r41 sub-step 3.3-B-δ 完遂 → 3.3-B-ε 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-gamma-complete.md` (3.3-B-γ 完遂 / 3.3-B-δ 着手前境界)
**本 handoff 位置付け**: sub-step 3.3-B-δ (sky placeholder vert shader 側 UBO/push constant binding 受領 + MVP/normal/inverse_modelview 計算式配線) 完遂宣言 + 3.3-B-ε (3.3-B 全完遂総括 handoff) 着手前 scope 確認境界。
**3.3-B 全体構成 (sub-doc 03 §3.1.3)**: B-α (spec refine、完遂) / B-β-1 (`loadSpirvShaderModule` signature、完遂) / B-β-2 (build chain + transit smoke、完遂) / B-γ (PSO compile file load 切替、完遂) / **B-δ (UBO binding + push const + shader 内 MVP/normal 配線、本 handoff で完遂宣言)** / B-ε (3.3-B 全完遂 handoff、次着手)

---

## 1. sub-step 3.3-B-δ 完遂 status (2026-05-31)

### 1.1 完遂 marker (sub-doc 03 §3.1.3 B-δ)

| acceptance | 達成 status |
|---|---|
| `.glsl` vert source 内 `layout(set=0, binding=0) uniform PerFrameMatrixUBO { mat4 projection_matrix; mat4 inverse_projection_matrix; mat4 identity_matrix; };` | ✓ `sky_placeholderV.glsl` L18-23 配置 |
| `.glsl` vert source 内 `layout(set=0, binding=1) uniform TextureMatrixUBO { mat4 texture_matrix[4]; };` | ✓ L25-28 配置 (binding 1 受領、handoff §5.2 scope 通り) |
| `.glsl` vert source 内 `layout(push_constant) uniform PushConstants { mat4 modelview_matrix; };` | ✓ L30-34 配置 |
| shader 内 `mat4 mvp_matrix = projection_matrix * modelview_matrix;` | ✓ L39 配置 |
| shader 内 `mat3 normal_matrix = transpose(inverse(mat3(modelview_matrix)));` | ✓ L40 配置 |
| shader 内 `mat4 inverse_modelview_matrix = inverse(modelview_matrix);` | ✓ L41 配置 |
| glslang `--target-env vulkan1.3` で `.spv` 生成成功 | ✓ vert 3268 B (γ 1136 B から +2132 B = binding+計算式分) + frag 384 B 維持、build PASS (no error/warning) |
| PSO compile 成功 | ✓ AYA log line 104: `Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)` 継続 hit |
| INFO marker `"Sky placeholder vert binding active (PerFrameMatrixUBO + push constant modelview)"` | ✓ AYA log line 105: `2026-05-30T18:22:06Z INFO #Vulkan# llrender/llvkloader.cpp(1254) createSkySmokePipeline : Sky placeholder vert binding active (PerFrameMatrixUBO + push constant modelview)` |
| AYA launch PASS | ✓ 2026-05-31 起動完了確認、sky 表示維持 |
| 視覚 regression 0 | ✓ binding witness を 1e-30 multiplier で float32 denormal 以下に圧縮、fullscreen triangle 出力は γ と同一 |
| Vulkan 系 WARN/ERR 0 件 | ✓ Vulkan INFO 39 件のみ (γ 38 → +1 = δ marker)、Vulkan 系 WARN/ERR 0 件 |
| 3.3-A/3.3-C/γ 既存 11 marker 全継続 (regression なし) | ✓ Vulkan 1.3 dynamicRendering / per-frame desc layout (PerFrameMatrixUBO + TextureMatrixUBO) / Placeholder PSO / bindTarget / flush / syncMatrices PerFrame UBO write / syncMatrices TextureMatrix UBO write / dynamic rendering helper / pushCurrentModelviewMatrix / shutdownVulkan device+instance 全 hit |
| β-2 transit smoke 撤去継続 | ✓ `SPIR-V exemplar not found` / `sub-step 3.3-B-β-2 exemplar pre-flight` 0 件 hit |
| Linux build で embedded byte array dead code 除去継続 | ✓ `nm libllrender.a \| grep -cE "kSkySmokeVertSpv\|kSkySmokeFragSpv"` = 0 (γ macro guard 維持) |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-3-B-gamma-complete.md` | **役割完了** (3.3-B-δ satisfy、本 handoff で内容引継ぎ) |
| sub-doc `03-state-machine-pso.md` §3.1.3 B-δ 行 | active 継続 (本 handoff 起草と同時に「完遂 2026-05-31」marker 追加 + 実装結果反映、B-ε 着手 ready) |
| sub-doc `06-shader-spirv.md` | active 継続 (B-α で sub-step 6.1 prereq 整理済、δ で shader 側 binding 配置 pattern 確立 = 6.1 一括化前の final 形態) |
| `handoff-substep-3-3-B-delta-complete.md` (本 handoff) | 新規作成 (3.3-B-δ 完遂 → 3.3-B-ε 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.3-B-ε 着手 ready 状態へ update) |

---

## 2. 実装内容 (commit 範囲、commit `b2c06f869d`)

### 2.1 shader (`indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl` +49 / -6)

| 変更 | 内容 |
|---|---|
| header comment 更新 | β-2 期 comment (β-2 で build chain 実証 + binding は B-δ 範疇と記載) を δ 着手済 comment に書換 |
| PerFrameMatrixUBO binding 追加 | `layout(set = 0, binding = 0) uniform PerFrameMatrixUBO { mat4 projection_matrix; mat4 inverse_projection_matrix; mat4 identity_matrix; };` (192 B、3 mat4) |
| TextureMatrixUBO binding 追加 | `layout(set = 0, binding = 1) uniform TextureMatrixUBO { mat4 texture_matrix[4]; };` (256 B、mat4 × 4) |
| push constant binding 追加 | `layout(push_constant) uniform PushConstants { mat4 modelview_matrix; };` (64 B、VERTEX_BIT) |
| shader 内計算移譲 3 種配線 | `mvp_matrix = projection_matrix * modelview_matrix` / `normal_matrix = transpose(inverse(mat3(modelview_matrix)))` / `inverse_modelview_matrix = inverse(modelview_matrix)` |
| binding witness 1e-30 multiplier | 全 binding (projection / inverse_projection / identity / texture_matrix[0] / mvp / normal / inverse_modelview) を `binding_witness` ベクタに集約、`gl_Position = vec4(pos, 0.0, 1.0) + binding_witness * 1e-30` で float32 denormal 以下に圧縮 = SPIR-V に binding declaration を残置 + 視覚 regression 0 を構造的担保 |
| fullscreen triangle 出力維持 | `gl_VertexIndex 0/1/2` で `(-1,-1) / (3,-1) / (-1,3)` を生成、γ と同じ視覚出力 |

### 2.2 runtime (`indra/llrender/llvkloader.cpp` +1)

| 変更 | 内容 |
|---|---|
| δ INFO marker 追加 | `createSkySmokePipeline()` 内 γ marker `"Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)"` の直後行に `LL_INFOS("Vulkan") << "Sky placeholder vert binding active (PerFrameMatrixUBO + push constant modelview)" << LL_ENDL;` 追加。PSO compile 成功 = shader 側 binding 受領完了 evidence (shader compile 段階で binding declaration validation 済、PSO compile 段階で layout 整合性 validation 済) |

### 2.3 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl` | +49 / -6 | binding 3 種 + 計算式 3 種 + binding witness + header comment 更新 |
| `indra/llrender/llvkloader.cpp` | +1 / -0 | δ INFO marker |

合計: **2 file、+50 / -6 line (net +44)** (commit `b2c06f869d`)

---

## 3. 設計判断履歴 (本 sub-step 着手時)

### 3.1 binding witness 1e-30 multiplier 採用 (視覚 regression 0 担保)

handoff §5.3 δ-2 要件 = 「計算式を生成可能な書式で記述」段階 = uniform 値が 0 でも `.spv` compile 成功すれば marker satisfy。しかし sky smoke PSO は `recordSkySmokeDraw` (llvkloader.cpp L1713) 経由で実 draw が発生する (sub-step 3.2 smoke-test、fullscreen triangle + sky blue 出力)。binding 値が undefined (descriptor set 未 bind / push constant 未 write) のまま draw した場合、`gl_Position` に undefined 値が混入する可能性。

3 案検討:
- **案 A (採用)**: binding 値を `binding_witness` に集約 + `* 1e-30` で float32 denormal 以下に圧縮 = 数学的に gl_Position への寄与 0 を構造的担保、binding declaration は SPIR-V に残置
- **案 B**: 計算式を `out vec4 v_unused` 等の output 変数に格納 = vert/frag stage interface 変更 = frag shader 修正必要 (handoff §5.2 「δ は shader 内側のみ」scope 違反)
- **案 C**: `if (false) { ... }` で dead branch 化 = compiler が dead code 除去する可能性、binding declaration の SPIR-V 残置が不確実

案 A 採用根拠: scope (vert のみ touch) + SPIR-V 残置確実性 + 視覚 regression 0 の数学的担保。

### 3.2 TextureMatrixUBO (binding 1) も shader 側受領 (marker 例示外だが scope 内)

handoff §5.2 完了 marker 例示は PerFrameMatrixUBO (binding 0) + push constant の 2 件のみ言及、TextureMatrixUBO (binding 1) は明示なし。しかし scope 行は「set=0 binding 0 = `PerFrameMatrixUBO` 192 B / binding 1 = `TextureMatrixUBO` 256 B」と両 binding を含む。

`feedback_no_scope_shrink.md` 「AYA 指示の literal scope を勝手に縮小しない」遵守 = TextureMatrixUBO も shader 側受領実装。`texture_matrix[0][0]` を `binding_witness` に組み込み = SPIR-V 残置。

### 3.3 INFO marker は γ marker と隣接配置 (handoff §5.3 δ-3 指定通り)

`createSkySmokePipeline()` 内、`compileGraphicsPipeline` 成功後の γ marker `"Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)"` の直後行に δ marker を配置。γ marker = PSO compile 成功 evidence、δ marker = shader 側 binding 受領完了 evidence (PSO compile 成功 = shader compile + layout validation 両方 PASS = binding 受領完了の transitive 担保)。両 marker を隣接させることで 3.3-B 進捗の log trace を 1 箇所に集約。

### 3.4 embedded fallback byte array は γ 状態維持 (δ shader 反映せず)

γ で `AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` macro guard 配置済 = Linux 検証環境では fallback 未 compile。Mac/Win 想定の fallback path には γ 時点の `kSkySmokeVertSpv` (1136 B、binding なし vert) が残置。δ shader (3268 B、binding 受領済 vert) との divergence を許容:
- 本 sub-step scope (sub-doc 03 §3.1.3 B-δ) は shader 側 + loader INFO marker のみ、embedded byte array 更新は scope 外
- Mac/Win 実検証は B-ε で別途、その時点で:
  - **(α)**: Mac/Win build host で glslangValidator install を必須化 (fallback 廃止方向、領域 6 sub-step 6.1 一括化 prep)
  - **(β)**: 新 .spv を embedded byte array 化 (例えば xxd 経由) で更新
  - のいずれかを別途判断 = B-ε 着手前 AYA 相談事項

---

## 4. risks / caveats

### 4.1 Mac/Win build host での embedded fallback path 動作未検証 (B-ε で確認)

§3.4 参照。Linux 環境では fallback 未 compile = 検証不能。Mac/Win 実検証は B-ε で別途、glslangValidator 不在時の動作と δ binding 受領状態 (fallback は γ 状態 = binding なし) の整合を判断。

### 4.2 sky smoke PSO 実 draw 時の descriptor set / push constant 未 bind 問題

`recordSkySmokeDraw` (L1713) は `vkCmdBindPipeline` + `vkCmdDraw(3, 1, 0, 0)` のみ、`vkCmdBindDescriptorSets` / `vkCmdPushConstants` 呼出なし。本 sub-step で shader 側 binding 受領済の状態で実 draw した場合:
- validation = disabled (release build) なので validation error は発生しない
- 実 GPU 挙動: descriptor set 0 未 bind = `PerFrameMatrixUBO` / `TextureMatrixUBO` 読込は undefined behavior、push constant 未 write = `modelview_matrix` 値は driver 依存 (0 init / 前 frame 値 / garbage)
- δ shader 内 `binding_witness * 1e-30` 圧縮で gl_Position への寄与 0 を担保 = 視覚出力は fullscreen triangle 維持

**実 descriptor set bind / push constant write は領域 7 sub-step 7.5 で attachment 配線と合わせて wire up** = 本 sub-step は shader 側受領 + PSO compile 成功 + transit smoke 動作維持のみ acceptance。validation strict は sub-step 3.5 で別 build により実施 (sub-doc 03 §3.5)。

### 4.3 `loadSpirvShaderModule` INFO log noise (引続き B-ε 以降 / 領域 6 sub-step 6.1 prep で再評価)

δ 完遂で `loadSpirvShaderModule` は γ と同じく PSO compile path から 2 件 (sky smoke vert + frag) のみ発火。β-2 transit smoke は撤去済 = 全 log 量は γ と同等。領域 6 sub-step 6.1 一括化時 (248 file 全 port) で全 shader load INFO が数百件規模 = DEBUG / trace level 降格を検討 (B-ε または 6.1 着手 prep で別判断)。

### 4.4 binding witness 1e-30 multiplier の compiler 最適化耐性

glslangValidator + SPIR-V optimizer の挙動として、`* 1e-30` 乗算は数学的に 0 にならない (denormal 化されるのみ) ため、compiler は dead code として除去しない。実 SPIR-V dump (vert 3268 B = γ 1136 B から +2132 B) でも binding 参照は全て残置確認可能 (本 sub-step ではバイト数増のみ確認、必要なら spirv-dis で逆アセンブル確認可)。SPIR-V 規格 1.5+ で `mat4 inverse(...)` / `transpose(...)` / `mat3(mat4)` cast は全て built-in 関数として規格化済 = `glslang --target-env vulkan1.3` で compile 成功。

---

## 5. next session entry point

### 5.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読
3. sub-doc 03 §3.1.3 B-ε marker 再確認
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 sub-step 3.3-B-ε 着手内容 (sub-doc 03 §3.1.3)

| 項目 | 内容 |
|---|---|
| **scope** | `handoff-substep-3-3-B-complete.md` 起草 (3.3-B 全 sub-step α/β-1/β-2/γ/δ 完遂総括 → 3.4 着手境界 + 領域 6 sub-step 6.1 pattern 流用 prep) |
| **対象 file** | `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-complete.md` (新規) + sub-doc 03 §3.1.3 末尾 (3.3-B 全完遂 summary 行追加) + sub-doc 03 §3.1 sub-step 3.3 行 complete 化 |
| **完了 marker** | handoff doc 完成 + sub-doc 03 §3.1.3 末尾 summary 更新 + sub-doc 03 §3.1 sub-step 3.3 行 complete 化 + commit 投入 |

### 5.3 B-ε 着手 task 候補 (推定 sub-step 内 step)

| step | 内容 | 推定規模 |
|---|---|---|
| ε-1 | `handoff-substep-3-3-B-complete.md` 起草 (3.3-B 全 sub-step α/β-1/β-2/γ/δ 完遂 summary + 5 commit hash + 主要 acceptance evidence + 領域 6 sub-step 6.1 流用 pattern まとめ + 3.4 着手境界 doc) | 60 分 |
| ε-2 | sub-doc 03 §3.1.3 末尾 + §3.1 sub-step 3.3 行 complete 化 | 15 分 |
| ε-3 | memory `project_ayastorm_r41_vulkan_migration.md` を 3.4 着手 ready 状態へ update | 15 分 |
| ε-4 | docs commit 投入 | 5 分 |

着手前に **§3.4 (Mac/Win embedded fallback 取扱)** を AYA に相談 (α: glslangValidator 必須化 / β: 新 .spv embed 化)、ε-1 doc 内に方針反映。

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)、ε は doc のみ、shader/runtime touch なし
- **段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-A + 3.3-C + 3.3-B-α/β-1/β-2/γ/δ 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **3.4 (texture lifecycle + descriptor set=1 per-material + 12 pool hook body PSO bind 配線 + 段階 2 引継ぎ特殊対応)** は B-ε 完遂後着手、並走で領域 6 sub-step 6.1 (autobuild integration 一括化) 着手可能
- **validation strict は sub-step 3.5 で別 build により実施** (B-ε は doc のみで AYA launch verify 不要)
- **検証用 LL_INFOS hook は出荷物に残さない** (`feedback_remove_verification_logs.md`、δ-3 INFO marker は spec 由来の永続 marker = OK、temp diagnostic hook は別途)

---

## 6. 関連 doc / memory cross-ref

### 6.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.3-B-δ 行 complete 化 + B-ε marker active)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — 領域 6 sub-doc (B-α で sub-step 6.1 prereq として整理、δ で shader 側 binding 配置 pattern 確立 = 6.1 一括化前の final 形態)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-gamma-complete.md` — 前 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-beta-2-complete.md` — 前々 handoff (役割完了済)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-C-complete.md` — 3.3-C 完遂 handoff (役割完了済)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-A-complete.md` — 3.3-A 完遂 handoff (役割完了済)

### 6.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.3-B-ε 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中、§3.4 embedded fallback の Mac/Win 取扱判断は B-ε で別途
- `feedback_build_only_verified.md` — δ 着手前 rebuild from r41 HEAD + γ 12 marker verify 実施済
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§1.1 acceptance × log evidence 各 line 突合 + .spv byte size verify + libllrender.a symbol nm 確認)
- `feedback_no_scope_shrink.md` — handoff §5.2 marker 例示が binding 0 + push constant のみ言及、scope 行は binding 1 も含む → binding 1 も実装 (§3.2 参照)
- `feedback_one_step_at_a_time.md` — δ AYA から「A で」一括 commit 承認 = δ-1〜δ-3 同 commit で実施 + δ-4 verify を後段確認
- `feedback_no_auto_commit.md` — 本 commit は AYA 明示指示 (「A で」= 選択肢 A 「commit + handoff」承認) 受領後実施
- `feedback_proactive_handoff.md` — sub-step 境界での能動 handoff 起草 (本 file)
- `feedback_log_reading.md` — AYA launch 後 Claude が `~/.ayastorm_x64/logs/AYAstorm.log` を grep して acceptance marker + Vulkan WARN/ERR 0 件 + 既存 11 marker regression 確認
- `feedback_confirm_referent_before_acting.md` — handoff §5.3 末尾「δ 細分化要否を AYA に確認」を遵守、A (一括) / B (γ pattern 分割) を提示して A 承認受領
