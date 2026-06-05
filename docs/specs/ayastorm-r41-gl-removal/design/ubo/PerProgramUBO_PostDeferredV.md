# PerProgramUBO_PostDeferredV — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_post_deferred_v.glsl` は実 shader `class1/deferred/postDeferredV.glsl:46 ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_PostDeferredV`
- **block_hash**: `0x9519f9e2u` (= FNV-1a("PerProgramUBO_PostDeferredV"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 2 (= 1 active + 1 tail pad)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_postdeferredv.inl
struct PerProgramUBO_PostDeferredVLayout {
    static constexpr std::uint32_t tc_scale_OFFSET = 0u;  // size=8 align=8
    static constexpr std::uint32_t _pad_pdv0_OFFSET = 8u;  // size=8 align=8
};
inline constexpr std::uint32_t PerProgramUBO_PostDeferredV_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_post_deferred_v.glsl
layout(std140, set = 2, binding = 7) uniform PerProgramUBO_PostDeferredV
{
    vec2 tc_scale;
    vec2 _pad_pdv0;
};
```

= active member 1 個 (= `tc_scale` vec2 8 B) + tail pad vec2 8 B。blueprint comment literal: `Source: literal extract from class1/deferred/postDeferredV.glsl:46 ifdef LL_VULKAN_GLSL block (single site)`。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 7
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯
- **source**: `ubo_metadata.inl:86` `{ "PerProgramUBO_PostDeferredV", 0x9519f9e2u, 256u, 2u, 7u, 0u, 1u, 2u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush。post-deferred V stage で texcoord scale を保持
- **source**: ubo_metadata.inl:86 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source**:
  - `tc_scale` = `LLPipeline` (= `pipeline.cpp:9274` `shader->uniform2f(LLShaderMgr::FXAA_TC_SCALE, scale_x, scale_y)` 経由、ただし本 UBO は post-deferred V 用、FXAA 経路と同 source か別 source か **要 verify**)
- **lifetime**: program 単位

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_post_deferred_v.glsl`
- **実 shader use site**: **`class1/deferred/postDeferredV.glsl:46 ifdef LL_VULKAN_GLSL block`** (= single site)
  - 既存 UBO block (postDeferredV.glsl:47):
    ```glsl
    vec2 tc_scale;
    ```
  - 既存 OpenGL `#else` block (postDeferredV.glsl:52):
    ```glsl
    uniform vec2 tc_scale;
    ```
  - 使用箇所: `postDeferredV.glsl:80` (`vary_tc = (pos.xy*0.5+0.5)*tc_scale`)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter**:
  - `pipeline.cpp:9274` `shader->uniform2f(LLShaderMgr::FXAA_TC_SCALE, scale_x, scale_y)` (= FXAA program 内 setter)
  - **本 UBO 用 setter site 直接特定不可** = `tc_scale` という名の reserved uniform が `LLShaderMgr` enum に直接無し (= grep 結果から FXAA_TC_SCALE のみ確認)。post-deferred V 用 setter は **要 verify** (= 別 setter / 共有 setter / 個別呼出 のいずれか)
- **本実装化後 setter**:
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **通電内容**: なし
- **blueprint 配置 commit**: 不明

---

## §8. 本実装化に必要な作業

1. **shell 通電**: blueprint ベースで host 側 buffer 配置 + descriptor set 配線
2. **実 member data 流入**:
   - `tc_scale` setter site 特定 (= **要 verify**) → UBO 化
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + window resize 等で texcoord scale 変化時 dirty (= verify 要)
4. **flush logic 追加**: PerProgram cadence flush (= `writeProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class1/deferred/postDeferredV.glsl:46 ifdef LL_VULKAN_GLSL block` UBO declaration 既存 = 追加 shader 改変なし
   - 既存 OpenGL `#else` block uniform 個別宣言は温存
6. **tail pad 維持**: `_pad_pdv0` vec2 (8 B) は std140 padding (16 B 完成のため)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=7 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**setter site 不明 risk**: `tc_scale` の直接 setter が grep で特定できず、reserved uniform enum の `FXAA_TC_SCALE` (`LLShaderMgr`) を経由している可能性 / postDeferredV program 専用 setter が別途あるか **要 verify**。Phase 2 着手時に setter 完全特定が前提。

**uniform 名 collision 可能性**: `tc_scale` という汎用名、他 shader でも同名 uniform が存在し得る (= verify 要、grep 確認結果は postDeferredV.glsl 単独だが、shader 間で同名 uniform が別 UBO で扱われるなら UBO 化時の名前空間整理要)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件の最大 cluster

### §11.3 同 shader consume UBO (= class1/deferred/postDeferredV.glsl)

- **不明 / verify 要** = postDeferredV.glsl 内で他に consume される UBO (= FrameViewProj 等、grep verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerProgramUBO_PostDeferredF** (= set=2 binding=20、本 V stage の F stage pair の可能性、verify 要)
- **PerProgramUBO_PostDeferredNoDoFF** (= set=2 binding=12、別 DoF mode F stage の可能性、verify 要)
- **PerProgramUBO_FxaaF** (= set=2 binding=9、`FXAA_TC_SCALE` 経由で同 tc_scale 共有の可能性、verify 要)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- window resize 時、関連 viewport / tc_scale UBO と同時 dirty 候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **`tc_scale` 直接 setter call site** = pipeline.cpp 内で postDeferredV program に対し setter を呼ぶ行番号 (= verify 要、`FXAA_TC_SCALE` 共有か別 enum か)
2. **`FXAA_TC_SCALE` enum との関係** = LLShaderMgr enum で同 uniform 名管理されているか、別 enum 名で別管理か (= verify 要)
3. **`_pad_pdv0` vec2 8 B の将来 member 追加意図** = 将来追加 member 予約 slot か、std140 padding のみか (= verify 要)
4. **同 shader file 内同時 consume UBO 一覧** = postDeferredV.glsl 内全 UBO declaration grep 要
5. **shell 通電 commit** = 未来作業
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要
7. **postDeferredV ↔ postDeferredF program pair 関係** = V/F が同 program に bind されるか、別 program か (= verify 要)

= 上記 7 項目は本 UBO file 完成時に逐次解消。
