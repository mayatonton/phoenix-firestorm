# handoff = r41 Phase 2.L0 着手 entry (= 再精査 → L0-1 trace small prototype → L0-1 実装 の 3 段階構造)

**起案日**: 2026-06-06 (= 旧版 a0a4f0a583 から書き直し)
**書き直し契機**: AYA literal 2026-06-06 雑談指摘「あなたも作れる気がしませんって資料を書いてきてる気がする」+「ではなぜそういう計画を立てて提案してこないんでしょうか?」 → 旧版 (= L0 全 4 protocol 一括着手提案) は **大塊一括着手 + 不確実性総量未 count + 立ち止まる sub-session 不在** で失敗、本版は **3 段階小さい cycle 構造** に再起案
**前提条件**: r41 Phase 2 前提条件 work 全完走 (= handoff `phase2-prep/handoff-phase2-prep-complete.md` 参照、AYA 承認受領 2026-06-06)
**設計 phase 解除点**: 本 Phase 2 = **実装 phase**、`indra/` 配下 code 改変開始 (= memory `feedback_design_phase_no_code_write` 規律解除点) **ただし sub-session 1-2 (= 再精査 + small prototype) は indra/ 改変ゼロ維持**

---

## §1. 経緯 + 旧版失敗 record

### §1.1 旧版 (= commit a0a4f0a583) の失敗

旧 handoff `handoff-phase2-l0-entry.md` (= 上書き前) は以下 3 件で失敗:

| 失敗 | 内容 | 根本原因 |
|---|---|---|
| **F1: 大塊一括着手** | L0-1〜L0-4 4 protocol 全件を 1 phase で進行する計画として書いた | memory `feedback_ubo_migration_one_at_a_time` の精神を planning phase に適用しない盲点 |
| **F2: 不確実性総量未 count** | WORK_ORDER 内 `[要追加調査]` / `[要 verify]` / `[要 AYA 判断]` マーク 100+ 件の total count + 分類をしていない | 「マーク並べれば OK」誤評価、実数 count による不確実性可視化を怠った |
| **F3: 立ち止まる sub-session 不在** | 「再精査 session」「small prototype session」を計画に含めず、即実装着手を提示 | 「進める = 良い」「立ち止まる = 進捗ゼロ」誤バイアス、AYA さんにリスク管理肩代わりさせた状態 |

### §1.2 AYA literal 確定 2026-06-06 (= 本書き直し契機)

1. 「**あなたも作れる気がしませんって資料を書いてきてる気がする**」 = 旧版は不確実性を抱えたまま「進める方向」だけで起案された (= F2 + F3 指摘)
2. 「**何が足りてないのかがわたしには見えてこない**」 = 不確実性 count + 分類が doc 上に明示されていない (= F2 指摘)
3. 「**もう 1 度再精査してもらったほうがいいのか、それとも L0 だけでも進めて再精査を踏むべきか悩んでいます**」 = 再精査 / small prototype の必要性に AYA さん自身が気付き、Claude 側が最初から提案すべきだった
4. 「**ではなぜそういう計画を立てて提案してこないんでしょうか?**」 = Claude 側のリスク管理視点欠落の指摘
5. 「**書き直してください**」 = 本書き直し承認

### §1.3 本書き直し方針 (= memory `feedback_proactive_risk_management` 適用)

| 適用 | 内容 |
|---|---|
| (a) 不確実性総量 count | sub-session 1 (= 再精査 session) で実 count、分類、AYA literal 確認候補 list 化 |
| (b) 再精査 / small prototype 必要性 self-check | 4 protocol 全件で不確実性大、再精査 + small prototype 先行 default 含める |
| (c) 大塊一括着手 default reject | L0 4 protocol 一括 reject、1 protocol ずつ 3 sub-session cycle (= 再精査 → trace prototype → 実装) で組む |

### §1.4 r41 milestone 進行状況 (= 旧版から継承)

| Phase | 状態 |
|---|---|
| Phase 0 計測 / Phase 1.A/1.B/1.C/1.D/1.E | ✅ 完了 |
| **Phase 2 前提条件 work** | ✅ 全完走 (= commit chain `aa65bf96f1` → `289fa2842c`) |
| **Phase 2.L0 sub-session 1 = 再精査** | 🔜 次 session 着手 (= 本 handoff 第一 entry) |
| **Phase 2.L0 sub-session 2 = L0-1 trace small prototype** | sub-session 1 完了後 separate session |
| **Phase 2.L0 sub-session 3 = L0-1 実装** | sub-session 2 完了後 separate session |
| Phase 2.L0 sub-session 4-9 (= L0-2 同 3 cycle / L0-3 同 3 cycle / L0-4 同 3 cycle) | L0-1 完了後、各 protocol で同 3 段階 cycle 繰返し |
| Phase 2.L0 Exit | 全 L0 4 protocol 実装完了 + READINESS.md update + 既存通電 UBO regression ゼロ |
| Phase 2.L1a〜L5 | Phase 2.L0 Exit 後 separate session |
| Phase 3-6 / r42 milestone | 旧版継承 |

---

## §2. Phase 2.L0 着手構造 (= 3 段階 × 4 protocol = 12 sub-session 想定)

### §2.1 基本 cycle (= 1 protocol あたり 3 sub-session)

各 L0 protocol (L0-1 / L0-2 / L0-3 / L0-4) で同 3 段階を順次:

| step | 内容 | `indra/` 改変 | Exit 条件 |
|---|---|---|---|
| **A: 再精査 session** | 該当 protocol 関連 doc cold read + 不確実性 count + AYA literal 確認候補 list 化 | **ゼロ** | 不確実性 count 完了 + AYA literal 確認 list 提示 + AYA literal 確認受領 |
| **B: trace small prototype session** | 既存 pilot 通電 UBO の該当 protocol 関連実装を既存 reading のみで trace、protocol 案との整合確認 | **ゼロ** (= 既存 reading のみ) | 整合確認 OK (= 実装着手 unblocking) or 整合 NG (= 設計再起案 → A step 戻り) |
| **C: 実装 session** | host C++ + GLSL shader 実装 + cold launch validation + visual regression ゼロ verify | **改変開始** | 該当 protocol cold launch validation PASS + visual regression ゼロ + AYA live verify |

### §2.2 全 sub-session 順序 (= 12 sub-session、Phase 2.L0 完走想定)

| # | sub-session | scope | 想定工数 |
|---|---|---|---|
| 1 | L0-1.A 再精査 | name-based dispatch 関連 doc cold read + 不確実性 count + AYA review 1 件 (= set=3 binding 衝突 3 site 解消方針) 確認候補 | 1 session |
| 2 | L0-1.B trace small prototype | 既存 pilot 通電 UBO (= Skin_GLTFJoints / Asset_GLTFMaterials / PerDrawUBO_LightParams / Asset_GLTFNodes / Global_ReflectionProbes) の dispatch logic 既存実装 reading | 1 session |
| 3 | L0-1.C 実装 | name-based dispatch logic 実装 (= L0-1.B 整合確認後)、cold launch + visual regression ゼロ | 1-2 session |
| 4 | L0-2.A 再精査 | LLStaticHashedString redirect 経路関連 doc cold read + 不確実性 count + AYA review 1 件 (= mapping table 構築方式) 確認候補 | 1 session |
| 5 | L0-2.B trace small prototype | LLStaticHashedString uniformN setter call site 全件 grep + 既存 dual-write 経路有無 確認 | 1 session |
| 6 | L0-2.C 実装 | LLStaticHashedString redirect mapping table + intercept + dual-write 経路実装 + cold launch | 1-2 session |
| 7 | L0-3.A 再精査 | per-shader UBO block 拡大方式関連 doc cold read + 全 shader file `LL_VULKAN_GLSL` block 完備性 grep + AYA review 2 件 (= preprocessor inject 方式 + upstream merge conflict strategy) 確認候補 | 1 session |
| 8 | L0-3.B trace small prototype | 既存 LL_VULKAN_GLSL block の preprocessor 動作 trace (= η-1〜η-28 phase で確立済 logic 再確認) | 1 session |
| 9 | L0-3.C 実装 | per-shader UBO block 拡大 preprocessor inject 実装 + cold launch | 1-2 session |
| 10 | L0-4.A 再精査 | cadence 関連 doc cold read + Phase 0 計測 input 反映状態 確認 + AYA review 2 件 (= cadence 再分類 strategy + sliced UBO Phase 2 内包) 確認候補 | 1 session |
| 11 | L0-4.B trace small prototype | 既存 PerProgram / PerFrame cadence の stale data risk 実 logic trace + UBO 単位 cadence 帰属再分類提案 | 1 session |
| 12 | L0-4.C 実装 + Phase 2.L0 Exit | cadence 再分類実装 + cold launch + READINESS.md update + Phase 2.L0 Exit handoff 起案 | 2-3 session |

合計 **12-16 session 想定** (= 旧版「数 sub-session」表記を実 count で具体化)。各 sub-session 完了境界が自然な handoff 切替境界。

### §2.3 cycle 中の手戻り protocol

各 sub-session で以下手戻りを default 想定:

- **B → A**: trace 結果が protocol 案と不整合 → 設計再起案 (= A step 戻り)
- **C → B**: 実装中の cold launch reject → 整合再確認 (= B step 戻り)
- **C → A**: cold launch reject 原因が設計案破綻 → 設計大幅変更 (= A step 戻り、§4.7 stage 2/3 適用)

手戻り発生は **失敗ではなく cycle の正常動作** (= memory `feedback_falsification_as_progress`)。AYA literal 確認 escalate も default 動作 (= §4.7 stage 3、memory `feedback_admit_unknown` 適用)。

---

## §3. Phase 2.L0 sub-session 1 (= 再精査 session) 詳細 scope

### §3.1 着手目的 (= AYA literal 「何が足りてないのかが見えてこない」解消)

**不確実性総量 count + 分類 + AYA literal 確認 candidate list 化** を doc 化、AYA さんが「Claude 提案を信用できるか / 何が不足か」を客観 view で判断可能にする。

### §3.2 cold read 対象 (= 全 read、`indra/` 改変ゼロ維持)

| # | doc | 目的 |
|---|---|---|
| 1 | `design/ubo/WORK_ORDER.md` 全 3500 行 | 各 sub-work (2) 不明事項マーク全件 count + 分類 |
| 2 | `design/ubo/READINESS.md` 全 | A/B/C 判定理由 + B 判定 31 件詳細 + C 判定 16 group 詳細 |
| 3 | `design/ubo/RELATIONS.md` 全 700+ 行 | cross-UBO 同期 protocol 4 trigger + 不明事項 15 dimension |
| 4 | `design/ubo/INDEX.md` 全 | 全 94 UBO summary + cadence_tag mapping 整合 |
| 5 | 各 `<UBO>.md` §10 不明事項 + §11 他 UBO 関係 (= 94 file × 2 section) | 個別 UBO 不明事項 + cross-reference 確認 |
| 6 | `design/01-overview.md` §2 2 大設計原則 + §5 確定事項 13 件 | 設計 phase 確定事項 cross-reference |
| 7 | `design/06a-prep-phase0-measurement.md` §6 反映 flow | Phase 0 計測結果 + cadence 帰属確定状況 |
| 8 | `design/06b-dirty-flag.md` / `design/06c-descriptor-set-bind.md` | Phase 1.B/1.C 既設実装 logic |
| 9 | `design/08-build-codegen-pipeline.md` §6.4 + §13 + §17 持越 | std140 padding + 3 OS 確証 + 持越項目 |
| 10 | `handoff/phase1/{a,b,c,d,e}/handoff-phase1-*-complete.md` | Phase 1.A〜1.E pilot 通電実績 + 残課題 |

### §3.3 count 出力 doc 起案

sub-session 1 完了時に新規 doc 起案: `handoff/phase2/audit/handoff-phase2-l0-uncertainty-audit.md` (= 不確実性監査 doc)

**内容**:

#### §3.3.1 マーク総 count

| マーク | 件数 | 分類 |
|---|---|---|
| `[要追加調査]` | N1 件 | 解消可能 (= grep / Read で確定可能、A1 件) / 実装試行必要 (= prototype 必要、B1 件) / AYA literal 確認 (= 設計判断、C1 件) |
| `[要 verify]` | N2 件 | (同上) |
| `[要 AYA 判断]` | N3 件 | (全件 AYA literal 確認 candidate) |
| `[要 L0-N 結果反映]` | N4 件 | (L0-N protocol 完了後 解消) |

合計 N = N1 + N2 + N3 + N4 件。

#### §3.3.2 protocol 別不確実性集約

| protocol | 関連不確実性 件数 | AYA literal 確認 candidate 件数 | 実装試行必要 件数 |
|---|---|---|---|
| L0-1 name-based dispatch | M1-1 件 | M1-2 件 | M1-3 件 |
| L0-2 LLStaticHashedString redirect | M2-1 件 | M2-2 件 | M2-3 件 |
| L0-3 per-shader UBO block 拡大 | M3-1 件 | M3-2 件 | M3-3 件 |
| L0-4 cadence 再評価 | M4-1 件 | M4-2 件 | M4-3 件 |

#### §3.3.3 AYA literal 確認 candidate list 全件

各 candidate に対して:
- 該当 doc 位置 (= file + line + 該当 sub-work)
- 確認内容 (= 何を AYA 判断要か)
- 確認 timing (= L0-N.A 着手前 / L0-N.B 着手前 / L0-N.C 着手前 / Phase 2.L0 Exit 前)
- 推奨案 (= protocol-A/B/C/D / memory feedback による default 推奨)

#### §3.3.4 不足要素 list (= AYA literal「何が足りてないのか」直接回答)

旧版 Claude side で挙げた 6 件 (= 既存 OpenGL UBO 実装 pilot trace 経験不足 / upstream Firestorm setter call site 全件 trace 不足 / 3 OS 別 driver 実機検証不足 / shader file `LL_VULKAN_GLSL` block 完備性 unknown / L0-3 prototype 未実装 / L0-4 cadence Phase 0 計測反映 unknown) を、sub-session 1 で各々 詳細 count + 解消 plan 提示。

#### §3.3.5 sub-session 1 完了時の AYA literal 確認内容

AYA さんに以下提示:
- 不確実性総量 N 件、protocol 別内訳
- AYA literal 確認 candidate 件数 + 内容
- 不足要素 6 件の各々 解消 plan
- sub-session 2 (= L0-1.B trace small prototype) 着手 OK / NG 判断要請

### §3.4 sub-session 1 Exit 条件

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | cold read 完了 | §3.2 全 10 件 doc 読了 |
| 2 | uncertainty audit doc 起案完了 | §3.3 全 sub-section 内容反映 |
| 3 | AYA literal 確認内容提示 | §3.3.5 5 件提示 |
| 4 | AYA literal 確認受領 | sub-session 2 着手承認 or 設計再起案指示 |

sub-session 1 結論パターン:
- **A: sub-session 2 着手 OK** = 不確実性 count + 不足要素 解消 plan が AYA さん納得、L0-1 trace small prototype 着手
- **B: 設計再起案** = 不確実性 count が大きすぎ / 不足要素解消 plan が不十分 → WORK_ORDER 部分再起案 (= sub-session 1.X として継続)
- **C: protocol 設計大幅変更** = 4 protocol 自体が現実的でない判断 → memory `project_r41_phase2_4_principles` 原則 3 再交渉 (= AYA literal 確認 escalate)

---

## §4. Phase 2.L0 sub-session 2 (= L0-1 trace small prototype) 詳細 scope

### §4.1 着手目的

**既存 pilot 通電 UBO 5 件の dispatch logic を既存実装 reading のみで trace**、L0-1 protocol 案 (= WORK_ORDER §2.1 起案) との整合確認。整合確認できれば L0-1 実装着手 unblocking、整合 NG なら設計再起案。

### §4.2 trace 対象 (= 既存 pilot 通電 UBO 5 件、`indra/` 改変ゼロ = Read only)

| pilot UBO | trace 内容 | 関連 file |
|---|---|---|
| Skin_GLTFJoints | host dispatch logic + set/binding bind 経路 + program 識別 logic | `llvkloader.cpp` PC-N-5/11/15c 5 setter site + `llgltfasset.cpp` writeSkinUbo |
| Asset_GLTFMaterials | host dispatch logic + set/binding bind 経路 | `llvkloader.cpp` PC-N-7 + `llfetchedgltfmaterial.cpp` writer |
| Asset_GLTFNodes | host dispatch logic + set/binding bind 経路 + per-node accessor | `llvkloader.cpp` + `llgltfnode.cpp` |
| PerDrawUBO_LightParams | host dispatch logic (= zero IS real data semantic) | `llvkloader.cpp` PC-N-13 |
| Global_ReflectionProbes | host dispatch logic (= shell zero dummy write) | `llvkloader.cpp` PA-8 + 1.C PC-7γ-1 |

### §4.3 trace 出力 doc 起案

sub-session 2 完了時に新規 doc 起案: `handoff/phase2/audit/handoff-phase2-l0-1-dispatch-trace.md` (= L0-1 dispatch logic trace 結果)

**内容**:
- 既存 pilot 通電 UBO の host dispatch logic 実装位置 + 動作 logic + L0-1 protocol 案との整合性
- set=3 binding 衝突 3 site (= AtmoExtra/Asset_GLTFNodes / SkyV/Asset_GLTFMaterials / SkyF/Skin_GLTFJoints) の既存 dispatch 動作確認
- L0-1 protocol-A (= host dispatch logic 仕様) + protocol-B (= shader UBO block 名称規約) + protocol-C (= binding 衝突解消) + protocol-D (= 既存 pilot 整合) の整合 / 不整合 判定
- 整合 NG 部分の設計再起案候補

### §4.4 sub-session 2 Exit 条件

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | 既存 pilot 通電 UBO 5 件 trace 完了 | 全 5 件で dispatch logic 実装位置 + 動作 logic 確定 |
| 2 | dispatch trace doc 起案完了 | §4.3 内容反映 |
| 3 | L0-1 protocol 案整合判定 | 整合 OK (= 実装着手 unblocking) / 整合 NG (= 設計再起案) 判定明示 |
| 4 | AYA literal 確認受領 | sub-session 3 着手承認 or 設計再起案指示 |

---

## §5. Phase 2.L0 sub-session 3 (= L0-1 実装) 詳細 scope

### §5.1 着手目的

**name-based dispatch logic 実装** (= host C++ + GLSL shader)、cold launch validation + visual regression ゼロ verify。

### §5.2 実装範囲 (= `indra/` 改変開始)

WORK_ORDER §2.1.4 protocol-A/B/C/D 採用案 (= sub-session 1/2 で確定):
- protocol-A 実装: host dispatch logic (= LLPipeline or LLGLSLShader 内、program ID + bind target slot 入力、UBO buffer handle + offset 出力)
- protocol-B 実装: shader UBO block 名称規約適用 (= INDEX.md 全 94 UBO 名称 verify)
- protocol-C 実装: set=3 binding 衝突 3 site 解消 (= AYA literal 確定方針採用)
- protocol-D 実装: 既存 pilot 通電 UBO logic 整合化

`mUseUBO` runtime flag default OFF 維持 (= 原則 4 §4.4 O-2)、`#ifdef LL_VULKAN_GLSL` C++ 不使用 (= 原則 4 §4.4 O-3)。

### §5.3 sub-session 3 Exit 条件

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | host C++ 実装完了 | `mUseUBO=true` 時 dispatch logic 動作 |
| 2 | GLSL shader 接続完了 | 該当 shader `#ifdef LL_VULKAN_GLSL` block 活性化 |
| 3 | cold launch validation | Linux validation layer warnings 0 件 |
| 4 | 既存通電 UBO regression 確認 | pilot 通電 5 UBO の動作変化なし |
| 5 | visual regression ゼロ verify | AYA live verify (= 視覚 regression なし) |
| 6 | commit | L0-1 実装 commit |
| 7 | AYA literal Exit 承認 | sub-session 4 (= L0-2.A 再精査) 着手承認 |

---

## §6. L0-2 / L0-3 / L0-4 sub-session 詳細 scope

各 protocol で sub-session A/B/C 同 3 段階 cycle 適用 (= §2.1)。詳細 scope は **L0-1 完了後の separate session で個別起案** (= 旧版「全件起案」失敗の轍を踏まない、protocol ごとに小さい cycle で起案)。

各 protocol で必読 doc + AYA review candidate + trace 対象 + 実装範囲を sub-session A 着手前に該当 handoff doc 起案。

---

## §7. Phase 2.L0 Exit 条件 (= 12 sub-session 全完走後)

WORK_ORDER §2.5.3 + §4.3 R-5 確定済:

1. L0-1〜L0-4 4 protocol 実装完了 (= cold launch validation + Linux validation 0)
2. 既存通電 UBO regression 確認 (= visual regression ゼロ §5.4)
3. **READINESS.md update** = §3 + §4 + §5 + §6 全反映 + B 判定 31 件 → A 昇格 candidate 確定 + 残 B/C 個別解消事項明示
4. A 昇格判定基準明示 = sub-work (6) A 確定条件達成で個別 sub-step 内最終確定
5. Phase 2.L1a〜L5 sub-step 着手 unblocking

Phase 2.L0 Exit handoff doc 起案: `handoff/phase2/handoff-phase2-l0-complete.md` (= sub-session 12 完了時起案)。

---

## §8. 着手前必読 (= sub-session 1 着手時)

### §8.1 最低限 3 件

1. **本 handoff doc** = 書き直し版、3 段階構造 + 旧版失敗 record + AYA literal 2026-06-06 確定
2. **`design/ubo/WORK_ORDER.md`** §2 L0 protocol 4 件詳細 + §2.5 AYA review check list + §4 4 原則 gate
3. **前 handoff** `phase2-prep/handoff-phase2-prep-complete.md` = Phase 2 前提条件 work 全完走 record

### §8.2 memory 必読

- **`feedback_proactive_risk_management`** = 本書き直しの直接根拠 (= 新規追加 memory)
- **`project_r41_phase2_4_principles`** = 4 原則確定 + Phase 2.L0 Exit 条件
- **`project_r41_phase1b_vulkan_host_gate`** = host C++ redirect 層 = `mUseUBO` runtime flag のみ、`LL_VULKAN_GLSL` C++ 不使用
- **`feedback_ubo_migration_one_at_a_time`** = 1 UBO ずつ実装 + cold launch 検証 (= planning phase でも適用)
- **`feedback_build_only_verified`** = 効果未確認 commit / 設計倒れ spec を積まない
- **`feedback_admit_unknown`** = 推論禁止、不明明示
- **`feedback_doubt_self_first`** = 効かない時はまず自分のコード / 仮説を疑う
- **`feedback_self_verify_before_handoff`** = handoff 出す前に self-trace
- **`feedback_falsification_as_progress`** = 全 REJECT verdict も生き残りルート絞り込みの成果 (= 手戻りは正常動作)
- **`feedback_no_scope_shrink`** = AYA literal scope 厳守
- **`feedback_release_flow`** = push / PR は AYA 側、Claude はローカル commit まで
- **`feedback_no_claude_coauthor`** = 全 commit message で Claude 共著行を含めない

---

## §9. 起案規律 (= 本 handoff 全 sub-session 規律)

### §9.1 立ち止まる sub-session を正常動作として扱う

- 再精査 / small prototype sub-session は **「進捗ゼロ」ではない** = 不確実性 count / 整合確認は同等の進捗
- 手戻り (= B → A / C → B / C → A) は **失敗ではない正常動作** = memory `feedback_falsification_as_progress`
- AYA literal 確認 escalate は **default 動作** = memory `feedback_admit_unknown` 適用

### §9.2 各 sub-session 完了時の handoff 起案

- 各 sub-session 完了時に **必ず handoff doc 起案** (= `handoff/phase2/audit/handoff-phase2-l0-{sub-session}.md`)
- 次 sub-session 着手前 AYA literal 確認
- AYA literal 確認受領後 separate session entry

### §9.3 大塊一括着手 default reject

- 「複数 sub-session を 1 session で進める」提案は default reject
- AYA literal「進められるところまで進めてください」自走承認継続中でも、sub-session 境界は守る
- 例外 = AYA literal 「ここまで 1 session で進めて OK」明示時のみ

### §9.4 indra/ 改変規律

- sub-session A (= 再精査) = `indra/` 改変ゼロ
- sub-session B (= trace small prototype) = `indra/` 既存 reading のみ、改変ゼロ
- sub-session C (= 実装) = `indra/` 改変開始 (= design-phase 規律解除)

---

## §A. 本 handoff 関連 commit

| commit | 内容 |
|---|---|
| `aa65bf96f1` | Phase 2 前提条件 WORK_ORDER C-6 L4 全 16 group 起案完了 |
| `323b6bf0df` | Phase 2 前提条件 work 全完走 (= C-8 §4 + §2.1.1 + L3 20 file §12) |
| `bd292e6968` | Phase 2 範囲確定 = AYA literal「全 UBO を Phase 2 のスコープ」+ Phase 3-6 繰上げ反映 |
| `289fa2842c` | Phase 2.L0 Exit 条件 = READINESS.md update 確定反映 |
| `a0a4f0a583` | (旧版、失敗 record として残存、本 handoff §1.1 で参照) |
| (本 handoff) | Phase 2.L0 着手 entry 書き直し版 + memory `feedback_proactive_risk_management` 新規追加 |

---

## §B. 次 session 開始時 AYA 確認

「上記 handoff 確認、Phase 2.L0 sub-session 1 = **再精査 session (= cold read + 不確実性 count + AYA literal 確認 candidate list 化)** で着手 OK か?」

**AYA literal「OK」受領後の即時着手項目**:
1. §3.2 cold read 対象 10 件を順次 Read (= `indra/` 改変ゼロ)
2. uncertainty audit doc 起案開始 (= `handoff/phase2/audit/handoff-phase2-l0-uncertainty-audit.md`)
3. 不確実性 count 完了時点で AYA literal 確認内容提示

**recover protocol**: sub-session 1 中に「不確実性が想定以上 / 不足要素が解消困難」と判明したら、即 AYA literal 確認 escalate (= 設計再起案 or protocol 設計大幅変更の判断要請)。
