# r41 sub-step 3.3-γ 完遂 → 3.3-δ 着手境界 handoff (2026-05-29)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-beta-2-complete.md` (3.3-β-2 = matrix UBO VkBuffer × 3 + descriptor pool/set alloc + persistent map + zero write smoke 完遂 → 3.3-γ 着手境界)
**本 handoff 位置付け**: sub-step 3.3-γ (placeholder + sky smoke 2 PSO の `VkPipelineLayout` を二段構え準拠化) 全完遂宣言 + sub-step 3.3-δ (LLRender::syncMatrices() Vulkan path 並走) 着手前 scope 確認境界。

---

## 1. sub-step 3.3-γ 全完遂 status (2026-05-29)

### 1.1 完遂 marker (sub-doc 03 §3.1.1 sub-step 3.3-γ)

| acceptance | 達成 status |
|---|---|
| placeholder PSO の `VkPipelineLayout` を二段構え準拠化 (set=0 = `sPerFrameDescriptorSetLayout` + push constant range = mat4 modelview_matrix / 64 B / VERTEX_BIT) | ✓ llvkloader.cpp:891 直前 で `VkDescriptorSetLayout set_layouts[1]` + `VkPushConstantRange push_constants[1]` 構築 → `createStandardPipelineLayout(set_layouts, 1, push_constants, 1)` |
| sky smoke PSO の `VkPipelineLayout` を二段構え準拠化 (同上) | ✓ llvkloader.cpp:1058 直前 で同一 layout 構築 → `createStandardPipelineLayout(set_layouts, 1, push_constants, 1)` |
| 2 PSO compile 成功 + bind 成功 (新 layout 統合後も graphics pipeline 構築通る) | ✓ log: `Placeholder PSO compiled (vert 752 B / frag 408 B)` (llvkloader.cpp:973) + `Sky smoke PSO compiled (vert 852 B / frag 336 B)` (llvkloader.cpp:1158) |
| Vulkan 系 `failed` / WARN / ERR 0 件 (release build / validation disabled、transit acceptance) | ✓ grep 結果 0 hit (RTX 5090 / Vulkan 1.4.319、Vulkan log 全 27 line すべて INFO) |
| build smoke (`autobuild build -A 64 -c ReleaseFS_open --no-configure`) | ✓ exit 0、`[100%] Built target llpackage` + `finished` |
| regression: 段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-β-2 動作維持 | ✓ Per-frame descriptor set layout / UBO buffers / descriptor sets 3 marker 引続き出力 + AYA launch PASS (login 画面到達) |
| AYA launch verify (`~/ayastorm/ayastorm` 起動 + 起動完了) | ✓ AYA 確認済 (2026-05-29) |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-3-beta-2-complete.md` | **役割完了** (3.3-γ 着手 satisfy、本 handoff で内容引継ぎ) |
| sub-doc `03-state-machine-pso.md` | active 継続 (§3.1.1 γ 行を「完遂 2026-05-29」+ layout 統合内容反映済、δ 着手 ready) |
| sub-doc `05-vulkan-api-design.md` | active 継続 (β-1 で sealed 済、γ では未変更 = layout 仕様は β-1 sealed の通り適用) |
| `handoff-substep-3-3-gamma-complete.md` (本 handoff) | 新規作成 (3.3-γ 全完遂 → 3.3-δ 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.3-δ 着手 ready 状態へ update) |

---

## 2. 実装内容 (commit 範囲)

### 2.1 spec sealing (sub-doc 03 §3.1.1)

| file | section | 変更内容 |
|---|---|---|
| sub-doc 03 | §3.1.1 γ 行 | 「(完遂 2026-05-29)」marker 追加 + 実装内容反映 (set=0 = `sPerFrameDescriptorSetLayout` + push constant range = mat4 / 64 B / VERTEX_BIT、+18/-2 行、2 PSO compile log + WARN/ERR 0 件 + AYA launch PASS で update) |

### 2.2 `llvkloader.cpp` 変更 (+18 / -2)

`createPlaceholderPipeline` + `createSkySmokePipeline` 内、`createStandardPipelineLayout` 呼出引数を空 layout (`nullptr, 0, nullptr, 0`) から二段構え準拠 layout に差替え:

**placeholder PSO (line 891 直前)**:
```cpp
// r41 sub-step 3.3-γ: 二段構え準拠 layout
// 実 descriptor set bind / push constant 投入は δ で初実施、γ は layout signature 統合のみ。
VkDescriptorSetLayout set_layouts[1]     = { sPerFrameDescriptorSetLayout };
VkPushConstantRange   push_constants[1]  = {};
push_constants[0].stageFlags             = VK_SHADER_STAGE_VERTEX_BIT;
push_constants[0].offset                 = 0;
push_constants[0].size                   = 64; // mat4 modelview_matrix
sPlaceholderLayout = createStandardPipelineLayout(set_layouts, 1, push_constants, 1);
```

**sky smoke PSO (line 1058 直前)**: 同一 layout を `sSkySmokeLayout` 向けに同様構築。

### 2.3 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | +1 / -1 | §3.1.1 γ 行 complete 化 + 実装内容反映 |
| `indra/llrender/llvkloader.cpp` | +18 / -2 | placeholder + sky smoke の `createStandardPipelineLayout` 呼出を二段構え準拠 layout 引数で差替え |

合計: **2 file、+19 / -3 line** (β-2 の +240/-1 より大幅軽量、案 A 一括 commit に整合)

---

## 3. 設計判断履歴 (本 sub-step 着手時 AYA 確認 1 件)

### 3.1 細分化粒度: 案 A (γ = 1 commit)

handoff §5.3 提示の γ-1/γ-2/γ-3 3 step を **1 commit に纏める案 A** で AYA 採用 (2026-05-29)。判断根拠:
- scope が β-2 (+239 行 state + create 関数 3 件) より大幅軽量 (`createStandardPipelineLayout` 呼出引数差替えのみ、新規 state/関数追加なし)
- γ-1 (placeholder) と γ-2 (sky smoke) はほぼ同一作業の対称、commit 分離による review 価値が低い
- 各 PSO 単独 build/launch 検証の独立価値が薄い (新規挙動は γ 全体完了後の「2 PSO bind + WARN/ERR 0 件」のみ)

### 3.2 anon ns 直接参照 vs 公開 getter

`sPerFrameDescriptorSetLayout` 参照は **anon ns 直接参照** で実装。`createPlaceholderPipeline`/`createSkySmokePipeline` は同一 TU の anon ns 内のため、公開 `getPerFrameDescriptorSetLayout()` を呼ぶ間接化は冗長。spec (`handoff-substep-3-3-beta-2-complete.md` §5.2) の「set=0 = LLVKLoader::getPerFrameDescriptorSetLayout()」は **意味としての set=0** (per-frame matrix UBO descriptor set layout) を指す表記であり、literal な関数呼出強制ではない。

### 3.3 init order self-verify

llvkloader.cpp:1211 (`createPerFrameDescriptorSetLayout`) → llvkloader.cpp:1217 (`createPlaceholderPipeline`) → llvkloader.cpp:1223 (`createSkySmokePipeline`) の順で initVulkan 内に配線済 (β-2 commit で確定)。γ 着手時の sPerFrameDescriptorSetLayout 参照可能性は initVulkan 既存順序で satisfy、追加配線不要。

---

## 4. risks / caveats

### 4.1 push constant + descriptor set bind 命令は未投入 (3.3-δ scope)

γ で `VkPipelineLayout` は二段構え準拠 signature を持つが、**実 `vkCmdBindDescriptorSets` + `vkCmdPushConstants` 投入経路は未成立**。layout はあくまで「PSO がどんな input を期待するか」の signature 宣言であり、command buffer に bind 命令を積むのは δ (`LLRender::syncMatrices()` Vulkan path 並走) で初実施。

**判断**: γ は layout signature 統合のみで transit acceptance、PSO compile + WARN/ERR 0 件で OK。δ で push constant + UBO write の実 GPU 経路成立確認。

### 4.2 既存 SPIR-V vertex shader の uniform 未参照

placeholder/sky smoke の現 SPIR-V は **空入力 fullscreen triangle** (`gl_Position` を gl_VertexIndex 駆動で算出、`modelview_matrix` 等 uniform 未参照) のため、layout に push constant range / descriptor set を追加宣言しても compile/bind は通る。Vulkan validation 仕様上、shader が参照しない push constant / descriptor binding を layout 側で宣言するのは「extra resources」として許容 (warn にもならない)。

### 4.3 set=1/2 layout は未配線 (将来 sub-step scope)

sub-doc 05 §3.5 `仮設計` の `setLayoutCount=3` (perFrame + perMaterial + perDraw) のうち、本 γ では **set=0 (perFrame) のみ wired**。set=1 (perMaterial) / set=2 (perDraw) は将来 3.3 後続 sub-step で必要となった際に layout 追加、本段階では `setLayoutCount=1` で固定。

### 4.4 push constant size = 64 (mat4) ハードコード

`push_constants[0].size = 64;` は mat4 modelview_matrix の sizeof 直書き。`sizeof(F32) * 16` に切替える refactor は将来 push constant block の struct 定義 (β-1 PerFrameMatrixUBO 同様) を入れた時点で実施、本段階では仕様明示の literal 64 で OK。

### 4.5 validation strict 確認は sub-step 3.5 に持越し

本 sub-step は **release build (validation = disabled)** で `failed` log 0 件 + AYA launch PASS という transit acceptance。validation strict 確認 (extra resources の warn 含めた厳密検証) は sub-doc 03 §3.5 (段階 3 self-check) で別 build により実施。

---

## 5. next session entry point

### 5.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読
3. sub-doc 03 §3.1.1 sub-step 3.3-δ marker 再確認
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 sub-step 3.3-δ 着手内容 (sub-doc 03 §3.1.1)

| 項目 | 内容 |
|---|---|
| **scope** | `LLRender::syncMatrices()` の Vulkan path 並走 (`vkCmdPushConstants` + UBO update path 投入)、GL path 残置 (sub-doc 03 §3.5 動作維持 = Vulkan path 並走で GL 描画は引続き正常) |
| **対象 file** | `indra/llrender/llrender.cpp` (`syncMatrices()` 周辺) + 必要に応じて `llvkloader.cpp` (UBO write helper / command buffer hook 追加) |
| **完了 marker** | syncMatrices Vulkan path 動作 (UBO write 後の persistent map 値変動確認 + push constant 投入 log) + GL path 並走動作 (既存 GL 描画 unchanged) |

### 5.3 δ 着手 task 候補 (推定 sub-step 内 step)

| step | 内容 | 推定規模 |
|---|---|---|
| δ-1 | UBO write path (`PerFrameMatrixUBO` + `TextureMatrixUBO` を `sPerFrameUboMapped[frame]` へ memcpy、syncMatrices GL path 直後に並走) | 0.5〜1 日 |
| δ-2 | push constant path (`vkCmdPushConstants` 呼出、現状 active command buffer が無いため hook 設計検討必要) | 0.5〜1 日 |
| δ-3 | build + AYA launch verify + log 確認 (UBO write 動作 + GL path 並走で従来描画維持) | verify |

**着手前に δ 細分化要否を AYA に確認** (sub-doc 03 §3.1.1 δ 行で「着手前に δ-1/δ-2 細分化要否を AYA 確認」明示済)。δ は β-2 と同等以上の規模 (command buffer hook + 2 path 並走の複雑度) の見込み、分割案 (δ-1 のみ先行 / δ-2 を別 commit) も有力候補。

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)
- **段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-β-2 + 3.3-γ 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **δ で UBO write + push constant の実 GPU 経路成立を確認**、ただし PSO bind / descriptor set bind / push constant 投入を実 draw 経路に組み込むのは 3.3 後続 sub-step (実 draw call 経由の Vulkan 描画実現は 3.3 終盤以降)
- **command buffer hook 設計**: 現状 Vulkan command buffer は `recordSkySmokeDraw` 等の transit smoke 用のみ、syncMatrices タイミングで active な per-frame command buffer は未配線。δ 着手前に「UBO write を syncMatrices 呼出時に行うか / per-frame command buffer 開始時に行うか」の設計判断が必要 (sub-doc 05 §3.5 で要 refine)
- **validation strict は sub-step 3.5 で別 build により実施** (本 sub-step 3.3-δ で validation = disabled の release build で AYA launch + WARN/ERR 0 件 という transit acceptance)
- **`identity_matrix` 残置可否は 3.3-B trace で再判断** (handoff `handoff-substep-3-3-beta-1-complete.md` §4.3 引継ぎ、β-2/γ では未触)

---

## 6. 関連 doc / memory cross-ref

### 6.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.3-δ marker active 化)
- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — Vulkan API 設計 (§3.5 二段構え、β-1 で sealed、γ で実 layout 反映)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-beta-2-complete.md` — 直前完了 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-beta-1-complete.md` — β-1 完了 handoff (役割完了済)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-2-complete.md` — 3.2 sky pool 完了 handoff (役割完了済)

### 6.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.3-δ 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§1.1 acceptance × log evidence 各 line 突合、init order 配線確認、shader 0 件 diff 確認)
- `feedback_one_step_at_a_time.md` — γ 着手前 AYA 1 件確認 (細分化案 A 採用)
- `feedback_no_auto_commit.md` — 本 handoff 完成後 commit は AYA 明示指示で実施
- `feedback_proactive_handoff.md` — sub-step 境界での能動 handoff 起草 (本 file)
- `feedback_log_reading.md` — AYA launch 後 Claude が `~/.ayastorm_x64/logs/AYAstorm.log` を grep して acceptance 7 marker + Vulkan WARN/ERR 0 件確認
