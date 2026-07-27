# 直列破壊 Phase B — camera scene pass 記録の worker 並列化(詳細設計)

- 状態: **設計者起草ドラフト 2026-07-27・approve 前**。HEAD `18580032416`(Phase A 実装完了・crowd gate 待ち)。
- 位置づけ: `docs/vknative_serialization_root_cure_design.md` §3 表の **Phase B** の JIT 詳細設計(同 doc §5 が予告した起草物)。Phase A の機構(seed / pin / lane CB / pre_cmds join)の直接拡張であり、新基盤の発明ではない。
- 前提: **Phase A crowd gate PASS 後に着手**(§0 の標的実測も同走行で採取)。
- 真実源: HEAD 実コード。トレース台帳 = scratch `trace_phase_b_readiness.md`(file:line → 事実の全 hop)。

---

## 0. 標的と実測前提

- 軽シーンの支配項 = display 記録 20-30ms/f(root cure doc §0.1)。うち camera scene = `ph(10)` renderGeomDeferred + `ph(11)` 内 renderGeomPostDeferred(forward/alpha)。
- **着手前の標的自己実測(必須・未)**: crowd gate 走行の PERF_LOG `fam` 表で camera 側の hot pool(mat / bump / simple 系 / alpha)と draws/f を確定する。**Phase A gate 走行がそのままこのデータを産む**ので追加走行は不要の見込み。fam の帰属が本設計の族分割(§3.3)と食い違えば族分割を実測に合わせて更新してから実装に入る。
- 期待配当: renderGeomDeferred の per-draw 記録(worker 適格分)が ~1/N + join へ。B2(postDeferred)は本 doc では概略のみ(§6)。

## 1. 設計テーゼ

**B1 = renderGeomDeferred(gbuffer pass)の worker 適格 pool-pass を record job 化する。** gbuffer は自己完結(deferredScreen 書き・consumer = lighting は main CB 内で後段)なので、Phase A と同じ **pre_cmds モデル**(worker CB は main CB の前に実行)がそのまま使える。doc §5 が想定した secondary CB / `vkCmdExecuteCommands` 拡張は **B1 には不要**。

- job 粒度 = **pool-pass 単位(1 shader program = 1 job)**。粒度の根拠は §4.1(per-program UBO 排他)。
- B2(renderGeomPostDeferred)= pre_cmds モデル不可(§6)。B1 gate 後の後続。

## 2. 成立条件(HEAD トレース済・file:line)

| # | 条件 | 根拠 |
|---|---|---|
| 1 | worker CB は main CB より前に実行 | llvkloader.cpp:6170/6220/6337(pre_cmds)。async uiscene モードでも cjob(pre_cmds)→ pjob → apjob の enqueue 順で scene CB より前 ✓ |
| 2 | RT scope は LOAD op で再入可能 | llrendertarget.cpp:551/558(bindTarget = LOAD 固定)・llvkloader.cpp:7246-7250(scope 逐次切替) |
| 3 | 記録経路の thread 安全 | gGL / gGLModelView / gGLLastModelView = thread_local(llrender.cpp:47-50)・fctx = thread_local(llpipelineframecontext.cpp:28)・UBO dynamic impl / DrawData scratch / matrix ring = Phase A で thread_local / atomic 化済 |
| 4 | record job 中の per-draw set 解決 | buildAndOverrideScenePerDrawSet = record job 中 scratch DrawData + params 直接 heap slot 解決(lldrawpool.cpp:635-643)+ populate の seed 分岐(llglslshader.cpp:3528-3559)。miss = C_RECORD_JOB_PULL で fail-visible |
| 5 | rigged palette | pin + prewarm(pipeline.cpp:13881-13888)+ objectSkinLookupEntry frame cache(mutex 済)= Phase A 実証済 |
| 6 | kill switch | `AYASTORM_MT_THREADS=1` → rw=0 → dispatchRecordJob inline 退化(llvkloader.cpp:5689)= 既存意味論そのまま |

## 3. 実装設計

### 3.1 dispatch 位置と骨組(pipeline.cpp renderGeomDeferred)

main camera(llviewerdisplay.cpp:1151 経路)かつ `mt_camera_capable = !gCubeSnapshot && vkInit && shouldUseVulkanRender && isBindlessActiveVk`(Phase A の mt_shadow_capable と同型)のとき:

1. deferredScreen.bindTarget() 後・pool loop 前に **seed ensure**(§3.2)と **pin sweep**(worker 族 pass の LLCullResult render map を全 pin + palette prewarm = Phase A pinShadowWorkerDrawInfos の camera 版)。
2. deferredScreen.clear()(llviewerdisplay:1115)は dispatch 成立時 **skip** し、**chunk 0 job が CLEAR load op で開始**(Phase A の shadow clear 移譲と同型)。CPU layout tracker は dispatch 時に main で更新(Phase A :15315 の setVkDepthLayout と同型を color 4 枚 + depth に)。
3. pool loop 走査中、worker 適格 pass(§3.3)に到達したら pool の begin/render/end を **job body へ包んで dispatchRecordJob**(main は当該 pass を skip)。非適格 pass は従来どおり main 実行。dispatch 失敗(seed 不成立等)= 当該 pass を main inline 実行 = fail-soft。
4. **join = renderGeomDeferred 末尾**(rt.flush() より前)。join 後に pin clear。main 残置族・doOcclusion と worker job が並走する。

### 3.2 CameraRecordCtx / seed(Phase A ShadowRecordCtx の拡張)

- ctx = view/proj(camera)・gGLLastModelView・result(camera cull)・rt=&deferredScreen・**color image/view ×4 + depth**・width/height・**initial_layout(chunk 0 のみ CLEAR + clear color)**・pass id・seeds。
- job preamble(recordShadowWorkerFamilies と同型の save/restore): fctx **setActiveRT(getFrameRT())** + setCullResult + mRenderingDeferred(worker fctx は初期値 nullptr/false のため必須)・sBoundTarget・sCurResX/Y・viewport・perf tag。行列 = gGLModelView memcpy + gGL matrix stack へ load(proj は gGL の thread_local stack のみ・共有 global gGLProjection には触れない = Phase A 前例)。
- 本体 = `poolp->beginDeferredPass(pass)` → `renderDeferred(pass)`(worker 族の draw loop)→ `endDeferredPass(pass)` を worker 上で実行。bindDeferredShader の texture unit bind は thread_local gGL 上で VK 的に inert(per-call set は seed で確定済・heap slot は params 直接解決)。
- 末尾 = endDynamicRendering + **attachment barrier(color 4 + depth の WAW/RAW・cmdShadowDepthWawBarrierVk の deferredScreen 版)** = chunk→chunk と chunk→main CB の順序をこの barrier が張る。
- **seed ensure** = 各 worker 族 shader を main で bind(begin pass 状態を再現)→ vkResolvePerCallSetForDraw → set/shape snapshot → 復元(ensureShadowWorkerSeeds の拡張)。deferredScreen bound 状態下で実施。ensure コストが fam で見えたら「set 内容不変 frame の seed 持ち越し」を後続最適化として起票(初版は毎 frame ensure = Phase A と同じ)。

### 3.3 族分割(初版・実測で更新)

- **worker(bindless heap 適格 = mVkUsesBindlessHeap で機械判定)**:
  - materials 24 pass(gDeferredMaterialProgram 12 種 ±rigged・lldrawpoolmaterials.cpp:57-244)= crowd の avatar 着装の本体想定
  - simple / alphamask / fullbright-alphamask の **rigged** pass(pushRiggedBatches 経路・program = mRiggedVariant で main の MDI static と別 object)
  - alphamask / fullbright-alphamask の static per-draw pass(camera MDI 非対象 = isCameraMdiPass は SIMPLE/FULLBRIGHT のみ)
- **main 残置**: cull/stateSort・PASS_SIMPLE/FULLBRIGHT static(camera MDI = 既に安い)・avatar pool(系 mesh per-draw)・tree/terrain/grass(per-draw texture 非 heap)・GLTF PBR(set1 直 sampler 非 heap)・doOcclusion・bump(初版は main = 母集団小の想定・fam 実測で hot なら worker 昇格を判断)
- **適格規則(不変条件)**: worker 族 pass の shader は ①bindless heap ②**in-frame 生成 RT を sample しない**(shadow map / deferredLight 等の同 frame RT read は pre_cmds では順序不成立 = §4.3)③per-program UBO を worker 内でのみ書く(§4.1)。

### 3.4 GPU 実行順(設計上の完全列)

pre_cmds: [shadow jobs(Phase A)] → [camera chunk 0 = CLEAR + draws + barrier] → [camera chunks 1..k(LOAD)+ barrier] → main CB: [probe/impostor 等] → [main 残置 gbuffer draws(LOAD)] → [doOcclusion(depth 完全)] → [lighting(gbuffer 完成読み)] → …。depth test 済み opaque は chunk 間で可換 = 描画結果 byte 同一。

## 4. 排他不変条件(本設計の生命線)

### 4.1 per-program 状態は「1 program = 1 thread」
`writeMaterialFPerDrawUBO`(llviewershadermgr.cpp:436-437)は shader object の UBO slot cursor を per-draw で回す = 同一 program の並行記録は race。**pool-pass 粒度 job がこの排他を構造的に保証する**(materials は pass=program 1:1・rigged は別 object)。Phase A 申告「worker 族に per-draw per-program uniform を足すなら lane 化必須」の充足形。**pass 内 range 分割(B1b)は per-program UBO ring の lane 化を実装するまで禁止。**

### 4.2 worker 族と main 族の program 集合は disjoint
window 中に main が bind してよいのは main 残置族の program のみ。設計上の確認済: MDI static(base program)vs worker rigged(mRiggedVariant)= 別 object・occlusion = gOcclusionProgram・avatar/tree/terrain/GLTF = 別 program。実装時に worker 族リストと main 残置経路の bind を全数照合し、重複があれば族を移す。

### 4.3 in-frame RT の read は pre_cmds に置けない
shadow map の SHADER_READ 遷移(pipeline.cpp:15341)は main CB 記録 = camera worker CB(pre_cmds)より後に実行される。∴ worker 族 shader に in-frame RT sampler が現れたら適格外(§3.3 ③)。gbuffer 族は enableTexture 判定で当該 channel 非所持(pipeline.cpp:11450-11524 は -1 skip)= 非該当を実装時に seed ensure で機械確認(ensure 中に in-frame RT bind が発生したら当該 shader を worker 族から除外)。

### 4.4 同値並行書きの容認(Phase A 申告の継続)
texture bind 統計 field・bind 時の定数 per-program 書き(:11565-11598 は 1 program 1 job で排他)。新規の容認項目は増やさない。

## 5. gate(命題様式)

- **命題**: 「camera gbuffer の worker 適格 pool-pass の command 記録を main 直列から pool-pass 単位 worker 並列へ移しても、描く物・cull・texture・rigged pose は byte 同一(記録先 CB と GPU 内実行順のみ変化・opaque depth test で可換)、frame time のみ短縮」。
- 検証: 視覚同一 + validation 0 + device-lost 0 + 診断起動(VKC)全層沈黙(C_RECORD_JOB_PULL 増分 0 含む)+ `ph(10)` wall 短縮 + `rw` 稼働 + mlp/pe で GPU 待ち非増 + 新規 alarm 0(default-deny)。
- kill switch A/B = `AYASTORM_MT_THREADS=1` vs 既定・同一シーン同所。
- **実効設定確認欄**: RenderShadowDetail(Phase A 併走条件)・av 数・シーン(crowd / 軽)・bindless 活性・RenderDepthPrePass=false(既定)。

## 6. B2 概略(postDeferred・後続・本 doc では着手しない)

renderGeomPostDeferred は pool 走査に SSS/atmospherics/water haze/exclusion の fullscreen 効果が interleave(pipeline.cpp:5981-6033・screen RT read/write)+ alpha は順序依存 = pre_cmds モデル不可。機構候補 = main CB の frame 内分割(cmd 列 submit)or dynamic rendering suspend/resume or secondary CB。**B1 gate の実測(postDeferred が残存支配項か)を見てから JIT 起草**。

## 7. 段階と工程

1. **B1a(本設計)**: pool-pass 粒度 job(materials 24 + simple 系 rigged + alphamask 系 static)。ctx/seed/pin/clear 移譲/barrier/join。
2. B1 gate(§5)→ 足場整理。
3. **B1b(条件付き)**: fam 実測で単一 pass が wall を支配する場合のみ、per-program UBO ring lane 化 + pass 内 range 分割。
4. B2 JIT 起草(§6)。

## 8. 縮小・省略・解釈申告

1. **B1 は main camera の renderGeomDeferred のみ**。probe/水面(pipeline.cpp:16021-16041)・HUD は gCubeSnapshot / 呼び元条件で従来直列(Phase A と同じ縮小)。
2. **bump / tree / terrain / avatar / GLTF PBR = main 残置**(非 heap or 母集団小の想定)。bump の worker 昇格は fam 実測が hot を示した時のみ(申告: 初版は安い方に寄せた)。
3. **postDeferred(B2)は概略のみ**(§6)= 分割着手。
4. seed の毎 frame ensure コスト(~24 bind + resolve)は未実測 = fam で見えたら持ち越し cache を起票(初版は実装しない)。
5. doOcclusion の GPU 実行タイミングが「全 worker chunk の後」へ移る = query 対象 depth が従来(pass 途中)より完全になる方向の変化。visibility は保守側にのみ動く(隠れ判定が増えるのは depth が増えた分だけ = 正しい遮蔽)= 品質非劣化と解釈。gate の視覚同一で検収。
6. 未確定 U(再開トリガー付き):
   - **U-B1**: worker 族 shader の per-call set に in-frame RT channel が本当に無いこと(§4.3)。再開トリガー = 実装時 seed ensure に検査を組み込み機械確認。
   - **U-B2**: fam 計器の worker 記録分の帰属(fam_us は main の dispatch 時間しか見えなくなる)。再開トリガー = 実装時に job 側で pass 別 us を計上する既存 VkPerf 欄拡張の要否判断。
   - **U-B3**: crowd fam 実測による族分割の最終確定(§0)。再開トリガー = Phase A crowd gate 走行のログ。
