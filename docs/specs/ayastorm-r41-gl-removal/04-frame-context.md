# AYAstorm r41 sub-doc 04-frame-context — 段階 4 pipeline.cpp frame state 集約 + LLGLState RAII dead-store 化

**status**: **draft 2026-05-31 (Pattern α 一括 draft、charter §7.4 標準範式、AYA review 待ち)**
**親 charter**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md` (closed 2026-05-28)
**前 sub-doc**: `03-state-machine-pso.md` (closed 2026-05-31、段階 3 完遂で役割完了)
**前 handoff**: `handoff-stage-3-complete.md` (段階 3 完遂 → 段階 4 着手境界)
**達成条件**: 段階 4 完遂 = pipeline.cpp 3 大グローバル + 周辺 frame state を `LLPipelineFrameContext` struct に集約 + 12 件 LLGLState RAII state class setter 内 GL call 物理削除 (dead-store 化、caller source-level compat 維持) + 段階 3 引継ぎ per-pool 実 scene draw 移植を frame context 経由で frame state 集約後に配置 + 領域 8 LLVKRenderer skeleton declaration 物理配置 (charter §3 #6 acceptance 段階 4 内 satisfy) + validation 0 件
**関連 charter section**: §2 領域 4 (段階 4 pipeline.cpp 3 大グローバル → frame context、1.00 PM 高 risk) + §3 #1 acceptance (RAII setter dead-store 化 = bridging item #1、段階 3 → 段階 4 scope refine 2026-05-29) + §3 #6 acceptance (LLVKRenderer skeleton declaration 物理配置、領域 8 並走) + §7.4 sub-doc 構成 + §7.5 boundary

---

## §1 段階 4 scope plan

### §1.1 段階 4 scope 再掲 (charter §2 領域 4 + handoff-stage-3-complete §2.1 反映)

- **対象**: `indra/newview/pipeline.cpp` + `pipeline.h` 内 3 大グローバル (`sCull` / `sShadowRender` / `sCurCameraID`) + 周辺 frame state 17 件を `LLPipelineFrameContext` struct に集約 + `indra/llrender/llgl.{cpp,h}` + `llglstates.h` 12 件 RAII state class (LLGLState ファミリ) setter 内 GL call 物理削除 (dead-store 化) + 段階 3 引継ぎ 3 件
- **境界条件 (charter §2 領域 4)**: 領域 5 (依存解決) 着手前に LLPipelineFrameContext が render path 全体で機能 + 12 件 RAII state class setter 内 GL call 物理削除完遂 + caller source-level compat 維持
- **依存順序 (charter §2 領域 4)**: 段階 3 完遂後着手 (本 sub-doc で boundary 確定) + 領域 8 (LLVKRenderer skeleton declaration 物理配置) と並走
- **risk 性質 (charter §2 領域 4)**: **高** — charter §2 高 risk 2 領域の 1 つ、a-3 §B sub-phase 1 で「pipeline.cpp 3 大グローバル + cull/stateSort 内 GL 呼出が並列化阻止」確認済の領域、charter §6.4 「描画再構築 phase」、余裕係数 +37% の主要因の 1 つ
- **段階 3 引継ぎ (handoff-stage-3-complete §1.3-1.4 で AYA 承認済 2026-05-31)**:
  - **per-pool 実 scene draw 移植** (12 pool 全 placeholder NDC 三角形描き → 実 scene draw 移植) を段階 4 LLPipelineFrameContext 配置後 frame state 集約経路で配置、段階 4 acceptance に併合
  - **acceptance #1-段階 3 RAII setter dead-store 化** (bridging item #1) を段階 4 内 satisfy (charter §2 領域 4 境界条件、sub-doc 03 §1.5.3 #1 反映)
  - **acceptance #6-段階 3 LLVKRenderer skeleton declaration 物理配置** を領域 8 sub-doc (`08-llvkrenderer-skeleton.md` 仮称) で段階 4 並走起草 + 物理配置で satisfy

### §1.2 段階 3 引継ぎ 3 件の satisfy 計画 (段階 4 sub-step マッピング)

handoff-stage-3-complete §1.3-1.4 で確定した段階 4 一体運用項目を sub-step マッピング:

| 段階 3 持越し項目 | 段階 4 sub-step での satisfy 経路 |
|---|---|
| **per-pool 実 scene draw 移植** (12 pool 全 placeholder helper 経由 `vkCmdDraw(3,1,0,0)` fullscreen NDC 三角形描き = 視覚 no-op 等価 placeholder) | **sub-step 4.3 (標準) で frame context 経由 frame state 集約後**、各 pool の `recordPoolDraws(VkCommandBuffer, LLPipelineFrameContext const&)` hook body 内に **実 scene visibility iteration + per-draw PSO bind + descriptor set bind + vertex/index buffer bind + `vkCmdDrawIndexed` 投入** を配線。**完了 marker**: 12 pool 全 hook で実 scene 描画動作 + AYA launch 時 placeholder 三角形消失 + scene 描画 (黒画面ベース、charter §1 thesis (2) vk-α 空転 baseline) |
| **acceptance #1-段階 3 RAII setter dead-store 化** (bridging item #1) | **sub-step 4.4 (特殊) で 12 件 RAII state class setter 内 GL call 物理削除**、PSO state alias 基盤 (sub-step 3.1b 配置) + LLPipelineFrameContext (sub-step 4.1 配置) で render path context 確定後に caller source-level compat 維持しつつ実施。**完了 marker**: `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/llrender/llgl.cpp indra/llrender/llglstates.h` が **wrapper 内部以外で 0 件 hit** + viewer 全 ~250-280 caller (Agent B 実測、handoff §5.3 推定 "850+" は 実測で refine、本 §4.3 参照) source-level compat 維持 |
| **acceptance #6-段階 3 LLVKRenderer skeleton declaration 物理配置** | **sub-step 4.4 (特殊) で領域 8 sub-doc `08-llvkrenderer-skeleton.md` (仮称) 並走起草 + skeleton declaration 物理配置**、charter §3 #6 acceptance 段階 4 内 satisfy。**完了 marker**: `grep -rE "class LLVKRenderer" indra/` で skeleton declaration 存在 + 05 doc §10.1-§10.2 hook site placeholder 配置 + pipeline.cpp inline 実装と signature 整合 |

### §1.3 並走領域との関係 (charter §2 領域 4 依存順序)

| 並走領域 | 段階 4 内での協調事項 |
|---|---|
| **領域 3 (llrender PSO 化、段階 3 完遂済)** | 段階 3 で確定した PSO state alias 基盤 + matrix stack 二段構え (push constant 64 B + per-frame UBO 2 binding) + dynamic rendering 並走基盤 + 3 階層 descriptor set + 12 pool record hook を **LLPipelineFrameContext 経由 frame state 受渡し前提に再配線**、source-level compat 維持 |
| **領域 5 (依存解決、段階 5 scope)** | 段階 4 完遂後着手、bridging item #1 dead-store 完遂後に llspatialpartition / llviewershadermgr / llvertexbuffer / llvosky / llvowlsky の残依存 file PSO 化 + PFNGL function pointer declarations 物理削除 (§1.5.2 Cluster D 189 file 波及、段階 5 一体運用) |
| **領域 6 (shader SPIR-V、active 継続)** | sub-step 6.1 (autobuild integration) は段階 4 並走で本格着手判断 (AYA 擦り合わせ)、frame context 経由 frame state 受渡しは shader 側 binding に影響無し |
| **領域 7 (descriptor / render pass、active 継続)** | sub-step 7.3 (material cache 本実装) / 7.4 (push descriptor 全配線) / 7.5 (実 attachment 配線) は段階 4-5 並走、frame context 経由 frame state 受渡しと協調 (set=0 PerFrame UBO 更新が frame context 経由) |
| **領域 8 (LLVKRenderer skeleton、本 sub-doc と並走起草判断)** | charter §3 #6 acceptance: pipeline.cpp inline 実装と skeleton hook の **signature 整合**、段階 3 完遂時 signature 確定済 (handoff §1.3 #6-段階 3)、本段階 4 内で skeleton declaration 物理配置で satisfy、領域 8 sub-doc 並走起草タイミングは sub-doc 04 完遂後に AYA 擦り合わせ (handoff §5.1 cadence 4. + 2026-05-31 AYA 推奨 = sub-doc 04 完遂後判断) |

### §1.4 sub-doc 04 起草 cadence (Pattern α 一括 draft → AYA review boundary)

- **起草 pattern**: Pattern α 一括 draft (charter §7.4 標準範式)、sub-doc 03 (closed 2026-05-31) 範式継承
- **起草 deliverable** (handoff §2.2):
  - §1 段階 4 scope plan (charter §2 領域 4 + 段階 3 引継ぎ反映)
  - §2 pipeline.cpp 3 大グローバル + 周辺 frame state inventory (Agent 並列 trace 反映)
  - §3 LLPipelineFrameContext struct 設計 + caller migration 順序
  - §4 12 件 LLGLState RAII state class setter 内 GL call dead-store 化計画 (Agent 並列 trace 反映)
  - §5 sub-step list (4.1 〜 4.5、段階 3 範式継承)
  - §6 段階 4 completion criteria (acceptance #1 + #6 段階 4 内 satisfy 計画含む)
  - §7 関連 doc / memory
- **AYA review boundary**: 本 draft 完成後 AYA review、PASS 後に sub-step 4.1 着手
- **段階 4 sub-step cadence**: 段階 3 5 sub-step (3.1a/3.1b/3.2/3.3/3.4/3.5) 範式継承想定、charter §7.5 で boundary refine 可

---

## §2 pipeline.cpp 3 大グローバル + 周辺 frame state inventory (Agent 並列 trace sealed 2026-05-31)

本 §2 は段階 4 着手前 trace-before-implement (`feedback_render_full_trace_first.md` 遵守) として Agent 並列 trace で取得した frame state inventory を sealed 形で保全。再 trace 不要。

### §2.1 3 大グローバル confirmation (charter §2 領域 4 + handoff §2.1 反映)

| # | 変数 | file:line | 型 | 用途 | write/read cadence per frame |
|---|---|---|---|---|---|
| 1 | **sCull** | `indra/newview/pipeline.cpp:477` | `static LLCullResult*` | 当該 frame の cull result への transient pointer (visible groups / drawables / faces 保持)、grabReferences で集約 → 当該 frame 内描画 path 全体で read | write 2 件 (`grabReferences` / `clearReferences`) / read 72 件 (occlusion / renderGeom / drawpool iteration 等) |
| 2 | **sShadowRender** | `indra/newview/pipeline.cpp:438` (+ `pipeline.h:798` LLPipeline class member) | `static bool` | shadow map 生成中 true / 主 scene 描画中 false の render-pass flag、shadow path / 主 path 分岐の主要 guard | write 6 件以上 / read 12 件 (条件 path 内 guard、occlusion / beacon / post-process skip 判定) |
| 3 | **sCurCameraID** | `indra/newview/llviewercamera.h:61` (LLViewerCamera class member、**別 class**) | `static eCameraID` | 当該 frame で active な camera (CAMERA_WORLD / CAMERA_SHADOW / etc.) 識別 enum、occlusion / visibility test の context 切替に使用 | write/reassign 14 件 / read 14 件 (occlusion / visibility guard / impostor 判定) |

**特記** (§2.4 migration risk と整合):

- **sCull / sShadowRender** は `pipeline.cpp` + `pipeline.h` 内 static / member、LLPipelineFrameContext 集約 target として **直接置換可** (low/medium risk)
- **sCurCameraID** は **`LLViewerCamera` 別 class の static member** = cross-class refactor、LLPipelineFrameContext への直接吸収困難、**accessor 経由 (LLViewerCamera::getCurCameraID() / LLViewerCamera::setCurCameraID(eCameraID))** で frame context との binding を保つのが妥当 (§2.4 で詳述)

### §2.2 周辺 frame state inventory (17 件、Agent A 並列 trace 結果)

| # | 変数 | file:line | 型 | 用途 | write/read cadence |
|---|---|---|---|---|---|
| 4 | sVisibleLightCount | pipeline.cpp:461 | `S32` | 当該 frame で visible な light 数 (ProbeManager / deferred lighting 用) | write ~N (light cull 中増分) / read 4 件 |
| 5 | sRenderGlow | pipeline.cpp:439 | `bool` | glow / bloom post-process 制御 per-frame flag | read ~15 件 (条件 path guard) |
| 6 | sRenderDeferred | pipeline.cpp:458 | `bool` | deferred shading enable per-frame flag | read ~10 件 |
| 7 | sReflectionRender | pipeline.cpp:440 | `bool` | reflection probe 描画中 flag | read ~8 件 (probe-specific path guard) |
| 8 | sDistortionRender | pipeline.cpp:441 | `bool` | distortion pass (water/glass) 描画中 flag | read ~5 件 |
| 9 | sImpostorRender | pipeline.cpp:442 | `bool` | impostor / jelly-doll avatar 描画中 flag | read ~6 件 |
| 10 | sUnderWaterRender | pipeline.cpp:449 | `bool` | camera が water 下 = clipping / fog 影響 flag | read ~5 件 |
| 11 | sReflectionProbesEnabled | pipeline.cpp:459 | `bool` | reflection probe 利用可 frame condition | read ~6 件 (occlusion / SSR guard) |
| 12 | sRenderingHUDs | pipeline.cpp:462 | `bool` | HUD render pass 中 flag | read query 時のみ |
| 13 | sLastFocusPoint | pipeline.cpp:464 | `LLVector3` | depth-of-field focus point per-frame cache | write 1 件 / read DOF shader 経由 |
| 14 | sDoFEnabled | pipeline.cpp:465 | `bool` | DOF post-process active per-frame flag | read ~3 件 |
| 15 | mNumVisibleNodes | pipeline.cpp:527 (LLPipeline ctor) | `S32` (member) | frame 内 cull node 数カウンタ | write 1 件 (line 3134) / read stats |
| 16 | mNumVisibleFaces | pipeline.cpp:527 (LLPipeline ctor) | `S32` (member) | frame 内 visible face 数カウンタ | write 1 件 (line 4229) / read stats |
| 17 | sCompiles | pipeline.cpp:404 | `S32` | transient compile-request counter per frame | frame start reset (line 2528) / shader update 中 increment |
| 18 | sIndicesDrawnCount | pipeline.cpp:5457 | `static U32` | profiling 用 draw call index 数カウンタ | write 1 件 / read stats |
| 19 | mRT | pipeline.h:871 (LLPipeline member) | `RenderTargetPack*` | active render target pack (main/aux/hero) pointer、render-pass context 切替 | write 不定 (pass context 切替) / read ~100 件以上 |
| 20 | (将来 candidate) | TBD | 段階 4 着手中追加候補 | sub-step 4.1 着手時に最終確定 (measurement-first) | — |

### §2.3 cross-cutting file dependencies (Agent A 並列 trace 結果)

3 大グローバル + 周辺 frame state を直接参照する file (file-level、line-level 詳細は段階 4 着手時 sub-step 4.1 で確定):

| file | 参照変数 | 用途 |
|---|---|---|
| `indra/newview/lldrawable.cpp` | sCull | spatial partition occlusion check |
| `indra/newview/llspatialpartition.cpp` | sCurCameraID | visibility test (camera context) |
| `indra/newview/lldrawpoolterrain.cpp` | sShadowRender | shadow pass 中 draw skip 判定 |
| `indra/newview/llviewerdisplay.cpp` | sShadowRender / sRenderGlow / mRT | frame flow orchestration (render-pass state 切替) |
| `indra/newview/gltfscenemanager.cpp` | sReflectionProbesEnabled | branching guard |
| `indra/newview/llvoavatar.cpp` | sCurCameraID | impostor visibility 判定 |
| `indra/newview/llvieweroctree.cpp` | sCurCameraID | occlusion state query |
| `indra/newview/llvocache.cpp` | sCull (spatial partition 経由) | visibility culling |

### §2.4 migration risk flags (Agent A 並列 trace 結果)

| # | 変数 | risk | 移行方針 |
|---|---|---|---|
| sCull | **low** | 常に `grabReferences` で frame 境界初期化、startup dependency 無し → LLPipelineFrameContext 直接吸収 |
| sShadowRender / sRenderGlow / sReflectionRender / sDistortionRender / sImpostorRender / sUnderWaterRender / sReflectionProbesEnabled / sRenderingHUDs / sDoFEnabled | **medium** | render-pass state machine (shadow → deferred → post-process) 内 set/unset、frame context 内 active pass enum + pass entry/exit helper で吸収 |
| **sCurCameraID** | **high** | LLViewerCamera 別 class static member、cross-class refactor 不可避。**LLPipelineFrameContext から accessor 経由 (LLViewerCamera::getCurCameraID() / setCurCameraID())** で binding 維持、spatial partition / visibility test API は無改修で動作維持 |
| mRT / mMainRT / mAuxillaryRT | **low** | LLPipeline member、frame context (or LLPipeline 内 PassContext sub-struct) に直接吸収可 |
| sVisibleLightCount / sLastFocusPoint | **low** | per-frame accumulator、frame context 直接吸収可 |
| **thread-safety** | **問題無し** | 全変数 main render thread 単独 access (非 render thread 経由 access 検出 0 件、Agent A trace 確認済) |

### §2.5 redesign-first 回避 (sub-doc 03 §1.5.4 measurement-first 範式継承)

sub-step 4.1 着手時に **`LLPipeline` ctor + frame entry/exit helper** 内で以下を log 出力 + verify:

- frame context size (sub-step 4.1 で確定する struct fields の bytes 合計)
- per-frame write 頻度 (Agent A inventory cadence の実測 verify)
- caller migration 順序の low → high risk 通過

3 driver baseline (NVIDIA RTX 5090 / Mesa RADV / Mesa ANV) は段階 4 内では NVIDIA RTX 5090 のみ Linux first-class baseline 厳守 (`project_ayastorm_three_platforms.md` Linux 先行例外、Mesa RADV / ANV は段階 10 polish 据置)。

**redesign-first 採用回避の理由**: Agent A inventory が「3 大グローバル + 17 件 周辺 frame state + 8 file cross-cutting」と粒度提示時、実際の sub-step 4.1 着手前に struct shape を speculation で確定せず、**measurement-first** で frame entry/exit helper 配置 → frame context size + write 頻度 実測 → sub-step 4.1 完了 marker で struct shape final 化 (§3.1 sub-step 4.1 marker 反映)。

---

## §3 LLPipelineFrameContext struct 設計 + caller migration 順序

### §3.1 struct lifecycle 設計 (frame 開始 → render pass 内 read-only 共有 → frame 終了 teardown)

- **frame 開始 (frame entry)**: `LLPipeline::beginFrameContext()` で `LLPipelineFrameContext` instance を stack 上構築 (heap alloc 不要、frame 単位 lifetime)、grabReferences 経由 cull result 取得 + per-frame state (sRenderGlow / sRenderDeferred / etc.) 初期化
- **render pass 内 (frame body)**: pass 切替時に `LLPipelineFrameContext& context` を 12 pool record hook + render path helper に **read-only 共有** (const-ref 受渡し)、pass-specific state (sShadowRender / sReflectionRender / etc.) は `context.beginPass(PassType)` / `context.endPass()` で transient 切替
- **frame 終了 (frame exit)**: `LLPipeline::endFrameContext()` で context teardown + cull result 解放 + per-frame state reset
- **thread model**: main render thread 単独 access (§2.4 thread-safety 確認済)、frame context は stack 上 instance なので thread crossing 不発生

### §3.2 struct fields 案 (sub-step 4.1 着手時 measurement-first で final 化)

```cpp
// indra/newview/llpipelineframecontext.h (新規、sub-step 4.1 配置)
class LLPipelineFrameContext
{
public:
    // §3.1 lifecycle
    LLPipelineFrameContext();  // frame entry
    ~LLPipelineFrameContext(); // frame exit

    // §3.1 pass transition
    void beginPass(EPassType pass_type);
    void endPass();

    // §2.1 3 大グローバル 集約 (sCull は frame context 内 owned、sShadowRender は pass-specific transient flag に統合、sCurCameraID は accessor 経由 binding)
    LLCullResult const& getCullResult() const;        // sCull 代替
    bool isShadowPass() const;                         // sShadowRender 代替
    LLViewerCamera::eCameraID getCurCameraID() const;  // accessor 経由 (§2.4 high risk 対応)

    // §2.2 周辺 frame state 集約 (low/medium risk 17 件)
    bool isRenderingGlow() const;        // sRenderGlow
    bool isRenderingDeferred() const;    // sRenderDeferred
    bool isRenderingReflection() const;  // sReflectionRender
    bool isRenderingDistortion() const;  // sDistortionRender
    bool isRenderingImpostor() const;    // sImpostorRender
    bool isUnderWaterRendering() const;  // sUnderWaterRender
    bool isReflectionProbesEnabled() const; // sReflectionProbesEnabled
    bool isRenderingHUDs() const;        // sRenderingHUDs
    bool isDoFEnabled() const;           // sDoFEnabled
    LLVector3 const& getLastFocusPoint() const; // sLastFocusPoint
    S32 getVisibleLightCount() const;    // sVisibleLightCount
    RenderTargetPack* getActiveRT() const; // mRT

    // §3.1 frame stats
    S32 getNumVisibleNodes() const;      // mNumVisibleNodes
    S32 getNumVisibleFaces() const;      // mNumVisibleFaces

private:
    LLCullResult mCullResult;
    EPassType mCurrentPass;
    // ... (sub-step 4.1 measurement-first で確定する fields)
};
```

**特記**:

- **struct shape は sub-step 4.1 着手時 measurement-first で final 化** (§2.5 redesign-first 回避)、本 §3.2 は **方向性 outline** で確定値ではない
- **bytes 合計**: 推定 ~200-300 bytes (frame stats + per-frame state + cull result pointer)、stack 上構築前提で size 制約は無し
- **getter のみ (setter は internal)**: read-only 共有原則、setter は `beginPass` / `endPass` 経由のみ、誤書込を構造的に防止
- **sCurCameraID accessor 経由**: `LLPipelineFrameContext::getCurCameraID()` は `LLViewerCamera::getInstance()->getCurCameraID()` を内部 forward、cross-class refactor を accessor 1 段で吸収

### §3.3 caller migration 順序 (low risk → high risk)

| 順序 | sub-step | 対象 | risk | 完了 marker |
|---|---|---|---|---|
| 1 | 4.1 (PSO 基盤相当) | LLPipelineFrameContext struct 配置 + frame entry/exit helper 配線 + sCull / mRT 移行 | **low** | struct 配置 + frame context 経由 cull result + mRT access が動作、AYA launch baseline 維持 |
| 2 | 4.2 (軽量) | sShadowRender + sRenderGlow + sRenderDeferred + sReflectionRender + sDistortionRender + sImpostorRender + sUnderWaterRender + sReflectionProbesEnabled + sRenderingHUDs + sDoFEnabled (10 件 bool フラグ) | **medium** | 10 件全 bool フラグの frame context 経由 access 動作、render-pass state machine 動作維持、validation strict 動作中 violation 0 件 |
| 3 | 4.3 (標準) | sCurCameraID accessor 化 + sVisibleLightCount / sLastFocusPoint / sCompiles / sIndicesDrawnCount / mNumVisibleNodes / mNumVisibleFaces 移行 + **per-pool 実 scene draw 移植 (段階 3 引継ぎ、12 pool 全 hook body)** | **high** | sCurCameraID accessor 経由 spatial partition + visibility test 動作維持、12 pool 全 hook 内実 scene 描画動作、AYA launch 時 placeholder 三角形消失 + scene 描画 (vk-α 空転 baseline) |
| 4 | 4.4 (特殊) | 12 件 LLGLState RAII state class setter dead-store 化 (本 sub-doc §4) + 領域 8 LLVKRenderer skeleton declaration 物理配置 | **high** | `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/llrender/llgl.cpp indra/llrender/llglstates.h` が wrapper 内部以外で 0 件 + ~250-280 caller source-level compat 維持 + LLVKRenderer skeleton declaration 存在 + signature 整合 |
| 5 | 4.5 (self-check) | 段階 4 self-check + validation strict 検証 + handoff doc 起草 | — | sub-doc 03 §3.1 sub-step 3.5 範式継承、`handoff-stage-4-complete.md` 起草 |

### §3.4 per-pool 実 scene draw 移植経路 (段階 3 引継ぎ、sub-step 4.3 配線)

段階 3 完遂時 12 pool record hook 内は共用 `recordPlaceholderPoolDraw` helper (avatar のみ `recordAvatarPlaceholderDraw`) 経由 fullscreen NDC 三角形描き = 視覚 no-op 等価 placeholder で `vkCmdDraw(3,1,0,0)` 動作 + validation 0 件 satisfy (handoff §1.3 #3-段階 3)。

段階 4 sub-step 4.3 で frame state 集約後に以下を配線:

1. **signature 変更**: `recordPoolDraws(VkCommandBuffer cmd_buf)` → `recordPoolDraws(VkCommandBuffer cmd_buf, LLPipelineFrameContext const& context)`、12 pool hook 全 同時更新
2. **scene visibility iteration**: `context.getCullResult().getDrawables()` 経由で当該 pool の visible drawables を iterate
3. **per-draw PSO bind**: 各 drawable の material 別 PSO を bind (sub-step 3.3 で配置済の per-pool PSO + material-specific override)
4. **descriptor set bind**: set=0 PerFrame UBO (frame context 経由更新)、set=1 PerMaterial 7 PBR slot (material cache 経由、領域 7 sub-step 7.3 残置範囲含む)、set=2 PerDraw (push descriptor、領域 7 sub-step 7.4 残置範囲含む)
5. **vertex/index buffer bind**: `vkCmdBindVertexBuffers` + `vkCmdBindIndexBuffer` (drawable の vertex buffer cache 経由、領域 5 llvertexbuffer Vulkan 化と協調)
6. **draw call**: `vkCmdDrawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance)`

**完了 marker** (sub-step 4.3 集約 = 4.3-ε' + 4.3-ζ' 完遂時): AYA launch 時 placeholder fullscreen 三角形消失 + scene 描画 (黒画面ベース、vk-α 空転 baseline)、validation strict 動作中 violation 0 件、12/12 pool 全 hook 内実 scene 描画動作、段階 3 範式継承 (one-shot INFO marker = 「Pool N real scene draw fired (frame=M)」風)。

**(refine 2026-05-31、handoff-substep-4-3-beta-prep.md §4.2 反映)**: 旧 §3.4 spec は sub-step 4.3-β/γ/δ/ε で 4 commit 分割 per-pool 移植を想定していたが、4.3-α 完遂後の 3-agent 並列 trace で **infrastructure 前提誤り発覚** = 上記 1-6 配線は領域 5 (LLVertexBuffer Vulkan 化) + 領域 6 sub-step 6.1 (WLSky 4 shader + WaterExclusion 等 SPIR-V port) + 領域 7 sub-step 7.3/7.4/7.5 (material cache + push descriptor + 実 attachment 配線) 着手前提だが未着手。案 D 採用 (AYA 「D お願いします」承認 2026-05-31) で新 cadence 再設計:

| 新 sub-step | scope | infrastructure 前提整備 |
|---|---|---|
| 4.3-β-prep ✓ | scope refine 提案 doc (AYA review PASS 2026-05-31) | — |
| **4.3-β'** | 領域 5 LLVertexBuffer Vulkan 化 (段階 5 部分前出し) | (VkBuffer + bind helper + `vkCmdBindVertexBuffers` / `vkCmdBindIndexBuffer` caller 配置) |
| **4.3-γ'** | 領域 6 sub-step 6.1 本格着手 (autobuild integration + base port ~228 file SPIR-V port) | (旧「sub-step 6.1 本格着手 = 段階 4 sub-step 4.5 完遂後」縛り解除、AYA 承認 update) |
| **4.3-δ'** | 領域 7 sub-step 7.3/7.4/7.5 本格化 (material cache + push descriptor 全配線 + 実 attachment 配線) | (旧「段階 4-5 並走判断保留」→ 段階 4 内本格化、AYA 承認 update) |
| **4.3-ε'** | Sky+WLSky+WaterExclusion + 9 pool 一括 per-pool 実 scene draw 移植 (旧 β+γ 集約、上記 1-6 配線を 12 pool 一括適用) | 4.3-β' + γ' + δ' 完遂前提で機械的 1:1 GL → Vk 替換 |
| **4.3-ζ'** | Avatar bone per-draw + GLTFPBR per-draw 移植 (旧 δ+ε 集約) | 4.3-β' + γ' + δ' 完遂前提で機械的 1:1 GL → Vk 替換 |
| **4.3-η'** | self-check + handoff (旧 ζ 範式継承) | — |

**partial 配線許容 (再評価 2026-05-31)**: 旧 spec の「partial 配線許容 = 領域 7 sub-step 7.3/7.4/7.5 が partial state でも placeholder material + placeholder descriptor で動作確認可」literal は valid だが、実 scene draw 移植が視覚効果を持つには領域 5+6+7 整備が前提 (Sky=stub iterate 0 件、WLSky=shader 未 port で bind 不可、WaterExclusion=gDrawColorProgram 未 port で bind 不可)。新 cadence では 4.3-δ' まで infrastructure を整備した後 4.3-ε' / 4.3-ζ' で機械的移植する設計に refine、partial 配線注記は 4.3-δ' 完遂前のみ適用 (handoff-substep-4-3-beta-prep.md §6.5 case-validity 担保)。

---

## §4 12 件 LLGLState RAII state class setter 内 GL call dead-store 化計画 (Agent 並列 trace sealed 2026-05-31)

本 §4 は段階 4 着手前 trace-before-implement (`feedback_render_full_trace_first.md` 遵守) として Agent B 並列 trace で取得した RAII state class inventory + dead-store 化計画を sealed 形で保全。

### §4.1 12 件 verify (Agent B 並列 trace 結果)

- **actual count = 12 件** (sub-doc 03 §1.5.2 表記正)、handoff §1.3 / §1.5.3 + 03 doc §1.5.3 #1 の「12 件」記述と整合
- **listing**: LLGLDepthTest / LLGLSDefault / LLGLSObjectSelect / LLGLSUIDefault / LLGLSPipeline / LLGLSPipelineAlpha / LLGLSPipelineSelection / LLGLSPipelineSkyBox / LLGLSPipelineDepthTestSkyBox / LLGLSPipelineBlendSkyBox / LLGLSTracker / LLGLSSpecular (12 件)
- **coverage check**: 全 class は `indra/llrender/llglstates.h:35-191` に集約、`indra/llrender/llgl.h` 内 utility / infrastructure 系 (`LLGLState` (base) / `LLGLEnable` / `LLGLDisable` / `LLGLSquashToFarClip` / `LLGLUserClipPlane`) は本 §4 dead-store 化対象**外** (state primitive、bridging item #1 から分離管理)
- **inheritance graph**: `LLGLSPipelineBlendSkyBox` → `LLGLSPipelineDepthTestSkyBox` → `LLGLSPipelineSkyBox` (member composition + 上位継承)、dtor chain は member dtor 経由 implicit

### §4.2 各 class setter body + GL call + PSO equivalent (Agent B 並列 trace 結果)

| # | class | decl | ctor body summary | dtor body summary | GL calls (ctor/dtor) | Vulkan PSO equivalent | caller count |
|---|---|---|---|---|---|---|---|
| 1 | **LLGLDepthTest** | llglstates.h:35 | prev depth enable/func/write 保存 + 新 depth state 設定 + cond flush | prev depth state 復元 + cond flush | `glEnable(GL_DEPTH_TEST)` / `glDisable(GL_DEPTH_TEST)` / `glDepthFunc(depth_func)` / `glDepthMask(write_enabled)` | `VkPipelineDepthStencilStateCreateInfo.depthTestEnable` / `.depthCompareOp` / `.depthWriteEnable` | **158** (llrender ctor instantiation grep) |
| 2 | **LLGLSDefault** | llglstates.h:56 | GL_BLEND + GL_CULL_FACE disable (LLGLDisable member 経由) | member dtor 経由 restore | `glDisable(GL_BLEND)` / `glDisable(GL_CULL_FACE)` | `VkPipelineRasterizationStateCreateInfo.cullMode` / `VkPipelineColorBlendStateCreateInfo.blendEnable` | **4** |
| 3 | **LLGLSObjectSelect** | llglstates.h:69 | GL_BLEND disable + GL_CULL_FACE enable | member dtor 経由 restore | `glDisable(GL_BLEND)` / `glEnable(GL_CULL_FACE)` | `VkPipelineColorBlendStateCreateInfo.blendEnable` / `VkPipelineRasterizationStateCreateInfo.cullMode` | **0** (dead code 可能性、§4.4 で削除候補) |
| 4 | **LLGLSUIDefault** | llglstates.h:83 | GL_BLEND enable + GL_CULL_FACE disable + depth test off | member dtor 経由 restore | `glEnable(GL_BLEND)` / `glDisable(GL_CULL_FACE)` / `glDisable(GL_DEPTH_TEST)` / `glDepthMask(GL_TRUE)` / `glDepthFunc(GL_LEQUAL)` | (3 set 統合) | **47** |
| 5 | **LLGLSPipeline** | llglstates.h:99 | GL_CULL_FACE enable + depth test on (GL_LEQUAL) | member dtor 経由 restore | `glEnable(GL_CULL_FACE)` / `glEnable(GL_DEPTH_TEST)` / `glDepthFunc(GL_LEQUAL)` / `glDepthMask(GL_TRUE)` | (cullMode + depth state) | **12** |
| 6 | **LLGLSPipelineAlpha** | llglstates.h:111 | GL_BLEND enable | member dtor 経由 restore | `glEnable(GL_BLEND)` | `VkPipelineColorBlendStateCreateInfo.blendEnable` | **6** |
| 7 | **LLGLSPipelineSelection** | llglstates.h:121 | GL_CULL_FACE disable | member dtor 経由 restore | `glDisable(GL_CULL_FACE)` | `VkPipelineRasterizationStateCreateInfo.cullMode` | **1** |
| 8 | **LLGLSPipelineSkyBox** | llglstates.h:131 (decl) / llgl.cpp:2998 (def) | GL_CULL_FACE disable + LLGLSquashToFarClip 構築、**explicit ctor empty body (llgl.cpp:2998)** | **explicit dtor empty body (llgl.cpp:3004)** | `glDisable(GL_CULL_FACE)` (member 経由) | (cullMode + viewport manipulation) | **0** + **既に dead-store 済 (§4.4 priority 0)** |
| 9 | **LLGLSPipelineDepthTestSkyBox** | llglstates.h:141 (decl) / llgl.cpp:3008 (def) | 親 ctor + LLGLDepthTest 構築 (args) | 親 dtor + LLGLDepthTest dtor | LLGLDepthTest ctor 経由 (depth state set) + 親 SkyBox 経由 cull | (depth + cull + SkyBox 統合) | **1** |
| 10 | **LLGLSPipelineBlendSkyBox** | llglstates.h:149 (decl) / llgl.cpp:3015 (def) | 親 ctor + LLGLEnable(GL_BLEND) + `gGL.setSceneBlendType` | 親 dtor + LLGLEnable dtor | `glEnable(GL_BLEND)` + 親 SkyBox 経由 cull + 親 DepthTest 経由 depth | (blend + depth + cull + SkyBox 統合) | **3** |
| 11 | **LLGLSTracker** | llglstates.h:156 | GL_CULL_FACE enable + GL_BLEND enable | member dtor 経由 restore | `glEnable(GL_CULL_FACE)` / `glEnable(GL_BLEND)` | (cullMode + blendEnable) | **1** |
| 12 | **LLGLSSpecular** | llglstates.h:169 (inline ctor/dtor:173-191) | mShininess > 0 時 GL_FRONT_AND_BACK GL_SPECULAR / GL_SHININESS set | mShininess > 0 時 zero restore | `glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color)` / `glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shiny)` | material state (Vulkan 後 shader UBO で処理、bridging item #7 set=1 per-material と整合) | **1** |

### §4.3 caller compat 維持経路 (caller count refinement)

- **handoff §5.3 推定 "viewer 全体 850+ caller"** → **Agent B 実測 ~250-280 件 (12 class 合計)**、**実測で refine**
- **乖離理由**: handoff §5.3 推定は transitive nesting (LLGLSPipeline 内 LLGLDepthTest 内 LLGLEnable 等の RAII 階層) を含む合計、Agent B 実測は **direct ctor instantiation grep 件数** で計測、本 §4 dead-store 化対象は direct instantiation の setter body 内 GL call なので **Agent B 実測値が正確**
- **caller compat 維持原則**:
  - 12 class の **public interface (class declaration + ctor signature + member fields)** は **無改修維持** (source-level compat 担保)
  - setter body 内 GL call (ctor/dtor body) のみ **物理削除 = dead-store 化**、body は no-op (PSO compile 時 state 固定の Vulkan モデルに整合、charter §1 thesis = parity 不要、setter 動作は PSO bind 経由で代替)
  - LLGLDepthTest の動的切替機構は **`VK_EXT_extended_dynamic_state2`** (Vulkan 1.3 core) `vkCmdSetDepthTestEnable` / `vkCmdSetDepthCompareOp` / `vkCmdSetDepthWriteEnable` 経由 dynamic state 化 (sub-doc 03 §1.5.3 #2 既設計、PSO recompile 不要)
- **caller source-level compat verify**: dead-store 化 commit 後 viewer 全 build pass + 起動 pass + sustained 動作確認、ctor/dtor 呼出 ABI 維持 (vtable / size 不変)

### §4.4 dead-store 化順序 (low priority → high priority)

| 順序 | priority | class | 理由 |
|---|---|---|---|
| 1 | **priority 0 (既達)** | LLGLSPipelineSkyBox | **既に ctor/dtor empty body 済** (llgl.cpp:2998, 3004)、追加作業無し、verify 1 件のみ |
| 2 | **priority 1 (低 caller、低 risk)** | LLGLSObjectSelect (0 caller) / LLGLSPipelineSelection (1) / LLGLSTracker (1) / LLGLSSpecular (1) / LLGLSPipelineDepthTestSkyBox (1) | caller 1-0 件、ctor/dtor 内 GL call 物理削除で side-effect ほぼ無し、verify cost 最小 |
| 3 | **priority 2 (中 caller、中 risk)** | LLGLSPipelineBlendSkyBox (3) / LLGLSDefault (4) / LLGLSPipelineAlpha (6) / LLGLSPipeline (12) | caller 3-12 件、verify は AYA launch 動作確認で吸収可、validation strict 動作中 state mismatch 0 件確認必須 |
| 4 | **priority 3 (高 caller、要 dynamic state 配線)** | LLGLSUIDefault (47) | caller 47 件、3 GL state set (BLEND/CULL/DEPTH) 統合 dead-store、UI 描画 path 全体で source-level compat verify 必須 |
| 5 | **priority 4 (最高 caller、要 dynamic state 主軸)** | LLGLDepthTest (158) | caller 158 件、**dynamic depth state 化 (`VK_EXT_extended_dynamic_state2`)** が必須 (sub-doc 03 §1.5.3 #2 既設計)、setter 内 GL call 物理削除 + dynamic state CMD 経由 depth state 動的切替で代替、AYA launch 動作確認 + validation strict 動作中 0 件確認、段階 4 sub-step 4.4 最終 satisfy 経路 |

**完了 marker** (sub-step 4.4 全 priority 通過時):

1. `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/llrender/llgl.cpp indra/llrender/llglstates.h` が wrapper 内部以外で **0 件 hit**
2. 12 class 全 setter body GL call 物理削除済、class public interface (declaration + ctor signature + member fields) 無改修維持
3. ~250-280 caller source-level compat 維持 (viewer 全 build pass + AYA launch 動作確認 + validation strict 動作中 state mismatch 0 件)
4. LLGLDepthTest の動的 depth state 切替が `vkCmdSetDepthTestEnable` / `vkCmdSetDepthCompareOp` / `vkCmdSetDepthWriteEnable` 経由動作、3 driver baseline で query 確認 (sub-doc 03 §1.5.4 measurement-first 範式継承)

---

## §5 sub-step list (4.1 〜 4.5、段階 3 範式継承)

charter §7.5 で boundary refine 可、本 §5 は段階 3 5 sub-step (3.1a/3.1b/3.2/3.3/3.4/3.5) 範式継承 outline。各 sub-step 完遂時 self-trace + handoff (`feedback_self_verify_before_handoff.md` + `feedback_proactive_handoff.md` 遵守)。

### §5.1 sub-step 4.1 (PSO 基盤相当、LLPipelineFrameContext struct 配置 + low-risk migration)

- **対象**: `indra/newview/llpipelineframecontext.h` (新規) + `llpipelineframecontext.cpp` (新規) + `pipeline.cpp` (frame entry/exit helper 配線) + sCull / mRT 移行
- **完了 marker**:
  - LLPipelineFrameContext struct declaration + ctor/dtor + lifecycle helper (beginFrameContext / endFrameContext) 配置
  - sCull / mRT が frame context 経由 access 動作 (frame entry で grabReferences → context 内 cull result 保持、render path 内 const-ref 受渡し)
  - frame context size + per-frame write 頻度 log 出力 + measurement-first verify (§2.5)
  - AYA launch baseline 維持 (placeholder 三角形 + scene state regression 0 件)
- **相対工数感**: 中 (struct 設計 + 2 件 migration、新規 file 2 件)
- **risk**: low-medium (sub-doc 03 §1.5.6 範式継承)

### §5.2 sub-step 4.2 (軽量、10 件 bool フラグ migration)

- **対象**: sShadowRender + sRenderGlow + sRenderDeferred + sReflectionRender + sDistortionRender + sImpostorRender + sUnderWaterRender + sReflectionProbesEnabled + sRenderingHUDs + sDoFEnabled の 10 件 bool フラグ
- **完了 marker**:
  - 10 件全 bool フラグの frame context 経由 access 動作 (PassType enum + beginPass/endPass 経由 pass-specific transient flag 化)
  - render-pass state machine 動作維持 (shadow / deferred / post-process / etc.)
  - validation strict 動作中 state machine violation 0 件
  - AYA launch baseline 維持
- **相対工数感**: 中 (10 件 migration、機械的だが caller 全件更新)
- **risk**: medium

### §5.3 sub-step 4.3 (標準、sCurCameraID accessor 化 + 残 frame state + per-pool 実 scene draw 移植)

**(refine 2026-05-31、handoff-substep-4-3-beta-prep.md §4.2 反映)**: 旧 sub-step 4.3 は単一 sub-step 想定だったが、4.3-α 完遂 (commit `9f13302078` = sCurCameraID accessor 配線) 後の 3-agent 並列 trace で infrastructure 前提誤り発覚 (§3.4 refine note 反映)、案 D 採用下 7 sub-step に再設計:

- **対象**: sCurCameraID accessor 化 + sVisibleLightCount / sLastFocusPoint / sCompiles / sIndicesDrawnCount / mNumVisibleNodes / mNumVisibleFaces 移行 + **12 pool record hook signature 変更 + 実 scene draw 移植 (段階 3 引継ぎ)** + **領域 5 LLVertexBuffer Vulkan 化 (段階 5 部分前出し)** + **領域 6 sub-step 6.1 本格着手 (前出し)** + **領域 7 sub-step 7.3/7.4/7.5 本格化 (前出し)**

| 新 sub-step | scope | 完了 marker | risk |
|---|---|---|---|
| **4.3-α (完遂 2026-05-31、commit `9f13302078`)** | sCurCameraID accessor 配線 (LLViewerCamera::getCurCameraID/setCurCameraID inline 配置 + LLPipelineFrameContext forward accessor + ScopedCameraID nested RAII 配置) + write 14 件 + read 38 件 + include 配線 | spatial partition + visibility test 動作維持 + AYA launch verify PASS | low-medium |
| **4.3-β-prep (AYA review PASS 2026-05-31)** | scope refine 提案 doc 起草 (handoff-substep-4-3-beta-prep.md = 案 D 採用下起草、infrastructure 前提誤り発覚 + 新 cadence 再設計 + AYA 承認境界 update 提案) | AYA 「OK」承認 | — |
| **4.3-β'** | 領域 5 LLVertexBuffer Vulkan 化 (VkBuffer + bind helper + `vkCmdBindVertexBuffers` / `vkCmdBindIndexBuffer` caller 配置 + LLVertexBuffer instance lifecycle Vk 化) | LLVertexBuffer Vk 経路動作 + 既存 GL drawpool regression 0 件 + AYA launch verify PASS | medium-high |
| **4.3-γ'** | 領域 6 sub-step 6.1 本格着手 (autobuild integration 一括化、base port ~228 file 全 SPIR-V port、AYAstorm 改変 13 file は r42-α/β/γ scope 外維持) | 248 shader SPIR-V port 完遂 + glslangValidator install + autobuild integration 動作 + AYA launch verify PASS | high (旧「段階 4 sub-step 4.5 完遂後」縛り解除、AYA 承認 update) |
| **4.3-δ'** | 領域 7 sub-step 7.3 material cache 本実装 + 7.4 push descriptor 全配線 + 7.5 実 attachment 配線 | 3 pool + 9 pool + Avatar + GLTFPBR の per-material descriptor 配信動作 + AYA launch verify PASS | medium-high (旧「段階 4-5 並走判断保留」→ 段階 4 内本格化、AYA 承認 update) |
| **4.3-ε'** | Sky+WLSky+WaterExclusion + 9 pool per-pool 実 scene draw 移植 (旧 β+γ 集約、§3.4 上記 1-6 配線を 12 pool 一括適用) | AYA launch 時 placeholder fullscreen 三角形消失 + scene 描画 (黒画面ベース、vk-α 空転 baseline) + validation strict 動作中 violation 0 件 | high (sub-doc 03 §1.5.6 sub-step 3.4 範式継承、段階 4 最大 refactor) |
| **4.3-ζ'** | Avatar bone per-draw + GLTFPBR per-draw 移植 (旧 δ+ε 集約) | Avatar bone push descriptor 経由 per-draw 動作 + GLTFPBR per-material descriptor 動作 + AYA launch verify PASS | high |
| **4.3-η'** | self-check + handoff (旧 ζ 範式継承) | 4.3-α/β-prep/β'/γ'/δ'/ε'/ζ' 全完遂 evidence + acceptance #3-段階 4 satisfy 確認 + handoff-substep-4-3-complete.md 起草 | low (verify 中心) |

- **partial 配線許容 (再評価 2026-05-31)**: 旧注記「領域 7 sub-step 7.3/7.4/7.5 が partial state でも sub-step 4.3 は placeholder material + placeholder descriptor で動作確認可」は literal valid だが、視覚効果を持つには領域 5+6+7 整備が前提。新 cadence では 4.3-δ' 完遂前のみ partial 配線注記適用 (handoff-substep-4-3-beta-prep.md §6.5 case-validity 担保 + §3.4 refine note 反映)。
- **AYA 承認境界 update 必要事項 (handoff-substep-4-3-beta-prep.md §5.1 反映)**:
  - sub-step 6.1 本格着手: 旧「段階 4 sub-step 4.5 完遂後、並走しない」→ **新「sub-step 4.3-γ' (段階 4 内前出し)、4.3-β' 完遂後着手」**
  - 領域 7 sub-step 7.3/7.4/7.5 着手判断: 旧「段階 4-5 並走、handoff で擦り合わせ」→ **新「sub-step 4.3-δ' (段階 4 内本格化)、4.3-γ' 完遂後着手」**
  - 段階 5 着手: 旧「段階 4 完遂後」→ **新「領域 5 LLVertexBuffer Vulkan 化のみ sub-step 4.3-β' で前出し、残 段階 5 scope (llspatialpartition / llviewershadermgr / llvosky / llvowlsky + PFNGL 削除) は段階 4 完遂後維持」**
- **boundary refine 根拠**: charter §7.5 「r41 着手中に refine 可な本 charter content」(§2 領域別 着手順序 refine)、§3 acceptance criterion 趣旨維持 (charter §7.5 「AYA 確認なしに変更しない」遵守)、§6.1 acceptance #1/#3/#5/#6 段階 4 内 satisfy 経路維持

### §5.4 sub-step 4.4 (特殊、12 件 LLGLState RAII dead-store + LLVKRenderer skeleton)

- **対象**: 12 件 LLGLState RAII state class setter dead-store 化 (本 §4) + 領域 8 LLVKRenderer skeleton declaration 物理配置 (charter §3 #6 acceptance 段階 4 内 satisfy、領域 8 sub-doc `08-llvkrenderer-skeleton.md` 仮称 並走起草)
- **完了 marker**:
  - §4.4 dead-store 化順序 (priority 0-4) 全通過、§4.4 完了 marker 4 件全 satisfy
  - 領域 8 sub-doc 並走起草完遂 + LLVKRenderer skeleton declaration 物理配置完遂、`grep -rE "class LLVKRenderer" indra/` で declaration 存在 + 05 doc §10.1-§10.2 hook site placeholder 配置 + pipeline.cpp inline 実装と signature 整合
  - viewer 全 build pass + AYA launch 動作確認 + validation strict 動作中 state mismatch 0 件
- **相対工数感**: 大 (12 class dead-store + 領域 8 並走 + signature 整合 verify)
- **risk**: high (bridging item #1 完遂、charter §3 #1 + #6 acceptance 段階 4 内 satisfy 経路)

### §5.5 sub-step 4.5 (self-check + handoff)

- **対象**: 段階 4 self-check + validation strict 検証 + handoff doc 起草
- **完了 marker** (sub-doc 03 §3.1 sub-step 3.5 範式継承):
  - validation strict force-enable build で起動 + sustained ~10 分動作 → vulkanDebugCallback 経由 `[VK ERROR]` / `[VK WARN]` 0 件
  - 12 pool 全 hook 実 scene 描画動作 (vk-α 空転 baseline + placeholder 三角形消失)
  - AYAstorm 機能 (audio / chat / login / inventory) regression 0 件
  - `handoff-stage-4-complete.md` 起草、sub-doc 03 §3.1 sub-step 3.5 範式継承
  - 検証用 force-enable patch を `git checkout --` で revert、working tree clean (`feedback_remove_verification_logs.md` 遵守)
- **相対工数感**: 小 (verify 中心、新規 bridging item 無し)
- **risk**: low (sub-doc 03 §1.5.6 sub-step 3.5 範式継承)

---

## §6 段階 4 completion criteria (charter §3 #1 + #6 acceptance 段階 4 内 satisfy 計画含む)

### §6.1 charter §3 acceptance 5 件 段階 4 satisfy 計画 (sub-doc 03 §4.1 範式継承)

| acceptance criterion | 段階 4 sub-step での satisfy 経路 | 完了判定 |
|---|---|---|
| **#1-段階 4 (RAII setter 内 GL call dead-store 化、bridging item #1 完遂)** | sub-step 4.4 で 12 件 LLGLState RAII state class setter body 内 GL call 物理削除、~250-280 caller source-level compat 維持。**完了 marker**: `grep -rE "gl[A-Z][a-zA-Z]+\s*\(" indra/llrender/llgl.cpp indra/llrender/llglstates.h` が wrapper 内部以外で **0 件 hit** + viewer 全 build pass + AYA launch 動作維持 | 段階 4 内 satisfy 必須 |
| **#3-段階 4 (PSO bind 動作 + validation 0 件、段階 3 placeholder → 実 scene draw 移植)** | sub-step 4.3 で 12 pool 全 record hook body 内 placeholder helper 経由 fullscreen NDC 三角形描き → 実 scene visibility iteration + per-draw PSO bind + descriptor set bind + vertex/index buffer bind + `vkCmdDrawIndexed` 投入動作に移植。**完了 marker**: AYA launch 時 placeholder 三角形消失 + scene 描画 (vk-α 空転 baseline) + validation strict 動作中 violation 0 件 | 段階 4 内 satisfy 必須 (段階 3 acceptance を段階 4 内で実 scene draw に refine 完成) |
| **#5-段階 4 (frame context 経由 descriptor 更新)** | sub-step 4.1 で frame context lifecycle 配線 + set=0 PerFrame UBO 更新が frame context 経由動作、領域 7 sub-step 7.3/7.4/7.5 と協調。**完了 marker**: PerFrame UBO 更新の frame context 経由 access + descriptor set 3 階層動作維持 + dynamic rendering 動作維持 | 段階 4 内 satisfy + 領域 7 並走 |
| **#6-段階 4 (LLVKRenderer skeleton declaration 物理配置)** | sub-step 4.4 で領域 8 sub-doc 並走起草 + LLVKRenderer skeleton declaration 物理配置。**完了 marker**: `grep -rE "class LLVKRenderer" indra/` で declaration 存在 + 05 doc §10.1-§10.2 hook site placeholder 配置 + pipeline.cpp inline 実装と signature 整合 | 段階 4 内 satisfy 必須 (charter §3 #6 acceptance 段階 4 内 satisfy 経路、領域 8 並走) |
| **regression (段階 1-3 動作維持)** | 段階 1-3 完遂 evidence 維持 (Vulkan instance + device + command pool + render pass + 12 pool record hook + validation 0 件)、段階 4 sub-step 全完了時 AYA launch sustained ~10 分動作 PASS、AYAstorm 機能 regression 0 件 | 段階 4 完遂時 satisfy |

### §6.2 段階 4 完遂宣言の解釈 (sub-doc 03 §1.4 範式継承)

- 段階 4 全 sub-step (4.1-4.5) 完遂 + charter §3 #1 / #3 / #5 / #6 acceptance 段階 4 内 satisfy 経路全達成 + regression 0 件 → 段階 4 完遂
- charter §3 #1 acceptance 全体 (GL 除去) は **段階 5 (依存解決) で PFNGL function pointer declarations 物理削除 + 残依存 file PSO 化 = 段階 5 完遂時に最終 satisfy**
- 段階 5 着手 GO 判定: 段階 4 完遂時 PSO 基盤 + 12 件 RAII dead-store + LLPipelineFrameContext + 12 pool 実 scene draw + LLVKRenderer skeleton declaration 全完遂 = 段階 5 (残依存解決) の前提整備済

### §6.3 段階 4 完遂後の次 milestone への引継ぎ

- **段階 5 (依存解決) 着手**: charter §2 領域 5、0.50 PM、llspatialpartition / llviewershadermgr / llvertexbuffer / llvosky / llvowlsky 残依存 file Vulkan 化 + PFNGL function pointer declarations 物理削除 (§1.5.2 Cluster D 189 file 波及、段階 5 一体運用)
- **領域 6 sub-step 6.1 (autobuild integration) 並走判断**: 段階 4 着手中 / 完遂後 AYA 擦り合わせ (handoff §5.1 cadence 4. 反映)
- **領域 7 sub-step 7.3/7.4/7.5 並走判断**: 段階 4-5 並走 (handoff §4.3 反映)

---

## §7 関連 doc / memory

### §7.1 関連 doc (r41 章)

- `docs/specs/ayastorm-r41-gl-removal/00-charter.md` — r41 charter 本体 (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/01-foundation.md` — sub-doc foundation (closed 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/02-portage-execution.md` — sub-doc 段階 2 (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/03-state-machine-pso.md` — sub-doc 段階 3 (役割完了 2026-05-31)
- `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` — **本 sub-doc** (active、段階 4 着手前 起草)
- `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` — sub-doc 領域 6 (active 継続)
- `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` — sub-doc 領域 7 (active 継続)
- `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` (仮称) — sub-doc 領域 8 (段階 4 並走起草、sub-doc 04 完遂後 AYA 擦り合わせで起草開始判断)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-1-complete.md` — 段階 1 完遂 → 段階 2 着手前 prep (役割完了 2026-05-28)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-2-complete.md` — 段階 2 完遂 → 段階 3 着手前 prep (役割完了 2026-05-29)
- `docs/specs/ayastorm-r41-gl-removal/handoff-stage-3-complete.md` — 段階 3 完遂 → 段階 4 着手境界 (active、本 sub-doc 04 起草の direct trigger)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-alpha-complete.md` — sub-step 4.3-α 完遂 → 4.3-β 着手境界 handoff (役割完了、4.3-β-prep で新 cadence に置換)
- `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-beta-prep.md` — **sub-step 4.3-α 完遂後 4.3-β scope refine 提案 doc (active 2026-05-31、案 D 採用下起草、AYA review PASS、§3.4 / §5.3 refine の direct trigger)**

### §7.2 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone active (段階 4 着手 ready 状態、本 sub-doc 04 起草中)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 + Linux 先行例外
- `project_ayastorm_release_chapters.md` — r41 milestone の release 番号帯位置付け
- `feedback_render_full_trace_first.md` — §2 / §4 trace-before-implement 範式継承
- `feedback_use_agents_proactively.md` — §2 / §4 Agent 並列 trace 採用
- `feedback_doubt_self_first.md` — §2 / §4 Agent inventory を AYA 共有前に自分で再確認 (sub-doc 03 §1.5.5 Cluster C 教訓継承、特に caller count refinement 850 → 280)
- `feedback_admit_unknown.md` — §3.2 struct shape outline (sub-step 4.1 着手時 measurement-first で final 化)、本 §3 で speculation 確定回避
- `feedback_self_verify_before_handoff.md` — §6 段階 4 完遂時 self-trace + handoff
- `feedback_proactive_handoff.md` — sub-step 完遂境界 handoff
- `feedback_no_auto_commit.md` — sub-step commit は AYA 明示指示後
- `feedback_release_flow.md` — push は AYA 手動
- `feedback_build_only_verified.md` — §6 acceptance satisfy は実機検証
- `feedback_remove_verification_logs.md` — sub-step 4.5 verify 用 force-enable は出荷物に残さない
- `feedback_one_step_at_a_time.md` — 各 sub-step 1 cycle 完結
- `feedback_no_scope_shrink.md` — §6 acceptance 全 PASS で達成 (literal scope 維持)
- `feedback_self_bug_no_defer_option.md` — §6 acceptance unmet 時の対処は fix のみ提示

### §7.3 関連 charter section cross reference

- charter §2 領域 4 (本 sub-doc §1 / §2 / §3 / §4 backbone)
- charter §3 #1 acceptance (本 sub-doc §4 / §6)
- charter §3 #3 acceptance (本 sub-doc §3.4 / §6 段階 3 → 段階 4 実 scene draw refine)
- charter §3 #5 acceptance (本 sub-doc §3.1 frame context 経由 descriptor 更新)
- charter §3 #6 acceptance (本 sub-doc §1.2 / §5.4 / §6 領域 8 並走 satisfy 経路)
- charter §6.4 描画再構築 phase (本 sub-doc §1.1 risk 性質)
- charter §7.4 sub-doc 構成 (本 sub-doc 配置 + 範式継承)
- charter §7.5 boundary refine (本 sub-doc 起草 + sub-step 着手中 refine 可)

---

## sub-doc 04 draft 完成宣言 (2026-05-31)

本 sub-doc 04 は **Pattern α 一括 draft 完成 (2026-05-31、AYA review 待ち)**。

### draft 完成宣言の内訳

- §1 段階 4 scope plan (charter §2 領域 4 + 段階 3 引継ぎ反映)
- §2 pipeline.cpp 3 大グローバル + 周辺 frame state inventory (Agent A 並列 trace sealed)
- §3 LLPipelineFrameContext struct 設計 + caller migration 順序 (段階 3 引継ぎ per-pool 実 scene draw 移植経路含む)
- §4 12 件 LLGLState RAII state class dead-store 化計画 (Agent B 並列 trace sealed、caller count 850+ 推定 → 250-280 実測 refine)
- §5 sub-step list (4.1 〜 4.5、段階 3 範式継承)
- §6 段階 4 completion criteria (charter §3 #1 + #3 + #5 + #6 acceptance 段階 4 内 satisfy 計画)
- §7 関連 doc / memory + charter cross reference

### AYA review boundary

本 draft AYA review PASS 後の次 action:

1. sub-step 4.1 着手 (LLPipelineFrameContext struct 配置 + frame entry/exit helper 配線 + sCull / mRT 移行、measurement-first verify)
2. 領域 8 sub-doc `08-llvkrenderer-skeleton.md` 並走起草着手判断 (sub-doc 04 完遂後 AYA 擦り合わせ、handoff §5.1 cadence 4. 反映、2026-05-31 AYA 推奨 = sub-doc 04 完遂後判断)
3. 領域 6 sub-step 6.1 (autobuild integration) 本格着手判断 (段階 4 並走、AYA 擦り合わせ)

### 関連 commit (本 sub-doc 04 起草 commit は AYA 指示後実施)

本 sub-doc 04 起草 commit は `feedback_no_auto_commit.md` 遵守で **AYA 明示指示後 Claude が実施**。commit message draft (AYA 指示時に refine 可):

```
docs(r41): sub-doc 04 起草 (段階 4 frame context + LLGLState RAII dead-store 計画)

- 04-frame-context.md 新規作成 (Pattern α 一括 draft 2026-05-31、charter §7.4 標準範式)
- §1 段階 4 scope plan (charter §2 領域 4 + 段階 3 引継ぎ反映 = per-pool 実 scene draw 移植併合 + acceptance #1/#6 段階 4 内 satisfy)
- §2 pipeline.cpp 3 大グローバル (sCull/sShadowRender/sCurCameraID) + 周辺 frame state 17 件 inventory (Agent A 並列 trace sealed)
- §3 LLPipelineFrameContext struct 設計 + caller migration 順序 (low → high risk: mRT/sCull → 10 bool フラグ → sCurCameraID accessor → 12 RAII dead-store) + per-pool 実 scene draw 移植経路 (段階 3 引継ぎ)
- §4 12 件 LLGLState RAII state class dead-store 化計画 (Agent B 並列 trace sealed、caller count 850+ 推定 → 250-280 実測 refine)
- §5 sub-step 4.1-4.5 list (段階 3 範式継承)
- §6 段階 4 completion criteria (charter §3 #1 + #3 + #5 + #6 acceptance 段階 4 内 satisfy 計画 + regression)
- §7 関連 doc / memory + charter cross reference
```

push は AYA 手動 (`feedback_release_flow.md` 遵守)。
