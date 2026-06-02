# r41 UBO 全体設計 Chapter 06a-prep: Phase 0 計測 spec

**起案日**: 2026-06-03
**位置付け**: 設計 chapter 群 (= 01-10) と並列に **Phase 0 計測軸** を独立 doc 化する preparation spec。本 doc は設計 chapter 全件起案完了後の **実装 phase 入口** で参照される手順書。設計起案中に program 改変を混入しないための分離措置 (= memory `feedback_design_phase_no_code_write` 由来)。
**pre-requisite**:
- `01-overview.md` (用語定義 + 2 大設計原則 + 確定事項表)
- `04-codegen-ubo.md` §6 (name-based 解決 dispatch)
- `05-existing-inventory-link.md` §3 / §5 / §7 (既存 inventory + bare uniform 集約表)
- `06a-cache-structure-and-setter-redirect.md` §0 (Phase 0 (D)(H1a) 結果) / §7 (hook 提案、本 doc へ pointer 化済)
- `ayastorm-r41-ubo-current-state-inventory.md` §3.3.1 / §7 (同一 binding 複数 UBO 名疑い / 残課題)

---

## §0 本 doc を起こすに至った経緯 (= scope 切出し根拠)

### §0.1 直接の経緯 (2026-06-03)

設計 chapter 06a 起案完了直後、Phase 0 (H1b) LL_INFOS hook の **実装方針** を AYA に提示。AYA 指摘:

> 「設計書書いてるのにプログラム書き換えるバカがいるとは思いませんでした。それを資料化すべきじゃないですか？」

= 設計 chapter 群 (01-10) の起案完了前に `indra/` 配下 program 改変を提案するのは、原則 (= 「議論先行・ファイル操作後行」「1 chapter ≈ 1 session」) 違反。Phase 0 計測も hook 仕様 / grep 手順 / 解析 protocol を含めて **全て spec doc 化** すべき、program 改変はその後に来る。

### §0.2 design-phase vs implementation-phase の分離

| phase | scope | 成果物 | 本 doc の関係 |
|---|---|---|---|
| design-phase (現在) | 設計 chapter 01-10 起案 + Phase 0 計測 spec 起案 | doc のみ (= `design/` 配下 + 本 doc) | 本 doc は design-phase の最終 deliverable 1 件 |
| implementation-phase (将来) | hook 実装 + AYA build run + log 解析 + (E')(F) grep 実施 + chapter 05 / 06a / 06b update | `indra/` 配下改変 + log 取得 + chapter live 表 update | 本 doc を実装 phase 入口で参照、書面通りに実施 |

= design-phase 完了判定 = `01-overview.md` §4 進捗表で **全 chapter (01-10) + 本 prep doc が起案済** になった時点。それまで `indra/` には触れない。

### §0.3 Phase 0 計測軸の独立性

| 計測 task | 軸 | doc 化先 |
|---|---|---|
| (D) shader 内動的 uniform 名存在確認 | static analysis (grep) | 06a §0.1 (= 結果記録、既に doc 化済) |
| (H1a) bare uniform C++ caller enumerate | static analysis (grep + 直 read) | 06a §0.2 (= 結果記録、既に doc 化済) |
| **(H1b) LL_INFOS hook cadence histogram** | runtime measurement (build + log) | **本 doc §2** |
| **(E') 同一 binding 複数 UBO 名疑い** | static analysis (grep + preprocessor gate 確認) | **本 doc §3** |
| **(F) MaterialUBO vs Legacy member 比較** | static analysis (member diff + attach grep) | **本 doc §4** |

(D) / (H1a) は **2026-06-03 本 session で消化済 = 06a §0 に結果記録**。(H1b) / (E') / (F) は実装 phase に実施、本 doc §2-§4 が手順仕様。

---

## §1 入出力契約

### §1.1 入力 source

| 入力 | 用途 |
|---|---|
| `06a-cache-structure-and-setter-redirect.md` §0.2 cadence 推定表 | §2 hook の不明 16 件 / per-program ↔ per-draw 境界 verification target |
| `05-existing-inventory-link.md` §7.3 集約表 framework | §5 解析で uniform 名 × cadence の集約先 mapping update target |
| `ayastorm-r41-ubo-current-state-inventory.md` §3.3.1 binding 重複疑い | §3 (E') grep の対象 5 UBO 名 source |
| `ayastorm-r41-ubo-current-state-inventory.md` §3.2 MaterialUBO 宣言例 | §4 (F) 比較の対象 file 2 件 source |
| `04-codegen-ubo.md` §6 dispatch 機構 | §5 解析結果が chapter 04 perfect hash table 入力に流れる契約点 |

### §1.2 出力

| 出力先 | 内容 |
|---|---|
| `06a-cache-structure-and-setter-redirect.md` §0.2 cadence 推定表 | 不明 16 件 → 確定 cadence に書き換え |
| `06a-cache-structure-and-setter-redirect.md` §9 持越 (H1b) | 消化済マーク |
| `05-existing-inventory-link.md` §3.3 set=2 帯 mapping | (E') 5 UBO の binding 帰属確定 → 補正 |
| `05-existing-inventory-link.md` §5 MaterialUBO 処遇 | (F) F1/F2/F3 のいずれか確定 |
| `05-existing-inventory-link.md` §7.3 集約表 | 267 uniform の cadence 列確定 → 別 file `05a-bare-uniform-mapping.md` 切出し準備完了 |
| `ayastorm-r41-ubo-current-state-inventory.md` §3.3.1 binding 重複疑い | A/B/C 案いずれか確定 |
| `ayastorm-r41-ubo-current-state-inventory.md` §7 残課題 #1 / #5 | 消化済マーク |
| **chapter 06b 起案 prerequisite** | 全 cadence 確定 = update site 5 種設計に進める state |

---

## §2 (H1b) LL_INFOS hook spec

### §2.1 目的

shader link 時 caller context grep (= 06a §0.2 Agent 解析) で得られた cadence 推定のうち **不明 16 件** + **per-program ↔ per-draw 境界 (= caller 関数名から判定不能な uniform)** を、実機 frame 内 setter call rate histogram で **確証取得** する。

### §2.2 配線位置 (= 全 30 setter + frame counter)

#### §2.2.1 integer index 経由 setter (= 17 method、`llglslshader.cpp`)

| method | line (HEAD 時点) | 引数 type |
|---|---|---|
| `uniform1i` | 2141 | (U32 index, GLint) |
| `uniform1f` | 2166 | (U32 index, GLfloat) |
| `fastUniform1f` | 2192 | (U32 index, GLfloat) |
| `uniform2f` | 2202 | (U32 index, GLfloat × 2) |
| `uniform3f` | 2229 | (U32 index, GLfloat × 3) |
| `uniform4f` | 2256 | (U32 index, GLfloat × 4) |
| `uniform1iv` | 2283 | (U32 index, U32 count, const GLint*) |
| `uniform4iv` | 2310 | 同上 |
| `uniform1fv` | 2338 | (U32 index, U32 count, const GLfloat*) |
| `uniform2fv` | 2365 | 同上 |
| `uniform3fv` | 2392 | 同上 |
| `uniform4fv` | 2419 | 同上 |
| `uniform4uiv` | 2447 | (U32 index, U32 count, const GLuint*) |
| `uniformMatrix2fv` | 2475 | (U32 index, U32 count, GLboolean, const GLfloat*) |
| `uniformMatrix3fv` | 2496 | 同上 |
| `uniformMatrix3x4fv` | 2517 | 同上 |
| `uniformMatrix4fv` | 2538 | 同上 |

**注**: 06a §5.1 は 16 method を listing したが、本 doc では `uniform1i` (line 2141) を追加した **17 method 構成**。`uniform1i` は sampler binding setter (= texture unit assignment) で UBO 化対象外だが、hook histogram で「sampler binding 呼出は per-program cadence で頻度低」を確証して inventory §2 cadence 推定と整合させる用途。

#### §2.2.2 LLStaticHashedString 経由 setter (= 13 method、`llglslshader.cpp`)

| method | line | 引数 type |
|---|---|---|
| `uniform1i` | 2617 | (const LLStaticHashedString&, GLint) |
| `uniform1iv` | 2634 | (const LLStaticHashedString&, U32 count, const GLint*) |
| `uniform4iv` | 2652 | 同上 |
| `uniform2i` | 2670 | (const LLStaticHashedString&, GLint × 2) |
| `uniform1f` | 2688 | (const LLStaticHashedString&, GLfloat) |
| `uniform2f` | 2705 | (const LLStaticHashedString&, GLfloat × 2) |
| `uniform3f` | 2723 | (const LLStaticHashedString&, GLfloat × 3) |
| `uniform4f` | 2740 | (const LLStaticHashedString&, GLfloat × 4) |
| `uniform1fv` | 2757 | (const LLStaticHashedString&, U32 count, const GLfloat*) |
| `uniform2fv` | 2774 | 同上 |
| `uniform3fv` | 2791 | 同上 |
| `uniform4fv` | 2808 | 同上 |
| `uniform4uiv` | 2826 | (const LLStaticHashedString&, U32 count, const GLuint*) |
| `uniformMatrix4fv` | 2844 | (const LLStaticHashedString&, U32 count, GLboolean, const GLfloat*) |

**注**: 06a §0.2 は 67 unique LLStaticHashedString literal を確認したが、実装 hook では **uniform.String() を直接 log に書き出す** (= value type で string を保持、`llstaticstringtable.h:45`)。`getGlobalRegistry()` は本 doc 起案時点で存在しないこと確認済 (= 06a §9 (S1) 解消)。

#### §2.2.3 frame counter increment 配線

| 配線箇所 | 根拠 |
|---|---|
| `LLAppViewer::idle()` 入口 (= main loop top) | per-frame 1 回確定、render path 経由しない idle frame も拾える、shader bind ゼロの frame でも setter call が無いことを log に反映可能 |

候補対案 (= 不採用): `LLPipeline::renderGeom()` 入口 — render path 経由しない frame を取りこぼす可能性、本 hook は cadence 確証目的のため idle 含む方が安全。

### §2.3 hook code shape

#### §2.3.1 helper function 配置 (= `llglslshader.cpp` 上部、anonymous namespace 内)

```cpp
#ifdef AYASTORM_UBO_CADENCE_HOOK
namespace {
    std::atomic<uint64_t> g_aya_ubo_hook_frame_counter{0};

    void ayaUboHookOnSetterByIndex(const char* setter_name, const LLGLSLShader* shader, U32 index)
    {
        const auto& reserved = LLShaderMgr::instance()->mReservedUniforms;
        const char* uniform_name = (index < reserved.size()) ? reserved[index].c_str() : "<oob>";
        LL_INFOS("UBO_CADENCE") << "frame=" << g_aya_ubo_hook_frame_counter.load()
            << " shader=" << (shader->mName.empty() ? "<unnamed>" : shader->mName.c_str())
            << " uniform=" << uniform_name
            << " setter=" << setter_name
            << " path=index"
            << LL_ENDL;
    }

    void ayaUboHookOnSetterByHashed(const char* setter_name, const LLGLSLShader* shader, const LLStaticHashedString& uniform)
    {
        LL_INFOS("UBO_CADENCE") << "frame=" << g_aya_ubo_hook_frame_counter.load()
            << " shader=" << (shader->mName.empty() ? "<unnamed>" : shader->mName.c_str())
            << " uniform=" << uniform.String().c_str()
            << " setter=" << setter_name
            << " path=hashed"
            << LL_ENDL;
    }
} // anonymous namespace
#define AYA_UBO_HOOK_IDX(name)    ayaUboHookOnSetterByIndex(name, this, index)
#define AYA_UBO_HOOK_HASH(name)   ayaUboHookOnSetterByHashed(name, this, uniform)
#else
#define AYA_UBO_HOOK_IDX(name)    ((void)0)
#define AYA_UBO_HOOK_HASH(name)   ((void)0)
#endif
```

**規律**:
- `g_aya_ubo_hook_frame_counter` は `std::atomic<uint64_t>` (= worker thread からの setter call も想定して thread-safe、現状 main thread 専有でも将来の core 分散時の安全網)
- `LL_INFOS` class 文字列 = `"UBO_CADENCE"` (= 既存 class 名と衝突しないこと、実装 phase 入口で `LL_INFOS\("UBO_CADENCE` を全 source grep して確認、衝突あれば `AYA_UBO_CADENCE` 等にずらす)
- helper / macro / counter は `#ifdef AYASTORM_UBO_CADENCE_HOOK` で gate、release build には混入しない

#### §2.3.2 各 setter 入口への hook 挿入 (= 30 method)

各 setter body の **先頭 1 行** に macro 挿入:

```cpp
// integer index 版 (例: uniform1f, line 2166):
void LLGLSLShader::uniform1f(U32 index, GLfloat x)
{
    AYA_UBO_HOOK_IDX("uniform1f");   // ← 追加 1 行
    if (mProgramObject) {
        // ... 既存 body
    }
}

// LLStaticHashedString 版 (例: uniform1f, line 2688):
void LLGLSLShader::uniform1f(const LLStaticHashedString& uniform, GLfloat v)
{
    AYA_UBO_HOOK_HASH("uniform1f");  // ← 追加 1 行
    GLint location = getUniformLocation(uniform);
    // ... 既存 body
}
```

**規律**:
- 30 method 全てに 1 行追加、引数 type による差異は macro 名 (`_IDX` / `_HASH`) のみ
- `mProgramObject` check より **前** に挿入 (= 0 program での setter call も log、anomaly 検知用)
- 既存 body は **1 文字も改変しない** (= 検証完了後の hook 除去で diff が hook 行のみになる、commit ミス防止)

#### §2.3.3 frame counter increment 配線

```cpp
// llappviewer.cpp の LLAppViewer::idle() 入口
bool LLAppViewer::idle()
{
#ifdef AYASTORM_UBO_CADENCE_HOOK
    ++g_aya_ubo_hook_frame_counter;
#endif
    // ... 既存 body
}
```

**規律**:
- `g_aya_ubo_hook_frame_counter` は `llglslshader.cpp` の anonymous namespace 内 = `llappviewer.cpp` から直接参照不可
- → `llglslshader.h` に `extern` 宣言追加 (= `namespace ayastorm_ubo_cadence_hook { extern std::atomic<uint64_t> g_frame_counter; }`)、または専用 helper `void ayaUboCadenceHookOnFrame()` を `llglslshader.h` に公開 → `llappviewer.cpp` から呼ぶ
- 推奨 = **helper 公開** (= internal counter を hide、ABI 安定)、実装 phase 入口で確定

### §2.4 build flag (CMake)

`AYASTORM_UBO_CADENCE_HOOK=1` を CMake で gate:

```cmake
# indra/cmake/00-Common.cmake or relevant build script
option(AYASTORM_UBO_CADENCE_HOOK "Enable r41 UBO cadence hook for Phase 0 measurement" OFF)
if(AYASTORM_UBO_CADENCE_HOOK)
    add_definitions(-DAYASTORM_UBO_CADENCE_HOOK=1)
endif()
```

**規律**:
- default OFF (= 通常 build には混入しない)
- AYA Linux build で `-DAYASTORM_UBO_CADENCE_HOOK=ON` 指定して計測専用 build を作る
- CMake patch 配置先 (`00-Common.cmake` vs `LLRender.cmake` 等) は実装 phase 入口で grep 確認、変更影響最小箇所を選択

### §2.5 log 出力仕様

#### §2.5.1 LL_INFOS format

```
INFO: UBO_CADENCE: frame=<N> shader=<shader_name> uniform=<uniform_name> setter=<setter_method> path=<index|hashed>
```

| field | 型 | 内容 |
|---|---|---|
| `frame` | uint64_t | LLAppViewer::idle() 入口 increment、cold launch 後 0 から開始 |
| `shader` | const char* | LLGLSLShader::mName (`llglslshader.h:336`)、空なら "<unnamed>" |
| `uniform` | const char* | integer index 版 = `mReservedUniforms[index]` literal / LLStaticHashedString 版 = `uniform.String()` |
| `setter` | const char* | 呼出された setter method 名 literal (= "uniform1f" / "uniformMatrix4fv" 等) |
| `path` | const char* | "index" / "hashed" (= 2 系統識別) |

#### §2.5.2 出力先

| OS | log path |
|---|---|
| Linux | `~/.ayastorm_x64/logs/AYAstorm.log` |
| (Windows / macOS は本計測 scope 外、Linux build で確認可能) | - |

### §2.6 計測 scenario (= 3 種、cadence 軸網羅)

| # | scenario | 目的 | 所要 frame | 撮影設定 |
|---|---|---|---|---|
| 1 | cold launch 直後 ~30 frame | shader bind 初期化 path / per-frame setter 検出 | 30 frame (= 0.5 秒 @60 FPS) | 起動直後の Welcome scene そのまま、avatar / GLTF rez 前 |
| 2 | GLTF avatar rez 完了後 ~30 frame | per-asset / per-skin setter 検出 / rigged animation 由来 setter 検出 | 30 frame | 自分の avatar 表示 + GLTF mesh attachment 1 件以上、SL 通常 scene (例 Aditi sandbox) |
| 3 | shader-heavy scene 切替直後 ~30 frame | per-program 切替頻度の hot path 検出 / reflection probe / postDeferred 系 setter 検出 | 30 frame | reflection probe 多い場所 (= 屋外、水面あり) で windlight 環境光切替 + 視点移動 |

**注**: scenario 1-3 連続 = 90 frame ≈ 1.5 秒分 log。log volume 想定 = setter call 1000/frame × 90 frame × 1 行 ≈ 90,000 行 = 数 MB、解析可能規模。

### §2.7 期待 histogram band

| cadence band | 想定 call rate (1 frame あたり / unique uniform 単位) | 該当 uniform 数 (06a §0.2 推定) |
|---|---|---|
| per-frame | > 50 / frame (= shader bind 数 × 1) | 85 |
| per-program | 5-50 / frame | 92 |
| per-draw | 100-5000 / frame | 68 |
| per-asset | 10-500 / frame (= rezzed asset 数次第) | 45 |
| per-skin | 5-100 / frame (avatar-heavy scene) | 12 |
| sampler binding (= UBO 化対象外) | 5-50 / frame (= shader bind 時のみ) | 49 |
| 不明 16 個 (= 本 hook で確定) | 上記いずれかの band に分布 | 16 |

### §2.8 検証完了後の除去 protocol

memory `feedback_remove_verification_logs` 準拠:

1. log 解析完了 → cadence histogram 全件確定 → 06a §0.2 表 + chapter 05 §3 表 update
2. **commit 前に必ず除去**:
   - `llglslshader.cpp` から helper / 30 method の `AYA_UBO_HOOK_*` 行 / anonymous namespace 内 counter / macro define を全削除
   - `llglslshader.h` から helper 公開宣言を削除
   - `llappviewer.cpp` から frame counter increment 行を削除
   - CMake 関連 `option()` + `add_definitions()` 行を削除
3. 削除後 `git diff` で `AYASTORM_UBO_CADENCE_HOOK` / `AYA_UBO_HOOK` / `g_aya_ubo_hook` / `UBO_CADENCE` の残存 0 件確認
4. 通常 build (= flag OFF) で build pass 確認 → 計測専用 build (= flag ON) は不要なので確認しない
5. migration commit へ進む

---

## §3 (E') 同一 binding 複数 UBO 名 grep spec

### §3.1 目的

inventory §3.3.1 で確認された **set=2 binding=0 候補に複数 UBO 名が並ぶ** 疑い:
- `PerDrawUBO_LightParams` (確定、η-3 起源、`deferredUtil.glsl:175`)
- `PerDrawUBO_ClipPlane` (未確定)
- `PerDrawUBO_SkinnedVelocity` (未確定)
- `PerDrawUBO_AvatarVelocity` (未確定)
- `PerDrawUBO_AvatarSkin` (未確定)
- `PerDrawUBO_ObjectSkin` (未確定)

3 候補:
- **A 案**: program 別 binding namespace 独立で binding=0 を再割当 (= GL/Vulkan の許される挙動)
- **B 案**: 起源 sub-step (η-3 / η-23) で複数 UBO に同一 binding を割当てたが後段で reorganize 漏れた dead code
- **C 案**: Agent 棚卸し grep が `PerDrawUBO_*` を一律 binding=0 として誤抽出

### §3.2 grep 手順

#### §3.2.1 step 1: 各 UBO 名の宣言行 + binding 値 grep

```
grep -rn "uniform.*\(PerDrawUBO_ClipPlane\|PerDrawUBO_SkinnedVelocity\|PerDrawUBO_AvatarVelocity\|PerDrawUBO_AvatarSkin\|PerDrawUBO_ObjectSkin\)" indra/newview/app_settings/shaders/
```

出力から:
- 宣言 file:line
- `layout(set=N, binding=M)` の N / M 実値
- preprocessor gate (= `#ifdef LL_VULKAN_GLSL` 内側か、別 gate か)

#### §3.2.2 step 2: preprocessor gate 詳細確認

各宣言行の **前後 30 行** を直 read で確認:
- `#if defined(...)` / `#ifdef LL_VULKAN_GLSL` / `#elif` 構造
- 同 file 内で別 `#ifdef` branch に別 binding 宣言があるか
- include されている side (例: `#include "deferredUtil.glsl"`) からの伝播か独立宣言か

#### §3.2.3 step 3: program 単位 attach 確認

```
grep -rn "PerDrawUBO_\(ClipPlane\|SkinnedVelocity\|AvatarVelocity\|AvatarSkin\|ObjectSkin\)" indra/newview/llviewershadermgr.cpp indra/llrender/llglslshader.cpp
```

= 該当 UBO を attach する program 名を確認。`addPermutation()` 呼出 / `mShaderFiles` 登録から program 識別。

### §3.3 判定基準

| 観測 | 結論 |
|---|---|
| 各 UBO が **別 program 専属** + binding=0 を再割当 | A 案確定 = program 別 namespace の意図的設計、inventory §3.3.1 を「program 別 namespace」と注記 |
| 一部 UBO が **どの program でも attach されない** | B 案確定 = dead code、`05-existing-inventory-link.md` §3 から削除 + GLSL 宣言行を migration 中に削除 |
| 全 UBO が **inventory §3.3.1 binding=0 listing 外** (= 実は binding=2/3/4...) | C 案確定 = Agent 抽出誤り、inventory §3.3.1 を実観測値に補正 |

### §3.4 出力先

| 出力 | 反映 |
|---|---|
| 5 UBO の確定 binding 値 / preprocessor gate / attach program 一覧 | 本 doc §3.5 に表として追記 |
| inventory §3.3.1 補正 | 観測結果で書き換え |
| `05-existing-inventory-link.md` §3.3 補正 | inventory 補正に追従 |

### §3.5 確定情報 (= 実装 phase 入口で埋める)

`(2026-06-03 起案時点: 空。実装 phase で §3.2 step 1-3 実施結果を埋める)`

---

## §4 (F) MaterialUBO vs MaterialUBO_Legacy 比較 spec

### §4.1 目的

inventory §3.2 で確認された **set=1 binding=0 に 2 UBO 名共存** の処遇確定:
- `MaterialUBO` (`class1/objects/simpleNoColorV.glsl:46` 他)
- `MaterialUBO_Legacy` (`class3/deferred/materialF.glsl:38`)

判定軸 (= `05-existing-inventory-link.md` §5.2):
- **F1**: member 同一 → 統合 + 新名 `MaterialPBR` / `_Legacy` 廃止
- **F2**: member 別物 → 別名分離 `MaterialPBR` / `MaterialLegacyBlinn`
- **F3**: 片方 dead → 削除

### §4.2 対象 file

| UBO 名 | 宣言 file:line |
|---|---|
| `MaterialUBO` | `indra/newview/app_settings/shaders/class1/objects/simpleNoColorV.glsl:46` (+ 他宣言行 — `MaterialUBO` で grep して全 file listing) |
| `MaterialUBO_Legacy` | `indra/newview/app_settings/shaders/class3/deferred/materialF.glsl:38` (+ 他宣言行 同様) |

### §4.3 比較手順

#### §4.3.1 step 1: 全宣言行 listing

```
grep -rn "uniform MaterialUBO" indra/newview/app_settings/shaders/
grep -rn "uniform MaterialUBO_Legacy" indra/newview/app_settings/shaders/
```

各 UBO の **全宣言 file:line** を取得 (= 同名 UBO が複数 file で再宣言されている可能性 = inventory §3.4 cloudsV/F の前例)。

#### §4.3.2 step 2: member 直接 diff

`simpleNoColorV.glsl:46` から `}` までを直 read + `materialF.glsl:38` から `}` までを直 read。

member 行を順次比較:
- type 一致 / member 名一致 → 同一
- 順序入れ替えあり → 別物 (std140 offset 異なる)
- type / 名前差異 → 別物

#### §4.3.3 step 3: program 単位 attach 確認

```
grep -rn "MaterialUBO\|MaterialUBO_Legacy" indra/newview/llviewershadermgr.cpp indra/llrender/llglslshader.cpp indra/newview/pipeline.cpp
```

attach program 一覧:
- 両 UBO が **同じ program に attach** されている → GL spec 違反、片方 dead で実害なし
- 別 program 専属 → F2 候補
- 片方が **どの program にも attach されない** → F3 候補

### §4.4 判定基準

| step 2 結果 | step 3 結果 | 結論 |
|---|---|---|
| member 同一 | 別 program 専属 | **F1** = 統合、新名 `MaterialPBR`、`_Legacy` 側削除 |
| member 別物 | 別 program 専属 | **F2** = 別名分離、`MaterialPBR` / `MaterialLegacyBlinn` |
| (任意) | 片方が dead | **F3** = dead 側削除、残側を `MaterialPBR` rename |
| 両 attach 重複 | (任意) | **F3 重複版** = GL spec 違反、片方を dead として削除 + bug report (= migration 範囲外) |

### §4.5 出力先

| 出力 | 反映 |
|---|---|
| F1/F2/F3 判定結果 + member diff 表 + attach program 一覧 | 本 doc §4.6 に追記 |
| `05-existing-inventory-link.md` §5.2 / §5.3 | 判定結果で書き換え (= 暫定名 `MaterialUBO` (暫定温存) → 確定名へ) |
| `02-naming-convention.md` §3.2 | 新名で表 update |

### §4.6 確定情報 (= 実装 phase 入口で埋める)

`(2026-06-03 起案時点: 空。実装 phase で §4.3 step 1-3 実施結果を埋める)`

---

## §5 解析 spec

### §5.1 log → uniform 名 × frame call count histogram 抽出

#### §5.1.1 抽出 command (= ad-hoc grep + 集計)

```
# 全 UBO_CADENCE 行抽出:
grep "UBO_CADENCE" ~/.ayastorm_x64/logs/AYAstorm.log > /tmp/aya_ubo_cadence_raw.log

# uniform 名別 frame call count:
grep "UBO_CADENCE" ~/.ayastorm_x64/logs/AYAstorm.log \
    | awk '{ for(i=1;i<=NF;i++) if($i ~ /^uniform=/) print substr($i,9) }' \
    | sort | uniq -c | sort -rn > /tmp/aya_ubo_uniform_counts.txt

# uniform 名 × frame 別 call count:
grep "UBO_CADENCE" ~/.ayastorm_x64/logs/AYAstorm.log \
    | awk '{ frame=""; uniform=""; for(i=1;i<=NF;i++) { if($i ~ /^frame=/) frame=substr($i,7); if($i ~ /^uniform=/) uniform=substr($i,9) } print frame"\t"uniform }' \
    | sort | uniq -c > /tmp/aya_ubo_per_frame_uniform.txt
```

#### §5.1.2 frame 期間ごとの分離

scenario 1-3 (= cold launch / GLTF rez / shader-heavy) を frame range で分離:
- scenario 1: frame 0-29
- scenario 2: frame 30-59 (= rez 完了 30 frame 経過後の連続 30 frame)
- scenario 3: frame 60-89

frame range は AYA build run 時に scenario 切替前後の frame 番号を記録、解析時に awk filter で範囲 grep。

### §5.2 cadence band 分類規則

各 uniform の **frame 内 call rate** = (該当 uniform の総 call 数) / (該当 scenario の frame 数 = 30)。

| call rate | cadence 結論 |
|---|---|
| ≥ 5000 / frame | per-draw (= 数千 draw call 級) |
| 100 ≤ rate < 5000 / frame | per-draw (= 中規模 scene) |
| 30 ≤ rate < 100 / frame | per-program (= shader bind 数 × 1) |
| 5 ≤ rate < 30 / frame | per-program (= 少数 shader bind) / per-asset (= 少数 asset) / per-skin (= 少数 rigged) |
| 1 ≤ rate < 5 / frame | per-frame (= 1 frame 1 回) |
| < 1 / frame (= 一部 frame でのみ呼出) | per-asset (= state 変化時のみ) / sampler (= shader bind 時のみ) |

**注**: 同一 uniform が複数 cadence で呼ばれる可能性あり (= 例: `modelview_matrix` は per-program で shader bind 時 + per-draw で transform 切替時)。scenario 別 rate 差で 2 cadence 共存を検出。

### §5.3 不明 16 件確定 protocol

06a §0.2 cadence 推定表の不明 16 件を以下手順で確定:

1. §5.1 抽出結果から 16 件各 uniform の scenario 別 call rate を取得
2. §5.2 規則で cadence band 判定
3. **2 候補 cadence にまたがる場合** = 06a §0.2 表に「primary cadence (= 最頻 band) + secondary cadence (= 副 band 注記)」で記録
4. **どの scenario でも call rate 0** = dead uniform 疑い → inventory §7 残課題に追記 (= migration 中の削除候補)

### §5.4 per-program ↔ per-draw 境界 verification

06a §0.2 推定表の per-program 92 件 + per-draw 68 件 = 160 件のうち、**境界 ambiguous な uniform** (= 推定根拠が caller 関数名のみ) を §5.1 抽出結果で再確認:

- per-program 推定 + 実 rate > 100 / frame → per-draw に補正
- per-draw 推定 + 実 rate < 30 / frame → per-program に補正
- 推定一致 → 確認済マーク

補正結果を 06a §0.2 表 + chapter 05 §7.3 集約表に反映。

---

## §6 chapter 05 / 06a / 06b への反映 flow

| 計測結果 | 反映先 chapter | 反映内容 |
|---|---|---|
| (H1b) 不明 16 件 cadence 確定 | 06a §0.2 cadence 推定表 | 不明行を確定 cadence に書き換え |
| (H1b) per-program ↔ per-draw 境界 verify | 06a §0.2 cadence 推定表 / chapter 05 §7.3 集約表 | 補正対象 uniform の cadence 列 update |
| (H1b) dead uniform 検出 | inventory §7 残課題 | 削除候補として追記、migration 中の削除対象 |
| (E') 5 UBO の binding 帰属確定 | inventory §3.3.1 / chapter 05 §3.3 | A/B/C 案いずれかの観測結果で書き換え |
| (F) MaterialUBO 処遇確定 | chapter 05 §5 / chapter 02 §3.2 | F1/F2/F3 結果で MaterialUBO 暫定名 → 確定名へ |
| 上記全件確定 | chapter 06b 起案前提整備完了 | cadence 5 種 update site 設計に進む state |

= 計測完了 → 反映完了 = 設計 doc 群が **chapter 06b 起案可能 state** に到達。

---

## §7 未確定事項

| # | 項目 | 解消先 |
|---|---|---|
| (P1) | `LL_INFOS("UBO_CADENCE")` class 名衝突有無 | 実装 phase 入口 grep |
| (P2) | CMake patch 配置先 (`00-Common.cmake` vs `LLRender.cmake` 等) | 実装 phase 入口 grep |
| (P3) | frame counter 公開方式 (= `extern` 宣言 vs helper 関数 vs 既存 frame counter 流用) | 実装 phase 入口 |
| (P4) | 既存 frame counter 流用可能性 (= `gFrameCount` 等が `llviewercontrol` / `llappviewer` に存在するか) | 実装 phase 入口 grep、流用可能ならそちらを優先 |

---

## §8 update 規律

- 本 doc は **実装 phase 入口で実施した計測結果を順次追記** する live doc
- §3.5 / §4.6 は実 grep 結果 + 判定で埋まる
- §2.8 除去 protocol は実装 phase で hook 削除完了時に「除去済」マーク
- §6 反映 flow が全行「反映済」になった時点で本 doc role 終了 = chapter 06b 起案前提整備完了マーク
- 反映済時点で本 doc を superseded mark せず、live reference として保存 (= 計測手順 + 結果 archive)

---

**= 本 doc で Phase 0 計測 (H1b)(E')(F) の全 spec が書面化された。実装 phase 入口で本 doc §2-§4 を手順書として実施 → §5 解析 → §6 反映 flow を駆動する。設計 chapter 群 (= 01-10) 起案完了後の実装 phase 入口で本 doc を再読込**。
