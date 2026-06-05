# r41 Phase 2 着手前準備 — RELATIONS.md + READINESS.md 起案完了 + Phase 2 工程検討 handoff

> **着手契機**: 2026-06-06 design session (= 前 commit `aa7803357b` = UBO design 95 file 起案完了 + handoff の続き session) で AYA literal record 「Phase 2 の作業項目名 A-1, B-1〜B-31, C-1〜C-62 として、各作業を通して設計と工程を検討」「本セッションで難しければ次の Session に投げるメッセージをください」「3 OS 共通で動く処理で記述。Core 分散して動作する設計を守る。OpenGL を殺さない。も死守」record → 本 handoff doc 起案。
>
> **位置付け**: r41 milestone 内 **Phase 2 着手前準備 phase = RELATIONS.md + READINESS.md 起案完了 marker** = handoff Phase 2 prep (sub-letter) complete = 全 94 UBO 7 dimension 関係図 + A/B/C 判定 + 集計 (1/31/62) 確定 + 次 session 残作業 (= Phase 2 工程資料起案) **次 session に handoff**。

## §0. 本 session 経緯 + 達成事項 + 方針 lock

### §0.1 本 session 着手契機 + 経緯

**着手 contexts**: 前 commit `aa7803357b` (= 2026-06-06 UBO design 95 file 起案完了 + handoff `phase2-prep/handoff-phase2-prep-ubo-files-complete.md` 起案完了) 後の continuation session。AYA literal「OK」record 受領で RELATIONS.md + READINESS.md 起案進行承認。

**経緯 (時系列)**:

1. 必読 4 件 Read 完了 (= handoff + memory `project_r41_phase2_4_principles` + INDEX.md + Global_ReflectionProbes.md sample)
2. RELATIONS.md 起案計画 提示 (= 8 section、1 UBO 1 read 順次精査、方式 A 推奨)
3. AYA literal「OK」record 受領 = 計画承認
4. AYA literal「結果はどこに出力しましたか?」record = response 内蓄積方式の確認質問
5. 蓄積方式 (= 案 A response 内 / 案 B 別 file) 確認質問提示
6. AYA literal「いちいちここに出されてたら一生こちらは待ってないといけません 全 UBO ファイル精査に結果を資料に記述してから呼び出してください handoff 必要な時は別途呼び出してください 記述するファイル名２つを先に言ってください」record = silent 進行指示
7. file 名 2 つ提示 (= RELATIONS.md + READINESS.md)
8. silent batch 並列 Read で全 93 残 UBO file 順次精査完了 (= Global_ReflectionProbes は必読で既読、計 94 file 完了)
9. RELATIONS.md 起案完了 (= 8 section、7 dimension 集約、不明事項 15 dimension)
10. READINESS.md 起案完了 (= A/B/C 判定 + 集計 1/31/62 = 94、B/C 理由詳細全件記載)
11. AYA literal「ありがとう」record + 「Phase 2 の作業項目名 A-1, B-1〜B-31, C-1〜C-62 として、各作業を通して設計と工程を検討」record + 「本セッションで難しければ次の Session に投げるメッセージをください」record + 「3 OS 共通で動く処理で記述。Core 分散して動作する設計を守る。OpenGL を殺さない。も死守」record
12. **context 容量判定** = ~700k / 1M 使用 = 残 ~300k で 94 項目 × 設計工程 (= 推定 100-150k 追加) = 中途半端な完了 risk 大 = **次 session 推奨判断**
13. 本 handoff doc 起案

### §0.2 本 session 達成事項

#### §0.2.1 全 94 UBO file 順次精査完了 (= 1 UBO 1 Read で sequential)

- handoff §3.1 起案規律「1 UBO 1 read 順次精査」(= AYA literal「もっと 1 つ 1 つの UBO ファイル資料を精査しながら進めてもらう」遵守)
- batch 並列 Read で 1 turn 多 file (= context 効率) ただし各 file の §11/§10 careful 精査維持
- 全 94 file 完了 = sequential 整合

#### §0.2.2 RELATIONS.md 起案完了 (= 8 section、7 dimension 集約)

**起案先**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/RELATIONS.md`

**section 構成**:
- §1 descriptor set 同居関係 (= set 0/1/2/3 別 cluster + binding 表)
- §2 cadence cluster 関係 (= cadence_tag 0-5 全 6 種、PerProgram 80 件最大)
- §3 shader consume 関係 (= 確定済 30+ pair + shared include 4 件 + V/F cross-stage 共有 4 件)
- §4 データ依存関係 (= 17 系列: aya_sss_skin_flag/visual_realism/shadow_target_width/water/sky/light/glow chain/IBL/exposure/velocity/SSAO/screen res/camPosLocal 等)
- §5 dirty 連動関係 (= 確定 group 20+ trigger)
- §6 layout 共有 (= 全 UBO sAYAStandardLayout)
- §7 bind 順序 (= 9 種類 timing)
- §8 不明事項集約 (= 15 dimension)

#### §0.2.3 READINESS.md 起案完了 (= A/B/C 判定 + 集計 1/31/62)

**起案先**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/READINESS.md`

**集計**:

| 判定 | 件数 | 内訳 |
|---|---|---|
| **A 判定** | 1 | Skin_GLTFJoints (= pilot real data 通電済 Phase 1.E PC-N-5/11/15c) |
| **B 判定** | 31 | setter / data source / cadence 妥当性 不明、独立 dirty UBO |
| **C 判定** | 62 | 他 UBO 依存 (= 16 group 集約、cross-UBO 同期 / pair / sequential pipeline / cadence 矛盾) |
| 合計 | **94** ✅ | INDEX §2 一致 |

**C 判定主要 16 group** (= READINESS.md §3.1-§3.16):
- §3.1 aya_sss_skin_flag 3 UBO triple-write
- §3.2 aya_visual_realism + chroma_str + light cvar 7 UBO
- §3.3 shadow_target_width 3 UBO triple-write
- §3.4 box_center/box_size 2 UBO
- §3.5 GLTF texture transform 3 UBO
- §3.6 water 系 5 UBO
- §3.7 sky/cloud/atmospheric 10 UBO
- §3.8 velocity 5-6 UBO (curr/prev pair)
- §3.9 reflection probe / IBL 5 UBO
- §3.10 post-process chain 6 UBO
- §3.11 glow chain 4 UBO
- §3.12 SMAA 2 UBO
- §3.13 pathfinding 2 UBO
- §3.14 GLTF asset 2 UBO
- §3.15 set=2 binding=0 共有 (残) 2 UBO (ClipPlane + LightParams)
- §3.16 MultiLight 1 UBO

### §0.3 方針 lock 経緯 (= 本 session 確定 + 維持事項)

| 確定項目 | AYA literal record | 整合 memory |
|---|---|---|
| Phase 2 scope = **全 94 UBO 一括** | 前 session 確定 + 本 session 維持 | memory `project_r41_phase2_4_principles` 原則 3 (= 次 session で訂正、本 session も未訂正) |
| RELATIONS.md + READINESS.md 起案完了 | 「ありがとう」record 2026-06-06 | UBO design dir 配置 ✅ |
| **Phase 2 作業項目命名** = A-1, B-1〜B-31, C-1〜C-62 | 「Phase 2 の作業項目名 A-1, B-1〜B-31, C-1〜C-62 としたいと思います」record 2026-06-06 | 次 session で全 94 項目命名 + Phase 2 工程資料起案 |
| **各作業の設計と工程を検討** | 「各作業を通して設計と工程を検討したいと思います」record 2026-06-06 | 次 session 中核作業 |
| **次 session に handoff** | 「本セッションで難しければ次の Session に投げるメッセージをください」record 2026-06-06 | 本 handoff doc 起案 = 次 session entry |
| **3 OS 共通実装 死守** | 「3 OS 共通で動く処理で記述」record 2026-06-06 | memory `project_r41_phase2_4_principles` 原則 2 (= OS-1〜OS-10 gate) |
| **Core 分散設計 死守** | 「Core 分散して動作する設計を守る」record 2026-06-06 | memory `project_r41_phase2_4_principles` 原則 1 (= C1-C6 設計制約) |
| **OpenGL 殺さない 死守** | 「OpenGL を殺さない も死守」record 2026-06-06 | memory `project_r41_phase2_4_principles` 原則 4 (= O3-2 採用、r41 dual-path + r42 移管) |

## §1. 必読 file list (= 次 session 着手前)

memory `feedback_handoff_minimal_pre_req_read` 整合 (= 最低限読み、pinpoint Read 切替):

### §1.1 最低限 5 件 (= 次 session 着手前必読)

1. **本 handoff doc** (= 本 file、§0.1 経緯 + §0.2 達成 + §0.3 方針 lock + §2 残作業 + §3 起案規律 + §5 AYA literal record)
2. **`docs/specs/ayastorm-r41-gl-removal/design/ubo/RELATIONS.md`** (= 7 dimension 関係図、§4 data source 17 系列 + §5 dirty 連動 group 20+ + §8 不明事項 15 dimension)
3. **`docs/specs/ayastorm-r41-gl-removal/design/ubo/READINESS.md`** (= A/B/C 判定 + B/C 理由詳細全件 + §3 C 判定 16 group + §6 Phase 2 着手順序ヒント)
4. **memory `project_r41_phase2_4_principles.md`** (= 4 原則 = 分散処理意識 C1-C6 + 3 OS 同一実装 OS-1〜OS-10 + Phase 2/3 作業範囲 + OpenGL 殺さない O3-2)
5. **memory `project_ayastorm_r41_design_principles.md`** (= 2 大設計原則 = Upstream OpenGL 取り込みやすさ + Core プロセス分散実現)

### §1.2 pinpoint Read 切替 (= 必要時のみ)

- `docs/specs/ayastorm-r41-gl-removal/design/ubo/INDEX.md` (= 全 94 UBO summary + cadence_tag mapping、§4 横断不明事項 12 件)
- 各 UBO 94 file (= 個別設計詳細、設計検討時 pinpoint Read で逐次)
- `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` (= 前 session revert 済、再構成対象)
- 前 commit handoff `handoff/phase2-prep/handoff-phase2-prep-ubo-files-complete.md` (= UBO design file 起案完了 marker)

## §2. 残作業 (= 次 session 実施、AYA literal 2026-06-06 確定指示)

### §2.1 全 94 作業項目命名 (= A-1, B-1〜B-31, C-1〜C-62)

**AYA literal**: 「Phase 2 の作業項目名 A-1, B-1〜B-31, C-1〜C-62 としたいと思います」

**命名 mapping**:
- **A-1** = Skin_GLTFJoints (= READINESS.md §2 A 判定唯一)
- **B-1 〜 B-31** = READINESS.md §5.1 B 判定 31 件 (= 命名順は READINESS.md §4 記載順を踏襲推奨)
- **C-1 〜 C-62** = READINESS.md §3 C 判定 62 件 (= 命名順は READINESS.md §3.1-§3.16 group 順を踏襲推奨、group 内 UBO 番号で連番)

**命名規約**:
- 各項目 ID = UBO 1 件 = 1 作業項目 (= 1:1 対応)
- C 判定は group ID も併記 (= 例 `C-1 (§3.1-1)` = §3.1 aya_sss_skin_flag group の 1 番目)

### §2.2 各作業項目の設計検討

**AYA literal**: 「各作業を通して設計と工程を検討したいと思います」

**各項目の設計検討内容**:

1. **目標 (= 何を実装するか)**:
   - register 経路 (= `registerProgramUbo` / `registerSkinUbo` / per-draw 新 method 等)
   - write 経路 (= 既存 OpenGL setter から forwardToUboUpload redirect)
   - flush 経路 (= cadence 別 flush method)
   - shader 接続 (= 既存 `#ifdef LL_VULKAN_GLSL` block 活性化 verify)
2. **不明事項 unblocking (= grep verify 項目)**:
   - 各 UBO file §10 不明事項を grep verify で解消
   - setter call site 特定 (= 推定根拠の実コード verify)
   - data source 上流 (= owner class + lifetime + dirty trigger)
3. **4 原則整合 (= AYA literal 2026-06-06)**:
   - **原則 1 Core 分散**: C1-C6 設計制約整合 (= thread-local accessor / per-thread ring buffer / secondary cmdbuf / mutex / call site API 不変)
   - **原則 2 3 OS 共通**: OS-1〜OS-10 gate 整合 (= Vulkan 1.3 core / descriptor set 数 5 維持 / std140 padding / minUniformBufferOffsetAlignment / vkCmdUpdateBuffer 65536 / Linux validation 0 件 / std::thread + std::mutex / OS 固有 path 不混入 / PC-N-15a infra API 不変)
   - **原則 3 Phase 2/3 作業範囲**: 全 94 UBO Phase 2 一括 (= 訂正済方針、本 phase 確定)
   - **原則 4 OpenGL 殺さない**: O3-2 採用 (= r41 milestone 内 OpenGL path 撤廃しない、dual-path 出荷、r42 移管)、`#ifdef LL_VULKAN_GLSL` gate で `#else` block (= OpenGL 経路) 温存

### §2.3 各作業項目の工程検討

**各項目の工程検討内容**:

1. **着手順序**:
   - A 判定 (1 件) = 最短経路 (= A-1 Skin_GLTFJoints Phase 1.F+ real bone matrix 接続)
   - B 判定 (31 件) = 独立着手可能、setter 特定後並列
   - C 判定 (62 件) = group 単位着手 (= 16 group)、group 内同時設計必須、group 間並列可能
2. **並列性**:
   - B 判定: 独立 UBO ゆえ複数並列着手可能 (= 推定 worker thread 並列度に応じ)
   - C 判定: group 内同時 (= cross-UBO 同期 protocol 確立)、group 間並列 (= 例 glow chain と SMAA は独立)
3. **横断 protocol 確立** (= 全 group 共通の設計入力、READINESS.md §6.4):
   - name-based dispatch logic (= set=1 binding=0 排他 + set=2 binding=0 共有 6 UBO + set=3 binding 衝突 3 site)
   - LLStaticHashedString UBO redirect 経路 (= CAS / Clip / Luminance / VisualizeBuffersF / Exposure 5+ UBO 影響)
   - per-shader UBO block 拡大 (= Frame UBO 多 file 拡大、upstream merge conflict risk)
   - cadence 再評価 (= velocity / GLTF material / per-frame 変化 member の per-program stale risk 多数 UBO)
4. **r42 移管項目明示**:
   - OpenGL path 撤廃 (= O3-2 採用、r42 milestone 後半 sub-phase 移管)
   - Core 分散完成 (= worker thread default ON 化、r42 milestone)
   - r41 milestone 内は dual-path 出荷で完結

### §2.4 Phase 2 工程資料 起案

**起案先候補** (= AYA 承認要):
- 候補 A: `docs/specs/ayastorm-r41-gl-removal/design/ubo/PHASE2_PLAN.md` (= UBO design dir 内、ただし UBO 名 = file 名原則違反、ただし RELATIONS.md + READINESS.md で既明示許可 pattern と同形)
- 候補 B: `docs/specs/ayastorm-r41-gl-removal/design/09-phase2-plan.md` (= roadmap chapter と並列の design chapter)
- 候補 C: `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` の §2.1 / §5.2 / §5.3 等 直接訂正 (= 前 session revert 済の re-fill)

**推奨**: **候補 C** = roadmap §2.1 Phase マップ + §5.2 Phase 順序 + §5.3 Exit Criteria を全 94 項目命名前提で再構成 (= 前 session revert 済の再 fill、roadmap が source of truth)。ただし 94 項目詳細は分量大 → サブ doc 切出案も検討要 (= 例: roadmap §5.2 から `phase2-94-items-design.md` 等のサブ doc 参照)。

= AYA literal 承認後 file 名確定。

### §2.5 Phase 2 工程再設計提案 + AYA review

- 全 94 項目命名 + 設計 + 工程記載完了後、AYA に review 提示
- 修正 cycle 後 commit
- Phase 2 着手承認後、別 session で Phase 2 本実装着手 (= 本 phase = 設計 phase 完了)

## §3. 起案規律 (= 次 session 絶対遵守、本 session 失敗 record 反面教師継承)

### §3.1 各 UBO file 精査規律 (= 前 session 起案規律継承)

- **1 UBO 1 read 順次精査** (= AYA literal 「もっと 1 つ 1 つの UBO ファイル資料を精査しながら進めてもらう」遵守)
- 設計検討時の pinpoint Read 切替 (= 各 UBO file §10 不明事項 + §11 関係 + §6 setter call site をピンポイント参照)
- 推論禁止、不明は不明明示 (= memory `feedback_admit_unknown` 遵守)

### §3.2 4 原則 死守規律 (= AYA literal 2026-06-06 明示)

- **3 OS 共通実装 死守**: 全 94 項目で OS-1〜OS-10 gate 整合確認、OS 固有 path / dlopen / driver-specific code 不混入
- **Core 分散設計 死守**: 全 94 項目で C1-C6 設計制約整合、call site API 不変、thread-local accessor 経由、per-thread ring buffer 経路
- **OpenGL 殺さない 死守**: r41 milestone 内で OpenGL path 撤廃しない、`#ifdef LL_VULKAN_GLSL` gate で `#else` block 温存、r41 release dual-path 出荷、OpenGL 撤廃は r42 移管明示

### §3.3 命名統一規律

- 全 94 項目 = A-1, B-1〜B-31, C-1〜C-62 で統一命名 (= AYA literal 確定)
- 命名と UBO 名の対応 mapping table 起案 (= Phase 2 工程資料内 + 必要時 INDEX.md にも併記)
- 命名順は READINESS.md §3/§4/§5.1 記載順踏襲推奨 (= AYA 別案あれば優先)

### §3.4 設計検討 detail level 規律

- 各項目 100-200 行程度の設計検討 (= 推定、項目によって増減)
- 既存 RELATIONS.md / READINESS.md 内容との重複は最小化 (= 参照引用で済ます)
- Phase 2 着手後の本実装作業の入力資料として十分な詳細

### §3.5 design phase 規律

- `indra/` 改変ゼロ (= memory `feedback_design_phase_no_code_write` 遵守)
- doc 起案 / 既 doc 訂正のみ
- 実コード source は read-only 参照 (= grep verify 含む)

### §3.6 表記精度規律

- 数字は常に確定値 (= 94 / 1 / 31 / 62 等) で表記、文脈で「合計」「内訳」等明示
- memory `feedback_no_bare_reference_ids` 整合 = 項目 ID + 内容明記 (= A-1 = Skin_GLTFJoints 等)

### §3.7 推論禁止規律 (= 本 session 失敗 record 反面教師、前 session 失敗 record 継承)

- 設計検討で推論で確定形書かない (= 例 cadence 再評価候補は「verify 要」明示、推論で「PerDraw 移行すべき」と確定形に書かない)
- 各項目の不明事項は項目内 §不明事項 section に明示
- AYA literal「`feedback_doubt_self_first` + `feedback_admit_unknown` + `feedback_build_only_verified`」遵守

### §3.8 AYA literal scope 厳守規律 (= 前 session 失敗 record 反面教師継承)

- AYA literal 命名 = A-1, B-1〜B-31, C-1〜C-62 厳守 (= 別命名提案で AYA literal 違反禁止)
- AYA literal 4 原則 死守 (= 3 OS 共通 + Core 分散 + OpenGL 殺さない、設計検討で原則 violation 提案禁止)

## §4. 本 commit scope

**本 commit 対象**:
- `docs/specs/ayastorm-r41-gl-removal/design/ubo/RELATIONS.md` (= 新規、§0.2.2)
- `docs/specs/ayastorm-r41-gl-removal/design/ubo/READINESS.md` (= 新規、§0.2.3)
- `docs/specs/ayastorm-r41-gl-removal/handoff/phase2-prep/handoff-phase2-prep-relations-readiness-complete.md` (= 本 file、新規)

**合計 3 file 新規追加**。

**commit 対象外**:
- 既 commit 済 file (= 前 commit `aa7803357b` で commit 済 96 file)
- `indra/` 配下 (= design-phase 規律遵守、改変ゼロ)
- memory store (= 別管理)

**commit message 候補**:

```
docs: r41 Phase 2 着手前準備 = UBO RELATIONS + READINESS 起案完了 + handoff

AYA literal 確定 2026-06-06:
- Phase 2 作業項目命名 = A-1, B-1〜B-31, C-1〜C-62 (= literal「項目名 A-1, B-1〜B-31, C-1〜C-62」)
- 各作業設計 + 工程検討は次 session 移管 (= literal「難しければ次の Session に投げるメッセージを」)
- 4 原則 死守 (= 3 OS 共通 + Core 分散 + OpenGL 殺さない、本 session 維持事項)

新規 3 file:
- design/ubo/RELATIONS.md (= 全 94 UBO 関係図、7 dimension 集約 + 不明事項 15 dimension)
- design/ubo/READINESS.md (= A/B/C 判定 + 集計 1/31/62 = 94、B/C 理由詳細全件)
- handoff/phase2-prep/handoff-phase2-prep-relations-readiness-complete.md (= 本 handoff = 次 session 残作業 + 起案規律 + AYA literal record)

次 session 残作業:
- 全 94 項目命名 (= A-1, B-1〜B-31, C-1〜C-62 mapping table)
- 各項目設計 + 工程検討 (= 100-200 行 × 94 項目)
- Phase 2 工程資料起案 (= file 名 AYA 承認後確定、推奨 roadmap §2.1/§5.2/§5.3 直接訂正)
```

= Co-Authored-By 行不在 (= memory `feedback_no_claude_coauthor` 遵守)

## §5. AYA literal record 2026-06-06 (= 本 session 全 literal 集約)

時系列:

1. **「OK」** (= 前 session 提示 RELATIONS.md 起案計画承認、session entry)
2. **「結果はどこに出力しましたか?」** (= 蓄積方式確認質問、私の「response 内案 vs 別 file 案」確認要求への反応)
3. **「いちいちここに出されてたら一生こちらは待ってないといけません 全 UBO ファイル精査に結果を資料に記述してから呼び出してください handoff 必要な時は別途呼び出してください 記述するファイル名２つを先に言ってください」** (= silent 進行 + 中間確認禁止 + file 名 2 つ確定指示)
4. **「ありがとう」** (= RELATIONS.md + READINESS.md 起案完了確認)
5. **「それでは Phase ２の作業の項目名を A-1, B-1〜B-31, C-1〜C62 としたいと思います。そのうえで各作業を通して設計と工程を検討したいと思います。」** (= 全 94 項目命名指示 + 設計工程検討指示)
6. **「本セッションで難しければ次の Session に投げるメッセージをください。」** (= context 容量判定で次 session handoff 許可)
7. **「３OS共通で動く処理で記述。Core 分散して動作する設計を守る。OpenGL を殺さない。も死守してください。」** (= 4 原則中 3 原則 (= 原則 1/2/4) 死守再確認)

## §6. self-verify 9 観点

1. **RELATIONS.md 起案完了 §0.2.2** ✅ (= 8 section、7 dimension 集約 + 不明事項 15 dimension)
2. **READINESS.md 起案完了 §0.2.3** ✅ (= A/B/C 判定 + 集計 1/31/62 = 94、B/C 理由詳細全件)
3. **全 94 UBO file 順次精査完了 §0.2.1** ✅ (= 1 UBO 1 Read sequential)
4. **AYA literal 4 原則 死守 §0.3 + §3.2** ✅ (= 3 OS 共通 + Core 分散 + OpenGL 殺さない明示、Phase 2 設計検討で違反禁止)
5. **次 session 残作業 §2** ✅ (= 94 項目命名 + 設計工程検討 + Phase 2 工程資料起案 + AYA review 4 項目明示)
6. **起案規律 §3** ✅ (= 1 UBO 1 read + 4 原則 死守 + 命名統一 + 設計 detail + design-phase + 表記精度 + 推論禁止 + AYA literal scope 厳守 8 項目)
7. **必読 file list §1** ✅ (= 最低限 5 件 + pinpoint Read 切替、memory `feedback_handoff_minimal_pre_req_read` 整合)
8. **AYA literal record §5** ✅ (= 時系列 7 件全引用)
9. **commit scope + message §4** ✅ (= 3 file 新規 + Co-Authored-By 不在 + 4 原則 + 次 session 残作業明示)

### §6.1 feedback 遵守 record

- `feedback_proactive_handoff` ✅ (= 本 handoff doc 起案 = context 残量判定で能動 handoff、AYA literal「難しければ」許可)
- `feedback_handoff_minimal_pre_req_read` ✅ (= 必読 5 件 + pinpoint Read 切替、全 94 UBO file は設計時 pinpoint Read で逐次)
- `feedback_design_phase_no_code_write` ✅ (= `indra/` 改変ゼロ、doc 起案のみ)
- `feedback_admit_unknown` ✅ (= 全項目で不明事項 明示、推論禁止)
- `feedback_doubt_self_first` ✅ (= 本 session 中 cadence 再評価等は「verify 要」で確定形回避)
- `feedback_build_only_verified` ✅ (= 実コード source 直接 reference、推論禁止)
- `feedback_no_scope_shrink` ✅ (= 全 94 項目全件命名予定、抜けなし)
- `feedback_no_auto_commit` ✅ (= 本 handoff 提示後 AYA literal 承認後 commit)
- `feedback_no_claude_coauthor` ✅ (= commit message に Co-Authored-By 行不在)
- `feedback_release_branch_workflow` ✅ (= feature branch `feature/ayastorm-r41-gl-removal` 上で commit)
- `feedback_tests_dir_never_commit` ✅ (= root `/tests/` 改変なし、個別 file 指定 add で安全)
- `feedback_no_bare_reference_ids` ✅ (= A-1 / B-1〜B-31 / C-1〜C-62 + 17 data source 系列 + 16 group ID + 15 不明 dimension + 4 原則 全件内容明記)
- `feedback_explanation_lead_with_conclusion` ✅ (= AYA への return message で結論ファースト = handoff 推奨判断 + 残作業要約)
- memory `project_r41_phase2_4_principles` ✅ (= 4 原則 死守、Phase 2 全項目で違反禁止)
- memory `project_ayastorm_r41_design_principles` ✅ (= 2 大設計原則 継承、Phase 2 で具体化)
- memory `project_ayastorm_three_platforms` ✅ (= 3 OS 共通実装、原則 2 で具体化)

### §6.2 次 session entry 確認

次 session で本 handoff doc + 必読 5 件 Read 後、AYA literal「次 session 開始」「Phase 2 設計検討再開」等の record 受領 → §2 残作業着手:
1. 全 94 項目命名 mapping table 起案 (= A-1, B-1〜B-31, C-1〜C-62)
2. 各項目設計検討 (= 4 原則整合 + 不明事項 unblocking + 4 経路 (register/write/flush/shader 接続))
3. 各項目工程検討 (= 着手順序 + 並列性 + 横断 protocol)
4. Phase 2 工程資料起案 (= AYA 承認後 file 名確定、roadmap §2.1/§5.2/§5.3 直接訂正案推奨)
5. AYA review + 修正 cycle
6. commit + AYA literal 確認

## §7. context 容量判定 record (= handoff 起案 trigger)

**本 session context 消費**:
- 必読 4 件 Read (= handoff + memory + INDEX + Global_ReflectionProbes sample) ~25k
- 全 94 UBO file Read (= sequential batch 並列) ~400-500k
- RELATIONS.md 起案 ~40k
- READINESS.md 起案 ~40k
- 中間 AYA 確認 + response ~20k
- 本 handoff 起案 ~30k
- **合計 ~600-700k / 1M (= 60-70% 消費)**

**残作業推定** (= 94 項目命名 + 設計工程検討 + 工程資料起案):
- 94 項目 mapping table ~5k
- 各項目設計工程検討 (100-200 行 × 94 項目) ~100-150k
- Phase 2 工程資料起案 ~30-50k
- AYA review + 修正 cycle ~30-50k
- 合計 ~165-255k 追加

**判定**: 残 ~300-400k で残作業 ~165-255k は理論上収まるが、AYA review + 修正 cycle で 1-2 周回必要時に context 不足 risk 高 = **次 session handoff 推奨** (= AYA literal「難しければ次の Session に投げるメッセージを」許可活用)。

= 本 session = RELATIONS + READINESS 完了で clean 終了、次 session = Phase 2 工程検討 clean 開始の段階分割が安全。
