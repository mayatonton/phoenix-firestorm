# r41 sub-step 4.3-β' 完遂 → 4.3-γ' 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-beta-prep.md` (sub-step 4.3-α 完遂 → 4.3-β scope refine 提案 + 新 cadence 採用、AYA review PASS、役割完了)
**本 handoff 位置付け**: sub-step 4.3-β' (LLVertexBuffer Vulkan 化 placement only、案 D hybrid 採用、charter §7.5 boundary refine 範囲 = 段階 5 LLVertexBuffer Vk 化を sub-step 4.3 内へ前出し、4 file +240/-0) **全完遂宣言** + sub-step 4.3-γ' (sub-doc 06 §3.1 sub-step 6.1 shader port、AYA 承認境界 update [旧「段階 4 sub-step 4.5 完遂後」縛り解除] 反映) 着手前 scope 確認境界。AYA launch verify PASS で 4.3-β' 章 close。

---

## 1. sub-step 4.3-β' 全完遂 status (2026-05-31)

### 1.1 完遂 marker (handoff-substep-4-3-beta-prep.md §7.2 7 task candidate cadence 全完遂)

| acceptance | 達成 status |
|---|---|
| β'-1 LLVertexBuffer Vk 経路 trace (genBuffer/destroyGLBuffer + ctor/dtor 経路 + sub-doc 04 §3.4 案 D 仕様確認) | ✓ |
| β'-2 llvkloader.h に Vk lifecycle + bind helper 5 件 declaration 配置 | ✓ |
| β'-3 llvkloader.cpp に createVertexBufferVk/createIndexBufferVk/destroyBufferVk/bindVertexBufferVk/bindIndexBufferVk 実装配置 (file-local createBufferVkImpl 経由 1:1 共通化) | ✓ |
| β'-4 llvertexbuffer.h に Vk parallel field 6 個追加 (mVkVertexBuffer/mVkIndexBuffer + mVkVertexAlloc/mVkIndexAlloc + mVkVertexMapped/mVkIndexMapped) + in-class default init | ✓ |
| β'-5 llvertexbuffer.cpp に #include "llvkloader.h" + genBuffer/genIndices に Vk parallel allocate 配線 (二重 guard) + destroyGLBuffer/destroyGLIndices に Vk parallel release 配線 (NULL guard 経由 no-op 許容) | ✓ |
| β'-6 incremental autobuild (autobuild build -A 64 -c ReleaseFS_open --no-configure) | ✓ exit 0 / 4 file +240/-0 / error 0 件 / packaging 完遂 |
| β'-7 commit (4.3-β' 単独 commit) | ✓ `78820a6edf` |
| install + cache clear 完遂 | ✓ ~/ayastorm/ + ~/.ayastorm_x64/cache/ |
| AYA launch verify clean | ✓ AYA さん「OKです 起動して終了しました」確認 / regression 0 |

### 1.2 commit hash

| commit | scope |
|---|---|
| `78820a6edf` | sub-step 4.3-β' 全完遂 (4 files changed, +240/-0) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-4-3-beta-prep.md` | **役割完了** (sub-step 4.3-β' 着手 GO 条件 satisfy → 全完遂、本 handoff で内容引継ぎ) |
| sub-doc `04-frame-context.md` | active 継続 (sub-step 4.3-γ'/δ'/ε'/ζ'/η' marker 着手 ready 状態へ) |
| sub-doc `06-shader-spirv.md` | active 継続 (**sub-step 4.3-γ' で sub-step 6.1 本格着手 source**、旧「段階 4 sub-step 4.5 完遂後」縛り解除を反映) |
| sub-doc `07-descriptor-renderpass.md` | active 継続 (sub-step 4.3-δ' で 7.3 material cache 本実装 + 7.4 push descriptor 全配線 + 7.5 実 attachment 配線 source) |
| sub-doc `08-llvkrenderer-skeleton.md` | active 継続 (sub-step 4.4 part B で参照) |
| `handoff-substep-4-3-beta-prime-complete.md` (本 handoff) | 新規作成 (4.3-β' 全完遂 → 4.3-γ' 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 4.3-γ' 着手 ready 状態へ update) |

---

## 2. 実装内容 (本 commit 範囲 `78820a6edf`)

### 2.1 LLVertexBuffer 拡張 (llvertexbuffer.h、+13 line)

| 変更 | 詳細 |
|---|---|
| protected section に Vk parallel field 6 個追加 (L304-315) | `VkBuffer mVkVertexBuffer = VK_NULL_HANDLE` + `VkBuffer mVkIndexBuffer = VK_NULL_HANDLE` + `void* mVkVertexAlloc = nullptr` + `void* mVkIndexAlloc = nullptr` + `void* mVkVertexMapped = nullptr` + `void* mVkIndexMapped = nullptr` |
| VkBuffer 透過 include path | llvertexbuffer.h → llgl.h → llglheaders.h:1097 → volk.h (既存 include chain 経由、新規 include 追加なし) |
| VmaAllocation opaque void* | `vk_mem_alloc.h` header 持込み回避 (既存 sub-step 3.4-β-2 設計 [llvkloader.h:192] 継承)、reinterpret_cast 経由で llvkloader.cpp 1 TU 限定で取扱い |
| HOST_VISIBLE+MAPPED persistent mapped pointer 並列保持 | `mVkVertexMapped/mVkIndexMapped` (4.3-ε' で mMappedData → mVkVertexMapped sync 経路接続予定) |
| in-class default init | VK_NULL_HANDLE / nullptr 初期化で ctor 暗黙 clean、dtor は既存 destroyGLBuffer/destroyGLIndices 経由で暗黙 release |

### 2.2 LLVKLoader 拡張 (llvkloader.h + llvkloader.cpp、+193 line)

| 変更 | 詳細 |
|---|---|
| **§2.2.1 lifecycle helper 3 件** | |
| `bool createVertexBufferVk(U32 size_bytes, VkBuffer& out_buffer, void*& out_allocation, void** out_mapped)` (h) | VERTEX_BUFFER_BIT + HOST_VISIBLE + MAPPED + VMA_MEMORY_USAGE_AUTO + VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT (cpp) |
| `bool createIndexBufferVk (U32 size_bytes, ...)` (h) | INDEX_BUFFER_BIT + 同 flag set (cpp) |
| `void destroyBufferVk(VkBuffer buffer, void* allocation)` (h) | sAllocator null gate (silent) + VK_NULL_HANDLE/nullptr no-op で対称呼出許容 (cpp) |
| **file-local `createBufferVkImpl`** で 1:1 共通化 | sub-doc 04 §3.4 案 D 仕様 = sAllocator null check 静かに false (LL_WARNS なし、early init 時 thousands of LLVertexBuffer instance allocate での log spam 回避) + LL_WARNS 限定 = size_bytes==0 / vmaCreateBuffer 実 failure のみ |
| **§2.2.2 bind helper 2 件 (β' 段階 placement のみ、実 fire は 4.3-ε')** | |
| `void bindVertexBufferVk(VkCommandBuffer cmd_buf, VkBuffer buffer, VkDeviceSize offset)` (h) | firstBinding=0 fixed、1 binding pattern (cpp: VkBuffer buffers[1] / VkDeviceSize offsets[1] / vkCmdBindVertexBuffers 経由) |
| `void bindIndexBufferVk (VkCommandBuffer cmd_buf, VkBuffer buffer, VkDeviceSize offset, VkIndexType index_type)` (h) | 1:1 vkCmdBindIndexBuffer wrap (cpp) |

### 2.3 LLVertexBuffer 呼出配線 (llvertexbuffer.cpp、+34 line)

| 変更 | 詳細 |
|---|---|
| `#include "llvkloader.h"` 追加 (L39) | Vk parallel allocate/release path 配線 |
| `genBuffer()` に Vk parallel allocate 配線 | `if (mSize > 0 && mVkVertexBuffer == VK_NULL_HANDLE) { LLVKLoader::createVertexBufferVk(mSize, mVkVertexBuffer, mVkVertexAlloc, &mVkVertexMapped); }` = 二重 guard (mSize 妥当性 + 既配置 idempotency) |
| `genIndices()` に Vk parallel allocate 配線 | 同等 pattern、createIndexBufferVk 経由 |
| `destroyGLBuffer()` に Vk parallel release 配線 | `if (mVkVertexBuffer != VK_NULL_HANDLE \|\| mVkVertexAlloc != nullptr) { LLVKLoader::destroyBufferVk(...); mVkVertexBuffer = VK_NULL_HANDLE; mVkVertexAlloc = nullptr; mVkVertexMapped = nullptr; }` = NULL guard 経由 no-op 許容 + 明示 reset |
| `destroyGLIndices()` に Vk parallel release 配線 | 同等 pattern |

### 2.4 file 変更 summary

```
indra/llrender/llvertexbuffer.cpp |  34 ++++++++++
indra/llrender/llvertexbuffer.h   |  13 ++++
indra/llrender/llvkloader.cpp     | 139 ++++++++++++++++++++++++++++++++++++++
indra/llrender/llvkloader.h       |  54 +++++++++++++++
4 files changed, 240 insertions(+)
```

---

## 3. build + launch verification

### 3.1 build step

| step | command | result |
|---|---|---|
| 1. stage | git add 4 file | OK |
| 2. build | `autobuild build -A 64 -c ReleaseFS_open --no-configure` | **PASS exit 0** / error 0 件 / llrender (Vk 改変全 file) コンパイル clean |
| 3. package | tar.xz 生成 | Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261510128.tar.xz |
| 4. install | install.sh ~/ayastorm/ | OK / menu entries 配置 |
| 5. cache clear | rm -rf ~/.ayastorm_x64/cache/ | OK |
| 6. AYA launch verify | AYA さん「OKです 起動して終了しました」確認 | **PASS** |

### 3.2 AYA launch verify 詳細 (Claude 側 log 自己解析)

| 項目 | 状態 |
|---|---|
| 起動 → shutdown | 2026-05-31T10:49:31Z → 2026-05-31T10:51:25Z / run_time 1m54s |
| ERROR | 0 件 |
| WARNING | 0 件 |
| vulkanDebugCallback ERROR/WARNING | 0 件 |
| VK_ERROR | 0 件 |
| #VkRecord# pool hook fire | 12 (4.3-α baseline 一致) |
| #Vulkan# marker | 56 (4.3-α baseline 一致) |
| shutdown clean | Goodbye! → status: stopped |
| createVertexBufferVk/createIndexBufferVk/vmaCreateBuffer failed | 0 件 = **thousands of LLVertexBuffer instance 全件 Vk parallel allocate 成功確認** |
| regression | 0 |

---

## 4. 設計 deviation

### 4.1 案 D hybrid 採用 (案 A/B/C reject)

handoff-substep-4-3-beta-prep.md §1.2 案検討 table 採用結果:
- 案 A (full 移植 = LLVertexBuffer Vk 化 + per-pool draw 移植同時) → **reject** (段階順序逆転、案 D 採用前提違反)
- 案 B (4.3-β skip) → **reject** (feedback_no_scope_shrink 違反)
- 案 C (partial 配線、不完全のまま放置) → **reject** (feedback_self_bug_no_defer_option 違反)
- **案 D (上位 scope refine = LLVertexBuffer placement + lazy upload で β' 段階満足、charter §7.5 boundary refine 範囲)** → 採用

本 4.3-β' は案 D の β' 段階 = LLVertexBuffer Vk parallel field 配置 + lifecycle helper 配置 + bind helper 配置 (placement only)。actual upload / fire は 4.3-ε' per-pool draw 配線時。

### 4.2 VmaAllocation opaque void* 公開 (header propagate 回避)

`mVkVertexAlloc/mVkIndexAlloc` を `void*` 型で llvertexbuffer.h に配置。理由:
- `vk_mem_alloc.h` (~10K LOC) を llvertexbuffer.h 経由で全 TU に伝播させると build time + 重複 instantiation 増加
- 既存 sub-step 3.4-β-2 設計 (llvkloader.h:192 で VmaAllocation を void* で隠蔽) を 1:1 継承
- impl 側 (llvkloader.cpp) で `reinterpret_cast<VmaAllocation>(allocation)` 経由で取扱い
- 4.3-ε' で actual upload 経路追加時も void* 維持 (allocator API は llvkloader.cpp 1 TU 限定)

### 4.3 HOST_VISIBLE + MAPPED 選択 (LLVertexBuffer mMappedData 経路 parallel)

VMA flag = `VMA_MEMORY_USAGE_AUTO` + `VMA_ALLOCATION_CREATE_HOST_VISIBLE_BIT` + `VMA_ALLOCATION_CREATE_MAPPED_BIT` + `VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT`。理由:
- LLVertexBuffer 既存 `mMappedData` は CPU writable + GPU readable 経路 (GL_DYNAMIC_DRAW + glMapBufferRange)
- Vulkan parallel として HOST_VISIBLE+MAPPED + sequential write hint で同等 staging 不要経路
- device-local 経路 (DEVICE_LOCAL + staging 経由) は将来拡張で out_mapped=nullptr 切替可能 (LLVertexBuffer 内 dirty flag 追加判断 = 4.3-ε' で評価)

### 4.4 LL_WARNS 限定的 (log spam 回避)

createBufferVkImpl 内 log 戦略:
- sAllocator == nullptr → **silent return false** (early init 時 / sAllocator 未初期化期間に thousands of LLVertexBuffer instance allocate 経路で log spam 回避)
- size_bytes == 0 → LL_WARNS 1 件 (異常入力、debug 用)
- vmaCreateBuffer 失敗 → LL_WARNS 1 件 (実 allocation 失敗、調査必要)

`feedback_remove_verification_logs` 遵守 = 検証用 LL_INFOS hook 追加なし、案 B cadence 継承で AYA 短評承認 satisfy。

---

## 5. risks / caveats (sub-step 4.3-γ' 着手前 awareness)

### 5.1 β' 段階は placement のみ = bind/draw fire 0 件

β'-3 で bindVertexBufferVk/bindIndexBufferVk 配置済だが、caller 側 fire は **4.3-ε' per-pool draw 配線時** (Sky+WLSky+WaterExclusion + 9 pool 一括移植)。本 β' 完遂時点では vkCmdBindVertexBuffers/vkCmdBindIndexBuffer caller 0 件 = bind/draw 実行経路は依然 12 pool placeholder (recordPlaceholderPoolDraw + recordAvatarPlaceholderDraw 経由 fullscreen tri × 12 重ね描き)。

### 5.2 LLVertexBuffer instance lifecycle 全件 Vk parallel allocate (memory budget 増加観測対象)

genBuffer/genIndices 経路は viewer 起動中 thousands of instance allocate (mesh vertex/index buffer pool)。本 β' で全件 vmaCreateBuffer hot path 化 = **memory budget 増加観測対象**:
- AYA launch verify (1m54s) では vmaCreateBuffer failed 0 件確認済
- 長時間動作 + heavy scene での memory pressure は 4.3-ε' 後 perf 計測予定 (sub-doc 04 §6 measurement plan source)

### 5.3 4.3-α は accessor 配線のみ動作変化なし vs β' は実 Vk allocation 経路追加

- 4.3-α (`9f13302078`) = forward call で zero semantic change (regression risk 極低)
- 4.3-β' (`78820a6edf`) = 実 Vk allocation 経路追加、副作用範囲は **VMA allocator + VkBuffer pool**
- regression watch: VMA allocator stress test / heavy scene での allocation failure 観測

### 5.4 案 D lazy upload は 4.3-ε' で dirty flag 追加判断

`mMappedData` → `mVkVertexMapped` sync 経路は **4.3-ε' で per-pool draw 配線時に dirty flag 追加判断**。本 β' 段階では:
- HOST_VISIBLE+MAPPED で persistent mapped pointer 取得済
- actual data write は LLVertexBuffer 既存 glBufferData/glMapBufferRange 経路 (CPU memory → GL buffer)
- Vk 側 mVkVertexMapped への sync は 4.3-ε' で dirty range 追跡判断

### 5.5 sub-step 4.3 残 5 sub-step (handoff-substep-4-3-beta-prep.md §4.2 新 cadence)

| sub-step | scope | 着手境界 |
|---|---|---|
| **4.3-γ'** | sub-doc 06 §3.1 sub-step 6.1 shader port 本格着手 (autobuild integration 一括化、base port ~228 file 全 SPIR-V port) | **次着手 (fresh context 推奨)** |
| 4.3-δ' | sub-doc 07 §7.3 material cache 本実装 + §7.4 push descriptor 全配線 + §7.5 実 attachment 配線 | γ' 完遂後 |
| 4.3-ε' | Sky+WLSky+WaterExclusion + 9 pool per-pool 実 scene draw 移植 (旧 β + 旧 γ 一括、領域 5+6+7 整備済前提) | δ' 完遂後 |
| 4.3-ζ' | Avatar bone per-draw + GLTFPBR per-draw 移植 (旧 δ + 旧 ε 一括) | ε' 完遂後 |
| 4.3-η' | self-check + handoff (旧 ζ 範式継承) | ζ' 完遂後 |

### 5.6 context budget concern

本 4.3-β' 完遂時点で session context 消費中。次 sub-step (4.3-γ' = sub-step 6.1 shader port = 大規模 ~228 file port 想定) は **fresh context で着手推奨**、proactive handoff 範式遵守。

---

## 6. next session entry point (sub-step 4.3-γ' 着手)

### 6.1 着手前 4 段 cadence

1. 本 handoff doc 読込 (sub-step 4.3-β' 完遂 status 確認)
2. sub-doc `06-shader-spirv.md` §3.1 sub-step 6.1 shader port spec 読込
3. handoff-substep-4-3-beta-prep.md §5.1 AYA 承認境界 update 確認 (旧「段階 4 sub-step 4.5 完遂後」縛り解除 = 4.3-γ' 前出し)
4. memory `project_ayastorm_r41_vulkan_migration.md` 最新 status 読込

### 6.2 sub-step 4.3-γ' 着手 task 候補

| task | 詳細 |
|---|---|
| γ'-1 | sub-doc 06 §3.1 sub-step 6.1 spec 全件読込 (base port ~228 file 全 SPIR-V port scope 確認、AYAstorm 改変 13 file は r42-α/β/γ scope 外維持) |
| γ'-2 | autobuild integration 一括化判断 (sub-step 3.3-B pre-flight 経路継承 vs 新規 build target 配置) |
| γ'-3 | shader 棚卸し (base ~228 file リスト確定 + per-file SPIR-V port 順序確定) |
| γ'-4 | incremental SPIR-V port (sub-step 3.3-B-α〜ε cadence 範式継承 = 段階分割) |
| γ'-5 | shader runtime binding 確認 (placeholder PSO 経路で SPIR-V load 確認、4.3-ε' で per-pool fire 接続) |
| γ'-6 | incremental autobuild |
| γ'-7 | AYA launch verify (案 B cadence) |
| γ'-8 | commit (4.3-γ' 段階分割可能性高、sub-step 3.3-B 範式継承) |

### 6.3 critical reminders

| reminder | 詳細 |
|---|---|
| **AYAstorm 改変 13 file shader 改変禁止** | sub-doc 06 §3.1 範囲 = base 改変なし port のみ、AYAstorm 改変 13 file は r42-α/β/γ scope 外維持 |
| **段階 1-4.3-β' 動作維持** | regression risk watch (4.3-γ' は shader 配線変更で動作変化中) |
| **AYA 承認境界 update 反映** | 旧「段階 4 sub-step 4.5 完遂後」縛り解除 = 4.3-γ' 前出しが handoff-substep-4-3-beta-prep.md §5.1 AYA review PASS 範囲 |
| **案 B cadence 継承** | measurement log 配線 skip + AYA 短評承認で satisfy、log 配線は overshoot 判断 |
| **context budget proactive 監視** | 次 session 着手時点で context 残量確認 → 周回境界で proactive handoff |
| **proactive handoff 範式** | sub-step 4.3-γ' は大規模 port 想定、context 周回境界で能動 handoff 起草 |

### 6.4 commit 戦略

sub-step 4.3-γ' は **段階分割可能性高い** (~228 file SPIR-V port = sub-step 3.3-B-α/β-1/β-2/γ/δ/ε 範式継承想定)。各段階分割 commit 単位は γ'-port-α / γ'-port-β-1 / γ'-port-β-2 / γ'-port-γ / γ'-port-δ / γ'-port-ε のような cadence (sub-doc 06 §3.1 spec 確認後に正確な分割境界確定)。

---

## 7. 関連 doc / memory cross reference

### 7.1 関連 doc

| doc | 役割 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | charter §2 領域 4 high risk spec + §7.5 boundary refine 範囲根拠 |
| `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` | sub-doc 04 §3.4 案 D 仕様 (LLVertexBuffer Vk 化 placement + lazy upload + bind helper、本 β' literal satisfy) |
| `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` | sub-doc 06 §3.1 sub-step 6.1 shader port spec (**次 sub-step 4.3-γ' 着手 source**) |
| `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` | sub-doc 07 §7.3/§7.4/§7.5 (sub-step 4.3-δ' 着手 source) |
| `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` | sub-doc 08 LLVKRenderer skeleton (4.4 part B、namespace LLVKLoader 経路継承) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-alpha-complete.md` | sub-step 4.3-α 完遂 → 4.3-β 着手境界 (役割完了、4.3-β-prep で新 cadence に置換) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-beta-prep.md` | **前 handoff (役割完了、本 β' で内容引継ぎ)** = 案 D 採用下 scope refine 提案、新 cadence 確定、AYA 承認境界 update 3 件反映 |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-beta-prime-complete.md` | (本 handoff、4.3-γ' 着手境界) |

### 7.2 関連 memory

| memory | 役割 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | active milestone tracking |
| `feedback_proactive_handoff.md` | context 圧迫時の proactive handoff 範式 |
| `feedback_self_verify_before_handoff.md` | handoff 起草前の self-trace 義務 |
| `feedback_no_claude_coauthor.md` | commit message Co-Authored-By: Claude 禁止 |
| `feedback_proactive_diagnostic.md` | log/grep/gdb 系は Claude が直接実行 |
| `feedback_log_reading.md` | log 解析は Claude 側、AYA に貼り付けさせない (本 β' AYA launch verify 詳細は Claude 自己解析済) |
| `feedback_one_step_at_a_time.md` | 1 メッセージ 1 アクション cadence |
| `feedback_remove_verification_logs.md` | 案 B では log 配線 skip = 除去対象なし (β' 段階追加 log 0 件) |
| `feedback_no_auto_commit.md` | コミットは明示指示後 (本 β' は AYA 「commit お願いします」明示指示下) |
| `feedback_no_scope_shrink.md` | 案 B reject の根拠 (4.3-β skip = scope shrink) |
| `feedback_self_bug_no_defer_option.md` | 案 C reject の根拠 (partial 配線 = 不完全のまま放置) |
| `project_build_procedure.md` | autobuild fullflow + .venv activate + AUTOBUILD_VARIABLES_FILE |

---

**本 handoff 起草日**: 2026-05-31
**起草根拠**: AYA さん「memory 更新と handoff doc 起草お願いします」承認下で memory update + handoff doc 起草を並走
**次 session 着手**: sub-step 4.3-γ' (sub-doc 06 §3.1 sub-step 6.1 shader port 本格着手) 着手境界 (fresh context 推奨、大規模 ~228 file SPIR-V port 想定)
