# AYAstorm 描画パフォーマンス棚卸し (調査メモ)

> 対象: `feature/r13-occlusion-spec-draft` 時点の AYAstorm (Firestorm fork + LL viewer ベース)
> 想定読者: AYA さん
> 目的: viewer の描画ホットスポットを広く列挙し、改善余地の議論のたたき台にする
> 性格: 設計提案ではなく **観測ノート**。「コードに何が書いてあるか」と「どこに時間が落ちそうか」を集める。判断は後段の議論用。
> コード変更: なし (本書はリードオンリーの調査)

---

## 0. このメモの読み方

- 各章は「何が動いているか」 → 「どこが重そうか」 → 「観測の根拠 (ファイル:行)」の順。
- §6 が改善候補リスト。§9 で議論しやすい形に絞り込む。
- 「推測」と書いた所は、コードからは確証できないが業界一般の振る舞いから推論したもの。Tracy / GPU timer query で確認しないと断定不可。
- 数字 (μs / ms / FPS) はオーダー感のためのもの。AYA さんの実マシン (Linux + dGPU) で計測してずらしてよい。
- AYAstorm 独自パッチには `<FS:AYA>` / `// r13` / `[ParcelHide]` 等の目印が入っている。LL/Firestorm 上流との切り分けにこのマーカーが便利。

---

## 1. フレームパイプライン全体図

### 1.1 1 フレームの大枠

`indra/newview/llappviewer.cpp::LLAppViewer::mainLoop()` から `idle()` → `display()` を 1 フレーム 1 回呼ぶシングルレンダーループ。Linden Lab viewer はもともと「描画専用スレッド」を切らない設計で、AYAstorm/Firestorm もそれを踏襲している。よって **描画コードは全部 main thread** が回し、別 thread で並列化されているのは「描画の素材 (texture decode / mesh fetch / HTTP / image cache writeback)」だけ。

メインループの主要呼び出し順 (`llappviewer.cpp:5800-6204` 周辺):

```
LLAppViewer::idle()
  ├ network / agent update / send_agent_update      (llappviewer.cpp:5840-5871)
  ├ idleNameCache / idleNetwork / update_statistics  (5915-5936)
  ├ gIdleCallbacks.callFunctions()                  (5953)
  ├ gViewerWindow->updateUI()                       (5982)
  ├ gObjectList.update(gAgent)                      (6041)     // FTM_OBJECTLIST_UPDATE
  ├ gObjectList.cleanDeadObjects()                  (6055)     // FTM_CLEANUP
  ├ LLDrawable::cleanupDeadDrawables()              (6059)     // FTM_CLEANUP_DRAWABLES
  ├ LLSelectMgr::updateEffects / LLHUDManager       (6076-6081) // FTM_HUD_EFFECTS
  ├ gVLManager.unpackData()                         (6089-6090) // FTM_NETWORK
  ├ LLWorld::updateVisibilities / updateRegions     (6098-6103) // FTM_REGION_UPDATE
  ├ gPipeline.updateMove()                          (6139)
  ├ LLWorld::updateParticles()                      (6142)
  ├ gAgentCamera.updateCamera()                     (6159)
  ├ gObjectList.updateApparentAngles(gAgent)        (6171-6172) // FTM_LOD_UPDATE
  ├ LLAvatarRenderInfoAccountant::idle()            (6176)
  └ audio_update_*, gAudiop->idle(),
    LLPositionalStreamMgr::instance().update()      (6179-6193) // FTM_AUDIO_UPDATE  ← AYAstorm 追加

LLAppViewer::mainLoop()
  ├ ... (上記 idle)
  ├ display(...)                                    (llviewerdisplay.cpp:469)
  │   ├ gPipeline.mHeroProbeManager.update / renderProbes      (831-832)
  │   ├ LLHUDManager::updateEffects / LLHUDObject::updateAll   (860-861)
  │   ├ gPipeline.createObjects / processPartitionQ /
  │   │  updateGeom                                            (868-870)  // FTM_GEO_UPDATE
  │   ├ gPipeline.updateGL                                     (874)
  │   ├ gPipeline.updateCull                                   (898)      // FTM_CULL
  │   ├ gPipeline.generateSunShadow                            (923)      // FTM_GEN_SUN_SHADOW
  │   ├ LLVOAvatar::updateImpostors                            (934)
  │   ├ LLViewerTexture::updateClass / gTextureList.updateImages (963-975)
  │   ├ gPipeline.stateSort                                    (999)
  │   ├ gPipeline.rebuildPools                                 (1009)
  │   ├ gSky.updateSky                                         (1023)     // FTM_UPDATE_SKY
  │   ├ gPipeline.mRT->deferredScreen.bindTarget / clear       (1085-1095)
  │   ├ gPipeline.renderGeomDeferred                           (1130)     // FTM_RENDER_GEOMETRY
  │   ├ gPipeline.renderDeferredLighting                       (1152)
  │   ├ (HUD render pass: 1441-1480)
  │   └ render_ui (UI 3D / UI 2D / debug text)                 (llviewerdisplay.cpp:1601 以降)
  └ gViewerWindow->swapBuffers()
```

主要な計装ラベル (Tracy のゾーン名 / 旧 FTM_*) を頭に入れておくと、Tracy / FastTimer の出力が一気に読みやすくなる。

### 1.2 main thread の負荷分担 (定性)

| ブロック | 主に何をする | CPU バウンド / GPU バウンド | 備考 |
|---|---|---|---|
| idle/network | UDP/HTTP の応答処理、ObjectUpdate のデシリアライズ | CPU | login / TP 直後にスパイク。AYAstorm の `LLPositionalStreamMgr` はここで `update()` |
| `gObjectList.update` | アバター・プリムの位置/姿勢/LOD のメインスレッド側更新 | CPU | アバター多数の sim でかなり重い (アバター 1 体 = 数十 joint x ボーン補間) |
| `gPipeline.updateMove` | `mMovedList` の drawable をオクトリーで再配置、`octree->balance()` を全 region x 全 partition で実行 | CPU | drawable が大量に動く (TP/login/動く乗り物) と支配的になる |
| `gPipeline.updateGeom` | `mBuildQ1` から `updateDrawableGeom()` を呼んで vertex buffer 構築 | CPU | mesh / sculpt / volume の rebuild がここでフラッシュされる |
| `gPipeline.updateCull` | 各 region の各 partition の octree を camera frustum で cull | CPU | 大規模な scene 数千 drawable で μs〜ms 単位、camera が動く度に re-visit |
| `gPipeline.generateSunShadow` | sun shadow の 6 split をそれぞれ render | GPU 主、CPU は draw call 投入 | shadowmap on の場合恒常的に存在する負荷 |
| `LLVOAvatar::updateImpostors` | imposter キャッシュが古いアバターを 512x512 FBO に再描画 | GPU (CPU は描画コマンド組立) | アバター集会で頻発、1 体あたり数 ms |
| `gTextureList.updateImages` | テクスチャ priority 再計算 + main thread への decoded image 取り込み | CPU | "Image Update" Tracy ゾーン |
| `gPipeline.stateSort` | culled drawable を draw map に並べ替え + alpha group sort | CPU | アルファ多数の scene で hot |
| `renderGeomDeferred` | G-buffer (位置/法線/material) を書き出す pass | GPU 主 | shader と draw call はここに集中 |
| `renderDeferredLighting` | sun + local lights + atmospherics + alpha + post | GPU 主 | local lights 数で線形に重くなる |

「**main thread が描画を全部やる**」ので、TP / login / 群集 でアバターやプリムが大量に流入したフレームは Render 時間ではなく Update 時間 (updateMove / updateGeom / updateCull) が伸びる、というのが Firestorm 系の典型的な張り付き原因。

### 1.3 LLDrawable の rebuild フロー

`indra/newview/lldrawable.h:255-265` に rebuild フラグ定義:

```cpp
REBUILD_VOLUME  = 0x00000100,   // volume changed LOD or parameters, or vertex buffer changed
REBUILD_TCOORD  = 0x00000200,   // texture coordinates changed
REBUILD_COLOR   = 0x00000400,   // color changed
REBUILD_POSITION= 0x00000800,   // vertex positions/normals changed
REBUILD_GEOMETRY= REBUILD_POSITION|REBUILD_TCOORD|REBUILD_COLOR,
REBUILD_MATERIAL= REBUILD_TCOORD|REBUILD_COLOR,
REBUILD_ALL     = REBUILD_GEOMETRY|REBUILD_VOLUME,
REBUILD_RIGGED  = 0x00001000,
```

LLPipeline 側のフロー (`pipeline.cpp:3640-3666` の `markRebuild`):

1. `markRebuild()` 呼び出しで `IN_REBUILD_Q` を立て、`mBuildQ1` へ push、対応 flag を OR。
2. `display()` の `updateGeom(max_dtime)` (`llviewerdisplay.cpp:867-870`, `pipeline.cpp:3067-3111`) が **時間予算 50 ms/s** (= 約 0.8 ms/frame@60Hz) を持って `mBuildQ1` をドレイン。
3. ドレインで `updateDrawableGeom()` → `LLVOVolume::updateGeometry()` (`llvovolume.cpp:2276`) → `regenFaces` / `lodOrSculptChanged` / `genBBoxes` を実行。
4. spatial group は `GEOM_DIRTY` を立て、`rebuildGeom()` が次の `postSort` または `stateSort` の "checkOcclusionAndRebuildMesh" ループで起動 (`pipeline.cpp:3690-3714`)。
5. `rebuildGeom` は `LLSpatialPartition::rebuildGeom()` で全 drawable / 全 face の頂点を 1 つの VBO に詰め直す (`llspatialpartition.cpp:346-402`)。
6. その中身が **`LLFace::getGeometryVolume()`** (`llface.cpp:1192` 以降) — viewer の中で最も重い CPU ホットパスのひとつ。位置・法線・UV・texgen・tangent・color・emissive をすべて CPU で再計算して buffer に書く。

→ rebuild は「volume が変わった面の per-vertex CPU 計算 + glBufferSubData」が支配的。AYAstorm 拡張は基本的にこのループの中身は触っていない (parcelhide はループの外側で drawable を弾く)。

### 1.4 オクトリー visit / cull の per-frame コスト

`pipeline.cpp:2707-2791` の `updateCull` がフレームごとに:

- 全 region (隣接 sim 込み、典型 4〜9) を回り、
- region ごとに `LLViewerRegion::NUM_PARTITIONS` 個の spatial partition (TERRAIN / WATER / TREE / BRIDGE / AVATAR / CONTROL_AV / HUD_PARTICLE / VOLUME / PARTICLE / GRASS / VOLATILE / VO_CACHE) を、
- それぞれの partition の octree を camera frustum で再帰 visit して、visible group を `sCull->pushVisibleGroup` する。

cull の visit は `llspatialpartition.cpp` の OctreeNode を再帰する。1 ノードあたりは数十命令で済むが、log-N の log がそこそこ大きい sim だと数万 node visit になり ms 単位を食う。LL_PROFILE_ZONE_SCOPED が 4416 行中 17 箇所に貼られている (`llspatialpartition.cpp:228, 359, 417, 506, 555, 642, 739, 820, 946, 972, 989, 1079, 1090, 1101, 1399, 1426, 1449`) — つまり「ここが重いのは LL も自覚済み」。

---

## 2. 計装済みホットスポットの棚卸し

### 2.1 LL_RECORD_BLOCK_TIME (FTM_*) ラベル — viewer が「自分でホットスポット」と名付けている関数

`indra/` 配下を一通り `Grep` した結果、描画 / 更新 / 同期系に貼られている FTM_* ラベル一覧:

| FTM ラベル | 貼られている場所 | 何を計測 | カテゴリ |
|---|---|---|---|
| `FTM_FRAME` | `llappviewer.cpp:1577` | 1 フレーム全体 | top-level |
| `FTM_NETWORK` | `llappviewer.cpp:5841, 6002, 6089` | UDP + ObjectUpdate のデシリアライズ | network |
| `FTM_REGION_UPDATE` | `llappviewer.cpp:6006, 6101`, `llviewerregion.cpp` | region tile 等の更新 | world |
| `FTM_AGENT_POSITION` | `llappviewer.cpp:6029` | agent 移動 + gesture | agent |
| `FTM_OBJECTLIST_UPDATE` | `llappviewer.cpp:6036` | `gObjectList.update()` | scene |
| `FTM_CLEANUP` / `FTM_CLEANUP_DRAWABLES` | `llappviewer.cpp:6053, 6058` | dead オブジェクト / drawable の回収 | scene |
| `FTM_HUD_EFFECTS` | `llappviewer.cpp:6077` | LLSelectMgr + LLHUDManager | hud |
| `FTM_LOD_UPDATE` | `llappviewer.cpp:6171` | `gObjectList.updateApparentAngles` | scene |
| `FTM_AUDIO_UPDATE` | `llappviewer.cpp:6179` | audio engine idle + AYA `LLPositionalStreamMgr::update` | audio |
| `FTM_STREAM3D_MGR_UPDATE` | `llpositionalstreammgr.cpp:1773` | AYAstorm r13 stream / occlusion mgr 自己計測 | AYAstorm |
| `FTM_GEO_UPDATE` | `pipeline.cpp:3072` | drawable rebuild queue ドレイン | pipeline |
| `FTM_CULL` (現在は `LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE` 名前付きで残る) | `pipeline.cpp:2709` | frustum cull | pipeline |
| `FTM_STATESORT_DRAWABLE` | `pipeline.cpp:3781` | drawable per-face pool 割当 | pipeline |
| `FTM_RENDER_GEOMETRY` | `pipeline.cpp:4603` (`renderGeomDeferred`) | G-buffer 描画ループ | drawpool |
| `FTM_RENDER_UI` | `pipeline.cpp:4451`, `llviewerdisplay.cpp:1601` | UI 描画 (HUD 含む) | ui |
| `FTM_RENDER_UI_3D` / `FTM_RENDER_UI_2D` | `llviewerdisplay.cpp:1654, 1672` | 3D HUD / 2D overlay | ui |
| `FTM_SHADOW_RENDER` / `FTM_SHADOW_SIMPLE` | `pipeline.cpp:10843, 10921` | shadow pass の simple drawable | shadow |
| `FTM_GEN_SUN_SHADOW` | `pipeline.cpp:11281` | sun shadow (4 split) 全体 | shadow |
| `FTM_RENDER_BLOOM` | `pipeline.cpp:9353` | post bloom | post |
| `FTM_RENDER_TREES` | `lldrawpooltree.cpp:58, 124` | LL tree (legacy) | drawpool |
| `FTM_RENDER_TERRAIN` (zone 名で残存) | `lldrawpoolterrain.cpp:134, 140, 147, 171, 182, 189` | terrain + shadow terrain | drawpool |
| `FTM_RENDER_WL_SKY` | `lldrawpoolwlsky.cpp:473` | WL sky | drawpool |
| `FTM_RENDER_SHINY` / `FTM_RENDER_BUMP` | `lldrawpoolbump.cpp:287, 358, 390, 499, 521, 546` | bump / shiny passes | drawpool |
| `FTM_RENDER_INVISIBLE` | `lldrawpoolwaterexclusion.cpp:45` | water exclusion | drawpool |
| `FTM_RENDER_SIMPLE_DEFERRED` / `FTM_RENDER_ALPHA_MASK_DEFERRED` / `FTM_RENDER_FULLBRIGHT` | `lldrawpoolsimple.cpp:101, 118, 158, 186` | simple / alpha-mask / fullbright | drawpool |
| `FTM_RENDER_CHARACTERS` | `lldrawpoolavatar.cpp:673` | アバター描画 | avatar |
| `FTM_UPDATE_GRASS` | `llvograss.cpp:418` | grass volume | drawpool |
| `FTM_HUD_UPDATE` | `llhudobject.cpp:286` | HUD object idle 全体 | hud |
| `FTM_UPDATE_HUD_EFFECTS` | `llhudmanager.cpp:61` | beam / spotlight などのフロート HUD | hud |
| `FTM_UPDATE_CAMERA` | `llagentcamera.cpp:1295` | camera tween | camera |
| `FTM_UPDATE_WORLD_VIEW` | `llviewerwindow.cpp:4510` | window view ジオメトリ更新 | viewport |
| `FTM_DISPLAY_DEBUG_TEXT` | `llviewerwindow.cpp:1045` | 右上の debug overlay | ui debug |
| `FTM_WINDOW_CHECK_SETTINGS` | `llviewerwindow.cpp:7177` | RenderResolutionDivisor などの再評価 | viewport |
| `FTM_SIMULATE_PARTICLES` | `llviewerpartsim.cpp:730` | particle source update | particles |
| `FTM_RENDER_TIMER` | `llfasttimerview.cpp:397` | FastTimer view 自体 | dev |
| `FTM_PROCESS_TIMES` | `llfasttimer.cpp:300` | timer 集計 | dev |
| `FTM_PROCESS_MESSAGES` | `lltemplatemessagereader.cpp:538` | LLMessageSystem 受信 | network |
| `FTM_MEDIA_*` | `llviewermedia.cpp:663, 690, 702, 707, 738, 925, 2207, 2933, 3794` | parcel media + CEF / Sentry | media |
| `FTM_MESH_FETCH` (zone 化) | `llmeshrepository.cpp:4443, 4531, 5047, 5075` | mesh HTTP coro | network/mesh |
| `FTM_RLV_EFFECT_SPHERE` | `rlveffects.cpp:379` | RLVa sphere | RLVa |
| `FTM_VIVOX_PROCESS` | `llvoicevivox.cpp:7389` | Vivox VAD | voice |
| `FTM_INVENTORY_*` / `FTM_BULK_FETCH` | inventory 系 | inventory 経路 | inventory |

→ 「`FTM_FRAME` を 100% として、main thread の中で **どこに何 % 落ちているか**」を見るのが Tracy / FastTimer の最初の使い道。

### 2.2 LL_PROFILE_ZONE_* (Tracy ゾーン) のカテゴリ一覧

`indra/llcommon/llprofilercategories.h:44-72` で enable/disable される 26 カテゴリ:

```
APP / AVATAR / DISPLAY / DRAWABLE / DRAWPOOL / ENVIRONMENT / FACE / INPUT (FS:Beq)
LLSD(off) / LOGGING / MATERIAL / MEDIA / MEMORY(off) / NETWORK / OCTREE
PIPELINE / SHADER / SPATIAL / STATS / STRING / TEXTURE / THREAD(off) / UI
VIEWER / VERTEX / VOLUME / WIN32(off) / GLTF / VOICE
```

`off` の 4 つ (LLSD / MEMORY / THREAD / WIN32) は Firestorm/AYAstorm が「Tracy メモリを食うわりに見返りが少ない」と判断して切ってあるので、必要に応じて on にする。

主要ファイルあたりの計装密度 (sample 件数):

| ファイル | LL_PROFILE_ZONE_ 件数 | 主目的 |
|---|---|---|
| `pipeline.cpp` | 約 70 件 (うち `_NAMED` の細分化が約 30) | 描画パイプ全段に貼ってある |
| `llvoavatar.cpp` | 約 30 件 | imposter / 各 idleUpdate* / updateAttachmentOverrides 等 |
| `llspatialpartition.cpp` | 17 件 | octree visit / rebuildGeom / changeLOD |
| `llvovolume.cpp` | 約 30 件 | rebuildGeom / rebuildMesh / genDrawInfo / face list |
| `llface.cpp` | 約 25 件 | getGeometryVolume の細分化 (位置/法線/UV/tangent/color/emissive) |
| `llreflectionmapmanager.cpp` | 約 15 件 | probe update / face / uniforms / occlusion |
| `lltexturefetch.cpp` | 約 25 件 | TextureFetchWorker の state machine 全部 |
| `lldrawpoolalpha.cpp` | 約 5 件 | alpha pass + emissives |
| `llappviewer.cpp` | 約 25 件 (NAMED 含む) | idle のステージごと |

→ 「LL の人が自分でホット候補と思った所」≒ Tracy zone が貼ってある場所。本書 §3 〜 §5 はこれを地図にしている。

### 2.3 仕事の質: CPU 仕事 / GPU 待ち / IPC 待ち

| ゾーン名 / 関数 | 推定区分 | 根拠 |
|---|---|---|
| `getGeometryVolume` 系 (face.cpp) | **CPU 純粋計算** | per-vertex 位置/法線/UV/texgen を SIMD 込みで計算 |
| `updateCull` / octree visit | **CPU 純粋計算** | frustum 4 平面 vs AABB の SIMD 比較 |
| `rebuildGeom` 内 `glBufferSubData` | **CPU だが IPC 寄り** | 大きな buffer は driver 内コピー、`pipeline.cpp:4262 "rebuild delayed upd groups"` |
| `generateSunShadow` | **GPU 主 + CPU の draw call 投入** | `LL_PROFILE_GPU_ZONE("generateSunShadow")` (`pipeline.cpp:11282`) |
| `generateImpostor` | **GPU 主** | 512x512 FBO に full avatar 描画 (`pipeline.cpp:12162`) |
| `renderGeomDeferred` / `renderDeferredLighting` | **GPU 主** | `LL_PROFILE_GPU_ZONE` が貼ってある |
| `updateImpostors` (汎用ループ) | **CPU + GPU 混合** | アバター毎に `generateImpostor` を呼ぶ |
| `LLPositionalStreamMgr::update()` / `LLOcclusionGeometryMgr::refreshOccluders()` | **CPU 純粋計算** (AYAstorm) | OBB transform 更新 + extractTriangles + raycast |
| `LLVOAvatar::idleUpdate*` 一族 | **CPU** | アニメ補間 + bone update |
| `LLViewerPartSim::updateSimulation` | **CPU** | particle source 全件ループ |
| `LLViewerTextureList::updateImages` | **CPU + IPC 寄り** | decode thread からのキューを main で取り込み + GL upload |
| `gPipeline.updateGL` | **GPU 寄り** | GL state 同期、driver と握手 |

「CPU か GPU か」は Tracy のレイヤーを 2 つ (CPU zone と GPU zone) 重ねれば一目で分かる。AYAstorm はすでに `LL_PROFILE_GPU_ZONE` を主要 pass に置いてある (`updateCull` / `doOcclusion` / `generateImpostor` / `generateSunShadow` / `renderGeomDeferred` / `renderGeomPostDeferred` / `reflection manager update` 等)。

---

## 3. CPU 単独で重い既知ポイント (深掘り)

### 3.1 Avatar skinning と attachments

#### 入口

- `LLVOAvatar::idleUpdate()` (`llvoavatar.cpp:3065`) — main thread 毎フレーム、すべての visible avatar に対して呼ばれる。
- 中で:
  - `idleUpdateVoiceVisualizer` (3269)
  - `idleUpdateMisc` (3383)
  - `updateCharacter` (5649) — animation matrix の補間と骨格 update が中心
  - `idleUpdateAppearanceAnimation` / `idleUpdateLipSync` / `idleUpdateLoadingEffect` / `idleUpdateWindEffect`
  - `idleUpdateNameTag` / `idleUpdateNameTagText` / `idleUpdateNameTagAlpha`
  - `idleUpdateBelowWater` / `idleUpdateRenderComplexity` / `idleUpdateDebugInfo`
- `LLVOAvatar::updateAttachmentOverrides()` (7524) — attachment の joint override を再評価。アタッチを着脱した直後にコストが集中。
- `LLVOAvatar::updateRiggingInfo()` (12081) — 全 LLVOVolume の rigged mesh について `mJointRiggingInfoTab` を再構築。`mLastRiggingInfoKey` でハッシュキャッシュしてあるので「衣装が変わらなければ早期 return」(12110)。

#### ホットスポット推定

- アバター 1 体あたり `updateCharacter` + `idleUpdate*` で典型 100〜300μs (Tracy 観測の経験則)。20 体集まれば 2〜6 ms/frame、これだけで 60 FPS の予算 (16.7 ms) を圧迫する。
- `updateAttachmentOverrides` は静的時間は薄いが TP / outfit change 直後にスパイク。
- skinning 行列 ([N_joints x 4x4]) は `LLVOAvatar::updateGeometry` で再計算され、最後は `LLFace::getGeometryVolume` 内の skinned path で頂点に乗る (CPU 側で乗算してから upload 観測中)。LL 上流が GLTF skin に移行中なので、その辺は将来 GPU 側にずれる可能性が高い。

#### ファイル参照

- `indra/newview/llvoavatar.cpp:3065-3267` (idleUpdate)
- `indra/newview/llvoavatar.cpp:5649` (updateCharacter)
- `indra/newview/llvoavatar.cpp:7524` (updateAttachmentOverrides)
- `indra/newview/llvoavatar.cpp:12081-12133` (updateRiggingInfo)
- `indra/newview/llvoavatar.cpp:12148-12166` (updateImpostors)

### 3.2 Particle system

- `LLViewerPartSim::updateSimulation()` (`llviewerpartsim.cpp:717-878`)
- フレームごとに `mViewerPartSources` 全件 + `mViewerPartGroups` 全件をループ。
- 各 part source の `update(dt)` は AYAstorm の `[parcelhide:...]` ゲート (`llviewerpartsim.cpp:795-800`) を通った後に呼ばれる。
- visible でない group は `visirate=8` で 1/8 にスキップする (823-833) — 既に LL が最低限の thinning は入れてある。
- ホットスポット: `update(dt)` は per-particle で位置 / 寿命 / アルファ / color を更新 + emit。**全部 CPU**。GPU compute / transform feedback には乗っていない。

→ "重い particle のついた爆発エフェクト" 周辺で 1〜数 ms/frame 食う実例は viewer フォーラムでも多数報告されてる典型ポイント。

### 3.3 Hovertext / HUD text

#### LLHUDNameTag (アバター上のネームタグ)

- `LLHUDNameTag::updateAll()` (`llhudnametag.cpp:720-810`) — 毎フレーム visible 全件を:
  1. visibility 計算 (730-733)
  2. screen 距離で sort (737)
  3. screen area で LOD 設定 (743-766)
  4. **2D overlap iteration を `NUM_OVERLAP_ITERATIONS` 回 (= 3 回)** 走らせて重なり調整 (777-810)
- `LLHUDNameTag::render` (226-233) → `renderText` (236-450) — per-text で background quad + label 行 + 本文行を `hud_render_text` 呼び出し。テキストは LLFontGL の glyph atlas 経由。

#### LLHUDText (object hovertext / floating text)

- `LLHUDText::renderText` (`llhudtext.cpp:159`)
- `LLPipeline::isParcelHideAlive` で AYAstorm の parcelhide ガードが入る (`llhudtext.cpp:133-137`)。

#### コスト構造 (推測)

- N tags × 3 overlap iterations × O(N) ペア比較 = **O(N²)** に近い。30 アバターが密集すると `updateAll` が 0.5〜2 ms/frame。
- レンダー側は文字数 × glyph 数 × draw call。LLFontGL が `LLFontVertexBuffer` でバッファ化されてるとはいえ font ごと style ごとに draw が出る。

### 3.4 Imposter rebuild

- `LLVOAvatar::updateImpostors()` (12148) — main thread が "isImpostor() && needsImpostorUpdate()" のアバターについて `gPipeline.generateImpostor(avatar)` を呼ぶ。
- `LLPipeline::generateImpostor` (`pipeline.cpp:12159-12450 付近`):
  - `pushRenderTypeMask` で sky / water / terrain / particles を全部消す
  - 512x512 (典型) の FBO に avatar 1 体だけを deferred + lighting で再描画
  - `LL_PROFILE_GPU_ZONE("generateImpostor")` (12162) で GPU 時間も計測
- impostor の更新条件:
  - `mNeedsImpostorUpdate` が立つ (`isRlvSilhouette` も含む — `llvoavatar.cpp:4776`)
  - `isVisuallyMuted()` / `isTooComplex()` / `isTooSlow()` のどれかで impostor 扱いになっている (12169-12193)
- AYAstorm パッチでは `FSImpostorAvatarExclude` で animesh / control av を例外扱いできる (`llvoavatar.cpp:4697-4736`)。
- ホットスポット: 1 体あたり典型 1〜3 ms (GPU 時間込み)。集会で 10 体が同時に動いて imposter を re-render するとフレームが lag spike。

### 3.5 Reflection probes

- `LLReflectionMapManager::update()` (`llreflectionmapmanager.cpp:206-731`) — `gFrameTimeSeconds` ベースでスケジュール、`doProbeUpdate()` (731) が実際の 6 面 cubemap render を駆動。
- `updateProbeFace(probe, face)` (773) は内部で `mRenderTarget` (super-sampled `mProbeResolution * 4`) に scene を 1 面分 deferred render、それを mip chain (`mMipChain`) に reduce。
- ENV `RenderReflectionProbeLevel` (0=none / 1=manual / 2=manual+terrain / 3=full) で probe 数が制御される (236-247)。`mDynamicProbeCount` で動的に絞る (229-262)。
- `updateNeighbors` (1044) は probe 同士の隣接探索、`mProbes.size()` 二重ループになっている疑い (推測 — 1069 周辺の "search" zone)。

→ probe update は **GPU 重い**。AYAstorm 上は Beq が `mDynamicProbeCount` で抑える既存のチューニングが既に入っている。

### 3.6 Texture pipeline

#### スレッド構成

- **decode thread**: `LLImageDecodeThread` (`llappviewer.cpp:752, 2651`) — JPEG2000 / PNG / TGA / KTX を main thread から切り離して decode。
- **fetch worker**: `LLTextureFetchWorker::doWork` (`lltexturefetch.cpp:1135-2150`) は LLQueuedThread 系のワーカー。State machine (INIT / LOAD_FROM_TEXTURE_CACHE / CACHE_POST / LOAD_FROM_NETWORK / LOAD_FROM_SIMULATOR / WAIT_HTTP_RESOURCE / SEND_HTTP_REQ / WAIT_HTTP_REQ / DECODE_IMAGE / DECODE_IMAGE_UPDATE / WRITE_TO_CACHE / WAIT_ON_WRITE / DONE) の各ステージに `LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE` が貼られている (`lltexturefetch.cpp:1152〜2130`)。
- **main thread への戻り**: `gTextureList.updateImages(max_image_decode_time)` (`llviewerdisplay.cpp:973-975`) が 2〜5 ms/frame の予算で:
  - decode 完了画像を取り込み
  - `LLViewerFetchedTexture::createGLTexture` で GL texture に upload
  - priority 計算 (距離 / 画面占有率)

#### ホットスポット推定

- `LLViewerTexture::updateClass` (`llviewerdisplay.cpp:963`) は全 texture を回って「class」を再評価。テクスチャ枚数 (典型 5,000〜20,000) で線形。
- GL upload (glTexSubImage2D) は main thread。PBO async upload には**乗っていない**(LL viewer の長年の宿題)。

### 3.7 Mesh / asset 読込

- `LLMeshRepoThread::run()` (`llmeshrepository.cpp:1035`) が repo thread。`HeaderRequest` / `LODRequest` を HTTP coro で並行に発行 (`llmeshrepository.cpp:4443, 4531, 5047, 5075` の `FTM_MESH_FETCH` zone)。
- main thread 側で `notifyLoadedMeshes()` が `mLoadedQ` をドレインして `LLVolume` を build、interested object に `notifyMeshLoaded` 発火。
- 多数の mesh が同時に来ると main thread の `mLoadedQ` ドレインがスパイクする (TP 直後の典型「カクッ」)。

### 3.8 Vertex buffer 更新

- `LLVertexBuffer::flushBuffers()` (`llvertexbuffer.cpp:1032-1043`) — `sMappedBuffers` を全部 `_unmapBuffer()`。これは `pipeline.cpp:4443` (postSort 末尾)、`pipeline.cpp:4647` (renderGeomDeferred の pool ループ前後) などで呼ばれる。
- `flush_vbo` (1372) は `glBufferSubData` を 64KB ブロックに分けて発行 (1396-1405)。
- macOS だけ別経路 — `glBufferData` で都度 buffer 再作成 (1440-1459)。
- **persistent mapped buffer / fence sync は使っていない**。Apple 経路以外は `glBufferSubData` の素直なコピー。Linden は SL viewer の長期計画として PBO/persistent mapping を構想しているが、現状実装は古典的。

### 3.9 Pool / Drawable rebuild

- `LLPipeline::rebuildPools()` (`pipeline.cpp:6052-6085`) — `mPools` set を 1 回ぐるっと回って、`isDead()` を捨て、それ以外は素通り。**重い処理ではない** が、`display()` の途中で呼ばれている (`llviewerdisplay.cpp:1009`)。
- 重いのはこの直前の `stateSort` 内の `rebuildGeom` (`pipeline.cpp:3713`, `3775`, `4184`) と、`postSort` 内の `rebuildGeom` (`pipeline.cpp:4151`)、`rebuild delayed upd groups` の `rebuildMesh` (4266-4270)。
- 各 pool の `renderDeferred(i)` (`pipeline.cpp:4699`) は **draw call の発行**。bind / unbind / state 変更が pool 数 (typical 12〜18) ぶん入る。

### 3.10 Alpha sort

- alpha pool は距離ソートが必要 — `postSort` の "sort alpha groups" zone (`pipeline.cpp:4281-4289`):
  - `std::sort(sCull->beginAlphaGroups(), ..., CompareDepthGreater())`
  - rigged alpha は `CompareRenderOrder()` (アタッチ順)
- per-group sort なので **group 数 × log(group数)** の比較。group 内の DrawInfo は `genDrawInfo` (`llvovolume.cpp:6614-7100`) の段階で per-face のアルファ振り分けが終わっている。
- per-face 距離ソートは `LLAlphaObject::updateGeometry` 系で sub-face 単位の頂点並べ替えになっている (mesh の中で複数の透明面がある場合)。これは CPU で透明 mesh ごとに走る。

### 3.11 Selection / Manipulator overlay

- `LLSelectMgr` の更新 — `LLAppViewer::idle()` で `deselectAllIfTooFar` (`llappviewer.cpp:6023`) + `LLSelectMgr::updateEffects` (6078)。
- 編集モード時に `gFloaterTools` を中心に highlight render が走る (`pipeline.cpp:4389-4441` の "Render face highlights" zone)。
- 選択面の `gPipeline.markRebuild(face->getDrawable(), LLDrawable::REBUILD_VOLUME)` (`pipeline.cpp:4434`) が走るので、テクスチャ channel を切り替えた瞬間に rebuild がスパイクする。

### 3.12 UI render

- `gViewerWindow->updateUI()` (`llappviewer.cpp:5982`) で全 floater / view の `draw()` 階段降下。
- LLFloater / LLPanel / LLView の `draw()` は単独でも UI スレッドが回らないので main thread が全部やる。
- `LLFontGL::render` (`llfontgl.cpp:152-447`) は `LL_PROFILE_ZONE_SCOPED_CATEGORY_UI`。glyph atlas (`LLFontBitmapCache`) で済む文字は高速だが、改行ごとに draw call が増える。
- HTML chat の `LLViewerTextEditor::draw` (および基底 `LLTextEditor::draw`) は segment ごとに style 切替 + image inline + emoji 描画があるため、長い chat history のフロータが開いていると毎フレーム 0.3〜1 ms 食う体感。
- AYAstorm r4 の chat font live-apply (`fschathistory.cpp`) もこの経路。

---

## 4. 並列化 / 非同期化されている領域

### 4.1 並列に逃せているもの

| 機構 | 何を別 thread で処理 | 連絡経路 |
|---|---|---|
| `LLImageDecodeThread` | JPEG2000 等のテクスチャ decode | `LLQueuedThread` + work item の done callback |
| `LLTextureFetch` ワーカープール | テクスチャ HTTP + cache I/O | LLQueuedThread |
| `LLMeshRepoThread` | mesh header / LOD HTTP, header parse, LOD unpack | mutex + signal、`mLoadedQ` を main thread が drain |
| `LL::WorkQueue ("General")` | 汎用 worker (HTTP coros の一部、`LLAppViewer::postToMainCoro`) | `gMainloopWorkQueue` (`llappviewer.cpp:6422, 6801`) |
| `LLCoros` (Fiber) | log-in / TP のコルーチン、`PositionalStreamMgr` の async open | viewer 内 fiber、main thread context で切替 |
| `LLPhysicsDecomp` thread | mesh の凸分解 (upload preview) | mutex + signal |
| HTTP core thread (`LLCore::HttpService`) | curl 多重化 | callback でメイン or worker に戻る |
| Vivox / WebRTC voice | sub-process or scoped thread | IPC / OS pipe |
| `gAudiop` (FMOD) | audio decode + mixer | FMOD 内部 thread (3 種)、main は idle で push のみ |
| AYAstorm `LLPositionalStreamMulti` decode (r7) | FFmpeg stream decode | 自前 thread + sample queue |

### 4.2 並列化されていない / 候補

- **avatar skinning**: main thread が CPU で全部やる。LL は GLTF/glTF skin の GPU skinning への移行を検討中。
- **particle update**: CPU 全件ループ。compute shader 化 (transform feedback or SSBO) で大幅短縮余地。
- **octree visit / cull**: 単一スレッドで sim 数 × partition 数を逐次。partition 単位で並列化 (task graph) する余地あり (LL の bidirectional contribution の中で議論された記録はないが理論的可能)。
- **HUD text overlap iteration**: 単スレッド O(N²)。
- **vertex buffer upload (glBufferSubData)**: main thread。PBO + worker thread upload に逃せる (driver 依存)。
- **post effects (bloom / SSR / SMAA)**: GPU 寄りなので並列化議論は別だが、CPU の組み立て部分は main thread。

#### 他 viewer の先行事例 (推測)

- **Black Dragon**: avatar 周辺の改修 (impostor 強制 / rigging cull) が活発。基本は main thread on のままチューニングという話が中心。
- **Catznip / Restrained Love**: 上流追随で大きな並列化は入れていない印象。
- **Alchemy**: 自前のシェーダパッチ多数、CPU 並列化の話はあまり聞かない (推測)。
- **CtrlAlt系** などの実験 fork で「shadow を別スレッドで」が試されたことがあるが、LL 本体の GLState は thread-affinity 強くて入っていない。

→ 結論: 「描画スレッド分離」は LL 本流が動いていないので、AYAstorm 単独で踏むには互換性リスクが大きい。並列化候補は「**main thread の CPU 仕事を別 thread か GPU に逃す**」局所改修が現実的。

---

## 5. AYAstorm 独自の描画レイヤ

### 5.1 ParcelHide (`[parcelhide:...]` / `FSRenderHideOutsideParcel`)

#### 統合ガード

中心関数は `LLPipeline::isParcelHideAlive(LLDrawable*)` (`pipeline.cpp:3340-3361`):

```
if (sParcelHideEnabled)  return shouldHideForOutsideParcel(drawablep);
if (sParcelOwnerTagActive) {
    // altitude 範囲指定があれば agent Z でゲート
    if (!sParcelOwnerTagAltRanges.empty()) { /* in_range check */ }
    return shouldHideForOutsideParcel(drawablep);
}
return false;
```

`shouldHideForOutsideParcel` (`pipeline.cpp:3254-3331`) は:

- 早期 return (`isSpatialBridge` / HUD attachment / self attachment) 後、
- `mLastParcelCheckSeq == sParcelCheckSeq` ならキャッシュ参照 (3322-3325) — 同一フレーム内の **per-drawable 1 回** に抑える設計。
- キャッシュミス時は `LLViewerParcelMgr::getInstance()->inAgentParcel(vobj->getPositionGlobal())` で実判定。

#### ガード設置点 (5 サイト)

| 設置箇所 | 役割 | 行 |
|---|---|---|
| `llvovolume.cpp:5996` | spatial group の rebuildGeom で face list から弾く | volume の VBO に乗らない |
| `llvograss.cpp:641` | grass rebuild | grass 面を弾く |
| `lldrawpooltree.cpp:101` | tree pool render | LL tree の draw を弾く |
| `llviewerpartsim.cpp:795-799` | particle source update を抑制 | emit 自体を止める |
| `llhudtext.cpp:133-137` | object hovertext の render を抑制 | floating text 描画を止める |

#### 想定オーバーヘッド

- 1 frame で `isParcelHideAlive` が呼ばれる回数 ≈ 「visible drawable 総数 + active particle source + visible HUDText」。
- visible drawable は典型 sim で 500〜2,000 件。最初の 1 件目は `inAgentParcel` (球面座標から parcel grid index 引き) を呼ぶが、それ以降の同一フレームは drawable のキャッシュフィールド (`mLastParcelCheckSeq` / `mLastParcelCheckHidden`) 参照のみで `O(1)` (3322-3325)。
- `sParcelCheckSeq` は parcel 切替 / TP / settings 変更で `refreshOutsideParcelHiding` が `++` (`pipeline.cpp:3365`)。よって **静止フレームではほぼゼロコスト**、parcel 跨ぎ直後の 1 フレームだけ `inAgentParcel` を全 drawable で 1 回ずつ呼ぶ。
- `inAgentParcel` (`llviewerparcelmgr.cpp` 側) は parcel bitmap lookup なので μs オーダー。500〜2,000 件で 0.5〜2 ms ぐらいの試算 (推測)。
- **問題なさそうな所**: ガードが OR ショートサーキット (`sParcelHideEnabled || sParcelOwnerTagActive`) で抜けるので、設定 OFF + tag 無しなら関数呼び出し 1 回で return。
- **要計測ポイント**:
  - parcel 跨ぎ直後の最初の 5 フレーム
  - パーティクル多数の sim での `LLViewerPartSim::updateSimulation` 内の per-source 呼び出し
  - `llhudtext` で多数 floating text がある region

### 5.2 r13 OBB / mesh raycast occlusion

`indra/newview/llocclusiongeometrymgr.cpp` — タグ `[ayastorm:occlude]` を持つプリムを OBB として登録、mesh の場合は実形状 (triangle list) も抽出して raycast に使う。

#### per-tick の負荷

`LLOcclusionGeometryMgr::refreshOccluders()` (`llocclusiongeometrymgr.cpp:275-358`):

1. `mLastTickTime` 更新 (281-289)
2. 全 `mOccluders` を回り、dead を捨て、生きてるものは position / scale / rotation を最新化 (299-328)
3. `scale_changed || isSelected()` なら `extractTriangles(obj, ...)` を **同 tick 内で同期実行** (320-326)
4. `mPendingExtract` を最大 `kExtractPerTickBudget` 件 (P15.9 で導入、6 件/tick) drain (337-357)

→ TP / login バーストで N 件まとめて register された場合も extract は 6 件/tick に絞られるため、main thread の hitch を平均化。

#### per-source の raycast (`firstHit`)

`LLOcclusionGeometryMgr::firstHit(a, b, ...)` (`llocclusiongeometrymgr.cpp:360-384`) — 線分と全 OBB の AABB-segment 交差テストを順に。OBB ヒットがあれば mesh 側の細粒度 raycast (実装 §15 以降) を呼ぶ。

per audio source 呼び出し回数: 各 channel の listener-側 update 時に 1 回。3D stream + llPlaySound の合計で typical 10〜30 source / tick = 数十 × OBB 数 のテスト。`mOccluders` は `kMaxOccluders = 64` (デフォルト) 上限。

cost 試算 (推測): 30 source × 64 OBB × ~50 ns/AABB-test ≒ 96 μs/tick。tick は `FTM_STREAM3D_MGR_UPDATE` のメインスレッドフレームに乗るので、frame budget では問題にならないレベル。

#### アロケーション / cache miss の観点

- `mOccluders` は `std::unordered_map<LLUUID, OccluderShape>` — UUID hash で per-frame lookup する都度 hash 計算 (内部で 16 byte の hash)。
- `OccluderShape::tris` は `std::vector<LLVector3>` を 3 連発 (a, b, c, a, b, c, ...) で保持。mesh 1 個あたり 2,000 tri cap → 24KB/prim。64 prim × 24KB = ~1.5 MB の余計な heap 常駐 (worst-case)。**問題なさそう** (L2 圧迫しない)。
- `mPendingExtract` は `std::unordered_set<LLUUID>` で `kMaxOccluders` 以下。

#### マークすべき所

- `refreshOccluders` 内の `obj->getPositionGlobal()` / `getScale()` / `getRotationRegion()` は LLViewerObject の getter — 内部で `LLXform` 経路。これを 64 prim 毎 tick やる。**LLXform は world-frame 計算のために region offset を引く**ので、`getPositionGlobal` は安価ではない (推測 ~100 ns/件)。64 件 × 100 ns = 6.4 μs/tick — 問題なし。
- mesh raycast 経路は §15.x の今回追加した実形状実装で、tri count × log(N) (BVH 入っていれば) または線形。BVH は入れていないはず — `Grep` で確認しよう (本書スコープ外、別調査の余地)。

### 5.3 audio 経路 (本書スコープ外だが触りだけ)

- `LLPositionalStreamMgr::update()` は audio update phase で走るので「描画」ではないが、main thread 上 — frame budget は共有。`FTM_STREAM3D_MGR_UPDATE` ラベルで Tracy に乗っている。
- r12.1 の `FSParcelStreamQuality` (parcel music ABR 抑制) は描画には影響しない。

---

## 6. GPU / 並列化 / batching に逃せる候補 (主リスト)

> 各候補:
> - **What**: 現在の CPU 仕事
> - **Where**: 該当ファイル:行
> - **Why GPU/Thread/Batch**: 逃せる根拠
> - **Risk**: LL/Firestorm 上流互換性、PBR 依存、driver 依存
> - **Effort**: S (1〜3日) / M (1〜2週) / L (1ヶ月) / XL (それ以上)
> - **Priority**: 高 / 中 / 低

### 6.1 alpha sort の GPU 化 (Order Independent Transparency)

- **What**: `postSort` で `std::sort(sCull->beginAlphaGroups()...)` を CPU で group ごとに距離 sort、その後 `LLDrawPoolAlpha::renderAlpha` で per-DrawInfo に流す。
- **Where**: `pipeline.cpp:4281-4289`, `lldrawpoolalpha.cpp:583-743`
- **Why GPU**: WBOIT (weighted blended OIT) や per-pixel linked list で sort 自体を消せる。draw 順を CPU で気にしなくて済む。
- **Risk**: 上流の deferred renderer と shader 全部書き換え、AYAstorm の water exclusion / atmospherics pass の挟み込み (`pipeline.cpp:4794-4814`) と衝突。
- **Effort**: L
- **Priority**: 中 — 効果はアバター集会 / 透明 mesh 多数の sim で目に見えるが、上流の PBR/GLTF 移行待ちのほうが筋がいい。

### 6.2 depth-only prepass のスキップ判断見直し

- **What**: 現在の deferred は G-buffer 単 pass、depth-only prepass は明示的に持っていない。だが alpha pre-water / atmospherics の挟み込みで実質 multi-pass。
- **Where**: `pipeline.cpp:4737-4814` (renderGeomPostDeferred)
- **Why GPU**: 早期 Z reject の効きを上げる。
- **Risk**: 効果がシーン依存、効かない sim もある。
- **Effort**: M
- **Priority**: 低 — 既に deferred なので利得は限定的。

### 6.3 rigged mesh の CPU skinning → GPU skinning

- **What**: `LLFace::getGeometryVolume` (`llface.cpp:1192`) 内で rigged mesh の 場合 CPU 側で skinning matrix を適用してから upload (LL 既存の動作)。
- **Where**: `llface.cpp:1192-2230`、`lldrawpoolavatar.cpp:673〜`
- **Why GPU**: vertex shader で `mat4 bones[N]` をサンプルすれば CPU から消える。アバター 1 体あたり 数百μs〜数 ms 短縮。
- **Risk**: LL 本流が GLTF skin に向かっているため、独自実装すると上流取り込みが衝突する。
- **Effort**: L
- **Priority**: 中 — 上流の動きを待つほうが安全。AYAstorm 単独でやるなら r14+。

### 6.4 hovertext / HUD text の atlas batch

- **What**: `LLHUDNameTag::renderText` が per-nametag で複数 draw call (背景 quad + label 行 + 本文行 + shadow 4 オフセット)。N アバターで 20〜80 draw call。
- **Where**: `llhudnametag.cpp:236-450`, `llhudtext.cpp:159-450`
- **Why Batch**: glyph atlas (`LLFontBitmapCache`) は同一テクスチャ。全 nametag の頂点を 1 つの dynamic VBO に連結して 1 draw call にできる (LLFontVertexBuffer の拡張)。
- **Risk**: shadow 描画の OFFSET 計算と LOD ステップ (745-766) の handling、updateAll の overlap iteration が screen rect ベースで動いてるので順序保存が必要。
- **Effort**: M
- **Priority**: **高** — 集会 (30+ アバター) で目に見える効果が出る上、上流とぶつかりにくい局所改修。

### 6.5 particle simulation の compute shader 化

- **What**: `LLViewerPartSim::updateSimulation` (`llviewerpartsim.cpp:717-878`) で per-source per-particle の位置/寿命/カラーを CPU 更新。
- **Where**: `llviewerpartsim.cpp`、`llvopartgroup.cpp`
- **Why GPU**: SSBO + compute shader で 1 draw → 全 particle 更新。LL の particle は 比較的単純な lifetime/velocity モデルなので compute kernel は数十行で書ける。
- **Risk**: AYAstorm の `isParcelHideAlive` ガードを GPU 側に移すのが面倒 (drawable 単位の判定が SSBO 上にない)。CPU 側で source flag を更新して GPU に渡す層が要る。
- **Effort**: L
- **Priority**: 中 — particle 重い sim で 2〜4 ms 短縮見込みだが、上流との分岐が大きい。

### 6.6 octree visit の SoA / SIMD 化

- **What**: `LLSpatialPartition::cull` (`llspatialpartition.cpp` の visit ループ) は OctreeNode の AoS 構造体を 1 件ずつ visit。
- **Where**: `llspatialpartition.cpp:228-555`
- **Why SIMD**: per-node の 6 平面 vs AABB は既に LLVector4a で書かれているが、ノード階層がポインタチェイス。flat array + SoA 化で cache miss 削減。
- **Risk**: 大改修。octree の insert/remove path も全部書き換え。
- **Effort**: XL
- **Priority**: 低 — 改修コストに対して利得が不明確。

### 6.7 reflection probe の更新頻度可変化 (距離 / 重要度ベース)

- **What**: `LLReflectionMapManager::doProbeUpdate` (`llreflectionmapmanager.cpp:731`) は全 probe を順に face ごと更新。
- **Where**: `llreflectionmapmanager.cpp:206-731`
- **Why**: 遠い / 視野外の probe は数 frame に 1 回でいい。
- **Risk**: probe の HDR データに gap が出るとアーティファクト。fade 制御 (`mResetFade`) と相互作用。
- **Effort**: M
- **Priority**: 中 — Firestorm/AYAstorm は probe heavy な builds で hit を取りやすい。Beq が既に `mDynamicProbeCount` を入れているので地続き。

### 6.8 main thread の物理待ちを別 thread に

- **What**: SL の物理は sim 側 (server-side) なので viewer に物理エンジンは無いが、`gAgent.updateAgentPosition` (`llappviewer.cpp:6032`) + `gAgentCamera.updateCamera` (6159) は順序依存で main thread に縛られる。
- **Why**: 真の意味では物理ではなく「sim 同期と camera」だが、TP / region 跨ぎ時にこのブロックが伸びる。
- **Effort**: 不可 (sim と同期が必須なので並列化できない)。
- **Priority**: なし — これは本質的にシリアル。むしろ TP 中の hitching 対策は「描画を 1 フレーム飛ばす」で十分。

### 6.9 GL コマンドの multi draw indirect 化

- **What**: 各 pool の `renderDeferred(i)` (`pipeline.cpp:4699`) が pool 内の DrawInfo を 1 件ずつ glDrawElementsBaseVertex で発行。
- **Where**: `lldrawpool*.cpp` の各 pool、`llrenderpass.cpp` の `pushBatch`
- **Why Batch**: `glMultiDrawElementsIndirect` で 1 call にまとめれば driver overhead が大幅減。
- **Risk**: GL 4.3+ 必須。AYAstorm のサポート最低 GL version は 4.0 〜 4.1 ぐらい (推測 — feature table 確認余地)。
- **Effort**: L
- **Priority**: 中 — シーンの draw call 数を 1/3〜1/10 にできる可能性。LL も検討しているはず。

### 6.10 cull pass の thread 並列化 (region/partition 単位)

- **What**: `updateCull` (`pipeline.cpp:2746-2770`) が region 全件 × partition 全件をシリアル。
- **Why Thread**: partition 同士は独立、`sCull` への push が thread-safe ならば。
- **Risk**: `sCull` (`LLCullResult`) の vector push は thread-unsafe。region 単位は隣接 SIM が並ぶので 4〜9 並列性は出る。
- **Effort**: M
- **Priority**: 中 — drawable 数が多い sim で 0.5〜1.5 ms。

### 6.11 vertex buffer upload の PBO 化

- **What**: `glBufferSubData` を 64KB ブロックで同期 upload (`llvertexbuffer.cpp:1396-1405`)。
- **Why Thread/Async**: PBO + unsync map + worker thread upload で main thread に CPU しか残らない。
- **Risk**: driver 依存。Intel iGPU で unsync map 不安定の歴史がある。
- **Effort**: L
- **Priority**: 中 — TP / rebuild 多発時に効く。

### 6.12 LLFontGL::render の draw call merge

- **What**: `LLFontGL::render` (`llfontgl.cpp:152, 526, 594, 935, 952` などに ZONE)。文字列ごとに `LLFontVertexBuffer` を借りて 1 draw。
- **Why Batch**: 同一 frame 内の同一 font の draw を deferred で集めて end-of-UI で 1 multi-draw。
- **Risk**: scissor / depth / blend state が文字列ごとに変わるので、state buckets を維持する必要。
- **Effort**: M
- **Priority**: 中 — chat console を開いた floater が多い構成で効果が出やすい。

### 6.13 nametag overlap O(N²) の grid 化

- **What**: `LLHUDNameTag::updateAll` (`llhudnametag.cpp:777-810`) の二重ループ × 3 反復。
- **Why CPU 局所最適化**: screen を 32x32 grid に分割し、隣接 cell のみペア比較で O(N) に。
- **Effort**: S
- **Priority**: **高** — local 改修で済む、上流と無関係、集会で効く。

### 6.14 chat history rendering の view recycling

- **What**: `fschathistory.cpp` (FTM_APPEND_MESSAGE) で chat 1 行ごとに `LLTextEditor` segment を生成。大量履歴で memory + draw 共に重い。
- **Why**: 表示外の segment を仮想化 (table view 的に reuse)。
- **Risk**: 検索 / コピー / inline image / emoji の参照が segment 実体に依存。
- **Effort**: L
- **Priority**: 中 — long-session で目に見える効き目だが、改修範囲が広い。

### 6.15 generateImpostor の 共有 FBO + atlas 化

- **What**: 1 アバター = 1 個別 FBO に書く。
- **Where**: `pipeline.cpp:12159` 以降
- **Why GPU/Batch**: 大きな atlas FBO に viewport を切って複数体まとめる。1 frame に複数 update を出すと FBO bind 切替が消える。
- **Risk**: アバターごとに resolution が違うので bin packing が要る。
- **Effort**: L
- **Priority**: 低 — 既に LL が impostor update 自体を絞っているので、bind overhead 削減の絶対効果は小さい。

### 6.16 attachment override 更新の dirty-graph 化

- **What**: `LLVOAvatar::updateAttachmentOverrides()` (`llvoavatar.cpp:7524`) が attachment 着脱で全 attachment を再評価。
- **Why CPU 局所**: 着脱した attachment subtree だけ再評価する dirty-flag を持つ。
- **Effort**: M
- **Priority**: 低 — 着脱は希なイベント。

### 6.17 spatial partition の `octree->balance()` をフレーム間で分散

- **What**: `LLPipeline::updateMove` (`pipeline.cpp:2382, 2390`) で region × partition 全部 balance。
- **Why CPU 局所**: 1 フレームで 1 partition だけ balance、ラウンドロビン。
- **Effort**: S
- **Priority**: 中 — TP 直後の hitch を平均化できる。

### 6.18 `LLViewerObject::getPositionGlobal` の per-frame キャッシュ

- **What**: AYAstorm `shouldHideForOutsideParcel` / r13 occlusion / particle hide で頻繁に呼ばれる。内部で region offset を 3 軸引いて float64。
- **Where**: `llviewerobject.cpp`、`llxform`
- **Why CPU 局所**: drawable に同一フレーム結果をキャッシュ (parcelhide はすでに seq cache 持つので追加コスト無し)、r13 mgr 側でも tick 開始時に bulk 計算。
- **Effort**: S
- **Priority**: 低 — 既に sequence cache がある。

### 6.19 deferred の `pool->prerender()` の 早期スキップ

- **What**: `pipeline.cpp:4638-4645` で全 pool に `prerender()` を呼ぶが、`hasRenderType(poolp->getType())` ならば、という分岐は入っている。
- **Why**: もう既に分岐入ってる。追加余地は小さいが、PBR 移行で pool 数が増えると効く。
- **Priority**: 低

### 6.20 `cleanDeadObjects` / `cleanupDeadDrawables` の 償却

- **What**: `gObjectList.cleanDeadObjects` (`llappviewer.cpp:6055`) と `LLDrawable::cleanupDeadDrawables` (6059) は frame ごと。
- **Why**: 1 frame で全件 sweep する代わりに budget 制で分散。
- **Risk**: 死んだ object への参照が残るので race 注意。
- **Effort**: M
- **Priority**: 低 — 死亡数は普通は少ない。region 跨ぎ直後だけスパイク。

### 6.21 r13 occlusion mgr の OBB array を SoA + SIMD batch raycast

- **What**: `LLOcclusionGeometryMgr::firstHit` (`llocclusiongeometrymgr.cpp:360-384`) が `mOccluders` を unordered_map で順次 traverse して OBB-segment テスト。
- **Why CPU 局所**: 64 OBB を 4 個ずつ SSE/AVX で並列に test。
- **Effort**: M
- **Priority**: 低 — frame budget では問題なしレベル。

### 6.22 mesh 実形状 raycast (r13 P15) の BVH 化

- **What**: `extractTriangles` で triangle list を持つだけ、raycast は全 tri を線形 test (推測 — `llocclusiongeometrymgr.cpp` 後半確認余地)。
- **Why**: BVH (or AABB k-d tree) を一度作って永続化、raycast は O(log N)。
- **Effort**: M
- **Priority**: 中 — 2,000 tri prim が増えるとレイ 1 本に数 μs かかる。

### 6.23 nametag / hovertext の screen culling 先行 + LOD step ダウン

- **What**: `LLHUDNameTag::updateAll` は updateVisibility を全件呼ぶ。
- **Why**: 視野角ベースの早期 reject を強める (`mFadeDistance` を環境に応じて自動短縮)。
- **Effort**: S
- **Priority**: 低

### 6.24 particle system の visible group ベース sleep 強化

- **What**: 現状 visirate=8 で 1/8 更新 (`llviewerpartsim.cpp:830-833`)。
- **Why**: 完全 culled な group は 1/16 や 1/32 に。スクリーン上で 1 px 以下のものは完全 sleep。
- **Effort**: S
- **Priority**: 中 — particle 多発 sim で 0.3〜1 ms。

### 6.25 sun shadow split の動的本数

- **What**: 4 split 固定 (LL deferred の典型)。
- **Why GPU**: 室内・暗所では split 数を 2 に削る、屋外でも遠景 split を解像度ダウン。
- **Effort**: M
- **Priority**: 中 — shadow on の sim でフレーム時間の 15〜25% を占める典型ホット。

→ 候補は 25 件 (列挙ベース)。優先度の集約は §9 へ。

---

## 7. 計測の現状とギャップ

### 7.1 現状見えているもの

- **Tracy 経由**: 上記のカテゴリ (APP / AVATAR / DISPLAY / DRAWABLE / DRAWPOOL / ... ) の `LL_PROFILE_ZONE_*` ゾーンが現状の Tracy capture で全部見える。GPU zone も主要 pass にあり (`generateImpostor` / `generateSunShadow` / `renderGeomDeferred` / `renderGeomPostDeferred` / `reflection manager update` / `doOcclusion` / `rebuildMesh`)。
- **FastTimer (built-in)**: `Ctrl+Shift+1` の FastTimer view (`llfasttimerview.cpp`) が FTM_* ラベルをツリー表示。`FTM_FRAME` を 100% としたフレーム内シェアが見える。
- **PerfStats** (Beq 拡張): `FSPerfFloater` で per-avatar の render time、TooSlow 判定の根拠が見える。
- **GPU timer query**: `LL_PROFILE_GPU_ZONE` の glQuery 結果が Tracy に同期される (driver サポート前提)。

### 7.2 見えにくいもの (今のままだと欠落)

- **glBufferSubData の累積コスト**: per-call zone はある (`llvertexbuffer.cpp:1391`) が、1 frame の累積で何 ms 食ってるかは集計が要る。
- **shadow split ごとの GPU 時間**: 全 `generateSunShadow` が 1 zone、splits 別計測は無し。
- **per-pool の GPU 時間**: `renderGeomDeferred` 全体は GPU zone 有りだが、pool ごとの GPU time は無し。
- **driver overhead** (CPU 側 GL コマンド組立てが GPU 待ちで stall してる時間): 専用の glFinish 系の挿入が必要。
- **アバター 1 体あたりの累積 idleUpdate 時間**: Tracy zone は per-call で出るが、その avatar UUID ごとの sum を Tracy 上で aggregate するのは難しい (text dump 経由)。PerfStats が補完。
- **per-frame の `inAgentParcel` 呼び出し数 + ms**: parcelhide で呼ばれている。zone は貼っていないので独立計測無し。
- **r13 occlusion `firstHit` の per-source ms 累積**: tick 内 zone はあるが per-source 集計は無し。
- **`LLVOAvatar::idleUpdateNameTag` 内の per-name 描画コスト**: zone は coarse。
- **GL state 切替のコスト**: GL state changes の cost を可視化する zone は無し。`LLGLState::checkStates` だけは debug 経路にある。

### 7.3 計測プラン (§6 候補のうち、どれが効くかを実数判定するための)

1. **frame budget breakdown — まずベースラインを取る**
   - 屋外 sim (アバター 5 名) / 集会 sim (アバター 30 名) / 室内パフォーマンス sim / TP 直後 1 秒 の 4 シーンで:
     - Tracy capture 30 秒
     - 1 frame の median と p95 で `FTM_FRAME` の中身を pie chart
   - 「どのカテゴリ (avatar / drawpool / pipeline / spatial / ...) が支配的か」を確定

2. **GPU vs CPU の切り分け**
   - 同じシーンで `RenderResolutionDivisor` を 2 / 4 にして CPU 側時間が動かないことを確認 → 動かない zone は CPU bound、動く zone は GPU/fillrate bound。
   - vsync OFF で実 FPS を見て、framerate cap が CPU 起因か GPU 起因かを切り分け (CPU bound なら vsync OFF で同じ FPS、GPU bound なら伸びる)。

3. **候補ごとの A/B の組み方**
   - **§6.4 nametag atlas**: 30 アバター shore で nametag ON/OFF (`RenderNameTags`) の `LLHUDNameTag::updateAll` zone 比較
   - **§6.13 overlap O(N²) → grid**: 上と同じ条件で zone 内訳
   - **§6.7 reflection probe 可変化**: 異なる `RenderReflectionProbeLevel` で `LLReflectionMapManager::update` の GPU zone 比較
   - **§6.5 particle compute**: heavy particle sim (花火 / fog) の `FTM_SIMULATE_PARTICLES` 比較
   - **§6.11 PBO upload**: TP 直後 5 秒の `glBufferSubData` zone と `flushBuffers` zone の累積を比較
   - **§6.25 shadow split**: シェード on/off の `FTM_GEN_SUN_SHADOW` 差分

4. **AYAstorm 拡張の per-overhead**
   - `parcelhide` ガード: 設定 OFF / 設定 ON で `updateGeom` zone と `updateSimulation` zone を比較。理論的にはほぼ同じはず。
   - r13 occlusion: occluder 0 個 / 10 個 / 64 個 で `FTM_STREAM3D_MGR_UPDATE` の median を測定。

### 7.4 追加すべき計装 (本書スコープ外だが指針)

- `inAgentParcel` 呼び出し回数のカウンタ (LL_PROFILE_ZONE_NUM で frame ごとに値を投げる)
- `glBufferSubData` の累積バイト数を frame counter として
- `LLDrawable::mLastParcelCheckSeq` cache miss 数

---

## 8. 移行リスクと依存

### 8.1 LL 上流の動き (公式 Linden Lab viewer)

直近の LL viewer は:

- **PBR / GLTF material**: 進行中。pipeline.cpp の `gltf_mat` 分岐が現状 alpha pool 等で増えている (`lldrawpoolalpha.cpp:690-742`)。alpha sort 改修系 (§6.1) は GLTF 側との衝突大。
- **Reflection probes**: 既に LL viewer 6.x 系で稼働。`LLReflectionMapManager` の API は LL 側に同期、Beq の `mDynamicProbeCount` パッチが上流に取り込まれる動きはあったかも (要確認)。可変更新頻度 (§6.7) は AYAstorm 独自にやれば OK だが、API の signature 変えると上流追随コストが出る。
- **Vulkan / WebGPU 移行**: 噂レベル。少なくとも viewer 7.x 系では Vulkan 移行の commit は本流に入っていない。当面は GL 4.x 前提で考えてよい。
- **GLTF skin / GPU skinning**: 検討中。§6.3 の rigged mesh GPU skinning は LL 本流とぶつかる可能性大。

### 8.2 Firestorm 取り込みとの衝突懸念

`<FS:Beq/>` / `<FS:Ansariel/>` / `<FS:LO/>` / `<FS:CR/>` / `<FS:PP/>` などのマーカーが Firestorm 由来のローカル改修。AYAstorm はこれらを継承しているので、上流 LL の同じ箇所が動くと merge conflict。

特に衝突しやすい所:

- `pipeline.cpp` の Beq 拡張 (Tracy zone 追加、PerfStats 統合) — 数十カ所。
- `llvoavatar.cpp` の Beq impostor / TooSlow 拡張 — `isVisuallyMuted`, `isImpostor`, `FSImpostorAvatarExclude`。
- `lldrawpool*` の Tracy zone 細分化。

AYAstorm 独自のレイヤ (parcelhide / r13 occlusion / r10〜r12 audio) は基本的に **既存 LL/FS のホットパスの外側にフックを置く**設計なので、衝突は少ない。例外:

- `pipeline.cpp:3113-3361` の parcel hide ロジック (300+ 行のブロック追加)
- `llvovolume.cpp:5994-6000`, `llvograss.cpp:641-643`, `lldrawpooltree.cpp:101-103`, `llviewerpartsim.cpp:789-800`, `llhudtext.cpp:133-137` の 5 サイト

これらは LL 上流の同じ関数に LL 側の修正が入ると merge 衝突を起こす。今後 LL が `LLVOVolume::rebuildGeom` を改修したら手動 merge 必須。

### 8.3 AYAstorm 内部のフィーチャ間衝突 (描画改修候補が壊しうるもの)

- **§6.1 OIT alpha**: r10〜r12 audio は描画と無関係なので無事。**r13 occlusion** は OBB の cyan 線描画 (selection overlay 系) を持つので、selection highlight 経路と相互依存はあるが alpha sort 側の改修と直接ぶつかる箇所は無い (推測)。
- **§6.4 nametag atlas**: nametag は audio や occlusion とは独立。安全。
- **§6.5 particle compute**: AYAstorm の `[parcelhide:...]` ガードを GPU 側 (SSBO) に逃す必要があり、ガード判定経路の二重実装になる。後段の議論材料。
- **§6.6 octree SIMD**: r13 occlusion mgr は octree を使わない (自分の OBB unordered_map)。衝突無し。
- **§6.7 probe 可変化**: AYAstorm 拡張と無関係。
- **§6.10 cull thread 並列化**: r13 occlusion mgr が `LLPipeline::updateCull` から呼ばれてはいないが、tick が main thread の前提なので thread モデルが変わるなら r13 mgr の `gObjectList.findObject` の thread-safety を再確認。

### 8.4 r14+ 想定 (Steam Audio / SOFA HRTF) との依存

- Steam Audio は OBB 抽出 + listener 経路を要求するため、§6.21〜§6.22 の occlusion 計算強化は地続き。
- SOFA HRTF は audio path であり描画とは無関係。
- §6.5 particle compute は GPU 側で audio reactive な particle (将来) に有利。

---

## 9. 議論用たたき台 (punch list)

| # | 候補 | 工数 | 効果見込み (推測) | リスク | 依存 |
|---|---|---|---|---|---|
| A | §6.4 nametag/hovertext の atlas batch | M | 集会 sim で 0.5〜1.5 ms 短縮 | 中 (LOD/順序維持) | 無し (独立) |
| B | §6.13 nametag overlap の grid 化 | S | A と組み合わせて updateAll 全体で 0.3〜1 ms 短縮 | 低 | A と相互補完 |
| C | §6.7 reflection probe 更新頻度可変化 | M | probe heavy sim で GPU 1〜3 ms 短縮 | 中 (probe fade / HDR gap) | LL probe API 安定 |
| D | §6.25 sun shadow split の動的本数 | M | shadow on で 1〜2 ms 短縮 | 中 (シェード品質劣化) | 無し |
| E | §6.10 cull pass の region 並列化 | M | drawable 多数 sim で 0.5〜1.5 ms 短縮 | 中 (LLCullResult の thread safety) | 無し |
| F | §6.17 octree balance のラウンドロビン | S | TP 直後 hitch 平均化 | 低 | 無し |
| G | §6.24 particle visible-group sleep 強化 | S | particle 多発 sim で 0.3〜1 ms 短縮 | 低 | 既存 visirate ロジックの上に乗る |
| H | §6.22 r13 occlusion mesh raycast の BVH 化 | M | occluder 多数で raycast μs 短縮、frame budget では小さい | 低 | r13 audio 経路と整合 |
| I | §6.11 vertex buffer の PBO async upload | L | TP 直後 / rebuild 多発で 1〜3 ms 短縮 | 高 (driver 依存、Intel iGPU テスト要) | 無し |
| J | §6.9 GL multi draw indirect | L | draw call 多シーン (UI 重複 + alpha 多) で 1〜3 ms 短縮 | 中 (GL 4.3+ 必須、driver 互換) | featuretable 改修 |

優先候補は **A + B** (合わせて M+S、独立で安全、効果が体感)。次点で **F** (S だが TP 体感に効く) と **G** (S だが particle 重い時に効く)。

GPU 系の本格改修は **C** (probe) と **D** (shadow) が手堅い。

**長期投資** は **I** (PBO) と **J** (multi draw indirect)。LL 上流が動かない限り AYAstorm が単独でやる価値あるかは議論余地。

---

## 10. 参考ファイル一覧 (調査で参照したもの)

### 描画パイプ本体

- `indra/newview/pipeline.cpp` (12,978 行)
- `indra/newview/pipeline.h`
- `indra/newview/llviewerdisplay.cpp` (2,088 行)
- `indra/newview/llviewerwindow.cpp`
- `indra/newview/llappviewer.cpp` — idle ループ

### 空間 / drawable

- `indra/newview/llspatialpartition.cpp` (4,416 行)
- `indra/newview/lldrawable.h` — REBUILD_* flags
- `indra/newview/llface.cpp` — `getGeometryVolume`

### Volume / VObject

- `indra/newview/llvovolume.cpp` (7,321 行) — updateGeometry / rebuildMesh / genDrawInfo
- `indra/newview/llvoavatar.cpp` (13,338 行) — idleUpdate / updateCharacter / updateImpostors
- `indra/newview/llvopartgroup.cpp`
- `indra/newview/llvograss.cpp`

### Drawpool / レンダーパス

- `indra/newview/lldrawpoolalpha.cpp`
- `indra/newview/lldrawpoolavatar.cpp`
- `indra/newview/lldrawpoolbump.cpp`
- `indra/newview/lldrawpoolsimple.cpp`
- `indra/newview/lldrawpoolterrain.cpp`
- `indra/newview/lldrawpooltree.cpp`
- `indra/newview/lldrawpoolwlsky.cpp`

### Particle / HUD text

- `indra/newview/llviewerpartsim.cpp`
- `indra/newview/llhudtext.cpp`
- `indra/newview/llhudnametag.cpp`
- `indra/newview/llhudobject.cpp`
- `indra/newview/llhudmanager.cpp`

### Texture / Mesh / Asset

- `indra/newview/lltexturefetch.cpp` (4,438 行)
- `indra/newview/llviewertexturelist.cpp`
- `indra/newview/llmeshrepository.cpp` (6,477 行)

### Reflection probe / Lighting

- `indra/newview/llreflectionmapmanager.cpp`
- `indra/newview/llheroprobemanager.cpp` (推測 — 参照のみ)

### Vertex buffer / GL

- `indra/llrender/llvertexbuffer.cpp`
- `indra/llrender/llvertexbuffer.h`
- `indra/llrender/llglslshader.cpp`
- `indra/llrender/llrender.cpp`
- `indra/llrender/llfontgl.cpp`
- `indra/llrender/llfontvertexbuffer.cpp`

### Profiler 基盤

- `indra/llcommon/llprofiler.h`
- `indra/llcommon/llprofilercategories.h` — 26 カテゴリ定義
- `indra/llcommon/lltrace.h`
- `indra/llcommon/lltracerecording.cpp`
- `indra/llcommon/llfasttimer.cpp`
- `indra/newview/llfasttimerview.cpp`

### AYAstorm 拡張

- `indra/newview/llocclusiongeometrymgr.cpp` — r13 OBB / mesh raycast
- `indra/newview/llocclusiongeometrymgr.h`
- `indra/newview/llpositionalstreammgr.cpp` — audio mgr (`FTM_STREAM3D_MGR_UPDATE`)
- `indra/llaudio/llpositionalstream.cpp` / `llpositionalstreamstereo.cpp` / `llpositionalstreammulti.cpp`
- `docs/ayastorm-render-hide-outside-parcel.md` — parcelhide 仕様
- `docs/ayastorm-r13-occlusion.md` — r13 occlusion 実装記録
- `docs/ayastorm-fix-parcel-hide-stale-position.md` — TP 直後の parcelhide キャッシュ不整合対策
- `doc/spec_obb_occlusion.md` — r13 occlusion 仕様書

### コミット履歴で参照すべきもの

- `986464b354` r13 P15.10 (TP/login freeze 対策)
- `2887e598f7` r13 P15.9 (extract 遅延ドレイン + Desc 同値スキップ)
- `605a479256` r13 P15.8 (mesh raycast 反映同期)
- `409dea6f53` r13 P15.6 (実形状 mesh raycast 仕様)

---

## 11. 残課題 / 不足調査

本書で**触れたが、深掘りしていない**もの:

- **LLVOVolume::genDrawInfo の細部** (`llvovolume.cpp:6614-7100`): per-face alpha 振り分け / texture indexing / batch boundary の決定ロジック。draw call 数の支配要因。
- **LLPipeline の RT (render target) サイズ管理**: `mRT->deferredScreen` / `mRT->screen` / `mPostMapBuffer` の resize ロジック (`pipeline.cpp:919-1090`)。`resizeScreenTexture` の頻度が問題になるケース。
- **GLTF Material / PBR の pool 分岐**: `lldrawpoolpbropaque.cpp` 周辺、PBR 移行のコスト。
- **Hero probe (mirror) の cost**: `LLHeroProbeManager`、mirror ON 時の per-frame 6 面 cubemap render。
- **Shadow の per-split GPU breakdown**: 4 split の各々が GPU でどれだけ食ってるか。LL_PROFILE_GPU_ZONE が split 別になっていない疑い。
- **`LLViewerTextureList::updateImages` の内部**: priority sort + decoded image 取り込みの実コスト測定。
- **PBR refprobe の更新スケジューラ** (`mPaused`, `mResumeTime`, `mResetFade`): 動的可変化 (§6.7) と相互作用。
- **r13 occlusion mesh raycast のアルゴリズム詳細** (`llocclusiongeometrymgr.cpp:400-712`): BVH があるか線形か (推測: 線形)、tri index buffer の最適化余地。
- **Linden の `LLVertexBufferPersistent` / `LLVertexBufferMapped` 系の有無**: 上流に持続マッピング型が入っているかコード上探索余地あり。
- **`LLDrawPoolAvatar::renderTransparent`**: 透明アバターの depth-only pass (本書では触れず)。
- **Wind effect / Particle wind**: 別計装は浅い。

これらは AYA さんとの会話で「ここも掘っといて」と指示があれば本書 v2 で追補。

---

(以上)
