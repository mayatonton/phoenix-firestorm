# r41 sub-step 3.3-B-γ 完遂 → 3.3-B-δ 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-beta-2-complete.md` (3.3-B-β-2 完遂 / 3.3-B-γ 着手前境界)
**本 handoff 位置付け**: sub-step 3.3-B-γ (sky smoke PSO file load 切替 + transit smoke 撤去 + embedded fallback macro guard) 完遂宣言 + 3.3-B-δ (shader 内 MVP/normal/inverse_modelview 計算配線 + UBO binding shader 側受領) 着手前 scope 確認境界。
**3.3-B 全体構成 (sub-doc 03 §3.1.3)**: B-α (spec refine、完遂) / B-β-1 (`loadSpirvShaderModule` signature、完遂) / B-β-2 (build chain + transit smoke、完遂) / **B-γ (PSO compile 配線、本 handoff で完遂宣言)** / B-δ (UBO binding + push const + shader 内 MVP/normal 配線、次着手) / B-ε (3.3-B 全完遂 handoff)

---

## 1. sub-step 3.3-B-γ 完遂 status (2026-05-31)

### 1.1 完遂 marker (sub-doc 03 §3.1.3 B-γ)

| acceptance | 達成 status |
|---|---|
| `createSkySmokePipeline()` の `VkShaderModule` source を embedded byte array から build chain 経由 file load 一本化 (Linux build) | ✓ `loadSpirvShaderModuleFromFile("shaders/aya_r41_exemplar/sky_placeholderV.spv")` + `sky_placeholderF.spv` 経由実走 |
| INFO marker `"Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)"` 出力 | ✓ AYA log line 110: `2026-05-30T18:00:40Z INFO #Vulkan# llrender/llvkloader.cpp(1253) createSkySmokePipeline : Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)` |
| file load 経路 evidence (`SPIR-V shader module loaded size=N bytes` × 2 vert+frag) | ✓ AYA log line 108-109: vert 1136 B + frag 384 B (β-2 build chain 出力サイズと完全一致) |
| β-2 transit smoke marker 完全消失 (`SPIR-V exemplar not found` / `sub-step 3.3-B-β-2 exemplar pre-flight` 文字列) | ✓ AYA log で 0 件 hit |
| Vulkan 系 `failed` / WARN / ERR 0 件 (release build / validation disabled、transit acceptance) | ✓ `grep -cE "(WARN|ERR\|ERROR).*Vulkan"` = 0 (38 INFO のみ) |
| 3.3-A / 3.3-C / 3.3-B-β-2 既存 marker 全継続 (regression なし) | ✓ Vulkan 1.3 dynamicRendering / per-frame desc layout (PerFrameMatrixUBO / TextureMatrixUBO) / Placeholder PSO / bindTarget / flush / syncMatrices PerFrame UBO write / syncMatrices TextureMatrix UBO write / beginDynamicRendering helper / pushCurrentModelviewMatrix / shutdownVulkan device+instance 全 hit |
| Linux build で embedded byte array dead code 除去 | ✓ `nm libllrender.a \| grep -cE "kSkySmokeVertSpv\|kSkySmokeFragSpv"` = 0 (macro 未定義時に symbol 物理削除確認) |
| 3 OS 互換維持 (Mac/Win 想定 glslangValidator 不在時 fallback) | ✓ cmake `if (NOT AYA_GLSLANG_VALIDATOR) target_compile_definitions(llrender PRIVATE AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK)` 配信で fallback path 自動 active 化 (Linux 検証環境では glslangValidator 検出済で fallback 不要、Mac/Win 実検証は B-ε で別途) |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-3-B-beta-2-complete.md` | **役割完了** (3.3-B-γ satisfy、本 handoff で内容引継ぎ) |
| sub-doc `03-state-machine-pso.md` §3.1.3 B-γ 行 | active 継続 (本 handoff 起草と同時に「完遂 2026-05-31」marker 追加 + 実装結果反映、B-δ 着手 ready) |
| sub-doc `06-shader-spirv.md` | active 継続 (B-α で sub-step 6.1 prereq 整理済、γ で impl 範疇変えず、領域 6 sub-step 6.1 一括化までの exemplar pre-flight pattern 確立) |
| `handoff-substep-3-3-B-gamma-complete.md` (本 handoff) | 新規作成 (3.3-B-γ 完遂 → 3.3-B-δ 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.3-B-δ 着手 ready 状態へ update) |

---

## 2. 実装内容 (commit 範囲、commit `020df91561`)

### 2.1 runtime (`indra/llrender/llvkloader.cpp` +87 / -67)

| 変更 | 内容 |
|---|---|
| 匿名 namespace 内 helper 追加 | `VkShaderModule loadSpirvShaderModuleFromFile(const char* rel_path)`: `LL_PATH_APP_SETTINGS` 経由で `.spv` 読込 (existence check / size check [>0 + 4-byte aligned] / read check) → `loadSpirvShaderModule` 経由で `VkShaderModule` 化、失敗時 `VK_NULL_HANDLE` + WARN log |
| `createSkySmokePipeline` 改修 | VkShaderModule 取得を `loadSpirvShaderModuleFromFile("shaders/aya_r41_exemplar/sky_placeholderV.spv")` + `sky_placeholderF.spv` primary 化 + `#ifdef AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` 内で embedded byte array fallback (vert/frag 個別 `VK_NULL_HANDLE` check 経由) + 最終 `VK_NULL_HANDLE` check で fail return |
| 完了 marker 文字列更新 | 旧: `"Sky smoke PSO compiled (vert N B / frag N B, sub-doc 03 §3.1 sub-step 3.2 sky pool 1 draw)"` → 新: `"Sky smoke PSO compiled via SPIR-V build chain (sub-step 3.3-B-γ)"` (size 情報は `loadSpirvShaderModule` 内 INFO marker が代替) |
| embedded byte array macro guard | `kSkySmokeVertSpv[]` (288 word = 1152 B) + `kSkySmokeFragSpv[]` (96 word = 384 B) + 関連 comment 全体を `#ifdef AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` ... `#endif` で wrap |
| `beginFrame` 内 β-2 transit smoke 撤去 | `static bool s_spirv_exemplar_smoke_done` + lambda `try_load` + 2 件 `try_load(...)` 呼出 (β-2 で 1 度限り発火していた build chain 実証 transit smoke) を撤去、置換 comment で履歴 trace |
| `loadSpirvShaderModule` INFO marker generic 化 | 旧: `"SPIR-V shader module loaded (sub-step 3.3-B-β-2 exemplar pre-flight) size=N bytes"` → 新: `"SPIR-V shader module loaded size=N bytes"` (γ 以降は createSkySmokePipeline からも呼出されるため sub-step 限定参照を除去) |
| `loadSpirvShaderModule` の `sInitialized` ガード撤去 | 旧: `if (!sInitialized \|\| sDevice == VK_NULL_HANDLE)` → 新: `if (sDevice == VK_NULL_HANDLE)`。`initVulkan` 内 `createSkySmokePipeline` 呼出時点では `sInitialized = false` (関数末尾で true 設定) のため、`sInitialized` 必須要件は誤抑制になる。`sDevice` null check で safety 担保継続 |

### 2.2 cmake (`indra/llrender/CMakeLists.txt` +9)

| 変更 | 内容 |
|---|---|
| `include(AyaShaderCompile)` 追加 | `AYA_GLSLANG_VALIDATOR` cache 変数を newview 側 include に先行 = llrender target 構築時点で参照可能化 |
| 条件 compile definition 配信 | `if (NOT AYA_GLSLANG_VALIDATOR) target_compile_definitions(llrender PRIVATE AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK) endif()`: Linux build (glslangValidator 検出) では macro 未定義 → embedded byte array dead code 除去、Mac/Win build (glslangValidator 不在想定) では macro 定義 → embedded fallback active |

### 2.3 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `indra/llrender/llvkloader.cpp` | +87 / -67 | helper 追加 + createSkySmokePipeline 改修 + macro guard + transit smoke 撤去 + INFO marker generic 化 + sInitialized ガード撤去 |
| `indra/llrender/CMakeLists.txt` | +9 / -0 | include(AyaShaderCompile) + 条件 compile def 配信 |

合計: **2 file、+96 / -67 line (net +29)** (commit `020df91561`)

---

## 3. 設計判断履歴 (本 sub-step 着手時)

### 3.1 embedded byte array 残置: macro guard 化 (AYA 採用)

handoff §5.3 γ-3 で 3 案提示 (完全削除 / macro guard 化 / 完全残置)、AYA 採用 = **macro guard 化**。理由:
- Mac/Win build host で glslangValidator 不在時の fallback として残置価値あり (3 OS 大前提 = `project_ayastorm_three_platforms.md`、Linux 先行例外適用中だが将来 Mac/Win Vulkan port [r42-α/β] で利用)
- Linux build (現状唯一の検証環境) では dead code として binary から除外、不要な byte 増加防止
- 完全残置は dead code 化が見えにくい、γ-4 verify で file load 経路実走の証明に不利

### 3.2 sky smoke のみ切替、placeholder PSO は untouched

handoff §5.2 / §5.3 では 「placeholder + sky smoke 2 PSO」記載だが、sub-doc 03 §3.1.3 B-γ marker は `createSkySmokePipeline()` のみ明示。実装観察:
- 生成済 GLSL exemplar (`sky_placeholderV.glsl` / `sky_placeholderF.glsl`) は **sky smoke (kSkySmokeVertSpv 1136B / kSkySmokeFragSpv 384B) と等価**
- `kPlaceholderVertSpv` / `kPlaceholderFragSpv` (placeholder PSO 用、別 shader) 等価 GLSL は **存在しない** = 切替不能
→ AYA 確認 = **sky smoke 1 PSO のみで進める** (sub-doc 03 spec 通り)。placeholder PSO の GLSL exemplar 化は領域 6 sub-step 6.1 一括化担当。

### 3.3 cmake-time gate (file load 必須化) vs ランタイム fallback の選択

Linux build で macro 未定義 = file load 必須 = .spv 不在時に PSO compile fail = `initVulkan` 失敗 = `shutdownVulkan()` で clean shutdown。GL path 並走で viewer 起動継続可能 (Vulkan 並走停止)。本選択により γ-4 verify で **file load 経路が実走したことを Linux build で確証**できる (embedded fallback で覆い隠されない)。

### 3.4 `loadSpirvShaderModule` の `sInitialized` ガード撤去

実装着手時に発見した潜在 bug: `initVulkan` 内 `createSkySmokePipeline` 呼出時点 (L1271) で `sInitialized = false` (関数末尾 L1278 で true 設定)。`loadSpirvShaderModule` 内 `!sInitialized` 早期 return が PSO compile path を無効化する。修正 = `sInitialized` ガード撤去、`sDevice` null check のみで safety 担保 (Vulkan device 未生成時の不正呼出は引続 block)。`sInitialized` フラグは「viewer 起動完了状態」signal として initVulkan 末尾でのみ使用される (β-2 transit smoke は beginFrame 内 = `sInitialized = true` 期だった、γ で呼出位置が initVulkan 内へ前倒し)。

---

## 4. risks / caveats

### 4.1 Mac/Win build host 実検証は B-ε

cmake 配信 `AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` 自動定義 logic 自体は Linux configure 段階で `if (NOT AYA_GLSLANG_VALIDATOR)` 評価が正しく走ることを確認済 (Linux では検出 = 未定義 = embedded dead code 除去)。**実際の Mac/Win build host で glslangValidator 不在時に embedded fallback が active 化 + PSO compile 成功**は B-ε (3.3-B 全完遂 handoff) で別途確認。Linux 環境のみでの γ-4 verify は本 sub-step scope。

### 4.2 PSO compile fail 時の viewer 動作

Linux build で何らかの理由 (.spv 削除 / file permission / build chain 破綻) で file load 失敗 → `createSkySmokePipeline` `false` return → `initVulkan` `false` return → `shutdownVulkan()` clean shutdown。GL path 並走で viewer 起動継続 (Vulkan 並走停止)、ユーザー体感は GL path のみで OK だが Vulkan 初期化失敗の WARN log が増える見込み。本 sub-step では検証環境で `.spv` 正常生成 + deploy 済のため発生せず。

### 4.3 `loadSpirvShaderModule` INFO log noise (B-δ 以降)

γ 完遂で `loadSpirvShaderModule` は **PSO compile path から 2 件 (sky smoke vert + frag)** + transit smoke は撤去済 = 全 log 量は β-2 と同等 (β-2 でも transit smoke 2 件)。B-δ で UBO binding 含む shader 再配置時にも同 2 件のみ (sky smoke exemplar 1 件のみ port)。**領域 6 sub-step 6.1 一括化時** (248 file 全 port) で全 shader load の INFO 発火 = 数百件規模になるため、6.1 着手前に DEBUG / trace level 降格を検討 (B-ε または 6.1 着手 prep で別判断)。

### 4.4 embedded fallback path の動作未検証 (Linux 環境のみ)

`AYA_R41_USE_EMBEDDED_SPIRV_FALLBACK` 定義時の `createSkySmokePipeline` 内 fallback コード path は Linux 環境では macro 未定義のためコンパイル時除外 = ランタイム未実走。コード論理 review レベルでは:
- file load 失敗 (`VK_NULL_HANDLE`) 後の `vkCreateShaderModule(sDevice, &vs_info, nullptr, &sSkySmokeVertModule)` は β-2 の embedded path と同型 (β-2 で動作実証済)
- 最終 `VK_NULL_HANDLE` check は file load primary path と同 check 経路
→ Mac/Win 実検証は B-ε で確認。

---

## 5. next session entry point

### 5.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読
3. sub-doc 03 §3.1.3 B-δ marker 再確認
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 sub-step 3.3-B-δ 着手内容 (sub-doc 03 §3.1.3)

| 項目 | 内容 |
|---|---|
| **scope** | UBO binding (set=0 binding 0 = `PerFrameMatrixUBO` 192 B / binding 1 = `TextureMatrixUBO` 256 B) + push constant range (0..64 B / VERTEX_BIT = `modelview_matrix`) の **shader 側受領** + shader 内 `mvp_matrix = projection_matrix * modelview_matrix` / `normal_matrix = transpose(inverse(mat3(modelview_matrix)))` / `inverse_modelview_matrix = inverse(modelview_matrix)` 計算配線 (3.3-A §3.1.1 trace inventory「shader 内計算移譲 3 種」を sky placeholder vert shader 内に具体配置) |
| **対象 file** | `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl` (vert に UBO binding + push constant + 計算式追加) + `indra/llrender/llvkloader.cpp` (PSO layout 既存流用 = `sSkySmokeLayout` の set=0 desc set layout + push constant range は γ 時点で配線済、δ は shader 内側のみ) |
| **完了 marker** | `.glsl` vert source 内 `layout(set=0, binding=0) uniform PerFrameMatrixUBO { mat4 projection_matrix; mat4 inverse_projection_matrix; mat4 identity_matrix; };` + `layout(push_constant) uniform PushConstants { mat4 modelview_matrix; };` + shader 内 MVP/normal/inverse_modelview 計算式 + glslang `--target-env vulkan1.3` で `.spv` 生成成功 + PSO compile 成功 + INFO marker `"Sky placeholder vert binding active (PerFrameMatrixUBO + push constant modelview)"` + AYA launch PASS + 視覚 regression 0 + Vulkan 系 WARN/ERR 0 件 |

### 5.3 B-δ 着手 task 候補 (推定 sub-step 内 step)

| step | 内容 | 推定規模 |
|---|---|---|
| δ-1 | `sky_placeholderV.glsl` に `layout(set=0, binding=0) uniform PerFrameMatrixUBO { ... };` + `layout(push_constant) uniform PushConstants { mat4 modelview_matrix; };` 追加 (現在は uniform 未参照 = fullscreen triangle で gl_VertexIndex のみ参照) | 30 分 |
| δ-2 | shader 内 MVP/normal/inverse_modelview 計算式追加 (現在 fullscreen triangle で実演 = `gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0)` 直接出力、δ は 「計算式を生成可能な書式で記述」段階 = uniform 値が 0 でも .spv compile 成功すれば marker satisfy) | 30 分 |
| δ-3 | INFO marker `"Sky placeholder vert binding active (PerFrameMatrixUBO + push constant modelview)"` を `createSkySmokePipeline` PSO compile 成功後に追加 (場所: 既存 γ marker と隣接、shader binding 設計の shader 側受領 evidence) | 15 分 |
| δ-4 | build + AYA launch verify (.spv 生成 + PSO compile 引続成功 + 新 INFO marker hit + Vulkan WARN/ERR 0 件) | verify |

着手前に δ 細分化要否を AYA に確認 (本 step リスト自体が細分化案、δ は scope が β-2/γ より軽量 = 一括 commit 案推奨見込み)。

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)、δ は exemplar shader (`sky_placeholderV.glsl`) のみ touch
- **段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-A + 3.3-C + 3.3-B-α/β-1/β-2/γ 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **3.3-B-δ で `mat4 inverse(...)` を使う場合は GLSL 450 + GL_GOOGLE_include_directive 等の preamble に注意** (glslang `--target-env vulkan1.3` で `#version 450` + `extension GL_EXT_scalar_block_layout : require` 等の宣言要否、δ-1 着手時に build chain で fail した場合は preamble 追記)
- **`mat4 inverse(...)` の `normal_matrix = transpose(inverse(mat3(modelview_matrix)))` は SPIR-V 1.5+ で GL_EXT_demote_to_helper_invocation 等の interaction なし** = 3.3-A §3.1.1 trace inventory の前提通り (3.3-A spec §3.1.1 末尾参照)
- **validation strict は sub-step 3.5 で別 build により実施** (γ/δ ともに validation = disabled の release build で AYA launch + WARN/ERR 0 件 という transit acceptance)
- **検証用 LL_INFOS hook は出荷物に残さない** (`feedback_remove_verification_logs.md`、δ-3 INFO marker は spec 由来の永続 marker = OK、temp diagnostic hook は別途)

---

## 6. 関連 doc / memory cross-ref

### 6.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.3-B-γ 行 complete 化 + B-δ marker active)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — 領域 6 sub-doc (B-α で sub-step 6.1 prereq として整理、γ は impl 範疇外、δ で shader 内 binding 配置の pattern が 6.1 一括化前 final 形態)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-B-beta-2-complete.md` — 前 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-C-complete.md` — 3.3-C 完遂 handoff (役割完了済)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-A-complete.md` — 3.3-A 完遂 handoff (役割完了済)

### 6.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.3-B-δ 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中、γ-3 macro guard 設計 (Mac/Win fallback) の根拠
- `feedback_build_only_verified.md` — γ 着手前 rebuild from r41 HEAD + 12 marker verify 実施済
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§1.1 acceptance × log evidence 各 line 突合 + libllrender.a symbol nm 確認)
- `feedback_one_step_at_a_time.md` — γ 4 step 実行 (γ-1 file load 切替 / γ-2 transit smoke 撤去 / γ-3 macro guard / γ-4 verify) を順次完遂
- `feedback_no_auto_commit.md` — 本 commit は AYA 明示指示 (「推奨で」= 選択肢 1 「commit + handoff draft 実施」承認) 受領後実施
- `feedback_proactive_handoff.md` — sub-step 境界での能動 handoff 起草 (本 file)
- `feedback_log_reading.md` — AYA launch 後 Claude が `~/.ayastorm_x64/logs/AYAstorm.log` を grep して acceptance marker + Vulkan WARN/ERR 0 件 + 3.3-A/3.3-C 11 marker regression 確認
- `feedback_confirm_referent_before_acting.md` — handoff §5.2 「placeholder + sky smoke 2 PSO」と sub-doc 03 「sky smoke 1 PSO のみ」の表記差を発見し、推測せず AYA に確認 (= sub-doc 03 spec 通り sky smoke 1 PSO のみ採用)
