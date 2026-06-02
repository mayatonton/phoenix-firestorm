# r41 UBO 全体設計 Chapter 01: 全体概観 + 設計原則 + 用語定義

**起案日**: 2026-06-03
**位置付け**: AYAstorm r41 Vulkan 移行 (sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28) における **UBO 全体設計 doc 群の入口**。設計 chapter 01-NN は本 doc を pre-requisite として読まれる前提で書かれる。
**source of truth**: `ayastorm-r41-ubo-current-state-inventory.md` (= 現状棚卸し、live doc)

---

## §1 なぜこの設計 doc 群を起こすか

η-24 から η-28 Phase 2c までの 5 sub-step 期間中、Vulkan parse error が出た program に対して **その program 単独の新規 UBO を切る** 操作を 26 回反復した。結果として `set=2 binding 0-25` の 26 個が積み上がったが、棚卸し (inventory) で次の構造欠陥が判明:

1. **OpenGL 実働 UBO = 論理 binding 4 種 / 物理 instance 1+2N+M 個** (`UB_REFLECTION_PROBES` / `UB_GLTF_NODES` / `UB_GLTF_MATERIALS` / `UB_GLTF_JOINTS` の 4 種、scene 規模で動的)
2. **GLSL Vulkan blueprint = 84 個** だが、**host C++ 側 redirect 層が完全欠落** → 値が UBO に入らない dead 状態
3. **bare uniform setter (16 method)** は GL 直呼びのまま、Vulkan path 分岐ゼロ

= 「Vulkan parse が通った」≠「Vulkan 描画が成立する」。**Vulkan 描画を成立させるための再設計** が本 doc 群の目的。

---

## §2 2 大設計原則

本 doc 群の全 chapter は次の 2 原則に従う。原則と矛盾する設計案は、その場で reject する。

### §2.1 原則 1: Upstream OpenGL 取り込みやすさ維持 (= **call site API 温存**)

- `LLGLSLShader::uniform4fv("color", ...)` のような既存 call site を **改名・引数変更しない**
- upstream Firestorm/Linden の OpenGL path 改修を AYAstorm に取り込む際、**call site 差分が出ない** ことを最優先
- Vulkan path 分岐は call site の **下層** (= setter 内部) に閉じ込める
- 例外: 既存 call site が **構造的に Vulkan 不適合** な場合のみ (e.g., raw `glUniformBlockBinding` 直呼び) → その場合は wrapper を新設し、call site は wrapper 呼出に統一

### §2.2 原則 2: Core プロセス分散実現 (= **UBO / cmdbuf 並列化容易な設計**)

- UBO upload (memcpy) は **worker thread から並列実行可能** な構造に設計
- per-frame / per-program / per-draw のように **cadence 単位で分離** することで、cadence ごとに独立 thread / 独立 cmdbuf に乗せ替え可能にする
- cmdbuf record も将来的に core 分散可能な分離度を確保 (= UBO bind の論理 binding 単位での独立性を維持)
- 物理 buffer instance を持つ owner (`Asset` / `Skin` / `Manager`) は **owner 単位で並列化可能** な構造を温存

---

## §3 用語定義

設計 doc 群を通じて以下の語を **定義に従って厳密に使う**。曖昧な使用は chapter 内で発見次第訂正する。

### §3.1 storage lifetime (= UBO の存在期間、暗黙的)

- **owner class が生存している間、UBO buffer は存続する**
- `LLReflectionMapManager` (singleton): app 起動から終了まで
- `gltf::Asset` (per-Asset): asset 生成から破棄まで
- `gltf::Skin` (per-Skin): skin 生成から破棄まで
- = OpenGL では `glGenBuffers` / `glDeleteBuffers` で owner dtor が解放
- = Vulkan でも同じ owner 構造を維持 (VMA allocator で `vmaCreateBuffer` / `vmaDestroyBuffer`)
- **本軸では設計判断が発生しない** (既存所有関係を踏襲)。設計議論の主軸は §3.2 の cadence。

### §3.2 update cadence (= UBO の更新頻度、設計分類の主軸)

- per-frame: frame loop 内で必ず 1 回 update (e.g., `FrameViewProj`)
- per-program: shader program bind 時に 1 回 update (e.g., `MaterialUBO`)
- per-draw: draw call ごとに update (e.g., transform / light params)
- per-asset: GLTF asset state 変化時に update (e.g., `mNodesUBO` / `mMaterialsUBO`)
- per-skin: rigged animation 毎 frame update (e.g., `mUBO` joint palette)
- per-material: material 切替時に update (= per-program の細分、要検討)

**含意**:
- storage lifetime と update cadence は **独立軸**。同一 owner の同一 buffer に対して、update cadence は frame ごと/draw ごとに変動しうる
- 設計議論で「UBO の寿命」と書いた場合は **必ず cadence を指す** (storage lifetime は暗黙)

### §3.3 logical binding (= 論理 binding 種類)

- shader 内 `layout(set=N, binding=M) uniform <Name> { ... }` で参照される binding point の **種類数**
- host 側 `LLGLSLShader::UB_*` enum 値の数
- 現状 OpenGL path = **4 種** (`UB_REFLECTION_PROBES` / `UB_GLTF_NODES` / `UB_GLTF_MATERIALS` / `UB_GLTF_JOINTS`)
- 現状 GLSL Vulkan blueprint = **84 個** (set=0:3 / set=1:2 / set=2:25 / set=3:54)

### §3.4 physical buffer instance (= 物理 GL/VK buffer instance)

- 実際に `glGenBuffers` (OpenGL) / `vmaCreateBuffer` (Vulkan) で生成された buffer object の **総数**
- 現状 OpenGL path = **1 (singleton) + 2N (per-Asset) + M (per-Skin) 個**、scene 規模で動的変動
- 大規模 GLTF avatar sim では数十〜100 個オーダー
- 設計議論で「memory 削減 / upload cost 削減 / Core 分散単位」を語るときは **常に physical instance 軸**

### §3.5 logical binding ≠ physical instance (= 2 軸厳密区別)

inventory §6.6 で確定したルール。設計議論の含意:

- 「UBO を統合 / 分割するか」は **logical binding 軸** (= 寿命分類 / shader 宣言数 / call site refactor)
- 「memory / upload cost / Core 分散」は **physical instance 軸** (= owner class 数 / per-instance scaling)
- 両軸を混同すると「24 個の per-program UBO」 ↔ 「数千個の per-draw light instance」のような不当な比較が発生する

### §3.6 Codegen-UBO (= 本設計の中核アイデア)

- **build-time** に GLSL 中の `uniform <Name> { ... }` 宣言を pre-process して:
  - GLSL → そのまま UBO ブロックで出力 (Vulkan SPIR-V 化)
  - C++ → block 名 + member 名 + offset の lookup table を **静的生成**
- 既存 `LLGLSLShader::uniform4fv("color", ...)` の name-based API を **保持したまま**、lookup table 経由で UBO 内 offset に memcpy する
- = call site 改修ゼロで Vulkan UBO 化を実現 (原則 1 を満たす)
- = build-time 静的生成のため、起動時の動的解析・runtime reflection 不要
- 詳細は chapter 04 (codegen-ubo) で詳述

### §3.7 bare uniform / UBO blueprint

- **bare uniform**: GLSL 中で UBO ブロックの外に置かれた `uniform vec4 color` 等の独立宣言
  - OpenGL = `glUniform4fv` で値投入可能
  - Vulkan = **opaque type (sampler 系) 以外は宣言禁止** = bare uniform → UBO 化が **構造的に不可避**
- **UBO blueprint**: GLSL に `uniform <Name> { ... }` ブロックは宣言されているが、host C++ 側に bind 経路が無く、値が来ない状態
  - 現状 84 個の Vulkan blueprint が全件これ
  - chapter 06 (redirect-layer-design) で実体化経路を設計

---

## §4 本 doc 群の構成 (= 関連 doc map)

設計 doc は **複数 chapter に分割** されて `design/` 配下に置かれる (一本巨大 doc は更新時メンテ困難のため不採用)。

| chapter | doc 名 | 位置付け | 状態 |
|---|---|---|---|
| 01 | `01-overview.md` (本 doc) | 全体概観 + 設計原則 + 用語定義 | ✅ 起案済 |
| 02 | `02-naming-convention.md` | UBO 命名規則 / 接頭辞ルール / 互換性表記 | ✅ 起案済 |
| 03 | `03-cadence-classification.md` | cadence 分類体系 + cadence source rule | ✅ 起案済 |
| 04 | `04-codegen-ubo.md` | Codegen-UBO 全体機構 / pre-process pipeline | ✅ 起案済 |
| 05 | `05-existing-inventory-link.md` | 既存 84 UBO blueprint の cadence 別 mapping + bare uniform 集約対応表 | ✅ 起案済 |
| 06a | `06a-cache-structure-and-setter-redirect.md` | mUniformUBOLoc cache 構造 + 16 method setter Vulkan path 分岐 | ✅ 起案済 |
| 06a-prep | `06a-prep-phase0-measurement.md` | Phase 0 計測 spec (= (H1b) LL_INFOS hook / (E') binding 重複 grep / (F) MaterialUBO 比較 / 解析 spec)、実装 phase 入口の手順書 | ✅ 起案済 |
| 06b | `06b-cadence-update-site-and-dirty.md` | cadence 別 update site + dirty flag + flush timing | ✅ 起案済 |
| 06c | `06c-descriptor-set-bind-wiring.md` | descriptor set bind 配線 + UB_* binding 4 種 vs 84 blueprint 接合 | ✅ 起案済 |
| 07 | `07-vulkan-api-state.md` | 現状 Vulkan API 実装状況棚卸し (vkQueueSubmit / swapchain / descriptor 等) | 未起案 |
| 08 | `08-build-codegen-pipeline.md` | build system 統合 (CMake / glslang / preprocess script) | 未起案 |
| 09 | `09-phase-roadmap.md` | Phase 番号体系再編 + 1 UBO ずつ migration scope | 未起案 |
| 10 | `10-open-questions.md` | 未確定事項 / 次セッションへ持ち越し | 未起案 |

**関連外部 doc**:
- `../ayastorm-r41-ubo-current-state-inventory.md` (現状棚卸し、本設計の前提資料)
- `../reference-shader-location-map.md` (shader file 場所 reference)
- `../handoff/` (sub-step ごとの handoff doc 群、~97 件)

---

## §5 設計議論の決定済事項 (= 2026-06-03 session で確定)

| # | 決定 | 出典 | 影響 chapter |
|---|---|---|---|
| 1 | UBO 化アプローチは Codegen-UBO (build-time static codegen) | session 議論 | 04 / 06 / 08 |
| 2 | name-based call site API (`uniform4fv("color", ...)`) を温存 | 原則 1 | 06 |
| 3 | cadence source = 既存 C++ 呼出 path のスケジュール (新規 viewer settings 追加禁止) | session 議論 | 03 |
| 4 | per-frame cadence は既存 frame loop (60 FPS 設定) に sync | session 議論 | 03 |
| 5 | 既存 84 GLSL UBO blueprint は **discard しない** (parse error 解消の蓄積を温存) | AYA 指示 | 05 |
| 6 | storage lifetime は owner class lifetime に従う (新規判断不要) | session 議論 | 06 |
| 7 | 設計判断は **論理 binding 軸 / 物理 instance 軸の両軸で評価** | inventory §6.6 | 全 chapter |
| 8 | UBO migration は **1 UBO ずつ実装 → cold launch 検証 → 次へ** (大塊バッチ禁止) | feedback memory | 09 |
| 9 | Codegen は **GLSL を改変しない** (= read-only 入力、出力は C++ header のみ、std140 整合は build-time check) | AYA 判断 2026-06-03 | 04 / 08 |
| 10 | name → offset dispatch = **compile-time perfect hash** (= 全 uniform 名 build-time enumerate、衝突 0 build-time 保証) | AYA 判断 2026-06-03 | 04 / 06 |
| 11 | Codegen は **bare uniform を取り込まない** (= 集約は chapter 05 集約表が決定権者、Codegen は出力 UBO 層のみ処理) | AYA 判断 2026-06-03 | 04 / 05 / 06 |
| 12 | per-material cadence は **per-draw + dirty flag に統合** (= 独立軸として持たない、現状 `mValue` cache を Vulkan UBO upload の dirty 判定にそのまま継承、cadence 軸は 5 分類に縮約) | AYA 判断 2026-06-03 | 02 / 03 / 05 / 06 |
| 13 | set=2 vs set=3 統廃合方針は **E3 (rename だけ)** を採用 (= 79 UBO 独立保持、`<Name>UBO_Legacy` → `Program_<Name>` 機械的 rename、program 単位 grouping 温存、統合判定の再評価は chapter 09 後半 / chapter 10) | AYA 判断 2026-06-03 | 02 / 05 / 09 / 10 |

---

## §6 本 doc の更新規律

- 用語の追加 / 修正は本 chapter に集約 (他 chapter で新語を導入したら本 chapter にも反映)
- 原則の追加は AYA 確認後にのみ追記 (Claude 単独追加禁止)
- §4 chapter 進捗表は新 chapter 起案 / 完成のたびに update
- 廃止判断 (= 原則違反確定の chapter) はファイル削除でなく `[OBSOLETE]` prefix で記録残し

---

**= 本 chapter を pre-requisite として chapter 02 以降を読み進めること**。
