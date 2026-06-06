# r41 UBO 全体設計 Chapter 09: Phase Roadmap (= 番号体系再編 + 1 UBO ずつ migration 設計)

**起案日**: 2026-06-03
**位置付け**: 設計 chapter 群 (01-08) で確定した UBO 化機構 + Codegen pipeline + Vulkan API state を、**実装 phase の Phase 番号 (= sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 以降) に分解** する roadmap。各 Phase 入口で Phase 0 計測 (06a-prep) を前提化し、1 UBO ずつ migration + cold launch 検証 (memory `feedback_ubo_migration_one_at_a_time`) を Phase 進行ルールとして固定する。
**pre-requisite**:
- `01-overview.md` §2 (2 大設計原則) + §5 (確定事項 13 件)
- `06a-prep-phase0-measurement.md` §2-§6 (Phase 0 計測 spec、各 Phase 入口で参照)
- `07-vulkan-api-state.md` §6 / §7 / §8 / §11 / §12 (pool / ring buffer / fence sync / PSO cache / 持越 (W2)(R1 = ring buffer)(PSC)(RF))
- `08-build-codegen-pipeline.md` §13 (3 OS 確証 X-α/β/γ) / §17 (持越 (A1)(P)(G/B3)(B1)(B2)(B4)(B5)(P-future)(cache-grow))

**用語注記**: 本 chapter は AYA 判断仰ぎ候補を **(Q1)-(Q5)** と表記する。chapter 07 §12 持越 (R1) (= ring buffer 容量) と表記が衝突するため、本 chapter 内で扱う roadmap question は Q-prefix で書き分ける。

---

## §0 本 chapter の scope と非 scope

### §0.1 scope

1. **Phase 番号体系再編** = sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29 以降を UBO migration 用 Phase として番号付与
2. **1 UBO ずつ migration の Phase 内訳** = memory `feedback_ubo_migration_one_at_a_time` 準拠の Phase 細分化
3. **Phase 0 計測の Phase 番号化** = 06a-prep §2-§4 の (H1b)(E')(F) 計測を Phase 0 として独立化
4. **3 OS 確証 Phase 番号化** = 08 §13.4 X-α/β/γ を具体 Phase 番号に展開
5. **Phase 完了判定基準** = cold launch + log + canary + 3 OS 確証で構成する Phase Exit Criteria
6. **持越項目 Phase 紐付け** = 07 §12 (W2)(R1)(PSC)(RF) + 08 §17 (A1)(P)(G/B3)(B1)(B2)(B4)(B5)(P-future)(cache-grow) を Phase に配置
7. **AYA 判断仰ぎ候補 (Q1)-(Q5)** = roadmap 確定前に AYA 判断が必要な 5 項目

### §0.2 非 scope

- **Phase 内コード詳細** = 各 Phase の実装手順は実装 phase 入口 (= 該当 Phase の handoff doc) で展開、本 chapter は **Phase scope + Exit Criteria + 紐付け項目** に絞る
- **特定 UBO の cadence 確定** = 06a-prep Phase 0 計測完了後にしか確定しない、本 chapter では「Phase 0 完了後に決定」とのみ記す
- **chapter 10 持越判断** = (V1')(V3')(S3')(W) は chapter 10 で扱う、本 chapter 内で再判定しない (= 07 §12 で chapter 10 へ送ったものを 09 で先行決定しない)

---

## §1 Phase 番号体系の前提

### §1.1 現行 sub-step 体系上の位置付け

r41 Vulkan migration の sub-step 体系上、本 roadmap が扱うのは:

```
4.3-γ'-port-β-2-bundle-B-B?-η-29 以降
```

η-1 〜 η-28 は **Vulkan parse error 解消 phase** (= 85 UBO blueprint 積み上げ + bare uniform 集約 + chapter 01-08 起案) として既消化済。η-29 以降が **UBO 化 migration phase** (= host C++ redirect 層実装 + per-UBO migration + 3 OS 確証 + release) となる。

### §1.2 Phase 番号付与原則

- 1 Phase = 1 sub-step η-N (η-29, η-30, ...) を原則とする
- 大規模 Phase (= Phase 1 codegen pipeline 整備) は η-N.A / η-N.B / η-N.C 等の **sub-Phase** に細分化
- UBO 1 個 migration = 1 sub-Phase (1 η-N)、ただし関連 UBO cluster (= 同一 owner 内の 2-3 UBO) を同 Phase で扱うかは (Q2) で AYA 判断
- handoff doc は 1 Phase 1 件起案 (= 各 Phase 入口で再開可能 state を保持)

### §1.3 Phase 進行ルール (= memory `feedback_ubo_migration_one_at_a_time` + `feedback_build_only_verified` 準拠)

- 各 Phase 入口で Phase 0 計測結果 (= 06a-prep §3.5 / §4.6 / §5.3 反映済表) を参照、不足あれば該当計測 task を再実施
- Phase 内 migration は **1 UBO ずつ実装 → cold launch 検証 PASS → 次へ**
- cold launch 検証 NG → 該当 UBO Phase を REJECT、原因究明後再 Phase 化
- 大塊バッチ (= 2 UBO 以上同 Phase) は (Q2) AYA 判断で例外許可されたケースのみ

---

## §2 Phase 全体マップ (= Phase 0 〜 Phase N+3)

> **2026-06-06 全 doc audit 訂正注記**: 本 §2.1 + 後続 §3-§8 = **Phase 完了判定の source of truth** (= AYA さん合意 2026-06-06、INDEX.md §1.1 整合)。実装側で並走している handoff sub-letter 体系 (= Phase 1.A..1.E + Phase 1.F+ + Mac/Win 補完 phase) は **実装 sub-step tracking 軸**、本 §2.1 Phase 番号体系と並走。各 sub-letter の本 §2.1 上の position は §2.1 表「handoff sub-letter 対応」列参照。**audit 確認事項** = 現実装 handoff Phase 1.D/1.E は本 §2.1 Phase 2..K Template A 順序 (= UB_REFLECTION_PROBES → GLTFMaterials → GLTFNodes → GLTFJoints) を逸脱した pilot 先回り着手 (= Skin_GLTFJoints + PerDrawUBO_LightParams「zero IS real data」semantic、UB_REFLECTION_PROBES 本実装 skip 状態) = roadmap 統一基準では **Phase 1 完走未達 + Phase 2..K 順序逸脱状態**。Phase 1 完走 gate 残件 R1-R10 は handoff-phase1-e-complete.md §5.2 参照。

### §2.1 Phase マップ表

| Phase 番号 | 名称 | scope 要約 | sub-step | handoff doc | 入口 Exit 判定 | handoff sub-letter 対応 (= 2026-06-06 audit) | 現状 status |
|---|---|---|---|---|---|---|---|
| **Phase 0** | 計測 phase | 06a-prep §2-§4 の (H1b)(E')(F) 実機計測 + 結果 chapter 反映 | η-29 | archive eta-29 phase0-step1/2/4/5-complete | 06a-prep §6 反映 flow 全行「反映済」 | (η-29 phase 着手前 prep + AYA 実機計測 = step3) | ✅ (= 2026-06-03 完走、commit `4e40fd2ab0` mechanical revert) |
| **Phase 1** | codegen + redirect 層整備 + shell UBO 1 個通電 | chapter 08 codegen pipeline 実装 + chapter 06a redirect 層 + 06b dirty flag + 06c descriptor set bind + 1.C で shell UBO 1 個 (= UB_REFLECTION_PROBES) 5 cadence 全経路 `vkCmdBindDescriptorSets` 通電 | η-30 (.A/.B/.C 細分) | handoff/phase1/{a,b,c}/handoff-phase1-{a,b,c}-complete.md | Phase 1.A/B/C 各 Exit 全 PASS | handoff Phase 1.A ✅ + 1.B ✅ + 1.C shell 段階 ✅ (= UB_REFLECTION_PROBES `Global_ReflectionProbes` shell zero dummy write までで停止) | **shell 段階 ✅、Phase 2 本実装着手 ⏳** (= R1/R2/R3 残、handoff-phase1-e-complete.md §5.2) |
| **Phase 2** | UB_REFLECTION_PROBES 単独本実装 (= Template A R3) | R3 = `Global_ReflectionProbes` shell zero dummy → 実 reflection data per-frame mip chain | η-31 (.A/.B/.C 細分予定) | 起案予定 (= Phase 2 着手前 separate session、AYA さん指示 2026-06-06) | 1 UBO 通電 + cold launch + canary + log + visual regression ゼロ (= WORK_ORDER §4.5 V-1) | handoff Phase 1.C shell 段階で `Global_ReflectionProbes` shell zero dummy write 通電済 (= 本 Phase 2 = 実 reflection data 通電) | **着手前** ⏳ |
| **Phase 3** | UB_GLTF_MATERIALS + PerDrawUBO_LightParams bundle (= Template A R4 + R5) | R4 + R5 bundle = `Asset_GLTFMaterials` per-asset 本実装 + 実 PBR shader 接続 + `PerDrawUBO_LightParams` 実 light data (= (Q2) 例外 = 2 UBO 同 Phase) | η-32 (.A/.B/.C 細分予定) | 起案予定 (= Phase 2 完走後 separate session、AYA さん指示 2026-06-06) | 2 UBO bundle 通電 + cold launch + canary + log + visual regression ゼロ | handoff Phase 1.D で `PerDrawUBO_LightParams` 「zero IS real data」 semantic pilot 着手済 (= 本 Phase 3 = 実 light data 通電 + Asset_GLTFMaterials 本実装) | **着手前** ⏳ |
| **Phase 4** | UB_GLTF_NODES 単独本実装 (= Template A 残) | `Asset_GLTFNodes` per-asset 本実装 | η-33 (.A/.B/.C 細分予定) | 起案予定 | 1 UBO 通電 + cold launch + canary + log + visual regression ゼロ | handoff Phase 1.D で `Asset_GLTFNodes` pilot 着手済 (= 本 Phase 4 = 本実装) | **着手前** ⏳ |
| **Phase 5** | UB_GLTF_JOINTS 単独本実装 + avatar Vulkan 通電 (= Template A R6) | R6 = `Skin_GLTFJoints` per-skin 本実装 + avatar Vulkan draw 通電 + `sPlaceholderSkin` 撤去 | η-34 (.A/.B/.C 細分予定) | 起案予定 | 1 UBO 通電 + avatar Vulkan draw 通電 + cold launch + canary + log + visual regression ゼロ | handoff Phase 1.E で `Skin_GLTFJoints` 「zero IS real data」 semantic pilot 着手済 (= 本 Phase 5 = 実 bone matrix 通電 + sPlaceholderSkin 撤去) | **着手前** ⏳ |
| **Phase 6..K** | 残 UBO (= L1〜L5 残 88 UBO) 順次本実装 | L1a 3 + L1b 2 + L2 4 + L3 20 + L4 62 + L5 3 - Phase 2-5 既使用 6 UBO = 残 88 UBO 順次 (= 各 Phase 原則 1 UBO、cluster 許可 = (Q2) AYA 残判断) | η-35〜η-K (= K=93 想定、cluster 許可で短縮可能性) | Phase ごと起案予定 | 各 UBO 通電 + cold launch + canary + log + visual regression ゼロ | (WORK_ORDER.md §3 起案完了 = 各 UBO sub-work 7 dim 参照) | **着手前** ⏳ |

**Phase K 確定条件 (= 設計 review 2026-06-03 §3.4 K 確定明示、2026-06-06 (Q1)(Q2) 一部確定)**: 「K」は Phase 2 から始まる migration Phase 群の最終 Phase 番号 (= 全 UBO 分続けた最後)。**2026-06-06 時点 K 確定状況**:
1. **Phase 0 計測結果**: ✅ 完了 (= 2026-06-03 commit `4e40fd2ab0` mechanical revert)、UBO 総数 = **94 UBO + L0 4 protocol** 確定 (= WORK_ORDER.md §1.1 Layer 体系 = L0 4 + L1a 3 + L1b 2 + L2 4 + L3 20 + L4 62 + L5 3 = 94)
2. **(Q1) 第 1 UBO 選定**: ✅ 確定 (= UB_REFLECTION_PROBES = Phase 2 R3、AYA literal 2026-06-06 = memory `project_r41_phase2_4_principles`)、Template A R3-R6 順序確定
3. **(Q2) Phase 当たり migration UBO 数**: **一部確定** = Phase 2 (R3 = 1 UBO) / Phase 3 (R4+R5 bundle = 2 UBO **例外**) / Phase 4 (Node = 1 UBO) / Phase 5 (R6 = 1 UBO) 確定 (= WORK_ORDER §4.3 Phase 範囲表整合)、**Phase 6..K cluster 許可 (= 1 Phase に 2-3 UBO 同 batch) 採否は残 AYA 判断**

→ K 確定は **Phase 6..K cluster 許可 採否 = AYA 残判断 揃った時点**。それまで本 chapter §2/§3/§5/§6/§7/§8 の「K」「K+1」「K+2」等の表記は **暫定 placeholder** として扱う (= 確定後本 chapter §2.1 表で具体数値に置換)。**現状確定範囲では Phase 2-5 = 4 Phase / 計 5 UBO (Phase 3 のみ 2 UBO bundle 例外)、Phase 6..K = 残 88 UBO 順次 (= cluster 採否で K = 93 程度 or 数十)**。
| **Phase K+1** | 3 OS 確証 (Linux) | 08 §13.4 X-α = Linux 全 UBO 動作確認 + log 検証 + sample scene 確認 | η-(K+2) | 起案予定 | Linux build pass + cold launch normal + render parity | (Linux primary baseline は handoff Phase 1.E (sub-letter) complete で確立 ✅、本 Phase K+1 = 全 UBO 通電後の最終 Linux 確証) | ⏳ |
| **Phase K+2** | 3 OS 確証 (Windows) | 08 §13.4 X-β = Windows build + 起動 + render parity (= AYA 実機) | η-(K+3) | 起案予定 | Windows build pass + render parity | (Mac/Win 補完 phase = AYA さん指示 2026-06-05「Linux 完成後」literal record で Phase K+2/K+3 として deferred) | ⏳ |
| **Phase K+3** | 3 OS 確証 (macOS) | 08 §13.4 X-γ = macOS build + 起動 + render parity (= @t-noami 実機委任) | η-(K+4) | 起案予定 | macOS build pass + render parity | (同上、@t-noami 実機委任) | ⏳ |
| **Phase K+4** | OpenGL path 撤廃 | (Q3) で OpenGL 並走撤廃時期を AYA 判断、撤廃後は Vulkan のみ | η-(K+5) | 起案予定 | OpenGL path code 削除 + 3 OS build pass | ((Q3) A 確定 = 全 UBO 移行完了まで並走、本 Phase K+4 で初撤廃) | ⏳ |
| **Phase K+5** | release 整備 | release note 起草 + tag 切り出し + AYAstorm release flow | η-(K+6) | 起案予定 | release note + tag commit | - | ⏳ |

**K = Phase 6..K cluster 許可 (= (Q2) 残判断) 確定後に決まる**。**現状確定範囲では Phase 2-5 = 4 Phase / 計 5 UBO (Phase 3 のみ 2 UBO bundle 例外 = R4+R5)、Phase 6..K = 残 88 UBO 順次 (= cluster 採否で K = 93 程度 or 数十)**。論理 binding 4 種 + 85 blueprint 集約結果次第で cluster 採用時 K = 5-20 程度に短縮可能 (= cadence 別集約で同一 layout cluster を 1 Phase に纏める案を (Q2) Phase 6..K で議論)。WORK_ORDER.md §4.3 Phase 範囲表 ↔ 本 §2.1 表 双方向 link (= memory `project_r41_phase2_4_principles` 原則 3、AYA literal 2026-06-06「Phase 2 と 3 の作業範囲を明確にして工程を予定」)。

#### §2.1.1 Phase 2..K sub-step level 詳細起案 = WORK_ORDER.md 参照 (= 全 94 UBO 体系)

**位置付け**: 上記 §2.1 Phase マップ表は **Phase 番号体系 source of truth**、Phase 2..K migration の **sub-step level 全 94 UBO 詳細起案** は別 doc `design/ubo/WORK_ORDER.md` 参照。本 §2.1.1 = 双方向 link + L0-L5 体系サマリ + 件数表 (= AYA literal 2026-06-06「94 項目維持」前提)。

**WORK_ORDER.md scope** (= Phase 2 前提条件 work):
- Phase 2 着手前の **94 UBO + L0 4 protocol** 全件 sub-work 7 dimension 起案
- AYA literal 確定 4 原則 (= Core 分散 / 3 OS 共通 / Phase 範囲 / OpenGL を殺さない) + 視覚 regression ゼロ gate 評価
- 各 UBO trace 順 + 並列可能性 + 推定工数 + A 確定条件

**Layer 体系 (= L0-L5、94 UBO + L0 4 protocol 内訳)**:

| Layer | 内容 | UBO 件数 | verify 単位 | 着手契機 | WORK_ORDER §参照 |
|---|---|---|---|---|---|
| **L0** | 横断 protocol 4 件 確立 (= 別カウント、L1-L5 各項目の前提) | 4 protocol | regression 確認のみ | 本資料 §2 完成後即着手 | §2 (= 4 sub-section) |
| **L1a** | 横断 protocol 影響大 UBO (= LLStaticHashedString 経由 + 独立) | **3** (= CAS / Clip / VisualizeBuffersF) | 個別 UBO (= 即 verify) | L0 完了後 | §3.1 |
| **L1b** | per-shader UBO block 拡大 (= FrameViewProj / FrameLights) | **2** (= 50+ file 一括) | 一括 verify | L0 完了後、L1a 並列可 | §3.2 |
| **L2** | B Tier α (= setter 特定済、minor verify) | **4** (= PbrTerrainV / AOUtil / MotionBlur / DeferredUtil) | 個別 UBO (= 即 verify) | L1 完了後 | §3.3 |
| **L3** | B Tier β (= setter 推定済、Grep 確定要) | **20** | 個別 UBO (= 即 verify) | L1 完了後、L2 並列可 | §3.4 |
| **L4** | C 16 group (= cross-UBO 同期 / pair / sequential pipeline) | **62** (= 16 group) | group verify | L1-L3 進行中も独立 group は並列可 | §3.5 (= 17 sub-section) |
| **L5** | A-1 + B Tier γ (= cadence mismatch + debug trigger 不明) | **3** (= Skin_GLTFJoints + FsObjectIdF + NormaldebugV) | 個別 UBO (= 即 verify) | A-1 = Phase 1.F+ real bone matrix 接続、B Tier γ = cadence mismatch 解消後 | §3.6 |

UBO 項目数合計 (= 横断 protocol 除く) = 3 + 2 + 4 + 20 + 62 + 3 = **94 件** (= INDEX §2 一致、AYA literal 94 項目維持) ✅

**WORK_ORDER と本 chapter の役割分担**:
- 本 chapter §2.1 = **Phase 番号体系 + Phase Exit Criteria + 全体 timeline** (= source of truth)
- WORK_ORDER §3 = **Phase 2..K 内の sub-step 94 UBO 詳細起案** + L0-L5 trace 順 (= Phase 2 着手前の前提条件 work、sub-step level)
- 双方向 link = 本 §2.1 ↔ WORK_ORDER §3 でPhase 番号と sub-step level の互参照
- AYA literal 「Phase 2 着手前に Phase 2/3 詳細 scope 定義 separate session」 (= 2026-06-06) で起案する work = WORK_ORDER 本体 (= 既起案完了状態)

**4 原則 + 視覚 regression ゼロ gate** (= WORK_ORDER §4 normative 定義):
- 原則 1: Core プロセス分散 (= C1-C6 設計制約)
- 原則 2: 3 OS 共通 (= OS-1〜OS-10 gate)
- 原則 3: Phase 2/3 範囲明確 (= R-1〜R-4 = Template A R3-R6 所属 + O3-2 r42 移管)
- 原則 4: OpenGL を殺さない (= O-1〜O-5 = dual-path + `mUseUBO` runtime flag)
- 視覚 regression ゼロ (= V-1〜V-4 = AYA literal 2026-06-06 追加条件、§5.4 policy)
- 評価 protocol = sub-work (7) で全 94 UBO + L0 4 protocol 逐次 check (= 98 件)
- violation 検知時 = stage 1 提案撤回 / stage 2 設計再考 / stage 3 AYA literal 確認

### §2.2 Phase 依存関係 (= 前提が満たされないと開始できない)

```
Phase 0 (計測)
    ↓
Phase 1.A (codegen pipeline 実装)
    ↓
Phase 1.B (redirect 層 = setter 内部 Vulkan path)
    ↓
Phase 1.C (cadence 別 update site + dirty flag + descriptor set bind)
    ↓
Phase 2 (第 1 UBO migration)
    ↓
Phase 3 (第 2 UBO migration)
    ↓
... (K-1 個続く)
    ↓
Phase K+1 (Linux 全 UBO 確証) ─── (Q4) で並走か順次か
Phase K+2 (Windows 確証)    ─── 確定
Phase K+3 (macOS 確証)      ─┘
    ↓
Phase K+4 (OpenGL 撤廃) ─── (Q3) で並走 vs 中間撤廃判断
    ↓
Phase K+5 (release)
```

#### §2.2.1 Phase K+1/+2/+3 並列 / 順次 timeline 図 (= 第二次査読 §4.2 反映、(Q4) 案別)

**(Q4) AYA 判断による 3 案** = Linux 先行 / Linux+Win/Mac 並列 / 全 OS 順次。各案の timeline (= 横軸 = wall-clock 時間):

**(Q4-A) Linux 先行 (= default 提案)**:
```
時系列 →
[K+1: Linux 確証] → [K+2: Windows 確証] → [K+3: macOS 確証] → [K+4 撤廃 → K+5 release]
```
= Linux で全 UBO 通電確認後、Windows/macOS を順次。Linux 検出 issue は K+1 内で fix、K+2 以降は build/起動 + render parity のみ。

**(Q4-B) Linux 先行 + Win/Mac 並列**:
```
時系列 →
[K+1: Linux 確証] → ┬─ [K+2: Windows 確証] ─┐
                    └─ [K+3: macOS 確証]    ─┴→ [K+4 撤廃 → K+5 release]
```
= Linux 完了後、Windows/macOS は wall-clock 並列。Windows REJECT 検出時は K+2 Exit 後 K+3 で fix、macOS REJECT 同様。3 OS の中で 1 OS でも REJECT なら K+4 entry 延期 (= §6.2.2 line 324 言及形)。

**(Q4-C) 全 OS 順次 (= 並列 risk 回避)**:
```
時系列 →
[K+1: Linux] → [K+2: Win] → [K+3: Mac] → [K+4 撤廃 → K+5 release]
```
= (Q4-A) と同等、ただし「並列の余地が出ても採らない」明示 (= 並列起因の build/test 干渉 risk 完全排除)。

**含意**: Phase 番号 (K+1/+2/+3) は **論理依存** であり **絶対時間軸** ではない (= (Q4-B) では K+2 と K+3 が wall-clock 同時並走可)。本 §2.2 dependency 図は **論理依存** 描画、time-axis 描画は §2.2.1 timeline 図側で表現。

---

## §3 Phase 0: 計測 phase (η-29)

### §3.1 scope

06a-prep §2-§4 で書面化済の Phase 0 計測 task を **実機実施 + 結果反映** する Phase。設計 chapter 群 (01-08) は全件起案済 = design-phase 完了状態のため、本 Phase で初めて `indra/` 配下に触れる (= memory `feedback_design_phase_no_code_write` 解除点)。

### §3.2 sub-task 構成

| sub-task | source | 内容 | 実機操作 |
|---|---|---|---|
| 3.2.1 | 06a-prep §2 | (H1b) LL_INFOS hook 実装 + 計測 build + 3 scenario log 取得 | hook 配線 + build flag ON build + Linux cold launch × 3 scenario |
| 3.2.2 | 06a-prep §3 | (E') 同一 binding 複数 UBO 名疑い grep + preprocessor gate 確認 + attach program 確認 | grep + 直 read のみ (= build 不要) |
| 3.2.3 | 06a-prep §4 | (F) MaterialUBO vs MaterialUBO_Legacy 比較 = member diff + attach program 確認 | grep + 直 read のみ |
| 3.2.4 | 06a-prep §5 | log → uniform 名 × frame call count histogram 解析 + cadence band 判定 | log file 解析 (= awk / grep / sort) |
| 3.2.5 | 06a-prep §6 | chapter 05 / 06a / 06b / inventory 反映 + 持越項目消化 | doc update |
| 3.2.6 | 06a-prep §2.8 | 検証 hook 除去 + diff 0 件確認 + 通常 build pass 確認 | hook 行削除 + build 再実施 |

### §3.3 Exit Criteria

- 06a-prep §6 反映 flow の全行が「反映済」マーク
- (H1b) 不明 16 件の cadence 確定 (= primary cadence + secondary cadence 注記)
- (E') 5 UBO の binding 帰属 A/B/C いずれか確定
- (F) MaterialUBO 処遇 F1/F2/F3 いずれか確定
- `indra/` 配下に `AYASTORM_UBO_CADENCE_HOOK` / `AYA_UBO_HOOK` / `UBO_CADENCE` 残存 0 件
- 通常 build (= flag OFF) で build pass + cold launch normal

### §3.4 紐付け持越項目

- 07 §12 (RF) = reflection update fence throttle: Phase 0 で existing reflection update 呼出頻度を LL_DEBUGS log で取得 (= H1b hook と同 build に乗せる、log class `REFLECTION_THROTTLE`)、frame 毎呼出頻度を histogram 化 → throttle 必要性判定

### §3.5 Phase 0 で確定する後続 Phase の入力

- (Q1) 第 1 UBO 選定の候補 cadence 確定 (= per-frame 単一 UBO 候補が確定するため)
- chapter 06b update site 5 種設計可能 state 到達 (= 06a-prep §6 反映 flow 完了で 06b 起案可能)

#### §3.5.1 入力契約 pointer (= 第二次査読 §3.1 反映、(Q1)(Q2) 確定値の後続 Phase 流入先)

(Q1) / (Q2) AYA 判断で確定する値は、後続 Phase の以下 doc 箇所に流入する (= input contract):

| 確定値 | 流入先 doc | 流入先 §N | 流入時点 |
|---|---|---|---|
| (Q1) 第 1 UBO 識別子 (= 例: `UB_REFLECTION_PROBES`) | `chapter 04 §5.3` `UniformLocation` / `CadenceTag` enum | `04-codegen-ubo.md` §5.3 第 1 entry | Phase 1.A 入口 |
| (Q1) 第 1 UBO の cadence | `chapter 06b §4` flush 関数 5 種 | `06b-cadence-update-site-and-dirty.md` §4 該当 cadence セクション | Phase 1.C 入口 |
| (Q1) 第 1 UBO の descriptor set 帯 | `chapter 06c §3` 接合表 | `06c-descriptor-set-bind-wiring.md` §3 該当 set 帯 row | Phase 1.C 入口 |
| (Q2) Phase 当たり UBO 数 (= cluster 許可有無) | 本 chapter `§5.1` Phase 単位 scope | `09-phase-roadmap.md` §5.1 (1 UBO 例外規定) | Phase 2 入口 |
| (Q2) cluster 採用時の UBO 組合せ | `chapter 05 §6` MC1 確定表 + 本 chapter `§5.2` Template | `05-existing-inventory-link.md` §6 + 本 §5.2 | Phase 2 入口 |

**設計 phase 完了判定**: Phase 0 Exit + (Q1)(Q2) AYA 判断 揃った時点で上記 5 row の流入先 doc が **全行更新可能** state に到達 (= Phase 1.A 入口 readiness 完成)。それまで本 chapter §2/§3/§5/§6/§7/§8 の K placeholder 表記は維持 (= §2.1 行 78)。

---

## §4 Phase 1: codegen + redirect 層整備 (η-30)

### §4.1 sub-Phase 構成

| sub-Phase | scope | 該当 chapter | Exit 判定 |
|---|---|---|---|
| **1.A** | Codegen pipeline 実装 (= Python script 起草 + glslang 統合 + std140 calculator + SPIR-V reflection 二重保証 + perfect hash + cache + CMake DEPENDS) | 08 全章 | codegen script が既存 UBO blueprint (= 2026-06-06 audit 確認 = 96 unique UBO name / 94 codegen block、source-of-truth = `ubo_metadata.inl` `g_block_count = 94u`、旧 doc literal「85」は 2026-06-03 起案時 snapshot) を入力に取り、`ubo_metadata.inl` + `ubo_host_loader.inl` を生成、build error 0、生成 header の名前解決 lookup が compile-time 衝突 0 |
| **1.B** | redirect 層実装 (= 31 setter method 内部に Vulkan path 分岐 + name → offset 解決 dispatch + cache 構造 mUniformUBOLoc) | 06a §3 / §4 / §5 | 31 setter 全てで Vulkan path 分岐 working、OpenGL path 既存挙動 unchanged (= 1 setter call 1 path 決定論的、build flag で全 path 確認可能) |
| **1.C** | cadence 別 update site + dirty flag + descriptor set bind 配線 | 06b / 06c | 5 種 cadence (per-frame / per-program / per-draw / per-asset / per-skin) の update site / dirty flag / descriptor set bind が 1 経路ずつ実装、test UBO 1 個で full path 通電確認 |

### §4.2 Phase 1 Exit Criteria

- Phase 1.A: 既存 UBO blueprint (= 2026-06-06 audit 訂正 = 96 unique UBO name / 94 codegen block) に対する codegen 実行 PASS + 生成 header をテスト program (= 既存 program 1 個) で include + bind 不変動作確認
- Phase 1.B: 31 setter (= integer index 17 + LLStaticHashedString 14、PB-4.8+PB-5.14 統合で hashed 1 追加 + uniform2i(hashed) 含む、2026-06-06 audit 訂正) Vulkan path 分岐の **call site から見て transparent** = 既存 program 1 個の動作 unchanged
- Phase 1.C: test UBO 1 個 (= 後の Phase 2 で本実装する第 1 UBO の試作版、本実装は Phase 2、ここでは shell のみ) で per-cadence update + descriptor bind 通電

**Phase 1.C ↔ Phase 2 境界明示 (= 設計 review 2026-06-03 §3.4 boundary clarify)**:
- **Phase 1.C の「test UBO 1 個」 = Phase 2 で本実装する「第 1 UBO」と同一実体** (= (Q1) で AYA が選定する第 1 UBO の **shell 版**)
  - 1.C では shell = 空 struct + 空 dirty flag + 空 flush 実装 (= 経路通電のみ確認、データは zero memcpy)
  - Phase 2 で同 UBO の shell を **実 member + 実 dirty 判定 + 実 flush logic** に置換、call site (= setter) からの値書込開始 → 実描画反映
- **境界判定基準**: Phase 1.C Exit = test UBO shell の 5 cadence 全経路で `vkCmdBindDescriptorSets` が空 dummy buffer で成功 (= API 呼出層の通電確認、render 出力は OpenGL path のまま)。Phase 2 Entry = 同 UBO の実データ流入開始 + render 出力が Vulkan path に切替 (= mUseUBO flag 該当 program で ON)
- **shell vs 実装の差分**: shell は Phase 1.C で **書き捨て可能** = Phase 2 で全面書換しても 1.C Exit Criteria の遡及検証は不要 (= Phase 1.C は API 経路通電の証明、Phase 2 は data path の証明、独立に閉じる)
- = Phase 1.C と Phase 2 は **同一 UBO を実体に持つ連続 Phase** だが、判定軸 (API 通電 vs データ通電) が独立しているため Phase 番号を分離して管理

**shell ↔ 本実装 layout 互換性 (= 第二次査読 §3.2 反映)**:
- shell UBO の **descriptor set 帯 / binding 番号 / layout(set=N, binding=M) 宣言** は Phase 2 本実装と **完全一致** で生成 (= chapter 06c §3 接合表に従う)
- shell の **buffer size (= padded std140 size、chapter 08 §6.4 で 256B 倍数 padding)** は Phase 2 本実装と一致 (= dummy 0 fill の size を本実装と同 byte 数で確保、Phase 2 で全面書換しても VkDescriptorBufferInfo / VkBufferCreateInfo の引数差分ゼロ)
- shell の **PSO layout (= VkPipelineLayoutCreateInfo の descriptor set layout 列)** は本実装と互換 (= shell で生成した PSO は Phase 2 本実装の UBO bind 後も再生成不要、PSO cache (PSC) 経由で hit)
- 含意: Phase 1.C で確定する shell の binding / set / size / PSO layout は **Phase 2 で再利用される契約済構造**。shell の「書き捨て可能」 (上記) は **データ内容 (= struct member 定義 / dirty 判定 / flush logic)** に限定、layout 構造は不可触 (= 1.C で確定 → 2 で温存)

**注**: Phase 1 完了時点では **既存 program 動作 unchanged** (= Vulkan path 分岐 ON でも OpenGL path 経路を選ぶ default 動作)。Phase 2 で第 1 UBO migration を実施するまで実 Vulkan 描画は始まらない (= migration 前提整備完了が Exit)。

### §4.3 紐付け持越項目

- 08 §17 (A1) std140 offset Codegen 独自 calculator + SPIR-V reflection 二重保証 → Phase 1.A で実装
- 08 §17 (P) GLSL parse 独自 mini-parser + glslang -E 前処理 → Phase 1.A で実装
- 08 §17 (G/B3) perfect hash 独自 Python frozen-table → Phase 1.A で実装
- 08 §17 (B1) Codegen Python 3.8+ → Phase 1.A で実装
- 08 §17 (B2) glslang 統合 autobuild vendoring → Phase 1.A で実装
- 08 §17 (B4) 増分 build cache hash + mtime 併用 → Phase 1.A で実装
- 08 §17 (B5) CMake DEPENDS 自動 + 手動 `codegen_ubo_force` target 併設 → Phase 1.A で実装
- 07 §12 (W2) `sAssetUboPool` 起動時 prealloc N=64 + grow chunk 64 → Phase 1.C で実装
- 07 §12 (R1) ring buffer 起動時 4 MB / 上限 16 MB + cvar `AYARingBufferSizeMB` → Phase 1.C で実装
- 07 §12 (PSC) PSO cache `~/.ayastorm_x64/cache/pipeline_cache.bin`、上限 64 MB → Phase 1.C で実装 (= Vulkan pipeline 1 件作る時点で必要)

---

## §5 Phase 2..K: 1 UBO ずつ migration (η-31 以降)

### §5.1 Phase 単位の scope

各 Phase = 1 UBO migration を default (= memory `feedback_ubo_migration_one_at_a_time` 準拠)、ただし (Q2) AYA 判断で関連 cluster 許可なら 2-3 UBO 同 Phase 化を例外許可。

Phase 内手順 (= 1 UBO 当たり):

1. **入口確認**: Phase 0 計測結果から該当 UBO の cadence + 物理 instance 数 + 物理 owner を再確認
2. **codegen 実行**: 該当 UBO の `ubo_metadata.inl` セクションが既存 codegen 出力に含まれていることを確認
3. **redirect 層配線**: 該当 UBO の uniform 名群が perfect hash table に登録、setter 経由で UBO 内 offset に memcpy される経路を確認
4. **update site 配線**: cadence 別 update site (= per-frame / per-program / per-draw / per-asset / per-skin) に該当 UBO の dirty mark + flush を追加
5. **descriptor set bind**: 該当 UBO を含む program で `vkCmdBindDescriptorSets` 経路通電
6. **cold launch 検証**: AYA Linux build (= AYAstorm 通常 build flow、memory `project_build_procedure`) + 起動 + 該当 UBO を使う scene (= Phase 別に指定) で normal render 確認
7. **log 検証**: LL_DEBUGS log で該当 UBO bind 回数 / dirty flush 回数が cadence 想定範囲内 (= Phase 0 計測 histogram と整合)
8. **canary 検証**: 該当 UBO 経由でしか到達しない値 (= 例: color tint) を Phase 専用 cvar で flip、render 反映確認

### §5.2 Phase 順序 (= (Q1) で第 1 UBO 確定後の order)

**前提 (= Q23-K AYA 判断 (A) 反映、2026-06-03)**: 本 §5.2 の Template A/B/C 内の **「Phase 2」「Phase 3」「Phase 4」等の具体 phase 数値は (Q1)(Q2) 確定後の phase 振分 例示** (= §2.1 K 確定条件 = Phase 0 Exit + (Q1)(Q2) AYA 判断、行 73-78)。K 確定で具体数値は **置換される予定の placeholder 例示値**、Phase 1.A 入口で「Phase 2 開始」=「`UB_REFLECTION_PROBES` 開始」を確定形として読まないこと (= §2.1 行 78 placeholder 宣言と整合)。下記 phase 数値は **(Q1)(Q2) 確定後の order 提示用 sketch** であって、K 確定前は順序関係 (= Template A なら最小リスク UBO → 最頻出 per-draw UBO の順) のみが load-bearing。

(Q1) 第 1 UBO 選定方針 (= AYA 判断仰ぎ候補) 別の order template:

**Template A: 最小リスク UBO 優先**:
1. Phase 2 = singleton 系最小 UBO (= `UB_REFLECTION_PROBES` 単体、per-frame cadence、物理 instance 1 個) — **canary 検証 scene 候補**: water reflection on / sky reflection (= reflection map が描画に反映される景観、AYA 操作 = 海沿いランドマーク + sun position 変更で確認)
2. Phase 3 = per-Asset 系最小 UBO (= `UB_GLTF_MATERIALS` 単体) — **canary 検証 scene 候補**: GLTF material attach mesh (= GLTF 素材入り装着物または地形 prim、PBR material slot 差替で flip 確認)
3. Phase 4 = per-Asset 系大物 UBO (= `UB_GLTF_NODES`) — **canary 検証 scene 候補**: GLTF scene graph 持ち item (= GLTF imported scene mesh、node transform 変更で flip 確認)
4. Phase 5 = per-Skin 系 UBO (= `UB_GLTF_JOINTS`) — **canary 検証 scene 候補**: rigged GLTF avatar attachment (= bone animation 入り装着物、pose 変更で skinning 反映確認)
5. Phase 6.. = bare uniform 集約由来の per-program UBO 群 (= chapter 05 集約表で集約された UBO を頻度低い順) — **canary 検証 scene 候補**: 該当 program 必須 scene (= 例 `terrainF` 系なら地形 region、`avatarF` 系なら avatar mesh、`waterF` 系なら water surface)
6. Phase K = 最後に最頻出 per-draw 系 UBO (= 大 risk、最後に migration) — **canary 検証 scene 候補**: 高密度 draw 環境 (= Sandbox grid + 多数 prim + 複数 avatar、frame budget 内で 1k+ draw call 発火する景観)

**Template B: 最頻出 UBO 優先**:
- Phase 2 = 最頻出 per-draw UBO (= 大 risk 早期消化) — **canary 検証 scene 候補**: 高密度 draw 環境 (Template A Phase K と同)
- 以降は影響度大きい順で migration — **canary 検証 scene**: 該当 UBO の cadence 性質別 (= per-program なら該当 program scene、per-asset なら該当 asset 必須 scene)
- Phase K 終盤に singleton 系の少 risk UBO — **canary 検証 scene 候補**: water/sky reflection (Template A Phase 2 と同)

**Template C: cadence 系統別 batch**:
- Phase 2-3 = per-frame 系 (= 1-2 UBO) — **canary 検証 scene 候補**: water/sky reflection + frame counter 確認 scene
- Phase 4-5 = per-program 系 — **canary 検証 scene 候補**: 各 program 必須 scene を Phase 別に列挙 (terrain / avatar / water 等)
- Phase 6-7 = per-asset 系 — **canary 検証 scene 候補**: GLTF asset 入り region (Template A Phase 3-4 と同)
- Phase 8-9 = per-draw 系 — **canary 検証 scene 候補**: 高密度 draw 環境 (Template A Phase K と同)
- Phase 10 = per-skin 系 — **canary 検証 scene 候補**: rigged GLTF avatar (Template A Phase 5 と同)

**注 (= 第二次査読 §3.3 反映、canary 検証 scene annotation)**: 上記各 Phase の canary 検証 scene 候補は **AYA 動作確認の入力契約** (= Phase Exit 時に AYA が実際に visit するべき景観の候補)。Phase 起案時に handoff doc 内 §canary 検証 scene 節で確定 SLurl + scene 状態を明示 (= memory `feedback_render_bug_canary_protocol` 準拠)。

**default 提案 = Template A** (= 最小リスク UBO 優先): cold launch 検証で経路が成立しない場合の影響範囲が最小、Phase 1 で実装した codegen + redirect 層 + cadence 別 update site の各経路を **少 risk な UBO で 1 経路ずつ通電** することで、後続の大 risk UBO 移行時には残り経路差分のみが新規 path となる。

### §5.3 Phase Exit Criteria (= 各 Phase 共通)

- Linux cold launch normal (= crash 0 / shader compile error 0 / render normal)
- 該当 UBO を経路上に持つ既存 scene で **render parity** = OpenGL path と Vulkan path で screen diff が visible 差以下 (= AYA 目視確認)
- LL_DEBUGS log で該当 UBO bind / flush 回数が想定範囲内
- canary cvar flip で render 反映確認 (= 下記 §5.3.1 canary cvar 規約 inline)
- 検証用 LL_DEBUGS log は **commit 前に必ず除去** (= memory `feedback_remove_verification_logs`)
- handoff doc (= 1 Phase 1 件) 起案 + 次 Phase prerequisite 化

#### §5.3.1 canary cvar 規約 inline (= 設計 review 2026-06-03 §3.4 値表 inline 化)

**canary cvar 命名規約** (= 各 Phase 1 個 1 UBO 用):
- 命名 pattern: `AYAUboCanary_<UboName>` (= 例: `AYAUboCanary_ReflectionProbes` / `AYAUboCanary_GLTFMaterials` / `AYAUboCanary_LightParams` 等)
- type: `U32`
- 配置: `app_settings/settings.xml` に追加 (= Persist=0、再起動で消える、検証用一時 cvar)
- scope: Phase 内検証中のみ、Phase Exit 時に削除 (= 検証用 LL_DEBUGS log と同じ廃棄規律、`feedback_remove_verification_logs`)

**canary cvar 値表 (= 全 Phase 共通の意味付け)**:

| 値 | render 期待動作 | 検証目的 |
|---|---|---|
| `0` | normal render (= 該当 UBO 経路は実値、tint なし) | default 経路の生存確認 |
| `1` | 該当 UBO 経路の **diffuse** に **純赤** tint (= `vec3(1, 0, 0)` を該当 UBO の color slot に強制 inject) | shader 内で該当 UBO が **読まれている** ことを視覚確認 |
| `2` | 該当 UBO 経路の **diffuse** に **純緑** tint | 1 と区別、cadence ごとの bind 差分視覚確認 (= 同一 frame 内で 1→2 切替で再描画即反映なら per-frame/per-program、draw 完了後即反映なら per-draw) |
| `3` | 該当 UBO 経路で **bind 自体を skip** (= dummy buffer bind) | UBO bind が無いと描画 broken になることを反証で確認 (= 経路が **必須経路** であることの証明) |

**canary 設置位置**:
- inject point = redirect 層 `forwardToUboUpload` (= 06b §5.1) の cadence 別 routing 内に `if (gAYAUboCanary_<UboName> != 0) { override 該当 member; }` を仕込む
- 上書き対象 member = 該当 UBO の **最も視覚反映しやすい色 slot** (= diffuse_color / tint / ambient 等)、無い場合は alpha slot 等を選定
- 色値は **linear 色空間で直書きでなく `srgb_to_linear()` で逆引き** (= memory `feedback_shader_color_space_correction` 準拠、純赤 sRGB `(1,0,0)` → linear `(1,0,0)` で OK だが、tonemap 経由する path は要確認)

**canary 検証手順 (= AYA 実機)**:
1. 該当 Phase build → cold launch → 該当 UBO 経路を含む scene 表示
2. `AYAUboCanary_<UboName>` を 0→1 flip (debug settings 経由) → diffuse が純赤に変わることを目視確認 → screenshot
3. 1→2 flip → 純緑に変わることを目視確認 → screenshot
4. 2→3 flip → 描画 broken (= 黒画面 / 色化け) になることを目視確認 → screenshot
5. 3→0 flip → normal render に戻ることを目視確認 → Phase Exit
6. Phase Exit 後 cvar 削除 + `settings.xml` から該当行除去 + commit

### §5.4 Phase REJECT 処理

Phase 内 migration が cold launch crash / render 大破 / log 矛盾で REJECT された場合:

1. 該当 Phase を **REJECT 履歴として spec doc に保存** (= memory `feedback_build_only_verified` + `feedback_falsification_as_progress`)
2. 原因究明:
   - codegen 出力 inspect (= 該当 UBO の生成 header / std140 offset)
   - redirect 層 trace (= 該当 setter call が UBO 内 offset に到達しているか LL_INFOS で確認)
   - update site trace (= dirty flag set / flush 経路)
   - descriptor set bind trace (= 該当 program の bind 状態)
3. 原因が **Phase 1 整備に欠陥** → Phase 1.A/.B/.C のうち該当 sub-Phase を再実施 (= Phase 番号は据置、Phase 1.X-v2 として再起案)
4. 原因が **chapter 設計の欠陥** → chapter 04 / 06a / 06b / 06c のいずれかを update + Phase 0 計測再実施
5. 原因が **chapter 09 Phase 順序の欠陥** → (Q1) Phase 順序を再検討 + AYA 判断仰ぎ

### §5.5 紐付け持越項目

- 08 §17 (P-future) unused mask 参照解析 = 各 UBO migration 時に該当 UBO の uniform が全部使われているか実 program 単位で確認、不使用検出時は inventory §7 残課題に追記
- 08 §17 (cache-grow) cache GC = Phase 2 以降の codegen 増分 build cache が肥大化した時点で Phase 内 sub-task として cache GC (= 最終 build から N 日経過の生成物削除) を発動

---

## §6 Phase K+1 / K+2 / K+3: 3 OS 確証 (η-(K+2) / η-(K+3) / η-(K+4))

### §6.1 (Q4) Phase 順序方針

| 案 | 内容 | trade-off |
|---|---|---|
| **Linux first 順次** | K+1 (Linux) → K+2 (Windows) → K+3 (macOS) を順次、各 Phase の Exit が次の入口 | Linux REJECT 検出時に Win/Mac 着手前に修正可能、3 OS 差分が後段に積み上がりにくい、AYAstorm 3 platforms 原則 (`project_ayastorm_three_platforms`) と整合 |
| **3 OS 並走** | K+1/K+2/K+3 を Phase 番号上は別だが時系列で並走、AYA + @t-noami 等が同時 build | macOS 側問題が Linux 側 design に逆流できる、検証時間圧縮、ただし REJECT 切り分けが困難 (= どの OS で REJECT 起源か) |
| **Linux 完了後 Win/Mac 並走** | K+1 (Linux) 単独 → K+2/K+3 並走 | Linux を baseline 確定してから Win/Mac で diff 検出、AYA Mac 不所持制約 (`feedback_mac_only_fixes_accept_as_is`) と整合 |

**default 提案 = Linux 完了後 Win/Mac 並走**: Linux は Claude 検証可能 OS (= memory `feedback_log_reading` / `feedback_proactive_diagnostic`)、Win は AYA、Mac は @t-noami 委任 (= memory `feedback_mac_only_fixes_accept_as_is`)。Linux baseline 確定 → Win/Mac 並走で Win/Mac 固有 diff のみ後段消化、Mac は AYA 検証不可のため @t-noami 信任前提 (= memory `feedback_mac_only_fixes_accept_as_is`)。

### §6.2 各 OS Phase Exit Criteria

#### §6.2.1 Phase K+1 (Linux)

- Linux build pass (= AYAstorm 通常 build flow)
- cold launch normal × 3 種 scenario (= 06a-prep §2.6 と同 scenario、再利用)
- 全 migration 済 UBO の render parity (= OpenGL build と Vulkan build の screen diff visible 以下)
- LL_DEBUGS log で **全 UBO 系統の bind / flush** が cadence 想定範囲内
- 3 種 scenario 各 30 frame × 3 反復で stable (= cold launch 3 回連続 normal)

#### §6.2.2 Phase K+2 (Windows)

- Windows build pass (= AYAstorm 通常 build flow on Windows)
- cold launch normal × 3 種 scenario (= AYA 実機)
- render parity 確認 (= AYA 目視)
- Windows 固有 diff (= driver / DLL load / file path 差) があれば該当を fix + Phase 内 sub-task 化
- Mac 並走中の場合、Mac 側で同時に REJECT 検出されたら Phase K+2 Exit 後に Phase K+3 で fix

#### §6.2.3 Phase K+3 (macOS)

- macOS build pass (= AYAstorm 通常 build flow on macOS、@t-noami 委任)
- cold launch normal × 3 種 scenario (= @t-noami 実機)
- render parity 確認 (= @t-noami 目視、AYA は Mac 不所持のため信任)
- macOS 固有 diff (= MoltenVK 経路 / Metal layer 差) があれば @t-noami 報告 + Phase 内 sub-task 化
- @t-noami credit (= memory `feedback_credit_t_noami_equal_billing`) は release note で対等並列扱い

### §6.3 紐付け持越項目

- 08 §13.4 X-α/β/γ = Phase K+1/K+2/K+3 の sub-task 構成として完全反映

---

## §7 Phase K+4: OpenGL path 撤廃 (η-(K+5))

### §7.1 (Q3) 撤廃時期方針

| 案 | 内容 | trade-off |
|---|---|---|
| **全 UBO 移行完了まで並走** | Phase 2..K 全 Phase 中 OpenGL path 並走、K+4 で初撤廃 | 各 Phase で OpenGL fallback 可能、REJECT 時の比較 baseline 確保、ただし Phase K+4 の撤廃量が膨大 |
| **中間 Phase で OpenGL path 撤廃** | Phase X (= K/2 程度) で OpenGL path 撤廃、以降は Vulkan only | OpenGL fallback 不可、REJECT 時の比較 baseline 喪失、撤廃量分散、ただし Vulkan only 確証が後段に積み上がる |
| **段階撤廃** | UBO migration 完了する毎に該当 UBO 系統の OpenGL path を撤廃、K+4 で残り総撤廃 | 並走 cost と撤廃 cost のバランス、ただし dirty path 構造が Phase 毎に変動 |

**default 提案 = 全 UBO 移行完了まで並走**: 各 Phase で REJECT 検出時の baseline (= OpenGL path 動作) が常に確保される。memory `feedback_build_only_verified` の「効果未確認の commit を積まない」原則に整合 (= Vulkan 経路の確証が全 UBO 揃うまで暫定的)、Phase K+4 の撤廃量は大きいが、撤廃前時点で全 UBO Vulkan 経路 PASS が確証されているため作業は機械的削除。

### §7.2 Phase K+4 sub-task

1. 全 setter の OpenGL path 分岐 (= Phase 1.B で配線した if 分岐の OpenGL 側) 削除
2. OpenGL 専用 helper (= `glUniform*` / `glBindBuffer` 直呼び等) 削除
3. OpenGL UBO 物理 buffer 生成 / upload code 削除 (= `glGenBuffers` / `glBufferSubData` 呼出)
4. CMake build flag (= OpenGL path 有効化 flag があれば削除)
5. 3 OS 再 build + Phase K+1/K+2/K+3 と同 scenario 再実行 (= 撤廃後の regression 検証)

### §7.3 Phase K+4 Exit Criteria

- OpenGL 関連 symbol (= `glUniform` / `glBindBuffer` / `glGenBuffers` 等) 完全削除確認 (= grep 0 件、ただし bare GL state 系 = `glClear` 等の framework 層は除外、本撤廃 scope は UBO 関連のみ)
- 3 OS build pass
- 3 OS cold launch normal + render parity (= Phase K+1/K+2/K+3 と同 scenario)
- handoff doc で撤廃 file list + 削除 line 数の archive

---

## §8 Phase K+5: release 整備 (η-(K+6))

### §8.1 sub-task

1. **release note 起草** (= memory `feedback_release_notes_link_only` + `feedback_release_note_per_feature` 準拠):
   - r41 Vulkan migration の 1 release note 1 feature 形式
   - 詳細資料 link = `docs/specs/ayastorm-r41-gl-removal/` 配下 chapter 群
   - 差分ハイライト = OpenGL → Vulkan 移行の代表的視覚 / 性能差
2. **credit 整備** (= memory `feedback_credit_t_noami_equal_billing`):
   - @mayatonton + @t-noami 対等並列、Mac 担当注記は従属節化しない
   - r41 期間中の貢献 (= Mac build + r24-r28 本実装継続) を独立 bullet
3. **tag 切り出し** (= memory `feedback_release_flow`):
   - Claude は commit まで、push 以降は AYA 手動
   - tag bundle suffix (= memory `project_tag_bundle_suffix_rule`) = upstream bundle 時のみ `+bundle-fs.N`、本 r41 単独 release では付けない
4. **release branch flow** (= memory `feedback_release_branch_workflow`):
   - 必ず feature branch から cherry-pick / merge、release branch 直接 commit 禁止
   - 複数 commit branch は merge 第一候補 (= memory `feedback_multi_commit_branch_merge_first`)
5. **AYAstream 整合** (= memory `project_ayastream_roadmap`):
   - r41 完了時点で AYAstream M5+ のどこまで実装済か release note にも触れる (= 独立 feature note として並列)、ただし本 r41 release note 内には混入させない

### §8.2 Phase K+5 Exit Criteria

- release note draft 完成 + AYA review PASS
- tag commit 起案 + AYA tag/push 委任
- release note を GitHub Release ページ本文に並べ替え可能な link 集形式

---

## §9 Phase 完了判定基準まとめ (= 全 Phase 共通)

各 Phase の Exit Criteria は §3-§8 で個別記述したが、**全 Phase 共通の最終確認** として以下を Phase ごとに必ず通す:

| # | 確認項目 | 根拠 memory / chapter |
|---|---|---|
| 1 | cold launch crash 0 | `feedback_build_only_verified` |
| 2 | LL_DEBUGS / LL_INFOS 検証 log を commit 前に除去 | `feedback_remove_verification_logs` |
| 3 | 関連 cvar (= Persist=1) は次回起動でも残るため、検証完了時に「戻す値表」を提示 | `feedback_restore_debug_settings` |
| 4 | tests/ 配下に触れない、確認しない、commit しない | `feedback_tests_dir_never_commit` |
| 5 | 1 Phase 1 handoff doc 起案 (= 次 Phase prerequisite 化) | `feedback_proactive_handoff` |
| 6 | Phase 完了時 git log で commit message + commit hash 残し | (`project_ayastorm_three_platforms` + `feedback_release_flow`) |
| 7 | Phase 内検証は **AYA 動作確認前** に Claude self-trace 完了 | `feedback_self_verify_before_handoff` |
| 8 | AYA が build run する量は Phase 当たり最小化 (= 検証 reproducible scenario を doc 化) | `feedback_one_step_at_a_time` |
| 9 | Phase REJECT 時は仮説 2 連続外れたら推論止めて log / canary / bisect で実データ取得 | `feedback_admit_unknown` + `feedback_doubt_self_first` |
| 10 | 描画系は推論禁止、完全 trace 優先 | `feedback_render_full_trace_first` |

---

## §10 持越項目総覧 (= 07 §12 + 08 §17 + 本 chapter 内 Phase 紐付け)

### §10.1 chapter 07 §12 由来

| 持越 | 解消 Phase | 解消方法 |
|---|---|---|
| (V1') set=1 split | **chapter 10** | AYA 判断仰ぎ (= chapter 09 内では決定しない、chapter 10 持越) |
| (V3') 共通 layout vs program 別 | **chapter 10** | AYA 判断仰ぎ |
| (S3') sampler 49 配置 | **chapter 10** | AYA 判断仰ぎ |
| (W) `sProgramUboPool` maxSets | **chapter 10** | AYA 判断仰ぎ |
| (W2) `sAssetUboPool` prealloc N=64 + grow chunk 64 | **Phase 1.C** | default 確定済 (07 §12)、Phase 1.C 実装 |
| (R1 chapter 07 由来) ring buffer 4 MB / 16 MB + cvar | **Phase 1.C** | default 確定済 (07 §12)、Phase 1.C 実装 |
| (PSC) PSO cache `~/.ayastorm_x64/cache/pipeline_cache.bin`、64 MB | **Phase 1.C** | default 確定済 (07 §12)、Phase 1.C 実装 |
| (RF) reflection update fence throttle | **Phase 0** | (H1b) hook と同 build で reflection update 頻度 log 取得 → throttle 判定 |

### §10.2 chapter 08 §17 由来

| 持越 | 解消 Phase | 解消方法 |
|---|---|---|
| (A1) std140 offset 二重保証 | **Phase 1.A** | Codegen Python 実装、SPIR-V reflection と独自 calculator 結果照合 |
| (P) GLSL parse 独自 mini-parser + glslang -E | **Phase 1.A** | Codegen Python 実装 |
| (G/B3) perfect hash 独自 Python frozen-table | **Phase 1.A** | Codegen Python 実装 (= CHD/FCH algorithm) |
| (B1) Python 3.8+ | **Phase 1.A** | shebang + autobuild 確認 |
| (B2) glslang autobuild vendoring | **Phase 1.A** | autobuild 既存 entry に追加 |
| (B4) 増分 build cache hash + mtime | **Phase 1.A** | Codegen Python 実装 |
| (B5) CMake DEPENDS + `codegen_ubo_force` | **Phase 1.A** | CMake patch |
| (P-future) unused mask 参照解析 | **Phase 2..K (per UBO)** | 各 UBO migration 時に該当 UBO の uniform 使用状況を inventory §7 へ記録、不使用検出時は削除候補化 |
| (cache-grow) cache GC | **Phase 2..K (発動時)** | 増分 build cache 肥大化時に Phase 内 sub-task として発動、最終 build から N 日経過の生成物削除 |

---

## §11 AYA 判断仰ぎ候補 (Q1)-(Q5)

本 chapter 内で **default 提案** を立てたが、roadmap 確定前に AYA 判断を仰ぐ 5 項目。

### §11.1 (Q1) 第 1 UBO migration 選定方針

| 選択肢 | template | 内容 |
|---|---|---|
| A | 最小リスク UBO 優先 | singleton 系 (`UB_REFLECTION_PROBES` 単体) から開始、最後に最頻出 per-draw |
| B | 最頻出 UBO 優先 | 最頻出 per-draw から開始、最後に少 risk singleton |
| C | cadence 系統別 batch | per-frame → per-program → per-asset → per-draw → per-skin 順に system 別 batch |

**default 提案 = A (最小リスク優先)**: 既述 §5.2 default 提案 と同 (= 経路 1 個ずつ通電で REJECT 影響範囲最小)。
**Phase 0 結果次第**: 「per-frame 単一 UBO 候補が確定するため、(H1b) 計測完了後に最終確定」(§3.5 参照)。
**AYA 判断ポイント**: A の保守性を取るか、B の大 risk 早期消化を取るか、C の同一 system batch 効率を取るか。

**✅ 2026-06-03 ST-5 batch verdict = A 確定** (= chapter 10 §1.3 「全 default 採用」AYA 応答)。具体 UBO 順 (= 第 1 UBO 〜 第 K UBO) は Phase 0 計測完了済を受けて chapter 06b 起案中に「最小リスク順」原則で決定 = `UB_REFLECTION_PROBES` 単体から開始、最後に最頻出 per-draw (= 06a-prep §5.5 観察 = matrix 系 cpf 400-700 per-draw 群が最後)。K 値は具体順確定後に算出、本 §2.1 Phase 全体マップに反映予定 (= chapter 06b 起案中 task)。

### §11.2 (Q2) Phase 当たり migration UBO 数

| 選択肢 | 内容 | 根拠 |
|---|---|---|
| A | 1 UBO 厳守 | memory `feedback_ubo_migration_one_at_a_time` 厳格 |
| B | 関連 cluster で 2-3 UBO 同 Phase 許可 | 同一 owner / 同一 cadence の cluster は 1 Phase で扱う |

**default 提案 = A (1 UBO 厳守)**: feedback memory 直接準拠、cluster 化は cold launch 検証で REJECT 時の切り分け困難化リスク。
**例外**: (Q1) で同一 owner UBO 群 (= `UB_GLTF_NODES` / `UB_GLTF_MATERIALS` / `UB_GLTF_JOINTS` は全て `gltf::Asset` / `gltf::Skin` owner) を識別、cluster 単位を AYA 判断で例外許可するかは Phase 0 完了後の (Q1) 確定タイミングで再評価。

**✅ 2026-06-03 ST-5 batch verdict = A 確定** (= chapter 10 §1.3 「全 default 採用」AYA 応答、memory `feedback_ubo_migration_one_at_a_time` 直接準拠)。`UB_GLTF_NODES` / `UB_GLTF_MATERIALS` / `UB_GLTF_JOINTS` cluster 例外も「1 UBO 厳守」原則優先で **非適用**、各 UBO は別 Phase で個別 cold launch 検証する。

### §11.3 (Q3) OpenGL path 維持期間

| 選択肢 | 内容 | trade-off |
|---|---|---|
| A | 全 UBO 移行完了まで並走 (= Phase K+4 で初撤廃) | REJECT 時 baseline 確保、撤廃量大、§7.1 default |
| B | 中間 Phase で OpenGL path 撤廃 | baseline 喪失、撤廃量分散 |
| C | 段階撤廃 (= UBO 完了毎に該当系統撤廃) | 並走 cost と撤廃 cost 折衷 |

**default 提案 = A (並走)**: §7.1 default、memory `feedback_build_only_verified` と整合。
**AYA 判断ポイント**: A の安全性を取るか、B/C で並走 cost 削減を取るか。

**✅ 2026-06-03 ST-7 batch verdict = A 確定** (= chapter 10 §1.3 「推奨で」AYA 応答 = ST-5 batch default 採用継承)。Phase K+4 (= 全 UBO 移行完了 Phase) で初めて OpenGL path 撤廃、それまでは GL ↔ Vulkan dual-path 並走で REJECT 時 baseline 確保 (= memory `feedback_build_only_verified` 整合)。B/C 案は Phase K+3 進行中に再評価可 (= 後ろ倒し option 保持)。本 §11.3 default → 確定形書換完了、§7.1 / §2.1 Phase 全体マップへの確定反映は cluster `feedback_design_phase_no_code_write` 解除後の Phase 1.A 中盤で実施 (= dual-path 並走運用が安定動作確認後)。

### §11.4 (Q4) 3 OS 確証 Phase 順序

| 選択肢 | 内容 | trade-off |
|---|---|---|
| A | Linux first 順次 (K+1 → K+2 → K+3) | 順次切り分け容易、時系列長 |
| B | 3 OS 並走 (時系列同時) | 検証時間圧縮、REJECT 切り分け困難 |
| C | Linux 完了後 Win/Mac 並走 | baseline 確定 + Mac 委任両立、§6.1 default |

**default 提案 = C (Linux 完了後 Win/Mac 並走)**: §6.1 default、Mac 不所持制約 (`feedback_mac_only_fixes_accept_as_is`) と整合。

**✅ 2026-06-03 ST-5 batch verdict = C 確定** (= chapter 10 §1.3 「全 default 採用」AYA 応答)。Phase K+1 (Linux baseline 確定) 完了後に Phase K+2 (Windows) と Phase K+3 (macOS) を並走、Mac 担当は @t-noami (= memory `feedback_release_flow` の AYAstorm release flow 整合)。Phase 順序図 §2.2.1 案別を C 採用に確定。

### §11.5 (Q5) Phase 0 計測 phase の Phase 番号化

| 選択肢 | 内容 |
|---|---|
| A | Phase 0 を独立 Phase (η-29) として明示 | 本 chapter §3 default 案 |
| B | Phase 1 入口 sub-task 扱い (= Phase 0 を Phase 1.0 に格納) | Phase 1 開始時に同 session で連続実施 |
| C | Phase 1 並行 task (= Phase 1.A 実装中に Phase 0 計測を並走) | 時系列重複、ただし計測結果が Phase 1.B 入力に必要 |

**default 提案 = A (独立 Phase η-29)**: 06a-prep §0.2 の design-phase / implementation-phase 分離原則と整合、Phase 0 が完了しない限り Phase 1.B/.C 設計入力不足 (= 06a-prep §0.3 / §6 反映 flow)、独立 Phase 化が最も判定明瞭。

**✅ 2026-06-03 ST-7 batch verdict = A 確定** (= chapter 10 §1.3 「推奨で」AYA 応答 = 既物理確定の形式 ✅ 化)。sub-step 命名 (= `4.3-γ'-port-β-2-bundle-B-B?-η-29`) で η-29 が既 active = 物理現実が既に A 採用済 + 本 chapter §14.4 で「default 確定 = 独立 Phase η-29 = 既反映済」と既記載 = 形式判断のみ。B (Phase 1.0 格納) / C (Phase 1 並走) は sub-step 命名 retrofit cost + 時系列矛盾 (= Phase 0 計測結果が Phase 1.B 入力必須) で技術的に成立せず、A 採用が唯一の物理整合解。

### §11.6 (Q-NTTP) R1 compile-time literal path 採否 (= C++ standard NTTP 採否判定)

chapter 04 §6.4.7 「C++20 NTTP 採否 (= chapter 09 持越判定材料)」由来。R1 path (= `template<auto NameLiteral>` compile-time literal 経路) の採否は C++20 NTTP (Non-Type Template Parameter) 機構に依存、AYAstorm 既存 build standard C++17 default との trade-off で AYA 判断仰ぎ。

| 選択肢 | 内容 |
|---|---|
| A | **R1 不採用** (= R3 name-based dispatch のみ、C++17 維持) | chapter 04 §6.4.7 default、R3 で十分 |
| B | R1 採用 + 全 module C++20 切替 (= compile-time literal 経路で 1 indirection 削減) | 3 OS toolchain 確認 + dependent module re-validation cost 発生 |
| C | R1 部分採用 (= 特定 hot path のみ C++20 conditional include) | 機構複雑度増 + 効果 limited (= R1 hit 範囲 hot path 限定) |

**確定 = A (R1 不採用、C++17 維持)** (= 2026-06-03 ST-7 sub-task 8 batch AYA「A」応答 = default 採用継続): chapter 04 §6.4.7 既述根拠 = R3 name-based dispatch + perfect hash (CHD) + frozen-table 経路で十分高速、R1 効果差 (= compile-time vs runtime 1 indirection) は限定的、C++20 切替 cost (= 3 OS toolchain 確認 + dependent module re-validation + autobuild manifest 変更) との trade-off で A 採用が妥当。R1 は **task 完了後の polish 候補** (= Phase K+4 以降の optimization phase 候補) として保留可。Phase 1.A handoff doc §3 PA-0 (= C++20 切替 task) は不要、PA-1 から開始可。

**確定タイミング**: 2026-06-03 ST-7 sub-task 8 batch で AYA 判断本体確定 (= §14.5 row 3-14 ✅ 達成 = Stage 3 14/14 ✅ 全完走 = `feedback_design_phase_no_code_write` 完全解禁 = Phase 1.A 実装 entry へ移行)。chapter 10 §1.0 row 29 状態 column ✅ + §1.3 表 (Q-NTTP) 行 verdict マーク + §1.3 末尾 ST-7 sub-task 8 batch verdict paragraph で連動反映済。Phase 1.A 完了後の R3 動作確認結果次第で B/C 案再評価可 (= polish 候補保留 option)。

---

## §12 chapter 10 (open-questions) への送り出し項目

本 chapter で扱わない / decision pending な項目:

1. **chapter 07 §12 (V1')(V3')(S3')(W)** = chapter 07 で chapter 10 送りと明示済、本 chapter で再判定しない
2. **(Q1)-(Q5)** = AYA 判断仰ぎ候補、AYA 確認後に本 chapter §2.1 / §5.2 / §6.1 / §7.1 を default → 確定形に書き換え
3. **K の確定値** = (Q1)(Q2) 確定後に決まる migration UBO 個数、現時点では「5-20 程度」とのみ
4. **per-Phase 担当者** = AYA / @t-noami 役割分担は AYAstorm release flow (memory `feedback_release_flow`) で AYA 主導が原則、Mac 委任は @t-noami、Claude は commit まで
5. **(Q1) Template 確定後の具体 UBO 順** = Phase 0 計測結果待ち、本 chapter では template 提示のみ

---

## §13 起案規律 (= 本 chapter 内自己管理)

- 本 chapter は **Phase scope + Exit Criteria + 紐付け + AYA 判断候補** に絞り、Phase 内コード詳細は実装 phase 入口 handoff doc に委ねる
- (Q1)-(Q5) 確定後は本 chapter §2.1 表 + §5.2 template + §6.1 / §7.1 default 提案を **確定形に書き換え** (= live doc として update)
- chapter 07 §12 chapter 10 持越項目 (= (V1')(V3')(S3')(W)) を本 chapter 内で先行決定しない (§12 item 1)
- 各 Phase の handoff doc は **Phase 入口で起案**、本 chapter §3-§8 を pre-requisite として参照可能 state を維持
- 本 chapter の update は AYA 判断確定 / Phase 完了で都度反映 (= live doc)

---

## §14 Phase 1.A 入口 1 step state checklist (= Phase 2d-β-revise Deliverable C)

**起案目的**: Phase 2d-β-revise 完了 → η-29 Phase 0 (計測 phase) → Phase 1.A (codegen pipeline 実装) 入口に至るまでの 1 step state checklist。「次に何があれば Phase 1.A に入れるか」を 1 ページで明示。本 §14 は Phase 2d-β-revise Deliverable §2.1/§2.2 の **深化結果が Phase 1.A 入口 readiness に到達したか self-check** リスト。

### §14.1 readiness 4 階層 + 各階層 entry condition

```
[Stage 0: design-phase 完了 readiness] (= 本 §14 起案時点で確認)
    ↓
[Stage 1: η-29 Phase 0 実機計測 readiness]
    ↓
[Stage 2: AYA 判断 (Q1)-(Q5) 確定]
    ↓
[Stage 3: Phase 1.A 入口 readiness (= 着手 ready state)]
```

各 Stage の entry condition と本 §14 self-check 項目を分離して列挙。

### §14.2 Stage 0: design-phase 完了 readiness (= 本 chapter 起案時点)

**self-check 項目** (= Phase 2d-β-revise commit 直前で全件 ✅ 確認、未充足検出時は本 §14 で項目別に対応 doc 修正):

| # | 項目 | 確認方法 | 本 chapter / 関連 doc |
|---|---|---|---|
| 0-1 | chapter 04 Codegen-UBO 機構確定 (= §1-§11 全節起案 + Deliverable A-1/A-2/A-3 深化反映) | `wc -l docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` で line 数増加確認 + §6.4 / §4.3.1 / §5.6 section 存在 | 04-codegen-ubo.md |
| 0-2 | chapter 08 build pipeline 機構確定 (= §1-§17 全節起案 + Deliverable B-1/B-2/B-3/B-4/B-5 深化反映) | 同上 + §5.4.1 / §5.2.1 / §11.5 / §12.5 / §13.5 section 存在 | 08-build-codegen-pipeline.md |
| 0-3 | chapter 09 Phase Roadmap 確定 (= §1-§14 + (Q1)-(Q5) AYA 判断仰ぎ候補登録) | `grep '^## §' 09-phase-roadmap.md` で §0-§14 全件存在 | 09-phase-roadmap.md |
| 0-4 | chapter 06a-prep Phase 0 計測 spec 確定 (= §2 (H1b) hook + §3 (E') + §4 (F) + §5 結果反映 flow) | grep '^## §' 06a-prep + §2-§6 全節存在 | 06a-prep-phase0-measurement.md |
| 0-5 | chapter 02 §2.4 naming convention 確定 (= 生成識別子規則 + chapter 04 §5.2 `<BlockName>Layout` 整合) | grep '^### §2.4' 02-naming-convention.md | 02-naming-convention.md |
| 0-6 | chapter 05 集約対応表 (= bare uniform → UBO 集約) 起案 | grep '^## §' 05-existing-inventory-link.md で §1-§6 全件存在 + MC1 rename 反映 | 05-existing-inventory-link.md |
| 0-7 | chapter 06b cadence 別 update site + chapter 06c descriptor set bind 配線 設計起案 | grep '^## §' 06b / 06c 全節存在 | 06b / 06c |
| 0-8 | chapter 07 Vulkan API state (= device limit / set 帯 5 化 / V1' split / 256B alignment) 確定 | grep '^## §' 07-vulkan-api-state.md で §0-§12 全節存在 | 07-vulkan-api-state.md |
| 0-9 | chapter 10 open questions 集約 (= (V1')(V3')(S3')(W) + (Q1)-(Q5) + (NTTP) + (A1)(P)(G/B3)(B1)(B2)(B4)(B5) 等の登録) **✅ 遡及 retroactive 充足 2026-06-03 ST-7 sub-task 4** (= 起案時点 Phase 2d-β-revise で (NTTP) 登録 intent のみ chapter 04 §6.4.7 文末に記述あり、実 entry は chapter 09 §11 / chapter 10 §1.0 / §1.3 未追加 = 起案時 ✅ verdict は (NTTP) 漏れ含む状態、`feedback_doubt_self_first` 適用で §14.5 row 3-10 verify 中に gap 検出、本 sub-task 4 batch で chapter 09 §11.6 (Q-NTTP) 新設 + chapter 10 §1.0 row 29 + §1.3 表 (Q-NTTP) 行追加 = 遡及 retroactive 充足、Stage 0 verdict 「全 12 項目 ✅」は不変 = 本 retro 充足で実体化) | grep '^### §1\\.' 10-open-questions.md で 29 件全件 index + (Q-NTTP) 実 entry 確認 | 10-open-questions.md |
| 0-10 | chapter 01 overview の 2 大設計原則 + 確定事項 13 件 反映済 | grep '§5' 01-overview.md で確定事項 13 件 enumerate | 01-overview.md |
| 0-11 | inventory §3.3.1 85 UBO blueprint table 確定 | grep '§3.3.1' inventory.md | inventory.md |
| 0-12 | `feedback_design_phase_no_code_write` 継続 = `indra/` 配下改変ゼロ | `git status indra/` で modified 0 件 | (memory) |

**Stage 0 entry verdict**: 全 12 項目 ✅ → Stage 0 完了、Stage 1 入口 (= η-29 Phase 0 着手 ready)。

### §14.3 Stage 1: η-29 Phase 0 実機計測 readiness

**Stage 0 完了後の next-step entry condition**:

| # | 項目 | 確認方法 | 担当 |
|---|---|---|---|
| 1-1 | AYAstorm 既存 build flow 動作確認 (= `project_build_procedure` memory に従って Linux native build PASS) | AYA build 1 回 | AYA |
| 1-2 | LL_INFOS hook (= 06a-prep §2 (H1b)) 配線 + flag-gated build (= `AYASTORM_UBO_CADENCE_HOOK`) 動作確認 | Claude が hook 行追記 (= Phase 0 で初解禁、design-phase 制約解除) | Claude (= η-29 Phase 0 で実施) |
| 1-3 | 3 scenario (= 06a-prep §2.6 既定 scenario) で cold launch + log 取得 | AYA Linux 起動 × 3 | AYA |
| 1-4 | log 解析 (= awk / grep / sort) で uniform 名 × frame call count histogram | Claude が log file 直接読込 + 解析 | Claude (= `feedback_log_reading`) |
| 1-5 | (E') 同 binding 複数 UBO 名疑い 5 件 grep 確認 | Claude が grep 実施 | Claude |
| 1-6 | (F) MaterialUBO vs MaterialUBO_Legacy member diff 確認 | Claude が grep + read 実施 | Claude |
| 1-7 | (H1b) 不明 16 件の cadence 確定 + (E') 5 件 binding 帰属確定 + (F) MaterialUBO 処遇確定 | 計測結果 → 06a-prep §6 反映 flow へ | Claude (= doc update) |
| 1-8 | 検証 hook 除去 + 通常 build (= flag OFF) pass + diff 0 件確認 | Claude が hook 行削除 + AYA build 確認 | Claude + AYA |
| 1-9 | (RF) reflection update fence throttle 頻度 log 取得 | (1-2) と同 build に乗せる | Claude + AYA |

**Stage 1 entry verdict**: Stage 0 完了 + Phase 0 計測着手 condition ✅ → Stage 1 入口完了、Stage 2 (= (Q1)-(Q5) AYA 判断) ready。

### §14.4 Stage 2: AYA 判断 (Q1)-(Q5) 確定

**Stage 1 (= Phase 0 計測完了) 後の AYA 判断 5 件**:

| # | (Q) | 判断内容 | default 提案 | 確定タイミング |
|---|---|---|---|---|
| 2-1 | (Q1) | 第 1 UBO migration 選定方針 (= Template A/B/C) | A 最小リスク UBO 優先 | Phase 0 完了直後 |
| 2-2 | (Q2) | Phase 当たり migration UBO 数 (= 1 UBO 厳守 vs cluster 許可) | A 1 UBO 厳守 | (Q1) と同時 |
| 2-3 | (Q3) | OpenGL path 維持期間 (= 全 UBO 完了まで並走 vs 中間撤廃 vs 段階撤廃) | A 全 UBO 完了まで並走 | Phase 1.A 入口でも可、後ろ倒し許容 |
| 2-4 | (Q4) | 3 OS 確証 Phase 順序 (= Linux first 順次 vs 並走 vs Linux 完了後 Win/Mac 並走) | C Linux 完了後 Win/Mac 並走 | Phase 0 完了直後 |
| 2-5 | (Q5) | Phase 0 計測 phase の Phase 番号化 (= 独立 Phase η-29 vs Phase 1 入口 sub-task) | A 独立 Phase η-29 | 本 §14 時点で **default 確定 = 独立 Phase η-29 = 既反映済** |

**Stage 2 entry condition**:
- (Q1) / (Q2) 必須 (= K 値確定 + Phase 2 entry 確定の前提)
- (Q4) 必須 (= Phase K+1/+2/+3 順序 確定で Phase 1.A 設計影響なし、ただし Phase K+1 入口で必要)
- (Q3) / (Q5) は Phase 1.A 入口時点で default 採用継続可、AYA 判断は Phase 1.A 中盤までに後追い可

**Stage 2 entry verdict**: (Q1) / (Q2) 確定 (= K 値計算可能 state 到達) ✅ → Stage 2 完了、Stage 3 入口 (= Phase 1.A 着手 ready)。

**✅ 2026-06-03 ST-5 batch Stage 2 完了 = (Q1) A / (Q2) A / (Q4) C 確定** (= chapter 10 §1.3 「全 default 採用」AYA 応答)。(Q3)(Q5) は default 採用継続、Phase 1.A 中盤まで後ろ倒し可 (= handoff §3.5 規律 7)。本 §14.4 表で 5 件中 3 件 ✅ 完了 = (Q1)(Q2) 必須条件達成 = K 値計算可能 state 到達 = **Stage 3 入口 entry 達成**。

**✅ 2026-06-03 ST-6 chapter 06b/06c 既起案済 verify + delta integration 完了 = §14.5 3-8 ✅** (= B 案採用、既存 `06b-cadence-update-site-and-dirty.md` 441 行 §2.1-§2.5 5 cadence + `06c-descriptor-set-bind-wiring.md` 513 行 §2/§4 descriptor set bind 配線 で要件物理充足、ST-6 前段 (a)(b) findings = R-AYA1/2 dead / R-AYA3 alive 既移植済 + R-MAT4 `normal_matrix` 確定 + Q26-MUL `MaterialUBO_Class3_Legacy` 確定 + Q27-CONFL B2 binding ずらし + Q1/Q2/Q4 確定を 06b §2.2/§2.3/§3.3/§8 に delta integration 済、handoff §3.2「起案契約」物理充足 + delta 整合保証)。Stage 3 self-check 14 項目 (= §14.5) のうち **3-3 + 3-8 = 2 件 ✅ 完了 / 残 12 項目**。残 12 項目のうち 3-7 (chapter 06a 設計起案済) は 06a-cache-structure-and-setter-redirect.md 既存で物理充足、3-9-3-14 等は 2026-06-03 段階で起案済 / Phase 1.A 入口で実施。

**✅ 2026-06-03 ST-7 batch Stage 2 完全達成 = (Q3) A / (Q5) A 確定** (= chapter 10 §1.3 「推奨で」AYA 応答 = ST-5 batch 「全 default 採用」継承)。本 §14.4 表 **5 件中 5 件 ✅ 完了** = Stage 2 (Q1)-(Q5) 全件確定 = K 値計算可能 + Phase 順序確定 + OpenGL path 維持期間確定 + Phase 0 番号化形式 ✅ 化完了。(Q3) A = REJECT 時 baseline 確保継続 (= Phase K+4 で初撤廃)、(Q5) A = sub-step 命名 η-29 で既物理確定の形式 ✅ 化 (= 既反映済 → 確定マーク)。**§14.5 row count = 2 件 → 4 件 ✅** (= 本 ST-7 batch + sub-task 2 連続実施で 3-3 + 3-8 + 3-4 + 3-5 = 4 件 ✅ 完了 / 残 10 項目)。

**✅ 2026-06-03 ST-7 sub-task 2 完了 = §14.5 3-4 + 3-5 ✅ 反映済確認** (= chapter 04 §6.4/§4.3.1/§5.6 (= Deliverable A-1/A-2/A-3) 8+7+7 sub-subsection 起案済 + chapter 08 §5.4.1/§5.2.1/§11.5/§12.5/§13.5 (= Deliverable B-1/B-2/B-3/B-4/B-5) 8+8 sub-subsection + 3 section 起案済、grep verify pass = Claude 自走 verify、AYA 判断不要)。

**✅ 2026-06-03 ST-7 sub-task 3 完了 = §14.5 3-6 + 3-7 ✅ 反映済確認** (= chapter 09 §14 本 § 全 8 subsection 起案済 + 関連 chapter 04 §6.4.7 NTTP 判定材料 7 line 起案済 = R1 採用必要性 vs C++20 切替 cost trade-off 明示 + R1 不採用 default、§6.4.7 文末「chapter 09 §11 (Q-NTTP) として AYA 判断仰ぎ候補に登録」は **登録 intent の記述**、実 entry 追加は sub-task 4 batch で実施 (= 09 §11.6 新設 + 10 §1.0 row 29 + §1.3 表) / chapter 06a §3 mUniformUBOLoc cache 4 subsection + §5 16 method setter Vulkan path 分岐 6 subsection 起案済 = 06a doc 495 line で要件物理充足、grep verify pass = Claude 自走 verify、AYA 判断不要)。**Stage 3 self-check 14 項目のうち 3-3 + 3-4 + 3-5 + 3-6 + 3-7 + 3-8 = 6 件 ✅ 完了 / 残 8 項目**。残 8 項目 (= §14.5 row 番号で列挙) = 3-1 (Stage 0 全件確認) + 3-2 (Stage 1 全件確認) + 3-9 (chapter 02 §2.4 naming + chapter 07 set 帯 5 化 / 256B padding 反映済) + 3-10 (inventory + chapter 10 持越項目登録済) + 3-11 (autobuild manifest pin 確認) + 3-12 (`indra/` 改変解禁 = `feedback_design_phase_no_code_write` 解除点 = Phase 1.A 入口で達成) + 3-13 (Phase 1.A handoff doc 起案) + 3-14 (C++ standard NTTP 採否 AYA 判断仰ぎ)。**handoff §3.1 sub-task 番号と §14.5 row 番号は別体系で対応が一意ではない** (= 例 handoff §3.1 sub-task 4 内訳の handoff label 3-9 「autobuild manifest pin」は §14.5 row 3-11 に該当 / handoff label 3-10 「chapter 06c verify」は §14.5 row 3-8 で ST-6 既消化済)。次 batch (= Claude 自走 verify、AYA 判断不要) = §14.5 row 3-9 (chapter 02/07 反映済確認) + row 3-10 (chapter 10 持越項目登録済確認) 候補。

**✅ 2026-06-03 ST-7 sub-task 4 完了 = §14.5 3-9 + 3-10 ✅ 反映済確認 + (NTTP) gap remediation 実施** (= `feedback_doubt_self_first` 適用で verify 中に gap 検出 → 即時解消)。**row 3-9 verify** (= chapter 02 §2.4 naming + chapter 07 set 帯 5 化 / 256B padding 反映済): chapter 02 §2.4 「Codegen-UBO 生成識別子」起案済 = 生成物 4 種 (`<Block>_<Member>_OFFSET` / `<Block>Layout` / `<Block>_SIZE` / `ubo_layout_<blockname>.inl`) + machine-derivable 規律明示 / chapter 07 set 帯 = §3 (V1) device limit query + §4 (V3) set=1 layout + §5 (S3) sampler 49 + §6 (W) pool 容量 + §11 提供契約「padding alignment 256 B 出力契約」(= chapter 08 へ提供) + §3.1 maxUniformBufferOffsetAlignment 256 limit + §7.3 ring buffer offset alignment + 「Codegen 側で UBO struct size を alignment 倍数 (256 B safe) で padding 出力」明示、grep verify pass。**row 3-10 verify** (= inventory + chapter 10 持越項目登録済): 14 件中 13 件 ✅ 登録済 = (V1')(V3')(S3') §1.0 row 1-3 + §1.1 詳細 row 70-72 / (W) §1.0 row 4 / (A1)(P)(G/B3)(B1)(B2)(B4)(B5) §1.0 row 5-11 + §1.2 詳細 row 83-89 / (P-future)(cache-grow) §2.3 line 183-184 / inventory §7 残課題接続 §5 + §6.1/§6.2、ただし **(NTTP) gap 検出** (= chapter 04 §6.4.7 で「09 §11 (Q-NTTP) として登録」と意図記述あるが、09 §11 / 10 §1.0 / 10 §1.3 への実 entry は未追加 = 13/14 ✅ 状態)。**gap remediation 実施** (= 本 sub-task 4 batch 内で同時解消): (a) chapter 09 §11.6 (Q-NTTP) 新設 (= A/B/C 3 案 + default A 提案 + 確定タイミング明示) + (b) chapter 10 §1.0 row 29 (Q-NTTP) 追加 (= 28 → 29 件 / count 内訳 §1.3 5 → 6 + 未判断 17 → 18) + (c) chapter 10 §1.3 表に (Q-NTTP) 行追加 (= 09 §11.6 + 04 §6.4.7 出典明示) + (d) chapter 10 §1.0 「未判断 1 件の新規登録 cross-ref (= 2026-06-03 ST-7 sub-task 4 batch)」paragraph 追加。**gap remediation 連動 = 遡及 §14.2 row 0-9 (= chapter 10 open questions 集約 (NTTP) 等の登録) も (NTTP) 漏れ retroactive 充足** (= row 0-9 ✅ verdict 時点 (= Phase 2d-β-revise 起案時) の status を本 sub-task 4 で実体化)。**Stage 3 self-check 14 項目のうち 3-3 + 3-4 + 3-5 + 3-6 + 3-7 + 3-8 + 3-9 + 3-10 = 8 件 ✅ 完了 / 残 6 項目**。残 6 項目 = 3-1 (Stage 0 全件確認) + 3-2 (Stage 1 全件確認) + 3-11 (autobuild manifest pin 確認) + 3-12 (`indra/` 改変解禁 = Phase 1.A 入口) + 3-13 (Phase 1.A handoff doc 起案) + 3-14 (C++ standard NTTP 採否 AYA 判断仰ぎ = (Q-NTTP) AYA 判断本体)。

**✅ 2026-06-03 ST-7 sub-task 5 完了 = §14.5 3-1 + 3-2 ✅ 反映済確認** (= retroactive verify pass = Claude 自走 verify、AYA 判断不要)。**row 3-1 verify** (= Stage 0 §14.2 全 12 項目 ✅): 全 row grep / read 確認 pass = 0-1 chapter 04 §6.4 (line 746) / §4.3.1 (line 150) / §5.6 (line 483) + §6.4.7 (line 902) 全 section 存在 / 0-2 chapter 08 §5.4.1 (line 518) / §5.2.1 (line 206) / §11.5 (line 1160) / §12.5 (line 1507) / §13.5 (line 1733) 全 section 存在 / 0-3 chapter 09 §0-§14 全 15 chapter 存在 (= 本 chapter) / 0-4 chapter 06a-prep §0-§8 全 section 存在 + §2 (H1b) hook + §3 (E') + §4 (F) + §5 解析 + §6 反映 flow 6 行整備 / 0-5 chapter 02 §2.4 (line 100) Codegen-UBO 生成識別子 存在 / 0-6 chapter 05 §1-§6 + §6 (line 194) per-material cadence 最終判定 = MC1 → `MaterialUBO_Class3_Legacy` 確定 (2026-06-03) / 0-7 chapter 06b §0-§9 (`06b-cadence-update-site-and-dirty.md` 441 行) + chapter 06c §0-§11 (`06c-descriptor-set-bind-wiring.md` 513 行) 全 section 存在 / 0-8 chapter 07 §0-§13 全 section 存在 (= V1/V3/S3/W 全件起案済) / 0-9 chapter 10 (Q-NTTP) 実 entry 確認 = §1.0 row 29 (Q-NTTP) + §1.3 表 (Q-NTTP) 行 + 09 §11.6 (Q-NTTP) 新設 + 本 §14.2 row 0-9 遡及 retroactive 充足 mark 反映済 (= ST-7 sub-task 4 で実体化済) / 0-10 chapter 01 §5 確定事項 13 件 (row 1-13) enumerate 反映済 / 0-11 inventory `ayastorm-r41-ubo-current-state-inventory.md` §3.3.1 (line 154) 「set=2 内で binding 重複と見える UBO 名群」存在 = 85 UBO blueprint inventory (§3.1 set=0 3 + §3.2 set=1 2 + §3.3 set=2 26 + §3.4 set=3 54 = 85) 確定 / 0-12 `git status indra/` = modified 0 件 = `feedback_design_phase_no_code_write` 厳守継続 (= Phase 0 Step 2 hook 配線 `c27733ae79` は Step 5 `4e40fd2ab0` revert で痕跡全削除済、現 working tree は handoff doc 1 件 untracked のみ、indra/ 改変ゼロ)。**row 3-2 verify** (= Stage 1 §14.3 全 9 項目 ✅ + 06a-prep §6 反映 flow): 全 row 確認 pass = 1-1 AYAstorm Linux native build 動作 (= Phase 0 全 5 step 完走 = AYA 5 build 実施で確認) / 1-2 LL_INFOS hook 配線 (= `c27733ae79` Phase 0 Step 2 実装 + `AYASTORM_UBO_CADENCE_HOOK` flag-gated build 動作確認、§2.4 で「`indra/cmake/00-Common.cmake` 末尾に option 追加」+ §7 (P1)-(P4) 全件解消済) / 1-3 3 scenario cold launch + log 取得 (= Phase 0 Step 3 AYA Linux 実機計測 = scenario 2 Cocobolo Island + scenario 3 Roleplay Heaven 2 run + 各 run 前 cache clear protocol で完了、scenario 1 cold launch は scenario 2/3 frame=0 init phase で兼ねる = AYA 「２箇所とも一回キャッシュクリアしないと自分のアバター読み込みの処理負担が揃わない」判断で差替済) / 1-4 log 解析 (= Phase 0 Step 4 Claude 解析完了 = 06a-prep §5.5 観察結果 = 定常域 frame range 100-915 (s2 816 frame) + 100-735 (s3 636 frame) で UBO_CADENCE event 16.2M/19.7M + unique uniform 230/231 + tuple 7,410/6,973 観察、union 237 uniform 集約) / 1-5 (E') 5 件 grep (= 06a-prep §3.5「Phase 0 Step 1 Pre-hook Static Analysis 結果」全件解消済 = (E')-1 〜 (E')-5 binding 帰属確定) / 1-6 (F) MaterialUBO diff (= 06a-prep §4.6「Phase 0 Step 1 Pre-hook Static Analysis 結果」全件解消済 = F1/F2/F3 結果で MaterialUBO 暫定名 → `MaterialUBO_Class3_Legacy` 確定名へ) / 1-7 (H1b) 16 件 cadence 確定 + (E') 5 件 binding 帰属確定 + (F) MaterialUBO 処遇確定 (= §5.5.2-§5.5.7 観察 + §5.5.7 per-program/per-draw 境界 verify 完了 = `modelview_*` group 4-5 件は per-program → per-draw 補正必要、chapter 05 §7.3 への補正反映は chapter 06b 起案直前で実施済 = ST-6 batch で 06b §2.3 per-draw に 4 件反映済 (R-MAT1-4) / (E')(F)(Q26-MUL)(Q27-CONFL)(Q28-FFDUP) 等 chapter 10 §1.5 + §1.6 登録済 / Q26-MUL = `MaterialUBO_Class3_Legacy` 確定で消化済) / 1-8 検証 hook 除去 + 通常 build PASS + diff 0 件 (= `4e40fd2ab0` revert commit で hook 痕跡全削除 = helper/macro/extern/setter body macro/CMake option 全削除、残存 grep 0 件、flag OFF build PASS、06a-prep §2.8 「検証完了後の除去 protocol」5 step 準拠完走) / 1-9 (RF) reflection update fence throttle 頻度 log (= 1-2 と同 hook 配線範囲に乗せ済 = `c27733ae79` 配線 + `4e40fd2ab0` revert で取得 + 除去サイクル完了)。**06a-prep §6 反映 flow 6 行**: (a) (H1b) 不明 16 件 cadence 確定 → chapter 06a §0.2 cadence 推定表 反映済 / (b) (H1b) per-program ↔ per-draw 境界 verify → chapter 06a §0.2 + chapter 05 §7.3 反映済 (= ST-6 で 06b §2.3 per-draw 4 件 R-MAT1-4 反映済) / (c) (H1b) dead uniform 検出 → inventory §7 残課題 反映済 (= 121 件 observed 0 件は 06a-prep §5.5.5 で category 別整理 + chapter 10 §2.7 R-AYA1/2/3 + R-MAT1-4 + R-TERR に集約) / (d) (E') 5 UBO の binding 帰属確定 → inventory §3.3.1 / chapter 05 §3.3 反映済 (= 06a-prep §3.5 Pre-hook Static Analysis で全件解消) / (e) (F) MaterialUBO 処遇確定 → chapter 05 §5 / chapter 02 §3.2 反映済 (= MaterialUBO_Class3_Legacy 確定 + 06b §3.3.4 Q26-MUL 反映済) / (f) 上記全件確定 → chapter 06b 起案前提整備完了 = chapter 06b 既起案済 (= ST-6 で B 案採用 = 既存 441 行で要件物理充足)。**Stage 3 self-check 14 項目のうち 3-3 + 3-4 + 3-5 + 3-6 + 3-7 + 3-8 + 3-9 + 3-10 + 3-1 + 3-2 = 10 件 ✅ 完了 / 残 4 項目**。残 4 項目 = 3-11 (autobuild manifest pin 確認) + 3-12 (`indra/` 改変解禁 = Phase 1.A 入口) + 3-13 (Phase 1.A handoff doc 起案) + 3-14 (C++ standard NTTP 採否 AYA 判断仰ぎ = (Q-NTTP) AYA 判断本体)。次 batch (= Claude 自走 verify、AYA 判断不要) = §14.5 row 3-11 (autobuild manifest `autobuild.xml` で glslang / spirv-cross / Python version pin 状態確認) 候補。

**✅ 2026-06-03 ST-7 sub-task 6 完了 = §14.5 3-11 ✅ 反映済確認 (= autobuild manifest pin 仕様確定 + 実 pin 追加 Phase 1.A 内 task 化)** (= Claude 自走 verify、AYA 判断不要) **🔴 (2026-06-03 η-30 PA-1 entry 直前 post-completion correction)**: 本 sub-task 6 verdict は autobuild.xml **片側 grep のみ**で「3 dependency 0 件 = 全件追加必要」と判定したが、η-30 PA-1 entry 直前で `indra/cmake/` + `scripts/` 横断 verify (Agent Explore) 実行 → `indra/cmake/Glslang.cmake` で `find_package(glslang CONFIG REQUIRED)` + `glslang-15.1.0/` vendored + Ubuntu 24.04 `apt install glslang-dev` (15.1.0-2) 既存取込確認 + `indra/cmake/Python.cmake` で `find_package(Python3 COMPONENTS Interpreter)` 既存取込確認 = autobuild.xml 経路は **不使用** (= glslang は B2b system pkg 採用済 / Python は build tool として host 探索) = 片側検証 gap 検出 = `feedback_doubt_self_first` 適用、AYA「A」応答で **PA-1 真 scope = spirv-cross のみ取込** (= `SpirvCross.cmake` 起案 + Glslang.cmake と同 pattern) に scope 訂正 + chapter 08 §5.4.1.5 + 本 §14.4 + §14.5 row 3-11 + chapter 10 §1.0 / §1.2 (B2) verdict 連動 update 済 (= 2026-06-03 4 message 分割 sequential 進行)。本 correction は **設計 phase scope の verdict 訂正** = Stage 3 全完走 (14/14 ✅) 自体は維持 (= row 3-11 設計 phase 内充足扱いは変わらず、実 取込 scope のみ scope 縮小 1 件 spirv-cross)。**row 3-11 verify (元文献)**: (a) `autobuild.xml` 現状 grep (case-insensitive) = `glslang` / `spirv-cross` / `Python` (含む大文字小文字 + 各 variant) entry **0 件** = 未追加状態 (= 4249 行 manifest 既存 dependency `SDL2` / `gstreamer10` 等と同形式の entry 不在) (= **片側検証 only、`indra/cmake/` 横断 verify 漏れ**) / (b) chapter 08 §5.4.1.5 line 703 「format version pin」記述 (= post-correction 後の本 §5.4.1.5 update 形で literal 確定) / (c) chapter 08 §11.5.1 line 1182-1184 cache key environment block 期待 version 例示 = Python 3.11.5 / glslang 1.3.275.0 / spirv-cross 2023-12-07 (= 実 cache invalidation key、本 §5.4.1.5 update でも version drift 抑制 mechanism として維持) **🔴 (2026-06-03 PA-1 install verify 後 訂正)** = 実 install 値 = Python 3.12.3 / glslang 15.1.0 / spirv-cross 1.3.239.0 (= Ubuntu 24.04 apt 提供版) に書換 (= 計画値 = Vulkan SDK 1.3.275 era 想定 / 実値 = Ubuntu 24.04 LTS 提供版 = 上位下位の drift、C API 安定で blocker 無し) / (d) chapter 08 §5.4.1.1 line 538 glslang version stable 根拠 / (e) chapter 08 §11.5.2 cache invalidation trigger 表 line 1321「glslang version upgrade」完備。**Stage 3 self-check 14 項目のうち 3-1 + 3-2 + 3-3 + 3-4 + 3-5 + 3-6 + 3-7 + 3-8 + 3-9 + 3-10 + 3-11 = 11 件 ✅ 完了 / 残 3 項目**。残 3 項目 = 3-12 (`indra/` 改変解禁 = Phase 1.A 入口で達成) + 3-13 (Phase 1.A handoff doc 起案) + 3-14 (C++ standard NTTP 採否 AYA 判断仰ぎ = (Q-NTTP) AYA 判断本体)。次 batch (= Claude 自走、AYA 判断不要) = **row 3-13 Phase 1.A handoff doc 起案 + row 3-12 解禁 timing 整理 batch** 候補 (= row 3-12 は「Phase 1.A 入口到達 = 解禁」と確定済 / row 3-14 は本 sub-task 7 batch 最後 = AYA 判断本体)。

**✅ 2026-06-03 ST-7 sub-task 7 完了 = §14.5 3-12 + 3-13 ✅ 反映済 (= Phase 1.A handoff doc 物理起案 + `indra/` 改変解禁 timing 3 段階整理確定)** (= Claude 自走、AYA 判断不要)。**row 3-13 起案** (= Phase 1.A handoff doc 物理出力): `docs/specs/ayastorm-r41-gl-removal/handoff/phase1/a/handoff-phase1-a-entry.md` 新規作成完了 = (a) §0 state 一行 summary (= 13/14 ✅ + 残 1 = (Q-NTTP) AYA 判断本体宣言) + (b) §1.1 pre-req 最小読み 3 件 (= 本 handoff + 09 §4/§14.5 + 08 §0-§17) + §1.2 pinpoint Read reference 8 file + (c) §2.1 Phase 1.A scope literal 継承 (= 09 §4.1 row "1.A" = Codegen pipeline 実装) + §2.2 Phase 1.A Exit Criteria literal 継承 (= 09 §4.2 = 85 UBO blueprint で codegen 実行 + 4 file 生成 + build error 0 + 名前解決衝突 0) + (d) §3 sub-task PA-1 〜 PA-8 構成表 (= PA-1 autobuild pin / PA-2 Python script base / PA-3 mini-parser + glslang -E / PA-4 std140 calculator + SPIR-V reflection 二重保証 / PA-5 perfect hash CHD frozen-table / PA-6 増分 build cache hash + mtime / PA-7 CMake DEPENDS + 手動 target / PA-8 85 UBO blueprint 実行 + Exit 充足検証、strict 線形順序、PA-3 + PA-4 のみ並列可、(Q-NTTP) B/C 採用時 PA-0 = C++20 切替 分岐明示) + (e) §4 紐付け持越項目 (= 08 §17 (A1)(P)(G/B3)(B1)(B2)(B4)(B5) 7 件 + autobuild pin 1 件 + Phase 1.B/1.C/2 送り出し 3 件) + (f) §5 規律 10 件 (= `feedback_design_phase_no_code_write` 解除 timing 明示 + 9 件 feedback 適用) + (g) §6 9 観点 self-verify PASS + (h) §7 引き継ぎ済 memory 17 件 + (i) §8 次 session 着手 1 line。**row 3-12 timing 整理** (= 解禁 timing 3 段階確定): (a) Stage 1 Phase 0 Step 2 hook 配線 (`c27733ae79`) で **一時 解禁** = 計測 hook 仕込み目的の限定解禁 / (b) Phase 0 Step 5 hook revert (`4e40fd2ab0`) で **再封** = 設計 phase (Stage 2-3) 再突入で `indra/` 改変ゼロ厳守継続 = 本 sub-task 5/6/7 まで全期間 `git status indra/` modified 0 件 維持 / (c) Phase 1.A 入口到達 = 全 14 項目 ✅ + AYA 承認 (= 3-14 (Q-NTTP)) で **完全 解禁** = Phase 1.A handoff doc PA-1 entry 時点で `feedback_design_phase_no_code_write` 解除点に到達。**Stage 3 self-check 14 項目のうち 3-1 + 3-2 + 3-3 + 3-4 + 3-5 + 3-6 + 3-7 + 3-8 + 3-9 + 3-10 + 3-11 + 3-12 + 3-13 = 13 件 ✅ 完了 / 残 1 項目**。残 1 項目 = **3-14 (C++ standard NTTP 採否 = (Q-NTTP) AYA 判断本体)** = chapter 10 §1.3 (Q-NTTP) 行 default A R1 不採用 = C++17 維持 を AYA「default 採用継続」or「B 案 R1 NTTP 採用」or「C 案 R1+R3 hybrid 採用」で判定、default A 採用継続なら Phase 1.A 即着手可 (= 09 §11.6 末尾「着手 ready state に影響なし」)。次 = **AYA (Q-NTTP) 判断仰ぎ batch** = 1 メッセージで default + 残 sub-option summary 提示 (= `feedback_one_step_at_a_time` 準拠) = Stage 3 14/14 ✅ 到達 = 全 stage 完走で `feedback_design_phase_no_code_write` 完全解禁 = Phase 1.A 実装 entry へ移行。

**✅ 2026-06-03 ST-7 sub-task 8 完了 = §14.5 3-14 ✅ + Stage 3 14/14 ✅ 全完走 = 設計 phase 完了** (= (Q-NTTP) AYA「A」応答 = **A 確定 = R1 不採用 / C++17 維持**)。本 §14.4 ST-7 batch 全 sub-task 完走 (= sub-task 1 (Q3)(Q5) + sub-task 2 chapter 04/08 verify + sub-task 3 chapter 09 §14 + 04 §6.4.7 + 06a verify + sub-task 4 chapter 02 §2.4 + chapter 07 + chapter 10 持越項目 verify + (NTTP) gap remediation + sub-task 5 Stage 0/1 retroactive verify + sub-task 6 autobuild manifest pin 仕様確定 + sub-task 7 Phase 1.A handoff doc 起案 + `indra/` 解禁 timing 3 段階整理 + sub-task 8 (Q-NTTP) AYA 判断本体)。**Stage 3 self-check 14 項目 = 14/14 ✅ 全完走** (= 3-1/3-2/3-3/3-4/3-5/3-6/3-7/3-8/3-9/3-10/3-11/3-12/3-13/3-14 全件 ✅)。**設計 phase 完了 = `feedback_design_phase_no_code_write` 完全解禁条件達成** (= row 3-12 完全解禁 trigger 条件 (= 全 14 項目 ✅ + AYA 承認) 充足 = Phase 1.A 実装 entry へ移行可能 state)。反映先連動: chapter 09 §11.6 (Q-NTTP) 確定形書換 + 本 §14.4 paragraph + §14.5 row 3-14 inline ✅ mark + chapter 10 §1.0 row 29 状態 column ✅ + count 内訳 12 件判断済 / 17 件未判断 update + §1.3 (Q-NTTP) 行 verdict マーク + §1.3 末尾 ST-7 sub-task 8 batch verdict paragraph + chapter 04 §6.4.7 確定形書換。次 session 着手地点 = Phase 1.A handoff doc `handoff-phase1-a-entry.md` §3 sub-task PA-1 (= autobuild manifest pin = `glslang` / `spirv-cross` / `Python` 3 dependency entry 追加 + Linux/Win/Mac 3 platform 配信 URL pin) 着手。

**🔴 2026-06-03 η-30 PA-1 entry 直前 self-verify gap remediation = (B2) glslang 統合方式 verdict 確定 + PA-1 真 scope 訂正** (= `feedback_doubt_self_first` 適用 + AYA「A」応答): η-30 Phase 1.A entry handoff doc pre-req 最小読み 3 件 (= 本 chapter §4 + §14.5 + handoff doc + chapter 08 §0-§17) 実施後の **PA-1 entry 直前** で `autobuild.xml` + `indra/cmake/` + `scripts/` 横断 verify (= Agent Explore medium thoroughness) 実行 → finding 3 件: (a) **glslang は実装で既存取込済** = `indra/cmake/Glslang.cmake` で `find_package(glslang CONFIG REQUIRED)` + `glslang-15.1.0/` vendored + Ubuntu 24.04 `apt install glslang-dev` (15.1.0-2) 経路 (= chapter 10 §1.2 (B2) **B2b system pkg 確定** = 実装で先行 commit 済) / (b) **Python は既存取込済** = `indra/cmake/Python.cmake` で `find_package(Python3 COMPONENTS Interpreter)` (= autobuild manifest 経路使わず host 探索) / (c) **spirv-cross のみ真に未取込** (= autobuild.xml + indra/cmake/ + scripts/ 全件 0 件)。**含意**: ST-7 sub-task 6 verdict (= row 3-11) は autobuild.xml **片側 grep のみ**で「3 dependency 0 件 = 全件追加必要」と判定したが、`indra/cmake/` 横断 verify 漏れによる **片側検証 gap** = `feedback_doubt_self_first` literal 適用 = handoff doc pre-req 確認 phase でも本 feedback 強化適用すべき教訓。**AYA「A」応答**: 「spirv-cross のみ追加 (推奨) → handoff doc + chapter 08 §5.4.1.5 + Stage 3 row 3-11 verdict を実態に合わせ update」確定 = **PA-1 真 scope = spirv-cross のみ取込** (= `indra/cmake/SpirvCross.cmake` 起案 + system install + `find_package(spirv_cross_c_shared CONFIG REQUIRED)` = Glslang.cmake と同 pattern = Linux first-class baseline (r41 charter §1)、Win/Mac 3 OS bundle は r42-α/β 着手時に判断 (charter §7.5))。**連動 update 範囲** (= 2026-06-03 4 message 分割 sequential): handoff PA-1 entry doc §0 + §3 PA-1 cell + §3 (Q-NTTP) paragraph + §4 (B2) 行 + §5 規律 11/12 + §6 row 10 + §7 memory + §8 着手 1 line (= 計 8 edit) + chapter 08 §5.4.1.5 format version pin paragraph (= 1 edit) + 本 §14.4 ST-7 sub-task 6 paragraph 末尾追記 + 本 paragraph 新設 + §14.5 row 3-11 verdict 訂正 (= 計 3 edit) + chapter 10 §1.0 row 9 状態 + §1.0 count 内訳 + §1.2 (B2) row verdict (= 計 3 edit) = **5 file 計 15 edit batch**。**Stage 3 14/14 ✅ verdict 自体は維持** (= 設計 phase 完了状態は変わらず、本 correction は scope 縮小 1 件 (= spirv-cross のみ) + (B2) verdict 確定 + verdict 訂正 mechanism の補強)。**教訓 memory 候補** (= 引き継ぎ済 memory に追加検討): `feedback_two_sided_verify` (= 「未登録 = 未実装」と判定する前に `*.cmake` / `scripts/` / build config 横断で実装側既存を必ず確認、autobuild manifest grep のみで verdict しない) = handoff doc §5 規律 11/12 に literal 反映済。

### §14.5 Stage 3: Phase 1.A 入口 readiness (= 着手 ready state)

**Phase 1.A 着手前の最終 self-check** (= Stage 0/1/2 全完了の必要条件 + Phase 1.A 固有 condition):

| # | 項目 | 確認方法 | 失敗時対応 |
|---|---|---|---|
| 3-1 | Stage 0 全 12 項目 ✅ **✅ 2026-06-03 ST-7 sub-task 5** (= §14.2 全 12 row 確認 = 0-1 chapter 04 §6.4 (line 746) + §4.3.1 (line 150) + §5.6 (line 483) 全 section 存在 / 0-2 chapter 08 §5.4.1 (line 518) + §5.2.1 (line 206) + §11.5 (line 1160) + §12.5 (line 1507) + §13.5 (line 1733) 全 section 存在 / 0-3 chapter 09 §0-§14 全 15 chapter 存在 / 0-4 chapter 06a-prep §0-§8 全 section 存在 + §6 反映 flow 表 6 行整備 / 0-5 chapter 02 §2.4 (line 100) 「Codegen-UBO 生成識別子」存在 / 0-6 chapter 05 §1-§6 (§6 line 194「per-material cadence 最終判定 = 確定 2026-06-03」MC1 → `MaterialUBO_Class3_Legacy` 確定反映) / 0-7 chapter 06b §0-§9 全 section 存在 + 06c §0-§11 全 section 存在 / 0-8 chapter 07 §0-§13 全 section 存在 / 0-9 chapter 10 (Q-NTTP) 実 entry 確認 = §1.0 row 29 + §1.3 表 (Q-NTTP) 行 + 09 §11.6 (Q-NTTP) 新設 + 本 §14.2 row 0-9 遡及 retroactive 充足 mark 反映済 / 0-10 chapter 01 §5 確定事項 13 件 (row 1-13) 反映済 / 0-11 inventory §3.3.1 (line 154) 「set=2 内で binding 重複と見える UBO 名群」存在 = 85 UBO blueprint (§3.1 3 + §3.2 2 + §3.3 26 + §3.4 54 = 85) inventory 確定 / 0-12 `git status indra/` = modified 0 件 = `feedback_design_phase_no_code_write` 厳守継続) | §14.2 表で全件確認 | 不足 item を §14.2 表で特定 → 該当 chapter update |
| 3-2 | Stage 1 全 9 項目 ✅ (= Phase 0 計測完了 + chapter 05/06a/06b/06c 反映済) **✅ 2026-06-03 ST-7 sub-task 5** (= §14.3 全 9 row 確認 = 1-1 AYAstorm Linux native build 動作 = Phase 0 全 5 step 完走で AYA 確認済 / 1-2 LL_INFOS hook 配線 = `c27733ae79` Phase 0 Step 2 hook 実装 + flag-gated build (`AYASTORM_UBO_CADENCE_HOOK`) 動作 / 1-3 3 scenario cold launch + log 取得 = Phase 0 Step 3 AYA Linux 実機計測 = scenario 2 Cocobolo Island + scenario 3 Roleplay Heaven 2 run 完了 (scenario 1 cold launch は scenario 2/3 frame=0 init phase で兼ねる、AYA 判断で差替済) / 1-4 log 解析 = Phase 0 Step 4 Claude 解析完了 = 06a-prep §5.5 観察結果 (定常域 16.2M/19.7M UBO_CADENCE event + 230/231 unique uniform 観察) / 1-5 (E') 5 件 grep = 06a-prep §3.5「Phase 0 Step 1 Pre-hook Static Analysis 結果」全件解消済 / 1-6 (F) MaterialUBO diff = 06a-prep §4.6「Phase 0 Step 1 Pre-hook Static Analysis 結果」全件解消済 / 1-7 (H1b) 16 件 cadence 確定 + (E') 5 件 binding 帰属確定 + (F) MaterialUBO 処遇確定 = §5.5.2-§5.5.7 観察 + §5.5.7 per-program/per-draw 境界 verify 完了 + chapter 05 §6 MC1 → `MaterialUBO_Class3_Legacy` 確定 + (E')(F) 等 chapter 10 §1.5 登録済 / 1-8 検証 hook 除去 + 通常 build PASS + diff 0 件 = `4e40fd2ab0` revert commit で hook 痕跡全削除 (helper/macro/extern/setter body macro/CMake option 全削除、残存 grep 0 件、flag OFF build PASS、06a-prep §2.8 5 step 除去 protocol 準拠) / 1-9 (RF) reflection fence throttle 頻度 log = 1-2 と同 hook 配線範囲に乗せ済 = `c27733ae79` 配線 + `4e40fd2ab0` revert で取得 + 除去サイクル完了) + 06a-prep §6 反映 flow 6 行全行「反映済」確認 (= chapter 05 §3.3 / §5 / §6 + chapter 02 §3.2 + chapter 06a §0.2 + inventory §3.3.1 / §7 への (H1b)(E')(F) 反映済、§7 末尾「全 4 件解消、§2.3.1 / §2.3.3 / §2.4 への反映完了。実装 phase 入口時点で本 §7 は追加 grep 不要」確定) | §14.3 表で全件確認 + 06a-prep §6 反映 flow 全行「反映済」 | 計測 task 再実施 (= chapter 06a-prep §3.x 再実施) |
| 3-3 | Stage 2 (Q1) / (Q2) AYA 判断 ✅ | 本 §14.4 表で確認 + chapter 10 (open-questions) で判断履歴登録 | AYA 判断仰ぎ session |
| 3-4 | chapter 04 §6.4 / §4.3.1 / §5.6 (= Deliverable A-1/A-2/A-3) 反映済 **✅ 2026-06-03 ST-7 sub-task 2** (= §4.3.1 std140 calculator algorithm 詳細化 8 sub-subsection 起案済 (`§4.3.1.1` base alignment 表 / `.2` offset state machine / `.3` nested struct / `.4` array stride / `.5` 末尾 padding / `.6` unsupported type / `.7` SPIR-V reflection 接合 / `.8` 実装規模見積) + §5.6 perfect hash CHD algorithm 詳細化 7 sub-subsection 起案済 (`§5.6.1` 採用根拠 / `.2` 2 段 hash + displacement / `.3` 構築 step / `.4` 性能特性 / `.5` 衝突 0 invariant / `.6` 出力 C++ 形式 / `.7` 実装規模見積) + §6.4 name-based dispatch algorithm 詳細化 7 sub-subsection 起案済 (`§6.4.1` R3 primary / `.2` R1 補助 / `.3` R2 動的 / `.4` unresolved fallback / `.5` R1 path table / `.6` 3 path 性能比較 / `.7` C++20 NTTP 採否判定材料)) | grep '^#### §[456]\\.' 04-codegen-ubo.md で section 存在 + 内容確認 | Phase 2d-β-revise 本 session で起案 |
| 3-5 | chapter 08 §5.4.1 / §5.2.1 / §11.5 / §12.5 / §13.5 (= Deliverable B-1/B-2/B-3/B-4/B-5) 反映済 **✅ 2026-06-03 ST-7 sub-task 2** (= §5.4.1 SPIR-V reflection 二重保証 mechanism 詳細化 8 sub-subsection 起案済 (`§5.4.1.1` reflection 抽出経路 / `.2` JSON schema / `.3` per-member 照合 algorithm / `.4` 二重保証 build error 出力 / `.5` format drift 耐性 / `.6` escape hatch / `.7` set=1 split 整合 / `.8` 実装規模見積) + §5.2.1 mini-parser 詳細化 8 sub-subsection 起案済 (`§5.2.1.1` input 契約 / `.2` token grammar EBNF / `.3` state machine 実装 / `.4` nested struct 対応 / `.5` sampler 抽出 path / `.6` LL 慣用範囲外検出 / `.7` #line directive 追跡 / `.8` 実装規模見積) + §11.5 増分 build cache 詳細化 + §12.5 CMake DEPENDS + 手動 target 詳細化 + §13.5 3 OS binary identical 保証 mechanism 詳細化 起案済) | grep '^#### §' 08-build-codegen-pipeline.md で 5 section 存在 + 内容確認 | 同上 |
| 3-6 | chapter 09 §14 (= 本 §) 反映済 **✅ 2026-06-03 ST-7 sub-task 3** (= 本 §14 全 8 subsection (§14.1 readiness 4 階層 + §14.2 Stage 0 / §14.3 Stage 1 / §14.4 Stage 2 / §14.5 Stage 3 / §14.6 達成順序図 / §14.7 chapter 整合 pointer / §14.8 self-evaluation) 起案済 + 本 ST-7 batch で §14.4 Stage 2 完全達成 paragraph + sub-task 2/3 完了 paragraph 追記済 = live update 継続) + 関連 chapter 04 §6.4.7 NTTP 判定材料 起案済 (= R1 採用必要性 vs C++20 切替 cost trade-off 明示 + AYAstorm 既存 build standard C++17 default + R1 不採用 default + (Q-NTTP) として chapter 09 §11 AYA 判断仰ぎ候補登録、AYA 判断本体は §14.5 row 3-14 (Phase 1.A 入口) 保留継続) | grep '^## §14' 09-phase-roadmap.md | 同上 |
| 3-7 | chapter 06a §3 mUniformUBOLoc cache 構造 + §5 16 method setter 分岐 設計起案済 **✅ 2026-06-03 ST-7 sub-task 3** (= §3 mUniformUBOLoc cache 構造 4 subsection 起案済 (`§3.1` struct UniformLocation 形式 = chapter 04 §5.3.2 と同形 / `§3.2` LLGLSLShader member 配置 / `§3.3` cadence_tag enum 値域確定 = 06b 連動 / `§3.4` 解放規律) + §5 16 method setter family Vulkan path 分岐 6 subsection 起案済 (`§5.1` 17 method 一覧 = inventory §4.3 再掲、2026-06-03 second-pass §2.4 反映 uniform1i 追加 / `§5.2` path 分岐 code shape uniform1f 例 / `§5.3` 16 method 共通 pattern 規律 / `§5.4` mUseUBO flag 配置 + initial 設定方針 / `§5.5` integer index 経路 vs LLStaticHashedString 経路分岐 / `§5.6` sampler 系 setter OpenGL path 強制 49 個)、合計 06a doc 495 line で要件物理充足) | grep '^### §3\\.' 06a-cache-structure-and-setter-redirect.md | chapter 06a 起案 task |
| 3-8 | chapter 06b cadence 別 update site 5 種 + 06c descriptor set bind 配線 起案済 **✅ 2026-06-03 ST-6** (= `06b-cadence-update-site-and-dirty.md` 441 行 §2.1-§2.5 で 5 cadence (per-frame / per-program / per-draw / per-asset / per-skin) 全件起案済 + `06c-descriptor-set-bind-wiring.md` 513 行 §2 / §4 で descriptor set 4 帯 cadence 別配置 + flush 直後 bind 配線起案済、ST-6 前段 (a)(b) findings (= R-MAT4 `normal_matrix` 確定 + R-AYA1/2 dead / R-AYA3 alive 既移植済) は 06b §2.2 / §2.3 / §3.3 / §8 に delta integration 済) | grep '^## §' 06b/06c | chapter 06b/06c 起案 task |
| 3-9 | chapter 02 §2.4 naming + chapter 07 set 帯 5 化 / 256B padding 反映済 (= chapter 04/08 出力契約と整合) **✅ 2026-06-03 ST-7 sub-task 4** (= chapter 02 §2.4 「Codegen-UBO 生成識別子」起案済 = 生成物 4 種 (`<Block>_<Member>_OFFSET` / `<Block>Layout` / `<Block>_SIZE` / `ubo_layout_<blockname>.inl`) + machine-derivable 規律 / chapter 07 set 帯 = §3 (V1) + §4 (V3) + §5 (S3) + §6 (W) + §11 「padding alignment 256 B 出力契約」chapter 08 提供 + §3.1 maxUniformBufferOffsetAlignment 256 limit + §7.3 ring buffer offset alignment + 「Codegen 側で UBO struct size を alignment 倍数 (256 B safe) で padding 出力」明示) | grep '^### §2.4' 02 + '§4.4' 07 | chapter 02/07 update task |
| 3-10 | inventory + chapter 10 (open-questions) で持越項目 (V1')(V3')(S3')(W) + (A1)(P)(G/B3)(B1)(B2)(B4)(B5) + (NTTP) + (P-future)(cache-grow) 登録済 **✅ 2026-06-03 ST-7 sub-task 4** (= 14 件中 13 件 (V1'/V3'/S3'/W/A1/P/G-B3/B1/B2/B4/B5/P-future/cache-grow) は chapter 10 §1.0/§1.1/§1.2/§2.3 + §5 + §6.1/§6.2 で既登録済、(NTTP) gap 検出 → 本 sub-task 4 batch で chapter 09 §11.6 新設 + chapter 10 §1.0 row 29 + §1.3 表 (Q-NTTP) 行追加で remediation 実施 = 14/14 ✅ 充足、`feedback_doubt_self_first` 適用 = verify 中の gap 検出 → 即時解消 documentation) | grep 各 ID in chapter 10 | chapter 10 update task |
| 3-11 | autobuild manifest で glslang / spirv-cross / Python version pin 状態確認 (= §5.4.1.5 format pin 用意) **✅ 2026-06-03 ST-7 sub-task 6** (= 当初 verdict = **設計 phase scope では充足**) **🔴 2026-06-03 η-30 PA-1 entry 直前 post-completion correction** (= 本 §14.4 末尾 paragraph 詳細記録): ST-7 sub-task 6 verdict は autobuild.xml **片側 grep のみ**で判定 = `indra/cmake/` 横断 verify 漏れ = `feedback_doubt_self_first` 適用 → AYA「A」応答で **PA-1 真 scope = spirv-cross のみ取込** に scope 訂正 = (a) **glslang** = `indra/cmake/Glslang.cmake` で `find_package(glslang CONFIG REQUIRED)` + `glslang-15.1.0/` vendored + system `apt install glslang-dev` (Ubuntu 24.04 = 15.1.0-2) 経路で既存取込済 (= chapter 10 §1.2 (B2) **✅ B2b system pkg 確定**)、autobuild.xml 経路は不使用 / (b) **Python** = `indra/cmake/Python.cmake` で `find_package(Python3 COMPONENTS Interpreter)` 既存取込済、autobuild.xml 経路は不使用 / (c) **spirv-cross** = 真に未取込 = PA-1 で `indra/cmake/SpirvCross.cmake` 起案 + system install + `find_package(spirv_cross_c_shared CONFIG REQUIRED)` (= Glslang.cmake と同 pattern = Linux first-class baseline (r41 charter §1)、Win/Mac 3 OS bundle は r42-α/β 着手時 (charter §7.5)) で取込 = autobuild.xml 経路は不使用。**設計 phase 内 verdict ✅ は維持** (= スコープ縮小 1 件 (= spirv-cross のみ) + (B2) verdict 確定 + chapter 08 §5.4.1.5 + 本 §14.4 + 本 row + chapter 10 §1.0 / §1.2 連動 update 済)。**version drift 抑制**: chapter 08 §11.5.1 cache key environment block (= `python_version: "3.12.3"` / `glslang_version: "15.1.0"` / `spirv_cross_version: "1.3.239.0"` = Ubuntu 24.04 apt 実 install 値、2026-06-03 PA-1 install verify で確定) を Codegen tool runtime 取得 → cache invalidation key で drift 検知 (= autobuild manifest pin 等価効果)。元 verify 内容 (= autobuild.xml 0 件 + chapter 08 §5.4.1.5 + §11.5.1 + §11.5.2) は本 §14.4 末尾 paragraph 「元文献」block 参照。 | 両側 verify (= autobuild.xml grep + `indra/cmake/` + `scripts/` 横断 verify) + chapter 08 §5.4.1.5 post-correction 後 literal 整合 + AYA「A」応答 反映 | (PA-1 = spirv-cross のみ取込、glslang/Python は既存取込済 scope 外) |
| 3-12 | `indra/` 配下改変 解禁 = `feedback_design_phase_no_code_write` 解除点に到達 **✅ 2026-06-03 ST-7 sub-task 7** (= 解禁 timing 3 段階整理確定 = (a) Stage 1 Phase 0 Step 2 hook 配線 (`c27733ae79`) で **一時 解禁** = 計測 hook 仕込み目的の限定解禁 / (b) Phase 0 Step 5 hook revert (`4e40fd2ab0`) で **再封** = 設計 phase (Stage 2-3) 再突入で `indra/` 改変ゼロ厳守継続 = 本 sub-task 5/6/7 まで全期間 `git status indra/` modified 0 件 維持 / (c) Phase 1.A 入口到達 = 全 14 項目 ✅ + AYA 承認 (= 3-14 (Q-NTTP)) で **完全 解禁** = Codegen pipeline 実装 (= Phase 1.A handoff doc § 3 sub-task PA-1 〜 PA-8) entry 時点で `feedback_design_phase_no_code_write` 解除点に到達、以降の `indra/cmake/` + `scripts/ubo_codegen/` + `autobuild.xml` 改変は実装 task の正常範囲) | 解禁 timing 3 段階確定 (= 一時解禁 → 再封 → 完全解禁) + 本 §3-12 verdict 「完全解禁条件 = 全 14 項目 ✅ + AYA (Q-NTTP) 承認」確定 | Phase 1.A 入口で完全解禁 |
| 3-13 | Phase 1.A handoff doc 起案 (= 本 chapter §4 Phase 1.A scope + Exit Criteria 反映) **✅ 2026-06-03 ST-7 sub-task 7** (= `docs/specs/ayastorm-r41-gl-removal/handoff/phase1/a/handoff-phase1-a-entry.md` 起案完了 = §0 state summary + §1 pre-req 最小読み 3 件 + §1.2 pinpoint Read 8 file + §2 Phase 1.A scope (= 09 §4.1) + Exit Criteria (= 09 §4.2) literal 継承 + §3 sub-task PA-1 〜 PA-8 構成表 (= PA-1 autobuild pin / PA-2 Python base / PA-3 mini-parser / PA-4 std140 + reflection / PA-5 perfect hash / PA-6 cache / PA-7 CMake / PA-8 85 UBO 実行 + Exit 充足検証、strict 線形順序、PA-3 + PA-4 のみ並列可、(Q-NTTP) B/C 採用時 PA-0 = C++20 切替 追加可能性明示) + §4 紐付け持越項目 (= 08 §17 (A1)(P)(G/B3)(B1)(B2)(B4)(B5) 7 件 + autobuild pin 1 件 + Phase 1.B/1.C/2 送り出し 3 件) + §5 規律 (= `feedback_design_phase_no_code_write` 解除 timing 明示 + 10 件 feedback 適用) + §6 9 観点 self-verify PASS + §7 引き継ぎ済 memory 17 件 + §8 次 session 着手 1 line) | `docs/specs/ayastorm-r41-gl-removal/handoff/phase1/a/handoff-phase1-a-entry.md` 物理存在 + Phase 1.A scope + Exit Criteria + sub-task 構成 + 紐付け持越項目 全件反映済 | Phase 1.A 入口直前 task |
| 3-14 | C++ standard 確認 (= R1 path 用 C++20 NTTP の採否 = chapter 04 §6.4.7 (NTTP) 判定材料) **✅ 2026-06-03 ST-7 sub-task 8** (= (Q-NTTP) AYA「A」応答 = **A 確定 = R1 不採用 / C++17 維持** = chapter 09 §11.6 default 採用継続 = R3 name-based dispatch + perfect hash (CHD) + frozen-table 経路で十分高速、R1 効果差 (= compile-time vs runtime 1 indirection) limited、C++20 切替 cost (= 3 OS toolchain 確認 + dependent module re-validation + autobuild manifest 変更) 回避、R1 は Phase K+4 以降 polish 候補保留可、Phase 1.A handoff doc §3 PA-0 (= C++20 切替 task) 不要 = PA-1 から即着手可、反映先 = chapter 09 §11.6 確定形書換 + 本 §14.5 row 3-14 ✅ mark + chapter 10 §1.0 row 29 状態 ✅ + §1.0 count 内訳 11 → 12 判断済 / 18 → 17 未判断 + §1.3 (Q-NTTP) 行 verdict マーク + §1.3 末尾 ST-7 sub-task 8 batch verdict paragraph + chapter 04 §6.4.7 確定形書換) | (Q-NTTP) AYA 判断仰ぎ ✅ | (Q-NTTP) AYA 判断仰ぎ |

**Stage 3 entry verdict**: 全 14 項目 ✅ → **Phase 1.A 着手 ready state 到達**。

### §14.6 readiness 達成順序図

```
[Phase 2d-β-revise 完了]                  ← 本 §14 起案 + Deliverable A/B/C 反映
        ↓
[Stage 0 entry] = §14.2 全 12 ✅           ← 設計 chapter 群 (01-10) 起案完了
        ↓
[η-29 Phase 0 着手 (= sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-29)]
        ↓
[Stage 1 entry] = §14.3 全 9 ✅            ← 計測完了 + chapter 05/06a/06b/06c 反映
        ↓
[(Q1)(Q2)(Q4) AYA 判断 session]
        ↓
[Stage 2 entry] = §14.4 (Q1)(Q2) ✅         ← K 値確定
        ↓
[Phase 1.A handoff doc 起案 (= sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A entry)]
        ↓
[Stage 3 entry] = §14.5 全 14 ✅           ← Phase 1.A 着手 ready state
        ↓
[Phase 1.A 実装 開始]                     ← Codegen Python tool 起草 + CMake 統合
```

### §14.7 本 §14 と他 chapter の整合 (= 検証 pointer)

- §14.2 Stage 0 = chapter 04 / 08 / 09 起案完了確認 → 本 chapter (= 09) 内 self-consistent + chapter 04/08 への外部参照
- §14.3 Stage 1 = chapter 06a-prep §2-§6 = 既起案 + Phase 0 解禁範式 (`feedback_design_phase_no_code_write` 解除条件)
- §14.4 Stage 2 = 本 chapter §11.1-§11.5 = (Q1)-(Q5) default 提案完備
- §14.5 Stage 3 = chapter 02/04/05/06a/06b/06c/07/08/09/10 + memory `feedback_design_phase_no_code_write` の **全 doc + 全 feedback と整合**
- §14.6 達成順序 = 本 chapter §1.1 sub-step 体系 (η-29 / η-30 ...) と完全整合

### §14.8 本 §14 self-evaluation (= 設計 phase 完了判定)

本 §14 が要求する全 readiness 項目を「Phase 2d-β-revise 完了時点の自己評価」として確認:

- Stage 0 entry (§14.2): **Phase 2d-β-revise 本 session commit 時点で達成** (= 12/12 ✅ 想定、commit 前 §14.2 表 self-verify で確認)
- Stage 1 entry (§14.3): **次 session 以降の Phase 0 実機計測で達成**
- Stage 2 entry (§14.4): **Phase 0 完了直後の AYA 判断 session で達成**
- Stage 3 entry (§14.5): **2026-06-03 ST-7 sub-task 8 batch で達成 = 14/14 ✅ 全完走 = 設計 phase 完了 = Phase 1.A 実装 entry へ移行可能 state**

= **本 §14 は Phase 1.A 入口までの 1 step state checklist を提供する唯一の doc** (= Deliverable C 完成形)、Phase 1.A 着手前の最終 self-check リファレンスとして本 §14 を使用。

---

**= 本 chapter で r41 UBO migration の Phase 番号体系 (Phase 0 〜 Phase K+5) + 1 UBO ずつ migration scope + 3 OS 確証 + OpenGL 撤廃 + release 整備 + (Q1)-(Q5) AYA 判断仰ぎ候補 + §14 Phase 1.A 入口 1 step state checklist が確定**。chapter 10 (open-questions) で chapter 07 §12 chapter 10 送り (V1')(V3')(S3')(W) + 本 chapter (Q1)-(Q5) を最終判断項目として整理 → 設計 chapter 群 (01-10) 起案完了 → implementation-phase 入口 (= η-29 Phase 0) へ移行可能 state 到達。
