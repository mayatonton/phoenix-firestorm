# r41 Phase 2 着手前準備 — UBO design 全 95 file 起案完了 + 残作業 handoff

> **着手契機**: 2026-06-06 design session (= Phase 2 着手前 separate session、AYA さん指示) で AYA literal「OK handoff 作成して進めてください」record 受領 → 本 handoff doc 起案。
>
> **位置付け**: r41 milestone 内 **Phase 2 着手前準備 phase = 全 UBO 個別設計資料 95 file (= INDEX + 94 UBO) 起案完了 marker** = handoff Phase 1.E (sub-letter) complete = Linux primary baseline 確立後、Phase 2 = **全 UBO 一括本実装化** 前提 (= AYA literal 「Phase 2 で触る UBO はすべてにしてください」record 2026-06-06) で全 94 UBO の個別設計資料 起案完了 + 残 2 資料 (= 関係図 + 判定資料 A/B/C) + Phase 2 工程再設計 を **次 session に handoff**。

## §0. 本 session 経緯 + 失敗 record + 方針 lock 経緯

### §0.1 本 session 着手契機 + 経緯

**着手 contexts**: 前 commit `8a1e4ef221` (= 2026-06-06 全 doc audit 訂正、handoff Phase 1.E (sub-letter) complete + roadmap §2.1 統一基準 Phase 1 完走 ✅ 確定) 後の Phase 2 着手前 separate session。AYA literal 「Phase 1 完了したので Phase 2 の作業内容、工程を明確にする」「Phase 2 と Phase 3 の定義を正確に行う」record 受領で開始。

**経緯 (時系列)**:

1. 私の初期提案 = Phase 2 = UB_REFLECTION_PROBES 単独 + (Q2) A 1 UBO 厳守継承 + Phase 3-5 で R3-R6 分散
2. AYA 懸念 1 = Core 分散対応の処遇 → O3-2 採用 = r42 milestone 移管確定 (= **原則 4 OpenGL 殺さない** 起源)
3. AYA 懸念 2 = OpenGL を殺さない (= dual-path 出荷) → O3-2 採用継続 = r41 release dual-path 出荷
4. AYA 懸念 3 = Mac/Win 対応 → OS-1〜OS-10 gate 起案 = **原則 2 3 OS 同一実装** 起源
5. AYA 4 原則確定 = literal「分散処理を意識した設計を維持、3 OS が同じ処理で動く実装をする、Phase 2 と 3 の作業範囲を明確にして工程を予定する、OpenGL を殺さない、以上を忘れないでください」record 2026-06-06
6. AYA 質問 = Phase 2 対象 UBO 個数? → 私「1 個 (UB_REFLECTION_PROBES)」
7. **AYA 強い指摘** = literal「ほらやっぱりこういうことですね 1 個の UBO だけやって Phase 2 と言う気だった」「現実的に考えてこんな工程の仕方したら Phase 100 まで伸びますよ?」「Phase 1 の設計と工程で実際最初の工程の 4 倍作業が発生しました。あまりにも実装調査せず推論で書いてるからです」「Phase 2 で触る UBO はすべてにしてください」record 2026-06-06
8. AYA 指示 = literal「全 UBO について設計が必要なのがわかってもらえたと思います」「全 UBO に対して全体設計からみてどのように実装する必要があるのか 94 UBO に対し現状状態で網羅した資料を作成する必要があります (実コードから)」「1 UBO 1 ファイルでも構わない」「UBO design のフォルダを作成」「これを先にせずに Phase 2 の工程などと言うものはそもそも語れるわけがありません」record 2026-06-06
9. AYA 質問 = 「UBO と UBO の関係図も必要になる気がしますがどう設計するつもりですか?」 → 私の RELATIONS.md 別 file 提案 → AYA literal「1 UBO 1 資料といいましたよ? UBO 名と資料が一致していたほうが圧倒的に見やすいでしょう」record 2026-06-06 → RELATIONS.md 廃止 + 各 UBO file 内 §11 関係 section 採用
10. AYA 指示 = literal「94 UBO? 91 UBO? どっちなんですいったい を全部書いてください」 → 全 94 UBO + 表記統一確定
11. 全 94 UBO file + INDEX.md = 95 file 起案完了 (= 本 session 達成事項)
12. AYA 追加指示 = literal「では更に 2 つ資料を作ってもらいますがもっと 1 つ 1 つの UBO ファイル資料を精査しながら進めてもらいます」「1 つは UBO の関係図を別資料として」「もう 1 つは各 UBO の実装がすでにすぐ可能である場合には A 判定、情報がまだ不明確でわからないが B 判定、他の UBO が出来上がらないと完成判断がつかないものを C 判定として」「以上の資料を作成してもらったのち、Phase 2 の設計と工程を再度競技します」「この作業に handoff が必要であれば作成してください」record 2026-06-06 → 本 handoff 起案契機

### §0.2 本 session 失敗 record (= 次 session 反面教師)

**失敗 1**: AYA「Phase 2 = 1 UBO」 と言うまで Phase 数約 93 案を確定形 doc 訂正 → AYA literal「ほらやっぱり」批判 = 実装調査せず推論で確定形書く悪癖。memory `feedback_doubt_self_first` + `feedback_admit_unknown` + `feedback_build_only_verified` 違反。

**失敗 2**: AYA「1 UBO 1 資料」指示済なのに RELATIONS.md 別 file 提案 = AYA literal「圧倒的に見やすい」指摘で訂正。AYA literal scope 縮小違反、memory `feedback_no_scope_shrink` 違反。

**失敗 3**: 表記混乱 (= 94 UBO vs 91 UBO) = AYA literal「どっちなんですいったい」指摘。memory `feedback_no_bare_reference_ids` 整合の精度低下。

= **次 session 必読 = §3 起案規律 + §0.2 失敗 record**。同 pattern 繰り返し絶対回避。

### §0.3 方針 lock 経緯

| 確定項目 | AYA literal record | 整合 memory |
|---|---|---|
| Phase 2 scope = **全 UBO 一括 (= 94 UBO)** | 「Phase 2 で触る UBO はすべてにしてください」 | memory `project_r41_phase2_4_principles` 原則 3 訂正必要 (= 次 session) |
| (Q2) literal = **「1 UBO 厳守」廃止、全 UBO 一括採用** | 同上 | (Q2) A → 訂正 (= 次 session roadmap §11.2 訂正) |
| Phase 番号体系 = Phase 2 + Phase 3-6 (= 旧 K+1..K+4 繰上げ) | (推定、AYA 黙示承認、次 session AYA 再確認推奨) | roadmap §2.1 訂正 (= 次 session) |
| **OpenGL 殺さない (= O3-2 採用)** | 「OpenGL を殺さない」record | memory `project_r41_phase2_4_principles` 原則 4 ✅ 確定 (= r42 milestone 移管) |
| **3 OS 同一実装 (= OS-1〜OS-10 gate)** | 「3 OS が同じ処理で動く実装をする」record | memory `project_r41_phase2_4_principles` 原則 2 ✅ 確定 |
| **分散処理意識設計維持 (= C1-C6 制約)** | 「分散処理を意識した設計を維持」record | memory `project_r41_phase2_4_principles` 原則 1 ✅ 確定 |
| 1 UBO 1 資料 + UBO 名 = file 名一致 | 「1 UBO 1 資料」「UBO 名と資料が一致していたほうが圧倒的に見やすい」record | UBO design 95 file 配置 ✅ |
| 関係図 = 別資料 1 file + 同 dir | 「1 つは UBO の関係図を別資料として」 | 次 session 起案 (= §2.1) |
| 判定資料 = A/B/C 別資料 1 file + 同 dir | 「もう 1 つは各 UBO の実装がすでにすぐ可能である場合には A 判定、情報がまだ不明確でわからないが B 判定、他の UBO が出来上がらないと完成判断がつかないものを C 判定」 | 次 session 起案 (= §2.2) |

## §1. 本 session 達成事項

### §1.1 AYA literal 4 原則確定 + memory 永続化

**memory 永続化**:
- 新 memory `project_r41_phase2_4_principles.md` 起案 = 4 原則 (= 分散処理意識設計維持 + 3 OS 同一実装 + Phase 2/3 作業範囲明確 + OpenGL 殺さない) + 適用範囲 + 起源 (= 2026-06-06 AYA literal record)
- MEMORY.md index 追記済 (= 既永続化、commit 対象外、memory store 別管理)

**4 原則 (= 次 session でも継続遵守)**:
1. 分散処理を意識した設計を維持 (= C1-C6 設計制約 6 項目)
2. 3 OS が同じ処理で動く実装をする (= OS-1〜OS-10 gate 10 項目)
3. Phase 2 と 3 の作業範囲を明確にして工程を予定する (= **Phase 2 = 全 UBO 一括** に訂正済、Phase 3 以降 scope 次 session 確定)
4. OpenGL を殺さない (= O3-2 採用 = r41 dual-path 出荷 + r42 milestone 撤廃移管)

### §1.2 全 94 UBO file + INDEX.md 起案完了

**起案先**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/`

**起案 file 95 件**:
- `INDEX.md` (= 全 UBO list + summary table + cadence_tag mapping 確定 + 横断不明事項 12 件集約)
- `Global_ReflectionProbes.md` (= sample 完成形 = 10 項目 + §11 関係 section)
- 残 93 UBO file (= agent 7 並列実施、各 agent 13-15 UBO 担当)

**起案規律遵守**:
- 全 file 実コード source 直接 reference (= ubo_metadata.inl literal + ubo_layout_*.inl + blueprint glsl + 実 shader + host C++)
- 不明事項 §10 + §11 で「不明 / verify 要」明示記載 (= memory `feedback_admit_unknown` 遵守)
- 1 UBO 1 資料 + UBO 名 = file 名厳守 (= AYA literal)
- `indra/` 改変ゼロ (= memory `feedback_design_phase_no_code_write` 遵守)
- §1-§11 全 11 項目 structure (= sample 形式踏襲)

**cadence_tag mapping 確定** (= `llglslshader.cpp:99` literal source):
- 0 = per-frame (3 UBO)
- 1 = per-program (80 UBO)
- 2 = per-draw (7 UBO)
- 3 = per-asset (2 UBO)
- 4 = per-skin (1 UBO)
- 5 = **SINGLETON** (= `flushSingletonUbos` 別経路、process-wide 1 instance、Global_ReflectionProbes 唯一)

**descriptor set 内訳**:
- set 0 = Frame + Global (4 UBO: FrameViewProj/FrameLights/FrameAtmosphere_Lighting/Global_ReflectionProbes)
- set 1 = Material (2 UBO: MaterialUBO/MaterialUBO_Legacy)
- set 2 = PerDraw + PerProgram (約 30 UBO)
- set 3 = Asset + Skin + Legacy (約 58 UBO)

### §1.3 横断的不明事項 12 件集約 (= INDEX §4)

各 UBO file 起案で集約された **横断的不明事項 12 件** = Phase 2 設計入力:

1. set/binding 衝突候補 4 件 (set=3 binding=0/2 cadence 別、set=2 binding=0 6 UBO 共有、set=1 binding=0 排他)
2. cadence 再評価候補 (= PerProgramUBO_VelocityAlphaV/PbrAlphaV/FsObjectIdF mismatch、PerDrawUBO_ObjectSkin size 大、Legacy 多数)
3. 同名 member 別 UBO 重複格納 (= aya_sss_skin_flag, box_center, shadow_target_width, texture_normal_transform, tc_scale)
4. SPIR-V 実 offset vs codegen literal 整合性 (= PostDeferredV/GodraysF/PostDeferredF)
5. member_count 表記揺れ (= SoftenLightParamUBO_Legacy metadata=8 vs blueprint=9)
6. 同 host data source 重複書込み (= WaterFog/UnderWater/WaterV)
7. set=3 帯 bind 単位 verify (~58 UBO 共通)
8. cinematic_bd 系 r30+ chapter member の host writer 未特定
9. V3a 5-set 設計と Legacy UBO 実 binding 配置乖離
10. shell 通電有無 verify (= Phase 1.C handoff doc 参照要)
11. 全 94 UBO の host owner class 多数未確定
12. AYAstorm r14-r28 章追加 cvar 経由 writer 経路未特定

= 詳細は `design/ubo/INDEX.md` §4 参照、本 handoff doc では pointer のみ。

## §2. 残作業 (= 次 session 実施、AYA literal 2026-06-06 確定指示)

### §2.1 UBO 関係図起案 (= 別資料 1 file)

**AYA literal**: 「1 つは UBO の関係図を別資料として USB ファイルと同じディレクトリに書いてください (UBO 関係図となります)」

**起案先**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/<関係図 file 名>.md` (= file 名 私 (Claude) 判断、AYA literal「2 つファイルの名前はおまかせします」)

**推奨 file 名**: `RELATIONS.md` (= 1 UBO 1 資料原則を維持しつつ「関係性専用」明示)

**起案内容**:
- 全 94 UBO の関係性 dimension 7 種一元集約
- §1 descriptor set 同居関係 (set 別 cluster)
- §2 cadence cluster 関係 (flush timing 別)
- §3 shader consume 関係 (同 shader 同時 consume)
- §4 データ依存関係 (同 host data source 由来)
- §5 dirty 連動関係 (1 UBO dirty 時に同時 dirty)
- §6 layout 共有関係
- §7 bind 順序関係
- §8 不明事項

**起案規律 (絶対遵守)**:
- **1 UBO 1 read 順次精査** (= 全 94 UBO file を 1 つずつ Read で精査、agent 並列禁止)
- 推論禁止、不明は不明明示
- 各 dimension で実コード調査 + 各 UBO file §11 内容集約
- memory `feedback_admit_unknown` 遵守

### §2.2 判定資料 (A/B/C) 起案 (= 別資料 1 file)

**AYA literal**: 「もう 1 つは各 UBO の実装がすでにすぐ可能である場合には A 判定、情報がまだ不明確でわからないが B 判定、他の UBO が出来上がらないと完成判断がつかないものを C 判定として、B と C に関してはその理由の詳細も記述して表にした資料を作ってください」

**起案先**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/<判定資料 file 名>.md`

**推奨 file 名**: `READINESS.md` (= 実装 readiness 判定資料、A/B/C 判定 table)

**起案内容**:
- 全 94 UBO の A/B/C 判定 table
- 判定基準:
  - **A 判定** = 実装すぐ可能 (= 全項目情報明確、依存 UBO なし)
  - **B 判定** = 情報まだ不明確、わからない (= **理由詳細記載必須**)
  - **C 判定** = 他 UBO 完成しないと完成判断つかない (= 依存先 UBO list + **理由詳細記載必須**)
- table 列: UBO 名 / 判定 / 理由詳細 / 依存先 UBO (C 判定の場合) / unblocking trigger

**起案規律 (絶対遵守)**:
- **1 UBO 1 read 順次精査** (= 全 94 UBO file を 1 つずつ Read で精査、agent 並列禁止)
- 判定根拠は各 UBO file §10 (不明事項) + §11 (他 UBO 関係) + §6 (既存 setter call site) を集約
- B/C 判定の理由詳細 = 推論禁止、実コード調査ベース
- memory `feedback_admit_unknown` 遵守 (= B 判定理由は「不明事項」literal 引用、推論で埋めない)

### §2.3 Phase 2 工程再設計 (= 2 資料完成後)

**AYA literal**: 「以上の資料を作成してもらったのち、Phase 2 の設計と工程を再度競技します」

**前提**:
- 関係図 + 判定資料 完成
- A 判定 UBO 群 = Phase 2 内即着手可能
- B 判定 UBO 群 = 不明事項解消後着手 (= Phase 2 内 sub-step として情報収集 phase 必要)
- C 判定 UBO 群 = 依存先 UBO 完成順序付き着手 (= Phase 2 内 dependency 順序設計必要)

**次 session 進行 plan**:
1. 関係図 (RELATIONS.md) 起案
2. 判定資料 (READINESS.md) 起案
3. AYA review + 修正 cycle
4. Phase 2 工程再設計 (= roadmap §2.1/§5.2/§5.3 等再構成)
5. AYA literal 承認後 commit

### §2.4 roadmap + handoff 再訂正 (= 既 revert 済 file の再構成)

本 session で起案した既 訂正 (= roadmap §11.3 (Q3) + §2.1 Phase マップ + §2.2/§2.2.1 dependency 図 + §5.2 + §5.3 Exit Criteria + §7 + §8 + handoff §4.4) は **revert 済** (= §6 参照)。次 session で:
- Phase 2 = 全 UBO 一括前提で再構成
- (Q2) literal 訂正 (= 1 UBO 厳守 → 全 UBO 一括)
- Phase 番号体系再確定 (= Phase 2 + Phase 3-6 = 旧 K+1..K+4 繰上げ)
- 4 原則 + r42 移管 + Exit Criteria + dependency 図 + Phase 2 sub-step 分解 を統合再起案
- 既 訂正で消化済の項目 (= 4 原則 + r42 移管 + Exit Criteria C1-C6 + OS-1〜OS-10) は内容 keep + Phase 2 scope のみ訂正

## §3. 起案規律 (= 次 session 絶対遵守、本 session 失敗 record 反面教師)

### §3.1 各 UBO file 精査規律

- **1 UBO 1 read 順次精査** = 全 94 UBO file を 1 つずつ Read で精査 (= agent 並列禁止、AYA literal「もっと 1 つ 1 つの UBO ファイル資料を精査しながら進めてもらいます」遵守)
- 各 file の §1-§11 全項目 + §10 不明事項 + §11 関係性を確認、関係図 + 判定資料起案に反映
- 推論禁止、不明は不明明示 (= memory `feedback_admit_unknown` 遵守)

### §3.2 推論禁止規律

- 本 session 失敗 1 (= Phase 数 93 案推論確定形) の繰り返し絶対回避
- 確定できない項目は「不明 / verify 要」literal 明示記載
- memory `feedback_doubt_self_first` + `feedback_admit_unknown` + `feedback_build_only_verified` 遵守

### §3.3 AYA literal scope 厳守規律

- 本 session 失敗 2 (= RELATIONS.md 別 file 提案で 1 UBO 1 資料原則違反) の繰り返し絶対回避
- AYA literal「1 UBO 1 資料」 + 「UBO 名 = file 名一致」 厳守
- 関係図 + 判定資料の 2 file = AYA literal で明示許可された別資料、ただし UBO 個別 file は別途存続

### §3.4 表記精度規律

- 本 session 失敗 3 (= 94 vs 91 表記混乱) の繰り返し絶対回避
- 数字は **常に確定値 (= 94)** で表記、文脈で「sample 後の残り」等の相対表現禁止
- memory `feedback_no_bare_reference_ids` 整合 = ID + 内容明記

### §3.5 design phase 規律

- `indra/` 改変ゼロ (= memory `feedback_design_phase_no_code_write` 遵守)
- doc 起案 / 既 doc 訂正のみ
- 実コード source は read-only 参照

## §4. 必読 file list (= 最低限 3 件、memory `feedback_handoff_minimal_pre_req_read` 整合)

次 session 着手前 **最低限 3 件 + 本 handoff doc** Read:

1. **`docs/specs/ayastorm-r41-gl-removal/design/ubo/INDEX.md`** (= 横断不明事項 12 件 + cadence_tag mapping 確定 + 全 94 UBO summary table + 段階起案 plan)
2. **`docs/specs/ayastorm-r41-gl-removal/design/ubo/Global_ReflectionProbes.md`** (= UBO file format sample = 10 項目 + §11 関係 section 完成形、関係図 + 判定資料起案の入力 sample)
3. **memory `project_r41_phase2_4_principles.md`** (= 4 原則 + 適用範囲)
4. **本 handoff doc** (= 本 file、§0.1 経緯 + §0.2 失敗 record + §0.3 方針 lock + §2 残作業 + §3 起案規律)

= 残 93 UBO file は **関係図 + 判定資料 起案時に 1 file 1 read で順次精査** = 起案前一括 read 不要 (= context 圧迫回避、memory `feedback_handoff_minimal_pre_req_read` 整合)。

## §5. AYA literal record 2026-06-06 (= 本 session 全 literal 集約)

時系列:

1. **「Phase 1 完了したので Phase 2 の作業内容、工程を明確にする」** + 「Phase 2 と Phase 3 の定義を正確に行う」 (= session 開始時)
2. 「だいたい OK なんですが、懸念は Core 分散対応をこのプロジェクトに内包するかどうかです」(= Core 分散懸念)
3. 「もう 1 つ OpenGL での描画パスを完全に捨てるのは出荷直前まで遅らせたほうが作業がしやすくないか?」(= OpenGL 殺さない懸念)
4. 「もう 1 つ付け足す必要がありそうです。Win と Mac の対応に対する処遇です」(= 3 OS 対応懸念)
5. 「Mac と Windows が困るような実装を Phase 2 でしないようにしてくれです」(= OS gate 起源)
6. **「Phase 2 は 分散処理を意識した設計を維持、3 OS が同じ処理で動く実装をする、Phase 2 と 3 の作業範囲を明確にして工程を予定する、OpenGL を殺さない、以上を忘れないでください」** (= **4 原則確定 literal**)
7. 「UBO はたしか 90 近くあったとおもいますが Phase 2 が対象とする UBO は何個ですか?」(= Phase 2 scope 質問)
8. **「ほらやっぱりこういうことですね 1 個の UBO だけやって Phase 2 と言う気だった」** + 「現実的に考えてこんな工程の仕方したら Phase 100 まで伸びますよ?」+ **「Phase 1 の設計と工程で実際最初の工程の 4 倍作業が発生しました。あまりにも実装調査せず推論で書いてるからです」** + **「Phase 2 で触る UBO はすべてにしてください」** (= **Phase 2 scope 確定 + 推論批判**)
9. **「そこですべての UBO について設計が必要なのがわかってもらえたと思います」** + 「全 UBO に対して全体設計からみてどのように実装する必要があるのか 94 UBO に対し現状状態で網羅した資料を作成する必要があります (実コードから)」 + 「1 UBO 1 ファイルでも構いません」 + 「UBO design のフォルダを作成するなりして各 UBO 名のファイルで実装に必要な情報と設定を正確に資料に落とし込んでください」 + 「これを先にせずに Phase 2 の工程などと言うものはそもそも語れるわけがありません」 (= **UBO design 資料起案指示**)
10. 「UBO と UBO の関係図も必要になる気がしますがどう設計するつもりですか?」(= 関係図必要性指摘)
11. **「OK まずいったんこれで 94 UBO? 91 UBO? どっちなんですいったい を全部書いてください」** + **「私は 1 UBO 1 資料といいましたよ? UBO 名と資料が一致していたほうが圧倒的に見やすいでしょう」** (= **RELATIONS.md 廃止 + 表記統一 + 全 94 起案指示**)
12. 「ありがとう、では更に 2 つ資料を作ってもらいますがもっと 1 つ 1 つの UBO ファイル資料を精査しながら進めてもらいます」 + 「1 つは UBO の関係図を別資料として USB ファイルと同じディレクトリに書いてください (UBO 関係図となります)」 + **「もう 1 つは各 UBO の実装がすでにすぐ可能である場合には A 判定、情報がまだ不明確でわからないが B 判定、他の UBO が出来上がらないと完成判断がつかないものを C 判定として、B と C に関してはその理由の詳細も記述して表にした資料を作ってください」** + 「2 つファイルの名前はおまかせしますが、いずれも UBO 資料と同じディレクトリで構いません」 + 「以上の資料を作成してもらったのち、Phase 2 の設計と工程を再度競技します」 + 「この作業に handoff が必要であれば作成してください」 (= **2 資料起案 + 精査規律 + handoff 指示**)
13. **「OK です handoff 作成して進めてください」** (= 本 handoff 起案 + revert + commit 進行承認)

## §6. 既 doc 訂正 revert 履歴

本 session で起案した既 訂正 (= Phase 2 scope = REFLECTION_PROBES 単独前提) は AYA「Phase 2 = 全 UBO」 指示で **revert 必要**。本 commit 直前に revert 実施。

**revert 対象 2 file**:

1. `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md`
   - 訂正内容: §11.3 (Q3) r42 移管 literal 追記 + §2.1 Phase マップ表 K+4 → r42-8 移管 + K+5 → 新 K+4 繰上げ + Template A R3-R6 反映 + §2.2 dependency 図 + §2.2.1 timeline 図 + §5.2 Phase 順序 Phase 2-5 R3-R6 分散 + §5.3 Exit Criteria 拡張 C1-C6 + OS-1〜OS-10 + §7 r42 reference 化 + §8 Phase K+4 繰上げ
   - revert 理由: §5.2 Phase 2-5 R3-R6 分散 literal が「Phase 2 = 全 UBO 一括」と矛盾、ただし 4 原則 + r42 移管 + Exit Criteria は次 session で再採用予定 = 全 revert + 次 session で「Phase 2 = 全 UBO」前提で統合再起案
2. `docs/specs/ayastorm-r41-gl-removal/handoff/phase1/e/handoff-phase1-e-complete.md`
   - 訂正内容: §4.4 r42 milestone entry 候補 (= r42-1〜r42-9) 追記
   - revert 理由: roadmap §7/§8 訂正と整合連動 = roadmap 訂正 revert に合わせて revert、次 session で r41 milestone 終了 + r42 移行 marker を統合再記載

**revert コマンド** (本 commit 直前実施):

```sh
git checkout -- docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md \
                 docs/specs/ayastorm-r41-gl-removal/handoff/phase1/e/handoff-phase1-e-complete.md
```

= 2 file 改変前 (= 前 commit `8a1e4ef221` 状態) に復元、本 commit 対象から除外。

## §7. commit scope

**本 commit 対象**:
- `docs/specs/ayastorm-r41-gl-removal/design/ubo/INDEX.md` (= 新規)
- `docs/specs/ayastorm-r41-gl-removal/design/ubo/<94 UBO 名>.md` (= 新規 94 file)
- `docs/specs/ayastorm-r41-gl-removal/handoff/phase2-prep/handoff-phase2-prep-ubo-files-complete.md` (= 本 file)

**合計 96 file 新規追加**。

**commit 対象外**:
- `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` (= revert 済)
- `docs/specs/ayastorm-r41-gl-removal/handoff/phase1/e/handoff-phase1-e-complete.md` (= revert 済)
- `memory/project_r41_phase2_4_principles.md` (= memory store 別管理、commit 対象外)
- `memory/MEMORY.md` (= 同上)

**commit message 候補**:

```
docs: r41 Phase 2 着手前準備 = UBO design 95 file 起案完了 + handoff doc

AYA literal 確定 2026-06-06:
- Phase 2 scope = 全 94 UBO 一括 (= literal「Phase 2 で触る UBO はすべてにしてください」)
- AYA 4 原則確定 (= 分散処理意識設計維持 + 3 OS 同一実装 + Phase 2/3 作業範囲明確 + OpenGL 殺さない)
- UBO design 1 file 1 資料 + UBO 名 = file 名一致 (= literal「1 UBO 1 資料」「UBO 名と資料が一致していたほうが圧倒的に見やすい」)

新規 96 file:
- design/ubo/INDEX.md (= 全 94 UBO summary + cadence_tag mapping 確定 (5 種 + SINGLETON) + 横断不明事項 12 件集約)
- design/ubo/<94 UBO 名>.md (= 各 UBO 個別設計資料、§1-§11 全 11 項目、実コード source 直接 reference、不明事項明示)
- handoff/phase2-prep/handoff-phase2-prep-ubo-files-complete.md (= 本 handoff = 次 session 残作業 + 起案規律 + AYA literal record + 失敗 record)

次 session 残作業:
- design/ubo/RELATIONS.md 起案 (= UBO 関係図、1 UBO 1 read 順次精査)
- design/ubo/READINESS.md 起案 (= A/B/C 判定資料、B/C 理由詳細)
- Phase 2 工程再設計 + roadmap 再構成
```

= Co-Authored-By 行不在 (= memory `feedback_no_claude_coauthor` 遵守)

## §A. self-verify 9 観点

1. **UBO design 95 file 起案完了 §1.2** ✅ (= 95 = INDEX + 94 UBO、全 file 配置確認 `ls` で 95 件確認済)
2. **4 原則 + memory 永続化 §1.1** ✅ (= memory file 起案 + MEMORY.md index 追記済)
3. **横断不明事項 12 件集約 §1.3** ✅ (= INDEX §4 配置 + 本 handoff §1.3 pointer)
4. **残作業 §2** ✅ (= 関係図 + 判定資料 + Phase 2 工程再設計 + roadmap 再訂正 4 項目明示)
5. **起案規律 §3** ✅ (= 1 UBO 1 read 順次精査 + 推論禁止 + AYA literal scope 厳守 + 表記精度 + design phase 規律 5 項目)
6. **必読 file list §4** ✅ (= 最低限 3 件 + 本 handoff = 計 4 件、memory `feedback_handoff_minimal_pre_req_read` 整合)
7. **AYA literal record §5** ✅ (= 時系列 13 件全引用)
8. **既 訂正 revert 履歴 §6** ✅ (= 2 file revert 対象 + 理由 + コマンド)
9. **commit scope + message §7** ✅ (= 96 file 新規 + Co-Authored-By 不在 + 4 原則 + 残作業)

### §A.1 feedback 遵守 record

- `feedback_proactive_handoff` ✅ (= 本 handoff doc 起案 = context 周回境界で能動 handoff)
- `feedback_handoff_minimal_pre_req_read` ✅ (= 必読 3 件 + 本 handoff、全 94 UBO file は順次 read で起案時)
- `feedback_design_phase_no_code_write` ✅ (= `indra/` 改変ゼロ、doc 起案のみ)
- `feedback_admit_unknown` ✅ (= 全 UBO file + INDEX §4 で不明事項明示)
- `feedback_doubt_self_first` ✅ (= 失敗 record §0.2 明記、次 session 反面教師)
- `feedback_build_only_verified` ✅ (= 実コード source 直接 reference、推論禁止)
- `feedback_no_scope_shrink` ✅ (= 全 94 UBO 全件 file 起案、抜けなし)
- `feedback_no_auto_commit` ✅ (= AYA literal「OK handoff 作成して進めてください」明示指示後 commit)
- `feedback_no_claude_coauthor` ✅ (= commit message に Co-Authored-By 行不在)
- `feedback_release_branch_workflow` ✅ (= feature branch `feature/ayastorm-r41-gl-removal` 上で commit)
- `feedback_tests_dir_never_commit` ✅ (= root `/tests/` 改変なし、untracked `scripts/ubo_codegen/tests/` は本 commit 対象外、個別 file 指定 add で安全)
- `feedback_no_bare_reference_ids` ✅ (= R3-R6 + C1-C6 + OS-1〜OS-10 + r42-1〜r42-9 + 横断不明事項 12 件 + 4 原則 + 失敗 3 件 全件内容明記)
- memory `project_r41_phase2_4_principles` ✅ (= 4 原則 + 適用範囲、次 session 継続遵守)
- memory `project_ayastorm_r41_design_principles` ✅ (= 2 大設計原則 (1)(2) 継承、原則 1/2 で具体化)
- memory `project_ayastorm_three_platforms` ✅ (= 3 OS 揃える前提、OS-1〜OS-10 gate で実装側担保)

### §A.2 次 session entry 確認 (= AYA literal 受領後)

次 session で本 handoff doc + 必読 3 件 Read 後、AYA literal「次 session 開始」「Phase 2 設計再開」等の record 受領 → §2 残作業着手:
1. RELATIONS.md 起案 (= 各 UBO file 1 つずつ Read で精査、関係 dimension 7 種集約)
2. READINESS.md 起案 (= 各 UBO file 1 つずつ Read で精査、A/B/C 判定 + B/C 理由詳細)
3. AYA review + 修正 cycle
4. Phase 2 工程再設計提案
5. AYA literal 承認後 roadmap 再訂正
6. commit + AYA literal 確認
