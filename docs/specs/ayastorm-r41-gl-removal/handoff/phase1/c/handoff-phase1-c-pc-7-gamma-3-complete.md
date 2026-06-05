# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7γ-3 complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-7γ-3 (per-asset / per-skin cadence **本格化**) 完了 marker + PC-7δ (= vkCmdBindDescriptorSets 通電) 引継

---

## §0. 必読 3 件 (次 session 着手前)

1. **本 handoff doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-gamma-3-complete.md`
2. **PC-7γ-3 design-lock handoff doc**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-gamma-3-design-lock.md`
   - (G1)..(G7) 7 件 ambiguity 採用根拠 + §4.1.1 / §4.1.2 / §4.1.3 step (a)..(o) 実装計画
3. **design 06b §2.4 + §2.5 + §5.2 + §5.3 + §5.4** + **design 06c §2 全体** + **design 07 §9.1 + §9.3 + §12**
   - pinpoint reference 別記:
     - `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` §2.4 / §2.5 (per-asset / per-skin update site) + §5.2 (forwardToUboUpload routing) + §5.3 (register hook) + §5.4 (thread wiring)
     - `docs/specs/ayastorm-r41-gl-removal/design/06c-buffer-layout-and-binding.md` §2 (V3a 5-set scheme = PC-7δ で bind 通電予定)
     - `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` §9.1 (vkCmdBindDescriptorSets 経路) + §9.3 (dynamic offset) + §12 (frame loop)

---

## §1. PC-7γ-3 完了内容 = per-asset / per-skin **本格化**

### §1.0 scope = AYA design-lock 確認 7 件実装 (= design-lock doc §5 Exit Criteria 8 項全充足)

PC-7γ-3 **literal scope** (= AYA 「PC-7γ-3 実装 phase 着手お願いします」literal 受領 2026-06-05、design-lock §7.1 「OK」literal で全 ambiguity 採用確定):

1. **GLSL block 名 rename** = `GLTFMaterials` → `Asset_GLTFMaterials` / `GLTFNodes` → `Asset_GLTFNodes` / `GLTFJoints` → `Skin_GLTFJoints` + host literal 追従 + enum comment update (= G1-A、step a/b/c)
2. **set3 blueprint 3 件新設** = `aya_r41_blueprints/set3/asset_gltf_{nodes,materials}.glsl` + `skin_gltf_joints.glsl` (set=3, binding=0/1/2 per llvkloader.cpp:657 V3A_ASSET_SET_BINDINGS=3、step d、G2-A 整合)
3. **codegen `block_hash` 名前空間 export 拡張** = `perfect_hash.py::_emit_block_hash_constants` 新設、`ubo_metadata.inl` に `namespace ubo::block_hash { inline constexpr std::uint32_t Asset_GLTFNodes = 0x6e78dce2u; ... }` 追加 (= G5-A、step e)
4. **ubo_metadata.inl 再生成 + unittest 130/130 PASS** = block_count 91 → 94 (= G7、step f/g)
5. **bare OpenGL UBO dual-write 配線** = `gltf/asset.cpp::uploadTransforms()` + `uploadMaterials()` + `gltf/animation.cpp::Skin::uploadMatrixPalette()` の `glBufferData` 直後で `LLVKLoader::writeAssetUbo` / `writeSkinUbo` を unconditional 並走 (= G3-A、step h/i/j)
6. **lazy register hook** = 3 site の `glGenBuffers` 直後で `LLVKLoader::registerAssetUbo` / `registerSkinUbo`、block_size=16384 B std140 upper bound (= G4-A + G5-A1、step k/l/m)
7. **Asset dtor 新設 + Skin dtor 拡張** = `~Asset()` 新設 (= `glDeleteBuffers` × 2 + `unregisterAssetUbo` × 2、既存 mNodesUBO/mMaterialsUBO leak fix 同梱) + `~Skin()` body 末尾に `unregisterSkinUbo` 1 行追加 (= G6-A symmetric、step n/o)

**本 PC-7γ-3 scope 外 (= PC-7δ 以降 scope)**:
- vkCmdBindDescriptorSets 通電 (= PC-7δ)
- set=3 swap + sAYAStandardLayout 経由 bind (= PC-7δ)
- dynamic offset 経路 ring buffer chunk hand-off (= PC-7ε)
- codegen ubo_metadata.inl V1' set=1a/1b split (= PC-7α'、Z2-C 持越)

### §1.1 採用根拠 7 件 (= AYA literal「OK」受領 record 2026-06-05)

(G1-A) GLSL block 名 rename 採用:
- 根拠 = design 06b §2.4 / §2.5 literal (= `Asset_GLTFNodes` / `Asset_GLTFMaterials` / `Skin_GLTFJoints`) 完全整合、既存 `UB_GLTF_*` enum 値不変で OpenGL UBO bind path 100% 互換、shader 内 member 参照 (= `gltf_material_data[...]` 等) は block 名と独立で touch 不要、codegen prefix mapping rule の単純性維持

(G2-A) CMake codegen input に gltf/ path 追加採用:
- 根拠 = blueprint dir `aya_r41_blueprints/set3/` 3 file 新設で `_discover_inputs()` rglob `*.glsl` 経由自動取込、CMake `AyaUboCodegen.cmake` 既存 helper (= aya_attach_ubo_codegen) で transitively newview に include path 提供 (= llrender PUBLIC target_include_directories 経由)

(G3-A) bare OpenGL UBO dual-write defensive 採用:
- 根拠 = MUSEUBO-A 整合 (= mUseUBO=false default で既存 OpenGL 描画 100% 維持)、PC-7δ bind 通電前は Vulkan UBO 経路の描画 visible 効果 0、UBO migration one-at-a-time 規律遵守 (= PC-7δ 通電後に bare OpenGL UBO 完全削除を別 sub-step で実施可能)、design 06b §2.4 注 literal 整合

(G4-A) lazy register on upload 採用:
- 根拠 = 既存 `glGenBuffers` lazy alloc pattern と同位置で symmetric、Asset/Skin instance のうち実 draw する subset のみ register = memory pressure 最小化、block_size は upload 時の data layout 由来で確定 (= ctor 時 data 未確定可能性)

(G5-A) compile-time const block_hash + G5-A1 upper bound at register / runtime size at write 採用:
- 根拠 = design 06a §4.2 R3 最速 path 設計趣旨 (= frame 内 hash 計算 0 回) 整合、`ubo_metadata.inl` 出力に `namespace block_hash` 追加で codegen 自動 export、`UboInstance::vk_buffer` 確保 size は固定 (= triple-buffer 同 size)、毎 write は runtime size、std140 array 上限 16384 B は Vulkan 1.3 min UBO size 整合

(G6-A) Asset dtor 新設 = glDeleteBuffers + unregisterAssetUbo 同梱採用:
- 根拠 = G3-A dual-write 採用で bare OpenGL UBO は依然有効 = `glDeleteBuffers` resource 解放必須 (= 別 sub に持ち越すと leak 継続)、既存 `Skin` dtor (animation.cpp:394-400) と symmetric (= 既存設計上の漏れ修正)、feedback_root_cause_not_dump 整合

(G7) codegen unittest hardcoded block_count informational 確認:
- 根拠 = 130 件 unittest 全件 PASS 維持確認、本 PC-7γ-3 で block 追加後 unittest 再走で `Ran 130 tests in 0.063s OK` 確認済 = test 内 block_count hardcoded 不在

加えて (§4.2 mUseUBO 経路 LLVKLoader internal defensive guard 代替) 採用:
- 根拠 = `registerAssetUbo` / `writeAssetUbo` 内部で既に `sAllocator == VK_NULL_HANDLE` 等の defensive guard を持つ (= PC-7γ-2 実装)、上位 caller 側で `if (mUseUBO)` のような明示 gate は不要、unconditional call で内部 guard が no-op safe

### §1.2 編集 1 (indra/llrender/llglslshader.cpp +5 / -2、line 1952-1968) = step (b) host literal 追従

PC-7γ-3 (b) tag block で `ubo_names[]` 配列の `"GLTFJoints"` / `"GLTFNodes"` / `"GLTFMaterials"` を `"Skin_GLTFJoints"` / `"Asset_GLTFNodes"` / `"Asset_GLTFMaterials"` に rename。`UB_GLTF_*` enum 値は不変、glGetUniformBlockIndex(prog, name) の name 文字列のみ変更で codegen cadence (PER_ASSET / PER_SKIN) と整合。

### §1.3 編集 2 (indra/llrender/llglslshader.h +6 / -3、line 163-170) = step (c) enum comment update

PC-7γ-3 (c) tag block で `UB_GLTF_JOINTS` / `UB_GLTF_NODES` / `UB_GLTF_MATERIALS` enum comment を rename 後 block 名に追従。enum 値自体は不変で host call site 改修不要。

### §1.4 編集 3 (indra/newview/app_settings/shaders/class1/gltf/pbrmetallicroughnessV.glsl +3 / -3) = step (a) GLSL block 名 rename

- Line 66: `uniform GLTFMaterials` → `uniform Asset_GLTFMaterials`
- Line 284: `uniform GLTFJoints` → `uniform Skin_GLTFJoints`
- Line 335: `uniform GLTFNodes` → `uniform Asset_GLTFNodes`

### §1.5 編集 4 (indra/newview/app_settings/shaders/class1/gltf/pbrmetallicroughnessF.glsl +1 / -1) = step (a)

- Line 38: `uniform GLTFMaterials` → `uniform Asset_GLTFMaterials` (= 同 block 定義 V/F 共有)

### §1.6 編集 5 (indra/newview/app_settings/shaders/aya_r41_blueprints/set3/*.glsl 3 new files) = step (d)

- `asset_gltf_nodes.glsl` = `layout(std140, set=3, binding=0) uniform Asset_GLTFNodes { vec4 gltf_nodes[1024]; }` (= PerAsset cadence)
- `asset_gltf_materials.glsl` = `layout(std140, set=3, binding=1) uniform Asset_GLTFMaterials { vec4 gltf_material_data[1024]; }`
- `skin_gltf_joints.glsl` = `layout(std140, set=3, binding=2) uniform Skin_GLTFJoints { vec4 gltf_joints[1024]; }` (= PerSkin cadence)
- 1024 vec4 = 16384 B = Vulkan 1.3 min UBO size、std140 upper bound at register、runtime size at write (= G5-A1)
- set=3 binding=0/1/2 は llvkloader.cpp:657 `V3A_ASSET_SET_BINDINGS = 3` 整合

### §1.7 編集 6 (scripts/ubo_codegen/perfect_hash.py +18 / -0) = step (e) codegen block_hash export

`_emit_block_hash_constants(blocks)` helper 新設 = `namespace ubo::block_hash` + 各 block で `inline constexpr std::uint32_t <block_name> = 0x<fnv1a_32>u;` emit。`emit_metadata_inl` 末尾で呼出。

### §1.8 編集 7 (build-linux-x86_64/codegen/ubo/ubo_metadata.inl regen、block_count 91 → 94)

新 entry 3 件:
- `{ "Asset_GLTFMaterials", 0xd0494ee1u, 16384u, 3u, 1u, 0u, 3u, 1u }` (= cadence=3 PerAsset)
- `{ "Asset_GLTFNodes", 0x6e78dce2u, 16384u, 3u, 0u, 0u, 3u, 1u }`
- `{ "Skin_GLTFJoints", 0xd86f22b5u, 16384u, 3u, 2u, 0u, 4u, 1u }` (= cadence=4 PerSkin)

`namespace block_hash` constants 94 件 export = caller `ubo::block_hash::Asset_GLTFNodes` 等で constexpr U32 取得可能。

注: build-linux-x86_64/ 配下 = gitignored、本 commit には含まれず、codegen 再走で再生成される (= AyaUboCodegen.cmake `codegen_ubo` target 経由)。

### §1.9 編集 8 (indra/newview/gltf/asset.cpp +50 / -2) = step (h)(i)(k)(l)(n)

- include 2 件追加 (= `#include "llvkloader.h"` + `#include "ubo/ubo_metadata.inl"`)、PC-7γ-3 tag block で llrender PUBLIC target_include_directories 経由 transitive 取得明文化
- step (n) Asset dtor 新設 (= line 134-150) = `~Asset() { if (mNodesUBO) glDeleteBuffers(...); if (mMaterialsUBO) glDeleteBuffers(...); LLVKLoader::unregisterAssetUbo(this, ubo::block_hash::Asset_GLTFNodes); LLVKLoader::unregisterAssetUbo(this, ubo::block_hash::Asset_GLTFMaterials); }`
- step (k) `uploadTransforms()` lazy register (= line 195-202) = `glGenBuffers` 直後 `LLVKLoader::registerAssetUbo(this, ubo::block_hash::Asset_GLTFNodes, 16384u)`
- step (h) `uploadTransforms()` dual-write (= line 208-214) = `glBufferData` 後 `LLVKLoader::writeAssetUbo(this, ubo::block_hash::Asset_GLTFNodes, 0, glmp.data(), glmp.size() * sizeof(F32))`
- step (l) `uploadMaterials()` lazy register (= 同 pattern、Asset_GLTFMaterials)
- step (i) `uploadMaterials()` dual-write (= 同 pattern、md.size() * sizeof(vec4))

### §1.10 編集 9 (indra/newview/gltf/asset.h +7 / -0) = step (n) Asset dtor 宣言

PC-7γ-3 (n) tag block で `~Asset();` 宣言追加。`Asset() = default;` の直後配置。

### §1.11 編集 10 (indra/newview/gltf/animation.cpp +28 / -0) = step (j)(m)(o)

- include 2 件追加 (= llvkloader.h + ubo_metadata.inl)
- step (o) Skin dtor 拡張 (= line 394-405) = 既存 `if (mUBO) glDeleteBuffers(...);` 後に `LLVKLoader::unregisterSkinUbo(this, ubo::block_hash::Skin_GLTFJoints);` 1 行追加
- step (m) `Skin::uploadMatrixPalette()` lazy register (= 同 pattern、Skin_GLTFJoints)
- step (j) `Skin::uploadMatrixPalette()` dual-write (= 同 pattern、glmp.size() * sizeof(F32))

---

## §2. build verify (= AYA `feedback_build_only_verified` 整合)

### §2.1 llrender build

```
make -j4 llrender
[100%] Linking CXX static library libllrender.a
[100%] Built target llrender
```

= ERROR 0 / WARNING 0 (= PC-7γ-3 改変関連、llglslshader.cpp + .h)

### §2.2 newview gltf TU + gltfscenemanager.cpp rebuild

```
touch indra/newview/gltf/asset.cpp indra/newview/gltf/animation.cpp \
      indra/newview/gltfscenemanager.cpp
make -j4 -f newview/CMakeFiles/ayastorm-bin.dir/build.make \
     newview/CMakeFiles/ayastorm-bin.dir/gltf/asset.cpp.o \
     newview/CMakeFiles/ayastorm-bin.dir/gltf/animation.cpp.o \
     newview/CMakeFiles/ayastorm-bin.dir/gltfscenemanager.cpp.o
[ 31%] Building CXX object newview/CMakeFiles/ayastorm-bin.dir/gltf/asset.cpp.o
[ 31%] Building CXX object newview/CMakeFiles/ayastorm-bin.dir/gltf/animation.cpp.o
[ 31%] Building CXX object newview/CMakeFiles/ayastorm-bin.dir/gltfscenemanager.cpp.o
exit=0
```

= ERROR 0 / WARNING 0、`#include "llvkloader.h"` + `#include "ubo/ubo_metadata.inl"` の transitively include 経路 (= llrender PUBLIC target_include_directories 経由) 動作確認。

### §2.3 TUT integration tests

```
sharedlibs/bin/INTEGRATION_TEST_lluboringbuffer
  Total Tests: 11 / Passed Tests: 11 YAY!! \o/

sharedlibs/bin/INTEGRATION_TEST_llassetubopool
  Total Tests: 10 / Passed Tests: 10 YAY!! \o/

sharedlibs/bin/INTEGRATION_TEST_llpipelinecachestorage
  Total Tests: 13 / Passed Tests: 13 YAY!! \o/
```

= PC-3 / PC-4 / PC-5 algorithm 層 regression なし

### §2.4 codegen unittest

```
python3 -m unittest discover -s scripts/ubo_codegen/tests
Ran 130 tests in 0.063s
OK
```

= 130/130 PASS = Phase 1.A / 1.B / 1.C PC-1..PC-7γ-2 regression なし

---

## §3. PC-7γ-3 Exit Criteria 8 項全充足

| # | Exit Criteria | 充足状況 |
|---|--------------|---------|
| (i) | GLSL block 名 rename + host literal + enum comment update (G1-A) | ✅ §1.2 / §1.3 / §1.4 / §1.5 |
| (ii) | CMake codegen input に gltf/*.glsl 取込 = block_count 91 → 94 (G2-A) | ✅ §1.6 / §1.8 |
| (iii) | codegen 拡張 block_hash export macro / constexpr (G5-A) | ✅ §1.7 / §1.8 |
| (iv) | bare OpenGL UBO dual-write 配線 (G3-A) | ✅ §1.9 / §1.11 |
| (v) | lifecycle hook = lazy register + Asset dtor 新設 + Skin dtor 拡張 (G4-A + G6-A) | ✅ §1.9 / §1.10 / §1.11 |
| (vi) | GATE-B 整合 = #ifdef LL_VULKAN_GLSL 新規追加 0 件 | ✅ §7 (5) |
| (vii) | MUSEUBO-A 整合 = mUseUBO=false default で既存 OpenGL 描画 100% 維持 | ✅ §7 (6) |
| (viii) | llrender + gltf TU rebuild + warning 0 + TUT 3 件 + codegen 130/130 PASS | ✅ §2 |

---

## §4. 残 strict 線形

- **PC-7δ** = vkCmdBindDescriptorSets 通電 + set=3 swap + sAYAStandardLayout 経由 bind = 既存 placeholder bind path から V3a layout へ移行 + SINGLETON case llassert_always → flushSingletonUbos 経由 bind 通電 (= source doc = 06c §2 全体 + 07 §9.1 + §9.3 + §12)
- **PC-7ε** = dynamic offset 経路 ring buffer chunk hand-off = set=2 per-draw を sDrawUboRingBufferMgr 経由 dynamic offset 計算 + pDynamicOffsets[4] 配線
- **PC-7α'** = codegen ubo_metadata.inl V1' update = scripts/ubo_codegen/main.py で set=1a/1b split 実装 + ubo_metadata.inl 再生成 + 130 件 unittest 回帰確認、Z2-C 持越、PC-7δ 通電前に必要
- **PC-8** = 3 OS build verify、Linux primary + Win/Mac 後段
- **PC-N** = Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点

---

## §5. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + **PC-7γ-3 ✅ 本 commit** + PC-7δ..PC-N ⏳ 次 session

---

## §6. self-verify 9 観点 全 ✅

1. **Exit Criteria 8 項全充足** = §3 確認
2. **GLSL block 名 rename source doc 整合** = design 06b §2.4 / §2.5 literal (= Asset_GLTFNodes / Asset_GLTFMaterials / Skin_GLTFJoints) 完全整合、`UB_GLTF_*` enum 値不変で OpenGL UBO bind path 100% 互換
3. **(G1-A)(G2-A)(G3-A)(G4-A)(G5-A + G5-A1)(G6-A)(G7) 7 件採用根拠 record** = §1.1 で 7 件 + §4.2 mUseUBO 代替 1 件確認 + 採用根拠明文化、AYA literal「OK」(2026-06-05) record
4. **GATE-B 整合** = `#ifdef LL_VULKAN_GLSL` 新規追加 0、PC-7γ-3 改変は全て mUseUBO runtime gate (= LLVKLoader internal defensive guard) 配下、scaffolding (= ubo_metadata.inl block_hash namespace + lifecycle hook) は Vulkan init 層単独動作
5. **MUSEUBO-A 整合** = mUseUBO=false default 時の経路 = setter 内 forwardToUboUpload 不呼出 + Asset/Skin 側 unconditional `writeAssetUbo`/`writeSkinUbo` 呼出は LLVKLoader 内側 `sAllocator == VK_NULL_HANDLE` + `sAssetUboDirty.find() == end()` defensive guard で no-op、bare OpenGL UBO 経路 (= glBufferData + glBindBufferBase) は不変温存で既存 OpenGL 描画 100% 維持
6. **llrender + gltf TU + gltfscenemanager rebuild + TUT 11+10+13 + codegen 130/130 全 PASS** = §2 確認
7. **commit 内容** = 5 modified (= llglslshader.cpp + .h + pbrmetallicroughnessV.glsl + pbrmetallicroughnessF.glsl + perfect_hash.py + asset.cpp + asset.h + animation.cpp の計 8) + 3 new blueprint glsl + 2 new doc (handoff design-lock + complete) + 新 file 0 (C++ 層) + CMake 改変 0 (= llrender PUBLIC include path で transitive 取得) + settings.xml 改変 0 + Co-Authored-By 不在
8. **feedback_no_scope_shrink 遵守** = PC-7γ-3 literal scope (= codegen Asset_*/Skin_* block 追加 + bare OpenGL UBO 置換 + GLTF lifecycle hook の 3 件 = §1.0 7 件 implementation) 完全実施、G3-A dual-write 採用は PC-7δ 前の安全側段階措置で scope 縮小ではない (= AYA design-lock 確認時明文化済)、bind 通電 (PC-7δ) + dynamic offset (PC-7ε) の分離は UBO migration one-at-a-time 規律遵守
9. **feedback_design_phase_no_code_write 整合** = 本 PC-7γ-3 は実装 phase (= design-lock doc commit 後の実装 phase)、indra/ + scripts/ + shader 配下改変 = 設計 phase ではない

---

## §7. 引き継ぎ memory 14 件

(既存 memory pointer 列挙、変更なし)

1. project_ayastorm_r41_vulkan_migration
2. project_r41_phase1b_vulkan_host_gate (GATE-B 確定)
3. project_ayastorm_r41_design_principles (2 大原則)
4. feedback_ubo_migration_one_at_a_time (1 つずつ実施)
5. feedback_proactive_handoff (引継 marker 起案)
6. feedback_handoff_minimal_pre_req_read (必読 3 件)
7. feedback_self_verify_before_handoff (9 観点)
8. feedback_build_only_verified (literal 検証取得)
9. feedback_no_scope_shrink (literal scope 完遂)
10. feedback_doubt_self_first (ambiguity 停止 + 確認)
11. feedback_confirm_referent_before_acting (batch AYA 確認)
12. feedback_design_phase_no_code_write (実装 phase 整合)
13. feedback_release_branch_workflow (feature branch 上 commit)
14. feedback_no_auto_commit (AYA 明示指示後 commit) + feedback_no_claude_coauthor

---

## §8. 次 session 着手 1 line

**PC-7δ** = vkCmdBindDescriptorSets 通電 = 既存 placeholder bind path から V3a layout (= sAYAStandardLayout、PC-7α で object 化済) へ移行 + set=3 swap (= Asset_GLTFNodes / Asset_GLTFMaterials / Skin_GLTFJoints 3 binding) + SINGLETON case llassert_always → flushSingletonUbos 経由 bind 通電 (= source doc = 06c §2 全体 + 07 §9.1 + §9.3 + §12)、Exit Criteria は次 session 着手前整理

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = PC-7δ 引継 marker 本 handoff
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 3 件 + pinpoint reference 別記
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅
- **feedback_build_only_verified** 遵守 = llrender build + gltf TU rebuild + TUT 11/11 + 10/10 + 13/13 + codegen 130/130 で literal 検証取得
- **feedback_no_scope_shrink** 遵守 = literal scope 完遂、G3-A dual-write は安全側段階措置で縮小ではない (= design-lock 時確認済)
- **feedback_doubt_self_first** 遵守 = (G1)..(G7) 7 件 ambiguity 発見で停止 + 推奨案提示 + AYA 確認 + literal「OK」受領後実装 (= design-lock phase で実施済)
- **feedback_confirm_referent_before_acting** 遵守 = 7 件 + 補足 2 件 batch AYA 確認、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 遵守 = PC-7γ-3 = per-asset / per-skin 本格化単独実施、bind 通電 (PC-7δ) + dynamic offset (PC-7ε) は別 sub-step へ分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-7γ-3 は実装 phase、indra/ + scripts/ + shader 改変
- **feedback_release_branch_workflow** 遵守 = feature branch feature/ayastorm-r41-gl-removal 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「handoff おねがいします そのあと commit してください」literal 受領 (2026-06-05) 後 commit
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
