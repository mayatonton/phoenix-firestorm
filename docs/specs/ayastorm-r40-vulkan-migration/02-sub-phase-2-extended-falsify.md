# r40 sub-phase 2: LL 待ち + 鉱脈発掘 phase (falsified 2026-05-28)

**status**: falsified 2026-05-28 (起動翌日、1 日鉱脈調査で鉱脈ゼロ確定)
**親 charter**: `00-charter.md`
**位置付け**: r40 章の sub-phase 2 (独立 chapter ではない)
**前章**: sub-phase 1 (`01-sub-phase-1-cpu-perf.md`)
**後章**: sub-phase 3 (Vulkan 化選択 + 工程プラン策定、active)

---

## 1. sub-phase thesis (起動時)

「**LL Vulkan が完成するまで本線を延命する。自前 Vulkan は作らない、体系的 cache も入れない、ad-hoc な「明らかに無駄」を 1 件ずつ削って 5% を 1-2 年で積み上げる。LL 着地時は本家に乗り換え、AYAstorm 機能を再 port する。**」

### 3 つの No と 1 つの Yes

- **No**: 自前 Vulkan 実装 (中継ぎ性質で LL 着地時 reset で消える、経済不合理)
- **No**: 体系的 AZDO 採用 (state cache 層 / PBO 体系的 / fence 全面)
- **No**: 大規模 refactor / 新機能追加
- **Yes**: ad-hoc な無駄削除 (明らかに不要な call / 計算 / polling)

### 起動経緯 (2026-05-27)

- sub-phase 1 全 REJECT + 自前 Vulkan 案 (旧 r41) も同日 drop した後の代替案として起動
- 起動時の論理: 「OpenGL main 剥がしは構造不能、自前 Vulkan も dual maintenance で経済不合理、では何もできないわけではなく、ad-hoc な明らかに無駄を削るのみ可能」
- LL Vulkan 着地 (推測 2029-2030) まで本線を延命、着地後に LL に乗り換え

## 2. 採用打ち手 4 類型 (scope)

| 類型 | 説明 |
|---|---|
| **(a)** | 明らかに無駄な GL call の単独削除 — redundant glBindBuffer / glActiveTexture / glBlendFunc 等 |
| **(b)** | 不要な計算 path の削除 — dead code、unreachable branch |
| **(c)** | 単発 cache hit rate 改善 — hot loop 内の repeated method call を local var に hoist |
| **(d)** | bug fix のついで micro-perf — bug 修正 + 周辺の単独 perf 改善 |

## 3. 採用判断 4 質問 self-check

新 patch 候補は以下 4 質問で判定、**全 YES のみ採用**:

1. 「明らかに無駄」か (毎 frame 同じ結果 / 結果を使ってない)
2. 削除しても visual 副作用ゼロ確認できるか (live A/B)
3. 3 OS 全部で動作確認可能か
4. timing 依存じゃないか (race window / multi-frame lifecycle / state sync 作らない)

1 つでも NO なら drop。

## 4. non-scope (採用しない打ち手)

- 自前 Vulkan 実装 (sub-phase 2 内では中継ぎ性質で却下、sub-phase 3 で復活採用)
- state cache 層の体系的導入 (Blender Eevee 2-3 年 bug 前例)
- PBO async readback 体系的採用 (multi-frame lifecycle 罠)
- fence sync polling 全面採用 (NVIDIA/AMD/Intel/Apple semantic 差)
- MultiDrawIndirect / PMB / bindless / SSBO / compute shader (Mac GL 4.1 で動かない)
- 計測 harness 大規模刷新 (sub-phase 1 で立てた AYAPerfLog で十分)
- visual feature 拡張 (r14+ 章は当時 suspend 想定 / r40 章では本線軽微改善のみ可)
- 大規模 refactor

## 5. release cadence (起動時計画)

| stage | 内容 | 期間目安 |
|---|---|---|
| r40.1 | 鉱脈 1-2 件 + 3 OS 一括 | 2-3 か月 |
| r40.2 | 鉱脈 3-4 件 + 3 OS 一括 | +3-4 か月 |
| r40.N | ... | (1-2 年で累積 5-10% 削減目標) |
| r40-final | LL Vulkan 着地時の最終 release | LL ETA 依存 |

3 OS cadence: (γ) 一括完成、t-noami さん Mac build は各 r40.N で 1 回依頼。

→ ただし r40.1 さえ着手前に falsify、cadence 全廃。

## 6. 1 日鉱脈調査 + ゼロ確定 (2026-05-28)

### 調査の進め方

- AYA さんが 1 日かけて AYAstorm 本線 (現 OpenGL stack) を hot path 中心に手動で棚卸し
- 4 類型 (a)-(d) に該当する候補を発見次第、4 質問 self-check で評価
- 1 つでも YES 全揃いの候補が出れば「鉱脈あり」と判定、r40.1 着手へ
- 1 日経過時点で「鉱脈あり」候補ゼロなら、鉱脈仮説そのものを falsify

### 調査結果

**鉱脈ゼロ確定** (2026-05-28)

- 4 類型 (a)-(d) の候補は複数発見されたが、4 質問 self-check で **全 YES の候補ゼロ**
- 主要な脱落理由:
  - (1) 「明らかに無駄」の閾値で fail (毎 frame 違う結果 / 結果を実際に使っている)
  - (2) visual 副作用が出る (live A/B で差分発生)
  - (4) timing 依存 (削除すると race / 順序 dep が露呈)
- 5% 削減を 1-2 年で積むには 1 件 0.05-0.1% 寄与 × 50-100 件必要、ゼロ候補ではゼロ%

### 帰結: 延命前提の falsify

- sub-phase 2 起動時 thesis 「鉱脈発掘で 5% 削減を 1-2 年で積める」は **falsify**
- 「LL Vulkan が完成するまで本線を延命する」前提 (本線が perf 余白を保ち続ける) も falsify
- AYA 確定: 「**消去法でマルチプロセスを達成するには LL に関係なく Vulkan 化以外に次の進化を乗せられない**」

## 7. r41 drop 論拠の再評価 (2026-05-28)

sub-phase 2 falsify を受けて、2026-05-27 drop した旧 r41 自前 Vulkan migration 案の drop 論拠 4 点を再評価:

| # | 旧 drop 論拠 | 再評価 |
|---|---|---|
| 1 | 中継ぎ性質 (LL 着地で reset) | **覆る** — 「中継ぎ作らない方が経済合理」の天秤で、「進化止まる cost」側が sub-phase 2 falsify で大幅に上昇、論拠崩壊 |
| 2 | viewer fork Vulkan 完遂事例ゼロ | 残る (やらない理由にはならない、charter §1 「parity 完遂前提だが事例ゼロ承知」として正面取扱) |
| 3 | 工数 6-15 人年 | 残る (time horizon (3) 無期限で受容可) |
| 4 | dual maintenance cost | **覆る** — 本線が r40 close 凍結保守になれば dual cost 自体下がる |

→ drop 論拠の (1) と (4) が論理崩壊、(2) と (3) は「やらない理由」ではないと整理、Vulkan 化選択を正式に sub-phase 3 で確定。

## 8. sub-phase close 判断 + sub-phase 3 への引継ぎ

### close 判断 (2026-05-28)

- 1 日鉱脈調査ゼロで延命前提が falsify、sub-phase 2 の thesis 全体が崩壊
- 4 質問 self-check は引き続き sub-phase 3 でも有用 (Vulkan 化期間中の本線軽微改善判定で再利用可) だが、本線延命戦略としては死

### sub-phase 3 への引継ぎ事項

- sub-phase 2 で立てた 4 類型 / 4 質問 self-check は本線凍結保守 (charter §4 (6)) の「軽微」境界線定義で再利用
- LL 着地観測の responsibility は sub-phase 3 charter §plan B trigger に継承
- AYAstorm-vk fork は作らず本線同居 (charter §4 (4)) で進める、これは sub-phase 2 の「中継ぎ作らない」精神に逆行するが、中継ぎ作らない論拠そのものが (4 と (1) で論理崩壊済) と整理

## 9. sub-phase 2 で得た知見 (sub-phase 3 で活用)

### 知見 1: 鉱脈発掘戦略は OpenGL viewer では成立しない

- ad-hoc 削除候補が出ても、4 質問の (2) (4) で大半落ちる
- pipeline.cpp 全体が timing dep / state dep / visual dep で密結合、isolation できる無駄が事実上ゼロ
- これは sub-phase 1 root cause (3 大グローバル + GL 呼出 + geometry mutation) と一致

### 知見 2: 「明らかに無駄」の閾値の現実性

- 「毎 frame 同じ結果 / 結果を使ってない」を満たす call は、過去の Linden Lab / Firestorm dev による掃除でほぼ枯渇
- 残っているのは「結果を使っている」「frame ごとに変わる」「visual 差を生む」もので、削除には risk + work

### 知見 3: 5% 削減の積上げは現実的でない

- 1 件 0.05-0.1% 寄与 × 50-100 件 = 5% という算定は理論的に成立するが、件数を集めるのが不可能
- たとえ件数を集めても 4 質問の (4) timing 依存で次々と回帰 risk、累積で 1 ヶ月 / 件の review 工数発生

→ sub-phase 3 の Vulkan 化は、これらの構造的制約を **OpenGL stack ごと捨てる** ことで解決する path。

## 10. 関連 doc / memory

### doc

- `00-charter.md` — r40 章 charter (sub-phase 1/2/3 包摂、§3 経緯で sub-phase 2 概要)
- `01-sub-phase-1-cpu-perf.md` — sub-phase 1 詳細 (前章)
- `03-sub-phase-3-vulkan-plan.md` — sub-phase 3 詳細 (起草予定、active phase)

### memory

- `project_ayastorm_r40_extended.md` — sub-phase 2 詳細 memory (本 doc と同等内容、historical)
- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory
- `feedback_falsification_as_progress.md` — sub-phase 2 鉱脈ゼロが sub-phase 3 への絞り込み成果
