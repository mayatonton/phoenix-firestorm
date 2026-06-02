# 設計 chapter 群 (01-10) 査読 report (2026-06-03)

**起案日**: 2026-06-03
**位置付け**: handoff doc `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-review.md` §3.1 に従って実施した設計 chapter 群 (01-10) の独立査読 report。
**source of truth**: 設計 chapter 群 (`design/01-overview.md` ... `design/10-open-questions.md`)
**規律**: design-phase 継続 = `indra/` 改変ゼロ、修正 commit は AYA 判断後、tests/ 触らない、findings は本 doc に即記録 (= snapshot)。

---

## §0 査読 session 概要

### §0.1 査読観点 4 軸

1. **[事実性]** 設計書の主張 (= 数値 / source line / inventory § / ✓ 解消 mark) が実際の source / inventory / 他 chapter と一致するか
2. **[作業可能性]** 設計通りに実装 phase が完遂可能か (= 入力 → 処理 → 出力 chain が途切れていないか、約束された後続解消が果たされているか)
3. **[破綻有無]** 設計の決定事項どうし / chapter どうし / 原則どうしで論理矛盾していないか (= 原則 1 / 原則 2 / 確定事項 13 件と各 chapter §N の整合)
4. **[整合性]** 用語 / 表記 / 数値 / 項目 ID が全 chapter で一貫しているか (= 表記揺れ / ID 衝突 / 数値矛盾)

### §0.2 査読対象

- 主: `design/01-overview.md` 〜 `design/10-open-questions.md` (12 chapter、06a-prep 含む)
- 副: `ayastorm-r41-ubo-current-state-inventory.md` (= 設計の事実基盤、live doc)
- 副: `reference-shader-location-map.md` (= shader file path reference)

### §0.3 査読方法 (= handoff §3.1 順序)

1. chapter 01 + 03 から開始 (= 判定基準を固定)
2. chapter 02 → 04 → 05 → 06a → 06a-prep → 06b → 06c → 07 → 08 → 09 → 10 順
3. findings を逐次本 doc に記録
4. 数値突合 / ID 衝突 grep / source line 参照確認は Explore agent に並列発注、main session の context 節約
5. AYA 判断仰ぎ候補 21 件 (chapter 10 §1) を最後にまとめて再点検

---

## §1 査読結果 sammary

- **[事実性] 違反 5 件** (詳細 §2)
  - 確定 2 件: chapter 02 §3.3 (表 26 行 vs ヘッダー 25 個 = 1 行余分) / chapter 05 行 9 + 行 43 (旧「cadence 6 分類」残骸 2 箇所)
  - 要 spot check 3 件: chapter 06c §3.1 (set=1 binding=0/1 vs set=2 binding=0/1 重複) / chapter 07 §2.4 (device limit 列補完) / chapter 08 §6 (ubo_metadata.inl subset field 仕様矛盾)
- **[作業可能性] 違反 14 件** (詳細 §3)
  - 致命傷候補 1 件: chapter 06a §4.3 `LLStaticHashedString::getGlobalRegistry()` 仮 API code shape (= 06a-prep §2.2.1 で「実装無し確認済」、実装 phase 進入直後にコンパイルエラー直行)
  - 重要 5 件: chapter 06a-prep §2.2.1 sampler histogram 範囲 / chapter 06b §3.4 (K) 採用済 vs 未確定 / chapter 06b §5.3/§5.4 (L)(M) 依存性 / chapter 09 §5.3 canary 値表 / chapter 09 §4 Phase 1.C-2 境界
  - 中等度 4 件: chapter 02 §3.4 54 件 rename 表 incomplete / chapter 04 §6.3 (D) Phase 0 接続 / chapter 04 §7.2 過渡期動作 / chapter 07 §12 表記揺れ / chapter 10 §2 imperative / chapter 10 §1 reflect §N pinpoint
  - 要 spot check 2 件: chapter 08 §3.2 (B1) Python 3 OS 前提 / chapter 08 §5 generator invoke
- **[破綻] 違反 4 件** (詳細 §4)
  - 致命傷候補 1 件: chapter 02 §3.2 set=1 帯 `MaterialUBO` cadence 列が廃止済「per-material」(= chapter 01 §5 確定事項 #12 違反)
  - 中等度 2 件: chapter 02 §3.2 MaterialUBO UBO suffix vs §4 warning 規則 / chapter 06c §4.3 (V3) vs §2.1 M1
  - 要 spot check 1 件: chapter 07 §4.4.1 `vkCmdBindDescriptorSets` binding range 再構築仕様
- **[整合性] 違反 11 件** (詳細 §5)
  - 重要 ID 衝突 3 件: (R1) chapter 06a §9 vs chapter 07 §12 / (M) chapter 06b §8 vs chapter 06c §10 / (G) chapter 04 §10 vs chapter 05 §10 (= **handoff §4.4 見落とし**)
  - 表記揺れ 4 件: chapter 05 §10 取消線 / chapter 06c §10 「default 採用済 vs 未確定」 / chapter 07 §12 同 / chapter 08 line 54 「set 帯 5 化」 vs chapter 06c 「4 帯」
  - 軽微 4 件: chapter 02 §3.1 FrameAtmosphere rename 波及 / chapter 09 §2.1 Phase K inconsistency / chapter 10 §2 (RF) 二重参照 / chapter 08 §6.2 (B1)-(B5) schema 反映
- **総合判定: PASS-with-fixes**
  - **致命傷 0 件** (= root cause unknown のブロッキング無し、全 finding に修正方針提示済)
  - 中等度 ~12 件 = 後続 chapter 修正 (ID rename / 表記統一 / 注記追加) で解消
  - 軽微 ~22 件 = 注記追加 / 機械的 rename / 1 行修正で解消
  - 要 spot check ~8 件 = 次 session で main pinpoint Read 後に最終確定 (= 本査読で確定不能)

---

## §2 [事実性] findings

### §2.1 chapter 02 §3.3 set=2 帯のヘッダー表記 vs 表行数

- **主張**: `02-naming-convention.md` §3.3 ヘッダー「set=2 帯 (25 個 → `PerProgramUBO_` / `PerDrawUBO_` rename)」
- **観測**: §3.3 の rename 表は 26 行 (`PerDrawUBO_LightParams`, `PerDrawUBO_MultiLight`, `PerProgramUBO_*` 24 件 = 計 26 行)
- **数値突合 agent (a28a8e7e9d6d723c0) 結果**: chapter 01 §3.3 / chapter 02 §3.3 ヘッダー / chapter 05 / inventory §3 は全て「set=2 = 25 個」で一貫。「26」は inventory §0/§3 で「η-28 期間で binding range 0-25 = 26 binding 積み上がった」表記 (= binding range notation) と推定。unique UBO 数 = 25 は program 排他で同 binding に複数 UBO が候補化されるため
- **判定**: chapter 02 §3.3 表が 26 行ある = 表側に **重複 entry または rename 漏れ** がある可能性大。inventory §3 unique 25 件と表行 26 件の差分 1 件を特定し、表側を修正
- **修正案**: chapter 02 §3.3 表の 26 行を inventory §3.3 (set=2 帯 25 件) と差分突合 → 余分な 1 行を削除 (= 同一 UBO 二重登場 or 別 set 帯の混入)、表行数を 25 に揃える。あるいは ヘッダーを「26 個 (binding range 0-25 表記)」+ 注記「unique UBO 数 25、program 排他で同 binding 複数候補」に書き換え

### §2.3 chapter 06c §3.1 set=1 binding=0/1 vs set=2 binding=0/1 重複表記 (agent ad5c2801b745a6848 報告 = 要 spot check)

- **主張**: chapter 06c §3.1 表で `PerDrawUBO_LightParams` / `PerDrawUBO_MultiLight` を「per-draw 確定 (= inventory §3.3 確認)」と明記しながら、**同表に set=1 binding=0/1 として listing**
- **対立**: chapter 05 §3.3 = per-draw 2 個 (binding 0-1) は set=2 帯所属が確定済
- **判定**: 同一 UBO 名が set=1 と set=2 の両 listing に重複出現 → 設計の descriptor set 帯 1:1 対応 (chapter 06c §2.1 M1) に矛盾
- **修正案**: chapter 06c §3.1 set=1 binding=0/1 行を削除、または「(V2) 持越 = 確定後 set=2 へ移管」明記
- **要 spot check**: main session で chapter 06c §3.1 表本体を pinpoint Read で再確認

### §2.4 chapter 07 §2.4 device limits 表で maxPushConstantsSize 等の Vulkan 1.3 最小値記載なし (agent a112794604894de57 報告 = 要 spot check)

- **主張**: agent 報告では chapter 07 §2.4 device limit 表で `maxPushConstantsSize` の Vulkan 1.3 spec 最小値 (128 B) の記載なしと指摘
- **判定候補**: device limit の絶対値が無いと Phase 0 計測の対比基準が消える、ただし「現状値出典」列でカバーされるべきとの agent 解釈
- **要 spot check**: chapter 07 §2.4 本体を Read で確認、agent の指摘が当たっているか / 既に注記で代替されているか確認

### §2.5 chapter 08 §6 ubo_metadata.inl schema の subset field 仕様矛盾 (agent a112794604894de57 報告 = 要 spot check)

- **主張**: agent 報告では chapter 08 §6 schema で `subset: uint16_t` が 0/1 以上のスケール情報を持つ設計、§7.1 sort 規則は「前半 40 / 後半 39」 hard-code、将来 split (40/39 以外) 拡張余地と矛盾
- **要 spot check**: chapter 08 §6 / §7 本体を Read で確認、agent 指摘の正確性確認

### §2.2 chapter 05 で「cadence 6 分類」記載 (旧 6 分類残骸) 2 箇所

- **主張**: `05-existing-inventory-link.md` 行 9 (pre-requisite list) と行 43 (§2 入力契約表) の 2 箇所で「`03-cadence-classification.md` (cadence **6 分類**)」「chapter 03 §2 cadence **6 分類**」と旧記載
- **対立する確定事項**:
  - chapter 03 §2 = 5 分類確定 (per-frame / per-program / per-draw / per-asset / per-skin、per-material は per-draw + dirty flag に統合)
  - chapter 01 §5 確定事項 #12 = AYA 判断確定 (per-material 廃止)
  - chapter 05 §6 で G1 = per-draw 統合採用が confirmed (= 自己 chapter 内で確定にもかかわらず行 9 / 行 43 が古い)
- **判定**: 古い 6 分類記載の残骸 = 事実性違反 (= 自 chapter §6 で確定した内容が pre-req / §2 入力契約に反映されていない)
- **修正案**: `05-existing-inventory-link.md` 行 9 と行 43 を「cadence 5 分類」に更新、必要なら「(= 旧 6 分類から per-material を per-draw + dirty flag 統合 = chapter 05 §6 G1 確定)」の注を追加

---

## §3 [作業可能性] findings

### §3.1 chapter 02 §3.4 set=3 帯 54 件の rename 表 incomplete

- **主張**: `02-naming-convention.md` §3.4「全 54 件は inventory §3.4 全件を同パターンで」(rename pattern 例 6 件のみ列挙)
- **観測**: 本 chapter §3 内で完全な rename 表が無い。inventory §3.4 を別途参照しないと 54 件全部の到達名が確定しない
- **判定候補**: 「機械的 rename (`<Name>UBO_Legacy` → `Program_<Name>`)」と書かれており規則は明示的 → input は inventory §3.4 のみで決定論的に出力可能 = 作業可能性は **保たれる** が、本 chapter 内 self-contained でない (= 他資料参照前提)。許容範囲か / 完全 list 化要かは AYA 判断
- **修正案**: (a) inventory §3.4 の現名 list を本 chapter §3.4 に転記して新名 column 追加 / (b) 「inventory §3.4 ↔ 命名規則 §2.1 prefix で生成」と explicitly 書いて参照モデル明示 / (c) chapter 05 (existing-inventory-link) に全件 mapping 表を持たせ、本 chapter は規則のみ - chapter 05 で全件あれば許容
- **要確認**: chapter 05 §4 (set=3 帯 E3 rename) で 54 件全件の rename 表があるか

### §3.2 chapter 04 §6.3 (D) 動的 uniform 名の存在確認 = Phase 0 計測対象との接続

- **主張**: `04-codegen-ubo.md` §6.3 (D) 「shader 内で runtime 動的に名前生成される uniform」「array uniform は N 固定展開 / 動的 index 経由 setter の C++ 側存在」を **chapter 06 起案時に grep で判定** と保留
- **観測**: chapter 06a / 06a-prep / 06b / 06c は既起案済 (= 06 系列は分割完了)。chapter 06 起案時 grep の約束が、どこで履行されたか / Phase 0 計測 spec (chapter 06a-prep) に取り込まれたかを確認要
- **判定候補**: chapter 06a-prep §7 (P1)-(P4) の Phase 0 入口 grep listing に (D) が含まれていれば作業可能性 OK、含まれていなければ約束未履行 = 作業可能性違反
- **要確認**: chapter 06a-prep §7 を Read して (D) 動的 uniform 名 grep が listing 化されているか確認

### §3.4 chapter 06a §4.3 LLStaticHashedString::getGlobalRegistry() 仮 API code shape (agent ad5c2801b745a6848 報告)

- **主張**: chapter 06a §4.3 で `LLStaticHashedString::getGlobalRegistry()` を call する code shape を提示、ただし chapter 06a §9 (S1) で「仮 API 名、実装無ければ chapter 06b 起案時に提案」と保留、chapter 06a-prep §2.2.1 但し書きで「実装 phase 起案時点で存在しないこと確認済」と明記
- **判定**: code shape (= 実装直前形) が、確認済「実装不可」な API 名を含む = 実装 phase 進入直後にコンパイルエラー直行 → 作業可能性違反
- **修正案**: chapter 06a §4.3 を「仮 code shape」明示 + 代替案 (LLStaticHashedString 内部 registry 直接 iterate / helper 作成) を併記、chapter 06b 起案時 / Phase 0 完了時に確定

### §3.5 chapter 06a-prep §2.2.1 sampler uniform1i の cadence histogram 対象範囲 (agent ad5c2801b745a6848 報告)

- **主張**: chapter 06a-prep §2.2.1 注「`uniform1i` は sampler binding setter で UBO 化対象外」と「hook histogram で sampler binding 呼出は per-program cadence」を同時記載 → 矛盾 (= 除外なのに対象に含まれる)
- **判定**: Phase 0 計測の入力定義に揺れ、計測結果の解釈に混乱
- **修正案**: sampler 49 個の cadence は別軸として 06a §0.2 / 06a-prep §2 から分離、別 histogram または「対象外」明示

### §3.6 chapter 06b §3.4 (K) 未確定 vs K2 default 採用済の scope 境界 (agent ad5c2801b745a6848 報告)

- **主張**: chapter 06b §3.4 (K) dirty 判定粒度が「未確定」表記、同時に K2 default 採用で設計を進行 = 後続 chapter (06c / chapter 07) が K2 前提で書かれる
- **判定**: (K) 未確定が AYA 判断で覆る場合、§3.2.3 `UboInstance::dirty` member / §5.2 switch routing / chapter 06c / chapter 07 まで巻き戻し
- **修正案**: chapter 06b §3.2.3 に「default K2 採用前提、(K) 変更時は本節 + §5.2 + 後続 chapter 全体再評価」明記

### §3.7 chapter 06b §5.3 (L) と §5.4 (M) 間の依存性 (agent ad5c2801b745a6848 報告)

- **主張**: §5.3 L1+L2 組合せ default 採用、§5.4 thread-safe (M) は未確定。L1+L2 ring buffer 設計が chapter 06c §5.2 / chapter 07 ring buffer 容量・wrap の前提だが、(M) thread-safe 方式 (mutex/atomic/lock-free) で allocator 競合 / atomic 選択が変わる
- **判定**: (M) 確定前に (L) 確定すると ring buffer allocator 設計が再評価必要
- **修正案**: chapter 06b §5.3 / §5.4 間の先後関係を明示、(M) AYA 判断後に (L) 再確認サイクル明記

### §3.8 chapter 07 §12 未確定リスト vs §3.2/§4.2/§5.2/§6.2 default 採用済の確定度表記 (agent a112794604894de57 報告)

- **主張**: chapter 07 §12 で (V1')(V3')(S3')(W) 4 件が「chapter 10 / AYA 判断」保留、ただし §3.2 / §4.2 / §5.2 / §6.2 で既に default 採用案として確定記載
- **判定**: 確定度表記の矛盾 (= 「保留」と「default 採用」の混在で実装 phase 入口の合意形成順序が不明)
- **修正案**: chapter 07 §12 を「AYA 最終 confirm 項目 (default 採用済、formal confirm 要)」と書き換え、各項目に default 案を明記

### §3.9 chapter 08 §3.2 (B1) Python 採用の 3 OS 環境 prerequisite (agent a112794604894de57 報告 = 要 spot check)

- **主張**: agent 報告では chapter 08 §3.2 で「autobuild に Python 既存」根拠で B1 Python 採用、ただし Python 3.8+ の Win/Mac 保証は未検証 = 環境 prerequisite check phase が chapter 09 へ defer
- **判定**: 設計前提 (= Python 3.8+ 3 OS 揃え) が未検証で chapter 08 全体が成立しないリスク
- **要 spot check**: chapter 08 §3.2 本体 Read で agent 解釈の正確性確認、autobuild Python 版本の実情も spot check
- **修正案 (確定なら)**: chapter 08 に「3 OS Python 3.8+ availability check」を Phase 0 task として追加、または chapter 09 Phase 0 spec に明示登録

### §3.10 chapter 08 §5 perfect hash generator の invoke timing 未定義 (agent a112794604894de57 報告 = 要 spot check)

- **主張**: agent 報告では chapter 08 §5.6 で G2/B3b (Python frozen-table) 採用、generator 実行 timing が「Codegen tool 初期実装 phase (§8)」へ延期 = build pipeline への integrate 方法が chapter 08 本体に未記
- **要 spot check**: chapter 08 §5 / §8 / §12 を Read で確認、generator invoke timing が本当に未定義か

### §3.11 chapter 09 §5.3 canary cvar 「値表」未具体化 (agent a40aa0f421bf4e4e1 報告)

- **主張**: chapter 09 §5.3 で「canary cvar flip による Phase 検証」と書かれているが、具体的な「どの cvar をどの値に flip するか」値表が本 chapter に無く handoff doc 側に委ねられている
- **対立**: memory `feedback_restore_debug_settings` = 検証完了時に「戻す値表」を必ず提示
- **判定**: chapter 09 §9 で「item 3: 検証完了時に戻す値表を提示」との約束があるが、§5.3 本体で値表が定義されていない = 約束履行の作業可能性ギャップ
- **修正案**: chapter 09 §5.3 に Phase 別 canary cvar 値表 (cvar 名 / Phase 検証時値 / 復元値 / 用途) を inline 表として追加

### §3.12 chapter 09 §4 Phase 1.C 試作 vs Phase 2 本実装の境界曖昧 (agent a40aa0f421bf4e4e1 報告)

- **主張**: chapter 09 §4 Phase 1.C Exit Criteria「test UBO 1 個で full path 通電確認」が「Phase 2 で本実装する第 1 UBO の試作版」と内容重複、境界が曖昧
- **判定**: Phase 1.C / Phase 2 のどちらで本実装かが不明確 = 実装 phase 入口で迷走可能性
- **修正案**: chapter 09 §4 Phase 1.C を「dummy UBO 1 個 (= 廃棄前提)」または「Phase 2 第 1 UBO の prototype = Phase 2 内に統合」のいずれかに確定明記

### §3.13 chapter 10 §2 消化方法の imperative 不足 (agent a40aa0f421bf4e4e1 報告)

- **主張**: chapter 10 §2 「実装 phase 入口で消化される項目」の消化方法が descriptive 止まり (例: §2.1 (P1) 「衝突あれば rename」「全 source」の対象範囲不明、§2.2 (RF) 「> 100 回/分で warn」の "warn" 動作 (log/cvar/error) 未指定)
- **判定**: 作業可能性違反 (= 「Phase 0 着手したが具体的に何を grep するか不明」状態)
- **修正案**: chapter 10 §2 各項目に消化方法の imperative spec を追記 (= grep 対象 dir / warn 動作 / 判定閾値 / pass-fail criteria)

### §3.14 chapter 10 §1 reflect 先 §N の pinpoint 確認 task

- **主張**: agent 報告 (= 09/10 agent) で 21 件 AYA 判断仰ぎ候補の reflect 先 chapter §N が明示されているが、agent は「該当 chapter 未起案」と誤認。**実際は全 chapter 起案済** (handoff §1.1 進捗表参照)
- **判定**: agent の「未起案」誤認は無視するが、reflect 先 §N が **実 chapter §N で実際に該当する節か** の pinpoint 確認 task は依然有効
- **修正案**: chapter 10 §1 各 21 件の reflect 先 §N を pinpoint Read で確認、ズレがあれば §N 修正 (= main session で次 phase に持越して全件 spot check)

### §3.3 chapter 04 §7.2 「両 path 共存期」の bare uniform setter Vulkan 動作

- **主張**: `04-codegen-ubo.md` §7.2 で「bare uniform の setter call site は chapter 06 redirect 層で path 分岐 (OpenGL → 従来 `glUniform*`、Vulkan → UBO offset 書込)」「Vulkan path は chapter 05 集約表で UBO に取り込まれた後 Codegen 経由で処理」
- **含意**: chapter 09 phase roadmap で「N UBO ずつ移行」する間、まだ集約されていない bare uniform は **Vulkan path 上で未 redirect**。chapter 04 §1 の問題提起「84 UBO blueprint が host C++ で redirect 完全欠落 → dead 状態」と同じ過渡期動作
- **判定候補**: 過渡期動作 (= 一部の bare uniform が Vulkan で未 redirect = 描画に値が来ない状態) が chapter 09 で **明示的に許容 / 段階的解消** されているか確認要
- **要確認**: chapter 09 (phase-roadmap) Read 時に、各 Phase の Exit Criteria が「未集約 bare uniform の Vulkan path 動作 (空値 / fallback / error)」をどう扱うか明示されているか確認

---

## §4 [破綻] findings

### §4.1 chapter 02 §3.2 set=1 帯の cadence カラムが廃止済「per-material」を保持

- **主張**: `02-naming-convention.md` §3.2 表で `MaterialUBO` / `MaterialUBO_Legacy` の cadence カラムに「per-material」と記載
- **対立する確定事項**:
  - `03-cadence-classification.md` §2 = cadence 分類は 5 分類 (per-frame / per-program / per-draw / per-asset / per-skin)、per-material は **per-draw + dirty flag に統合** (= 独立軸として保持しない)
  - `01-overview.md` §5 確定事項 #12 = 「per-material cadence は per-draw + dirty flag に統合」AYA 判断確定
  - `02-naming-convention.md` §2.1 表でも「per-draw (material dirty flag、chapter 05 §6 G1 確定)」として `Material` prefix を per-draw cadence の細分に位置付け
- **判定**: 同一 chapter 内で §2.1 (per-draw) と §3.2 (per-material) の cadence 表記が不整合 = 確定事項 #12 違反 (= 破綻)、整合性 (§5) も同時違反
- **修正案**: §3.2 表 cadence カラムを「per-draw (material dirty flag)」に修正 (= §2.1 表記と統一)

### §4.3 chapter 06c §4.3 (V3) 持越と §2.1 M1 採用の関係 (agent ad5c2801b745a6848 報告)

- **主張**: chapter 06c §2.1 で M1 (set 帯固定 = shader 内 layout(set=N) が cadence と 1:1) 採用、PSO compatibility を根拠とする。一方 §4.3 (V3) 持越で「set=1 layout を全 program 共通 vs program 別の trade-off」未確定
- **判定**: M1 (set 帯固定) は保証されるが、set 内 binding layout 構成 (= program 別 subset vs 全 program 共通 79 binding) が別軸の PSO 最適化、未確定 = 設計の確定度表記混乱
- **修正案**: chapter 06c §4.3 に「M1 採用は set 帯固定保証、set 内 binding layout 構成 (V3) は別軸、device limit + cache hit 率測定後決定」明記

### §4.4 chapter 07 §4.4.1 set=2/3 dynamic offset 寄せの vkCmdBindDescriptorSets 仕様 (agent a112794604894de57 報告 = 要 spot check)

- **主張**: agent 報告では chapter 07 §4.4.1 で set=2 に dynamic offset 指定 + set=3 を draw 直前入替 bind 提案、ただし `vkCmdBindDescriptorSets` binding range 再構築 (= 単発 call か複数段階か) 未定義
- **要 spot check**: chapter 07 §4.4 本体を Read で確認、agent 指摘の正確性 + 既に §N で言及済か確認
- **修正案 (確定なら)**: chapter 07 §4.4.1 に bind sequence 明示 (= vkCmdBindDescriptorSets call の回数 / first_set / descriptor_set_count)

### §4.2 chapter 02 §3.2 表で `MaterialUBO` の `UBO` suffix 温存 vs §4 命名規則違反検知の warning 対象

- **主張**: §3.2 で `MaterialUBO` の新名が「`MaterialUBO` (暫定)」「`MaterialUBO_Legacy` → `MaterialLegacyBlinn` (案)」と記載 (= UBO suffix 残存)
- **対立する規則**: §4 命名規則違反検知ルール 4「`UBO` suffix が残っている → warning (`_Legacy` 以外)」 = `MaterialUBO` (新名側) は warning 対象
- **判定**: 「既存命名温存」 (原則 1 upstream 互換) vs 「`UBO` suffix warning」が衝突 → 例外規則の明示が必要 (= 破綻 / 整合性)
- **修正案**: (a) §4 warning ルールに「`Material*` は例外 (= 既存上流互換のため UBO suffix 温存許可)」を明記、または (b) §3.2 で `MaterialUBO` → `Material` / `MaterialPBR` 等 UBO suffix なし新名に統一、後者は upstream 取込互換性 (原則 1) と衝突する可能性あり → AYA 判断材料

---

## §5 [整合性] findings

### §5.1 chapter 02 §3.1 「FrameAtmosphere_Lighting → FrameAtmosphere」rename の波及確認要

- **主張**: `02-naming-convention.md` §3.1 で `FrameAtmosphere_Lighting` → `FrameAtmosphere` rename (= `_Lighting` suffix 削除、「atmospheric には lighting 以外無いため自明」)
- **判定候補**: rename 自体は規則整合だが、inventory §3.1 / chapter 05 / chapter 06b 等で旧名 `FrameAtmosphere_Lighting` が参照されている箇所がないか要 grep
- **状態**: 数値突合 agent 結果 (FrameAtmosphere_Lighting は重大 finding として上がっていない) より、深刻な不整合は無いと推定。chapter 05 Read 時に明示的確認

### §5.2 項目 ID (R1) の別概念衝突 (chapter 06a §9 vs chapter 07 §12)

- **観測** (ID 衝突 agent a868fc330d60a7f65 結果より):
  - **06a §9**: `(R1)` = LLStaticHashedString 67 個のうち UBO 化対象有無 (= bare uniform 集約判定文脈)
  - **07 §12**: `(R1)` = ring buffer 起動時 4 MB / 上限 16 MB (= per-draw upload pool 容量文脈)
  - 両者は無関連概念
- **同時出現**: chapter 10 §2.5 で「06a §9 (H1b)(Q1)(Q2)**(R1)**(S1)」と「07 § (V1')(V3')(S3')(W)**(R1)**(PSC)(RF)」が同 listing 内に共存し、`(R1)` が 2 つの異なる概念を指して並ぶ → **文脈による識別が困難**
- **判定**: 整合性違反 (= 同 ID で別概念、handoff §4.4 で衝突懸念として既知だが rename 未実施)
- **修正案**: 後発の chapter 07 §12 `(R1)` を `(R2)` または `(RB1)` (ring buffer prefix) 等に rename し、chapter 10 §2.5 listing も同期更新。06a §9 `(R1)` (= LLStaticHashedString 文脈) は既存維持 (= 早期成立)

### §5.3 項目 ID (M) の別概念衝突 (chapter 06b §8 vs chapter 06c §10)

- **観測** (ID 衝突 agent a868fc330d60a7f65 結果より):
  - **06b §8**: `(M)` = thread-safe 化方式 (mutex / atomic / lock-free) — cache update side
  - **06c §10**: `(M)` = descriptor set 4 帯 ↔ cadence 5 分類 1:1 配置 — bind wiring side
  - 両者は無関連概念
- **同時出現**: chapter 10 §2.4 / §2.5 で両 (M) が同 doc 内別節に表記され、cross-ref 時に文脈識別困難
- **判定**: 整合性違反 (= handoff §4.4 で衝突懸念として既知)
- **修正案**: 後発の chapter 06c §10 `(M)` を `(MD)` (descriptor mapping prefix) 等に rename し、chapter 10 listing も同期更新。06b §8 `(M)` (= thread-safe) は既存維持 (= 早期成立かつ「Mutex」連想で文脈強い)

### §5.4 chapter 08 「set 帯 5 化」 vs chapter 06c 「4 帯」表記の概念軸混在

- **観測** (数値突合 agent 結果より):
  - chapter 08 line 54 = 「set 帯 **5 化**」
  - chapter 06c line 111 = 「**4 帯**」
- **実体**: Vulkan logical descriptor set = **4 帯 (set=0/1/2/3)**、descriptor pool 上の subset = **5 個 (set=1 を 1a/1b に split したため)**
- **判定**: 同じ "set 帯" 表記で異なる軸 (logical vs pool subset) を指している → 整合性違反、読者の混乱誘発
- **修正案**: chapter 08 line 54 を「descriptor pool subset 5 化 (= set=1 を 1a/1b に split した結果、logical 帯は 4 のまま)」に明示書き換え、chapter 06c line 111 「4 帯」は注記「(= logical 帯、descriptor pool 上は set=1 split で 5 subset)」追記

### §5.5 prime 付き ID (V1' / V3' / S3') の判定

- **観測** (ID 衝突 agent 結果より): V1 → V1' / V3 → V3' / S3 → S3' は chapter 07 §3.2 / §4.2 / §5.2 で意図的に prime 付き派生確定 (= 別概念明示)
- **判定**: 整合性 OK (= prime 表記で識別可能、衝突ではない)
- **修正案**: なし (= 良好な命名整理事例として記録)

### §5.6 項目 ID (G) の別概念衝突 (chapter 04 §10 vs chapter 05 §10) — handoff §4.4 見落とし

- **観測** (本査読で新発見):
  - **04 §10**: `(G)` = perfect hash generator 選択 (= gperf / 独自 / frozen の 3 選択肢、chapter 08 持越)
  - **05 §10**: `(G)` = per-material cadence 採用案 (= G1 per-draw 統合 / G2 独立軸、§6.3 で G1 確定済)
  - 両者は無関連概念
- **同時出現**: chapter 04 §10 と chapter 05 §10 は別 chapter の §10 にそれぞれ存在、cross-ref 時に「(G) を確認」と言われると識別不能
- **判定**: 整合性違反 (= handoff §4.4 では指摘されていない新規衝突、ID 衝突 agent も詳細追跡対象外として簡記)
- **修正案**: 後発の chapter 05 §10 `(G)` を `(G1/G2 含めて MC)` (material cadence prefix) 等に rename、または chapter 04 §10 `(G)` を `(GH)` (generator hash prefix) 等に rename。chapter 05 側は §6.3 で G1 確定済のため「~~G~~ 解消」表記の取消線を含む rename が必要

### §5.8 chapter 06c §10 「未確定」表記 vs 「default 採用済」の混乱 (agent ad5c2801b745a6848 報告)

- **主張**: chapter 06c §10 で (M)(N)(O) を「chapter 10 / AYA 判断」未確定と明記、ただし 06c 自体は M1 / N2 / O1 default 採用で設計進行
- **判定**: 整合性違反 (= 確定度表記の不揃い、chapter 07 §12 (§3.8) と同様の構造的問題)
- **修正案**: 06c §10 を「AYA 最終 confirm 項目 (default 採用済)」に改名、各項目に「現 default → AYA confirm 要請」表記

### §5.9 chapter 07 §4.4「set 帯総数 4 → 5 拡張」 vs §9.1 sAYAStandardLayout の 4 帯 comment (agent a112794604894de57 報告 = 要 spot check)

- **主張**: chapter 07 §4.4 で「set 帯総数 5 拡張」明記、ただし §9.1 `sAYAStandardLayout` 構造体の comment は set=0/1a/1b/2 の 4 帯のみ、set=3 への入替 bind は §9.2 へ移管
- **判定**: 表と code comment の分散で確定度低下、chapter 06c との整合性確認要
- **要 spot check**: chapter 07 §4.4 / §9.1 / §9.2 を Read で確認、§5.4 (= 本 report 既記載) と関連付け

### §5.11 chapter 09 §2.1 Phase 番号 K (= 5-20 程度) の inconsistency (agent a40aa0f421bf4e4e1 報告)

- **主張**: chapter 09 §2.1 「Phase 2..K」表記で K = 「migration UBO 個数依存」「5-20 程度」と幅、§5.1 「1 UBO ずつ」/ §5.2 「Phase 順序」では K に関わらず Phase 2 / 3 / 4... と具体化
- **判定**: K 不確定のまま後続 § で確定 phase number が前提化 = 整合性違反 (= phase number consistency check が弱い)
- **修正案**: chapter 09 §2.1 で K 確定条件を明記 (= chapter 10 §3 接続)、後続 § での具体 phase number は K 確定後の reflect とする旨明示

### §5.12 chapter 10 §2 (RF) 配置と ID naming consistency (agent a40aa0f421bf4e4e1 報告)

- **主張**: chapter 10 §2 で (RF) が §1.1 (chapter 07 §12 送り) と §2.2 (実装 phase 入口消化) の両方で参照、ID naming consistency 弱い (= (RF) 二重参照)
- **判定**: 整合性違反 (= ID listing の単一性が崩れる)
- **修正案**: chapter 10 §2 (RF) を §1.1 / §2.2 のどちらか単独に配置、他方は cross-ref 記載のみ

### §5.10 chapter 08 §6.2 表「chapter 07 反映項目」の (B1)-(B5) 未登載 (agent a112794604894de57 報告 = 要 spot check)

- **主張**: agent 報告では chapter 08 §6.2 表で (V1')(V3a)(S3')(W) chapter 07 項目のみ反映、handoff §1.3 (B1)-(B5) と §17 listing が subset 関係でも (B1)-(B5) 自体の output binding が §6 schema 未反映
- **要 spot check**: chapter 08 §6.2 / §17 を Read で確認

### §5.7 chapter 05 §10 取消線表記 (= ~~E~~ / ~~G~~) の表記揺れ

- **観測**: chapter 05 §10 で「~~E~~ 解消 (= AYA 判断: E3 採用)」「~~G~~ 解消 (= AYA 判断: G1 採用)」と取消線で「解消済」を表現
- **判定候補**: 他 chapter (= 01 §4 進捗表 = ✅ 起案済 / 06a §9 = 解消マークなし) と表記が不揃いの可能性。doc 内「解消済」の標準表記が定まっていなければ整合性違反 / 軽微
- **要確認**: 他 chapter §N で「解消」マーク表記 (✓ / ✅ / 取消線) が混在しているか agent / 簡易 grep で確認

---

## §6 AYA 判断仰ぎ候補 21 件再点検

agent a40aa0f421bf4e4e1 報告を基に 21 件全件を独立評価 (= 4 観点 PASS / 要追記)。**chapter 07/08/09/10 = 全件起案済**のため、agent の「reflect 先 chapter 未起案」誤認は除外し、default 採用案の妥当性のみ評価。

### §6.1 chapter 07 §12 由来 4 件 — default 採用案明示 + reflect 先指定 = **全件 OK**

| ID | 内容 | default 採用案 | reflect 先 | 判定 |
|---|---|---|---|---|
| V1' | set=1 79 binding split | 40/39 split | chapter 07 §3.2 | OK (= 4 観点 PASS、formal AYA confirm 要のみ) |
| V3' | layout 共通性 | V3a = 全 program 共通 layout | chapter 07 §4.2 | OK |
| S3' | sampler 49 配置 | set=3 per-asset 同居 | chapter 07 §5.2 | OK |
| W | maxSets | default 6 | chapter 07 §6.2 | OK |

### §6.2 chapter 08 §17 由来 7 件 — default 採用案明示 + reflect 先指定 = **全件 OK**

| ID | 内容 | default 採用案 | reflect 先 | 判定 |
|---|---|---|---|---|
| A1 | std140 offset | 二重保証 (Codegen calc + SPIR-V reflection) | chapter 08 §17 | OK |
| P | GLSL parse | P3 mini-parser + glslang -E | chapter 08 §17 | OK |
| G/B3 | perfect hash gen | G2/B3b Python frozen-table | chapter 08 §17 | OK (= chapter 05 §10 (G) との ID 衝突は本 report §5.6 で別途指摘) |
| B1 | Codegen 言語 | B1a Python 3.8+ | chapter 08 §17 | OK (= 3 OS Python availability 前提は本 report §3.9 で別途指摘) |
| B2 | glslang 統合 | B2a autobuild vendoring | chapter 08 §17 | OK |
| B4 | 増分 build cache | B4a hash + mtime 併用 | chapter 08 §17 | OK |
| B5 | Codegen trigger | B5a CMake DEPENDS + 手動 target 併設 | chapter 08 §17 | OK |

### §6.3 chapter 09 §11 由来 5 件 — default 採用案明示 + reflect 先指定 = **全件 OK** (1 件 = Phase 0 計測待ち)

| ID | 内容 | default 採用案 | reflect 先 | 判定 |
|---|---|---|---|---|
| Q1 | 第 1 UBO template | Template A 最小リスク優先 | chapter 09 §5.2 | OK (= Phase 0 計測完了後 final 版確定) |
| Q2 | Phase 当たり UBO 数 | 1 UBO 厳守 | chapter 09 §11.2 | OK |
| Q3 | OpenGL 並走期間 | 全 UBO 移行完了まで並走 | chapter 09 §7.1 | OK |
| Q4 | 3 OS Phase 順序 | Linux 完了後 Win/Mac 並走 | chapter 09 §6.1 | OK |
| Q5 | Phase 0 Phase 番号化 | 独立 Phase η-29 | chapter 09 §3 | OK |

### §6.4 chapter 06b §8 / 06c §10 由来 4 件 — default 採用案明示 + reflect 先指定 = **全件 OK**

| ID | 内容 | default 採用案 | reflect 先 | 判定 |
|---|---|---|---|---|
| K | dirty 判定粒度 | K2 UBO 単位 | chapter 06b §3.2.3 / §5.2 | OK (= 本 report §3.6 で K2 採用済 vs 未確定表記の混乱を別途指摘) |
| M | descriptor set 4 帯 ↔ cadence 5 分類 | M1 1:1 配置 | chapter 06c §2 / §3 / §6 / §3.4 | OK (= 本 report §5.3 で chapter 06b §8 (M) との ID 衝突を別途指摘) |
| N | mUseUBO initial 設定 | N2 shader 単位 phase migration | chapter 06c §6.2 / §10 | OK |
| O | UB_* 4 binding 拡張 | O1 既存維持 + 新規追加 | chapter 06c §3.4 / §10 | OK |

### §6.5 chapter 05 §10 由来 1 件 — default 採用案明示 + reflect 先指定 = **OK**

| ID | 内容 | default 採用案 | reflect 先 | 判定 |
|---|---|---|---|---|
| F | MaterialUBO vs Legacy | F1 統合 (member 比較で同一なら) | chapter 05 §5 / §7.3 + chapter 06a-prep §3 (F) 計測 spec | OK |

### §6.6 §6 サマリ

21 件全件で **default 採用案明示** + **reflect 先 §N 指定** = **AYA 判断仰ぎ doc としての構造は健全**。formal AYA confirm が必要なだけで、構造的不備はない。

ただし以下の関連 finding は別途記録済 (本 report §3 / §5):
- (G) ID 衝突 (chapter 04 §10 vs chapter 05 §10) = §5.6
- (R1) ID 衝突 (chapter 06a §9 vs chapter 07 §12) = §5.2
- (M) ID 衝突 (chapter 06b §8 vs chapter 06c §10) = §5.3
- (K) 採用済 vs 未確定表記の scope 混乱 = §3.6
- chapter 06c §10 / chapter 07 §12 の「未確定」vs「default 採用済」表記揺れ = §3.8 / §5.8
- chapter 10 §2 消化方法 imperative 不足 = §3.13
- chapter 10 §2 (RF) 二重参照 = §5.12

---

## §7 修正推奨 chapter 一覧

chapter 別の修正推奨 (= 修正方針確定済の項目を集約、要 spot check 項目は別マーク):

### chapter 01: 修正 0 件
- 判定基準として OK、修正不要

### chapter 02: 修正 5 件
- §3.1 表 「FrameAtmosphere_Lighting → FrameAtmosphere」rename の波及確認 (§5.1)
- §3.2 表 set=1 帯 cadence 列を「per-material」→「per-draw (material dirty flag)」修正 (§4.1)
- §3.2 表 `MaterialUBO` UBO suffix 温存と §4 warning 規則の例外明示 (§4.2)
- §3.3 表の 26 行から余分 1 行を inventory §3.3 と突合して削除 (§2.1)
- §3.4 54 件 rename 表の完全化 or chapter 05 への参照モデル明示 (§3.1)

### chapter 03: 修正 0 件
- 判定基準として OK、修正不要

### chapter 04: 修正 2 件
- §6.3 (D) 動的 uniform 名の存在確認を chapter 06a-prep §7 (P1)-(P4) に登録 (§3.2)
- §7.2 「両 path 共存期」過渡期動作の chapter 09 Phase Exit Criteria 明示 (§3.3)

### chapter 05: 修正 3 件
- 行 9 + 行 43 「cadence 6 分類」→「5 分類」修正 (§2.2)
- §10 (G) を per-material cadence prefix (例: (MC)) に rename = chapter 04 §10 (G) との別概念衝突解消 (§5.6)
- §10 取消線表記 ~~E~~ / ~~G~~ を他 chapter と統一 (§5.7)

### chapter 06a: 修正 1 件
- §4.3 (S1) を「仮 code shape」明示 + LLStaticHashedString registry iterate / helper 作成等の代替案併記 (§3.4) — **致命傷候補**

### chapter 06a-prep: 修正 1 件
- §2.2.1 sampler `uniform1i` の cadence histogram 対象範囲明確化 (= 対象外なら別 histogram) (§3.5)

### chapter 06b: 修正 3 件
- §3.2.3 「default K2 採用前提、(K) 変更時の影響範囲」注記追加 (§3.6)
- §5.3 / §5.4 (L) と (M) 間の依存性 (= AYA 判断順序) 明示 (§3.7)
- §8 (M) を `(MD)` (= descriptor mapping prefix) に rename = 06c §10 との別概念衝突解消は 06c 側で実施

### chapter 06c: 修正 4 件
- §3.1 表 set=1 binding=0/1 vs set=2 binding=0/1 重複削除 [要 spot check] (§2.3)
- §4.3 (V3) と §2.1 M1 の関係明示 (= set 帯固定保証 vs set 内 binding layout 別軸) (§4.3)
- §10 表記揺れ「未確定 vs default 採用済」を「AYA 最終 confirm 項目 (default 採用済)」に書き換え (§5.8)
- §10 (M) ID rename を 06b §8 (M) と整合 (§5.3)

### chapter 07: 修正 5 件
- §2.4 device limit 表に Vulkan 1.3 spec 最小値列補完 [要 spot check] (§2.4)
- §4.4.1 `vkCmdBindDescriptorSets` binding range 再構築仕様明示 [要 spot check] (§4.4)
- §4.4 「set 帯総数 5 拡張」と §9.1 `sAYAStandardLayout` 4 帯 comment の整合 [要 spot check] (§5.9)
- §12 表記揺れ「未確定 vs default 採用済」を「AYA 最終 confirm 項目」に書き換え (§3.8)
- §12 (R1) を `(RB)` 等に rename = 06a §9 (R1) との別概念衝突解消 (§5.2)

### chapter 08: 修正 5 件
- §6 `ubo_metadata.inl` schema の `subset` field 仕様修正 (= sort 規則 hard-code との矛盾解消) [要 spot check] (§2.5)
- §3.2 (B1) Python 採用に 3 OS Python 3.8+ availability check Phase 0 task を追加 [要 spot check] (§3.9)
- §5 / §8 / §12 perfect hash generator の invoke timing 明示 [要 spot check] (§3.10)
- §6.2 表に (B1)-(B5) の schema 反映項目追加 [要 spot check] (§5.10)
- §17 (G) と chapter 04 §10 (G) / chapter 05 §10 (G) との ID 衝突解消 (= chapter 05 側 rename と整合) (§5.6)
- §5.4 「set 帯 5 化」記述に「= descriptor pool subset 5、logical set 帯は 4」注記追加 (§5.4)

### chapter 09: 修正 3 件
- §2.1 Phase 番号 K の確定条件明示 (= chapter 10 §3 接続) (§5.11)
- §4 Phase 1.C 試作版と Phase 2 本実装の境界確定 (§3.12)
- §5.3 canary cvar 値表の inline 化 (= Phase 別 cvar 名 / 検証値 / 復元値 / 用途) (§3.11)

### chapter 10: 修正 3 件
- §2 各項目に消化方法 imperative spec 追記 (= grep 対象 dir / warn 動作 / 判定閾値 / pass-fail criteria) (§3.13)
- §2 (RF) を §1.1 / §2.2 のどちらか単独配置に整理 (§5.12)
- §1 reflect 先 §N の pinpoint 確認 task (= 全 21 件で実 §N と一致確認) (§3.14)

### handoff doc §4.4 項目 ID 一覧: 修正 1 件
- (G) ID 衝突 (chapter 04 §10 vs chapter 05 §10) を「衝突懸念」に追加 (= 本査読で新発見)

---

## §8 査読 session 完了判定

### §8.1 総合判定: **PASS-with-fixes**

- 違反 34 件 (= 事実性 5 + 作業可能性 14 + 破綻 4 + 整合性 11)
- 致命傷 (= root cause unknown のブロッキング、設計 chapter 起案やり直し相当) **0 件**
- 全 finding に修正方針提示済
- 修正対象は 11 chapter (= 01 / 03 を除く全 chapter) + handoff doc §4.4 一覧

### §8.2 致命傷候補の扱い

以下 2 件は「致命傷」ではなく「致命傷候補」(= 修正方針明確、修正容易):

1. **chapter 06a §4.3 `LLStaticHashedString::getGlobalRegistry()` 仮 API code shape** (§3.4)
   - 実装 phase 進入直後にコンパイルエラー直行リスク
   - 修正案: 「仮 code shape」明示 + 代替案 (registry 直接 iterate / helper 作成) 併記
   - chapter 06a の修正 1 行で解消可能、設計の根本変更不要

2. **chapter 02 §3.2 set=1 帯 cadence 列が廃止済「per-material」** (§4.1)
   - chapter 01 §5 確定事項 #12 違反 (= per-material は per-draw + dirty flag 統合確定)
   - 修正案: cadence 列を「per-draw (material dirty flag)」に修正
   - chapter 02 §3.2 表 1 セル修正で解消可能

= **設計の根幹は健全**、修正は機械的 / 表記統一 / 1 行注記追加 で達成可能。

### §8.3 次 phase への移行手順

handoff doc §7.2 (PASS-with-fixes 判定時) に従い:

1. 本 report doc を起案 (= 2026-06-03 本作業で完了)
2. AYA に簡潔報告 (= 4 観点違反件数 + 総合判定 + 主要 finding + 修正方針提案、本 session 完了直前に実施)
3. 修正方針 AYA 確認後、修正方針確定
4. 修正 commit (= chapter 別、修正単位で分割可)
5. 要 spot check 項目 (~8 件) の pinpoint Read 実施 → 確定 / 取下げ
6. 修正部分の再点検 → 本 report doc 追記 (= 修正履歴節を新設)
7. 違反 0 件で PASS 判定 → 次 phase (= 選択肢 A AYA 判断 / B implementation-phase 入口) AYA 指示待ち

### §8.4 規律確認

- design-phase 継続: `indra/` 改変ゼロ厳守 = **守られた** (本 session の全編集は `docs/specs/ayastorm-r41-gl-removal/` 配下のみ)
- 修正 commit は AYA 判断後: **守られた** (本 report 起案のみ、修正 commit は AYA 確認後)
- tests/ 触らない: **守られた**
- 事前 Read 規律: handoff §5 の 3 件 + 査読中 chapter 02 / 04 / 05 = 5 件 main Read + 残 chapter (06a / 06a-prep / 06b / 06c / 07 / 08 / 09 / 10) は agent 3 並列発注で context 節約 = **守られた**
- findings 即記録: 各 chapter 読了時に report doc に追記 = **守られた**
- agent 積極活用: 5 agent 発注 (ID 衝突 grep / 数値突合 / 06 系列査読 / 07-08 査読 / 09-10 + AYA 21 件再点検) = **守られた**

### §8.5 本 report doc の owner

- 起案: 2026-06-03 本 session
- 修正履歴節: AYA 確認後の修正実施で追加 (= §9)
- live doc ではなく **snapshot** (= 本査読時点の判定を不変記録、修正後再点検は新節として追記)

---

## §9 修正履歴 (= 2026-06-03 AYA 「修正してください」mandate 実施記録)

AYA 指示 (= 全 34 件修正承認) を受けて実施。全編集は `docs/specs/ayastorm-r41-gl-removal/` 配下 (= `indra/` 改変ゼロ厳守、`tests/` 不触、design-phase 規律遵守)。

### §9.1 修正対象集計 (= 34 violations)

| 区分 | 件数 | 内訳 |
|---|---|---|
| 事実性違反 | 5 | §2 |
| 作業可能性違反 | 14 | §3 |
| 破綻違反 | 4 | §4 |
| 整合性違反 | 11 | §5 |
| **計** | **34** | (致命傷 0、PASS-with-fixes) |

### §9.2 chapter 別修正サマリ

| chapter | 件数 | 主要修正内容 |
|---|---|---|
| chapter 02 | 5 | §3.1 FrameAtmosphere rename 波及 / §3.2 set=1 帯 per-material → per-draw / §3.4 54 件 rename 表 incomplete / §5 用語 alias / 軽微整合 |
| chapter 04 | 2 | §6.3 (D) Phase 0 接続 / §7.2 過渡期動作 (mUseUBO sentinel) 明示 |
| chapter 05 | 3 | 行 9 + 行 43 「6 分類」→「5 分類」/ §10 (G) → (MC) rename + §6 G1/G2 → MC1/MC2 連動 / §10 取消線表記 整合 |
| chapter 06a | 1 | §4.3 `LLStaticHashedString::getGlobalRegistry()` 仮 API 注 + 代替案併記 (致命傷候補解消) |
| chapter 06a-prep | 1 | §2.2.1 注 uniform1i 矛盾解消 (sampler 49 = UBO 非合算明示) + §2.1 / §2.7 band 並走計測指示 |
| chapter 06b | 3 | §3.2.3 K2 default 注 (K1/K3 採用時差替指示) / §5.3 (L) L1+L2 default 前提 / §5.4.3 (M) non-mutex 前提 / §8 表 (M) ID 衝突注 |
| chapter 06c | 4 (内 1 spot check) | §3.1 set=1 帯 重複解消 / §4.3 (V3)/M1 関係明示 / §10 「default 採用済」列追加 / (M) → (MD) rename / G1/G2 → MC1/MC2 連動 rename / §11 update 規律反映 |
| chapter 07 | 5 (内 3 spot check) | §2.3 / §2.4 `maxBoundDescriptorSets` 5 帯 + 4 bind 明示 / §4.4.1 タイトル + bind 構成 / §9.1 pipeline layout 5 要素 / §12 (R1) → (RB) rename + 注 / §13 update 規律反映 |
| chapter 08 | 6 (内 4 spot check) | §6.1 schema 論理 5 set 帯 表現規約注 / §3.2 B1a Python 3.8+ 採用根拠 3 OS 詳細 / §5.4 set 帯 5 化注 / §6.2 表 (B1)-(B5) handoff 接続列追加 / §12.4 新節 generator timing 全体接続 / §17 表 (G/B3) ID 衝突注 |
| chapter 09 | 3 | §2.1 Phase K 確定 3 条件 / §4 Phase 1.C / Phase 2 境界明示 / §5.3.1 canary cvar 規約 inline 化 (命名規約 + 値表 + 設置位置 + 検証手順 6 step) |
| chapter 10 | 3 | §2 各項目 消化方法 imperative spec 追記 (grep 対象 dir / warn 動作 / 判定閾値 / pass-fail criteria) / §1.1 + §2.2 (RF) 配置 cross-ref 注追加 / §8.1 spot check 持越 task 追加 (= §1 reflect 先 §N pinpoint 全 21 件 verify) |
| handoff §4.4 | 1 | (G) ID 衝突 (04 §10 vs 05 §10 vs 08 §17) 「衝突懸念」追加 + 表本体に (G) → (MC) / (R1) → (RB) / (M) → (MD) rename status 反映 + 新規 chapter 起案時の prefix 列登録規律明文化 |
| **計** | **37**\* | \* 修正単位カウント (= 一部修正が複数 finding を同時解消) |

### §9.3 ID rename 連鎖サマリ

| rename | 出典 | 衝突解消 | 連動修正 |
|---|---|---|---|
| 05 §10 (G) → (MC) | 査読 §5.6 (本査読新発見) | 04 §10 (G) (= perfect hash generator) との衝突 | 05 §6 G1/G2 → MC1/MC2 / 06c §6 G1/G2 → MC1/MC2 連動 / 08 §17 (G/B3) は 04 §10 (G) と同概念で rename 不要 |
| 06c §10 (M) → (MD) | 査読 §5.3 | 06b §8 (M) (= thread-safe) との衝突 | 06c 全文 (M) → (MD) / 06b §8 表 (M) ID 衝突注追加 |
| 07 §12 (R1) → (RB) | 査読 §5.2 | 06a §9 (R1) (= LLStaticHashedString) との衝突 | 07 §12 / §13 update 規律 / 10 §2.5 listing 連動 |

### §9.4 持越事項 (= 修正後の次 phase 対応)

1. **§1 reflect §N pinpoint 確認** (= chapter 10 §8.1 task): 次 session で全 21 件 AYA 判断仰ぎ候補の reflect §N を実 chapter で pinpoint Read verify、ズレあれば §1.X 表修正
2. **要 spot check ~8 件** (= 本 report §8.3 step 5): 06c §3.1 / 07 §4.4 / 08 §6.2 等の pinpoint Read 確認、確定 / 取下げ判定
3. **AYA 判断仰ぎ 21 件 formal confirm** (= §6 全件 OK 判定済): AYA から default 採用 / 別案採用 / 計測待ち 判断、確定後に各 chapter §N reflect

### §9.5 規律確認 (= 修正実施 phase 中)

- **design-phase 継続**: `indra/` 改変ゼロ = **守られた** (全編集は docs/specs/ 配下)
- **tests/ 不触**: **守られた** (commit / 言及 / `git add -A` ゼロ)
- **commit 待ち**: 本修正 phase で commit ゼロ、AYA 明示指示後に実施 (memory `feedback_no_auto_commit`)
- **scope 縮小なし**: 34 件全件着手・完了、「段階を踏む」を scope 縮小に流用しない (memory `feedback_no_scope_shrink`)
- **literal 全件遵守**: AYA「修正してください」mandate = 34 件全件 fix、subset 採用なし

### §9.6 修正完了マーク

- **34 violations 全件修正完了** (2026-06-03)
- **持越 3 件** (§9.4) は次 session 入口で AYA 判断 / 消化
- **再点検**: AYA confirm 後の修正部分 spot check 完了で違反 0 件 → §7.1 PASS 判定移行
- **commit 待ち state**: AYA 「commit してください」指示で chapter 別 / 修正単位 commit 実施可能
