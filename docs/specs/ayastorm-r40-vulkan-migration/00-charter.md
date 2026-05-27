# AYAstorm r40 章 charter — Vulkan 化選択ストーリー + 工程プラン

**status**: **closed 2026-05-28** (work item (a)-(e) 全完了 + AYA review PASS、**r40 達成 = 工程プラン完成**、r41 charter 起草へ移行)
**起草**: 2026-05-28
**達成条件**: Vulkan 化作業の工程プラン (設計および工程想定) が完成した段階
**親 memory**: `project_ayastorm_r40_cpu_parallel.md`

---

## 1. 章 thesis

「**色々検討した結果、最終的に Vulkan 化を選択するに至ったストーリーと、その Vulkan 化作業の工程プランを含む 1 章。読めば「なぜ Vulkan 化が必要になったか」の経緯が追える。**」

r40 章は AYAstorm の CPU/GPU パフォーマンス余白枯渇に対する複数案の検討記録 + 最終的に Vulkan 化を選択した経緯 + その Vulkan 化全工程のプラン (設計および工程想定) を 1 つの章として包摂する。r41 以降は本章で確定した工程プランに沿って milestone 単位で実装する。

## 2. 章スコープ定義

r40 章は **3 つの sub-phase** を包摂する。

| sub-phase | 内容 | status |
|---|---|---|
| **1** | CPU perf 案 (剥がし候補 4 件 O/Q/R-refined/P-refined) の検討 | closed (2026-05-27 全 REJECT) |
| **2** | LL Vulkan 着地待ち + ad-hoc 鉱脈発掘案 (旧 r40+ extended) の検討 | closed (2026-05-28 鉱脈ゼロで falsify) |
| **3** | Vulkan 化選択 + 工程プラン策定 | active |

**r40 達成条件** = sub-phase 3 完了 = 工程プラン (設計および工程想定) doc 化完成。達成時に r41 (GL 依存除去 + Vulkan 空転) milestone へ移行。

## 3. 経緯

### sub-phase 1: CPU perf 案 (closed 2026-05-27)

- Phase 1.1 で計測 infra (AYAPerfLog) + Layer 1-8 zone 配線 + deep-dive (02 §A1-A14) を完成、剥がし候補 4 件に絞込
- Phase 2 で全候補 REJECT:
  - 案 O (doOcclusion async): commit `f695722a07` 実装後 gate REJECT、default OFF 出荷
  - 案 R-refined (renderShadow pre-cull): pre-impl REJECT、worker-safe 上限 0.25 ms/frame (gate 1.33 ms に 84% 不足)
  - 案 P-refined (parallel cull): pre-impl REJECT、worker-safe 上限 0.29 ms/frame (gate 1.33 ms に 78% 不足)
  - 案 Q (vwDraw text cache): 実測 0.5% で DROP
- Root cause 確定: pipeline.cpp の 3 大グローバル (`sCull` / `sShadowRender` / `sCurCameraID`) + cull/stateSort 内 GL 呼出 (`checkOcclusion`) と geometry mutation (`rebuildMesh` / `markOccluder`) が render path 全体の並列化を阻止 → **OpenGL viewer の main 剥がしは構造的に不可能**
- 詳細 → `01-sub-phase-1-cpu-perf.md`

### sub-phase 2: LL 待ち + 鉱脈発掘案 (closed 2026-05-28)

- sub-phase 1 直後の 2026-05-27 起動 (CPU perf 案も自前 Vulkan 案も両方 drop した時点での代替)
- thesis: 「LL Vulkan が完成するまで本線を延命する。自前 Vulkan は作らない、体系的 cache も入れない、ad-hoc な明らかに無駄を 1 件ずつ削って 5% を 1-2 年で積み上げる」
- 2026-05-28 AYA さん 1 日かけて鉱脈調査 → **鉱脈ゼロ確定**、延命前提が falsify
- 帰結: 「鉱脈発掘で延命可能」の前提が崩壊 → 消去法で Vulkan 化以外に次の進化を乗せる手段なし
- 詳細 → `02-sub-phase-2-extended-falsify.md`

### sub-phase 3: Vulkan 化選択 + 工程プラン策定 (active 2026-05-28〜)

- sub-phase 1 / 2 双方の falsification が「Vulkan 化以外に道なし」を結論として絞り込み、Vulkan 化を正式に選択
- 旧 r41 自前 Vulkan migration 案 (2026-05-27 drop) の drop 論拠 4 点を再評価、(1) 中継ぎ性質 と (4) dual maintenance cost が論理崩壊 ((2) 完遂事例ゼロ と (3) 工数 6-15 人年 は残るが「やらない理由ではない」と整理)
- AYA 確定: 「**消去法でマルチプロセスを達成するには LL に関係なく Vulkan 化以外に次の進化を乗せられない**」
- 工程プラン策定の擦り合わせ (§4 の 8 確定事項) を 2026-05-28 完了
- 残作業 = Vulkan portage 棚卸し + 設計 + 工程算定 + r42+ 区切り確定 → §9 work breakdown
- 詳細 → `03-sub-phase-3-vulkan-plan.md` (active 2026-05-28〜)

## 4. 工程プラン確定事項 (2026-05-28 AYA 擦り合わせ完了)

| # | 要点 | 確定値 |
|---|---|---|
| 1 | 完遂 goal | **parity 完遂** — vk-RC 相当、AYAstorm r1-r30 全機能を Linux+Win+Mac 全部で Vulkan 上に再現するまで章を閉じない |
| 2 | scope | **Linux 先行 → Win/Mac 後追い**、GL 完全削除。Vulkan portage critical path = **C++ ~63K LOC** (llrender 28K / pipeline.cpp+.h 16K / lldrawpool 8K / llspatialpartition 4K / llviewershadermgr 4K / llvosky+llvowlsky 2K) + **GLSL shader 248 file** + **GL header 依存 189 file (99% は llgl.h wrapper 経由)**。wrapper 局在化により llglheaders.h + llglstates.h + llgltypes.h の **3 file 置換 + volk loader 導入で 188 file の上流 file は変更不要** = abstraction 設計の最大の追い風。詳細 → `04-portage-inventory.md` §6 |
| 3 | time horizon | **無期限 / AYA life plan** — 6-15 人年規模を charter 明記、撤退条件は時間軸では設けない |
| 4 | branch 戦略 | **2 phase 構成** — Phase 1 (r41 達成まで): 本線 `ayastorm-release` 内 long-lived feature branch 群 / Phase 2 (r41 達成後 = r41.5): AYAstorm VK repo 新規 git init で立ち上げ + 物理分離 (dynamic link、LGPL 法的分離達成) |
| 5 | LL 着地時 reset | **その時点で判断** — charter で固定せず、AYAstorm-vk 進捗 × LL 公式の質的評価で reset / maintain / merge を選別 (判断指針は §7) |
| 6 | 本線 r40 close 凍結保守 | **critical security/crash bug + minor bugfix + AYAstorm r1-r30 系統 bug 周辺の小改善 OK**、新章不可 |
| 7 | r14+ visual realism suspend | **特例 drop** — (6) ルールに包摂で十分、独立扱い不要 |
| 8 | r42+ ロードマップ | **描画 stage 単位** で区切る方針 (旧 charter vk-β/γ/δ/RC 流れ継承)、細かい区切りは **work item (d) で確定済 2026-05-28** (詳細 §6 正式 line up + `07-r42-plus-milestone-mapping.md`) |

### 各確定値の含意

#### (1) parity 完遂 — 完遂事例ゼロ問題の正面取扱

- viewer fork で Vulkan 完遂事例ゼロ (Doom 2016 = 3 名 6-12 か月 / Blender Vulkan = 2 名 2019 着手 → 2026 現在 7 年未完 / AYAstorm 体制 = 1 人 本職並走)
- 完遂前提だが事例ゼロを承知の上で着手、time horizon (3) の無期限 + branch (4) の本線同居 + LL 着地時の柔軟判断 (5) で完遂前事故率を下げる設計
- 各 milestone (r41 / r42 / ...) を独立 release として shippable に保つ (parity 未完遂でも各段階を user に届けられる状態を維持) → 完遂未達でも user 価値を積み上げる構造

#### (2) Linux 先行 — 既存「3 OS 大前提」memory の明示指示要件を満たす例外

- vk-α (r41) 〜 vk-δ までは Linux のみで開発進行
- vk-RC (parity 完遂) 直前に Win 移植 → t-noami さん Mac 移植のスポット依頼
- 3 OS 完遂自体は維持 (vk-RC = 3 OS parity)、先行/後追いの順序問題に限定
- Win/Mac user は vk-RC 直前まで本線 OpenGL を使用 = **6-15 年スパンの本線継続使用**、本線凍結保守 (6) で Win/Mac user の期待値に最低限応える

#### (3) 無期限 / AYA life plan — 6-15 人年規模の charter 明記

- 工数算定参照点:
  - Doom 2016 (id Tech 6): 3 名 6-12 か月 (clean abstraction あり)
  - Blender Vulkan: 2019 着手 → 2026 現在 7 年未完 (近似 scale / GPU module 先行整備済)
  - AYAstorm 規模: **6-15 人年** (abstraction 不在 + 3 大グローバル + 248 shader)
  - 1 人 full-time 換算で物理 6-15 年、本職並走なら 15-30 年
- 撤退条件は時間軸では設けない、plan B trigger は外部条件のみ (§8)
- 5 年 / 10 年 / 終わるまでのスケールを許容
- **a-3 / a-4 段階 port 戦略 工数感との関係**: a-3 §5.4 で段階 port 戦略 (base LL port 5 段階 + AYAstorm 3 機能合成順) の **フルタイム dev / 経験者前提 概算工数感** が **合計 7-8 人月** (base 4-5 + AYAstorm 3) と出ているが、本 (3) 6-15 人年は **AYA 本職並走 / Vulkan 初見前提**。乖離理由 = 並走係数 3-5x + 学習曲線 + 不確実性 2-3x。a-3/a-4 工数感は段階順序 + 概算オーダー確認の目的のみ、絶対値の精緻化は work item (c) 工程算定で実施 (`04-portage-inventory.md` §5.4.3 / §6.3)

#### (4) branch 戦略 (2 phase 構成)

**Phase 1: r41 達成まで = 本線同居** (作業の単純さ優先 / 空転達成までの分離複雑化を回避)

- GitHub 二次 fork 制約: phoenix-firestorm fork である AYAstorm から GitHub UI 上で通常 fork は不可
- r41 達成 (GL 除去 + Vulkan 空転) までは本線 `ayastorm-release` で long-lived feature branch 群を持続:
  - 例: `feature/vulkan-foundation` / `feature/vulkan-gl-removal` / `feature/r41-vulkan-context` / `feature/r41-shader-port` 等
  - branch 命名規則は 03-sub-phase-3-vulkan-plan.md で確定
- release tag は `r40-final` (本線 close 後の最終 tag) と `vk-*` / `r41` (Vulkan 化系統) が同 repo に共存
- CI/CD は branch filter で分離 (本線 branch 群と Vulkan 化 branch 群で別 workflow)

**Phase 2: r41 達成後 = AYAstorm VK repo へ物理分離** (LGPL 法的分離達成)

- **AYAstorm VK repo を新規 git init で立ち上げ** (phoenix-firestorm 二次 fork ではなく完全独立 repo、GitHub 二次 fork 制約に依らない)
- Vulkan code (描画エンジン部分) を本線 `ayastorm-release` から VK repo へ **directory 単位で移動**
- 本線 = LGPL 維持 (GUI / chat / picker / scene graph / 等)、VK repo = 独自 license (詳細は r41.5 charter で擦り合わせ)
- **dynamic link 構成**: 本線 binary (LGPL) と VK repo binary (独自 license) を dynamic link で結合 (= LGPL combined work 回避)
- ビルド統合: 本線 build script から VK repo を fetch + build + link
- 詳細工程 → §6 r41.5 milestone
- 法的分離達成の condition (本 doc §「LGPL 法的分離 condition」予定): 独立 binary / dynamic link / interface header のみ公開 / 一方向依存 (GUI → VK のみ) / ユーザーが VK layer 入れ替え可能

#### (5) LL 着地時 reset — その時点で判断

- charter で reset 方針を固定しない理由: 着地時期 (vk-α 前 / 中盤 / parity 完遂後) で work loss と LL 採用 merit が大幅に変動
- 判断指針は §7 で展開

#### (6) 本線凍結保守範囲 — 「軽微」境界線の詳細化

- 「軽微」の境界線は 03-sub-phase-3-vulkan-plan.md で詳細化:
  - LOC 閾値 (例: 1 commit 500 LOC 未満 / 1 PR 2000 LOC 未満)
  - 影響範囲閾値 (例: 単一 feature module 内、3 OS build 一発 PASS)
  - 工数閾値 (例: Claude/AYA 工数 8 時間以内、t-noami さん依頼なし)
- 新章開始 (r31 / r32 等の新規 chapter) は不可、軽微改善のみ
- Claude/AYA リソース配分は Vulkan 化 (r40 章) 優先、本線軽微改善は余力配分

#### (8) r42+ ロードマップ — 描画 stage 単位、work item (d) で確定済

- 旧 charter の vk-β/γ/δ/RC 流れを継承:
  - vk-β = 静止 scene + avatar (no shadow, no light)
  - vk-γ = deferred lighting + 基本 material
  - vk-δ = reflection probe / SMAA / SSAO / DoF / shadow cascade
  - vk-RC = AYAstorm r1-r30 全機能 parity
- AYAstorm 固有機能 (r1-r30) は描画 stage で必要になったタイミングで pull-in (audio r1-r13 は描画 stage 非依存なので並行 port 可能、視覚表現 r14-r24 と Cinematic r30 は描画 stage と密接結合)
- 細かい r42 / r43 / r44 / r45+ の区切りは work item (d) で 2026-05-28 確定済 (§6 正式 line up 参照、`07-r42-plus-milestone-mapping.md` §1-§5 で詳細)

## 5. r41 達成基準

**r41 達成 = GL 依存除去 + Vulkan 空転 (描画は最低限)**

### GL 依存除去 (a-4 棚卸し final 反映)

C++ critical path 約 63K LOC + GLSL shader 248 file の完全置換:

- `indra/llrender/` 配下 51 files (header 25 + source 26) / 28.2K LOC / 381 GL calls の Vulkan 等価実装への完全置換 (header wrapper 3 file 含む)
- `indra/newview/pipeline.cpp + .h` 15.9K LOC の Vulkan 化 + 3 大グローバル (`sCull` / `sShadowRender` / `sCurCameraID`) の frame context 集約 (LLPipelineFrameContext 仮称)
- `indra/newview/lldrawpool*.cpp` 13 file / 7.9K LOC の Vulkan command buffer 化 (terrain.cpp glTexGen → shader 側 explicit UV)
- `indra/newview/llspatialpartition.cpp` 4.4K LOC の geometry rebuild + occlusion 再設計 (occlusion query → VkQueryPool)
- `indra/newview/llviewershadermgr.{cpp,h}` 4.4K LOC の shader manager Vulkan 化
- `indra/newview/llvosky.cpp + llvowlsky.cpp` 2.2K LOC の sky dome + atmospherics Vulkan 化 (r14+ visual realism 関連)
- GL header 依存 189 file の Vulkan header 移行 — **99% は llgl.h wrapper 経由**、llglheaders.h + llglstates.h + llgltypes.h の 3 file 置換 + volk loader で 188 file の上流 file は変更不要
- GLSL shader 248 file の SPIR-V 移行 (glslang/spirv-cross 半自動 + descriptor set 再設計 / sampler 206 個収容)
  - compute / geometry / tessellation / bindless / atomic ゼロ → cross compile で ~85% 素直に通る見込み

詳細 → `04-portage-inventory.md` §6.1 / §6.2 / §6.3

### Vulkan 空転 (描画は最低限)

- Vulkan instance + physical device + logical device + queue 取得
- swapchain + image / image view
- render pass + framebuffer (最低限の color + depth)
- command buffer recording + submission
- frame in flight 同期 (fence + semaphore)
- 描画は **最低限**: viewer window が立ち上がり、何らかの描画 (黒画面 + UI 程度) が出る、segfault せず frame loop が回る
- AYAstorm 固有機能の port は r42+ で行う、r41 では parity 不要

### r41 non-scope

- vk-γ 以降の機能 (deferred lighting / material / shadow / SSAO / DoF / 等)
- AYAstorm r1-r30 全機能 (audio / 視覚表現 / Cinematic / chat / picker 等)
- visual feature 拡張 / shader 効果拡張 / perf tuning

詳細な acceptance criteria は r41 着手時に `docs/specs/ayastorm-r41-gl-removal/00-charter.md` で詳細化。

## 6. r42+ ロードマップ方針

§4 (8) 確定の通り **描画 stage 単位** で区切る。旧 charter の vk-β/γ/δ/RC 流れを継承、AYAstorm 固有機能 (r1-r30) は描画 stage 必要時に pull-in。

### 正式 line up (work item (d) 確定 2026-05-28)

| 正式 milestone | 描画 stage | AYAstorm 機能 pull-in | repo 構成 | base PM | 暦月マーカー |
|---|---|---|---|---|---|
| r41 | vk-α (空転) | (なし、parity 不要) | 本線同居 (Phase 1) | 16.17 | ~2033 年中 |
| r41.5 | (描画 stage 進行なし、構造 refactor のみ) | Vulkan code abstraction 化 + VK repo 分離 | VK repo 新規立ち上げ (Phase 2 開始) | 1.50 | ~2034 年初 |
| r42-α | vk-β 着手 (render pass attachment + read-pick) | **r21.1 self-rigged picker port** | 本線 + VK repo (dynamic link) | 0.65 | ~2034 年前半 |
| r42-β | vk-β 完遂 + vk-δ 部分 (DoF + state enum 化) | **r30 Cinematic mode port** | 本線 + VK repo | 3.15 | ~2035 年中 |
| r42-γ | vk-γ + vk-δ 部分 (lighting + post-process chain) | **r14+ visual realism port** | 本線 + VK repo | 3.18 | ~2036 年後半 |
| r42-δ | vk-δ 完遂 + parity 残機能 polish | r25-r29 3D stream + r1-r13 audio 確認 + regression sweep | 本線 + VK repo | 2.25 | ~2037 年中 |
| r43 | vk-RC (parity 補強 + 性能 polish 一部) | Linux baseline 安定維持 + **Win parity 完遂** (Win driver matrix + 旧 driver fallback + WHCK 認定) | 本線 + VK repo | 2.63 | ~2038 年中 - 2039 年初 |
| r44 | vk-RC 完遂 (parity 補強 + 性能 polish + Mac portable subset 詳細化) | **Mac parity 完遂** (MoltenVK + UMA + MSL + t-noami workflow) = **vk-RC 達成** = §4 (1) 完遂 goal 到達 | 本線 + VK repo | 6.30 | ~2039 年初 - 2040 年後半 |
| (vk-RC 達成 = r44 達成 = r40 章工程プラン完遂 marker) | — | — | — | **35.83** (累積) | **~2040 年後半** (累積 ~170 暦月 / ~14.2 年) |
| r45+ | (本算定範囲外、別章 charter で扱う) | visual realism 次世代 / ray tracing / HDR / GPU-driven / AYAstorm 独自進化 | (TBD) | — | — |

**注**: 上記は work item (d) 確定の正式 line up (`07-r42-plus-milestone-mapping.md` §1-§4 結論)、(a) Vulkan portage 棚卸し + (b) Vulkan API 設計 + (c) 工程算定 + (d) r42+ 区切り確定 の出力統合反映。r45+ は本算定範囲外 (本 §6 末尾「r45+ 範囲外 + 別章 charter 起草指針」sub-section 参照、`docs/specs/ayastorm-r45-plus-xxx/00-charter.md` 別章で扱う)。

### a-4 棚卸しで確定した AYAstorm 機能 pull-in 順 (work item (d) で正式 mapping 確定済 2026-05-28)

work item (a) a-4 §6.3.2 で touchpoint 規模順 + 独立度順を確定 (合計 20-30 call、base portage の 0.03% 未満で **局在性確定**、base LL port 完成後の modular patch 合成可能)。work item (d) `07-r42-plus-milestone-mapping.md` §1.3 で描画 stage × AYAstorm 機能 × milestone 三軸 mapping を正式確定:

| milestone | 描画 stage (主) | 描画 stage (副) | AYAstorm 機能 | 進行範囲 |
|---|---|---|---|---|
| r42-α | **vk-β 着手** | — | **r21.1 self-rigged picker** | 静止 scene + avatar + render pass attachment (mObjectIDBuffer) + read-pick path、shader 2 file SPIR-V 化 |
| r42-β | **vk-β 完遂** | vk-δ 部分 (DoF) | **r30 Cinematic mode** | scene + avatar 完成 + DoF state enum 化 + post-process pass 着手、shader 4 file SPIR-V 化 |
| r42-γ | **vk-γ 着手 + vk-γ 進行** | vk-δ 部分 (post-process chain) | **r14+ visual realism** | deferred lighting + sky dome (llvosky + llvowlsky) + atmospherics + post-process chain 7 sub-pass、shader 7 file SPIR-V 化 |
| r42-δ | vk-γ 完遂 + **vk-δ 完遂** | vk-RC 直前 polish | r25-r29 3D stream + r1-r13 audio 確認 | reflection / SMAA / SSAO / shadow cascade 完遂 + parity 残機能 polish + regression sweep |

**含意**:

- 描画 stage は r42 内で 4 段階進行 (vk-β 着手 → vk-β 完遂 + vk-δ 部分 → vk-γ + vk-δ 部分 → vk-γ + vk-δ 完遂)、旧仮 line up の「r42 = vk-β / r43 = vk-γ / r44 = vk-δ」より **r42 内で大半の stage が進む**
- AYAstorm 機能 port = 描画 stage の trigger (r21.1 picker = vk-β 着手 trigger / r30 Cinematic = vk-β 完遂 + DoF trigger / r14+ visual realism = vk-γ + vk-δ 完遂 trigger)
- r43-r44 = vk-RC (parity 補強 + 性能 polish + 3 OS parity 完遂) に振替え (本 §6 冒頭の正式 line up 表参照)

### r41.5 milestone (新規追加 2026-05-28)

r41 達成 (GL 除去 + Vulkan 空転) 直後の **構造 refactor milestone**。描画 stage は進行しないが、以下を達成する:

- **Vulkan code の abstraction 化**: pipeline.cpp 等の Vulkan API 直接 call を interface 経由 call に置換
- **AYAstorm VK repo の新規立ち上げ** (GitHub 二次 fork 制約に依らない完全独立 git init)
- **物理分離 = directory 単位移動**: Vulkan layer (描画エンジン) を本線 `ayastorm-release` から VK repo へ移動
- **dynamic link 構成**: 本線 binary (LGPL) と VK repo binary (独自 license) を dynamic link で結合 (LGPL combined work 回避 → 法的分離達成)
- **ビルド統合**: 本線 build script から VK repo を fetch + build + link

VK repo の license 戦略 (proprietary / open-source MIT / Apache 2.0 / 等) は r41.5 charter (= 本 charter とは別 doc、r41 達成後に起草) で詳細化、AYA さんと擦り合わせ確定する。

#### r41.5 のメリット (charter 採用根拠)

1. **空転までの作業を単純に保つ** — r41 までは分離設計を考えず、Vulkan 空転に集中。空転達成可否がまだ不確実な段階で分離複雑化を持ち込まない判断
2. **動作確認のしやすさ** — Vulkan 空転動いた状態で分離 refactor、segfault / regression 切り分けやすい
3. **LL UI 変更時の defensibility** — LL 公式 VK が AYAstorm 非互換 UI を伴って着地した場合、r41.5 分離後なら VK engine だけ LL 公式に差替えて AYAstorm GUI 維持可 (§7 判断軸 3 参照)

#### r41.5 の cost (charter 認識根拠)

- r41 達成後の追加 refactor 工数 (Vulkan code abstraction 化 + directory 移動 + dynamic link 化)
- 後付け分離なので一部 code 重複 / interface 設計変更が発生
- ただし「空転達成」という大きな milestone の後、分離可否が確定した状態での refactor なので work-loss risk は低い

#### r41.5 charter 起草 cadence (work item (d) §5.2 反映 2026-05-28)

- **起草 timing**: r41 達成宣言直後 (~2033 年中、`07-r42-plus-milestone-mapping.md` §5.2)
- **起草主体**: AYA + Claude (法的 review 関与で AYA 比重大、必要なら外部法務 advice)
- **起草先**: `docs/specs/ayastorm-r41-5-vk-repo-separation/00-charter.md`
- **工数 + 暦月**: ~1.50 PM / ~7.2 暦月 (06 doc §3.10 + §5.3、Linux first-class 並走 ratio 4x 反映)
- **acceptance criteria draft**: work item (d) §5.2 反映 8 件 (Vulkan code abstraction 化 / VK repo 新規立ち上げ / 物理分離 / dynamic link 動作 / ビルド統合 / 法的分離 / LL UI 変更時 defensibility 確保 / regression 無し)
- **詳細化**: charter 起草時に LLVKRenderer interface signature + VK repo directory 構造 + VK repo license 戦略 + LL UI 変更時 defensibility 詳細を AYA + Claude で確定 (work item (d) outline は base)

### r45+ 範囲外 + 別章 charter 起草指針 (work item (d) §4 反映 2026-05-28)

本 r40 章工程プランは vk-RC parity 完遂 (= r44 達成、~2040 年後半 / ~170 暦月) までを算定範囲、r45+ は本算定範囲外。詳細は `07-r42-plus-milestone-mapping.md` §4 参照。

#### r45+ scope broad outline

| topic 領域 | 内容 broad outline |
|---|---|
| visual realism 次世代 | r14+ 章 thesis 「写真を撮るに値する空気と空間」の next iteration、AYAstorm 独自進化路線 |
| ray tracing | `VK_KHR_ray_tracing_pipeline` + `VK_KHR_acceleration_structure` 活用、reflection / shadow / GI 等の hardware ray tracing 実装 (Mac MoltenVK 非対応のため 3 OS parity 対象外、Linux/Win first-class) |
| HDR (High Dynamic Range) | 10-bit / 12-bit per channel HDR display 対応 + HDR-aware tonemap + monitor calibration |
| GPU-driven rendering | indirect draw / draw call merging / GPU-side culling + scene graph traversal、`VK_EXT_mesh_shader` 活用での mesh shader pipeline 導入 |
| AYAstorm 独自進化 | r14+ 章を含む AYAstorm 独自路線の自由扱い (§3 「parity 完遂後の self-driven 章」) |

#### r45+ 着手 trigger

- **必須 trigger**: r44 達成 (= vk-RC parity 完遂 = §4 (1) 完遂 goal 到達) + AYA judgment (§3 「AYA life plan」前提)
- **任意 trigger**: §7 LL 着地時判断指針 + §8 plan B trigger との連動 (§7 判断軸 1 「vk-RC 後」では reset cost 最大 / LL 採用 merit 低 → (ii) maintain + AYAstorm 独自路線 が r45+ scope に重なる)
- **時間軸 trigger 無し** (§3 「時間軸では撤退条件を設けない」遵守)

#### r45+ charter 起草 cadence

- **起草 timing**: r44 達成宣言 + 6 か月以内に AYA 擦り合わせ開始 (§3 時間軸非設定遵守下の合理的擦り合わせ期間)
- **起草主体**: AYA 主体 (scope 判断) + Claude 補助 (technical draft + memory / charter / doc cross-reference)
- **起草先**: `docs/specs/ayastorm-r45-plus-xxx/00-charter.md` (xxx は scope による、例: `r45-plus-raytracing` / `r45-plus-hdr` / `r45-plus-gpu-driven` 等の分章可能)
- **本 charter (00-charter.md) との関係**: r45+ は別章 charter で扱う、本 charter §6 では broad outline のみ反映 (本 r45+ section)

## 7. LL 着地時判断指針

LL 公式 Vulkan が phoenix-firestorm 系の upstream (Linden Lab 本家) で着地した場合の判断は **その時点で行う** (§4 (5))、charter では reset 方針を固定しない。判断指針は以下:

### 判断軸 1: AYAstorm-vk 進捗 stage

| 着地時 AYAstorm 進捗 | reset cost (失う work) | LL 公式採用 merit |
|---|---|---|
| vk-α 前 (r41 着手前) | ~ゼロ | 高い (棚卸し済 work loss、LL に乗り換えで完遂可能性が大幅 up) |
| vk-α 直後 (r41 達成、r41.5 / r42 着手前) | 低い (空転までの work、~16 PM / ~7 年経過) | 高い (基本機能 port + 視覚表現 port を全スキップ可能) |
| vk-β / vk-γ 進行中 (r42-α / r42-β / r42-γ 達成、deferred + 視覚表現 port 中) | 中 (~17-27 PM / ~7-11 年経過) | 中 (lighting/material 部分は LL に揃える価値あり、AYAstorm 固有 shader は再 port) |
| vk-δ 完遂 (r42-δ 達成、effects 動作中、3 OS parity 完遂前) | 高 (~27 PM / ~11 年経過) | 中 (effects 部分の質的比較で選別) |
| vk-RC 進行中 (r43 達成、Linux baseline + Win parity 完遂、Mac 未) | 高 (~30 PM / ~12.7 年経過) | 低 (Mac parity 残のみ、reset で失う Linux+Win parity work 大) |
| vk-RC 達成 (r44 達成、3 OS parity 完遂) | 最大 (全 work loss、~36 PM / ~14.2 年経過) | 低 (parity 達成済、LL 採用は work loss > merit) |

**注**: 上記 reset cost / 経過年数は本 §6 正式 line up + `06-effort-estimation.md` §3-§5 + `07-r42-plus-milestone-mapping.md` §1-§4 確定値ベース。中央値で記載、uncertainty band は 06 doc §6 参照。

### 判断軸 2: LL 公式の質的評価

- **API カバレッジ**: AYAstorm が使う Vulkan 機能を全部 cover しているか (例: 描画 stage / shader / sync primitive / extension)
- **AYAstorm 機能の port 可能性**: LL 公式の上に AYAstorm 固有機能 (audio / 視覚表現 / Cinematic / 等) を port できるか
- **SL backward compat**: LL 公式が SL backward compat 維持しているか (旧 region / 旧 asset で動くか)
- **3 OS 動作**: LL 公式が 3 OS 全部で安定動作しているか
- **maintainer 継続性**: LL が Vulkan branch を継続 maintain する見込みがあるか

### 3 つの選択肢

判断軸 1 + 2 から以下のいずれかを選択:

- **(i) reset**: AYAstorm-vk 系統を破棄、LL 公式に乗り換え、AYAstorm 機能を再 port (vk-α 前 / vk-α 直後で AYAstorm-vk work loss が低い場合)
- **(ii) maintain**: AYAstorm-vk 独自実装を維持、LL 公式は無視 (vk-RC 後 / LL 公式が AYAstorm 用途に届かない場合)
- **(iii) module 単位選別 merge**: LL 公式と AYAstorm 独自を module 単位で比較、優れた方を採用 (中盤の場合、LL 公式の特定 module だけ採用)

判断は AYA さんが行う、Claude は判断材料 (work 進捗 + LL 公式 audit) を提供する。

### 判断軸 3 (r41.5 達成後限定): LL UI 変更時の defensibility

r41.5 = VK repo 分離達成後は、LL 公式 VK が AYAstorm 非互換 UI を伴って着地した場合の **追加選択肢**:

- **(iv) LL 公式 VK engine 採用 + AYAstorm GUI 維持** — VK repo を LL 公式 VK engine に差し替え、本線 (AYAstorm GUI = LGPL) はそのまま維持。LL UI 変更の影響を受けずに AYAstorm の chat / picker / Cinematic 等の独自 GUI を生存させる
- この選択肢は r41.5 達成前は不可、達成後限定で有効
- 採用条件:
  - LL 公式 VK engine の API surface が AYAstorm VK repo の interface と互換 (または互換 wrapper を差し込める)
  - LL 公式 VK engine が AYAstorm GUI が必要とする描画機能 (atmospheric / Cinematic / picker 等) を提供している (または AYAstorm 側で wrapper で代替可能)

判断フロー:
1. AYAstorm 進捗 stage (§7 判断軸 1) を確認
2. LL 公式の質的評価 (§7 判断軸 2) を実施
3. r41.5 達成後で LL UI が非互換なら **(iv) を最優先検討**、互換なら (i)-(iii) で判断
4. r41.5 達成前なら (i)-(iii) のみで判断

## 8. plan B trigger

以下のいずれかで charter 再評価:

- **(A) LL Vulkan 先に着地** — §7 の判断指針を発動
- **(B) 工程プラン破綻** — 6-15 人年算定が大きく外れる (例: r41 達成が 3 年経過しても未達 / 棚卸しで判明する portage 規模が想定の 2 倍以上)
- **(C) AYA life plan 変更** — 本職 / 健康 / 家庭等の事情で本章継続不可
- **(D) 5 年経過 (2031-05-28) で LL Vulkan release ETA も公開されない** — plan A 継続か別 backend (D3D12 / Metal native / WebGPU) 検討かを再評価
- **(E) LL Vulkan release されたが quality が AYAstorm 用途 (撮影描画 / Cinematic) に届かない** — (ii) maintain に倒すか、plan B 別 backend 検討

trigger 発動時は当時の AYAstorm 体制 / industry 状況を再評価した上で本 charter を更新。

## 9. work breakdown (sub-phase 3 内訳)

r40 達成 (工程プラン完成) までに進める work item:

### (a) Vulkan portage 棚卸し phase

- 現 OpenGL コード base の Vulkan port 影響範囲を分類:
  - **要 port**: indra/llrender + GL header 212 files + GLSL shader 248 file + pipeline.cpp 等
  - **要再設計**: pipeline.cpp 3 大グローバル + cull/stateSort 内 GL 呼出 + geometry mutation
  - **不要 port**: OpenGL 固有 API で Vulkan に直接対応物のないもの (削除 or 別実装)
  - **AYAstorm 固有機能の影響範囲**: r1-r30 全機能を再分類 (描画 stage 依存度 / 並行 port 可能性)
- 出力 = portage 棚卸し doc (要 port file list + LOC + 影響範囲 + 各 file の Vulkan 等価実装方針)

### (b) Vulkan API 設計

- Vulkan version / loader / SDK の選定 (vulkan-1.3 + volk + Vulkan SDK 1.3.x or 等)
- shader cross compile chain (glslang / spirv-cross / DXC / etc.)
- descriptor set / pipeline layout 設計 (uniform / SSBO / texture binding 戦略)
- render pass / framebuffer 構造 (deferred g-buffer の Vulkan 表現)
- sync 戦略 (fence / semaphore / barrier の使い分け、frame in flight 数)
- memory allocator (VMA = Vulkan Memory Allocator か自前か)
- swapchain / present mode (FIFO / mailbox / immediate)
- 3 OS 対応 (Linux = Vulkan native / Win = Vulkan native / Mac = MoltenVK 経由)
- 出力 = Vulkan 設計 doc

### (c) 工程算定

- (a) 棚卸し + (b) 設計から具体的人年算定
- Doom/Blender 参照点を反映した上方/下方修正
- milestone 単位 (r41 / r42 / ... ) の所要月数算定
- 出力 = 工程算定 doc

### (d) r42+ 区切り確定

- (a) 棚卸し結果 + (c) 工程算定から r42 / r43 / r44 / r45+ の正式区切り line up を確定
- §6 の仮 line up を本 charter で更新
- 出力 = 本 charter §6 の更新

### (e) charter 完成 → r40 達成

- (a) (b) (c) (d) の出力を 03-sub-phase-3-vulkan-plan.md に統合
- 本 charter (00-charter.md) も最終 review
- AYA さん承認で r40 達成、r41 着手へ

### work item の進め方

- (a) → (b) → (c) → (d) の順序依存あり (棚卸し → 設計 → 算定 → 区切り)
- (e) は (a)-(d) 全完了後
- 各 work item は Claude/AYA 共同で進める、Claude が draft 作成 → AYA review → 修正 → 確定
- 中間 doc は `docs/specs/ayastorm-r40-vulkan-migration/` 配下に作成 (例: `04-portage-inventory.md` / `05-vulkan-api-design.md` / `06-effort-estimation.md` / `07-r42-plus-milestone-mapping.md`)

## 10. 関連 doc

### r40 章内部

- `00-charter.md` — 本 doc (工程プラン本体 = r40 達成成果物)
- `01-sub-phase-1-cpu-perf.md` — sub-phase 1 CPU perf 全 REJECT 記録
- `02-sub-phase-2-extended-falsify.md` — sub-phase 2 鉱脈ゼロ記録
- `03-sub-phase-3-vulkan-plan.md` — sub-phase 3 work item 統合 (active 2026-05-28〜)
- `04-portage-inventory.md` — Vulkan portage 棚卸し (work item (a) 出力、完了 2026-05-28)
- `05-vulkan-api-design.md` — Vulkan 設計 (work item (b) 出力、完了 2026-05-28)
- `06-effort-estimation.md` — 工程算定 (work item (c) 出力、完了 2026-05-28)
- `07-r42-plus-milestone-mapping.md` — r42+ 区切り確定 (work item (d) 出力、完了 2026-05-28)
- `handoff-work-item-*-complete.md` — 各 work item / group 区切り cadence handoff (historical、a-complete / b-foundation/group-a/complete / c-foundation/group-a/group-b/group-c-complete / d-foundation/group-a/group-b-complete)

### 関連 memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active (本 charter の親 memory)
- `project_ayastorm_r40_extended.md` — sub-phase 2 詳細 (history)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (GL 除去 + Vulkan 空転)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 (§4 (2) で Linux 先行 = 明示指示例外、3 OS 完遂は維持)
- `project_ayastorm_release_chapters.md` — release 番号帯と章構成 (r1-13 audio / r14-24 視覚表現 / r25-29 3D stream / r30+ 撮影描画)
- `feedback_falsification_as_progress.md` — sub-phase 1, 2 の REJECT/ゼロ が sub-phase 3 への絞り込み成果

### 関連 r40 章前史 doc

- `docs/specs/ayastorm-render-perf-survey.md` — r40 sub-phase 1 で参照した既存 perf 棚卸し doc
