# Global_ReflectionProbes — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 (= Phase 1.C PC-2 で `bringupTestUBO()` 経由 zero dummy buffer + 5 cadence 全経路 `vkCmdBindDescriptorSets` 通電完了、shader consume 未開始)

**本実装化に必要な作業**: shell の `_shell_placeholder` member を実 reflection probe data (= LLReflectionMapManager 等経由、verify 要) に置換 + dirty 判定 logic 追加 + flush logic 追加 + 実 shader consume 接続

---

## §1. UBO identity

- **block_name**: `Global_ReflectionProbes`
- **block_hash**: `0xabdfdb31u` (= FNV-1a("Global_ReflectionProbes"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B = 256 B 倍数 padding 充足)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_global_reflectionprobes.inl
struct Global_ReflectionProbesLayout {
    static constexpr std::uint32_t _shell_placeholder_OFFSET = 0u;  // size=16 align=16
};
inline constexpr std::uint32_t Global_ReflectionProbes_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set0/global_reflection_probes.glsl
layout(std140, set = 0, binding = 3) uniform Global_ReflectionProbes
{
    vec4 _shell_placeholder;
};
```

= **shell placeholder 1 member 状態**、Phase 2 で実 reflection probe data 群に置換予定 (= layout 不可触 = set/binding/size 不変契約、roadmap §4.2 + 第二次査読 §3.2)

---

## §2. binding 配線

- **descriptor_set**: 0
- **binding**: 3
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通、`vkCmdBindDescriptorSets` 経路 verify 要)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set layout)
- **set 0 内訳** (= `llvkloader.cpp:858` `V3A_FRAME_SET_BINDINGS = 4` literal):
  - set=0 binding=0 = FrameViewProj
  - set=0 binding=1 = FrameLights
  - set=0 binding=2 = FrameAtmosphere_Lighting
  - set=0 binding=3 = **Global_ReflectionProbes** (本 UBO)
- **source**: `ubo_metadata.inl:45` `{ "Global_ReflectionProbes", 0xabdfdb31u, 256u, 0u, 3u, 0u, 5u, 1u }` + `llvkloader.cpp:858` set 0 同居 literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 5
- **意味**: **SINGLETON** (= `llglslshader.cpp:99` literal: `constexpr U32 kCadenceSingleton = 5u; // Global_ReflectionProbes (flushSingletonUbos 別経路)`)
- **意味詳細**: per-frame 系 (cadence=0) とは別経路、`flushSingletonUbos` で flush、process-wide UBO (= shader-agnostic、reflection map state は frame 内で複数 update し得る)
- **source**: ubo_metadata.inl + llglslshader.cpp:99 literal

---

## §4. 物理 owner

- **shell 段階**: owner なし (= singleton で process-wide instance 1 個)
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `LLReflectionMapManager` (= 推定、grep verify 要、`gPipeline.mReflectionMapManager` 等経路)
  - 既存 OpenGL 経路の reflection probe uniform 書込み site (= `class3/deferred/reflectionProbeF.glsl` 等、grep verify 要)
- **lifetime**: process-wide (= viewer 起動から終了まで)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/global_reflection_probes.glsl` (= Phase 1.C PC-1 起案、shell placeholder 1 vec4 member)
- **実 shader use site** (= 本実装化時に consume 開始する shader):
  - **不明 / verify 要**: 実 PBR / reflection probe consume shader (= `class3/deferred/reflectionProbeF.glsl` 等、grep verify 要)
  - 既存 OpenGL 経路では reflection probe uniform は uniform 個別宣言 (`#else` block)、Vulkan path では UBO 化必要
- **shell 段階 consume**: なし (= bringupTestUBO で経路通電のみ、実 shader は UBO consume せず)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: `LLGLSLShader::bringupTestUBO()` (= `llglslshader.cpp:2273`)
  - 内容: `ubo::lookup_block("Global_ReflectionProbes")` で metadata 取得 → contract assertion (= block_hash + block_size verify) → singleton cadence flush 経路発火
  - source literal: `llglslshader.cpp:2287-2288`:
    ```cpp
    llassert(block->block_hash == 0xabdfdb31u);  // PC-1 contract: FNV-1a("Global_ReflectionProbes")
    llassert(block->block_size == ubo::Global_ReflectionProbes_SIZE);  // PC-1 contract: 256B padded
    ```
- **本実装化後 setter** (= **不明 / verify 要**):
  - 31 setter のうち該当 setter (= reflection probe member 書込み、`uniformMatrix4fv` / `uniform4fv` 等経路) を **grep verify 要**
  - data 書込み tip = `LLReflectionMapManager` 経由の per-probe data 集約 (= 推定、verify 要)

---

## §7. 現状通電状態

- **状態**: **shell 通電済** (= Phase 1.C PC-2)
- **通電 commit**: 不明 (= Phase 1.C handoff doc chain `handoff/phase1/c/handoff-phase1-c-pc-2.md` 参照要、本 file 起案では未引用)
- **通電内容**:
  - zero dummy buffer write (= 16 B std140 / 256 B device-padded 全 zero memcpy)
  - 5 cadence 全経路 `vkCmdBindDescriptorSets` 通電 (= per-frame / per-program / per-draw / per-asset / per-skin、Phase 1.C PC-7δ 完了)
  - shader consume 未開始 (= `_shell_placeholder` は実 shader で参照されない)
- **bringupTestUBO 呼出 site**: `llglslshader.cpp:2062` (= LLGLSLShader 初期化経路、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell `_shell_placeholder` → 実 reflection probe data 群に置換**:
   - 実 member 候補 (= **不明 / verify 要**、既存 OpenGL 経路 uniform 棚卸し要):
     - reflection probe count
     - probe positions array
     - probe influence radii array
     - probe HDR exposure / intensity
     - probe cubemap index / atlas slot 等
2. **dirty 判定 logic 追加**:
   - 持越項目 (RF) = reflection update fence throttle (= roadmap §10.1 + chapter 07 §12 (RF))
   - dirty 判定 trigger = reflection probe update event (= camera move / probe regenerate / scene change)
3. **flush logic 追加**:
   - singleton cadence ゆえ `flushSingletonUbos` 経路使用 (= 既経路活用、roadmap §5.1 cadence 別 update site)
4. **shader 接続**:
   - 該当 shader (= **不明 / verify 要**、class3/deferred/reflectionProbeF.glsl 等) で UBO member consume 開始
   - 既存 OpenGL 経路 (`#else` block) の uniform 個別宣言を UBO member access に書換
5. **codegen 再実行**:
   - blueprint `global_reflection_probes.glsl` の member を実 member に書換 → codegen 再実行で `ubo_layout_global_reflectionprobes.inl` 生成 → host C++ 経路再 build

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合 (= memory `project_r41_phase2_4_principles` 原則 2):

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=0 内に収める | ✅ 維持 (= set 0 内 binding=3、Phase 1.C で配置済) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16B → 256B padded (codegen 出力で生成) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 (= 既存 ring buffer 経路で query 結果使用しているか確認) |
| OS-5 | shader 改変ゼロ | ⚠️ **本 UBO 本実装化は shader 接続必須** = 該当 shader で UBO member access 追加、新 shader 追加なし (= 既存 shader 編集のみ) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 (= Phase 2 cold launch 時) |

**shader 接続 risk**:
- 既存 OpenGL 経路 (`#else` block) の uniform 個別宣言を UBO member access に書換する作業 = shader compile 差分 risk (= 3 OS SPIR-V compile で同一動作確認要)
- 既存 GLSL shader file は upstream Firestorm 由来 = 改変による upstream patch conflict risk (= 設計原則 (1) Upstream 取り込みやすさ維持 と緊張、verify 要)

**layout 互換性**:
- shell layout (= set=0 binding=3 size=256 PSO layout) は Phase 2 本実装と完全一致契約 (= roadmap §4.2)
- Phase 2 で member を実 reflection probe data 群に置換しても、buffer size 256 B / set / binding は不変 (= shell の VkBufferCreateInfo / VkDescriptorBufferInfo / VkPipelineLayoutCreateInfo 引数差分ゼロ契約)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=0、`llvkloader.cpp:858` `V3A_FRAME_SET_BINDINGS = 4` literal)

- FrameViewProj (set=0 binding=0)
- FrameLights (set=0 binding=1)
- FrameAtmosphere_Lighting (set=0 binding=2)
- **Global_ReflectionProbes (set=0 binding=3、本 UBO)**

### §11.2 同 cadence cluster UBO (= cadence_tag=5 SINGLETON、`flushSingletonUbos` 別経路)

- **Global_ReflectionProbes (本 UBO、singleton 唯一)** = singleton cluster は本 UBO 1 個のみ

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- **不明 / verify 要** = reflection probe を consume する shader file 群 (= `class3/deferred/reflectionProbeF.glsl` 等候補) で他に consume される UBO を grep verify 要
- 推定候補 (= verify 要): FrameViewProj + FrameLights + FrameAtmosphere_Lighting (= 同 set=0 同居 UBO は同時 bind ゆえ deferred lighting shader で同時 consume の可能性)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **ReflectionProbeUBO_Legacy** (= `ubo_metadata.inl:72` set=3 binding=17 cadence_tag=1 PerProgram、本 UBO と同 LLReflectionMapManager 由来候補、verify 要)
- 同 data source 命名 pattern = `*ReflectionProbe*` (= 本 UBO `Global_ReflectionProbes` + Legacy `ReflectionProbeUBO_Legacy` の 2 UBO)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = reflection update 時 (= probe regenerate / camera move 等) に同時 dirty になる UBO 確認要
- 推定候補 (= verify 要): ReflectionProbeUBO_Legacy (= 同 data source 由来ゆえ連動可能性大)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout (= Phase 1.A 確立、全 UBO 共通)

### §11.7 bind 順序関係

- frame start で set=0 全 4 UBO 同時 bind (= `vkCmdBindDescriptorSets` 1 回呼出で set=0 帯全 binding 含む)
- 本 UBO の bind timing = frame start (= per-frame UBO 群と同 timing、singleton ゆえ追加 bind なし)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

本 file 起案時点で実コード調査で確定できなかった項目:

1. **実 reflection probe data 構造** = LLReflectionMapManager 内 reflection probe state 構造 (= grep verify 要、`indra/newview/llreflectionmapmanager.h` 等)
2. **実 shader use site** = reflection probe を consume する shader file (= class3/deferred/reflectionProbeF.glsl 等候補、grep verify 要)
3. **既存 OpenGL setter call site** = 既存経路で reflection probe uniform を書込む 31 setter call site (= grep verify 要)
4. **本実装化時 dirty 判定 trigger** = reflection update fence throttle (RF) の具体実装 (= chapter 07 §12 持越項目、Phase 0 計測結果参照要)
5. **bringupTestUBO 呼出 site の完全特定** = `llglslshader.cpp:2062` 呼出 context (= LLGLSLShader 初期化のどこか、verify 要)
6. **shell 通電 commit hash** = Phase 1.C PC-2 完了 commit (= handoff doc 参照要、本 file 未引用)
7. **set=0 内 dynamic offset 経路** = singleton flush 経路の VkDescriptorBufferInfo bind 詳細 (= `vkCmdBindDescriptorSets` 呼出 site verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消、確定後に「不明」記載削除 + 確定 literal 追記。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.9 同期)

**Layer**: L4-9 sub-cluster (a) (= Global_ReflectionProbes 単独 SINGLETON、set=0 PerFrame 帯外 process-wide)
**status**: **起案済** (= 2026-06-06 C-6-d、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.9` (= single source of truth)
**要点**: shell `_shell_placeholder` vec4 → 実 reflection probe data 群置換、**SINGLETON cadence 唯一** (= `flushSingletonUbos` 別経路、`llglslshader.cpp:99` literal)、**shell 通電済 (= Phase 1.C PC-2)** = bringupTestUBO 経由 zero dummy + 5 cadence 全経路 `vkCmdBindDescriptorSets` 通電完了、shader consume 未開始、layout 不可触契約 256B/set=0/binding=3 不変、shader 接続必須 (= class3/deferred/reflectionProbeF.glsl 等候補で UBO member access 追加)、実 member 候補全件不明 (= probe count/positions/radii/HDR/atlas slot 等) [要追加調査]、reflection update fence throttle (RF) 配線必要 (= chapter 07 §12 持越項目)、ReflectionProbeUBO_Legacy との統合 vs 維持判断 [要 AYA 判断]、工数 group 全体 M-L 内
**関連**: L0-1 dispatch (= 衝突なし set=0 binding=3) / §3.5.9 sub-cluster (b) ReflectionProbeUBO_Legacy (= 同 LLReflectionMapManager 由来候補、cadence/set 異) / sub-cluster (c) IBL mip pipeline (= probe regenerate trigger 連動) / Phase 1.C PC-2 (= shell 通電 commit) / chapter 07 §12 (= RF throttle 持越)


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

