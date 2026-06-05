# PerProgramUBO_PostDeferredF — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_post_deferred_f.glsl` は実 shader `class1/deferred/postDeferredF.glsl:108 ifdef LL_VULKAN_GLSL block` から literal extract 済 (= postDeferredF / postDeferredHQDoFF 2 site cross-program 共有)。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_PostDeferredF`
- **block_hash**: `0x8519e0b2u` (= FNV-1a("PerProgramUBO_PostDeferredF"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 2
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_postdeferredf.inl
struct PerProgramUBO_PostDeferredFLayout {
    static constexpr std::uint32_t res_scale_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t chroma_str_OFFSET = 4u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_PostDeferredF_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_post_deferred_f.glsl
layout(std140, set = 2, binding = 20) uniform PerProgramUBO_PostDeferredF
{
    float res_scale;
    float chroma_str;
};
```

= 全 2 member 実 data slot 確定 (= blueprint comment literal: `Source: literal extract from class1/deferred/postDeferredF.glsl:108 ifdef LL_VULKAN_GLSL block` + `(verified identical across 2 sample sites = postDeferredF / postDeferredHQDoFF)`)。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 20
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯
- **source**: `ubo_metadata.inl:84` `{ "PerProgramUBO_PostDeferredF", 0x8519e0b2u, 256u, 2u, 20u, 0u, 1u, 2u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush。post-deferred F stage で DoF resolution scale + chroma aberration strength を保持
- **source**: ubo_metadata.inl:84 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source**:
  - `res_scale` = `CameraDoFResScale` (= `pipeline.cpp:10056` `gDeferredPostProgram.uniform1f(LLShaderMgr::DOF_RES_SCALE, CameraDoFResScale)`)
  - `chroma_str` = `RenderChromaStrength` cvar (= `pipeline.cpp:10060` `const F32 dof_chroma_str = gSavedSettings.getF32("RenderChromaStrength")` + `:10061` `gDeferredPostProgram.uniform1f(LLShaderMgr::DEFERRED_CHROMA_STRENGTH, dof_chroma_str)`)
- **lifetime**: program 単位

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_post_deferred_f.glsl`
- **実 shader use site** (= 2 site cross-program 共有):
  - **`class1/deferred/postDeferredF.glsl:108 ifdef LL_VULKAN_GLSL block`** (= primary)
  - **`class1/deferred/postDeferredHQDoFF.glsl`** (= cross-program 同 layout、blueprint comment literal verified identical)
  - 既存 UBO block (postDeferredF.glsl:109-110):
    ```glsl
    float res_scale;   // offset 0,  size 4 + 12 pad
    float chroma_str;  // offset 16, size 4 + 12 pad
    ```
  - 既存 OpenGL `#else` block (postDeferredF.glsl:114-115):
    ```glsl
    uniform float res_scale;
    uniform float chroma_str;
    ```
  - 使用箇所: `postDeferredF.glsl:134` (`float mult = sc * (chroma_str * 0.2)`)
  - **GLSL ↔ blueprint padding 差分**: shader 側 comment `offset 16 + 12 pad` vs codegen `OFFSET = 4u` (packed)。**要 verify** (= GodraysF と同種類の差分)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter** (= `pipeline.cpp` 内):
  - **`pipeline.cpp:10028`** `gDeferredCoFProgram.uniform1f(LLShaderMgr::DOF_RES_SCALE, CameraDoFResScale)`
  - **`pipeline.cpp:10056`** `gDeferredPostProgram.uniform1f(LLShaderMgr::DOF_RES_SCALE, CameraDoFResScale)`
  - **`pipeline.cpp:10060-10061`** `dof_chroma_str = gSavedSettings.getF32("RenderChromaStrength")` + `gDeferredPostProgram.uniform1f(LLShaderMgr::DEFERRED_CHROMA_STRENGTH, dof_chroma_str)` (= AYAstorm r30 P4 step 4 BD chroma_str、comment literal)
  - **`pipeline.cpp:10088`** `gDeferredDoFCombineProgram.uniform1f(LLShaderMgr::DOF_RES_SCALE, CameraDoFResScale)`
- **本実装化後 setter**:
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **通電内容**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲)

---

## §8. 本実装化に必要な作業

1. **shell 通電**: blueprint ベースで host 側 buffer 配置 + descriptor set 配線
2. **実 member data 流入**:
   - `pipeline.cpp:10056` (res_scale) + `:10061` (chroma_str) の setter を UBO 化
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + cvar `RenderChromaStrength` 変更時 dirty (= cvar listener trigger)
4. **flush logic 追加**: PerProgram cadence flush (= `writeProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class1/deferred/postDeferredF.glsl:108 ifdef LL_VULKAN_GLSL block` UBO declaration 既存 = 追加 shader 改変なし
   - 既存 OpenGL `#else` block uniform 個別宣言は温存
6. **cross-program 共有整理**:
   - postDeferredF + postDeferredHQDoFF 2 program で同 UBO layout 共有、各 program で flush 必要 (= 同 layout 別 program 別 UBO instance)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=20 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**std140 padding 差分**: shader 側 comment `offset 16 + 12 pad` (= scalar 16 B aligned) vs codegen `OFFSET = 4u` (= packed float)。std140 の scalar に対する規則上、後者 (packed) が正しい (= `vec4` だけが 16 B aligned)。shader comment は documentation のみで実 SPIR-V offset は codegen 通り (= verify 要、Phase 2 cold launch validation 確認)。

**`pipeline.cpp` 内 dof_combine 含む 3 program で同 res_scale 共有**: gDeferredCoFProgram / gDeferredPostProgram / gDeferredDoFCombineProgram で同 setter、各 program 別 UBO instance で同値書込み必要。

**AYAstorm r30 P4 BD chroma_str 由来**: `RenderChromaStrength` は r30 BD 改善由来 cvar (= comment literal `<AYAstorm r30 P4 step 4>`)、upstream Firestorm に無い AYAstorm 独自。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件の最大 cluster

### §11.3 同 shader consume UBO (= class1/deferred/postDeferredF.glsl)

- **不明 / verify 要** = postDeferredF.glsl 内で他に consume される UBO (= FrameViewProj / sampler 等、grep verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerProgramUBO_PostDeferredNoDoFF** (= set=2 binding=12、同 `chroma_str` を持つ DoF 無し版、`pipeline.cpp:9552-9557` setter 経由で同 source)
- **PerProgramUBO_PostDeferredV** (= set=2 binding=7、本 F の V stage pair)
- **PerProgramUBO_CofF** (= set=2 binding=21、CoF program 内 setter `pipeline.cpp:10028` で同 DOF_RES_SCALE 経由)
- **DofCombineFParamUBO_Legacy** (= set=3 binding=24、`pipeline.cpp:10088` 同 setter 経由)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- `CameraDoFResScale` 変化時、関連 DoF UBO (= PerProgramUBO_CofF / DofCombineF) と同時 dirty 候補
- `RenderChromaStrength` 変化時、PerProgramUBO_PostDeferredNoDoFF と同時 dirty 候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **postDeferredHQDoFF 共有実態** = blueprint `verified identical across 2 sample sites = postDeferredF / postDeferredHQDoFF` literal、postDeferredHQDoFF 側 file line 番号 (= verify 要)
2. **3 program (CoF / Post / DoFCombine) における DOF_RES_SCALE 共有** = 各 program で別 UBO instance か共通 UBO 経由か (= verify 要、Phase 2 設計判断)
3. **shader `// offset 16 + 12 pad` vs codegen `OFFSET = 4u` SPIR-V offset** = どちらが SPIR-V compile 後の実 offset か (= Phase 2 cold launch validation 必須)
4. **同 shader file 内同時 consume UBO 一覧** = postDeferredF.glsl 内全 UBO declaration grep 要
5. **shell 通電 commit** = 未来作業
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要
7. **`PerProgramUBO_PostDeferredNoDoFF` (binding=12) との重複** = 同 `chroma_str` member、別 program 別 UBO で double-write 必要か、cadence 制約での共有可能性 (= verify 要)

= 上記 7 項目は本 UBO file 完成時に逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.2 同期)

**Layer**: L4-2 sub-cluster (b) (= chroma_str 2 UBO cross-write)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.2` (= single source of truth)
**要点**: chroma_str 2 UBO cross-write (= PostDeferredF + NoDoFF)、本 UBO `chroma_str` offset=4 + `res_scale` offset=0、AYAstorm r30 P4 BD 改善 (RenderChromaStrength cvar)、HAS_DOF_CHROMA permutation で別 program 別 UBO instance、res_scale は §3.5.10 group DOF data 交差 [要 verify]、postDeferredF/HQDoFF 2 program 共有、工数 L (group 全体)
**関連**: L0-1 dispatch (= postDeferredF/HQDoFF 2 program 識別) / §3.5.10 post-process group (= res_scale CameraDoFResScale 交差) / §5.4 visual regression policy
