# pipeline.cpp pure-move 分割 詳細設計(2026-08-11・デカファイル解体 ①)

**原則(AYA 裁定)**: 可読性 = 安全性。16,547 行の単一 file は grep を無意味化しトレースを行番号考古学にする。**ロジック変更ゼロの pure move** で意味単位に分割する。②(llvkloader.cpp)は本工事で確立する手順の流用(別弾)。

## 1. 分割形(LLPipeline class は 1 つのまま・複数翻訳単位実装)

| 新 file | 関数数 | 守備範囲 |
|---|---|---|
| llpipelinealloc.cpp | 30 | init/cleanup/screen・probe chain/GL・LUT buffer/BRDF/settings/restoreGL |
| llpipelineupdate.cpp | 35 | updateMove 族/rebuild 族/mark 族/updateGeom/parcel hide/hide・restore object |
| llpipelinecull.cpp | 23 | updateCull/occlusion/stateSort/references/visible extents |
| llpipelinerender.cpp | 13+1 | renderGeom* 4 駆動/renderObjects 族/buildRecordPassContext/renderHighlight(core から移す) |
| llpipelineshadow.cpp | 18 | renderShadow 族・影 MDI・cascade・shadowMatricesFinite 等 |
| llpipelinelighting.cpp | 26 | deferred lighting(soften/light/spot/multi)/bind・unbindDeferredShader/atmospherics/godrays/volumetric/enableLights 族 |
| llpipelinepost.cpp | 21 | exposure/tonemap/gamma/DoF/glow/SMAA/CAS/motionblur/SSR copy/vignette/blit/composite |
| llpipelinecapture.cpp | 15 | generateImpostor/snapshot/cube face/water exclusion mask/rigged-ID picker 族 |
| llpipelinedebug.cpp | 52 | toggle・beacon 族/debug render/lineSegmentIntersect 族/profileAvatar 族/verify |
| llpipelinepools.cpp | 16 | pool 生成・検索・QuickLookup/resetDrawOrders |
| **pipeline.cpp(core 残留)** | 19 | static member 定義群・render type mask 族・render map/alpha groups accessor |

## 2. 全数割当表(269 関数)

### llpipelinealloc.cpp(30)
connectRefreshCachedSettingsSafe, init, cleanup, destroyGL, requestResizeScreenTexture, resizeScreenTexture, allocateScreenBuffer, doAllocateScreenBuffer, mainChainComplete, probeChainComplete, heroChainComplete, allocateProbeChains, allocateScreenBufferInternal, refreshCachedSettings, releaseGLBuffers, releaseLUTBuffers, releaseScreenBuffers, createGLBuffers, loadColorGradingLUT, createLUTBuffers, generateBrdfLut, updateBrdfLut, restoreGL, shadersLoaded, canUseWindLightShaders, canUseAntiAliasing, unloadShaders, assertInitializedDoError, resetVertexBuffers, initDeferredVB

### llpipelineupdate.cpp(35)
updateRenderTransparentWater, resetFrameStats, updateMoveDampedAsync, updateMoveNormalAsync, updateMovedList, updateMove, calcPixelArea, calcPixelArea, updateDrawableGeom, updateGL, clearRebuildGroups, clearRebuildDrawables, rebuildPriorityGroups, updateGeom, parseParcelHideTag, shouldHideForOutsideParcel, isParcelHideAlive, refreshOutsideParcelHiding, markTextured, markGLRebuild, markPartitionMove, processPartitionQ, markMeshDirty, markRebuild, markRebuild, forAllVisibleDrawables, hidePermanentObjects, restorePermanentObjects, skipRenderingOfTerrain, hideObject, hideDrawable, unhideDrawable, restoreHiddenObject, rebuildDrawInfo, rebuildTerrain

### llpipelinecull.cpp(23)
grabReferences, clearReferences, checkReferences, checkReferences, checkReferences, checkReferences, visibleObjectsInFrustum, getVisibleExtents, isWaterClip, updateCull, markNotCulled, markOccluder, doOcclusion, markVisible, markMoved, markShift, shiftObjects, stateSort, stateSort, stateSort, stateSort, postSort, getVisiblePointCloud

### llpipelinerender.cpp(13)
isCinematicMode, renderHighlights, buildRecordPassContext, renderGeomDeferred, renderGeomPostDeferred, addTrianglesDrawn, recordTrianglesDrawn, renderObjects, renderGLTFObjects, renderAlphaObjects, renderMaskedObjects, renderFullbrightMaskedObjects, renderAlphaObjectsMultiview

### llpipelineshadow.cpp(18)
requestResizeShadowTexture, resizeShadowTexture, allocateShadowBuffer, releaseShadowBuffers, releaseSunShadowTargets, releaseSpotShadowTargets, enableShadows, renderGeomShadow, bindShadowMaps, renderShadowOpaqueBucketizedMultiview, renderShadowAlphaMultiview, renderShadowOpaqueBucketized, renderShadow, getSunShadowTarget, getSpotShadowTarget, generateSunShadow, skipRenderingShadows, handleShadowDetailChanged

### llpipelinelighting.cpp(26)
removeMutedAVsLights, renderVolumetric, setupAvatarLights, calcNearbyLights, setupHWLights, enableLights, enableLightsDynamic, enableLightsAvatar, enableLightsPreview, enableLightsAvatarEdit, enableLightsFullbright, disableLights, setLight, bindLightFunc, bindDeferredShaderFast, bindDeferredShader, renderDeferredLighting, doAtmospherics, doGodrays, doSkinSSS, doWaterHaze, setupSpotLight, unbindDeferredShader, setEnvMat, bindReflectionProbes, unbindReflectionProbes

### llpipelinepost.cpp(21)
renderGeomMotionBlur, renderMotionBlurComposite, compositeForwardFlip, visualizeBuffers, generateLuminance, generateExposure, tonemap, gammaCorrect, copyScreenSpaceReflections, generateGlow, applyCAS, applyFXAA, generateSMAABuffers, applySMAA, resolveSMAAT2x, copyRenderTarget, blitScenePresentToSwapchain, combineGlow, renderVignette, renderDoF, renderFinalize

### llpipelinecapture.cpp(15)
renderSnapshotGuidesOverlay, renderSnapshotFrame, renderRiggedObjectIDBufferForAvatar, armSelfRiggedObjectIDBuffer, isSelfRiggedObjectIDBufferArmed, isSelfRiggedObjectIDBufferReady, armOtherRiggedObjectIDBuffer, isOtherRiggedObjectIDBufferArmed, isOtherRiggedObjectIDBufferReady, clearOtherRiggedObjectIDBuffer, renderSelfRiggedObjectIDBuffer, renderOtherRiggedObjectIDBuffer, snapshotSceneDepthToWaterDis, doWaterExclusionMask, generateImpostor

### llpipelinedebug.cpp(52)
renderSelectedFaces, renderFocusPoint, renderPhysicsDisplay, renderDebug, findReferences, verify, toggleRenderType, toggleRenderTypeControl, toggleRenderTypeControlNegated, toggleRenderDebug, toggleRenderDebugControl, toggleRenderDebugFeature, toggleRenderDebugFeatureControl, setRenderDebugFeatureControl, pushRenderDebugFeatureMask, popRenderDebugFeatureMask, setRenderScriptedBeacons, toggleRenderScriptedBeacons, getRenderScriptedBeacons, setRenderScriptedTouchBeacons, toggleRenderScriptedTouchBeacons, getRenderScriptedTouchBeacons, setRenderMOAPBeacons, toggleRenderMOAPBeacons, getRenderMOAPBeacons, setRenderPhysicalBeacons, toggleRenderPhysicalBeacons, getRenderPhysicalBeacons, setRenderParticleBeacons, toggleRenderParticleBeacons, getRenderParticleBeacons, setRenderSoundBeacons, toggleRenderSoundBeacons, getRenderSoundBeacons, setRenderBeacons, toggleRenderBeacons, getRenderBeacons, setRenderHighlights, toggleRenderHighlights, getRenderHighlights, setRenderRegionCornerBeacons, toggleRenderRegionCornerBeacons, getRenderRegionCornerBeacons, setRenderHighlightTextureChannel, lineSegmentIntersectParticle, lineSegmentIntersectInWorld, lineSegmentIntersectInHUD, profileAvatar, enqueueProfileAvatar, drainPendingProfileAvatars, drainPendingAttachmentProfiles, addDebugBlip

### llpipelinepools.cpp(16)
dirtyPoolObjectTextures, findPool, getPool, getPoolFromTE, getPoolTypeFromTE, addPool, allocDrawable, unlinkDrawable, addObject, createObjects, createObject, rebuildPools, addToQuickLookup, removePool, removeFromQuickLookup, resetDrawOrders

### core(20)
sStatBatchSize, hasRenderTypeControl, getSpatialPartition, renderHighlight, hasRenderBatches, beginRenderMap, endRenderMap, beginAlphaGroups, endAlphaGroups, beginRiggedAlphaGroups, endRiggedAlphaGroups, hasRenderType, setRenderTypeMask, hasAnyRenderType, pushRenderTypeMask, popRenderTypeMask, andRenderTypeMask, clearRenderTypeMask, setAllRenderTypes, clearAllRenderTypes

- 例外裁定: `renderHighlight` は core → llpipelinerender.cpp / `hasRenderTypeControl` は core → llpipelinedebug.cpp(toggle 族)。`sStatBatchSize` は static 定義 = core 残留。

## 3. file-static / 無名 namespace の帰属(13 + 4 塊)

| symbol(行) | 帰属 |
|---|---|
| notifyMotionBlurSkippedOnce(:169)/ notifySSAOShadowSkippedOnce(:184)/ notifyDoFSkippedOnce(:199) | post(利用者と同居・要 grep 確認) |
| sGlNdcToSampleBias(:336) | 利用者 grep で確定(lighting/shadow 疑い) |
| sPipelineListener(:468・無名 ns :470) | alloc(settings 連結) |
| vkcScanEmptyDrawmapDefects(:4172) | update(呼び元と同居・検出器スキャン = pure move 厳守) |
| bindHighlightProgram/unbind(:5151/:5160) | render(renderHighlight と同居) |
| postDeferredPoolCheckpointLabel(:5681) | render(renderGeomPostDeferred と同居) |
| sIndicesDrawnCount(:5971) | core(addTrianglesDrawn 族と同居 = core → render に両方) |
| calc_light_dist(:7655) | lighting |
| shadowMatricesFinite(:13732)/ sShadowAlphaMvEnabled(:13781)/ FTM_SHADOW 系 timer(:13721-13730)/ 無名 ns(:13746) | shadow |
| 無名 ns(:11268/:11499) | 中身を移動時に判定(bindLightFunc/picker 近傍) |
- **方針**: 単独利用 = 利用者と同居移動。複数 file から使われる場合のみ `llpipelineinternal.h`(新設 private header)へ宣言を出し static を外す(linkage 変更 = 挙動中立・申告)。

## 4. 方式(pure move の機械保証)

1. 各新 file = PCH(llviewerprecompiledheaders.h)+ pipeline.cpp の include block を複製(pruning は本工事でやらない = 別途。重複 include はビルド時間コストのみで挙動中立)。
1b. **file ヘッダー(license)は移動元と同一を引き継ぐ**(AYA 指示 2026-08-11): pipeline.cpp = Linden 由来 viewerlgpl ヘッダー → 分割先 10 file も同一(@file/@brief のみ調整)。②の llvkloader 系は AYAstorm 著作ヘッダーを同様に引き継ぐ。
2. 関数本体は**一字一句そのまま移動**。
3. **pure-move oracle(gate)**: 移動前後で「全移動関数の本体テキストが byte 同一」を script 照合(関数単位抽出 → diff)。+ link 成功(移動漏れ = リンクエラーで全列挙)+ 判定走行。
4. CMakeLists(newview)へ新 file 10 本追加。
5. commit = 1 個(粗め・移動のみ。意味変更を混ぜない)。

## 5. gate

層0 build/link 成功 → pure-move oracle(byte 同一)全緑 → 判定走行(硬チャネル 0・VKC 沈黙)→ 層2 視覚(AYA)。

## 6. 申告欄

- include block の複製 = 不要 include が各 file に残る(pruning は意味変更リスクを避け本工事対象外)。
- 分類は名前ベース + 例外裁定。実装時に「隣接静的依存(直上の static を参照)」が見つかったら §3 方針で個別処置し完了報告に列挙。
- pipeline.h は無傷(class 定義は 1 つ)。将来の header 分割は別議題。

## 実装結果(2026-08-11・gate PASS)

- build 5 巡収束・**pure-move oracle = 分割前 rev との全行 multiset 照合で非意図差分 0**(削除 2 行 = inline 化 2 関数の旧 signature と 1:1 対応)。配置スポットチェック 5/5 ✓。判定走行 = 硬チャネル 0・VKC 全層沈黙・S1 0・視覚 OK(AYA)。
- 結果: 16,547 行 → core 1,197 + 10 file(888〜2,956)+ `llpipelineinternal.h`(共有 file-scope 部品: getFrame*/isFrame* getter・共有定数・octree traveler class・addDeferredAttachments 等)。
- pure move からの意図的逸脱(全列挙): inline 化 2(addDeferredAttachments/ayaDeriveReflectionProbeAmbianceVk)/ extern 宣言 4 行 / include 追加(llerror.h・SMAA tex→alloc・internal header)。static 剥がし = 0。
- CMake: viewer_SOURCE_FILES(共通リスト)+ viewer_HEADER_FILES に登録 = 3 OS 共通。Mac/Win は実機未検証(Linux のみ・構造要因なし)。
- 帰属の実測補正: vkcScanEmptyDrawmapDefects は update でなく cull / notifySSAOShadowSkippedOnce は post でなく lighting。
