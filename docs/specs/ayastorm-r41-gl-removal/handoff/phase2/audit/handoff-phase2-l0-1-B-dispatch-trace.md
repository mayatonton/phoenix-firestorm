# handoff = r41 Phase 2.L0 sub-session 2 = L0-1.B trace small prototype = dispatch logic trace doc

**起案日**: 2026-06-06
**位置付け**: Phase 2.L0 sub-session 2 (= L0-1.B trace small prototype) 出力 doc。既存 pilot 通電 UBO 5 件の host dispatch logic を `indra/` 既存 reading のみで trace、L0-1 protocol-A/B/C/D との整合判定。
**起案契機**: handoff `phase2/handoff-phase2-l0-entry.md` §4.3 出力 doc 仕様 + sub-session 1 `handoff-phase2-l0-uncertainty-audit.md` §3.4 N1 解消。
**起案規律**:
- memory `feedback_admit_unknown` 適用 (= 推論禁止、不明明示)
- memory `feedback_doubt_self_first` 適用 (= 仮説の前に literal 取得)
- memory `feedback_handoff_minimal_pre_req_read` 適用 (= 最低限 3 件 + pinpoint)
- 推奨案 OK 自走承認継続、ただし AYA literal 確認 candidate 省略しない
- `indra/` 改変ゼロ (= sub-session B 規律)

---

## §1. 着手目的

既存 pilot 通電 UBO 5 件の host dispatch logic を実コード literal で trace、WORK_ORDER §2.1 L0-1 protocol-A/B/C/D との整合確認。**整合 OK 判定** = sub-session 3 (= L0-1.C 実装) 着手 unblocking、**整合 NG 判定** = sub-session 1 (= L0-1.A 再精査) 戻り。

---

## §2. dispatch logic 全体構造 (= literal 取得結果)

### §2.1 caller path (= UBO write entry point)

```
LLGLSLShader::uniformN(name, ...)
  → getUniformLocation(name) → ubo::UniformLocation { block_hash, offset, size, cadence_tag, ... }
  → if (mUseUBO) forwardToUboUpload(loc, data, size); return;
```

source literal:
- `indra/llrender/llglslshader.cpp:2139-2240` `LLGLSLShader::forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size)`
- `indra/llrender/llglslshader.h:415` 注記「offset / size / cadence_tag を得て forwardToUboUpload() に転送 (= PB-2 / PB-4)」
- `indra/llrender/llglslshader.h:458` `void forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size);`

### §2.2 forwardToUboUpload switch (= 6 cadence case + 3 sentinel case)

cadence_tag (= `ubo::UniformLocation::cadence_tag`) で switch、case 別 setter dispatch:

| cadence_tag | 値 | dispatch | dispatch key |
|---|---|---|---|
| kCadencePerFrame | 0 | `LLVKLoader::writeFrameUbo(loc.block_hash, loc.offset, data, size)` (`llglslshader.cpp:2144`) | block_hash 単独 |
| kCadencePerProgram | 1 | `LLVKLoader::writeProgramUbo(this, loc.block_hash, loc.offset, data, size)` (`llglslshader.cpp:2148`) | `<shader, block_hash>` (= UboInstanceKey) |
| kCadencePerDraw | 2 | `LLVKLoader::writeDrawUbo(loc.block_hash, loc.offset, data, size, dynamic_offset)` (`llglslshader.cpp:2167`) | block_hash + ring buffer dynamic offset |
| kCadencePerAsset | 3 | `getCurrentAsset()` → `writeAssetUbo(asset, loc.block_hash, loc.offset, data, size)` (`llglslshader.cpp:2181/2193`) | `<Asset*, block_hash>` (= UboAssetKey) |
| kCadencePerSkin | 4 | `getCurrentSkin()` → `writeSkinUbo(skin, loc.block_hash, loc.offset, data, size)` (`llglslshader.cpp:2202/2212`) | `<Skin*, block_hash>` (= UboSkinKey) |
| kCadenceSingleton | 5 | `LLVKLoader::writeSingletonUbo(loc.block_hash, loc.offset, data, size)` (`llglslshader.cpp:2226`) | block_hash 単独 |
| kCadenceSampler / kCadenceUnknown / kCadenceInvalid | - | 早期 return (sentinel skip) | - |

source literal: `llglslshader.cpp:94-99`:
```cpp
constexpr U32 kCadencePerFrame   = 0u;
constexpr U32 kCadencePerProgram = 1u;
constexpr U32 kCadencePerDraw    = 2u;
constexpr U32 kCadencePerAsset   = 3u;
constexpr U32 kCadencePerSkin    = 4u;
constexpr U32 kCadenceSingleton  = 5u;
```

### §2.3 name-based dispatch の実体

「name-based dispatch」= **block 名 → block_hash → ubo::UniformLocation 解決経路**:
1. shader 側 GLSL `layout(set=N, binding=M) uniform <UBO_NAME> { ... }` 宣言
2. host C++ 起動時 link 時に `getUniformLocation(name)` で `block_hash` (= FNV-1a hash of `<UBO_NAME>`) + `cadence_tag` + `offset` + `size` を確定
3. `ubo::lookup_block(name)` で `g_block_metadata[]` (= `build-linux-x86_64/codegen/ubo/ubo_metadata.inl:25-120`) walk
4. runtime dispatch は `block_hash` を key として `writeXxxUbo` で UBO buffer / VkDescriptorSet を resolve

⇒ **L0-1 protocol-A (host dispatch logic 仕様) の「program ID + bind target slot 入力、UBO buffer handle + offset 出力」は実装上**:
- 入力 = `LLGLSLShader* this` + `block_hash` + `offset` + `data` + `size` + (cadence 別 `Asset*` / `Skin*`)
- 出力 = cadence 別 `UboInstance` (= mapped buffer + size) or ring buffer chunk (PerDraw)
- program 識別 = `LLGLSLShader*` (= PerProgram cadence のみ key 構成要素)

---

## §3. 5 pilot UBO の dispatch logic 実装位置 (= entry handoff §4.2 trace 対象)

### §3.1 Skin_GLTFJoints (set=3 binding=2, cadence_tag=4 PerSkin)

**通電 status**: real Skin path 一本化済 (= Phase 1.E PC-N-5/11/15c)

| 経路 | function | 行 | 動作 logic |
|---|---|---|---|
| register | `registerSkinUbo(LL::GLTF::Skin*, U32 block_hash, U32 block_size)` | `llvkloader.cpp:5824` 付近 | `sSkinUboDirty.try_emplace(UboSkinKey)` + 内部 wire (= sAssetUboSetV3a binding=2 update) |
| wire (独立) | `wireSkinUboSetV3aToBinding2(LL::GLTF::Skin*)` | `llvkloader.cpp:3180` | `w.dstSet = sAssetUboSetV3a[f]`, `w.dstBinding = 2; // design 06c §2.5: set=3 binding=2 = Skin_GLTFJoints` (`:3215`) |
| write | `writeSkinUbo(LL::GLTF::Skin*, U32 block_hash, U32 offset, const void*, size_t)` | `llvkloader.cpp:5899` | `sSkinUboDirty.find(<skin, block_hash>)` → memcpy → `dirty.store(release)` |
| flush | `flushSkinUbos(LL::GLTF::Skin*)` | `llvkloader.cpp:5250` | walk `sSkinUboDirty` + `dirty.exchange(false)` |
| unregister | `unregisterSkinUbo(LL::GLTF::Skin*, U32 block_hash)` | `llvkloader.cpp:5883` 付近 | `sSkinUboDirty.find` + `destroyUboInstanceBuffers` + erase |
| caller path | `recordGltfAssetDraw` 内 `wireSkinUboSetV3aToBinding2(sCurrentSkin)` + `flushSkinUbos(sCurrentSkin)` | `llvkloader.cpp:6661-6662` | per-Skin instance 切替 timing で wire + flush |

### §3.2 Asset_GLTFMaterials (set=3 binding=1, cadence_tag=3 PerAsset)

**通電 status**: pilot 段階通電済 (= Phase 1.C PC-7γ-3)

| 経路 | function | 行 | 動作 logic |
|---|---|---|---|
| register | `registerAssetUbo(LL::GLTF::Asset*, U32 block_hash, U32 block_size)` | `llvkloader.cpp:5729-5840` 付近 (= PC-7δ (e) register-once + bind-many) | `sAssetUboDirty.try_emplace(UboAssetKey)` + `w.dstSet = sAssetUboSetV3a[f]`, `w.dstBinding = meta.binding` (= set=3 binding=1) (`:5757`) |
| write | `writeAssetUbo(LL::GLTF::Asset*, U32 block_hash, U32 offset, const void*, size_t)` | `llvkloader.cpp:5792` | `sAssetUboDirty.find(<asset, block_hash>)` → memcpy → `dirty.store(release)` |
| flush | `flushAssetUbos(LL::GLTF::Asset*)` | `llvkloader.cpp:5225` | walk `sAssetUboDirty` + `dirty.exchange(false)` |
| caller path | `forwardToUboUpload` PerAsset case → `sCurrentAsset = getCurrentAsset()` → `writeAssetUbo` | `llglslshader.cpp:2172-2195` | `sCurrentAsset` accessor 経由 |

### §3.3 Asset_GLTFNodes (set=3 binding=0, cadence_tag=3 PerAsset)

**通電 status**: pilot 段階通電済 (= Phase 1.C PC-7γ-3、Asset_GLTFMaterials と同経路共有)

dispatch logic は §3.2 と同 (= writeAssetUbo generic 経路、block_hash で識別)。**唯一の差** = binding=0 (= set=3 内位置)、`w.dstBinding = meta.binding` で resolve。

### §3.4 PerDrawUBO_LightParams (set=2 binding=0, cadence_tag=2 PerDraw)

**通電 status**: zero IS real data 通電済 (= Phase 1.E PC-N-13)

| 経路 | function | 行 | 動作 logic |
|---|---|---|---|
| write | `writeDrawUbo(U32 block_hash, U32 offset, const void*, size_t, U32& out_dynamic_offset)` | `llvkloader.cpp:5555` | `sDrawUboRingBufferMgr` で chunk allocate → memcpy → `out_dynamic_offset` 設定 |
| flush | `flushDrawUbos()` | `llvkloader.cpp:5172` | **役割整理済**: setter 内 immediate allocate ゆえ flush 側は no-op (= first-fire LL_INFOS のみ、`:5202-5203` literal) |
| caller path 1 | `recordPlaceholderPoolDraw` 内 `writeDrawUbo(PerDrawUBO_LightParams, 0, zero_buf, 256, dyn_off)` | `llvkloader.cpp:6470-6475` (PC-N-1 (c)) | placeholder phase zero buffer write |
| caller path 2 | `recordGltfAssetDraw` 内 `writeDrawUbo(PerDrawUBO_LightParams, ...)` | `llvkloader.cpp:6612-6617` (PC-N-13 (a)) | real GLTF Asset path、`AYAGltfRealLightParamsEnabled` cvar gate |
| caller path 3 | `:6870-6880` 付近 (PC-N-2 sky_smoke pipeline draw) | 同形 zero buffer write |

### §3.5 Global_ReflectionProbes (set=0 binding=3, cadence_tag=5 SINGLETON)

**通電 status**: shell zero dummy 通電のみ (= Phase 1.C PC-2)、shader consume 未開始

| 経路 | function | 行 | 動作 logic |
|---|---|---|---|
| register | `initVulkan` 内 `sSingletonUboInstances` 先回り allocate (= PC-7δ (p) register-once 経路) | `llvkloader.cpp:4067-4096` 付近 | `sFrameUboInstances` 同形 |
| write | `writeSingletonUbo(U32 block_hash, U32 offset, const void*, size_t)` | `llvkloader.cpp:5470` 付近 | `sSingletonUboInstances[block_hash]` → memcpy → `dirty.store` |
| flush | `flushSingletonUbos()` | `llvkloader.cpp:5292` | walk `sSingletonUboInstances` + `dirty.exchange(false)`、`flushProgramUbos` 対称形 |
| caller path | `LLGLSLShader::bringupTestUBO()` 内 `forwardToUboUpload(loc, s_dummy, ...)` | `llglslshader.cpp:2273` | bringup phase zero dummy write |

---

## §4. set=3 binding 衝突 3 site の既存 dispatch 動作確認

### §4.1 ubo_metadata.inl literal (= 衝突宣言の確認)

`build-linux-x86_64/codegen/ubo/ubo_metadata.inl:25-120` literal:

| 行 | UBO | set | binding | cadence_tag |
|---|---|---|---|---|
| 27 | Asset_GLTFMaterials | 3 | 1 | 3 (PerAsset) |
| 28 | Asset_GLTFNodes | 3 | 0 | 3 (PerAsset) |
| 29 | AtmoExtraUBO_Legacy | 3 | 0 | 1 (PerProgram) |
| 105 | Skin_GLTFJoints | 3 | 2 | 4 (PerSkin) |
| 106 | SkyFParamUBO_Legacy | 3 | 2 | 1 (PerProgram) |
| 107 | SkyVParamUBO_Legacy | 3 | 1 | 1 (PerProgram) |

⇒ **set=3 binding=0/1/2 で各 2 UBO 衝突宣言** (= 計 6 UBO):
- binding=0: Asset_GLTFNodes (PerAsset) ↔ AtmoExtraUBO_Legacy (PerProgram)
- binding=1: Asset_GLTFMaterials (PerAsset) ↔ SkyVParamUBO_Legacy (PerProgram)
- binding=2: Skin_GLTFJoints (PerSkin) ↔ SkyFParamUBO_Legacy (PerProgram)

### §4.2 実 pipeline layout = sAYAStandardLayout の binding 配置 (= literal)

`llvkloader.cpp:858-868` literal:
```cpp
constexpr U32 V3A_FRAME_SET_BINDINGS     = 4;   // set=0
constexpr U32 V3A_PROGRAM_SET_A_BINDINGS = 40;  // set=1a
constexpr U32 V3A_PROGRAM_SET_B_BINDINGS = 40;  // set=1b
constexpr U32 V3A_DRAW_SET_BINDINGS      = 4;   // set=2 UBO_DYNAMIC
constexpr U32 V3A_ASSET_SET_BINDINGS     = 3;   // set=3: Asset_GLTFNodes + Asset_GLTFMaterials + Skin_GLTFJoints
```

`createV3aDescriptorSetLayouts()` (`llvkloader.cpp:2799-2882`) で `build_ubo_layout(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, V3A_ASSET_SET_BINDINGS, sAssetUboLayoutV3a, ...)` → set=3 layout = **3 binding (= 0/1/2) のみ確保**。

⇒ **実 pipeline layout の set=3 = Asset_GLTFNodes/Asset_GLTFMaterials/Skin_GLTFJoints 専有**、Legacy UBO (AtmoExtra/SkyV/SkyF) は実 layout に存在しない。

### §4.3 Legacy UBO の現状通電 (= literal 確認)

`indra/llrender/llvkloader.cpp` 内 "AtmoExtra" / "SkyV" / "SkyF" literal grep:
- **0 件 hit** (= host C++ 側 register / wire / write 配線が全く存在しない)

design file 別 (= sub-session 2 §4 Read 結果):
- AtmoExtraUBO_Legacy: 「untouched (= shell 通電なし、host C++ writer / register 配線無し、shader 側 LL_VULKAN_GLSL block でのみ宣言済)」(line 3)
- SkyVParamUBO_Legacy: 「untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)」(line 3)
- SkyFParamUBO_Legacy: 「untouched (= ... bringupTestUBO 経由 zero dummy buffer write は per-program cadence 全 UBO 一括対象ゆえ shell 通電済の可能性大 / verify 要)」(line 3)

⇒ **現状 paper concern**: Legacy UBO は実 pipeline layout 不在 + host register 配線無し → mUseUBO=true でも writeProgramUbo 経路は走らない (= dispatch 経路で sProgramUboDirty に entry 無し → `LL_WARNS_ONCE` 警告 + 早期 return、`llvkloader.cpp:5503-5515`)。

### §4.4 本実装化時の顕在化 risk (= 推定、verify 要)

Phase 2 で Legacy UBO 群 (= 88 件 PerProgram cluster) を本実装化すると:
1. `writeProgramUbo` 経路で `sProgramUboDirty[<shader, block_hash>]` entry が必要 → register 配線追加要
2. PerProgram cadence の VkDescriptorSet は set=1a / set=1b layout 経路 (= 40+40=80 binding 確保) で運用想定 → **AtmoExtra/SkyV/SkyF の set=3 binding=0/1/2 宣言と矛盾**
3. shader 側 GLSL の `layout(set=3, binding=0) uniform AtmoExtraUBO_Legacy` 宣言 → SPIR-V compile 時に set=3 layout の VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER binding=0 と shader 側 layout 宣言 binding=0 が整合する必要、しかし実 dispatch では sAssetUboSetV3a (= Asset_GLTFNodes) と同 binding 共有
4. ⇒ **shader compile error / Vulkan validation layer warning 候補 (= verify 要、本 sub-session 2 範囲外)**

---

## §5. L0-1 protocol-A/B/C/D 整合判定

| protocol | 内容 (= WORK_ORDER §2.1.4 literal) | 整合判定 | 根拠 |
|---|---|---|---|
| **protocol-A** | host dispatch logic 仕様: 入力=program ID + bind target slot、出力=UBO buffer handle + offset、program 識別 = LLGLSLShader name ベース | **整合 OK** | `forwardToUboUpload` で 6 cadence case + 3 sentinel case 全実装済 (= §2.2)、program 識別は PerProgram cadence のみ `LLGLSLShader*` 経由、他 cadence は `block_hash` + (`Asset*` / `Skin*`) で識別 |
| **protocol-B** | shader-side UBO block 名称規約: GLSL `layout(set=N, binding=M) uniform <UBO_NAME>` の `<UBO_NAME>` = INDEX.md 記載 UBO 名 1:1 mapping | **整合 NG** | `ubo_metadata.inl:25-120` literal で **set=3 binding=0/1/2 が 6 UBO 衝突宣言**、実 pipeline layout の set=3 = 3 binding (Asset+Skin) のみ、Legacy UBO の set/binding 宣言と実 layout に乖離 |
| **protocol-C** | set=3 binding 衝突 3 site 解消方針: (i) 新規 binding allocation / (ii) program 識別 runtime dispatch / (iii) 設計再考 (衝突 UBO 統合) **[要 AYA 判断]** | **未確定** | 現状 Legacy UBO untouched ゆえ paper concern、本実装化時に顕在化、AYA literal 判断要 (= §6 確認 candidate 1) |
| **protocol-D** | 既存 pilot 通電 UBO logic 整合: Skin_GLTFJoints / PerDrawUBO_LightParams で既存 dispatch がどう動作しているか verify | **整合 OK** | §3 で 5 pilot UBO 全件 (Skin_GLTFJoints / Asset_GLTFMaterials / Asset_GLTFNodes / PerDrawUBO_LightParams / Global_ReflectionProbes) の dispatch logic 実装位置 + 動作 logic 確定 |

### §5.1 整合 OK 部分の含意

protocol-A + D 整合 OK:
- host C++ dispatch logic (= 6 cadence × writeXxxUbo)、5 pilot UBO 通電実績、name-based dispatch (= block_hash 経由) は **既に確立済**
- L0-1 protocol の本質 = ubo_metadata.inl + forwardToUboUpload + writeXxxUbo + (sCurrentAsset / sCurrentSkin) bridge は **新規実装不要**

### §5.2 整合 NG 部分の含意 (= protocol-B)

protocol-B 整合 NG:
- ubo_metadata.inl literal の set/binding 値は **Legacy UBO で実 pipeline layout と不一致**
- 「INDEX.md 記載 UBO 名 1:1 mapping」は **block 名 → block_hash mapping のみ正しい**、set/binding mapping は要再起案
- ⇒ **L0-1 protocol-B 再起案要素**:
  - shader 側 GLSL `layout(set=N, binding=M)` 宣言の N/M を Legacy UBO で実 pipeline layout (= set=1a/1b binding=0..39) に合致させる
  - ubo_metadata.inl の Legacy UBO 全件 (88 件 PerProgram cluster) で set/binding 値を **set=3 → set=1a or set=1b** に書き換える必要
  - codegen pipeline (= `scripts/ubo_codegen/perfect_hash.py`) の set/binding 割当 logic も連動修正要

### §5.3 未確定部分の含意 (= protocol-C)

protocol-C は本質的に L0-1 protocol-B の解決 strategy の一部として AYA literal 判断要:
- (i) 新規 binding allocation = Legacy UBO 88 件を set=1a/1b に再配置 (= V1' split 40+40 既存配置への流入)、binding 衝突解消
- (ii) program 識別 runtime dispatch = 同 binding 共存 → **Vulkan 仕様上不可** (= 同 DescriptorSetLayout 内 binding 番号 unique 必須)
- (iii) 設計再考 (衝突 UBO 統合) = AtmoExtra と Asset_GLTFNodes 等の data 統合 → **cadence 不一致 (PerProgram vs PerAsset) ゆえ不可**

⇒ **推奨案 = (i)** (= 新規 binding allocation): 既設 V3A_PROGRAM_SET_A_BINDINGS=40 + V3A_PROGRAM_SET_B_BINDINGS=40 = 計 80 binding 確保済、PerProgram cluster 88 件のうち 80 件は割当可能、残 8 件は追加 binding allocation 要 (= verify 要)。

---

## §6. AYA literal 確認 candidate 全件 (= entry handoff §4.7 stage 1)

### 確認 1: L0-1 protocol-C 採用案 (= AYA review candidate)

| 案 | 内容 | Vulkan 仕様適合 | 推奨度 |
|---|---|---|---|
| (i) | 新規 binding allocation = Legacy UBO 88 件を set=1a/1b に再配置 | ✅ 適合 | **推奨** |
| (ii) | 同 binding 共存 (= program 別 dispatch) | ❌ 不可 (= 同 DescriptorSetLayout 内 binding unique 必須) | reject |
| (iii) | 設計再考 (= 衝突 UBO 統合) | ❌ 不可 (= cadence 不一致) | reject |

**Claude 推奨**: 案 (i) 採用、Legacy UBO 88 件の set/binding 値を **set=1a/1b 内 binding=0..39 + 必要時 set 拡張** に再配置。実 pipeline layout (= V3A_PROGRAM_SET_A/B_BINDINGS=40 + 40) は既設、binding 値の codegen 側 mapping 修正で対応可能。

**AYA literal 確認要請**:
- 案 (i) 採用で OK か?
- 案 (i) 実装 timing = L0-1.C 実装内で codegen pipeline + ubo_metadata.inl 連動修正で対応するか?
- 残 8 件 (= 88 - 80) の追加 binding allocation strategy は L0-1.C 内で詳細化するか、L0-3 (per-shader UBO block 拡大) protocol に統合するか?

### 確認 2: L0-1 protocol 整合判定 (= sub-session 2 結論パターン判断)

| pattern | 含意 |
|---|---|
| (A) sub-session 3 (= L0-1.C 実装) 着手 OK | protocol-A + D 整合 OK ゆえ実装 unblocking、protocol-B 整合 NG 部分は L0-1.C 内で codegen 再起案で解決、protocol-C は確認 1 採用案 (i) で進行 |
| (B) sub-session 1 (= L0-1.A 再精査) 戻り | protocol-B 整合 NG が重大判断、設計再起案要 (= WORK_ORDER §2.1 部分書き直し、ubo_metadata.inl 全 88 件 PerProgram cluster の set/binding 配置を再設計) |
| (C) protocol 設計大幅変更 | L0-1 4 protocol 自体の枠組み再交渉 (= memory `project_r41_phase2_4_principles` 原則 3 再交渉) |

**Claude 推奨**: pattern (A) 着手 OK。

理由:
- protocol-A + D 整合 OK = 既存 pilot 5 UBO 通電で実証済、host dispatch logic 新規実装不要
- protocol-B 整合 NG = ubo_metadata.inl set/binding 値の修正で対応可能 (= codegen pipeline 内 mapping 修正、shader 側 layout 宣言の自動連動)、設計 thesis 自体は破綻していない
- protocol-C 採用案 (i) で binding 衝突解消可能、Vulkan 仕様適合 confirmed

**AYA literal 確認要請**: pattern (A) 着手 OK か?

### 確認 3: sub-session 2 trace 範囲補足 (= verify 要箇所)

本 sub-session 2 で確定できなかった項目 (= memory `feedback_admit_unknown` 遵守):
1. **shader compile 時の set=3 binding 衝突実際の挙動**: Legacy UBO `layout(set=3, binding=0) uniform AtmoExtraUBO_Legacy` を mUseUBO=true で compile した時の SPIR-V validation 結果 = sub-session 2 範囲外、L0-1.C 実装 sub-session で実機 verify 要
2. **88 件 PerProgram cluster の set=1a/1b 残 8 件追加 binding allocation 詳細**: 確認 1 残課題、L0-1.C 実装内 or L0-3 統合で対応
3. **ubo_metadata.inl の `subset` 列 (= 全件 0u)**: subset 値の dispatch 経路使用有無確認 = L0-1.C 実装内で grep verify

**AYA literal 確認要請**: 上記 3 件を L0-1.C 実装 sub-session 内 verify 持越で OK か?

---

## §7. sub-session 2 Exit 条件 (= entry handoff §4.4 cross-ref)

| # | Exit 項目 | 判定基準 | 状態 |
|---|---|---|---|
| 1 | 既存 pilot 通電 5 UBO の dispatch logic trace 完了 | 全 5 件で実装位置 + 動作 logic 確定 | ✅ (= §3 全 5 件) |
| 2 | dispatch trace doc 起案完了 | §4.3 entry handoff 内容反映 | ✅ (= 本 doc) |
| 3 | L0-1 protocol 整合判定明示 | 整合 OK / NG いずれか | ✅ (= §5: A+D OK / B NG / C 未確定) |
| 4 | AYA literal 確認受領 | sub-session 3 着手承認 / 設計再起案指示 | ⏳ AYA literal 待ち |

---

## §8. sub-session 2 後の sub-session 順序確認 (= entry handoff §2.2 + uncertainty audit §5 cross-ref)

| # | sub-session | scope | 着手契機 |
|---|---|---|---|
| 3 | **L0-1.C 実装** | name-based dispatch logic 実装 = ubo_metadata.inl set/binding 値修正 + codegen pipeline 連動修正 + shader layout 宣言整合化 + cold launch validation | 本 sub-session 2 Exit (4) AYA literal「sub-session 3 着手 OK + protocol-C 採用案 (i)」受領 |
| 4 | L0-2.A 再精査 | LLStaticHashedString redirect 関連 doc cold read | sub-session 3 Exit 後 |
| 5-12 | (entry handoff §2.2 cross-ref) | 同 cycle 繰返し | 各 sub-session Exit 後 |

**手戻り protocol** (= entry handoff §2.3 cross-ref):
- B → A = 本 sub-session 2 = trace 結果が protocol 案と不整合 → 設計再起案 = **本 doc §5: protocol-B 整合 NG は部分手戻り (= L0-1.C 実装内で codegen 修正で対応可能ゆえ A 戻り不要、Claude 判断、AYA literal 確認要)**

---

## §A. 本 doc の起案規律

- `indra/` 改変ゼロ (= sub-session B 規律、本 doc 起案で `build-linux-x86_64/codegen/ubo/ubo_metadata.inl` は generated file 直接 Read のみ、`indra/` 配下改変なし)
- 推論禁止、不明明示 (= memory `feedback_admit_unknown`)
- 整合 OK / NG / 未確定の 3 値で各 protocol 明示
- AYA literal 確認 candidate 3 件は省略せず提示 (= AYA literal「推奨案で OK」自走承認継続中でも、設計判断は明示確認継続)
- 推奨案明示 (= protocol-C 採用案 (i) + pattern (A) 着手 OK)、ただし AYA literal 判断は強制しない

---

## §B. 関連 commit + doc

| 種別 | 内容 |
|---|---|
| 関連 doc (本 sub-session 前提) | `handoff/phase2/handoff-phase2-l0-entry.md` §4 (= 本 sub-session 2 仕様起源、commit `0bc409461d` 書き直し版) |
| 関連 doc (本 sub-session 入口) | `handoff/phase2/audit/handoff-phase2-l0-uncertainty-audit.md` (= sub-session 1 出力、commit `cecb55ceb1`、§3.4 N1 解消 = 本 doc) |
| 関連 doc (実装方針) | `design/ubo/WORK_ORDER.md` §2.1 L0-1 protocol-A/B/C/D 詳細 |
| 関連 doc (5 UBO design) | `design/ubo/{Skin_GLTFJoints, Asset_GLTFMaterials, Asset_GLTFNodes, PerDrawUBO_LightParams, Global_ReflectionProbes}.md` |
| 関連 doc (Legacy UBO design = 衝突 3 site) | `design/ubo/{AtmoExtraUBO_Legacy, SkyVParamUBO_Legacy, SkyFParamUBO_Legacy}.md` |
| 関連 source | `indra/llrender/llglslshader.cpp:94-99` cadence_tag literal + `:2139-2240` forwardToUboUpload + `:2273` bringupTestUBO |
| 関連 source | `indra/llrender/llvkloader.cpp:858-868` V3A_*_BINDINGS literal + `:2799-2882` createV3aDescriptorSetLayouts + `:5117-5306` flush 経路 + `:5440-6000` write 経路 |
| 関連 source | `build-linux-x86_64/codegen/ubo/ubo_metadata.inl:25-120` g_block_metadata + `:122-130` lookup_block + `:132-229` block_hash namespace |
| 関連 memory | `feedback_admit_unknown` / `feedback_doubt_self_first` / `feedback_handoff_minimal_pre_req_read` / `project_r41_phase2_4_principles` / `project_r41_phase1b_vulkan_host_gate` / `feedback_ubo_migration_one_at_a_time` / `feedback_proactive_risk_management` |
| 本 doc | sub-session 2 出力 doc、commit 候補 (= AYA literal Exit 承認後) |

---

## §C. 次 sub-session 開始時の AYA 確認

「上記 dispatch trace doc 確認、§6 AYA literal 確認 3 件への回答受領後、**Phase 2.L0 sub-session 3 = L0-1.C 実装 sub-session (= name-based dispatch logic 実装 = ubo_metadata.inl set/binding 値修正 + codegen pipeline 連動修正 + shader layout 宣言整合化 + cold launch validation、`indra/` 改変開始 = design-phase 規律解除)** で着手 OK か?」

**AYA literal 確認内容 (= §6 cross-ref)**:
1. L0-1 protocol-C 採用案 = (i) 新規 binding allocation で OK か?
2. sub-session 2 結論パターン = (A) sub-session 3 着手 OK で OK か?
3. trace 範囲補足 verify 3 件を L0-1.C 内持越で OK か?
