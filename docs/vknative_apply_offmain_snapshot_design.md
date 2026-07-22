# apply(DrawInfo 構築)の off-main 化 = register-snapshot 設計(P1-c γ B.2)

- 状態: 設計者起草 2026-07-22 深夜。真実源 = HEAD の実行コード。approve 前。
- 位置づけ: `docs/vknative_avatar_relocate_design.md` §5 の Phase 1 P1-c(avatar 描画 off-main)の、**apply 相を off-main 化する下位設計**。
- **supersede**: 会話中の暫定実装 B.2a(「applyGeoStaged を live-read のまま off-main へ routing」)= **C++ data race = UB ゆえ破棄**。本設計がその正しい姿。
- 依拠: registerFace / applyGeoStaged / clearDrawMapStaged / staging(genDrawInfo)の HEAD 全 body 監査(下記 file:line)。

---

## 0. 問題

crowd 本体回復の gate 費目 `pub_ms`(= apply)を self avatar について off-main 化したい。だが apply の実体 `LLVolumeGeometryManager::registerFace`(llvovolume.cpp:6837)は **live main state を大量に read し、face/group に write する**:

- read: face 状態(VB/isState/extents/skinInfo/mAvatar/getSkinHash)・TE/material(getBumpmap/getShiny/getColor/getMaterialParams/**getGLTFRenderMaterial**/getShaderMask/getAlphaMaskCutoff/getDiffuseAlphaMode/getSpecular*)・texture(getTexture/getTextureIndex/getTESpecularMap/getTENormalMap)・drawable transform(getWorldMatrix/getRenderMatrix/getRegion)・selection/RLV(LLSelectMgr/RlvActions/gRlvAttachmentLocks)。
- write: `facep->setDrawInfo`(:7181)・`group->mDrawMap`(draw_vec)。加えて applyGeoStaged が `facep->setVertexBuffer/setGeomIndex/setIndicesIndex`(:6101-6106)。

これらを off-main で live に触れば **data race = UB**。「右クリック編集で TE が変わり得る」等は blocker ではない — **既存 fill worker は全オブジェクトを既に off-main 処理**しており、その解法 = **staging(main)で `LLGeoFaceSnapshot` に immutable 化**。本設計はその解法を identity/DrawInfo まで拡張する。

## 1. 不変条件(fill と同一保証)

**INV-APPLY**: off-main が触ってよいのは ①staging で作った immutable snapshot ②job-local 出力 の 2 つのみ。**shared(live face / TE / material / group / selection)への read/write は staging(main)か fold(main)でのみ行う**。

→ off-main 相に data race が構造的に存在しない(by construction)。1-frame ズレ(staging N の snapshot を apply N+1 で使用)は relocate の通貨と整合。TE 変更は group 再 dirty → 次 frame 追従(fill と同一挙動)。

## 2. 3 相設計(capture / build / fold)

現行 2 相(stage=main / fill=worker / apply=main)を、apply について 3 相へ:

```
staging(main)          worker(off-main)   avatar-domain(off-main)   fold(main, Update-Geom)
─────────────          ───────────────    ─────────────────────     ────────────────────
genDrawInfo:                                                          drainAvatarPublished:
 ・face list                                                          ・staleness 検証(live face)
 ・VB alloc(mega)                                                    ・setVertexBuffer/GeomIndex
 ・LLGeoFaceSnapshot(頂点)  → runVkGeoFill(VB bytes)                 ・setDrawInfo(face)
 ・LLDrawInfoSnapshot(identity) ────────────→ buildDrawInfos:        ・built list を mDrawMap install
                                              ・snapshot→LLDrawInfo   ・patchGroup(bucket fold)
                                              ・batch(among new)      ・geoAbCheckJob / unpin
                                              ・job-local list へ
```

- **capture(staging・main・安全)**: registerFace が今 apply で読む identity/payload を staging で読み `LLDrawInfoSnapshot` に格納。現行 staging は既に te/gltf/fullbright を読んで pass 決定(:8457-)しているので同じ live に main で触れる自然な点。
- **build(off-main・純関数)**: snapshot + staged VB index から LLDrawInfo を alloc・batch し **job-local list** へ。live/group/face に触れない。
- **fold(main・軽)**: built list を受け、staleness 検証 + face write(setVertexBuffer/setDrawInfo)+ mDrawMap install + patchGroup。全 shared write が main = render と順序担保。

## 3. `LLDrawInfoSnapshot`(capture 対象・per (face,pass))

registerFace が LLDrawInfo に設定する全フィールドの source(llvovolume.cpp)。VB range(start/end/offset/count/mVertexBuffer)は staged record(LLGeoFaceApply.mBuffer/mGeomIndex/mGeomCount/mIndicesIndex/mIndicesCount)から得るので snapshot 外。

| snapshot field | 現 apply source(file:line) |
|---|---|
| passType(rigged で +1) | :6884-6893(facep->isState(RIGGED)) |
| fullbright | :6899-6903(type + isState(FULLBRIGHT) + teFullbrightEnabled) |
| hasNormal | :6905-6907(VB typemask・staging で VB alloc 済ゆえ可) |
| bump | :6949(te->getBumpmap・pass 依存) |
| shiny | :6950(te->getShiny) |
| tex(LLPointer) / texIndex | :6952-6954 / gltf 時 :6966-6969 で nullptr 化 |
| tex_mat(ptr) | :6914-6918(facep->mTextureMatrix・TEXTURE_ANIM 条件) |
| model_mat(ptr) | :6920-6944(rigged=null / ANIMATED_CHILD/isActive/region) |
| lastModelMat(ptr) | :7088(&facep->getDrawable()->mLastVelocityMatrix) |
| shader_mask | :6982-7000(mat->getShaderMask) |
| mat_id(LLUUID) | :6965 / :6976 |
| material(LLMaterial*) | :6973(te->getMaterialParams().get()) |
| gltf_mat(ptr) | :6960 |
| avatar(LLPointer) | :7118(facep->mAvatar) |
| skinInfo(LLPointer)/skinHash | :7119 / getSkinHash() |
| srcDrawable(ptr) | :7085(facep->getDrawable()) |
| extents[2] | :7091-7092(facep->mExtents) |
| objectAlpha | :7101(te->getColor().mV[3]) |
| specColor/envIntensity | :7110-7113 / material 時 :7151-7159 |
| specularMap(LLPointer) | :7160(getTESpecularMap) |
| normalMap(LLPointer) | :7165(getTENormalMap) |
| alphaMaskCutoff/diffuseAlphaMode | :7163-7176 |
| isSSSTarget/pickerLocalID/attachedToAvatar | :7128-7135(vobj) |
| hidden(bool) | :6859-6868(isSelected + LLSelectMgr::mHideSelectedObjects + RLV)。**selection/RLV read はここで staging に閉じる** |

## 4. registerFace の分割(現 1 関数 → 3 関数)

- `LLDrawInfoSnapshot captureRegisterSnapshot(group, facep, type)` = 現 registerFace の :6859-7178 の **read 部**を移植(main・staging で呼ぶ)。hidden 判定・nullvb/hidden の skip も snapshot の bool として記録(skip 面は「登録しない」を意味する snapshot フラグ)。
- `void buildDrawInfoFromSnapshot(const LLDrawInfoSnapshot& s, const VbRange& vb, drawmap_local_t& out)` = 現 :7026-7189 の **batch+alloc 部**を snapshot 参照に書換(off-main・job-local out へ)。batch 比較(:7028-7050)は snapshot 値と out の既存 entry の値で行う(live 参照ゼロ)。
- `void foldBuiltDrawInfo(group, staged, built)` = **fold(main)**。staleness 検証 → face write → mDrawMap へ built を install → patchGroup。

staging 側 hook: 現行 `sGeoCurrentApply=apply`(:8457)+ registerFace(pass 記録)を、**captureRegisterSnapshot を呼び snapshot を apply record(LLGeoFaceApply.mSnaps)へ push** に置換。mPasses と mSnaps は index 対応。

## 5. データ構造変更

- `struct LLDrawInfoSnapshot { … §3 の全 field … }`(llvovolume.cpp・LLGeoFaceApply 近傍)。
- `LLGeoFaceApply` に `std::vector<LLDrawInfoSnapshot> mSnaps;`(mPasses と parallel)。
- job-local build 出力 = `LLGeoRebuildJob` に `std::vector<LLPointer<LLDrawInfo>> mBuilt;`(pass ごとに built・fold で mDrawMap へ)。または pass→list の map。

## 6. race-free 証明(命題)

- build 相が read するのは `LLDrawInfoSnapshot`(immutable・job 所有)+ staged VB index(immutable)+ job-local out のみ。
- build 相が write するのは job-local out のみ。
- ⟹ build 相は shared state に一切触れない = **他 thread と競合し得ない**(検出器 C_MEGA/DRAWDATA_RACE は allocator 用・本相は allocator も触らない)。
- capture/fold は main-only(staging / Update-Geom fold)= 従来 apply と同じ thread。
- **正のオラクル**(gate)= ①GEOAB(L3 A/B・幾何 byte)沈黙 ②視覚同一 ③main self pub_ms 減(直接計測)④kill-switch inline との A/B 完全一致。

## 7. 罠(申告・実装時に潰す)

1. **batch-against-preserved**: 現行は new face を draw_vec 末尾(preserved 含む)と merge。job-local build は new 同士のみ merge → fold で preserved と非 merge(draw 数微増・視覚同一)。要 keep/measure。**縮小申告**。
2. **staleness 検証の置き場**: face の getFace/getGeomCount 照合(:6024-6047)は live read = **fold(main)で実施**(build 前でなく)。stale なら該当 face を drop。build は snapshot で無条件構築、fold で捨てる(wasteful だが安全)。
3. **face write(setVertexBuffer/setDrawInfo)**: fold(main)でのみ。off-main で書かない。
4. **material/gltf ポインタの寿命**: snapshot が raw ptr を持つ間(1 frame)に main が material を差し替え可 → snapshot は 1-frame ゆえ古い ptr。LLDrawInfo も raw mMaterial を持つ既存挙動と同じ寿命前提(object が保持・変更で再 dirty)。LLPointer 化で安全側に倒すか要検討。**解釈申告**。
5. **hidden(selection/RLV)**: staging で判定 → 1-frame 遅延で hide が反映。編集 hide の 1-frame 遅れ = 不可視。
6. **mSpecularMap/mNormalMap の getTESpecularMap/NormalMap**: vobj 経由 texture 取得 = staging で読む。
7. **static 経路(mInline / worker 無効時)**: self でも inline 退化時は現行 apply(main)を通す(kill-switch AYASTORM_MT_THREADS=1 で全 inline)。

## 8. gate(命題様式)

- main の self pub_ms が有意に減(直接計測・捏造不能)+ 視覚同一 + validation 0 + 装置全層沈黙 + **GEOAB verdict クリーン(必須)** + C_*_RACE 0 + kill-switch inline A/B 一致。
- 未証明項: batch 数の微増(罠1)は perf 微差・視覚同一で受容可否を AYA。build 相が pub_ms の movable 主項かは fold 後の pub_ms 実測で確定(read=staging に残る分と build=off-main に出る分の内訳)。

## 9. 実装順(直列・各 compile 検証)

**安全化の原則(2 段階)**: 350 行の critical batching 関数を「分割 + 移設」を一気にやらない。**まず in-place で構造分離(全 main・挙動不変)→ 検証 → 移設**。

- **Step 1** = `LLDrawInfoSnapshot` struct + `LLGeoFaceApply.mSnaps` + `LLGeoRebuildJob.mBuilt` 定義。
- **Step 2(挙動不変の in-place 分離・main のまま)**: registerFace を `captureRegisterSnapshot`(read 部 → snapshot)+ `buildDrawInfoFromSnapshot`(snapshot + VB index → LLDrawInfo・batch)に **その場で** 割る。呼び出しは registerFace 内で capture→build を連続実行(= 現行と 1:1 挙動)。**全 main・thread 移動なし**。gate = 現行 apply gate 一致(視覚同一・GEOAB 沈黙)= 分離の正しさを移設前に確定。
- **Step 3(capture を staging へ移設)**: staging hook(:8457)で captureRegisterSnapshot を呼び mSnaps 充填。apply 側は build のみ(capture は staging 済)。まだ build は main(apply)。gate = 挙動不変(capture の phase だけ前倒し・1-frame ズレは staging→apply 同 frame ゆえこの段では無し)。
- **Step 4(build を off-main へ)**: avatar-domain worker が buildDrawInfoFromSnapshot を job-local list へ(self のみ・純関数)。
- **Step 5(fold)**: `foldBuiltDrawInfo`(main)= staleness + face write + mDrawMap install + patchGroup。drainAvatarPublished を fold へ。
- **Step 6** = self routing(既 B.2a routing を build 相へ繋ぎ替え)。
- **Step 7** = kill-switch・GEOAB 配線確認 → gate。

**各 Step で compile + 可能なら AYA gate**。Step 2/3 は挙動不変ゆえ「分離が正しい」を移設前に証明できる(移設後に幾何が壊れたら分離バグか移設バグか切り分け可能)。

## 10. 申告(縮小・省略・解釈)

- batch-against-preserved の非 merge(罠1)= 意図的縮小(draw 数微増・視覚同一)。
- material raw ptr 寿命(罠4)= 既存 LLDrawInfo と同前提に倒す解釈(LLPointer 化は保守判断で後日可)。
- build 相が pub_ms の主項である確証は fold 後実測で確定(readiness）= 本設計は「apply を race-free に off-main 化する構造」を確定するもので、main 空き量の証明は gate 実測。
- PASS は宣言しない。本 doc は設計地図・動作証明は §8 gate の実測。
