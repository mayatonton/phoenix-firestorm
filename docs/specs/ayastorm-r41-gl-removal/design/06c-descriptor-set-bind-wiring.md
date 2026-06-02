# r41 UBO 全体設計 Chapter 06c: descriptor set bind 配線

**起案日**: 2026-06-03
**位置付け**: chapter 06 (redirect-layer-design) を 3 sub-chapter に分割した第 3 部。**descriptor set 4 帯 (set=0/1/2/3) の cadence 別配置 + 既存 UB_\* 4 binding ↔ 84 blueprint 接合表 + flush 直後 bind 配線 + dynamic offset (L2) bind 側責務 + `mUseUBO` initial 設定方針 + PSO compatibility / cadence 跨ぎ rebind 範囲 + triple-buffering 配下 per-frame set rotate + sampler 49 個 chapter 07 bridge** を確定する。実 Vulkan API 実装 (= chapter 07) は別 chapter で扱う。
**pre-requisite**:
- `01-overview.md` §2 (2 大設計原則) / §3 (用語) / §5 (確定事項 13 件)
- `02-naming-convention.md` §2.3.1 (UB_\* enum 命名) / §3 (84 UBO rename 表)
- `03-cadence-classification.md` §2 (cadence 5 分類) / §4 (cadence 別 update site overview)
- `04-codegen-ubo.md` §5 (生成物 = `ubo_metadata.inl` で block_name → (size, set, binding)) / §5.3 (`UniformLocation` / `CadenceTag`)
- `05-existing-inventory-link.md` §3 (cadence 別 mapping) / §4 (E3 rename 確定) / §6 (MC1 = 旧 G1 per-material → per-draw、設計 review 2026-06-03 §3.1 ID rename = material cadence prefix)
- `06a-cache-structure-and-setter-redirect.md` §3 (cache 構造) / §5.4 (`mUseUBO` flag 配置、初期化方針は本 chapter 持越)
- `06b-cadence-update-site-and-dirty.md` §4 (flush 関数 5 種) / §5.3 (L1+L2 推奨) / §8 ((K)/(L)/(U1) default 採用案)
- `ayastorm-r41-ubo-current-state-inventory.md` §1 (OpenGL 実働 UBO 4 種 + UB_\* enum) / §3 (84 GLSL blueprint)

---

## §0 本 chapter の scope

### §0.1 scope (= 本 chapter で確定するもの)

1. **descriptor set 4 帯 cadence 別配置** (= set=0 per-frame / set=1 per-program / set=2 per-draw / set=3 per-asset+per-skin+singleton)
2. **既存 UB_\* 4 binding ↔ 84 blueprint 接合表** (= upstream LL 4 種温存 + Frame\* 3 + Program_\* 79 + Draw_\* 2 + Material\* 2 + Asset_\* 2 + Skin_\* 1 + Global_\* 1 の最終配線)
3. **flush 直後 descriptor set bind 配線** (= 06b §4.1 flush 関数 5 種の直後 rebind 範囲)
4. **dynamic offset (L2) bind 側責務** (= 06b §5.3 L1+L2 default 採用の bind 側実装)
5. **`mUseUBO` flag initial 設定方針確定** (= 06a 持越 (Q1) 解消)
6. **PSO compatibility / cadence 跨ぎ rebind 範囲**
7. **triple-buffering (U1=3) 配下 per-frame set rotate 方式**
8. **sampler / opaque 49 個 descriptor set 経由 binding の配置 bridge** (= 配置のみ、実 binding は chapter 07)
9. **chapter 07 (vulkan-api-state) への bridge**

### §0.2 非 scope (= 他 chapter / 他 phase 譲り)

- 実 `vkCreateDescriptorPool` / `vkAllocateDescriptorSets` / `vkUpdateDescriptorSets` 実装 → **chapter 07**
- per-draw ring buffer 容量決定 / secondary cmdbuf 並列化 → **chapter 07**
- sampler 49 個 実 texture binding / image view 作成 → **chapter 07**
- Codegen 側の set/binding 値出力 (= `ubo_metadata.inl` build 詳細) → **chapter 04 §5.1 / chapter 08**
- shader 単位 migration 順序 → **chapter 09 (phase-roadmap)**
- bare uniform 267 個の集約先 UBO 確定 → **chapter 05a (= bare-uniform-mapping 切出し doc)**

---

## §1 入力契約

| 入力 source | 本 chapter での用途 |
|---|---|
| `02-naming-convention.md` §2.3.1 UB_\* enum 命名 | §3 接合表で UB_\* enum → block 名対応 |
| `02-naming-convention.md` §3 rename 表 | §3 接合表で旧名 → 新名 適用 |
| `04-codegen-ubo.md` §5.1 `ubo_metadata.inl` (block_name → (size, set, binding)) | §2 配置決定の Codegen 出力契約 |
| `04-codegen-ubo.md` §5.3 `UniformLocation` struct (= `block_hash` で物理 UBO 識別) | §5 dynamic offset の loc → block_hash 解決 |
| `05-existing-inventory-link.md` §4 E3 (= 79 個独立保持) | §2.2 set=1 帯への 79 個集約 |
| `05-existing-inventory-link.md` §6 MC1 (= 旧 G1、Material\* per-draw 統合、ID rename 経緯は chapter 05 §6 header 注) | §2.3 set=2 帯への Material\* 配置 |
| `06a-cache-structure-and-setter-redirect.md` §3.2 `mUseUBO` flag 配置 | §6 initial 設定方針確定 |
| `06b-cadence-update-site-and-dirty.md` §4.1 flush 関数 5 種 | §4 flush 直後 bind 配線 |
| `06b-cadence-update-site-and-dirty.md` §5.3 L1+L2 default | §5 dynamic offset bind 側責務 |
| `06b-cadence-update-site-and-dirty.md` §4.3 triple-buffering (U1=3) | §7 set=0 rotate 方式 |
| `ayastorm-r41-ubo-current-state-inventory.md` §1 OpenGL 実働 UBO 4 種 | §3 接合表で既存 UB_\* binding 温存根拠 |

---

## §2 descriptor set 4 帯 cadence 別配置

### §2.1 配置原則 (= M1 採用、§0.2 (MD) default)

**descriptor set 4 帯 ↔ cadence 5 分類の 1:1 マッピング** (= **M1 採用、本 chapter §10 (MD) で AYA 確認対象**):

| set | cadence 帯 | rebind 頻度 | 物理 UBO 数 | bind 駆動関数 (06b §4.1) |
|---|---|---|---|---|
| **set=0** | per-frame + singleton | frame 開始時 1 回 | 3 (Frame\*) + 1 (Global_\*) = 4 | `flushFrameUbos()` 直後 + reflection update 時に該当 entry のみ rebind |
| **set=1** | per-program | shader bind 時 | 79 (Program_\*) | `flushProgramUbos(LLGLSLShader*)` 直後 |
| **set=2** | per-draw (Draw_\* + Material\* MC1 統合) | draw call ごと (= dynamic offset で参照点切替、descriptor set rebind は最小限) | 4 (Draw_\* 2 + Material\* 2) | `flushDrawUbos()` 直後 (= dynamic offset 更新主体、descriptor set 自体は program 切替時のみ rebind) |
| **set=3** | per-asset + per-skin | asset/skin owner 切替時 | (Asset_\* 2 × N) + (Skin_\* 1 × M) per-owner instance、descriptor set は owner 単位 | `flushAssetUbos(asset)` / `flushSkinUbos(skin)` 直後 |

**根拠** (= M1 採用 default の判定 4 軸):

1. **PSO compatibility 安定**: set 帯固定 = shader 内 `layout(set=N, binding=M)` 宣言が cadence と 1:1 対応、shader compile 後の pipeline layout が cadence 跨ぎで再生成不要 (= chapter 07 で PSO cache hit 率最大)
2. **cadence 跨ぎ rebind 最小**: per-frame は frame 開始 1 回、per-program は shader bind 時のみ、per-draw は dynamic offset 主体で descriptor set rebind ゼロ近く、per-asset/per-skin は owner 切替時 = bind 頻度が cadence 頻度に従順
3. **Vulkan 教科書通り**: AMD/NVIDIA driver の典型 best practice = cadence と set を 1:1 で揃える設計 (= bind cost の予測可能性、driver internal cache hit 率)
4. **06b §4.1 flush 関数 5 種と 1:1 対応** (set=0/1/2/3 ↔ flushFrame/flushProgram/flushDraw/flushAsset+flushSkin) = 設計 stack 全体で一貫

### §2.2 set=0 (per-frame + singleton 帯) — 4 binding

| binding | block 名 (chapter 02 §3.1 / §2.1 確定) | UB_\* enum (chapter 02 §2.3.1) | cadence | 物理 instance | triple-buffering |
|---|---|---|---|---|---|
| 0 | `FrameViewProj` | `UB_FRAME_VIEW_PROJ` | per-frame | `LLGLSLShader::mFrameUBO` slot (= chapter 02 §2.3.2) | N_buf=3 rotate (= §7) |
| 1 | `FrameLights` | `UB_FRAME_LIGHTS` | per-frame | 同上 | 同上 |
| 2 | `FrameAtmosphere` | `UB_FRAME_ATMOSPHERE` | per-frame | 同上 | 同上 |
| 3 | `Global_ReflectionProbes` | `UB_GLOBAL_REFLECTION_PROBES` | singleton (reflection update 時) | `LLReflectionMapManager::mUBO` (singleton、inventory §1) | rotate 不要 (= update 頻度 < frame 頻度、§7.3) |

**配置根拠**:
- Frame\* 3 個は 06b §2.1 per-frame cadence + §4.3 triple-buffering 対象 = stable set として frame 開始時 bind
- `Global_ReflectionProbes` (= 旧 `UB_REFLECTION_PROBES`) は singleton owner で per-frame cadence に近い (= reflection update 6 frame 周期、inventory §1)。Frame\* と同じ stable set に同居が論理的に綺麗
- Vulkan space では既存 GL space の binding 番号 (= upstream LL 確立済 `binding=0`) と 1:1 対応せずに済む (= GL/Vulkan binding namespace 独立、設計判断として set=0 帯に集約)

### §2.3 set=1 (per-program 帯) — 79 binding

| binding 範囲 | block 名群 | UB_\* enum 範囲 | cadence | 物理 instance |
|---|---|---|---|---|
| 0-22 | `Program_*` (= 旧 set=2 帯 23 個、chapter 02 §3.3 rename 後) | `UB_PROGRAM_*` | per-program | `LLGLSLShader::mProgramUBO` slot 群 |
| 23-78 | `Program_*` (= 旧 set=3 帯 54 個 + 上記の重複なし、chapter 02 §3.4 rename 後、chapter 05 §4 E3 採用で 79 個独立保持) | 同上 | 同上 | 同上 |

**配置根拠**:
- chapter 05 §4 E3 採用 = 旧 set=2 帯 23 個 + 旧 set=3 帯 54 個 = 79 個 Program_\* を独立保持
- 全 79 個が同 cadence (= per-program) のため set=1 に集約
- `flushProgramUbos(LLGLSLShader*)` (= 06b §4.1) で shader bind 時 1 回 rebind、bind されない program の Program_\* は upload も bind も走らない (= 06b §2.2)

**注 (= §10 (V1) 持越)**: 79 binding は **Vulkan `maxDescriptorSetUniformBuffers` device limit** (= 通常 hardware 84-90、minimum spec 96+) ギリギリ。chapter 07 で実 device feature query + 不足時の split 対応 (= set=1 を 2 set に分割) を確認必要。本 chapter は default 79 binding 1 set 構成、chapter 07 で device 制約検知時に再評価。

### §2.4 set=2 (per-draw 帯) — 4 binding

| binding | block 名 | UB_\* enum | cadence | 物理 instance | bind 戦略 |
|---|---|---|---|---|---|
| 0 | `Draw_LightParams` | `UB_DRAW_LIGHT_PARAMS` | per-draw | ring buffer 内 dynamic offset 参照 | L1+L2 (= 06b §5.3) |
| 1 | `Draw_MultiLight` | `UB_DRAW_MULTI_LIGHT` | per-draw | 同上 | 同上 |
| 2 | `MaterialUBO` (= MC1 統合で per-draw cadence、命名は §2.1 chapter 05 §6.3 で温存) | `UB_MATERIAL` | per-draw (MC1) | 同上 | 同上 |
| 3 | `MaterialLegacyBlinn` (= 暫定名、chapter 05 §5 / (F) で確定後 update) | `UB_MATERIAL_LEGACY` | per-draw (MC1) | 同上 | 同上 |

**配置根拠**:
- chapter 05 §6 MC1 (= 旧 G1) 採用 = Material\* は per-draw cadence + dirty flag 統合 → set=2 per-draw 帯に配置 (cadence と命名の独立性、chapter 02 §2.2)
- 4 binding 全件が dynamic offset (= L2、`VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`) 参照 = descriptor set 自体は **program 切替時のみ rebind**、draw call ごとは offset 数値変更のみ (= §5)

### §2.5 set=3 (per-asset + per-skin 帯) — 3 binding × N owner

| binding | block 名 (chapter 02 §3.1) | UB_\* enum | cadence | 物理 instance |
|---|---|---|---|---|
| 0 | `Asset_GLTFNodes` | `UB_ASSET_GLTF_NODES` (= 旧 `UB_GLTF_NODES`) | per-asset | `gltf::Asset::mNodesUBO` × N (rezzed GLTF asset 数) |
| 1 | `Asset_GLTFMaterials` | `UB_ASSET_GLTF_MATERIALS` (= 旧 `UB_GLTF_MATERIALS`) | per-asset | `gltf::Asset::mMaterialsUBO` × N |
| 2 | `Skin_GLTFJoints` | `UB_SKIN_GLTF_JOINTS` (= 旧 `UB_GLTF_JOINTS`) | per-skin | `gltf::Skin::mUBO` × M (rigged GLTF Skin 数) |

**配置根拠**:
- Asset\* 2 個と Skin\* 1 個は owner 単位の物理 instance を多数持つ (= inventory §1 で 2N + M)、descriptor set は owner 切替時に rebind (= `flushAssetUbos(asset)` / `flushSkinUbos(skin)` 直後)
- owner 数 N + M は scene 規模次第 (= 典型 10-50、大規模 sim で 100+)。descriptor set 自体は **3 binding 固定**、owner 切替で 3 binding の参照先 buffer を rebind
- 既存 GL path の binding 番号 (= upstream LL `binding=1/2/3`) は GL space で確立済、Vulkan space では set=3 内 binding=0/1/2 に再割当 (= GL/Vulkan binding namespace 独立、§3 接合表で詳述)

---

## §3 既存 UB_\* 4 binding ↔ 84 blueprint 接合表

### §3.1 接合表 (= 全 84 + 既存実働 4 の最終配線)

#### set=0 帯 (= per-frame + singleton)

| Vulkan space | block 名 (chapter 02 §3.1) | UB_\* enum | 旧 GL binding (inventory §4.1) | cadence | flush 関数 |
|---|---|---|---|---|---|
| set=0 binding=0 | `FrameViewProj` | `UB_FRAME_VIEW_PROJ` (新規) | (GL path 未確立) | per-frame | `flushFrameUbos()` |
| set=0 binding=1 | `FrameLights` | `UB_FRAME_LIGHTS` (新規) | (GL path 未確立) | per-frame | 同上 |
| set=0 binding=2 | `FrameAtmosphere` | `UB_FRAME_ATMOSPHERE` (新規) | (GL path 未確立) | per-frame | 同上 |
| set=0 binding=3 | `Global_ReflectionProbes` | `UB_GLOBAL_REFLECTION_PROBES` (= 旧 `UB_REFLECTION_PROBES` rename) | GL binding=0 (upstream LL 確立) | singleton | 同上 + reflection update 時 |

#### set=1 帯 (= per-program、77 個 + (V2) 確定後 +2)

| Vulkan space | block 名 (chapter 02 §3.3 / §3.4 rename 後) | 旧名 | 起源 sub-step |
|---|---|---|---|
| set=1 binding=0-22 | `Program_GammaCorrect` / `Program_AlphaParams` / ... | chapter 02 §3.3 PerProgramUBO_\* 23 個 | η-24 〜 η-28 Phase 2c |
| set=1 binding=23-76 | `Program_AtmoExtra` / `Program_SkyVParam` / ... | chapter 02 §3.4 \<Name\>UBO_Legacy 54 個 (E3 採用) | η-6 / η-13 期 |

**= set=1 帯の実配置 = 77 個** (= 23 + 54、(V2) 確定後最終 77-79)。下記 set=1 binding 番号は (V2) 解消結果に依存し、`Program_LightParams` / `Program_MultiLight` が per-program 帰属に確定した場合のみ binding=77/78 として追加される。

##### set=1 帯 rename audit trail (= 設計 review 2026-06-03 §3.1 set=1/set=2 重複表示解消、配置外)

下記 2 行は **配置でない** (= 上記 set=1 表に含めない、set=2 帯 binding=0/1 に実配置済)。chapter 02 §3.3 旧名 `PerDrawUBO_*` から `Draw_*` への rename 経緯を audit trail として記録するためだけのリスト:

| 旧 inventory §3.3 binding 配置 (= 旧表記) | 旧名 | 新名 | 実配置 (本 chapter §2.4) |
|---|---|---|---|
| (旧) set=1 binding=0 候補 | `PerDrawUBO_LightParams` | `Draw_LightParams` | **set=2 binding=0** |
| (旧) set=1 binding=1 候補 | `PerDrawUBO_MultiLight` | `Draw_MultiLight` | **set=2 binding=1** |

**注**: 06b §1.4 / chapter 05 §3.3 で binding=0/1 の cadence 帰属は **per-draw** 確定 (= 旧 inventory §3.3 表現の `PerDrawUBO_*` 名前空間そのまま per-draw 帯)。`Program_LightParams` / `Program_MultiLight` 候補は (V2) で「同一 binding 複数 UBO 名疑い」の文脈で per-program 再分類が必要かを (H1b) 計測で確定する論点として残存、確定後 per-program 帯入りすれば上記 set=1 表に binding=77/78 として追記。

(V2) 持越は inventory §3.3.1 の同一 binding 複数 UBO 名疑い (= ClipPlane / SkinnedVelocity / AvatarVelocity / AvatarSkin / ObjectSkin) のみ、(E') として Phase 0 計測対象 (= `06a-prep` §3)。本 chapter は **確定済 2 個 (LightParams / MultiLight) は per-draw 帯**、その他 23 + 54 = 77 個 Program_\* を set=1 帯に配置。

#### set=2 帯 (= per-draw、4 個)

| Vulkan space | block 名 | 旧名 | cadence | dynamic offset |
|---|---|---|---|---|
| set=2 binding=0 | `Draw_LightParams` | `PerDrawUBO_LightParams` | per-draw | あり (= L2、§5) |
| set=2 binding=1 | `Draw_MultiLight` | `PerDrawUBO_MultiLight` | per-draw | あり |
| set=2 binding=2 | `MaterialUBO` | `MaterialUBO` (温存) | per-draw (MC1) | あり |
| set=2 binding=3 | `MaterialLegacyBlinn` | `MaterialUBO_Legacy` (= 暫定名、(F) 確定後 update) | per-draw (MC1) | あり |

#### set=3 帯 (= per-asset + per-skin、3 個)

| Vulkan space | block 名 (chapter 02 §3.1) | UB_\* enum | 旧 GL binding (inventory §4.1) | cadence | flush 関数 |
|---|---|---|---|---|---|
| set=3 binding=0 | `Asset_GLTFNodes` | `UB_ASSET_GLTF_NODES` (= 旧 `UB_GLTF_NODES` rename) | GL binding=1 (upstream LL 確立) | per-asset | `flushAssetUbos(asset)` |
| set=3 binding=1 | `Asset_GLTFMaterials` | `UB_ASSET_GLTF_MATERIALS` (= 旧 `UB_GLTF_MATERIALS` rename) | GL binding=2 | per-asset | 同上 |
| set=3 binding=2 | `Skin_GLTFJoints` | `UB_SKIN_GLTF_JOINTS` (= 旧 `UB_GLTF_JOINTS` rename) | GL binding=3 | per-skin | `flushSkinUbos(skin)` |

### §3.2 集計

| set | binding 数 | 物理 instance 数 | 帰属 cadence |
|---|---|---|---|
| set=0 | 4 | 3 + 1 = 4 (per-frame slot 3 + singleton 1) | per-frame + singleton |
| set=1 | 77 (= 23 + 54、(V2) 確定後最終 77-79) | program 単位、bind されている program 数 (= 典型 20-50) | per-program |
| set=2 | 4 | ring buffer 1 + dynamic offset (= L1+L2、§5) | per-draw + Material\* MC1 統合 |
| set=3 | 3 | (Asset 2 × N) + (Skin 1 × M)、owner 数 scene 規模次第 | per-asset + per-skin |
| **合計** | **88** | scene 規模で 1 + 2N + M + (典型 20-50) + 数物理 instance | — |

**整合確認 (= chapter 01 §3.3 / §3.4 用語)**:
- **論理 binding 種類** = 88 (= shader 内 `layout(set=N, binding=M)` 総数 = 既存 84 GLSL blueprint + 既存実働 UB_\* 4 種 (rename 後の set=3 帯 3 個 + set=0 帯 `Global_ReflectionProbes` 1 個))
- **物理 buffer instance** = scene 規模次第 (= owner 単位 dynamic 変動、chapter 01 §3.4)

### §3.3 既存 GL path との binding 番号差分 (= 原則 1 含意)

| 旧 GL path | 新 Vulkan path | call site 影響 |
|---|---|---|
| `glUniformBlockBinding(prog, blockIdx, 0)` for `ReflectionProbes` | `layout(set=0, binding=3)` in shader (= GL space binding=0 と Vulkan space set=0/binding=3 で **GL/Vulkan binding namespace 独立**、shader 側で path 別宣言、host call site 無改変) | call site 改修ゼロ (= 原則 1 維持) |
| `glUniformBlockBinding(prog, blockIdx, 1)` for `GLTFNodes` | `layout(set=3, binding=0)` in shader | 同上 |
| `glUniformBlockBinding(prog, blockIdx, 2)` for `GLTFMaterials` | `layout(set=3, binding=1)` in shader | 同上 |
| `glUniformBlockBinding(prog, blockIdx, 3)` for `GLTFJoints` | `layout(set=3, binding=2)` in shader | 同上 |

**含意**:
- GL space の binding 番号 (= 0/1/2/3) は upstream LL 確立済、call site は `LLGLSLShader::UB_*` enum 値経由で参照 (= rename 後の `UB_GLOBAL_REFLECTION_PROBES` / `UB_ASSET_GLTF_*` / `UB_SKIN_GLTF_JOINTS` も同じ整数値 0/1/2/3 を保持)
- Vulkan space の set/binding は shader 内 `layout(set=N, binding=M) #ifdef LL_VULKAN_GLSL` で path 別宣言、GL path と Vulkan path が同一 enum 値で **異なる空間** を指す = host 側 enum 値は GL path 互換、Vulkan path bind は別 dispatcher (= chapter 07)
- 原則 1 (call site 温存) 完全達成: host `LLGLSLShader::UB_*` enum と既存 `glBindBufferBase(GL_UNIFORM_BUFFER, UB_*, ...)` call site は無改変、Vulkan path の descriptor set bind は chapter 07 で新規 dispatcher 追加

### §3.4 (O) UB_\* 4 binding 拡張方針 = O1 採用 (§0 default)

| 案 | 内容 | 採否 |
|---|---|---|
| O1 | **既存 4 種維持 + 新規 binding 追加** = `UB_REFLECTION_PROBES` / `UB_GLTF_NODES` / `UB_GLTF_MATERIALS` / `UB_GLTF_JOINTS` の enum 値 (0/1/2/3) を温存 (rename だけ実施)、新規 UBO は新 enum 値 (4+) で追加 | **採用 (= default)** |
| O2 | cadence 別 binding 拡張 = 既存 4 種を破棄、cadence 5 分類で 5 種に再構成 | 却下 (= 原則 1 違反、call site 改修必須) |

**O1 採用根拠**:
- 既存 `glBindBufferBase(GL_UNIFORM_BUFFER, UB_REFLECTION_PROBES, ...)` 等の call site (= `llreflectionmapmanager.cpp:1334` / `gltfscenemanager.cpp:693/696/736`) を改名・引数変更しない = 原則 1 (call site 温存) 完全達成
- 新規 UBO (= 84 GLSL blueprint 由来) は新 enum 値 (= `UB_FRAME_*` / `UB_PROGRAM_*` / `UB_DRAW_*` / `UB_MATERIAL*` 等、chapter 02 §2.3.1) で追加 = 既存 4 種と独立 namespace
- upstream Firestorm/Linden の OpenGL path 改修取込時、既存 4 種の binding 番号差分が出ない = upstream 互換最大

---

## §4 flush 直後 descriptor set bind 配線

### §4.1 cadence 別 bind sequence (= 06b §4.1 flush 関数 5 種直後の rebind)

```
[ frame loop 開始 ]
  ↓
flushFrameUbos()                                ← 06b §4.1
  ↓ (dirty bit check → mapped memory write → triple-buffer rotate)
[ set=0 rebind ]  bindDescriptorSet(set=0, current_frame_idx)   ← §7 rotate 反映
  ↓
[ shader program A bind ]
  ↓
flushProgramUbos(shaderA)                       ← 06b §4.1
  ↓ (dirty bit check → upload)
[ set=1 rebind ]  bindDescriptorSet(set=1, shaderA_program_set) ← 79 binding 1 set
  ↓
[ asset X draw 開始 ]
  ↓
flushAssetUbos(assetX)                          ← 06b §4.1
  ↓
[ set=3 rebind (partial) ]  bindDescriptorSet(set=3, assetX_owner_set) ← Asset_* 2 binding
  ↓
[ skin Y bind (rigged asset の場合) ]
  ↓
flushSkinUbos(skinY)                            ← 06b §4.1
  ↓
[ set=3 rebind (partial) ]  bindDescriptorSet(set=3, skinY_owner_set)  ← Skin_* 1 binding
  ↓
[ per-draw loop 開始 ]
  ↓
[ draw N ]
  ↓
flushDrawUbos()                                 ← 06b §4.1
  ↓ (dirty bit check → ring buffer offset 進行 → memcpy)
[ set=2 dynamic offset 更新 ]  vkCmdBindDescriptorSets(..., 1 dynamic offset = current ring offset) ← §5 dynamic offset
  ↓
[ vkCmdDraw ]
  ↓ (loop)
[ frame loop 終了 ]
```

**規律**:
- **set=0 / set=1 / set=3** は cadence 駆動の **descriptor set rebind** (= `vkCmdBindDescriptorSets` で set 入替)
- **set=2** は descriptor set 自体は **program 切替時のみ rebind**、draw call ごとは **dynamic offset 数値変更のみ** (= L2 default、§5)
- 各 flush 関数の **直後** に該当 set の bind が走る = upload → bind → draw の順序保証 (= 06b §4.2)

### §4.2 cadence 跨ぎ rebind 範囲

| 跨ぎパターン | rebind 必要 set | 根拠 |
|---|---|---|
| frame N → frame N+1 | set=0 (rotate) | triple-buffering で per-frame buffer 切替 (= §7) |
| shader A → shader B (per-program 切替) | set=1 + (PSO 切替に伴い) set=2 dynamic offset reset | program 単位 UBO 集合切替 |
| asset X → asset Y (per-asset 切替) | set=3 partial (Asset_\* 2 binding) | owner 単位 buffer 切替 |
| skin P → skin Q (per-skin 切替) | set=3 partial (Skin_\* 1 binding) | 同上 |
| draw N → draw N+1 (同 shader / 同 asset) | set=2 dynamic offset のみ | descriptor set 不変、offset 数値変更 |

**含意**:
- cadence 跨ぎ rebind は **必要最小限** (= set 帯と cadence 1:1 配置の M1 採用効果)
- 1 frame での descriptor set bind cost = (set=0 rebind 1 回) + (set=1 rebind × shader 数 = 20-50) + (set=3 rebind × owner 数 = N + M) + (set=2 dynamic offset 更新 × draw 数 = 数百〜数千、ただし descriptor set rebind ではない)
- = bind cost が cadence 頻度に従順、Vulkan driver の bind cache hit 率最大化

### §4.3 PSO compatibility

**Vulkan PSO (pipeline state object) compatibility** は **pipeline layout 単位** で決まる (= `VkPipelineLayoutCreateInfo::pSetLayouts` に渡す descriptor set layout 配列が一致すれば compatible)。

| compatibility 軸 | 本設計での扱い |
|---|---|
| set=0 layout = Frame\* 3 + Global_\* 1 binding | 全 shader 共通固定 layout = PSO 跨ぎ stable |
| set=1 layout = Program_\* 79 binding | program ごとに layout 異なる可能性 → **§10 (V3) 持越**: 全 program 共通 79 binding layout で揃える (= 一部 program は dummy binding) vs program ごとに layout 異なる (= PSO 別 cache) のトレードオフ |
| set=2 layout = Draw_\* 2 + Material\* 2 binding | 全 shader 共通固定 layout |
| set=3 layout = Asset_\* 2 + Skin_\* 1 binding | 全 shader 共通固定 layout (= GLTF 描画 program のみ参照、非 GLTF program は set=3 を dummy bind) |

**(V3) 持越** = set=1 layout を全 program 共通にするか program 別にするかは PSO cache 効率 vs descriptor set 効率のトレードオフ、chapter 07 で実 device feature query + 計測後決定。default は **全 program 共通 79 binding layout** (= PSO compatibility 最大、不使用 binding は dummy で埋める)。

**(V3) と (MD)/M1 採用の関係明示 (= 設計 review 2026-06-03 §3.4 修正)**:
- 本 chapter §2.1 で **M1 採用 (= descriptor set 4 帯 ↔ cadence 5 分類 1:1 配置)** を default に置いた前提で、(V3) は set=1 帯 (= per-program) 内部の layout 統一/分散のトレードオフ議論
- もし §10 (MD) で AYA が **M2 (= cadence 別 set 拡張)** を選択すると (V3) は前提崩壊: set=1 帯自体が複数 set に分散され「set=1 layout を全 program で共通にするか」という設問が成立しなくなる (= per-program 帯の各 sub-set がそれぞれ別の compatibility 判定)
- → (V3) chapter 07 消化は (MD) 解消 (chapter 10 / AYA 判断) 後でないと正しい論点設定にならない、(MD) → (V3) の順序依存あり (chapter 09 Phase Roadmap で順序保証)

---

## §5 dynamic offset (L2) bind 側責務

### §5.1 dynamic offset の Vulkan API 概要

Vulkan の `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (= UBO dynamic) は:
- descriptor set 内 binding に **buffer 全体** を参照させる (= `VkDescriptorBufferInfo::range` で UBO 1 個分の size を指定)
- 描画時 `vkCmdBindDescriptorSets()` の `pDynamicOffsets[]` 引数で **runtime offset 数値** を渡す
- shader 内 `layout(set=N, binding=M) uniform Block { ... }` 宣言は同じ、参照先 memory が offset 分シフト

= **descriptor set rewrite ゼロ で draw call ごとに UBO 内容切替** (= 06b §5.3 L2 default の本質)

### §5.2 per-draw ring buffer + dynamic offset bind 側責務

| 責務 | 配線位置 | 詳細 |
|---|---|---|
| ring buffer 1 個 allocate | chapter 07 (= 実 VMA `vmaCreateBuffer`) | per-draw 全 4 UBO (= Draw_\* 2 + Material\* 2) を 1 ring buffer に格納、UBO 単位で std140 size 整列 |
| draw 直前 dirty 判定 | 06b §5.2 `forwardToUboUpload` 内 (= stage 3 dirty bit) | per-draw cadence の 4 UBO それぞれ dirty bit、立っている UBO のみ ring buffer の next offset 位置に memcpy |
| dirty UBO の offset 確定 | `flushDrawUbos()` 内 | 該当 draw で参照する 4 UBO それぞれの ring buffer offset を `pDynamicOffsets[]` 用に確保 |
| `vkCmdBindDescriptorSets()` 呼出 | `flushDrawUbos()` 直後、`vkCmdDraw()` 直前 | set=2 を 1 回 bind + 4 個の dynamic offset 同時 set (= descriptor set rewrite なし、offset 数値のみ) |
| ring buffer wrap-around | chapter 07 (= ring buffer 容量決定 + wrap 検知) | 容量は frame 内 全 draw + α、wrap 時は **次 frame slot に rotate** (= write-after-read 回避、§7 と同じ paradigm) |

### §5.3 06b §5.3 L1+L2 default の bind 側成立条件

06b §5.3 で L1+L2 (= ring buffer + dynamic offset) を default 採用、本 chapter §5 はその **bind 側責務** を確定:

| 06b §5.3 確定 | 本 chapter §5 確定 |
|---|---|
| ring buffer 1 物理 buffer | descriptor set 1 個 + dynamic offset 4 個 (= per-draw 4 UBO の参照点) |
| draw 毎 offset 進行 | `flushDrawUbos()` 内で `pDynamicOffsets[]` 算出 |
| descriptor set rewrite ゼロ | `vkCmdBindDescriptorSets()` の `pDynamicOffsets` 引数のみ runtime 値 |
| wrap-around | frame 内 ring buffer 容量上限到達時に次 frame slot に rotate |

**= 06b §5.3 ring buffer 設計と本 chapter §5 dynamic offset bind 配線が 1:1 整合**、chapter 07 で実 Vulkan API 接合。

---

## §6 `mUseUBO` flag initial 設定方針 (= 06a 持越 (Q1) 解消、N 採用)

### §6.1 (N) 論点再掲

06a §5.4 で `LLGLSLShader::mUseUBO` flag の **initial 設定方針** が持越 (= (Q1)):

| 案 | 内容 | 影響 |
|---|---|---|
| N1 | **Vulkan path 全 ON** (= Vulkan build で全 shader `mUseUBO=true` 強制) | cold launch 時に全 shader UBO redirect 経路 active、(H1b) 計測前の Vulkan 起動成立可能性懸念 |
| N2 | **shader 単位 phase migration** (= chapter 09 Phase 別段階導入、初期は dummy / 段階的に shader を `mUseUBO=true` 化) | safe な phase progression、Phase 0 (H1b) 計測との順序整合 |

### §6.2 N2 採用 (= default、本 chapter §0 (N) で AYA 確認対象)

**採用根拠**:
1. **Phase 0 (H1b) 計測の先行性**: 06a §0.1 で確定済 = 動的 uniform 名 0 件 / shader link 時 1 回完結。これで perfect hash 事前 enumerate は成立するが、**実機 Vulkan 描画の成立性** は別軸。N1 全 ON は cold launch 時に Vulkan 描画破綻リスクがあり、(H1b) 計測前は安全側 N2 採用
2. **chapter 09 Phase Roadmap との整合**: chapter 09 で「1 UBO ずつ migration → cold launch 検証 → 次へ」(= 01-overview §5 #8) の原則を本 chapter で破る理由が無い
3. **`mUseUBO` flag の意味論的明確化**: shader 単位で flip 可能 = どの shader が UBO redirect 経路に入っているか明確、debug / live A/B 検証が shader 単位で可能 (= memory `feedback_visual_decisions_need_live_ab`)
4. **N1 全 ON が必要な phase は chapter 09 後半に来る**: 全 shader UBO 化が完了した段階で N1 (全 ON) に切替、それまでは N2 (個別 ON) で段階導入

### §6.3 N2 採用の具体実装

#### §6.3.1 initial 値設定

```cpp
// LLGLSLShader::LLGLSLShader() ctor
#ifdef LL_VULKAN_GLSL
    mUseUBO = false;  // initial = OFF (= 既存 OpenGL path 挙動 default、06a §3.2 既述)
#endif
```

#### §6.3.2 shader 単位 flip 機構

`LLGLSLShader::createShader()` (= shader link 時) で以下の判定で `mUseUBO` 設定:

```cpp
#ifdef LL_VULKAN_GLSL
    if (LLRender::sIsVulkan) {  // Vulkan path 全体 enable 判定 (chapter 07 で確定)
        // Phase migration list 参照: chapter 09 で確定する shader 名 whitelist
        if (LLPhaseMigrationList::isUboReady(mName)) {
            mUseUBO = true;
        }
    }
#endif
```

- `LLPhaseMigrationList::isUboReady(shader_name)` = chapter 09 Phase Roadmap で確定する shader 名 whitelist (= migration 完了 shader を時系列で追加)
- Phase 0 / Phase 1 / Phase 2 ... の各 Phase で whitelist に shader 追加 → cold launch 検証 → 次 Phase
- 全 shader 完了で whitelist 不要に縮約、N1 (全 ON) 形に最終収束

#### §6.3.3 debug cvar (= live A/B 検証用)

memory `feedback_visual_decisions_need_live_ab` 準拠 で:

| cvar 名 (案) | 役割 | initial |
|---|---|---|
| `AYAUboRedirectEnabled` | Vulkan path 全 shader UBO redirect master switch | OFF (= 既存 GL path default) |
| `AYAUboPhaseMigrationOverride` | Phase migration list 無視で全 shader ON 強制 | OFF |
| `AYAUboShaderWhitelist` (CSV) | 個別 shader 名指定で ON 強制 | empty |

cvar 詳細仕様は chapter 09 Phase Roadmap で確定、本 chapter §6 は **存在 + 役割** のみ確定。

---

## §7 triple-buffering (U1=3) 配下 set=0 rotate 方式

### §7.1 06b §4.3 確定の再掲

per-frame UBO は **frame N の draw が GPU で読み終わる前に frame N+1 の host write を始める** = write-after-read hazard。06b §4.3 で triple-buffering 採用、N_buf=3 default (= (U1))。

### §7.2 set=0 rotate 配線

triple-buffering の物理形態:
- 各 Frame\* UBO (= 3 物理 instance: `FrameViewProj` / `FrameLights` / `FrameAtmosphere`) を **N_buf=3 個ずつ** rotate 配置 = 計 9 物理 buffer + Global_\* 1 物理 buffer = **10 物理 buffer**
- frame N で write 対象 = instance idx (N % 3)、frame N-2 は GPU 読込完了済 (= safe)

set=0 descriptor set の rotate 方式:
- **方式 A**: descriptor set を 3 個 rotate (= 各 set が固定 instance idx を指す)、`flushFrameUbos()` 直後に current frame の descriptor set を bind
- **方式 B**: descriptor set 1 個固定、frame ごとに `vkUpdateDescriptorSets()` で参照先 buffer を入れ替え

| # | 案 | bind cost | update cost | 推奨 |
|---|---|---|---|---|
| A | 3 descriptor set rotate | bind 1 回 / frame | update 0 回 | **Claude 推奨 default** |
| B | 1 descriptor set + 毎 frame update | bind 1 回 / frame | update 4 binding × 1 回 / frame | 不採用 |

**A 採用根拠**: `vkUpdateDescriptorSets()` は driver 内で descriptor pool memory rewrite が走る (= cost あり)。3 set rotate は memory footprint +2 set 分 (= 微小)、update cost ゼロ、bind cost 同等。

### §7.3 Global_ReflectionProbes の例外扱い

`Global_ReflectionProbes` は singleton owner で **reflection update 時のみ**書込 (= update 頻度 < frame 頻度、inventory §1)。triple-buffering 不要、**1 物理 buffer** で十分:

- set=0 binding=3 は 3 descriptor set rotate 全てで同じ Global_\* buffer を参照
- reflection update 時の write-after-read 回避は **GPU fence 経由 sync** (= chapter 07 で確定)
- = 3 descriptor set rotate の binding=3 だけは同 buffer pointer、binding=0/1/2 は frame_idx 別 buffer pointer

---

## §8 sampler / opaque 49 個 descriptor set 経由 binding (bridge)

### §8.1 06a §5.6 確定の再掲

sampler 49 個 (= `diffuseMap` / `normalMap` / `shadowMap0-5` 等) は GLSL spec 上 UBO 化対象外。06a §5.6 で OpenGL path 強制 (= setter 内 `cadence_tag == CADENCE_SAMPLER` で skip)、Vulkan path は **descriptor set 経由 binding** を chapter 07 で配線と既述。

### §8.2 本 chapter §8 の bridge 範囲

本 chapter は **配置 set 帯の確定** のみ:

| 案 | sampler 配置 set 帯 | 影響 |
|---|---|---|
| S1 | **set 帯独立**: 新規 set=4 (sampler 専用帯) | bind cost + descriptor set 1 個追加、cadence と独立 |
| S2 | **既存 set 帯混在**: set=1 (per-program 帯) に sampler binding 追加 | set=1 layout 拡大 (= 79 → 79 + 49 = 128 binding、device limit 超過リスク高) |
| S3 | **chapter 07 譲り**: 本 chapter は配置決定せず chapter 07 で確定 | scope 切出し、本 chapter focus 維持 |

**Claude 推奨 = S3** (= chapter 07 譲り):
- sampler 49 個の cadence 帰属が unclear (= per-program で shader bind 時に binding する texture vs per-asset で asset 切替時に rebind する texture が混在)
- bind cost + device limit の実 device feature query 必要 (= chapter 07 軸)
- 本 chapter §0.2 非 scope (= sampler 49 個 実 binding 配線) と整合

### §8.3 sampler 配置決定後の本 chapter への波及

chapter 07 で sampler 配置 (S1 / S2 / 別案) 確定後、本 chapter §3 接合表に **set=4 (sampler 帯) または set=1 拡張 layout** を追記。本 chapter §3.2 集計表も更新。

---

## §9 chapter 04 / 05 / 06a / 06b / 07 / 09 との分担境界

| chapter | 本 chapter からの入力 | 本 chapter への出力 |
|---|---|---|
| **04** (codegen-ubo) | `ubo_metadata.inl` (= block_name → set/binding/size) / `UniformLocation` struct / `CadenceTag` enum | (本 chapter は受け側) — `ubo_metadata.inl` の set/binding 出力値が本 chapter §2/§3 配置と一致することを Codegen 側で保証 |
| **05** (existing-inventory-link) | 84 UBO の cadence 別 mapping (= §3) / E3 (= 79 個独立保持) / MC1 (= 旧 G1、Material\* per-draw 統合) | (本 chapter は受け側) — chapter 05 §7 集約 mapping は 06c 配置に従う |
| **06a** (cache-structure-and-setter-redirect) | `mUseUBO` flag 配置 / `mUniformUBOLoc[index]` cache / setter redirect 入口 | (Q1) `mUseUBO` initial 設定方針確定 (= §6 N2 採用) |
| **06b** (cadence-update-site-and-dirty) | flush 関数 5 種 / L1+L2 default / triple-buffering (U1=3) / dirty 二段階 dedup | (本 chapter は受け側) — flush 直後 bind 配線 (§4) / dynamic offset bind 側責務 (§5) / set=0 rotate (§7) で 06b 確定事項を bind 側に展開 |
| **07** (vulkan-api-state) | (本 chapter は提供側) | descriptor set layout 確定 (§2/§3) / bind sequence (§4) / dynamic offset bind 側責務 (§5) / triple-buffering rotate 方式 (§7) / sampler 配置決定 (§8 持越) / device limit 検知 (§2.3 (V1) / §4.3 (V3)) |
| **09** (phase-roadmap) | (本 chapter は提供側) | `LLPhaseMigrationList::isUboReady()` whitelist 構築 (§6.3.2) / cvar `AYAUboRedirectEnabled` 等の運用 (§6.3.3) |

---

## §10 未確定事項 (→ chapter 07 / 09 持越 / AYA 判断仰ぎ)

**読み方 (= 設計 review 2026-06-03 §3.4 修正)**: 「default 採用案」列は **本 chapter 全節が当該案を採用済前提で記述されている** ことを意味する (= Claude が已に default として配線済、chapter 内本文・表・code shape の全てが当該案で整合)。AYA が解消先で別案を選択した場合は本 chapter の該当節 (= 列内 § ポインタ) を覆って書換が必要。

| # | 項目 | 解消先 | default 採用案 (= 本 chapter 採用済) | 別案採用時 影響範囲 |
|---|---|---|---|---|
| **(MD)** (= 旧 (M)、設計 review 2026-06-03 §3.1 ID rename = material domain prefix、chapter 06b §8 thread-safe の (M) と衝突回避) | descriptor set 4 帯 ↔ cadence 5 分類 1:1 配置 (M1) vs cadence 別 set 拡張 (M2) | **chapter 10 / AYA 判断** | **M1 (= 1:1 配置、§2.1) — 採用済** | M2 採用時: §2 全体 / §3 接合表 / §4 bind sequence / §10 (V3) 前提崩壊 (= 上記 §4.3 (V3) 関係明示) |
| (N) | `mUseUBO` initial 設定 = N1 全 ON / N2 shader 単位 phase migration | **chapter 10 / AYA 判断** | **N2 (= phase migration、§6.2) — 採用済** | N1 採用時: §6.3.1 / §6.3.2 / §6.3.3 を full ON 形に書換 |
| (O) | UB_\* 4 binding 拡張 = O1 既存維持 + 新規追加 / O2 全体再構成 | **chapter 10 / AYA 判断** | **O1 (= 既存維持、§3.4) — 採用済** | O2 採用時: §3.3 既存 GL binding 番号差分が全面再構成、原則 1 (= upstream 取込互換) 影響大 |
| (V1) | set=1 が 79 binding で device `maxDescriptorSetUniformBuffers` 限界懸念 | chapter 07 | **default 79 binding 1 set、device limit 検知時に split 検討 — 採用済** | split 化採用時: §2.3 set=1 を 2 set に分割、§4.1 bind sequence + §10 (MD)/M1 維持判定再評価 |
| (V2) | inventory §3.3.1 同一 binding 複数 UBO 名疑い (= ClipPlane / SkinnedVelocity 等 5 個) | 実装 phase 入口 (= `06a-prep` §3 (E')) | A/B/C 案いずれか、Phase 0 計測待ち (= 本 chapter は **計測結果待ち、暫定 §3.1 set=1 帯 77 個**) | 確定次第 §3.1 set=1 帯に binding=77/78 追加可能性、§3.2 集計表 update |
| (V3) | set=1 layout = 全 program 共通 79 binding (= dummy 埋め) / program 別 layout | chapter 07 | **全 program 共通 (= PSO compatibility 最大) — 採用済** | program 別採用時: §4.3 PSO compatibility 表全面書換 + PSO cache 戦略再設計 |
| (S3) | sampler 49 個の descriptor set 配置帯 | chapter 07 | **配置決定は chapter 07 譲り、§8.2 で 3 案併記 — 採用未済 (= 本 chapter は bridge のみ)** | 確定次第 §3.1 接合表 + §3.2 集計表に set=4 or set=1 拡張形を追記 |

---

## §11 update 規律

- §2 配置の追加 / 変更は本 chapter に集約、chapter 04 `ubo_metadata.inl` 出力と整合確認
- §3 接合表は live 表として migration 進行で update (= rename 反映 / cadence 確定反映 / (V2) Phase 0 計測結果反映)
- §4 bind sequence の変更 (= chapter 07 確定後) は本 chapter §4 に反映、06b §4.1 flush 関数と整合維持
- §5 dynamic offset 配線は chapter 07 確定後に本 chapter §5 を update
- §6 `mUseUBO` 設定方針は chapter 09 Phase Roadmap 確定で whitelist 具体形を反映
- §7 set=0 rotate は chapter 07 実装で方式 A 確定したら §7.2 を確定形に書換え
- §8 sampler 配置は chapter 07 で S1/S2/別案確定後、本 chapter §3 接合表 + §3.2 集計表に追記
- §10 (MD)(N)(O)(V1)(V2)(V3)(S3) 持越は chapter 07 / 09 / 10 / 実装 phase で消化したら本 chapter から「保留候補」を剥がして reflect。なお (MD) は設計 review 2026-06-03 §3.1 で旧 (M) から rename (= material domain、chapter 06b §8 thread-safe の (M) と衝突回避)

---

**= 本 chapter で descriptor set 4 帯 cadence 別配置 + UB_\* 4 binding ↔ 84 blueprint 接合表 + flush 直後 bind 配線 + dynamic offset bind 側責務 + `mUseUBO` initial 設定方針 + PSO compatibility + triple-buffering rotate + sampler bridge が確定したため、chapter 06 (06a / 06a-prep / 06b / 06c) 全件起案完了 state に到達。chapter 07 (vulkan-api-state) で実 Vulkan API 配線 (= VMA / descriptor pool / pipeline layout / 実 buffer 生成 / sampler 配置確定 / device limit query / triple-buffering 実装) に進める**。
