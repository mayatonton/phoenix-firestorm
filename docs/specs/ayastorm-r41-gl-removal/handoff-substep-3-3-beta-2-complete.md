# r41 sub-step 3.3-β-2 完遂 → 3.3-γ 着手境界 handoff (2026-05-29)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-beta-1-complete.md` (3.3-β-1 = matrix UBO struct 定義 + spec sealed 完遂 → 3.3-β-2 着手境界)
**本 handoff 位置付け**: sub-step 3.3-β-2 (VkBuffer × 3 frame + descriptor pool/set allocation + persistent mapping + zero write smoke) 全完遂宣言 + sub-step 3.3-γ (placeholder + sky smoke PSO の VkPipelineLayout 二段構え準拠化) 着手前 scope 確認境界。

---

## 1. sub-step 3.3-β-2 全完遂 status (2026-05-29)

### 1.1 完遂 marker (sub-doc 03 §3.1.1 sub-step 3.3-β-2)

| acceptance | 達成 status |
|---|---|
| `VkDescriptorSetLayout` set=0 (binding 0/1 = UNIFORM_BUFFER, stage = VERTEX\|FRAGMENT) 作成成功 | ✓ log: `Per-frame descriptor set layout created (binding 0=PerFrameMatrixUBO, 1=TextureMatrixUBO)` (llvkloader.cpp:659) |
| `VkBuffer` × 3 (HOST_VISIBLE_COHERENT、512 B/frame、PerFrame @ offset 0 + Texture @ offset 256) 作成成功 + persistent mapping 取得成功 + 初期 zero write | ✓ log: `Per-frame UBO buffers created (3 frames × 512 B, HOST_VISIBLE_COHERENT + persistent map + zero write)` (llvkloader.cpp:725) |
| `VkDescriptorPool` (3 set × 2 binding = 6 descriptor) 作成 + `VkDescriptorSet` × 3 alloc + `vkUpdateDescriptorSets` (buffer info × 2 × 3) 配線 | ✓ log: `Per-frame descriptor sets allocated + updated (3 sets × 2 binding)` (llvkloader.cpp:800) |
| `getPerFrameDescriptorSetLayout()` body 実装 (β-1 で signature 先行) | ✓ llvkloader.cpp:1330-1334 |
| Vulkan 系 `failed` / WARN / ERR 0 件 (release build / validation disabled、transit acceptance) | ✓ grep 結果 0 hit (RTX 5090 / Vulkan 1.4.319) |
| build smoke (`autobuild build -A 64 -c ReleaseFS_open --no-configure`) | ✓ exit 0、`[100%] Built target llpackage` + `finished` |
| regression: 段階 1 + 段階 2 + 3.1b + 3.2 動作維持 | ✓ Placeholder PSO + Sky smoke PSO 引続き compile + log 確認 |
| AYA launch verify (`~/ayastorm/ayastorm` 起動 + 起動完了) | ✓ AYA 確認済 (2026-05-29) |

### 1.2 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-3-3-beta-1-complete.md` | **役割完了** (3.3-β-2 着手 satisfy、本 handoff で内容引継ぎ) |
| sub-doc `03-state-machine-pso.md` | active 継続 (§3.1.1 β-2 行を「完遂 2026-05-29」+ 512 B/frame 実装結果反映済、γ 着手 ready) |
| sub-doc `05-vulkan-api-design.md` | active 継続 (β-1 で sealed 済、本 sub-step では未変更) |
| `handoff-substep-3-3-beta-2-complete.md` (本 handoff) | 新規作成 (3.3-β-2 全完遂 → 3.3-γ 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 3.3-γ 着手 ready 状態へ update) |

---

## 2. 実装内容 (commit 範囲)

### 2.1 spec sealing (sub-doc 03 §3.1.1)

| file | section | 変更内容 |
|---|---|---|
| sub-doc 03 | §3.1.1 β-2 行 | 「(完遂 2026-05-29)」maker 追加 + 512 B/frame 実装結果反映 (元 spec の 448 B は PerFrame+Texture 単純合計、256 B align 安全側で 512 B/frame 実装) + 完遂 marker を log 出力 + AYA launch PASS で update |

### 2.2 `llvkloader.cpp` 変更 (+239 / -0)

`namespace LLVKLoader` 内 anon namespace に **3 件 state + 3 件 create 関数** 追加、initVulkan + shutdownVulkan 配線、public getter body 実装:

**anon ns state (constants + globals)**:
```cpp
constexpr U32          FRAMES_IN_FLIGHT      = 3;
constexpr VkDeviceSize PERFRAME_UBO_OFFSET   = 0;
constexpr VkDeviceSize PERFRAME_UBO_SIZE     = sizeof(PerFrameMatrixUBO);  // 192
constexpr VkDeviceSize TEXTURE_UBO_OFFSET    = 256;                        // 256 B align
constexpr VkDeviceSize TEXTURE_UBO_SIZE      = sizeof(TextureMatrixUBO);   // 256
constexpr VkDeviceSize UBO_BUFFER_SIZE_FRAME = TEXTURE_UBO_OFFSET + TEXTURE_UBO_SIZE;  // 512

VkDescriptorSetLayout sPerFrameDescriptorSetLayout;
VkDescriptorPool      sPerFrameDescriptorPool;
VkBuffer              sPerFrameUboBuffer[FRAMES_IN_FLIGHT];
VkDeviceMemory        sPerFrameUboMemory[FRAMES_IN_FLIGHT];
void*                 sPerFrameUboMapped[FRAMES_IN_FLIGHT];
VkDescriptorSet       sPerFrameDescriptorSet[FRAMES_IN_FLIGHT];
```

**anon ns create 関数 (3 件、各 bool 返却)**:
- `createPerFrameDescriptorSetLayout()` — set=0 layout (binding 0/1 = UNIFORM_BUFFER、stage = VERTEX\|FRAGMENT)
- `createPerFrameUbos()` — VkBuffer × 3 + findMemoryType (HOST_VISIBLE | HOST_COHERENT) + vkAllocateMemory + vkBindBufferMemory + vkMapMemory (persistent) + 起動時 memset(0)
- `createPerFrameDescriptorSets()` — VkDescriptorPool (maxSets=3, descriptorCount=6) + vkAllocateDescriptorSets × 3 + vkUpdateDescriptorSets (2 binding × 3 set)

**initVulkan hook**: `createPipelineCache()` 成功後、`createPlaceholderPipeline()` 前に `createPerFrameDescriptorSetLayout() || createPerFrameUbos() || createPerFrameDescriptorSets()` 連鎖呼出 (失敗時 shutdownVulkan + return false)

**shutdownVulkan teardown** (sPipelineCache destroy の前):
- `sPerFrameDescriptorPool` 破棄 (set は自動 free)
- `sPerFrameUboBuffer[i]` / `sPerFrameUboMemory[i]` / `sPerFrameUboMapped[i]` 3 frame 分の unmap + destroy + free
- `sPerFrameDescriptorSetLayout` 破棄

**public `getPerFrameDescriptorSetLayout()` body** (β-1 で signature 先行、本 sub-step で .cpp 側追加):
```cpp
VkDescriptorSetLayout getPerFrameDescriptorSetLayout()
{
    return sPerFrameDescriptorSetLayout;
}
```

### 2.3 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` | +1 / -1 | §3.1.1 β-2 行 complete 化 + 512 B/frame 実装結果反映 |
| `indra/llrender/llvkloader.cpp` | +239 / -0 | anon ns state (6 件) + create 関数 (3 件) + initVulkan/shutdownVulkan 配線 + public getter body |

合計: **2 file、+240 / -1 line**

---

## 3. 設計判断履歴 (本 sub-step 着手時 AYA 確認 3 件)

### 3.1 細分化粒度: 案 A (β-2 = 1 commit)

handoff §5.3 提示の β-2-1/β-2-2/β-2-3/β-2-4 4 step を **1 commit に纏める案 A** で AYA 採用。各 step 単独 build できるが、step 単独では呼出元ゼロで dead code、合体して初めて zero write smoke 成立する性質。粒度過大化防止は β-1/β-2 分割で既達。

### 3.2 buffer 配置: 案 P (単一 VkBuffer × 3 frame)

binding 0 (PerFrame 192 B) + binding 1 (Texture 256 B) を **単一 VkBuffer に offset 配置** で AYA 採用。alignment は `minUniformBufferOffsetAlignment` (NVIDIA = 64 B、AMD 最大 256 B) 安全側 256 B padding 採用 → buffer size = 256 (PerFrame area) + 256 (Texture) = 512 B/frame。descriptor 2 binding が同一 buffer の別 range を参照、allocation 3 回で済む。

### 3.3 VMA 不採用

1.4 KB allocation に VMA library 導入 (charter §6 領域 7 coordinate 必要) は不当、`vkAllocateMemory` 直叩き + HOST_VISIBLE_COHERENT で実装。β-2 単独完結で領域 7 進捗に依存しない。

---

## 4. risks / caveats

### 4.1 PSO 側 VkPipelineLayout は未統合 (3.3-γ scope)

3.1b placeholder PSO + 3.2 sky smoke PSO の `VkPipelineLayout` は **空 layout** (descriptor set / push constant 共に無し) のまま。β-2 で descriptor set=0 layout/buffer/set を物理 allocation したが、両 PSO は引続き空 layout のため `vkCmdBindDescriptorSets` 投入経路は未成立。

**判断**: 3.3-γ で 2 PSO の `VkPipelineLayout` を二段構え準拠化 (set=0 binding + push constant range) して初めて descriptor set bind 経路成立。β-2 は あくまで UBO buffer + descriptor pool/set の物理 allocation のみ。

### 4.2 zero write smoke の transit nature

起動時 `memset(sPerFrameUboMapped[frame], 0, 512)` で初期化のみ実施。GPU shader 読出は γ で PSO layout 統合 + δ で `LLRender::syncMatrices()` Vulkan path 並走時に初実施。本 sub-step ではあくまで buffer + descriptor 物理確保が成立して `vkCreate*` / `vkAllocate*` / `vkBindBufferMemory` / `vkMapMemory` 全て成功 + Vulkan WARN/ERR 0 件 という transit acceptance のみ。

### 4.3 validation strict 確認は sub-step 3.5 に持越し

本 sub-step は **release build (validation = disabled)** で `failed` log 0 件 + AYA launch PASS という transit acceptance。validation strict 確認は sub-doc 03 §3.5 (段階 3 self-check) で別 build により実施。

### 4.4 256 B alignment の安全側選択

vendor 実機の `minUniformBufferOffsetAlignment` は実 RTX 5090 で 64 B (Vulkan 1.3 仕様の 256 B 最大保証より緩い) だが、3 OS 統一実装で AMD/Intel/macOS MoltenVK の 256 B 上限に揃える方針。実 alignment query (runtime device limit) 経由の動的 padding 化は将来 perf optim 必要時に検討、現状 1.5 KB allocation は memory pressure 対象外。

---

## 5. next session entry point

### 5.1 次 session 着手前の準備

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得)
2. 本 handoff doc 通読
3. sub-doc 03 §3.1.1 sub-step 3.3-γ marker 再確認
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 5.2 sub-step 3.3-γ 着手内容 (sub-doc 03 §3.1.1)

| 項目 | 内容 |
|---|---|
| **scope** | placeholder + sky smoke 2 PSO の `VkPipelineLayout` を二段構え準拠化 (set=0 binding + push constant range 反映)、3.1b/3.2 PSO 引継ぎ動作維持 |
| **対象 file** | `indra/llrender/llvkloader.cpp` (createPlaceholderPipeline + createSkySmokePipeline の VkPipelineLayout 設定箇所 refine) |
| **完了 marker** | 2 PSO bind 成功 + Vulkan WARN/ERR 0 件 + 3.1b/3.2 動作維持 (Placeholder PSO compiled + Sky smoke PSO compiled log 引続き出力) |

### 5.3 γ 着手 task 候補 (推定 sub-step 内 step)

| step | 内容 | 推定規模 |
|---|---|---|
| γ-1 | placeholder PSO の `createStandardPipelineLayout()` 呼出引数を `getPerFrameDescriptorSetLayout()` + push constant range (mat4 / 64 B / VERTEX_BIT) で更新 | 0.5 日 |
| γ-2 | sky smoke PSO 同様の layout 統合 + 既存 SPIR-V との binding 整合 (現 sky smoke vert/frag SPIR-V は uniform 参照無しのため compile 互換性問題は出ない見込み) | 0.5 日 |
| γ-3 | build + AYA launch verify + log 確認 (2 PSO 引続き compile 成功 + WARN/ERR 0 件) | verify |

着手前に γ 細分化要否を AYA に確認 (本 step リスト自体が細分化案、γ は scope が β-2 より軽量 = 一括 commit 案 A 推奨見込み)。

### 5.4 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)
- **段階 1 + 段階 2 + 3.1b + 3.2 + 3.3-β-2 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **γ で PSO layout を二段構え準拠化しても、実 descriptor set bind 命令は δ (syncMatrices Vulkan path) で初投入**。γ は layout signature 統合のみ
- **placeholder/sky smoke の既存 SPIR-V vertex shader が空入力 (`gl_Position = vec4(0)` / fullscreen triangle gl_VertexIndex 駆動) なので、push constant `modelview_matrix` 未使用でも compile/bind は通る** — uniform 未参照のため layout の push constant range 宣言だけで OK
- **validation strict は sub-step 3.5 で別 build により実施** (本 sub-step 3.3-γ で validation = disabled の release build で AYA launch + WARN/ERR 0 件 という transit acceptance)
- **`identity_matrix` 残置可否は 3.3-B trace で再判断** (handoff `handoff-substep-3-3-beta-1-complete.md` §4.3 引継ぎ)

---

## 6. 関連 doc / memory cross-ref

### 6.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — 段階 3 sub-doc (本 handoff で sub-step 3.3-γ marker active 化)
- `docs/specs/ayastorm-r40-vulkan-migration/05-vulkan-api-design.md` — Vulkan API 設計 (§3.5 二段構え、β-1 で sealed)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-3-beta-1-complete.md` — 直前完了 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-3-2-complete.md` — 3.2 sky pool 完了 handoff (役割完了済)

### 6.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 3.3-γ 着手 ready 状態に update)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§1.1 acceptance × log evidence 各 line 突合)
- `feedback_one_step_at_a_time.md` — β-2 着手前 AYA 3 件確認 (細分化 / buffer 配置 / VMA) 適用
- `feedback_no_auto_commit.md` — 本 handoff 完成後 commit は AYA 明示指示で実施
- `feedback_proactive_handoff.md` — sub-step 境界での能動 handoff 起草 (本 file)
- `feedback_log_reading.md` — AYA launch 後 Claude が `~/.ayastorm_x64/logs/AYAstorm.log` を grep して acceptance 3 marker + Vulkan WARN/ERR 0 件確認
