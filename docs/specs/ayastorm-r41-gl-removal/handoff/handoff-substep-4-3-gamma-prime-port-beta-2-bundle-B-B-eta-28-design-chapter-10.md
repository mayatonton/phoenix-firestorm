# sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 handoff: chapter 10 (open-questions) 起案完了 = 設計 chapter 群 (01-10) 全件起案完了 → AYA commit 指示待ち → implementation-phase (η-29 Phase 0) 入口へ

**完了日**: 2026-06-03
**位置付け**: 前 handoff (chapter 09) を受けて chapter 10 (open-questions) 起案完了 = **設計 chapter 群 (01-10) 全 chapter 起案完了マーク到達**。**AYA commit 指示待ち** → 次 session で commit → implementation-phase (= η-29 Phase 0 計測) 入口 session に移行。

---

## §0 現在の git state

- **unstaged**: `design/01-overview.md` (§4 進捗表 10 → ✅ 起案済)
- **untracked**: `design/10-open-questions.md` (245 行) + 本 handoff doc + `tests/` (= **絶対除外**)
- branch = `feature/ayastorm-r41-gl-removal`、commit 14 進み (push 未)

---

## §1 chapter 10 完成形 (= 設計 chapter 群 最終 chapter)

詳細は **`design/10-open-questions.md` §0-§8** に集約。本 handoff では pointer のみ:

### §1.1 chapter 10 構成 (= §0-§8)

| § | 役割 |
|---|---|
| §0 | 本 chapter の役割と扱い方 |
| §1 | **AYA 判断仰ぎ候補集約** (= 主要 open question) |
| §2 | 実装 phase 入口で消化される項目 (listing のみ、判断は持越) |
| §3 | chapter 09 §12 持越項目 (K の確定値 / 担当者 / 具体 UBO 順) |
| §4 | 各 chapter 内 live 表 (= 本 chapter で持たず該当 chapter 内に保持) の pointer |
| §5 | inventory §7 残課題 9 件の設計 chapter 群カバー状況 |
| §6 | 設計 chapter 未カバー項目 (inventory §7 #7 / #8) |
| §7 | chapter 10 完了 → implementation-phase (η-29 Phase 0) 入口 |
| §8 | 本 chapter update 規律 |

### §1.2 AYA 判断仰ぎ候補総覧 (= 10 §1.1-§1.5 集約)

| 出典 chapter | 候補数 | 項目 ID |
|---|---|---|
| 07 §12 chapter 10 送り | 4 | (V1') / (V3') / (S3') / (W) |
| 08 §17 chapter 10 送り | 7 | (A1) / (P) / (G/B3) / (B1) / (B2) / (B4) / (B5) |
| 09 §11 (Phase Roadmap) | 5 | (Q1) / (Q2) / (Q3) / (Q4) / (Q5) |
| 06b §8 / 06c §10 | 4 | (K) / (M) / (N) / (O) |
| 05 §10 (E3 採用後) | 1 | (F) |

**合計 = 21 件**、各項目 default 採用案 + 判断ポイント明示済 (= 10 §1 表)。AYA 確定で各 chapter §N の default → 確定形書換えが連鎖発動。

### §1.3 実装 phase 入口で消化される項目 (= 10 §2 listing)

- (P1)-(P4): 06a-prep §7 入口 grep
- (RF): 07 §12 reflection update fence throttle 計測
- (P-future) / (cache-grow): 08 §17 実装 phase 評価
- (L)(M)(U1)-(U4): 06b §8 default 確定済、chapter 07 / Phase 進行中で消化
- (V1)(V2): 06c §10、(V1) は (V1') 派生集約済、(V2) は Phase 0 計測待ち
- (T1): 06a §9 scope 外 bug fix track

### §1.4 inventory §7 残課題接続 (= 10 §5)

9 件中:
- **✓ 解消済 = 3 件** (#2 UB_* enum / #3 set=3 Legacy 個数 / #6 redirect 痕跡)
- **設計 chapter でカバー済 = 4 件** (#1 binding 重複 / #4 Frame* member 重複 / #5 MaterialUBO / #9 instance 数)
- **設計 chapter 未カバー = 2 件** (#7 upstream diff / #8 棚卸し外 SSBO/image binding)、本 chapter §6.1 / §6.2 で消化 spec 明示

### §1.5 chapter 09 §12 持越 = 10 §3

- §3.1 K の確定値 = (Q1)(Q2) 確定後
- §3.2 per-Phase 担当者 = AYA 主導 (Linux/Win) + @t-noami (Mac) default
- §3.3 (Q1) Template 確定後の具体 UBO 順 = Phase 0 計測待ち

---

## §2 次 session = AYA commit 指示後 commit phase → implementation-phase 入口 session 起案

### §2.1 commit 単位 (明示パス、`tests/` 厳守除外)

```
git add docs/specs/ayastorm-r41-gl-removal/design/01-overview.md
git add docs/specs/ayastorm-r41-gl-removal/design/10-open-questions.md
git add docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-10.md
```

commit subject:
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 UBO 全体設計 chapter 10 (open-questions) 起案 = 設計 chapter 群 (01-10) 全件起案完了 + AYA 判断仰ぎ候補 21 件集約 + inventory §7 接続
```

### §2.2 commit 後 = AYA 判断 session vs implementation-phase 入口 session どちらに進むか

**選択肢 A (AYA 判断 session 先行)**: AYA 判断仰ぎ候補 21 件 (= 10 §1.1-§1.5) を順次確定 → 各 chapter §N default → 確定形書換え → implementation-phase 入口へ
**選択肢 B (implementation-phase 入口先行)**: (Q1)-(Q5) default 採用案を仮確定として η-29 Phase 0 計測 session に進む → Phase 0 計測結果で (Q1) 具体 UBO 順 + (F) MaterialUBO 比較 + (V2) binding 重複 + (RF) fence throttle が連鎖確定 → 残 17 件を Phase 0 結果反映後に AYA 判断

**default 推奨 = B (implementation-phase 入口先行)**: Phase 0 計測結果が 21 件中 (Q1) (F) (V2) (RF) 4 件 の入力に必要、AYA 判断を Phase 0 計測後に集約した方が判断材料揃う + AYA 判断 session の往復回数最小化。

ただし AYA 判断指示優先 = 本判断は AYA に委ねる。

### §2.3 commit 後 → implementation-phase 入口 session scope (= 選択肢 B 採用時)

implementation-phase 入口 session = **η-29 Phase 0 計測 session** = 設計 chapter 群 (01-10) 完了後の最初の indra/ 改変 phase。

#### §2.3.1 Phase 0 入口 pre-flight check (= chapter 09 handoff §5 + chapter 10 §7.2 集約)

1. **06a-prep §7 (P1)-(P4) 4 件 grep 消化**:
   - (P1) `LL_INFOS("UBO_CADENCE")` class 名衝突 grep
   - (P2) CMake patch 配置先 grep (`cmake/*.cmake`)
   - (P3) frame counter 公開方式 = (P4) 結果次第
   - (P4) `gFrameCount` 等流用可能性 grep (`llviewercontrol` / `llappviewer`)
2. **AYAstorm build flow 確認** (memory `project_build_procedure`):
   - configure → build → install → cache clear
   - `--fmodstudio` + `LL_DULLAHAN_AUDIO_CALLBACK` フラグ
3. **log path 確認** (memory `reference_log_path`):
   - Linux `~/.ayastorm_x64/logs/AYAstorm.log`
4. **計測 build flag 配置先確定** (= (P2) grep 結果次第):
   - `-DAYASTORM_UBO_CADENCE_HOOK=ON`

#### §2.3.2 Phase 0 計測 spec 実施 (= `06a-prep-phase0-measurement.md` 手順書)

- (H1b) LL_INFOS hook 配置 → 計測 build → AYA 起動 → log 取得
- (E') inventory §3.3.1 binding 重複 grep
- (F) MaterialUBO vs MaterialUBO_Legacy member 比較 + program 単位 attach grep
- (RF) reflection update fence throttle 計測同梱

#### §2.3.3 Phase 0 解析 + 反映 flow

- §5 解析 (= cadence 別 update 頻度集計 + per-frame / per-program / per-draw 境界判定)
- §6 反映 flow (= 結果を chapter 03 / 05 / 06b に追記、Phase 1 入力 state を整備)

#### §2.3.4 Phase 0 完了 → Phase 1.A 入口 handoff doc 起案

- (Q1) Template 確定 + 具体 UBO 順確定 (= AYA 判断仰ぎ)
- (F) MaterialUBO 処遇確定
- Phase 1.A scope = Codegen Python 実装 + CMake patch (= chapter 08 §17 7 件 + chapter 09 §10.1 持越紐付け)

### §2.4 commit 後 → AYA 判断 session scope (= 選択肢 A 採用時)

- 10 §1.1-§1.5 表 21 件を順次 AYA に判断仰ぎ
- 判断確定毎に該当 chapter §N default → 確定形書換え
- 21 件確定完了で implementation-phase 入口 (= 選択肢 B §2.3) に移行

---

## §3 必須 Read 2 件 (= **これ以上事前 Read しない**、不足は現場 pinpoint Read)

1. **本 handoff doc** (= chapter 10 完成形 + 次 session scope (選択肢 A/B) + Phase 0 入口 pre-flight + Phase 0 計測 spec 実施 flow が pointer 化済)
2. **`design/10-open-questions.md` §0-§8** (= AYA 判断仰ぎ候補 21 件 + 実装 phase 入口消化 listing + inventory §7 接続が集約済、次 session の主入力)

**意図的に除外**:
- 各 chapter 01-09 / 06a-prep / inventory = 本 handoff §1 + chapter 10 §1-§5 で pointer 化済。次 session で個別調査必要時のみ pinpoint Read (= 該当節のみ)
- `06a-prep-phase0-measurement.md` = 実装 phase 入口 session で手順書として fresh read、本 handoff 段階では §1.4 (= H1b/E'/F/RF) 集約済の pointer のみで十分
- chapter 09 §11 + §12 = 本 handoff §1.2 (= (Q1)-(Q5) 集約表 + chapter 09 §12 持越 = 10 §3) で集約済、再読不要

---

## §4 規律 (絶対遵守)

- **design-phase**: chapter 10 完了で **design-phase 完了マーク**、次 session 以降は implementation-phase = `indra/` 配下改変 OK (= memory `feedback_design_phase_no_code_write` の design-phase 制約は本 handoff 完了 commit で終了)
- **tests/**: commit / 確認 / 言及禁止 (memory `feedback_tests_dir_never_commit`) = implementation-phase 入口でも継続
- **commit**: AYA 明示指示まで実行しない (memory `feedback_no_auto_commit`)、`git add -A` / `git add .` 禁止 (明示パスのみ)
- **事前 Read**: §3 の 2 件のみ全読、その他は現場 pinpoint Read (memory `feedback_handoff_minimal_pre_req_read`)
- **完了時**: 設計 chapter 群 (01-10) 完成 = AYA 報告 → commit 指示待ち → implementation-phase 入口 (= η-29 Phase 0 計測) session 起動
- **context 残量**: 周回境界で自分から handoff doc 提案 (memory `feedback_proactive_handoff`)
- **1 UBO 厳守**: implementation-phase Phase 2 以降は 1 UBO ずつ migration 厳守 (memory `feedback_ubo_migration_one_at_a_time`)、cluster 化は AYA 判断のみ
- **3 OS 大前提**: Phase K+1 以降は 3 OS 確証必須 (memory `project_ayastorm_three_platforms`)、Linux 単独判断は明示指示なき限り取らない

---

## §5 implementation-phase 入口 session 用 memo (= chapter 10 commit 後の次々 session 用)

η-29 Phase 0 入口 session で必要となる準備 memo:

### §5.1 Phase 0 計測 build flag default 値

- `-DAYASTORM_UBO_CADENCE_HOOK=ON` で計測 hook 有効化
- log tag `LL_INFOS("UBO_CADENCE")` (= P1 衝突確認後確定)
- frame counter source = `gFrameCount` 等流用 (= P4 確認後確定)
- 計測対象 = 16 不明 cadence + per-frame / per-program / per-draw 境界 + reflection update fence

### §5.2 Phase 0 log 取得 protocol

- AYAstorm 起動 → AYA 機 typical 撮影 scene (= memory `project_ayaudit_account` Morris+Bonifacio SLurl) で 30 秒 idle + 30 秒 cam pan + 30 秒 zoom in/out
- log file = `~/.ayastorm_x64/logs/AYAstorm.log`、Claude 側 grep + 集計 (memory `feedback_log_reading`)
- 集計結果 → 06a-prep §5 解析 spec に従って per-UBO update 頻度 + per-draw 帯確定 + per-program 帯確定

### §5.3 Phase 0 完了基準

- 16 不明 cadence 全件 per-frame / per-program / per-draw / per-asset / per-skin に分類確定
- (Q1) Template 確定の入力データ揃う
- (F) MaterialUBO 処遇 default 採用可否判定可能
- (V2) inventory §3.3.1 binding 重複 5 個 解消方向確定
- (RF) reflection update fence throttle 必要性確定

### §5.4 Phase 0 完了後の AYA 判断 session

- (Q1)(F)(V2)(RF) 4 件は Phase 0 結果で確定
- 残 17 件 = (V1')(V3')(S3')(W)(A1)(P)(G/B3)(B1)(B2)(B4)(B5)(Q2)(Q3)(Q4)(Q5)(K)(M)(N)(O) を AYA 判断 session で集約確定
- 確定完了で chapter 04 / 05 / 07 / 08 / 09 / 06b / 06c の default → 確定形書換え連鎖発動

---

**= 本 handoff doc + §3 の Read 2 件で chapter 10 確認 + implementation-phase 入口 session 起動再現可能**。**設計 chapter 群 (01-10) 全件起案完了 = design-phase 完了マーク到達**、次 phase = implementation-phase 入口 (= η-29 Phase 0 計測) session 起動可能 state に到達。
