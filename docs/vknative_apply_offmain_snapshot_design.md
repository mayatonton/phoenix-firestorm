# apply(DrawInfo 構築)の off-main 化 = move-materialize 設計(P1-c γ B.2 v2)

- 状態: 設計者 v2 改訂 2026-07-22 深夜。真実源 = HEAD の実行コード。approve 前。
- 位置づけ: `docs/vknative_avatar_relocate_design.md` §5 Phase 1 P1-c(avatar 描画 off-main)の apply 相を **丸ごと** off-main 化する下位設計。
- **🔀 v1 破棄(2026-07-22 深夜・AYA 裁定)**: v1(build が snapshot の LLPointer を **copy** = 非 atomic refcount を off-main で `ref()` → main と data race = UB)は**設計欠陥**。さらに「materialize は main 固定」への退避案は**分散化の矮小化**として却下。本 v2 = **materialize を丸ごと off-main へ置く**(copy でなく move で refcount 操作をゼロにする)。

---

## 0. 問題(v1 の欠陥)

apply の実体 `registerFace` が LLDrawInfo に設定する payload には共有オブジェクトへの `LLPointer` が多数ある(texture/material/gltf/skininfo/avatar/drawable)。これらの基底は**全て非 atomic `LLRefCount`**(実コード確認):
- `LLTexture : virtual LLRefCount`(lltexture.h:45)/ `LLMaterial : LLRefCount` / `LLGLTFMaterial : LLRefCount` / `LLMeshSkinInfo : LLRefCount` / `LLVertexBuffer : LLRefCount`(llvertexbuffer.h:85)。
- `LLRefCount::ref()` = `mRef++`(非 atomic・llrefcount.h:56)。

⟹ build を off-main 化して snapshot の LLPointer を **copy** すると、共有オブジェクトの `mRef` を off-main で increment = main の並行 ref/unref と **data race(torn RMW)= lost update → 早期 free/leak → heap 破壊**。

**既存 fill worker が安全な不変条件**: LLPointer の ref/unref は **main でのみ**行い、worker は raw pointer を読むだけ。off-main で refcount を触らない。v1 build はこれを破っていた。

## 1. 解 = MOVE(off-main の refcount 操作をゼロにする)

実コード確認: **`LLPointer` の move ctor(llpointer.h:87-90)= `mPointer = ptr.mPointer; ptr.mPointer = nullptr;` = ref/unref を一切しない**。null 先への move 代入も unref なし。

⟹ **build(off-main)は snapshot の LLPointer を LLDrawInfo へ `std::move` = 所有権移転のみ・refcount 操作ゼロ = race 不能**。refcount の増減は全て main で起きる:
- **increment = main**: capture(staging)で snapshot が live からコピー時(既存 = Steps 1-3)。
- **off-main build = move(no-op)**: snapshot の ref が LLDrawInfo へ移る(snapshot 側 null 化)。純増減ゼロ。
- **decrement = main**: ①job/snapshot 破棄時に非 move 分を unref ②LLDrawInfo 破棄(group->mDrawMap clear)時に移転分を unref。

**INV-APPLY(v2)**: off-main が触ってよいのは ①新規 alloc した job-local LLDrawInfo ②immutable snapshot(single-consumer・move 元)③raw 値 のみ。**off-main で共有 refcount を増減しない**(move は no-op)。全 ref/unref は main。

## 2. 3 相設計(capture / build / fold)

```
staging(main)              avatar-domain(off-main)            fold(main, Update-Geom)
─────────────              ──────────────────────             ─────────────────────
genDrawInfo:               runAvatarJobBuild:                 foldBuiltDrawInfo:
 ・VB alloc(mega)           ・batch 判定(raw 値比較)            ・staleness 検証(live face)
 ・LLGeoFaceSnapshot(頂点)  ・new LLDrawInfo(VB-less ctor)     ・mVertexBuffer 張り(ref)+ validateRange
 ・LLDrawInfoSnapshot        ・数値 field 設定                   ・mTextureList[leaderTexIdx]=mTexture(二重分 ref)
   (identity・LLPointer 保持)・snapshot の LLPointer を MOVE     ・setDrawInfo(leaderFace)
   → mSnaps                  ・merged texture を textureList へ  ・mBuilt を group->mDrawMap install
                              MOVE                              ・patchGroup
                             → job->mBuilt(+ 各 draw の
                               VB raw / leaderFace / leaderTexIdx)
```

- **capture(main・staging・既存 Steps 1-3)**: `captureRegisterSnapshot` が live を読み `LLDrawInfoSnapshot`(LLPointer 群を main で ref 保持)を作り `LLGeoFaceApply.mSnaps` へ push。**refcount increment は全てここ(main)**。
- **build(off-main・move materialize)**: `runAvatarJobBuild` が snapshot から LLDrawInfo を丸ごと構築。alloc(VB-less ctor)+ 数値 + LLPointer **move** + batch 判定(raw)。**共有 refcount を一切触らない**。出力 = `job->mBuilt`(per-draw に LLDrawInfo + fold 用 side-data)。
- **fold(main・軽い共有 ref のみ)**: staleness 検証 + **構造的に job 内共有な ref だけ**(mVertexBuffer と leader texture 二重分)+ setDrawInfo + mDrawMap install + patchGroup。

## 3. off-main で move する field / main-fold で ref する field(実コード)

registerFace が LLDrawInfo に設定する全 field の扱い(llvovolume.cpp registerFace + llspatialpartition.h LLDrawInfo):

| LLDrawInfo field | 型 | 相・操作 |
|---|---|---|
| mStart/mEnd/mCount/mOffset | 数値 | build(off-main)= VbSlice/累積 |
| mBatchExtents[2]/mBoundRadius | 数値 | build = snapshot extents から |
| mBump/mShiny/mObjectAlpha/mSpecColor/mEnvIntensity | 値 | build |
| mMaterialID(LLUUID)/mShaderMask/mFSPickerLocalID/mSkinHash | 値 | build |
| mIsSSSTarget/mFullbright/mAlphaMaskCutoff/mDiffuseAlphaMode | 値 | build |
| mTextureMatrix/mModelMatrix/mLastModelMatrix | **raw ptr** | build(copy=ref なし・matrix は別所有) |
| mTexture | LLPointer\<LLViewerTexture\> | **build = `std::move(s.mTexture)`** |
| mSpecularMap/mNormalMap | LLPointer\<LLViewerTexture\> | **build = move** |
| mAvatar/mAttachedToAvatar | LLPointer\<LLVOAvatar\> | **build = move** |
| mSkinInfo | LLConstPointer\<LLMeshSkinInfo\> | **build = move** |
| mMaterial | LLPointer\<LLMaterial\> | **build = move** |
| mGLTFMaterial | LLPointer\<LLFetchedGLTFMaterial\> | **build = move** |
| mSrcDrawable | LLPointer\<LLDrawable\> | **build = move** |
| mTextureList[i](merged 分) | vector\<LLPointer\<LLViewerTexture\>\> | **build = merged snapshot の move** |
| **mVertexBuffer** | LLPointer\<LLVertexBuffer\> | **fold(main)= ref**(構造的に job 全 draw + face + render が共有 = 単一 snapshot から move 不能) |
| **mTextureList[leaderTexIdx]** | 同上 | **fold(main)= `= mTexture`(copy)**(leader 自身の texture が mTexture と textureList に二重 = move は1回のみ可) |

**main-fold の refcount 操作 = 1 draw あたり VB ref 1 + leader-tex-dup 1 の計 ~2 個のみ**(構造的に単一 snapshot から move 不能な共有 ref)。alloc・数値・8 種 LLPointer 移転・batch 判定は全て off-main。**= 矮小化でない・materialize 丸ごと off-main。**

## 4. LLDrawInfo の VB-less ctor(要追加・renderer 自前クラス)

現 ctor(llspatialpartition.cpp:4167)= `mVertexBuffer(buffer)` で VB を ref し `mVertexBuffer->validateRange(...)` で **deref** → null VB 不可・VB を ref する。off-main 構築には使えない。

**追加**: `LLDrawInfo(U16 start, U16 end, U32 count, U32 offset, bool fullbright, U8 bump)` = 数値のみ設定・`mVertexBuffer`/`mTexture` は null・**validateRange しない**。VB 張りと validateRange は fold(main)へ。LLDrawInfo は render 側自前クラス(architecture §7)ゆえ改修可。**憲法 4 対象外**(検出器でない)。

## 5. データ構造(llvovolume.cpp)

- `LLGeoFaceApply.mSnaps`(既存 Steps 1-3)= `std::vector<LLDrawInfoSnapshot>`。**build で move するため非 const 参照でイテレート**。
- job-local build 出力:
  ```
  struct BuiltDraw {
      LLPointer<LLDrawInfo> mInfo;
      LLVertexBuffer* mVb = nullptr;   // fold の VB 張り + off-main batch 比較用(mInfo->mVertexBuffer は fold まで null)
      LLFace* mLeaderFace = nullptr;   // fold の setDrawInfo(identity・off-main で deref しない)
      U8 mLeaderTexIndex = 0xFF;       // fold の leader-tex-dup(FACE_DO_NOT_BATCH_TEXTURES なら未設定)
  };
  ```
  `job->mBuilt` = `std::unordered_map<U32, std::vector<BuiltDraw>>`(pass type → draws・batch 隣接は同 passType 末尾)。
- ⚠️ 旧 `LLGeoRebuildJob.mBuilt`(`draw_map_t`)は本 struct へ置換。

## 6. race-free 証明(v2・命題)

- **build(off-main)が触るもの**: ①新規 `new LLDrawInfo`(alloc = jemalloc thread-safe / 生成物は job-local・publish handoff まで他 thread 不可視)②job-local `BuiltDraw`(worker 専有)③immutable snapshot(move 元・single-consumer)④VbSlice/snapshot の **raw 値読み**(比較)。
- **build が共有 refcount に対して行う操作 = move のみ = increment/decrement ゼロ**(llpointer.h:87-90)。
- **capture(main)= increment / fold(main)= VB+dup の増 / 破棄(main)= decrement**。全 refcount RMW が main = 単一 thread = race 不能。
- publish handoff(`sAvatarPublishMutex`)が build→fold の happens-before を張る = mBuilt/snapshot の可視性担保。
- ⟹ 共有 state への並行非 atomic アクセスは構造的に存在しない(by construction)。
- **正のオラクル(gate)= ①GEOAB(L3 A/B・幾何 byte)沈黙 ②視覚同一 ③main self pub_ms 減(直接計測)④kill-switch inline との A/B 一致 ⑤C_*_RACE 発火 0。**

## 7. 罠(実装時に潰す)

1. **snapshot は single-consumer(move 元)**: build が move した後の snapshot は空。**再読み禁止**。GEOAB/geoAbCheckJob は `mFills`(fill data)を見て `mSnaps` を見ない = 安全。fold も `mBuilt` を使い mSnaps を見ない。
2. **batch 比較の VB**: off-main の `mInfo->mVertexBuffer` は null(fold まで)。batch 隣接比較(`info->mVertexBuffer == vb`)は **`BuiltDraw.mVb`(raw)** で行う。
3. **leader-tex-dup**: leader 自身の texture は `mTexture`(move)+ `mTextureList[leaderTexIdx]`(fold で copy)。off-main では mTexture へ move し textureList の leader slot は空、fold で `= mInfo->mTexture`。
4. **VB ref + validateRange = fold(main)**。off-main は VB-less ctor(validate せず)。
5. **staleness = fold(main)**: live face の getFace/isDead/getGeomCount 照合。stale は該当 draw を drop。build は無条件構築。
6. **mLastModelMatrix = &drawable->mLastVelocityMatrix**(raw ptr)。drawable は snapshot の `mSrcDrawable`(LLPointer→LLDrawInfo へ move)が 1-frame 生存させる = ptr 有効。
7. **material/gltf raw ptr 寿命**: move ゆえ LLDrawInfo が所有権を持つ(既存 LLDrawInfo と同寿命)。
8. **batch-against-preserved 非 merge**: build は job-local `mBuilt` にのみ batch(preserved と非 merge)= draw 数微増・幾何 byte 同一・**縮小申告**。
9. **kill-switch** = `AYASTORM_MT_THREADS=1` → worker 起動せず → routing off → build+fold を main で連続実行(inline 退化)。

## 8. gate(命題様式)

- main の self pub_ms が有意に減(直接計測・捏造不能)+ 視覚同一 + validation 0 + 装置全層沈黙 + **GEOAB verdict クリーン(kernel 0・必須)** + C_*_RACE 0 + kill-switch inline A/B 一致。
- 未証明項: batch 数微増(罠8)の perf 影響・視覚同一で受容可否 AYA。fold の main 残 ref(VB+dup ~2/draw)が pub_ms に占める割合(移設後実測)。

## 9. 実装順(直列・各 compile 検証・2 段階安全化)

Steps 1-3(snapshot 定義 + registerFace 分割 + capture を staging へ)= **完了・保持**(commit `bd5db0a02c`+`caa040a769`)。以降を v2 で作り直す:

- **Step 4(挙動不変・全 main で move materialize + fold を確立)**:
  - LLDrawInfo に VB-less ctor 追加。
  - `BuiltDraw` struct + `job->mBuilt` を `unordered_map<U32, vector<BuiltDraw>>` へ(v1 の draw_map_t 版を置換)。
  - build を「VB-less alloc + 数値 + snapshot LLPointer **move** + batch(BuiltDraw.mVb 比較)」に書換(snapshot 非 const 化)。
  - fold(`foldBuiltDrawInfo`)を「staleness + VB ref + validateRange + leader-tex-dup + setDrawInfo + install」に拡張。
  - applyGeoStaged / registerFace dispatcher で build→mBuilt→fold を **main で連続実行**(挙動不変)。
  - gate = 視覚同一 + GEOAB kernel 0 + validation 0(move の正しさと fold の完全性を移設前に証明)。
- **Step 5(build を off-main へ)**: avatar-domain worker(`runAvatarJobBuild`)が build を job-local mBuilt へ(self のみ)。fold は main 残置(`drainAvatarPublished`)。
- **Step 6(self routing)**: `drainGeoPublishQueue` で self bridge group を `sAvatarJobQueue` へ回す(main apply せず)→ worker build → `sAvatarPublishQueue` → main fold。
- **Step 7 = kill-switch・GEOAB 配線確認 → gate**。

各 Step compile + 可能なら AYA gate。Step 4 は挙動不変ゆえ「move materialize + fold split が正しい」を off-main 移設前に証明できる。

## 10. 申告(縮小・省略・解釈)

- **batch-against-preserved 非 merge(罠8)= 意図的縮小**(draw 微増・視覚同一)。
- **leader-tex-dup + VB ref を fold(main)に残す = 縮小でない**(構造的に単一 snapshot から move 不能な共有 ref のみ・1 draw ~2 個・材料の大宗は off-main)。
- **VB-less ctor 追加 = LLDrawInfo(render 自前)への改修**(architecture §7・憲法 4 対象外)。
- **snapshot 非 const 化(move 元)**: mSnaps を build 後は再利用しない前提(罠1)。
- PASS は宣言しない。本 doc は設計地図・動作証明は §8 gate の実測。
