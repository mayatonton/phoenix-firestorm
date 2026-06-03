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

**並走計測**: sampler binding (= 49 個、`uniform1i` 経由) も同一 hook で独立 band として histogram 化する (= UBO cadence inventory への合算は §5 で行わない、§2.2.1 注を参照)。

### §2.2 配線位置 (= 全 31 setter + frame counter)

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

**注**: 06a §5.1 は 16 method を listing したが、本 doc では `uniform1i` (line 2141) を追加した **17 method 構成**。

**`uniform1i` の位置付け (= 設計 review 2026-06-03 §3.5 矛盾解消)**:
- `uniform1i` は sampler binding setter (= texture unit assignment) で **UBO 化対象外** (= chapter 06a §0.2 の 261 unique uniform inventory にも非合算)
- ただし sampler binding cadence も「shader bind 時のみ呼出」仮説を実測で確証する必要があるため、本 hook では **独立 band** (= §2.7 表の最終 band「sampler binding」枠) として histogram に並走計測する
- §5 解析時点で **sampler band は UBO inventory の cadence 結論には合算せず別表で報告**、chapter 05 §3 UBO 表の cadence 列には反映しない (= UBO 数値の汚染回避)
- inventory §2 への効果 = sampler 49 個の cadence band を別軸で確証 → texture binding 高速 path (= per-program cadence 維持 or per-draw 化が必要か) の Vulkan descriptor 化 (= chapter 07 §3 sampler 帯) の前提資料

#### §2.2.2 LLStaticHashedString 経由 setter (= 14 method、`llglslshader.cpp`)

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

**注**: 06a §0.2 は 67 unique LLStaticHashedString literal を確認したが、実装 hook では **uniform.String() を直接 log に書き出す** (= value type で string を保持、`llstaticstringtable.h:45`)。`getGlobalRegistry()` は本 doc 起案時点で存在しないこと確認済 (= 06a §9 **(S1-存在) のみ解消**、2026-06-03 Q24-S1 (A) 反映で 2 軸分割 = §4.3.1 代替案 S1-A/B/C/D の確定 = **(S1-代替)** は AYA 判断仰ぎ事項として未消化、Q24-S1 として chapter 10 §1 登録予定)。

#### §2.2.3 frame counter increment 配線

| 配線箇所 | 根拠 |
|---|---|
| `LLAppViewer::idle()` 入口 (= main loop top) | per-frame 1 回確定、render path 経由しない idle frame も拾える、shader bind ゼロの frame でも setter call が無いことを log に反映可能 |

候補対案 (= 不採用): `LLPipeline::renderGeom()` 入口 — render path 経由しない frame を取りこぼす可能性、本 hook は cadence 確証目的のため idle 含む方が安全。

### §2.3 hook code shape

#### §2.3.1 helper function 配置 (= `llglslshader.cpp` 上部、anonymous namespace 内)

**(2026-06-03 Phase 0 Step 1 update)**: §7 (P4) 解消結果 = 既存 `gFrameCount` (`U32` in `llappviewer.cpp:369` + `extern` in `llappviewer.h:422`) 流用に簡素化 (= 専用 counter `g_aya_ubo_hook_frame_counter` + §2.3.3 frame counter increment 配線は不要、setter 経路は現状 main thread 専有のため `gFrameCount` の `U32` 非 atomic で十分)。

**(2026-06-03 Phase 0 Step 2 update)**: 実装時に `#include "llappviewer.h"` は llrender → newview 上向き依存 = layering 違反 (= `libllrender.a` build TU から newview header 不可視) を検出。`extern U32 gFrameCount;` 直接宣言に置換 (= symbol は最終 viewer link 時に newview の `U32 gFrameCount = 0;` (`llappviewer.cpp:369`) で resolve、設計意図同等)。

```cpp
#ifdef AYASTORM_UBO_CADENCE_HOOK
extern U32 gFrameCount;  // declared in llappviewer.cpp:369, resolved at viewer link time
                          // (avoid #include "llappviewer.h" — would violate llrender → newview layering)

namespace {
    void ayaUboHookOnSetterByIndex(const char* setter_name, const LLGLSLShader* shader, U32 index)
    {
        const auto& reserved = LLShaderMgr::instance()->mReservedUniforms;
        const char* uniform_name = (index < reserved.size()) ? reserved[index].c_str() : "<oob>";
        LL_INFOS("UBO_CADENCE") << "frame=" << gFrameCount
            << " shader=" << (shader->mName.empty() ? "<unnamed>" : shader->mName.c_str())
            << " uniform=" << uniform_name
            << " setter=" << setter_name
            << " path=index"
            << LL_ENDL;
    }

    void ayaUboHookOnSetterByHashed(const char* setter_name, const LLGLSLShader* shader, const LLStaticHashedString& uniform)
    {
        LL_INFOS("UBO_CADENCE") << "frame=" << gFrameCount
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
- frame counter = 既存 `gFrameCount` (`U32`) を直接参照 (= §7 (P4) 解消、`std::atomic` 不要)
- `LL_INFOS` class 文字列 = `"UBO_CADENCE"` (= §7 (P1) で衝突 0 件確認済、実装時改 grep 不要)
- helper / macro / `extern U32 gFrameCount;` 宣言は `#ifdef AYASTORM_UBO_CADENCE_HOOK` で gate、release build には混入しない

#### §2.3.2 各 setter 入口への hook 挿入 (= 31 method)

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
- 31 method 全てに 1 行追加、引数 type による差異は macro 名 (`_IDX` / `_HASH`) のみ
- `mProgramObject` check より **前** に挿入 (= 0 program での setter call も log、anomaly 検知用)
- 既存 body は **1 文字も改変しない** (= 検証完了後の hook 除去で diff が hook 行のみになる、commit ミス防止)

#### §2.3.3 frame counter increment 配線 (= **2026-06-03 Phase 0 Step 1: 不要マーク**)

**結論**: §7 (P3)(P4) 解消結果 = 既存 `gFrameCount` (`llappviewer.cpp:369` で `LLAppViewer::idle()` 入口に既存 increment 配線あり) を §2.3.1 hook helper から直接参照 → 本 §2.3.3 の追加配線は **不要**。

**確認済 (Phase 0 Step 1 grep 結果)**:
- `gFrameCount` 定義: `indra/newview/llappviewer.cpp:369` (`U32 gFrameCount = 0;`)
- `gFrameCount` 公開: `indra/newview/llappviewer.h:422` (`extern U32 gFrameCount;`)
- increment 経路: `llappviewer.cpp` 内既存 main loop で frame 毎に自動 increment (= `gSimFrames = (F32)gFrameCount` 等、line 1344 / 1605 / 1864 / 6563 で読出済)
- 流用方針: §2.3.1 helper 内で `gFrameCount` を直接読出 = 配線追加ゼロ

**規律**:
- 本 §2.3.3 は **実装 phase で skip** (= AYA build 時 `llappviewer.cpp` への hook 編集なし)
- 実装 phase entry 時に「§2.3.3 不要、§2.3.1 helper に `extern U32 gFrameCount;` 直接宣言追加のみで完了」と確認 → §8 update 規律に従い本節を superseded mark せずそのまま保存 (= 経緯 archive)。当初は `#include "llappviewer.h"` 案だったが llrender → newview 上向き依存 = layering 違反のため `extern` 直接宣言に変更 (= 2026-06-03 Phase 0 Step 2 実装時検出、§2.3.1 で詳細)

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
| sampler binding (= UBO 化対象外、独立報告軸 = §2.2.1 注) | 5-50 / frame (= shader bind 時のみ) | 49 |
| 不明 16 個 (= 本 hook で確定) | 上記いずれかの band に分布 | 16 |

### §2.8 検証完了後の除去 protocol

memory `feedback_remove_verification_logs` 準拠:

1. log 解析完了 → cadence histogram 全件確定 → 06a §0.2 表 + chapter 05 §3 表 update
2. **commit 前に必ず除去**:
   - `llglslshader.cpp` から helper / 31 method の `AYA_UBO_HOOK_*` 行 / anonymous namespace 内 counter / macro define を全削除
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

### §3.5 確定情報 (= **2026-06-03 Phase 0 Step 1 Pre-hook Static Analysis 結果**)

#### §3.5.1 step 1-2: 全宣言 listing + binding / preprocessor gate 確認結果

| UBO 名 | 宣言 file:line | set / binding | std140 | preprocessor gate |
|---|---|---|---|---|
| `PerDrawUBO_ClipPlane` | `class3/deferred/reflectionProbeF.glsl:791` | set=2, binding=0 | ✓ | `#ifdef LL_VULKAN_GLSL` |
| `PerDrawUBO_ClipPlane` | `class3/deferred/softenLightF.glsl:90` | set=2, binding=0 | ✓ | `#ifdef LL_VULKAN_GLSL` |
| `PerDrawUBO_ClipPlane` | `class1/gltf/pbrmetallicroughnessF.glsl:164` | set=2, binding=0 | ✓ | `#ifdef LL_VULKAN_GLSL` |
| `PerDrawUBO_ClipPlane` | `class1/deferred/pbropaqueF.glsl:180` | set=2, binding=0 | ✓ | `#ifdef LL_VULKAN_GLSL` |
| `PerDrawUBO_ClipPlane` | `class1/deferred/globalF.glsl:45` | set=2, binding=0 | ✓ | `#ifdef LL_VULKAN_GLSL` |
| `PerDrawUBO_AvatarSkin` | `class1/avatar/avatarSkinV.glsl:45` | set=2, binding=0 | ✓ | `#ifdef LL_VULKAN_GLSL` |
| `PerDrawUBO_ObjectSkin` | `class1/avatar/objectSkinV.glsl:43` | set=2, binding=0 | ✓ | `#ifdef LL_VULKAN_GLSL` |
| `PerDrawUBO_SkinnedVelocity` | `class1/deferred/skinnedVelocityAlphaV.glsl:110` | set=2, binding=0 | ✓ | `#ifdef LL_VULKAN_GLSL` |
| `PerDrawUBO_SkinnedVelocity` | `class1/deferred/skinnedVelocityV.glsl:81` | set=2, binding=0 | ✓ | `#ifdef LL_VULKAN_GLSL` |
| `PerDrawUBO_AvatarVelocity` | `class1/deferred/avatarVelocityV.glsl:53` | set=2, binding=0 | ✓ | `#ifdef LL_VULKAN_GLSL` |

= 全 5 UBO 名 / 10 宣言行で **set=2, binding=0, std140 + `#ifdef LL_VULKAN_GLSL` gate** 確定。inventory §3.3.1 binding=0 listing は実観測一致 = **C 案 (Agent 抽出誤り) は反証**。

#### §3.5.2 step 3: C++ shader manager attach 確認結果

```
grep -rn "PerDrawUBO_\(ClipPlane\|SkinnedVelocity\|AvatarVelocity\|AvatarSkin\|ObjectSkin\)" indra/newview/llviewershadermgr.cpp indra/llrender/llglslshader.cpp
```
**→ 0 matches**

= **C++ 側で UBO 名による attach 配線は 0 件**。GLSL 宣言時の `layout(set=2, binding=0)` が binding 唯一の source of truth (= Vulkan-style 宣言)、`glUniformBlockBinding`-相当の rebind なし。

#### §3.5.3 shader file → program attach 確認結果

shader file 名で `llviewershadermgr.cpp` 内 grep:

| shader file | attach program / 配置 line | stage |
|---|---|---|
| `avatar/avatarSkinV.glsl` | 共通 shaders list line 857 (= 多 program に自動 attach) | V |
| `avatar/objectSkinV.glsl` | 共通 shaders list line 858 (= 同上) | V |
| `deferred/globalF.glsl` | 共通 shaders list line 963 | F |
| `deferred/reflectionProbeF.glsl` | 共通 shaders list line 968 | F |
| `deferred/pbropaqueF.glsl` | `gDeferredPBROpaqueProgram` line 1479 / `gHUDPBROpaqueProgram` line 1546 | F |
| `gltf/pbrmetallicroughnessF.glsl` | `gGLTFPBRMetallicRoughnessProgram` line 1502 | F |
| `deferred/softenLightF.glsl` | `gDeferredSoftenProgram` line 2233 | F |
| `deferred/skinnedVelocityV.glsl` | `gVelocitySkinnedProgram` line 3346 | V |
| `deferred/skinnedVelocityAlphaV.glsl` | `gVelocityAlphaSkinnedProgram` line 3378 | V |
| `deferred/avatarVelocityV.glsl` | `gAvatarVelocityProgram` line 3396 | V |

#### §3.5.4 同 program 内 V+F 共存 risk 分析

V stage の 4 UBO (AvatarSkin / ObjectSkin / SkinnedVelocity / AvatarVelocity) と F stage の ClipPlane が **同一 program に attach** されると、Vulkan link 時に set=2 binding=0 が **二重宣言** → link conflict 候補:

| V 側 attach 経路 | F 側 attach 経路 | 同 program で共存？ | conflict 候補 |
|---|---|---|---|
| `avatarSkinV.glsl` (共通 list) | `globalF.glsl` / `reflectionProbeF.glsl` (共通 list) | 高 (= 多 program で auto-attach) | 高 |
| `objectSkinV.glsl` (共通 list) | 同上 | 高 | 高 |
| `skinnedVelocityV.glsl` (`gVelocitySkinnedProgram`) | 同 program 内 F は velocity-F (= `globalF.glsl` 等は別 attach) | 要 program 内 V+F enumerate | 低-中 |
| `skinnedVelocityAlphaV.glsl` (`gVelocityAlphaSkinnedProgram`) | 同上 | 同上 | 低-中 |
| `avatarVelocityV.glsl` (`gAvatarVelocityProgram`) | 同上 | 同上 | 低-中 |

**現状**: `#ifdef LL_VULKAN_GLSL` は GL build で OFF = 全宣言 dormant = **未顕在化 dormant conflict** (= Phase 1.A で LL_VULKAN_GLSL 有効化時に link 失敗候補)。

#### §3.5.5 A/B/C 案 verdict (= 暫定)

- **B 案 (dead code)**: 全 UBO に実 attach 確定 = **反証**
- **C 案 (Agent 抽出誤り)**: §3.5.1 表で 全件実観測一致 = **反証**
- **A 案 (program 別 binding namespace 独立で binding=0 再割当)**:
  - GL spec / Vulkan spec で **同 program 内 set+binding は unique 必須** (= V/F stage は同 set+binding 空間共有)
  - → 純粋な A 案は Vulkan で成立しない
  - 成立条件 = 各 V+F 組合せが **異なる program** で binding=0 を独立使用 (= program 跨ぎ binding namespace は独立、program 内は unique)
  - §3.5.4 の共通 shaders list 経由 attach は high risk (= 多 program で V+F 共存疑い)

#### §3.5.6 持越事項 (= Phase 1.A 入口 task)

| # | item | 反映先 |
|---|---|---|
| (E')-1 | 共通 shaders list 経由 attach の全 program enumerate (= `LLViewerShaderMgr::loadBasicShaders()` / `loadShadersDeferred()` / `loadShadersObject()` 等 全 program の V+F shader file 一覧抽出) | Phase 1.A 入口 doc + chapter 06c descriptor set bind 配線 |
| (E')-2 | enumerate 結果と本 §3.5.3 表の cross-check で V+F 共存 program 確定 listing | Phase 1.A 入口 doc |
| (E')-3 | 共存検出 program に対する解決方針 (= binding ずらし / V 側 set 帯分離 / F 側 set 帯分離 / shader 分割) | **chapter 10 §1 (Q27-CONFL)** AYA 判断仰ぎ事項 (= 2026-06-03 Phase 0 Step 1 で新規登録済、26 → 27 件) |
| (E')-4 | 計測 hook (§2) で実機の uniform 名 × shader 名対応取得 → 同 shader (= 同 program) 内で複数 UBO 出現確認 | §2 hook 実行時並走 |

#### §3.5.7 inventory §3.3.1 補正方針

- 5 UBO 全件「binding=0 listing 確実」と確証 → inventory §3.3.1 は補正なし、**注記追加**: 「同 program V+F 共存 risk → §3.5.6 で持越」
- chapter 05 §3.3 set=2 帯 mapping = 5 UBO 全件登録継続、§3.5.4 risk を注記

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

### §4.6 確定情報 (= **2026-06-03 Phase 0 Step 1 Pre-hook Static Analysis 結果**)

#### §4.6.1 step 1: 全宣言 listing 結果

| UBO 名 | 宣言件数 | 出現 file (代表 + 件数) |
|---|---|---|
| `MaterialUBO` | **52 件** | 多数 (例: `class1/objects/simpleNoColorV.glsl:46`, `class1/deferred/materialV.glsl:68`, `class1/deferred/pbropaqueV.glsl:69` 等、全 52 件 `set=1, binding=0, std140` 統一) |
| `MaterialUBO_Legacy` | **1 件のみ** | `class3/deferred/materialF.glsl:38` (= 単独) |

= **圧倒的非対称** (52 vs 1)、inventory §3.2 の「2 UBO 名共存」記述は **共存ではなく Legacy 側 1 件のみ**。

C++ 参照 (`grep MaterialUBO_Legacy indra/`): **shader 宣言行 1 件のみ、C++ 配線なし** (= 純 GLSL 配線、attach は file 名経由)。

#### §4.6.2 step 2: member 直接 diff

**MaterialUBO** (`class1/objects/simpleNoColorV.glsl:46`, `class1/deferred/materialV.glsl:68` 共通):
```glsl
layout(set=1, binding=0, std140) uniform MaterialUBO {
    mat4  texture_matrix0;                  // 64 B
    vec4  texture_base_color_transform[2];  // 16×2 = 32 B
    vec4  texture_emissive_transform[2];    // 16×2 = 32 B
    vec4  color;                            // 16 B
    vec3  emissiveColor;                    // 12 B
    float _pad_emissive;                    //  4 B
};
// 総 size = 64 + 32 + 32 + 16 + 12 + 4 = 160 B (= std140 整列済)
```

**MaterialUBO_Legacy** (`class3/deferred/materialF.glsl:38`):
```glsl
layout(set=1, binding=0, std140) uniform MaterialUBO_Legacy {
    vec4  morphFactor;             // 16 B
    vec4  specular_color;          // 16 B
    vec3  camPosLocal;             // 12 B
    float emissive_brightness;     //  4 B
    float is_mirror;               //  4 B
    float env_intensity;           //  4 B
    float aya_sss_skin_flag;       //  4 B
    float _pad_material_legacy_0;  //  4 B
};
// 総 size = 16 + 16 + 12 + 4 + 16 = 64 B (= std140 整列済、vec3+float scalar 後の 4 float は同 16 B チャンク)
```

= **member 完全別物** (= 共通 member 0 件、type / 名前 / 順序 全て差異) → F1 統合 (member 同一) は **反証**。

#### §4.6.3 step 3: attach program 確認結果

```
grep -n "deferred/materialF\.glsl" indra/newview/llviewershadermgr.cpp
→ 1408: gDeferredMaterialProgram[i].mShaderFiles.push_back(make_pair("deferred/materialF.glsl", GL_FRAGMENT_SHADER));
```

`gDeferredMaterialProgram[i]` (`llviewershadermgr.cpp:1395-1408` 周辺):
- V shader = `deferred/materialV.glsl` (= 自動 class 解決、mShaderLevel に従い `class1/deferred/materialV.glsl` 等が選択)
- F shader = `deferred/materialF.glsl` (= 同上、mShaderLevel に従い `class1/` / `class3/` 等)
- mShaderLevel = `mShaderLevel[SHADER_DEFERRED]` (= graphics 設定 high で class3 選択)
- permutation = `HAS_NORMAL_MAP` / `HAS_SPECULAR_MAP` / `DIFFUSE_ALPHA_MODE`
- Skinned 変種 = `gDeferredMaterialProgram[i]` に skinning V を追加 attach (= `if (...) "Skinned Material Shader %d"`)

#### §4.6.4 同 program 内 set=1 binding=0 共存 risk 分析

`mShaderLevel[SHADER_DEFERRED] = class3` 設定時:
- V = `class1/deferred/materialV.glsl` (= MaterialUBO 宣言、§4.6.2 表)
- F = `class3/deferred/materialF.glsl` (= MaterialUBO_Legacy 宣言、§4.6.2 表)
- **両者とも `#ifdef LL_VULKAN_GLSL` gate + 同 `set=1, binding=0, std140`**
- → Vulkan link 時に set=1 binding=0 が **MaterialUBO + MaterialUBO_Legacy** 両 UBO で二重宣言 = **link 失敗候補**

`mShaderLevel[SHADER_DEFERRED] = class1/class2` 設定時:
- F = `class1/deferred/materialF.glsl` or `class2/deferred/materialF.glsl` (= MaterialUBO_Legacy 宣言なし、本 grep で確認)
- → 衝突回避

**現状**: `#ifdef LL_VULKAN_GLSL` は GL build で OFF = 全 UBO 宣言 dormant = **未顕在化 dormant conflict** (= Phase 1.A で LL_VULKAN_GLSL 有効化時、mShaderLevel=class3 で顕在化)。

#### §4.6.5 F1/F2/F3 verdict (= 確定)

| 案 | 判定 | 根拠 |
|---|---|---|
| F1 (member 同一 → 統合) | **反証** | §4.6.2 member 完全別物 |
| F2 (member 別物 → 別名分離) | **第一候補** | member 別物 + program 別シナリオ (= mShaderLevel 切替) で分離設計可能 |
| F3 (片方 dead) | **反証** | 両者とも実 attach 確認 (= `gDeferredMaterialProgram` の class3 path で MaterialUBO_Legacy 使用) |
| F3 重複版 (= 同 program 両 attach、GL spec 違反) | **顕在化候補** | mShaderLevel=class3 で MaterialUBO + Legacy が同 program 共存 (= Vulkan link 失敗) |

**verdict = F2 + 同 program 内構造改修要** (= MaterialUBO_Legacy を別 binding / 別 set 帯に逃がす、または mShaderLevel=class3 用 V shader を別途用意して MaterialUBO 不宣言にする等)。

#### §4.6.6 持越事項 (= Phase 1.A 入口 task)

| # | item | 反映先 |
|---|---|---|
| (F)-1 | 構造改修方針確定 (= rename + binding ずらし / class3 専用 V shader / 集約 UBO 案) | **chapter 10 §1 新規 (Q26-MUL) AYA 判断仰ぎ事項** |
| (F)-2 | `MaterialUBO_Legacy` → 確定名 rename (= 例: `MaterialUBO_Class3F_Legacy` 等の specific 名) | chapter 02 §3.2 |
| (F)-3 | `05-existing-inventory-link.md` §5.2 / §5.3 補正 | chapter 05 |
| (F)-4 | inventory §3.2 「2 UBO 名共存」記述補正 (= 「共存ではなく Legacy 側 1 件、ただし同 program V+F 共存 risk」へ) | inventory §3.2 |
| (F)-5 | 計測 hook (§2) で実機の mShaderLevel=class3 build 時 link 試行確認 (= 現状 GL build で dormant、Phase 1.A 入口の Vulkan build で初検出) | Phase 1.A 入口

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

### §5.5 観察結果 (= **2026-06-03 Phase 0 Step 4 = AYA Linux 実機計測 + Claude 解析**)

#### §5.5.1 計測条件 (= §5.1.2 単発 90 frame 案からの逸脱、AYA 指示)

- **2 run 分割 + 各 run 前 cache clear** protocol を採用 (= §5.1.2 単発 1 run 案を AYA 判断で差替)
  - 理由: 「２箇所とも一回キャッシュクリアしないと自分のアバター読み込みの処理負担が揃わない」(AYA、2026-06-03)
  - cache clear 対象 = `~/.ayastorm_x64/cache/` (texture cache + shader_cache 含む全 cache)
- 各 run = SLurl 直 login → 60 秒 rez 待機 → quit (= 60 sec 内の steady-state を観察)
- scenario 1 (= §2.6.1 cold launch) は単独 run せず、scenario 2/3 各 run の **frame=0 cold launch init phase** で兼ねる (= 全 shader binds が `gFrameCount==0` の間に発生、§5.5.2 で別観察)
- scenario 2 = Cocobolo Island/230/126/3004 (= **GLTF rez 主体**、projector light + PBR object 混在)
- scenario 3 = Roleplay Heaven/48/129/23 (= **water/atmosphere/shader-heavy 主体**)
- G3 体感計 (= §2.6.3) は ii 採用 (= 60 sec rez 待ち中の体感、定量計測不可)

#### §5.5.2 全体統計 (= 2 run aggregate)

| 項目 | scenario 2 (Cocobolo) | scenario 3 (Roleplay Heaven) |
|---|---|---|
| 総 frame 数 (run 開始から quit まで) | 922 | 740 |
| frame=0 cold launch init lines | ~1.3M (= 全 shader 初回 bind の dump、38 shader) | 同等規模 |
| 定常域 frame range | 100-915 (= **816 frame**) | 100-735 (= **636 frame**) |
| 定常域 UBO_CADENCE event 総数 | **16.2M** | **19.7M** |
| 定常域 unique shader 数 | 145 | 135 |
| 定常域 unique uniform 数 | **230** | **231** |
| 定常域 unique (shader, uniform, setter) tuple | 7,410 | 6,973 |

- 定常域認定方法: per-frame line count を 30-frame rolling window で平滑化、mean 変化 < 10% の連続区間を「定常」と判定 (`identify_rez_boundary.py`)
- 両 scenario 共通 = frame 100 前後で rez 完了 (= rolling mean 安定化)
- **union 237 uniform** (= s2 230 + s3 231 / 重複 224 / s2-only 6 / s3-only 7)
  - s2-only 6 = PBR/GLTF factor 系 (`baseColorFactors`, `emissiveColors`, `metallicFactors`, `roughnessFactors`, `terrain_texture_transforms`, `texture_base_color_transform`)
  - s3-only 7 = water/bump 系 (`bump_code`, `bumpyScaleX`, `bumpyScaleY`, `bumpyStepX`, `bumpyStepY`, `aya_fog_density`, `gltf_alpha_mode`)
  - = **scene complementarity 確認** (= GLTF vs water/atmosphere 系で uniform 群が補完関係)

#### §5.5.3 cadence band 観察 vs §0.2 推定 delta

| Band | §0.2 推定 (318 件分配) | 観察 (237 件分配) | delta 解釈 |
|---|---|---|---|
| per-draw | 68 | **72** | 推定とほぼ一致 (+4) |
| per-frame | 85 | 69 + per-frame-or-conditional 4 = **73** | 推定より少 (-12) = 一部 per-frame 推定 uniform が観察ゼロ (= dead) |
| per-program | 92 | 45 + per-program-narrow 24 = **69** | 推定より少 (-23) = 推定の一部が dead (= per-program 92 件のうち post-effect/terrain detail/cube map probe 系が観察 0) |
| per-asset | 45 | per-asset-or-conditional **18** | 推定より少 (-27) = sampler 系 (= `bumpMap` / `diffuseMap` 等) は LLStaticHashedString 経由設定だが本計測 hook は **uniform 値 set 時のみ trace**、texture bind (`glActiveTexture` + `glBindTexture`) は別 cadence、setter 経由しない sampler は観察対象外 |
| per-skin | 12 | **5** | -7 = 観察 5 件は `last_object_matrix` / `lastMatrixPalette` / `matrixPalette` / `glow_lod` / `texture_matrix0` (= **2 shader 限定**、cpf 200-500、narrow shader-set 高 rate path 確認) |
| 不明 (= §0.2 16 件) | 16 | hashed-path 40 件として観察 (§5.5.6) | 観察済、§5.5.6 で詳細 |

**解釈**: 318 reserved 中 121 件 (= 38%) は本 2 run で観察ゼロ (§5.5.5)。観察された 197 件 + hashed-path 40 件 = 237 件の cadence は spec 推定の主要 band (per-draw / per-frame / per-program 上位) を概ね支持する。per-asset (= sampler) は本 hook で観察不能 (= hook 対象は uniform 数値 setter 限定、texture bind は別経路)、推定値 45 件は spec doc 上で「sampler 系 = 別計測軸」と注記必要。

#### §5.5.4 scene-scaling 観察 (= s3/s2 rate 比 ≥ 2.5x)

scene 複雑度・shader 内容に応じて rate が大きく変化する uniform:

| uniform | s2 cpf | s3 cpf | scaling | 解釈 |
|---|---|---|---|---|
| `minimum_alpha` | 1003.71 | 2859.69 | 2.85x | per-draw 最高頻度、s3 alpha mask object 多 |
| `shadow_target_width` | 328.82 | 1512.17 | 4.60x | s3 shadow map sampling 集中 |
| `env_intensity` / `specular_color` | 259.57 | 1229.40 | 4.74x | s3 reflection probe heavy |
| `emissive_brightness` | 302.03 | 1322.89 | 4.38x | s3 emissive material 多 |
| `sun_up_factor` | 499.10 | 1697.57 | 3.40x | s3 atmospheric scattering shader 集中 |

**逆 scaling (s2 > s3、cocobolo の projector light + PBR factor 影響)**:

| uniform | s2 cpf | s3 cpf | scaling | 解釈 |
|---|---|---|---|---|
| projector 系 (`proj_origin`, `proj_p`, `proj_n`, etc.) | 高 | 低 (~0.3x) | < 0.5x | s2 projector light 多 |
| `baseColorFactor` / `emissiveColor` 等 PBR factor 系 | 中 | 0 (s3 dead) | inf 逆 | s2 GLTF object 集中 |

**意味**: cadence band 帰属は scene 不変だが、`cpf_overall` 値は scene-scaling factor に強く依存。**設計上は band 帰属が確定値、絶対 rate は scene 依存可変** と扱う。

#### §5.5.5 reserved 318 件中 **observed 0 件 = 121 件** (= dead candidate)

`indra/llrender/llshadermgr.cpp:1506-1904` で `mReservedUniforms.push_back()` 登録 318 件 - 観察 union 237 件 = **観察ゼロ 121 件** (= 38%)。

**category 別**:

| category | dead 数 | 例 |
|---|---|---|
| terrain `detail_*` PBR 6 ch 別 | 20 | `detail_0_base_color`, `detail_0_emissive`, `detail_0_metallic_roughness`, `detail_0_normal`, ..., `detail_3_*` (= terrain は 2 scene 共未踏 area) |
| post-effect sampler 系 | ~15 | `bloomMap`, `glowNoiseMap`, `color_grading_lut`, `noiseMap`, `edgesTex`, `blendTex`, `areaTex`, `exposureMap`, `halo_map` |
| atmospheric next-frame interp | 2 | `cloud_noise_texture`, `cloud_noise_texture_next` (= sky shader 別 path) |
| **`aya_*` AYAstorm 独自 3 件** | 3 | `aya_alpha_plate`, `aya_alpha_plate_enabled`, `aya_sss_skin_flag` (= **2 scene 共 dead = 該当 shader path 未踏 / または `LLStaticHashedString` 経由配線で本 hook 不通過 = 要確認**) |
| その他 sampler / debug / SMAA 段階別 | ~80 | `altDiffuseMap`, `border_color`, `border_thickness`, `debug_normal_draw_length`, etc. |

**要追跡 (= chapter 06b 起案前提に影響)**:
- terrain `detail_*` 20 件 = AYA テスト 2 scene が terrain 不在、別 scene (= PBR terrain region) で再計測必要か? → **chapter 10 残課題追記候補**
- `aya_*` 3 件 = AYAstorm 独自設定 path が dead か hash 経由か、grep 確認必要 → **chapter 10 残課題追記**
- post-effect / SMAA = scenario によっては観察可 (= 全 post-effect 有効時)、現 2 run は default post-effect state、scenario 1 (cold launch) と差別化しない

#### §5.5.6 LLStaticHashedString 経由 setter 40 件 (= `mReservedUniforms` 不在)

`mReservedUniforms` 318 件には不在だが本 hook で観察された uniform = 40 件 (= union 237 - 在 reserved 197):

**category**:
- `aya_*` 経由 hash setter = `aya_blur_dir`, `aya_blur_radius`, `aya_glow_color`, `aya_glow_gain`, `aya_strength`, `aya_translucency_params`, `aya_translucency_tint` (= **AYAstorm 独自 uniform は hash 経由設定**、§5.5.5 で dead 扱いした `aya_alpha_plate` 等とは別群)
- SMAA / CAS 関連 = `SMAA_RT_METRICS`, `cas_param_0`, `cas_param_1`
- 動的 exposure = `dynamic_exposure_params`, `dynamic_exposure_params2`, `diffuse_luminance_scale`
- 描画パス補助 = `above_water`, `bump_code`, `custom_alpha`, `delta`, `direction`, `dist_factor`, `dt`, etc.

= **§0.2 「不明 16 件」推定 → 実観察 40 件**。chapter 06b 起案時に「reserved 318 + hashed-path 40 = 計測対象 358 件」inventory として確定。

#### §5.5.7 per-program / per-draw 境界 verification (= §5.4 protocol 適用)

§5.4 規則で再確認:

| 推定 → 観察 | 件数 | 例 |
|---|---|---|
| per-program 推定 + 実 rate > 100 → per-draw 補正 | ~20 | `modelview_matrix` (437/688, 93/97 shader), `inv_modelview` 同、`modelview_projection_matrix` (375/619, 37/46 shader) = **matrix 系は per-draw 確定** |
| per-draw 推定 + 実 rate < 30 → per-program 補正 | ~10 | `gltf_alpha_mode` (s2-only, rate < 10), `texture_base_color_transform` (低 rate) |
| 推定一致 | ~140 件 | 大半が一致 |

**結論**: §0.2 表は概ね正確だが、matrix 系 (= `modelview_*` group 4-5 件) は per-program 推定 → per-draw に上書き必要。chapter 05 §7.3 への補正反映は chapter 06b 起案直前で実施。

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

## §7 未確定事項 (= **2026-06-03 Phase 0 Step 1 Pre-hook Static Analysis で全件解消**)

| # | 項目 | 解消結果 |
|---|---|---|
| (P1) | `LL_INFOS("UBO_CADENCE")` class 名衝突有無 | **解消 = 衝突 0 件** (`grep -rn "UBO_CADENCE" indra/` → 0 matches、`"UBO_CADENCE"` literal 安全採用) |
| (P2) | CMake patch 配置先 (`00-Common.cmake` vs `LLRender.cmake` 等) | **解消 = `indra/cmake/00-Common.cmake` 確定** (260 行、`option(LL_*)` pattern 0 件 = プロジェクトで `option()` block 自体は使用可、`LL_DULLAHAN_AUDIO_CALLBACK` は autobuild 経由 `CEFPlugin.cmake:13` で `if(...)` 参照のみ。AYASTORM_UBO_CADENCE_HOOK は `option()` を `00-Common.cmake` 末尾に追加、`-DAYASTORM_UBO_CADENCE_HOOK=ON` で AYA 計測 build 切替) |
| (P3) | frame counter 公開方式 (= `extern` 宣言 vs helper 関数 vs 既存 frame counter 流用) | **解消 = 既存 `gFrameCount` 流用** (= `extern U32 gFrameCount;` in `llappviewer.h:422`、§2.3.1 helper から `extern U32 gFrameCount;` 直接宣言で参照 = 2026-06-03 Phase 0 Step 2 実装時に当初の `#include "llappviewer.h"` 案を llrender → newview 上向き依存 layering 違反のため `extern` 宣言に変更) |
| (P4) | 既存 frame counter 流用可能性 (= `gFrameCount` 等が `llviewercontrol` / `llappviewer` に存在するか) | **解消 = 流用可能** (`U32 gFrameCount = 0;` in `llappviewer.cpp:369`、main loop で increment 配線済、`llappviewer.cpp:1344/1605/1864/6563` で読出経験あり = 安定 inventory) |

= 全 4 件解消、§2.3.1 / §2.3.3 / §2.4 への反映完了。実装 phase 入口時点で本 §7 は **追加 grep 不要**。

---

## §8 update 規律

- 本 doc は **実装 phase 入口で実施した計測結果を順次追記** する live doc
- §3.5 / §4.6 は実 grep 結果 + 判定で埋まる
- §2.8 除去 protocol は実装 phase で hook 削除完了時に「除去済」マーク
- §6 反映 flow が全行「反映済」になった時点で本 doc role 終了 = chapter 06b 起案前提整備完了マーク
- 反映済時点で本 doc を superseded mark せず、live reference として保存 (= 計測手順 + 結果 archive)

---

**= 本 doc で Phase 0 計測 (H1b)(E')(F) の全 spec が書面化された。実装 phase 入口で本 doc §2-§4 を手順書として実施 → §5 解析 → §6 反映 flow を駆動する。設計 chapter 群 (= 01-10) 起案完了後の実装 phase 入口で本 doc を再読込**。
