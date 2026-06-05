# PerProgramUBO_FsObjectIdF — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerProgramUBO_FsObjectIdF` への setter 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 `object_id_packed` vec4 (= GPU object-ID buffer 用 packed ID、r21 self rigged picker と関連) を host から PerProgram cadence setter 経由で書込 + class1/deferred/fsObjectIDF.glsl 内 UBO consume へ切替

**章 thesis 関連**: AYAstorm r21 self rigged picker (= memory `project_ayastorm_r21_self_rigged_picker.md` GPU object-ID buffer 経路) と直結 UBO

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_FsObjectIdF`
- **block_hash**: `0x0b5bcc6au`
- **block_size**: 256 B (= std140=16 B, device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_fsobjectidf.inl:12-15
struct PerProgramUBO_FsObjectIdFLayout {
    static constexpr std::uint32_t object_id_packed_OFFSET = 0u;  // size=16 align=16
};
inline constexpr std::uint32_t PerProgramUBO_FsObjectIdF_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_fs_object_id_f.glsl:9-12
layout(std140, set = 2, binding = 13) uniform PerProgramUBO_FsObjectIdF
{
    vec4 object_id_packed;
};
```

= **single vec4 member (= 16 B 内 object ID 4 byte 分割 pack、r21 self rigged picker 経路用)**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 13
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: ubo_metadata.inl:75 `{ "PerProgramUBO_FsObjectIdF", 0x0b5bcc6au, 256u, 2u, 13u, 0u, 1u, 1u }`

**Note**: cadence_tag=1 (= PerProgram) だが r21 self rigged picker は per-draw GPU object-ID write 用 = **cadence が draw-time の id write と矛盾する可能性、verify 要** (= 既存 OpenGL では per-draw uniform4iv 経由、UBO 化で per-program へ昇格は r21 設計と整合性検証要)

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95`)
- **flush 経路**: PerProgram cadence setter
- **r21 整合性 risk**: r21 self rigged picker は per-draw object ID write (= memory `project_ayastorm_r21_self_rigged_picker.md`)、cadence=1 PerProgram への変換は draw 毎再 flush 必要 (= 設計再検討 verify 要)

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `LLDrawInfo::mFSPickerLocalID` (= 推定、memory `project_skin_hash_collision_bom_body.md` M4.17 採用 reference)
  - 既存 OpenGL 経路で `object_id_packed` uniform4iv 書込 site (= r21 実装、grep verify 要)

---

## §5. use site (shader)

- **blueprint file**: `aya_r41_blueprints/set2/per_program_ubo_fs_object_id_f.glsl`
  - source extract from `class1/deferred/fsObjectIDF.glsl:33` ifdef LL_VULKAN_GLSL block (single site)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/deferred/fsObjectIDF.glsl` (= single site、fragment shader、object ID gbuffer write)

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerProgramUBO_FsObjectIdF 専用 setter 不在** (= grep 確認済)
- **本実装化後 setter** (= **不明 / verify 要**):
  - r21 self rigged picker 経路で per-draw object ID 書込 site (= grep verify 要、`LLShaderMgr::FS_OBJECT_ID_PACKED` 等候補)
  - cadence=1 PerProgram 変更時 setter 適合性 verify 要

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点)

---

## §8. 本実装化に必要な作業

1. **host C++ setter 配線** = fsObjectIDF program bind 時に object_id_packed 書込
2. **cadence 整合性 verify** = PerProgram cadence で r21 per-draw write の semantics 維持可能か検証 (= per-draw flush 経路 or cadence 再分類検討)
3. **data source 特定** = `LLDrawInfo::mFSPickerLocalID` 経路 verify
4. **dirty 判定** = per-draw 毎更新ゆえ cadence 不適合可能性
5. **shader 側 #else block 撤去** (= Phase 2+ 時)

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.6.2 同期)

**Layer**: L5-2 (= B Tier γ、cadence mismatch 重大、r21 self rigged picker 機能関連)
**status**: **起案済** (= 2026-06-06 C-7、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.6.2` (= single source of truth)
**要点**: trace L5-2 (= L0-4 cadence 結果待ち)、工数 M、cadence mismatch 重大 (PerProgram vs r21 per-draw)、setter 不明 (= `LLDrawInfo::mFSPickerLocalID` 経路推定)、AYA r21 機能維持必須 (memory `project_ayastorm_r21_self_rigged_picker`)、visual regression ゼロ §5.4、cadence 再分類 = PerProgram 維持 / PerDraw 移行 / sliced UBO **[要 AYA 判断 = L0-4 protocol-B]**
**関連**: L0-1 + L0-4 (= cadence 再分類 strategy 必須) / §5.4 visual regression policy / memory `project_ayastorm_r21_self_rigged_picker`

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=13 独立 | ✅ 独立配置 |
| OS-3 | std140 padding | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**cadence 矛盾 risk** ⚠️:
- r21 per-draw object ID write vs cadence=1 PerProgram = semantics 矛盾
- 解決策候補 = cadence 再分類 (= PerDraw=2 へ変更) または PerProgram cadence で per-draw 毎 flush 経路 (= 既存 setter dispatch 改修)
- = **本 UBO は本実装化前に Phase 1.B 設計 verify 要**

---

## §10. 不明事項

1. **shell 通電 commit hash**
2. **r21 既存 OpenGL setter call site** = `object_id_packed` 書込 site (= `LLShaderMgr::FS_OBJECT_ID_PACKED` 等候補、grep verify 要)
3. **cadence 整合性 verify** = PerProgram cadence で per-draw semantics 維持可能か
4. **`LLDrawInfo::mFSPickerLocalID` 経路** = r21 M4.17 採用経路詳細 verify 要
5. **vec4 pack 内訳** = 16 B 内 object ID 32-bit + 12 B 何が入るか verify 要 (= 単一 ID か複数 ID か LSB/MSB pack か)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=13

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 本 batch PerProgram UBO 群 8 件 (= cadence 矛盾 verify 要は本 UBO 特殊)

### §11.3 同 shader consume UBO

- `fsObjectIDF.glsl` 内 = Frame_* + Material set=1 (= verify 要)

### §11.4 同 data source UBO

- r21 self rigged picker 経路独立 = 他 UBO と data source 共有なし (= 推定)

### §11.5 dirty 連動 UBO

- 単独 dirty (= per-draw 毎更新、他 UBO と非連動、推定)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ fsObjectIDF program bind 時 flush (cadence 矛盾 verify 要)
