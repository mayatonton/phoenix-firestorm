# 設計 chapter 群 (01-10) 第二次査読 report (2026-06-03 second pass)

**起案日**: 2026-06-03
**位置付け**: 第一次査読 (`design-review-2026-06-03.md`) → 34 violations 全件修正 (commit `dfc5ad5aa2`) を受けて、**修正後 state を再査読する独立 session** の report。AYA 指示「もう一度査読」mandate に基づき、修正の質 (= 修正が真に違反を解消したか / 新規違反を生んでいないか) + 第一次で見落とした違反 (= 修正 commit 後の最新 state を 4 観点で総点検) を検出する。
**handoff doc**: `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-review.md`
**第一次 report**: `design-review-2026-06-03.md` (= snapshot、本 doc は独立 snapshot)
**規律**: design-phase 継続 (= `indra/` 改変ゼロ、`docs/specs/ayastorm-r41-gl-removal/` 配下のみ修正)、修正 commit は AYA 確認後、tests/ 言及禁止

---

## §0 第二次査読 session 概要

### §0.1 第二次査読の焦点

第二次査読は次の 2 軸で実施:

1. **修正の質 check** (= 第一次 §9 修正履歴の 34 件が真に違反解消したか): 各 chapter §N の修正部分を pinpoint Read で確認、(a) 修正が完了している (b) 修正が新規違反を生んでいない (c) cross-ref / 連動 rename が抜けていない の 3 点で評価
2. **第一次見落とし check** (= 修正後 state の 4 観点総点検): chapter 02 → 04 → 05 → 06a → 06a-prep → 06b → 06c → 07 → 08 → 09 → 10 順次再査読、第一次で気付かなかった違反を検出

### §0.2 査読観点 4 軸 (= 第一次と共通)

1. **[事実性]** 設計書の主張 (= 数値 / source line / inventory § / ✓ 解消 mark) が実際の source / inventory / 他 chapter と一致するか
2. **[作業可能性]** 設計通りに実装 phase が完遂可能か (= 入力 → 処理 → 出力 chain が途切れていないか、約束された後続解消が果たされているか)
3. **[破綻有無]** 設計の決定事項どうし / chapter どうし / 原則どうしで論理矛盾していないか
4. **[整合性]** 用語 / 表記 / 数値 / 項目 ID が全 chapter で一貫しているか

### §0.3 査読対象 (= 修正後 state)

- 主: `design/01-overview.md` 〜 `design/10-open-questions.md` (12 chapter、06a-prep 含む)
- 副: `ayastorm-r41-ubo-current-state-inventory.md` / `reference-shader-location-map.md`
- handoff `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-review.md` §4.4 項目 ID 一覧

### §0.4 査読方法 (= handoff §3.1 順序 + 修正部分 pinpoint)

1. chapter 01 + 03 = 判定基準 (= 第一次で確認済、本 session は前提固定)
2. chapter 02 → 04 → 05 → 06a → 06a-prep → 06b → 06c → 07 → 08 → 09 → 10 順次
3. 各 chapter で先に第一次 §9 修正履歴の対応 §N を pinpoint Read → 修正の質 check
4. 次に chapter 全体を 4 観点で総点検 → 新規違反候補抽出
5. Explore agent を **数値突合 / 項目 ID 衝突 grep / source line 参照確認 / 第一次 finding 修正反映 grep** に並列発注、main context 節約
6. AYA 判断仰ぎ 21 件 (chapter 10 §1) を最後にまとめて再点検

### §0.5 判定基準固定 (= chapter 01 + 03 抜粋、第一次と同一)

**設計原則 2 件**: 原則 1 = call site 温存 / 原則 2 = Core 分散容易な設計
**確定事項 13 件** (chapter 01 §5): Codegen-UBO / API 温存 / cadence source = 既存 path / per-frame = frame loop sync / 84 UBO 温存 / lifetime owner 踏襲 / 論理 + 物理両軸 / 1 UBO ずつ migration / GLSL 不可侵 / perfect hash / bare uniform 不取込 / per-material → per-draw 統合 / E3 rename
**cadence 5 分類**: per-frame `Frame*` / per-program `Program_*` / per-draw `Draw_*` `Material*` / per-asset `Asset_*` / per-skin `Skin_*`
**数値基準**: 84 UBO / 4 種 UB_* / set=0:3 / set=1:2 / set=2:25 / set=3:54 / 79 binding / 49 sampler / 16 不明 cadence / 物理 instance = 1 + 2N + M

---

## §1 第二次査読結果 sammary

- **致命傷候補**: **2 件**
  1. §2.2 = chapter 01/05/inventory「set=2:25 + 総数 84」vs chapter 02「set=2:26」の根本数値矛盾 (= 第一次「26 採用」確定は chapter 02 内のみ反映、波及 4 箇所未追従、総数 84 と内訳 26 で 3+2+26+54=85 が成立する数値破綻、chapter 05 §3.3 行 67-75 で「per-program 23 + per-draw 2 = 25」が独立根拠、25 が正の可能性大)
  2. §4.1 = chapter 09 §2.1「K 値 placeholder」宣言と §5.2 template の「Phase 2 = UB_REFLECTION_PROBES」具体 phase 確定形が論理矛盾 (= K 未確定なら後続 phase 確定不可、K 確定なら placeholder 表記不要)
- [事実性] 違反: 4 件 (§2.1 / §2.2 / §2.3 / §2.4) ※ §2.4 = chapter 06a §5.1 setter 表 16 method (uniform1i 欠落) と 06a-prep §2.2.1 の 17 method 不一致
- [作業可能性] 違反: 3 件 (§3.1 / §3.2 / §3.3) ※ いずれも chapter 09 入力契約 / Phase 1.C ↔ 2 境界 / canary cvar 検証 scenario
- [破綻] 違反: 2 件 (§4.1 致命傷 + §4.2 Phase K+1/+2/+3 直列 / 並列 ambiguous)
- [整合性] 違反: 7 件 (§5.1 / §5.2 / §5.3 / §5.4 / §5.5 / §5.6 / §5.7)
- **修正反映確認**: 第一次 34 件 部分反映 (= chapter 02 / 05 / 06a で波及修正漏れ 7 件検出、chapter 06b/06c/07/08 は完全反映)
- **総合判定**: **PASS-with-major-fixes** (= 致命傷候補 2 件 + 補強 16 件、設計 chapter 群そのものは構造健全、修正範囲限定的だが波及度高、実装 phase 入口前に致命傷 2 件の AYA 判断必須)

---

## §2 [事実性] findings (第二次査読)

> 査読進行中。各 finding を以下形式で記録:
> - **主張**: 「設計書記述」
> - **期待**: 「実際にはこうあるべき」
> - **観測**: 「実 source / inventory / 別 chapter での記述」
> - **判定**: ✓ 一致 / ✗ 不一致
> - **修正案**: 「...」

### §2.1 set=2 帯 25 vs 26 の波及修正漏れ (= 第一次修正の連鎖が chapter 02 内で止まっている)

- **主張**: `02-naming-convention.md` §1 行 15 / §3.3 行 138 / §3.3 行 140「set=2 帯 = **26 個** (binding 0-25、unique entry 26)」明示
- **同 doc §3.3 行 140 で明記**: 「inventory + chapter 01 + chapter 05 は別 task で後追い修正対象」 (= 第一次修正で chapter 02 内部の数値訂正は実施したが、波及 chapter の修正は **未実施で持越**)
- **観測** (= 数値突合 agent grep 結果):
  - `01-overview.md:73` = 「84 個 (set=0:3 / set=1:2 / **set=2:25** / set=3:54)」← 未修正
  - `05-existing-inventory-link.md:67` = 「§3.3 set=2 帯 (**25 個**): per-program 23 + per-draw 2」← 未修正
  - `05-existing-inventory-link.md:121` = 「**set=2 帯 25** + set=3 帯 54」← 未修正
  - `ayastorm-r41-ubo-current-state-inventory.md:373` = 「84 個 (set=0:3 / set=1:2 / **set=2:25** / set=3:54)」← 未修正
- **期待**: 第一次 §9 で「全件修正完了」と総括 → 数値主張は全 chapter 一貫が事実性の根幹のため、chapter 02 内部だけでなく波及修正が必要
- **判定**: ✗ 不一致 (= 第一次修正の波及が chapter 02 内で止まっている、4 箇所未追従)
- **修正案**: §2.2 (= 25 vs 26 の根本確定) と一体で対処。「26 が正」確定なら上記 4 箇所を「26」に修正 + 総数を「85」に書き換え。「25 が正」確定なら chapter 02 §1 行 15 / §3.3 行 138 / 行 140 / 表本体を巻き戻し

### §2.3 chapter 04 §5.1 「84 + 既存実働 4 = 88」の "既存実働 4" 根拠 spot check 要

- **主張**: `04-codegen-ubo.md` §5.1 表 行 167「`ubo_layout_<blockname>.inl` ... UBO 数 (現状 **84 + 既存実働 4 = 88**)」
- **観測**: "既存実働 4" は chapter 01 §3.3 の「host C++ 上 4 種 UB_* binding (UB_REFLECTION_PROBES / UB_GLTF_NODES / UB_GLTF_MATERIALS / UB_GLTF_JOINTS)」を指していると推定
- **疑問**: 4 種 UB_* は host C++ 上の **logical binding** であり、GLSL block 宣言が **84 GLSL blueprint と独立に存在するか**は未確認。`reflectionProbes.glsl` / `gltfPbrV.glsl` 等で `uniform <BlockName> { ... }` 宣言が「84 個に含まれず別に 4 個」存在する保証が本 chapter 内にない (= chapter 01 §3.3 では 84 個 = 4 種に含まれない独立 blueprint と読める)
- **対立可能性**: 84 個 GLSL blueprint の中に 4 種 UB_* binding 対応 GLSL block が **既に含まれている** 場合は「88」でなく「84」が正
- **判定**: ✗ 要 spot check (= 第一次査読で未指摘、第二次新発見、inventory §6 で UB_* 4 種の GLSL 宣言と 84 個 blueprint の包含関係を確認要)
- **修正案**: inventory §6 で「UB_* 4 種 GLSL 宣言 = 84 個に含む / 含まない」明示確認 → 「84 + 4 = 88」または「84 (= 4 種包含)」のどちらに統一するか chapter 04 §5.1 と整合修正
- **波及**: §2.2 (= 25/26 致命傷候補) と独立の数値矛盾候補、合わせて AYA 判断仰ぎ事項として整理

### §2.4 chapter 06a §5.1 setter family listing が 16 method (= uniform1i 欠落)、06a-prep §2.2.1 の 17 method と不一致

- **主張 A** (chapter 06a §5.1 setter family 表、line 261-279): 16 method listing (`uniform1f` (line 2166) 起点)
- **主張 B** (chapter 06a §5.5 line 356): 「16 method × 2 系統 = **計 32 entry point**」
- **主張 C** (chapter 06a-prep §2.2.1 表、line 84-104): **17 method** listing (`uniform1i` (line 2141) を追加)、注 line 106-107「06a §5.1 は 16 method を listing したが、本 doc では `uniform1i` を追加した 17 method 構成」と相違 explicit 明示
- **主張 D** (chapter 06a-prep §2.2.2 表、line 116-131): LLStaticHashedString 版 **13 method** (uniform1i / uniform2i 含む)
- **観測**: 06a-prep 起案時に 06a §5.1 の 16 method 不足 (= uniform1i 欠落) を把握、ただし 06a 側を更新せず注で逃げた。結果として chapter 06a §5.1 / §5.5 が現状認識から 1 件 stale (= 32 entry point は本来 17+13=**30 entry point**)
- **判定**: ✗ 不一致 (= 06a 表が古い、06a-prep が正、source code 実体は 06a-prep 側に一致)
- **修正案**: chapter 06a §5.1 表に `uniform1i` (line 2141) 1 行追加 → 17 method 表に書き換え + §5.5「16 method × 2 系統 = 32 entry point」を「integer index 17 method + LLStaticHashedString 13 method = **計 30 entry point**」に修正 + §5.6 sampler 49 個記述は uniform1i が sampler binding setter として該当 (= UBO 化対象外) の説明と整合させる

### §2.2 「84 個」総数と「内訳 25 vs 26」の根本矛盾 (= 致命傷候補)

- **主張**: chapter 01 §1 §3.3 / inventory §1 §3 / handoff doc 等で **総数 84 個** 主張
- **対立する内訳**:
  - chapter 02 §3.3 = set=2 帯 **26 個** (binding 0-25、unique entry 26) → 3+2+26+54 = **85** (= 84 と 1 違い)
  - chapter 01 §3.3 / chapter 05 §3.3 / inventory §3.3 = set=2 帯 **25 個** → 3+2+25+54 = **84** (= 整合)
- **観測**: 第一次 §2.1 で「26 = binding range 0-25 表記 / 25 = unique UBO 数 (program 排他で同 binding 複数候補)」両論可能性に言及、第一次 §9 で「26 が正」確定 → ただし総数「84」は据置 → **総数 84 と内訳 26 の数値矛盾が解消されないまま修正完了と総括**
- **chapter 02 §3.3 注の根拠精査**: 行 140「実数は 26 (binding 0-25、unique entry 26、本査読 2026-06-03 確認)」と書かれているが、「unique entry 26」は表行数を unique と数えた可能性大 (= 表本体には同一 binding に複数 UBO 候補が並ぶ余地、第一次 §2.1 修正案で「unique UBO 数 25、program 排他で同 binding 複数候補」と注記提案あった)
- **判定**: ✗ 不一致 (= 第一次修正で「26 採用」確定したが、総数 84 との整合性検証が抜けた、第一次の修正方針自体が不十分の可能性大)
- **致命傷候補**: 修正が「数値の根本見直し」 (= 25 / 26 / 85 / 84 のどれが正か AYA 判断仰ぎ) を要する、設計の信頼性に影響
- **修正案**:
  - (A) **「25 個 + 84 個」採用**: chapter 02 §3.3 注記 + 表本体を巻き戻し、ヘッダー「set=2 帯 (25 個、binding 0-25 = 26 slot、program 排他)」、表本体は 25 行 + 1 行を注記「program 排他で同 binding 複数候補」として整理。これが「84 個総数」と整合する
  - (B) **「26 個 + 85 個」採用**: chapter 01 / 05 / inventory / handoff doc の「84」「set=2:25」を「85」「set=2:26」に全面波及修正
  - **(A) 採用が妥当と推定**: 第一次 §2.1 修正案 (= ヘッダーに注記追加、unique 25 / binding range 26 を書き分け) + inventory §3.3 本体 (= 25) が source of truth として扱われている経緯 (= inventory は live doc) から、(A) が一貫性高い
  - **AYA 判断仰ぎ事項として chapter 10 §1 に追加登録要請**
- **補強証拠 (chapter 05 査読時に新規発見)**:
  - `05-existing-inventory-link.md` §3.3 行 67「**set=2 帯 (25 個): per-program 23 + per-draw 2**」+ 行 75 表「2-25 | `Program_<X>` (= 23 個) | per-program」 (= per-draw 2 + per-program 23 = 25 unique UBO)
  - 一方 `02-naming-convention.md` §3.3 表本体 (行 144-169) を listing すると `PerDrawUBO_*` 2 + `PerProgramUBO_*` **24** = 26 (= per-program 数が chapter 05 vs chapter 02 で **23 vs 24** に分裂)
  - **= chapter 02 表に 1 行余分** = 二重登場または別 set 帯混入の可能性大、「25 が正、chapter 02 §3.3 「26」採用が誤り」の解釈が有力
  - inventory §3.3 全件 enumerate と突合して **chapter 02 表 余分 1 行を特定** する作業が必要 (= AYA 判断より先に主執行可能な事実確認 task)
  - 推定: (A 案)「25 個 + 84 個」採用 → chapter 02 §3.3 表本体から余分 1 行を削除して整合

---

## §3 [作業可能性] findings (第二次査読)

### §3.1 chapter 09 Phase 0 計測と (Q1) 入力契約の可視性不足

- **主張** (chapter 09 §3.5 line 145-148): 「Phase 0 で確定する後続 Phase の入力 = (Q1) 第 1 UBO 選定の候補 cadence 確定」
- **観測**: chapter 06a-prep §6「計測結果反映 flow」のどの row が (Q1) 入力提供かが chapter 09 §3.5 から pointer 化されていない
- **判定**: △ 入力 chain の可視性不足 = 実装者が chapter 09 → 06a-prep 移行時に「どの計測 row が (Q1) 入力か」読み取り困難
- **修正案**: chapter 09 §3.5 (= 或 §1 pre-requisite) に「06a-prep §6 row X (= (H1b) hook 計測) が (Q1) 入力提供」を annotation 追記

### §3.2 chapter 09 Phase 1.C shell ↔ Phase 2 本実装 memory layout 互換性 implicit

- **主張** (chapter 09 §4.2 line 172): 「shell は Phase 1.C で **捨てる前提**、Phase 2 で全面書換可」
- **観測**: Phase 1 Exit 「既存 program 動作 unchanged」(line 176) は shell 実装が descriptor pool 構造と互換が前提
- **判定**: ✗ implicit 前提 = Phase 1.C shell の memory layout が Phase 2 本実装 UBO layout と互換である assumption が明文化されていない → UBO サイズ不一致時 Phase 2 entry 失敗リスク
- **修正案**: chapter 09 §4.2 Phase 1.C / 2 境界に「shell は最終 UBO struct サイズで先行確保 (member は dummy zero)、Phase 2 で member logic 置換のみ」明示

### §3.3 canary cvar 検証 scenario の reproducibility 不足

- **主張** (chapter 09 §5.3.1 line 268-273): 5 step 検証手順 + AYA 実機実行
- **観測**: 「該当 UBO 経路を含む scene 表示」の場面確定が phase 別に明確でない (例: Phase 2 で使う scene 名)
- **判定**: ✗ reproducibility 不足 = phase 別 scene 未明示で AYA 独立再検証不可
- **修正案**: chapter 09 §5.2 phase 順 template 各項目に「canary 検証用 scene: `<scene 名>`」annotation 追加

---

## §4 [破綻] findings (第二次査読)

### §4.1 [致命傷候補 2 件目] chapter 09 §2.1 K placeholder vs §5.2 template 具体 phase 記述の矛盾

- **主張 A** (chapter 09 §2.1 line 78): 「K 確定前は §2 / §3 / §5 / §6 / §7 / §8 の『K』『K+1』等は **暫定 placeholder**」
- **主張 B** (chapter 09 §5.2 template A): 「Phase 2 = `UB_REFLECTION_PROBES`」「Phase 3 = `UB_GLTF_MATERIALS`」← 具体 phase 確定形
- **判定**: ✗ 論理矛盾 = K 未確定なら §5.2 以降の具体 phase は前提化不可、K 確定なら §2.1 で「暫定」表記不要。実装 phase 入口で「Phase 2 開始」可能か不明確 → 作業可能性ブロック
- **致命傷候補**: 設計信頼性影響、AYA 判断仰ぎ事項として整理
- **修正案** (二択):
  - (A) §5.2 template を「(Q1)(Q2) 確定後の phase 振分 template 例」前提で書き直し
  - (B) §5.2 の Phase 2/3/... 記述を「(Q1) 確定後 phase X = 該当 UBO」一般化に書き直し
- **AYA 判断仰ぎ事項として chapter 10 §1 追記要請**

### §4.2 chapter 09 Phase K vs Phase K+1/+2/+3 直列 / 並列 ambiguous

- **主張 A** (chapter 09 §2.2 dependency 図): Phase K → K+1 → K+2 → K+3 直列
- **主張 B** (chapter 09 §6.1 (Q4)): 「Win/Mac 並列可」案提示
- **観測** (§6.2.2 line 324): 「Mac 並行中 Win で REJECT 検出 → K+2 Exit 後 K+3 で fix」← 並列前提記述
- **判定**: △ phase number ≠ 絶対時間軸の関係が §2.2 図で不明
- **修正案**: chapter 09 §6.1 (Q4) 各案 (Linux first / 並列 / 順次並列) ごとに phase timeline 図 (例: K+1 \| K+2/K+3 並列) を提供

---

## §4.5 (旧 §4 移動なし、Phase 0 入力契約整合性は §3.1 参照)

---

## §5 [整合性] findings (第二次査読)

### §5.1 chapter 02 §2.1 表 行 41 「G1 確定」が rename 後 (MC1) に未追従

- **主張**: `02-naming-convention.md` §2.1 表 行 41 「per-draw (material dirty flag、chapter 05 §6 **G1** 確定)」
- **対立する修正**: 第一次 §9.3 ID rename 連鎖サマリで「05 §10 (G) → (MC) rename + 05 §6 G1/G2 → **MC1/MC2** 連動 + 06c §6 G1/G2 → MC1/MC2 連動」確定
- **観測**: chapter 05 / 06c 内では G1/G2 → MC1/MC2 連動完了 (= agent B 確認済)、ただし chapter 02 §2.1 表 行 41 の `(G1)` 参照が **連動漏れ**
- **判定**: ✗ 不一致 (= ID rename 連鎖が chapter 02 §2.1 まで波及していない)
- **修正案**: chapter 02 §2.1 表 行 41 を「chapter 05 §6 **MC1** 確定」に修正

### §5.3 (S1) 解消状態が chapter 06a §9 と 06a-prep §2.2.2 で矛盾

- **主張 A** (chapter 06a-prep §2.2.2 注、line 133): 「`getGlobalRegistry()` は本 doc 起案時点で存在しないこと確認済 (= 06a §9 (S1) 解消)」← (S1) を **解消済** と明言
- **主張 B** (chapter 06a §9 持越表、line 478): (S1) `LLStaticHashedString::getGlobalRegistry()` API 存在確認 ... | **06b 起案時 grep** ← (S1) を **未確定** と listing
- **主張 C** (chapter 06a §4.3 但し書き、line 212): 「実装に当たっては §4.3.1 の代替案いずれかを採用、**最終確定は (S1) を chapter 06b / Phase 0 で消化した後**」← (S1) を **未消化** と明言
- **判定**: ✗ 不一致 (= 06a-prep は解消、06a は未消化、3 箇所間で状態認識が割れている)
- **実体分析**: (S1) は本来 2 軸 = (i) API 存在確認 (= 不存在) + (ii) 代替案 (S1-A/B/C/D) 採用確定。06a-prep は (i) 完了をもって「(S1) 解消」と総括、06a は (ii) 未確定を理由に「未消化」maintain → **実体は (i) 完了 / (ii) 未確定の状態を、表現方法だけ chapter 間でずれた**
- **修正案**: chapter 06a §9 (S1) を分割 = 「(S1-存在) `getGlobalRegistry()` 不存在確認済 (= 06a-prep §2.2.2 確認)、解消マーク」 + 「(S1-代替) 代替案 S1-A/B/C/D の確定 = 06b / Phase 0」と書き直し、AYA 判断仰ぎ candidate に再配置 (= chapter 10 §1 にも追記要)
- **波及**: chapter 06a §4.3 但し書き / §9 listing / 06a-prep §2.2.2 注 / chapter 10 §1 (AYA 判断仰ぎ) すべて整合させる必要

### §5.5 chapter 09 §2.1 K placeholder 表記と §5.2 具体 phase 数の表記不一致

- **§4.1 (= 致命傷候補 2) と同根**、整合性 axis での重複記録: K 値 placeholder 宣言 vs §5.2 で Phase 2 / 3 確定形採用の表記矛盾。詳細 + 修正案は §4.1 参照。

### §5.6 chapter 10 §1「21 件」の double-count 検証未了

- **主張** (chapter 10 §1): AYA 判断仰ぎ 4+7+5+4+1 = 21 件
- **観測** (agent grep): chapter 09 内 `(Q*)` 名指し 32 件 (= 重複含む)、最終 21 件確定が真であるか cross-chapter 集約検証未了
- **判定**: △ 集計検証未了 = double-count なしの保証無し
- **修正案**: chapter 10 §1 冒頭に「21 件 = chapter 07 (4) + chapter 08 (7) + chapter 09 (5) + chapter 06b/06c (4) + chapter 05 (1)」分類表 + 各 (Q*) 名 listing 追加

### §5.7 handoff §4.4 「(MC2)」listing vs chapter 06c 出現ゼロ (= 表現不備)

- **主張** (handoff §4.4): 「06c §10 | MD (旧 M) / ... / **MC1 / MC2** (旧 G1 / G2) | 9 件」
- **観測**: chapter 06c に `MC1` 出現確認、`MC2` 一切出現なし。chapter 05 §6.3 で「MC1 採用」確定 → MC2 = 未採用選択肢
- **判定**: ✗ 表現不備 (= handoff listing で採用未済 ID を「実装済」と紛らわしく扱った)
- **修正案**: handoff §4.4 で `MC2` を「(採用未済選択肢、chapter 05 §6.3 で MC1 採用確定により未使用)」と明示 + chapter 06c §10 cross-ref 追加なら 注記化

### §5.4 chapter 06a §4.3 cross-ref section 番号誤り (§2.2.1 → §2.2.2)

- **主張**: `06a-cache-structure-and-setter-redirect.md` §4.3 但し書き (line 212): 「本査読 2026-06-03 §3.4 致命傷候補解消... 実装が存在しないことが確認済 (= **chapter 06a-prep §2.2.1 但し書き**)」
- **観測**:
  - `06a-prep-phase0-measurement.md` §2.2.1 (line 108-112) = `uniform1i` の位置付け (= 設計 review 2026-06-03 §3.5 矛盾解消) の注 (= getGlobalRegistry() 言及なし)
  - `06a-prep-phase0-measurement.md` §2.2.2 (line 132-133) = `getGlobalRegistry()` 不存在確認注 (= 実体の参照対象)
- **判定**: ✗ 不一致 (= cross-ref が §2.2.1 を指しているが、実体は §2.2.2)
- **修正案**: chapter 06a §4.3 但し書きの「§2.2.1 但し書き」を「§2.2.2 但し書き」に修正

### §5.2 chapter 05 内 G1 残骸 2 件 (= rename 連鎖が chapter 05 内でも完全には果たされていない)

- **主張**: `05-existing-inventory-link.md`
  - 行 9 (pre-requisite 注記)「`03-cadence-classification.md` (cadence 5 分類、= 旧 6 分類から per-material を per-draw + dirty flag 統合 = 本 chapter §6 **G1** 確定)」
  - 行 43 (§2 入力契約表 行)「chapter 03 §2 cadence 5 分類 (= 旧 6 分類から per-material → per-draw + dirty flag 統合済、本 chapter §6 **G1**) | §3 各 UBO の cadence 判定」
- **対立する修正**: 第一次 §9.3 ID rename 連鎖サマリで「05 §6 G1/G2 → **MC1/MC2** 連動」確定 (= chapter 05 §6 本文の G1/G2 → MC1/MC2 rename 自体は agent B で確認済、行 167-201)
- **観測**: chapter 05 §6 本文では MC1/MC2 採用済、ただし **chapter 05 内で §6 を指す cross-ref 2 箇所** (= 行 9 pre-requisite / 行 43 入力契約) が `(G1)` 残骸
- **判定**: ✗ 不一致 (= ID rename 連鎖が chapter 05 内 cross-ref で完了せず、第一次「全件修正」総括の精度不足)
- **修正案**: chapter 05 行 9 / 行 43 を「本 chapter §6 **MC1**」に修正 (= §5.1 chapter 02 §2.1 と同一方針)
- **示唆**: 第一次 §9.3 で chapter 05 §6 本文のみ確認、cross-ref まで grep 漏れの可能性 → 第二次 main session の grep で 2 件追加検出

---

## §6 第一次修正反映確認 (= 34 violations の修正反映状況 spot check)

### §6.1 agent 結果サマリ

#### §6.1.1 数値突合 agent (= a233b175e47bdbf74) 結果

- **84 UBO blueprint**: ✗ chapter 01 §3.3 行 73 で「set=2:25」のまま (波及修正漏れ、第二次 §2.1)
- **84 vs 85 総数**: ✗ 内訳 26 採用なら 85 に書き換え必要 (致命傷候補、第二次 §2.2)
- **descriptor pool 5 帯 / logical 4 帯**: ✓ 全 chapter 反映済
- **4 種 UB_***: ✓ 完全一貫
- **cadence 5 分類**: ✓ 旧 6 分類残骸 0
- **79 binding (40/39 split)**: ✓ chapter 07 / 08 / 10 一貫
- **物理 instance 式**: ✓ 1+2N+M 全 chapter 一貫

#### §6.1.2 ID 衝突再 grep agent (= a55fbd319659e2ca5) 結果

- **(G) → (MC)** 05 §10 + 06c §6 G1/G2→MC1/MC2: ✓ 連動完了 (ただし chapter 02 §2.1 表 行 41 残骸、第二次 §5.1)
- **(M) → (MD)** 06c §10: ✓ 完了
- **(R1) → (RB)** 07 §12: ✓ 完了 + 10 §2.5 listing 連動完了
- **prime 関係** (V1/V1' 等): ✓ 衝突なし
- **handoff §4.4 prefix 列登録規律**: ✓ 明文化済

#### §6.1.3 修正反映確認 agent (= a6d37028809ed4fcd) 結果

- **34 修正単位全反映と判定**: △ chapter 02 内部のみ確認、波及修正漏れ (= 第二次 §2.1 / §5.1) を検出できず → agent C 判定は **chapter 02 内部完結性のみ確認**、第二次 main session の波及 grep で 5 件追加検出

### §6.2 修正反映状況 (chapter 別)

chapter 02 査読時点で確定済 (他 chapter は査読進行に従い追記):

| chapter | 第一次修正件数 | 反映状況 |
|---|---|---|
| 02 | 5 | **部分反映** (= §3.3 内部 26 採用は反映、但し波及 4 箇所未追従 §2.1、§2.1 表 G1→MC1 連動漏れ §5.1) |
| 04 | 2 | **反映済** (= §6.3 (D) Phase 0 接続 + §7.2.1 過渡期動作 chapter 09 入力契約 ✓)、軽微 1 件 (= 第一次 §9.2 修正サマリ「mUseUBO sentinel 明示」と実 §7.2.1「(a)/(b)/(c) 3 案提示 + chapter 09 確定」の文言不一致、実体は適切) |
| 05 | 3 | **部分反映** (= §6 G1/G2→MC1/MC2 本文 rename は反映、但し chapter 05 内 cross-ref 2 箇所 行 9 / 行 43 が G1 残骸 §5.2、§3.3 数値「25」は §2.1 / §2.2 致命傷候補と一体) |
| 06a | 致命傷 1 + 補強 数 | **部分反映** (= §4.3 但し書き + §4.3.1 (S1) 代替案 4 案明示は反映、但し §9 (S1) 持越マーク残置 (= 06a-prep §2.2.2 注「(S1) 解消」と矛盾 §5.3)、§4.3 cross-ref §2.2.1→§2.2.2 番号誤り §5.4、§5.1 setter 表 16 method (uniform1i 欠落) / §5.5「32 entry point」が 06a-prep §2.2.1 17 method と矛盾 §2.4) |
| 06a-prep | 新規起案 | **整合性精査済** (= §2.2.1 注で「06a §5.1 = 16、本 doc = 17」を明示、ただし 06a 側を未更新で逃げた → §2.4 finding 起因) |
| 06b | - | **整合確認、新規 finding なし** (Agent A 査読、G1→MC1 rename / forwardToUboUpload / cadence 5 種 update site / dirty flag 全件確認、致命傷 §2.2 が解消待ち) |
| 06c | - | **整合確認、新規 finding なし** (Agent A 査読、M→MD rename / set=1 / V3/M1 関係 / mUseUBO 全件確認。ただし §5.7 = handoff §4.4 で MC2 が listing されているが chapter 06c に出現しない表現不備) |
| 07 | - | **整合確認、新規 finding なし** (Agent B 査読、maxBoundDescriptorSets 5 帯+4 bind / R1→RB / 4 種 UB_* / V1'/V3a/S3' 整合) |
| 08 | - | **整合確認、新規 finding なし** (Agent B 査読、schema 5 set 帯 / G/B3 / A1/P/B1-B5 7 件確定 / Codegen pipeline 全件反映) |
| 09 | - | **致命傷候補 2 件目検出** §4.1 (K placeholder vs §5.2 具体 phase 矛盾) + §3.1-3.3 / §4.2 / §5.5 各種 finding (Agent C 査読) |
| 10 | - | **整合性精査済** (Agent C 査読)、ただし §5.6 = 21 件 double-count 検証未了 (要 cross-chapter 集計確認、§8.1 spot check task に登録済) |

---

## §7 AYA 判断仰ぎ 21 件 再点検 (第二次)

第二次査読で **AYA 判断仰ぎ事項として新規追加** 候補 (= 致命傷候補から起因):

| 新規候補 ID 案 | 内容 | 関連 finding |
|---|---|---|
| (Q22-NUM) | set=2 帯 = **25 か 26 か** (= 総数 84 か 85 か) の根本確定 | §2.2 致命傷 1 |
| (Q23-K) | chapter 09 §2.1 K placeholder 表記の扱い (= §5.2 template の Phase 2/3 を「(Q1) 確定後の置換 template」 と読み替えるか、§5.2 から具体 phase 名を抜くか) | §4.1 致命傷 2 |
| (Q24-S1) | chapter 06a §4.3.1 (S1-A/B/C/D) のいずれを採用するか (= 06a-prep が「(S1) 解消」と書いた範囲は「存在確認 (i)」のみ、「代替案確定 (ii)」は未消化) | §5.3 |
| (Q25-21CNT) | chapter 10 §1 「21 件」の double-count 検証 (= 各 chapter 内 (Q*) listing を集計、分類表を §1 冒頭追加) | §5.6 |

**既存 21 件の reflect §N pinpoint** = chapter 10 §8.1 に登録済の next-session task に委ねる (= 本第二次 session の scope 外、ただし主要 (Q1)(Q2)(Q4) は Agent C 査読時に reflect 先 §N 実在確認済)。

---

## §8 第二次査読 修正推奨 chapter 一覧

### §8.1 設計 chapter 群 (design/01-10 + 06a-prep + handoff) 修正推奨

| chapter | 修正推奨件数 | 内容概要 |
|---|---|---|
| chapter 01 | 1 | §3.3「set=2:25」の致命傷候補 §2.2 解消方針に追従 |
| chapter 02 | 2 | §3.3「set=2:26」/ 表本体 の致命傷 §2.2 解消方針に追従 + §2.1 表 行 41 G1→MC1 (§5.1) |
| chapter 05 | 3 | §3.3 / §10 数値の致命傷 §2.2 追従 + 行 9 / 行 43 G1→MC1 (§5.2) |
| chapter 06a | 4 | §9 (S1) 持越分割 (§5.3) + §4.3 cross-ref §2.2.1→§2.2.2 (§5.4) + §5.1 表に uniform1i 追加 (16→17 method) + §5.5「32→30 entry point」(§2.4) |
| chapter 09 | 5 | §2.1 K placeholder と §5.2 template の整合修正 (§4.1 致命傷) + §2.2 phase timeline 図 (§4.2) + §3.5 Phase 0 (Q1) 入力契約 pointer (§3.1) + §4.2 shell ↔ 本実装 layout 互換明示 (§3.2) + §5.2 各 phase 「canary 検証 scene」annotation (§3.3) |
| chapter 10 | 2 | §1 冒頭に「21 件 分類表 (chapter 別)」追加 (§5.6) + (Q22-NUM) / (Q23-K) / (Q24-S1) / (Q25-21CNT) 新規 4 件登録 (§7) |
| inventory | 1 | §3.3 / §1 数値の致命傷 §2.2 解消方針追従 |
| handoff | 1 | §4.4 で (MC2) を「採用未済選択肢、chapter 05 §6.3 で MC1 採用確定」と注記 (§5.7) |

= **18 件 修正推奨** (= 致命傷 2 + 補強 16)、chapter 06b / 06c / 07 / 08 は修正推奨ゼロ (= 第一次修正完全反映 + 新規違反ゼロ)。

### §8.2 工程表 (00-charter + 04-frame-context) 修正実施済 (2026-06-03)

η-28 UBO pivot 確定の工程表反映漏れを 5 箇所追記、本 session で実施完了 (= AYA 指示「現在の工程表を設計書を前提に修正の必要があれば修正してください」反映)。

| doc | 修正箇所 | 内容 | 状態 |
|---|---|---|---|
| `00-charter.md` | §2 領域 4 末尾 | scope refine 2026-06-03 (η-28 後続) 注記追加 = UBO 化への pivot 受け入れ準備内包明示 + design chapter 群 source of truth 参照経路 | **完了** |
| `00-charter.md` | §5.3a 新設 | 設計 chapter 群 12 件 (design/01-10 + 06a-prep + η-28 handoff) cross-ref 表追加 = 領域 4 source of truth pointer | **完了** |
| `04-frame-context.md` | §1.1 末尾 | scope refine 2026-06-03 (η-28 後続) 注記追加 = UBO 受け入れ準備内包 + 30 entry point setter redirect + canary cvar 規約反映 + 詳細設計 source of truth 参照 | **完了** |
| `04-frame-context.md` | §1.3 領域 7 行 | η-28 descriptor set 帯構成 (4 帯 → 5 帯) + 84 UBO blueprint source of truth 整合明示追記 | **完了** |
| `04-frame-context.md` | §6.1 acceptance #5 | η-28 後続注記 = set=0:3 UBO 確定 + Phase 0-K migration roadmap + canary cvar on/off 切替動作 完了 marker 具体化 | **完了** |

= **5 箇所 工程表追記実施済**。`02-portage-execution.md` (closed 2026-05-28) / `03-state-machine-pso.md` (役割完了) / 領域 6 sub-doc / 領域 7 sub-doc / 領域 8 sub-doc は基本整合済 (= 修正不要、設計 chapter 群 source of truth pointer は 00-charter §5.3a + 04 sub-doc §1.3 経由で参照可能)。

### §8.3 修正推奨 総計

= **23 件** (= §8.1 設計 chapter 群 18 件 + §8.2 工程表 5 件)、うち **5 件 (工程表) 本 session 実施済**、**18 件 (設計 chapter 群) AYA 判断後 main session で実施予定**。

---

## §9 第二次査読 完了判定

### §9.1 判定: **PASS-with-major-fixes**

- 設計 chapter 群 (01-10) **構造健全** (= 全 chapter で原則 1 (call site 温存) / 原則 2 (Core 分散) の枠組み一貫)
- 致命傷候補 2 件 (§2.2 / §4.1) は **数値 / 表記の調整** で解消可能、設計 rework 不要
- 第一次修正の波及不徹底 (= chapter 02 内のみ反映で chapter 01/05/inventory/handoff 未追従) が第二次主要 finding 群、構造的な falsification ではなく **修正の質 (= 連鎖完遂) 不足**

### §9.2 実装 phase 入口前に必須

1. **AYA 判断 4 件** (= (Q22-NUM) / (Q23-K) / (Q24-S1) / (Q25-21CNT) を chapter 10 §1 へ追記して仰ぐ)
2. **修正 18 件** (= §8 修正推奨一覧、AYA 判断後に main session で実施、design-phase 規律維持で `indra/` には触れない)
3. **chapter 10 §8.1 spot check task** (= AYA 判断仰ぎ 21 件 全件 reflect 先 §N pinpoint verify)

### §9.3 次 session 開始 protocol

- 本 doc §1 sammary + §7 (AYA 判断仰ぎ新規候補) + §8 修正推奨一覧 を読込 → AYA に 4 件判断仰ぎ → 修正実施 → handoff §4.4 / chapter 10 §1 更新 → 設計 chapter 群 第三次査読 (= 修正後 state の最終 closure)

---

**= 第二次査読 完了。致命傷候補 2 件 + 修正推奨 18 件を確定。AYA 判断仰ぎ 4 件追加候補を §7 で集約。実装 phase 入口前に修正反映が前提**。
