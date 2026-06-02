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

η-1 〜 η-28 は **Vulkan parse error 解消 phase** (= 84 UBO blueprint 積み上げ + bare uniform 集約 + chapter 01-08 起案) として既消化済。η-29 以降が **UBO 化 migration phase** (= host C++ redirect 層実装 + per-UBO migration + 3 OS 確証 + release) となる。

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
| **Phase K+1** | 3 OS 確証 (Linux) | 08 §13.4 X-α = Linux 全 UBO 動作確認 + log 検証 + sample scene 確認 | η-(K+2) | 起案予定 | Linux build pass + cold launch normal + render parity |
| **Phase K+2** | 3 OS 確証 (Windows) | 08 §13.4 X-β = Windows build + 起動 + render parity (= AYA 実機) | η-(K+3) | 起案予定 | Windows build pass + render parity |
| **Phase K+3** | 3 OS 確証 (macOS) | 08 §13.4 X-γ = macOS build + 起動 + render parity (= @t-noami 実機委任) | η-(K+4) | 起案予定 | macOS build pass + render parity |
| **Phase K+4** | OpenGL path 撤廃 | (Q3) で OpenGL 並走撤廃時期を AYA 判断、撤廃後は Vulkan のみ | η-(K+5) | 起案予定 | OpenGL path code 削除 + 3 OS build pass |
| **Phase K+5** | release 整備 | release note 起草 + tag 切り出し + AYAstorm release flow | η-(K+6) | 起案予定 | release note + tag commit |

**K = (Q1)(Q2) 確定後に決まる migration UBO 個数依存**。論理 binding 4 種 + 84 blueprint 集約結果次第で K = 5-20 程度の範囲が想定 (= cadence 別集約で同一 layout cluster を 1 Phase に纏める案を (Q2) で議論)。

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

---

## §4 Phase 1: codegen + redirect 層整備 (η-30)

### §4.1 sub-Phase 構成

| sub-Phase | scope | 該当 chapter | Exit 判定 |
|---|---|---|---|
| **1.A** | Codegen pipeline 実装 (= Python script 起草 + glslang 統合 + std140 calculator + SPIR-V reflection 二重保証 + perfect hash + cache + CMake DEPENDS) | 08 全章 | codegen script が既存 84 UBO blueprint を入力に取り、`ubo_metadata.inl` + `ubo_host_loader.inl` を生成、build error 0、生成 header の名前解決 lookup が compile-time 衝突 0 |
| **1.B** | redirect 層実装 (= 30 setter method 内部に Vulkan path 分岐 + name → offset 解決 dispatch + cache 構造 mUniformUBOLoc) | 06a §3 / §4 / §5 | 30 setter 全てで Vulkan path 分岐 working、OpenGL path 既存挙動 unchanged (= 1 setter call 1 path 決定論的、build flag で全 path 確認可能) |
| **1.C** | cadence 別 update site + dirty flag + descriptor set bind 配線 | 06b / 06c | 5 種 cadence (per-frame / per-program / per-draw / per-asset / per-skin) の update site / dirty flag / descriptor set bind が 1 経路ずつ実装、test UBO 1 個で full path 通電確認 |

### §4.2 Phase 1 Exit Criteria

- Phase 1.A: 既存 84 UBO blueprint に対する codegen 実行 PASS + 生成 header をテスト program (= 既存 program 1 個) で include + bind 不変動作確認
- Phase 1.B: 30 setter Vulkan path 分岐の **call site から見て transparent** = 既存 program 1 個の動作 unchanged
- Phase 1.C: test UBO 1 個 (= 後の Phase 2 で本実装する第 1 UBO の試作版、本実装は Phase 2、ここでは shell のみ) で per-cadence update + descriptor bind 通電

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

(Q1) 第 1 UBO 選定方針 (= AYA 判断仰ぎ候補) 別の order template:

**Template A: 最小リスク UBO 優先**:
1. Phase 2 = singleton 系最小 UBO (= `UB_REFLECTION_PROBES` 単体、per-frame cadence、物理 instance 1 個)
2. Phase 3 = per-Asset 系最小 UBO (= `UB_GLTF_MATERIALS` 単体)
3. Phase 4 = per-Asset 系大物 UBO (= `UB_GLTF_NODES`)
4. Phase 5 = per-Skin 系 UBO (= `UB_GLTF_JOINTS`)
5. Phase 6.. = bare uniform 集約由来の per-program UBO 群 (= chapter 05 集約表で集約された UBO を頻度低い順)
6. Phase K = 最後に最頻出 per-draw 系 UBO (= 大 risk、最後に migration)

**Template B: 最頻出 UBO 優先**:
- Phase 2 = 最頻出 per-draw UBO (= 大 risk 早期消化)
- 以降は影響度大きい順で migration
- Phase K 終盤に singleton 系の少 risk UBO

**Template C: cadence 系統別 batch**:
- Phase 2-3 = per-frame 系 (= 1-2 UBO)
- Phase 4-5 = per-program 系
- Phase 6-7 = per-asset 系
- Phase 8-9 = per-draw 系
- Phase 10 = per-skin 系

**default 提案 = Template A** (= 最小リスク UBO 優先): cold launch 検証で経路が成立しない場合の影響範囲が最小、Phase 1 で実装した codegen + redirect 層 + cadence 別 update site の各経路を **少 risk な UBO で 1 経路ずつ通電** することで、後続の大 risk UBO 移行時には残り経路差分のみが新規 path となる。

### §5.3 Phase Exit Criteria (= 各 Phase 共通)

- Linux cold launch normal (= crash 0 / shader compile error 0 / render normal)
- 該当 UBO を経路上に持つ既存 scene で **render parity** = OpenGL path と Vulkan path で screen diff が visible 差以下 (= AYA 目視確認)
- LL_DEBUGS log で該当 UBO bind / flush 回数が想定範囲内
- canary cvar flip で render 反映確認
- 検証用 LL_DEBUGS log は **commit 前に必ず除去** (= memory `feedback_remove_verification_logs`)
- handoff doc (= 1 Phase 1 件) 起案 + 次 Phase prerequisite 化

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

### §11.2 (Q2) Phase 当たり migration UBO 数

| 選択肢 | 内容 | 根拠 |
|---|---|---|
| A | 1 UBO 厳守 | memory `feedback_ubo_migration_one_at_a_time` 厳格 |
| B | 関連 cluster で 2-3 UBO 同 Phase 許可 | 同一 owner / 同一 cadence の cluster は 1 Phase で扱う |

**default 提案 = A (1 UBO 厳守)**: feedback memory 直接準拠、cluster 化は cold launch 検証で REJECT 時の切り分け困難化リスク。
**例外**: (Q1) で同一 owner UBO 群 (= `UB_GLTF_NODES` / `UB_GLTF_MATERIALS` / `UB_GLTF_JOINTS` は全て `gltf::Asset` / `gltf::Skin` owner) を識別、cluster 単位を AYA 判断で例外許可するかは Phase 0 完了後の (Q1) 確定タイミングで再評価。

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

**= 本 chapter で r41 UBO migration の Phase 番号体系 (Phase 0 〜 Phase K+5) + 1 UBO ずつ migration scope + 3 OS 確証 + OpenGL 撤廃 + release 整備 + (Q1)-(Q5) AYA 判断仰ぎ候補が確定**。chapter 10 (open-questions) で chapter 07 §12 chapter 10 送り (V1')(V3')(S3')(W) + 本 chapter (Q1)-(Q5) を最終判断項目として整理 → 設計 chapter 群 (01-10) 起案完了 → implementation-phase 入口 (= η-29 Phase 0) へ移行可能 state 到達。
