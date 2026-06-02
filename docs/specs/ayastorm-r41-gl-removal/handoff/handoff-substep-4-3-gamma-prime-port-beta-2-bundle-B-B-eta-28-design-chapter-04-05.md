# sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 handoff: 設計 chapter 04-05 起案 + 判断 5 件確定

**完了日**: 2026-06-03
**位置付け**: 前 handoff (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-doc-split.md`、chapter 01-03 起案完了) を受けて、本 session で **chapter 04 (Codegen-UBO 機構) + chapter 05 (existing-inventory-link)** を起案し、判断 (A)(B)(C)(E)(G) 5 件を AYA 確認で確定した state。次 session で chapter 06 (redirect-layer-design) 起案に進む入口 handoff。

---

## §0 本 session で完了した作業

### §0.1 clean-up

inventory doc (= `ayastorm-r41-ubo-current-state-inventory.md`) 内 path 参照補正 2 箇所:
- 冒頭 `**起源 handoff**:` → `handoff/handoff-substep-...` に prefix 付与
- §A.3 参照 handoff doc 2 件 → 同様に prefix 付与

(reference-shader-location-map.md は同階層のため変更なし)

### §0.2 設計 chapter 起案

| chapter | file | 規模 | 主内容 |
|---|---|---|---|
| 04 | `design/04-codegen-ubo.md` | 約 240 行、§1-§11 | Codegen-UBO 機構: 3 stage pipeline / GLSL read-only 入力 / std140 整合 build-time check / 生成物 4 ファイル / compile-time perfect hash / R3 (`mUniform[index]` 経由) 最速 path / bare uniform 取り込み外 |
| 05 | `design/05-existing-inventory-link.md` | 約 270 行、§1-§11 | 既存 84 UBO の cadence 別 mapping / set=2+set=3 統廃合方針 / MaterialUBO 処遇 / per-material cadence 判定 / bare uniform 集約対応表 framework / GLSL 改変規律 / build-time check |

### §0.3 AYA 判断確定 5 件 (= chapter 01 §5 #9-#13 と同一)

| # | 判断 | 出典 chapter |
|---|---|---|
| (A) | Codegen は **GLSL を改変しない** (= read-only 入力、出力は C++ header のみ、std140 整合は build-time check) | 04 §3 |
| (B) | name → offset dispatch = **compile-time perfect hash** (= 全 uniform 名 build-time enumerate、衝突 0 build-time 保証) | 04 §5.3 / §6 |
| (C) | Codegen は **bare uniform を取り込まない** (= 集約は chapter 05 集約表が決定権者、Codegen は出力 UBO 層のみ処理) | 04 §7 |
| (E) | set=2 vs set=3 統廃合 = **E3 (rename だけ)** (= 79 UBO 独立保持、program 単位 grouping 温存、統合判定再評価は chapter 09 後半 / 10) | 05 §4.3 / 02 §3.4 |
| (G) | per-material cadence = **G1 (per-draw + dirty flag 統合)** (= 独立軸として持たない、`mValue` cache を Vulkan UBO upload 側 dirty 判定にそのまま継承、cadence 軸 5 分類縮約) | 05 §6.3 / 03 §2 |

### §0.4 判断 5 件の波及 reflect (= 整合 update)

| update 先 | 内容 | 状態 |
|---|---|---|
| chapter 01 §4 (進捗表) | chapter 04 / 05 を ✅ 起案済 | unstaged |
| chapter 01 §5 (確定済事項) | #9-#13 として 5 件追加 | unstaged |
| chapter 02 §2.1 (命名規則表) | per-material 行 cadence 列を「per-draw (material dirty flag)」に変更、prefix `Material` は温存 | unstaged |
| chapter 02 §3.4 (set=3 rename 表) | E3 確定として全件 `<Name>UBO_Legacy` → `Program_<Name>` 機械的 rename に書き換え、代表 6 件 + 残 48 件は inventory 参照 | unstaged |
| chapter 03 §2 (cadence 表) | 6 → **5 分類** に縮約、注を G1 確定根拠 (現状動作との連続性) で書き換え | unstaged |
| chapter 03 §4.3 (per-draw cadence 表) | dirty 判定行に「material 切替は本 cadence の dirty flag で吸収 (= `mValue` cache 機構継承)」を追記 | unstaged |
| chapter 04 §10 / chapter 05 §10 | 持越 (E) / (G) を解消マーク | 起案時から確定 |

---

## §1 次 session の作業 (= chapter 06 起案)

### §1.1 chapter 06 (redirect-layer-design) scope

| 項目 | 詳細 |
|---|---|
| runtime name 解決 | chapter 04 §6.2 の `mUniformUBOLoc[index]` cache 構造実装、shader link 時 pre-cache の流れ |
| 16 method setter family の Vulkan path 分岐 | `LLGLSLShader::uniform*fv()` 16 method (inventory §4.3) に redirect 配線 (`#ifdef LL_VULKAN_GLSL` / `if (mUseUBO)` 等) |
| cadence 別 update site 実装 | per-frame / per-program / per-draw / per-asset / per-skin 5 分類それぞれの upload 位置と thread |
| dirty 判定機構 | 既存 `mValue` cache を UBO upload 側に乗せ替え、Material* も per-draw 内 dirty flag で吸収 (= G1 確定) |
| descriptor set bind 配線 | set=0/1/2/3 帯の cadence 別 rebind タイミング (Vulkan 詳細は chapter 07 と接続) |

### §1.2 chapter 06 起案前の prerequisite = Phase 0 計測 task

chapter 06 §1 を起案する前に、以下 4 件の grep / hook で実データを取得する必要あり (= chapter 05 §10 残持越のうち chapter 06 直接依存分):

| # | 計測 task | 手段 | 出力先 |
|---|---|---|---|
| (H1) | **bare uniform 集合の完全 enumerate** | (a) `LLGLSLShader::uniform*fv()` 16 method の全 caller を grep / (b) shader link 時 `mUniform[]` index → uniform 名 mapping を LL_INFOS hook で dump | chapter 05 §7.3 集約表本体 |
| (D) | shader 内 **動的 uniform 名** (array flatten 等) の存在確認 | GLSL preprocess 後 (= glslang -E) の output で `[i]` 等の動的 index が flatten されているか直接確認 | chapter 06 で perfect hash 事前 enumerate vs local fallback 設計の判断材料 |
| (E') | inventory §3.3.1 **同一 binding 複数 UBO 名疑い** | `PerDrawUBO_ClipPlane` / `SkinnedVelocity` / `AvatarVelocity` / `AvatarSkin` / `ObjectSkin` の GLSL 宣言行 + preprocessor gate を grep + program 単位 attach 確認 | inventory §3.3.1 / chapter 05 §3.3 補正 (A 案 program 別 namespace / B 案 dead code / C 案 Agent 抽出誤りのどれかを確定) |
| (F) | **MaterialUBO vs MaterialUBO_Legacy member 比較** | 両 UBO 宣言 (`class1/objects/simpleNoColorV.glsl:46` 他 / `class3/deferred/materialF.glsl:38`) member を直接 diff | chapter 05 §5 F1 統合 / F2 別名分離 / F3 廃止のどれか確定 |

これらは **chapter 06 起案の前段 grep session として 1 session 程度** で消化可能。chapter 06 起案中に同 session 内で実施する流れも可。

### §1.3 chapter 06 起案で AYA 判断を仰ぐ可能性のある論点 (= 暫定)

| # | 論点 | 影響 |
|---|---|---|
| (K) | dirty 判定の粒度 = member 単位 / UBO 単位 / cadence 単位 | upload 粒度 / overhead トレードオフ |
| (L) | per-draw cadence の Vulkan 最適化 = ring buffer / dynamic offset / sub-allocation | 数百〜数千 / frame の upload を Vulkan で捌く方式 |
| (M) | descriptor set 4 帯 (set=0/1/2/3) の Vulkan 側 bind 戦略 | chapter 07 (vulkan-api-state) と直接接続、PSO compatibility 設計 |

chapter 04 (A)(B)(C) / chapter 05 (E)(G) と同様、起案時に Claude 推奨 + AYA 判断境界として提示する想定。

### §1.4 次 session 着手前の前提読み込み (= pre-requisite)

順序固定:

1. `design/01-overview.md` (用語定義 + 設計原則 + 本 session 含む確定事項 13 件)
2. `design/02-naming-convention.md` (命名規則 + rename 表)
3. `design/03-cadence-classification.md` (cadence 5 分類)
4. `design/04-codegen-ubo.md` (Codegen-UBO 機構、本 session 起案)
5. `design/05-existing-inventory-link.md` (既存 inventory link + 集約 framework、本 session 起案)
6. `ayastorm-r41-ubo-current-state-inventory.md` (現状棚卸し、live doc)
7. 本 handoff doc (本 session 完了状態 + chapter 06 scope + Phase 0 計測 task)

**= この 7 件で次 session の文脈は完全 reconstruct 可能**。 chapter 06 起案中に過去 handoff doc (97 件) を 1 件ずつ参照する必要は無い (chapter 01-05 が全集約)。

### §1.5 次 session 推奨進め方

- chapter 06 起案前に Phase 0 計測 task (H1)(D)(E')(F) を消化、または起案中に並行
- chapter 06 は redirect 層の最大規模、必要なら **sub-chapter に分割** (`06a-cache-structure.md` / `06b-cadence-update-site.md` 等)
- 起案中の論点 (K)(L)(M) は chapter 04 / 05 と同じ「Claude 推奨 + AYA 判断仰ぎ」パターンで進行
- 1 chapter ≈ 1 session で context を 1 サイクル分使うため、適時 `/clear`

---

## §2 残持越 item (= 全 chapter の §10 / 本 handoff からの累積)

### §2.1 chapter 04 §10 持越

| # | 項目 | 解消先 |
|---|---|---|
| (A1) | std140 offset 計算: **Codegen 独自 calculator vs SPIR-V reflection 抽出** | chapter 08 |
| (G_codegen) | perfect hash generator: gperf / 独自 / frozen | chapter 08 |
| (D) | shader 内 **動的 uniform 名**存在確認 | chapter 06 起案時 grep (= §1.2) |
| (P) | parse 手段: 独自 mini-parser vs glslang reflection | chapter 08 |

### §2.2 chapter 05 §10 持越

| # | 項目 | 解消先 |
|---|---|---|
| (E') | inventory §3.3.1 同一 binding 複数 UBO 名疑い | chapter 06 起案前 grep (= §1.2) |
| (F) | MaterialUBO vs MaterialUBO_Legacy 処遇 | chapter 06 起案前 grep (= §1.2) |
| (H1) | bare uniform 集合の完全 enumerate | chapter 06 起案前 grep + LL_INFOS hook (= §1.2) |
| (H2) | 集約表の owner (inline / 別 file 切出し) | 表 size > 50 行で再判定 |
| (H3) | 集約判定 conflict 時の AYA 判断ループ | chapter 09 Phase 進行中に case-by-case |

### §2.3 前 session 持越 (= `design-doc-split.md` §2) の本 session 消化状態

| # | 項目 | 状態 |
|---|---|---|
| A | set=2 内 binding 重複疑い | → 本 session (E') として残持越、chapter 06 直前で消化 |
| B | set=2 vs set=3 統廃合 | ✅ 本 session (E) で E3 確定 |
| C | MaterialUBO vs MaterialUBO_Legacy attach 排他 | → 本 session (F) として残持越、chapter 06 直前で消化 |
| D | 典型 scene N (rezzed GLTF) / M (rigged Skin) 計測 | chapter 09 Phase 0 |
| E | Vulkan vkQueueSubmit / swapchain 現状実装 | chapter 07 |
| F | redirect 層 dirty 判定方式 | chapter 06 (= §1.3 (K)) |
| G | per-material cadence 統合判断 | ✅ 本 session (G) で G1 確定 |
| H | bare uniform → UBO 集約粒度 | ✅ 本 session chapter 05 §7 で framework 確定、表本体は live |
| I | Phase 2d-α 適用済 commit 群 (`0587c574da` 等) 処遇 | chapter 09 |
| J | upstream Firestorm UBO blueprint diff 検証 | chapter 09 開始前 |

**= 前 session 持越 10 件のうち、本 session で 3 件 (B / G / H) が確定、2 件 (A → E' / C → F) は名前変えで継続持越、5 件は他 chapter への配分継続**。

---

## §3 unstaged / untracked 状態 (= AYA 判断対象)

### §3.1 累積状態

| 種別 | 内訳 |
|---|---|
| staged (前 session) | `git mv` で `handoff/` 配下に移動した 97 件 |
| unstaged (前 session + 本 session) | inventory doc 8 箇所 update (前 6 + 本 2) / chapter 01 (§4 / §5) / chapter 02 (§2.1 / §3.4) / chapter 03 (§2 / §4.3) |
| untracked (前 session + 本 session) | design/ 配下: 01-overview.md / 02-naming-convention.md / 03-cadence-classification.md / 04-codegen-ubo.md / 05-existing-inventory-link.md / handoff/ 配下: 前 session handoff doc 1 件 + 本 handoff doc 1 件 |

### §3.2 commit / push / clear 判断は AYA 側

memory `feedback_no_auto_commit` + 前 session で AYA 「commit とかいまもういいよ」指示継続。Claude 側から proactive に commit 提案しない。

push / session clear / Phase 0 計測 task の実施 session タイミング も AYA 側で判断。次 session 開始時は §1.4 の 7 件読込から再開可能。

---

## §4 本 session の感触 (= 設計議論結果の要約)

1. **chapter 04 起案で Codegen-UBO の機構が確定** = upstream OpenGL 取込互換 (原則 1) + Core 分散容易性 (原則 2) の両方を満たす「build-time 静的解決 + compile-time perfect hash + name-based call site API 完全温存」が固まった。R3 (`mUniform[index]` 経由) を最速 path に据えることで原則 1 完全達成
2. **chapter 05 で既存 84 blueprint の最終配置と bare uniform 集約 framework が確定** = Codegen 入力契約と redirect 層への引継ぎ点が明確に。集約表本体は live で migration 進行中に埋める運用
3. **判断 5 件 (A)(B)(C)(E)(G) が連続して固まった** = AYA 質問「per-material って何」「現状動作にいちばん近いのは?」のように、技術的根拠を明示してから判断確定する形が機能。memory `feedback_explanation_lead_with_conclusion` の効用が大きい
4. **(G) G1 確定の根拠が「現状動作にいちばん近い」軸** = 原則 1 (call site 温存) と現状動作再現が同じ方向を指していることが確認された。これは今後の判断軸として再利用可能
5. **Phase 0 計測 task (H1)(D)(E')(F) 4 件が chapter 06 起案前の prerequisite として整理された** = grep + LL_INFOS hook の 1 session で消化可能な粒度、chapter 06 起案と並行 / 前段どちらでも可
6. **設計 doc 群が 5 chapter (01-05) に達した** = chapter 01 §4 進捗表は 5/10 (50%) 完成。残り chapter 06-10 のうち chapter 06 が最大規模、その後は chapter 07-10 で短めに収束する見込み

---

## §5 本 handoff doc の更新規律

- 次 session で本 doc を入口として読み込む際、§1.2 Phase 0 計測 task の消化状況を本 doc に上書き反映 (= 計測完了したら結果を chapter 05 §7.3 集約表 / inventory §3.3.1 / chapter 05 §5 に流し込み)
- chapter 06 起案完了時は本 doc を superseded mark、新 handoff doc (`...-design-chapter-06.md` 等) に引継ぎ
- §3 unstaged / untracked 状態は AYA の commit 判断に追従して更新

---

**= 本 handoff doc を次 session の入口として、Phase 0 計測 task (§1.2) 消化 → chapter 06 起案の流れで再開する**。
