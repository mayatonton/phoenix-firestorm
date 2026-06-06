# PerDrawUBO_AvatarVelocity — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、shader 既 UBO 宣言済、host C++ register/write/flush 経路未着工)

**本実装化に必要な作業**: per-draw cadence で `LLRenderPass::uploadMatrixPaletteLastFrame` (= `lldrawpool.cpp:1021` の `uniformMatrix3x4fv(AVATAR_LAST_MATRIX, ...)` 呼出) を ring buffer (= `sDrawUboRingBufferMgr`) 経由 UBO write に redirect + dynamic offset bind 経路で `vkCmdBindDescriptorSets` set=2 binding=0 に bind + first-frame fallback logic (= `mLastGLMp.empty() ? mGLMp : mLastGLMp`) 維持

---

## §1. UBO identity

- **block_name**: `PerDrawUBO_AvatarVelocity`
- **block_hash**: `0xfa009835u` (= FNV-1a("PerDrawUBO_AvatarVelocity"))
- **block_size**: 768 B (= std140 720 B、device-padded 768 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perdrawubo_avatarvelocity.inl:12-14
struct PerDrawUBO_AvatarVelocityLayout {
    static constexpr std::uint32_t lastMatrixPalette_OFFSET = 0u;  // size=720 align=16 stride=16
};
inline constexpr std::uint32_t PerDrawUBO_AvatarVelocity_SIZE = 768u; // std140=720, device-padded=768
```

```glsl
// indra/newview/app_settings/shaders/class1/deferred/avatarVelocityV.glsl:53-55
layout(set=2, binding=0, std140) uniform PerDrawUBO_AvatarVelocity {
    vec4 lastMatrixPalette[45];
};
```

= Blueprint (= `aya_r41_blueprints/set2/per_draw_ubo_avatar_velocity.glsl:10-13`) 一致。本 UBO は **PerDrawUBO_AvatarSkin の prev-frame 版** = 同 size / 同 layout / 同 bone count、member 名のみ `lastMatrixPalette`。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (= 推定、`llvkloader.cpp:861/867` literal、AvatarSkin と同じ)
- **pipeline layout**: `sAYAStandardLayout` 5-set V3a layout 内 set=2 (= `sDrawUboLayoutV3a`)
- **set=2 binding=0 共有 6 UBO**: PerDrawUBO_AvatarSkin / **AvatarVelocity (本 UBO)** / ClipPlane / LightParams / ObjectSkin / SkinnedVelocity (= blueprint header literal「set=2 binding=0 を共有する 6 UBO の 1 つ」)
- **source**: `ubo_metadata.inl:65` `{ "PerDrawUBO_AvatarVelocity", 0xfa009835u, 768u, 2u, 0u, 0u, 2u, 1u }` + `class1/deferred/avatarVelocityV.glsl:53` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 2
- **意味**: **per-draw** (= `llglslshader.cpp:96` literal)
- **意味詳細**: avatar mesh velocity pass (= motion blur source velocity 計算) で前 frame の matrix palette を upload、curr_pose - last_pose で velocity 計算
- **source**: ubo_metadata.inl + llglslshader.cpp:96 literal

---

## §4. 物理 owner

- **owner**: `LLVOAvatar::MatrixPaletteCache::mLastGLMp` (= 前 frame の bone matrix snapshot、`lldrawpool.cpp:1015` literal `const std::vector<F32>& src = mpc.mLastGLMp.empty() ? mpc.mGLMp : mpc.mLastGLMp;`)
- **lifetime**: per-draw / per-(avatar, mesh)、frame 跨ぎ (= curr frame の matrix → next frame の `mLastGLMp`)
- **用途**: avatar mesh velocity (= motion blur source) shader で curr_pose - last_pose による screen-space velocity 計算 (= `avatarVelocityV.glsl:101-103` `mix(lastMatrixPalette[i+0], lastMatrixPalette[i+1], x)` で interpolate)
- **first-frame fallback** (= `lldrawpool.cpp:1009-1015` literal comment + code):
  ```cpp
  // First-frame fallback: when mLastGLMp hasn't been populated yet (new hash
  // or first visible frame), uploading nothing would leave lastMatrixPalette[]
  // holding bones from whichever rig drew previously — those get read as the
  // "last frame" of this rig and produce lightning-streak velocity. Upload
  // mGLMp instead so last_pose == curr_pose → velocity = 0, the correct
  // "no motion captured yet" answer.
  const std::vector<F32>& src = mpc.mLastGLMp.empty() ? mpc.mGLMp : mpc.mLastGLMp;
  ```

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_avatar_velocity.glsl`
- **実 shader use site** = **`class1/deferred/avatarVelocityV.glsl:53` 単独** (= blueprint header literal「single site」)
- consume 内容 (= grep 確認済):
  - `avatarVelocityV.glsl:101` `ret[0] = mix(lastMatrixPalette[i+0], lastMatrixPalette[i+1], x);`
  - `avatarVelocityV.glsl:102` `ret[1] = mix(lastMatrixPalette[i+15], lastMatrixPalette[i+16], x);`
  - `avatarVelocityV.glsl:103` `ret[2] = mix(lastMatrixPalette[i+30], lastMatrixPalette[i+31], x);`
- **同 member 名 別 shader** (= grep 結果):
  - `class1/deferred/skinnedVelocityV.glsl:82/86` (= `mat3x4 lastMatrixPalette_skinned_velocity[MAX_JOINTS_PER_MESH_OBJECT]` + bare uniform、別 UBO PerDrawUBO_SkinnedVelocity 候補 = size=5376B = 384B × 14 mat3x4 想定だがメタデータと整合確認要)
  - `class1/deferred/skinnedVelocityAlphaV.glsl:111/115` (= 同上)
  - `class1/avatar/objectSkinV.glsl:45/50/112-120` (= attachment object 用 `lastMatrixPalette[MAX_JOINTS_PER_MESH_OBJECT]`、別 UBO PerDrawUBO_ObjectSkin 候補)
  - `class1/deferred/motionBlurF.glsl:82` comment「buffer fully — accepts the upstream avatar lastMatrixPalette」 + `motionBlurF.glsl:145` comment「avatar lastMatrixPalette uninitialized → NaN」

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線
- **既存 OpenGL 経路 setter** (= grep 結果):
  - `indra/newview/lldrawpool.cpp:1021-1024`:
    ```cpp
    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLShaderMgr::AVATAR_LAST_MATRIX,
        count, false, (GLfloat*)&(src[0]));
    ```
- **call site context**: `lldrawpool.cpp:1000` comment「Known limitation: the avatar shader expects lastMatrixPalette[45] for」 + `lldrawpool.cpp:1081` comment「直接の原因: avatarVelocityV.glsl は lastMatrixPalette[45] を読むが、」
- **reserved uniform 登録**: `indra/llrender/llshadermgr.cpp:1878` `mReservedUniforms.push_back("lastMatrixPalette");`
- **enum entry**: `indra/llrender/llshadermgr.h:397` `AVATAR_LAST_MATRIX, // "lastMatrixPalette"` (= `llshadermgr.h:386` literal「Imported from BlackDragon Viewer 995a1354d8 (with AVATAR_LAST_MATRIX」)

---

## §7. 現状通電状態

- **状態**: **untouched**
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済
- **blueprint 起案済**: `aya_r41_blueprints/set2/per_draw_ubo_avatar_velocity.glsl`
- **shader 宣言済**: `class1/deferred/avatarVelocityV.glsl:53` `#ifdef LL_VULKAN_GLSL` block
- **ring buffer infrastructure**: `sDrawUboRingBufferMgr` (= Phase 1.B PC-6β 配線済) + per-thread `mDrawUboRingBuffer` (= PC-N-15a 配線済)
- **register/write/flush 経路**: 未配線 (= 本 UBO 専用 register 経路、PerDrawUBO_AvatarSkin と並列開発候補)

---

## §8. 本実装化に必要な作業

1. **register 経路**: `mapUniforms()` で avatar velocity program (= avatarVelocityV を含む program) に対し per-draw 用 register 経路新設
2. **write 経路**: `lldrawpool.cpp:1021` の `uniformMatrix3x4fv(AVATAR_LAST_MATRIX, ...)` を ring buffer 経由 UBO write に redirect:
   - `sDrawUboRingBufferMgr->acquireSlot(768B)` で slot 取得 → `src` (= 720 B、`mLastGLMp` or `mGLMp` fallback) を memcpy → dynamic offset 取得
3. **flush/bind 経路**: cmdbuf bind 経路で `vkCmdBindDescriptorSets` set=2 binding=0 に dynamic offset 引数で bind (= `bindV3aRigged` 経路想定)
4. **shader 接続**: shader 追加改変なし
5. **first-frame fallback logic 維持**: `lldrawpool.cpp:1015` の `mLastGLMp.empty() ? mGLMp : mLastGLMp` 分岐を ring buffer write 側でも維持 (= lightning-streak velocity 回避)
6. **set=2 binding=0 共有 6 UBO の program 識別**: avatar velocity program → PerDrawUBO_AvatarVelocity (本 UBO) layout

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 720B → 768B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既に UBO 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**特記 risk**:
- **first-frame fallback の必須遵守**: 既存 OpenGL 経路の fallback (= `mLastGLMp.empty() ? mGLMp : mLastGLMp`) は lightning-streak velocity 回避の本質的安全弁、ring buffer 経路でも同等 logic 維持必須 = 違反すると motion blur で前回 rig の bone が漏れて視覚 bug
- **set=2 binding=0 共有 6 UBO の dispatch logic 確立**: AvatarSkin と並列、program 識別で正しい layout で write
- **`motionBlurF.glsl:145` comment 警告**「avatar lastMatrixPalette uninitialized → NaN, per-frame matrix」= upload skip すると undefined behavior、必ず upload 必須
- **per-draw bandwidth**: AvatarSkin と並列、合計 1 avatar あたり 1.5 KB (= 768 × 2)

---

## §10. 不明事項

1. **PerDraw register 経路** = AvatarSkin と同じく新 method 設計要
2. **set=2 binding=0 共有 6 UBO の program 識別 logic** = AvatarSkin と同じ
3. **ring buffer slot acquisition flow** = AvatarSkin と同じ
4. **dedup logic と ring buffer 整合性** = `uploadMatrixPaletteLastFrame` の dedup 条件 (= AvatarSkin の `(avatar, mHash)` dedup と同じか確認要)
5. **first-frame fallback の ring buffer 経路実装** = 既存 fallback logic を ring buffer write 側でどう移植するか
6. **`bindV3aRigged` 経路での velocity pass bind timing** = AvatarSkin と velocity の bind 順序 (= 1 draw で両方 bind か、別 draw か)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- PerDrawUBO_AvatarSkin (binding=0、**同 binding 共有**、本 UBO の curr-frame 対称)
- **PerDrawUBO_AvatarVelocity (本 UBO、binding=0)**
- PerDrawUBO_ClipPlane / LightParams / ObjectSkin / SkinnedVelocity (binding=0)
- PerDrawUBO_MultiLight (binding=1)
- + PerProgramUBO_* 30 個

### §11.2 同 cadence cluster UBO (= cadence_tag=2 PerDraw)

- 7 件 PerDraw cluster の 1 UBO

### §11.3 同 shader consume UBO (= `class1/deferred/avatarVelocityV.glsl` で同時 consume)

- 不明 / verify 要 (= avatarVelocityV.glsl 全文の UBO 宣言群確認要、FrameViewProj / motion blur 関連 UBO 同時 consume 候補)
- 関連 shader: `motionBlurF.glsl` (= 別 program、fragment shader 側で velocity texture を read)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerDrawUBO_AvatarSkin** (set=2 binding=0、size=768B) = `mGLMp` (curr frame) vs 本 UBO の `mLastGLMp` (prev frame) = **curr/prev 対称 UBO**
- **PerDrawUBO_SkinnedVelocity** (set=2 binding=0、size=5376B) = attachment object 用 velocity (= `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` で使用)
- **PerDrawUBO_ObjectSkin** (set=2 binding=0、size=10752B) = `lastMatrixPalette` を `objectSkinV.glsl` で同時参照

### §11.5 dirty 連動 UBO

- avatar frame 切替時 (= bone pose 更新) → **PerDrawUBO_AvatarSkin** + 本 UBO 同時 dirty (= curr/prev 対称ゆえ必須)
- attachment 描画時 → **PerDrawUBO_SkinnedVelocity** / **PerDrawUBO_ObjectSkin** 同時 dirty 候補
- motion blur enable 時 → 本 UBO + AvatarSkin + SkinnedVelocity の velocity 系全体 dirty 必須

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout、set=2 は UBO_DYNAMIC

### §11.7 bind 順序関係

- avatar velocity draw call 毎に set=2 binding=0 を本 UBO の dynamic offset で rebind
- AvatarSkin と velocity の bind 順序 = 不明 / verify 要 (= 同 draw call で 2 UBO 同時 bind は不可、別 draw call (= main pass + velocity pass) と推定)

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.8 同期)

**Layer**: L4-8 sub-cluster (a) (= avatar curr/prev pair 2 UBO、lightning-streak fallback 必須)
**status**: **起案済** (= 2026-06-06 C-6-c、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.8` (= single source of truth)
**要点**: 1 member (lastMatrixPalette[45] vec4)、bone animation frame swap trigger で AvatarSkin (curr) と同時 dirty、setter `lldrawpool.cpp:1021` AVATAR_LAST_MATRIX `uniformMatrix3x4fv` literal 確認、**first-frame fallback `mLastGLMp.empty() ? mGLMp : mLastGLMp` 必須遵守** (= `lldrawpool.cpp:1015` literal、lightning-streak velocity 回避担保、`motionBlurF.glsl:145` 警告「uninitialized → NaN」)、BlackDragon import (= `llshadermgr.h:386` literal Imported from BlackDragon Viewer 995a1354d8)、cadence PerDraw 維持、工数 group 全体 L 内
**関連**: L0-1 dispatch (= set=2 binding=0 共有 6 UBO) / §3.5.8 sub-cluster (a) AvatarSkin (= curr/prev pair 対称) / sub-cluster (b) SkinnedVelocity (= object 版 lastMatrixPalette 抽出) / `motionBlurF.glsl` (= 後段 motion blur pass)


---

## §13. Phase 2.α 案 X 確定 record (= blueprint dir 位置付け + 二重 source 同期 protocol)

### §13.1 blueprint dir の位置付け = codegen 入力 source of truth

- **blueprint file** (= `aya_r41_blueprints/<set>/<ubo_lower>.glsl`) は本 UBO の **codegen 入力 source of truth** (= 案 X 確定 2026-06-06)。`indra/cmake/AyaUboCodegen.cmake` の `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 経由で `scripts/ubo_codegen/main.py` の入力に渡され、`ubo_metadata.inl` + `ubo_layout_<ubo>.inl` を生成する。
- **AYAstorm shader runtime compile target は別 GLSL 系統** (= `class*/` + `cinematic_bd/` 配下の実 shader use site) で並列 build process (= design/04-codegen-ubo.md §2.2 literal「別 GLSL 並列 build process」)。
- 二系統は二重 source として共存し、**`scripts/ubo_codegen/main.py` の二重 source 同期 protocol で整合 verify** される (= §13.2)。
- 案 X 確定 source of truth = `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9
- blueprint dir 内 README = `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= phase B commit `f95182ded5`、位置付け literal source)

### §13.2 二重 source 同期 protocol (= main.py で formal化)

- **`_verify_block_match`** (= α-2 commit `b66ec99f72`) = 同名 UBO 複数 file (= blueprint + actual の cross-source pair、または cinematic_bd 上書き path) の set/binding + subset/cadence + member 全件 layout 一致を構造的 verify。不一致時 `CodegenError` で abort。
- **`_verify_blueprint_actual_consistency`** + **`--verify-target-paths`** option (= phase F commit `868bc38cc9`) = blueprint と actual の二重 source 整合 verify を formal化、`--verify-target-paths` で blueprint と actual を区別して対称的 cross-verify。
- 本 UBO の場合 = blueprint file (= §5 / §1 で記載) と実 shader use site (= §5 で記載) が **両 path で同一 layout (set/binding/member)** を保持する protocol。改修時は両方を同期書換するか、blueprint 側のみ書換後 codegen 再生成 + actual の `#ifdef LL_VULKAN_GLSL` block を手動同期する。
- sub-session 5 step 2-batch-0-a 7 UBO の同期書換 record = phase E commit `09ee5e8a8e` (= actual class*/ + cinematic_bd/ 14 file の新 set/binding を blueprint dir 内 7 UBO 7 file に同期反映、案 X 確定後の整合修復)

### §13.3 cross-ref

- 設計 doc = `design/04-codegen-ubo.md` §2.2 (= 別 GLSL 並列 build process) / §4.4 (= 同名 UBO 複数 GLSL 宣言の整合 verify)
- handoff doc = `handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9 (= 案 X 確定 source of truth、6 commit revert record + 改修方針 9 件)
- blueprint dir README = `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= phase B commit `f95182ded5`)
- 二重 source 同期 protocol formal化 = `scripts/ubo_codegen/main.py` `_verify_block_match` + `_verify_blueprint_actual_consistency`

