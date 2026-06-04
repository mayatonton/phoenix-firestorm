# r41 UBO 全体設計 Chapter 06a: cache 構造 + setter redirect

**起案日**: 2026-06-03
**位置付け**: chapter 06 (redirect-layer-design) を 3 sub-chapter に分割した第 1 部。**host C++ 側 redirect 層の cache 構造 + shader link 時 pre-cache フロー + 16 method setter family の Vulkan path 分岐実装パターン** を確定する。cadence 別 update site (= 06b) / descriptor set bind 配線 (= 06c) は別 chapter で扱う。
**pre-requisite**:
- `01-overview.md` (用語定義 + 2 大設計原則)
- `02-naming-convention.md` (命名 + Codegen 生成識別子)
- `03-cadence-classification.md` (cadence 5 分類)
- `04-codegen-ubo.md` §6 (name-based 解決 dispatch)
- `05-existing-inventory-link.md` §7 (bare uniform 集約フロー)
- `ayastorm-r41-ubo-current-state-inventory.md` §4.3 (16 method setter family の現状)
- Phase 0 (D)(H1a) 計測結果 (= 本 chapter §0)

---

## §0 Phase 0 計測結果 (= 本 chapter 起案の前提データ)

### §0.1 Phase 0 (D) 動的 uniform 名存在確認 (= 2026-06-03 実施)

| 軸 | 結論 |
|---|---|
| 軸 1: C++ setter call site の name 引数性質 | **全て build-time static** (integer index 経由 `mUniform[index]` 主流 + `LLStaticHashedString` literal 経由、動的名前生成 setter 0 件) |
| 軸 2: GLSL 内 array uniform の N 固定性 | **全て preprocess-time 数値確定** (数値リテラル直記 + `#define` 経由 + `addPermutation("LIGHT_COUNT", ...)` の build-time variant 展開、runtime 動的 size 0 件) |
| 軸 3: shader link 時 uniform 名解決 | **shader link 時 1 回完結** (`LLGLSLShader::mapUniforms()` で `glGetActiveUniform` + `mReservedUniforms` match、frame 内再解決ゼロ) |

→ **compile-time perfect hash 事前 enumerate は完全成立、fallback 設計 0 件**。本 chapter §4 shader link 時 pre-cache フローはこの前提に立つ。

### §0.2 Phase 0 (H1a) bare uniform C++ caller enumerate (= 2026-06-03 実施)

| 集合 | 件数 | 用途 |
|---|---|---|
| **`LLShaderMgr::mReservedUniforms`** static 登録 (= Axis 1) | **318 unique names** (index 0-317、`llshadermgr.cpp:1506-1904`、`llassert` で全件検証 line 1906) | integer index 経由 setter の主集合 |
| **`LLStaticHashedString`** literal 経由 (= Axis 2) | **67 unique names** | post-process / utility shader 用 (pipeline.cpp / lldrawpool*.cpp / llreflectionmapmanager.cpp 等)、core UBO 化対象外と判定 |
| **sampler / opaque type** (= UBO 化対象外、GLSL spec 上 bare uniform 維持必須) | **49 個** | `sampler2D` / `samplerCube` / `sampler2DArray` / `sampler2DShadow` 等 |
| **数値型 (= UBO 化対象)** | **267 個** (= 84.0%) | chapter 05 §7.3 集約表の入力集合 |

**cadence 推定 (Agent caller context 解析、不明 16 個は (H1b) hook で確証取得)**:

| cadence | 件数 | 代表例 |
|---|---|---|
| per-frame | 85 | viewport / sun_dir / moon_dir / shadow_matrix / screen_res / motion_blur_strength |
| per-program | 92 | modelview_matrix / projection_matrix / normal_matrix / texture_matrix* / env_mat |
| per-draw | 68 | diffuse_color / emissive_color / specular_color / metallicFactor / roughnessFactor |
| per-asset | 45 | gltf_node_id / gltf_material_id / base_color_texcoord / 各 GLTF transform |
| per-skin | 12 | matrixPalette / translationPalette / avatar_wind / lastMatrixPalette |
| 不明 (= 06b 前 (H1b) hook 対象) | 16 | sampler binding / RLV 条件 path / 一部 terrain/water |

**集約規模含意**:
- chapter 05 §7.3 集約表は **267 行** → §7.3 owner rule (50 行超で切出し) 発動、別 file `05a-bare-uniform-mapping.md` に切出し対象 (= 06a 起案範囲外、別 session)
- 本 chapter §5 setter redirect 配線は **integer index 経由 318 + LLStaticHashedString 67 個の 2 系統** を path 分岐で吸収

---

## §1 本 chapter の scope

### §1.1 scope (= 本 chapter で確定するもの)

1. **`mUniformUBOLoc[index]` cache 構造** (= LLGLSLShader member 配置 + UniformLocation struct layout)
2. **shader link 時 pre-cache フロー** (= `mapUniforms()` Vulkan path 拡張、runtime hash 計算は link 時 1 回限り)
3. **16 method setter family の Vulkan path 分岐実装パターン** (= `#ifdef LL_VULKAN_GLSL` / `if (mUseUBO)` 分岐位置と code shape)
4. **LLStaticHashedString 経由 setter の補助 path** (= 補助 cache `mUniformUBOLocByHash` 配置)
5. **sampler 系 setter の OpenGL path 強制** (= UBO 化対象外 49 個の path 分岐)
6. **`mUseUBO` flag の配置 + initial 設定方針** (= 詳細決定は 06c に持越)
7. **(H1b) LL_INFOS hook 提案** (= 06b 起案前 prerequisite として AYA build run 提案)

### §1.2 非 scope (= 06b / 06c / chapter 07 譲り)

- cadence 別 update site (= per-frame frame loop / per-program shader bind / per-draw renderGeom 各位置で `forwardToUboUpload` を呼ぶ点) → **06b**
- dirty 判定機構 (= 既存 `mValue` cache を UBO upload 側に乗せ替え) → **06b**
- descriptor set bind 配線 (= set=0/1/2/3 帯 cadence 別 rebind タイミング、PSO compatibility) → **06c**
- `forwardToUboUpload(loc, data, size)` 本体実装 (= 実 memcpy / ring buffer / dynamic offset / thread 配線) → **06b / chapter 07**
- sampler 系 descriptor set 経由 binding (= 49 個 texture binding 配線) → **chapter 07**
- 実 UBO buffer 生成 / 解放 / VMA / vkQueueSubmit → **chapter 07**

---

## §2 入力契約 (= chapter 04 §6.2 受け側)

chapter 04 (Codegen-UBO) が提供する API (= 本 chapter が利用する面):

| chapter 04 出力 | 本 chapter での用途 |
|---|---|
| `ubo::lookup_runtime(const char* name) → const UniformLocation*` (= chapter 04 §5.3.2) | shader link 時 pre-cache で各 uniform 名 → UniformLocation 解決 (§4) |
| `ubo::g_uniform_table[]` (= compile-time perfect hash table、chapter 04 §5.3.2) | runtime lookup の bucket array、本 chapter は indirect 利用 (`lookup_runtime` 内部) |
| `<BlockName>Layout` struct (= chapter 04 §5.2) | sub-allocation 時 offset 定数参照、06b で利用 (本 chapter は受け側のみ) |
| `ubo_metadata.inl` (block_name → size / set / binding) | descriptor set 配線で利用、06c が消費 (本 chapter は受け側のみ) |

chapter 04 §6.1 解決 3 局面のうち、本 chapter は **R3 (`mUniform[index]` 経由) 最速 path** を実装。R1 (静的 literal compile-time 解決) / R2 (runtime 動的 string) は **shader link 時 1 度の R2 経由 cache build** に縮約して frame 内では R3 直引きで済ます。

---

## §3 mUniformUBOLoc[index] cache 構造

### §3.1 struct UniformLocation (= chapter 04 §5.3.2 と同形を本 chapter で利用)

```cpp
// ubo_perfect_hash.inl (chapter 04 §5.3.2、再掲)
namespace ubo {
struct UniformLocation {
    uint32_t block_hash;   // block 識別 hash (= ubo_metadata.inl で block_name → (size,set,binding) 引け)
    uint32_t offset;       // block 内 offset (std140)
    uint32_t size;         // member 値 size (= setter write 量)
    uint32_t cadence_tag;  // cadence (§3.3)
};
} // namespace ubo
```

### §3.2 LLGLSLShader member 配置

```cpp
// llglslshader.h (Vulkan path 拡張、既存 mUniform と parallel)
class LLGLSLShader {
public:
    // 既存:
    std::vector<S32> mUniform;  // mUniform[index] = GL location (= mReservedUniforms と parallel、size=318)

#ifdef LL_VULKAN_GLSL
    // 新規: integer index 経由 setter の Vulkan path lookup table
    std::vector<ubo::UniformLocation> mUniformUBOLoc;  // mUniform と parallel、size=318
    // 新規: LLStaticHashedString 経由 setter の補助 path lookup table (= §4.3)
    std::unordered_map<U64 /*hash*/, ubo::UniformLocation> mUniformUBOLocByHash;
    // 新規: Vulkan UBO redirect 有効化 flag (= §5.4)
    bool mUseUBO = false;
#endif
};
```

**規律**:
- `mUniformUBOLoc` は `mUniform` と **同 size / 同 index 順** で並列配置 (= R3 最速 path)
- 解放は `std::vector` / `std::unordered_map` の dtor で自動、`LLGLSLShader::~LLGLSLShader()` に特別な処理を追加しない
- `mUseUBO` initial value = `false` (= 既存 OpenGL path の挙動を default 維持)

### §3.3 cadence_tag enum (= 06b で詳細利用、本 chapter は値域確定のみ)

```cpp
namespace ubo {
enum CadenceTag : uint32_t {
    CADENCE_PER_FRAME    = 0,
    CADENCE_PER_PROGRAM  = 1,
    CADENCE_PER_DRAW     = 2,
    CADENCE_PER_ASSET    = 3,
    CADENCE_PER_SKIN     = 4,
    CADENCE_SAMPLER      = 5,  // UBO 化対象外、bare uniform 維持 / Vulkan は descriptor set 経由
    CADENCE_UNKNOWN      = 6,  // (H1b) hook 取得まで暫定値、default routing = per-program (conservative)
    CADENCE_INVALID      = 0xFFFFFFFF, // mReservedUniforms 登録だが UBO 集約表で未集約 (= chapter 05 §7 entry 無し)
};
} // namespace ubo
```

`cadence_tag` は **build-time perfect hash table (`g_uniform_table[]`) entry に含まれる**。chapter 04 Codegen-UBO pipeline 内で uniform 名 → cadence 判定 → tag 付与 (= chapter 05 §7 集約表の cadence 列を Codegen 入力に流し込む)。

### §3.4 解放規律

`LLGLSLShader::~LLGLSLShader()` で `mUniformUBOLoc.clear()` / `mUniformUBOLocByHash.clear()` は **不要** (= dtor で自動)。Vulkan UBO buffer 自体の解放は別 owner (= chapter 07 で確定)、本 cache は location ポインタ等を持たないため pointer 解放経路ゼロ。

---

## §4 shader link 時 pre-cache フロー

### §4.1 mapUniforms() の Vulkan path 拡張点

既存 `LLGLSLShader::mapUniforms()` (`llglslshader.cpp:1704` 付近) は:

1. `mUniform.resize(LLShaderMgr::instance()->mReservedUniforms.size(), -1)` (line 1713)
2. `for (S32 i = 0; i < activeCount; ++i) mapUniform(i)` (line 1842-1849)
3. 内部 `mapUniform(GLint index)` (line 1554) で `glGetActiveUniform` + `mReservedUniforms` との match → `mUniform[i] = location`

Vulkan path 拡張 (= 本 chapter の中核):

```cpp
void LLGLSLShader::mapUniforms() {
    // (既存 OpenGL path: mUniform[] 構築は維持)

#ifdef LL_VULKAN_GLSL
    if (mUseUBO) {
        const auto& reserved = LLShaderMgr::instance()->mReservedUniforms;
        mUniformUBOLoc.resize(reserved.size());
        for (size_t i = 0; i < reserved.size(); ++i) {
            const char* name = reserved[i].c_str();
            const ubo::UniformLocation* loc = ubo::lookup_runtime(name);
            if (loc) {
                mUniformUBOLoc[i] = *loc;
            } else {
                mUniformUBOLoc[i] = ubo::UniformLocation{
                    .block_hash   = 0,
                    .offset       = 0,
                    .size         = 0,
                    .cadence_tag  = ubo::CADENCE_INVALID,
                };
            }
        }
    }
#endif
}
```

### §4.2 runtime hash 計算は shader link 時 1 回限り (= R3 設計の実体化)

| 局面 | 頻度 | hash 計算回数 |
|---|---|---|
| shader link (= `mapUniforms()` 実行) | app 起動時 + shader cache invalidate 時 = 数百〜数千回 / app 起動 | `mReservedUniforms.size() = 318` × shader 数 |
| frame 内 setter call (= `uniform*fv()` 呼出) | 60 FPS × 数千 call/frame | **0 回** (= `mUniformUBOLoc[index]` 直引き、`std::vector::operator[]` のみ) |

= 既存 `mUniform[index]` の overhead と同等まで圧縮。runtime hash 計算は frame 内では完全に消える。

### §4.3 LLStaticHashedString 経由 uniform の補助 path

(H1a) Axis 2 で確認した 67 個の LLStaticHashedString 経由 uniform は `mReservedUniforms` 未登録 = `mUniformUBOLoc[index]` 経由不可。

補助 cache 構築 step (= `mapUniforms()` Vulkan path 内の追加 step、§4.1 拡張部の後)。

**⚠ 重要 (= 仮 code shape、本査読 2026-06-03 §3.4 致命傷候補解消、2026-06-03 Q24-S1 (A) 反映で 2 軸分割)**: 下記 code shape の `LLStaticHashedString::getGlobalRegistry()` は **実装が存在しないことが確認済** (= chapter 06a-prep §2.2.2 但し書き、(S1-存在) 解消マーク)。本 code shape のまま実装 phase に進めばコンパイルエラー直行する。実装に当たっては §4.3.1 の代替案 S1-A/B/C/D いずれかを採用、最終確定は **(S1-代替)** を chapter 06b / Phase 0 で消化した後 (= AYA 判断仰ぎ事項、Q24-S1 として chapter 10 §1 登録予定)。

```cpp
// ⚠ 仮 code shape: getGlobalRegistry() は実装無し、§4.3.1 代替案で書き換え必須
#ifdef LL_VULKAN_GLSL
    if (mUseUBO) {
        // (§4.1 mUniformUBOLoc 構築の後)
        // LLStaticHashedString global registry を iterate
        for (const auto& [hash, name] : LLStaticHashedString::getGlobalRegistry()) {
            const ubo::UniformLocation* loc = ubo::lookup_runtime(name.c_str());
            if (loc) {
                mUniformUBOLocByHash[hash] = *loc;
            }
        }
    }
#endif
```

#### §4.3.1 代替案 (= (S1-代替) 消化候補、いずれかを chapter 06b / Phase 0 完了時に確定)

| 案 | 内容 | 利点 | 欠点 |
|---|---|---|---|
| S1-A | `LLStaticHashedString` 内部 static container を直接 iterate する helper (例: `LLStaticHashedString::forEachInstance(callback)`) を新設、本 chapter §4.3 code shape の `getGlobalRegistry()` 部分を helper 呼出に置換 | 1 helper 追加で本 chapter code shape の構造維持、call site 1 箇所 | LLStaticHashedString class への侵襲、upstream divergence 1 件 (原則 1 軽微違反) |
| S1-B | shader link 時 `mReservedUniforms` に **無い** uniform を `glGetActiveUniform` 列挙結果から拾い、それぞれを `LLStaticHashedString(name)` で hash 計算 → `mUniformUBOLocByHash` に登録 | 既存 API のみで実装可、LLStaticHashedString 側無改修 | shader link 時 GL call 増、Vulkan path 専用 cache のため OpenGL build には影響なし |
| S1-C | LLStaticHashedString 経由 setter 67 個の名前を chapter 05 集約表確定時に build-time list 化 (= 静的配列 `g_static_hashed_uniform_names[]`)、shader link 時はその配列を iterate | runtime registry iterate 不要、build-time decidable | 67 個 list の保守責任が chapter 05 集約表に追加、追加忘れで silent skip |
| S1-D | 補助 path (= `mUniformUBOLocByHash`) を **廃止**、LLStaticHashedString 経由 setter 67 個を全て chapter 05 集約表で `mReservedUniforms` 化 (= integer index 経路に統合) | path 分岐削減 (32 entry point → 16)、cache 構造単純化 | 67 個全て mReservedUniforms 増要、chapter 05 集約表 67 行追加、各 program で全 67 個が active uniform 化される負担 |

**default 候補** (= chapter 06b / Phase 0 (S1-代替) 消化時の起点): **S1-C** (= build-time list 化)。理由: (1) runtime registry iterate を回避できる確定性、(2) chapter 05 集約表との一体管理で saving は明示的、(3) LLStaticHashedString class 無侵襲。最終確定は chapter 06b 起案時 / Phase 0 (S1-代替) で AYA 判断 (= Q24-S1)。

注:
- 67 個のうち UBO 化対象有無は **chapter 05 §7.3 集約表 (= 切出し後の `05a-bare-uniform-mapping.md`) で個別判定** (= 未確定 (R1)、§9 持越)
- 集約しない uniform は `mUniformUBOLocByHash` に entry が無く、setter 内 `find()` で `end()` 返却 → OpenGL path fallback
- 上記 §4.3 code shape は **設計意図の表現** であり、実コードは §4.3.1 代替案 + (S1-代替) 消化結果で置換

### §4.4 pre-cache フローの整合 check

shader link 完了後、debug build で以下を `llassert` 検証:
- `mUniformUBOLoc.size() == mUniform.size()` (= 並列配置の維持)
- 各 `mUniformUBOLoc[i].cadence_tag != CADENCE_INVALID` で対応する `mUniform[i] != -1` (= shader 内 active uniform は両 cache に存在)
- cadence_tag == CADENCE_SAMPLER の uniform は OpenGL path 強制 (= §5.6)

検証失敗は build error ではなく runtime assert (= Vulkan path 動作確認の安全網)、release build で除去。

#### §4.4.1 PB-7 実装確定 (= AYA 判断 2026-06-04、Phase 1.B PB-7 sub-step)

§4.4 整合 check 3 項目の `llassert` 実装方針:
- (1) `mUniformUBOLoc.size() == mUniform.size()` = **assert として実装**
- (2) cadence_tag != CADENCE_INVALID で対応する `mUniform[i] != -1` = **loop で per-element assert として実装**
- (3) cadence_tag == CADENCE_SAMPLER の uniform は OpenGL path 強制 (= §5.6) = **コメント注釈のみ、本 §4.4 assert 対象外**

(3) を assert 対象外とした根拠 (= AYA 判断採択):
- spec literal の (1) (2) は等式・含意形式で assert に直接展開可、(3) は命題 + §5.6 への参照記述 (= 文体が異なる)
- §5.6 setter 側で sampler は OpenGL path 強制が実体 = §4.4 はそれを案内する記述で本 check は (1) (2) の構造整合に限定するのが自然
- 現 Phase 1.B で sampler 集約自体未確定の可能性ゆえ、(3) を assert 化すると将来 sampler 集約変更時に false trip risk

実装位置 = `indra/llrender/llglslshader.cpp` `mapUniforms()` 末尾、PB-3 block 直後 (line 1944 直後)、`unbind()` 直前 (= PB-2/PB-3/PB-7 を 1 block 内集中、handoff PB-3 §3.1 (iv) の前後 maintenance 局所化方針通り)。

---

## §5 16 method setter family の Vulkan path 分岐

### §5.1 17 method 一覧 (= inventory §4.3 再掲、line 番号は HEAD 時点、2026-06-03 second-pass §2.4 反映で uniform1i 追加)

| method | line | 引数 type |
|---|---|---|
| `uniform1i` | 2141 | (U32 index, GLint) — sampler binding setter として該当 (= UBO 化対象外、§5.6 sampler 経路) |
| `uniform1f` | 2166 | (U32 index, GLfloat) |
| `fastUniform1f` | 2192 | (U32 index, GLfloat) |
| `uniform2f` | 2202 | (U32 index, GLfloat, GLfloat) |
| `uniform3f` | 2229 | (U32 index, GLfloat × 3) |
| `uniform4f` | 2256 | (U32 index, GLfloat × 4) |
| `uniform1iv` | 2283 | (U32 index, U32 count, const GLint*) |
| `uniform4iv` | 2310 | (U32 index, U32 count, const GLint*) |
| `uniform1fv` | 2338 | (U32 index, U32 count, const GLfloat*) |
| `uniform2fv` | 2365 | (U32 index, U32 count, const GLfloat*) |
| `uniform3fv` | 2392 | (U32 index, U32 count, const GLfloat*) |
| `uniform4fv` | 2419 | (U32 index, U32 count, const GLfloat*) |
| `uniform4uiv` | 2447 | (U32 index, U32 count, const GLuint*) |
| `uniformMatrix2fv` | 2475 | (U32 index, U32 count, GLboolean, const GLfloat*) |
| `uniformMatrix3fv` | 2496 | 同上 |
| `uniformMatrix3x4fv` | 2517 | 同上 |
| `uniformMatrix4fv` | 2538 | 同上 |

加えて LLStaticHashedString 経由の overload (= `uniform*fv(const LLStaticHashedString&, ...)`) が各 method に存在。本 chapter ではこれら全てを redirect 対象とする。

### §5.2 path 分岐の code shape (= `uniform1f` を例に)

```cpp
void LLGLSLShader::uniform1f(U32 index, GLfloat x)
{
    if (mProgramObject) {
        // 既存 mValue cache check (= 同値時 GL call 省略の既存 dedup、本 chapter では維持)
        // 注: dirty 判定の本格的 UBO upload 側乗せ替えは 06b §3.3 MC1 (= 旧 G1)
        auto it = mValue.find(index);
        if (it != mValue.end() && it->second == LLVector4(x, 0.f, 0.f, 0.f)) {
            return;
        }
        mValue[index] = LLVector4(x, 0.f, 0.f, 0.f);

#ifdef LL_VULKAN_GLSL
        if (mUseUBO) {
            // Vulkan path: mUniformUBOLoc[index] 経由で UBO offset 解決
            llassert(index < mUniformUBOLoc.size());
            const ubo::UniformLocation& loc = mUniformUBOLoc[index];

            if (loc.cadence_tag == ubo::CADENCE_INVALID) {
                // UBO 集約表で未集約 (chapter 05 §7) = silent skip
                // = bare uniform 残存中の遷移期、集約完了で本 path は消える
                return;
            }
            if (loc.cadence_tag == ubo::CADENCE_SAMPLER) {
                // sampler は UBO 化対象外 = OpenGL path に fallthrough (§5.6)
                // (= Vulkan では descriptor set 経由 binding、chapter 07)
                // 本 chapter では skip
                return;
            }

            // upload 本体 / dirty / descriptor set bind は 06b / 06c 譲り
            // 本 chapter (06a) では path 分岐位置と loc 取得点までを確定
            forwardToUboUpload(loc, &x, sizeof(GLfloat));  // 06b で実装
            return;
        }
#endif
        // OpenGL path 維持 (= 原則 1 完全達成: 既存 glUniform1f 呼出を 1 case 内に温存)
        glUniform1f(mUniform[index], x);
    }
}
```

### §5.3 分岐配置の規律 (= 16 method 全部に適用される pattern)

| 層 | 規律 |
|---|---|
| `mProgramObject` check (= 既存) | 最外周、program bind 済の前提 |
| `mValue` cache check (= 既存 dedup) | 分岐前、**本 chapter では維持** (06b で UBO upload 側に乗せ替え) |
| `#ifdef LL_VULKAN_GLSL` (= compile-time gate) | 外側、OpenGL build に Vulkan code を含めない |
| `if (mUseUBO)` (= runtime gate) | 内側、Vulkan build でも GL fallback 経路を残す可能性 |
| `mUniformUBOLoc[index]` 直引き | `llassert(index < size())` で範囲 check、release では index trusting |
| `cadence_tag == CADENCE_INVALID` skip | bare uniform 残存中の silent skip path |
| `cadence_tag == CADENCE_SAMPLER` skip | UBO 化対象外、本 chapter scope 外 |
| `forwardToUboUpload(loc, data, size)` | 06b 実装、本 chapter は interface 呼出位置を確定 |
| OpenGL path | 最後の case に温存、既存呼出 1 行のまま |

= **16 method 全部が同 pattern で 1 対 1 に展開可能**。pattern を一旦確定すれば残 15 method は機械的 copy + 引数型差異のみ修正。

### §5.4 mUseUBO flag の配置と initial 設定方針

`LLGLSLShader::mUseUBO` (= 新規 bool member、§3.2 既述):

| 局面 | 設定値 | 根拠 |
|---|---|---|
| LLGLSLShader 生成時 (= ctor) | `false` | 既存 OpenGL path の挙動を default 維持 |
| shader link 時 (= `mapUniforms()` 前) | shader 種別 / SPIR-V binary 存在 / Vulkan build か / runtime cvar で決定 | 詳細決定は 06c で確定 (= chapter 07 vulkan-api-state 接続点) |
| migration 中の段階 cvar | `debug settings: AYAUboRedirectEnabled` 等で flip 可能 | live A/B 検証用 (memory `feedback_visual_decisions_need_live_ab` 準拠) |

**本 chapter 06a では `mUseUBO` の存在 + setter 内 check 位置までを確定**、決定方法 (= 自動判定 vs cvar) は 06c で詰める。

### §5.5 integer index 経路 vs LLStaticHashedString 経路の分岐

setter family = **integer index 17 method + LLStaticHashedString 13 method = 計 30 entry point** (= integer index 版 + LLStaticHashedString 版、2026-06-03 second-pass §2.4 反映で uniform1i 追加 + LLStaticHashedString 経路実態 13 method 反映):

| 系統 | lookup | 出口 |
|---|---|---|
| integer index 経由 (= 主) | `mUniformUBOLoc[index]` 直引き | `forwardToUboUpload(loc, data, size)` |
| LLStaticHashedString 経由 (= 補助) | `mUniformUBOLocByHash.find(hash)` lookup | 同上 |

両系統で **`forwardToUboUpload(loc, data, size)` 共通入口** に到達 (= 06b で実装)。

LLStaticHashedString 版 path 分岐 code shape (= `uniform1f(const LLStaticHashedString&, GLfloat)` を例に):

```cpp
void LLGLSLShader::uniform1f(const LLStaticHashedString& uniform, GLfloat x)
{
    if (mProgramObject) {
        // (mValue cache check は LLStaticHashedString 版にも存在、本 chapter では維持)

#ifdef LL_VULKAN_GLSL
        if (mUseUBO) {
            auto it = mUniformUBOLocByHash.find(uniform.getStringHash());
            if (it != mUniformUBOLocByHash.end()) {
                const ubo::UniformLocation& loc = it->second;
                // (cadence_tag check は §5.2 と同様)
                forwardToUboUpload(loc, &x, sizeof(GLfloat));
                return;
            }
            // hash 未集約 = silent skip (= 集約表に entry 無し)
            return;
        }
#endif
        // OpenGL path 維持
        GLint location = getUniformLocation(uniform);
        if (location >= 0) {
            glUniform1f(location, x);
        }
    }
}
```

### §5.6 sampler 系 setter の OpenGL path 強制 (= 49 個)

sampler 系 uniform (= `diffuseMap` / `normalMap` / `shadowMap0-5` / `cloud_noise_texture` 等) は GLSL spec 上 UBO 化対象外。

本 chapter 06a での扱い:
- shader link 時 pre-cache で `cadence_tag = CADENCE_SAMPLER` を設定 (= Codegen-UBO pipeline 内で uniform 名 → sampler 判定、chapter 04 §7 で確定済の bare uniform 取込外集合に符号付与)
- setter 内分岐で `if (loc.cadence_tag == CADENCE_SAMPLER) return;` で UBO upload skip
- Vulkan path での **descriptor set 経由 binding** は chapter 07 (vulkan-api-state) で配線 (= 本 chapter scope 外)

= **本 chapter は sampler 49 個を「UBO upload 経路に乗せない」 path 分岐の挙動までを確定**、実 texture binding は chapter 07 譲り。

---

## §6 他 chapter との bridge 整理

| 譲り先 | 譲る内容 |
|---|---|
| **06b** (cadence-update-site-and-dirty) | `forwardToUboUpload(loc, data, size)` 本体実装 / cadence 別 update site 配線 (per-frame frame loop / per-program shader bind / per-draw renderGeom) / dirty 判定機構 (= `mValue` cache を UBO upload 側に乗せ替え + Material* per-draw dirty flag 統合) / ring buffer / dynamic offset / thread 配線 |
| **06c** (descriptor-set-bind-wiring) | set=0/1/2/3 帯 cadence 別 rebind タイミング / PSO compatibility / `mUseUBO` flag の決定方法 (= shader 種別自動判定 vs runtime cvar) / Vulkan side descriptor set layout 構成 |
| **chapter 07** (vulkan-api-state) | 実 UBO buffer 生成 / 解放 / VMA allocator / `vmaMapMemory` / `vkUpdateDescriptorSets` / sampler 系 49 個 descriptor set 経由 binding |
| **chapter 05a** (= bare-uniform-mapping 切出し doc、別 session で起案) | bare uniform 267 個の集約先 UBO 確定 / LLStaticHashedString 67 個のうち UBO 化対象有無 |

---

## §7 (H1b) LL_INFOS hook 提案

本 chapter (06a) は cache 構造 + setter Vulkan path 分岐 (= 設計 chapter) の scope に閉じる。06b (cadence-update-site-and-dirty) 起案前に **不明 16 件 cadence + per-program ↔ per-draw 境界** を実機 frame 内 setter call rate histogram で確証取得するため、`LLGLSLShader` setter 入口に LL_INFOS hook を 1 行追加する計測 build を提案する。

### §7.1 hook 目的

- 06a §0.2 cadence 推定表の不明 16 件確定
- per-program / per-draw 推定境界の verification (= caller 関数名のみ根拠の uniform を実 rate で再判定)
- dead uniform 検出 (= どの scenario でも call rate 0 の uniform)

### §7.2 hook 配線位置 (= 17 + 13 setter + frame counter)

| 対象 | file | 件数 | 詳細 |
|---|---|---|---|
| integer index 経由 setter | `indra/llrender/llglslshader.cpp` (line 2141-2538) | 17 method | `uniform1i` / `uniform1f` / `uniform2f` / ... / `uniformMatrix4fv` (= 06a §5.1 + `uniform1i`) |
| LLStaticHashedString 経由 setter | `indra/llrender/llglslshader.cpp` (line 2617-2844) | 13 method | `uniform1i` (LLStaticHashedString version) / ... / `uniformMatrix4fv` |
| frame counter increment | `indra/newview/llappviewer.cpp` の `LLAppViewer::idle()` 入口 | 1 箇所 | per-frame 1 回、render path 経由しない idle frame も拾う |

各 setter body 先頭 1 行に macro 挿入 (= `AYA_UBO_HOOK_IDX("setter_name")` / `AYA_UBO_HOOK_HASH("setter_name")`)。既存 body は **1 文字も改変しない** (= 検証完了後の hook 除去で diff が hook 行のみになる、commit ミス防止)。

### §7.3 build flag

CMake `option(AYASTORM_UBO_CADENCE_HOOK ... OFF)` で gate。AYA Linux build で `-DAYASTORM_UBO_CADENCE_HOOK=ON` 指定の計測専用 build を作る、通常 build には混入しない。

### §7.4 計測 scenario (= 3 種、cadence 軸網羅)

1. cold launch 直後 ~30 frame (= per-frame / shader bind 初期化検出)
2. GLTF avatar rez 完了後 ~30 frame (= per-asset / per-skin 検出)
3. shader-heavy scene 切替直後 ~30 frame (= per-program 切替頻度検出)

合計 ~90 frame ≈ 1.5 秒分 log = 数 MB 規模、解析可能。

### §7.5 完全 spec への pointer

本 §7 は 06a chapter scope 内の **提案部分** のみ。**実装仕様 / hook code shape / log format / 解析 protocol / 除去手順** の完全 spec は scope 軸が異なる (= 実装 phase 入口の手順書) ため、**`06a-prep-phase0-measurement.md` §2 に独立 doc として書面化済** (起案 2026-06-03、本 §7 提案を発展拡充)。実装 phase 入口で 06a-prep §2 を再読込して §7.2 の配線位置に従って実施する。

設計 chapter 群 (= 01-10) が全件起案完了するまで `indra/` 配下 program 改変を行わない原則 (memory `feedback_design_phase_no_code_write` 由来) は本 §7 配線位置記述には適用されない (= spec doc としての file 言及は設計書の本質)。program 改変は **実装 phase 入口で別 session として実施**。

---

## §8 chapter 04 / 05 / 06b / 06c との分担境界 (= 入出力契約)

| chapter | 本 chapter からの入力 | 本 chapter への出力 |
|---|---|---|
| 04 (codegen-ubo) | `lookup_runtime()` / `g_uniform_table[]` / `UniformLocation` struct / `cadence_tag` enum 値域 | (なし、本 chapter は受け側) |
| 05 (existing-inventory-link) | bare uniform 集約結果 (= 267 個の UBO mapping、切出し doc `05a-bare-uniform-mapping.md` で確定) | sampler 49 個 (§5.6) / LLStaticHashedString 67 個 (§4.3 / §5.5) を本 chapter scope 範疇として明示 |
| 06b (cadence-update-site-and-dirty) | `mUniformUBOLoc[index]` cache / `mUniformUBOLocByHash` / `mUseUBO` flag / `forwardToUboUpload(loc, data, size)` interface 呼出位置 | (本 chapter は提供側) |
| 06c (descriptor-set-bind-wiring) | `mUseUBO` 決定方法 / `cadence_tag` の descriptor set 帯 mapping | (本 chapter は受け側) |

---

## §9 未確定事項 (→ 他 chapter / Phase 0 後段で消化)

| # | 項目 | 解消先 |
|---|---|---|
| (H1b) | 不明 cadence 16 個 + per-program / per-draw 境界の実機確証 | **06b 起案前 AYA build run** (= §7) |
| (Q1) | `mUseUBO` flag の決定方法 (= shader 種別自動判定 vs runtime cvar 切替) | 06c (= chapter 07 接続) |
| (Q2) | `forwardToUboUpload(loc, data, size)` interface 詳細 (= signature / ring buffer / thread / dirty) | 06b |
| (R1) | LLStaticHashedString 67 個のうち UBO 化対象有無 (= 集約しないなら `mUniformUBOLocByHash` 空のまま) | 05a (= bare-uniform-mapping 切出し doc、別 session) |
| (S1-存在) | `LLStaticHashedString::getGlobalRegistry()` API 存在確認 = **不存在確認済** (= 06a-prep §2.2.2 grep 確認、解消マーク) | **解消済** (2026-06-03 Q24-S1 (A) 反映) |
| (S1-代替) | §4.3.1 代替案 S1-A/B/C/D のいずれを採用するか = AYA 判断仰ぎ事項 (= Q24-S1) | 06b / Phase 0 + chapter 10 §1 |
| (T1) | `glUniform4iv` setter が内部で `glUniform1iv` を呼んでいる bug 疑い (= inventory §4.3 line 2330) | 本 migration とは独立、別 bug fix |

---

## §10 本 chapter update 規律

- §3 cache 構造 / `mUniformUBOLoc` member 配置の変更は本 chapter に集約、06b / 06c が後追い参照
- §5 path 分岐 pattern (= `#ifdef LL_VULKAN_GLSL` + `if (mUseUBO)` + `cadence_tag` check) の追加 / 修正は本 chapter で確定
- §7 hook 配線位置の変更 / re-run 提案は本 chapter で update
- §9 持越 ((H1b) / (Q1) / (Q2) / (R1) / (S1-存在) / (S1-代替)) が他 chapter / Phase 0 で解消したら本 chapter から「保留候補」を剥がして reflect ((S1-存在) は既に解消済、(S1-代替) は Q24-S1 AYA 判断仰ぎ未消化)
- (T1) `uniform4iv` bug 疑いは本 migration scope 外、別軸 bug fix track で扱う

---

**= 本 chapter で setter redirect の cache 構造 + path 分岐 + shader link pre-cache が確定したため、06b (cadence-update-site-and-dirty) で `forwardToUboUpload(loc, data, size)` 本体実装 + cadence 別 update site + dirty 判定機構の実装に進める。06b 起案前に (H1b) LL_INFOS hook を AYA build run で消化する**。
