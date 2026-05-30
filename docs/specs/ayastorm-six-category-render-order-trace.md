# AYAstorm 6 カテゴリ × MASK 変種 (= 8 カテゴリ) 描画順 trace

**作成日**: 2026-05-29 (装着物 二重アルファブロック regression を case-by-case sort 設計で直そうとして Phase 1/2/2.1 3 連続 falsify。sort 設計層では解けず、bug 本体は pool / pass / dispatcher / depth-write の **routing 側** にある可能性が高いと判明。設計層に戻る前に「現状の描画経路」を静的 trace で確定させ、仮説を一切混ぜずに事実だけ並べた本書を起点に再設計する。)

**対象**: 装着物 (rigged + non-rig) + world rez prim の 3 軸 × opaque / MASK / BLEND 3 軸の組み合わせで 8 カテゴリを定義し、各カテゴリが踏む Pool / PASS / dispatcher / depth-write / blend func / sort key / 統合 view 参加 を `file:line` 付きで確定する。

**起点 reference doc** (本書は両方の鏡像でなく **統合 + MASK 変種を追加** した形):

- `docs/specs/ayastorm-attachment-rendering-routing.md` — 装着物 `mAttachedToAvatar.notNull()` 側、全 pool × shader × dispatcher。
- `docs/specs/ayastorm-rez-object-rendering-routing.md` — rez object `mAttachedToAvatar.isNull()` 側、canary=11/12 全 dispatcher。

両 doc が押さえているのは「**alpha pool 経由 vs alpha pool 以外**」と「**装着物 vs 非装着物**」の 2x2 マップ。**本書はその上に MASK ↔ BLEND の細分化** (alpha MASK が **どの pool 経由で描かれるか** = alpha pool ではなく opaque path 経由) と **rigged ↔ non-rig 細分化** (同 pool の `pushBatches` ↔ `pushRiggedBatches` 別) を重ねて 8 カテゴリ表を作っている。

---

## §1 目的と非目的

### §1.1 目的

二重アルファブロック regression の **直前 routing 状態** を、静的に C++ / GLSL を上から下まで追って `file:line` 付きで固定する。後続の修正設計はこの表を起点に「どの dispatcher の depth-write / blend / sort key を触ると、どのカテゴリの観測症状が動くか」を逆引きできるようにする。

### §1.2 非目的

- **修正提案は書かない**。各表セルに事実だけを並べる。
- **仮説は書かない**。「ここが原因かもしれない」「ここを変えれば直る」は本書では一切扱わない。表のどの行が観測症状に対応するかの突き合わせは後続 phase。
- **実機実行はしない**。grep + Read で静的 C++/GLSL を追うのみ。canary 検証や RenderDoc dump は本書 scope 外 (既存 reference doc §7 / §7.4 に既出)。
- 不明箇所は「不明」と明記し、推測で埋めない。

---

## §2 起点 reference doc サマリ

### §2.1 `ayastorm-attachment-rendering-routing.md` (装着物側)

- `LLDrawInfo::mAttachedToAvatar.notNull()` で「装着物」と定義 (= avatar / control avatar (animesh) を親に持つ vobj、または HUD attachment)。
- 装着物の主要 path: 8 dispatcher (lldrawpool.cpp の `pushBatch:665` / `pushUntexturedBatch:698` / `pushGLTFBatch:1138` / `pushUntexturedGLTFBatch:1165`、lldrawpoolbump.cpp `pushBumpBatch:1068`、lldrawpoolmaterials.cpp `renderDeferred:307`、lldrawpoolalpha.cpp `renderAlpha:859`)。
- canary 値: 1=BoM body/head (magenta) / 2=opaque 装着物 (blue) / 3=legacy prim 装着物 (gray) / 4=alpha BLEND 装着物 (brown) / 5+=Rez Object (11=黒 / 12=緑)。
- 「茶 (4)」 = forward alpha BLEND 経路 = 撮影描画系バグの鬼門 (DoF / SSAO / Z-order / 影 / 反射プローブが透過部で抜ける)。**本書 §3 でカテゴリ R-BL / N-BL に対応**。

### §2.2 `ayastorm-rez-object-rendering-routing.md` (rez object 側)

- 同じ `mAttachedToAvatar` flag を **isNull() 側** で参照したものが Rez Object。
- 装着物と Rez Object は **同じ pool / 同じ dispatcher** を共有する (例: `LLDrawPoolSimple` は装着物の不透明 mesh と Rez された装飾物の不透明 mesh を両方流す)。違いは LLDrawInfo の `mAttachedToAvatar` のみ。
- canary 11=Rez Object opaque/MASK (黒) / 12=Rez Object alpha BLEND (緑)。
- Linden tree は LLDrawInfo を経由しない `LLDrawPoolTree::renderDeferred` 経路 (drawRange 直叩き、shader bind 時 forced 11)。

### §2.3 両 doc が押さえていない軸 = 本書で追加するもの

| 軸 | 既存 doc | 本書 |
|---|---|---|
| 装着物 ↔ Rez Object | カテゴリ分け済 | 8 カテゴリの行軸として再利用 |
| 不透明 ↔ alpha BLEND | 2 vs 4 / 11 vs 12 で分離済 | 列軸として再利用 |
| **rigged ↔ non-rig** | 同一 dispatcher で扱う (PASS_*_RIGGED は PASS_* +1、`pushRiggedBatches`) | **本書で R / N に分離して別行化** |
| **alpha MASK 変種** | opaque 側に含めて扱う (canary=11 で 1 まとめ) | **opaque と別列 (MA) として独立行化** |
| Pool 単位の流路 | dispatcher までは押さえている | **pipeline.cpp の renderGeomDeferred / renderGeomPostDeferred / renderShadow / renderDoF 呼出順を `file:line` 付きで固定** |
| LLCullResult group list | 触れてない | §6 で全 list と Pool の対応を整理 |
| LLRenderPass::PASS_* enum | 触れてない | §7 で全数挙げて registerFace から各 pool draw map への登録経路を整理 |

---

## §3 8 カテゴリ確定表

### §3.1 カテゴリ定義

| 略称 | カテゴリ | `mAttachedToAvatar` | mesh tag | alpha mode (TE.color.a + material/gltf alpha) |
|---|---|---|---|---|
| R-OP | rigged 装着物 opaque | notNull() | `LLFace::RIGGED` set | non-blend, non-mask |
| R-MA | rigged 装着物 MASK | notNull() | `LLFace::RIGGED` set | `DIFFUSE_ALPHA_MODE_MASK` または `ALPHA_MODE_MASK` (GLTF) または `canRenderAsMask()` |
| R-BL | rigged 装着物 BLEND | notNull() | `LLFace::RIGGED` set | `DIFFUSE_ALPHA_MODE_BLEND`/`ALPHA_MODE_BLEND` / blinn_phong_alpha < 0.999 / is_alpha & !canRenderAsMask |
| N-OP | non-rig 装着物 opaque | notNull() | `LLFace::RIGGED` not set | non-blend, non-mask |
| N-MA | non-rig 装着物 MASK | notNull() | `LLFace::RIGGED` not set | MASK 同上 |
| N-BL | non-rig 装着物 BLEND | notNull() | `LLFace::RIGGED` not set | BLEND 同上 |
| W-OP | world prim opaque (+ alpha MASK 含む W-MA は本表で W-OP と同 dispatcher なので W-OP に統合表記) | isNull() | (RIGGED は Rez では稀。本表は non-rig 前提) | non-blend |
| W-BL | world prim BLEND | isNull() | non-rig | BLEND |

**注**: W-MA は depth-write / blend / pool 全ての観点で W-OP と同経路 (opaque pool 経由) のため、ユーザー指示の 8 カテゴリ枠では W-OP に併記する形で扱う。W-OP 行末尾に MASK 細別の注を付ける。

### §3.2 8 カテゴリ × 描画項目 確定表

凡例: file:line は当該 dispatcher 本体行 (drawRange を呼ぶ位置 or pushBatches 呼出位置)。**統合 view** = `mAYAAlphaColor` plate (POST_WATER の alpha BLEND を別 RT に分離) に参加するか。

| カテゴリ | 所属 Pool | 通過 PASS | dispatch 関数 | pipeline.cpp 呼出位置 | depth-write | blend func | sort key | 統合 view 参加 |
|---|---|---|---|---|---|---|---|---|
| **R-OP** (rigged 装着 opaque) | `LLDrawPoolSimple` (POOL_SIMPLE) / `LLDrawPoolBump` (POOL_BUMP) / `LLDrawPoolMaterials` (POOL_MATERIALS) / `LLDrawPoolGLTFPBR` opaque (POOL_GLTF_PBR) | `PASS_SIMPLE_RIGGED` (148) / `PASS_BUMP_RIGGED` (161) / `PASS_MATERIAL_RIGGED` 等 (165) / `PASS_GLTF_PBR_RIGGED` (209) | `LLDrawPoolSimple::renderDeferred` `lldrawpoolsimple.cpp:99` → `pushRiggedBatches(PASS_SIMPLE_RIGGED, true, true)` lldrawpoolsimple.cpp:110 / `LLDrawPoolBump::renderDeferred` `lldrawpoolbump.cpp:544` ループ i=1 で `pushBumpBatch` を `pushBumpBatches` 経由で叩く (lldrawpoolbump.cpp:579+584) / `LLDrawPoolMaterials::renderDeferred` `lldrawpoolmaterials.cpp:105` pass>=12 で rigged 化 (130-141) → drawRange 296 / `LLDrawPoolGLTFPBR::renderDeferred` `lldrawpoolpbropaque.cpp:53` → `pushRiggedGLTFBatches(mRenderType+1)` (lldrawpoolpbropaque.cpp:68) | **ON** (renderGeomDeferred は全 deferred pool で `LLGLEnable cull(GL_CULL_FACE)` のみ enable、explicit `LLGLDepthTest` 無し = GL default `GL_TRUE, GL_TRUE` + 各 shader bind 前に明示的 disable 無し = depth-write ON。`LLDrawPoolAlphaMask::renderDeferred` も同 ; 装着物資料 §0 確認との整合) | **無し** (deferred opaque は `LLGLDisable blend(GL_BLEND)` で BLEND off : lldrawpoolsimple.cpp:102, lldrawpoolsimple.cpp:202 (FullbrightAlphaMask)) | batch 互換 (`CompareBatchBreakerRigged` llvovolume.cpp:6676) — avatar + mesh id で grouping、距離 sort なし | renderGeomDeferred → gbuffer (frag_data[0..3]) 直書き = 統合 view 参加 (= deferred lighting で素材 + lit を統合) |
| **R-MA** (rigged 装着 MASK) | `LLDrawPoolAlphaMask` (POOL_ALPHA_MASK) / `LLDrawPoolFullbrightAlphaMask` (POOL_FULLBRIGHT_ALPHA_MASK) / `LLDrawPoolMaterials` の `_MASK` 系 / `LLDrawPoolGLTFPBR` で `mRenderType == PASS_GLTF_PBR_ALPHA_MASK` (POOL_GLTF_PBR_ALPHA_MASK) | `PASS_ALPHA_MASK_RIGGED` (203) / `PASS_FULLBRIGHT_ALPHA_MASK_RIGGED` (205) / `PASS_MATERIAL_ALPHA_MASK_RIGGED` 系 (169 / 177 / 185 / 193) / `PASS_GLTF_PBR_ALPHA_MASK_RIGGED` (211) | `LLDrawPoolAlphaMask::renderDeferred` `lldrawpoolsimple.cpp:116` → `pushRiggedMaskBatches(PASS_ALPHA_MASK_RIGGED, true, true)` lldrawpoolsimple.cpp:127 / `LLDrawPoolFullbrightAlphaMask::renderPostDeferred` `lldrawpoolsimple.cpp:184` → `pushRiggedMaskBatches(PASS_FULLBRIGHT_ALPHA_MASK_RIGGED, ...)` lldrawpoolsimple.cpp:212 / `LLDrawPoolMaterials::renderDeferred` mask 系 pass (idx 2/6/10/14 → PASS_*_ALPHA_MASK[_RIGGED]) lldrawpoolmaterials.cpp:105 / `LLDrawPoolGLTFPBR::renderDeferred` (POOL_GLTF_PBR_ALPHA_MASK は pool として別 instance) `lldrawpoolpbropaque.cpp:53` | **ON** (AlphaMask: lldrawpoolsimple.cpp:116 ; FullbrightAlphaMask: `LLGLDisable blend(GL_BLEND)` lldrawpoolsimple.cpp:202 → BLEND 無し / depth-write は default ON) | **無し** (`LLGLDisable blend(GL_BLEND)` lldrawpoolsimple.cpp:202 ; AlphaMask 系 deferred は BLEND off、`setMinimumAlpha` で fragment discard) | batch 互換 (rigged は `CompareBatchBreakerRigged`) | renderGeomDeferred (POOL_ALPHA_MASK / POOL_FULLBRIGHT_ALPHA_MASK は renderPostDeferred、後者は POOL_FULLBRIGHT 後) で gbuffer 直書き = 統合 view 参加 |
| **R-BL** (rigged 装着 BLEND) | `LLDrawPoolAlpha` (POOL_ALPHA_PRE_WATER / POOL_ALPHA_POST_WATER の 2 instance、実 PASS は `PASS_ALPHA[_RIGGED]` を共用) | `PASS_ALPHA_RIGGED` (201) | `LLDrawPoolAlpha::renderPostDeferred` `lldrawpoolalpha.cpp:143` → `forwardRender(true)` lldrawpoolalpha.cpp:284 (POST_WATER) または lldrawpoolalpha.cpp:291 (PRE_WATER / HUD) → `forwardRender` 内 `renderAlpha(mask, false, true)` lldrawpoolalpha.cpp:444 → group ループ + `drawRange` lldrawpoolalpha.cpp:991。group は `gPipeline.beginRiggedAlphaGroups()` (lldrawpoolalpha.cpp:776) | **`write_depth = rigged ∨ sSkipScreenCopy ∨ sImpostorRenderAlphaDepthPass ∨ POOL_ALPHA_PRE_WATER`** lldrawpoolalpha.cpp:403-408 → **rigged path は常に depth-write ON** (`LLGLDepthTest depth(GL_TRUE, GL_TRUE)` lldrawpoolalpha.cpp:411) | `color = (BF_SOURCE_ALPHA, BF_ONE_MINUS_SOURCE_ALPHA)` lldrawpoolalpha.cpp:413-414 / `alpha = (BF_ZERO, BF_ONE_MINUS_SOURCE_ALPHA)` 通常時 lldrawpoolalpha.cpp:428-429、`mForwardToAlphaRT` 時 `(BF_ONE, BF_ONE_MINUS_SOURCE_ALPHA)` lldrawpoolalpha.cpp:423-424、blendFunc 設定行 lldrawpoolalpha.cpp:432。loop 内で `params.mBlendFuncSrc/Dst` で per-draw override lldrawpoolalpha.cpp:979 | **`CompareRenderOrder`** llspatialpartition.h:276-287 = `(avatarp, mRenderOrder)` 順 (= rigged は avatar 単位で grouping して `mRenderOrder` 降順)、ソース pipeline.cpp:4596。LLDrawInfo 内では batch 互換 (rigged) | **POST_WATER 時のみ** `mAYAAlphaColor` plate に redirect (`use_alpha_rt` 条件 lldrawpoolalpha.cpp:220-227)、PRE_WATER / HUD / impostor / cube snapshot 時は mRT->screen 直書き。renderPostDeferred 末尾の DoF depth prepass (lldrawpoolalpha.cpp:326-353) で alpha BLEND fragment を mRT->screen.depth にも write、mAYAAlphaDepth.bindTarget (lldrawpoolalpha.cpp:370-391) で alpha 0.5 cutoff の re-inject — **statue view = mRT->screen 上に opaque + alpha BLEND が forward 合成された状態**。alpha plate redirect 時のみ alpha BLEND が「分離された統合 view 候補」になる |
| **N-OP** (non-rig 装着 opaque) | R-OP と同 pool (Simple / Bump / Materials / GLTFPBR opaque) | `PASS_SIMPLE` (147) / `PASS_BUMP` (160) / `PASS_MATERIAL` 等 (164) / `PASS_GLTF_PBR` (208) | `LLDrawPoolSimple::renderDeferred` `lldrawpoolsimple.cpp:99` → `pushBatches(PASS_SIMPLE, true, true)` lldrawpoolsimple.cpp:106 / Bump は `lldrawpoolbump.cpp:544` ループ i=0 / Materials は pass<12 / GLTFPBR は `pushGLTFBatches(mRenderType)` lldrawpoolpbropaque.cpp:63 | **ON** (R-OP と同条件、`LLGLDisable blend(GL_BLEND)` で BLEND off + depth-write default ON) | **無し** | batch 互換 (`CompareBatchBreaker` llvovolume.cpp:6682) — 距離 sort なし | gbuffer 直書き = 統合 view 参加 |
| **N-MA** (non-rig 装着 MASK) | R-MA と同 pool (AlphaMask / FullbrightAlphaMask / Materials MASK 系 / GLTFPBR MASK) | `PASS_ALPHA_MASK` (202) / `PASS_FULLBRIGHT_ALPHA_MASK` (204) / `PASS_MATERIAL_ALPHA_MASK` 系 (168/176/184/192) / `PASS_GLTF_PBR_ALPHA_MASK` (210) | `LLDrawPoolAlphaMask::renderDeferred` `lldrawpoolsimple.cpp:116` → `pushMaskBatches(PASS_ALPHA_MASK, true, true)` lldrawpoolsimple.cpp:123 / FullbrightAlphaMask `pushMaskBatches(PASS_FULLBRIGHT_ALPHA_MASK)` lldrawpoolsimple.cpp:206 / Materials `pushBatches(type)` (type は §6 の type_list[] から決まる、lldrawpoolmaterials.cpp:108-126) / GLTFPBR は同 pool で type=PASS_GLTF_PBR_ALPHA_MASK | **ON** (R-MA と同) | **無し** (R-MA と同) | batch 互換 | gbuffer 直書き = 統合 view 参加 |
| **N-BL** (non-rig 装着 BLEND) | `LLDrawPoolAlpha` (POOL_ALPHA_PRE_WATER / POOL_ALPHA_POST_WATER) | `PASS_ALPHA` (200) | `LLDrawPoolAlpha::renderPostDeferred` `lldrawpoolalpha.cpp:143` → `forwardRender()` (non-rigged 版) lldrawpoolalpha.cpp:283 (POST_WATER) または lldrawpoolalpha.cpp:293 (PRE_WATER / HUD) → `renderAlpha(mask, false, false)` lldrawpoolalpha.cpp:444 → group ループ + `drawRange` lldrawpoolalpha.cpp:991。group は `gPipeline.beginAlphaGroups()` (lldrawpoolalpha.cpp:781) | **`write_depth = sSkipScreenCopy ∨ sImpostorRenderAlphaDepthPass ∨ POOL_ALPHA_PRE_WATER`** lldrawpoolalpha.cpp:403-408 → **non-rigged POST_WATER 通常 path では depth-write OFF** (`LLGLDepthTest depth(GL_TRUE, GL_FALSE)` lldrawpoolalpha.cpp:411)。**PRE_WATER は water fog 整合性のため常に ON**。R-BL と **ここで非対称** | R-BL と同 blendFunc (lldrawpoolalpha.cpp:413-432) | **`CompareDepthGreater`** llspatialpartition.h:268-274 = `mDepth` 降順 (= 後ろから前、back-to-front)、ソース pipeline.cpp:4593。LLDrawInfo は genDrawInfo で `LLFace::CompareDistanceGreater()` 距離 sort (llvovolume.cpp:6687, distance_sort=true 時) | **POST_WATER 時** mAYAAlphaColor plate に redirect (R-BL と同 `use_alpha_rt` 条件)。N-BL は POST_WATER で depth-write OFF のため、forward 合成後の mRT->screen 上では N-BL 自身は depth に痕跡を残さない (= 後続の N-BL を遮蔽しない)。但し R-BL (depth-write ON) は痕跡を残す → 二重アルファブロック regression の core 構造 |
| **W-OP** (world prim opaque / 含む W-MA) | opaque: R-OP / N-OP と同 pool (Simple / Bump / Materials / GLTFPBR opaque) + `LLDrawPoolTree` (POOL_TREE) + `LLDrawPoolTerrain` (POOL_TERRAIN — 本書 scope 外注釈) + `LLDrawPoolGrass` (POOL_GRASS — alpha MASK shader 使用、本書扱い W-MA 内) ; MASK: R-MA / N-MA と同 pool + Tree shadow / Grass | opaque: `PASS_SIMPLE` (147) / `PASS_BUMP` (160) / `PASS_MATERIAL` (164) / `PASS_GLTF_PBR` (208) / `PASS_GRASS` (149) ; MASK: `PASS_ALPHA_MASK` (202) / `PASS_FULLBRIGHT_ALPHA_MASK` (204) / `PASS_MATERIAL_ALPHA_MASK` 系 / `PASS_GLTF_PBR_ALPHA_MASK` (210) | 同 dispatcher の non-rigged path (`pushBatches` / `pushBumpBatches` / Materials non-rigged pass / `pushGLTFBatches`)。LLDrawInfo の `mAttachedToAvatar.isNull()` で N-OP / N-MA と区別されるが **dispatcher は同一**。Tree は `LLDrawPoolTree::renderDeferred` (drawRange 直叩き、LLDrawInfo 経由なし、`lldrawpooltree.cpp:78-93` で canary=11 forced)。Grass は `LLDrawPoolGrass::renderDeferred` `lldrawpoolsimple.cpp:137` → `pushBatches(PASS_GRASS)` lldrawpoolsimple.cpp:145 | **ON** (N-OP / N-MA と同) | **無し** (N-OP / N-MA と同) | batch 互換 (`CompareBatchBreaker`) | gbuffer 直書き = 統合 view 参加。**alpha MASK (W-MA) も同経路 = opaque view と統合** = canary 11 (黒) 1 色になるのと同じ理由 |
| **W-BL** (world prim BLEND) | `LLDrawPoolAlpha` (POOL_ALPHA_PRE_WATER / POOL_ALPHA_POST_WATER) | `PASS_ALPHA` (200) | N-BL と同 dispatcher (`LLDrawPoolAlpha::renderPostDeferred` → `forwardRender()` → `renderAlpha(mask, false, false)`)。LLDrawInfo の `mAttachedToAvatar.isNull()` で N-BL と区別されるが **dispatcher は同一 group ループ** (= 同じ `beginAlphaGroups()`〜`endAlphaGroups()` iteration 内で混在処理) | **N-BL と同**: POST_WATER 通常 path は depth-write OFF / PRE_WATER は ON | **N-BL と同** | **`CompareDepthGreater`** (= back-to-front)。W-BL と N-BL は **同じ alpha group list (mAlphaGroups) に共存** し、`mDepth` 順に混ぜて sort される | N-BL と同: POST_WATER 時 mAYAAlphaColor plate redirect、PRE_WATER は mRT->screen 直書き |

### §3.3 表で読み取れる構造的事実 (= 二重アルファブロック regression の boundary)

- R-BL と N-BL は **同じ `LLDrawPoolAlpha::renderAlpha` の中で別 group list (mRiggedAlphaGroups vs mAlphaGroups) として処理される** が、`forwardRender(rigged)` の bool ひとつで depth-write の真偽が変わる。
- R-BL の **depth-write ON** は `write_depth = rigged || ...` lldrawpoolalpha.cpp:403 が単独条件 (= rigged だと無条件で ON)。一方 N-BL は POST_WATER 通常 path で **depth-write OFF**。
- POST_WATER で AYAstorm fix lldrawpoolalpha.cpp:277-294 が `forwardRender()` (non-rigged) → `forwardRender(true)` (rigged) の **back-to-front 順序を default 化**。但し非 POST_WATER (= PRE_WATER / HUD) は元の rigged → non-rigged 順を維持 (lldrawpoolalpha.cpp:286-294)。
- W-BL は LLDrawInfo の `mAvatar` が nullptr のため `renderAlpha(rigged=false)` の loop 内で扱われる → N-BL と完全に同じ pool / 同じ group list / 同じ depth-write / 同じ sort key。**W-BL ≡ N-BL (dispatcher 視点)**。違いは `mAttachedToAvatar` flag のみ。

---

## §4 pipeline.cpp 描画 phase 順 (dispatcher 呼出順を line 番号付きで)

### §4.1 main render frame entry (camera = world camera, `LLViewerCamera::getInstance()`)

`indra/newview/llviewerdisplay.cpp` `display()` 内のメインフロー:

| 呼出順 | 関数 | file:line | 効果 |
|---|---|---|---|
| 1 | `gPipeline.renderGeomDeferred(*LLViewerCamera::getInstance(), true)` | llviewerdisplay.cpp:1130 | deferred opaque pool 全 dispatch (= R-OP / N-OP / W-OP / R-MA / N-MA / W-MA の gbuffer 書込) |
| 2 | `rt.flush()` (rt = `gPipeline.mRT->deferredScreen`) | llviewerdisplay.cpp:1147-1148 | deferred gbuffer FBO を unbind |
| 3 | `gPipeline.renderDeferredLighting()` | llviewerdisplay.cpp:1152 | softenLight / deferred shadows / SSAO 等の lit 統合 (内部で renderGeomPostDeferred を呼ぶ) |
| 4 | `gPipeline.renderFinalize()` | llviewerdisplay.cpp:1624 | tonemap / bloom / DoF combine / UI / mAYAAlphaColor plate 合成 |

### §4.2 `LLPipeline::renderGeomDeferred(camera, do_occlusion=true)` 内 dispatcher 順

`pipeline.cpp:5039` (`renderGeomDeferred`):

| 呼出順 | 処理 | file:line | 関与カテゴリ |
|---|---|---|---|
| 1 | `setupHWLights()` | pipeline.cpp:5070 | light setup |
| 2 | mPools iterator で **POOL 番号順** (lldrawpool.h:57-79 の enum 昇順) に `poolp->renderDeferred(i)` を全 deferred pass 分呼出 | pipeline.cpp:5077-5160 | POOL_SKY → POOL_WATEREXCLUSION → POOL_WL_SKY → POOL_SIMPLE → POOL_FULLBRIGHT → POOL_BUMP → POOL_MATERIALS → POOL_GLTF_PBR → POOL_TERRAIN → POOL_GRASS → POOL_GLTF_PBR_ALPHA_MASK → POOL_TREE → POOL_ALPHA_MASK → POOL_FULLBRIGHT_ALPHA_MASK → POOL_AVATAR → POOL_CONTROL_AV の順。POOL_GLOW / POOL_ALPHA_PRE_WATER / POOL_VOIDWATER / POOL_WATER / POOL_ALPHA_POST_WATER は deferred pass を持たず後段 postDeferred のみ。 |
| 3 | POOL_GRASS の直前で occlusion query 発火 (`occlude && cur_type >= POOL_GRASS`) | pipeline.cpp:5109-5116 | hierarchical-Z 確保のため alpha-masked pool 前で occlusion 切替 |
| 4 | gGL setColorMask(true, false) で alpha channel write off に戻す | pipeline.cpp:5166 | postDeferred 用の準備 |

**この phase で書込まれる buffer**: `mRT->deferredScreen` の MRT (frag_data[0]=diffuse / [1]=spec / [2]=norm+flags / [3]=emissive)。R-OP / N-OP / W-OP / R-MA / N-MA / W-MA はここで gbuffer に確定。

### §4.3 `LLPipeline::renderDeferredLighting()` → `renderGeomPostDeferred(world_cam)` 呼出位置

`pipeline.cpp:11054` (`renderDeferredLighting`) 内で:

| 呼出順 | 処理 | file:line | 関与カテゴリ |
|---|---|---|---|
| 1 | shadow map render (deferred shadow pass、別 file: §4.5) | (renderDeferredLighting 内 各所) | 全 opaque + alpha MASK (alpha BLEND は通常 cast しない) |
| 2 | softenLight / SSAO / SSR / reflection probes 等の lit 統合 | renderDeferredLighting 内 | gbuffer から色を解く |
| 3 | `renderGeomPostDeferred(*LLViewerCamera::getInstance())` | pipeline.cpp:11649 | 後段 forward 系 dispatch (R-BL / N-BL / W-BL + glow + fullbright + alpha pool + water) |
| 4 | `renderGeomMotionBlur()` | pipeline.cpp:11655 | velocity buffer pass (Cinematic 時のみ実体配線) |

### §4.4 `LLPipeline::renderGeomPostDeferred(camera)` 内 dispatcher 順

`pipeline.cpp:5178` (`renderGeomPostDeferred`):

| 呼出順 | 処理 | file:line | 関与カテゴリ |
|---|---|---|---|
| 1 | `calcNearbyLights(camera)`, `setupHWLights()` | pipeline.cpp:5219-5220 | light state |
| 2 | `gGL.setSceneBlendType(LLRender::BT_ALPHA)` | pipeline.cpp:5222 | post deferred default blend |
| 3 | mPools iterator で POOL 番号順に `poolp->renderPostDeferred(i)` 呼出ループ開始 | pipeline.cpp:5244-5350 | 各 pool 順 (下表) |
| 3-a | `cur_type >= POOL_WATEREXCLUSION` で `doWaterExclusionMask()` | pipeline.cpp:5250-5254 | water clip mask |
| 3-b | **`cur_type >= POOL_FULLBRIGHT` で `doSkinSSS()`** (AYAstorm fix lldrawpoolalpha.cpp 行 5216, AYA_VR + AYAR20AvatarSkinSSSEnabled 条件) | pipeline.cpp:5262-5277 | SSS post-process。**`sss_pass = POOL_FULLBRIGHT` は AYAstorm 独自** (旧 LL/BD は atmospherics と同 block) |
| 3-c | `cur_type >= POOL_ALPHA_POST_WATER` (= `atmospherics_pass`) で `doAtmospherics()` + `doGodrays()` (Cinematic 条件) | pipeline.cpp:5280-5297 | 大気散乱 + light scattering |
| 3-d | `cur_type >= POOL_ALPHA_PRE_WATER` (= `water_haze_pass`) で `doWaterHaze()` | pipeline.cpp:5299-5303 | water fog 合成 |
| 3-e | 各 pool の `renderPostDeferred` 呼出 (postDeferred pass を持つ pool のみ): POOL_FULLBRIGHT → POOL_BUMP → POOL_GLTF_PBR → POOL_GLOW → POOL_ALPHA_PRE_WATER (LLDrawPoolAlpha) → POOL_WATER → POOL_ALPHA_POST_WATER (LLDrawPoolAlpha 別 instance) | pipeline.cpp:5306-5335 (各 pool の renderPostDeferred 内訳は §4.4.1) | R-BL / N-BL / W-BL はここで forward 合成 |
| 4 | `renderHighlights()` `renderDebug()` (世界カメラのみ) | pipeline.cpp:5359-5362 | デバッグ overlay |

### §4.4.1 各 pool の renderPostDeferred 詳細 (該当行のみ抜粋)

| Pool | function | file:line | 内部 dispatcher |
|---|---|---|---|
| `LLDrawPoolFullbright` | `renderPostDeferred(pass)` | lldrawpoolsimple.cpp:156 | `pushBatches(PASS_FULLBRIGHT)` (lldrawpoolsimple.cpp:174) + `pushRiggedBatches(PASS_FULLBRIGHT_RIGGED)` (lldrawpoolsimple.cpp:180) |
| `LLDrawPoolFullbrightAlphaMask` | `renderPostDeferred(pass)` | lldrawpoolsimple.cpp:184 | GLTF unlit (lldrawpoolsimple.cpp:189-190) + `pushMaskBatches(PASS_FULLBRIGHT_ALPHA_MASK)` (lldrawpoolsimple.cpp:206) + `pushRiggedMaskBatches(PASS_FULLBRIGHT_ALPHA_MASK_RIGGED)` (lldrawpoolsimple.cpp:212) |
| `LLDrawPoolBump` | `renderPostDeferred(pass)` | lldrawpoolbump.cpp:598 | 2 loop (static + rigged) で `beginFullbrightShiny / renderFullbrightShiny / endFullbrightShiny` + `beginBump / renderBump(PASS_POST_BUMP) / endBump` (lldrawpoolbump.cpp:602-617) |
| `LLDrawPoolGLTFPBR` | `renderPostDeferred(pass)` | lldrawpoolpbropaque.cpp:76 | HUD なら HUD shader bind + `pushGLTFBatches(mRenderType)` (lldrawpoolpbropaque.cpp:80-81)、else PBR_GLOW (lldrawpoolpbropaque.cpp:86-91) |
| `LLDrawPoolGlow` | `renderPostDeferred(pass)` | lldrawpoolsimple.cpp:45 | additive blend で `pushBatches(PASS_GLOW)` (lldrawpoolsimple.cpp:62) + `pushRiggedBatches(PASS_GLOW_RIGGED)` (lldrawpoolsimple.cpp:67) |
| `LLDrawPoolAlpha` (POOL_ALPHA_PRE_WATER 個体) | `renderPostDeferred(pass)` | lldrawpoolalpha.cpp:143 | PRE_WATER 分岐 = 元順 (rigged-first) lldrawpoolalpha.cpp:286-294 → `forwardRender(true)` → `forwardRender()` |
| `LLDrawPoolWater` | `renderPostDeferred(pass)` | (lldrawpoolwater.cpp、本表 scope 外) | 水面描画 |
| `LLDrawPoolAlpha` (POOL_ALPHA_POST_WATER 個体) | `renderPostDeferred(pass)` | lldrawpoolalpha.cpp:143 | POST_WATER 分岐 = AYAstorm fix 順 (non-rigged 先 → rigged 後) lldrawpoolalpha.cpp:277-294 → `forwardRender()` → `forwardRender(true)` + alpha plate redirect (use_alpha_rt) + DoF depth prepass |

### §4.5 `LLPipeline::renderShadow(view, proj, shadow_cam, result, depth_clamp)` 内 dispatcher 順

`pipeline.cpp:12409`。詳細未展開だが概要:

- shadow caster = opaque + alpha MASK (alpha BLEND は基本 cast しない)。
- `LLPipeline::renderAlphaObjects(rigged)` `pipeline.cpp:8457` が **shadow path 用** の alpha-mask drawRange loop (PASS_ALPHA の `LLDrawInfo` を rigged / non-rigged で振分、`gDeferredShadowAlphaMaskProgram` / `gDeferredShadowGLTFAlphaBlendProgram` で render — alpha cutoff 0.33 程度)。
- `LLPipeline::renderMaskedObjects(type, ..., rigged)` `pipeline.cpp:8537` が `LLDrawPoolAlphaMask::pushMaskBatches` / `pushRiggedMaskBatches` を直叩き。
- `LLPipeline::renderFullbrightMaskedObjects` `pipeline.cpp:8555` が `LLDrawPoolFullbrightAlphaMask::pushMaskBatches` を直叩き。
- LLDrawPoolAvatar の shadow pass は `beginShadowPass` で SHADOW_PASS_AVATAR_OPAQUE / ALPHA_BLEND / ALPHA_MASK の 3 mode を切替 (lldrawpoolavatar.cpp:287-343)。

### §4.6 HUD pass (= `LLPipeline::sRenderingHUDs == true` 期間)

`indra/newview/llviewerdisplay.cpp:1404-1492` の `display()` 内 1 ブロックのみ:

| 呼出順 | 処理 | file:line |
|---|---|---|
| 1 | `LLPipeline::sRenderingHUDs = true` | llviewerdisplay.cpp:1406 |
| 2 | hud_cam (origin (-1,0,0), identity axes) を組む | llviewerdisplay.cpp:1407-1412 |
| 3 | RENDER_TYPE_* / RENDER_TYPE_PASS_* を HUD 用に全 toggle on | llviewerdisplay.cpp:1449-1472 |
| 4 | `gPipeline.renderGeomPostDeferred(hud_cam)` 1 回 (renderGeomDeferred 経由しない) | llviewerdisplay.cpp:1476 |
| 5 | `LLPipeline::sRenderingHUDs = false` | llviewerdisplay.cpp:1491 |

HUD pass では gbuffer を持たず frag_color 直書き、IS_HUD permutation の shader を bind、`renderGeomDeferred` を経由しないため R-OP / N-OP / W-OP / R-MA / N-MA / W-MA の gbuffer 化はスキップされ、各 pool の renderPostDeferred 内で **forward 直接合成** される。

---

## §5 各 dispatcher 内 depth-write / blend state 詳細

### §5.1 deferred opaque path

| dispatcher | 確定 file:line | depth-write | blend |
|---|---|---|---|
| `LLDrawPoolSimple::renderDeferred` | lldrawpoolsimple.cpp:99 | GL default ON (明示的 disable 無し) | `LLGLDisable blend(GL_BLEND)` lldrawpoolsimple.cpp:102 |
| `LLDrawPoolAlphaMask::renderDeferred` | lldrawpoolsimple.cpp:116 | GL default ON | 明示的 disable / enable 無し (= GL default disable) ; `setMinimumAlpha` で discard |
| `LLDrawPoolGrass::renderDeferred` | lldrawpoolsimple.cpp:137 | GL default ON | 明示的 BLEND 操作無し、`setMinimumAlpha(0.5f)` lldrawpoolsimple.cpp:142 |
| `LLDrawPoolBump::renderDeferred` | lldrawpoolbump.cpp:544 | GL default ON | 明示 BLEND 操作なし |
| `LLDrawPoolMaterials::renderDeferred` | lldrawpoolmaterials.cpp:105 | GL default ON | 明示 BLEND 操作なし |
| `LLDrawPoolGLTFPBR::renderDeferred` | lldrawpoolpbropaque.cpp:53 | GL default ON | 明示 BLEND 操作なし |

renderGeomDeferred 全体 (pipeline.cpp:5039) の包む状態: `LLGLEnable cull(GL_CULL_FACE)` pipeline.cpp:5075 のみ。明示的 depth state 設定なし → GL default (`GL_TRUE, GL_TRUE`) のまま全 pool が deferred dispatch する。

### §5.2 postDeferred / forward path

| dispatcher | 確定 file:line | depth-write | blend |
|---|---|---|---|
| `LLDrawPoolFullbright::renderPostDeferred` | lldrawpoolsimple.cpp:156 | GL default ON (明示なし) | `gGL.setSceneBlendType(LLRender::BT_ALPHA)` lldrawpoolsimple.cpp:170 |
| `LLDrawPoolFullbrightAlphaMask::renderPostDeferred` | lldrawpoolsimple.cpp:184 | GL default ON | `LLGLDisable blend(GL_BLEND)` lldrawpoolsimple.cpp:202 |
| `LLDrawPoolBump::renderPostDeferred` | lldrawpoolbump.cpp:598 | (各 sub-routine 内、本書 scope 浅追い) | (sub-routine 別。`beginFullbrightShiny` 内で `LLGLDepthTest` 等) |
| `LLDrawPoolGLTFPBR::renderPostDeferred` | lldrawpoolpbropaque.cpp:76 | GL default ON | (PBR glow 分岐で `setColorMask(false, true)` lldrawpoolpbropaque.cpp:85) |
| `LLDrawPoolGlow::renderPostDeferred` | lldrawpoolsimple.cpp:45 | **OFF** `LLGLDepthTest depth(GL_TRUE, GL_FALSE)` lldrawpoolsimple.cpp:57 | `LLGLEnable blend(GL_BLEND)` lldrawpoolsimple.cpp:50 + `BT_ADD` lldrawpoolsimple.cpp:55 + `LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL)` lldrawpoolsimple.cpp:53 |
| `LLDrawPoolAlpha::renderPostDeferred` 内 `forwardRender(rigged)` | lldrawpoolalpha.cpp:394 | **動的**: `write_depth = rigged ∨ sSkipScreenCopy ∨ sImpostorRenderAlphaDepthPass ∨ POOL_ALPHA_PRE_WATER` lldrawpoolalpha.cpp:403-408 → `LLGLDepthTest depth(GL_TRUE, write_depth ? GL_TRUE : GL_FALSE)` lldrawpoolalpha.cpp:411 | `LLGLSPipelineAlpha gls_pipeline_alpha` lldrawpoolalpha.cpp:398 = `LLGLEnable mBlend(GL_BLEND)` llglstates.h:114-118。`blendFunc(BF_SOURCE_ALPHA, BF_ONE_MINUS_SOURCE_ALPHA, mAlphaSFactor, mAlphaDFactor)` lldrawpoolalpha.cpp:432 ; mAlphaSFactor は `mForwardToAlphaRT` 時 `BF_ONE` lldrawpoolalpha.cpp:423、通常時 `BF_ZERO` lldrawpoolalpha.cpp:428 |
| `LLDrawPoolAlpha::renderPostDeferred` 内 DoF depth prepass | lldrawpoolalpha.cpp:326-353 | ON (色 mask off で depth のみ) `gGL.setColorMask(false, false)` lldrawpoolalpha.cpp:345 | (color 書込なし) |
| `LLDrawPoolAlpha::renderPostDeferred` 内 mAYAAlphaDepth re-inject | lldrawpoolalpha.cpp:370-391 | ON (mAYAAlphaDepth FBO に depth のみ) | (color 書込なし) |
| `LLDrawPoolAlpha::renderPostDeferred` 内 mAYAAlphaColor clear | lldrawpoolalpha.cpp:236-249 | **OFF** `LLGLDepthTest depth_off(GL_FALSE, GL_FALSE)` lldrawpoolalpha.cpp:244 | (clear のみ) |
| `LLDrawPoolAlpha::renderAlpha` 内 emissive pass | lldrawpoolalpha.cpp:1038-1115 | (renderAlpha 外側の depth state 継承) | `blendFunc(BF_ZERO, BF_ONE, BF_ONE, BF_ONE)` lldrawpoolalpha.cpp:1068 = 色変えず alpha (glow) に additive |
| `LLDrawPoolAlpha::renderEmissives` / renderPbrEmissives | lldrawpoolalpha.cpp:686 / 699 | `LLGLDepthTest depth(GL_TRUE, GL_FALSE)` lldrawpoolalpha.cpp:715 / 739 | (外側継承) |

### §5.3 renderGeomPostDeferred 包む状態

pipeline.cpp:5190 で `LLGLEnable cull(GL_CULL_FACE)` のみ enable。pipeline.cpp:5222 で `gGL.setSceneBlendType(LLRender::BT_ALPHA)`、pipeline.cpp:5223 で `gGL.setColorMask(true, false)`。各 pool の renderPostDeferred 内で local 上書きされる。

---

## §6 LLCullResult group list と Pool の対応

`indra/newview/llspatialpartition.h:500-616` の `LLCullResult` 内 list:

| List 名 | サイズ getter | iterator | 内容 | 関連 Pool |
|---|---|---|---|---|
| `mVisibleGroups` | `getVisibleGroupsSize()` | `beginVisibleGroups()` / `endVisibleGroups()` | 可視 `LLSpatialGroup*` 全般 | 全 pool 共通 (drawmap registration の準備段) |
| `mAlphaGroups` | `getAlphaGroupsSize()` | `beginAlphaGroups()` / `endAlphaGroups()` | `PASS_ALPHA` を持つ非 rigged group。`std::sort(..., CompareDepthGreater())` pipeline.cpp:4593 で **mDepth 降順 (back-to-front)** に sort | `LLDrawPoolAlpha` の `renderAlpha(rigged=false)` で iterate (lldrawpoolalpha.cpp:781) |
| `mRiggedAlphaGroups` | `getRiggedAlphaGroupsSize()` | `beginRiggedAlphaGroups()` / `endRiggedAlphaGroups()` | `PASS_ALPHA_RIGGED` を持つ rigged group。`std::sort(..., CompareRenderOrder())` pipeline.cpp:4596 で **(mAvatarp, mRenderOrder 降順)** sort | `LLDrawPoolAlpha` の `renderAlpha(rigged=true)` で iterate (lldrawpoolalpha.cpp:776) |
| `mOcclusionGroups` | `getOcclusionGroupsSize()` (hasOcclusionGroups で空チェック) | `beginOcclusionGroups()` / `endOcclusionGroups()` | occlusion query 対象 group | `doOcclusion` (pipeline.cpp:5115 含む) |
| `mDrawableGroups` | `getDrawableGroupsSize()` | `beginDrawableGroups()` / `endDrawableGroups()` | drawable 単位の group | impostor / particle 等 |
| `mVisibleList` | `getVisibleListSize()` | `beginVisibleList()` / `endVisibleList()` | 可視 `LLDrawable*` 直 list | drawable 単位処理 |
| `mVisibleBridge` | `getVisibleBridgeSize()` | `beginVisibleBridge()` / `endVisibleBridge()` | `LLSpatialBridge*` (= attachment partition 等 nested partition) | attachment / animesh 経路 |
| `mRenderMap[type]` (NUM_RENDER_TYPES 次元) | `getRenderMapSize(type)` | `beginRenderMap(type)` / `endRenderMap(type)` | `LLDrawInfo*` の per-PASS list | 各 pool が `gPipeline.beginRenderMap(PASS_*)` で iterate (例: bump pool lldrawpoolbump.cpp:559、materials pool lldrawpoolmaterials.cpp:143、shadow alpha pipeline.cpp:8474) |

### §6.1 group list push の発火位置

`pipeline.cpp:4515-4551` 周辺の cull traversal で `group->mDrawMap.find(PASS_ALPHA)` / `find(PASS_ALPHA_RIGGED)` に存在する group のみ `sCull->pushAlphaGroup(group)` / `pushRiggedAlphaGroup(group)` を発火。distance update は CAMERA_WORLD + 非 cube snapshot 時のみ (pipeline.cpp:4523-4534)。rigged は distance update なし (= attachment が avatar の depth を再利用、pipeline.cpp:4544-4550 のコメント参照)。

### §6.2 sort 発火位置

| sort 対象 | comparator | file:line |
|---|---|---|
| `mAlphaGroups` | `LLSpatialGroup::CompareDepthGreater` (llspatialpartition.h:268-274, `mDepth > rhs->mDepth` = 降順) | pipeline.cpp:4593 |
| `mRiggedAlphaGroups` | `LLSpatialGroup::CompareRenderOrder` (llspatialpartition.h:276-287, `mAvatarp` 一次 + `mRenderOrder > rhs->mRenderOrder` 二次) | pipeline.cpp:4596 |

LLDrawInfo の **per-face 内 sort** (genDrawInfo 内、llvovolume.cpp:6657 関数): `distance_sort=true` 時は `LLFace::CompareDistanceGreater` (llvovolume.cpp:6687)、それ以外は `CompareBatchBreaker` (non-rigged llvovolume.cpp:6682) / `CompareBatchBreakerRigged` (rigged llvovolume.cpp:6676)。distance_sort=true なのは PASS_ALPHA / 一部 material 系のみ (llvovolume.cpp:7106 の assert `pass[mask] == PASS_ALPHA ? distance_sort : true` で確定)。

---

## §7 LLRenderPass::PASS_* enum 全数と pool draw map 登録経路

### §7.1 全 PASS_* enum (`indra/newview/lldrawpool.h:147-212`)

`LLRenderPass::PASS_*` (= NUM_POOL_TYPES から開始する render type enum、`NUM_RENDER_TYPES` まで連番):

| PASS 名 | line | rigged variant (= +1) |
|---|---|---|
| `PASS_SIMPLE` | 147 | `PASS_SIMPLE_RIGGED` 148 |
| `PASS_GRASS` | 149 | (none) |
| `PASS_FULLBRIGHT` | 150 | `PASS_FULLBRIGHT_RIGGED` 151 |
| `PASS_INVISIBLE` | 152 | `PASS_INVISIBLE_RIGGED` 153 |
| `PASS_INVISI_SHINY` | 154 | `PASS_INVISI_SHINY_RIGGED` 155 |
| `PASS_FULLBRIGHT_SHINY` | 156 | `PASS_FULLBRIGHT_SHINY_RIGGED` 157 |
| `PASS_SHINY` | 158 | `PASS_SHINY_RIGGED` 159 |
| `PASS_BUMP` | 160 | `PASS_BUMP_RIGGED` 161 |
| `PASS_POST_BUMP` | 162 | `PASS_POST_BUMP_RIGGED` 163 |
| `PASS_MATERIAL` | 164 | `PASS_MATERIAL_RIGGED` 165 |
| `PASS_MATERIAL_ALPHA` | 166 | `PASS_MATERIAL_ALPHA_RIGGED` 167 |
| `PASS_MATERIAL_ALPHA_MASK` | 168 | `PASS_MATERIAL_ALPHA_MASK_RIGGED` 169 |
| `PASS_MATERIAL_ALPHA_EMISSIVE` | 170 | `PASS_MATERIAL_ALPHA_EMISSIVE_RIGGED` 171 |
| `PASS_SPECMAP` | 172 | `PASS_SPECMAP_RIGGED` 173 |
| `PASS_SPECMAP_BLEND` | 174 | `PASS_SPECMAP_BLEND_RIGGED` 175 |
| `PASS_SPECMAP_MASK` | 176 | `PASS_SPECMAP_MASK_RIGGED` 177 |
| `PASS_SPECMAP_EMISSIVE` | 178 | `PASS_SPECMAP_EMISSIVE_RIGGED` 179 |
| `PASS_NORMMAP` | 180 | `PASS_NORMMAP_RIGGED` 181 |
| `PASS_NORMMAP_BLEND` | 182 | `PASS_NORMMAP_BLEND_RIGGED` 183 |
| `PASS_NORMMAP_MASK` | 184 | `PASS_NORMMAP_MASK_RIGGED` 185 |
| `PASS_NORMMAP_EMISSIVE` | 186 | `PASS_NORMMAP_EMISSIVE_RIGGED` 187 |
| `PASS_NORMSPEC` | 188 | `PASS_NORMSPEC_RIGGED` 189 |
| `PASS_NORMSPEC_BLEND` | 190 | `PASS_NORMSPEC_BLEND_RIGGED` 191 |
| `PASS_NORMSPEC_MASK` | 192 | `PASS_NORMSPEC_MASK_RIGGED` 193 |
| `PASS_NORMSPEC_EMISSIVE` | 194 | `PASS_NORMSPEC_EMISSIVE_RIGGED` 195 |
| `PASS_GLOW` | 196 | `PASS_GLOW_RIGGED` 197 |
| `PASS_GLTF_GLOW` | 198 | `PASS_GLTF_GLOW_RIGGED` 199 |
| `PASS_ALPHA` | 200 | `PASS_ALPHA_RIGGED` 201 |
| `PASS_ALPHA_MASK` | 202 | `PASS_ALPHA_MASK_RIGGED` 203 |
| `PASS_FULLBRIGHT_ALPHA_MASK` | 204 | `PASS_FULLBRIGHT_ALPHA_MASK_RIGGED` 205 |
| `PASS_ALPHA_INVISIBLE` | 206 | `PASS_ALPHA_INVISIBLE_RIGGED` 207 |
| `PASS_GLTF_PBR` | 208 | `PASS_GLTF_PBR_RIGGED` 209 |
| `PASS_GLTF_PBR_ALPHA_MASK` | 210 | `PASS_GLTF_PBR_ALPHA_MASK_RIGGED` 211 |

NUM_RENDER_TYPES = 212 (= PASS_GLTF_PBR_ALPHA_MASK_RIGGED + 1)。

**registerFace の +1 規則** (llvovolume.cpp:5619-5626): `facep->isState(LLFace::RIGGED)` なら `passType = type + 1`、それ以外は `type`。enum はこれを前提に「non-rigged 連番 + RIGGED 偶数 +1」の組で並んでいる。

### §7.2 各 PASS と registerFace 発火条件 (llvovolume.cpp:6960-7250 の分岐)

| 条件 (gltf_mat / mat / fullbright / blinn_phong / is_alpha / canRenderAsMask) | 出力 PASS | file:line |
|---|---|---|
| `gltf_mat && alphaMode==BLEND` | `PASS_ALPHA` | 6995 |
| `gltf_mat && alphaMode==MASK` | `PASS_GLTF_PBR_ALPHA_MASK` | 7000 |
| `gltf_mat && alphaMode==OPAQUE` | `PASS_GLTF_PBR` | 7004 |
| `fullbright + mat MASK + blinn_phong_opaque` | `PASS_FULLBRIGHT_ALPHA_MASK` | 7018 |
| `fullbright + mat MASK + !blinn_phong_opaque` | `PASS_ALPHA` | 7022 |
| `fullbright + is_alpha` | `PASS_ALPHA` | 7027 |
| `fullbright + (envIntensity>0 \|\| shiny>0)` | `material_pass = true` → §7.2.1 表 | 7031-7034 |
| `fullbright + blinn_phong_opaque + simple` | `PASS_FULLBRIGHT` | 7039 |
| `fullbright + !blinn_phong_opaque` | `PASS_ALPHA` | 7043 |
| `mat + blinn_phong_transparent` | `PASS_ALPHA` | 7050 |
| `mat + legacy_bump` | `PASS_BUMP` | 7056 |
| `mat + non-fullbright + material_pass` | `pass[mask]` 配列 (§7.2.1) | 7107 |
| `mat (non-fullbright) + DIFFUSE_ALPHA_MODE_MASK` | `fullbright ? PASS_FULLBRIGHT_ALPHA_MASK : PASS_ALPHA_MASK` | 7123 |
| `mat (non-fullbright) + is_alpha` | `PASS_ALPHA` | 7127 |
| `mat (non-fullbright) + shiny + can_be_shiny` | `fullbright ? PASS_FULLBRIGHT_SHINY : PASS_SHINY` | 7133 |
| `mat (non-fullbright) + else` | `fullbright ? PASS_FULLBRIGHT : PASS_SIMPLE` | 7137 |
| `no mat + is_alpha + faceColor.a<=0` | `PASS_ALPHA_INVISIBLE` | 7146 |
| `no mat + is_alpha + canRenderAsMask + !hud + (fullbright \|\| sNoAlpha)` | `PASS_FULLBRIGHT_ALPHA_MASK` | 7152 |
| `no mat + is_alpha + canRenderAsMask + !hud + else` | `PASS_ALPHA_MASK` | 7156 |
| `no mat + is_alpha + else` | `PASS_ALPHA` | 7161 |
| `no mat + shiny + can_be_shiny + invisiprim` | `PASS_INVISI_SHINY` + `PASS_INVISIBLE` | 7172-7173 |
| `no mat + shiny + can_be_shiny + !hud + fullbright` | `PASS_FULLBRIGHT_SHINY` (+ `PASS_POST_BUMP` if bumpmap) | 7180-7184 |
| `no mat + shiny + can_be_shiny + !hud + legacy_bump` | `PASS_BUMP` | 7189 |
| `no mat + shiny + can_be_shiny + !hud + else` | `PASS_SIMPLE` | 7194 |
| `no mat + shiny + can_be_shiny + hud + fullbright` | `PASS_FULLBRIGHT_SHINY` | 7199 |
| `no mat + shiny + can_be_shiny + hud + else` | `PASS_SHINY` | 7203 |
| `no mat + !is_alpha + !shiny + invisiprim` | `PASS_INVISIBLE` | 7212 |
| `no mat + !is_alpha + !shiny + (fullbright \|\| bake_sunlight) + mat MASK` | `PASS_FULLBRIGHT_ALPHA_MASK` | 7219 |
| `no mat + !is_alpha + !shiny + (fullbright \|\| bake_sunlight) + else` | `PASS_FULLBRIGHT` (+ `PASS_POST_BUMP` if !hud + legacy_bump) | 7223 / 7227 |
| `no mat + !is_alpha + !shiny + legacy_bump` | `PASS_BUMP` | 7235 |
| `no mat + !is_alpha + !shiny + else` | `PASS_SIMPLE` | 7239 周辺 |

### §7.2.1 material_pass の pass[mask] 配列 (llvovolume.cpp:7063-7107)

`mat->getShaderMask(alpha_mode, is_alpha)` で 0〜15 (12-bit subset) の mask を取り、配列 lookup:

```
0  → PASS_MATERIAL
1  → PASS_ALPHA  (元 PASS_MATERIAL_ALPHA → forced to PASS_ALPHA で alpha pool に流す)
2  → PASS_MATERIAL_ALPHA_MASK
3  → PASS_MATERIAL_ALPHA_EMISSIVE
4  → PASS_SPECMAP
5  → PASS_ALPHA  (元 PASS_SPECMAP_BLEND → forced)
6  → PASS_SPECMAP_MASK
7  → PASS_SPECMAP_EMISSIVE
8  → PASS_NORMMAP
9  → PASS_ALPHA  (元 PASS_NORMMAP_BLEND → forced)
10 → PASS_NORMMAP_MASK
11 → PASS_NORMMAP_EMISSIVE
12 → PASS_NORMSPEC
13 → PASS_ALPHA  (元 PASS_NORMSPEC_BLEND → forced)
14 → PASS_NORMSPEC_MASK
15 → PASS_NORMSPEC_EMISSIVE
```

= material 系の BLEND は全て `PASS_ALPHA` に流れて `LLDrawPoolAlpha` に委譲される (= material BLEND 専用 pool は無い、alpha pool 一本化)。MASK / EMISSIVE は LLDrawPoolMaterials が deferred で取扱。

### §7.3 各 Pool が iterate する PASS 全数

| Pool | iterate する PASS (deferred / postDeferred / shadow / motion blur 別) | 確定 file:line |
|---|---|---|
| `LLDrawPoolSimple` | deferred: `PASS_SIMPLE` + `PASS_SIMPLE_RIGGED` | lldrawpoolsimple.cpp:106 / 110 |
| `LLDrawPoolAlphaMask` | deferred: `PASS_ALPHA_MASK` + `PASS_ALPHA_MASK_RIGGED` | lldrawpoolsimple.cpp:123 / 127 |
| `LLDrawPoolGrass` | deferred: `PASS_GRASS` | lldrawpoolsimple.cpp:145 |
| `LLDrawPoolFullbright` | postDeferred: `PASS_FULLBRIGHT` + `PASS_FULLBRIGHT_RIGGED` | lldrawpoolsimple.cpp:174 / 180 |
| `LLDrawPoolFullbrightAlphaMask` | postDeferred: `PASS_FULLBRIGHT_ALPHA_MASK` + `PASS_FULLBRIGHT_ALPHA_MASK_RIGGED` | lldrawpoolsimple.cpp:206 / 212 |
| `LLDrawPoolBump` | deferred: `PASS_BUMP` + `PASS_BUMP_RIGGED` (lldrawpoolbump.cpp:558) ; postDeferred: shiny + `PASS_POST_BUMP` + `PASS_POST_BUMP_RIGGED` | lldrawpoolbump.cpp:558 / 615 |
| `LLDrawPoolMaterials` | deferred 12pass×2 (rigged): PASS_MATERIAL / _ALPHA_MASK / _ALPHA_EMISSIVE / PASS_SPECMAP / _MASK / _EMISSIVE / PASS_NORMMAP / _MASK / _EMISSIVE / PASS_NORMSPEC / _MASK / _EMISSIVE (全 12 + RIGGED 12 = 24) | lldrawpoolmaterials.cpp:108-126 / 137-141 |
| `LLDrawPoolGLTFPBR` (POOL_GLTF_PBR) | deferred: `PASS_GLTF_PBR` (+RIGGED) ; postDeferred: PBR glow `PASS_GLTF_GLOW` (+RIGGED) | lldrawpoolpbropaque.cpp:63 / 68 / 87 / 90 |
| `LLDrawPoolGLTFPBR` (POOL_GLTF_PBR_ALPHA_MASK) | deferred: `PASS_GLTF_PBR_ALPHA_MASK` (+RIGGED) | lldrawpoolpbropaque.cpp:53 周辺 (mRenderType で分岐) |
| `LLDrawPoolGlow` | postDeferred: `PASS_GLOW` + `PASS_GLOW_RIGGED` | lldrawpoolsimple.cpp:62 / 67 |
| `LLDrawPoolAlpha` | postDeferred: group ループで `PASS_ALPHA` (non-rigged) / `PASS_ALPHA_RIGGED` (rigged) (group->mDrawMap[PASS_ALPHA+pass]) | lldrawpoolalpha.cpp:526 / 852 |
| `LLDrawPoolAvatar` | deferred 3 pass: impostor / rigid / skinned (= 自分のアバター本体描画) | lldrawpoolavatar.cpp:184-194 |
| `LLDrawPoolTree` | deferred: drawRange 直叩き (LLDrawInfo 経由なし、mDrawFace を直 iterate) | lldrawpooltree.cpp:139 |
| `LLDrawPoolTerrain` / `LLDrawPoolSky` / `LLDrawPoolWLSky` / `LLDrawPoolWater` / `LLDrawPoolWaterExclusion` | (本書 scope 外) | — |

---

## §8 不明箇所一覧 (推論禁止のため明示)

以下は静的 grep + Read では確定できなかった箇所。canary 検証 / RenderDoc / 実機 step 実行で確定する必要あり。

1. **HUD pass での R-OP / N-OP / W-OP の depth-write 実効値**: HUD は renderGeomDeferred を経由せず renderGeomPostDeferred のみ。各 pool の renderPostDeferred (Fullbright / FullbrightAlphaMask / Bump / GLTFPBR) が IS_HUD permutation で動くが、その時の depth-write 状態は pool 内 explicit 設定が無いため GL default 継承。直前の dispatcher 状態を引き継ぐので「呼出順依存」。本書 §4.6 の 5 step では確定できない。

2. **`LLDrawPoolBump::renderPostDeferred` 内 sub-routine `beginFullbrightShiny` / `beginBump` の詳細 depth-write / blend state**: shiny / bump sub-routine は `lldrawpoolbump.cpp:285` / `:497` あたりに居て、複数の `LLGLDepthTest` を持つ可能性が高い。本書では浅追いに留めた。R-OP / N-OP / W-OP のうち bump-mapped + shiny 系の post 処理を確定する場合は別途 trace 必要。

3. **POOL_AVATAR / POOL_CONTROL_AV の renderDeferred 経由でくる「装着物以外の avatar body」の扱い**: `LLDrawPoolAvatar::renderDeferred` (`lldrawpoolavatar.cpp:224`) は `render(pass)` 経由で `renderAvatars(NULL, pass)` を呼び (lldrawpoolavatar.cpp:431-441)、pass 1=rigid (= legacy non-mesh rigid attachments) / pass 2=skinned (= avatar body geometry) で振り分け。R-OP / R-MA の rigged-mesh 装着物は **AvatarPool ではなく Simple/Bump/Materials/GLTFPBR pool の rigged path** で描画される (= registerFace で PASS_*_RIGGED に register された結果)。AvatarPool が描く skinned 本体と rigged-mesh 装着物が **同一 frame 内でどう depth 競合するか** は未追跡。

4. **shadow pass の各カテゴリ ↔ shadow shader 対応の精密 map**: §4.5 で外形のみ。R-OP / R-MA / N-OP / N-MA / W-OP / W-MA は基本 shadow caster (alpha BLEND は cast しない) だが、`LLDrawPoolMaterials::beginShadowPass` 等の renderShadow loop と `LLPipeline::renderShadow` の呼出位置 (`pipeline.cpp:12409`) の対応を line 単位で追っていない。

5. **`mAYAAlphaColor` plate 合成位置の精密 line**: lldrawpoolalpha.cpp:259 で bindTarget、:301 で flush。合成 (dofCombineF / pre-tonemap composite) は `LLPipeline::renderFinalize` (llviewerdisplay.cpp:1624) 内のどこかだが、本書では追っていない。

6. **`PASS_ALPHA_INVISIBLE` / `PASS_INVISIBLE` / `PASS_INVISI_SHINY` の pool 帰属**: registerFace では割当られている (§7.2) が、どの pool が iterate するかは pushUntexturedBatches で `LLDrawPoolAlpha::renderDebugAlpha` (lldrawpoolalpha.cpp:469-470) でしか観測できなかった = 通常 path での draw は無い? 確定するには各 pool の `iterate するPASS` 表 (§7.3) に対する補強が必要。

7. **rigged + alpha MASK が R-MA 行で正しく `pushRiggedMaskBatches(PASS_ALPHA_MASK_RIGGED)` 経由になるか**: registerFace は PASS_ALPHA_MASK_RIGGED を直接生成しない (llvovolume.cpp:7123 等で PASS_ALPHA_MASK を渡し、registerFace 内で `passType += 1` で PASS_ALPHA_MASK_RIGGED 化される、5621-5625)。本書 §3.2 表では正しく `pushRiggedMaskBatches(PASS_ALPHA_MASK_RIGGED)` と書いたが、これは「+1 規則が常に作用」前提の演繹。実機 group->mDrawMap[PASS_ALPHA_MASK_RIGGED] が空でない (= rigged MASK face が実際に register されている) ことの直接確認はしていない。

8. **`canRenderAsMask()` の判定詳細**: llvovolume.cpp:7148 で no-material is_alpha が MASK 化される条件。`LLFace::canRenderAsMask` の中身は `lldrawpool.cpp` でない別ファイルにあり、本書 scope では追っていない。BLEND と MASK の境界判定として裏で動いているが、R-MA / N-MA 行の「どの face がここに来るか」の正確な絞り込みは未確定。

9. **POOL_WATER 描画と R-BL / N-BL / W-BL の depth 順序**: POST_WATER と PRE_WATER の境界は POOL_WATER (= 水面描画) を挟んで配置されているが、水面自体が `mRT->screen.depth` をどう変えるか (= mAYAAlphaColor plate との depth 共有が水面で破壊されないか) は本書では追っていない。

10. **`gPipeline.mRT == &gPipeline.mMainRT` 制約の意味**: lldrawpoolalpha.cpp:226 で `use_alpha_rt` 条件に含まれる。preview / profile / probe path で `mRT != &mMainRT` となるとき alpha plate が無効化される理由は コメント (lldrawpoolalpha.cpp:213-215) にあるが、preview / probe path の具体的な mRT 切替地点 (`LLPipeline::setMainRenderTarget` 等?) は本書未追跡。

---

## §9 関連リファレンス

- `docs/specs/ayastorm-attachment-rendering-routing.md` — 装着物側 全 pool × shader × dispatcher (canary 1〜4 の C++/GLSL 配線)
- `docs/specs/ayastorm-rez-object-rendering-routing.md` — Rez Object 側 全 pool × shader × dispatcher (canary 11/12 の C++/GLSL 配線)
- `docs/specs/ayastorm-deferred-shader-routing.md` — gbuffer flag → softenLightF 分岐の対応 (lit 計算側)
- `docs/specs/ayastorm-gbuffer3-trace.md` — gbuffer3 storage 仕様 (alpha channel が無いことの確認)
- `docs/specs/ayastorm-double-alpha-attachment-fix-handoff.md` — 本書作成の出発点 (Phase 1/2/2.1 全 reject + 6 カテゴリ trace pivot 指示)

---

## §10 詳細ケース展開 (8 カテゴリ × fullbright/glow/alpha 直交 3 軸 + multi-avatar)

### §10.0 §10 のスコープと前提

§3.2 の 8 カテゴリ表を出発点に、TE-level 直交 3 軸 (fullbright / glow / alpha BLEND) で各カテゴリを更に細分化した「同一 face が register される PASS と描画される dispatcher の完全列挙」を確定する。

- 対象軸: **R** = rigged 装着物 / **N** = non-rigged 装着物 / **P** = world rez prim (`mAttachedToAvatar.isNull()`、§3.2 の **W** 行と同 dispatcher。表記は AYA さん指示の **P** を採用)。
- 属性軸 (face 1 枚に同時付与可能):
  - **+alpha** = (a) `gltf_mat && alphaMode==BLEND` (llvovolume.cpp:6993) または (b) `blinn_phong_alpha = te->getColor().mV[3] < 0.999` (llvovolume.cpp:6979-6984) または (c) `mat && DIFFUSE_ALPHA_MODE_BLEND` (llvovolume.cpp:7114-7118) または (d) material_pass で `getShaderMask` index が 1/5/9/13 (llvovolume.cpp:7065-7083) → 結果として `PASS_ALPHA` に register される経路を踏むこと。
  - **+fullbright** = `teFullbrightEnabled(te)` (llvovolume.cpp:7012 / 5636) → `LLFace::FULLBRIGHT` state または TE fullbright flag set。
  - **+glow** = `te->getGlow() > 0.f` (llvovolume.cpp:7273)。**PASS_GLOW 独立追加は `!is_alpha && LLPipeline::sRenderGlow && te->getGlow() > 0.f` 限定** (llvovolume.cpp:7273-7283)。alpha BLEND 時は PASS_GLOW に register **されず**、alpha pool 内 emissive 2nd pass (TYPE_EMISSIVE vertex buffer 経由、lldrawpoolalpha.cpp:1000-1026) で同 face を 2 度引いて bloom を出す。
- 「不明」 = 静的 trace では決定できなかった項目。§8 に追加する。

### §10.1 1 avatar の場合の 24 ケース描画順

`base` = +alpha なし / +fullbright なし / +glow なし の素のカテゴリ (§3.2 の R-OP / N-OP / W-OP 行に対応)。

#### §10.1.1 R-OP / N-OP / P-OP base / +alpha / +fullbright / +glow / +alpha+fullbright / +alpha+glow / +fullbright+glow / +alpha+fullbright+glow

`base` = `!gltf_mat && !mat && !is_alpha && !shiny && !fullbright` (llvovolume.cpp:7235-7239) → `PASS_SIMPLE` (rigged は +1 → `PASS_SIMPLE_RIGGED`)。

P-OP も R-OP / N-OP と同 dispatcher (§3.2 の W-OP に対応)。Tree (`LLDrawPoolTree`) と Grass (`LLDrawPoolGrass`) は P 側固有 path だが、本表では `vobj->isMesh()` または default prim 経路の P (= `LLDrawPoolSimple` 経由) のみを対象 (Tree は `LLDrawInfo` を経由せず drawRange 直叩き、rez-object-routing §3 参照)。

| ケース | register される PASS 全部 | 各 PASS 担当 Pool / dispatcher | 描画順 (POOL enum 番号順) | 同 frame 内 draw call 回数 | 主視覚効果 |
|---|---|---|---|---|---|
| **R-OP base** | `PASS_SIMPLE_RIGGED` (llvovolume.cpp:7239 + 5621 で +1) | POOL_SIMPLE / `LLDrawPoolSimple::renderDeferred` `pushRiggedBatches(PASS_SIMPLE_RIGGED)` lldrawpoolsimple.cpp:110 | 1) POOL_SIMPLE deferred (gbuffer) | 1 | gbuffer 直書き → softenLight で lit 統合 |
| **R-OP +alpha** = R-BL (§3.2 R-BL に降格) | `PASS_ALPHA_RIGGED` (llvovolume.cpp:7161 等 + +1) | POOL_ALPHA_POST_WATER (= POST) / POOL_ALPHA_PRE_WATER (= PRE) `LLDrawPoolAlpha::renderPostDeferred` → `forwardRender(true)` → `renderAlpha(rigged=true)` lldrawpoolalpha.cpp:444 → drawRange lldrawpoolalpha.cpp:991 | 1) POOL_ALPHA_PRE_WATER renderPostDeferred (rigged-first lldrawpoolalpha.cpp:286-294) 2) POOL_ALPHA_POST_WATER renderPostDeferred (non-rigged → rigged lldrawpoolalpha.cpp:277-294) | 1 (depth prepass を含めると最大 2 — depth-only pass lldrawpoolalpha.cpp:326-353 が POST_WATER で発火) | forward 合成、depth-write ON (rigged 単独条件 lldrawpoolalpha.cpp:403)、mAYAAlphaColor plate redirect (POST_WATER + use_alpha_rt 時) |
| **R-OP +fullbright** | `PASS_FULLBRIGHT_RIGGED` (llvovolume.cpp:7039 / 7223 + +1) | POOL_FULLBRIGHT / `LLDrawPoolFullbright::renderPostDeferred` `pushRiggedBatches(PASS_FULLBRIGHT_RIGGED)` lldrawpoolsimple.cpp:180 | 1) POOL_FULLBRIGHT renderPostDeferred (POOL_GLOW より前) | 1 | forward fullbright (大気適用なし or 一部適用、softenLight に乗らない) |
| **R-OP +glow** | `PASS_SIMPLE_RIGGED` + `PASS_GLOW_RIGGED` (llvovolume.cpp:7239 + 7281 で independent if 重複 register) | (1) POOL_SIMPLE / `LLDrawPoolSimple::renderDeferred` `pushRiggedBatches(PASS_SIMPLE_RIGGED)` lldrawpoolsimple.cpp:110 (2) POOL_GLOW / `LLDrawPoolGlow::renderPostDeferred` `pushRiggedBatches(PASS_GLOW_RIGGED)` lldrawpoolsimple.cpp:67 | 1) POOL_SIMPLE deferred (gbuffer) 2) POOL_GLOW renderPostDeferred (additive blend、depth-write OFF lldrawpoolsimple.cpp:57) | **2** (= simple gbuffer + glow additive) | base 色 + bloom alpha 加算 |
| **R-OP +alpha+fullbright** | `PASS_ALPHA_RIGGED` (llvovolume.cpp:7022 / 7027 / 7043 で fullbright+alpha は `PASS_ALPHA` に降格 + +1) | POOL_ALPHA_POST/PRE_WATER `forwardRender(true)` → drawRange | R-OP +alpha と同 | 1 (depth prepass 含めれば 2) | forward alpha 合成、shader 内で `params.mFullbright` true 検出 → fullbright_shader bind (lldrawpoolalpha.cpp:891-901)。**alpha pool 内 fullbright permutation = fullbright shader を bind するが gbuffer は経由しない** |
| **R-OP +alpha+glow** | `PASS_ALPHA_RIGGED` のみ (llvovolume.cpp:7273 `!is_alpha` ガードで PASS_GLOW 独立追加なし) | POOL_ALPHA_POST/PRE_WATER `forwardRender(true)` → drawRange + alpha pool 内 emissive 2nd pass (lldrawpoolalpha.cpp:1000-1026、`params.mVertexBuffer->hasDataType(TYPE_EMISSIVE)` 条件、`PRE_WATER` 時は除外 lldrawpoolalpha.cpp:1001) → `renderRiggedEmissives` lldrawpoolalpha.cpp:713 | 1) POOL_ALPHA_POST_WATER renderPostDeferred 内で BLEND 描画 → 直後 emissive pass で additive bloom (BF_ZERO/BF_ONE/BF_ONE/BF_ONE lldrawpoolalpha.cpp:1068) | **2** (= alpha BLEND base + emissive additive) — ただし PRE_WATER では emissive pass がスキップされ **1 回** | bloom が glow ではなく **alpha pool emissive** 経由で出る。POOL_GLOW は経由しない |
| **R-OP +fullbright+glow** | `PASS_FULLBRIGHT_RIGGED` + `PASS_GLOW_RIGGED` (llvovolume.cpp:7039 + 7281 で重複 register) | (1) POOL_FULLBRIGHT renderPostDeferred (2) POOL_GLOW renderPostDeferred | 1) POOL_FULLBRIGHT (BT_ALPHA blend、depth ON?) 2) POOL_GLOW (additive、depth-write OFF) | **2** | fullbright 色 + bloom 加算 |
| **R-OP +alpha+fullbright+glow** | `PASS_ALPHA_RIGGED` のみ (PASS_GLOW 独立追加なし `!is_alpha` ガード llvovolume.cpp:7273) | POOL_ALPHA_POST/PRE_WATER `forwardRender(true)` + alpha pool 内 emissive 2nd pass (TYPE_EMISSIVE 条件で発火) | R-OP +alpha+glow と同 | **2** (POST_WATER) / 1 (PRE_WATER) | alpha BLEND + fullbright shader + emissive bloom |
| **N-OP base** | `PASS_SIMPLE` (llvovolume.cpp:7239) | POOL_SIMPLE / `LLDrawPoolSimple::renderDeferred` `pushBatches(PASS_SIMPLE)` lldrawpoolsimple.cpp:106 | 1) POOL_SIMPLE deferred | 1 | gbuffer 直書き |
| **N-OP +alpha** = N-BL (§3.2) | `PASS_ALPHA` | POOL_ALPHA_POST/PRE_WATER `forwardRender()` (non-rigged) → `renderAlpha(rigged=false)` lldrawpoolalpha.cpp:444 → drawRange | 1) POOL_ALPHA_PRE_WATER (PRE 時 rigged-first 元順 lldrawpoolalpha.cpp:289-294) 2) POOL_ALPHA_POST_WATER (POST 時 non-rigged 先 lldrawpoolalpha.cpp:283-284) | 1 (depth prepass 含めれば 2 — POST_WATER で発火) | **depth-write OFF** (POST_WATER 通常 path、`write_depth = rigged ∨ ...` lldrawpoolalpha.cpp:403-408 で non-rigged は false)、PRE_WATER は ON |
| **N-OP +fullbright** | `PASS_FULLBRIGHT` | POOL_FULLBRIGHT renderPostDeferred lldrawpoolsimple.cpp:174 | 1) POOL_FULLBRIGHT renderPostDeferred | 1 | forward fullbright |
| **N-OP +glow** | `PASS_SIMPLE` + `PASS_GLOW` | (1) POOL_SIMPLE deferred (2) POOL_GLOW renderPostDeferred lldrawpoolsimple.cpp:62 | 1) POOL_SIMPLE deferred 2) POOL_GLOW renderPostDeferred | **2** | base + bloom |
| **N-OP +alpha+fullbright** | `PASS_ALPHA` (fullbright+alpha は PASS_ALPHA に降格 llvovolume.cpp:7022/7027/7043) | POOL_ALPHA_POST/PRE_WATER `forwardRender()` 経由 | N-OP +alpha と同 | 1 | alpha BLEND + fullbright shader bind (`params.mFullbright` 検出) |
| **N-OP +alpha+glow** | `PASS_ALPHA` のみ (PASS_GLOW 独立追加なし `!is_alpha`) | POOL_ALPHA_POST/PRE_WATER `forwardRender()` + alpha pool 内 emissive 2nd pass `renderEmissives` (non-PBR) lldrawpoolalpha.cpp:686 または `renderPbrEmissives` lldrawpoolalpha.cpp:699 (gltf_mat 有無で振分 lldrawpoolalpha.cpp:1006/1017) | POST_WATER の場合 1) BLEND 描画 2) 直後 emissive additive | **2** (POST_WATER) / 1 (PRE_WATER) | bloom が alpha pool emissive 経由 |
| **N-OP +fullbright+glow** | `PASS_FULLBRIGHT` + `PASS_GLOW` | POOL_FULLBRIGHT + POOL_GLOW | 1) POOL_FULLBRIGHT 2) POOL_GLOW | **2** | |
| **N-OP +alpha+fullbright+glow** | `PASS_ALPHA` のみ | POOL_ALPHA + emissive 2nd pass | N-OP +alpha+glow と同 | **2** (POST) / 1 (PRE) | |
| **P-OP base / +alpha / +fullbright / +glow / +alpha+fb / +alpha+glow / +fb+glow / +alpha+fb+glow** | N-OP 各ケースと **同一 PASS / 同一 dispatcher / 同一描画順** (`mAttachedToAvatar.isNull()` flag のみ違う) | N-OP と同 | N-OP と同 | N-OP と同 | dispatcher 視点では区別不能、`LLDrawInfo::mAttachedToAvatar` の値のみで分離。**Tree pool は §3.2 W-OP 注釈の通り別 path で本表外** |

##### §10.1.1 用補足

- POOL 番号順 (lldrawpool.h:57-79): POOL_SIMPLE(60) < POOL_FULLBRIGHT(61) < POOL_GLOW(73) < POOL_ALPHA_PRE_WATER(74) < POOL_WATER(76) < POOL_ALPHA_POST_WATER(77)。
- renderGeomDeferred (pipeline.cpp:5077-5160) は POOL_SIMPLE 等 deferred 系を pool 番号順に dispatch、renderGeomPostDeferred (pipeline.cpp:5244-5350) は POOL_FULLBRIGHT 以降を pool 番号順に dispatch (POOL_FULLBRIGHT → POOL_BUMP → POOL_GLTF_PBR → POOL_GLOW → POOL_ALPHA_PRE_WATER → POOL_WATER → POOL_ALPHA_POST_WATER)。
- POOL_GLOW (=73) は POOL_ALPHA_PRE_WATER (=74) より前。= **opaque +glow の bloom additive は alpha BLEND より前に書き終わる**。
- POOL_FULLBRIGHT_ALPHA_MASK (=70) は POOL_AVATAR (=71) より前、POOL_GLOW (=73) より前。POOL_BUMP の postDeferred は POOL_FULLBRIGHT より後 (POOL_BUMP=62 だが postDeferred の iter は番号順 → POOL_FULLBRIGHT(61) → POOL_BUMP(62))。`grep -n "iter1" pipeline.cpp:5244-5305` 経由で iter 順は pool list の昇順で確定。
- alpha pool 内 emissive 2nd pass は **同 group の同 face を 2 度引く** (lldrawpoolalpha.cpp:1019 で emissives リストに push → 同 group ループ末尾で `renderEmissives` 1022 → `drawRange` 682)。`PRE_WATER` は emissive 2nd pass を抑制 (lldrawpoolalpha.cpp:1001 `getType() != POOL_ALPHA_PRE_WATER`)。

#### §10.1.2 P-OP の Tree / Grass 補足 (本書 scope 外注釈)

`LLDrawPoolTree::renderDeferred` (lldrawpooltree.cpp) は LLDrawInfo 経由でなく `mDrawFace` 直 iterate → drawRange。registerFace を踏まないので PASS_* enum 上のどこにも乗らない (= 上表の 24 ケースに直接マップしない)。Grass は `PASS_GRASS` 単独で `LLDrawPoolGrass::renderDeferred` `pushBatches(PASS_GRASS)` lldrawpoolsimple.cpp:145。+alpha / +fullbright / +glow の組合せは LL 仕様上 Grass / Tree では発生しない (TE による fullbright/glow toggle が無い)。

### §10.2 2 以上の avatar が重なった場合の描画順

avatar A と avatar B が camera から見て重なっている場合、両 avatar とも §10.1 の R-* / N-* 全 24 ケースを装着していると仮定し、両者がどう mix されて描画されるかを sort key / list 構造から確定する。

#### §10.2.1 確定表

| ケース (2 avatar 重なり時) | 描画順序 (A 全カテゴリ → B 全カテゴリ ? カテゴリ単位で A B mix ?) | sort key / grouping 起源 | file:line |
|---|---|---|---|
| **R-OP / R-MA** の avatar 間順序 (`PASS_SIMPLE_RIGGED` 等 deferred opaque rigged) | 不明 (LLCullResult::mRenderMap[PASS_*_RIGGED] iterate 順は本書 scope 内で確定できなかった、§8 へ追加) | `gPipeline.beginRenderMap(PASS_*_RIGGED)` / `endRenderMap` の iterate 順は LLCullResult の `mRenderMap` 配列 push 順に依存。push 発火は `pipeline.cpp:4515-4551` 周辺の cull traversal だが **sort 行は静的 trace では見つからず** | (不明) |
| **N-OP / N-MA / W-OP / W-MA** の avatar 間順序 (`PASS_SIMPLE` 等 deferred opaque non-rigged) | 不明 (R-OP と同 — mRenderMap[PASS_SIMPLE] iterate 順は cull push 順に依存、静的 trace で確定できず) | mRenderMap push は cull traversal、sort 行は本書 scope 内で発見できず | (不明) |
| **R-BL (rigged alpha BLEND)** の avatar 間順序 | **avatar 単位で grouping** → A の R-BL 全部 (mRiggedAlphaGroups 内 A の group) → B の R-BL 全部、または B → A (ポインタ比較順)。同 avatar 内では `mRenderOrder` 降順 (= 大きい mRenderOrder が先) | `mRiggedAlphaGroups` sort = `CompareRenderOrder` llspatialpartition.h:276-287 → 一次キー `lhs->mAvatarp < rhs->mAvatarp` (ポインタ比較、llspatialpartition.h:282)、二次キー `lhs->mRenderOrder > rhs->mRenderOrder` (llspatialpartition.h:285)。sort 発火 pipeline.cpp:4596。`mAvatarp` / `mRenderOrder` set は `LLVOAvatar::updateAttachmentVisibility` 内 `group->mAvatarp = this; group->mRenderOrder = draw_order++;` (llvoavatar.cpp:3472-3473)、`draw_order` は avatar 自身の local counter (llvoavatar.cpp:3400) で 0 から増分 — avatar 内 attachment 走査順依存 | llspatialpartition.h:276-287, pipeline.cpp:4596, llvoavatar.cpp:3400/3472/3473 |
| **N-BL / W-BL (non-rigged alpha BLEND)** の avatar 跨り | **avatar 単位の grouping 無し、depth 順に mix**。A の non-rigged 装着 alpha BLEND と B の non-rigged 装着 alpha BLEND と world prim alpha BLEND が **同じ `mAlphaGroups` list に共存**し、group の `mDepth` 降順で sort される (back-to-front) | `mAlphaGroups` sort = `CompareDepthGreater` llspatialpartition.h:268-274 → `lhs->mDepth > rhs->mDepth`。sort 発火 pipeline.cpp:4593。distance update は CAMERA_WORLD + 非 cube snapshot 時のみ (pipeline.cpp:4523-4534)。LLDrawInfo 内 per-face sort は `CompareDistanceGreater` (llvovolume.cpp:6687、distance_sort=true 時) | llspatialpartition.h:268-274, pipeline.cpp:4593, llvovolume.cpp:6687 |
| **R-BL ↔ N-BL ↔ W-BL の混在順序 (= rigged ↔ non-rigged BLEND 跨ぎ)** | `LLDrawPoolAlpha::renderPostDeferred` 内の **`forwardRender(rigged)` 呼出 2 回** で順序が決まる。POST_WATER: **non-rigged (N-BL ∪ W-BL の depth mix) 先 → rigged (R-BL の avatarp mix) 後** lldrawpoolalpha.cpp:277-294 (AYAstorm fix)。PRE_WATER / HUD: rigged 先 → non-rigged 後 lldrawpoolalpha.cpp:286-294 (元順を維持) | lldrawpoolalpha.cpp:277-294 (POST_WATER swap to non-rigged-first), lldrawpoolalpha.cpp:286-294 (PRE_WATER rigged-first 元順) |
| **R-OP の avatar (e.g. avatar B) と R-BL の avatar (e.g. avatar A) が頂点空間で貫通している場合の depth 整合** (= 別 avatar の異 pool 跨り) | **R-OP は POOL_SIMPLE 等 deferred pass で gbuffer + depth 直書き** (renderGeomDeferred、pipeline.cpp:5077-5160) → renderDeferredLighting で lit 統合 → **その後** renderGeomPostDeferred で R-BL の POOL_ALPHA_POST_WATER が depth-write ON (rigged 単独条件 lldrawpoolalpha.cpp:403) で forward 合成。= **A の R-BL は B の R-OP に対し正しく depth-test される (GL_LEQUAL、depth は共有 mRT->screen.depth)**。逆向き (R-BL を先に描いた後で R-OP が来る) は時系列上発生しない (POOL_SIMPLE deferred は POOL_ALPHA_POST_WATER renderPostDeferred より前) | POOL 番号順 lldrawpool.h:60(POOL_SIMPLE) vs lldrawpool.h:77(POOL_ALPHA_POST_WATER) + renderGeomDeferred → renderDeferredLighting → renderGeomPostDeferred (pipeline.cpp:11649) のフェーズ順 |
| **N-BL ↔ R-BL の depth 整合 (avatar A の N-BL と avatar B の R-BL)** | POST_WATER で非対称: **N-BL は depth-write OFF** (lldrawpoolalpha.cpp:403-408)、**R-BL は depth-write ON** (rigged 条件)。N-BL を先描画 (non-rigged-first 順) → R-BL が後で depth-test。R-BL の方は N-BL の **depth に痕跡が無い** ため、N-BL の手前/奥に関係なく opaque z (sky 等) と depth-test。これが **二重アルファブロック regression の depth 非対称の根拠** (§3.3 で既出) | lldrawpoolalpha.cpp:403-408 (write_depth 条件), lldrawpoolalpha.cpp:277-294 (POST_WATER 順序) |

#### §10.2.2 「重なり時の混在ルール」要約 (= 上表の構造的読み)

- **opaque (R-OP / N-OP / W-OP / R-MA / N-MA / W-MA)** の avatar 間順序は **mRenderMap[PASS_*] の iterate 順 = cull push 順** に従う。深さ sort は無い (deferred opaque は gbuffer 上で depth-test で解決するため front-to-back sort も無効化されている)。具体的な push 順 (BFS / DFS / spatial octree 順) は本書では確定できず → §8 に追加。
- **alpha BLEND (R-BL / N-BL / W-BL)** は 3 系統で異なる:
  - R-BL: **avatar 単位でひとかたまり** (CompareRenderOrder の `mAvatarp` 一次キー)。avatar 間順序は **ポインタ比較** (= 起動時 / 装着時の alloc 順依存 = 視覚的には決定不能)。
  - N-BL: **depth 順 mix** (CompareDepthGreater)。複数 avatar の non-rigged 装着 alpha BLEND は **avatar 単位で mix されず**、group の mDepth で完全 back-to-front sort される。
  - W-BL: N-BL と同 list (`mAlphaGroups`)、同 sort key で完全混在。
- **POST_WATER 内の rigged↔non-rigged 跨ぎ順序**: AYAstorm fix で `forwardRender()` (non-rigged 全 avatar + world) → `forwardRender(true)` (rigged 全 avatar) の順に **default 化**。PRE_WATER / HUD は元順 (rigged → non-rigged) を維持。

### §10.3 同 face が複数 PASS に register される pattern 表

llvovolume.cpp:6960-7250 の registerFace 分岐は `else if` ベースで **基本は排他**だが、**末尾に独立した `if` ブロックが 2 つ**ある:

1. `if (!is_alpha && hud_group)` (llvovolume.cpp:7261-7271) → `use_legacy_bump` 時に `PASS_BUMP` を **追加** register。
2. `if (!is_alpha && LLPipeline::sRenderGlow && te->getGlow() > 0.f)` (llvovolume.cpp:7273-7283) → `gltf_mat ? PASS_GLTF_GLOW : PASS_GLOW` を **追加** register。

= **alpha BLEND face は PASS_GLOW / PASS_GLTF_GLOW 追加 register を踏まない**。= alpha BLEND glow は POOL_GLOW 経由ではなく **alpha pool 内 emissive 2nd pass** (lldrawpoolalpha.cpp:1000-1026) 経由でのみ出る。

#### §10.3.1 属性組合せ → register PASS / 描画 dispatcher 全列挙

| 属性組合せ | 同 face が register される PASS 全列挙 | 描画 dispatcher 全列挙 | 同 frame 内 draw call 回数 |
|---|---|---|---|
| **opaque (base)** | `PASS_SIMPLE` (llvovolume.cpp:7239 周辺) | `LLDrawPoolSimple::renderDeferred` `pushBatches/pushRiggedBatches(PASS_SIMPLE[_RIGGED])` lldrawpoolsimple.cpp:106/110 | 1 |
| **opaque + glow** | `PASS_SIMPLE` + `PASS_GLOW` (llvovolume.cpp:7239 + 7281 で独立 if 重複 register) | (1) `LLDrawPoolSimple::renderDeferred` (2) `LLDrawPoolGlow::renderPostDeferred` `pushBatches/pushRiggedBatches(PASS_GLOW[_RIGGED])` lldrawpoolsimple.cpp:62/67 | **2** |
| **opaque + glow (gltf_mat)** | `PASS_GLTF_PBR` + `PASS_GLTF_GLOW` (llvovolume.cpp:7004 + 7277) | (1) `LLDrawPoolGLTFPBR::renderDeferred` `pushGLTFBatches(PASS_GLTF_PBR)` (2) `LLDrawPoolGLTFPBR::renderPostDeferred` PBR_GLOW 分岐 lldrawpoolpbropaque.cpp:87-90 | **2** |
| **opaque + fullbright** | `PASS_FULLBRIGHT` (llvovolume.cpp:7039 / 7223、`PASS_SIMPLE` は走らない、else if 排他) | `LLDrawPoolFullbright::renderPostDeferred` `pushBatches/pushRiggedBatches(PASS_FULLBRIGHT[_RIGGED])` lldrawpoolsimple.cpp:174/180 | 1 |
| **opaque + fullbright + glow** | `PASS_FULLBRIGHT` + `PASS_GLOW` (llvovolume.cpp:7039 + 7281 重複 register、PASS_FULLBRIGHT への else if 後に独立 if で PASS_GLOW 追加) | (1) `LLDrawPoolFullbright::renderPostDeferred` (2) `LLDrawPoolGlow::renderPostDeferred` | **2** |
| **MASK** (= alpha MASK only) | `PASS_ALPHA_MASK` (llvovolume.cpp:7123 / 7156) | `LLDrawPoolAlphaMask::renderDeferred` `pushMaskBatches/pushRiggedMaskBatches(PASS_ALPHA_MASK[_RIGGED])` lldrawpoolsimple.cpp:123/127 | 1 |
| **MASK + glow** | `PASS_ALPHA_MASK` + `PASS_GLOW` (llvovolume.cpp:7123 + 7281、`!is_alpha` ガードは「BLEND でない」を意味するため MASK は通過、独立 if で PASS_GLOW 追加) | (1) `LLDrawPoolAlphaMask::renderDeferred` (2) `LLDrawPoolGlow::renderPostDeferred` | **2** |
| **MASK + fullbright** | `PASS_FULLBRIGHT_ALPHA_MASK` (llvovolume.cpp:7018 / 7123 / 7152 / 7219) | `LLDrawPoolFullbrightAlphaMask::renderPostDeferred` `pushMaskBatches/pushRiggedMaskBatches(PASS_FULLBRIGHT_ALPHA_MASK[_RIGGED])` lldrawpoolsimple.cpp:206/212 | 1 |
| **MASK + fullbright + glow** | `PASS_FULLBRIGHT_ALPHA_MASK` + `PASS_GLOW` | (1) `LLDrawPoolFullbrightAlphaMask::renderPostDeferred` (2) `LLDrawPoolGlow::renderPostDeferred` | **2** |
| **alpha BLEND (base)** | `PASS_ALPHA` (llvovolume.cpp:6995 / 7022 / 7027 / 7043 / 7050 / 7068 等 / 7127 / 7161) | `LLDrawPoolAlpha::renderPostDeferred` → `forwardRender(rigged)` → `renderAlpha` → drawRange | 1 (depth prepass を含めれば最大 2、POST_WATER で発火) |
| **alpha BLEND + glow** | **`PASS_ALPHA` のみ** (llvovolume.cpp:7273 `!is_alpha` ガードで PASS_GLOW 独立追加なし) | `LLDrawPoolAlpha::renderPostDeferred` BLEND drawRange + alpha pool 内 emissive 2nd pass `renderEmissives` (lldrawpoolalpha.cpp:686) or `renderPbrEmissives` (lldrawpoolalpha.cpp:699) — `params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE)` 条件で同 face が emissives リストに push (lldrawpoolalpha.cpp:1000-1026)、PRE_WATER は除外 (1001) | **2** (POST_WATER、BLEND + emissive additive) / **1** (PRE_WATER、emissive スキップ) |
| **alpha BLEND + fullbright** | `PASS_ALPHA` (fullbright+is_alpha は PASS_ALPHA に降格 llvovolume.cpp:7027 / 7043 / `material_pass` で getShaderMask idx 1/5/9/13 → PASS_ALPHA llvovolume.cpp:7065-7083) | `LLDrawPoolAlpha::renderPostDeferred` 内で `params.mFullbright` true 検出 → fullbright_shader bind (lldrawpoolalpha.cpp:891-901 / 925-927) | 1 |
| **alpha BLEND + fullbright + glow** | `PASS_ALPHA` のみ (PASS_GLOW 独立追加なし) | `LLDrawPoolAlpha::renderPostDeferred` BLEND (fullbright shader) + alpha pool 内 emissive 2nd pass | **2** (POST_WATER) / **1** (PRE_WATER) |

##### §10.3.1 用補足

- `registerFace` の戻り値: void (lldrawpool.h 周辺、`LLRenderPass::registerFace(LLSpatialGroup*, LLFace*, U32)` シグネチャ、本書では実体未追跡だが C++ で void 関数として呼ばれる箇所複数あり llvovolume.cpp:6995/7000/7004/... 全て式文)。**重複登録防止は無い** — 同 group->mDrawMap[passType] の vector に push_back されるだけ (llvovolume.cpp:5628 `LLSpatialGroup::drawmap_elem_t& draw_vec = group->mDrawMap[passType]`)。= **同一 face を同 PASS に複数回 registerFace すると DrawInfo が重複生成され、drawRange が複数回引かれる**。実際 §10.3.1 の各「+glow」行は **独立 if で 2 度別 PASS に register** されるので重複 register ではなく異 PASS 登録 = 重複描画問題ではなく意図的 2 path 描画。
- llvovolume.cpp:5638-5645 の `llassert(false)` ガード: `!fullbright && type != PASS_GLOW && !hasDataType(TYPE_NORMAL)` で assert。**PASS_GLOW 登録時は法線無しでも OK** (= glow pool は法線を読まない bloom 専用 path で、頂点シェーダから unbind されている)。
- 「alpha BLEND が emissive 2nd pass を発火するか」の決定要素は `params.mVertexBuffer->hasDataType(LLVertexBuffer::TYPE_EMISSIVE)` のみ (lldrawpoolalpha.cpp:1002)。`te->getGlow() > 0` 経由で TYPE_EMISSIVE が vertex buffer に追加されるが、その追加判定は `LLVOVolume::genDrawInfo` 内の buffer mask 構築 path にあり本書では追っていない (§8 に追加)。
- alpha BLEND + fullbright で `params.mFullbright` が true となる set 場所は LLDrawInfo 構築側 (llvovolume.cpp の draw_info ctor 周辺) で確定する。本書 §10 scope では「shader bind の段で true 判定して fullbright_shader に切替」事実のみ確認 (lldrawpoolalpha.cpp:891-901)。

### §10.4 新たに発見した「不明」項目 (§8 に追加)

11. **mRenderMap[PASS_*] (= deferred opaque) iterate 順 (R-OP / N-OP / W-OP / R-MA / N-MA / W-MA の avatar 間 / cull push 順)**: §3.2 表で「`pushBatches` は batch 互換 sort のみ、距離 sort 無し」と確定したが、複数 avatar / 複数 group が同 PASS_* に register された際の **avatar 間 iterate 順 = LLCullResult が push した順** の決定論理 (BFS / DFS / spatial octree 順 / 距離順 / その他) は静的 trace で確定できなかった。`pipeline.cpp:4515-4551` 周辺の cull traversal で `mRenderMap[type].push_back(info)` 相当の push 行と、その traversal が octree のどの順序で進むかを追加 trace する必要あり。

12. **`params.mFullbright` flag の set 経路**: §10.3.1 で alpha BLEND + fullbright → alpha pool 内で `params.mFullbright` true 検出 → fullbright_shader bind (lldrawpoolalpha.cpp:891) と確定したが、`LLDrawInfo::mFullbright` の set 場所は `llvovolume.cpp` の draw_info ctor / 構築周辺で行われる (推定) ものの本書 scope では追っていない。`teFullbrightEnabled(te)` の結果が draw_info にどう転写されるか line 単位の確認が必要。

13. **`LLVertexBuffer::TYPE_EMISSIVE` mask の vertex buffer への追加判定**: alpha BLEND + glow の 2 nd pass (alpha pool emissive) は `params.mVertexBuffer->hasDataType(TYPE_EMISSIVE)` で発火するが、TYPE_EMISSIVE が vertex buffer mask に追加される条件 (= `te->getGlow() > 0` 単独か、他の条件混在か) は `LLVOVolume::genDrawInfo` の buffer mask 構築 path で確定する必要あり。本書未追跡。

14. **opaque + glow (GLTF) の `PASS_GLTF_GLOW` 描画 dispatcher の dispatch 順 vs `PASS_GLOW`**: 両者は POOL_GLOW (POOL=73) と POOL_GLTF_PBR (POOL=64) の renderPostDeferred という **別 pool 別位置**で発火する。pool iterate 番号順 → POOL_GLTF_PBR (PBR_GLOW) が先、POOL_GLOW (legacy glow) が後。両方の glow が同 frame に出る face が存在する場合 (= legacy material + GLTF material の両方を 1 face で持つケース) があるかどうかは本書 scope 外。

15. **`pushBatches` / `pushRiggedBatches` 内部の batch 互換 sort key の同 avatar 内決定論理**: §3.2 で「`CompareBatchBreaker` / `CompareBatchBreakerRigged` で sort」と確定したが、これは LLDrawInfo の per-face genDrawInfo 内 sort であり、その後 mRenderMap への push 順との関係 (sort 後そのまま push? それとも追加 sort? push 後の vector はその順を保持するか?) は本書未追跡。`pushBatches` 実体 (lldrawpool.cpp 周辺) の iterate 行で確認が必要。
