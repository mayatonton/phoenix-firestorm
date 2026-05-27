# r40 sub-phase 3 work item (d): r42+ 区切り確定

**status**: **work item (d) 完了 (AYA review PASS 2026-05-28)** — foundation group (§1 + §2) + group A (§3 + §4) + group B (§5 + §6) 全完了、§1-§6 全 section 確定。本 §6 diff draft の 00-charter.md §6 への実 update は work item (e)-1 で実施済 (2026-05-28)、work item (e) charter 完成 着手中
**親 doc**: `03-sub-phase-3-vulkan-plan.md` work item (d)
**前置 doc**:
- `00-charter.md` §6 — r42+ 仮 line up (本 work item (d) で正式 mapping 化)
- `04-portage-inventory.md` (work item (a)) — §5.4 段階 port 戦略 + §6.3.2 AYAstorm 機能 pull-in 順
- `05-vulkan-api-design.md` (work item (b)) — §10 skeleton 時系列 + §8 OS 別
- `06-effort-estimation.md` (work item (c)) — §3 milestone 別工数 + §4.4 OS 着手 timing + §5.6 marker 暦年
**達成条件**: §1-§6 全 section draft 完成 + AYA review PASS → work item (e) charter 完成 → r40 達成宣言

---

## 算定方針 (全 section 共通の原則)

### 目的

charter §6 が定義する r42-r45+ の **仮 line up** (棚卸し後に確定と明記) を、(a)(b)(c) 完了 input を反映した **正式 mapping** に切替える。具体的には以下を確定する:

1. **r42 区切り algorithm 化** (§1): charter 仮 line up の r42 単一 milestone を r42-α/β/γ/δ に細分化する logic、描画 stage (vk-β/γ/δ) と AYAstorm 機能 (r42-α/β/γ) の対応、05 doc §10 skeleton の時系列接続
2. **r42 milestone 内訳** (§2): r42-α/β/γ/δ 各 sub-milestone の構成 (work breakdown + 06 doc §3 工数反映 + acceptance criteria draft)
3. **r43-r44 区切り** (§3): charter 仮 line up r43-r45+ の正式区分 (parity 補強 / 性能 polish / Mac portable subset 詳細化 = vk-RC) と sub-milestone 構成
4. **r45+ 区切り** (§4): vk-RC parity 完遂 (= r44) 後の visual realism 次世代 / ray tracing / HDR / GPU-driven の broad placeholder (本算定範囲外、charter §3 時間軸非設定遵守)
5. **各 milestone charter 草案 outline** (§5): r41 / r41.5 / r42-α/β/γ/δ / r43 / r44 charter outline (各 milestone 着手前に詳細化、本 (d) 段階では outline のみ)
6. **charter §6 反映** (§6): 00-charter.md §6 仮 line up を本 §1-§5 で確定した正式 mapping で update、work item (e) charter 完成への引継ぎ

### 区切りの基本軸

charter §4 (8) 確定の「**描画 stage 単位** で区切る」方針を継承、ただし以下の 3 軸を併用して細分化:

1. **描画 stage 軸**: vk-α (r41 空転) / vk-β (静止 scene + avatar) / vk-γ (deferred lighting + 基本 material) / vk-δ (reflection/SMAA/SSAO/DoF/shadow cascade) / vk-RC (parity 完遂)
2. **AYAstorm 機能 pull-in 軸**: 04 doc §6.3.2 で確定の touchpoint 規模順 + 独立度順 (r21.1 picker / r30 Cinematic / r14+ visual realism)
3. **構造 refactor 軸**: r41.5 (VK repo 分離) — 描画 stage 進行なしの独立 milestone (charter §6 で追加済)

これら 3 軸の **AND 条件** で milestone を切るのではなく、各軸の自然な切れ目を **align** させて milestone を確定する (例: r42-β = vk-β + r30 Cinematic + DoF state enum 化 refactor が同時に進む milestone)。

### 範囲

本 (d) 算定範囲は **vk-RC parity 完遂 (= r44 達成)** まで。**r45+ は本算定範囲外** (06 doc §3.8 / charter §3 「時間軸では撤退条件設けない」遵守):

- r45+ visual realism 次世代 / ray tracing / HDR / GPU-driven は parity 完遂後の self-driven 章
- 本 (d) は §4 で broad placeholder のみ提示、詳細化は r44 達成後の別章 charter に委ねる

### 算定 source map

| 算定対象 | 主要 source |
|---|---|
| r42 区切り algorithm (§1) | charter §6 仮 line up + 04 doc §5.4 段階 port 戦略 + 04 doc §6.3.2 AYAstorm pull-in 順 + 05 doc §10 skeleton 時系列 + 06 doc §5.6 marker 暦年 |
| r42 milestone 内訳 (§2) | 06 doc §3.3-§3.6 r42-α/β/γ/δ 詳細 + 04 doc §B.x AYAstorm 3 機能 + 05 doc §3 (descriptor set) + §4 (render pass) |
| r43-r44 区切り (§3) | 06 doc §3.7 r43-r44 + §4 OS 別 + 05 doc §8 OS 別 + §9.4 MoltenVK portable subset |
| r45+ 区切り (§4) | charter §3 時間軸非設定 + 06 doc §3.8 範囲外宣言 |
| charter 草案 outline (§5) | 各 milestone 達成基準 + charter §5 r41 達成基準 template |
| charter §6 反映 (§6) | 本 §1-§5 結論 + 00-charter.md §6 現状 |

---

## §1 r42 区切りの algorithm 化

### §1.0 mapping 方針

charter §6 仮 line up は **r42 を単一 milestone** として扱う (vk-β 描画 stage + r1-r13 audio port)。これを 06 doc §3.3-§3.6 算定区切り (r42-α/β/γ/δ) に細分化する logic を本 §1 で確定する。細分化の 3 軸:

1. **AYAstorm 機能 pull-in 軸**: 04 doc §6.3.2 で確定の 3 機能 (picker / Cinematic / visual realism) を r42-α/β/γ に直接 mapping
2. **描画 stage 軸**: r42-α 着手で vk-β 領域に入る (静止 scene + avatar + picker)、r42-β で vk-β 完遂 (Cinematic = DoF + post-process)、r42-γ で vk-γ + vk-δ 部分着手 (visual realism = lighting + post-process)、r42-δ で vk-δ 完遂 + parity 残機能 polish
3. **OS 着手軸**: r42-α で Win 着手 / r42-β で Mac 着手 (06 doc §4.4 着手 timing)

3 軸の align によって r42-α/β/γ/δ 4 sub-milestone が自然に切れる (各軸の境界が 1 milestone 単位で揃う)。

### §1.1 charter §6 仮 line up の現状と limitation

#### charter §6 仮 line up 抜粋 (現状 2026-05-28 時点)

| milestone | 描画 stage 相当 | AYAstorm 機能 pull-in 想定 | repo 構成 |
|---|---|---|---|
| r41 | vk-α | (なし、parity 不要) | 本線同居 (Phase 1) |
| r41.5 | (描画 stage 進行なし、構造 refactor のみ) | Vulkan code abstraction 化 + VK repo 分離 | VK repo 新規立ち上げ (Phase 2 開始) |
| r42 | vk-β (静止 scene + avatar) | r1-r13 audio port (描画非依存、並行可) | 本線 + VK repo (dynamic link) |
| r43 | vk-γ (deferred lighting + 基本 material) | r14-r24 視覚表現の lighting 系統 port | 本線 + VK repo |
| r44 | vk-δ (reflection / SMAA / SSAO / DoF / shadow cascade) | r14-r24 視覚表現の effects 系統 port | 本線 + VK repo |
| r45+ | vk-RC (parity 完遂) | r25-r29 3D stream / r30 Cinematic / chat / picker / 残り全機能 + Win/Mac 移植 | 本線 + VK repo |

#### limitation 4 件

(a)(b)(c) 完了で以下が判明、仮 line up を本 (d) で更新する根拠:

1. **r42 単一は粗すぎる**: 06 doc §3 では r42 が **4 sub-milestone (α/β/γ/δ)** に分解、各 sub-milestone の工数は 0.65 〜 3.18 PM、合計 8.97 PM ≒ 9 PM = r41 の半分強、これを単一 milestone で扱うと acceptance criteria の粒度が荒くなる
2. **AYAstorm 機能 pull-in の正式 mapping 未確定**: 04 doc §6.3.2 で picker / Cinematic / visual realism の 3 機能 pull-in 順は確定済 (charter §6 末尾で「本 line up 格上げは work item (d) で確定」と保留)、本 (d) で正式 mapping 化する
3. **r43-r45+ の意味整理が必要**: charter 仮では r45+ = vk-RC、06 doc 算定では r43-r44 = vk-RC + r45+ = visual realism 次世代 (本算定範囲外)、本 (d) で正式区分
4. **r41.5 の位置付けは正式採用済 (charter §6 で更新済)**: ただし本 (d) で「r41 → r41.5 → r42-α の遷移」の sub-milestone level での順序を確定する必要

### §1.2 06 doc 算定区切りからの正式区分

06 doc §3 で算定した milestone 区切りを **正式区分** として採用:

| 正式 milestone | 描画 stage | 主作業 | base PM (06 doc §3.9) | 中央値暦月 (06 doc §5.3) | 中央値暦年 (06 doc §5.6) |
|---|---|---|---|---|---|
| r41 | vk-α | GL 除去 + Vulkan 空転 (16.17 PM) | 16.17 | ~84.1 暦月 (~7 年) | ~2033 年中 |
| r41.5 | (描画 stage 進行なし) | VK repo 分離 + Vulkan code abstraction 化 | 1.50 | ~7.2 暦月 (~0.6 年) | ~2034 年初 |
| r42-α | vk-β 着手 (render pass attachment + read-pick) | r21.1 self-rigged picker port | 0.65 | ~2.9 暦月 (~0.24 年) | ~2034 年前半 |
| r42-β | vk-β 完遂 + vk-δ 部分 (DoF + state enum 化) | r30 Cinematic mode port | 3.15 | ~13.9 暦月 (~1.16 年) | ~2035 年中 |
| r42-γ | vk-γ + vk-δ 部分 (lighting + post-process chain) | r14+ visual realism port | 3.18 | ~14.0 暦月 (~1.17 年) | ~2036 年後半 |
| r42-δ | vk-δ 完遂 + parity 残機能 polish | r25-r29 3D stream + r1-r13 audio 確認 + regression sweep | 2.25 | ~9.9 暦月 (~0.83 年) | ~2037 年中 |
| r43-r44 | vk-RC (parity 補強 + 性能 polish + Mac portable subset 詳細化) | Mac MoltenVK + Win driver matrix + perf tuning | 3.00 + 1.89 (Win) + 4.05 (Mac) | ~12.0 + 8.3 + 17.8 暦月 | ~2038-2040 年 |
| **vk-RC 達成 (= r44 達成)** | parity 完遂 | charter §4 (1) 達成 | **35.84 PM** | **~170 暦月 (~14.17 年)** | **~2040 年後半** |
| r45+ | (本算定範囲外) | visual realism 次世代 / ray tracing / HDR / GPU-driven | — | — | — |

#### 細分化 logic の妥当性

charter §6 仮 line up の r42 単一を α/β/γ/δ 4 分割する妥当性:

- **04 doc §6.3.2 確定 = α/β/γ 自然境界**: AYAstorm 機能 pull-in 順 (picker / Cinematic / visual realism) が 3 sub-milestone を自然に切る
- **δ = parity 残機能 polish + r1-r13 audio + r25-r29 3D stream**: charter 仮の「r42 = r1-r13 audio port 並行」を 4 番目の sub-milestone (r42-δ) に集約、vk-RC 直前 polish も同 milestone
- **工数 ratio**: α (0.65) ≪ β (3.15) ≈ γ (3.18) > δ (2.25)、各 sub-milestone が 1 PM 以上 = 「単一 milestone と呼べる最小単位」を超える
- **OS 着手境界 align**: r42-α 着手 = Win 追加開始 / r42-β 着手 = Mac 追加開始 (06 doc §4.4)、OS 着手境界も sub-milestone 境界と一致

### §1.3 描画 stage (vk-β/γ/δ) と AYAstorm 機能 (r42-α/β/γ) の対応 mapping

charter §6 仮 line up では「r42 = vk-β / r43 = vk-γ / r44 = vk-δ / r45+ = vk-RC」と stage 単位で割当てていた。06 doc §3 算定で **AYAstorm 機能 pull-in が描画 stage を跨ぐ** 構造が判明したので、対応を本 §1.3 で正式 mapping 化:

#### 描画 stage × AYAstorm 機能 × milestone 三軸 mapping 表

| milestone | 描画 stage (主) | 描画 stage (副) | AYAstorm 機能 | 進行範囲 |
|---|---|---|---|---|
| r41 | vk-α (空転) | — | (なし) | swapchain + render pass + 黒画面 + UI 描画 |
| r41.5 | (進行なし) | — | (なし) | 構造 refactor (Vulkan code abstraction + VK repo 分離) |
| r42-α | **vk-β 着手** | — | **r21.1 self-rigged picker** | 静止 scene 描画 + avatar 描画 + render pass attachment (mObjectIDBuffer) + read-pick path |
| r42-β | **vk-β 完遂** | vk-δ 部分 (DoF) | **r30 Cinematic mode** | scene + avatar 描画完了 + DoF state enum 化 (frame context 統合) + post-process pass 着手 |
| r42-γ | **vk-γ 着手** + vk-γ 進行 | vk-δ 部分 (volumetricLight / godrays / vignette / tone map) | **r14+ visual realism** | deferred lighting (sky dome + atmospherics) + post-process chain 7 sub-pass 統合 |
| r42-δ | vk-γ 完遂 + **vk-δ 完遂** | vk-RC 直前 polish | r25-r29 3D stream + r1-r13 audio 確認 | reflection / SMAA / SSAO / shadow cascade 完遂 + parity 残機能 polish + regression sweep |
| r43-r44 | **vk-RC (parity 補強 + 性能 polish + Mac portable subset 詳細化)** | — | (全 r1-r30 機能 3 OS parity 完遂) | Mac MoltenVK + Win driver matrix + frame in flight tuning + VMA allocation strategy |

#### mapping の含意

- **描画 stage は r42 内で 4 段階進行**: vk-β 着手 (α) → vk-β 完遂 + vk-δ 部分 (β) → vk-γ + vk-δ 部分 (γ) → vk-γ 完遂 + vk-δ 完遂 (δ)、charter 仮 line up の「r42 = vk-β / r43 = vk-γ / r44 = vk-δ」より **r42 内で大半の stage が進む** ことが判明
- **AYAstorm 機能 = 描画 stage の trigger**: r21.1 picker = vk-β 着手 trigger / r30 Cinematic = vk-β 完遂 + DoF trigger / r14+ visual realism = vk-γ + vk-δ 完遂 trigger、AYAstorm 機能 port が描画 stage を引っ張る構造
- **r43-r44 = vk-RC (parity 補強 + 性能 polish + Mac portable subset 詳細化)**: charter 仮の「r43 = vk-γ / r44 = vk-δ」は r42 内で大半完了するため、r43-r44 は parity 補強 + 性能 polish + Mac portable subset 詳細化に振替え (= vk-RC 完遂)
- **vk-RC 達成 = r44 達成**: charter §4 (1) 完遂 goal の到達 marker、本算定の最大累積閾値 (~170 暦月 / ~14.17 年 / 2040 年後半)

### §1.4 05 doc §10 skeleton (r41.5) + 04 doc §5.4 段階 port 戦略 との時系列整合

#### 05 doc §10 skeleton の時系列 hooks

05 doc §10 skeleton で確定した **VK repo 分離 + Vulkan code abstraction** が r41 → r41.5 → r42-α の時系列に直接対応:

| 時系列 | 05 doc §10 hook | 本 (d) milestone |
|---|---|---|
| r41 着手前 | §10.1 LLVKRenderer interface 骨子 (placeholder のみ) | r41 着手準備 (04 doc §5.4 段階 1) |
| r41 進行中 | §10.2 LLVKRenderer 直接実装 (本線同居、pipeline.cpp 内 inline) | r41 work (04 doc §5.4 段階 2-5) |
| r41 達成 | (vk-α 空転動作確認) | r41 完了 (本 §1.2 r41) |
| r41.5 着手 | §10.2 LLVKRenderer 詳細化 + interface 経由 call に置換 | r41.5 work (構造 refactor) |
| r41.5 進行中 | §10.3 VK repo 物理分離 + dynamic link 化 | r41.5 work |
| r41.5 達成 | (VK repo 立ち上げ + dynamic link 動作確認) | r41.5 完了 (本 §1.2 r41.5) |
| r42-α 着手 | (LLVKRenderer interface 確立済、新機能 implementation は interface 経由) | r42-α 着手準備 |
| r42-α 進行中 | mObjectIDBuffer render pass attachment 統合 (interface 経由) | r42-α work (本 §1.2 r42-α) |
| r42-β 以降 | (各 AYAstorm 機能 port が LLVKRenderer interface 経由で本線/VK repo どちらにも追加可能) | r42-β/γ/δ work |

#### 04 doc §5.4 段階 port 戦略との対応

04 doc §5.4 で確定の 5 段階 base port 戦略を本 (d) milestone に分配:

| 04 doc §5.4 段階 | base 工数 | 本 (d) milestone |
|---|---|---|
| 段階 1: GL header wrapper 置換 + volk loader | 0.5 PM | r41 (本 §1.2) |
| 段階 2: lldrawpool Vulkan 化 (13 file) | 1.0 PM | r41 (本 §1.2) |
| 段階 3: state machine → PSO 化 (llrender 主要 5 file) | 1.5 PM | r41 (本 §1.2) |
| 段階 4: pipeline.cpp 3 大グローバル → frame context | 1.0 PM | r41 (本 §1.2、最高密度の 1 file) |
| 段階 5: llspatialpartition / llviewershadermgr / llvertexbuffer 依存解決 | 0.5 PM | r41 (本 §1.2) |
| (段階外) §B.1 AYAstorm picker | 0.5 PM | **r42-α** (本 §1.2) |
| (段階外) §B.3 AYAstorm Cinematic | 2-3 PM | **r42-β** (本 §1.2) |
| (段階外) §B.2 AYAstorm visual realism | 2-3 PM | **r42-γ** (本 §1.2) |
| (段階外) VK repo 分離 | (本 (d) §5.4 算出範囲外) | **r41.5** (本 §1.2) |
| (段階外) parity 残機能 + 3D stream + audio 確認 | (本 §3.6) | **r42-δ** (本 §1.2) |
| (段階外) Mac MoltenVK + Win driver matrix + perf tuning | (本 §3.7 + §4) | **r43-r44** (本 §1.2) |

#### 時系列整合判定

- 05 doc §10 skeleton の r41 → r41.5 → r42-α 遷移と本 §1.2 milestone 順序が **完全 align** ✓
- 04 doc §5.4 段階 1-5 が全て r41 範囲内 + AYAstorm §B.1/B.2/B.3 が r42-α/β/γ に直接 mapping ✓
- 04 doc §5.4 外の追加 work (r41.5 / r42-δ / r43-r44) は本算定で新規追加 (06 doc §3.0 振り分け方針で計上済) ✓

### §1.5 r45+ 範囲外宣言 + charter §3 「時間軸では撤退条件設けない」遵守

#### r45+ = vk-RC parity 完遂 (= r44 達成) 後

vk-RC parity 完遂 = r44 達成 = AYAstorm r1-r30 全機能を 3 OS で Vulkan 上に再現、charter §4 (1) 完遂 goal の到達。これ以降の r45+ は:

- visual realism 次世代 (r14+ 章の next iteration、ray tracing / HDR / GPU-driven 等)
- AYAstorm 独自進化路線 (charter §3 「parity 完遂後の self-driven 章」)

#### 本 (d) 範囲外とする理由

1. **charter §3 「時間軸では撤退条件を設けない」遵守**: r45+ 着手時期は parity 完遂後の AYAstorm 体制 / industry 状況 / LL 着地 status 次第、本 (d) で時期を固定すると charter §3 違反
2. **06 doc §3.8 本算定範囲外宣言と整合**: 06 doc §3.8 で「r45+ は parity 完遂後の self-driven 章として独立算定 (将来別 work item or 別章 charter)」と確定済、本 (d) は同方針継承
3. **vk-RC 達成 = r40 章の最大閾値**: 本算定中央値 14.17 年 / 上方 27 年 = charter §4 (3) 想定 15-30 年帯の下限-上限近接、r45+ を含めると上方が charter 上限を超える可能性、charter §4 (3) 想定範囲を維持するため r45+ は別算定

#### 本 (d) §4 で扱う placeholder

§4 r45+ 区切り (本算定範囲外) では以下のみ broad placeholder で提示:

- r45+ scope の broad outline (visual realism 次世代 / ray tracing / HDR / GPU-driven の topic 列挙)
- r45+ 着手 trigger 条件 (r44 達成 + AYA judgment、時間軸 trigger は設けない)
- r45+ charter 起草 timing (r44 達成宣言 + 6 か月以内に AYA さんと擦り合わせて別章 charter 起草)

### §1.6 §1 結論 (r42 区切り algorithm 確定)

本 §1 で確定した r42 区切り algorithm:

1. **charter §6 仮 line up の r42 単一を r42-α/β/γ/δ 4 sub-milestone に細分化** (§1.2)
2. **AYAstorm 機能 pull-in (04 doc §6.3.2) を r42-α/β/γ に直接 mapping**、parity 残機能 + r25-r29 3D stream + r1-r13 audio 確認 は r42-δ に集約 (§1.2)
3. **描画 stage (vk-α/β/γ/δ/RC) は r42-α/β/γ/δ + r43-r44 と多軸 mapping**、r42 内で大半の stage が進行、r43-r44 = vk-RC 完遂 (parity 補強 + 性能 polish + Mac portable subset 詳細化) に振替え (§1.3)
4. **r41 → r41.5 → r42-α 遷移は 05 doc §10 skeleton で時系列整合確認済**、04 doc §5.4 段階 1-5 は全 r41 範囲内、AYAstorm §B.1/B.2/B.3 は r42-α/β/γ に直接 mapping (§1.4)
5. **vk-RC 達成 = r44 達成 = charter §4 (1) 完遂 goal の到達 marker**、本算定中央値 ~14.17 年 / 上方 ~27 年 = charter §4 (3) 想定 15-30 年帯内 (§1.5)
6. **r45+ は本算定範囲外**、charter §3 時間軸非設定遵守、別章 charter 起草で詳細化 (§1.5)

→ 本 §1 algorithm を base として、§2 で各 sub-milestone の内訳 (work breakdown + acceptance criteria draft) を確定する。

---

## §2 r42 milestone 内訳

### §2.0 算定方針

§1 で確定した r42-α/β/γ/δ 4 sub-milestone の **内訳 (work breakdown + acceptance criteria draft)** を本 §2 で確定する。各 sub-milestone について以下を提示:

- **目的** (charter §4 (1) parity goal における該当機能 + 描画 stage 達成範囲)
- **work breakdown** (06 doc §3.3-§3.6 詳細を sub-milestone に落とし込み、foundation 帰属 + 追加 work + 余裕係数)
- **acceptance criteria draft** (parity 達成 + visual A/B + 3 OS 動作確認 + regression 無し の 4 軸)
- **OS 着手 timing** (06 doc §4.4 反映)
- **依存 milestone** (前 milestone 完了 + interface 確立済 + 設計 doc 確定済)

各 acceptance criteria は本 (d) 段階では **draft** (各 milestone 着手前の charter 起草時に詳細化)、本 (d) では outline のみ。

### §2.1 r42-α (r21.1 self-rigged picker port) sub-milestone 構成

#### 目的

AYAstorm r21.1 self-rigged picker (mObjectIDBuffer = gbuffer3 inline attachment + 単一回 click → ObjectID 取得) を Vulkan 上で parity 再現。描画 stage は **vk-β 着手** (静止 scene 描画 + avatar 描画 + render pass attachment + read-pick path)。

#### work breakdown (06 doc §3.3 反映)

| 領域 | 工数 (PM) | 主作業 | 出典 |
|---|---|---|---|
| §2.3 picker shader 2 file SPIR-V 化 | 0.10 | fsObjectIDV.glsl + fsObjectIDF.glsl の SPIR-V cross compile + descriptor set binding | foundation 帰属 (06 doc §3.3) |
| pipeline.cpp 4 LOC | 0.05 | mObjectIDBuffer setup → Vulkan attachment binding 置換 | 追加 work (a-3 §B.1) |
| render pass attachment 設計 + inline 統合 | 0.20 | 05 doc §4.5 deferred main pass 内 inline attachment 設計 | 追加 work (a-3 §B.1) |
| read-pick テスト | 0.15 | single click + drag select、existing AYAstorm test 流用 | 追加 work (a-3 §B.1) |
| **base work 計** | **0.50** | — | a-3 §B.1 工数感 0.5 PM 整合 ✓ |
| 余裕係数 +30% | +0.15 | touchpoint 局在 + 既存実装あり、低 risk (06 doc §3.0) | — |
| **r42-α total (フルタイム dev 換算)** | **~0.65** | — | 06 doc §3.3 |

#### acceptance criteria draft (r42-α 着手前の charter 起草で詳細化)

1. **parity 達成**: AYAstorm r21.1 self-rigged picker の単一 click → ObjectID 取得が Vulkan 上で動作 (本線 GL 実装と同等の click 精度 + 反応時間)
2. **render pass attachment 統合**: mObjectIDBuffer が Vulkan deferred main pass 内 inline attachment として動作、本線 GL の gbuffer3 inline と同等の memory bandwidth profile
3. **read-pick path 動作**: Vulkan staging buffer + transfer queue 経由の ObjectID readback が動作、本線 GL の glReadPixels と同等の latency (drag select も成立)
4. **shader cross compile**: picker shader 2 file (fsObjectIDV/F.glsl) SPIR-V 化が成立、glslang + spirv-cross 半自動で通る (compute/geometry/tessellation ゼロ、cross compile 容易)
5. **Win 追加開始**: r42-α 進行中に Win 側 LLWindow Win32 surface 化 + LunarG SDK 統合 + driver matrix 着手 (06 doc §4.4)、Linux + Win 両方で picker 動作
6. **regression 無し**: r41 で確立した GL 除去 + Vulkan 空転の baseline が r42-α 完了時点で stable (vk-α 機能の degradation 無し)

#### OS 着手 timing (06 doc §4.4)

- **Linux**: first-class baseline、r42-α 着手 = Linux で picker port 完了
- **Win**: **r42-α 着手 = Win 追加開始** (LLWindow Win32 surface 化 + driver matrix 着手)、r42-α 完了時点で Win でも picker 動作確認
- **Mac**: r42-α 段階では本線 GL 維持 (Mac 着手は r42-β から)

#### 依存 milestone

- **前 milestone**: r41.5 達成 (VK repo 分離 + LLVKRenderer interface 確立)
- **interface 確立**: 05 doc §10.2 LLVKRenderer interface 経由で AYAstorm 機能 port が可能
- **設計 doc 確定**: 05 doc §3 descriptor set + §4.5 render pass attachment 設計

### §2.2 r42-β (r30 Cinematic mode port) sub-milestone 構成

#### 目的

AYAstorm r30 Cinematic mode (DoF state enum 化 + Cinematic 関連 shader 4 file + AYAstorm View mode==2 = Cinematic) を Vulkan 上で parity 再現。描画 stage は **vk-β 完遂** (静止 scene + avatar 描画完了) + **vk-δ 部分** (DoF state enum 化 + post-process pass 着手)。

#### work breakdown (06 doc §3.4 反映)

| 領域 | 工数 (PM) | 主作業 | 出典 |
|---|---|---|---|
| §2.3 Cinematic shader 4 file SPIR-V 化 | 0.40 | volumetricLightF class1/class3 + screenSpaceReflUtil class3 + DoF 関連 1 file の cross compile + descriptor set binding | foundation 帰属 (06 doc §3.4) |
| pipeline.cpp 6 分岐 frame context bleed | 0.20 | Cinematic 関連 DoF mode 分岐の LLPipelineFrameContext 統合 (r41 で frame context 集約完了済の上に AYAstorm 6 分岐を追加) | 追加 work (a-3 §B.3) |
| DoF state enum 化 | 0.50 | 現 hardcoded → enum class + frame context 経由配信 (DoF mode の state machine refactor) | 追加 work (a-3 §B.3) |
| visual quality テスト | 1.00 | BD live cvar 13 件 + Cinematic Controls の visual A/B、live screenshot 比較 (memory `project_r30_cinematic_control_tuning_deferred.md` 反映) | 追加 work (a-3 §B.3) |
| 余 (regression sweep / 残細部 polish) | 0.15 | — | 追加 work |
| **base work 計** | **2.25** | — | a-3 §B.3 工数感 2-3 PM 中央値 2.5 整合 ✓ |
| 余裕係数 +40% | +0.90 | DoF state enum 化 + visual quality verify (live A/B 必須、memory `feedback_visual_decisions_need_live_ab.md`) | — |
| **r42-β total (フルタイム dev 換算)** | **~3.15** | — | 06 doc §3.4 |

#### acceptance criteria draft

1. **parity 達成**: AYAstorm r30 Cinematic mode (AYAstorm View mode==2) が Vulkan 上で動作、DoF blur + volumetric light + screen space refl が本線 GL 実装と visual 同等
2. **DoF state enum 化**: pipeline.cpp 内 hardcoded DoF mode 分岐 (6 件) が enum class 化、frame context 経由配信に refactor 完了、本線 GL でも同 refactor 維持 (本線 GL も r42-β refactor の恩恵を受ける invariant)
3. **shader cross compile**: Cinematic shader 4 file SPIR-V 化が成立、volumetricLightF class1/class3 の階層化を Vulkan descriptor set 階層 (per-frame / per-material) で再現
4. **visual A/B PASS**: r30 Cinematic Controls 13 件 BD live cvar (memory `project_r30_cinematic_control_tuning_deferred.md` 反映) との live A/B で visual 同等性確認、live cvar 経由の opt-in/out 動作
5. **Mac 追加開始**: r42-β 着手 = Mac portable subset check 開始 (t-noami さんに事前共有、05 doc §8.3)、Mac での Vulkan 動作初期確認 (parity 完遂は r43-r44 まで遅延)
6. **Win 並走**: Win driver matrix 継続 (r42-α で着手済の NVIDIA / AMD / Intel Arc 動作確認の継続 cycle)
7. **regression 無し**: r41 / r41.5 / r42-α で確立した baseline が stable

#### OS 着手 timing (06 doc §4.4)

- **Linux**: first-class、Cinematic port 完了
- **Win**: Win 並走 (driver matrix 継続)、Win で Cinematic 動作確認
- **Mac**: **r42-β 着手 = Mac 追加開始** (portable subset check + t-noami さん事前共有)、ただし Mac での Cinematic parity 完遂は r43-r44 まで遅延

#### 依存 milestone

- **前 milestone**: r42-α 達成 (picker port + Win 追加開始 baseline)
- **interface 確立**: 05 doc §10.2 LLVKRenderer interface + r41 frame context 集約 base
- **設計 doc 確定**: 05 doc §4.4 r14+ post-process 7 sub-pass の Vulkan render pass chain 設計 (本 r42-β 段階では DoF 関連 sub-pass のみ着手、r42-γ で post-process chain 完遂)

### §2.3 r42-γ (r14+ visual realism port) sub-milestone 構成

#### 目的

AYAstorm r14+ visual realism (post-process pass chain 統合 + visual realism 関連 shader 7 file + llvosky + llvowlsky sky dome) を Vulkan 上で parity 再現。描画 stage は **vk-γ 着手 + vk-γ 進行** (deferred lighting + sky dome + atmospherics) + **vk-δ 部分** (volumetricLight / godrays / vignette / tone map = post-process chain 統合)。

#### work breakdown (06 doc §3.5 反映)

| 領域 | 工数 (PM) | 主作業 | 出典 |
|---|---|---|---|
| §1.5 llvosky + llvowlsky | 0.27 | r14+ 基盤の sky dome + atmospherics Vulkan 化 (2.2K LOC) | foundation 帰属 (06 doc §3.5) |
| §2.3 visual realism shader 7 file SPIR-V 化 | 0.60 | vignette / tone map / godrays / bloom / glow / atmospherics 関連 shader 7 file の cross compile + descriptor set binding | foundation 帰属 (06 doc §3.5) |
| post-process descriptor set 整備 | 0.30 | 05 doc §3 r14+ post-process 7 sub-pass 分の sampler binding 設計 (per-frame / per-material / per-draw 3 階層) | 追加 work (a-3 §B.2) |
| pipeline.cpp post-process chain 5 LOC | 0.10 | post-process pass dispatch の Vulkan render pass chain 接続 | 追加 work (a-3 §B.2) |
| performance profile | 0.50 | post-process per-pass cost (`VK_EXT_calibrated_timestamps` 経由)、r14+ visual realism の per-pass cost 計測 | 追加 work (a-3 §B.2) |
| visual A/B | 0.50 | godrays / volumetricLight / vignette / scene buffer alpha invariant 確認 (memory `project_aya_visual_realism_alpha_protect.md` 必須遵守) | 追加 work (a-3 §B.2) |
| **base work 計** | **2.27** | — | a-3 §B.2 工数感 2-3 PM 中央値 2.5 整合 ✓ |
| 余裕係数 +40% | +0.91 | post-process descriptor 整備 + perf profile + visual A/B の iterate (r14+ 章 thesis = 「写真を撮るに値する空気と空間」、live A/B 必須) | — |
| **r42-γ total (フルタイム dev 換算)** | **~3.18** | — | 06 doc §3.5 |

#### acceptance criteria draft

1. **parity 達成**: AYAstorm r14+ visual realism (vignette / tone map / godrays / bloom / glow / volumetricLight / atmospherics) が Vulkan 上で動作、本線 GL 実装と visual 同等
2. **sky dome + atmospherics Vulkan 化**: llvosky + llvowlsky (2.2K LOC) が Vulkan 上で動作、本線 GL の sky dome 描画と同等の visual quality (atmosFragLighting の atten scalarization 規約遵守、memory `project_atmos_atten_scalarized.md`)
3. **shader cross compile**: visual realism shader 7 file SPIR-V 化が成立、shader 出力 = linear、sRGB 色は srgb_to_linear で逆引き invariant 遵守 (memory `feedback_shader_color_space_correction.md`)
4. **post-process pass chain 統合**: 7 sub-pass の Vulkan render pass chain が動作、subpass dependency + image transition 適切、scene buffer alpha invariant (frag_color.a=0、additive ONE/ONE pass の alpha 破壊禁止) 遵守
5. **performance profile 取得**: post-process per-pass cost (`VK_EXT_calibrated_timestamps`) が計測、本線 GL と同等の frame time 範囲内 (regression ≤10%)
6. **visual A/B PASS**: r14+ visual realism の既存 AYAstorm 実装と Vulkan port の visual 同等性確認、live A/B (sustained viewing) で cumulative 効果も観測 (memory `feedback_instant_ab_vs_sustained.md`)
7. **Win/Mac 並走**: Win parity 継続 + Mac 並走確立 (t-noami さん検証 cycle 始動 + MoltenVK 1.2 fallback path 整備、05 doc §8.3)
8. **regression 無し**: r41 / r41.5 / r42-α/β baseline が stable

#### OS 着手 timing (06 doc §4.4)

- **Linux**: first-class、visual realism port 完了
- **Win**: Win 並走 (driver matrix 継続)、Win で visual realism 動作確認
- **Mac**: Mac 並走 (t-noami さん検証 cycle 始動 + MoltenVK 1.2 fallback path 整備)、Mac での visual realism 初期動作確認 (parity 完遂は r43-r44 まで遅延)

#### 依存 milestone

- **前 milestone**: r42-β 達成 (Cinematic port + DoF state enum 化 base)
- **interface 確立**: r42-β で確立した post-process pass chain の initial integration
- **設計 doc 確定**: 05 doc §3 descriptor set 設計 + §4.4 post-process render pass chain 設計

### §2.4 r42-δ (parity 残機能 / vk-RC 直前 polish) sub-milestone 構成

#### 目的

charter §6 仮 line up の「r42 = r1-r13 audio port 並行」+ AYAstorm r25-r29 3D stream + parity 残機能 + vk-RC 直前 regression sweep を集約した polish milestone。描画 stage は **vk-γ 完遂 + vk-δ 完遂** (reflection / SMAA / SSAO / shadow cascade) + **vk-RC 直前 polish**。

#### work breakdown (06 doc §3.6 反映)

| 領域 | 工数 (PM) | 主作業 | 出典 |
|---|---|---|---|
| r25-r29 3D stream Vulkan 描画 stage 接続 | 0.50 | NDI / OBS 等の Vulkan-side hook 検討、05 doc §9.2 `VK_KHR_external_memory_*` 予約のみ採用判断 (r42-δ で実装は最低限、本格は r45+) | 追加 work |
| r1-r13 audio 系 Vulkan 非依存確認 | 0.30 | FMOD callback + Dullahan path の Vulkan-agnostic invariant 検証、memory `project_pr69_fallback_switch.md` 反映 (LL_DULLAHAN_AUDIO_CALLBACK フラグ整合確認) | 追加 work |
| vk-RC 直前 regression sweep | 0.70 | r41-r42-γ 残 bug 集中 fix、parity 残機能 polish (chat / picker / Cinematic / visual realism の細部) | 追加 work |
| **base work 計** | **1.50** | — | 06 doc §3.6 |
| 余裕係数 +50% | +0.75 | vk-RC 直前 unknown + parity 残機能の発掘 cost (charter §4 (3) 不確実性 2-3x の中央寄り反映) | — |
| **r42-δ total (フルタイム dev 換算)** | **~2.25** | — | 06 doc §3.6 |

#### acceptance criteria draft

1. **r25-r29 3D stream parity**: NDI / OBS / 3D stream 配信機能が Vulkan 上で動作、本線 GL と同等の stream quality (描画 stage 軽依存、Vulkan で接続 cleanup のみ)
2. **r1-r13 audio Vulkan 非依存確認**: FMOD callback + Dullahan path が Vulkan に invariant、audio 系の全機能 (r1-r13) が Vulkan 上で本線 GL と同等動作
3. **vk-δ 完遂**: reflection / SMAA / SSAO / shadow cascade の本線 GL parity 完遂 (charter §6 仮 line up の r44 部分が r42-δ で達成)
4. **vk-RC 直前 polish**: r41-r42-γ で発見した bug の集中 fix、AYAstorm r1-r30 全機能 (Linux baseline) の parity 完遂
5. **3 OS 状況確認**: Linux baseline parity 完遂、Win は driver matrix polish 段階、Mac は MoltenVK 詳細化 phase (r43-r44 での完遂を見据えた事前準備)
6. **regression sweep PASS**: r41 / r41.5 / r42-α/β/γ baseline + r42-δ 追加機能の全 regression 無し

#### OS 着手 timing (06 doc §4.4)

- **Linux**: first-class、parity 残機能 完遂
- **Win**: Win polish (旧 driver fallback + WHCK 整備)
- **Mac**: **Mac MoltenVK 詳細化** (05 doc §9.4 / §8.3 vk-RC 直前 phase 詳細化指示、本 r42-δ で実施)

#### 依存 milestone

- **前 milestone**: r42-γ 達成 (visual realism port + sky dome + post-process chain)
- **interface 確立**: r42-γ までで Vulkan interface 確立済、本 r42-δ は polish 中心
- **設計 doc 確定**: 05 doc §9.2 external memory 予約 + §9.4 MoltenVK portable subset

### §2.5 各 sub-milestone acceptance criteria draft の運用方針

#### draft の位置付け

本 §2.1-§2.4 で提示した acceptance criteria は **draft** (各 milestone 着手前の charter 起草時に詳細化):

- 本 (d) 段階では outline + 主要 acceptance 項目 6-8 件 / sub-milestone
- 各 milestone 着手前に `docs/specs/ayastorm-r4X-xxx/00-charter.md` を別途起草、本 §2 outline を base に詳細化 (各 acceptance 項目の具体 metric / test procedure / regression criteria を追加)

#### draft → charter 詳細化の cadence

| milestone | charter 起草 timing | 詳細化主体 |
|---|---|---|
| r41 | r40 達成宣言直後 (~2026-06) | AYA + Claude (本 (d) §5 で outline 提示済) |
| r41.5 | r41 達成宣言直後 (~2033 中) | AYA + Claude (法的 review 関与で AYA 比重大) |
| r42-α | r41.5 達成宣言直後 (~2034 初) | AYA + Claude (本 §2.1 outline 反映) |
| r42-β | r42-α 達成宣言直後 (~2034 前半) | AYA + Claude (本 §2.2 outline 反映、BD cvar 13 件 visual A/B 含む) |
| r42-γ | r42-β 達成宣言直後 (~2035 中) | AYA + Claude (本 §2.3 outline 反映、scene buffer alpha invariant 含む) |
| r42-δ | r42-γ 達成宣言直後 (~2036 後半) | AYA + Claude (本 §2.4 outline 反映) |
| r43 | r42-δ 達成宣言直後 (~2037 中) | AYA + Claude (本 (d) §3 で outline 提示予定) |
| r44 | r43 達成宣言直後 (~2038-2039) | AYA + Claude (本 (d) §3 で outline 提示予定、vk-RC 達成宣言を含む) |

#### draft が確定値ではない理由

- **時系列が長い** (~14 年累積、最遠 milestone は 2040 年): 詳細 acceptance metric は着手直前の AYAstorm + Vulkan ecosystem 状況に依存
- **AYAstorm 進化に追随**: r42-α 着手時点で AYAstorm 本体が r31 / r32 等の追加機能を持っている可能性、acceptance criteria は着手時点の AYAstorm parity goal に合わせて update
- **LL 着地 status 反映**: charter §7 LL 着地時判断指針に従い、各 milestone 着手前に LL 公式 Vulkan の status を audit、acceptance criteria に LL 着地 reset の選択肢を反映

### §2.6 §2 結論 (r42 内訳確定)

本 §2 で確定した r42 milestone 内訳:

1. **r42-α (r21.1 picker port)** ~0.65 PM / ~2.9 暦月 / ~2034 年前半: render pass attachment 統合 + read-pick path + Win 追加開始 (§2.1)
2. **r42-β (r30 Cinematic mode port)** ~3.15 PM / ~13.9 暦月 / ~2035 年中: DoF state enum 化 + Cinematic shader 4 file + visual A/B + Mac 追加開始 (§2.2)
3. **r42-γ (r14+ visual realism port)** ~3.18 PM / ~14.0 暦月 / ~2036 年後半: post-process chain 統合 + sky dome + visual realism shader 7 file + Win/Mac 並走 (§2.3)
4. **r42-δ (parity 残機能 / vk-RC 直前 polish)** ~2.25 PM / ~9.9 暦月 / ~2037 年中: r25-r29 3D stream + r1-r13 audio 確認 + vk-RC 直前 regression sweep + Mac MoltenVK 詳細化 (§2.4)
5. **acceptance criteria は draft、各 milestone 着手前の charter 起草で詳細化** (§2.5)

合計 r42 (α+β+γ+δ) = **~9.23 PM / ~40.7 暦月 / ~3.4 年** (06 doc §3.9 + §5.3 整合 ✓、PM = 0.65 + 3.15 + 3.18 + 2.25、暦月 = 2.9 + 13.9 + 14.0 + 9.9)

→ §3 r43-r44 区切り + §4 r45+ 区切り + §5 charter 草案 outline + §6 charter §6 反映 は group A / group B で続行。

---

## §3 r43-r44 区切り

### §3.0 算定方針

charter §6 仮 line up では「r43 = vk-γ / r44 = vk-δ / r45+ = vk-RC」と stage 単位で割当てていたが、本 §1.3 で確定の通り **r42 内で vk-γ + vk-δ 大半完遂** する設計に切替えたため、r43-r44 は **vk-RC (parity 補強 + 性能 polish + 3 OS parity 完遂)** に振替える。本 §3 で r43 と r44 を **3 OS parity 完遂順** に分割し、各 sub-milestone の内訳 + acceptance criteria draft + vk-RC 達成宣言の acceptance criteria を確定する。

#### 振替え logic の根拠

| 旧 charter §6 仮 line up | 本 §1.3 確定 mapping | 振替え理由 |
|---|---|---|
| r43 = vk-γ (deferred lighting + 基本 material) | r42-γ で vk-γ 着手 + r42-δ で vk-γ 完遂 | r14+ visual realism pull-in が描画 stage を引っ張るため (本 §1.3) |
| r44 = vk-δ (reflection / SMAA / SSAO / DoF / shadow cascade) | r42-β で vk-δ 部分 (DoF) + r42-γ で vk-δ 部分 (post-process) + r42-δ で vk-δ 完遂 | r30 Cinematic + r14+ visual realism pull-in が vk-δ を分散完遂するため (本 §1.3) |
| r45+ = vk-RC (parity 完遂) | r43-r44 = vk-RC (parity 補強 + 性能 polish + 3 OS parity 完遂)、r45+ = 本算定範囲外 (visual realism 次世代) | vk-RC は r42-δ までで Linux baseline 達成、r43-r44 で Win/Mac parity 完遂 + 性能 polish に振替え (本 §1.5) |

#### r43 / r44 の分割方針

06 doc §3.7 では r43-r44 を **合算** (3.00 PM、Linux baseline 上の追加 polish 軸算定) し、§4.2 / §4.3 で Win 増分 1.89 PM / Mac 増分 4.05 PM を OS 軸で重複排除済純増分として算出。本 §3 では **3 OS parity 完遂順** で r43 と r44 を分割:

- **r43 = Linux baseline parity 完遂 + Win parity 完遂**: 06 doc §3.7 r43-r44 milestone work の Linux baseline 部分 + Win driver matrix 部分 + Win 増分 (§4.2)
- **r44 = Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成**: 06 doc §3.7 r43-r44 milestone work の Mac MoltenVK 詳細化部分 + Mac 増分 (§4.3) + 性能 polish 仕上げ

この分割は 06 doc §5.6 marker 暦年とも整合 (r43-r44 達成 = Linux baseline parity ~144 暦月 → +Win 増分完遂 ~152 暦月 → vk-RC 3 OS parity 完遂 ~170 暦月 の 3 段 marker が r43 と r44 の境界に対応)。

#### 各 sub-milestone の提示項目

§3.1 / §3.2 で各 sub-milestone について以下を提示:

- **目的** (vk-RC 達成軸での該当 work + 描画 stage 達成範囲)
- **work breakdown** (06 doc §3.7 + §4.2 / §4.3 反映、foundation 帰属 + 追加 work + 余裕係数)
- **acceptance criteria draft** (parity 達成 + Win/Mac 動作確認 + 性能 ≤10% + regression 無し の 4 軸、本 (d) 段階では outline)
- **OS 着手 timing** (06 doc §4.4 反映、本 milestone での state)
- **依存 milestone** (前 milestone 完了 + Vulkan interface 確立済 + Win/Mac surface 化済)

### §3.1 r43 (Linux baseline parity 完遂 + Win parity 完遂) sub-milestone 構成

#### 目的

r42-δ までで達成済の Linux baseline AYAstorm r1-r30 全機能 Vulkan 動作を **stable parity 状態に補強** + **Win parity 完遂** (Win driver matrix 全 driver 動作 + 旧 driver fallback + WHCK 認定整備)。描画 stage は **vk-RC (parity 補強 + 性能 polish 一部)**。

#### work breakdown (06 doc §3.7 r43-r44 milestone work の Win 寄与 + §4.2 Win 純増分 反映)

| 領域 | 工数 (PM) | 主作業 | 出典 |
|---|---|---|---|
| Win driver matrix 完遂 (§3.7 r43-r44 milestone work の Win 寄与) | 0.50 | NVIDIA / AMD / Intel Arc 全 driver 動作確認、旧 driver fallback path 動作検証、driver-specific quirks 対応 (起動時 instance 初期化 timing 等) | 06 doc §3.7 |
| Win 純増分 (§4.2 = 1.35 base、§3.7 r43-r44 milestone との OS 軸重複排除済) | 1.35 | LLWindow Win32 surface 化 + `VK_KHR_win32_surface` 統合 + `VK_EXT_swapchain_maintenance1` Intel Arc 未対応 fallback + LunarG SDK Win 統合 + Win-specific bug fix 余裕 | 06 doc §4.2 + 05 doc §8.2 + §8.4 |
| **base work 計** | **1.85** | — | — |
| 余裕係数 +42% (Linux baseline 軸 +50% / Win 増分 +40% 加重平均) | +0.78 | WHCK 認定の手続 + driver matrix iterate + Win-specific quirks に対する加重 (Mac t-noami workflow 不確定性は r44 に集中、本 r43 は Linux baseline 補強 + Win の確定性高い work) | — |
| **r43 total (フルタイム dev 換算)** | **~2.63** | — | — |

注: Linux baseline parity 完遂自体は r42-δ acceptance #4 (vk-RC 直前 polish) + 06 doc §3.6 r42-δ work (1.50 base / 2.25 余裕係数後) で達成済、本 r43 では **Linux baseline 安定維持 (stable parity 状態の継続)** であり追加 work 計上なし。r43 は **Win parity 完遂 = 本 milestone の新規 work** が中心。

#### 工数の 06 doc との対応

- 06 doc §3.7 r43-r44 milestone work (2.00 PM base / 3.00 PM 余裕係数後、Linux baseline 軸の追加 polish): Mac MoltenVK 1.00 + Win driver matrix 0.50 + 性能 polish 0.50 のうち、**Win driver matrix 0.50 を本 r43 に配分** (Mac 1.00 + 性能 polish 0.50 は r44 へ)
- 06 doc §4.2 Win 純増分 (1.35 PM base / 1.89 PM 余裕係数後、OS 軸重複排除済): 本 r43 に全配分
- 合計 r43 base = 0.50 + 1.35 = **1.85 PM** → 余裕係数 +42% 加重平均適用後 ~**2.63 PM** ✓ (06 doc §3.7 + §4.2 + §4.3 の Win 寄与 + 重複排除構造と整合)

#### 暦月変換 (06 doc §5.3 + §5.6 整合)

| 換算項目 | 値 | 出処 |
|---|---|---|
| r43 base PM (Linux baseline 安定維持 + Win parity 完遂) | ~2.63 | 本 §3.1 work breakdown |
| 並走 ratio 中央値 4x (06 doc §5.2 確定) + 学習曲線 + Win 固有 quirks lead time + WHCK 認定の手続 | — | 06 doc §5.1 + §5.2 + §5.3 |
| r43 中央値暦月 (Linux baseline 安定維持 + Win 増分完遂までの total 期間) | ~20 暦月 (累積 ~152 暦月、~2039 年初) | 06 doc §5.3 + §5.6 (r42-δ 達成 ~132 + r43-r44 milestone Linux ~12 + Win 増分 ~8 = ~152 暦月) |
| 暦年マーカー | ~2038 年中 〜 2039 年初 | 06 doc §5.6 marker (Linux baseline parity 完遂 ~144 + Win 増分完遂 ~152 を r43 達成の終端と扱う) |

#### acceptance criteria draft (r43 着手前の charter 起草で詳細化)

1. **Linux baseline parity 完遂**: AYAstorm r1-r30 全機能 (audio r1-r13 / 視覚表現 r14-r24 / 3D stream r25-r29 / Cinematic r30 / chat / picker) が Linux 上で Vulkan 動作、本線 GL 実装と visual + 機能同等、edge case regression 無し
2. **Win driver matrix 完遂**: NVIDIA GeForce/Quadro (RTX 20/30/40 系) + AMD Radeon (RDNA 1/2/3) + Intel Arc/Iris Xe の Win 上での全 driver 動作確認、`VK_EXT_swapchain_maintenance1` Intel Arc 一部未対応 fallback (`vkDeviceWaitIdle` fallback) 動作、旧 driver fallback path 動作
3. **Win surface 化完遂**: LLWindow Win32 implementation で `VK_KHR_win32_surface` 経由 swapchain 動作、HWND 流用、現 GL WGL 経由は廃止
4. **WHCK 認定 minimum 版数の release note 整備**: Windows Hardware Compatibility Kit Vulkan logo program 経由 driver 認定 minimum 版数を release note に記載 (運用 doc 整備、AYA さん配信告知の前提)
5. **LunarG SDK Win 統合**: autobuild Win 統合 (LunarG SDK Win 版の path 構造差吸収)、Win build から SPIR-V cross compile chain 動作
6. **Win-specific bug fix 集中対応**: 起動時 instance 初期化 timing / driver-specific quirks 等、Win 限定 bug の集中 fix (memory `feedback_mac_only_fixes_accept_as_is` に倣い、Win 限定 fix は他開発者検証信任で as-is 受入の方針を Win にも適用)
7. **regression 無し**: r41 / r41.5 / r42-α/β/γ/δ baseline が stable、Linux baseline parity 状態が r43 work で degradation 無し

#### OS 着手 timing (06 doc §4.4)

- **Linux**: first-class、parity 補強完遂
- **Win**: **Win parity 完遂** (driver matrix 全 driver 動作 + 旧 driver fallback + WHCK 整備、r42-α 以降の累積 work を r43 で完遂宣言)
- **Mac**: Mac MoltenVK 詳細化 phase 継続 (r42-δ で詳細化開始、r43 では並走、parity 完遂は r44)

#### 依存 milestone

- **前 milestone**: r42-δ 達成 (Linux baseline parity 残機能 完遂 + vk-RC 直前 polish + Mac MoltenVK 詳細化開始)
- **interface 確立**: r42-α 以降の Win surface 化 base work が r42-α/β/γ/δ の各 milestone で incremental に進行済
- **設計 doc 確定**: 05 doc §8.2 Win driver matrix + §8.4 WSI Win32 + §7.6 旧 driver fallback

### §3.2 r44 (Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成) sub-milestone 構成

#### 目的

r43 で達成済の Linux + Win parity を **Mac MoltenVK 経由で 3 OS parity 完遂** + **vk-RC 達成宣言** (charter §4 (1) parity 完遂 goal の到達)。描画 stage は **vk-RC 完遂** (parity 補強 + 性能 polish + Mac portable subset 詳細化)。

#### work breakdown (06 doc §3.7 Mac 部分 + §4.3 Mac 増分 + 性能 polish 反映)

| 領域 | 工数 (PM) | 主作業 | 出典 |
|---|---|---|---|
| Mac MoltenVK 詳細化 (§3.7 の r43-r44 milestone 計上分 = 1.00) | 1.00 | MoltenVK portable subset (Vulkan 1.2 core + 一部 1.3 KHR) 詳細化、`VK_KHR_portability_subset` enable、t-noami さん workflow 連携での Mac 固有問題対応 | 06 doc §3.7 + 05 doc §8.3 + §9.4 |
| 性能 polish (frame in flight tuning / barrier sequence / VMA allocation strategy、§3.7 で 0.50 計上) | 0.50 | frame in flight 数の本格 tuning (2/3 切替の per-OS 最適化)、barrier sequence の per-pass optimal 化、VMA allocation strategy の本格 tuning (UMA Apple Silicon 含む 3 OS 全部) | 06 doc §3.7 + 05 doc §5 + §6 |
| Mac 純増分 (§4.3 = 2.70 base、重複排除済) | 2.70 | LLWindow Mac surface 化 (`VK_EXT_metal_surface`) + Apple Silicon UMA 対応 + MoltenVK 1.2 core fallback path + MSL 経由 shader 動作確認 (spirv-cross MSL 変換 + MoltenVK runtime) + t-noami さん workflow cycle + Mac-specific bug fix 余裕 | 06 doc §4.3 + 05 doc §8.3 + §8.4 + §9.1-§9.4 |
| **base work 計** | **4.20** | — | — |
| 余裕係数 +50% (Mac t-noami workflow cycle + MoltenVK portable subset untested feature + macOS 14+ Metal 3 minimum 動作確認 cycle) | +2.10 | — | — |
| **r44 total (フルタイム dev 換算)** | **~6.30** | — | — |

#### 工数の 06 doc との対応

- 06 doc §3.7 r43-r44 milestone work (3.00 PM、余裕係数 +50% 適用後) のうち、Mac MoltenVK 詳細化 1.00 + 性能 polish 0.50 を本 r44 に配分: ~1.50 PM (work) → 余裕係数適用後 ~2.25 PM (Mac + 性能 polish 寄与)
- 06 doc §4.3 Mac 増分 (4.05 PM、余裕係数 +50% 適用後) を本 r44 に配分: 4.05 PM
- 合計 ~6.30 PM (= 2.25 + 4.05 = §3.7 r43-r44 milestone の Mac + 性能 polish 寄与 + §4.3 Mac 純増分)

注: r43 + r44 合計 ~2.63 + ~6.30 = **~8.93 PM**、06 doc §4.5 3 OS 合計 (35.84 PM) - r41 (16.17) - r41.5 (1.50) - r42 α+β+γ+δ (9.23) = **8.94 PM** と整合 ✓ (差 ~0.01 PM は丸め誤差範囲内、本 (d) の r43/r44 sub-milestone level 分割が 06 doc §4.5 の vk-RC 累積と整合)。

#### 暦月変換 (06 doc §5.3 + §5.6 整合)

| 換算項目 | 値 | 出処 |
|---|---|---|
| r44 base PM (Mac + 性能 polish) | ~6.30 | 本 §3.2 work breakdown |
| 並走 ratio 中央値 4x + 学習曲線 +5-10% (Vulkan 経験 + MoltenVK 経験積み済、学習曲線は最も低い段階) | — | 06 doc §5.1 + §5.2 |
| r44 中央値暦月 | ~18 暦月 (累積 ~170 暦月、~2040 年後半 = vk-RC 達成) | 06 doc §5.3 + §5.6 (vk-RC 3 OS parity 完遂 ~170 暦月) |
| 暦年マーカー | ~2039 年初 〜 2040 年後半 (vk-RC 達成) | 06 doc §5.6 marker |

#### acceptance criteria draft (r44 着手前の charter 起草で詳細化)

1. **Mac parity 完遂**: AYAstorm r1-r30 全機能 (audio r1-r13 / 視覚表現 r14-r24 / 3D stream r25-r29 / Cinematic r30 / chat / picker) が Mac (macOS 14+ Metal 3 minimum) 上で MoltenVK 経由 Vulkan 動作、Linux baseline と visual + 機能同等
2. **MoltenVK portable subset 動作確認**: `VK_KHR_portability_subset` enable 下で本 design 利用 feature の MoltenVK 実装動作確認、1.3 機能の MoltenVK 1.2 fallback path 動作 (dynamic rendering / sync2 / push descriptor 等)
3. **Mac surface 化完遂**: LLWindow Mac implementation で `VK_EXT_metal_surface` 経由 swapchain 動作 (NSView → CAMetalLayer)、CGL/AGL 廃止
4. **Apple Silicon UMA 対応**: VMA `_AUTO_PREFER_HOST` 動作確認、`VK_EXT_memory_budget` query 整合、M1/M2/M3 (Apple Silicon) で UMA 経由 frame time が NVIDIA/AMD/Intel discrete GPU と同等帯
5. **MSL 経由 shader 動作確認**: 248 + 13 file (base port + AYAstorm 機能) の spirv-cross MSL 変換 + MoltenVK runtime 動作、Mac 上での shader 動作 smoke test (memory `feedback_credit_t_noami_equal_billing` 反映、t-noami さん検証 cycle 経由)
6. **t-noami workflow cycle 完遂**: Linux build 完成 → t-noami さん検証 → patch return cycle が回り、r44 終盤までに Mac 固有 quirk が全て resolve (memory `feedback_credit_t_noami_equal_billing` 対等並列、`feedback_mac_only_fixes_accept_as_is` 受入方針)
7. **性能 polish 3 OS 適用**: frame in flight tuning (2/3 切替の per-OS 最適化) + barrier sequence per-pass optimal 化 + VMA allocation strategy 本格 tuning が Linux + Win + Mac 全部で適用、本線 GL と同等以上の frame time (regression ≤10%)
8. **vk-RC 達成宣言**: §3.3 acceptance criteria draft 全項目 PASS で **vk-RC 達成** = **r44 達成** = **charter §4 (1) parity 完遂 goal の到達** = **r40 章工程プランの完遂 marker** (詳細 §3.3)
9. **regression 無し**: r41 / r41.5 / r42-α/β/γ/δ / r43 baseline が stable、Linux baseline parity + Win parity が r44 work で degradation 無し

#### OS 着手 timing (06 doc §4.4)

- **Linux**: first-class baseline、性能 polish 適用
- **Win**: r43 parity 完遂状態の維持 + 性能 polish 適用 (Linux baseline と同等 frame time 帯)
- **Mac**: **Mac parity 完遂** (MoltenVK 詳細化 + t-noami workflow cycle の最終 lap、vk-RC 3 OS parity 完遂)

#### 依存 milestone

- **前 milestone**: r43 達成 (Linux baseline + Win parity 完遂)
- **interface 確立**: r42-β 以降の Mac portable subset check 累積 + r42-δ Mac MoltenVK 詳細化開始 work
- **設計 doc 確定**: 05 doc §8.3 Mac driver capability matrix + §8.4 WSI metal surface + §9.4 MoltenVK portable subset 詳細化

### §3.3 vk-RC 達成宣言の acceptance criteria draft

#### vk-RC 達成 = r44 達成 = charter §4 (1) parity 完遂 goal の到達 marker

vk-RC 達成宣言は **r40 章工程プランの最終 marker** (本 (d) §1.5 確定済)。AYAstorm r1-r30 全機能を Linux + Win + Mac 3 OS で Vulkan 上に再現、charter §4 (1) で確定済の完遂 goal を達成した時点で宣言。

#### vk-RC 達成 acceptance criteria draft (charter §4 (1) parity 完遂 goal の具体 metric)

| 軸 | acceptance criterion | source |
|---|---|---|
| (1) **全機能 parity (3 OS)** | AYAstorm r1-r30 全機能が Linux + Win + Mac の 3 OS 全部で Vulkan 上に再現、本線 GL 実装と visual + 機能同等、edge case regression 無し | charter §4 (1) |
| (2) **audio chapter (r1-r13)** | r1-r13 audio 系全機能 (FMOD callback + Dullahan path + 配信周辺) が 3 OS Vulkan 上で本線 GL と同等動作、Vulkan invariant (memory `project_pr69_fallback_switch` 反映) | r42-δ acceptance #2 + 本 §3.2 acceptance #1 |
| (3) **視覚表現 chapter (r14-r24)** | r14+ visual realism (sky dome + atmospherics + post-process chain 7 sub-pass + scene buffer alpha invariant 遵守) が 3 OS Vulkan 上で本線 GL と visual 同等、live A/B (sustained viewing) で cumulative 効果も観測 (memory `project_aya_visual_realism_alpha_protect` / `project_atmos_atten_scalarized` / `feedback_shader_color_space_correction` / `feedback_instant_ab_vs_sustained` / `feedback_visual_decisions_need_live_ab` 反映) | r42-γ acceptance #1-#6 + 本 §3.2 acceptance #1 |
| (4) **3D stream chapter (r25-r29)** | NDI / OBS / 3D stream 配信機能が 3 OS Vulkan 上で本線 GL と同等の stream quality、`VK_KHR_external_memory_*` 予約活用 (r42-δ で最低限実装、本格は r45+) | r42-δ acceptance #1 + 本 §3.2 acceptance #1 |
| (5) **Cinematic chapter (r30)** | r30 Cinematic mode (DoF state enum 化 + BD cvar 13 件 visual A/B + Cinematic Controls 全機能) が 3 OS Vulkan 上で本線 GL と visual 同等、BD cvar 13 件 live A/B で visual 同等性確認 (memory `project_r30_cinematic_control_tuning_deferred` 反映) | r42-β acceptance #1-#4 + 本 §3.2 acceptance #1 |
| (6) **picker / chat / その他** | r21.1 self-rigged picker + chat tab split + その他 r1-r30 全機能の 3 OS Vulkan parity | r42-α acceptance #1-#3 + 本 §3.2 acceptance #1 |
| (7) **3 OS driver coverage** | Linux: Mesa RADV + Mesa ANV + NVIDIA proprietary first-class / Win: NVIDIA GeForce + AMD Radeon + Intel Arc first-class + 旧 driver fallback / Mac: macOS 14+ Metal 3 + MoltenVK 1.2.x portable subset first-class | 05 doc §8.1-§8.3 + 本 §3.1-§3.2 acceptance |
| (8) **性能 polish** | frame in flight tuning + barrier sequence + VMA allocation strategy 本格 tuning 適用、本線 GL と同等以上の frame time (regression ≤10%)、AYAstorm 開発機 (AMD RX 7900 XTX + Mesa RADV) で baseline 確認 + 各 OS で baseline 比較 | 本 §3.1 acceptance #1 + §3.2 acceptance #7 |
| (9) **release note + 運用 doc** | vk-RC 達成 release note + 3 OS driver minimum 版数 + WHCK 認定情報 + MoltenVK 1.2 fallback note の整備 (memory `feedback_release_notes_link_only` / `feedback_release_note_per_feature` 反映、永続 spec は本 (d) + r41-r44 charter doc に集約) | 本 §3.1 acceptance #4 + §3.2 acceptance #2 |
| (10) **regression sweep** | r41 / r41.5 / r42-α/β/γ/δ / r43 baseline が stable、本 r44 work で **全 milestone の累積 regression 無し** | 本 §3.1 acceptance #7 + §3.2 acceptance #9 |

#### vk-RC 達成宣言時の運用

- **宣言主体**: AYA さんが acceptance criteria 全項目 PASS 確認後に宣言
- **宣言場所**: r44 charter の達成宣言 section + r40 章 charter §4 (1) 完遂 marker の到達記録
- **宣言効果**: r40 章工程プランの完遂 marker、charter §3 「時間軸では撤退条件を設けない」遵守下での parity 完遂 goal 到達
- **宣言後の作業**: r45+ scope (本算定範囲外 = 本 §4) の broad outline 確認 → 別章 charter 起草の AYA 擦り合わせ開始 (本 §4.3 反映)

#### draft の位置付け再掲

本 §3.3 acceptance criteria は **draft** (各 milestone 着手前の charter 起草時に詳細化)。各項目の具体 metric / test procedure / regression criteria は r44 charter 起草時に詳細化 (本 §2.5 運用方針継承)。

### §3.4 3 OS parity 完遂 marker

#### 3 OS 着手 → 完遂 cadence (06 doc §4.4 反映)

charter §4 (2) Linux 先行 + 3 OS 大前提 (memory `project_ayastorm_three_platforms`) を本 (d) で正式 mapping:

| milestone | Linux | Win | Mac |
|---|---|---|---|
| r41 | **着手 + first-class baseline** (全 work) | (本線 GL 維持) | (本線 GL 維持) |
| r41.5 | first-class | (Win/Mac 着手前) | (Win/Mac 着手前) |
| r42-α | first-class | **着手** (LLWindow Win32 surface 化 + driver matrix 着手) | (本線 GL 維持) |
| r42-β | first-class | 並走 (driver matrix 継続) | **着手** (portable subset check + t-noami さん事前共有) |
| r42-γ | first-class | 並走 | 並走 (t-noami さん検証 cycle 始動 + MoltenVK 1.2 fallback path 整備) |
| r42-δ | first-class | polish (旧 driver fallback + WHCK 整備) | **MoltenVK 詳細化開始** (05 doc §9.4 / §8.3 vk-RC 直前 phase) |
| **r43** | **first-class baseline parity 完遂** (~144 暦月、~2038 年中) | **Win parity 完遂** (~152 暦月、~2039 年初) | MoltenVK 詳細化継続 |
| **r44** | first-class baseline 維持 + 性能 polish | Win parity 維持 + 性能 polish | **Mac parity 完遂 = vk-RC 達成** (~170 暦月、~2040 年後半) |

#### 3 OS parity 完遂順の根拠

- **Linux 先行 (r41-r43 完遂)**: charter §4 (2) Linux 先行 + 開発機 AMD/Linux baseline (memory `project_ayastorm_three_platforms`)、AYAstorm 開発機での first-class 検証が最優先
- **Win 完遂 (r43)**: r42-α で着手 → r42-β/γ/δ 並走 + polish 累積 → r43 で parity 完遂、driver matrix 全 driver + 旧 driver fallback + WHCK 認定の incremental 整備
- **Mac 完遂 (r44 = vk-RC 達成)**: r42-β で着手 → r42-γ で並走 → r42-δ で MoltenVK 詳細化開始 → r43 で MoltenVK 詳細化継続 → r44 で parity 完遂、t-noami さん workflow cycle 経由の Mac 固有 quirk resolve

#### t-noami さん workflow cycle の lead time

memory `feedback_credit_t_noami_equal_billing` + `feedback_mac_only_fixes_accept_as_is` 反映:

- t-noami さん検証 cycle は Linux build 完成 → 検証 → patch return の lead time が cycle 単位で発生
- r42-β 〜 r44 の cycle 数 = AYAstorm 進化中の各 milestone × t-noami さん検証 lap
- r44 終盤までに全 Mac 固有 quirk を resolve、Mac 限定 fix は他開発者検証信任で as-is 受入 (本 §3.2 acceptance #6)

#### 3 OS parity 完遂宣言の運用

- **r43 完遂宣言**: Linux + Win parity 完遂 (charter §4 (2) Linux 先行 + Win 後追いの中間 marker)、r44 着手前提
- **r44 完遂宣言 = vk-RC 達成宣言**: 3 OS parity 完遂 (charter §4 (1) 完遂 goal 到達)、本 §3.3 acceptance criteria 全項目 PASS で宣言
- **宣言後**: r40 章工程プラン完遂 marker、本 (d) 算定範囲終了、r45+ scope (本算定範囲外) の broad outline 確認 → 別章 charter 起草 (本 §4.3 反映)

### §3.5 §3 結論 (r43-r44 区切り確定)

本 §3 で確定した r43-r44 区切り:

1. **r43-r44 = vk-RC (parity 補強 + 性能 polish + 3 OS parity 完遂) に振替え** (§3.0)、charter §6 仮 line up の r43 = vk-γ / r44 = vk-δ は本 §1.3 で r42-β/γ/δ に分散完遂、r43-r44 は parity 補強に振替え
2. **r43 = Linux baseline 安定維持 + Win parity 完遂** (~2.63 PM / ~20 暦月 / ~2038 年中 〜 2039 年初): Win driver matrix 全 driver + 旧 driver fallback + WHCK 認定整備 + LunarG SDK Win 統合 (§3.1、Linux baseline parity 完遂自体は r42-δ acceptance #4 で達成済、r43 は安定維持のみ。暦月は Win 固有 quirks lead time + WHCK 認定手続で並走 ratio 名目より長期化)
3. **r44 = Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成** (~6.30 PM / ~18 暦月 / ~2039 年初 〜 2040 年後半): MoltenVK portable subset + Apple Silicon UMA + MSL 経由 shader + t-noami workflow cycle + 性能 polish 3 OS 適用 (§3.2)
4. **vk-RC 達成 acceptance criteria draft 10 軸確定**: 全機能 parity (3 OS) / audio / 視覚表現 / 3D stream / Cinematic / picker / 3 OS driver coverage / 性能 polish / release note / regression sweep (§3.3)
5. **3 OS parity 完遂順 = Linux 先行 → Win 後追い (r43) → Mac 後追い (r44)** が確定 (§3.4)、charter §4 (2) Linux 先行 + 3 OS 大前提 (memory `project_ayastorm_three_platforms`) 整合
6. **r43-r44 合計 ~8.93 PM / ~38 暦月 / ~3.2 年**、06 doc §4.5 3 OS 合計 (35.84) - r41 (16.17) - r41.5 (1.50) - r42 (9.23) = 8.94 PM と整合 ✓ (差 ~0.01 PM は丸め誤差範囲内、暦月は 06 doc §5.6 marker (~170 - ~132 = ~38 暦月) と整合)

→ 本 §3 確定値 + 本 §2 r42 内訳 (~9.23 PM / ~40.7 暦月) + r41 (16.17 PM / ~84.1 暦月) + r41.5 (1.50 PM / ~7.2 暦月) で **vk-RC 累積 ~35.84 PM / ~170 暦月 / ~14.2 年 / ~2040 年後半** が確定、charter §4 (3) 想定 15-30 年帯の下方近接で **本算定中央値 = charter §4 (3) 想定範囲内** ✓

---

## §4 r45+ 区切り (本算定範囲外)

### §4.0 算定方針

charter §3 「時間軸では撤退条件を設けない」 + charter §4 (3) 「無期限 / AYA life plan」 + 06 doc §3.8 範囲外宣言 + 本 (d) §1.5 r45+ 範囲外宣言 を 4 重に遵守し、本 §4 では **r45+ scope の broad placeholder** のみ提示。詳細化は r44 達成宣言後の別章 charter 起草で実施。

#### 4 重遵守の根拠

| 出典 | 内容 | 本 §4 への含意 |
|---|---|---|
| charter §3 | 「時間軸では撤退条件を設けない」 | r45+ 着手 timing は r44 達成後の AYAstorm 体制 / industry 状況 / LL 着地 status 次第、本 (d) で時期を固定すると charter §3 違反 |
| charter §4 (3) | 「無期限 / AYA life plan 前提で 6-15 人年規模」 | 本算定は vk-RC 完遂 (r44 達成) までの 6-15 人年規模算定、r45+ を含めると charter §4 (3) 上限超過の risk、charter §4 (3) 想定範囲を維持するため r45+ は別算定 |
| 06 doc §3.8 | 「r45+ visual realism 次世代は本算定範囲外」 | 06 doc 段階で範囲外宣言済、本 (d) は同方針継承 |
| 本 (d) §1.5 | 「vk-RC 達成 = r44 達成 = 本算定終了 marker」 | foundation group §1.5 で確定済、group A §4 で broad outline のみ提示 |

#### 本 §4 で扱う範囲

- §4.1 r45+ scope の broad outline (visual realism 次世代 / ray tracing / HDR / GPU-driven の topic 列挙、詳細化なし)
- §4.2 r45+ 着手 trigger 条件 (r44 達成 + AYA judgment、時間軸 trigger 無し、charter §7 LL 着地時判断指針 + §8 plan B trigger との連動方針)
- §4.3 r45+ charter 起草 timing (r44 達成宣言 + 6 か月以内に AYA さんと擦り合わせて別章 charter 起草)

詳細 acceptance criteria / work breakdown / 暦月変換 / uncertainty band は本 §4 では **扱わない** (詳細化は別章 charter で実施)。

### §4.1 r45+ scope の broad outline

#### r45+ topic 列挙 (charter §6 仮 line up + 05 doc §9.5 反映)

charter §6 仮 line up + 05 doc §9.5 で予約のみ採用された topic を broad outline で列挙:

| topic 領域 | 内容 broad outline | 出典 |
|---|---|---|
| **visual realism 次世代** | r14+ 章 thesis 「写真を撮るに値する空気と空間」(memory `project_ayastorm_visual_realism_chapter`) の next iteration、AYAstorm 独自進化路線 | charter §3 + memory |
| **ray tracing** | `VK_KHR_ray_tracing_pipeline` + `VK_KHR_acceleration_structure` 活用、reflection / shadow / GI 等の hardware ray tracing 実装 (Mac MoltenVK 非対応のため 3 OS parity 対象外、Linux/Win first-class) | 05 doc §9.5 予約 |
| **HDR (High Dynamic Range)** | 10-bit / 12-bit per channel HDR display 対応 + HDR-aware tonemap + monitor calibration | charter §3 |
| **GPU-driven rendering** | indirect draw / draw call merging / GPU-side culling + scene graph traversal、`VK_EXT_mesh_shader` 活用での mesh shader pipeline 導入 | 05 doc §9.5 予約 |
| **AYAstorm 独自進化** | r14+ 章を含む AYAstorm 独自路線の自由扱い (charter §3 「parity 完遂後の self-driven 章」) | charter §3 |

#### outline 詳細化を本 §4 で扱わない理由

- **時系列長による不確実性**: r45+ 着手 timing が ~2040 年後半以降 = AYAstorm + Vulkan ecosystem + industry 状況の予測不可
- **AYAstorm 進化追随**: r44 達成時点で AYAstorm 本体が r31 / r32 等の追加機能を持っている可能性、r45+ scope は r44 達成時の AYAstorm parity goal に追加される形で決まる
- **LL 着地 status 反映**: charter §7 LL 着地時判断指針に従い、r45+ 着手前に LL 公式 Vulkan の status を audit、scope に LL 着地 reset の選択肢を反映
- **本 (d) の目的 (r40 達成)**: r40 章工程プランの完遂 = vk-RC parity 完遂 (r44 達成) までの算定、r45+ は parity 完遂後の self-driven 章として独立算定 (06 doc §3.8 範囲外宣言継承)

### §4.2 r45+ 着手 trigger 条件

#### 着手 trigger = r44 達成 + AYA judgment、時間軸 trigger 無し

charter §3 「時間軸では撤退条件を設けない」を r45+ 着手 trigger にも適用、時間軸 trigger 無し:

| trigger 種別 | r45+ 着手判断への含意 |
|---|---|
| **必須 trigger**: r44 達成 (= vk-RC parity 完遂 = charter §4 (1) 完遂 goal 到達) | r44 達成前に r45+ 着手は不可、本 (d) §3.3 acceptance criteria 全項目 PASS が前提 |
| **必須 trigger**: AYA judgment (charter §3 「AYA life plan」前提) | r44 達成後の AYAstorm 体制 / 健康 / industry 状況の AYA 評価、r45+ scope への commit 可否を AYA さんが判断 |
| **任意 trigger**: charter §7 LL 着地時判断指針 + §8 plan B trigger の発動 | LL 公式 Vulkan 着地 + AYAstorm-vk parity 完遂後の場合、charter §7 判断軸 1 「vk-RC 後」では reset cost 最大 / LL 採用 merit 低の評価、§7 (iii) maintain + 独自 visual realism 路線 が r45+ scope に重なる可能性 |
| **時間軸 trigger 無し** (charter §3 遵守) | r44 達成後 N 年以内に r45+ 着手の義務 / N 年経過で r45+ 諦め 等の時間軸条件は **設けない** |

#### r45+ 着手と charter §7 / §8 の連動

charter §7 LL 着地時判断指針:

- r44 達成後 (= vk-RC parity 完遂後) に LL 公式 Vulkan 着地した場合、charter §7 判断軸 1 「vk-RC 後 (parity 完遂)」 = reset cost 最大 / LL 採用 merit 低、AYAstorm-vk maintain + 独自 visual realism 路線 = r45+ scope に重なる
- r44 達成前に LL 公式 Vulkan 着地した場合、charter §7 判断軸 1 / 2 / 3 を本 (d) §1.5 r45+ 範囲外宣言と独立に発動、r45+ scope は影響を受けない (r45+ 着手 trigger 必須 = r44 達成のため)

charter §8 plan B trigger:

- charter §8 (E) 「LL Vulkan release されたが quality が AYAstorm 用途 (撮影描画 / Cinematic) に届かない」 → (ii) maintain に倒すか plan B 別 backend (D3D12 / Metal native / WebGPU) 検討、r45+ scope は plan B 検討対象から外す方針 (charter §8 trigger 発動時は別 backend 検討が優先、r45+ scope は AYAstorm-vk 継続前提)

#### 着手 trigger の判定主体

- **AYA judgment 主体**: r44 達成後の AYAstorm 体制 / 健康 / industry 状況の評価は AYA さん主体
- **Claude 補助**: r45+ scope の broad outline + 別章 charter 起草の technical draft + memory / charter / doc cross-reference は Claude 補助

### §4.3 r45+ charter 起草 timing

#### 起草 cadence = r44 達成宣言 + 6 か月以内に AYA 擦り合わせ開始

r44 達成宣言後の r45+ charter 起草の cadence:

| 段階 | timing | 主体 | 内容 |
|---|---|---|---|
| r44 達成宣言 | ~2040 年後半 (06 doc §5.6 marker) | AYA + Claude | vk-RC 3 OS parity 完遂 + 本 (d) §3.3 acceptance criteria 全項目 PASS |
| r45+ 着手 trigger 評価 | r44 達成宣言直後 | AYA 主体 | 本 §4.2 trigger 条件評価 (AYA judgment + charter §7 / §8 連動評価) |
| r45+ scope 擦り合わせ開始 | r44 達成宣言 + ~3 か月以内 | AYA + Claude | 本 §4.1 broad outline を base に AYA さんと scope 擦り合わせ、AYAstorm 進化路線の next iteration を確定 |
| r45+ charter 起草 | r44 達成宣言 + ~6 か月以内 | Claude 主体 (AYA review) | `docs/specs/ayastorm-r45-plus-xxx/00-charter.md` 起草 (xxx は scope による、例: `r45-plus-raytracing` / `r45-plus-hdr` / `r45-plus-gpu-driven` 等の分章) |
| r45+ 着手 | r45+ charter 完成後 | AYA + Claude | r45+ 章 active 化、本 (d) 算定範囲外 |

#### 起草 cadence の根拠

- **r44 達成宣言 + 6 か月以内**: charter §3 時間軸非設定遵守下での「合理的な擦り合わせ期間」、AYA さんの体制 / 健康 / industry 状況の評価期間として 3-6 か月の cadence
- **6 か月以内に擦り合わせ完了しない場合**: charter §3 「時間軸では撤退条件を設けない」遵守、r45+ scope 擦り合わせを継続 (期限切れでの r45+ 諦めは無し)
- **r45+ scope 分章**: r45+ は visual realism 次世代 / ray tracing / HDR / GPU-driven が独立 topic なので 1 charter にまとめず、必要に応じて分章可能 (`r45-plus-raytracing` / `r45-plus-hdr` 等)

#### 起草 doc の location

- **r40 章内**: 本 (d) §4 placeholder のみ、詳細は r40 章外の別 doc
- **r45+ charter 起草先**: `docs/specs/ayastorm-r45-plus-xxx/00-charter.md` (r40 章とは別 directory)
- **r40 章 charter (00-charter.md) §6 への反映**: r45+ broad outline のみ、charter §6 末尾に「r45+ は別章 charter で扱う、本 charter §6 では broad outline のみ反映」を記載 (group B §6 反映で実施予定)

### §4.4 §4 結論 (r45+ 区切り = 本算定範囲外確定)

本 §4 で確定した r45+ 区切り:

1. **r45+ = 本算定範囲外、broad placeholder のみ** (§4.0): charter §3 / §4 (3) + 06 doc §3.8 + 本 (d) §1.5 の 4 重遵守、詳細化は r44 達成後の別章 charter で実施
2. **r45+ scope = visual realism 次世代 / ray tracing / HDR / GPU-driven / AYAstorm 独自進化** (§4.1): charter §6 仮 line up + 05 doc §9.5 予約継承、本 (d) では broad outline 列挙のみ
3. **r45+ 着手 trigger = r44 達成 + AYA judgment、時間軸 trigger 無し** (§4.2): charter §7 LL 着地時判断指針 + §8 plan B trigger と連動、AYA 主体 + Claude 補助
4. **r45+ charter 起草 cadence = r44 達成宣言 + 6 か月以内に AYA 擦り合わせ開始** (§4.3): `docs/specs/ayastorm-r45-plus-xxx/00-charter.md` 別 directory、必要に応じて分章

→ 本 §4 確定により、**r40 章工程プランの終了 marker = vk-RC 達成 (r44 達成) = ~2040 年後半** が確定、r45+ は本算定範囲外 + 別章 charter で扱う方針。group B §5 charter outline + §6 charter §6 反映 で本 §4 結論を 00-charter.md §6 に反映予定。

---

## §5 各 milestone の charter 草案 outline

### §5.0 算定方針 — charter outline 統一 template

#### 目的

本 (d) §1-§4 で確定した r41 / r41.5 / r42-α/β/γ/δ / r43 / r44 の 8 milestone について、各 milestone 着手前に起草する charter (`docs/specs/ayastorm-r4X-xxx/00-charter.md`) の **outline 統一 template** を本 §5 で提示する。実際の charter 起草は各 milestone 着手前に AYA + Claude で実施 (本 (d) §2.5 acceptance criteria 運用方針継承)、本 §5 では outline (header + thesis + work breakdown + acceptance criteria draft + 暦月 + 依存 + 起草 cadence + 詳細化方針 + 関連 doc) のみ確定。

#### outline 統一 template (各 milestone charter 起草の base 8 section 構成)

| section | 内容 | 出処 |
|---|---|---|
| **header** | `# AYAstorm rXX (milestone short name)` + status (起草前) + 親 doc (r40 章 charter `00-charter.md`) + 前置 doc (本 (d) `07-r42-plus-milestone-mapping.md` + 必要なら 04/05/06 doc) | 統一 template |
| **§1 milestone thesis** | 本 milestone の goal (描画 stage 達成範囲 + AYAstorm 機能 port + 3 OS 着手 timing) + 完遂後の next milestone への引継ぎ | 本 (d) §2 / §3 |
| **§2 work breakdown** | 本 (d) §2.1-§2.4 / §3.1-§3.2 の work breakdown table 継承、各領域の work + 出典 + base PM + 余裕係数 | 本 (d) §2 / §3 + 06 doc §3 |
| **§3 acceptance criteria** | 本 (d) §2.1-§2.4 / §3.1-§3.2 の acceptance criteria draft 継承、各項目に具体 metric / test procedure / regression criteria 追加 | 本 (d) §2 / §3 |
| **§4 暦月変換 + 暦年マーカー** | 本 (d) §2 / §3 の中央値暦月 + 06 doc §5.3 並走 ratio + §5.6 marker 暦年 | 本 (d) §2 / §3 + 06 doc §5 |
| **§5 依存 milestone** | 前 milestone 完了 + interface 確立 + 設計 doc 確定 (本 (d) §2 / §3 「依存 milestone」継承) | 本 (d) §2 / §3 |
| **§6 起草 timing / 主体** | 本 (d) §2.5 cadence (本 milestone 着手前に AYA + Claude で起草)、起草先 directory pattern | 本 (d) §2.5 |
| **§7 詳細化方針** | 本 (d) outline は base、charter 起草時に追加詳細化 (具体 metric / test procedure / 各 file の port 順 / 詳細 schedule / memory 反映 等) | 本 (d) §2.5 |
| **§8 関連 doc / memory** | r40 章内部 doc (00-charter / 03/04/05/06/07) + 関連 memory | 統一 template |

#### template の運用方針

- **本 (d) §5 段階 = outline 提示**: 上記 8 section 構成と各 section の base 内容 (本 (d) §2-§3 継承) を確定
- **実 charter 起草 = 詳細化**: 各 milestone 着手前に上記 outline を base に AYA + Claude で詳細化、`docs/specs/ayastorm-r4X-xxx/00-charter.md` 起草
- **起草 cadence**: 本 (d) §2.5 で確定 (前 milestone 達成宣言直後に着手前 charter 起草)
- **詳細化が本 (d) で確定値ではない理由**: 本 (d) §2.5 反映 3 件 (時系列長 / AYAstorm 進化追随 / LL 着地 status)、charter 起草時に当時の状況で詳細化

### §5.1 r41 charter outline (GL 除去 + Vulkan 空転)

#### §1 milestone thesis

AYAstorm 本線から **OpenGL を完全除去** + **Vulkan で空転動作** (swapchain + render pass + 黒画面 + UI 描画) 達成。描画 stage は **vk-α** (空転)。parity 不要 (charter §6 仮 line up = AYAstorm 機能 pull-in なし)、04 doc §5.4 段階 1-5 を完遂。Linux 限定 first-class baseline (Win/Mac は r42-α / r42-β で Vulkan 着手、本 r41 段階では本線 GL 維持)。

#### §2 work breakdown (本 (d) §1.4 + 06 doc §3.1 / §3.2 反映)

| 領域 | 工数 (PM) | 主作業 | 出典 |
|---|---|---|---|
| 段階 1: GL header wrapper 置換 + volk loader | 0.5 | 212 GL header → volk-based 置換 + Vulkan instance / device 初期化 | 04 doc §5.4 + 06 doc §3.1 |
| 段階 2: lldrawpool Vulkan 化 (13 file) | 1.0 | lldrawpool 系 13 file の GL call → Vulkan command buffer 化 | 04 doc §5.4 |
| 段階 3: state machine → PSO 化 (llrender 主要 5 file) | 1.5 | llrender state machine の Vulkan PSO 化 + render pass 統合 | 04 doc §5.4 + 06 doc §3.2 |
| 段階 4: pipeline.cpp 3 大グローバル → frame context | 1.0 | pipeline.cpp 3 大グローバル (描画 state / cull / stateSort) → LLPipelineFrameContext 集約 | 04 doc §5.4 + 06 doc §3.2 |
| 段階 5: llspatialpartition / llviewershadermgr / llvertexbuffer 依存解決 | 0.5 | 残依存 file の Vulkan 等価実装 | 04 doc §5.4 |
| 248 GLSL shader SPIR-V 化 (base port 分 ~228 file) | 4.96 | base shader の SPIR-V cross compile + descriptor set 整合 (AYAstorm 機能 13 file は r42-α/β/γ で port) | 06 doc §3.1 + §3.2 |
| descriptor set + render pass 設計反映 | 1.50 | 05 doc §3 + §4 base 実装 (per-frame / per-material / per-draw 3 階層) | 06 doc §3.1 |
| Vulkan code abstraction skeleton (interface placeholder) | 0.50 | 05 doc §10 LLVKRenderer interface 骨子 (空転完成までは pipeline.cpp 内 inline、r41.5 で interface 経由 call に置換) | 06 doc §3.1 + 05 doc §10.1-§10.2 |
| swapchain + present + UI 黒画面動作確認 | 0.40 | viewer 起動 → 黒画面 + UI 描画 (vk-α 空転 acceptance) | 06 doc §3.1 |
| Linux 限定 baseline polish | 0.36 | Linux Mesa RADV / Mesa ANV / NVIDIA proprietary first-class driver matrix 初期動作確認 | 06 doc §3.2 |
| **base work 計** | **11.78** | — | 04 doc §5.4 + 06 doc §3.1-§3.2 集計 |
| 余裕係数 +37% (低 risk path 多くも base port 248 shader cross compile / 3 大グローバル refactor の不確実性反映) | +4.39 | — | 06 doc §3.2 |
| **r41 total (フルタイム dev 換算)** | **~16.17** | — | 06 doc §3.9 |

#### §3 acceptance criteria draft

1. **GL 除去完遂**: 本線 binary から OpenGL link / runtime call を完全除去、`ldd` 等で確認 (Linux baseline)
2. **Vulkan 空転動作**: viewer 起動 → swapchain 経由で黒画面 + UI 描画 (vk-α 空転)、Linux Mesa RADV / NVIDIA proprietary で動作
3. **段階 1-5 全完遂**: 04 doc §5.4 段階 1 (GL header wrapper) / 段階 2 (lldrawpool) / 段階 3 (state machine PSO) / 段階 4 (pipeline.cpp 3 大グローバル) / 段階 5 (残依存) 全 file の Vulkan 化
4. **248 shader SPIR-V 化 base port**: ~228 file (base port、AYAstorm 機能 13 file は r42-α/β/γ で port) の SPIR-V cross compile 動作
5. **descriptor set + render pass 設計実装**: 05 doc §3 + §4 設計に従った per-frame / per-material / per-draw 3 階層 descriptor set + deferred main pass + post-process pass の Vulkan render pass 構造
6. **LLVKRenderer interface skeleton**: 05 doc §10.1-§10.2 hook 配置済、ただし pipeline.cpp 内 inline 実装 (interface 経由 call への置換は r41.5)
7. **Linux baseline first-class**: Mesa RADV / Mesa ANV / NVIDIA proprietary の 3 driver で空転動作、本線 GL 除去後の Linux 安定動作確認
8. **regression sweep**: AYAstorm r1-r30 機能のうち audio (r1-r13) + 描画非依存機能は本線 GL 除去後も動作 (本 r41 段階では描画依存機能は vk-α 空転のため動作不可、parity 不要)
9. **Win/Mac 未着手宣言**: Win/Mac の Vulkan 着手は r42-α / r42-β、r41 段階では Linux 限定 first-class baseline (本線 GL 維持 Win/Mac は r41 達成時点で停止 + Vulkan 着手は r42-α/β)

#### §4 暦月変換 + 暦年マーカー (06 doc §5.3 + §5.6 反映)

| 換算項目 | 値 | 出処 |
|---|---|---|
| r41 base PM | 16.17 | 06 doc §3.9 |
| 並走 ratio 中央値 4x (本職並走 charter §4 (3) 反映) | — | 06 doc §5.2 |
| 学習曲線 +20-30% (Vulkan 初期学習 cost、全 milestone 中最も高い段階) | — | 06 doc §5.1 |
| r41 中央値暦月 | ~84.1 | 06 doc §5.3 |
| 暦年マーカー | ~2033 年中 | 06 doc §5.6 |

#### §5 依存 milestone

- **前 milestone**: r40 達成 (本 r40 章 close、本 (d) work item (e) 完了)
- **interface 確立**: なし (r41 で skeleton 配置のみ、interface 経由 call 化は r41.5)
- **設計 doc 確定**: 04 doc §5.4 段階 port 戦略 + 05 doc §3 descriptor + §4 render pass + §10 skeleton + 06 doc §3.1-§3.2 工程算定

#### §6 起草 timing / 主体

- **起草 timing**: r40 達成宣言直後 (~2026-06 想定、本 (d) work item (e) 完了 = r40 章 close と同時)
- **起草主体**: AYA + Claude (本 (d) §5.1 outline base + 04/05/06 doc cross reference)
- **起草先**: `docs/specs/ayastorm-r41-gl-removal/00-charter.md`

#### §7 詳細化方針 (本 (d) outline base + charter 起草時に追加)

- 各 acceptance criterion の具体 metric (例: shader cross compile coverage % / driver matrix 動作 driver 数 / regression 件数)
- 04 doc §5.4 段階 1-5 の各段階内 file list 詳細 (どの file を先に port するか) + dependency graph
- LLVKRenderer interface skeleton の signature 詳細 (本 (d) outline では 05 doc §10 hook 配置のみ)
- Linux Mesa RADV / NVIDIA proprietary の driver-specific quirks list (起草時に audit)

#### §8 関連 doc / memory

- `00-charter.md` r40 章 charter (§6 r41 行 + 本 (d) §1.2 正式区分)
- `04-portage-inventory.md` (§5.4 段階 port 戦略)
- `05-vulkan-api-design.md` (§3 descriptor set + §4 render pass + §10 skeleton)
- `06-effort-estimation.md` (§3.1-§3.2 工程算定 + §5.3 暦月 + §5.6 marker)
- `07-r42-plus-milestone-mapping.md` (本 (d)、§1.4 時系列整合 + §5.1 本 outline)
- memory `project_ayastorm_r41_vulkan_migration.md` (r41 milestone active)
- memory `project_ayastorm_three_platforms.md` (3 OS 大前提、Linux 先行)

### §5.2 r41.5 charter outline (VK repo 分離 + Vulkan code abstraction + 法的 review)

#### §1 milestone thesis

r41 達成 (GL 除去 + Vulkan 空転 = 本線同居) 直後の **構造 refactor milestone**。描画 stage は進行しないが、以下を達成: Vulkan code abstraction 化 (pipeline.cpp 内 inline → interface 経由 call) + AYAstorm VK repo 新規立ち上げ + 物理分離 (directory 移動) + dynamic link 構成 (LGPL combined work 回避) + 法的 review。charter §6 r41.5 milestone section + charter §7 判断軸 3 (iv) 発動 condition の確立。

#### §2 work breakdown (06 doc §3.10 反映)

| 領域 | 工数 (PM) | 主作業 | 出典 |
|---|---|---|---|
| LLVKRenderer interface 詳細化 (r41 skeleton → interface 経由 call) | 0.40 | 05 doc §10.2-§10.3 hook 配置 (pipeline.cpp 内 inline 実装を interface 経由 call に置換) | 06 doc §3.10 + 05 doc §10.2 |
| VK repo 物理分離 (directory 単位移動) | 0.30 | indra/llrender + 05 doc §10.3 で抽出した Vulkan layer file を VK repo に移動 | 06 doc §3.10 + 05 doc §10.3 |
| dynamic link 構成 (本線 binary ↔ VK repo binary) | 0.30 | dynamic link 構成 + 本線 build script から VK repo fetch + build + link、LGPL combined work 回避達成 | 06 doc §3.10 + charter §6 r41.5 |
| 法的 review (VK repo license 戦略確定) | 0.30 | VK repo license (proprietary / MIT / Apache 2.0 等) の選定、AYA さんとの擦り合わせ + 法務 review (必要なら external advice) | 06 doc §3.10 + charter §6 r41.5 |
| **base work 計** | **1.30** | — | 06 doc §3.10 |
| 余裕係数 +15% (法的 review 不確実性 + dynamic link build 統合 cost、ただし low risk milestone 区分) | +0.20 | — | 06 doc §3.10 |
| **r41.5 total (フルタイム dev 換算)** | **~1.50** | — | 06 doc §3.10 |

#### §3 acceptance criteria draft

1. **Vulkan code abstraction 化完遂**: pipeline.cpp 等の Vulkan API 直接 call が LLVKRenderer interface 経由 call に置換、05 doc §10.2-§10.3 hook 配置全完了
2. **VK repo 新規立ち上げ**: AYAstorm VK repo (GitHub 二次 fork 制約に依らない完全独立 git init) が立ち上げ完了
3. **物理分離 (directory 単位移動)**: indra/llrender + Vulkan layer 関連 file が本線 `ayastorm-release` から VK repo へ移動完了
4. **dynamic link 構成動作**: 本線 binary (LGPL) と VK repo binary (独自 license) が dynamic link で結合、viewer 起動 + Vulkan 空転動作維持 (r41 達成状態の degradation 無し)
5. **ビルド統合**: 本線 build script から VK repo fetch + build + link が動作、3 OS でビルド可能 (Linux first-class、Win/Mac は依然本線 GL 維持で r41.5 段階では VK repo 統合のみ動作確認)
6. **法的分離達成**: LGPL combined work 回避達成、VK repo license 戦略確定 (AYA + Claude + 必要なら法務 advice で確定)、charter §6 r41.5 「LGPL combined work 回避 → 法的分離達成」 acceptance
7. **LL UI 変更時 defensibility 確保**: charter §7 判断軸 3 (iv) の選択肢 (LL 公式 VK engine 採用 + AYAstorm GUI 維持) が r41.5 達成後から有効 (interface 経由 call 化 + VK repo 物理分離 + dynamic link 構成の 3 要素が成立)
8. **regression 無し**: r41 で確立した GL 除去 + Vulkan 空転の baseline が r41.5 完了時点で stable (vk-α 機能の degradation 無し)

#### §4 暦月変換 + 暦年マーカー (06 doc §5.3 + §5.6 反映)

| 換算項目 | 値 | 出処 |
|---|---|---|
| r41.5 base PM | 1.50 | 06 doc §3.10 |
| 並走 ratio 中央値 4x + 学習曲線 (Vulkan 経験積み済、refactor 中心の低学習曲線) | — | 06 doc §5.1 + §5.2 |
| r41.5 中央値暦月 | ~7.2 | 06 doc §5.3 |
| 暦年マーカー | ~2034 年初 | 06 doc §5.6 |

#### §5 依存 milestone

- **前 milestone**: r41 達成 (GL 除去 + Vulkan 空転 = LLVKRenderer skeleton 配置済)
- **interface 確立**: r41 で skeleton 配置済 → r41.5 で interface 経由 call に詳細化
- **設計 doc 確定**: 05 doc §10.2-§10.3 hook 詳細

#### §6 起草 timing / 主体

- **起草 timing**: r41 達成宣言直後 (~2033 年中 想定、06 doc §5.6 marker)
- **起草主体**: AYA + Claude (法的 review 関与で AYA 比重大、必要なら外部法務 advice)
- **起草先**: `docs/specs/ayastorm-r41-5-vk-repo-separation/00-charter.md`

#### §7 詳細化方針 (本 (d) outline base + charter 起草時に追加)

- LLVKRenderer interface signature 完全詳細化 (05 doc §10.2-§10.3 hook 配置を実 API surface に詳細化)
- VK repo directory 構造詳細 (どの directory / file を移動するか)
- VK repo license 戦略の決定 (proprietary vs MIT vs Apache 2.0 vs 等、AYA + 法務で詰める)
- LL UI 変更時の defensibility 詳細 (charter §7 判断軸 3 (iv) の発動 condition 詳細化)

#### §8 関連 doc / memory

- `00-charter.md` r40 章 charter (§6 r41.5 milestone section + §7 判断軸 3)
- `05-vulkan-api-design.md` (§10 LLVKRenderer skeleton + §10.2-§10.3 hook 詳細)
- `06-effort-estimation.md` (§3.10 r41.5 工程算定 + §5.3 暦月 + §5.6 marker)
- `07-r42-plus-milestone-mapping.md` (本 (d)、§5.2 本 outline)

### §5.3 r42-α/β/γ/δ charter outline (本 (d) §2.1-§2.4 内訳 base)

r42 は 4 sub-milestone (α / β / γ / δ) で構成、各 sub-milestone charter は本 (d) §2.1-§2.4 内訳 base + §5.0 統一 template で起草。以下に各 sub-milestone の outline 要約を提示 (本 (d) §2 内訳の charter outline 形式継承):

#### §5.3.1 r42-α (r21.1 self-rigged picker port)

| section | 内容 |
|---|---|
| **§1 milestone thesis** | AYAstorm r21.1 self-rigged picker (mObjectIDBuffer = gbuffer3 inline attachment + 単一 click → ObjectID 取得) を Vulkan parity 再現。描画 stage = **vk-β 着手** (静止 scene + avatar + render pass attachment + read-pick path)。**Win 追加開始** = LLWindow Win32 surface 化 + `VK_KHR_win32_surface` + driver matrix 着手 |
| **§2 work breakdown** | 本 (d) §2.1 反映、~0.65 PM (base 0.50 + 余裕係数 +30%): picker shader 2 file SPIR-V 化 0.10 + pipeline.cpp 4 LOC 0.05 + render pass attachment 設計 0.20 + read-pick テスト 0.15 |
| **§3 acceptance criteria** | 本 (d) §2.1 反映 6 件 (parity / render pass attachment / read-pick path / shader cross compile / **Win 追加開始** / regression 無し) |
| **§4 暦月変換 + 暦年** | ~2.9 暦月 / ~2034 年前半 (06 doc §5.3 + §5.6) |
| **§5 依存 milestone** | r41.5 達成 + LLVKRenderer interface 確立 + 05 doc §3 + §4.5 設計確定 |
| **§6 起草 timing / 主体** | r41.5 達成宣言直後 (~2034 年初) + AYA + Claude |
| **§7 詳細化方針** | acceptance #1-#6 各々の具体 metric + Win driver matrix の対象 driver list 詳細化 + r21.1 既存 AYAstorm test の流用方針 |
| **§8 関連 doc / memory** | `00-charter.md` (§6 r42 行 + 本 (d) §1.2 正式区分) + 04 doc §B.1 + 05 doc §3 + §4.5 + §8.2 (Win) + 06 doc §3.3 + 07 doc §2.1 + memory `project_ayastorm_r21_self_rigged_picker.md` |
| **起草先** | `docs/specs/ayastorm-r42-alpha-picker/00-charter.md` |

#### §5.3.2 r42-β (r30 Cinematic mode port)

| section | 内容 |
|---|---|
| **§1 milestone thesis** | AYAstorm r30 Cinematic mode (DoF state enum 化 + Cinematic shader 4 file + AYAstorm View mode==2 = Cinematic) を Vulkan parity 再現。描画 stage = **vk-β 完遂** (静止 scene + avatar 完成) + **vk-δ 部分** (DoF state enum 化 + post-process pass 着手)。**Mac 追加開始** = portable subset check + t-noami さん事前共有 |
| **§2 work breakdown** | 本 (d) §2.2 反映、~3.15 PM (base 2.25 + 余裕係数 +40%): Cinematic shader 4 file SPIR-V 化 0.40 + pipeline.cpp 6 分岐 frame context 0.20 + DoF state enum 化 0.50 + visual quality test 1.00 + 余 0.15 |
| **§3 acceptance criteria** | 本 (d) §2.2 反映 7 件 (parity / DoF state enum 化 / shader cross compile / **BD cvar 13 件 visual A/B** / **Mac 追加開始** / Win 並走 / regression 無し) |
| **§4 暦月変換 + 暦年** | ~13.9 暦月 / ~2035 年中 |
| **§5 依存 milestone** | r42-α 達成 + Win 追加開始 baseline + 05 doc §10.2 interface + r41 frame context 集約 base |
| **§6 起草 timing / 主体** | r42-α 達成宣言直後 (~2034 年前半) + AYA + Claude (BD cvar 13 件 visual A/B 含む = memory `project_r30_cinematic_control_tuning_deferred.md` 反映) |
| **§7 詳細化方針** | DoF state enum 化の具体 enum class signature + BD cvar 13 件 visual A/B test procedure + Mac portable subset check の対象 feature list + t-noami さん事前共有 cadence |
| **§8 関連 doc / memory** | 04 doc §B.3 + 05 doc §3 + §4.4 + §8.3 (Mac portable subset) + 06 doc §3.4 + 07 doc §2.2 + memory `project_r30_cinematic_control_tuning_deferred.md` + `feedback_visual_decisions_need_live_ab.md` + `feedback_credit_t_noami_equal_billing.md` |
| **起草先** | `docs/specs/ayastorm-r42-beta-cinematic/00-charter.md` |

#### §5.3.3 r42-γ (r14+ visual realism port)

| section | 内容 |
|---|---|
| **§1 milestone thesis** | AYAstorm r14+ visual realism (post-process pass chain + visual realism shader 7 file + llvosky + llvowlsky sky dome + atmospherics) を Vulkan parity 再現。描画 stage = **vk-γ 着手 + vk-γ 進行** (deferred lighting + sky dome + atmospherics) + **vk-δ 部分** (volumetricLight / godrays / vignette / tone map 等 post-process chain) |
| **§2 work breakdown** | 本 (d) §2.3 反映、~3.18 PM (base 2.27 + 余裕係数 +40%): llvosky + llvowlsky 0.27 + visual realism shader 7 file SPIR-V 化 0.60 + post-process descriptor set 整備 0.30 + pipeline.cpp post-process chain 5 LOC 0.10 + performance profile 0.50 + visual A/B 0.50 |
| **§3 acceptance criteria** | 本 (d) §2.3 反映 8 件 (parity / sky dome + atmospherics / shader cross compile + **linear/sRGB invariant** / post-process chain / **scene buffer alpha invariant** / performance ≤10% / **visual A/B sustained** / Win-Mac 並走 / regression 無し) |
| **§4 暦月変換 + 暦年** | ~14.0 暦月 / ~2036 年後半 |
| **§5 依存 milestone** | r42-β 達成 + DoF state enum 化 base + post-process pass chain initial integration + 05 doc §3 + §4.4 設計確定 |
| **§6 起草 timing / 主体** | r42-β 達成宣言直後 (~2035 年中) + AYA + Claude (scene buffer alpha invariant + sustained A/B 含む = memory `project_aya_visual_realism_alpha_protect.md` + `feedback_instant_ab_vs_sustained.md` 反映) |
| **§7 詳細化方針** | visual realism shader 7 file の具体 file list + descriptor set per-frame / per-material / per-draw 3 階層 binding 詳細 + sustained viewing test procedure + AYAstorm 開発機 (AMD RX 7900 XTX + Mesa RADV) baseline performance profile 詳細 |
| **§8 関連 doc / memory** | 04 doc §B.2 + 05 doc §3 + §4.4 + 06 doc §3.5 + 07 doc §2.3 + memory `project_aya_visual_realism_alpha_protect.md` + `project_atmos_atten_scalarized.md` + `feedback_shader_color_space_correction.md` + `feedback_instant_ab_vs_sustained.md` |
| **起草先** | `docs/specs/ayastorm-r42-gamma-visual-realism/00-charter.md` |

#### §5.3.4 r42-δ (parity 残機能 / vk-RC 直前 polish)

| section | 内容 |
|---|---|
| **§1 milestone thesis** | charter §6 仮 line up の「r42 = r1-r13 audio port 並行」+ AYAstorm r25-r29 3D stream + parity 残機能 + vk-RC 直前 regression sweep を集約した polish milestone。描画 stage = **vk-γ 完遂 + vk-δ 完遂** (reflection / SMAA / SSAO / shadow cascade) + vk-RC 直前 polish。**Mac MoltenVK 詳細化開始** (05 doc §9.4 / §8.3) |
| **§2 work breakdown** | 本 (d) §2.4 反映、~2.25 PM (base 1.50 + 余裕係数 +50%): r25-r29 3D stream Vulkan-side hook 0.50 + r1-r13 audio Vulkan 非依存確認 0.30 + vk-RC 直前 regression sweep 0.70 |
| **§3 acceptance criteria** | 本 (d) §2.4 反映 6 件 (r25-r29 3D stream parity / r1-r13 audio Vulkan 非依存 / vk-δ 完遂 / vk-RC 直前 polish / 3 OS 状況確認 / regression sweep PASS) |
| **§4 暦月変換 + 暦年** | ~9.9 暦月 / ~2037 年中 |
| **§5 依存 milestone** | r42-γ 達成 + visual realism port 完遂 + sky dome 確立 + post-process chain 確立 |
| **§6 起草 timing / 主体** | r42-γ 達成宣言直後 (~2036 年後半) + AYA + Claude (r1-r13 audio invariant 確認 = memory `project_pr69_fallback_switch.md` 反映、Mac MoltenVK 詳細化 = `feedback_credit_t_noami_equal_billing.md` + `feedback_mac_only_fixes_accept_as_is.md` 反映) |
| **§7 詳細化方針** | r25-r29 3D stream の `VK_KHR_external_memory_*` 採用範囲詳細 (r45+ 本格実装との境界明示) + r1-r13 audio FMOD callback + Dullahan path の invariant 検証 procedure + Mac MoltenVK portable subset の対象 feature list 詳細化 |
| **§8 関連 doc / memory** | 04 doc §B.x + 05 doc §8.3 + §9.2 + §9.4 + 06 doc §3.6 + 07 doc §2.4 + memory `project_pr69_fallback_switch.md` + `feedback_credit_t_noami_equal_billing.md` + `feedback_mac_only_fixes_accept_as_is.md` |
| **起草先** | `docs/specs/ayastorm-r42-delta-polish/00-charter.md` |

### §5.4 r43 / r44 charter outline (本 (d) §3.1-§3.2 内訳 base)

#### §5.4.1 r43 (Linux baseline 安定維持 + Win parity 完遂)

| section | 内容 |
|---|---|
| **§1 milestone thesis** | r42-δ までで達成済の Linux baseline AYAstorm r1-r30 全機能 Vulkan 動作を **stable parity 状態に補強** + **Win parity 完遂** (Win driver matrix 全 driver + 旧 driver fallback + WHCK 認定整備)。描画 stage = **vk-RC (parity 補強 + 性能 polish 一部)**。Linux baseline 自体は 0 新規 work、Win parity 完遂が中心 |
| **§2 work breakdown** | 本 (d) §3.1 反映、~2.63 PM (base 1.85 + 余裕係数 +42% 加重平均): Win driver matrix 0.50 + Win 純増分 1.35 |
| **§3 acceptance criteria** | 本 (d) §3.1 反映 7 件 (Linux baseline parity 完遂 / Win driver matrix / Win surface 化 / WHCK 認定 minimum 版数 release note / LunarG SDK Win 統合 / Win-specific bug fix / regression 無し) |
| **§4 暦月変換 + 暦年** | ~20 暦月 (累積 ~152 暦月、~2039 年初) / ~2038 年中 - 2039 年初 |
| **§5 依存 milestone** | r42-δ 達成 + Linux baseline parity 残機能完遂 + vk-RC 直前 polish + Mac MoltenVK 詳細化開始 |
| **§6 起草 timing / 主体** | r42-δ 達成宣言直後 (~2037 年中) + AYA + Claude (Win 限定 fix 受入方針 = memory `feedback_mac_only_fixes_accept_as_is.md` を Win にも適用) |
| **§7 詳細化方針** | Win driver matrix 対象 driver list 詳細 (NVIDIA RTX 20/30/40 / AMD RDNA 1/2/3 / Intel Arc/Iris Xe の各 version list) + 旧 driver fallback path の具体 fallback condition + WHCK 認定 minimum 版数の release note template + LunarG SDK Win 版の path 構造差吸収詳細 |
| **§8 関連 doc / memory** | 04 doc + 05 doc §8.2 (Win driver matrix) + §8.4 (WSI Win32) + §7.6 (旧 driver fallback) + 06 doc §3.7 + §4.2 + 07 doc §3.1 + memory `project_ayastorm_three_platforms.md` + `feedback_mac_only_fixes_accept_as_is.md` |
| **起草先** | `docs/specs/ayastorm-r43-win-parity/00-charter.md` |

#### §5.4.2 r44 (Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成)

| section | 内容 |
|---|---|
| **§1 milestone thesis** | r43 で達成済の Linux + Win parity を **Mac MoltenVK 経由で 3 OS parity 完遂** + **vk-RC 達成宣言** (charter §4 (1) parity 完遂 goal 到達)。描画 stage = **vk-RC 完遂** (parity 補強 + 性能 polish + Mac portable subset 詳細化)。**vk-RC 達成宣言 = r40 章工程プラン完遂 marker** |
| **§2 work breakdown** | 本 (d) §3.2 反映、~6.30 PM (base 4.20 + 余裕係数 +50%): Mac MoltenVK 詳細化 1.00 + 性能 polish 0.50 + Mac 純増分 2.70 |
| **§3 acceptance criteria** | 本 (d) §3.2 反映 9 件 (Mac parity / MoltenVK portable subset / Mac surface 化 / Apple Silicon UMA / MSL / t-noami workflow cycle / 性能 polish 3 OS / **vk-RC 達成宣言** / regression 無し) + **§3.3 vk-RC 達成 acceptance criteria 10 軸** (全機能 / audio / 視覚表現 / 3D stream / Cinematic / picker / 3 OS driver / 性能 polish / release note / regression sweep) |
| **§4 暦月変換 + 暦年** | ~18 暦月 (累積 ~170 暦月、~2040 年後半 = vk-RC 達成) / ~2039 年初 - 2040 年後半 |
| **§5 依存 milestone** | r43 達成 + Linux + Win parity 完遂 + Mac MoltenVK 詳細化累積 (r42-β 着手 → r42-δ 開始 → r43 継続) |
| **§6 起草 timing / 主体** | r43 達成宣言直後 (~2038-2039 年) + AYA + Claude (vk-RC 達成宣言含む = charter §4 (1) 完遂 goal 到達 marker、t-noami workflow cycle 完遂 = memory `feedback_credit_t_noami_equal_billing.md` + `feedback_mac_only_fixes_accept_as_is.md` 反映、release note + 運用 doc 整備 = `feedback_release_notes_link_only.md` + `feedback_release_note_per_feature.md` 反映) |
| **§7 詳細化方針** | MoltenVK portable subset の対象 feature 詳細 (`VK_KHR_portability_subset` enable 下の各 feature audit) + Apple Silicon UMA の VMA 動作詳細 + spirv-cross MSL 変換 + MoltenVK runtime 動作確認 procedure + vk-RC 達成宣言の release note 整備 (Linux/Win/Mac driver minimum 版数 + WHCK 認定 + MoltenVK 1.2 fallback note) |
| **§8 関連 doc / memory** | 04 doc + 05 doc §8.3 (Mac driver capability matrix) + §8.4 (WSI metal surface) + §9.4 (MoltenVK portable subset) + 06 doc §3.7 + §4.3 + §5.6 marker + 07 doc §3.2 + §3.3 vk-RC acceptance 10 軸 + memory `feedback_credit_t_noami_equal_billing.md` + `feedback_mac_only_fixes_accept_as_is.md` + `feedback_release_notes_link_only.md` + `feedback_release_note_per_feature.md` |
| **起草先** | `docs/specs/ayastorm-r44-mac-parity-vk-rc/00-charter.md` |

### §5.5 §5 結論 (全 8 milestone charter outline 提示完了)

本 §5 で提示した 8 milestone charter outline:

| # | milestone | outline section | 起草先 directory | 起草 timing | base PM | 暦月 |
|---|---|---|---|---|---|---|
| 1 | r41 (GL 除去 + Vulkan 空転) | §5.1 | `ayastorm-r41-gl-removal/` | r40 達成宣言直後 (~2026-06) | 16.17 | ~84.1 |
| 2 | r41.5 (VK repo 分離 + abstraction + 法的 review) | §5.2 | `ayastorm-r41-5-vk-repo-separation/` | r41 達成宣言直後 (~2033 中) | 1.50 | ~7.2 |
| 3 | r42-α (r21.1 picker port) | §5.3.1 | `ayastorm-r42-alpha-picker/` | r41.5 達成宣言直後 (~2034 初) | 0.65 | ~2.9 |
| 4 | r42-β (r30 Cinematic mode port) | §5.3.2 | `ayastorm-r42-beta-cinematic/` | r42-α 達成宣言直後 (~2034 前半) | 3.15 | ~13.9 |
| 5 | r42-γ (r14+ visual realism port) | §5.3.3 | `ayastorm-r42-gamma-visual-realism/` | r42-β 達成宣言直後 (~2035 中) | 3.18 | ~14.0 |
| 6 | r42-δ (parity 残機能 / vk-RC 直前 polish) | §5.3.4 | `ayastorm-r42-delta-polish/` | r42-γ 達成宣言直後 (~2036 後半) | 2.25 | ~9.9 |
| 7 | r43 (Linux baseline 安定維持 + Win parity 完遂) | §5.4.1 | `ayastorm-r43-win-parity/` | r42-δ 達成宣言直後 (~2037 中) | 2.63 | ~20 |
| 8 | r44 (Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成) | §5.4.2 | `ayastorm-r44-mac-parity-vk-rc/` | r43 達成宣言直後 (~2038-2039) | 6.30 | ~18 |
| **合計** | (本 §5 outline 提示完了) | — | — | — | **~35.83** (06 doc §4.5 35.84 丸め誤差 ✓) | **~170** (06 doc §5.6 ✓) |

#### §5 結論の含意

1. **全 8 milestone について outline 統一 template (§5.0) で提示完了**: 各 milestone charter は 8 section 構成 (header + §1 thesis + §2 work breakdown + §3 acceptance + §4 暦月 + §5 依存 + §6 起草 + §7 詳細化 + §8 関連 doc)
2. **work breakdown / acceptance criteria は本 (d) §2 / §3 base に継承**: 各 milestone outline で本 (d) 結論を 1 対 1 反映
3. **起草 cadence = 前 milestone 達成宣言直後**: 本 (d) §2.5 cadence と整合、各 milestone 着手前に AYA + Claude で詳細化
4. **起草先 directory = `docs/specs/ayastorm-rXX-xxx/00-charter.md`**: r45+ 別章 charter (本 (d) §4.3) と同 directory pattern、本 (d) outline が base
5. **詳細化方針 = 各 milestone 着手前に追加 (本 (d) outline では metric / file list / 詳細 schedule は仮)**: charter 起草時に AYAstorm 進化 + LL 着地 status を audit して詳細化 (本 (d) §2.5 運用方針継承)

→ 本 §5 charter outline は **work item (e) charter 完成** で各 milestone charter 起草 cadence の引継ぎ input、§6 charter §6 反映 で本 §5 outline を 00-charter.md §6 に反映予定。

---

## §6 charter §6 仮 line up の本 §1-§5 反映

### §6.0 反映方針

#### 目的

00-charter.md §6 r42+ ロードマップ方針 を、本 (d) §1-§5 で確定した正式 mapping で update。仮 line up 表 (charter §6 line 186-195) を本 §1.2 正式区分に置換、a-4 棚卸し AYAstorm 機能 pull-in 順 (charter §6 line 197-205) を本 §1.3 mapping に昇格、r41.5 milestone section (charter §6 line 207-229) を本 §5.2 outline cadence 反映、r45+ 範囲外 + 別章 charter 起草指針 (本 §4) を charter §6 末尾に追加。

実 update は work item (e) charter 完成で実施、本 §6 では update 用の **diff draft** を提示。

#### 反映の 4 軸

| 軸 | source | target |
|---|---|---|
| 1. 仮 line up 表 → 正式区分置換 | 本 §1.2 | charter §6 line 186-195 (仮 line up 表 + 注釈) |
| 2. a-4 棚卸し AYAstorm 機能 pull-in 順 → mapping 昇格 | 本 §1.3 (描画 stage × AYAstorm 機能 × milestone 三軸 mapping) | charter §6 line 197-205 (a-4 棚卸しで確定した AYAstorm 機能 pull-in 順 section) |
| 3. r41.5 milestone section → 本 §5.2 outline cadence 反映 | 本 §5.2 (r41.5 charter outline) | charter §6 line 207-229 (r41.5 milestone section) |
| 4. r45+ 範囲外 + 別章 charter 起草指針 → charter §6 末尾追加 | 本 §4 | charter §6 末尾 (line 230 以降の追加 sub-section) |

#### 反映の運用

- 本 §6 では各軸の diff draft (target section の before / after) を提示
- 実 update は work item (e) charter 完成 (sub-phase 3 全 work item 完了 + 03 doc 最終 review + 00-charter.md final review) で実施
- diff の AYA review は本 (d) 完了宣言 → work item (e) 着手で対応 (本 §6 では draft、実反映は work item (e))

### §6.1 charter §6 仮 line up 表 → 本 §1.2 正式区分への置換 draft

#### before (charter §6 line 186-195 の現状)

```text
| milestone | 描画 stage 相当 | AYAstorm 機能 pull-in 想定 | repo 構成 |
|---|---|---|---|
| r41 | vk-α | (なし、parity 不要) | 本線同居 (Phase 1) |
| **r41.5** | (描画 stage 進行なし、構造 refactor のみ) | **Vulkan code abstraction 化 + VK repo 分離** | **VK repo 新規立ち上げ (Phase 2 開始)** |
| r42 | vk-β (静止 scene + avatar) | r1-r13 audio port (描画非依存、並行可) | 本線 + VK repo (dynamic link) |
| r43 | vk-γ (deferred lighting + 基本 material) | r14-r24 視覚表現の lighting 系統 port (atmospheric / volumetric / etc.) | 本線 + VK repo |
| r44 | vk-δ (reflection / SMAA / SSAO / DoF / shadow cascade) | r14-r24 視覚表現の effects 系統 port | 本線 + VK repo |
| r45+ | vk-RC (parity 完遂) | r25-r29 3D stream / r30 Cinematic / chat / picker / 残り全機能 + Win/Mac 移植 | 本線 + VK repo |

**注**: 上記は仮 line up。Vulkan portage 棚卸し (§9 (a)) 完了後に正式区切りを確定、本 §6 を更新する。
```

#### after (本 §1.2 正式区分への置換 draft)

```text
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

**注**: 上記は本 (d) 確定の正式 line up (`07-r42-plus-milestone-mapping.md` §1-§4 結論)、(a) Vulkan portage 棚卸し + (b) Vulkan API 設計 + (c) 工程算定 + (d) r42+ 区切り確定 の出力統合反映。r45+ は本算定範囲外 (本 charter §6 末尾 r45+ section 参照、`docs/specs/ayastorm-r45-plus-xxx/00-charter.md` 別章で扱う)。
```

### §6.2 charter §6 a-4 棚卸し AYAstorm 機能 pull-in 順 → 本 §1.3 mapping 昇格 draft

#### before (charter §6 line 197-205 の現状)

```text
### a-4 棚卸しで確定した AYAstorm 機能 pull-in 順 (本 line up 格上げは work item (d) で確定)

work item (a) a-4 §6.3.2 で touchpoint 規模順 + 独立度順を確定 (合計 20-30 call、base portage の 0.03% 未満で **局在性確定**、base LL port 完成後の modular patch 合成可能):

- **r42-α**: r21.1 self-rigged picker (mObjectIDBuffer → render pass attachment、shader 2 file SPIR-V 化)
- **r42-β**: r30 Cinematic mode (DoF state enum 化 + frame context 統合、shader 4 file SPIR-V 化)
- **r42-γ**: r14+ visual realism (post-process pass chain 統合、shader 7 file SPIR-V 化、llvosky/llvowlsky port 含む)

上記 3 順序の **仮 line up §6 r42-r45+ への正式 mapping** は work item (d) r42+ 区切り確定で行う。
```

#### after (本 §1.3 mapping 昇格 draft)

```text
### a-4 棚卸しで確定した AYAstorm 機能 pull-in 順 (本 (d) で正式 mapping 確定済 2026-05-28)

work item (a) a-4 §6.3.2 で touchpoint 規模順 + 独立度順を確定 (合計 20-30 call、base portage の 0.03% 未満で **局在性確定**、base LL port 完成後の modular patch 合成可能)。本 (d) `07-r42-plus-milestone-mapping.md` §1.3 で描画 stage × AYAstorm 機能 × milestone 三軸 mapping を正式確定:

| milestone | 描画 stage (主) | 描画 stage (副) | AYAstorm 機能 | 進行範囲 |
|---|---|---|---|---|
| r42-α | **vk-β 着手** | — | **r21.1 self-rigged picker** | 静止 scene + avatar + render pass attachment (mObjectIDBuffer) + read-pick path、shader 2 file SPIR-V 化 |
| r42-β | **vk-β 完遂** | vk-δ 部分 (DoF) | **r30 Cinematic mode** | scene + avatar 完成 + DoF state enum 化 + post-process pass 着手、shader 4 file SPIR-V 化 |
| r42-γ | **vk-γ 着手 + vk-γ 進行** | vk-δ 部分 (post-process chain) | **r14+ visual realism** | deferred lighting + sky dome (llvosky + llvowlsky) + atmospherics + post-process chain 7 sub-pass、shader 7 file SPIR-V 化 |
| r42-δ | vk-γ 完遂 + **vk-δ 完遂** | vk-RC 直前 polish | r25-r29 3D stream + r1-r13 audio 確認 | reflection / SMAA / SSAO / shadow cascade 完遂 + parity 残機能 polish + regression sweep |

**含意**:

- 描画 stage は r42 内で 4 段階進行 (vk-β 着手 → vk-β 完遂 + vk-δ 部分 → vk-γ + vk-δ 部分 → vk-γ + vk-δ 完遂)、charter §6 仮 line up の「r42 = vk-β / r43 = vk-γ / r44 = vk-δ」より **r42 内で大半の stage が進む**
- AYAstorm 機能 port = 描画 stage の trigger (r21.1 picker = vk-β 着手 trigger / r30 Cinematic = vk-β 完遂 + DoF trigger / r14+ visual realism = vk-γ + vk-δ 完遂 trigger)
- r43-r44 = vk-RC (parity 補強 + 性能 polish + 3 OS parity 完遂) に振替え (本 §6.1 正式区分参照)
```

### §6.3 charter §6 r41.5 milestone section → 本 §5.2 outline cadence 反映 draft

#### before (charter §6 line 207-229 の現状)

既存の「r41.5 milestone (新規追加 2026-05-28)」section + 「r41.5 のメリット」 + 「r41.5 の cost」 sub-section (詳細は 00-charter.md 参照)。

#### after (本 §5.2 outline cadence 反映 draft、既存 メリット / cost section は保持、末尾に追加)

charter §6 r41.5 milestone section 末尾に以下 sub-section を **追加** (既存 メリット / cost section は保持):

```text
#### r41.5 charter 起草 cadence (本 (d) §5.2 反映 2026-05-28)

- **起草 timing**: r41 達成宣言直後 (~2033 年中、`07-r42-plus-milestone-mapping.md` §5.2)
- **起草主体**: AYA + Claude (法的 review 関与で AYA 比重大、必要なら外部法務 advice)
- **起草先**: `docs/specs/ayastorm-r41-5-vk-repo-separation/00-charter.md`
- **工数 + 暦月**: ~1.50 PM / ~7.2 暦月 (06 doc §3.10 + §5.3、Linux first-class 並走 ratio 4x 反映)
- **acceptance criteria draft**: 本 (d) §5.2 反映 8 件 (Vulkan code abstraction 化 / VK repo 新規立ち上げ / 物理分離 / dynamic link 動作 / ビルド統合 / 法的分離 / LL UI 変更時 defensibility 確保 / regression 無し)
- **詳細化**: charter 起草時に LLVKRenderer interface signature + VK repo directory 構造 + VK repo license 戦略 + LL UI 変更時 defensibility 詳細を AYA + Claude で確定 (本 (d) outline は base)
```

### §6.4 r45+ 範囲外 + 別章 charter 起草指針 → charter §6 末尾追加 draft

#### 追加位置

charter §6 末尾 (現 r41.5 milestone section 終了後、line 230 以降に追加 = 新規 sub-section)

#### 追加 sub-section draft

```text
### r45+ 範囲外 + 別章 charter 起草指針 (本 (d) §4 反映 2026-05-28)

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
```

### §6.5 §6 結論 (work item (e) charter 完成への引継ぎ)

本 §6 で確定した charter §6 update draft:

1. **§6.1 仮 line up 表 → 正式区分置換**: r41 / r41.5 / r42-α/β/γ/δ / r43 / r44 / r45+ の正式 milestone 表 (base PM + 暦月マーカー 含む)、vk-RC 達成 marker 追加
2. **§6.2 a-4 棚卸し AYAstorm 機能 pull-in 順 → mapping 昇格**: 描画 stage × AYAstorm 機能 × milestone 三軸 mapping 表、charter §6 仮 line up より r42 内で大半 stage 進行の含意明示
3. **§6.3 r41.5 milestone section → 本 §5.2 outline cadence 反映**: r41.5 charter 起草 cadence + 工数 + acceptance criteria + 詳細化方針 を charter §6 r41.5 milestone section 末尾に追加 (既存メリット / cost section は保持)
4. **§6.4 r45+ 範囲外 + 別章 charter 起草指針 → charter §6 末尾追加**: r45+ scope broad outline + 着手 trigger + 起草 cadence を新規 sub-section で追加

#### work item (e) charter 完成への引継ぎ

- 本 §6 draft の 4 軸 update を work item (e) で **実 update** (実際に 00-charter.md §6 を edit)
- work item (e) は本 §6 + 03 doc 最終 review + 04/05/06/07 doc final review + 00-charter.md §6 update 確定 を統合した final review milestone
- work item (e) 完了 = **r40 章 close** = r41 着手準備完了 (本 §5.1 r41 charter outline → r41 charter 起草 → r41 着手)
- AYA review pattern (work item (e) 着手前):
  - Pattern A: 本 §6 update draft そのまま OK → work item (e) で 00-charter.md edit
  - Pattern B: 本 §6 update draft 一部修正 → 修正後 work item (e) で 00-charter.md edit
  - Pattern C: 本 §6 update draft 大幅変更 → 影響範囲確認 (本 §5 outline + 03 doc + 07 doc 反映) → 修正後 work item (e) 着手判定

→ 本 §6 で **work item (d) r42+ 区切り確定 の全 §1-§6 draft 完成**、AYA review PASS で work item (d) 完了宣言 → work item (e) charter 完成 着手 → r40 達成宣言 → r41 着手 cadence へ。

---

## foundation group + group A + group B 確定値 summary (work item (d) §1-§6 全 draft 完成)

### §1 r42 区切り algorithm 確定値

| 出処 | 値 | 用途 |
|---|---|---|
| §1.2 正式区分 | r41 / r41.5 / r42-α/β/γ/δ / r43-r44 / vk-RC = r44 達成 | charter §6 仮 line up の正式区分 (group B §6 反映 input) |
| §1.3 描画 stage mapping | r42-α = vk-β 着手 / r42-β = vk-β 完遂 + vk-δ 部分 / r42-γ = vk-γ + vk-δ 部分 / r42-δ = vk-γ + vk-δ 完遂 / r43-r44 = vk-RC | charter §6 描画 stage 表の更新 input |
| §1.4 時系列整合 | 05 doc §10 skeleton hooks + 04 doc §5.4 段階 1-5 が r41 範囲、§B.1/B.2/B.3 が r42-α/β/γ | charter §6 + §10 doc 整合 ✓ |
| §1.5 r45+ 範囲外 | vk-RC 達成 = r44 達成 = 本算定終了 marker、r45+ は別章 charter | charter §3 時間軸非設定遵守 |

### §2 r42 milestone 内訳確定値

| sub-milestone | work breakdown total | acceptance criteria 件数 | OS 着手 timing |
|---|---|---|---|
| r42-α (picker) | ~0.65 PM / ~2.9 暦月 | 6 件 draft | Linux baseline + **Win 追加開始** |
| r42-β (Cinematic) | ~3.15 PM / ~13.9 暦月 | 7 件 draft (BD cvar 13 件 visual A/B 含む) | Linux + Win 並走 + **Mac 追加開始** |
| r42-γ (visual realism) | ~3.18 PM / ~14.0 暦月 | 8 件 draft (scene buffer alpha invariant + sustained A/B 含む) | Linux + Win 並走 + Mac 並走 |
| r42-δ (parity 残機能 / vk-RC 直前 polish) | ~2.25 PM / ~9.9 暦月 | 6 件 draft | Linux + Win polish + **Mac MoltenVK 詳細化** |
| **r42 合計** | **~9.23 PM / ~40.7 暦月 / ~3.4 年** | 27 件 draft | 3 OS 全並走 (Linux first-class + Win/Mac 追従) |

### §3 r43-r44 区切り確定値

| sub-milestone | work breakdown total | acceptance criteria 件数 | OS parity 完遂 marker |
|---|---|---|---|
| r43 (Linux baseline 安定維持 + Win parity 完遂) | ~2.63 PM / ~20 暦月 / ~2038 中 - 2039 初 | 7 件 draft (Win driver matrix + WHCK + LunarG SDK Win 含む) | **Linux baseline + Win parity 完遂** (~144 → ~152 暦月) |
| r44 (Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成) | ~6.30 PM / ~18 暦月 / ~2039 初 - 2040 後半 | 9 件 draft (MoltenVK + UMA + MSL + t-noami workflow 含む) | **Mac parity 完遂 = vk-RC 達成** (~170 暦月) |
| **r43-r44 合計** | **~8.93 PM / ~38 暦月 / ~3.2 年** | 16 件 draft | 3 OS parity 完遂 = charter §4 (1) 達成 |
| vk-RC 達成 acceptance criteria (§3.3) | — | **10 軸** (全機能 parity / audio / 視覚表現 / 3D stream / Cinematic / picker / 3 OS driver / 性能 polish / release note / regression sweep) | r44 達成宣言の基準 |

### §4 r45+ 区切り (本算定範囲外) 確定値

| 出処 | 値 | 用途 |
|---|---|---|
| §4.0 4 重遵守 | charter §3 + §4 (3) + 06 doc §3.8 + 本 (d) §1.5 で範囲外確定 | r45+ 詳細化を本 (d) で扱わない根拠 |
| §4.1 scope broad outline | visual realism 次世代 / ray tracing / HDR / GPU-driven / AYAstorm 独自進化 | 別章 charter 起草の input |
| §4.2 着手 trigger | r44 達成 + AYA judgment、時間軸 trigger 無し | charter §3 / §7 / §8 連動方針 |
| §4.3 charter 起草 cadence | r44 達成宣言 + 6 か月以内に AYA 擦り合わせ開始、`docs/specs/ayastorm-r45-plus-xxx/00-charter.md` 別 directory | r40 章 close 後の cadence |

### §5 charter outline 確定値 (本 (d) group B §5)

| outline | 起草先 directory | 起草 timing | base PM | 暦月 | acceptance criteria 件数 |
|---|---|---|---|---|---|
| §5.1 r41 (GL 除去 + Vulkan 空転) | `ayastorm-r41-gl-removal/` | r40 達成宣言直後 (~2026-06) | 16.17 | ~84.1 | 9 件 |
| §5.2 r41.5 (VK repo 分離 + abstraction + 法的 review) | `ayastorm-r41-5-vk-repo-separation/` | r41 達成宣言直後 (~2033 中) | 1.50 | ~7.2 | 8 件 |
| §5.3.1 r42-α (r21.1 picker port) | `ayastorm-r42-alpha-picker/` | r41.5 達成宣言直後 (~2034 初) | 0.65 | ~2.9 | 6 件 |
| §5.3.2 r42-β (r30 Cinematic mode port) | `ayastorm-r42-beta-cinematic/` | r42-α 達成宣言直後 (~2034 前半) | 3.15 | ~13.9 | 7 件 |
| §5.3.3 r42-γ (r14+ visual realism port) | `ayastorm-r42-gamma-visual-realism/` | r42-β 達成宣言直後 (~2035 中) | 3.18 | ~14.0 | 8 件 |
| §5.3.4 r42-δ (parity 残機能 / vk-RC 直前 polish) | `ayastorm-r42-delta-polish/` | r42-γ 達成宣言直後 (~2036 後半) | 2.25 | ~9.9 | 6 件 |
| §5.4.1 r43 (Linux baseline 安定維持 + Win parity 完遂) | `ayastorm-r43-win-parity/` | r42-δ 達成宣言直後 (~2037 中) | 2.63 | ~20 | 7 件 |
| §5.4.2 r44 (Mac portable subset 詳細化 + Mac parity 完遂 = vk-RC 達成) | `ayastorm-r44-mac-parity-vk-rc/` | r43 達成宣言直後 (~2038-2039) | 6.30 | ~18 | 9 件 + §3.3 vk-RC 10 軸 |
| **8 milestone 合計** | — | — | **~35.83** (06 doc §4.5 35.84 丸め誤差 ✓) | **~170** (06 doc §5.6 ✓) | 60 件 + §3.3 vk-RC 10 軸 |

| outline 統一 template | 反映 |
|---|---|
| 8 section 構成 | header + §1 thesis + §2 work breakdown + §3 acceptance criteria + §4 暦月変換 + §5 依存 + §6 起草 timing / 主体 + §7 詳細化方針 + §8 関連 doc / memory |
| base 内容 | 本 (d) §2 / §3 work breakdown + acceptance criteria 継承、各 milestone outline で 1 対 1 反映 |
| 起草 cadence | 前 milestone 達成宣言直後 (本 (d) §2.5 cadence 整合)、各 milestone 着手前に AYA + Claude で詳細化 |

### §6 charter §6 反映 確定値 (本 (d) group B §6)

| 軸 | source | target | 反映内容 |
|---|---|---|---|
| §6.1 仮 line up → 正式区分置換 | 本 §1.2 | charter §6 line 186-195 | 9 行 milestone 表 (r41 / r41.5 / r42-α/β/γ/δ / r43 / r44 / vk-RC 達成 marker / r45+ 範囲外)、base PM + 暦月 + 描画 stage + AYAstorm 機能 + repo 構成 含む |
| §6.2 a-4 棚卸し pull-in → mapping 昇格 | 本 §1.3 | charter §6 line 197-205 | 描画 stage × AYAstorm 機能 × milestone 三軸 mapping 表 (r42-α/β/γ/δ)、charter §6 仮 line up より r42 内大半 stage 進行の含意明示 |
| §6.3 r41.5 milestone section → §5.2 cadence 反映 | 本 §5.2 | charter §6 line 207-229 | r41.5 charter 起草 cadence + 工数 + acceptance criteria 8 件 + 詳細化方針 を末尾追加 (既存メリット / cost section は保持) |
| §6.4 r45+ 範囲外 + 別章 charter 起草指針 → 末尾追加 | 本 §4 | charter §6 line 230 以降 | r45+ scope broad outline 5 領域 + 着手 trigger + 起草 cadence + `docs/specs/ayastorm-r45-plus-xxx/` 別 directory pattern |

| 反映の運用 | 内容 |
|---|---|
| 本 §6 = diff draft 提示 | 各軸 4 軸 = before / after 差分提示完了 |
| 実 update = work item (e) charter 完成 | 00-charter.md §6 を実際に edit、本 (d) 完了宣言 → work item (e) 着手 |
| AYA review pattern | Pattern A (draft そのまま OK) / B (一部修正) / C (大幅変更 → 影響範囲確認) |

### vk-RC 累積確定値 (本 (d) §1-§6 全 draft 完了時点)

| milestone 累積 | base PM | 中央値暦月 | 暦年マーカー |
|---|---|---|---|
| r41 (Linux baseline) | 16.17 | ~84.1 | ~2033 中 |
| r41 + r41.5 | 17.67 | ~91.3 | ~2034 初 |
| r41 + r41.5 + r42 (α+β+γ+δ) | 26.90 | ~132 | ~2037 中 |
| r41 + r41.5 + r42 + r43 (Linux + Win parity 完遂) | ~29.53 | ~152 | ~2039 初 |
| r41 + r41.5 + r42 + r43 + r44 (vk-RC 達成 = 3 OS parity) | **~35.83 (= 06 doc §4.5 35.84 と丸め誤差 0.01 範囲整合 ✓)** | **~170 (= 06 doc §5.6 整合 ✓)** | **~2040 後半 (= 06 doc §5.6 整合 ✓)** |

→ 本 (d) §3.5 結論で確定の **vk-RC 累積 ~35.84 PM / ~170 暦月 / ~14.2 年 / ~2040 年後半** = charter §4 (3) 想定 15-30 年帯の下方近接、本算定中央値 = charter §4 (3) 想定範囲内 ✓

### 次 step

- **work item (d) 完了宣言** (本 §1-§6 全 draft 完成 → AYA review PASS で work item (d) 完了)
- **work item (e) charter 完成** (sub-phase 3 全 work item (a)-(d) の statement of completion + 03 doc 最終 review + 00-charter.md §6 update **実施** = 本 §6 draft の 4 軸を 00-charter.md に edit 反映)
- **r40 達成宣言** (work item (e) 完了 = r40 章 close)
- **r41 着手** (`docs/specs/ayastorm-r41-gl-removal/00-charter.md` 起草、本 §5.1 outline base)

---

## 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (§3 時間軸非設定 / §4 (1) parity 完遂 goal / §6 仮 line up = 本 (d) §6 で正式 mapping 反映)
- `03-sub-phase-3-vulkan-plan.md` — work item (d) 親 doc (本 doc は work item (d) 出力先)
- `04-portage-inventory.md` — work item (a) 完了 (§5.4 段階 port 戦略 + §6.3.2 AYAstorm 機能 pull-in 順 = 本 §1.2/§1.4 input)
- `05-vulkan-api-design.md` — work item (b) 完了 (§10 skeleton 時系列 + §3 descriptor set + §4 render pass + §8 OS 別 + §9.4 MoltenVK = 本 §1.4/§2 input)
- `06-effort-estimation.md` — work item (c) 完了 (§3 milestone 別工数 + §4.4 OS 着手 timing + §5.6 marker 暦年 = 本 §1.2/§2 input)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (本 group A 完了を反映予定)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 §2.1 + §5.1 charter outline で詳細化反映)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 (本 §1.3 + §2 OS 着手 timing + §3.4 3 OS parity 完遂順 で反映)
- `project_r30_cinematic_control_tuning_deferred.md` — r30 BD cvar 13 件 tuning (本 §2.2 r42-β visual A/B 反映)
- `project_aya_visual_realism_alpha_protect.md` — scene buffer alpha invariant (本 §2.3 r42-γ + §3.3 vk-RC acceptance criteria 反映)
- `project_atmos_atten_scalarized.md` — atmosFragLighting atten scalarization (本 §2.3 r42-γ sky dome + §3.3 vk-RC 視覚表現 acceptance 反映)
- `project_pr69_fallback_switch.md` — LL_DULLAHAN_AUDIO_CALLBACK 整合 (本 §2.4 r42-δ + §3.3 vk-RC audio acceptance 反映)
- `feedback_shader_color_space_correction.md` — shader 出力 linear / sRGB 逆引き (本 §2.3 r42-γ shader cross compile + §3.3 vk-RC 視覚表現 acceptance 反映)
- `feedback_visual_decisions_need_live_ab.md` — visual 決定は live A/B 必須 (本 §2.2 / §2.3 visual A/B + §3.3 Cinematic acceptance 反映)
- `feedback_instant_ab_vs_sustained.md` — instant vs sustained A/B (本 §2.3 sustained viewing + §3.3 視覚表現 acceptance 反映)
- `feedback_credit_t_noami_equal_billing.md` — Mac t-noami workflow (本 §2.2 / §2.3 / §2.4 Mac 着手 timing + §3.2 r44 Mac parity 完遂 + §3.4 t-noami cycle 反映)
- `feedback_mac_only_fixes_accept_as_is.md` — Mac 限定 fix 受入 (本 §2.4 Mac MoltenVK 詳細化 + §3.1 r43 Win 限定 fix にも適用 + §3.2 r44 Mac 固有 quirk 受入 反映)
- `feedback_release_notes_link_only.md` — Release Notes は詳細資料へのリンクで十分 (本 §3.3 vk-RC release note + 運用 doc 整備 acceptance 反映)
- `feedback_release_note_per_feature.md` — 1 feature 1 note (本 §3.3 vk-RC release note 整備 acceptance 反映)
- `feedback_proactive_handoff.md` — group 境界 handoff (本 group A 完了で次 session への handoff doc 作成)
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace (本 group A 完了で実施)
