# AYAstorm r41 handoff-stage-4-prelude — 段階 4 着手前 周回境界 (sub-doc 04 + sub-doc 08 PASS 後 / pre-emptive handoff)

**status**: **active 2026-05-31 (pre-emptive handoff、context 圧迫予防 = `feedback_proactive_handoff.md` absolute rule 遵守)**
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (closed 2026-05-28)
**前 handoff**: `handoff-stage-3-complete.md` (段階 3 完遂 → 段階 4 着手境界、2026-05-31)
**関連 sub-doc**: `04-frame-context.md` (draft 2026-05-31 / AYA review PASS、段階 4 領域 4) + `08-llvkrenderer-skeleton.md` (draft 2026-05-31 / AYA review PASS、段階 4 領域 8)
**達成条件**: 次 session で sub-step 4.1 着手 (fresh context、`LLPipelineFrameContext` struct 配置 + sCull/mRT migration、low-medium risk) GO 条件確立

---

## §1 周回境界 state

### §1.1 本周回 完遂事項

| # | 完遂事項 | 完遂日 | 関連 doc / commit |
|---|---|---|---|
| 1 | **sub-doc 04-frame-context.md** Pattern α 一括 draft 起草完了 + AYA review PASS | 2026-05-31 | `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` (463 行 / 47 KB、未 commit) |
| 2 | **sub-doc 08-llvkrenderer-skeleton.md** Pattern α 一括 draft 起草完了 + AYA review PASS | 2026-05-31 | `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` (382 行、未 commit) |
| 3 | **領域 6 sub-step 6.1 着手 timing AYA 承認** = 段階 4 sub-step 4.1〜4.5 完遂後着手 (並走しない) | 2026-05-31 | 本 handoff §3 |
| 4 | **pre-emptive handoff 起草着手 AYA 承認** = context 圧迫予防、本 handoff doc | 2026-05-31 | 本 handoff doc (未 commit) |

### §1.2 sub-doc 04 PASS 時 refine 1 件 (AYA 認知済)

- handoff-stage-3-complete.md §5.3 で「850+ caller」推定 → sub-doc 04 §4.3 Agent B 実測で **〜250-280 件** に refine
- dead-store cost 圧縮側に振れた事実 reporting、12 件 RAII の class 数は不変

### §1.3 sub-doc 08 PASS 時 divergence note 1 件 (AYA 承認待ち、本 handoff で公式化)

- **charter §7.4 sub-doc 構成 命名 divergence**:
  - charter §7.4 では領域 8 対応 sub-doc = `05-skeleton-interface.md`
  - 現実の sub-doc 命名は「番号 = 領域番号」運用 (`03-state-machine-pso.md` = 領域 3、`04-frame-context.md` = 領域 4、本 sub-doc `08-llvkrenderer-skeleton.md` = 領域 8)
  - 既存 03 / 04 も charter §7.4 outline (`03-shader-port.md` / `04-descriptor-render-pass.md`) と divergent、実装中 refine の結果
- **公式化判断**: charter §7.5 「r41 着手中に refine 可な本 charter content」範囲内 refine として AYA 承認 (本 handoff §1.3 で記録、後続 sub-doc 命名も「番号 = 領域番号」運用継承)

### §1.4 段階 4 着手境界 state (sub-doc 04 + sub-doc 08 PASS 後)

- **sub-doc 04 / 08 両 PASS 完遂**: 段階 4 sub-step 4.1 着手 GO 条件 sub-doc 整備側充足
- **未 commit 3 件残存**: sub-doc 04 / sub-doc 08 / 本 handoff (本 session 内 commit 戦略は §5 で AYA 確認、案 B 3 commit 分割 vs 1 commit)
- **メモリ更新未完了**: `project_ayastorm_r41_vulkan_migration.md` 反映は本 handoff commit と同時実施想定

---

## §2 次 session 着手 cadence (sub-step 4.1 〜 4.5 全体 preview)

### §2.1 sub-step 4.1 着手 (PSO 基盤相当、low-medium risk)

**scope** (sub-doc 04 §5 / §3 反映):

- `LLPipelineFrameContext` struct 配置: `indra/newview/pipeline.h` に struct declaration 物理配置 + `pipeline.cpp` に lifecycle 実装 (`beginFrameContext` / `endFrameContext` / `beginPass` / `endPass`)
- `sCull` / `mRT` migration (low-medium risk から先行): `static LLCullResult* sCull` → `LLPipelineFrameContext::pCullResult` 経由 access、`mRT` 系 (render target stack) を struct member に集約
- caller source-level compat: 既存 `sCull` 直接参照 caller 72 件は `LLPipelineFrameContext::getCullResult()` accessor 経由 1:1 替換、API surface 不変

**着手前確認 (next session 冒頭)**:

1. branch state: `feature/ayastorm-r41-gl-removal` HEAD = `4a9011d196` (段階 3 完遂 commit) + sub-doc 04 + sub-doc 08 + 本 handoff の commit 状態確認
2. sub-doc 04 §2 frame state inventory + §3 struct outline 再読 (Agent A sealed 内容)
3. sub-doc 08 §2 namespace LLVKLoader inventory 再読 (Agent B sealed、段階 4 sub-step 4.4 part B で参照)

**完了 marker (sub-step 4.1)**:

- `LLPipelineFrameContext` struct 物理配置確認 (pipeline.h grep hit + pipeline.cpp lifecycle 関数 4 件配置)
- sCull / mRT 経路 frame context 経由化、autobuild build clean (compile warning / error 0 件)
- AYA launch verify (段階 3 範式継承、12 #VkRecord# pool hook 12/12 fire + validation 0 件 + shutdown clean、unique #Vulkan# INFO marker baseline +N 想定)
- regression 0 件 (段階 3 全 sub-step 動作維持)

### §2.2 sub-step 4.2-4.5 preview (sub-doc 04 §5 範式継承)

| sub-step | scope | risk | 完了 marker (概観) |
|---|---|---|---|
| **4.2 (軽量)** | 10 bool flag (sShadowRender + 周辺 frame state inventory §2.2 該当 bool 系) を struct migration | medium | bool flag 全 caller source-level compat 維持、autobuild clean + AYA verify PASS |
| **4.3 (標準)** | sCurCameraID accessor pattern 経由 frame context binding + **per-pool 実 scene draw 移植** (12 pool 全 placeholder NDC 三角形 → 実 scene visibility iteration + PSO bind + descriptor bind + vertex/index bind + vkCmdDrawIndexed、handoff §1.3 #3-段階 3 引継ぎ) | **high** | LLViewerCamera::getCurCameraID accessor 動作 + 12 pool 全 hook で実 scene 描画 + AYA launch 時 placeholder 三角形消失 + 黒画面ベース scene 描画 (vk-α 空転 baseline) |
| **4.4 (特殊)** | **part A**: 12 件 LLGLState RAII state class setter 内 GL call 物理削除 (dead-store 化、sub-doc 04 §4 単独範囲、〜250-280 caller source-level compat 維持) + **part B**: `class LLVKRenderer` skeleton declaration 物理配置 (sub-doc 08 §3、新規 `indra/llrender/llvkrenderer.{h,cpp}` + CMake 配線、21 件 thin wrapper、案 B 2 commit 分割想定) | **high** | `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/llrender/llgl.cpp indra/llrender/llglstates.h` wrapper 内部以外 0 件 hit + `grep -rE "class LLVKRenderer" indra/` 1 件以上 hit + autobuild clean + AYA verify PASS |
| **4.5 (self-check + handoff)** | 段階 4 完遂条件 satisfy 自己 trace (acceptance #1 / #3 / #5 / #6 段階 4 内 satisfy 確認) + `handoff-stage-4-complete.md` 起草 + sub-doc 04 / 08 closed 化 | medium | acceptance 5 件 satisfy 確認 + handoff doc 完成 + sub-doc 04 / 08 status closed 化 |

### §2.3 commit 戦略 (次 session 内、案 B 5 commit 分割範式継承想定)

- sub-step 4.1: 1 commit (struct 配置 + sCull/mRT migration + AYA verify PASS 反映)
- sub-step 4.2: 1 commit (bool flag migration + AYA verify PASS 反映)
- sub-step 4.3: 案 B 2 commit 分割想定 (per-pool 実 scene draw 移植は 12 pool 全 hook 配線 + AYA verify、移植 cadence で更に分割可能性)
- sub-step 4.4: 案 B 2 commit 分割 (part A 12 件 RAII dead-store + part B LLVKRenderer skeleton)
- sub-step 4.5: 案 B 2 commit 分割範式 (verify + handoff doc)

---

## §3 deferred 残 (本 周回後の次次 cadence)

### §3.1 領域 6 sub-step 6.1 (autobuild Vulkan integration) 本格着手

- **AYA 承認 timing** (2026-05-31): 段階 4 sub-step 4.1〜4.5 完遂後着手 (並走しない、`feedback_experiment_branch_single_scope.md` / `feedback_one_step_at_a_time.md` 精神)
- **scope**: sub-step 3.3-B で確立した build chain pattern (system glslangValidator + `aya_compile_shader_spirv` cmake function + viewer_manifest.py recursive copy + `loadSpirvShaderModuleFromFile` helper + macro guard fallback) を 248 shader file 全 port に一般化
- **着手 trigger**: 段階 4 sub-step 4.5 (handoff-stage-4-complete.md 起草) 完遂後の handoff 段階で AYA 擦り合わせ

### §3.2 領域 7 sub-step 7.3 / 7.4 / 7.5 (descriptor + render pass 残)

- 段階 4-5 並走、本周回では着手判断未実施 (handoff-stage-3-complete §5.1 cadence 通り、段階 4 完遂後 handoff で擦り合わせ想定)

### §3.3 acceptance #1 PFNGL declarations 物理削除

- 段階 5 一体運用 (charter §1.5.2 Cluster D 189 file 波及、handoff-stage-3-complete §1.3 #1-段階 3)

---

## §4 関連 doc / memory cross reference

### §4.1 関連 sub-doc

| sub-doc | 関係 |
|---|---|
| `00-charter.md` (closed 2026-05-28) | 親 charter、§2 領域 4 + 領域 8 + §3 acceptance #1 / #3 / #5 / #6 + §7.4 sub-doc 構成 + §7.5 refine boundary |
| `03-state-machine-pso.md` (closed 2026-05-31) | 段階 3 sub-doc、参照 archive (§1.5 trace inventory + §3 sub-step + §4.1 acceptance) |
| `04-frame-context.md` (draft 2026-05-31 / PASS) | 段階 4 領域 4 sub-doc、本周回起草完了、sub-step 4.1〜4.5 着手 source |
| `08-llvkrenderer-skeleton.md` (draft 2026-05-31 / PASS) | 段階 4 領域 8 sub-doc、本周回並走起草完了、sub-step 4.4 part B 実装 source |

### §4.2 関連 handoff

| handoff | 関係 |
|---|---|
| `handoff-stage-3-complete.md` (active 2026-05-31) | 段階 3 完遂 + 段階 4 着手境界 (本 handoff の前 handoff、§5.1 cadence で本周回作業指示) |
| **本 handoff** `handoff-stage-4-prelude.md` (active 2026-05-31) | 段階 4 着手前 pre-emptive handoff、次 session sub-step 4.1 着手 GO 条件確立 |

### §4.3 関連 memory

| memory | 関係 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | r41 milestone active、本 handoff で sub-doc 04 / 08 PASS + 領域 6 sub-step 6.1 timing + 命名 divergence 公式化 反映予定 |
| `feedback_proactive_handoff.md` | 本 handoff 起草の absolute rule 根拠 |
| `feedback_one_step_at_a_time.md` | 領域 6 sub-step 6.1 並走 deferred 判断の根拠 |
| `feedback_render_full_trace_first.md` | sub-doc 04 §2 + sub-doc 08 §2 inventory の Agent 並列 trace sealed 根拠 |
| `feedback_no_claude_coauthor.md` | 全 commit message から Claude 共著行排除遵守 |

---

## §5 次 session GO 条件

### §5.1 次 session 着手前確認 (cadence)

1. **本 handoff Read**: 本 handoff doc 全文を Read で確認 (周回境界 state + sub-step 4.1〜4.5 cadence + deferred 残 reconfirm)
2. **branch state 確認**: `feature/ayastorm-r41-gl-removal` HEAD 状態 + sub-doc 04 / 08 / 本 handoff の commit 状態確認 (本周回末 commit 完遂後想定)
3. **sub-doc 04 + sub-doc 08 再読**: §2 inventory + §3 struct/skeleton outline + §5 sub-step 範式継承
4. **sub-step 4.1 着手**: pipeline.h struct declaration + pipeline.cpp lifecycle 実装 + sCull/mRT migration + autobuild build verify + AYA launch verify

### §5.2 deferred 擦り合わせ 残 (本 handoff 完遂後 / 次 session 着手前 1 件)

- **commit 戦略** (本 session 内): sub-doc 04 / sub-doc 08 / 本 handoff の commit を 1 commit / 案 B 3 commit 分割 のいずれで進めるか AYA 確認 (本 handoff §1.4 反映)

### §5.3 次次 session 移行条件 (段階 4 完遂後)

- 段階 4 全 sub-step (4.1〜4.5) 完遂 + `handoff-stage-4-complete.md` 起草 + sub-doc 04 / 08 closed 化 → 段階 5 着手境界 (領域 5 依存解決 + PFNGL declarations 物理削除 + 領域 6 sub-step 6.1 本格着手 並走判断)

---

## 起草 cadence 完了宣言

- 本 handoff Pattern α 一括 draft 起草完了 (2026-05-31、pre-emptive handoff、context 圧迫予防)
- 次 session で本 handoff §5.1 cadence 通り着手、sub-step 4.1 (`LLPipelineFrameContext` struct 配置 + sCull/mRT migration、low-medium risk) を fresh context で実施
- AYA 認知済の commit 戦略 1 件擦り合わせ後に本周回末 commit (案 B 3 commit 分割 vs 1 commit) で session close 想定

**commit message draft** (本 handoff 起草分、3 commit 分割案 採用時):

```
docs(r41): handoff-stage-4-prelude.md 起草 (sub-doc 04 + sub-doc 08 AYA review PASS 後 pre-emptive handoff、context 圧迫予防 = feedback_proactive_handoff.md absolute rule 遵守、§1 周回境界 state [sub-doc 04 / 08 PASS + 領域 6 sub-step 6.1 段階 4 完遂後着手 AYA 承認 + charter §7.4 命名 divergence 公式化] + §2 次 session cadence [sub-step 4.1 LLPipelineFrameContext struct 配置 + sCull/mRT migration low-medium risk → 4.2 軽量 → 4.3 標準 per-pool 実 scene draw 移植併合 → 4.4 part A 12 件 RAII dead-store + part B class LLVKRenderer skeleton → 4.5 self-check + handoff] + §3 deferred 残 [領域 6 sub-step 6.1 / 領域 7 sub-step 7.3-7.5 / acceptance #1 PFNGL 段階 5 一体運用] + §4 関連 doc/memory + §5 次 session GO 条件、feedback_proactive_handoff / feedback_one_step_at_a_time / feedback_render_full_trace_first / feedback_no_claude_coauthor 遵守) → 次 session sub-step 4.1 着手境界
```
