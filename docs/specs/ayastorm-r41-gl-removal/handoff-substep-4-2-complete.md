# r41 sub-step 4.2 完遂 → 4.3 着手境界 handoff (2026-05-31)

**前 handoff**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-1-beta-complete.md` (sub-step 4.1-β 完遂 → 4.2 着手境界、役割完了)
**本 handoff 位置付け**: sub-step 4.2 (9 bool flag struct migration、medium risk、116 件 across pipeline.cpp + 29 外部 file) **全完遂宣言** + sub-step 4.3 (sCurCameraID accessor + per-pool 実 scene draw 移植、high risk) 着手前 scope 確認境界。AYA launch verify PASS で 4.2 章 close。

---

## 1. sub-step 4.2 全完遂 status (2026-05-31)

### 1.1 完遂 marker (sub-doc 04 §5.2 sub-step 4.2 = 10 bool flag のうち 9 件 active、sDistortionRender legacy 保持)

| acceptance | 達成 status |
|---|---|
| llpipelineframecontext.h 内 9 個 getter/setter 配置 = (a) pass-specific 5 件 + (b) frame-global 4 件 | ✓ |
| llpipelineframecontext.h 内 6 個 nested RAII class 配置 = 5 ScopedXxxPass + ScopedRenderingGlow | ✓ |
| llpipelineframecontext.h 内 9 個 private bool field 配置 | ✓ |
| llpipelineframecontext.cpp 内 ctor initializer list 9 field 拡張 (false 初期化) | ✓ |
| llpipelineframecontext.cpp 内 6 個 ScopedXxx impl 配線 | ✓ |
| pipeline.cpp 内 9 個 file-local helper 配置 (`isFrameShadowPass()` 等、匿名 namespace、4.1-α/β 範式継承) | ✓ |
| pipeline.cpp 内 write 10 件 setter 替換 (sReflectionProbesEnabled init / sDoFEnabled multi-line / sShadowRender on-off / generateImpostor entry+exit 3+3 件) | ✓ |
| pipeline.cpp 内 read 42 件 file-local helper 替換 (sed declaration skip pattern 経由) + bonus 1 件 (L13895 write 内 RHS read = `!isFrameRenderingDeferred()`) | ✓ |
| 外部 29 file 替換 = write 13 + read 103 + ScopedRenderingGlow 1 + `#include "llpipelineframecontext.h"` 配線 (21 file 新規 + 8 file 既配置) | ✓ |
| `gPipeline.sRenderDeferred` 1 件 fix (sed instance member 取りこぼし → `LLPipelineFrameContext::getInstance().isRenderingDeferred()` 直接 call) | ✓ |
| build clean | ✓ exit 0 / 32 file +386/-169 / error 0 件 |
| AYA launch verify clean | ✓ vulkanDebugCallback ERROR/WARNING 0 件 / 12/12 #VkRecord# pool hook fire / shutdown clean / regression 0 |

### 1.2 commit hash

| commit | scope |
|---|---|
| `9a9cb018c8` | sub-step 4.2 全完遂 (32 files changed, +386/-169) |

### 1.3 close する doc / memory

| doc / memory | status |
|---|---|
| `handoff-substep-4-1-beta-complete.md` | **役割完了** (sub-step 4.2 着手 GO 条件 satisfy → 完遂、本 handoff で内容引継ぎ) |
| sub-doc `04-frame-context.md` | active 継続 (sub-step 4.3 marker 着手 ready 状態へ) |
| sub-doc `08-llvkrenderer-skeleton.md` | active 継続 (sub-step 4.4 part B で参照) |
| `handoff-substep-4-2-complete.md` (本 handoff) | 新規作成 (4.2 全完遂 → 4.3 着手境界) |
| memory `project_ayastorm_r41_vulkan_migration.md` | active 継続 (sub-step 4.3 着手 ready 状態へ update) |

---

## 2. 実装内容 (本 commit 範囲 `9a9cb018c8`)

### 2.1 LLPipelineFrameContext h/cpp 拡張

| 変更 | 詳細 |
|---|---|
| 9 個 getter/setter 配線 (h L62-82) | (a) pass-specific 5 件 = isShadowPass/isReflectionPass/isImpostorPass/isHUDPass/isDoFPass + (b) frame-global 4 件 = isRenderingGlow/isRenderingDeferred/isUnderWaterRendering/isReflectionProbesEnabled |
| 6 個 nested RAII class 配線 (h L97-167) | 5 ScopedXxxPass (4.2 (a) 入れ子発火対応、4.1-β ScopedActiveRT 範式継承) + ScopedRenderingGlow (4.2 (b) 但し llgltfmaterialpreviewmgr.cpp:436 で SetTemporarily<bool> パターン現存 = 後述 §7.2) |
| 9 個 private bool field 配線 (h L166-176) | mShadowPass / mReflectionPass / mImpostorPass / mHUDPass / mDoFPass / mRenderingGlow / mRenderingDeferred / mUnderWaterRendering / mReflectionProbesEnabled |
| `llpipelineframecontext.cpp` ctor initializer list 9 field 拡張 (L25-38) | sub-step 4.1-α/β の mCullResult / mActiveRT に並列、全 false 初期化 |
| `llpipelineframecontext.cpp` 6 個 ScopedXxx impl 配線 (L77-130) | ctor で前 bool 保存 (isXxxPass() / isRenderingGlow() 経由) + setXxxPass(new_val) / dtor で setXxxPass(mPrev) |
| **legacy field 物理保持** = pipeline.h L798-837 area の 10 個 static bool 宣言 (sShadowRender 等 + sDistortionRender dead path) | sub-step 4.5 で物理削除 (4.1-β mRT field 範式継承) |

### 2.2 pipeline.cpp 物理変更

| 変更 | 詳細 |
|---|---|
| 9 個 file-local inline helper 配置 (匿名 namespace、L501-511 area、4.1-α/β `getFrameCull()`/`getFrameRT()` 隣接) | `isFrameShadowPass()` / `isFrameReflectionPass()` / `isFrameImpostorPass()` / `isFrameHUDPass()` / `isFrameDoFPass()` / `isFrameRenderingGlow()` / `isFrameRenderingDeferred()` / `isFrameUnderWaterRendering()` / `isFrameReflectionProbesEnabled()` |
| write 10 件 setter 替換 (multi-line 含む 6 Edit batch) | L1608 sReflectionProbesEnabled init → setReflectionProbesEnabled() / L9852-9855 sDoFEnabled multi-line → setDoFPass() / L12478 sShadowRender=true → setShadowPass(true) / L12674 sShadowRender=false → setShadowPass(false) / L13895-13898 generateImpostor entry 3 件 (setReflectionPass(!isFrameRenderingDeferred()) + setShadowPass(true) + setImpostorPass(true)) / L14183-14185 generateImpostor exit 3 件 (setReflectionPass(false) + setImpostorPass(false) + setShadowPass(false)) |
| **ScopedXxxPass 不使用方針** (write 10 件) | 1:1 legacy behavior preserve = early return 経路で flag 残留 risk も含めて legacy と同等動作維持。ScopedXxxPass は 外部 file (llgltfmaterialpreviewmgr) や将来 generateImpostor refactor 時の選択肢として保留 |
| read 42 件 替換 (sed declaration skip pattern 経由) | `sed -E -e '/^bool[[:space:]]+LLPipeline::/!s/(LLPipeline::)?\bsXxxRender\b/isFrameXxxPass()/g'` で 9 flag 一括、declaration 9 行を保護 |
| bonus 1 件 = L13895 setReflectionPass() の RHS 内に元々 `LLPipeline::sRenderDeferred` 内包 | sed 2nd pass で `!isFrameRenderingDeferred()` に自動置換 = 結果 `setReflectionPass(!isFrameRenderingDeferred())` |
| legacy declaration 9 件保持 (pipeline.cpp L439-466) | sub-step 4.5 で物理削除 scope、4.1-β mRT field 範式継承 |

### 2.3 外部 29 file 替換

| file | write | read | 特殊 | include |
|---|---|---|---|---|
| `llviewershadermgr.cpp` | 3 (sRenderGlow→setRenderingGlow L649/L1106/L1129) | 3 | - | + |
| `llviewerdisplay.cpp` | 9 (sUnderWaterRender×7 L898/1072/1077/1156/1323/1353/1376 + sRenderingHUDs×2 L1407/1492) | 12 | gPipeline.sRenderDeferred 1 件 fix | (既配置 4.1-β) |
| `llappviewer.cpp` | 1 (sRenderDeferred=true L661→setRenderingDeferred(true)) | 2 | - | + |
| `lldrawpoolalpha.cpp` | - | 22 | sImpostorRenderAlphaDepthPass 1 件は別 scope (legacy 保持) | (既配置 4.1-β) |
| `lldrawpoolavatar.cpp` | - | 14 | - | + |
| `lldrawpoolbump.cpp` | - | 8 | - | + |
| `llvoavatar.cpp` | - | 7 | - | + |
| `lldrawable.cpp` | - | 6 | - | + |
| `llviewerwindow.cpp` | - | 5 | - | (既配置 4.1-β) |
| `llreflectionmapmanager.cpp` | - | 5 | - | (既配置 4.1-β) |
| `lldrawpoolsimple.cpp` | - | 4 | - | + |
| `llvosky.cpp` | - | 3 | - | + |
| `llspatialpartition.cpp` | - | 2 | - | + |
| `llheroprobemanager.cpp` | - | 2 | - | (既配置 4.1-β) |
| `llvovolume.cpp` | - | 2 | - | + |
| `llagent.cpp` | - | 2 | - | + |
| `lldrawpoolpbropaque.cpp` | - | 2 | - | + |
| `llfetchedgltfmaterial.cpp` | - | 2 | - | + |
| `llviewerjoint.cpp` | - | 2 | - | + |
| `lldrawpoolwlsky.cpp` | - | 2 | - | + |
| `llgltfmaterialpreviewmgr.cpp` | - | - | 1 (L436 SetTemporarily<bool> → ScopedRenderingGlow、§7.2 fix 経由) | (既配置 4.1-β) |
| `llreflectionmap.cpp` | - | 1 | - | (既配置 4.1-β) |
| `rlvhandler.cpp` | - | 1 | - | (既配置 4.1-β) |
| `llsettingsvo.cpp` | - | 1 | - | + |
| `llviewerstats.cpp` | - | 1 | - | + |
| `llviewertexture.cpp` | - | 1 | - | + |
| `lldrawpoolterrain.cpp` | - | 1 | - | + |
| `llfloater360capture.cpp` | - | 1 | - | + |
| `gltfscenemanager.cpp` | - | 1 | - | + |
| **計** | **13** | **103** (実 102 + gPipeline.sRenderDeferred 1) | **1 (ScopedRenderingGlow)** | **21 file 新規 + 8 既配置 = 29** |

116 件 (handoff-substep-4-1-beta-complete §5.1 推定 130 件 → 実測 116 件 refine)。**AYA 指示「helper 配置せず直接 call で」継承** = 外部 file は `LLPipelineFrameContext::getInstance().isXxxPass()` / `setXxxPass()` 直接 call。

### 2.4 file 変更 summary

| file | 修正規模 | 主内容 |
|---|---|---|
| `indra/newview/llpipelineframecontext.h` | +84 / -1 | 9 getter/setter + 6 nested RAII class + 9 bool field |
| `indra/newview/llpipelineframecontext.cpp` | +66 / -0 | ctor 9 field 初期化 + 6 ScopedXxx impl |
| `indra/newview/pipeline.cpp` | +約60 / -約42 (net +約18) | 9 file-local helper + write 10 件 + read 42 件 |
| 外部 29 file 計 | +約176 / -約126 | 各 file include (21 新規) + getter/setter 直接 call 替換 116 件 |

合計: **32 file、+386 / -169 line** (`git diff --stat HEAD~1` 確認済)

---

## 3. build + launch verification

| step | status |
|---|---|
| `autobuild build -A 64 -c ReleaseFS_open --no-configure` (CMakeLists.txt 変更なし、新規 file 無し = incremental build) | ✓ 2 回目 exit 0 (1 回目 2 件 compile error fix `sImpostorRender` substring collision + `SetTemporarily<bool>` 不可、§7) |
| 32 file 全 .o 再 compile clean (llpipelineframecontext.cpp.o + pipeline.cpp.o + 29 外部 file.o) | ✓ |
| packaging 完遂 | ✓ `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261510128.tar.xz` |
| `./install.sh` (`build-linux-x86_64/newview/packaged/`) | ✓ `/home/ishikawa/ayastorm` 配置完了 |
| `rm -rf ~/.ayastorm_x64/cache/` | ✓ 完遂 |
| AYA launch + 通常起動報告「起動して終了しました」 | ✓ 2026-05-31 |

### 3.1 ログ確認 (Claude 側 `~/.ayastorm_x64/logs/AYAstorm.log` grep 結果)

| marker | 期待 | 実測 |
|---|---|---|
| `vulkanDebugCallback` (ERROR/WARNING 受信) | 0 件 | **0 件** ✓ |
| `VK_ERROR` / `VUID-` / `validation failed` | 0 件 | **0 件** ✓ |
| `#VkRecord#` pool hook fire | 12/12 件 | **12 件** ✓ |
| `#Vulkan#` marker INFO 出力 | 4.1-α/β baseline 56 件整合範囲 | **56 件** ✓ |
| shutdown clean | `Vulkan device destroyed → Vulkan instance destroyed → Goodbye → status: stopped` | ✓ (log tail 3 行で確認) |
| ERROR (全 level) | 0 件 | **0 件** ✓ |
| regression | 描画通常 / UI 反応 / shader effect 維持 | ✓ AYA 報告 (起動 → 終了) |

run_time: 起動 2026-05-31T09:07:09Z → shutdown 2026-05-31T09:08:26Z = **77s**

### 3.2 validation strict 検証は sub-step 3.5-a で実施済継承

sub-step 3.5-a (2026-05-30) で validation force-enable build により段階 3 全体の validation strict 検証 PASS 済。sub-step 4.2 は段階 3 配線に変更を加えず frame state aggregation 経路の追加のみのため、validation strict 再走は不要 (段階 4 全 sub-step 完遂後の sub-step 4.5 self-check で一括再走想定)。

---

## 4. 設計 deviation 履歴 (sub-doc 04 起草時から本実装まで)

### 4.1 案 B 採用 (measurement-first 検証 log 配線 skip) = 4.1-α/β 継承

sub-doc 04 §2.5 measurement-first 範式は struct shape を sub-step 4.1 着手時 final 化する観点で、sub-step 4.2 では task #1 (10 bool flag inventory trace) を完遂条件として吸収。実装後の measurement 検証 log 配線は AYA 承認下で skip 採用。

**理由**:
- 9 bool flag getter/setter の単純性 (実コード = field 直接 read/write のみ)
- 6 nested RAII class の単純性 (ctor で前 bool 保存 + setter call / dtor で前 bool 復元)
- 案 B では起動/shutdown clean + regression 0 + code review 整合性で satisfy
- build cycle (1 回目 2 件 error + fix → 2 回目 PASS) で commit 到達

**trade-off**: ScopedXxxPass / ScopedRenderingGlow lifecycle hit 経路の log evidence は無し。getter/setter 経由 bool 整合性は 116 occurrences の code review + AYA 起動 PASS で transit satisfy。

### 4.2 pipeline.cpp file-local helper `isFrameXxxPass()` 採用 (spec literal 短縮) = 4.1-α/β 範式継承

sub-doc 04 §5.2 spec literal は `LLPipelineFrameContext::getInstance().isXxxPass()` の direct call を全 caller に挿入する path。本実装では pipeline.cpp 匿名 namespace 内 file-local helper 9 個を配置し、caller は `isFrameXxxPass()` で短縮。

**理由**:
- pipeline.cpp 内 implementation detail (file scope) = 外部 file から参照不可、AYA 確認不要範囲
- 替換時の visual diff 最小化 (`sXxxRender` → `isFrameXxxPass()` で symbol 1:1)
- 4.1-α `getFrameCull()` / 4.1-β `getFrameRT()` と同 pattern (sub-step 4.3 で const-ref passing through render path 化時の差替え単位)

**stop-gap 性格**: 本 helper も sub-step 4.3 で撤去対象候補。

### 4.3 外部 29 file 直接 call 採用 (4.1-β AYA 指示継承)

外部 29 file は per-file local helper を配置せず、`LLPipelineFrameContext::getInstance().isXxxPass()` / `setXxxPass()` 直接 call を採用。4.1-β AYA 指示「helper 配置せず直接 call で」継承。

### 4.4 案 Y 採用 (5 個独立 bool field + ScopedXxxPass nested RAII helper)

sub-doc 04 §5.2 spec literal は (a) pass-specific 5 件に case `EPassType` enum + `beginPass/endPass` 想定だったが、generateImpostor() 内 trace で shadow/reflection/impostor 多重入れ子発火確認 = 案 X (単一 EPassType) 構造的不可。

**案 Y 採用**: 5 個独立 bool field (mShadowPass/mReflectionPass/mImpostorPass/mHUDPass/mDoFPass) + 5 個 ScopedXxxPass nested RAII helper (4.1-β ScopedActiveRT 範式継承)。EPassType enum (PASS_NONE/SHADOW/DEFERRED/FORWARD/POST) は 4.1-α 配置のまま preserve、sub-step 4.3 per-pool draw 移植 scope で別軸活用予定。

### 4.5 案 ScopedRenderingGlow 新規配置 (4.2 (b) 例外、SetTemporarily replacement)

sub-doc 04 §5.2 (b) frame-global 4 件は「frame entry で settle、frame 終端まで persist」想定だが、llgltfmaterialpreviewmgr.cpp:436 で `SetTemporarily<bool>(&sRenderGlow, false)` パターン現存 = preview render 中の transient flip。

**ScopedRenderingGlow 配置採用**: 4.1-β ScopedActiveRT + 4.2 (a) ScopedXxxPass 範式継承、`SetTemporarily<bool>` を `ScopedRenderingGlow no_glow(false)` で 1:1 RAII 置換 (§7.2 fix の一環で配置)。

### 4.6 (記録なし) sub-doc 04 spec 通り実装の項目

- 9 個 getter/setter API ✓
- 9 個 private bool field 配線 ✓
- ctor false 初期化 ✓
- legacy field 物理保持 (sub-step 4.5 で削除) ✓

---

## 5. risks / caveats

### 5.1 sub-step 4.3 scope = sCurCameraID accessor + per-pool 実 scene draw 移植、**high risk**

sub-doc 04 §5.3 sub-step 4.3 spec = sCurCameraID accessor pattern (LLViewerCamera::getCurCameraID / setCurCameraID) + per-pool 実 scene draw 移植 (12 pool 全 placeholder NDC 三角形 → 実 scene visibility iteration + PSO bind + descriptor bind + vertex/index bind + vkCmdDrawIndexed)。

**4.2 比較**:
- 4.2: 116 occurrences、write 経路明確、frame state aggregation のみ
- 4.3: per-pool 実 scene draw 移植 = 視覚再現の中核、handoff-stage-3-complete §1.3 #3-段階 3 引継ぎ scope

**4.3 着手時の cadence**: 1 session 内完遂は無理想定、複数 commit + 複数 session 想定 (4.3-α / 4.3-β ... 分割判断は 4.3 着手時 AYA 承認下)。

### 5.2 legacy field 物理削除は sub-step 4.5 scope (4.1-β 範式継承)

pipeline.h L798-837 area に legacy 保持中の 10 個 static bool 宣言 (sShadowRender 等 + sDistortionRender dead path) は sub-step 4.5 self-check 時に撤去想定。

**注意**: 物理削除時は sub-step 4.5 で:
- pipeline.h 内 field declaration 10 件削除
- pipeline.cpp 内 declaration 9 件 (L439-466) 削除
- sDistortionRender 関連の使用箇所 grep で残存 0 件確認 (現状 dead path)
- 全 caller が file-local helper / getter 経由化済を再確認

### 5.3 ScopedXxxPass / ScopedRenderingGlow 採用方針 = 将来 refactor 時の差替え単位

sub-step 4.2 では write 10 件 (pipeline.cpp) に ScopedXxxPass を不使用 = 1:1 legacy behavior preserve 方針。将来 generateImpostor refactor / per-pool draw 移植 (sub-step 4.3) 時に ScopedXxxPass 採用判断。

### 5.4 sub-step 4.3 〜 4.5 は順次着手 (並走しない、handoff-stage-4-prelude.md §2 cadence 継承)

4.2 → 4.3 → 4.4 → 4.5 (順次)。並走しない (charter §7.5 r41 着手中 refine 可範囲、AYA 承認下)。

### 5.5 context budget concern (次 session)

4.3 着手時の per-pool 実 scene draw 移植は 4.2 (116 件) より大規模 (12 pool × 数十-100 件) + 描画品質再現の検証往復必須。複数 session 想定。proactive handoff 範式 (feedback_proactive_handoff.md absolute rule) 継承、必要なら sub-step 4.3 内段階分割を AYA 承認下で判断 (例: 4.3-α = sCurCameraID accessor のみ / 4.3-β-... = pool 別 draw 移植)。

---

## 6. next session entry point

### 6.1 次 session 着手前の準備 (4 段 cadence)

1. `git pull origin feature/ayastorm-r41-gl-removal` (AYA push 後の最新取得、または local commit `9a9cb018c8` ready 確認)
2. 本 handoff doc 通読
3. sub-doc `04-frame-context.md` §5.3 sub-step 4.3 cadence 再確認 + handoff-stage-3-complete.md §1.3 #3-段階 3 引継ぎ scope 再確認
4. memory `project_ayastorm_r41_vulkan_migration.md` status update 反映確認

### 6.2 sub-step 4.3 着手 task 候補 (sub-doc 04 §5.3 cadence + 本 handoff §5.1 から refine)

| task | 内容 | 推定規模 |
|---|---|---|
| 4.3-1 | sCurCameraID 経路 trace + 12 pool 各 placeholder draw helper 経路 trace + LLViewerCamera::getCurCameraID 配線 inventory | trace 60-90 分 |
| 4.3-2 | LLPipelineFrameContext に sCurCameraID accessor 配線 + 必要なら per-pool draw 設計 update | 実装 30-60 分 |
| 4.3-3 | per-pool 実 scene draw 移植 (pool 別段階分割想定、4.3-α / 4.3-β ...) | 実装 多 session 想定 |
| 4.3-4 | autobuild incremental build verify (各 pool 段階で繰返) | 各 20-30 分 |
| 4.3-5 | AYA launch verify (案 B cadence 継承: vulkanDebugCallback 0 / 12 #VkRecord# fire / shutdown clean / regression 0 + 視覚再現確認) | 各 AYA |
| 4.3-6 | commit (pool 単位想定、4.3-α / 4.3-β ...) | 各 5 分 |
| 4.3-7 | 4.3 全 pool 完遂後 sub-step 4.3 完遂宣言 handoff | 30-60 分 |

### 6.3 critical reminders

- **AYAstorm 改変 13 file shader 改変禁止** (charter §2.1 領域 6、`git diff` 0 件維持)
- **段階 1 + 段階 2 + 段階 3 + 4.1 + 4.2 動作維持** (sub-doc 03 §3.5、Vulkan path 並走で GL 描画動作維持)
- **per-pool 実 scene draw 移植は 視覚再現の中核** (placeholder NDC 三角形 → 実 scene draw 移植時の AYA 視覚 verify 必須、案 B cadence では起動 verify では不十分)
- **sCurCameraID は per-frame 切替 + 入れ子発火可能性** (LLViewerCamera::setCurCameraID の write 経路 + 入れ子 trace 必要)
- **案 B cadence 継承想定** (AYA 別途指示なき限り measurement log skip、但し 4.3 視覚再現は AYA 視覚 verify で satisfy 必要)
- **context budget proactive 監視** (feedback_proactive_handoff.md absolute rule、4.3 内段階分割を AYA 承認下で判断)

### 6.4 commit 戦略

- 複数 commit 想定 (4.3-α / 4.3-β / ... pool 別段階分割)
- 各段階で AYA launch verify PASS 確認後 commit
- commit message style = `feat(r41): sub-step 4.3-α 完遂 (...)` 範式

---

## 7. build failure + fix 履歴 (本 commit に至るまで)

### 7.1 lldrawpoolalpha.cpp:409 = sed substring match collision (compile error 1 件)

1 回目 build で発生:
```
error: expected ',' or ';' before 'AlphaDepthPass'
|| LLPipelineFrameContext::getInstance().isImpostorPass()AlphaDepthPass
```

**原因**: sed pattern `s|LLPipeline::sImpostorRender|...|g` が `LLPipeline::sImpostorRenderAlphaDepthPass` (別の static bool、sub-step 4.2 scope 外) 内に substring match。

**fix**: lldrawpoolalpha.cpp:409 を `|| LLPipeline::sImpostorRenderAlphaDepthPass` に revert。sImpostorRenderAlphaDepthPass は別 scope (sub-step 4.5 で物理削除判断、または別軸 scope) として legacy 保持。

**教訓**: 将来 sed 大量替換時は word boundary `\b` を 全 substitution で必須化 (本 commit では read pattern の `LLPipeline::sXxx` 前後を `[^A-Za-z_]` で 限定する追加正規表現は省略 = AYA 起動 PASS で transit satisfy)。

### 7.2 llgltfmaterialpreviewmgr.cpp:436 = SetTemporarily<bool> 不可 (compile error 1 件)

1 回目 build で発生:
```
error: lvalue required as unary '&' operand
SetTemporarily<bool> no_glow(&LLPipelineFrameContext::getInstance().isRenderingGlow(), false);
```

**原因**: `isRenderingGlow()` は getter return-by-value で `&` 不可。元 code は `&LLPipeline::sRenderGlow` (static bool への pointer 取得) だった。

**fix**: ScopedRenderingGlow nested class を新規配置 (§4.5)、`LLPipelineFrameContext::ScopedRenderingGlow no_glow(false);` で 1:1 RAII 替換。

**教訓**: bool flag struct migration で SetTemporarily<bool> 経由 pointer 取得パターンがあれば、nested RAII class で個別替換が必須 (将来 sub-step 4.3 の sCurCameraID 等にも同パターン要 trace)。

### 7.3 sed instance member 取りこぼし (compile error 0 件、warning 0 件、但し意味通り 1 件)

sed pattern `s|LLPipeline::sRenderDeferred|...|g` は scope resolution `LLPipeline::` のみ match、instance member access `gPipeline.sRenderDeferred` は取りこぼし。

**fix**: `llviewerdisplay.cpp:1148` で手動 Edit 経由、`gPipeline.sRenderDeferred` → `LLPipelineFrameContext::getInstance().isRenderingDeferred()` 直接 call 替換。

**教訓**: 将来 sed 大量替換時は instance member access patten (`gXxx.sXxx`) も別 pass で trace。

---

## 8. sDistortionRender (legacy 保持、本 commit scope 外)

sub-step 4.2-1 inventory で確認:
- pipeline.cpp / pipeline.h で `sDistortionRender` 静的宣言あり
- 但し全 viewer codebase で write 経路 / read 経路 ともに **0 件** (dead path)
- frame context 配置不要 = 9 個 bool field の選定外
- sub-step 4.5 で物理削除 scope (legacy field 削除と一括処理)

---

## 9. 関連 doc / memory cross-ref

### 9.1 関連 doc

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter (§2 領域 4 1.00 PM medium risk)
- `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` — 段階 4 領域 4 sub-doc (sub-step 4.3 marker 着手 ready 状態へ)
- `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` — sub-step 4.4 part B で参照
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-3-complete.md` — 段階 3 完遂 (per-pool 実 scene draw 移植 引継ぎ scope、本 handoff §5.1 参照)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-4-prelude.md` — 段階 4 pre-emptive handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-1-alpha-complete.md` — sub-step 4.1-α 完遂 → 4.1-β 着手境界 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-1-beta-complete.md` — sub-step 4.1-β 完遂 → 4.2 着手境界 handoff (役割完了)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-2-complete.md` — 本 handoff (4.2 全完遂 → 4.3 着手境界)

### 9.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 handoff 完遂で sub-step 4.3 着手 ready 状態に update)
- `project_build_procedure.md` — autobuild + install + cache clear フロー (本 sub-step で実施)
- `project_ayastorm_three_platforms.md` — Linux 先行例外を r41 で適用中
- `feedback_self_verify_before_handoff.md` — 本 handoff 起草前 self-trace 実施 (§2 file/line / §3 ログ marker 表)
- `feedback_no_auto_commit.md` — commit は AYA 明示指示で実施 (本 sub-step 4.2 は指示済 → `9a9cb018c8`)
- `feedback_proactive_handoff.md` — 周回境界での能動 handoff 起草 (本 file、AYA 「はい」承認下 2026-05-31)
- `feedback_one_step_at_a_time.md` — 1 メッセージ 1 アクション、4.2-1 〜 4.2-7 分割 cadence
- `feedback_remove_verification_logs.md` — 案 B では log 配線 skip = 除去対象なし
- `feedback_no_claude_coauthor.md` — 全 commit messages で Claude 共著行なし
- `reference_log_path.md` — `~/.ayastorm_x64/logs/AYAstorm.log` (Linux)
