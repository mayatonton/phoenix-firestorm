# r41 UBO 全体設計 Chapter 06b: cadence 別 update site + dirty + flush + forwardToUboUpload interface

**起案日**: 2026-06-03
**位置付け**: chapter 06 (redirect-layer-design) を 3 sub-chapter に分割した第 2 部。**5 cadence 別 update site の C++ 配置点 + dirty 判定機構 (= 既存 `mValue` cache を Vulkan UBO upload 側に乗せ替え + Material* per-draw 統合) + flush timing 論理仕様 + `forwardToUboUpload(loc, data, size)` interface 詳細** を確定する。descriptor set bind 配線 (= 06c) / 実 Vulkan API (= chapter 07) は別 chapter で扱う。
**pre-requisite**:
- `01-overview.md` §2 (2 大設計原則) / §3 (用語) / §5 (確定事項 13 件)
- `02-naming-convention.md` §2 (命名規則 + cadence prefix)
- `03-cadence-classification.md` §2 (cadence 5 分類) / §4 (cadence 別 update site overview)
- `04-codegen-ubo.md` §5.3 (`UniformLocation` / `CadenceTag` enum 値域) / §6 (R3 path)
- `05-existing-inventory-link.md` §6 (MC1 = 旧 G1, per-draw + dirty 確定) / §7 (集約フロー)
- `06a-cache-structure-and-setter-redirect.md` §3 (cache 構造) / §5 (setter 分岐 + `forwardToUboUpload` 呼出位置)
- `ayastorm-r41-ubo-current-state-inventory.md` §1.1 (既存 OpenGL upload site) / §4.3 (`mValue` cache 現状)

---

## §0 本 chapter の scope

### §0.1 scope (= 本 chapter で確定するもの)

1. **5 cadence 別 update site の C++ 配置点** (= per-frame / per-program / per-draw / per-asset / per-skin 各 cadence で `forwardToUboUpload` を呼ぶ既存 / 新設関数の特定)
2. **dirty 判定機構** (= 既存 `mValue` cache を Vulkan UBO upload 側 dirty flag に乗せ替え + Material* per-draw cadence 統合の具体実装、chapter 05 §6 MC1 (= 旧 G1) の実体化)
3. **flush timing 論理仕様** (= upload → descriptor set bind → draw の順序保証、cadence 跨ぎ memory barrier の論理要件、Vulkan API 詳細は chapter 07)
4. **`forwardToUboUpload(loc, data, size)` interface 詳細** (= signature / cadence 別 routing / ring buffer 論点 (L) / thread 配線)

### §0.2 非 scope (= 06c / chapter 07 / chapter 05a 譲り)

- descriptor set bind タイミング詳細 (= set=0/1/2/3 帯 cadence 別 rebind / PSO compatibility) → **06c**
- `mUseUBO` flag の決定方法 (= shader 種別自動判定 vs runtime cvar) → **06c**
- 実 Vulkan buffer 生成 / VMA allocator / `vmaMapMemory` / `vkCmdUpdateBuffer` / `vkCmdPipelineBarrier` / triple-buffering 実装 / secondary cmdbuf 並列化 → **chapter 07**
- sampler 系 49 個 descriptor set 経由 binding → **chapter 07**
- bare uniform → UBO 集約表本体 (= 267 行集約 mapping) → **chapter 05a** (= bare-uniform-mapping 切出し doc、別 session)

---

## §1 入力契約

| 入力 source | 本 chapter での用途 |
|---|---|
| `04-codegen-ubo.md` §5.3 `UniformLocation` struct + `CadenceTag` enum | §5 `forwardToUboUpload` signature と switch routing の input |
| `04-codegen-ubo.md` §6.2 R3 path (`mUniform[index]` → `mUniformUBOLoc[index]`) | §2 各 cadence update site で setter 経由到達する path 前提 |
| `05-existing-inventory-link.md` §6 MC1 (= 旧 G1) per-material → per-draw + dirty | §3.3 Material* 統合実装の根拠 |
| `06a-cache-structure-and-setter-redirect.md` §3 `mUniformUBOLoc[index]` cache | §2 / §3 各 update site で値書込先となる cache lookup の前提 |
| `06a-cache-structure-and-setter-redirect.md` §5.2 setter 内 `forwardToUboUpload(loc, data, size)` 呼出位置 | §5 interface 本体実装が受ける契約点 |
| `ayastorm-r41-ubo-current-state-inventory.md` §1.1 既存 OpenGL upload site (`updateNodeData` / `updateTransforms` / `setUniforms`) | §2.4 / §2.5 per-asset / per-skin update site が既存関数を継承する根拠 |
| `ayastorm-r41-ubo-current-state-inventory.md` §4.3 `mValue` cache 現状 | §3.1 / §3.2 dirty 判定の乗せ替え対象 |

---

## §2 cadence 別 update site

各 cadence で確定する 4 軸:
- **更新契機**: いつ `forwardToUboUpload` が呼ばれるか
- **配置点**: どの C++ 関数で値が流れ込むか (= 既存関数継承 / 新設)
- **upload thread**: 現 phase の thread (= main thread) と将来 worker thread 化方針 (= 原則 2)
- **flush 単位**: 物理 buffer instance flush の発火点

### §2.1 per-frame cadence

| 項目 | 設計 |
|---|---|
| 更新契機 | frame loop 開始時 1 回 |
| 配置点 (= upload 駆動側) | 既存 `LLAppViewer::idle()` の `gPipeline.updateGL()` 直後相当位置 / 既存 `LLEnvironment::updateShaderUniforms(shader)` (= shader bind 後 path、inventory §2.1) — どちらも setter 経由で `mUniformUBOLoc[]` 直引き、本 chapter は **共通の入口 (= setter)** だけ確定 |
| 配置点 (= flush 駆動側) | frame loop 開始時 1 回、`LLPipeline::renderGeom()` 入口前段で `flushFrameUbos()` (新設) を呼ぶ |
| upload thread | 現 phase = main thread / 将来 phase = worker thread 先行 upload (= 原則 2、chapter 09 Phase Y) |
| flush 単位 | Frame* UBO 群 (= 3 個、§2 chapter 02 §3.1) を一括 flush |
| 対応 UBO | `FrameViewProj` / `FrameLights` / `FrameAtmosphere` |
| 注 | per-frame は frame 内で値が変化しない (= shader bind 数 × bind 時 dispatcher 経由でも同値) ため、frame 内 dirty bit はほぼ常に立つ → §4.2 で triple-buffering で frame 跨ぎ write-after-read を回避 |

### §2.2 per-program cadence

| 項目 | 設計 |
|---|---|
| 更新契機 | shader program bind 時 (= `LLGLSLShader::bind()`) + その後 `LLEnvironment::updateShaderUniforms(shader)` 経由の setter 投入時 |
| 配置点 (= upload 駆動側) | setter 経由のみ (= 06a §5 path 分岐、本 chapter は受け側) |
| 配置点 (= flush 駆動側) | `LLGLSLShader::bind()` 内、`mUniformUBOLoc` 構築済の前提で `flushProgramUbos(this)` を呼ぶ (= bind 直後 / descriptor set bind 直前) |
| upload thread | 現 phase = main thread / 将来 phase = program 単位 worker thread (= 原則 2、chapter 09 Phase Y+1) |
| flush 単位 | bound program に attach された Program_* UBO 群 (= chapter 04 §6.2 で shader link 時 pre-cache 済、bind 時に該当 set だけ flush) |
| 対応 UBO | `Program_*` (80 個、= set=2 帯 24 + set=3 帯 54 + 2 extra、chapter 02 §3.3 / §3.4。**注**: 24+54+2=80 の +2 は 06c §2.3 の「上記の重複なし」 2 entry に対応、Q22-NUM 解消 A' 反映で per-program set=2 23→24) |
| 注 | shader bind 数は典型 20-50 / frame、bind 時 dirty 立ってなければ skip = 大半は upload 走らない (= cache hit が主流、chapter 03 §4.2 ) |
| 注 (Q27-CONFL 反映 = 2026-06-03 ST-6) | per-program set=2 帯で V/F stage 同 binding=0 共存 5 UBO (= `PerDrawUBO_ClipPlane` F 側 + `PerDrawUBO_AvatarSkin` / `PerDrawUBO_ObjectSkin` / `PerDrawUBO_SkinnedVelocity` / `PerDrawUBO_AvatarVelocity` V 側 4 件) は **A1+B2+C1 確定** (= chapter 10 §1.5 ST-3 batch verdict) = V 側 4 UBO を `set=2, binding=1/2/3/4` に振り直し + F 側 ClipPlane を `binding=0` 維持、Phase 1.A 入口 (= C1) で LL_VULKAN_GLSL 有効化前に全件解消。本 §2.2 per-program flush で Program_* 80 個 (= 上記 24 + 54 + 2 extra) は B2 binding ずらし後 layout を前提とする。具体 binding 確定 reflect 先 = chapter 06c §3 接合表 + chapter 04 §5 `ubo_metadata.inl` 出力契約。 |

### §2.3 per-draw cadence (= Material* 含)

| 項目 | 設計 |
|---|---|
| 更新契機 | draw call ごと + Material* は material 切替時 (= MC1 (= 旧 G1) 統合、§3.3) |
| 配置点 (= upload 駆動側) | `LLDrawPool*::render()` / `renderGeom*` 系の draw call 直前 setter (= 既存 path) |
| 配置点 (= flush 駆動側) | draw call 直前、`LLDrawPool*::renderItem()` 等の最深 dispatcher 入口で `flushDrawUbos()` を呼ぶ |
| upload thread | 現 phase = main thread / 将来 phase = secondary cmdbuf で draw call 単位分散 (= 原則 2、chapter 09 Phase Y+2) |
| flush 単位 | ring buffer 1 つにつき current draw slot 単位 (= §5.3 L 論点) |
| 対応 UBO | `Draw_LightParams` / `Draw_MultiLight` + `Material*` (MC1 (= 旧 G1) で per-draw cadence 帯入り) |
| 注 | 数百〜数千 / frame の upload を支える ring buffer / dynamic offset 設計が必須 (= §5.3 L 論点)、dirty hit 率は material 切替頻度次第 (= §3.3) |
| 注 (R-MAT1-4 反映 = 2026-06-03 ST-6) | Phase 0 計測 (= 06a-prep §5.5.7 観察値 437-688 cpf) で **per-draw cadence 確定 4 件** = `modelview_matrix` / `inv_modelview` / `modelview_projection_matrix` / `normal_matrix` (= LLShaderMgr canonical "matrix state" reserved uniforms `llshadermgr.cpp:1505-1512` の 7 件中 4 件、残 3 件 = `projection_matrix` / `inv_proj` / `identity_matrix` は per-frame / per-program 帯)。本 §2.3 per-draw cadence flush で `Draw_*` + `Material*` と並列に matrix 系 4 件も per-draw 帯 upload に乗る (= R-MAT4 = `normal_matrix` は 172 occurrences で per-draw 確定、当初 R-MAT4 候補 `modelview_projection_inverse` は grep 0 件で撤回 = 2026-06-03 chapter 10 §2.7 ST-6 前段 (b))。具体 binding 確定 reflect 先 = chapter 10 §2.7 + chapter 05 §7.3.4。 |
| 注 (R-AYA3 反映 = 2026-06-03 ST-6) | Phase 0 grep で R-AYA3 (= `aya_sss_skin_flag`) は **alive 確定** (= reserved enum 登録 + setter 4 site + shader 既使用 + Vulkan UBO `AvatarFParamUBO_Legacy` 既移植済 dual-path、chapter 10 §2.7 ST-6 前段 (a))。R-AYA1 (= `aya_alpha_plate`) / R-AYA2 (= `aya_alpha_plate_enabled`) は **dead 確定** (= grep 0 件、setter site 不在)、追記候補から除外。R-AYA3 cadence = per-draw (= avatar 描画毎切替 flag、`AvatarFParamUBO_Legacy` set=3 binding=54 経由)。 |

### §2.4 per-asset cadence

| 項目 | 設計 |
|---|---|
| 更新契機 | GLTF asset state 変化時 (= node transform / material 変化、既存 path) |
| 配置点 (= upload 駆動側) | **既存関数継承**: `gltf::Asset::updateNodeData()` (`gltf/asset.cpp:183` 付近) / `Asset::updateMaterialData()` (`gltf/asset.cpp:232` 付近) — inventory §1.1 で確認済の OpenGL path 関数を Vulkan path で `forwardToUboUpload` 経路に置換 |
| 配置点 (= flush 駆動側) | 既存 `GLTFSceneManager::render(variant)` (`gltfscenemanager.cpp:693/696`) 直前で `flushAssetUbos(asset)` を呼ぶ — asset draw 順序に同期 |
| upload thread | 現 phase = main thread (= 既存 OpenGL path と同位置) / 将来 phase = per-Asset worker thread (= owner 単位並列、原則 2) |
| flush 単位 | per-Asset owner (= `gltf::Asset::mNodesUBO` / `mMaterialsUBO` の 2 物理 instance を asset 単位で flush) |
| 対応 UBO | `Asset_GLTFNodes` (= `UB_ASSET_GLTF_NODES`) / `Asset_GLTFMaterials` (= `UB_ASSET_GLTF_MATERIALS`) |
| 注 | 既存 OpenGL path との差 = `glBufferData` + `glBindBufferBase` 直呼びを **Vulkan VMA mapped write + descriptor set update** に乗せ替えるのみ (= 06c / chapter 07)。call site は無改変 (= 原則 1) |

### §2.5 per-skin cadence

| 項目 | 設計 |
|---|---|
| 更新契機 | rigged animation 毎 frame (= joint palette 計算後) |
| 配置点 (= upload 駆動側) | **既存関数継承**: `gltf::Skin::updateTransforms()` (`gltf/animation.cpp:411` 付近) — inventory §1.1 確認済 |
| 配置点 (= flush 駆動側) | 既存 `GLTFSceneManager::render(variant)` (`gltfscenemanager.cpp:736`) 直前で `flushSkinUbos(skin)` を呼ぶ — rigged draw 順序に同期 |
| upload thread | 現 phase = main thread / 将来 phase = per-Skin worker thread (= rigged animation CPU 計算 + 直接 upload、原則 2 の典型例) |
| flush 単位 | per-Skin owner (= `gltf::Skin::mUBO` の 1 物理 instance を skin 単位で flush) |
| 対応 UBO | `Skin_GLTFJoints` (= `UB_SKIN_GLTF_JOINTS`) |
| 注 | matrix palette は rigged GLTF avatar 数次第で物理 instance 数 M が決まる (= inventory §1)、大規模 sim では M = 10-50。worker thread 化で per-Skin 並列計算 → 直接 upload が原則 2 の本丸 |

---

## §3 dirty 判定機構

### §3.1 既存 `mValue` cache の現状 (= 乗せ替え元)

inventory §4.3 / 06a §5.2 で確認済:

| 項目 | 現状 |
|---|---|
| 構造 | `LLGLSLShader::mValue` (= `std::unordered_map<U32 index, LLVector4>`) |
| 機能 | setter 入口で前回値と比較、同値なら GL call (= `glUniform*`) skip |
| 適用範囲 | scalar / vec2 / vec3 / vec4 (= LLVector4 で表現可能な型のみ) |
| 適用外 | `uniform4iv` / `uniformMatrix2/3/3x4/4fv` (= 5 method、§3.4 (U4) 持越) |
| 目的 | GL API call 数削減 (= OpenGL 側の最適化、UBO upload とは独立) |

### §3.2 Vulkan UBO upload 側 dirty flag への乗せ替え方針

#### §3.2.1 二段階 dedup 構造

```
setter call
  ↓
[ stage 1: mValue cache check (= 既存、値 dedup) ]
  ↓ cache hit → return (= upload 経路に入らない)
  ↓ cache miss → mValue 更新
  ↓
[ stage 2: mUniformUBOLoc[index] 直引き → forwardToUboUpload ]
  ↓
[ stage 3: UBO physical instance dirty bit set (= 新設、upload dedup) ]
  ↓
[ flush 時: dirty bit check → upload → clear ]
```

= **stage 1 (値 dedup) + stage 3 (upload dedup) の二段階**。stage 1 で setter 入口の通常 path を、stage 3 で flush 時の物理 upload を、それぞれ独立に間引く。

#### §3.2.2 既存 `mValue` cache の温存

- 06a §5.2 で確認済: cache check は `if (mUseUBO)` 分岐の **前** = OpenGL/Vulkan path 共通で stage 1 を経由
- = `mValue` cache の温存は call site 改変ゼロ (= 原則 1)
- = 既存 OpenGL path 挙動 (= GL call dedup) を Vulkan path でも同じ意味論で継承

#### §3.2.3 Vulkan 側 dirty flag の配置

**⚠ 重要 (= 設計 review 2026-06-03 §3.4 修正)**: 下記 code shape は **K2 (UBO 単位 dirty、§3.4) を default 採用した前提形** であり、(K) は chapter 10 持越で AYA 判断待ち。AYA が **K1 / K3 を選択した場合** は本 §3.2.3 code shape を以下のように差し替える:
- **K1 採用時**: `UboInstance::dirty` を `std::atomic<bool>` 単一値ではなく **`std::vector<std::atomic<uint64_t>>` (= member 数 / 64 の bitset)** に変更、`forwardToUboUpload` で member offset 単位の bit set、flush 関数は dirty bit 範囲を partial upload に翻訳 (= chapter 07 で `vkCmdUpdateBuffer` 等)
- **K3 採用時**: `UboInstance::dirty` は廃し、cadence 単位の **`std::array<std::atomic<bool>, 5>`** を upload ownership に追加、§5.2 routing は cadence index 単位で dirty set
- いずれの場合も §5.2 routing の 5 case 分岐構造と §4.1 flush 関数の cadence 別呼出順序 (= 本 chapter 確定事項) は不変

dirty flag は **UBO physical instance 単位** で持つ (= §3.4 K 論点で Claude 推奨 = K2):

```cpp
// chapter 07 で確定する UboInstance owner の概念形 (本 chapter は論理仕様のみ)
namespace ubo {
struct UboInstance {
    VkBuffer        vk_buffer;       // chapter 07 で確定
    void*           mapped_ptr;      // VMA persistent map、chapter 07
    uint32_t        size;            // ubo_metadata.inl から
    std::atomic<bool> dirty{false};  // §3.2 stage 3、本 chapter で確定
};
} // namespace ubo
```

- `dirty` は `std::atomic<bool>` (= 将来 worker thread からの set / flush thread からの check が並走、§5.4 thread 配線)
- `forwardToUboUpload` 内で member offset 書込時に `dirty.store(true, std::memory_order_release)` を立てる
- flush 関数 (= §4) で `dirty.exchange(false, std::memory_order_acq_rel)` で true 時のみ upload 実行

### §3.3 Material* per-draw cadence 統合 (= MC1 (= 旧 G1) 確定の実装)

#### §3.3.1 chapter 01 §5 #12 / chapter 05 §6 MC1 (= 旧 G1) の再掲

- per-material cadence は **per-draw + dirty flag に統合** = 独立軸として保持しない
- `Material*` UBO は per-draw cadence で扱い、material 切替を per-draw dirty flag で吸収

#### §3.3.2 本 chapter での具体実装

- `Material*` UBO は §2.3 per-draw cadence の update site / flush 単位に完全に乗る
- material 切替検知 = setter `uniform4fv("diffuse_color", ...)` 等の `mValue` cache miss (= 値が前回 draw と異なる)
- 連続同 material draw = `mValue` cache hit = stage 1 で return = `Material*` UBO の dirty bit は立たない = upload skip
- material 切替直後 1 回 = cache miss → `forwardToUboUpload` → `Material*` UBO dirty bit set → 該当 draw 直前 flush で upload
- → 既存 OpenGL path での「material 切替時のみ GL call、連続同 material は dedup で skip」挙動を Vulkan UBO 側でも同じ意味論で再現

#### §3.3.3 命名と cadence の独立性 (= chapter 02 §2.2 / chapter 03 §2.2 の再確認)

- `Material*` prefix (= 命名) は **既存命名温存** (= upstream 取込互換、原則 1)
- cadence 軸 = per-draw (= MC1、= 旧 G1)
- → cadence 表 (chapter 03 §2) は 5 分類で確定、`Material*` は per-draw 行の `Draw_*` と並列 prefix として共存

#### §3.3.4 Q26-MUL 確定名 反映 (= 2026-06-03 ST-6 / chapter 10 §1.5)

- `MaterialUBO` (= class3 path 主流名) / `MaterialUBO_Class3_Legacy` (= legacy class3 fallback、Q26-MUL `A1+B1` 確定 = chapter 02 §3.2 の MUL-A1/B1 規律 paragraph 反映済)
- 旧 `MaterialUBO_Legacy` → **`MaterialUBO_Class3_Legacy` に改名確定** = AYA 「全 default 採用」 batch verdict 反映 (= chapter 10 §1.0 Q26-MUL 状態 ✅)
- 本 §3.3 per-draw cadence 統合は `MaterialUBO` / `MaterialUBO_Class3_Legacy` の **両 prefix** が `Material*` 集合に属する前提で扱う = §2.3 per-draw flush で `mValue` cache miss → `forwardToUboUpload` → UBO 単位 dirty bit set の path 共通
- = 命名差は cadence / dirty 機構には影響しない (= §3.3.3 命名と cadence の独立性の具体例)

### §3.4 dirty 判定粒度の論点 (= K)

| # | 案 | dirty bit 個数 | upload 粒度 | overhead |
|---|---|---|---|---|
| K1 | **member 単位** | UBO member 数 (= 数千〜数万 / cadence) | UBO 内 partial write (= `vkCmdUpdateBuffer` で部分書込) | bit 管理 + partial write overhead |
| K2 | **UBO 単位** (= Claude 推奨) | UBO physical instance 数 (= scene 規模で 88 + N + M、§1) | UBO 全体 upload (= mapped memory に 1 block memcpy) | bit 1 個 / UBO、upload は std140 size 上限 (典型 < 1KB) |
| K3 | **cadence 単位** | 5 (= cadence 分類数) | cadence 内 全 UBO upload | bit 5 個、upload waste 大 (= 同 cadence 内変化無 UBO も上がる) |

#### Claude 推奨 = K2 (UBO 単位)

根拠:
1. **K1 partial write overhead**: Vulkan `vkCmdUpdateBuffer` / VMA mapped memory write は **小規模 partial write の overhead が UBO 全体 memcpy より高い** (= std140 で UBO size は典型 < 1KB、L1 cache 内 memcpy が走る規模)。member 単位 dirty bit 管理の bit op cost も加味すると K1 は劣勢
2. **K3 cadence 単位 upload waste**: per-program cadence 内 79 UBO のうち bind されている program 分 (= 20-50) しか upload 必要なし、K3 は残 30-60 UBO の空 upload が発生
3. **K2 は ring buffer / dynamic offset との相性**: per-draw cadence で ring buffer に UBO 単位 sub-allocation する設計 (= §5.3 L1+L2 推奨) と K2 (= UBO 単位 dirty) が自然に対応

→ **(K)** chapter 10 持越、AYA 判断仰ぐ。K2 が default 採用、K1 / K3 は反証材料が出たら再評価。

---

## §4 flush timing 論理仕様

### §4.1 cadence 別 flush 順序

| cadence | flush 駆動関数 (新設名) | 駆動位置 | flush 後 |
|---|---|---|---|
| per-frame | `flushFrameUbos()` | frame loop 開始 1 回 (= `LLPipeline::renderGeom()` 入口前) | frame 内全 draw で同 descriptor set (= 06c set=0 stable) |
| per-program | `flushProgramUbos(LLGLSLShader*)` | `LLGLSLShader::bind()` 内 (= bind 直後 / descriptor set bind 直前) | program 切替まで同 descriptor set (= 06c set=1) |
| per-draw | `flushDrawUbos()` | draw call 直前 (= `LLDrawPool::renderItem()` 等の最深 dispatcher 入口) | dirty 時のみ upload、bind 後即 draw |
| per-asset | `flushAssetUbos(gltf::Asset*)` | `GLTFSceneManager::render(variant)` 直前 (`gltfscenemanager.cpp:693/696`) | asset 関連 draw 群中は同 descriptor set |
| per-skin | `flushSkinUbos(gltf::Skin*)` | `GLTFSceneManager::render(variant)` 直前 (`gltfscenemanager.cpp:736`) | rigged draw 中は同 descriptor set |

### §4.2 upload → descriptor set bind → draw の順序保証

論理仕様 (= Vulkan API 詳細は chapter 07 / 06c):

1. **cadence の update site での `forwardToUboUpload` 呼出** = mapped memory への memcpy (= host write)
2. **cadence の flush 駆動関数 呼出** = dirty bit check → 該当 UBO physical instance の Vulkan upload 命令 record (= chapter 07)
3. **descriptor set bind** = §4.1 flush 直後、cadence 帯の descriptor set rebind (= 06c)
4. **draw call** = descriptor set bind 直後、draw command record

= **必ず (1) → (2) → (3) → (4) の順**。本 chapter は論理仕様までを確定、Vulkan `vkCmdPipelineBarrier` / `VK_ACCESS_UNIFORM_READ_BIT` 等の API 詳細は chapter 07 配線。

### §4.3 frame 跨ぎ write-after-read 対策 (= triple-buffering 論理仕様)

per-frame UBO は **frame N の draw が GPU で読み終わる前に frame N+1 の host write を始める** = write-after-read hazard:

| 対策 | 論理仕様 |
|---|---|
| triple-buffering | per-frame UBO physical instance を **N 個** (= frame in flight 数 + α) rotate して持つ。frame N は instance idx (N % N_buf) に write、frame N-2 は GPU 読込完了済 |
| buffer 個数 | N_buf = 2 (= double) / 3 (= triple) のいずれか、(U1) chapter 07 持越 |
| per-program / per-draw | shader bind 周期 / draw 周期で write が走るが、descriptor set bind が即 draw record と相対安全。chapter 07 で詳細 |

### §4.4 cadence 跨ぎ memory barrier の論理要件

- write-after-write (= 同 buffer の per-draw 連続 upload): ring buffer / dynamic offset で **同 buffer 領域への上書きを避ける** = barrier 不要 (= §5.3 L1+L2 推奨設計の根拠)
- read-after-write (= host write → GPU read): Vulkan host-coherent memory (= VMA `VMA_MEMORY_USAGE_CPU_TO_GPU`) で host write completion は GPU side automatic visibility、明示 barrier 不要 (= chapter 07)
- write-after-read (= §4.3): triple-buffering で回避

= **本 chapter は barrier 不要設計を前提**、Vulkan API での実装裏付けは chapter 07。

---

## §5 forwardToUboUpload interface 詳細

### §5.1 signature

```cpp
// chapter 06b で確定する interface (= chapter 07 で実体実装)
namespace ubo {
void forwardToUboUpload(const UniformLocation& loc, const void* data, size_t size);
}
```

引数:
- `loc.block_hash` = UBO physical instance 識別 hash (= chapter 04 §5.3.2 perfect hash table から取得)
- `loc.offset` = block 内 std140 offset
- `loc.size` = member 値 size (= setter write 量)
- `loc.cadence_tag` = §5.2 routing 先決定
- `data` = setter 引数の値 pointer
- `size` = `loc.size` と等しい (= debug assert で `llassert(size == loc.size)`)

### §5.2 cadence 別 routing

```cpp
void forwardToUboUpload(const UniformLocation& loc, const void* data, size_t size)
{
    llassert(size == loc.size);

    switch (loc.cadence_tag) {
        case CADENCE_PER_FRAME: {
            // Frame* UBO の current frame slot に memcpy + dirty bit set
            UboInstance* ubo = getFrameUboInstance(loc.block_hash);
            std::memcpy(static_cast<char*>(ubo->mapped_ptr) + loc.offset, data, size);
            ubo->dirty.store(true, std::memory_order_release);
            break;
        }
        case CADENCE_PER_PROGRAM: {
            // bound program の per-program slot に memcpy + dirty bit set
            UboInstance* ubo = getCurrentProgramUboSlot(loc.block_hash);
            std::memcpy(static_cast<char*>(ubo->mapped_ptr) + loc.offset, data, size);
            ubo->dirty.store(true, std::memory_order_release);
            break;
        }
        case CADENCE_PER_DRAW: {
            // ring buffer の current draw slot に memcpy + dirty bit set
            UboInstance* ubo = getDrawUboRingSlot(loc.block_hash);
            std::memcpy(static_cast<char*>(ubo->mapped_ptr) + loc.offset, data, size);
            ubo->dirty.store(true, std::memory_order_release);
            break;
        }
        case CADENCE_PER_ASSET: {
            // current asset owner の mNodesUBO / mMaterialsUBO に memcpy + dirty
            UboInstance* ubo = getCurrentAssetUbo(loc.block_hash);
            std::memcpy(static_cast<char*>(ubo->mapped_ptr) + loc.offset, data, size);
            ubo->dirty.store(true, std::memory_order_release);
            break;
        }
        case CADENCE_PER_SKIN: {
            // current skin owner の mUBO に memcpy + dirty
            UboInstance* ubo = getCurrentSkinUbo(loc.block_hash);
            std::memcpy(static_cast<char*>(ubo->mapped_ptr) + loc.offset, data, size);
            ubo->dirty.store(true, std::memory_order_release);
            break;
        }
        case CADENCE_SAMPLER:
        case CADENCE_INVALID:
        case CADENCE_UNKNOWN:
            // setter 内で既に early-out 済 (06a §5.2)、ここに来たら設計違反
            llassert(false && "forwardToUboUpload reached with non-UBO cadence");
            break;
    }
}
```

**規律**:
- routing 5 cadence × 1 default skip = 6 branch、cadence_tag の値域を完全網羅
- `getXxxUboInstance(block_hash)` helper は cadence 別 owner table を引く (= §5.4 thread-safe 要件あり)
- mapped memory write は VMA persistent map 前提 (= chapter 07)

### §5.3 per-draw cadence の Vulkan 最適化 (= L 論点)

per-draw cadence は数百〜数千 / frame の upload が走るため、physical buffer 配置戦略が支配的:

| # | 案 | 物理 buffer 構造 | descriptor set 更新 | overhead |
|---|---|---|---|---|
| L1 | **ring buffer** (= 大容量 1 buffer に draw 毎 offset 進行) | per-draw UBO 1 つにつき N MB の ring buffer 1 個 | offset 経由参照 (= 後述 L2 と組合せ前提) | allocator 軽い、buffer 数少ない |
| L2 | **dynamic offset** (= `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` + draw 毎 offset 変更) | L1 と同 | 1 descriptor set + draw 毎 offset 数値変更 (= rewrite 不要) | descriptor set bind cost 最小 |
| L3 | **sub-allocation** (= 各 draw に小 buffer を allocate) | draw 毎 1 buffer | 描画毎 descriptor set 更新 | allocator + bind cost 大、却下 |

#### Claude 推奨 = L1 + L2 組合せ

根拠:
1. ring buffer (L1) で 1 物理 buffer に複数 draw の per-draw 値を **offset 進行** で並べる = physical instance 数を抑制
2. dynamic offset (L2) で draw 毎の bind を **offset 数値変更のみ** に縮約 = descriptor set rewrite ゼロ
3. L3 は draw 毎 buffer allocate で allocator pressure + descriptor set update が draw 数だけ走る = Vulkan 描画 hot path に不向き

→ **(L)** chapter 07 / 06c 持越 (= 実 buffer / dynamic offset 配線は chapter 07、bind タイミング は 06c)。L1+L2 が default 採用。

**⚠ 依存性明示 (= 設計 review 2026-06-03 §3.4 修正)**: 上記 §5.2 routing 内 `case CADENCE_PER_DRAW` の `getDrawUboRingSlot(loc.block_hash)` helper は **L1+L2 default 採用前提**の signature。AYA が **L3 (sub-allocation)** を選択した場合は同 case で `getDrawUboFreshBuffer(loc.block_hash)` 相当の helper に差し替え + descriptor set rewrite path (= 06c 側) も同時改修必須。本 chapter §5.2 の case 分岐構造自体は (L) 解消後も不変。

### §5.4 thread 配線

#### §5.4.1 現 phase = main thread 専有

- `forwardToUboUpload` 呼出は setter 経由 (= 06a §5.2 path 分岐内)、setter は main thread からのみ呼ばれる (= 既存 OpenGL path と同)
- mapped memory write + dirty bit set は main thread 連続実行、競合無し
- flush 関数 (= §4.1) も main thread で同期実行

#### §5.4.2 将来 phase = 原則 2 並列化対応

将来 phase で worker thread 化が走る (= 原則 2、chapter 09 Phase Y/Y+1/Y+2):

| 並列化対象 | thread-safe 要件 |
|---|---|
| per-frame upload を worker thread で先行 | host write は main thread と worker thread のいずれかから、`dirty.store(release)` で order 確保 |
| per-program 単位 worker thread | program owner が thread 跨ぐ場合、`getCurrentProgramUboSlot` が thread-local current program を引く必要 |
| per-asset / per-skin 単位 worker | owner table への concurrent access、`std::shared_mutex` で reader 並列 / writer 排他 |
| per-draw secondary cmdbuf | 各 secondary cmdbuf で独立 ring buffer sub-allocation (= chapter 07) |

#### §5.4.3 現 phase 実装の thread-safe 化方針

- `dirty` flag = `std::atomic<bool>` (= §3.2.3 既述、現 phase でも安全側設計)
- `getXxxUboInstance(block_hash)` helper の owner table = 現 phase は **non-mutex** (= main thread 専有前提)、将来 phase で `std::shared_mutex` 追加
- = **(M)** thread-safe 化方式の chapter 07 持越

**⚠ 依存性明示 (= 設計 review 2026-06-03 §3.4 修正)**: 上記 §5.4.3 の `getXxxUboInstance(block_hash)` owner table = 現 phase non-mutex は **「現 phase main thread 専有」前提**。AYA が **mutex (shared_mutex)** を選択した場合は helper 内で reader-writer lock 追加、**lock-free (= cmd buffer per-thread sub-allocation)** を選択した場合は owner table 自体を thread-local 化 (= chapter 07 secondary cmdbuf 設計と一体)。本 §5.4.3 の `dirty` `std::atomic<bool>` (= §3.2.3) は (M) 解消に依存せず安全側設計のため不変。

---

## §6 chapter 06c / chapter 07 / chapter 09 への bridge

| 譲り先 | 譲る内容 |
|---|---|
| **06c** (descriptor-set-bind-wiring) | flush 直後の descriptor set bind タイミング / cadence 帯 (set=0/1/2/3) ↔ UB_* 4 binding 接合 / `mUseUBO` flag 決定方法 / dynamic offset (L2) の bind side |
| **chapter 07** (vulkan-api-state) | 実 `UboInstance` 構造体実体化 / VMA `vmaCreateBuffer` + persistent map / `vkCmdUpdateBuffer` or memcpy + `vkFlushMappedMemoryRanges` 選択 / triple-buffering buffer 個数 (U1) / per-draw ring buffer 容量 / secondary cmdbuf 並列化 / `getXxxUboInstance` helper 実装 |
| **chapter 09** (phase-roadmap) | cadence 別 migration 順序 (= どの cadence から実装着手するか) / UBO 単位 1 つずつ実装 ↔ cadence 帯まとめて実装 のトレードオフ |
| **chapter 05a** (= bare-uniform-mapping 切出し doc) | bare uniform 267 個の cadence 列確定 (= Phase 0 (H1b) 結果反映) → 各 UBO の dirty hit 率実測の前提 |

---

## §7 chapter 04 / 05 / 06a / 06c との分担境界

| chapter | 本 chapter からの入力 | 本 chapter への出力 |
|---|---|---|
| **04** (codegen-ubo) | `UniformLocation` struct / `CadenceTag` enum / perfect hash table / `<BlockName>Layout` struct | (なし、本 chapter は受け側) |
| **05** (existing-inventory-link) | MC1 (= 旧 G1) per-material → per-draw + dirty 確定 (§6) / bare uniform 集約フロー | per-draw cadence dirty 実装で `Material*` 統合の具体形 (= §3.3) |
| **06a** (cache-structure-and-setter-redirect) | `mUniformUBOLoc[index]` cache / `mUniformUBOLocByHash` / `forwardToUboUpload(loc, data, size)` 呼出位置 / `mUseUBO` flag 配置 | `forwardToUboUpload` interface 本体実装 (§5) / cadence 別 routing / dirty 機構の二段階 dedup 構造 (§3.2) |
| **06c** (descriptor-set-bind-wiring) | (本 chapter は提供側) | flush 後の descriptor set bind タイミング契約 (= §4.1 flush 駆動関数 直後に bind が走る前提) / dynamic offset (L2) の bind 側責務 |

---

## §8 未確定事項 (→ chapter 10 持越 / AYA 判断仰ぎ)

| # | 項目 | 解消先 | default 採用案 |
|---|---|---|---|
| (K) | dirty 判定粒度 = K1 member 単位 / K2 UBO 単位 / K3 cadence 単位 | **chapter 10 / AYA 判断** | K2 (= UBO 単位、§3.4) |
| (L) | per-draw Vulkan 最適化 = L1 ring buffer / L2 dynamic offset / L3 sub-allocation | **chapter 07 / 06c** | L1 + L2 組合せ (= §5.3) |
| (M) | thread-safe 化方式 = mutex / atomic / lock-free | chapter 07 | 現 phase atomic (= §5.4.3) |
| **(M) ID 衝突注 (= 設計 review 2026-06-03 §3.1 ID rename 整合)** | 本 chapter §8 (M) = thread-safe 化方式 (本 chapter 06b 固有)。chapter 06c §10 でも別概念に (M) ID が使われていた歴史があり、chapter 06c 側のみ **(M) → (MD)** rename (= material domain) を実施。**本 chapter 06b の (M) ID は thread-safe を指すまま維持** (= 06b ↔ 06c の (M) ID は別概念で、文脈で識別) | — | — |
| (U1) | per-frame triple-buffering buffer 個数 = 2 (double) / 3 (triple) / N | chapter 07 | 3 (= triple-buffering 標準、§4.3) |
| (U2) | per-asset / per-skin dirty 判定 = 既存 owner state 変化検知継承 / Vulkan 側 dirty bit 追加 | chapter 07 | 既存 owner 変化検知 + Vulkan 側 dirty bit の両立 (= 既存 path 改変ゼロ + Vulkan upload dedup 両得) |
| (U3) | flush timing で per-program ↔ per-draw 境界 = bind 直後 upload vs draw 直前 upload | (H1b) hook 計測後再評価 | bind 直後 upload (= per-program 帯)、draw 直前 upload (= per-draw 帯) で分離 (= §4.1) |
| (U4) | `mValue` cache 適用外 5 method (= `uniform4iv` / `uniformMatrix2/3/3x4/4fv`) の Vulkan dirty 判定 | chapter 07 / Phase 進行中 | stage 1 を bypass、stage 3 dirty bit のみで dedup (= 値比較せず常に `forwardToUboUpload`、UBO 単位 dirty で flush dedup) |

### §8.1 確定 cross-ref (= 2026-06-03 ST-6 反映)

本 chapter §8 持越 (K)(L)(M)(U1)(U2)(U3)(U4) と独立に、chapter 10 batch verdict で確定済の関連項目を列挙 (= 本 chapter 設計の前提条件確認):

| Q-ID | 確定 verdict | 本 chapter での反映点 |
|---|---|---|
| **Q1** | A (= AYA 「全 default 採用」、chapter 09 §11.1) | §2.2 per-program flush 単位 / §5.2 routing `CADENCE_PER_PROGRAM` case の前提が確定 |
| **Q2** | A (= AYA 「全 default 採用」、chapter 09 §11.2) | §3.2 二段階 dedup 構造 / §5.4 thread 配線の前提が確定 |
| **Q4** | C (= AYA 「全 default 採用」、chapter 09 §11.4) | §3.4 K2 (= UBO 単位 dirty) Claude 推奨の前提が確定 (= 他案で覆らない場合 K2 採用) |
| **Q26-MUL** | A1+B1 (= chapter 10 §1.0、`MaterialUBO_Class3_Legacy` 改名) | §3.3.4 命名 reflection 反映済 |
| **Q27-CONFL** | A1+B2+C1 (= chapter 10 §1.5 ST-3 batch) | §2.2 注 Q27-CONFL paragraph 反映済 |
| **Q28-FFDUP** | A1+B2 (= chapter 10 §1.5 ST-3 batch、F+F `PerDrawUBO_ClipPlane` 重複解消) | (本 chapter 06b は cadence 設計、binding 重複は 06c 接合表で扱う) |
| **R-AYA1/2/3** | dead / dead / alive (= chapter 10 §2.7 ST-6 前段 (a) 確定) | §2.3 注 R-AYA3 反映 paragraph 反映済 |
| **R-MAT1-4** | per-draw cadence 4 件 (= `modelview_matrix` / `inv_modelview` / `modelview_projection_matrix` / `normal_matrix`、chapter 10 §2.7 / chapter 05 §7.3.4 ST-6 前段 (b)) | §2.3 注 R-MAT1-4 反映 paragraph 反映済 |

---

## §9 update 規律

- §2 update site の追加 / 変更は本 chapter に集約、06c / chapter 07 が後追い参照
- §3 dirty 機構の粒度 (= K1/K2/K3) 確定 (= chapter 10 で AYA 判断後) で §3.4 を解消マーク + §3.2.3 / §5.2 を確定形に書換え
- §4 flush timing の barrier 要件変更 (= chapter 07 確定後) は本 chapter §4.4 に集約反映
- §5 `forwardToUboUpload` signature 変更は本 chapter で確定、06a §5 / chapter 04 §6 が後追い参照
- §8 (K)(L)(M)(U1)(U2)(U3)(U4) 持越は他 chapter / Phase で消化したら本 chapter から「保留候補」を剥がす

---

**= 本 chapter で cadence 別 update site + dirty + flush + `forwardToUboUpload` interface が確定したため、06c (descriptor-set-bind-wiring) で set=0/1/2/3 帯 cadence 別 rebind + UB_* 4 binding vs 79 Program_* + 3 Frame* + Asset_* + Skin_* の接合 + `mUseUBO` flag 決定方法に進める**。
