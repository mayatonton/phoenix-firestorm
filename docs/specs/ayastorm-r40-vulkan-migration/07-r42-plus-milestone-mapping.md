# r40 sub-phase 3 work item (d): r42+ 区切り確定

**status**: foundation group (§1 r42 区切り algorithm 化 + §2 r42 milestone 内訳) draft 完了 — group A (§3 r43-r44 + §4 r45+) / group B (§5 charter outline + §6 charter §6 反映) は次 session 以降
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

## §3 r43-r44 区切り — group A 着手予定 (本 session 未着手)

(placeholder、group A で draft 着手)

### 予定 sub-section

- §3.0 算定方針 (charter §6 仮 line up の r43-r45+ 振替え logic)
- §3.1 r43 sub-milestone 構成 (parity 補強 + Win driver matrix polish)
- §3.2 r44 sub-milestone 構成 (Mac portable subset 詳細化 + 性能 polish + vk-RC 達成)
- §3.3 vk-RC 達成宣言の acceptance criteria
- §3.4 3 OS parity 完遂 marker
- §3.5 §3 結論

---

## §4 r45+ 区切り (本算定範囲外) — group A 着手予定 (本 session 未着手)

(placeholder、group A で draft 着手)

### 予定 sub-section

- §4.0 算定方針 (charter §3 時間軸非設定遵守 + 06 doc §3.8 範囲外宣言)
- §4.1 r45+ scope の broad outline (visual realism 次世代 / ray tracing / HDR / GPU-driven の topic 列挙)
- §4.2 r45+ 着手 trigger 条件 (r44 達成 + AYA judgment、時間軸 trigger 無し)
- §4.3 r45+ charter 起草 timing (r44 達成後の AYA 擦り合わせ)
- §4.4 §4 結論

---

## §5 各 milestone の charter 草案 outline — group B 着手予定 (本 session 未着手)

(placeholder、group B で draft 着手)

### 予定 sub-section

- §5.0 算定方針 (各 milestone charter の outline 統一 template)
- §5.1 r41 charter outline
- §5.2 r41.5 charter outline (法的 review + dynamic link + VK repo 立ち上げ)
- §5.3 r42-α/β/γ/δ charter outline (本 §2.1-§2.4 内訳 base)
- §5.4 r43 / r44 charter outline (本 §3 内訳 base)
- §5.5 §5 結論

---

## §6 charter §6 仮 line up の本 §1-§5 反映 — group B 着手予定 (本 session 未着手)

(placeholder、group B で draft 着手)

### 予定 sub-section

- §6.0 反映方針 (00-charter.md §6 update のための diff 提示)
- §6.1 charter §6 仮 line up 表の本 §1.2 正式区分への置換 draft
- §6.2 charter §6 「a-4 棚卸しで確定した AYAstorm 機能 pull-in 順」section の本 §1.3 mapping への昇格 draft
- §6.3 charter §6 r41.5 milestone section の本 §2.5 charter outline cadence への反映 draft
- §6.4 r45+ 範囲外 + 別章 charter 起草指針の charter §6 追加 draft
- §6.5 §6 結論 (work item (e) charter 完成への引継ぎ)

---

## foundation group 確定値 summary (work item (d) §1 + §2 draft 完成)

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

### 次 step

- **group A (§3 r43-r44 + §4 r45+)**: charter §6 仮 line up の r43-r44 振替え logic + vk-RC 達成 acceptance criteria + r45+ 別章 charter 指針
- **group B (§5 charter outline + §6 charter §6 反映)**: 全 milestone charter outline + 00-charter.md §6 update draft
- **work item (d) 完了宣言 → work item (e) charter 完成 → r40 達成宣言** が cadence

---

## 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (§3 時間軸非設定 / §4 (1) parity 完遂 goal / §6 仮 line up = 本 (d) §6 で正式 mapping 反映)
- `03-sub-phase-3-vulkan-plan.md` — work item (d) 親 doc (本 doc は work item (d) 出力先)
- `04-portage-inventory.md` — work item (a) 完了 (§5.4 段階 port 戦略 + §6.3.2 AYAstorm 機能 pull-in 順 = 本 §1.2/§1.4 input)
- `05-vulkan-api-design.md` — work item (b) 完了 (§10 skeleton 時系列 + §3 descriptor set + §4 render pass + §8 OS 別 + §9.4 MoltenVK = 本 §1.4/§2 input)
- `06-effort-estimation.md` — work item (c) 完了 (§3 milestone 別工数 + §4.4 OS 着手 timing + §5.6 marker 暦年 = 本 §1.2/§2 input)

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory (本 foundation group 完了を反映予定)
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (本 §2.1 + §5.1 charter outline で詳細化反映)
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 (本 §1.3 + §2 OS 着手 timing で反映)
- `project_r30_cinematic_control_tuning_deferred.md` — r30 BD cvar 13 件 tuning (本 §2.2 r42-β visual A/B 反映)
- `project_aya_visual_realism_alpha_protect.md` — scene buffer alpha invariant (本 §2.3 r42-γ acceptance criteria 反映)
- `project_atmos_atten_scalarized.md` — atmosFragLighting atten scalarization (本 §2.3 r42-γ sky dome 反映)
- `feedback_shader_color_space_correction.md` — shader 出力 linear / sRGB 逆引き (本 §2.3 r42-γ shader cross compile 反映)
- `feedback_visual_decisions_need_live_ab.md` — visual 決定は live A/B 必須 (本 §2.2 / §2.3 visual A/B 反映)
- `feedback_instant_ab_vs_sustained.md` — instant vs sustained A/B (本 §2.3 sustained viewing 反映)
- `feedback_credit_t_noami_equal_billing.md` — Mac t-noami workflow (本 §2.2 / §2.3 / §2.4 Mac 着手 timing 反映)
- `feedback_mac_only_fixes_accept_as_is.md` — Mac 限定 fix 受入 (本 §2.4 Mac MoltenVK 詳細化 反映)
- `feedback_proactive_handoff.md` — group 境界 handoff (本 foundation group 完了で次 session への handoff doc 作成)
- `feedback_self_verify_before_handoff.md` — group 完了時 self-trace (本 foundation group 完了で実施)
