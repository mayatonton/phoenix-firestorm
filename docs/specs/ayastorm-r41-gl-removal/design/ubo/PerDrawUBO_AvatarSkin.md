# PerDrawUBO_AvatarSkin — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、shader 既 UBO 宣言済、host C++ register/write/flush 経路未着工)

**本実装化に必要な作業**: per-draw cadence で `LLRenderPass::uploadMatrixPalette` 3 site の `uniformMatrix3x4fv(AVATAR_MATRIX, ...)` 呼出を ring buffer (= `sDrawUboRingBufferMgr`、Phase 1.B PC-6β 配線済) 経由 UBO write に redirect + dynamic offset bind 経路で `vkCmdBindDescriptorSets` set=2 binding=0 に bind

---

## §1. UBO identity

- **block_name**: `PerDrawUBO_AvatarSkin`
- **block_hash**: `0x667c5023u` (= FNV-1a("PerDrawUBO_AvatarSkin"))
- **block_size**: 768 B (= std140 720 B、device-padded 768 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perdrawubo_avatarskin.inl:12-14
struct PerDrawUBO_AvatarSkinLayout {
    static constexpr std::uint32_t matrixPalette_OFFSET = 0u;  // size=720 align=16 stride=16
};
inline constexpr std::uint32_t PerDrawUBO_AvatarSkin_SIZE = 768u; // std140=720, device-padded=768
```

```glsl
// indra/newview/app_settings/shaders/class1/avatar/avatarSkinV.glsl:45-47
layout(set=2, binding=0, std140) uniform PerDrawUBO_AvatarSkin {
    vec4 matrixPalette[45];
};
```

= Blueprint (= `aya_r41_blueprints/set2/per_draw_ubo_avatar_skin.glsl:10-13`) 一致。**注: 45 個 vec4 = 720 B (std140)**、avatar bone count 上限 45 (= LL_CHARACTER_MAX_ANIMATED_JOINTS 想定、verify 要)

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (= 推定、`llvkloader.cpp:861` literal `V3A_DRAW_SET_BINDINGS = 4; // set=2: ... (UBO_DYNAMIC)` + `llvkloader.cpp:867` `V3A_DRAW_POOL_MAX_SETS = 1; // ring buffer + dynamic offset rotation で 1 set 固定`)
- **pipeline layout**: `sAYAStandardLayout` 5-set V3a layout 内 set=2 (= `sDrawUboLayoutV3a`、`llvkloader.cpp:873`)
- **set=2 binding=0 共有 UBO** (= blueprint header literal「set=2 binding=0 を共有する 6 UBO の 1 つ (= per_draw_ubo_clip_plane.glsl header 参照)」):
  - PerDrawUBO_AvatarSkin (本 UBO)
  - PerDrawUBO_AvatarVelocity
  - PerDrawUBO_ClipPlane
  - PerDrawUBO_LightParams
  - PerDrawUBO_ObjectSkin
  - PerDrawUBO_SkinnedVelocity
  - (PerDrawUBO_MultiLight は binding=1 = 別 binding)
  = **6 UBO 同 set=2 binding=0**、program 識別で名前 dispatch (= host C++ 側 set=2 内 binding=0 ring buffer 経路 1 つを program 毎に異なる UBO レイアウトで write)
- **source**: `ubo_metadata.inl:64` `{ "PerDrawUBO_AvatarSkin", 0x667c5023u, 768u, 2u, 0u, 0u, 2u, 1u }` + `class1/avatar/avatarSkinV.glsl:45` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 2
- **意味**: **per-draw** (= `llglslshader.cpp:96` literal: `constexpr U32 kCadencePerDraw = 2u; // PC-7ε で ring buffer 経路本格化`)
- **意味詳細**: avatar mesh 1 draw call 毎に matrix palette を upload、ring buffer (= `sDrawUboRingBufferMgr`) で dynamic offset rotation、同 frame 内で多数 avatar/mesh の matrix palette を順次 upload
- **source**: ubo_metadata.inl + llglslshader.cpp:96 literal

---

## §4. 物理 owner

- **owner**: `LLVOAvatar::MatrixPaletteCache::mGLMp` (= `lldrawpool.cpp:704` `(GLfloat*)&(mpc.mGLMp[0])` literal、`std::vector<F32>` 720 B 相当)
- **lifetime**: per-draw / per-(avatar, mesh) (= `avatar == lastAvatar && skinInfo->mHash == lastMeshId` で同 hash skip、別 avatar / 別 skin で再 upload)
- **用途**: avatar mesh skinning (= avatar bone matrix palette、`avatarSkinV.glsl:59-61` で `mix(matrixPalette[i+0], matrixPalette[i+1], x)` 等で bone transform 計算)
- **NB**: `LL_CHARACTER_MAX_ANIMATED_JOINTS` 想定 = 45 bone × 3 mat3x4 row (= 4 vec4 で 1 mat3x4 表現に近い、ただし shader 側は vec4[45] 解釈で `i+0/i+15/i+30` index = 15 bone × 3 row layout、verify 要 = avatarSkinV.glsl:59-61 で 3 row 構造前提)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_avatar_skin.glsl`
- **実 shader use site** = **`class1/avatar/avatarSkinV.glsl:45` 単独** (= blueprint header literal「Source: literal extract from class1/avatar/avatarSkinV.glsl:45 ifdef LL_VULKAN_GLSL block (single site)」)
- consume 内容 (= grep 確認済):
  - `avatarSkinV.glsl:59` `ret[0] = mix(matrixPalette[i+0], matrixPalette[i+1], x);`
  - `avatarSkinV.glsl:60` `ret[1] = mix(matrixPalette[i+15], matrixPalette[i+16], x);`
  - `avatarSkinV.glsl:61` `ret[2] = mix(matrixPalette[i+30], matrixPalette[i+31], x);`
  - `avatarSkinV.glsl:68-69` `vec4 dummy1 = matrixPalette[0]; vec4 dummy2 = matrixPalette[44];`
- **同 member 名 別 shader**:
  - `class1/avatar/objectSkinV.glsl:44/49` (= `mat3x4 matrixPalette[MAX_JOINTS_PER_MESH_OBJECT]`、別 UBO PerDrawUBO_ObjectSkin (10752B) で wrap)
  - `class1/deferred/fsObjectIDV.glsl:6` comment (= `objectSkinV.glsl` の path 参照、UBO 自体は別)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線
- **既存 OpenGL 経路 setter** (= grep 結果、3 site で同一 call):
  - `indra/newview/lldrawpool.cpp:701-704`:
    ```cpp
    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX,
        count, false, (GLfloat*)&(mpc.mGLMp[0]));
    ```
  - `lldrawpool.cpp:737-740` (= 同 call、`uploadMatrixPalette` overload)
  - `lldrawpool.cpp:775-778` (= 同 call、`uploadMatrixPalette` 別 overload with lastAvatarShader)
- **call site**: `LLRenderPass::uploadMatrixPalette` (= `lldrawpool.cpp:711, 748` の 2 overload + line 690 周辺の 3rd overload)
- **dedup logic** (= 既存 OpenGL 経路):
  - same avatar + same mHash → skip (= `if (avatar == lastAvatar && skinInfo->mHash == lastMeshId)` literal `lldrawpool.cpp:723`)
- **reserved uniform 登録**: `indra/llrender/llshadermgr.cpp:1734` `mReservedUniforms.push_back("matrixPalette");`
- **enum entry**: `indra/llrender/llshadermgr.h:246` `AVATAR_MATRIX, // "matrixPalette"`

---

## §7. 現状通電状態

- **状態**: **untouched** (= ring buffer infrastructure `sDrawUboRingBufferMgr` は Phase 1.B PC-6β で配線済だが、本 UBO の register/write/flush 経路はまだ未通電)
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済
- **blueprint 起案済**: `aya_r41_blueprints/set2/per_draw_ubo_avatar_skin.glsl`
- **shader 宣言済**: `class1/avatar/avatarSkinV.glsl:45` `#ifdef LL_VULKAN_GLSL` block
- **ring buffer infrastructure**: `sDrawUboRingBufferMgr` (= `llvkloader.cpp:440`、PC-6β 配線済) + per-thread `mDrawUboRingBuffer` (= `llvkloader.cpp:753`、PC-N-15a 配線済)
- **register/write/flush 経路**: 未配線 (= 本 UBO 専用 register 経路、PerDraw write 経路は本 UBO で初通電候補)

---

## §8. 本実装化に必要な作業

1. **register 経路**: `mapUniforms()` で avatar skin program (= avatarSkinV を含む program) に対し per-draw 用 register 経路新設 (= `registerProgramUbo` ではなく PerDraw 経路、`registerPerDrawUbo` 等の新 method 候補、設計要)
2. **write 経路**: `lldrawpool.cpp:701/737/775` 3 site の `uniformMatrix3x4fv(AVATAR_MATRIX, ...)` を ring buffer 経由 UBO write に redirect:
   - `sDrawUboRingBufferMgr->acquireSlot(768B)` で slot 取得 → `mGLMp` の 720 B を memcpy → dynamic offset 取得
   - 同 frame 内 1 buffer に多数 avatar matrix palette を順次格納
3. **flush/bind 経路**: cmdbuf bind 経路で `vkCmdBindDescriptorSets` set=2 binding=0 に dynamic offset 引数で bind (= `bindV3aRigged` 経路想定、`llvkloader.cpp:2192/2233` 既存)
4. **shader 接続**: shader 追加改変なし (= 既に UBO 宣言済)
5. **set=2 binding=0 共有 6 UBO の program 識別**:
   - avatar mesh program → PerDrawUBO_AvatarSkin (本 UBO) layout
   - object mesh program → PerDrawUBO_ObjectSkin (10752B) layout
   - clip plane 必要 program → PerDrawUBO_ClipPlane layout
   - 等で同 set=2 binding=0 を program 毎に別 UBO で wrap = host C++ 側で program 識別後 ring buffer slot に正しい layout で write
6. **dedup logic 維持**: 既存 `(avatar, mHash)` dedup logic を ring buffer write でも維持 (= 同 hash 再 upload 回避で frame 当 bandwidth 削減)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=2 binding=0、UBO_DYNAMIC) |
| OS-3 | std140 padding 厳守 | ✅ 720B → 768B padded (vec4[45] stride=16) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 (= ring buffer slot alignment) |
| OS-5 | shader 改変ゼロ | ✅ 既に UBO 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**特記 risk**:
- **set=2 binding=0 共有 6 UBO の dispatch logic 確立** = 同 set=2 binding=0 を program 毎に別 layout の UBO で wrap = host C++ 側で program 識別必須、誤った layout で write すると skinning 崩壊
- **per-draw bandwidth**: 1 avatar = 768 B (= padded)、100 avatars × 768 B = 75 KB/frame = OK (= ring buffer 数 MB)
- **同 thread 内 ring buffer 競合**: PC-N-15a で per-thread `mDrawUboRingBuffer` 化済 = thread 競合解消済
- **first-frame fallback (= velocity の `lastMatrixPalette` 問題)**: AvatarSkin 自体は dedup あり、`AvatarVelocity` 側で fallback logic (= `lldrawpool.cpp:1015` `mpc.mLastGLMp.empty() ? mpc.mGLMp : mpc.mLastGLMp`)

---

## §10. 不明事項

1. **PerDraw register 経路 method 名** = `registerPerDrawUbo` 等の新 method を新設するか、program ベース既経路を流用するか設計要
2. **set=2 binding=0 共有 6 UBO の program 識別 logic** = program ベース dispatch の実装方式
3. **ring buffer slot acquisition flow** = `sDrawUboRingBufferMgr` API の正確な call sequence (= acquireSlot → memcpy → dynamic offset 取得 → 次 frame 再利用)
4. **dedup logic と ring buffer 整合性** = ring buffer 経由でも `(avatar, mHash)` dedup で write skip するか、それとも常に新 slot 取得か
5. **bone count 45 の意味** = `LL_CHARACTER_MAX_ANIMATED_JOINTS` 等の host 側定数との整合
6. **`uniformMatrix3x4fv` の data layout** = 3 row × 4 col の mat3x4 を vec4[45] にどう pack するか (= shader 側 `matrixPalette[i+0/i+15/i+30]` index pattern から逆算要)
7. **`bindV3aRigged` 経路の通電状態** = PC-N-15a 配線後の bind 経路実通電確認

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- PerDrawUBO_AvatarSkin (本 UBO、binding=0)
- PerDrawUBO_AvatarVelocity (binding=0、**同 binding 共有**)
- PerDrawUBO_ClipPlane (binding=0、**同 binding 共有**)
- PerDrawUBO_LightParams (binding=0、**同 binding 共有**)
- PerDrawUBO_MultiLight (binding=1、別 binding)
- PerDrawUBO_ObjectSkin (binding=0、**同 binding 共有**、size=10752B)
- PerDrawUBO_SkinnedVelocity (binding=0、**同 binding 共有**、size=5376B)
- + PerProgramUBO_* 30 個 (= cadence_tag=1 だが set=2 配置、verify 要)

### §11.2 同 cadence cluster UBO (= cadence_tag=2 PerDraw)

- 7 件 PerDraw cluster (= INDEX.md §1):
  - PerDrawUBO_AvatarSkin / AvatarVelocity / ClipPlane / LightParams / MultiLight / ObjectSkin / SkinnedVelocity
- 同 cluster で ring buffer 経路共有

### §11.3 同 shader consume UBO (= `class1/avatar/avatarSkinV.glsl` で同時 consume)

- 不明 / verify 要 (= avatarSkinV.glsl 全文の UBO 宣言群確認要、FrameViewProj / FrameLights 等同時 consume 候補)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerDrawUBO_AvatarVelocity** (set=2 binding=0、size=768B) = `mGLMp` / `mLastGLMp` 同 LLVOAvatar 由来、本 UBO の prev-frame 版 (= `matrixPalette` vs `lastMatrixPalette`)
- **PerDrawUBO_ObjectSkin** (set=2 binding=0、size=10752B) = avatar mesh と attachment object mesh の対称関係、別 size の mat3x4 配列

### §11.5 dirty 連動 UBO

- avatar frame 切替時 (= bone pose 更新) → 本 UBO (matrixPalette) + PerDrawUBO_AvatarVelocity (lastMatrixPalette) 同時 dirty
- attachment 描画時 → PerDrawUBO_ObjectSkin (10752B) 同時 dirty 候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout、set=2 は UBO_DYNAMIC

### §11.7 bind 順序関係

- avatar draw call 毎に set=2 binding=0 を本 UBO の dynamic offset で rebind (= per-draw cadence 標準)
- 同 set=2 内の他 binding=0 共有 UBO は program 切替時に layout 切替 (= ring buffer slot は同 buffer、layout のみ program で識別)
