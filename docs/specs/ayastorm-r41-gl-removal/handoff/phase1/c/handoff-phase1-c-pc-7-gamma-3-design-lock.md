# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7γ-3 design-lock** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-7γ-3 (per-asset / per-skin cadence **本格化**) 着手前 design-lock + ambiguity 7 件 AYA 確認仰ぎ

---

## §0. PC-7γ-3 literal scope (= AYA 指示 2026-06-05)

1. **codegen Asset_*/Skin_* block 追加** = `scripts/ubo_codegen/main.py` 拡張 + `ubo_metadata.inl` 再生成 + 130 件 unittest 回帰確認
2. **bare OpenGL UBO 置換** = `asset.mNodesUBO` / `asset.mMaterialsUBO` / `skin.mUBO` の `glBindBufferBase` 経路 → cadence UBO 経路
3. **GLTF asset/skin lifecycle hook** = 構築時 `registerAssetUbo` / `registerSkinUbo` + 破棄時 `unregisterAssetUbo` / `unregisterSkinUbo`

**scope 外**:
- PC-7δ (= vkCmdBindDescriptorSets 通電)
- PC-7ε (= dynamic offset 経路)
- PC-7α' (= ubo_metadata.inl V1' set=1a/1b split)

---

## §1. 必読 3 件 (本 design-lock 起案前 Read 済)

1. PC-7γ-2 complete handoff = `handoff/handoff-substep-4-3-...-pc-7-gamma-2-complete.md` (= sCurrentAsset/Skin + setCurrentAsset/Skin + writeAssetUbo/writeSkinUbo + forwardToUboUpload PER_ASSET/PER_SKIN case defensive 通電完了)
2. design 06b §2.4 (per-asset cadence) + §2.5 (per-skin) + §5.3 (per-program/asset/skin register hook) + §5.4 (thread wiring)
3. design 06a §5.4 (mUseUBO flag placement) = mUseUBO=false default 維持

---

## §2. 現状調査結果 (= design-lock 判断材料)

### §2.1 codegen 現状 (= scripts/ubo_codegen/main.py + ubo_metadata.inl)

| 項目 | 現状 |
|------|------|
| cadence enum 6 件 | `CADENCE_PER_ASSET = 3` / `CADENCE_PER_SKIN = 4` 既定義 (`main.py:61-62`) |
| prefix mapping | `("Asset_", CADENCE_PER_ASSET)` / `("Skin_", CADENCE_PER_SKIN)` 既定義 (`main.py:69-70`) |
| ubo_metadata.inl 現 block_count | **91 件** = PER_FRAME 3 + PER_PROGRAM 24 + PER_DRAW 6 + SINGLETON 1 + その他 (PER_PROGRAM_LEGACY 等) |
| Asset_* / Skin_* entry | **0 件** = `grep "Asset_\|Skin_" ubo_metadata.inl` 結果 |
| GLTF* entry | **0 件** = `grep "GLTF" ubo_metadata.inl` 結果 = 現 codegen は `indra/newview/app_settings/shaders/class1/gltf/*.glsl` を取込していない |
| codegen unittest | 130 件 PASS (= `python3 -m unittest discover -s scripts/ubo_codegen/tests`) |

### §2.2 GLSL 現状 (= indra/newview/app_settings/shaders/class1/gltf/)

| GLSL file:line | block 定義 |
|---|---|
| `pbrmetallicroughnessV.glsl:66-82` | `layout (std140) uniform GLTFMaterials { vec4 gltf_material_data[MAX_UBO_VEC4S]; }` |
| `pbrmetallicroughnessV.glsl:284-287` | `layout (std140) uniform GLTFJoints { vec4 gltf_joints[MAX_NODES_PER_GLTF_OBJECT]; }` |
| `pbrmetallicroughnessV.glsl:335-338` | `layout (std140) uniform GLTFNodes { vec4 gltf_nodes[MAX_NODES_PER_GLTF_OBJECT]; }` |
| `pbrmetallicroughnessF.glsl:38` | `layout (std140) uniform GLTFMaterials` |

**block 名は全て prefix なし** (= `GLTFMaterials` / `GLTFJoints` / `GLTFNodes`)。design 06b §2.4 / §2.5 literal の `Asset_GLTFMaterials` / `Asset_GLTFNodes` / `Skin_GLTFJoints` とは命名乖離。

### §2.3 host C++ 既存 binding wire-up

| 参照 site | 内容 |
|---|---|
| `llglslshader.h:166-168` | `UB_GLTF_JOINTS / UB_GLTF_NODES / UB_GLTF_MATERIALS` enum 既定義 (comment 内に `"GLTFJoints"` 等 literal) |
| `llglslshader.cpp:1958-1960` | string literal `"GLTFJoints" / "GLTFNodes" / "GLTFMaterials"` を `glGetUniformBlockIndex` に渡して binding 解決 |
| `llglslshader.cpp:1967-1970` | `UBOBlockIndex = glGetUniformBlockIndex(prog, ubo_names[i])` + `glUniformBlockBinding(prog, UBOBlockIndex, i)` = binding 番号 = enum index |
| `gltfscenemanager.cpp:707` | `glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_NODES, asset.mNodesUBO)` |
| `gltfscenemanager.cpp:710` | `glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_MATERIALS, asset.mMaterialsUBO)` |
| `gltfscenemanager.cpp:761` | `glBindBufferBase(GL_UNIFORM_BUFFER, LLGLSLShader::UB_GLTF_JOINTS, skin.mUBO)` |

### §2.4 host C++ bare OpenGL UBO 経路

| file:line | 内容 |
|---|---|
| `gltf/asset.cpp:181-188` (`uploadTransforms`) | lazy `glGenBuffers(1, &mNodesUBO)` + `glBindBuffer` + `glBufferData(GL_UNIFORM_BUFFER, node_count*12*sizeof(F32), data, GL_STREAM_DRAW)` |
| `gltf/asset.cpp:230-237` (`uploadMaterials`) | lazy `glGenBuffers(1, &mMaterialsUBO)` + `glBufferData(GL_UNIFORM_BUFFER, material_count*12*sizeof(vec4), ...)` |
| `gltf/animation.cpp:409-457` (`Skin::uploadMatrixPalette`) | lazy `glGenBuffers(1, &mUBO)` + `glBufferData(GL_UNIFORM_BUFFER, joint_count*12*sizeof(F32), ...)` |
| `gltf/animation.cpp:394-400` (`Skin::~Skin`) | `if (mUBO) glDeleteBuffers(1, &mUBO);` 既実装 |
| `Asset` dtor | **存在せず** = `mNodesUBO` / `mMaterialsUBO` の `glDeleteBuffers` 経路なし (= memory leak 既存、PC-7γ-3 で同梱 fix 候補) |

### §2.5 host C++ `mUseUBO` 参照 (= GATE-B 整合確認)

`grep "mUseUBO" indra/newview/gltf/` = **0 件** = 既存 `asset.cpp` / `animation.cpp` は `mUseUBO` 分岐を持たず、無条件で bare OpenGL UBO 経路を実行中。

### §2.6 LLVKLoader 6 method 既実装 (= PC-7γ-2 完了)

| method | 実装 site (llvkloader.cpp) | signature |
|---|---|---|
| `registerAssetUbo` | line 4178-4196 | `bool(LL::GLTF::Asset*, U32 block_hash, U32 block_size)` |
| `unregisterAssetUbo` | line 4198-4212 | `void(LL::GLTF::Asset*, U32 block_hash)` |
| `writeAssetUbo` | line 4214-4241 | `void(Asset*, U32 block_hash, U32 offset, const void* data, size_t size)` |
| `registerSkinUbo` | line 4246-4263 | `bool(LL::GLTF::Skin*, U32 block_hash, U32 block_size)` |
| `unregisterSkinUbo` | line 4265-4279 | `void(LL::GLTF::Skin*, U32 block_hash)` |
| `writeSkinUbo` | line 4281-4312 | `void(Skin*, U32 block_hash, U32 offset, const void* data, size_t size)` |

→ PC-7γ-3 host 側実装は本 6 method を call site から呼出するだけで、API 新設は不要。

---

## §3. AYA 確認仰ぎ ambiguity 7 件

### (G1) GLSL block 名 = rename or 温存

**判断点**: design 06b §2.4 / §2.5 literal は `Asset_GLTFNodes` / `Asset_GLTFMaterials` / `Skin_GLTFJoints` (= prefix 付き)、現 GLSL は prefix なし (`GLTFNodes` 等)。codegen prefix mapping (`Asset_` / `Skin_`) を発火させるには block 名 rename が必要。

| 案 | 内容 | 利点 | 欠点 |
|---|---|---|---|
| **(G1-A)** GLSL block 名 rename (= **Claude 推奨**) | `GLTFMaterials` → `Asset_GLTFMaterials` / `GLTFNodes` → `Asset_GLTFNodes` / `GLTFJoints` → `Skin_GLTFJoints`、+ host literal (`llglslshader.cpp:1958-1960`) 追従、+ enum comment (`llglslshader.h:166-168`) 追従、enum 値 (`UB_GLTF_*`) は不変 | design 06b literal 整合、codegen 自動取込、shader 内 member 名 (= `gltf_material_data` 等) 不変、既存 enum (`UB_GLTF_*`) 不変で OpenGL UBO bind path 100% 互換、`glGetUniformBlockIndex(prog, "Asset_GLTFMaterials")` が新 block 名 hit | GLSL 4 file + host 2 file = 計 6 file touch、upstream divergence 微増 (= shader literal 4 行 + host literal 3 行) |
| (G1-B) GLSL block 名温存 + codegen prefix mapping 例外追加 | `main.py` の `_PREFIX_TO_CADENCE` に `("GLTFMaterials", CADENCE_PER_ASSET)` / `("GLTFNodes", CADENCE_PER_ASSET)` / `("GLTFJoints", CADENCE_PER_SKIN)` を hard-coded 追加 | GLSL 改変ゼロ、host literal 不変、upstream divergence 最小 | design 06b literal 乖離 (= cadence 表現 spec drift)、codegen mapping rule が complex 化 (prefix だけでなく block 名 full-match 例外が混在) |

**Claude 推奨 = (G1-A)**

**根拠**:
- design 06b spec literal 完全整合 (= 「Asset_GLTFNodes」「Asset_GLTFMaterials」「Skin_GLTFJoints」3 件 literal 引用)
- 既存 `UB_GLTF_*` enum 値 + `glGetUniformBlockIndex` + `glBindBufferBase(..., UB_GLTF_*, ...)` の OpenGL UBO bind path は enum 値経由で string literal 1 箇所 (= `llglslshader.cpp:1958-1960`) だけ追従すれば 100% 互換
- shader 側 member 参照 (= `gltf_material_data[...]`、`gltf_joints[...]`、`gltf_nodes[...]`) は block 名と独立で touch 不要
- codegen prefix mapping rule の単純性維持 (= prefix-only rule、特例なし)

### (G2) codegen --input 経路に gltf/*.glsl 追加

**判断点**: 現 codegen は gltf 配下 GLSL を取込していない (= `ubo_metadata.inl` に `GLTF*` entry 0 件)。Asset_*/Skin_* block を codegen で取込むには CMake codegen target に gltf/*.glsl path 追加が必要。

**Claude 推奨 = (G2-A) CMake codegen target に gltf/*.glsl 取込追加**

**根拠**:
- 既存 `_discover_inputs()` (`main.py:136-148`) は dir recursive rglob `*.glsl` で取込 = input dir に gltf 含めれば自動取込
- 現 CMake codegen invoke の `--input` path 確認後、必要なら gltf/*.glsl を含む dir 指定に変更
- 別 dir 複製案は二重管理リスク = 不採用

→ ただし CMake 改変は実装 phase で確認 (= shader build pipeline)、本 design-lock では「取込必要」確定のみ。

### (G3) bare OpenGL UBO 置換 strategy

**判断点**: PC-7γ-3 scope literal「置換」の解釈。mUseUBO=false default (= MUSEUBO-A 整合) との両立。

| 案 | 内容 | mUseUBO=false 時 | mUseUBO=true 時 |
|---|---|---|---|
| **(G3-A)** dual-write defensive (= **Claude 推奨**) | `if (mUseUBO)` 分岐で `writeAssetUbo` / `writeSkinUbo` を **追加** 呼出、`glBindBufferBase` 経路は不変温存 | 既存 OpenGL UBO 経路 100% 維持 = MUSEUBO-A 整合 | OpenGL UBO + Vulkan UBO 並列 write (= 過渡期 defensive) |
| (G3-B) 完全置換 | `glGenBuffers` / `glBindBufferBase` / `glBufferData` を完全削除、`writeAssetUbo` / `writeSkinUbo` のみ | OpenGL UBO 描画破綻 = MUSEUBO-A 違反 | Vulkan UBO 経路単独 |
| (G3-C) mUseUBO swap | `if (mUseUBO) { writeAssetUbo(...); } else { glBufferData(...); }` で排他 | OpenGL 経路維持 | Vulkan 経路単独、PC-7δ bind 通電前は描画破綻 |

**Claude 推奨 = (G3-A) dual-write defensive**

**根拠**:
- MUSEUBO-A 整合 (= mUseUBO=false default で既存 OpenGL 描画 100% 維持)
- PC-7δ (= vkCmdBindDescriptorSets 通電) 前は Vulkan UBO 経路の描画 visible 効果は 0、bare OpenGL UBO 削除は安全側でない
- UBO migration one-at-a-time 規律遵守 (= PC-7δ 通電後に bare OpenGL UBO 完全削除を別 sub-step で実施可能)
- design 06b §2.4 注 literal「既存 OpenGL path との差 = glBufferData + glBindBufferBase 直呼びを Vulkan VMA mapped write + descriptor set update に乗せ替えるのみ (= 06c / chapter 07)」は最終 state を指す = 過渡期に dual-write 段階を許容

### (G4) lifecycle hook strategy

**判断点**: `registerAssetUbo` / `registerSkinUbo` 呼出位置 = lazy on upload vs eager on ctor。

| 案 | register 位置 | unregister 位置 |
|---|---|---|
| **(G4-A)** lazy on upload (= **Claude 推奨**) | `uploadTransforms()` / `uploadMaterials()` / `uploadMatrixPalette()` 内 `if (mNodesUBO == 0) glGenBuffers(...)` 直後 (= 既存 lazy alloc 同 pattern) | `Asset` dtor 新設 + `Skin` dtor 拡張 |
| (G4-B) eager on ctor | `Asset::Asset()` / `Skin::Skin()` 内、ctor body 末尾 | 同上 |

**Claude 推奨 = (G4-A) lazy on upload**

**根拠**:
- 既存 `glGenBuffers` lazy alloc pattern と同位置で symmetric
- Asset/Skin instance のうち実 draw する subset のみ register = memory pressure 最小化
- mUseUBO 経路追加で既存 lazy alloc の隣接位置に hook 追加 = diff localize
- block_size は upload 時の data layout 由来で確定 (= ctor 時には data 未確定可能性、lazy なら ready 後)

### (G5) block_hash / block_size 取得方法

**判断点**: `registerAssetUbo(asset, block_hash, block_size)` / `writeAssetUbo(asset, block_hash, offset, data, size)` の `block_hash` 引数解決。

| 案 | 取得経路 | frame 内 cost |
|---|---|---|
| **(G5-A)** compile-time const (= **Claude 推奨**) | `ubo_metadata.inl` 生成の `UB_ASSET_GLTF_NODES` / `UB_ASSET_GLTF_MATERIALS` / `UB_SKIN_GLTF_JOINTS` enum 値 or macro 直接参照 (= codegen 出力で確定) | 0 hash 計算 / frame |
| (G5-B) runtime lookup | `ubo::lookup_runtime("Asset_GLTFNodes")` 等 string hash | shader link 時 1 回 (= R3 path 整合) |

**Claude 推奨 = (G5-A) compile-time const**

**根拠**:
- design 06a §4.2 R3 最速 path 設計趣旨 (= frame 内 hash 計算 0 回) 整合
- codegen 生成 `ubo_metadata.inl` で block_hash を constexpr U32 として export 可能
- block_name → block_hash の string-based runtime lookup は遅延コスト + 文字列定数依存
- ただし block_hash / block_size export の codegen 拡張は本 PC-7γ-3 で同梱 (= main.py 出力 format 追加)

**block_size**:
- (G5-A1) **upper bound** (= MAX_UBO_VEC4S × 16 B = std140 array 上限) を register に渡す + write 時 partial size
- (G5-A2) runtime per-asset 個別計算 size を register 時に渡す
**Claude 推奨 = (G5-A1) upper bound at register, runtime size at write**
- 根拠 = `UboInstance::vk_buffer` 確保 size は固定 (= triple-buffer 同 size)、毎 write は runtime size、std140 array size 上限は `MAX_UBO_VEC4S` / `MAX_NODES_PER_GLTF_OBJECT` 由来で確定

### (G6) Asset dtor 新設 = scope 同梱 vs 別 fix

**判断点**: 既存 `gltf::Asset` class に dtor 存在せず (= `mNodesUBO` / `mMaterialsUBO` の `glDeleteBuffers` なし、memory leak 既存)。PC-7γ-3 で `unregisterAssetUbo` 呼出位置として Asset dtor 新設が必要。

| 案 | 内容 |
|---|---|
| **(G6-A)** Asset dtor 新設 = `glDeleteBuffers` + `unregisterAssetUbo` 両方 (= **Claude 推奨**) | `~Asset()` body で `if (mNodesUBO) glDeleteBuffers(1, &mNodesUBO);` + `if (mMaterialsUBO) glDeleteBuffers(1, &mMaterialsUBO);` + `LLVKLoader::unregisterAssetUbo(this, UB_ASSET_GLTF_NODES);` + 同 MATERIALS |
| (G6-B) Asset dtor 新設 = `unregisterAssetUbo` のみ | G3-A dual-write 採用なら bare OpenGL UBO leak は維持、PC-7δ 後の別 sub で `glDeleteBuffers` 同時 fix |

**Claude 推奨 = (G6-A) glDeleteBuffers + unregisterAssetUbo 両方同梱**

**根拠**:
- G3-A dual-write 採用 = bare OpenGL UBO は依然有効 = `glDeleteBuffers` resource 解放は必須 (= 別 sub に持ち越すと leak 継続)
- Asset dtor 新設 = scope は「lifecycle hook」literal に含意 (= AYA 指示「破棄時 unregister*Ubo」literal)
- Skin dtor (animation.cpp:394-400) 既存 `glDeleteBuffers` 路を symmetric に Asset 側でも実装する整合性 (= 既存設計上の漏れ修正)
- feedback_root_cause_not_dump 整合 (= fallback で誤魔化さず根本修正)

### (G7) codegen unittest block_count hardcoded fail risk

**判断点**: ubo_metadata.inl block_count 91 → 94 (+3) で test fail risk。

| 確認項目 | 対応 |
|---|---|
| `scripts/ubo_codegen/tests/` の block_count hardcoded 有無 | grep で確認、必要なら test 更新 (= 91 → 94) |
| 130 件 unittest 全件 PASS 維持 | 本 PC-7γ-3 で block 追加後 unittest 再走で確認 |

→ 本判断は実装 phase で確認 + test 更新、design-lock 段階の判断不要。

---

## §4. PC-7γ-3 実装計画 (= AYA 確認後実施)

### §4.1 §0 scope 3 件分解 (= 7 件 ambiguity 確定後)

#### §4.1.1 scope 1: codegen Asset_*/Skin_* block 追加

| step | 内容 | file 改変 |
|---|---|---|
| (a) GLSL block 名 rename (G1-A) | `GLTFMaterials` → `Asset_GLTFMaterials` / `GLTFNodes` → `Asset_GLTFNodes` / `GLTFJoints` → `Skin_GLTFJoints` | `class1/gltf/pbrmetallicroughnessV.glsl` 3 件 + `pbrmetallicroughnessF.glsl` 1 件 |
| (b) host literal 追従 (G1-A 同梱) | `"GLTFMaterials"` 等 string literal を rename | `indra/llrender/llglslshader.cpp:1958-1960` |
| (c) enum comment update (G1-A 同梱) | comment 追従 | `indra/llrender/llglslshader.h:166-168` |
| (d) CMake codegen input に gltf/ path 追加 (G2-A) | shader build 取込確認 | CMake (= shader build target、要確認) |
| (e) codegen 拡張 = block_hash export | `ubo_metadata.inl` に `UB_ASSET_GLTF_NODES` 等 macro / constexpr 追加 (= G5-A 対応) | `scripts/ubo_codegen/main.py` / `perfect_hash.py` |
| (f) codegen 再実行 = `ubo_metadata.inl` 再生成 | 91 → 94 件 block entry | `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` |
| (g) 130 件 unittest 回帰確認 (G7) | test 更新 (block_count hardcoded 有れば) | `scripts/ubo_codegen/tests/` (= 必要時) |

#### §4.1.2 scope 2: bare OpenGL UBO 置換 (= dual-write defensive G3-A)

| step | 内容 | file 改変 |
|---|---|---|
| (h) `uploadTransforms()` 内 dual-write 配線 | `if (mUseUBO) LLVKLoader::writeAssetUbo(this, UB_ASSET_GLTF_NODES, 0, data, size);` を `glBufferData` 後に追加 | `gltf/asset.cpp:181-188` |
| (i) `uploadMaterials()` 内 dual-write 配線 | 同 pattern (UB_ASSET_GLTF_MATERIALS) | `gltf/asset.cpp:230-237` |
| (j) `Skin::uploadMatrixPalette()` 内 dual-write 配線 | 同 pattern (UB_SKIN_GLTF_JOINTS) | `gltf/animation.cpp:455-457` |

注: `mUseUBO` flag は GLTF::Asset / GLTF::Skin にはなく、`LLGLSLShader::mUseUBO` 経路。Asset/Skin 側では別 condition (= 例: `LLVKLoader::isVulkanActive()` or 同等) が必要、これは forwardToUboUpload 経路と同 flag を共有可能。具体 condition は §4.2 で詳述。

#### §4.1.3 scope 3: GLTF asset/skin lifecycle hook

| step | 内容 | file 改変 |
|---|---|---|
| (k) `uploadTransforms()` lazy register 追加 (G4-A) | `if (mNodesUBO == 0) { glGenBuffers(...); LLVKLoader::registerAssetUbo(this, UB_ASSET_GLTF_NODES, ASSET_NODES_UPPER_BOUND_SIZE); }` | `gltf/asset.cpp:181-184` |
| (l) `uploadMaterials()` lazy register 追加 (G4-A) | 同 pattern (UB_ASSET_GLTF_MATERIALS) | `gltf/asset.cpp:230-232` |
| (m) `Skin::uploadMatrixPalette()` lazy register 追加 (G4-A) | 同 pattern (UB_SKIN_GLTF_JOINTS) | `gltf/animation.cpp:409-411` |
| (n) Asset dtor 新設 (G6-A) | `~Asset()` body で `glDeleteBuffers` + `unregisterAssetUbo` × 2 | `gltf/asset.h` + `gltf/asset.cpp` |
| (o) Skin dtor 拡張 (G6-A symmetric) | 既存 `~Skin()` body に `unregisterSkinUbo(this, UB_SKIN_GLTF_JOINTS);` 1 行追加 | `gltf/animation.cpp:394-400` |

### §4.2 `mUseUBO` flag 経路の §2.5 で 0 件問題

**現状**: gltf/asset.cpp / animation.cpp に `mUseUBO` 参照 0 件 = dual-write 配線時の condition は何?

**候補**:
- (a) **`LLVKLoader::isVulkanActive()`** 相当 helper 経由 = Vulkan init 済かを runtime check (= sAllocator != VK_NULL_HANDLE 等)、これは Vulkan 経路有効化条件
- (b) **runtime cvar** (= `AYAUboRedirectEnabled` 等) で flip
- (c) GLTF Asset draw 時の shader (= GLTFSceneManager 経由) の `mUseUBO` を参照

**Claude 推奨 = (a) `LLVKLoader::isVulkanActive()` 等の helper 経由**:
- 根拠 = registerAssetUbo / writeAssetUbo 内部で既に `sAllocator == VK_NULL_HANDLE` 等の defensive guard を持つ (= PC-7γ-2 実装、llvkloader.cpp:4178+)
- → 上位 caller 側で `if (mUseUBO)` のような明示 gate は不要、registerAssetUbo / writeAssetUbo を **無条件で呼出すれば内部 guard で no-op safe**
- → §4.1.2 / §4.1.3 の dual-write / register 呼出は `if (mUseUBO)` 等の gate 不要、registerAssetUbo / writeAssetUbo を unconditional call (= 内部 defensive guard で no-op)

**(G3-A) dual-write defensive の実体修正**:
- Asset/Skin 側に `mUseUBO` 参照を新規追加せず、registerAssetUbo / writeAssetUbo を unconditional call
- LLVKLoader 側 internal guard (= sAllocator / sAssetUboDirty.find) で:
  - register: sAllocator == VK_NULL_HANDLE → 即 return (= Vulkan 未初期化時 no-op)
  - write: sAssetUboDirty.find() == end() → 即 return (= 未 register 時 no-op、PC-7γ-2 既実装)
- → mUseUBO=false default (= LLGLSLShader::mUseUBO) でも Asset/Skin 側 dual-write は走るが、descriptor bind 未通電 (= PC-7δ scope) で実描画には影響ゼロ
- → MUSEUBO-A 整合維持 + UBO migration one-at-a-time 整合

### §4.3 ASSET_NODES_UPPER_BOUND_SIZE 等の数値確定

| 定数 | 数値 | 由来 |
|---|---|---|
| `MAX_UBO_VEC4S` | shader preprocess const、要 grep / GLSL 定義確認 | `class1/gltf/*.glsl` 等の `#define MAX_UBO_VEC4S N` or shader define 経路 |
| `MAX_NODES_PER_GLTF_OBJECT` | 同上 | 同上 |
| Asset_GLTFNodes block_size | `MAX_NODES_PER_GLTF_OBJECT * 3 * sizeof(vec4)` = `MAX × 48` B (= node 1 個 = 3 vec4 = mat3x4 packed) | shader 内 `vec4 gltf_nodes[MAX_NODES_PER_GLTF_OBJECT]` の std140 sizeof |
| Asset_GLTFMaterials block_size | `MAX_UBO_VEC4S * sizeof(vec4)` = `MAX × 16` B | 同上 (`vec4 gltf_material_data[MAX_UBO_VEC4S]`) |
| Skin_GLTFJoints block_size | `MAX_NODES_PER_GLTF_OBJECT * sizeof(vec4)` = `MAX × 16` B | 同上 (`vec4 gltf_joints[MAX_NODES_PER_GLTF_OBJECT]`) |

注: `MAX_UBO_VEC4S` / `MAX_NODES_PER_GLTF_OBJECT` の literal 値は実装 phase で grep 確認、shader preprocess 後 codegen が std140 size として導出する経路 (= 既存 codegen の std140 layout 計算機能、main.py:240 付近) を信頼。

---

## §5. Exit Criteria 8 項 (= PC-7γ-3 完了判定)

| # | Exit Criteria |
|---|---|
| (i) | GLSL block 名 rename = `Asset_GLTFNodes` / `Asset_GLTFMaterials` / `Skin_GLTFJoints` (G1-A) + host literal 追従 (`llglslshader.cpp:1958-1960`) + enum comment update (`llglslshader.h:166-168`) |
| (ii) | CMake codegen input に gltf/*.glsl 取込 (G2-A) = codegen 再生成で `ubo_metadata.inl` block_count 91 → 94 |
| (iii) | codegen 拡張 = block_hash export macro / constexpr (= `UB_ASSET_GLTF_NODES` 等) `ubo_metadata.inl` 出力 (G5-A) |
| (iv) | bare OpenGL UBO 置換 = `uploadTransforms` / `uploadMaterials` / `uploadMatrixPalette` 内 dual-write 配線 (G3-A) |
| (v) | lifecycle hook = lazy register on upload (G4-A) + Asset dtor 新設 + Skin dtor 拡張 (G6-A) |
| (vi) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、mUseUBO 経路は LLVKLoader internal defensive guard で代替 (§4.2) |
| (vii) | MUSEUBO-A 整合 = mUseUBO=false default (= LLGLSLShader::mUseUBO) で既存 OpenGL 描画 100% 維持、Vulkan UBO 経路は PC-7δ bind 通電前で実描画影響ゼロ |
| (viii) | build verify = llrender + gltfscenemanager + asset/animation TU rebuild PASS + warning 0 + TUT 3 件 (11+10+13) + codegen 130/130 (= block_count update 反映後) PASS |

---

## §6. 残 strict 線形 (= PC-7γ-3 後 sub-step)

- **PC-7δ** = vkCmdBindDescriptorSets 通電 + set=3 swap + sAYAStandardLayout 経由 bind = 既存 placeholder bind path から V3a layout へ移行 + SINGLETON case llassert_always → flushSingletonUbos 経由 bind 通電
- **PC-7ε** = dynamic offset 経路 ring buffer chunk hand-off
- **PC-7α'** = codegen ubo_metadata.inl V1' update = set=1a/1b split
- **PC-8** = 3 OS build verify
- **PC-N** = Phase 1.C complete marker

---

## §7. AYA 確認依頼事項 (= 7 件 batch)

design-lock 確認 batch (literal「OK」or 「採用案 X-X」回答求む):

1. **(G1)** GLSL block 名 = (G1-A) rename 採用? `GLTFMaterials` → `Asset_GLTFMaterials` / `GLTFNodes` → `Asset_GLTFNodes` / `GLTFJoints` → `Skin_GLTFJoints` (GLSL 4 file + host 2 file 計 6 file touch、enum 値不変)
2. **(G2)** CMake codegen input に gltf/*.glsl 取込追加採用?
3. **(G3)** bare OpenGL UBO 置換 = (G3-A) dual-write defensive 採用? (= mUseUBO=false 時 OpenGL 維持 + Vulkan 並列 write 追加)
4. **(G4)** lifecycle hook = (G4-A) lazy on upload 採用?
5. **(G5)** block_hash 取得 = (G5-A) compile-time const macro 採用? + block_size = upper bound at register / runtime size at write
6. **(G6)** Asset dtor 新設 = (G6-A) glDeleteBuffers + unregisterAssetUbo 同梱採用? (= 既存 leak fix 同梱)
7. **(G7)** codegen unittest hardcoded block_count 確認 + 必要時更新 = 実装 phase で確認 (= design-lock 判断不要、informational)

加えて:
- §4.2 `mUseUBO` 経路 = LLVKLoader internal defensive guard で代替 (= Asset/Skin 側 `if (mUseUBO)` 不要、unconditional call) 採用?
- §5 Exit Criteria 8 項全採用?

ambiguity batch 確認 + Exit Criteria 確定後、実装 phase 着手します。

---

## §7.1 AYA 確認 record (= 2026-06-05 受領)

**AYA literal 回答** (2026-06-05): **「OK」**

→ G1..G7 全件 (= G1-A / G2-A / G3-A / G4-A / G5-A + G5-A1 / G6-A / G7 informational) + §4.2 mUseUBO 経路 LLVKLoader internal defensive guard 代替 + §5 Exit Criteria 8 項 **全件採用確定**。

**context 残量により本 session で実装着手せず、新 session で実装 phase 着手** (= AYA 指示 2026-06-05「コンテキストもうないと思うので新セッションで作業」literal)。

---

## §7.2 新 session 引継 marker (= PC-7γ-3 実装 phase 着手 1 line)

**着手 1 line**: PC-7γ-3 実装 phase = §4.1 scope 3 件 (codegen + bare OpenGL UBO dual-write + lifecycle hook) 全件採用確定済、本 design-lock doc §4.1.1 / §4.1.2 / §4.1.3 step (a)..(o) 順実施。

**必読 1 件**: 本 design-lock doc (= PC-7γ-3 全 ambiguity 確定 + 実装計画 + Exit Criteria 含む)

**実装 step 順序** (= §4.1 step (a)..(o) 順):
- (a)-(d) GLSL block 名 rename + host literal + enum comment + CMake codegen input
- (e)-(g) codegen 拡張 + ubo_metadata.inl 再生成 + unittest
- (h)-(j) bare OpenGL UBO dual-write 配線 3 site
- (k)-(o) lazy register + Asset dtor 新設 + Skin dtor 拡張

**新 session 着手前 self-verify**:
- 本 design-lock doc 全文 Read (= ambiguity 7 件 + Claude 推奨根拠 + §4.1 step 詳細)
- AYA 追加質問なしで実装 phase 直入り可能
- 実装後 self-verify 9 観点 → AYA「commit してください」literal 受領後 commit → PC-7γ-3 complete handoff doc 起案

---

## §A. feedback 遵守 record (= 本 design-lock phase)

- **feedback_design_phase_no_code_write** 遵守 = 本 PC-7γ-3 design-lock phase = `indra/` 改変ゼロ、本 markdown 起案のみ
- **feedback_doubt_self_first** 遵守 = G1..G7 7 件 ambiguity 発見で停止 + Claude 推奨 + AYA 確認仰ぎ
- **feedback_confirm_referent_before_acting** 遵守 = 7 件 batch AYA 確認、推測実装回避
- **feedback_ubo_migration_one_at_a_time** 遵守 = PC-7γ-3 scope literal 3 件 (codegen + bare OpenGL UBO 置換 + lifecycle hook) 単独実施、bind 通電 (PC-7δ) + dynamic offset (PC-7ε) は別 sub-step 分離
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 3 件 + pinpoint reference Explore 1 件
- **feedback_no_scope_shrink** 遵守 = PC-7γ-3 literal scope (= codegen + bare OpenGL UBO 置換 + lifecycle hook) 完全カバー、過渡期 dual-write (G3-A) は scope 縮小ではなく PC-7δ 前の安全側段階措置
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示まで commit 実施せず

---

**= 本 design-lock doc 起案完了。AYA 確認 (= §7 7 件 + 補足 2 件) 後、実装 phase 着手します。**
