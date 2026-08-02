# set1 UBO 供給モデル(層1 設計正本)

> 制定 2026-08-02(設計者・AYA 指示による doc 化)。位置づけ = Material D 段階計画の **PP1 層**: GL uniform 移植以来 未設計だった「shader への uniform データ供給」を契約化する。Shadow 工事の F1-F5(layout/sync/record foundation)と同型 = **層を設計として完成させ、Material D(binding0 block 撤去 + per-draw-indexed 化)が層の正しさの最終証拠になる**。全 file:line は 2026-08-02 HEAD(`d181c6357b8`)接地。
> 段階 = PP1(本層)→ PP2(DrawData slot 契約)→ D(行使段)。関連 = `docs/vknative_bind_redesign.md` §7 / `docs/vknative_pass_resume_contract.md`(F2・別層)。

## 0. 一文モデル
**set1 の UBO binding には供給者が 5 族あり、族ごとに「誰が・いつ・どこへ書き・どの機構が draw に届けるか」が異なる。従来この分類は暗黙(発見的検出 + 命名の混乱)だった。本書がそれを規約に昇格する。**

## 1. 供給 5 族(set1 UBO の全分類)
| 族 | 供給者 | binding | 例 | 届く機構 |
|---|---|---|---|---|
| **A: shader-owned per-program** | shader 毎の専用 write 関数(pipeline.cpp/drawpool 群 等) | **0 規約**(例外 = 下記 §3) | MaterialF / AlphaF / GlowF / PointLightPerDraw | shadow → draw 毎 arena resolve(§2) |
| **B: loader-owned shared 静的** | LLVKLoader の accessor(登録表 = llglslshader.cpp:2864-2952 の add_ubo) | 8,9,10,11,12,14,15,16,18,22,26,28,30,31,38,52 等 | WindlightAtmos(8) / DeferredUtil(30) / ShadowUtil(31) | desc build が accessor で解決(llglslshader.cpp:3845-3857) |
| **C: loader-owned shared dynamic** | getSharedDynamicUBOForBinding(llvkloader.cpp:8264-)+ ring 実装(:7821 macro) | 39,45,46,48,49,51,53(+54) | ReflectionProbeF(39) / SSRUtil(49) / ObjectSkin(46) | dynamic offset collect(llglslshader.cpp:3284-3297) |
| **D: GLTF 明示引数** | buildAndOverrideScenePerDrawSet の gltf_materials_ubo / gltf_geometry_ubo(lldrawpool.cpp:586-590) | 7, 44 | Asset_GLTFMaterials(7) / Skin_GLTFJoints・Asset_GLTFNodes(44) | 呼び出し引数で直接供給(per-program 検出から明示除外 :3020) |
| **E: blockless** | 供給不要 | —(phantom binding0 のみ) | Diffuse 等 createVkPipeline(0) の全 shader | dummy 供給(§4) |

命名注意(歴史的混乱・是正はしない): サフィックス `_PerProgramBind` は族 A/B/C の全てに現れる。**族の判定は名前でなく本表(binding + accessor 表)で行う。**

## 2. 族 A の機構と契約(本層の中核)
### 2.1 機構(as-built・全枝トレース済)
- **生成**: `createVkPipeline(size)` の明示引数(llviewershadermgr.cpp 全呼び出し)。size は block sizeof の人手転記。size>0 → 専用 UBO + shadow 生成(llglslshader.cpp:3173-3191)。
- **検出**: 「宣言済 UBO・accessor 無し・binding∉{7,44}」の最小 binding = per-program binding(:3011-3028)。
- **⭐ 核心**: binding0 は layout 上 **無条件 DYNAMIC**(add_ubo(0)・:2857-2858)→ **block@0 を持つ全 shader は `perProg==0 && dynCount>0` = arena 経路**に入る。
- **書込先の単一化**: arena 経路 shader は生成時に `mVkPerProgramUBOMapped` / `mVkActivePerProgramUBOMapped` が **shadow.data() に差し替わる**(:3184-3189)。∴ コード上に併存する 2 idiom —— ①直書き `memcpy(shader.mVkPerProgramUBOMapped, …)` ②`rotatePerProgramUBOSlot()` → `mVkActivePerProgramUBOMapped` —— は**両方 shadow に書いており等価**(idiom 差は GL 移植の化石・意味論は単一)。
- **rotate の実効**(:3198-3211): arena 経路では ring を回さず generation bump + desc offsets dirty のみ。
- **消費**: draw 毎に `vkResolvePerProgramForDraw`(:3385-3416)= `allocPerDrawUBOSlice`(frame-ring bump allocator・atomic cursor・overflow block・llvkloader.cpp:7438-)+ **shadow 全量 memcpy** + dynamic offset。
- **fallback 欠陥(登録済)**: arena 失敗時 `C_PP_FALLBACK_LOSSY`(:3409-3414)= base UBO 直書き = in-flight GPU read と競合し得る lossy。Material D で material 分は構造消滅・残余は台帳管理。

### 2.2 契約(規約として宣言)
1. **書込契約**: 族 A の block 内容は「**draw の前に shadow へ書けば、その draw の resolve が拾う**」。書き手は shader 専用 write 関数のみ(汎用 setUniform は VK では族 A block に到達しない)。新規コードの idiom は直書き型(`mVkPerProgramUBOMapped` 宛)に統一(rotate 呼びは不要・害もない)。
2. **arena 契約**: slice は frame-ring の一時領域・当該 draw 限り有効。frame 跨ぎ参照禁止。
3. **用途 2 分類**: 族 A は (a) **per-bind 定数**(post/fullscreen 系 = frame 数回書き)と (b) **真の per-draw params**(PointLightPerDraw/SpotLightPerDraw = light volume 毎)を含む。(b) のうち **scene pool の大量 draw(material 等)だけが per-draw コストの癌** → per-draw-indexed(DrawData slot)への移行対象(= Material D)。light 系は draw 数が少なく arena per-draw が設計通り = 移行対象外。

## 3. 族 A 例外: perProg≠0(ring 供給)
- **母集団 = 1 本**(全 shader ソース census 2026-08-02): `MultiPointLightF_PerProgramBind` @ **binding1**(multiPointLightF.glsl)。binding1 は標準表では sampler だが、catch-all(:2980-2996)が宣言済み未登録 binding を **plain UBO(非 dynamic)** として layout に追加 → 検出が perProg=1 と判定。
- この族の機構: phantom binding0 は erase(:3030-3035)。書込は **rotate 必須**(`rotatePerProgramUBOSlot` :3213-3248 が frame×index の ring buffer を回し、`mVkActivePerProgramUBO` を切替)→ desc build(:3827-3837)が active buffer を descriptor に書く。**非 dynamic block を frame 内複数 bind で使うための機構 = 生きている・撤去禁止**。
- **契約**: perProg≠0 族の書き手は「rotate → `mVkActivePerProgramUBOMapped` へ書く」を厳守(直書き idiom は base UBO に当たり ring と不整合 = 禁止)。現行の書き手(pipeline.cpp の multi-light loop)は遵守済み。

## 4. 族 E: phantom binding0 + dummy 契約
- add_ubo(0, keep)(:2857)が**全 shader の set1 layout に binding0(DYNAMIC)を常設**する。block 無し shader では:
  - desc build: N==0==perProg → resolve が size 0 で即 false(:3389-3391)→ descriptor 書込 skip(:3866)。
  - dynamic collect(:3266-3282): binding0 の write が無い場合 **arena buffer size64 offset0 の dummy descriptor** を書く。offset 恒等 0。
- **契約**: 「set1 layout の binding0 は常設・未宣言 shader には有効 dummy が供給される・shader が binding0 を読まない限り安全」。Material D で非 BLEND material 変種はこの族へ合流する(= 新規機構ゼロの移行)。

## 5. 検出の規約化と計器
- 発見的検出(:3011-3028)を「**binding0 規約 + 例外 1 本(§3)**」として明文宣言。新 shader が per-program block を書く場合は binding0 に置くこと(それ以外は catch-all + ring 族になる = 意図がなければ設計違反)。
- **計器 = CLASSIFY_AUDIT**(perf log 起動時・:3070-3089): shader 毎に perProg / dyn 一覧 / UNRESOLVABLE を出力 = 本契約の検証器。resolvable 集合(:3076-3078 = 0,39,45,46,48,49,51,53)は族 C の表と一致すること。
- 既知 alarm: `C_PP_FALLBACK_LOSSY`(§2.1)/ `C_UBO_SLICE_FAIL` / `C_REFRESH_*` = 供給契約破れの fail-closed 検出。

## 6. 母集団 census(2026-08-02・shader ソース全数)
set1 std140 uniform block の全出現(binding 順): 0 = 族 A 約 50 block(*_PerProgramBind + PointLightPerDraw/SpotLightPerDraw)/ 1 = MultiPointLightF(§3)/ 4 = SMAABlendWeightsF(族 B・feature 依存 accessor :2862-2869)/ 7,44 = GLTF(族 D)/ 8-31,38,52 = 族 B / 39,45,46,48,49,51,53,54 = 族 C。

### 6.1 族 A write site 全数表(hop B・2026-08-02 完了)
再現手順(census の正)= repo sweep 2 本: `grep -rn mVkActivePerProgramUBOMapped indra --include=*.cpp`(rotate idiom)/ `grep -rn mVkPerProgramUBOMapped indra --include=*.cpp`(direct idiom)。memcpy 実書込の分類結果:
- **rotate idiom(27 site)**: lldrawpoolbump 871 / rlveffects 339 / llheroprobemanager 462 / llreflectionmapmanager 938,986 / lldrawpoolalpha 914,1332,1346 / lldrawpoolwater 311,334 / llviewershadermgr 439,468(writeMaterialF 系)/ lldrawpoolavatar 938 / pipeline 9913,11769,12283,12320,12592,12679,12750,12807,13025,13226,13303 / lldrawpoolwlsky 313,356 / gltfscenemanager 649。
- **direct idiom(44 site)**: llscenemonitor 358,427 / llspatialpartition 2344 / pipeline 5571,5620,6645,6670,6697,6738,6750,6774,6797,6930,6942,9413,9463,9587,9703,9747,9846,9986,10087,10143,10185,10266,10384,10511,10690,10905,10950,10990,11311,12202,12431,13095,13389 / lldrawpoolwlsky 543,611,671 / gltfscenemanager 787,914 / lldrawpoolterrain 467 / llrender 981。
- **変種(offset-pointer/memset/flag-gated)**: pipeline 11560,11574,11583 + llrender 990,1030 + llglslshader 2739(setMinimumAlpha)= `mWritePerProgramUBOMinimumAlpha` gate 付き / llviewershadermgr 3227(avatar 初期化 memset)。
- **安全結論(構造導出)**: ①arena 経路 shader(block@0 全部)では両 idiom とも shadow 書込 = 等価・安全(§2.1)②blockless shader への書込は全 site が `mVkPerProgramUBOMapped != nullptr` guard で no-op ③**ring 族(MultiPointLightF)への direct 混入 = ゼロ**(multi-light loop = pipeline 12592-12807 は全て rotate idiom)。∴ per-site の shader 名帰属表は安全性に非寄与のため省略(申告: 縮小 = 帰属列。根拠 = 上記 ①②③ で全 site の安全が site 個別でなく構造で確定するため)。

## 7. Material D との関係(行使段)
D は本契約の初の大規模行使: 非 BLEND material 16 変種の族 A block を廃し(shader から block 宣言を削除)、5 値 32B を **DrawData slot(PP2 層)** の予約欄 [4..11] に移す。**UBO 供給次元**では C++ 側は本契約 §4 により変更ゼロで族 E に合流する(検出は perProg=0 のまま・phantom+dummy が吸収・39/49 は族 C のまま)。gate で per-draw arena alloc+memcpy の material 分消滅と desc dedup 実効化(dyn offset 定数化)を確認する。実装正本 = 実装 Brief(memory `impl_brief_material_D_f4f5`)。

### 7.1 D の設計上の主作用 = heap-set 反転(監査 F1 反映 2026-08-02)
aya_dd(set2 b0)の宣言追加は、非 BLEND material shader の `mVkReflUsesHeapSet`(reflection = set2 の任意宣言で true・llglslshader.cpp:1296-1299)を経由して **`mVkUsesHeapSet` を false→true に反転**させる(:2954)。これは副作用でなく **D の descriptor 経路設計の本体**であり、下流は以下の通り(全て設計時トレース・実装後突合済):
1. `establishPerDrawId` が活性化(旧 = 非 heap で PERDRAW_SLOT_INHERIT)→ per-draw ID + DrawData slot 供給が material に開通(= D の目的そのもの)。
2. `buildAndOverrideScenePerDrawSet` が heap 枝(per-shader `mVkPerDrawLane` memo)へ移行 = AYA_MAT_HEAP 変種が既に実証済みの経路に diffuse-only 変種が合流。
3. set1 sampler(diffuseMap b2 等)は不変: 汎用 sampler 解決 loop は heap flag 非依存に per-draw で binding 2/3/4 を解決し、materials pool の bindFast も無条件実行(lldrawpoolmaterials.cpp:172-197・`mat_bindless` は perf counter のみ :161-170)。
4. immediate 経路の `imm_cache`(= !heap)は material で無効化 = heap 経路の per-draw cache(evidence/validate)に置換。
5. **alpha-run/MDI collapse には到達しない**: `alphaRunCandidate` が `params.mMaterial != nullptr` を除外(lldrawpoolalpha.cpp:872-898、特に :879)するため、material draw は heap 化後も collapse_now = false = inline 経路のまま(監査 F1 の当該サブ主張はこの除外により不成立)。
