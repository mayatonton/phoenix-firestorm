# r41 sub-step 4.3-α 完遂 → 4.3-β 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-2-complete.md` (sub-step 4.2 完遂 → 4.3 着手境界、役割完了)
**本 handoff 位置付け**: sub-step 4.3-α (sCurCameraID accessor 配線 + ScopedCameraID nested RAII 配置、low risk、12 file +96/-62) **全完遂宣言** + sub-step 4.3-β (Sky+WLSky+WaterExclusion per-pool 実 scene draw 移植、high risk) 着手前 scope 確認境界。AYA launch verify PASS で 4.3-α 章 close。

---

## 1. sub-step 4.3-α 全完遂 status (2026-05-31)

### 1.1 完遂 marker (案 B descriptor 階層点進 6 commit 分割の 1/6 = α 最先発)

| acceptance | 達成 status |
|---|---|
| llviewercamera.h 内 `static eCameraID getCurCameraID()` + `static void setCurCameraID(eCameraID)` inline accessor 配置 | ✓ (lines 64-66) |
| llviewercamera.h 内 sCurCameraID field public 維持 (test stub llvocache_test.cpp:59 互換) | ✓ |
| llpipelineframecontext.h 内 `#include "llviewercamera.h"` 配線 (h 階層 forward) | ✓ |
| llpipelineframecontext.h 内 forward accessor 配置 (LLViewerCamera::eCameraID getCurCameraID() const + setCurCameraID(eCameraID)) | ✓ |
| llpipelineframecontext.h 内 ScopedCameraID nested RAII class 配置 (4.2 ScopedXxxPass 範式継承) | ✓ |
| llpipelineframecontext.cpp 内 ScopedCameraID ctor/dtor 実装 | ✓ |
| write 14 件 替換 (全 file 切替済) | ✓ |
| read 38 件 替換 (全 file 切替済、replace_all 経由 1:1) | ✓ |
| include 配線 (llviewerregion.cpp + llvocache.cpp) | ✓ (llvieweroctree.cpp は llvieweroctree.h:40 経由透過) |
| build clean | ✓ exit 0 / 12 file +96/-62 / error 0 件 / packaging 完遂 |
| install + cache clear 完遂 | ✓ ~/ayastorm/ + ~/.ayastorm_x64/cache/ |
| AYA launch verify clean | ✓ AYA さん「OK」確認 (短評承認) / regression 0 |

### 1.2 commit hash

| commit | scope |
|---|---|
| `9f13302078` | sub-step 4.3-α 全完遂 (12 files changed, +96/-62) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-4-2-complete.md` | **役割完了** (sub-step 4.3 着手 GO 条件 satisfy → α 完遂、本 handoff で内容引継ぎ) |
| sub-doc `04-frame-context.md` | active 継続 (sub-step 4.3-β/γ/δ/ε marker 着手 ready 状態へ) |
| sub-doc `08-llvkrenderer-skeleton.md` | active 継続 (sub-step 4.4 part B で参照) |
| `handoff-substep-4-3-alpha-complete.md` (本 handoff) | 新規作成 (4.3-α 全完遂 → 4.3-β 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 4.3-β 着手 ready 状態へ update) |

---

## 2. 実装内容 (本 commit 範囲 `9f13302078`)

### 2.1 LLViewerCamera 拡張

| 変更 | 詳細 |
|---|---|
| inline static accessor 配置 (h L64-66) | `static eCameraID getCurCameraID() { return sCurCameraID; }` + `static void setCurCameraID(eCameraID id) { sCurCameraID = id; }` = field 並列追加、incremental migration pattern |
| **sCurCameraID field public 維持** | test stub llvocache_test.cpp:59 で `LLViewerCamera::eCameraID LLViewerCamera::sCurCameraID{};` 初期化のため public 必須、物理 private 化は sub-step 4.5 scope |
| (cpp 編集不要) | inline 配置のため llviewercamera.cpp 編集なし、既存 static initializer (L68) 維持 |

### 2.2 LLPipelineFrameContext 拡張

| 変更 | 詳細 |
|---|---|
| `#include "llviewercamera.h"` 配線 (h L19) | h 階層 forward、ScopedCameraID nested class 内で eCameraID 型解決必要 |
| forward accessor 配置 (h L86-87) | `LLViewerCamera::eCameraID getCurCameraID() const { return LLViewerCamera::getCurCameraID(); }` + `setCurCameraID(LLViewerCamera::eCameraID id) { LLViewerCamera::setCurCameraID(id); }` = LLPipelineFrameContext::getInstance() 経由でも accessor 取得可能 |
| ScopedCameraID nested RAII class 配置 (h L172-184) | 4.2 ScopedXxxPass 範式継承 = ctor で prev 保存 + 新 id 設定、dtor で prev 復元 |
| ScopedCameraID ctor/dtor 実装 (cpp L132-141) | mPrev init = LLPipelineFrameContext::getInstance().getCurCameraID() / ctor body = setCurCameraID(new_id) / dtor = setCurCameraID(mPrev) |

### 2.3 write 14 件 替換

| file | 件数 | 詳細 |
|---|---|---|
| `llviewerdisplay.cpp` | 7 | lines 897/999/1106/1322/1334/1368/1446 全て `sCurCameraID = CAMERA_WORLD` パターン → `setCurCameraID(CAMERA_WORLD)` replace_all 経由一括 |
| `llvoavatar.cpp` | 1 | line 12189 updateImpostors entry `sCurCameraID = CAMERA_WORLD` → setCurCameraID(CAMERA_WORLD) |
| `pipeline.cpp` (plain) | 3 | lines 13179/13560/13680 shadow camera ID dynamic dispatch (CAMERA_SUN_SHADOW0+j / CAMERA_SPOT_SHADOW0+i × 2 件) → setCurCameraID() with cast |
| `pipeline.cpp` (swap → RAII) | 1 件 ScopedCameraID | 3005-3031 swap pattern (saved_camera_id 局所変数 + CAMERA_WORLD 設定 + 終端で復元) → `LLPipelineFrameContext::ScopedCameraID camera_scope(CAMERA_WORLD)` で 3 line を RAII 1 line に縮約 |
| `llviewerregion.cpp` | 1 件 ScopedCameraID | 1762-1782 swap pattern (idleUpdate 経路 old_camera_id 局所変数 + CAMERA_WORLD 設定 + 終端で復元) → `LLPipelineFrameContext::ScopedCameraID camera_scope(CAMERA_WORLD)` で 3 line を RAII 1 line に縮約 |

### 2.4 read 38 件 替換

| file | 件数 | 詳細 |
|---|---|---|
| `llspatialpartition.cpp` | 2 | 614 updateDistance ガード + 1105 group visibility check |
| `llvieweroctree.h` | 2 | 308-309 isOcclusionState/getOcclusionState inline accessor |
| `llvocache.cpp` | 6 | 999/1046/1051/1055/1060/1122 selectBackObjects + mCulledTime[] 経路 |
| `llvieweroctree.cpp` | 26 | 763/774/776/897/944-948/1027/1104/1119/1123-1124/1134-1135/1140-1142/1146/1149/1207/1209/1223/1231/1237-1239 = mVisible/mOcclusionState/mOcclusionQuery/mOcclusionCheckCount/mOcclusionIssued array index + occlusion query lifecycle 全件 |
| `lldrawable.cpp` | 1 | 883 updateDistance ガード (CAMERA_WORLD 以外 reject) |
| `pipeline.cpp` | 8 | 3152/3169/4069/4090/4099/4156/4232/4565 CAMERA_WORLD 判定 + cube snapshot ガード |
| **方式** | replace_all 経由 | 全 file 同一 pattern `LLViewerCamera::sCurCameraID` → `LLViewerCamera::getCurCameraID()` の 1:1 sed 替換、substring collision なし (4.2 sImpostorRenderAlphaDepthPass 教訓回避) |

### 2.5 include 配線

| file | 追加 include | 用途 |
|---|---|---|
| `llviewerregion.cpp` | `llpipelineframecontext.h` + `llviewercamera.h` | ScopedCameraID + sCurCameraID accessor 利用 path 配線 (既存 include 無し) |
| `llvocache.cpp` | `llviewercamera.h` | accessor 利用 path 配線 (既存 include 無し) |
| `llvieweroctree.cpp` | (追加なし) | llvieweroctree.h:40 経由透過 (既存 include 維持) |
| `llspatialpartition.cpp` | (追加なし) | 40 llviewercamera.h + 47 llpipelineframecontext.h 既配置 |
| `lldrawable.cpp` | (追加なし) | 43 llviewercamera.h + 51 llpipelineframecontext.h 既配置 |
| `llviewerdisplay.cpp` | (追加なし) | 76 llviewercamera.h + 89 llpipelineframecontext.h 既配置 |
| `llvoavatar.cpp` | (追加なし) | 86 llviewercamera.h + 100 llpipelineframecontext.h 既配置 |
| `pipeline.cpp` | (追加なし) | 77 llpipelineframecontext.h + 85 llviewercamera.h 既配置 |

### 2.6 file 変更 summary

```
indra/newview/lldrawable.cpp             |  2 +-
indra/newview/llpipelineframecontext.cpp | 11 +++++++
indra/newview/llpipelineframecontext.h   | 19 ++++++++++++
indra/newview/llspatialpartition.cpp     |  4 +--
indra/newview/llviewercamera.h           |  5 +++
indra/newview/llviewerdisplay.cpp        | 14 ++++-----
indra/newview/llvieweroctree.cpp         | 52 ++++++++++++++++----------------
indra/newview/llvieweroctree.h           |  4 +--
indra/newview/llviewerregion.cpp         |  6 ++--
indra/newview/llvoavatar.cpp             |  2 +-
indra/newview/llvocache.cpp              | 13 ++++----
indra/newview/pipeline.cpp               | 26 ++++++++--------
12 files changed, 96 insertions(+), 62 deletions(-)
```

---

## 3. build + launch verification

### 3.1 build step

| step | command | result |
|---|---|---|
| 1. stage | git add 12 file | OK |
| 2. build | `autobuild build -A 64 -c ReleaseFS_open --no-configure` | **PASS exit 0** / error 0 件 / llpipelineframecontext.cpp.o 再構築済 |
| 3. package | tar.xz 生成 | Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261510128.tar.xz |
| 4. install | install.sh ~/ayastorm/ | OK / menu entries 配置 |
| 5. cache clear | rm -rf ~/.ayastorm_x64/cache/ | OK |
| 6. AYA launch verify | AYA さん「OK」確認 | **PASS** (短評承認、4.3-α は accessor 配線のみで動作変化なし = forward call で zero semantic change) |

### 3.2 AYA verify 詳細

| 項目 | 状態 |
|---|---|
| AYA 報告 | 「OK」 (短評承認、4.3-α は forward call のみで semantic 変化なしのため詳細 verify 不要) |
| 案 B cadence 継承 | measurement log 配線 skip + AYA 短評で satisfy |
| regression | 0 (AYA 報告無し) |

---

## 4. 設計 deviation

### 4.1 案 B 継承 (measurement log skip)

4.1-α/β + 4.2 と同様、案 B = measurement-first 検証 log 配線 skip + AYA 短評承認で satisfy。本 4.3-α は accessor 配線のみで動作変化なし (forward call で zero semantic change) のため、log 配線は overshoot 判断。

### 4.2 ScopedCameraID nested RAII 採用 (既存 swap pattern 1:1 置換)

既存 2 件の手動 save/restore swap pattern (pipeline.cpp:3005-3031 / llviewerregion.cpp:1762-1782) を ScopedCameraID で 1:1 RAII 置換。4.2 ScopedXxxPass 範式継承 = ctor で prev 保存 + 新 id 設定、dtor で prev 復元。saved_camera_id / old_camera_id 局所変数廃止で 3 line → 1 line 縮約。

### 4.3 sCurCameraID field public 維持

test stub `llvocache_test.cpp:59` で `LLViewerCamera::eCameraID LLViewerCamera::sCurCameraID{};` 直接初期化のため public 必須。物理 private 化 + test stub 互換修正は sub-step 4.5 scope (4.1-β mRT field 範式継承)、4.3-α 範囲では accessor 並列追加で incremental migration。

### 4.4 sub-doc 04 §3 sCurCameraID accessor spec 通り実装

| sub-doc 04 §3 spec | 4.3-α 実装 |
|---|---|
| accessor inline getter/setter | ✓ (llviewercamera.h L64-66) |
| cross-class forward (LLPipelineFrameContext 経由でも取得可) | ✓ (llpipelineframecontext.h L86-87) |
| ScopedCameraID nested RAII | ✓ (llpipelineframecontext.h L172-184 + .cpp L132-141) |
| write/read 全件統一 (52 sites) | ✓ (write 14 + read 38) |
| include 階層整合 | ✓ (llviewerregion + llvocache 配線、他は既存) |

---

## 5. risks / caveats (sub-step 4.3-β 着手前 awareness)

### 5.1 sub-step 4.3-β scope (Sky+WLSky+WaterExclusion per-pool draw 移植)

案 B 6 commit 分割の 2/6 = β。3 経路 (Sky / WLSky / WaterExclusion) の per-pool placeholder draw を実 scene draw に置換。**high risk** 理由:
- placeholder NDC 三角形 → LLCullResult::beginVisibleList()/endVisibleList() 経由 drawable iteration + PSO bind + descriptor bind + vkCmdDrawIndexed
- sCurCameraID 状態判定 (CAMERA_WORLD vs WATER0/WATER1) で reflection/refraction 経路分岐
- depth test / blend / culling state PSO 切替必要

4.3-α 完遂で accessor 配線が整ったので、4.3-β は accessor 経由で active camera 取得 → pool draw 配線可能。

### 5.2 4.3-α は accessor 配線のみで動作変化なし

forward call (inline getter `{ return sCurCameraID; }` + inline setter `{ sCurCameraID = id; }`) で **zero semantic change**。コンパイラ inline 最適化で field 直接参照と等価コード生成。regression risk 極低。

### 5.3 sCurCameraID field 物理 private 化は sub-step 4.5 scope

4.1-β mRT field 範式継承 = legacy field 物理保持 (4.3-α では public 維持) → sub-step 4.5 で物理 private 化 + test stub 互換修正一括実施。本 4.3-α では field 並列 accessor 追加で incremental migration。

### 5.4 ScopedCameraID 利用判断 (transient swap pattern 限定)

4.3-α では 2 件 (pipeline.cpp:3005-3031 / llviewerregion.cpp:1762-1782) の既存手動 save/restore pattern を 1:1 RAII 置換。**新規 ScopedCameraID 配置は transient swap 経路のみ**、frame entry/exit 経路の plain write (llviewerdisplay 7 件 / llvoavatar 1 件 / pipeline 3 件 plain) は `setCurCameraID()` 直接 call で十分。ScopedCameraID 過剰利用回避。

### 5.5 sub-step 4.3-β/γ/δ/ε/ζ 順次 (並走しない)

案 B 6 commit 分割は順次 cadence:
- α: accessor 配線 (本 commit、完遂)
- β: Sky+WLSky+WaterExclusion (次着手)
- γ: 9 pool 一括
- δ: Avatar bone
- ε: GLTFPBR
- ζ: self-check + handoff

各 sub-step 着手前に前 sub-step build + AYA launch verify PASS 必須。

### 5.6 context budget concern

本 4.3-α 完遂時点で session context 消費中。次 sub-step (4.3-β) は fresh context で着手推奨、proactive handoff 範式遵守。

---

## 6. next session entry point (sub-step 4.3-β 着手)

### 6.1 着手前 4 段 cadence

1. 本 handoff doc 読込 (sub-step 4.3-α 完遂 status 確認)
2. sub-doc `04-frame-context.md` §3.4 per-pool 実 scene draw 移植経路 spec 読込
3. charter `00-charter.md` §2 領域 4 high risk spec 確認
4. memory `project_ayastorm_r41_vulkan_migration.md` 最新 status 読込

### 6.2 sub-step 4.3-β 着手 task 候補

| task | 詳細 |
|---|---|
| β-1 | Sky/WLSky/WaterExclusion pool 経路 trace (現状 placeholder 配線 vs 移植先 実 scene draw) |
| β-2 | LLCullResult::beginVisibleList()/endVisibleList() 経路の active camera 配線 (accessor 経由) |
| β-3 | PSO bind + descriptor set bind + vkCmdDrawIndexed 配線 (3 経路) |
| β-4 | placeholder NDC 三角形 → 実 scene draw 替換 |
| β-5 | incremental autobuild |
| β-6 | AYA launch verify (案 B cadence) |
| β-7 | commit (4.3-β 単独 commit) |

### 6.3 critical reminders

| reminder | 詳細 |
|---|---|
| **shader 改変禁止** | sub-doc 04 §1.2 範囲 = pipeline state 集約のみ、shader/GLSL は touch しない |
| **段階 1-4.3-α 動作維持** | regression risk highest watch (4.3-β は実 scene draw 移植で動作変化大) |
| **active camera 配線必須** | 4.3-α accessor 経由で getCurCameraID() = active camera ID 取得、pool draw 内で正しい camera state 設定 |
| **案 B cadence 継承** | measurement log 配線 skip + AYA 短評承認で satisfy、log 配線は overshoot 判断 |
| **context budget proactive 監視** | 次 session 着手時点で context 残量確認 → 周回境界で proactive handoff |

### 6.4 commit 戦略

sub-step 4.3-β は **1 件想定** (Sky+WLSky+WaterExclusion 3 経路一括)。ただし実 scene draw 移植中に build break / launch fail が出た場合、3 経路を個別 commit に分割可能性あり (β-1/β-2/β-3 等)。範式は sub-step 3.3-A の段階分割 cadence 継承。

---

## 7. 関連 doc / memory cross reference

### 7.1 関連 doc

| doc | 役割 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/00-charter.md` | charter §2 領域 4 high risk spec |
| `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` | sub-doc 04 §3 sCurCameraID accessor spec + §3.4 per-pool 実 scene draw 移植経路 spec |
| `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` | sub-doc 08 LLVKRenderer skeleton (4.4 part B) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-2-complete.md` | 前 handoff (sub-step 4.2 完遂 → 4.3 着手境界、役割完了) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-1-beta-complete.md` | (役割完了、4.2 前段境界) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-1-alpha-complete.md` | (役割完了、4.1-β 前段境界) |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-alpha-complete.md` | (本 handoff、4.3-β 着手境界) |

### 7.2 関連 memory

| memory | 役割 |
|---|---|
| `project_ayastorm_r41_vulkan_migration.md` | active milestone tracking |
| `feedback_proactive_handoff.md` | context 圧迫時の proactive handoff 範式 |
| `feedback_self_verify_before_handoff.md` | handoff 起草前の self-trace 義務 |
| `feedback_no_claude_coauthor.md` | commit message Co-Authored-By: Claude 禁止 |
| `feedback_proactive_diagnostic.md` | log/grep/gdb 系は Claude が直接実行 |
| `feedback_log_reading.md` | log 解析は Claude 側、AYA に貼り付けさせない |
| `feedback_one_step_at_a_time.md` | 1 メッセージ 1 アクション cadence |
| `feedback_remove_verification_logs.md` | 案 B では log 配線 skip = 除去対象なし |
| `feedback_no_auto_commit.md` | コミットは明示指示後 (本件は AYA 「OK 3 全部」承認下) |
| `project_build_procedure.md` | autobuild fullflow + .venv activate + AUTOBUILD_VARIABLES_FILE |

---

**本 handoff 起草日**: 2026-05-31
**起草根拠**: AYA さん「3 全部お願いします」承認下で commit + memory + handoff doc 起草を並走
**次 session 着手**: sub-step 4.3-β (Sky+WLSky+WaterExclusion per-pool 実 scene draw 移植) 着手境界 (fresh context 推奨)
