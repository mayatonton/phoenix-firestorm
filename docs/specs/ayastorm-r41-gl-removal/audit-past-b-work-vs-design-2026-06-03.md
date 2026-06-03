# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 設計 pivot 後 過去 B 作業 整合性 audit (2026-06-03)

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff**: `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-pivot-to-ubo-design.md`
**audit trigger**: AYA さん指示「B 作業を設計を前提に再開、過去にした作業も設計にそっているか確認」(2026-06-03)
**audit scope** (AYA 指示 = (c) + (d)): set=2 26 UBO + set=3 Legacy 32 UBO + set=0 Frame 3 UBO + set=1 Material 2 UBO + sampler 16 個 = **計 79 UBO + sampler**

---

## §0 audit 目的

η-28 pivot 前に積み上げた既存 UBO/sampler 配置が、η-28 期間中に起案された新設計 chapter 群 (`design/01-10` + `06a/06a-prep/06b/06c`) に整合しているかを確認。**revert 要否 / 残置可否 / 設計漏れ有無** を判定し、本 audit 後の Phase 2d-α 処遇 (pivot doc §3.5) + B 作業再開時の追加作業を確定する。

---

## §1 audit 範囲 + 実施手段

### §1.1 audit 対象 (= AYA 指示 (c) + (d) literal scope)

| 帯 | UBO 数 | sampler 数 | 起源 | 確定マップ出典 |
|---|---|---|---|---|
| set=0 `Frame*` | 3 | 4 | r41 base + η-X | `reference-shader-location-map.md` §6-B |
| set=1 `Material*` | 2 | 12 | r41 base | `reference-shader-location-map.md` §6-C |
| set=2 `PerDrawUBO_*` / `PerProgramUBO_*` | 26 | — | η-3 〜 η-28 Phase 2d-α | `pivot-to-ubo-design.md` §2 + `reference-shader-location-map.md` §6-A |
| set=3 `<Name>UBO_Legacy` | 32 | — | η-6 / η-13 期 grouping | `reference-shader-location-map.md` §6-D |
| **合計** | **63** | **16** | — | — |

(注: pivot doc §8 「合計 62+α」表現は本 audit で **63 UBO + 16 sampler = 79 entry** に確定、「+α」の解明は §3.4 に記載)

### §1.2 audit 実施手段

3 Agent 並列起動 (Explore subagent_type) で read-only audit、結果を本 doc に集約。

- **Agent A**: set=2 26 UBO vs 設計 chapter 群
- **Agent B**: set=3 Legacy 32 UBO vs 設計 chapter 群
- **Agent C**: set=0 Frame 3 UBO + set=1 Material 2 UBO + 棚卸し未実施分発見 vs 設計 chapter 群

### §1.3 設計 source of truth (Agent 共通)

- `design/01-overview.md` (= 設計全体像)
- `design/02-naming-convention.md` (= 命名規約 + rename 機械規則)
- `design/03-cadence-classification.md` (= cadence 5 分類: per-frame / per-program / per-draw / per-asset / per-skin)
- `design/04-codegen-ubo.md` (= 新 blueprint 85 UBO 確定)
- `design/05-existing-inventory-link.md` (= 既存 inventory ↔ 新 blueprint mapping)
- `design/06a-cache-structure-and-setter-redirect.md` (= setter 30 entry point redirect)
- `design/06c-descriptor-set-bind-wiring.md` (= set=0/1/2/3 配線確定)
- `design/09-phase-roadmap.md` (= Phase 0-K migration roadmap)

### §1.4 audit 観点 (Agent 共通)

各 UBO の judgment:
- (1) **そのまま残す** = 新 blueprint と整合、機械的 rename のみ
- (2) **統合** = 別 UBO に merge
- (3) **昇格 / 降格** = 寿命分類変更 (e.g., per-program → per-frame)
- (4) **削除** = blueprint 対応無し / 役割重複
- (5) **blueprint 不在 = 設計漏れ** = 既存 UBO 対応 blueprint が新設計に無い (= 致命傷候補)
- (6) **rename 必要** = chapter 02 命名規約乖離
- (7) **field 差分** = field set 不一致

---

## §2 audit 結果 (3 Agent 並列実施、2026-06-03)

### §2.1 Agent A: set=2 26 UBO audit

**整合率 26/26 = 100%**、**blueprint 不在 0 件**。

| 観点 | 結果 |
|---|---|
| cadence 分類一致 (chapter 03 5 分類) | 26/26 (per-program 24 + per-draw 2) |
| naming 規約準拠 (chapter 02 §3.3 rename 表) | 26/26 (機械的 rename `PerDrawUBO_*` → `Draw_*`、`PerProgramUBO_*` → `Program_*`) |
| chapter 05 §3.3 集約表掲載 | 26 件掲載済 (= 2026-06-03 AYA 採用 A' で Q22-NUM 解消、Ch05 §3.3 = 26 個 + per-program 24 + per-draw 2 修正反映済) |
| Phase 配置可能 (chapter 09 Phase 0-K) | 26/26 (1 UBO/Phase 原則で migration 可) |
| field/member 整合 | 26/26 (既存 GLSL 宣言と cadence 分類が乖離無し) |

**判定**: 全 26 件が **(1) そのまま残す** 判定 = 機械的 rename + Codegen-UBO pipeline 入力として確定状態。

**軽微不整合 1 件**:
- Ch05 §3.3 表 head「set=2 帯 (25 個)」記述精度 = 実数 26 個 (binding 0-25)、Ch02 §3.3「個数注」で「26 個が正」と明記済 = **Q22-NUM 解消済 (2026-06-03 AYA 採用 A' = 26 個 + 総数 85 個 確定、Ch05 §3.3 = 26 個 + per-program 24 + per-draw 2 修正完了)**

### §2.2 Agent B: set=3 Legacy 32 UBO audit

**整合率 32/32 = 100%**、**blueprint 不在 0 件**。

| 観点 | 結果 |
|---|---|
| cadence 分類一致 | 32/32 (全件 per-program 寿命統一) |
| naming 規約準拠 | 32/32 (chapter 02 §3.4 機械的 rename `<Name>UBO_Legacy` → `Program_<Name>`) |
| chapter 05 §4.3 処遇判定 | 32/32 (E3 採用 = rename only、set=2 との統合判定は §4.4 後続 phase 移管) |
| Codegen pipeline 入力 | 32/32 (chapter 04 §4.1「UBO ブロック宣言のみ入力」仕様で投入可) |
| set=3 帯 54 UBO への包含 | 32/32 (新 blueprint set=3:54 UBO に完全包含) |

**判定**: 全 32 件が **(1) そのまま残す + (6) rename 必要** 複合判定 = chapter 05 §4.3 E3 採用 (rename only) で確定、設計フェーズを次段階 (chapter 06 redirect-layer-design / chapter 09 phase roadmap) に進行可。

**不整合 0 件**。

### §2.3 Agent C: set=0 + set=1 + 棚卸し未実施分 audit

**整合率 5/5 = 100% (UBO)**、**blueprint 不在 0 件**、**棚卸し漏れ 0 件**。

| 帯 | UBO 数 | 判定 | 備考 |
|---|---|---|---|
| set=0 `FrameViewProj` (binding 0) | 1 | (1) そのまま残す | chapter 06c §2.2 set=0 binding=0 整合 |
| set=0 `FrameLights` (binding 1) | 1 | (1) そのまま残す | chapter 06c §2.2 set=0 binding=1 整合 |
| set=0 `FrameAtmosphere_Lighting` (binding 2) | 1 | (1) そのまま残す + (6) 軽微 rename | `Atmosphere_Lighting` → `Atmosphere` 推奨 (chapter 02 命名規約) |
| set=1 `MaterialUBO` (binding 0) | 1 | **(2) 統合 = set=1 → set=2 per-draw 帯** | chapter 06c §2.4 MC1 採用、set=2 へ昇格 |
| set=1 `MaterialUBO_Legacy` (binding 0、同 binding 衝突) | 1 | **(2) 統合 = set=1 → set=2 per-draw 帯** | chapter 06c §2.4 暫定 binding=3、chapter 05 §5 (F) 待ち |
| sampler 4 個 (set=0) | (sampler) | (3b) 昇格 (set=0 → set=4 or mixed) | chapter 07 持越 (§8 bridge S3) |
| sampler 12 個 (set=1) | (sampler) | (3b) 昇格 (set=1 → set=4 or mixed) | chapter 07 持越 (§8 bridge S3) |

**棚卸し漏れ「+ α」解明**: pivot doc §8 「62 + α」の正体 = set=2 (26) + set=3 (54) = 80 個が **chapter 04 blueprint 85 / reference §6-A / §6-D で既に inventory 化済** (= 漏れではなく集計形式の差)、本 audit で確定。**注**: 2026-06-03 Q22-NUM 解消 A' 反映で旧記 (25/79/84) → 新記 (26/80/85)、+1 per-program UBO 反映。

### §2.4 統合 audit 結果

| 帯 | 件数 | 整合率 | blueprint 不在 | 軽微不整合 |
|---|---|---|---|---|
| set=2 PerDraw + PerProgram | 26 | 26/26 (100%) | 0 件 | 1 件 (Ch05 §3.3 25 vs 26 = Q22-NUM 既知) |
| set=3 Legacy | 32 | 32/32 (100%) | 0 件 | 0 件 |
| set=0 Frame | 3 | 3/3 (100%) | 0 件 | 1 件 (`FrameAtmosphere_Lighting` rename 軽微) |
| set=1 Material | 2 | 2/2 (100%) | 0 件 | 0 件 (set=2 統合判定) |
| **UBO 計** | **63** | **63/63 (100%)** | **0 件** | **2 件 (軽微記述)** |
| sampler 計 | 16 | 16/16 (chapter 07 持越) | — | — |

---

## §3 audit からの導出事項

### §3.1 設計 chapter 群への追加修正 (= 第二次査読 §8.1 修正推奨 18 件への追加)

| 修正 | 内容 | 状態 |
|---|---|---|
| **追加 1** | Ch05 §3.3 表 head「set=2 帯 (25 個)」→「set=2 帯 (26 個)」 = Q22-NUM 解消方針追従 | **解消済** (2026-06-03 AYA 採用 A' = 26 個 + 総数 85 個 確定、Ch05 §3.3 修正反映完了) |
| **追加 2** | Ch02 / 06c で `FrameAtmosphere_Lighting` → `FrameAtmosphere` rename 推奨 (= `_Lighting` suffix 削除のみ、chapter 02 §3.1 既存案準拠) | **解消済** (2026-06-03 Wave G 反映、本 audit §2.3 表 line 106 と本 §3.1 表記揺れ訂正完了、chapter 02 §3.1 既存案 `FrameAtmosphere` 維持 = prefix + 主名規則 `FrameViewProj` / `FrameLights` と整合、06c は該当 string 不在で no-op 確認) |

**= 追加 2 件で設計 chapter 群 修正推奨 18 → 20 件** (= 第二次査読 §8.1 + 本 audit §3.1)、第二次査読 report §8.3 総計 23 件 → 25 件に更新。

**Wave G 表記揺れ訂正 (2026-06-03)**: 本 audit doc 起案時、line 106 「`Atmosphere_Lighting` → `Atmosphere`」 (= member 名視点) と line 134 (旧) 「`Frame_Atmosphere`」 (= 区切り underscore 入り) で表記揺れ。整合 target は **chapter 02 §3.1 既存案 `FrameAtmosphere`** (= `_Lighting` suffix 削除のみ、prefix `Frame` + 主名規則 `FrameViewProj` / `FrameLights` と完全整合) = audit doc 内 `Frame_Atmosphere` (区切り入り) は単純な表記揺れ = chapter 02 §3.1 既存案維持で確定。handoff prep §5.2 / verify_prep §3.x の `Frame_Atmosphere` 言及も同方針で訂正済。

### §3.2 過去 B 作業 (η-3 〜 η-28 Phase 2d-α) の設計整合性結論

| 項目 | 結果 |
|---|---|
| **revert 必要件数** | **0 件** (= 全 26 UBO が新設計に整合、過去 B 作業の implement commit は全部「そのまま残す」判定) |
| **再構成 (統合 / 削除) 必要件数** | **0 件** (= set=2 26 個 + set=3 32 個は機械的 rename のみで設計反映可) |
| **設計漏れ (blueprint 不在) 件数** | **0 件** (= η-28 設計 chapter 群が過去 B 作業を完全包含) |
| **追加 implement 必要件数** | **set=1 → set=2 統合 2 件** (Material* 移動 = chapter 06c §2.4 MC1)、新規実装ではなく既存 binding 配置変更 |

**結論**: 過去 B 作業 (η-28 Phase 2a/2b/2c/2d-α + η-3 〜 η-27 期間中の全 UBO 切出) は **新設計 chapter 群と完全整合**、η-28 implement commit (4 件) は **revert 不要 + 残置可能**。

### §3.3 Phase 2d-α 処遇判断 (pivot doc §3.5)

pivot doc §3.5 の 3 択 (A) push + verify / (B) push 保留 / (C) revert に対する本 audit からの判定:

| 判断軸 | 結果 |
|---|---|
| 設計整合性 (= 本 audit) | (A) 適格 (= 整合率 100%、revert 不要) |
| push 状態 | **既に push 済** (= 本 session 2026-06-03 で `9b4033a735` まで origin 同期完了) |
| cold launch verify 状態 | **未実施** (= pivot 直前で保留) |

**判定**: pivot doc §3.5 の **(A) cold launch verify ルート** を採用、Phase 2d-α 適用済 5 GLSL file (commit `0587c574da`) は **設計適合確認済として残置**、cold launch verify は B 作業再開時の最初の verify task として実施。

### §3.4 pivot doc §8 「62 + α」表現の解明

pivot doc §8 「**合計 UBO 62 + α (要棚卸し)**」の「+ α」は本 audit で **解明 0 件** = **棚卸し漏れ無し**。

- 62 = pivot 査定時点 (η-28 Phase 2c 末) の表記
- 本 audit 時点 (η-28 Phase 2d-α 末) = 63 UBO (= 62 + Phase 2d-α で `SpotLightF` に `vec3 center` field 追加 = UBO 数増加無し、count 1 増の見かけ差)
- 「+ α」候補だった棚卸し未実施分は全件 reference doc §6-A 〜 §6-D に inventory 化済 = chapter 04 blueprint 85 に包含確認 (= 2026-06-03 Q22-NUM 解消 A' 反映で 84→85)

**= pivot doc §8 注記「要棚卸し」は本 audit で解消**。

---

## §4 B 作業再開時の追加作業 (= audit 後の implement scope)

### §4.1 設計 source of truth に従う implement scope

| Phase | scope | source of truth |
|---|---|---|
| **Phase 0 (計測)** | setter 30 entry point 計測 + 既存 GL state cookbook 反映 | `design/06a-prep-phase0-measurement.md` |
| **Phase 1-J (= migration)** | Codegen-UBO pipeline 配線 + 1 UBO/Phase 原則で 63 UBO + 16 sampler 全件 migration | `design/09-phase-roadmap.md` |
| **Phase K (= 完遂判定)** | descriptor set 4 帯 → 5 帯動作維持 + canary cvar on/off 検証 | `design/09 §2.2 phase timeline` (= 第二次査読 §4.2 で AYA 判断待ち) |

### §4.2 Phase 2d-α 残作業 (B 作業再開時の first task)

- **cold launch verify** (= pivot doc §4.4 未実施事項): reference doc §8 cookbook の **parse fail 7→2 / link 0 / 5 program SPIR-V PASS** 期待値で AYA cold launch 動作確認
- **Phase 2d-α complete handoff 起草** (= 動作確認後)
- 結果 PASS → B 作業再開、結果 FAIL → 設計 chapter 群と乖離有無を再 audit

### §4.3 残課題 (= 本 audit 範囲外、AYA 判断待ち)

| 残課題 | 出典 | 状態 |
|---|---|---|
| 致命傷候補 2 件 AYA 判断 | 第二次査読 §2.2 / §4.1 | (Q22-NUM) / (Q23-K) AYA 判断待ち |
| 新規 AYA 判断仰ぎ 4 件 | 第二次査読 §7 | (Q22-NUM) / (Q23-K) / (Q24-S1) / (Q25-21CNT) chapter 10 §1 反映待ち |
| 設計 chapter 群 18+2 件修正 | 第二次査読 §8.1 + 本 audit §3.1 | AYA 判断後に main session で実施予定 |

---

## §5 本 audit 完了宣言

| 項目 | 件数 | 結果 |
|---|---|---|
| audit 対象 | 63 UBO + 16 sampler = 79 entry | 全件 audit 完了 |
| 整合率 | 63/63 (100%) | blueprint 不在 0 件 |
| 軽微不整合 | 2 件 (Ch05 §3.3 25 vs 26 + `FrameAtmosphere_Lighting` rename) | 既存修正推奨 + 追加 2 件 |
| revert 必要件数 | 0 件 | 過去 B 作業全件残置可 |
| Phase 2d-α 処遇 | (A) cold launch verify ルート | B 作業再開時の first task |
| pivot doc §8「+ α」解明 | 0 件追加発見 | 棚卸し漏れ無しと確定 |

**= AYA 指示「過去にした B 作業が設計に沿っているか確認」 = 整合率 100% で完了**、過去 B 作業の **revert 0 件 + 残置可能**、B 作業再開は Phase 2d-α cold launch verify から開始可能。

---

## §6 関連 doc / handoff cross-ref

- `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-pivot-to-ubo-design.md` (= pivot doc、本 audit 直接 trigger)
- `design-review-2026-06-03-second-pass.md` §8 修正推奨一覧 (= 本 audit §3.1 で追加 2 件登録)
- `design/01-10` + `06a/06a-prep/06b/06c` (= 設計 source of truth)
- `reference-shader-location-map.md` §6-A 〜 §6-D (= 既存 inventory 確定マップ)
- `00-charter.md` §5.3a (= 設計 chapter 群 cross-ref 表、本 session 起案)
- `04-frame-context.md` §1.1 / §1.3 / §6.1 (= UBO pivot 反映、本 session 追記)

**本 audit doc は r41 B 作業再開境界の確定資料、永続参照**。
