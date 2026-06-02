# r41 sub-step 3.3-B-β-2 完遂 → 3.3-B-γ 着手境界 handoff (2026-05-30)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-session-pause-2026-05-29-after-3-3-C.md` (3.3-C 全完遂 / 3.3-B 着手前 session pause)
**本 handoff 位置付け**: sub-step 3.3-B-β-2 (SPIR-V build chain + transit smoke) 完遂宣言 + 3.3-B-γ (SPIR-V 並走 PSO compile 配線) 着手前 scope 確認境界。
**3.3-B 全体構成 (sub-doc 03 §3.1.3)**: B-α (spec refine、完遂) / B-β-1 (`loadSpirvShaderModule` signature、完遂) / **B-β-2 (build chain + transit smoke、本 handoff で完遂宣言)** / B-γ (PSO compile 配線、次着手) / B-δ (UBO binding + push const + shader 内 MVP/normal 配線) / B-ε (3.3-B 全完遂 handoff)

---

## 1. sub-step 3.3-B-β-2 完遂 status (2026-05-30)

### 1.1 完遂 marker (sub-doc 03 §3.1.3 B-β-2)

| acceptance | 達成 status |
|---|---|
| `glslangValidator` system path 経由で発見 + cmake 段階で STATUS log 出力 | ✓ cmake configure log: `AYAstorm r41: glslangValidator = /usr/bin/glslangValidator` |
| GLSL exemplar 2 file (sky_placeholderV.glsl / sky_placeholderF.glsl) pre-compile → .spv 生成 | ✓ build log: `aya_r41_shaders` target + .spv 出力 (vert 1136 B / frag 384 B、glslangValidator -V --target-env vulkan1.3) |
| viewer_manifest.py recursive copy で .spv が packaged/app_settings/shaders/aya_r41_exemplar/ に同梱 | ✓ `ls build-linux-x86_64/newview/packaged/app_settings/shaders/aya_r41_exemplar/` で .glsl + .spv 4 file 確認 |
| `~/ayastorm/` 同期 + cache clear 後 AYA launch + beginFrame 初回に transit smoke 発火 | ✓ AYA launch verify (2026-05-30 12:28:48Z): `SPIR-V shader module loaded (sub-step 3.3-B-β-2 exemplar pre-flight) size=1136 bytes` + `size=384 bytes` × 2 件 |
| Vulkan 系 `failed` / WARN / ERR 0 件 (release build / validation disabled、transit acceptance) | ✓ `grep -cE "WARN.*Vulkan\|ERR.*Vulkan\|ERROR.*Vulkan"` = 0 |
| 3.3-A / 3.3-C の 12 marker 全継続 (regression なし) | ✓ dynamicRendering / desc layout / 両 UBO / Placeholder PSO / Sky smoke PSO / syncMatrices 両 write / beginDynamicRendering / pushCurrentModelviewMatrix / bindTarget / flush / shutdownVulkan clean 全 log 健在 |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-session-pause-2026-05-29-after-3-3-C.md` | **役割完了** (3.3-B-α/β-1/β-2 satisfy、本 handoff で内容引継ぎ) |
| sub-doc `03-state-machine-pso.md` §3.1.3 B-β-2 行 | active 継続 (本 handoff 起草と同時に「完遂 2026-05-30」marker 追加 + 実装結果反映、B-γ 着手 ready) |
| sub-doc `06-shader-spirv.md` | active 継続 (B-α で sub-step 6.1 prereq 整理済、β-2 で実装範疇は変えず) |
| `handoff-substep-3-3-B-beta-2-complete.md` (本 handoff) | 新規作成 (3.3-B-β-2 完遂 → 3.3-B-γ 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.3-B-γ 着手 ready 状態へ update) |

---

## 2. 実装内容 (commit 範囲、commit `5d4999ec4a`)

### 2.1 build chain

| file | 状態 | 主内容 |
|---|---|---|
| `indra/cmake/AyaShaderCompile.cmake` | new (+50) | `find_program(AYA_GLSLANG_VALIDATOR glslangValidator)` + `aya_compile_shader_spirv(STAGE INPUT_GLSL OUTPUT_SPV)` function (add_custom_command 経由で glslangValidator -V --target-env vulkan1.3 起動)。未発見時は WARNING + skip (Mac/Win build host 互換) |
| `indra/newview/CMakeLists.txt` | +23 / -1 | `include(AyaShaderCompile)` + shader glob 後に exemplar 2 file pre-compile wiring + `add_custom_target(aya_r41_shaders DEPENDS .spv)` + `add_dependencies(${VIEWER_BINARY_NAME} aya_r41_shaders)` |
| `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl` | new | gl_VertexIndex 0/1/2 で fullscreen triangle、3.3-A γ kSkySmokeVertSpv と等価 |
| `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderF.glsl` | new | 固定色 (0.4, 0.6, 0.9, 1.0) 出力、3.3-A γ kSkySmokeFragSpv と等価 |
| `.gitignore` | +4 | `*.spv` 追加 (build artifact、source tree 内生成だが commit 除外、viewer_manifest.py で deploy) |

### 2.2 runtime (`indra/llrender/llvkloader.cpp` +80)

**impl body**:
- `loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes)` body: arg validation (null / size 0 / size % 4 != 0) + `VkShaderModuleCreateInfo` + `vkCreateShaderModule` + INFO marker `"SPIR-V shader module loaded (sub-step 3.3-B-β-2 exemplar pre-flight) size=N bytes"`
- include 追加: `lldir.h` (path 解決用) + `<fstream>` (.spv 読込用)

**transit smoke** (beginFrame 内、3.3-C-β-2 transit smoke の直後):
- `static bool s_spirv_exemplar_smoke_done` で 1 度限り (first frame)
- lambda `try_load(const char* rel_path)` で `gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, rel_path)` → `std::ifstream binary` → `loadSpirvShaderModule()` → `vkDestroyShaderModule()` (即破棄、PSO 配線は γ)
- `sky_placeholderV.spv` / `sky_placeholderF.spv` 2 件呼出

### 2.3 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `.gitignore` | +4 / -0 | *.spv 除外 |
| `indra/cmake/AyaShaderCompile.cmake` | +50 / -0 | new module |
| `indra/llrender/llvkloader.cpp` | +80 / -0 | loadSpirvShaderModule impl + transit smoke + include |
| `indra/newview/CMakeLists.txt` | +23 / -0 | include + exemplar wiring + viewer dep |
| `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderV.glsl` | +17 / -0 | new |
| `indra/newview/app_settings/shaders/aya_r41_exemplar/sky_placeholderF.glsl` | +16 / -0 | new |

合計: **6 file、+190 / -0 line** (commit `5d4999ec4a`)

---

## 3. 設計判断履歴 (本 sub-step 着手時)

### 3.1 SPIR-V compiler 経路選択: build-time pre-compile + system glslangValidator (AYA 推奨採用)

`shaderc` (runtime compile) ではなく **build-time pre-compile via system glslangValidator** を AYA 採用。理由:
- runtime compile は viewer 起動時間 + shader cache 整合性 (r28 章で実装済 GL shader cache と並走) 課題を新規発生させる
- build-time は autobuild prebuilt 経由でなく system package (apt: `glslang-tools` / Vulkan SDK) 依存に統一できる
- Mac/Win build host で `glslangValidator` 未 install 時は cmake WARNING + skip、embedded byte array (3.3-A γ で配置済 kSkySmokeVertSpv/kSkySmokeFragSpv) で fallback 可能 — 3 OS build 互換性を維持

### 3.2 .spv 配置: source tree 内 (build dir 外) + .gitignore 除外

build dir に出力 → install/copy step を追加する案ではなく、**source tree 内 (.glsl 隣接) 配置 + .gitignore で commit 除外** を採用。viewer_manifest.py line 104 `self.path("shaders")` の recursive copy が既に shaders/ 配下全体を packaging 対象としているため、追加の install step 不要。

### 3.3 transit smoke の配置 (3.3-C-β-2 transit pattern 踏襲)

3.3-C-β-2 で確立した「beginFrame 初回 1 度限り発火 + 即破棄」transit smoke pattern を踏襲。`static bool` guard で初回のみ実行、loadSpirvShaderModule で module 取得 → 即 vkDestroyShaderModule で破棄 (PSO compile への持込は γ)。本 β-2 は build chain (glslangValidator → .spv → file load → vkCreateShaderModule) 全段の実証のみに limit。

---

## 4. risks / caveats

### 4.1 PSO 側未配線 (3.3-B-γ scope)

本 β-2 で `loadSpirvShaderModule()` が動作することは実証したが、戻り値の `VkShaderModule` は即破棄しているため、実 PSO compile (placeholder / sky smoke) は引続き embedded byte array (kSkySmokeVertSpv / kSkySmokeFragSpv) 経由。γ で build chain → PSO compile への配線変更を実施。

### 4.2 Mac/Win build host での glslangValidator 未 install 時の挙動

`find_program` が NOTFOUND だと `add_custom_command` が定義されず、`aya_r41_shaders` target も作成されない。`add_dependencies(${VIEWER_BINARY_NAME} aya_r41_shaders)` は `if (TARGET aya_r41_shaders)` guard 付きなので build は通るが、.spv 不在で runtime transit smoke は `SPIR-V exemplar not found:` WARN 出力後 skip。3 OS 全環境での glslangValidator install validation は 3.3-B 全完遂 (B-ε) で別途確認。

### 4.3 transit smoke の 1 度限り実行

`static bool s_spirv_exemplar_smoke_done` で 1 度限り発火するため、PSO 配線が γ で完了して transit smoke を撤去するタイミング (γ-1 step で beginFrame から削除) でも regression にはならない。

### 4.4 INFO log noise (γ 移行後)

`loadSpirvShaderModule()` 内の INFO marker は β-2 transit smoke では 2 件 (vert + frag) のみ。γ で実 PSO 配線後は全 shader load で INFO 発火するため log noise 増加見込み — 領域 6 sub-step 6.1 一括化までに DEBUG / trace level 降格を検討 (B-γ または B-ε で別判断)。

---

## 5. next session entry point

### 5.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読
3. sub-doc 03 §3.1.3 B-γ marker 再確認
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 sub-step 3.3-B-γ 着手内容 (sub-doc 03 §3.1.3)

| 項目 | 内容 |
|---|---|
| **scope** | placeholder + sky smoke 2 PSO の VkShaderModule source を embedded byte array (kSkySmokeVertSpv/kSkySmokeFragSpv) から `loadSpirvShaderModule(file load from sky_placeholderV.spv / sky_placeholderF.spv)` 経由に切替。3.3-C で確立した bindTarget/flush 並走経路 + 3.3-A 二段構え matrix 系は据置 |
| **対象 file** | `indra/llrender/llvkloader.cpp` (`createPlaceholderPipeline` + `createSkySmokePipeline` の VkShaderModule source 切替)、可能なら embedded byte array (kSkySmokeVertSpv/kSkySmokeFragSpv) 削除 |
| **完了 marker** | 2 PSO compile 成功 + Vulkan WARN/ERR 0 件 + 3.1b/3.2/3.3-A/3.3-C 動作維持 (Placeholder PSO compiled + Sky smoke PSO compiled log 引続き出力) + transit smoke 撤去 |

### 5.3 B-γ 着手 task 候補 (推定 sub-step 内 step)

| step | 内容 | 推定規模 |
|---|---|---|
| γ-1 | `createPlaceholderPipeline` + `createSkySmokePipeline` の VkShaderModule 取得経路を file load 経由に切替 (lambda 共通化推奨) | 0.5 日 |
| γ-2 | beginFrame の transit smoke 撤去 (静的 once 削除、unused warning 防止) | 30 分 |
| γ-3 | embedded byte array (kSkySmokeVertSpv/kSkySmokeFragSpv) 削除可否確認 (PSO 直接参照無くなれば dead code) | 30 分 |
| γ-4 | build + AYA launch verify + log 確認 (2 PSO 引続き compile 成功 + WARN/ERR 0 件 + transit smoke marker 消失) | verify |

着手前に γ 細分化要否を AYA に確認 (本 step リスト自体が細分化案、γ は scope が β-2 より軽量 = 一括 commit 案推奨見込み)。

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)
- **段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-A + 3.3-C + 3.3-B-β-2 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **3.3-B-γ で transit smoke 撤去後は loadSpirvShaderModule INFO marker は PSO compile 経路から発火** (β-2 の 1 度限り発火から PSO 数分の発火に変わる、γ-4 verify で初回 2 件分の log を確認)
- **build host の glslangValidator install validation は B-ε で 3 OS 別途確認** (β-2 は Linux 環境のみ AYA launch verify、Mac/Win build chain validation は別途)
- **embedded byte array fallback (kSkySmokeVertSpv/kSkySmokeFragSpv)** は γ で消すか残すか確認 — Mac/Win で glslangValidator 不在時の fallback としては残す価値あり、ただし dead code 化避けるなら別 macro guard 化検討
- **validation strict は sub-step 3.5 で別 build により実施** (β-2/γ ともに validation = disabled の release build で AYA launch + WARN/ERR 0 件 という transit acceptance)

---

## 6. 関連 doc / memory cross-ref

### 6.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.3-B-γ marker active 化)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — 領域 6 sub-doc (B-α で sub-step 6.1 prereq として整理、本 β-2 は impl 範疇外)
- `docs/specs/ayastorm-r41-gl-removal/handoff-session-pause-2026-05-29-after-3-3-C.md` — 前 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-C-complete.md` — 3.3-C 完遂 handoff (役割完了済)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-A-complete.md` — 3.3-A 完遂 handoff (役割完了済)

### 6.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.3-B-γ 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_build_only_verified.md` — β-2 着手前 rebuild from r41 HEAD + 12 marker verify 実施済
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§1.1 acceptance × log evidence 各 line 突合)
- `feedback_one_step_at_a_time.md` — β-2 7 step 実行 (glslangValidator 確認 / GLSL exemplar 作成 / cmake module / cmake wiring / runtime impl / transit smoke / build verify) を順次完遂
- `feedback_no_auto_commit.md` — 本 commit は AYA 明示指示 (「commit して handoff ください」) 受領後実施
- `feedback_proactive_handoff.md` — sub-step 境界での能動 handoff 起草 (本 file)
- `feedback_log_reading.md` — AYA launch 後 Claude が `~/.ayastorm_x64/logs/AYAstorm.log` を grep して acceptance marker + Vulkan WARN/ERR 0 件 + 3.3-A/3.3-C 12 marker regression 確認
