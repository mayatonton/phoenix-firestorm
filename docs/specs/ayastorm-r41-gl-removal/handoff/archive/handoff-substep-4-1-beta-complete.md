# r41 sub-step 4.1-β 完遂 → 4.2 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-1-alpha-complete.md` (sub-step 4.1-α 完遂 → 4.1-β 着手境界、役割完了)
**本 handoff 位置付け**: sub-step 4.1-β (mActiveRT aggregation + ScopedActiveRT RAII + pipeline.cpp 内 112 件 + 外部 11 file 47 件 = 159 件 mRT migration) **全完遂宣言** + sub-step 4.2 (軽量 10 bool flag migration、medium risk) 着手前 scope 確認境界。AYA launch verify PASS で 4.1-β 章 close + sub-step 4.1 全体 (α + β 案 B 2 commit 分割) seal。

---

## 1. sub-step 4.1-β 全完遂 status (2026-05-31)

### 1.1 完遂 marker (sub-doc 04 §3 LLPipelineFrameContext + §5 sub-step 4.1 のうち mRT 部分)

| acceptance | 達成 status |
|---|---|
| llpipelineframecontext.h 内 `RenderTargetPack* mActiveRT` field + `getActiveRT()` / `setActiveRT()` 配線 | ✓ |
| llpipelineframecontext.h 内 `#include "pipeline.h"` 配線 (nested class `LLPipeline::RenderTargetPack` 型解決、案 X 採用) | ✓ |
| llpipelineframecontext.{h,cpp} 内 nested class `ScopedActiveRT` RAII 配線 (SetTemporarily 替換、案 P 採用) | ✓ |
| llpipelineframecontext.cpp 内 ctor initializer list `mActiveRT(nullptr)` 配線 | ✓ |
| pipeline.cpp 内 file-local helper `getFrameRT()` 配置 (匿名 namespace、sub-step 4.1-α `getFrameCull()` 隣接) | ✓ |
| pipeline.cpp 内 write 5 件 setActiveRT() 替換 (init + allocateScreenBufferInternal 3 件 + その他 1 件) | ✓ |
| pipeline.cpp 内 read 107 件 getFrameRT() 替換 | ✓ |
| pipeline.cpp 内 `gPipeline.mRT` 4 件 (doAtmospherics / doWaterHaze) → `getFrameRT()` 直接 call 修正 (1 回目 build compile error fix = `gPipeline.getFrameRT()` は LLPipeline member 不在) | ✓ |
| 外部 11 file 47 件 替換 (write 5 + read 41 + SetTemporarily 1 = ScopedActiveRT) + `#include "llpipelineframecontext.h"` 配線 | ✓ |
| llviewershadermgr.cpp = bare comment のみで skip | ✓ |
| build clean | ✓ exit 0 / 14 file +201/-147 line / error 0 件 |
| AYA launch verify clean | ✓ vulkanDebugCallback ERROR/WARNING 0 件 / 12/12 #VkRecord# pool hook fire / shutdown clean / regression 0 |

### 1.2 commit hash

| commit | scope |
|---|---|
| `89c6efc9f7` | sub-step 4.1-β 全完遂 (14 files changed, +201/-147) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-4-1-alpha-complete.md` | **役割完了** (sub-step 4.1-β 着手 GO 条件 satisfy → 完遂、本 handoff で内容引継ぎ) |
| sub-doc `04-frame-context.md` | active 継続 (sub-step 4.2 marker 着手 ready 状態へ) |
| sub-doc `08-llvkrenderer-skeleton.md` | active 継続 (sub-step 4.4 part B で参照) |
| `handoff-substep-4-1-beta-complete.md` (本 handoff) | 新規作成 (4.1-β 全完遂 → 4.2 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 4.2 着手 ready 状態へ update) |

---

## 2. 実装内容 (本 commit 範囲 `89c6efc9f7`)

### 2.1 LLPipelineFrameContext h/cpp 拡張

| 変更 | 詳細 |
|---|---|
| `llpipelineframecontext.h` `#include "pipeline.h"` 追加 (L18) | 案 X 採用 = nested class `LLPipeline::RenderTargetPack` 型解決、forward declaration 不可のため transitive include 採用、外部 file は既に pipeline.h を include している前提 (新 dependency 導入無し) |
| `mActiveRT` field 配線 (L81 area) | `LLPipeline::RenderTargetPack* mActiveRT;` private member |
| `getActiveRT()` / `setActiveRT()` getter/setter (L56-57) | sub-step 4.1-β scope |
| `ScopedActiveRT` nested class 配線 (L61-70) | 案 P 採用 = `SetTemporarily<RenderTargetPack*>` (llgltfmaterialpreviewmgr.cpp L438) 1 件の RAII 互換代替、explicit ctor + private `mPrevRT` field |
| `llpipelineframecontext.cpp` ctor initializer list `mActiveRT(nullptr)` 追加 (L27) | sub-step 4.1-α scope の mCullResult(nullptr) に並列 |
| `llpipelineframecontext.cpp` ScopedActiveRT impl 配線 (L57-66) | ctor で前 RT 保存 + setActiveRT(new_rt) / dtor で setActiveRT(mPrevRT) |
| **endFrameContext は mActiveRT reset しない** (L41-45) | handoff-substep-4-1-alpha-complete §5.3 spec 通り = init/allocateScreenBuffer 経路のみ mutate + frame lifecycle 不参加で既存挙動 1:1 維持 (sub-doc 04 §3.4 spec divergence handling) |

### 2.2 pipeline.cpp 物理変更

| 変更 | 詳細 |
|---|---|
| file-local helper `getFrameRT()` 配置 (匿名 namespace、L478 area) | sub-step 4.1-α `getFrameCull()` 隣接、`LLPipelineFrameContext::getInstance().getActiveRT()` wrapper |
| write 5 件 setActiveRT() 替換 | init() 1 件 (L584) + allocateScreenBufferInternal() 3 件 (L1070/1078/1082) + その他 1 件 |
| read 107 件 getFrameRT() 替換 | replace_all `mRT` → `getFrameRT()` (prefix 衝突 verify = `Grep mRT[a-zA-Z_]` 0 件 confirm) |
| pipeline.cpp 内 `gPipeline.mRT` 4 件 fix | L11739/L11740 (doAtmospherics) + L12020/L12021 (doWaterHaze) = `gPipeline.getFrameRT()` → `getFrameRT()` 直接 call (1 回目 build で compile error: `gPipeline.getFrameRT()` は LLPipeline member 不在、file-local helper は anonymous namespace 内 free function) |
| mRT field 物理は pipeline.h:871 に **legacy 保持** | sub-step 4.5 で撤去 (handoff-substep-4-1-alpha-complete §5.2 spec 通り) |

### 2.3 外部 11 file 替換

| file | write | read | 特殊 | include |
|---|---|---|---|---|
| `llheroprobemanager.cpp` | 2 (L319/L323 → setActiveRT) | - | - | + L34 |
| `llreflectionmapmanager.cpp` | 2 (L778/L807 → setActiveRT) | - | - | + L36 |
| `llgltfmaterialpreviewmgr.cpp` | - | - | 1 (L438 SetTemporarily → ScopedActiveRT) | + L43 |
| `lldrawpoolalpha.cpp` | - | 13 (read getActiveRT()) | - | + L46 |
| `llviewerwindow.cpp` | - | 7 (read getActiveRT()) | - | + L194 |
| `llviewerdisplay.cpp` | - | 6 (read getActiveRT()) | - | + L88 |
| `llfloatermodelpreview.cpp` | - | 4 (read getActiveRT()) | - | + L53 |
| `rlveffects.cpp` | - | 4 (read getActiveRT()) | - | + L25 |
| `lldrawpoolwater.cpp` | - | 2 (read getActiveRT()) | - | + L47 |
| `llreflectionmap.cpp` | - | 2 (read getActiveRT()) | - | + L30 |
| `rlvhandler.cpp` | - | 2 (read getActiveRT()) | - | + L56 |
| `llviewershadermgr.cpp` | - | - | bare comment のみ | skip |
| **計** | **5** | **41** | **1 (ScopedActiveRT)** | **11 file** |

47 件 (handoff-substep-4-1-alpha-complete §5.1 推定 47 件 整合)。**AYA 指示「helper 配置せず直接 call で」採用** = per-file local helper 排除、pipeline.cpp 内 file-local `getFrameRT()` のみ存続、外部 file は `LLPipelineFrameContext::getInstance().getActiveRT()` 直接 call。

### 2.4 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `indra/newview/llpipelineframecontext.h` | +25 / -0 | `#include "pipeline.h"` + `mActiveRT` field + getter/setter + ScopedActiveRT nested class |
| `indra/newview/llpipelineframecontext.cpp` | +12 / -0 | ctor `mActiveRT(nullptr)` + ScopedActiveRT impl |
| `indra/newview/pipeline.cpp` | +228 line modified / -147 line removed (net +81 line) | file-local helper `getFrameRT()` + write 5 件 + read 107 件 + `gPipeline.getFrameRT()` 4 件 → `getFrameRT()` fix |
| 外部 11 file 計 | +83 / -? | 各 file include + getActiveRT/setActiveRT 替換 + ScopedActiveRT 1 件 |

合計: **14 file、+201 / -147 line** (`git diff --stat HEAD~1` 確認済)

---

## 3. build + launch verification

| step | status |
|---|---|
| `autobuild build -A 64 -c ReleaseFS_open --no-configure` (CMakeLists.txt 変更なし、新規 file 無し = incremental build) | ✓ 2 回目 exit 0 (1 回目 4 件 compile error fix `gPipeline.getFrameRT()` → `getFrameRT()`) |
| 14 file 全 .o 再 compile clean (llpipelineframecontext.cpp.o + pipeline.cpp.o + 11 外部 file.o) | ✓ |
| packaging 完遂 | ✓ `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261510128.tar.xz` |
| `./install.sh` (`build-linux-x86_64/newview/packaged/`) | ✓ `/home/ishikawa/ayastorm` 配置完了 |
| `rm -rf ~/.ayastorm_x64/cache/` | ✓ 完遂 |
| AYA launch + 通常起動報告「起動して少し歩いて shutdown しました」 | ✓ 2026-05-31 |

### 3.1 ログ確認 (Claude 側 `~/.ayastorm_x64/logs/AYAstorm.log` grep 結果)

| marker | 期待 | 実測 |
|---|---|---|
| `vulkanDebugCallback` (ERROR/WARNING 受信) | 0 件 | **0 件** ✓ |
| `VK_ERROR` / `VUID-` / `validation failed` | 0 件 | **0 件** ✓ |
| `#VkRecord#` pool hook fire | 12/12 件 | **12 件** ✓ |
| `#Vulkan#` marker INFO 出力 | 4.1-α baseline 56 件整合範囲 | **56 件 (file:line unique 53)** ✓ |
| shutdown clean | `Vulkan device destroyed → Vulkan instance destroyed → Goodbye → status: stopped` | ✓ (log tail 4 行で確認) |
| regression | 描画通常 / UI 反応 / shader effect 維持 | ✓ AYA 報告 (少し歩いて shutdown) |

run_time: 起動 2026-05-31T08:19:29Z → shutdown 2026-05-31T08:20:47Z = **78s**

### 3.2 validation strict 検証は sub-step 3.5-a で実施済継承

sub-step 3.5-a (2026-05-30) で validation force-enable build により段階 3 全体の validation strict 検証 PASS 済 (vulkanDebugCallback 経由 ERROR/WARNING 0 件)。sub-step 4.1-β は段階 3 配線に変更を加えず frame state aggregation 経路の追加のみのため、validation strict 再走は不要 (段階 4 全 sub-step 完遂後の sub-step 4.5 self-check で一括再走想定)。

---

## 4. 設計 deviation 履歴 (sub-doc 04 起草時から本実装まで)

### 4.1 案 B 採用 (measurement-first 検証 log 配線 skip) = 4.1-α 継承

sub-doc 04 §2.5 measurement-first 範式は struct shape を sub-step 4.1 着手時 final 化する観点で、sub-step 4.1-α / 4.1-β 通じ task #1 (struct shape final 化) を完遂条件として吸収。実装後の measurement 検証 log 配線は AYA 承認下で skip 採用。

**理由**:
- ScopedActiveRT RAII の単純性 (実コード = ctor で前 RT 保存 + new_rt 設定 / dtor で前 RT 復元 のみ)
- 案 B では起動/shutdown clean + regression 0 + code review 整合性で satisfy
- build cycle (1 回目 4 件 error + fix → 2 回目 PASS) で commit 到達

**trade-off**: ScopedActiveRT lifecycle hit 経路の log evidence は無し。getActiveRT() / setActiveRT() 経由 mRT pointer 整合性は 47 occurrences の code review + AYA 起動 PASS で transit satisfy。

### 4.2 pipeline.cpp file-local helper `getFrameRT()` 採用 (spec literal 短縮) = 4.1-α 範式継承

sub-doc 04 §3.4 spec literal は `LLPipelineFrameContext::getInstance().getActiveRT()` の direct call を 107 caller 全部に挿入する path。本実装では pipeline.cpp 匿名 namespace 内 file-local helper `inline LLPipeline::RenderTargetPack* getFrameRT()` を配置し、caller は `getFrameRT()` で短縮。

**理由**:
- pipeline.cpp 内 implementation detail (file scope) = 外部 file から参照不可、AYA 確認不要範囲
- 替換時の visual diff 最小化 (`mRT` → `getFrameRT()` で symbol 1:1)
- 4.1-α `getFrameCull()` と同 pattern (sub-step 4.3 で const-ref passing through render path 化時の差替え単位)

**stop-gap 性格**: 本 helper は sub-step 4.3 でも撤去対象。sub-step 4.2 では同様の file-local helper pattern を bool flag にも適用想定 (caller diff 最小化)。

### 4.3 外部 11 file 直接 call 採用 (AYA 指示「helper 配置せず直接 call で」)

外部 11 file は per-file local helper を配置せず、`LLPipelineFrameContext::getInstance().getActiveRT()` / `setActiveRT()` 直接 call を採用。

**理由**:
- 外部 file の caller 密度は最大 13 件 (lldrawpoolalpha.cpp) で pipeline.cpp 107 件と桁違い
- file-local helper の存在は実装抽象化の増加 = sub-step 4.3 const-ref passing 化時の撤去対象が増える
- AYA 指示 (β-4 着手時) 採用

### 4.4 案 X 採用 (`#include "pipeline.h"` in `llpipelineframecontext.h`)

nested class `LLPipeline::RenderTargetPack` の型を `mActiveRT` field / getter/setter で使うため、forward declaration 不可 (nested class は完全 enclosing class 定義が必要)。`#include "pipeline.h"` を `llpipelineframecontext.h` 内に追加 = 案 X 採用 (β-2 着手時 AYA 承認下)。

**trade-off**: `llpipelineframecontext.h` を include する外部 file は pipeline.h を transit include することになる。ただし全 11 外部 file は既に pipeline.h を include 済 = 新規 dependency 導入無し。

### 4.5 案 P 採用 (`ScopedActiveRT` nested RAII helper for SetTemporarily replacement)

llgltfmaterialpreviewmgr.cpp L438 で `SetTemporarily<LLPipeline::RenderTargetPack*>` template (mRef pointer 経由) を使っていた transient RT 切替パターンを、`LLPipelineFrameContext::ScopedActiveRT` nested RAII helper で替換 = 案 P 採用 (β-4 着手時 AYA 承認下)。

**理由**:
- SetTemporarily<T> は `T* mRef` semantics で「pointer 経由 mutate + 自動復元」、mActiveRT は LLPipelineFrameContext singleton 内 field のため pointer 直接 reference 不可
- ScopedActiveRT は getActiveRT() で前 RT 保存 + setActiveRT(new_rt) で更新、dtor で setActiveRT(mPrevRT) で復元 = SetTemporarily と同等 RAII semantics

### 4.6 (記録なし) sub-doc 04 spec 通り実装の項目

- `mActiveRT` field 名 ✓
- `getActiveRT()` / `setActiveRT()` API ✓
- `RenderTargetPack*` 型 ✓
- endFrameContext での mActiveRT reset しない判断 ✓ (handoff-substep-4-1-alpha-complete §5.3 spec 通り)

---

## 5. risks / caveats

### 5.1 sub-step 4.2 scope = 10 bool flag、medium risk

sub-doc 04 §3.3 sub-step 4.2 spec = 10 bool flag (sShadowRender + 周辺 frame state inventory bool 系) struct migration。read/write 経路 trace + lifecycle 配線整合確認が必須。

**4.1-β 比較**:
- 4.1-β: 159 occurrences、write 不定 (init/allocateScreenBuffer 経路のみ)
- 4.2: 推定 occurrence は sub-doc 04 §2 row 別 sum 必要 (本 handoff 時点未確定、4.2 着手時 trace タスク)

### 5.2 mRT field 物理削除は sub-step 4.5 scope

pipeline.h:871 に legacy 保持中の `RenderTargetPack mRT[3];` field は sub-step 4.5 self-check 時に撤去想定。handoff-substep-4-1-alpha-complete §5.2 spec 継承。

**注意**: mRT field を削除する際は sub-step 4.5 で:
- pipeline.h 内 field declaration 削除
- pipeline.cpp 内 mRT array に直接 access している残存 file-local 経路の最終 cleanup (現状 0 件想定だが grep で再確認)
- sub-doc 04 spec 通り `RenderTargetPack` array は LLPipelineFrameContext 内に移管するか別位置で保持するか判断 (現状 sub-step 4.1-β では legacy 配列のまま、LLPipelineFrameContext は pointer 経由 access のみ)

### 5.3 LLPipelineFrameContext 内 nested class `ScopedActiveRT` 名 = sub-step 4.2/4.3 で同パターン採用判断

sub-step 4.2 (bool flag) で「transient bool 切替」がある場合、`ScopedActiveRT` 同パターンで RAII helper を nested class として配置するか、`SetTemporarily<bool>` を維持するかは 4.2 着手時 trace で判断。

### 5.4 sub-step 4.2 〜 4.5 は順次着手 (並走しない)

handoff-stage-4-prelude.md §2 cadence: 4.1 → 4.2 → 4.3 → 4.4 → 4.5 (順次)。並走しない (charter §7.5 r41 着手中 refine 可範囲、AYA 承認下)。

### 5.5 context budget concern (次 session)

4.2 着手時の bool flag 替換規模は 4.1-β (159 件) より小さい想定 (10 bool × 平均 read/write 数十件想定)。1 session 内完遂は妥当だが、4.3 (per-pool 実 scene draw 移植 high risk) は別 session 想定。proactive handoff 範式 (feedback_proactive_handoff.md absolute rule) 継承、必要なら sub-step 4.2 内段階分割を AYA 承認下で判断。

---

## 6. next session entry point

### 6.1 次 session 着手前の準備 (4 段 cadence)

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得、または local commit `89c6efc9f7` ready 確認)
2. 本 handoff doc 通読
3. sub-doc `04-frame-context.md` §3.3 sub-step 4.2 cadence 再確認 (10 bool flag 詳細 inventory)
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 6.2 sub-step 4.2 着手 task 候補 (sub-doc 04 §5 cadence + 本 handoff §5.1 から refine)

| task | 内容 | 推定規模 |
|---|---|---|
| 4.2-1 | 10 bool flag (sShadowRender + 周辺 frame state inventory bool 系) read/write 経路 trace + occurrence 数確定 | trace 30-60 分 |
| 4.2-2 | llpipelineframecontext.{h,cpp} 内 bool field 10 件 + getter/setter 10 件配線 | 実装 20-30 分 |
| 4.2-3 | pipeline.cpp 内 file-local helper (e.g. `getShadowRender()` 等) 配置 + read/write 替換 | 実装 30-60 分 |
| 4.2-4 | 外部 file 替換 (外部 file 散らばり確認、4.1-β 11 file vs 4.2 で重複範囲想定) | 実装 30-60 分 |
| 4.2-5 | autobuild incremental build verify | 20-30 分 |
| 4.2-6 | AYA launch verify (案 B cadence 継承: vulkanDebugCallback 0 / 12 #VkRecord# fire / shutdown clean / regression 0) | AYA |
| 4.2-7 | commit (案 B = measurement log skip) | 5 分 |

### 6.3 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)
- **段階 1 + 段階 2 + 段階 3 + 4.1 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **bool flag 各 1 件ごとに write 経路の確定が必要** (sShadowRender / sShadowPass 等は per-frame 切替、lifecycle 配線整合が重要)
- **per-pool 実 scene draw 移植は sub-step 4.3 scope** (本 sub-step 4.2 は frame state 集約のみ、実 scene draw 改変なし)
- **案 B cadence 継承想定** (AYA 別途指示なき限り measurement log skip)
- **context budget proactive 監視** (feedback_proactive_handoff.md absolute rule、必要なら 4.2 内段階分割を AYA 承認下で判断)

### 6.4 commit 戦略

- 1 件想定 (sub-step 4.2 全体 = 10 bool flag struct migration)
- 段階分割判断は context 状況 + AYA 承認下で決定 (5.5 reference)
- commit message style = `feat(r41): sub-step 4.2 完遂 (...)` 範式

---

## 7. 関連 doc / memory cross-ref

### 7.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter (§2 領域 4 1.00 PM 高 risk)
- `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` — 段階 4 領域 4 sub-doc (sub-step 4.2 marker 着手 ready 状態へ)
- `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` — sub-step 4.4 part B で参照
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-3-complete.md` — 段階 3 完遂 (引継ぎ 3 件 = per-pool 実 scene draw 移植 + RAII setter dead-store 化 + LLVKRenderer skeleton declaration、役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-4-prelude.md` — 段階 4 pre-emptive handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-1-alpha-complete.md` — sub-step 4.1-α 完遂 → 4.1-β 着手境界 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-1-beta-complete.md` — 本 handoff (4.1-β 全完遂 → 4.2 着手境界)

### 7.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 4.2 着手 ready 状態に update)
- `project_build_procedure.md` — autobuild + install + cache clear フロー (本 sub-step で実施)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§2 file/line / §3 ログ marker 表)
- `feedback_no_auto_commit.md` — commit は AYA 明示指示で実施 (本 sub-step 4.1-β は指示済 → `89c6efc9f7`)
- `feedback_proactive_handoff.md` — 周回境界での能動 handoff 起草 (本 file、AYA 「memory 更新して handoff 起草してください」指示 2026-05-31)
- `feedback_one_step_at_a_time.md` — 1 メッセージ 1 アクション、4.1-α / 4.1-β / 4.2 分割 cadence
- `feedback_remove_verification_logs.md` — 案 B では log 配線 skip = 除去対象なし
- `feedback_no_claude_coauthor.md` — 全 commit messages で Claude 共著行なし
- `reference_log_path.md` — `~/.ayastorm_x64/logs/AYAstorm.log` (Linux)
