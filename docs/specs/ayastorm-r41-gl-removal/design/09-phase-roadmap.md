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

### §2.1 Phase マップ表

| Phase 番号 | 名称 | scope 要約 | sub-step | handoff doc | 入口 Exit 判定 |
|---|---|---|---|---|---|
| **Phase 0** | 計測 phase | 06a-prep §2-§4 の (H1b)(E')(F) 実機計測 + 結果 chapter 反映 | η-29 | 起案予定 | 06a-prep §6 反映 flow 全行「反映済」 |
| **Phase 1** | codegen + redirect 層整備 | chapter 08 codegen pipeline 実装 + chapter 06a redirect 層 + 06b dirty flag + 06c descriptor set bind | η-30 (.A/.B/.C 細分) | 起案予定 | Phase 1.A/B/C 各 Exit 全 PASS |
| **Phase 2..K** | 1 UBO ずつ migration | (Q1) で確定する第 1 UBO から順に migration、各 Phase = 1 UBO (= (Q2) で cluster 許可なら例外) | η-31, η-32, ... | Phase ごと起案 | cold launch + canary + log で当該 UBO 経路成立 |

**Phase K 確定条件 (= 設計 review 2026-06-03 §3.4 K 確定明示)**: 「K」は Phase 2 から始まる migration Phase 群の最終 Phase 番号 (= 1 UBO ずつ migration を全 UBO 分続けた最後)。**K の具体数値は以下 3 条件揃ったときに確定**:
1. **Phase 0 計測結果**: 06a-prep §2 (H1b) cadence hook 計測完了で、84 + 不明 16 件 cadence 帰属 確定後の **実 migration 対象 UBO 総数** (= 推定 88-104 個) が確定
2. **(Q1) 第 1 UBO 選定**: AYA 判断で第 1 UBO 確定 → Phase 2 が確定 → migration order template (= §5.2 A/B/C) も確定
3. **(Q2) Phase 当たり migration UBO 数**: AYA 判断で cluster 許可 (= 1 Phase に 2-3 UBO 同 batch) 採否確定 → K = (UBO 総数 / Phase 当たり UBO 数) で K 値計算可能

→ K 確定は **Phase 0 Exit + (Q1)(Q2) AYA 判断 揃った時点** = Phase 1 開始前。それまで本 chapter §2/§3/§5/§6/§7/§8 の「K」「K+1」「K+2」等の表記は **暫定 placeholder** として扱う (= 確定後本 chapter §2.1 表で具体数値に置換)。
| **Phase K+1** | 3 OS 確証 (Linux) | 08 §13.4 X-α = Linux 全 UBO 動作確認 + log 検証 + sample scene 確認 | η-(K+2) | 起案予定 | Linux build pass + cold launch normal + render parity |
| **Phase K+2** | 3 OS 確証 (Windows) | 08 §13.4 X-β = Windows build + 起動 + render parity (= AYA 実機) | η-(K+3) | 起案予定 | Windows build pass + render parity |
| **Phase K+3** | 3 OS 確証 (macOS) | 08 §13.4 X-γ = macOS build + 起動 + render parity (= @t-noami 実機委任) | η-(K+4) | 起案予定 | macOS build pass + render parity |
| **Phase K+4** | OpenGL path 撤廃 | (Q3) で OpenGL 並走撤廃時期を AYA 判断、撤廃後は Vulkan のみ | η-(K+5) | 起案予定 | OpenGL path code 削除 + 3 OS build pass |
| **Phase K+5** | release 整備 | release note 起草 + tag 切り出し + AYAstorm release flow | η-(K+6) | 起案予定 | release note + tag commit |

**K = (Q1)(Q2) 確定後に決まる migration UBO 個数依存**。論理 binding 4 種 + 85 blueprint 集約結果次第で K = 5-20 程度の範囲が想定 (= cadence 別集約で同一 layout cluster を 1 Phase に纏める案を (Q2) で議論)。

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
| **1.A** | Codegen pipeline 実装 (= Python script 起草 + glslang 統合 + std140 calculator + SPIR-V reflection 二重保証 + perfect hash + cache + CMake DEPENDS) | 08 全章 | codegen script が既存 85 UBO blueprint を入力に取り、`ubo_metadata.inl` + `ubo_host_loader.inl` を生成、build error 0、生成 header の名前解決 lookup が compile-time 衝突 0 |
| **1.B** | redirect 層実装 (= 30 setter method 内部に Vulkan path 分岐 + name → offset 解決 dispatch + cache 構造 mUniformUBOLoc) | 06a §3 / §4 / §5 | 30 setter 全てで Vulkan path 分岐 working、OpenGL path 既存挙動 unchanged (= 1 setter call 1 path 決定論的、build flag で全 path 確認可能) |
| **1.C** | cadence 別 update site + dirty flag + descriptor set bind 配線 | 06b / 06c | 5 種 cadence (per-frame / per-program / per-draw / per-asset / per-skin) の update site / dirty flag / descriptor set bind が 1 経路ずつ実装、test UBO 1 個で full path 通電確認 |

### §4.2 Phase 1 Exit Criteria

- Phase 1.A: 既存 85 UBO blueprint に対する codegen 実行 PASS + 生成 header をテスト program (= 既存 program 1 個) で include + bind 不変動作確認
- Phase 1.B: 30 setter Vulkan path 分岐の **call site から見て transparent** = 既存 program 1 個の動作 unchanged
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
| 0-9 | chapter 10 open questions 集約 (= (V1')(V3')(S3')(W) + (Q1)-(Q5) + (NTTP) + (A1)(P)(G/B3)(B1)(B2)(B4)(B5) 等の登録) | grep '^### §1\\.' 10-open-questions.md で 25 件全件 index | 10-open-questions.md |
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

### §14.5 Stage 3: Phase 1.A 入口 readiness (= 着手 ready state)

**Phase 1.A 着手前の最終 self-check** (= Stage 0/1/2 全完了の必要条件 + Phase 1.A 固有 condition):

| # | 項目 | 確認方法 | 失敗時対応 |
|---|---|---|---|
| 3-1 | Stage 0 全 12 項目 ✅ | §14.2 表で全件確認 | 不足 item を §14.2 表で特定 → 該当 chapter update |
| 3-2 | Stage 1 全 9 項目 ✅ (= Phase 0 計測完了 + chapter 05/06a/06b/06c 反映済) | §14.3 表で全件確認 + 06a-prep §6 反映 flow 全行「反映済」 | 計測 task 再実施 (= chapter 06a-prep §3.x 再実施) |
| 3-3 | Stage 2 (Q1) / (Q2) AYA 判断 ✅ | 本 §14.4 表で確認 + chapter 10 (open-questions) で判断履歴登録 | AYA 判断仰ぎ session |
| 3-4 | chapter 04 §6.4 / §4.3.1 / §5.6 (= Deliverable A-1/A-2/A-3) 反映済 | grep '^#### §[456]\\.' 04-codegen-ubo.md で section 存在 + 内容確認 | Phase 2d-β-revise 本 session で起案 |
| 3-5 | chapter 08 §5.4.1 / §5.2.1 / §11.5 / §12.5 / §13.5 (= Deliverable B-1/B-2/B-3/B-4/B-5) 反映済 | grep '^#### §' 08-build-codegen-pipeline.md で 5 section 存在 + 内容確認 | 同上 |
| 3-6 | chapter 09 §14 (= 本 §) 反映済 | grep '^## §14' 09-phase-roadmap.md | 同上 |
| 3-7 | chapter 06a §3 mUniformUBOLoc cache 構造 + §5 16 method setter 分岐 設計起案済 | grep '^### §3\\.' 06a-cache-structure-and-setter-redirect.md | chapter 06a 起案 task |
| 3-8 | chapter 06b cadence 別 update site 5 種 + 06c descriptor set bind 配線 起案済 **✅ 2026-06-03 ST-6** (= `06b-cadence-update-site-and-dirty.md` 441 行 §2.1-§2.5 で 5 cadence (per-frame / per-program / per-draw / per-asset / per-skin) 全件起案済 + `06c-descriptor-set-bind-wiring.md` 513 行 §2 / §4 で descriptor set 4 帯 cadence 別配置 + flush 直後 bind 配線起案済、ST-6 前段 (a)(b) findings (= R-MAT4 `normal_matrix` 確定 + R-AYA1/2 dead / R-AYA3 alive 既移植済) は 06b §2.2 / §2.3 / §3.3 / §8 に delta integration 済) | grep '^## §' 06b/06c | chapter 06b/06c 起案 task |
| 3-9 | chapter 02 §2.4 naming + chapter 07 set 帯 5 化 / 256B padding 反映済 (= chapter 04/08 出力契約と整合) | grep '^### §2.4' 02 + '§4.4' 07 | chapter 02/07 update task |
| 3-10 | inventory + chapter 10 (open-questions) で持越項目 (V1')(V3')(S3')(W) + (A1)(P)(G/B3)(B1)(B2)(B4)(B5) + (NTTP) + (P-future)(cache-grow) 登録済 | grep 各 ID in chapter 10 | chapter 10 update task |
| 3-11 | autobuild manifest で glslang / spirv-cross / Python version pin 状態確認 (= §5.4.1.5 format pin 用意) | (Phase 1.A 入口時点で実 manifest 編集予定 = §3-12 で実施) | (Phase 1.A 内 task) |
| 3-12 | `indra/` 配下改変 解禁 = `feedback_design_phase_no_code_write` 解除点に到達 | Stage 1 で hook 配線時に既に解禁、Stage 3 entry 時点では継続改変可能 state | (常時継続) |
| 3-13 | Phase 1.A handoff doc 起案 (= 本 chapter §4 Phase 1.A scope + Exit Criteria 反映) | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B?-eta-30-phase1-a-entry.md` 起案 | Phase 1.A 入口直前 task |
| 3-14 | C++ standard 確認 (= R1 path 用 C++20 NTTP の採否 = chapter 04 §6.4.7 (NTTP) 判定材料) | AYA 判断仰ぎ予定 (= Phase 1.A 入口) | (Q-NTTP) AYA 判断仰ぎ |

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
- Stage 3 entry (§14.5): **(Q1)(Q2) 確定後の Phase 1.A 入口 handoff 起案 session で達成**

= **本 §14 は Phase 1.A 入口までの 1 step state checklist を提供する唯一の doc** (= Deliverable C 完成形)、Phase 1.A 着手前の最終 self-check リファレンスとして本 §14 を使用。

---

**= 本 chapter で r41 UBO migration の Phase 番号体系 (Phase 0 〜 Phase K+5) + 1 UBO ずつ migration scope + 3 OS 確証 + OpenGL 撤廃 + release 整備 + (Q1)-(Q5) AYA 判断仰ぎ候補 + §14 Phase 1.A 入口 1 step state checklist が確定**。chapter 10 (open-questions) で chapter 07 §12 chapter 10 送り (V1')(V3')(S3')(W) + 本 chapter (Q1)-(Q5) を最終判断項目として整理 → 設計 chapter 群 (01-10) 起案完了 → implementation-phase 入口 (= η-29 Phase 0) へ移行可能 state 到達。
