# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C prep (= candidate (Y))

**作成日**: 2026-06-04
**前 commit chain** (= 直前 handoff 起案):
- `35c4be1046` = Phase 1.B **complete** marker
- `60070d048b` = (X) Phase 1.A residual prep
- `5bc170579f` = (Z) AYAstorm r20 SSS verify prep
- `33f983c272` = (W) 上流 uniform4iv bug fix prep
- `fe2f3a81c6` = (X) Phase 1.A PA-B + PA-N **complete** = **Phase 1.A 章クローズ**
- `7401feeb1f` = (Z) AYAstorm r20 SSS verify **complete**
- `5aadf174f2` = (W) 上流 uniform4iv bug fix **complete**

**本 handoff doc 目的**: **Phase 1.A 章クローズ後の Phase 1.C 着手 prep**。(Q1) AYA 判断仰ぎ要件 (= 第 1 UBO 識別子 + Template A/B/C 選択) + Phase 1.C sub-task 構成案 + 持越項目 3 件 (W2 / R1 / PSC) Phase 1.C 内位置 + Phase 1.C ↔ Phase 2 境界明示 + 必読 + pinpoint reference 整理。

---

## §0 state 一行 summary

候補 (Y) Phase 1.C = **test UBO 1 個 (= Phase 2 第 1 UBO の shell 版) で 5 cadence 全経路 + `vkCmdBindDescriptorSets` 通電確認**:

- **Phase 1.A 章クローズ済** (= `fe2f3a81c6` PA-B SPIR-V cross-check 通電 + PA-N complete)
- **Phase 1.B host-side 完了済** (= `35c4be1046` 30 setter Vulkan path 分岐 + `if (mUseUBO)` runtime gate)
- **Phase 1.C 入口前提** = (Q1) 第 1 UBO 識別子 AYA 判断 + cadence + descriptor set 帯 確定が必要
- **Phase 1.C scope** = test UBO shell (= 空 struct + 空 dirty flag + 空 flush 実装) + 5 cadence 全経路 update site + `vkCmdBindDescriptorSets` 空 dummy buffer で成功 + 持越項目 3 件 (W2 sAssetUboPool prealloc / R1 ring buffer / PSC PSO cache) 実装
- **Phase 1.C Exit Criteria** = API 呼出層の通電確認、render 出力は OpenGL path のまま (= Phase 2 で実 data 流入開始)

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc | 全文 | Phase 1.C 入口 + (Q1) AYA 判断要件 + sub-task 構成案 + 持越項目 位置 + Phase 1.C ↔ Phase 2 境界 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §4.2 (Phase 1 Exit Criteria) + §4.3 (紐付け持越項目) + §5.2 (Template A/B/C) + §10.1 (chapter 07 §12 由来持越) | Phase 1.C scope + 持越項目 3 件 + Phase 2 順序選択肢 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/07-descriptor-renderpass.md` | §12 (W2 / R1 / PSC default 値) | 持越項目 3 件の default 値 + 実装方針 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/06-shader-spirv.md` | §4 (= cadence 別 flush 関数 5 種、Phase 1.C update site 設計) |
| `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` (= 09 §3.5.1 流入先) | §4 (= 該当 cadence セクション、test UBO の flush 関数選定) |
| `docs/specs/ayastorm-r41-gl-removal/design/06c-descriptor-set-bind-wiring.md` (= 09 §3.5.1 流入先) | §3 (= 接合表、test UBO の set 帯) |
| `docs/specs/ayastorm-r41-gl-removal/04-frame-context.md` (or `design/04-codegen-ubo.md`) | §5.3 (= `UniformLocation` / `CadenceTag` enum、test UBO 識別子登録) |
| `docs/specs/ayastorm-r41-gl-removal/05-existing-inventory-link.md` | §6 (= MC1 確定表、85 UBO blueprint 内訳、(Q1) 選定候補列挙) |
| `docs/specs/ayastorm-r41-gl-removal/08-llvkrenderer-skeleton.md` | §6.4 (= padded std140 size 256B 倍数 padding、shell buffer size) + §17 (持越項目別記) |
| `indra/llrender/llglslshader.cpp:2480-2563, 3006-3079` (r41 branch 上) | Phase 1.B 完了済 30 setter Vulkan path 分岐 reference (= test UBO host-side 接続点) |
| `scripts/ubo_codegen/main.py` + 90 blueprint emit 出力 (= `indra/llrender/codegen_ubo/` 配下) | Phase 1.A 完了済 codegen header 実 #include 起点 (= test UBO header gen) |

---

## §2 Phase 1.C scope 全体

### §2.1 Phase 1.C 三本柱 (= 09 §4.2 Exit Criteria literal 由来)

| 柱 | 内容 | 完了基準 |
|---|---|---|
| **(I) test UBO shell 実装** | (Q1) 確定第 1 UBO の shell 版 = 空 struct + 空 dirty flag + 空 flush 実装、zero memcpy data | shell の binding / set / size / PSO layout を **Phase 2 本実装と完全一致** (= 09 §4.2 shell ↔ 本実装 layout 互換性 literal) |
| **(II) 5 cadence 全経路 update site** | per-frame / per-pass / per-asset / per-draw / per-skin の 5 cadence 全 flush 関数で test UBO 経路成立 | 06b §4 該当 cadence セクション の flush 関数 5 種全てで test UBO に空 dummy 書込 (= 経路通電のみ確認) |
| **(III) `vkCmdBindDescriptorSets` 通電** | descriptor set 帯 (= 06c §3 接合表) で test UBO bind 成功 | 5 cadence 全経路で `vkCmdBindDescriptorSets` 空 dummy buffer で成功 = API 呼出層の通電確認、render 出力は OpenGL path のまま |

### §2.2 持越項目 3 件 (= 09 §10.1 + 07 §12 由来)

| 項目 | default 値 | Phase 1.C 内位置 |
|---|---|---|
| **(W2) `sAssetUboPool` 起動時 prealloc N=64 + grow chunk 64** | N=64 / grow=64 (07 §12 確定済) | per-asset cadence 経路実装時 (= (II) 内) |
| **(R1) ring buffer 起動時 4 MB / 上限 16 MB + cvar `AYARingBufferSizeMB`** | 4 MB 起動 / 16 MB 上限 / cvar 露出 (07 §12 確定済) | per-frame + per-pass cadence 経路実装時 (= (II) 内、ring buffer は frame-scoped data 一時退避用) |
| **(PSC) PSO cache `~/.ayastorm_x64/cache/pipeline_cache.bin`、上限 64 MB** | 64 MB (07 §12 確定済) | Vulkan pipeline 1 件作る時点 (= (III) 内、test UBO PSO layout 確定で PSC 起動初回 hit miss 想定) |

### §2.3 Phase 1.C ↔ Phase 2 境界明示 (= 09 §4.2 literal)

- **Phase 1.C Exit** = test UBO shell の 5 cadence 全経路で `vkCmdBindDescriptorSets` 空 dummy buffer で成功 (= API 呼出層の通電確認)
- **Phase 2 Entry** = 同 UBO の **実 data 流入開始** + render 出力が Vulkan path に切替 (= `mUseUBO` flag 該当 program で ON)
- **shell vs 実装の差分**: shell は Phase 1.C で **書き捨て可能** (= データ内容 = struct member 定義 / dirty 判定 / flush logic は Phase 2 で全面書換可)
- **shell の不可触 layout**: shell の **binding / set / size / PSO layout** は Phase 2 本実装と完全一致、Phase 2 で再利用される契約済構造 (= 1.C で確定 → 2 で温存)
- **判定軸独立**: Phase 1.C = API 経路通電の証明、Phase 2 = data path の証明、独立に閉じる

---

## §3 sub-task 構成案

### §3.1 sub-task table (= PC-1 ... PC-N 案、Phase 1.B PB-N と pattern 統一)

| sub-task | scope | 主体 | Exit |
|---|---|---|---|
| **PC-0** ✅ | (Q1) AYA 判断確定 (= 第 1 UBO 識別子 + Template A/B/C 選択 + cadence + descriptor set 帯) | AYA | ✅ **確定済 2026-06-04** = `UB_REFLECTION_PROBES` + per-frame + Template A + 06c §3 接合表機械決定 (§4.1 記録) |
| **PC-1** | test UBO header codegen (= scripts/ubo_codegen/ で第 1 UBO の shell blueprint emit) | Claude | `indra/llrender/codegen_ubo/<UBO 識別子>.h` 生成 + std140 size 256B 倍数 padding 確認 |
| **PC-2** | test UBO shell C++ 接続 (= Phase 1.B 完了済 setter から test UBO 識別子 + binding 取出経路成立、空 dirty flag set) | Claude | test UBO 1 個の `if (mUseUBO)` redirect 内で `forwardToUboUpload` 空 dummy buffer 書込 PASS |
| **PC-3** | (W2) `sAssetUboPool` 起動時 prealloc N=64 + grow chunk 64 実装 | Claude | per-asset cadence pool 起動時 N=64 alloc + dynamic grow chunk 64 動作 unittest PASS |
| **PC-4** | (R1) ring buffer 起動時 4 MB / 上限 16 MB + cvar `AYARingBufferSizeMB` 実装 | Claude | per-frame cadence ring buffer 4 MB alloc + cvar 反映 + 16 MB 上限 enforcement |
| **PC-5** | (PSC) PSO cache `~/.ayastorm_x64/cache/pipeline_cache.bin` 上限 64 MB 実装 | Claude | Vulkan pipeline 1 件作成時 PSC hit miss + 起動初回 cache 生成 + 64 MB 上限 enforcement |
| **PC-6** | 5 cadence 全経路 update site 実装 (= 06b §4 該当 cadence セクション flush 関数 5 種全て) | Claude | per-frame / per-pass / per-asset / per-draw / per-skin の 5 cadence 全 flush 関数で test UBO 空 dummy 書込 PASS |
| **PC-7** | `vkCmdBindDescriptorSets` 通電 (= 06c §3 接合表に従い test UBO bind 経路成立) | Claude | 5 cadence 全経路で `vkCmdBindDescriptorSets` 空 dummy buffer で成功、log fail 0 件 |
| **PC-8** | full viewer build + 起動 verify (= host-side 改変 + GLSL 0 touch 維持で既存 program 動作 unchanged) | AYA + Claude | build EXIT 0 + viewer 起動 fail 0 件 + AYA 起動目視 (= 既存描画 unchanged) |
| **PC-N** | Phase 1.C Exit Criteria 検証 + handoff complete marker | Claude | 5 cadence 全経路 vkCmdBindDescriptorSets 通電 PASS + 9 観点 self-verify + handoff doc 起案 + 1 commit |

### §3.2 strict 線形 (= PC-0 → PC-1 → ... → PC-N)

理由:
- **PC-0** = (Q1) AYA 判断確定が全 PC 起点 (= 第 1 UBO 識別子確定無しに PC-1 以降不可、09 §3.5.1 入力契約 pointer literal)
- **PC-1** (codegen header) は **PC-2** (C++ 接続) の前提
- **PC-2** test UBO shell 接続後に **PC-3..5** 持越項目 3 件 (= W2 / R1 / PSC) を並行的に実装 (= 各々独立、ただし建議は PC-3 → PC-4 → PC-5 順、cadence 出現順)
- **PC-6** 5 cadence update site は PC-3 + PC-4 + PC-5 全完了後 (= pool / ring / PSC 揃った状態で 5 cadence 全経路通電)
- **PC-7** `vkCmdBindDescriptorSets` 通電は PC-6 後 (= update site が空 dummy buffer 書込済前提)
- **PC-8** build verify は PC-7 完了後 (= 全 C++ 改変揃った後の通電 verify)
- **PC-N** Exit Criteria + handoff は PC-1..PC-8 全終了統合

### §3.3 並行可能性

- **PC-3 / PC-4 / PC-5** は **相互独立** (= W2 = per-asset pool / R1 = per-frame ring / PSC = PSO cache、別 subsystem)、Claude 単独 batch or AYA 判断で 1 件ずつ
- 残 PC-0 / PC-1 / PC-2 / PC-6 / PC-7 / PC-8 / PC-N は strict 線形 (= 上流依存あり)

---

## §4 (Q1) AYA 判断要件 (= PC-0 scope)

### §4.1 (Q1) 確定値 4 件 (= **PC-0 AYA 確定済 2026-06-04**)

| # | 項目 | **確定値** | 流入先 doc | 流入時点 |
|---|---|---|---|---|
| (Q1-a) | **第 1 UBO 識別子** | ✅ **`UB_REFLECTION_PROBES`** (= Claude 推奨 Template A 採用、AYA literal「Claude 推奨で OK」2026-06-04) | `04-codegen-ubo.md` §5.3 第 1 entry | **Phase 1.A 入口** (本来は 1.A 前確定、PC-0 で確定) |
| (Q1-b) | **第 1 UBO の cadence** | ✅ **per-frame** (= `UB_REFLECTION_PROBES` cadence、Template A 整合) | `06b-cadence-update-site-and-dirty.md` §4 per-frame セクション | **Phase 1.C 入口** (本 prep PC-0 確定済) |
| (Q1-c) | **第 1 UBO の descriptor set 帯** | ✅ **06c §3 接合表で機械決定** (= per-frame cadence row、PC-1 codegen 時に物理 set/binding 確定) | `06c-descriptor-set-bind-wiring.md` §3 接合表 | **Phase 1.C 入口** (本 prep PC-0 確定済) |
| (Q1-d) | **Template A/B/C 選択** | ✅ **Template A** (= singleton 系最小 UBO 起点、canary 検証容易、AYA literal「Claude 推奨で OK」2026-06-04) | `09-phase-roadmap.md` §5.2 | **Phase 1.C 確定済** (= Phase 2..K migration order 確定、PC-0 で確定) |

**AYA 確定 literal** (= 2026-06-04 session): 「Claude 推奨で OK」(= §4.2 Claude 推奨 Template A + `UB_REFLECTION_PROBES` 採用、根拠 3 件全受領)。

### §4.2 (Q1-d) Template 3 案 (= 09 §5.2 literal)

| Template | Phase 2 第 1 UBO | 戦略意図 |
|---|---|---|
| **A** | singleton 系最小 UBO (= `UB_REFLECTION_PROBES` 単体、per-frame、物理 instance 1 個) | canary 検証容易 (= water reflection / sky reflection 景観で確認)、low risk 起点 |
| **B** | 最頻出 per-draw UBO (= 大 risk 早期消化) | 早期に最大 risk 消化、後続 Phase 安心、ただし初動 risk 高 |
| **C** | per-frame 系 1-2 UBO (Phase 2-3 で per-frame 系完了) → per-program 系 (Phase 4-5) → per-asset 系 (Phase 6-7) → per-draw 系 (Phase 8-9) → per-skin 系 (Phase 10) | cadence 別段階消化、risk 中庸、Phase 数多 |

**Claude 推奨 (= AYA 判断仰ぐ前提): Template A** = singleton 系 `UB_REFLECTION_PROBES` 起点。根拠 3 件:
1. canary 検証 scene が分かりやすい (= water reflection / sky reflection 景観で AYA が flip 確認可能、`feedback_visual_decisions_need_live_ab` 整合)
2. 物理 instance 1 個 = pool 不要 / ring buffer 不要で migration 設計の degenerate case、初動 complexity 最小
3. per-frame cadence = update site 1 種のみ touch、Phase 1.C で 5 cadence 全経路通電試験には paralle で他 4 cadence は空 dummy のみ実装、test UBO 自体は per-frame に集中 = 試験の主軸明確

### §4.3 (Q1-a) 候補 UBO 識別子 (= 09 §5.2 + 05 §6 由来)

| 候補 | cadence | 物理 instance | 推奨理由 |
|---|---|---|---|
| `UB_REFLECTION_PROBES` | per-frame | 1 個 (singleton) | Template A 推奨、canary 易、initial risk 最小 |
| `UB_GLTF_MATERIALS` | per-asset | N 個 (asset 数依存) | Template A Phase 3、per-asset pool 試験 (W2 と整合) |
| `UB_GLTF_NODES` | per-asset | N 個 | Template A Phase 4、per-asset 大物 |
| `UB_GLTF_JOINTS` | per-skin | N 個 (rigged avatar 依存) | Template A Phase 5、skinning 経路試験 |
| その他 per-program / per-draw 系 (= 05 §6 集約表内) | 各種 | 各種 | Template B/C 採用時の候補 |

---

## §5 持越項目 3 件 Phase 1.C 内位置

### §5.1 (W2) `sAssetUboPool` 起動時 prealloc N=64 + grow chunk 64

- **default 値**: N=64 / grow chunk=64 (07 §12 確定済)
- **Phase 1.C 内位置**: per-asset cadence 経路実装時 (= PC-3)
- **依存**: Vulkan pool 初期化経路 (= chapter 07 §11 既存設計)
- **AYA 判断要件**: なし (= default 確定済、実装のみ)
- **test UBO 関連性**: (Q1) で per-asset cadence UBO 選定された場合に主軸、per-frame cadence 選定なら sParallel test (= test UBO 自体は per-frame、(W2) は他 cadence 試験用)

### §5.2 (R1) ring buffer 起動時 4 MB / 上限 16 MB + cvar `AYARingBufferSizeMB`

- **default 値**: 4 MB 起動 / 16 MB 上限 / cvar 露出 (07 §12 確定済)
- **Phase 1.C 内位置**: per-frame + per-pass cadence 経路実装時 (= PC-4、ring buffer は frame-scoped data 一時退避用)
- **依存**: Vulkan ring buffer 初期化経路 (= chapter 07 §11 既存設計)
- **AYA 判断要件**: なし (= default 確定済、実装のみ)
- **cvar 仕様**: `AYARingBufferSizeMB` debug settings 露出、起動時読込、再起動反映、default 4

### §5.3 (PSC) PSO cache `~/.ayastorm_x64/cache/pipeline_cache.bin`、上限 64 MB

- **default 値**: `~/.ayastorm_x64/cache/pipeline_cache.bin` / 上限 64 MB (07 §12 確定済)
- **Phase 1.C 内位置**: Vulkan pipeline 1 件作成時 (= PC-5、test UBO PSO layout 確定 → PSC 起動初回 hit miss → 2 回目以降 hit 期待)
- **依存**: Vulkan PSO 作成経路 (= chapter 07 §11 既存設計)
- **AYA 判断要件**: なし (= default 確定済、実装のみ)
- **path 仕様**: `~/.ayastorm_x64/cache/` 配下 (= 既存 shader_cache と同列)、初回起動 file 生成、64 MB 上限到達時 LRU 削除

---

## §6 残 strict 線形 (= 候補 (Y) Phase 1.C 着手後反映)

### §6.1 r41 milestone state (= 本 prep 起案時点)

| Phase | 状態 |
|---|---|
| Phase 1.A | ✅ 章クローズ (= `fe2f3a81c6`) |
| Phase 1.B (host-side) | ✅ complete (= `35c4be1046`) |
| (Z) AYAstorm r20 SSS verify | ✅ complete (= `7401feeb1f` + `4dde489ec4`、PR #130) |
| (W) 上流 uniform4iv bug fix | ✅ complete (= `5aadf174f2` + `2a06e12f44`、PR #131) |
| **(Y) Phase 1.C prep** | ✅ **本 doc + PC-0 (Q1) AYA 確定済 2026-06-04** |
| Phase 1.C 実装 PC-0 | ✅ 確定 (= `UB_REFLECTION_PROBES` + per-frame + Template A、AYA literal 受領) |
| Phase 1.C 実装 PC-1..PC-N | ⏳ **次 session 引継** (= AYA 指示 literal「PC-1 次 session 引継」2026-06-04) |
| (W) (b) upstream LL PR | ⏳ ayastorm-release work 時判断 (= `project_uniform4iv_upstream_pr_deferred`) |

### §6.2 Phase 1.C 着手後 strict 線形

```
✅ Phase 1.A 章クローズ
✅ Phase 1.B host-side complete
  ✅ (Z) SSS verify
  ✅ (W) uniform4iv (a) fix
  ✅ (Y) Phase 1.C prep (= 本 doc)
  ✅ PC-0 (Q1) AYA 確定 (= `UB_REFLECTION_PROBES` + per-frame + Template A、2026-06-04)
    → ⏳ **次 session 引継** PC-1 codegen (`UB_REFLECTION_PROBES` header emit)
    → ⏳ PC-2 C++ shell 接続 → PC-3 (W2) → PC-4 (R1) → PC-5 (PSC)
    → ⏳ PC-6 5 cadence update site → PC-7 vkCmdBindDescriptorSets 通電 → PC-8 build verify
    → ⏳ PC-N Phase 1.C complete marker
  → ⏳ Phase 2 (= `UB_REFLECTION_PROBES` 本実装 migration、09 §5)
```

---

## §7 self-verify (= 本 handoff 起案時点、commit 前確認)

| # | 観点 | 確認方法 | 期待 |
|---|---|---|---|
| (1) Phase 1.C scope 三本柱明文化 | §2.1 表 (I)(II)(III) Exit Criteria 連動 | ✅ |
| (2) 持越項目 3 件 (W2 / R1 / PSC) Phase 1.C 内位置 | §2.2 + §5 default 値 + Phase 1.C 内位置記録 | ✅ |
| (3) Phase 1.C ↔ Phase 2 境界明示 | §2.3 = 09 §4.2 boundary clarify literal 反映 | ✅ |
| (4) sub-task 構成 PC-0..PC-N + strict 線形 | §3.1 table + §3.2 strict 線形理由 + §3.3 並行可能性 | ✅ |
| (5) (Q1) AYA 判断要件 4 件 (a)(b)(c)(d) | §4.1 table + §4.2 Template 3 案 + §4.3 候補 UBO 列挙 | ✅ |
| (6) Claude 推奨 Template A + 根拠 3 件 | §4.2 推奨根拠 | ✅ |
| (7) 残 strict 線形反映 | §6.1 r41 milestone state + §6.2 Phase 1.C 着手後 strict 線形 | ✅ |
| (8) commit 内容 handoff doc 1 件 + Co-Authored-By 不在 | 本 commit indra/ + scripts/ + cmake/ 改変 0、Co-Authored-By 行 0 件 | ✅ (本 commit 段) |
| (9) `feedback_design_phase_no_code_write` 遵守 | 本 prep 起案 phase は design-phase 扱い、indra/ 配下改変 0、計測 hook 必要なら別 sub-task で doc 化 | ✅ |

---

## §8 引き継ぎ memory (= 次 session 着手時参照)

特に重要 (= 既存 memory から):

- `project_ayastorm_r41_vulkan_migration` (= r41 milestone state pointer)
- `project_ayastorm_r41_design_principles` (= 2 大原則: 上流取込やすさ + Core 分散実現)
- `project_r41_phase1b_vulkan_host_gate` (= GATE-B = `mUseUBO` runtime flag 単独、`#ifdef LL_VULKAN_GLSL` C++ では使わない)
- `feedback_ubo_migration_one_at_a_time` (= UBO 化作業は 1 つずつ、大塊バッチ禁止、cold launch 検証挟む)
- `feedback_handoff_minimal_pre_req_read` (= 次 session pre-req は最小 3 件 + pinpoint 別記)
- `feedback_doubt_self_first` (= AYA 報告を尊重、自分の改変を疑う)
- `feedback_self_bug_no_defer_option` (= 自作 bug 先送り禁止、ただし上流 bug 別)
- `feedback_no_scope_shrink` (= Phase 1.C literal scope = 5 cadence + 3 持越項目 全扱う)
- `feedback_release_branch_workflow` (= release branch 直 commit せず、PR 経由)
- `feedback_no_auto_commit` (= AYA 明示指示後 commit)
- `feedback_no_claude_coauthor` (= Co-Authored-By 行不在)
- `feedback_self_verify_before_handoff` (= 9 観点 self-verify 全 ✅)
- `feedback_design_phase_no_code_write` (= 本 prep 起案 phase は design-phase 扱い、indra/ 改変禁止)
- `feedback_build_only_verified` (= PC-8 build verify literal 充足、机上推論せず)
- `feedback_visual_decisions_need_live_ab` (= Phase 1.C Exit は render unchanged で AYA 起動目視 + log verify、Phase 2 で実 visual 反映)

---

## §9 次 session 着手 1 line

**「前 session で候補 (Y) Phase 1.C prep handoff doc 起案 + PC-0 (Q1) AYA 確定済 (= `UB_REFLECTION_PROBES` + per-frame + Template A + descriptor set 帯 06c §3 接合表機械決定、AYA literal「Claude 推奨で OK」2026-06-04)。本 session = **PC-1 着手** = `scripts/ubo_codegen/` で `UB_REFLECTION_PROBES` shell blueprint emit (= `indra/llrender/codegen_ubo/UB_REFLECTION_PROBES.h` 生成 + std140 size 256B 倍数 padding 確認)。PC-1 後 strict 線形 PC-2 C++ shell 接続 → PC-3 (W2 `sAssetUboPool` prealloc N=64) → PC-4 (R1 ring buffer 4 MB/16 MB + cvar) → PC-5 (PSC PSO cache 64 MB) → PC-6 5 cadence update site → PC-7 `vkCmdBindDescriptorSets` 通電 → PC-8 build verify → PC-N complete marker 着手。必読 3 件 = (1) 本 handoff doc 全文 (= §4.1 PC-0 確定値 4 件 + §3.1 sub-task table) + (2) `design/09-phase-roadmap.md` §4.2/§4.3/§5.2/§10.1 + (3) `07-descriptor-renderpass.md` §12。Phase 1.C Exit = 5 cadence 全経路で `vkCmdBindDescriptorSets` 空 dummy buffer 成功 + render 出力は OpenGL path のまま。」**
