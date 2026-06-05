# sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 handoff: 設計 chapter 群 (01-10) 査読 session

**起案日**: 2026-06-03
**位置付け**: 設計 chapter 群 (01-10) 全件起案完了 (commit `f67c68792b`) を受けて、**実装 phase 入口 (η-29 Phase 0) 着手前に設計書全体の査読を実施する独立 session**。査読観点 4 軸 (事実性 / 作業可能性 / 破綻有無 / 整合性) で全 chapter を点検 → 問題発見時は AYA に報告 → 修正方針確定 → 必要なら chapter 修正 commit → 査読 PASS で implementation-phase 入口へ移行。

---

## §0 査読 session の位置付け

- chapter 10 handoff §2.2 で提示した「選択肢 A (AYA 判断先行) / B (implementation-phase 入口先行)」に対し **選択肢 C (査読先行) が AYA 指示で追加採用**
- design-phase 完了直後・AYA 判断仰ぎ前の **第三者視点での独立検査** = 設計破綻 / 事実誤り / 不整合を AYA 判断材料に混入させない防壁
- 査読 PASS 後に選択肢 A (AYA 判断) または B (implementation-phase 入口) のどちらに進むかは AYA 判断
- **本 session は indra/ 改変ゼロ厳守** = design-phase 規律継続 (memory `feedback_design_phase_no_code_write`)、`design/` `inventory` 等 doc 修正のみ許可

---

## §1 査読対象 (= 設計 doc 全体)

### §1.1 主査読対象 (= design/ 配下 chapter 群)

| chapter | doc | 主要主張の type |
|---|---|---|
| 01 | `design/01-overview.md` | 全体概観 / 設計原則 2 / 用語 / 確定事項 13 件 / 進捗表 |
| 02 | `design/02-naming-convention.md` | UBO 命名規則 / 既存 84 UBO rename 表 |
| 03 | `design/03-cadence-classification.md` | cadence 5 分類 / cadence source rule (= 原則の根幹) |
| 04 | `design/04-codegen-ubo.md` | Codegen 機構 / pre-process pipeline / 出力契約 |
| 05 | `design/05-existing-inventory-link.md` | 既存 84 UBO mapping / bare uniform 集約対応表 |
| 06a | `design/06a-cache-structure-and-setter-redirect.md` | mUniformUBOLoc cache 構造 / setter Vulkan path 分岐 |
| 06a-prep | `design/06a-prep-phase0-measurement.md` | Phase 0 計測 spec / 解析 spec / 反映 flow |
| 06b | `design/06b-cadence-update-site-and-dirty.md` | cadence 別 update site / dirty / flush / `forwardToUboUpload` interface |
| 06c | `design/06c-descriptor-set-bind-wiring.md` | descriptor set 配線 / UB_* 4 binding ↔ 84 blueprint 接合 / mUseUBO flag |
| 07 | `design/07-vulkan-api-state.md` | 現状 Vulkan API 棚卸し / device limit / pool / ring buffer / fence sync / 共通 PSO layout |
| 08 | `design/08-build-codegen-pipeline.md` | build 統合 / CMake / glslang / 増分 build cache |
| 09 | `design/09-phase-roadmap.md` | Phase 番号体系 / 1 UBO ずつ migration / 3 OS 確証 / OpenGL 撤廃 |
| 10 | `design/10-open-questions.md` | AYA 判断仰ぎ候補 21 件 / 実装 phase 入口消化 listing / inventory §7 接続 |

### §1.2 副査読対象 (= 設計 chapter 群の前提資料)

| doc | 役割 |
|---|---|
| `ayastorm-r41-ubo-current-state-inventory.md` | 現状棚卸し (= 設計の事実基盤、§3 84 UBO blueprint / §4 host C++ API / §7 残課題 9 件) |
| `reference-shader-location-map.md` | shader file 場所 reference (= 設計 chapter 内 file path 参照の正解集) |

### §1.3 副副査読対象 (= 設計 chapter 起案 handoff 群)

設計 chapter 起案 session の handoff doc は **本 session では原則 read 不要** (= chapter 本文に決定事項が反映済の前提)。例外: 査読中に「決定経緯が不明」「default 採用理由不明」等で本文では判断つかない時のみ該当 handoff を pinpoint Read。

---

## §2 査読観点 (= 4 軸)

### §2.1 [事実性] 設計書の主張は事実か

**問い**: 設計書が「現状こうである」「source code でこう書かれている」「inventory §N にこう記載」と主張する箇所は、実際にその通りか。

**点検対象**:
- inventory §3 / §4 への参照 (= 84 UBO blueprint / host C++ API 棚卸し) が inventory 本体と一致するか
- source code への参照 (= `LLGLSLShader::uniform*fv` line 番号 / `llvkloader.{cpp,h}` 棚卸し / `MaterialUBO` 宣言位置等) が実 source と一致するか
- 「✓ 解消」マーク (= inventory §7 / 各 chapter §N) が実際に該当 chapter で解消されているか
- 引用された数値 (= 84 UBO / 79 binding / 49 sampler / 53 → 54 set=3 Legacy / set=2 25 個 / UB_* 4 binding 等) が一貫しているか

**点検方法**:
- 数値主張 = grep + count で実数確認
- source line 参照 = 該当 file 該当 line を Read で確認
- inventory 参照 = inventory 本体該当節を Read で確認

### §2.2 [作業可能性] この設計で実装 phase の作業が可能か

**問い**: 設計書通りに実装 phase に着手して、各 Phase の作業が実際に完遂可能か。手順が抜けている / 入力が揃わない / 出力定義不明 等の致命的不備がないか。

**点検対象**:
- chapter 04 §3-§7 Codegen pipeline の **入力 → 処理 → 出力** が具体的に決まっているか
- chapter 06a / 06b / 06c の redirect 層 / forwardToUboUpload / descriptor set bind 配線が **具体的 signature + 配線位置** まで決まっているか
- chapter 06a-prep §2-§4 Phase 0 計測 spec が **そのまま手順書として実施可能** な粒度で書かれているか
- chapter 07 §3-§10 が **実 Vulkan API call レベル** で配線可能 (= API 名 / parameter / 順序) か
- chapter 08 §1-§16 build 統合が **CMake target / DEPENDS / script 配置先** まで決まっているか
- chapter 09 §3-§9 Phase 別 Exit Criteria が **客観判定可能** な形で書かれているか
- chapter 10 §1 AYA 判断仰ぎ候補 21 件が **判断後の reflect 先 chapter §N** まで指定されているか

**点検方法**:
- 各 chapter §N で「次の作業はこれ」と書かれた箇所を抽出 → 後続 chapter / 後続 Phase で実際に拾えるか trace
- 「default 採用案」が AYA 判断仰ぎなしで Phase 0 着手可能か確認 (= Phase 0 入力に必要な確定事項が揃っているか)
- 入力 → 出力チェーンの **どこかで途切れていないか** 確認

### §2.3 [破綻有無] 設計に論理的矛盾はないか

**問い**: 設計の決定事項どうし / chapter どうし / 原則どうしで論理矛盾が発生していないか。

**点検対象**:
- 原則 1 (upstream OpenGL 取り込みやすさ維持) と原則 2 (Core プロセス分散実現) の **trade-off が両立** しているか、片方を犠牲にしていないか
- cadence source rule (chapter 03 §3、新規 viewer settings 追加禁止) と chapter 09 §11 (Q5) / chapter 07 §6 PSO cache cvar 等で **新規 cvar 追加** が衝突していないか
- name-based call site API 温存 (= 原則 1 由来、確定事項 #2) と Codegen が `uniform*fv("name", ...)` を perfect hash 経由で UBO 経路に redirect する設計が **真に call site 改変ゼロ** で成立するか
- chapter 06a §9 (Q1)(Q2)(R1)(S1) と chapter 06b / 06c / 07 / Phase 0 の **解消先指定** が後続 chapter で実際に解消されているか (= 解消約束の不履行検出)
- chapter 10 §1.1 (V1') (V3') (S3') (W) と chapter 07 §3/§4/§5/§6 default 採用案が **同じ案を指している** か (= 表記揺れ・default 不一致検出)
- chapter 10 §1.4 (M) (N) (O) と chapter 06c §10 (M) (N) (O) の default 採用案が **同じ案を指している** か
- 確定事項 13 件 (01-overview §5) と各 chapter §N の決定内容が **逆行していない** か

**点検方法**:
- 原則 / 確定事項 / cadence source rule を起点に逆引き grep
- 同一項目 ID (= (V1) (Q1) (K) 等) の出現箇所を全 chapter で集約 → 内容一致確認
- 「禁止」「不可」「ゼロ」等の **強い制約語** を全 chapter grep → 後続 chapter で違反していないか確認

### §2.4 [整合性] 用語 / 表記 / 数値が全 chapter で一貫しているか

**問い**: 同じ概念が複数の用語で呼ばれていないか / 同じ数値が章で食い違っていないか / numbering が連続しているか。

**点検対象**:
- 用語: cadence / Codegen / redirect 層 / bare uniform / descriptor set 帯 / per-program / per-asset / per-skin / per-draw / per-frame の表記
- 同一概念の同義語: 「UBO blueprint」「84 UBO」「論理 binding」「物理 instance」等の使い分けが定義通りか
- 数値: 84 UBO / 79 binding / 49 sampler / 16 不明 cadence / 4 UB_* binding / set 0-3 の 4 帯 / cadence 5 分類 / set=3 Legacy 54 / set=2 25 等
- 項目 ID numbering: (V1) (V1') / (Q1)-(Q5) / (K) (L) (M) (N) (O) / (A1) (P) (G) (B1)-(B5) / (P1)-(P4) 等の ID が **章をまたいで衝突していないか** (= chapter 09 handoff §1.4 で (Q1)-(Q5) 改名済の経緯、再衝突がないか確認)
- 引用記法: `(memory ID)` / file path / chapter §N 参照が一貫しているか
- 「default 採用案」「default 提案」「default」の表記揺れ
- 進捗マーク: ✅ 起案済 / 解消済 / 未起案 / 未解消 / 部分解消 等の表記揺れ

**点検方法**:
- 用語 grep → 同義語使用箇所を特定
- 数値 grep → 表中の数値と本文記述を突合
- 項目 ID grep → 同 ID で別概念を指していないか確認

---

## §3 査読方法 (= 各観点別 procedure)

### §3.1 全観点共通の手順

1. **chapter 01 + 03 から開始** = 設計原則 / 確定事項 13 件 / cadence source rule を基準点として固定 (= 査読の判定基準)
2. **chapter 02 → 04 → 05 → 06a → 06a-prep → 06b → 06c → 07 → 08 → 09 → 10 の順** で各 chapter を 4 観点で点検
3. **各 chapter §N 単位** で点検、findings を report doc に逐次記録
4. **inventory § 主参照** を chapter 跨ぎで集約点検 (= 副査読対象との突合は最後にまとめて実施)
5. **AYA 判断仰ぎ候補 21 件** (= chapter 10 §1) の default 採用案を最後にまとめて再点検 (= 4 観点全部に該当する複合査読)

### §3.2 [事実性] 点検手順

- 数値主張 = `Grep` + 集計で実数確認
  - 例: 「84 UBO blueprint」 → inventory §3 表で 84 行 / 84 entry あるか確認
  - 例: 「set=3 Legacy 54 個」 → inventory §3.4 確認 + 「53」表記残骸の grep
- source line 参照 = `Read` で該当 line 確認
  - 例: 「`LLGLSLShader::uniform*fv()` line 2166-2557」 → llglslshader.cpp の該当 line Read で setter 関数群存在確認
  - 例: 「`glUniform4iv` 内部で `glUniform1iv` 呼出 bug 疑い line 2330」 → 該当 line Read で確認
- inventory 参照 = inventory § 該当節 Read

### §3.3 [作業可能性] 点検手順

- 各 chapter §N で「実装する」「Phase X で実施」「次 chapter で確定」と書かれた約束を listing
- listing した約束を後続 chapter / 後続 Phase で実際に拾えるか trace
- 拾えない場合 = 作業可能性違反 として report 記録

### §3.4 [破綻有無] 点検手順

- 原則 1 / 原則 2 / 確定事項 13 件を起点に逆引き grep
- 原則違反候補 (= 新規 cvar / call site 改変 / Codegen 経由しない upload / OpenGL path 並走 vs 早期撤廃の論争点) を全 chapter で点検
- 同一項目 ID (= (V1') (Q1) (K) (M) (N) (O) 等) の出現箇所を全 chapter で集約 → default 採用案 / 内容が一致するか確認
- 不一致 = 破綻 として report 記録

### §3.5 [整合性] 点検手順

- 用語表 (= chapter 01 §3 用語定義) を起点に同義語 / 略語 grep
- 数値表 (= inventory §1 / §3 等) を起点に数値突合
- 項目 ID 一覧 (= 本 handoff §4.4) を起点に ID 衝突 grep

---

## §4 report 形式

### §4.1 report doc 配置先

新規 doc を作成: `docs/specs/ayastorm-r41-gl-removal/design-review-2026-06-03.md`

### §4.2 report doc 構成

```
# 設計 chapter 群 (01-10) 査読 report (2026-06-03)

## §0 査読 session 概要 (= 4 観点 / 査読対象 / 査読方法)

## §1 査読結果 sammary
- [事実性] 違反 N 件 (詳細 §2)
- [作業可能性] 違反 N 件 (詳細 §3)
- [破綻] 違反 N 件 (詳細 §4)
- [整合性] 違反 N 件 (詳細 §5)
- 総合判定: PASS / PASS-with-fixes / FAIL

## §2 [事実性] findings
### §2.1 chapter NN §M
- 主張: 「...」
- 期待: 「...」
- 観測: 「...」
- 判定: ✓ 一致 / ✗ 不一致
- 修正案: 「...」

(... 各 finding を同形式で列挙)

## §3 [作業可能性] findings (同形式)
## §4 [破綻] findings (同形式)
## §5 [整合性] findings (同形式)

## §6 AYA 判断仰ぎ候補 21 件再点検
- 各候補に対し default 採用案が 4 観点 PASS か独立評価

## §7 修正推奨 chapter 一覧
- 修正が必要な chapter §N と修正案を集約

## §8 査読 session 完了判定
- PASS なら次 phase 移行可、PASS-with-fixes なら修正 commit 後再点検、FAIL なら根本見直し
```

### §4.3 report doc 起案方針

- 違反 0 件で **PASS** の場合も report doc を残す (= 査読実施履歴として保存、live doc ではなく snapshot)
- 違反 1 件以上で **PASS-with-fixes** = 修正方針 AYA 確認 → 修正 commit → report 更新 (= 査読再実施部分のみ)
- 違反多数で論理破綻が明らかな場合 **FAIL** = 設計 chapter 起案やり直し検討を AYA に上申

### §4.4 項目 ID 一覧 (= 整合性査読の基準表)

設計 chapter 群で使用中の項目 ID (= 査読時の衝突確認基準):

| 出典 chapter | ID prefix | 範囲 | 用途 |
|---|---|---|---|
| 04 §10 | A1 / G / D / P | 4 件 | std140 / 動的 uniform / parse 手段、A1/G/P は 08 §17 で詳細化 |
| 05 §10 | E' / F / H1 / H2 / H3 / MC (旧 G) | 6 件 (E は ✓ 解消、MC は 2026-06-03 (G) から rename + ✓ 解消) | binding 重複 / MaterialUBO 処遇 / 集約表 live / material cadence (= 04 §10 (G) との衝突回避 rename) |
| 06a §9 | H1b / Q1 / Q2 / R1 / S1 / T1 | 6 件 | cache / mUseUBO / forwardToUboUpload / LLStaticHashedString / API / bug 疑い |
| 06a-prep §7 | P1 / P2 / P3 / P4 | 4 件 | Phase 0 入口 grep |
| 06b §8 | K / L / M / U1 / U2 / U3 / U4 | 7 件 | dirty 粒度 / per-draw 最適化 / thread (= 06b 内 (M) は thread-safe 文脈、06c (MD) と別概念) / triple-buffer / mValue 適用外 |
| 06c §10 | MD (旧 M) / N / O / V1 / V2 / V3 / S3 / MC1 / MC2 (旧 G1 / G2) | 9 件 (MD / MC1 / MC2 は 2026-06-03 rename) | descriptor 配置 (= 06b §8 (M) thread-safe との衝突回避で MD rename) / mUseUBO initial / UB_* 拡張 / set=1 limit / binding 重複 / layout / sampler / per-draw material cadence |
| 07 §12 | V1' / V3' / S3' / W / W2 / RB (旧 R1) / PSC / RF | 8 件 (RB は 2026-06-03 rename) | set=1 split / layout / sampler 配置 / pool / prealloc / ring buffer (= 06a §9 (R1) LLStaticHashedString との衝突回避で RB rename) / PSO cache / fence throttle |
| 08 §17 | A1 / P / G(B3) / B1 / B2 / B4 / B5 / P-future / cache-grow | 9 件 | Codegen 実装詳細 (G/B3 は 04 §10 (G) と同概念 = 詳細化、05 §10 旧 (G) とは別概念 = 05 側 MC rename で解消) |
| 09 §11 | Q1 / Q2 / Q3 / Q4 / Q5 | 5 件 | Phase Roadmap (= 06a §9 の Q1/Q2 と衝突懸念、09 §11 で R1-R5 → Q1-Q5 に rename 経緯あり) |
| 10 §1.5 | F | 1 件 | MaterialUBO 処遇 (= 05 §10 (F) 同一) |

**衝突懸念**:
- **(Q1) (Q2)** = 06a §9 (cache / mUseUBO 文脈) vs 09 §11 (Phase Roadmap 文脈) で **同 ID 別概念**
- **(R1)** = 06a §9 (LLStaticHashedString) vs 07 §12 (ring buffer 容量) で **同 ID 別概念** → 2026-06-03 査読 §5.2 で 07 §12 (R1) → (RB) rename 解消
- **(M)** = 06b §8 (thread-safe) vs 06c §10 (descriptor 配置) で **同 ID 別概念** → 2026-06-03 査読 §5.3 で 06c §10 (M) → (MD) rename 解消
- **(G)** = 04 §10 (perfect hash generator) vs 05 §10 (per-material cadence、§6.3 G1 確定済) vs 08 §17 (G/B3 perfect hash generator) で **同 ID 別概念** (= 2026-06-03 査読 §5.6 本査読新発見、本 handoff 初版で見落とし) → 2026-06-03 査読 §5.6 で 05 §10 (G) → (MC) rename 解消 (= 06c G1/G2 → MC1/MC2 連動 rename 含む)。04 §10 (G) と 08 §17 (G/B3) は同概念で「04 §10 で概念提起 → 08 §17 で詳細化」関係のため rename 不要
- **(V1) (V3) (S3)** = 06c §10 vs 07 §12 (V1') (V3') (S3') で **prime 付き別 ID 関係**、prime 無しと有りで指す概念が一致しているか確認必要
- **(P) (A1)** = 04 §10 と 08 §17 で **同 ID、08 で詳細化** 関係、内容一致確認必要

整合性査読時、これらの衝突 ID が **文脈で識別可能** か検証 (= 同一 chapter 内 / 単一節内で複数衝突 ID 同時出現していないか確認、出現していれば rename 推奨)。本 handoff 初版で (G) 衝突を見落とした reflect として、**新規 chapter 起案時は本表に prefix 列を新規登録時点で追加** + **複数 chapter 共通使用 prefix (= A1 / G / P 等) は本表内 cross-ref 行を明示**を遵守する。

---

## §5 必須 Read (= 査読 session 入口で読む最低限)

1. **本 handoff doc** (= 査読観点 4 軸 / 査読方法 / report 形式 / 項目 ID 衝突一覧)
2. **`design/01-overview.md` §3 用語定義 + §5 確定事項 13 件** (= 査読判定基準)
3. **`design/03-cadence-classification.md` §2 cadence 5 分類 + §3 cadence source rule** (= 原則の根幹、破綻 / 整合性査読の基準)

**意図的に除外**:
- chapter 02 / 04-10 / 06a-prep / inventory = 査読対象、**全 chapter 順次 Read が査読本体作業** のため事前 Read 不要 (= 査読 session 中に §3.1 順序で各 chapter Read)
- 設計 chapter 起案 handoff 群 = 査読中に「決定経緯不明」時のみ pinpoint Read

---

## §6 規律 (絶対遵守)

- **design-phase 継続**: `indra/` 改変ゼロ厳守 (memory `feedback_design_phase_no_code_write`)、`docs/specs/ayastorm-r41-gl-removal/` 配下のみ修正許可
- **修正 commit は AYA 判断後**: 査読で違反検出しても **即修正 commit しない**、report に記録 → AYA 確認 → 修正方針確定 → 修正 commit (memory `feedback_no_auto_commit`)
- **tests/**: commit / 確認 / 言及禁止 (memory `feedback_tests_dir_never_commit`)
- **事前 Read 規律**: §5 の 3 件のみ、その他は査読中 pinpoint Read (memory `feedback_handoff_minimal_pre_req_read`)
- **chapter 順次 Read**: §3.1 順序 (01 / 03 → 02 / 04-10 / inventory) で各 chapter を読み切ってから次へ、複数 chapter 並行 grep で context 圧迫しない
- **findings 即記録**: 1 件発見 → 即 report doc に追記、最後にまとめて書く方式取らない (= 抜け漏れ防止)
- **agent 積極活用** (memory `feedback_use_agents_proactively`): 「同 ID 出現箇所 grep」「数値突合」「source line 参照確認」等は **Explore agent に並列発注**、main session の context 節約
- **context 残量監視** (memory `feedback_proactive_handoff`): 査読途中で context 圧迫したら handoff doc 起案 → 次 session に継続

---

## §7 査読 session 完了マーク

### §7.1 PASS 判定

- 違反 0 件 → report doc 起案 → AYA 報告 → commit 指示後 commit → 次 phase (= 選択肢 A AYA 判断 / B implementation-phase 入口) 指示待ち

### §7.2 PASS-with-fixes 判定

- 違反 N 件 (= 軽微 / 限定的) → report doc 起案 → AYA 報告 → 修正方針 AYA 確認 → 修正 commit → 修正部分の再点検 (= report doc 追記) → 違反 0 件で PASS → §7.1 へ

### §7.3 FAIL 判定

- 違反多数 / 論理破綻明白 → report doc 起案 → AYA 報告 → 設計 chapter 起案やり直し方針を上申 → AYA 判断待ち

---

## §8 次 session 起動メッセージ要約

次 session = /clear 後の起動メッセージ案:

```
設計 chapter 群 (01-10) 起案完了 (commit f67c68792b) を受けて、
実装 phase 入口前の独立査読 session を開始する。

まず以下 3 件のみ Read (= handoff §5 規律):
1. docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-review.md
2. docs/specs/ayastorm-r41-gl-removal/design/01-overview.md §3 + §5
3. docs/specs/ayastorm-r41-gl-removal/design/03-cadence-classification.md §2 + §3

handoff §3.1 順序で各 chapter を 4 観点 (事実性 / 作業可能性 / 破綻 / 整合性)
で点検、findings を逐次 docs/specs/ayastorm-r41-gl-removal/design-review-2026-06-03.md
に記録。Explore agent を grep / 数値突合 / source line 参照確認に並列活用。

絶対規律: indra/ 改変禁止 / tests/ 触らない / 違反検出しても即修正 commit せず
report → AYA 確認後 / 事前全読禁止。

進めて。
```

---

**= 本 handoff doc + §5 の Read 3 件で査読 session 起動再現可能**。査読 PASS で implementation-phase 入口 (= η-29 Phase 0 計測) または AYA 判断 session のどちらに進むかは査読 PASS 後の AYA 判断。
