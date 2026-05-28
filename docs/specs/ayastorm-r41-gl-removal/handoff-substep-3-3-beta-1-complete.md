# r41 sub-step 3.3-β-1 完遂 → 3.3-β-2 着手境界 handoff (2026-05-29)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-2-complete.md` (3.2 sky pool 1 draw smoke-test 完遂 → 3.3 着手境界、`handoff-substep-3-2-complete.md` §6.2 task 候補 3.3-A の起点)
**本 handoff 位置付け**: sub-step 3.3-A (matrix stack 棚卸し + 二段構え layout 設計確定) → 3.3-α (spec refine) → **3.3-β-1 (UBO 構造体定義 + set=0 layout 設計 + spec sealed)** の全完遂宣言 + sub-step 3.3-β-2 (VkBuffer × 3 frame + descriptor pool/set allocation + persistent mapping) 着手前 scope 確認境界。

---

## 1. sub-step 3.3-β-1 全完遂 status (2026-05-29)

### 1.1 完遂 marker (sub-doc 03 §3.1.1 sub-step 3.3-β-1)

| acceptance | 達成 status |
|---|---|
| `PerFrameMatrixUBO` (binding 0 = projection 系 3 mat4 / 192 B / std140) C++ struct 定義 | ✓ `llvkloader.h` L66-71 |
| `TextureMatrixUBO` (binding 1 = `texture_matrix[0..3]` 4 mat4 / 256 B / std140) C++ struct 定義 | ✓ `llvkloader.h` L76-79 |
| `static_assert` で std140 layout size 確定 (192 B / 256 B) | ✓ `llvkloader.h` L72-73 / L80-81 (`sizeof(struct) == N` 検証) |
| `VkDescriptorSetLayout getPerFrameDescriptorSetLayout()` getter signature 追加 | ✓ `llvkloader.h` L87 (実装 body は β-2、現状は VK_NULL_HANDLE 返却前提) |
| spec sealed (sub-doc 03 §1.2 #3 / §1.2 特記 / §3.1 sub-step 3.3 marker / §3.1.1 + sub-doc 05 §3.5) | ✓ commit `62778dcac4` 投入済 |
| build smoke (`autobuild build -A 64 -c ReleaseFS_open --no-configure`) | ✓ exit 0、`[100%] Built target llpackage` (static_assert 通過 = struct size OK) |
| regression: 段階 1 + 段階 2 + 3.1b + 3.2 動作維持 | ✓ header-only 変更、cpp side 未触で 3.2 動作維持 |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-2-complete.md` | **役割完了** (3.3 着手 satisfy、本 handoff で内容引継ぎ) |
| sub-doc `03-state-machine-pso.md` | active 継続 (sub-step 3.3-β-2 marker 着手 ready 状態へ) |
| sub-doc `05-vulkan-api-design.md` | active 継続 (§3.5 二段構え full refine 反映済) |
| `handoff-substep-3-3-beta-1-complete.md` (本 handoff) | 新規作成 (3.3-β-1 全完遂 → 3.3-β-2 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.3-β-2 着手 ready 状態へ update) |

---

## 2. 実装内容 (commit `62778dcac4` 範囲)

### 2.1 spec sealing (sub-doc 03 + sub-doc 05)

| file | section | 変更内容 |
|---|---|---|
| sub-doc 03 | §1.2 #3 (llrender.cpp 行) | push constant 中身 `model_matrix` → `modelview_matrix` (GL 流儀継承)、shader 内計算移譲明記 (MVP/normal/inverse_modelview)、5/29 refine マーカー追加 |
| sub-doc 03 | §1.2 特記 #3 | 二段構え説明文を refine (UBO 縮約 6 mat4 → 3 mat4、push constant 64 B + UBO 448 B = 合計、`modelview` 分解 refactor 不採用理由記載) |
| sub-doc 03 | §3.1 sub-step 3.3 marker | push constant `model_matrix` → `modelview_matrix` GL 流儀継承、UBO binding 0 = projection 系 3 mat4 へ縮約、shader 内計算移譲明記、3.3-A 細分化記述に β-1/β-2 反映 |
| sub-doc 03 | §3.1.1 (3.3-A 細分化表) | 旧 3.3-β 行を **β-1 (UBO 構造体定義 + set=0 layout 設計 + spec sealed)** + **β-2 (VkBuffer + descriptor pool/set + persistent mapping)** に分割。β-1 = 本 commit、β-2 = 次 session |
| sub-doc 03 | §3.1.1 trace inventory | 2 行追加 (push constant 中身 + shader 内計算移譲) |
| sub-doc 05 | §3.5 | 二段構え full refine (旧 6 mat4 binding 0 → projection 系 3 mat4、texture_matrix binding 1 配線、modelview push constant、shader 内計算移譲) |

### 2.2 `llvkloader.h` 変更 (+37 / -0)

`namespace LLVKLoader` 内に **header-only** 追加 (.cpp 触らず):

```cpp
// r41 sub-step 3.3-β-1: per-frame matrix UBO layout (二段構え)
// sub-doc 03 §3.1.1 / sub-doc 05 §3.5 (AYA 確定 2026-05-29)
//
// 二段構え:
//   push constant : modelview_matrix (mat4 = 64 B、VERTEX_BIT、GL 流儀継承)
//   UBO binding 0 : PerFrameMatrixUBO (3 mat4 = 192 B)
//   UBO binding 1 : TextureMatrixUBO  (4 mat4 = 256 B)
//
// MVP / normal_matrix / inverse_modelview は vertex shader 内で
// `projection_matrix × modelview_matrix` 等から算出 (3.3-B 範疇)。

struct PerFrameMatrixUBO {
    float projection_matrix[16];
    float inverse_projection_matrix[16];
    float identity_matrix[16];
};
static_assert(sizeof(PerFrameMatrixUBO) == 192, ...);

struct TextureMatrixUBO {
    float texture_matrix[4][16];
};
static_assert(sizeof(TextureMatrixUBO) == 256, ...);

VkDescriptorSetLayout getPerFrameDescriptorSetLayout();
```

`getPerFrameDescriptorSetLayout()` の **実装 body は β-2 まで未配線** (header signature だけ先行追加して β-2 で .cpp 側を埋める方針)。当面の caller がいないため、β-2 完了までは未呼出のまま残置でも build はリンクエラーにならない (header-only declaration、定義は β-2 で `.cpp` 側に追加)。

### 2.3 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | +9 / -4 | β-1/β-2 split + modelview refine + shader 内計算移譲明記 |
| `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` | +9 / -10 | §3.5 二段構え full refine |
| `indra/llrender/llvkloader.h` | +37 | UBO struct × 2 + static_assert × 2 + getter signature |

合計: **3 file、+55 / -14 line** (`git diff HEAD~1 HEAD --stat` 確認済)

---

## 3. 設計 deviation 履歴 (3.3-A 着手時から本実装まで)

### 3.1 push constant 中身: `model_matrix` → `modelview_matrix` (GL 流儀継承)

着手時 (sub-doc 05 §3.5 初稿) は `model_matrix` (pure model、view は UBO 側) で設計。実装直前の自己検証 (LLRender::syncMatrices() trace 結果) で **AYAstorm の現状実装は `modelview` (view × model 結合済) を直接 shader に渡している** ことを確認。`modelview` を分解 (`model_matrix` + `view_matrix` 分離) する refactor は 3.3 scope 大幅超過 (call site が全 push 経路に分散、view-only 分離は LLRender::pushMatrix/popMatrix の再構築必要)。

**判断**: AYA 確認の上、**GL 流儀継承** = push constant に `modelview_matrix` を直接乗せる方針へ refine。view × model 分離は将来段階 (r42+) に持越し。

### 3.2 UBO binding 0 縮約: 6 mat4 (384 B) → 3 mat4 (192 B)

着手時設計は binding 0 = `view / projection / inverse_modelview / inverse_projection / normal_matrix / mvp_matrix` の 6 mat4 = 384 B。modelview 継承確定により **MVP / normal_matrix / inverse_modelview** は vertex shader 内で `projection × modelview` 等から算出可能となり、UBO から削減可能と判定。

**判断**: binding 0 = `projection_matrix` + `inverse_projection_matrix` + `identity_matrix` の 3 mat4 = 192 B へ縮約。shader 内計算は **3.3-B (shader port)** scope。`identity_matrix` 残置理由: AYAstorm 改変 13 file shader で identity 参照経路がある可能性 (3.3-B trace で再確認、無ければ削除候補)。

### 3.3 texture × 3 → × 4 是正

着手時 sub-doc 03 §1.2 #3 行は「texture × 3」記載。実 code (`llrender.h` L370-376: MM_TEXTURE0 / MM_TEXTURE1 / MM_TEXTURE2 / MM_TEXTURE3) で **4 texture matrix** と確認、spec drift を正面修正。

**判断**: `TextureMatrixUBO::texture_matrix[4][16]` 採用、256 B 確定。

### 3.4 β-1 / β-2 split

着手時 3.3-A は α/β/γ/δ/ε の 5 細分化。AYA から「段階を分けて安全に」「粒度過大化防止」明示指示を受け、β を更に β-1 (struct + layout 設計 + spec sealed) / β-2 (VkBuffer + descriptor + persistent mapping) に分割。

**判断**: 案 A (β-1 + β-2) を AYA 採用。本 handoff は β-1 完遂 → β-2 着手境界。

---

## 4. risks / caveats

### 4.1 `getPerFrameDescriptorSetLayout()` は body 未配線

header signature だけ先行追加した状態。β-2 で `.cpp` 側に body 追加 (createDescriptorSetLayout + 保持 global) するまで、本 getter は **呼び出すと unresolved symbol で linker error** になる。現状 caller ゼロのため build は通っているが、β-2 で `.cpp` 側 body 追加または stub return が必須。

### 4.2 std140 layout assumption

`static_assert(sizeof(PerFrameMatrixUBO) == 192)` は **C++ 側 struct を std140 layout と仮定** したサイズ確認。実態:
- `mat4 = 16 float = 64 B`、std140 で 16 B align (mat4 は要件満たす)
- 3 mat4 連続 = 192 B、4 mat4 連続 = 256 B (pack 揃い)
- GLSL std140 と C++ struct がそのまま binary 一致するのは「全 member が mat4」のため (vec3 等が混じると padding 罠あり)

将来 UBO に vec3 / float を追加する場合は std140 padding 規則を再確認。

### 4.3 `identity_matrix` 残置の妥当性 (3.3-B 確認事項)

UBO binding 0 に `identity_matrix` を残置したが、shader 側 (AYAstorm 改変 13 file + base 248 file) で identity_matrix 参照経路があるか 3.3-B trace 時に再確認。参照ゼロなら binding 0 = 2 mat4 = 128 B 縮約も可能 (β-2 buffer allocation 前なら spec 改訂で済む)。

### 4.4 sky smoke / placeholder PSO の VkPipelineLayout 未更新

3.1b / 3.2 で配置した 2 PSO の VkPipelineLayout は **空 layout** (descriptor set / push constant 共に無し)。β-2 で descriptor set=0 layout を作成しても、両 PSO は引続き空 layout のままで `vkCmdBindDescriptorSets` 投入が無効。

**判断**: 3.3-γ (sub-doc 03 §3.1.1) で 2 PSO の VkPipelineLayout を二段構え準拠化 (set=0 binding + push constant range) して、初めて descriptor set bind 経路成立。β-2 はあくまで UBO buffer + descriptor pool/set の物理 allocation のみ、PSO 側統合は γ。

---

## 5. next session entry point

### 5.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読
3. sub-doc 03 §3.1.1 sub-step 3.3-β-2 marker 再確認 + §1.2 #3 二段構え記述確認
4. sub-doc 05 §3.5 二段構え full refine 確認
5. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 sub-step 3.3-β-2 着手内容 (sub-doc 03 §3.1.1)

| 項目 | 内容 |
|---|---|
| **scope** | β-1 layout base で `VkBuffer` (HOST_VISIBLE_COHERENT、frame in flight 3 個 × 448 B = ~1.4 KB) + `VkDescriptorPool` + `VkDescriptorSet` allocation + persistent mapping + 初期 zero write smoke |
| **対象 file** | `indra/llrender/llvkloader.cpp` (header は β-1 で確定済) |
| **完了 marker** | UBO buffer 作成成功 + descriptor pool / set 0 allocation 成功 + persistent mapping 取得成功 + 初期 zero write で validation 0 件 |

### 5.3 β-2 着手 task 候補 (推定 sub-step 内 step)

| step | 内容 | 推定規模 |
|---|---|---|
| β-2-1 | `getPerFrameDescriptorSetLayout()` body 実装 (anonymous namespace global + create + destroy 配線) | 0.5 日 |
| β-2-2 | UBO buffer 3 個 (frame in flight) 配置 + VMA or 生 `vkAllocateMemory` 経由 HOST_VISIBLE_COHERENT 確保 + persistent map | 0.5-1 日 |
| β-2-3 | `VkDescriptorPool` (1 set × 2 binding) + `VkDescriptorSet` allocate + `vkUpdateDescriptorSets` (buffer info × 2) 配線 | 0.5 日 |
| β-2-4 | 初期 zero write smoke + validation 0 件確認 + 着手前に β-2 細分化要否を AYA 確認 (本 step リスト自体が細分化案) | 0.5 日 |

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)
- **段階 1 + 段階 2 + 3.1b + 3.2 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **β-2 は header-only 不可、`.cpp` 側 implementation 必須**、build 失敗時の影響範囲は llvkloader.cpp のみ (header 経由の他 file への波及無し)
- **VMA 採用判断は β-2 着手時に再確認** — sub-doc 03 §3.1.1 β-2 行では「HOST_VISIBLE_BIT 経由 host write or `VK_EXT_memory_priority` 経由 device」記載、実機 RTX 5090 では HOST_VISIBLE_COHERENT が単純で十分 (1.4 KB は memory pressure 対象外)。VMA library 導入は領域 7 (charter §6 領域 7) と coordinate
- **`identity_matrix` 残置可否は 3.3-B trace で再判断** (本 handoff §4.3)
- **`getPerFrameDescriptorSetLayout()` β-2 未完了時の linker error 回避** (本 handoff §4.1) — β-2 着手中は最低でも stub return `VK_NULL_HANDLE` で実装 body だけは置く
- **validation strict は sub-step 3.5 で別 build により実施** (本 sub-step 3.3-β-2 で validation = disabled の release build で run + error log 0 件 という transit acceptance、strict 確認は 3.5)

---

## 6. 関連 doc / memory cross-ref

### 6.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.3-β-2 marker active 化)
- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — Vulkan API 設計 (§3.5 二段構え full refine 反映済)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-2-complete.md` — 直前完了 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — SPIR-V 化 (3.3-B shader matrix port で cross-ref、起草は未)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — descriptor + render pass (3.3-β-2 / γ で cross-ref、起草は未)

### 6.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.3-β-2 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_render_full_trace_first.md` — 本 sub-step 3.3-A で適用 (llrender 棚卸し agent 経由)
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§2.3 file/line / §3.1-3.4 deviation 履歴)
- `feedback_no_auto_commit.md` — 本 handoff 完成後 commit は AYA 明示指示で実施
- `feedback_proactive_handoff.md` — sub-step 境界での能動 handoff 起草 (本 file)
- `feedback_one_step_at_a_time.md` — 3.3-A → 3.3-α → 3.3-β-1 へ細分化適用
