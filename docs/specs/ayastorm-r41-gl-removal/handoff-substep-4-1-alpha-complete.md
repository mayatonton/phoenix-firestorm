# r41 sub-step 4.1-α 完遂 → 4.1-β 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-stage-4-prelude.md` (段階 4 pre-emptive handoff、sub-step 4.1 着手境界 = fresh context cadence)
**本 handoff 位置付け**: sub-step 4.1-α (LLPipelineFrameContext struct 配置 + sCull aggregation + lifecycle 4 関数 + pipeline.cpp sCull 経路 1:1 替換) **全完遂宣言** + sub-step 4.1-β (mRT migration、159 occurrences across pipeline.{h,cpp} + 12 external files) 着手前 scope 確認境界。AYA launch verify PASS で 4.1-α 章 close。

---

## 1. sub-step 4.1-α 全完遂 status (2026-05-31)

### 1.1 完遂 marker (sub-doc 04 §3 LLPipelineFrameContext struct + §5 sub-step 4.1)

| acceptance | 達成 status |
|---|---|
| LLPipelineFrameContext struct 物理配置 (singleton Meyer + lifecycle 4 関数 + sCull aggregation) | ✓ `indra/newview/llpipelineframecontext.{h,cpp}` 新規配置 |
| CMakeLists.txt 配線 (viewer_SOURCE_FILES + viewer_HEADER_FILES alphabet sort) | ✓ |
| pipeline.cpp frame entry/exit lifecycle 配線 (grabReferences/clearReferences) | ✓ beginFrameContext + setCullResult + endFrameContext + setCullResult(nullptr) |
| sCull 経路 1:1 替換 (file-local helper `getFrameCull()` 経由) | ✓ 45 occurrences (Agent A 推定 72 → 実測 refine = pipeline.cpp translation unit-local) |
| build clean | ✓ exit 0 / `llpipelineframecontext.cpp.o` 構築済 / error 0 件 |
| AYA launch verify clean | ✓ vulkanDebugCallback ERROR/WARNING 0 件 / 12/12 #VkRecord# pool hook fire / shutdown clean / regression 0 |

### 1.2 commit hash

| commit | scope |
|---|---|
| `58ae59bcb7` | sub-step 4.1-α 全完遂 (4 files changed, +212/-72) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-stage-4-prelude.md` | **役割完了** (sub-step 4.1 着手 GO 条件 satisfy、本 handoff で内容引継ぎ) |
| sub-doc `04-frame-context.md` | active 継続 (sub-step 4.1-β marker 着手 ready 状態へ) |
| sub-doc `08-llvkrenderer-skeleton.md` | active 継続 (sub-step 4.4 part B で参照) |
| `handoff-substep-4-1-alpha-complete.md` (本 handoff) | 新規作成 (4.1-α 全完遂 → 4.1-β 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 4.1-β 着手 ready 状態へ update) |

---

## 2. 実装内容 (本 commit 範囲 `58ae59bcb7`)

### 2.1 新規 file 物理配置

| file | size | 主内容 |
|---|---|---|
| `indra/newview/llpipelineframecontext.h` | 64 行 | LLPipelineFrameContext class (singleton Meyer + lifecycle 4 関数 [beginFrameContext / endFrameContext / beginPass / endPass] + EPassType enum [PASS_NONE / SHADOW / DEFERRED / FORWARD / POST] + sCull aggregation getter/setter + deleted copy ctor/assignment) |
| `indra/newview/llpipelineframecontext.cpp` | 54 行 | static instance Meyer's singleton (`getInstance()`) + lifecycle stub 実装 (sub-step 4.1-α scope = sCull aggregation のみ実 use、他 fields は sub-step 4.1-β / 4.2 / 4.3 で順次追加) |

License header pattern = AYAstorm short header (`indra/llrender/llvkloader.h` 参照範式)、Linden Research 長 header は採用せず。

### 2.2 CMakeLists.txt 配線

| section | 追加 entry | 配置 |
|---|---|---|
| `viewer_SOURCE_FILES` (line 683 前) | `llpipelineframecontext.cpp` | alphabet sort 範式 (llpipeline < llpipelineframecontext < llpipelinelistener、f < l) |
| `viewer_HEADER_FILES` (line 1548 前) | `llpipelineframecontext.h` | 同上 |

### 2.3 pipeline.cpp 物理変更

| 変更 | 詳細 |
|---|---|
| include 追加 (line 77 area) | `#include "llpipelineframecontext.h"` を `llpipelinelistener.h` の直前に挿入 |
| sCull static declaration 削除 (line 477 area) | 既存 `static LLCullResult* sCull = NULL;` を物理削除 |
| file-local namespace helper 追加 (line 478 area) | 匿名 namespace 内 `inline LLCullResult* getFrameCull() { return LLPipelineFrameContext::getInstance().getCullResult(); }` (pipeline.cpp 内 implementation detail = AYA 確認不要範囲、後続 sub-step 4.3 const-ref passing through render path 化前 stop-gap) |
| sCull 替換 (45 occurrences) | `getFrameCull()` に 1:1 替換、replace_all 安全性 verify (`Grep sCull[a-zA-Z]` 0 hit confirm = prefix 衝突なし) |
| grabReferences lifecycle 配線 (line 2740-2745) | `LLPipelineFrameContext::getInstance().beginFrameContext(); LLPipelineFrameContext::getInstance().setCullResult(&result);` |
| clearReferences lifecycle 配線 (line 2748-2754) | `LLPipelineFrameContext::getInstance().setCullResult(nullptr); LLPipelineFrameContext::getInstance().endFrameContext();` |

### 2.4 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `indra/newview/llpipelineframecontext.h` | +64 (新規) | LLPipelineFrameContext class declaration |
| `indra/newview/llpipelineframecontext.cpp` | +54 (新規) | LLPipelineFrameContext stub 実装 |
| `indra/newview/CMakeLists.txt` | +2 | viewer_SOURCE_FILES + viewer_HEADER_FILES 配線 |
| `indra/newview/pipeline.cpp` | +94 / -72 (45 occurrences 替換 + lifecycle + helper) | sCull 経路 file-local helper 経由 1:1 替換 + grabReferences/clearReferences lifecycle 配線 |

合計: **4 file、+212 / -72 line** (`git diff --stat HEAD~1` 確認済)

---

## 3. build + launch verification

| step | status |
|---|---|
| `autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio -DLL_TESTS:BOOL=FALSE -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE --package --chan AYAstorm-release` (新規 file 追加で configure 必須) | ✓ exit 0 |
| `autobuild build -A 64 -c ReleaseFS_open --no-configure` | ✓ exit 0、`llpipelineframecontext.cpp.o` 構築済、`[100%] Built target llpackage` |
| `./install.sh` (`build-linux-x86_64/newview/packaged/`) | ✓ `/home/ishikawa/ayastorm` 配置完了 |
| `rm -rf ~/.ayastorm_x64/cache/` | ✓ 完遂 |
| AYA launch + 通常起動報告「ちゃんと描画されてます」 | ✓ 2026-05-31 |

### 3.1 ログ確認 (Claude 側 `~/.ayastorm_x64/logs/AYAstorm.log` grep 結果)

| marker | 期待 | 実測 |
|---|---|---|
| `vulkanDebugCallback` (ERROR/WARNING 受信) | 0 件 (release build = severity filter VERBOSE/INFO 購読外、ERROR/WARNING は購読対象だが今回 callback fire 0) | 0 件 ✓ |
| `VK_ERROR` / `VUID-` / `validation failed` | 0 件 | 0 件 ✓ (false positive 2 件は `LogViewerStatsPacket` 内 `'warning_count':i0` JSON 値、Vulkan 由来でない) |
| `#VkRecord#` pool hook fire | 12/12 件 | 12 件 ✓ (段階 3 範式継承) |
| `#Vulkan#` marker INFO 出力 | prior baseline 57 unique 整合範囲内 | 56 件 ✓ (起動から shutdown まで全 unique 含む) |
| shutdown clean | `Vulkan device destroyed → Vulkan instance destroyed → Goodbye → status: stopped` | ✓ (log tail 10 行で確認) |
| regression | 描画通常 / UI 反応 / shader effect 維持 | ✓ AYA 報告 + run_time 150.3s / meters_traveled 264.4m / regions_visited 2 / fps 45 = 通常使用域 |

### 3.2 validation strict 検証は sub-step 3.5-a で実施済継承

sub-step 3.5-a (2026-05-30) で validation force-enable build により段階 3 全体の validation strict 検証 PASS 済 (vulkanDebugCallback 経由 ERROR/WARNING 0 件)。sub-step 4.1-α は段階 3 配線に変更を加えず lifecycle wrapper 追加のみのため、validation strict 再走は不要 (段階 4 全 sub-step 完遂後の sub-step 4.5 self-check で一括再走想定)。

---

## 4. 設計 deviation 履歴 (sub-doc 04 起草時から本実装まで)

### 4.1 案 B 採用 (measurement-first 検証 log 配線 skip)

sub-doc 04 §2.5 measurement-first 範式は struct shape を sub-step 4.1 着手時 final 化する観点で、本 sub-step 4.1-α では task #1 (struct shape final 化) を完遂条件として吸収。実装後の measurement 検証 log 配線は AYA 承認下で skip 採用。

**理由**:
- lifecycle 4 関数 stub の単純性 (実コード = pass type enum 代入 + cull pointer 代入 のみ)
- 案 B では起動/shutdown clean + regression 0 + code review 整合性で satisfy
- build cycle 1 回で commit 到達、案 A (2 build cycle) 対比 build 時間節約

**trade-off**: lifecycle hit 経路の log evidence は無し。getFrameCull() 経由 cull pointer 整合性は 45 occurrences の code review + AYA 起動 PASS で transit satisfy。sub-step 4.3 で per-pool 実 scene draw 移植時に必要となれば再度 measurement log 配線判断。

### 4.2 pipeline.cpp file-local helper `getFrameCull()` 採用 (spec literal 短縮)

sub-doc 04 §3.2 spec literal は `LLPipelineFrameContext::getInstance().getCullResult()` の direct call を 45 caller 全部に挿入する path。本実装では pipeline.cpp 匿名 namespace 内 file-local helper `inline LLCullResult* getFrameCull()` を配置し、caller は `getFrameCull()` で短縮。

**理由**:
- pipeline.cpp 内 implementation detail (file scope) = 外部 file から参照不可、AYA 確認不要範囲
- 替換時の visual diff 最小化 (`sCull` → `getFrameCull()` で symbol 1:1)
- 後続 sub-step 4.3 で const-ref passing through render path 化する際の差替え単位 (本 helper 撤去 + 各 caller に const-ref 受渡し置換)

**stop-gap 性格**: 本 helper は sub-step 4.3 でも撤去対象。sub-step 4.1-β / 4.2 では同様の file-local helper pattern を mRT / bool flags にも適用想定 (caller diff 最小化)。

### 4.3 (記録なし) sub-doc 04 spec 通り実装の項目

- LLPipelineFrameContext singleton Meyer pattern ✓
- lifecycle 4 関数命名 (beginFrameContext / endFrameContext / beginPass / endPass) ✓
- EPassType enum 配置 (PASS_NONE / SHADOW / DEFERRED / FORWARD / POST) ✓
- sCull aggregation getter/setter (`getCullResult` / `setCullResult`) ✓
- grabReferences / clearReferences lifecycle 配線位置 ✓

---

## 5. risks / caveats

### 5.1 sub-step 4.1-β scope = 159 occurrences、α (45) より約 3.5x

mRT 替換は pipeline.{h,cpp} 内 112 件 + external 12 file 47 件 = 159 occurrences (実測 confirm 2026-05-31: pipeline.cpp 106 / pipeline.h 6 / external 47)。external file 12 件の breakdown:

| file | occurrences |
|---|---|
| `lldrawpoolalpha.cpp` | 13 |
| `llviewerwindow.cpp` | 7 |
| `llviewerdisplay.cpp` | 6 |
| `llfloatermodelpreview.cpp` | 4 |
| `rlveffects.cpp` | 4 |
| `llheroprobemanager.cpp` | 2 |
| `llreflectionmapmanager.cpp` | 2 |
| `lldrawpoolwater.cpp` | 2 |
| `llviewershadermgr.cpp` | 2 |
| `llreflectionmap.cpp` | 2 |
| `rlvhandler.cpp` | 2 |
| `llgltfmaterialpreviewmgr.cpp` | 1 |

(shader 2 件 `ayaAlphaPlateCompositeF.glsl` / `fsObjectIDV.glsl` の `mRT` 文字列は GLSL 内 identifier、AYAstorm 改変 13 file shader 改変禁止 + そもそも替換対象外 = exclude。`llpipelineframecontext.h` 1 件は本 sub-step で配置済 sCull aggregation の forward declaration 行 = mRT ではない別文脈、exclude)

### 5.2 mRT 替換 strategy (案 B file-local helper 適用想定)

α phase と同 pattern で進める想定:
- pipeline.cpp 内 file-local helper `getFrameRT()` 配置 (LLPipelineFrameContext 内 `getActiveRT()` wrapper)
- pipeline.h 内 mRT member 宣言は当面残置 (LLPipeline class member 削除は per-pool 実 scene draw 移植完遂後の sub-step 4.5 で実施想定)
- external 12 file はそれぞれ `LLPipelineFrameContext::getInstance().getActiveRT()` 直接 call、または local accessor 配置の判断 (file ごとに caller 密度で決定)

**注意**: mRT は **write 不定 (pass context 切替)** = sub-doc 04 §2 row 19。getter のみでなく setter access path も必要、lifecycle 配線箇所 (beginPass / endPass) との整合確認が必須。sub-step 4.1-β 着手時の最初の trace は「mRT write 経路一覧 (pipeline.cpp 内 何箇所で `mRT = ...` 代入されているか)」。

### 5.3 LLPipelineFrameContext 内 RT 配置 = sub-doc 04 §3.4 spec 通り `RenderTargetPack*` 採用

sub-doc 04 §3.4 spec literal:
```cpp
RenderTargetPack* getActiveRT() const; // mRT
```

本 sub-step 4.1-β 着手時に `llpipelineframecontext.h` 内 fields に `RenderTargetPack* mActiveRT;` を追加 + `getActiveRT()` / `setActiveRT()` getter/setter 配線。lifecycle 4 関数内での null reset 配線判断 (現状 mCullResult は endFrameContext で nullptr reset、mActiveRT も同様 reset するか pass context 切替で挙動継続するかは write 経路 trace 後判断)。

### 5.4 sub-step 4.2 (10 bool flag) は 4.1-β 完遂後着手

handoff-stage-4-prelude.md §2 cadence: 4.1 → 4.2 → 4.3 → 4.4 → 4.5 (順次)。並走しない (charter §7.5 r41 着手中 refine 可範囲、AYA 承認下)。

### 5.5 context budget concern (次 session)

mRT 替換 159 occurrences は α (45) より約 3.5x、replace_all + 個別 edit + build verify + log 解析を全部 1 session 内で完遂すると context 圧迫の可能性。**proactive handoff 範式 (feedback_proactive_handoff.md absolute rule)** に従い、必要なら sub-step 4.1-β 内でも段階分割 (例: pipeline.{h,cpp} 内 112 件先行 commit + external 12 file 後 commit) を AYA 承認下で判断。

---

## 6. next session entry point

### 6.1 次 session 着手前の準備 (4 段 cadence)

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得、または local commit `58ae59bcb7` ready 確認)
2. 本 handoff doc 通読
3. sub-doc `04-frame-context.md` §3 LLPipelineFrameContext struct 設計 + §5 sub-step 4.1 cadence 再確認
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 6.2 sub-step 4.1-β 着手 task 候補 (本 handoff §5.2 から refine)

| task | 内容 | 推定規模 |
|---|---|---|
| 4.1-β-1 | mRT write 経路 trace (pipeline.cpp 内 `mRT = ...` 代入箇所一覧 + lifecycle 4 関数 (beginPass / endPass) との整合確認) | trace 30 分 |
| 4.1-β-2 | llpipelineframecontext.{h,cpp} 内 `RenderTargetPack*` field + `getActiveRT()` / `setActiveRT()` 配線 | 実装 15 分 |
| 4.1-β-3 | pipeline.cpp 内 file-local helper `getFrameRT()` 配置 + 112 件 replace_all 1:1 替換 (replace_all 安全性 `Grep mRT[a-zA-Z]` 事前 verify) | 実装 30 分 |
| 4.1-β-4 | external 12 file 47 件 1:1 替換 (file ごとに include 追加判断) | 実装 1-2 時間 |
| 4.1-β-5 | autobuild incremental build verify (CMakeLists.txt 変更なし、`--no-configure` で OK) | 20-30 分 |
| 4.1-β-6 | AYA launch verify (案 B cadence 継承: vulkanDebugCallback 0 / 12 #VkRecord# fire / shutdown clean / regression 0) | AYA |
| 4.1-β-7 | commit (案 B = measurement log skip) | 5 分 |

### 6.3 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)
- **段階 1 + 段階 2 + 段階 3 + 4.1-α 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **mRT は write 不定** (sub-doc 04 §2 row 19、setter 経路必須、lifecycle 配線整合確認)
- **per-pool 実 scene draw 移植は sub-step 4.3 scope** (本 sub-step 4.1-β は frame state 集約のみ、実 scene draw 改変なし)
- **案 B cadence 継承想定** (AYA 別途指示なき限り measurement log skip)
- **context budget proactive 監視** (feedback_proactive_handoff.md absolute rule、必要なら 4.1-β 内段階分割を AYA 承認下で判断)

### 6.4 commit 戦略

- 1 件想定 (sub-step 4.1-β 全体 = mRT 159 occurrences 替換 + lifecycle 配線)
- 段階分割判断は context 状況 + AYA 承認下で決定 (5.5 reference)
- commit message style = `feat(r41): sub-step 4.1-β 完遂 (案 B 2 commit 分割 2/2 = ...)` 範式 (本 handoff §6.4 = 4.1 全体は α/β の 2 commit 分割 sealed)

---

## 7. 関連 doc / memory cross-ref

### 7.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter (§2 領域 4 1.00 PM 高 risk)
- `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` — 段階 4 領域 4 sub-doc (sub-step 4.1-β marker 着手 ready 状態へ)
- `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` — sub-step 4.4 part B で参照
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-3-complete.md` — 段階 3 完遂 (引継ぎ 3 件 = per-pool 実 scene draw 移植 + RAII setter dead-store 化 + LLVKRenderer skeleton declaration)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-4-prelude.md` — 段階 4 pre-emptive handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-1-alpha-complete.md` — 本 handoff (4.1-α 全完遂 → 4.1-β 着手境界)

### 7.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 4.1-β 着手 ready 状態に update)
- `project_build_procedure.md` — autobuild + install + cache clear フロー (本 sub-step で実施)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§2 file/line / §3 ログ marker 表)
- `feedback_no_auto_commit.md` — commit は AYA 明示指示で実施 (本 sub-step 4.1-α は指示済 → `58ae59bcb7`)
- `feedback_proactive_handoff.md` — 周回境界での能動 handoff 起草 (本 file、AYA 「案 Y で handoff お願いします」承認 2026-05-31)
- `feedback_one_step_at_a_time.md` — 1 メッセージ 1 アクション、4.1-α / 4.1-β 分割 cadence
- `feedback_remove_verification_logs.md` — 案 B では log 配線 skip = 除去対象なし
- `feedback_no_claude_coauthor.md` — 全 commit messages で Claude 共著行なし
- `reference_log_path.md` — `~/.ayastorm_x64/logs/AYAstorm.log` (Linux)
