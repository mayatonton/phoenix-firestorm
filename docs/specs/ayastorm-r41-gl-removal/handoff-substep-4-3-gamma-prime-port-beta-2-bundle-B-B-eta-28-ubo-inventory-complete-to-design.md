# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 UBO 棚卸し完了 → 設計起案 handoff

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-pivot-to-ubo-design.md` (pivot 経緯 + §2 26 個一覧 + §3 議題リスト)
**産出資料**: `ayastorm-r41-ubo-current-state-inventory.md` (= 本 session の主成果、設計 doc 起案の基礎資料)

---

## §0 本 handoff の位置付け (= 次 session 着手地点)

前 handoff で確定した **UBO 設計 pivot** を受けて、本 session で **OpenGL 現状棚卸し** を完了した。次 session では棚卸し doc を前提に **設計 doc (`ayastorm-r41-ubo-overall-design.md`) を新規起案** する。本 handoff はその引き継ぎ。

**= 設計 doc 起案条件は揃った状態**。

---

## §1 本 session で達成したこと (時系列)

### §1.1 pivot 経緯の理解と棚卸し方針の確立

- AYA さん指摘「設計しないで Vulkan エラーを取ることに躍起になっている」+ 「現在の OpenGL での描画で UBO 何個作って Bind 数がどれぐらいになるかを考慮しないと仕事になってない」を受領
- 26 個 UBO の起源が **設計でなく parse error 反復対応の累積**であることを §2 経緯整理 + AYA さんに率直に説明 (3 つの問い: なぜ OpenGL 棚卸しを欠落させたか / なぜ parse error にこだわったか / 後から設計指針だったのか → 全て「いいえ、私の構造的誤り」と回答)

### §1.2 OpenGL 棚卸し Agent 実行 (Explore, very thorough)

- 軸 A (GLSL UBO 宣言全件) / 軸 B (host C++ bind site) / 軸 C (frame 内 bind 数) で網羅調査
- 主成果: **OpenGL path で実働している UBO は 4 個のみ** (`UB_REFLECTION_PROBES` / `UB_GLTF_JOINTS` / `UB_GLTF_NODES` / `UB_GLTF_MATERIALS`) と判明
- §3 で前倒し宣言した 80+ 個 UBO は **全て `#ifdef LL_VULKAN_GLSL` 内 = OpenGL path 到達せず**

### §1.3 棚卸し doc 起草 (= 本 session の主成果物)

新規 doc: `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md` (~650 行)

構成:
- §0 経緯 (失敗反省)
- §1 OpenGL path 実働 UBO 4 個
- §2 OpenGL bare uniform 更新メカニズム
- §3 GLSL UBO blueprint 全件 (set=0:3 / set=1:2 / set=2:25 / set=3:54)
- §4 host C++ UBO 管理 API + redirect 層欠落の発見
- §5 1 frame 仕事量見積
- §6 構造的所見 6 件
- §7 不確実点 / 残課題 8 件
- §8 設計 doc 起案への含意 5 議題

### §1.4 §7 残課題のうち #2 / #3 / #6 を解消 (= 推奨優先 3 件)

| # | 項目 | 結論 |
|---|---|---|
| #2 | `LLGLSLShader::UB_*` enum 全件 | `llglksshader.h:157-160` 直 read で **4 個確定** |
| #3 | set=3 Legacy 個数 (handoff §6-D 32 vs Agent 推定 53 の乖離) | grep 集計で **54 個確定** (handoff doc 古い、棚卸し doc が正) |
| #6 | `LLGLSLShader::uniform*fv()` redirect 痕跡有無 | `llglslshader.cpp:2166-2557` 直 read で **redirect 痕跡ゼロ確定** (16 method 全件 GL 直呼びのみ) |

副産物: `uniform4iv` (line 2310) が内部で `glUniform1iv` を呼ぶ bug 候補発見 (本 scope 外として棚卸し doc に脚注)。

棚卸し doc §3.2 / §3.4 / §4.1 / §4.3 / §7 を update 済。

---

## §2 確定した事実サマリ (= 設計 doc が前提として参照する事実)

### §2.1 OpenGL path 現状 (= 「実働しているもの」)

- **bind されている UBO = 4 個のみ** (LLGLSLShader::UB_REFLECTION_PROBES / UB_GLTF_JOINTS / UB_GLTF_NODES / UB_GLTF_MATERIALS)
- **値の流れは bare uniform 主体**: `LLGLSLShader::uniform*fv()` family (`llglslshader.cpp:2166-2557` の 16 method) が `glUniform*` を直接呼ぶ
- 1 frame の API call 推定: bare uniform 数百〜数千 + UBO bind 5-20 回

### §2.2 Vulkan blueprint 現状 (= 「宣言だけあって dead」)

- **set=0 (per-frame backbone)**: 3 個 (`FrameViewProj` / `FrameLights` / `FrameAtmosphere_Lighting`)
- **set=1 (per-program material)**: 2 個 (`MaterialUBO` + `MaterialUBO_Legacy`、同 binding=0 共有、program 単位 attach 排他想定)
- **set=2 (PerDraw / PerProgram)**: 25 個 (binding 0-25、η-3 から η-28 Phase 2c までの累積)
- **set=3 (Legacy)**: 54 個 (handoff §6-D の 32 個より 22 個多い)
- **合計 = 84 個** (棚卸し doc §3 全節集計)

### §2.3 host C++ 側 redirect 層 = **完全欠落**

- `LLGLSLShader::uniform*fv()` の 16 method 全件で **`#ifdef LL_VULKAN_GLSL` 分岐ゼロ / `if (mUseUBO)` 分岐ゼロ**
- `LLGLSLShader::UB_*` enum に登録された 4 個以外は **host から bind する識別子が存在しない**
- = **Vulkan path で shader compile が通っても、§2.2 の 84 個 UBO には値が来ない**
- = **本棚卸しで最大の発見事項**、Vulkan 化完成の必須条件

### §2.4 構造的所見 (= 設計判断時に効く事実)

- **OpenGL と Vulkan blueprint は完全分離** (`#ifdef LL_VULKAN_GLSL` gate)、blueprint 再構成は OpenGL 描画動作に影響しない
- **set=2 PerProgramUBO (25) と set=3 Legacy (54) は寿命同じ (per-program)** = 命名規則違いで役割重複、統合候補
- **bare uniform → UBO 集約の対応表が未整備** (= 設計 doc 主題)

---

## §3 次 session で起案する設計 doc の主題 5 件

棚卸し doc §8 を起点に、設計 doc (`ayastorm-r41-ubo-overall-design.md`) で扱う議題:

### §3.1 議題 1: host C++ redirect 層の枠組み (= 描画成立の必須条件)

§2.3 で確定した「redirect 層完全欠落」を受けて、**Vulkan path 用 redirect 層の設計**を冒頭で決める。前 handoff §3.2 で起草した方向:

```
LLGLSLShader::uniform4fv("color", ...) 呼出
  ↓
shader 内部 metadata で "color" → {ubo_index=..., offset=..., size=...} を lookup
  ↓
ubo_index 番目の dirty buffer (host memory copy) に memcpy
  ↓
dirty flag を立てる
  ↓
draw call 直前に dirty な UBO だけ batch upload
```

**設計判断ポイント**:
- metadata の自動生成 (shader load 時 GLSL reflection) vs 手書き宣言
- dirty flag 粒度 (UBO 単位 / member 単位)
- worker thread 安全性 (原則 2)
- 既存 GLTF/ReflectionProbes 4 個 (UB_* enum) との統合方針

### §3.2 議題 2: bare uniform → UBO 集約対応表

OpenGL の bare uniform 群 (数千 / frame) を **寿命別に分類** し、それぞれ **どの UBO に集約するか** を表として確定。

- per-frame: view/proj/sun/atmosphere
- per-program: gamma / fxaa rcpFrame / material params 等
- per-draw: mTransform / per-light dynamic / per-mesh color 等

**棚卸し doc §3 の 84 個 blueprint がこの分類に当てはまるか** を判定する作業も含む。

### §3.3 議題 3: set=2 (25) + set=3 Legacy (54) の再構成

§2.4 で指摘した役割重複。§3.2 の集約対応表に従って:

- **そのまま残す**: 寿命分類と binding 割当が整合する UBO
- **統合する**: 同寿命 + 同 program で複数 UBO に分かれているもの (e.g., light 系 binding 0/1/5/10/25)
- **昇格 / 降格する**: per-program だが実は per-frame で十分なもの (e.g., gamma)
- **削除する**: 役割重複 (set=2 と set=3 で同じ役割を二重定義しているもの)

### §3.4 議題 4: 寿命分類体系の最終確定

per-frame / per-view / per-program / per-material / per-draw の 5 分類で過不足ないか。前 session 議論で私は **「per-view を per-frame 派生 sub-tier」推奨** を出していたが、AYA さん採否未取得。**§3.2 の集約表が実際に当てはまるかで判定**。

### §3.5 議題 5: Phase 番号体系の再設計 + Phase 2d-α 適用済 commit 処遇

前 handoff §3.4 で起草した α/β/γ/δ/ε 体系:

- Phase α: UBO 全体設計 doc 確定
- Phase β: host C++ redirect 層実装 (寿命分類順)
- Phase γ: GLSL 側 UBO 再構成
- Phase δ: 残り未対応 program の UBO 化
- Phase ε: cold launch verify + Vulkan 実 build 検証

Phase 2d-α 適用済 4 commit (`fd4dbb71aa` / `9acf70acdf` / `0587c574da` / `9a576884c0`) の処遇:
- **§2.3 含意**: §2.2 の 84 個 UBO は全部 dead なので、Phase 2d-α 適用済 commit (5 file feat) も実害ゼロ・実効果ゼロ
- 判断 (A) push して残置 / (B) push 保留 / (C) revert を §3.1〜§3.4 確定後に決定

---

## §4 次 session 開始 protocol

### §4.1 必読 file 順

1. **本 handoff doc** (= 本 file) - 全体把握 + 設計 doc 起案条件確認
2. **`ayastorm-r41-ubo-current-state-inventory.md`** - 棚卸し全件、設計 doc の前提資料 (= 本 session の主成果)
3. **memory `project_ayastorm_r41_design_principles.md`** - 2 大設計原則 (原則 1: upstream 取り込みやすさ維持 / 原則 2: Core 分散実現)
4. **memory `feedback_ubo_migration_one_at_a_time.md`** - 本 pivot で「1 UBO ずつ」を「1 寿命分類ずつ」or 「1 program ずつ」に解釈し直す課題あり
5. **前 handoff `...-pivot-to-ubo-design.md`** §1 (2 大設計原則再掲) + §3 (議題リスト原型) - 本 handoff §3 はこれを発展させたもの
6. (任意) **`reference-shader-location-map.md`** §6-A〜E - 既存 UBO binding マップ、棚卸し doc と乖離あり (set=3 が 32 vs 54)

### §4.2 議題進行順 (= §3 の優先順位)

設計 doc は live doc として起案、議題を上から順に詰める:

1. **§3.1 redirect 層枠組み** (描画成立の必須条件、これ無しでは他議題が空想)
2. **§3.4 寿命分類** (per-view 採否、5 分類体系の確定)
3. **§3.2 bare uniform → UBO 集約対応表** (§3.4 寿命分類を当てはめる)
4. **§3.3 set=2 + set=3 再構成** (§3.2 対応表に従う作業)
5. **§3.5 Phase 体系 + Phase 2d-α commit 処遇**

### §4.3 アウトプット成果物

- **新規 doc**: `ayastorm-r41-ubo-overall-design.md` (live document、本 handoff §3 議題 5 件を発展)
- **棚卸し doc §7 残課題の追加解消** (#1 set=2 binding 重複疑い / #4 Frame UBO 詳細 / #5 Material 排他確認 / #7 upstream diff / #8 SSBO 棚卸し)、設計 doc 起案と並行で OK
- **memory 更新**: `feedback_ubo_migration_one_at_a_time` の表現再定義 (「1 UBO ずつ」を寿命分類単位 or program 単位どちらに解釈し直すか)

### §4.4 設計議論で詰まったとき

- AYA さんが「OpenGL の bare uniform を全件見たい」と言われたら → `LLEnvironment::updateShaderUniforms` 実装 + `llglslshader.cpp:2166-2557` setter family + 各 caller site の Agent 棚卸し提案
- AYA さんが「redirect 層は具体的にどう実装する?」と聞かれたら → `LLShaderUniforms` 内部 cache class の拡張案 + `LLGLSLShader::apply()` 改修案 提示
- AYA さんが「84 個全部見直す必要あるのか?」と聞かれたら → 本棚卸し §2.4 (set=2 vs set=3 寿命重複) + 設計 doc §3.3 で「統合 / 削除 / 保持」を分類予定と説明

---

## §5 不確実点 / 残課題 (= 設計 doc 起案と並行解消)

棚卸し doc §7 から本 session で解消した #2 / #3 / #6 を除いた残 5 件:

| # | 項目 | 設計 doc への影響 |
|---|---|---|
| 1 | set=2 内で同一 binding に複数 UBO 名と見える件 (Agent 棚卸しが ClipPlane / SkinnedVelocity / AvatarVelocity / AvatarSkin / ObjectSkin を binding=0 と推定) | 軽: §3.3 set=2 再構成議論時に program 単位 binding namespace 検証で解消可 |
| 4 | `FrameViewProj` / `FrameLights` / `FrameAtmosphere_Lighting` の正確な member 一覧と bare uniform との重複 | 中: §3.2 集約対応表起こす時に必要、各 GLSL 宣言行直 read + bare uniform 名突合 |
| 5 | `MaterialUBO` と `MaterialUBO_Legacy` の attach 排他確認 | 軽: §3.3 set=1 再構成議論時に program 単位 attach 確認 |
| 7 | upstream Firestorm との UBO blueprint 差分 | 中: 原則 1 (upstream 取り込みやすさ) 評価に必要、upstream HEAD との diff |
| 8 | OpenGL path に存在するが棚卸し外の SSBO / image binding 有無 | 軽: §3.1 redirect 層実装後の検証時に必要 |

---

## §6 本 session の commit 状態

### §6.1 本 session で作成した file (未 commit)

| file | 種別 | 行数 | 状態 |
|---|---|---|---|
| `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md` | 新規 doc | ~650 | 未 commit |
| `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-ubo-inventory-complete-to-design.md` (= 本 doc) | 新規 handoff | (本 doc) | 未 commit |

両者 docs only、code 編集なし。次 session 冒頭で commit 提案する想定。

### §6.2 前 session までの未 push commit (本 pivot 対象)

```
9a576884c0 docs(r41): η-28 Phase 2d-α reference doc 更新
0587c574da feat(r41): η-28 Phase 2d-α 適用 (5 file)
9acf70acdf docs(r41): η-28 Phase 2d-α prep §0.5 新設 + §6-A/§6-E 訂正
fd4dbb71aa docs(r41): η-28 Phase 2d-α prep handoff 起草
```

- 全て **push 保留**
- 処遇は **設計 doc §3.5 で確定後に判断** (push / 保留 / revert の 3 択)
- §2.3 含意 (= 84 個全部 dead) からして実効果ゼロなので、revert しても害は無いし、push して残置しても害は無い (純粋に整理判断)

---

## §7 注意点 (= 次 session 開始時 Claude が忘れがちな点)

### §7.1 設計 pivot の主旨を縮小しない (`feedback_no_scope_shrink`)

- 設計対象は **84 個全体 + bare uniform 集約** (= OpenGL 現状の値の流れ全体)
- 「Phase 2d-α の範囲だけ」「set=2 の 25 個だけ」「parse error が出た program だけ」と縮小解釈しない
- AYA さん「すべて」「全部」の literal scope を勝手に縮小しない

### §7.2 parse error 潰し reflex に戻らない (`feedback_doubt_self_first`)

- 設計 doc が固まる前に「先に 1 個試しに parse pass してみる」「とりあえず動くもの作る」は禁止
- 設計確定 → host C++ redirect 層 → GLSL 再構成 → 実 build verify の順序を守る

### §7.3 AYA さんの focus は技術詳細でなく全体像

- 「memcpy のサイズが何 byte」「dirty flag は 32-bit ビットマスクか」等の詳細を並べすぎない
- 「寿命分類が当てはまるか」「call site API は温存できるか」「Core 分散容易か」が AYA さんの focus

### §7.4 棚卸し doc は live doc

- 設計 doc 起案中に新事実が出たら棚卸し doc にも反映 (= source of truth)
- 設計 doc が棚卸し doc を参照する形を保つ (= 重複記述しない)

### §7.5 build_only_verified に従う

- 設計 doc の判断は推測でなく **棚卸し doc + 直 source read** から導く
- 「OpenGL ではこうなってるはず」「Vulkan ならこう動くはず」の **空論 reject**

### §7.6 結論ファースト (`feedback_explanation_lead_with_conclusion`)

- 設計判断は「**結論 + 根拠 + (必要なら) 選択肢**」の順
- 「ABC のうち A 案推奨、理由は X」が型、「ABC 並べて意見を求める」は下手

---

## §8 補足: コンテキスト残量と handoff 判断の経緯

本 session でコンテキスト消費した主要操作:

| 操作 | 消費見込み |
|---|---|
| 前 handoff doc 全 read (343 行) | 中 |
| reference doc §6 全節 read (~250 行) | 中 |
| memory 2 件 read | 軽 |
| Agent 棚卸し (Explore very thorough) 結果 ~700 行 | 大 |
| 棚卸し doc 起草 (~650 行 write) | 大 |
| 棚卸し doc 6 箇所 edit | 中 |
| `llglslshader.cpp:2166-2557` read (400 行) | 中 |
| grep 結果 (set=3 全件 ~60 行) | 軽 |

設計 doc は **棚卸し doc と同等規模 (500-1000 行) になる見込み** + 議題 5 件の議論で AYA さんとの往復が複数回必要。

AYA さん指示「コンテキスト残量 Claude 側で能動監視、handoff-*.md 作成 → 次 session へ」(`feedback_proactive_handoff`、absolute rule) に従い、本 handoff を起草。

---

**本 handoff は r41 UBO 設計 doc 起案完了まで永続参照**。次 session で `ayastorm-r41-ubo-overall-design.md` 起案後、本 handoff は **棚卸し → 設計遷移の経緯資料** として as-is 保存。
