# handoff = r41 Phase 2.L0 sub-session 1 = 再精査 session = 不確実性監査 doc

**起案日**: 2026-06-06
**位置付け**: Phase 2.L0 sub-session 1 (= 再精査 session) 出力 doc。`indra/` 改変ゼロ、cold read + Grep ベースの不確実性 quantitative 監査。
**起案契機**: handoff `phase2/handoff-phase2-l0-entry.md` §3.3 出力 doc 仕様 + AYA literal 2026-06-06「何が足りてないのかが見えてこない」直接回答。
**起案規律**:
- memory `feedback_proactive_risk_management` 適用 (= リスク管理肩代わり禁止)
- memory `feedback_admit_unknown` 適用 (= 推論禁止、不明明示)
- memory `feedback_handoff_minimal_pre_req_read` 適用 (= cold read 最低限 3 件 + pinpoint)
- 推奨案 OK 自走承認継続、ただし AYA literal 確認 candidate は省略しない

---

## §1. 着手目的 (= AYA literal「何が足りてないのか見えてこない」解消)

不確実性総量 count + 分類 + AYA literal 確認 candidate list 化を doc 化し、AYA さんが客観 view で「Claude 提案を信用できるか / 何が不足か」を判断可能にする。

旧版 (= 上書き前 `handoff-phase2-l0-entry.md` commit `a0a4f0a583`) は不確実性総量 count を doc 上に提示せず「進める方向」だけで起案された (= F2 指摘)。本 doc は **実 Grep count で 510 件 marker** を確定数字として提示、protocol 別 + 解消可否別に分類して可視化する。

---

## §2. cold read 完了状態 (= sub-session 1 §3.2 全 10 件)

| # | doc | 状態 | 主要 finding |
|---|---|---|---|
| 1 | `design/ubo/WORK_ORDER.md` §1 + §2 + §4 + §5.4 | ✅ 読了 (§3 = 3000+ 行は Grep 集約のみ) | L0 4 protocol 詳細 + 4 原則 gate 29 件 + visual regression policy + sub-work 7 dim template |
| 2 | `design/ubo/READINESS.md` 全 | ✅ 読了 | A=1 (Skin_GLTFJoints) / B=31 / C=62 = 計 94、判定基準 + 詳細理由全件 |
| 3 | `design/ubo/RELATIONS.md` 全 705 行 | ✅ 読了 | §1 set/binding 同居、§4 data source 17 系列、§5 dirty 連動 20+ group、§8 不明事項 15 dimension |
| 4 | `design/ubo/INDEX.md` 全 | ✅ 読了 | 全 94 UBO summary + cadence_tag 6 種 (PerFrame 3 / PerProgram 80 / PerDraw 7 / PerAsset 2 / PerSkin 1 / SINGLETON 1) + §4 横断不明事項 12 件 |
| 5 | 各 UBO file §10 + §11 (= 94 × 2 section) | ⚠️ Grep count のみ (= sample 3 件 sufficient 確認後 fileごと精読は不要判断) | per-UBO file 内 marker = 平均 1-2 件 / file (= §10 不明事項の concentrated) |
| 6 | `design/01-overview.md` §1 + §2 + §5 | ✅ 読了 | 2 大設計原則 (= call site API 温存 / Core 分散) + §5 13 確定事項 (= 2026-06-03 session 確定) |
| 7 | `design/06a-prep-phase0-measurement.md` §5.5 + §6 + §7 | ✅ 読了 | Phase 0 計測 2026-06-03 完走、§5.5.5 dead candidate 121 件 / §5.5.6 LLStaticHashedString 40 件 / §7 未確定全件解消 |
| 8 | `design/06b` + `06c` 構造 (= 全 section title) | ✅ Grep のみ (= dirty/bind 詳細は L0-1.B/L0-3.B trace session で精読) | 06b §8 / 06c §10 持越事項あり |
| 9 | `design/08-build-codegen-pipeline.md` §6.4 + §13 + §17 (= 持越) | ✅ Grep のみ | §6.4 256B padding / §13 3 OS 確証 / §17 持越事項 |
| 10 | Phase 1.A〜1.E complete handoff | ✅ Phase 1.E + 1.A complete sample 読了 | Phase 1 完走 (R1/R2/R9/R10 ✅) + R3/R4/R5/R6 = Phase 2 仕事、Linux primary baseline 確立、pilot 通電 Skin_GLTFJoints + Asset_GLTFMaterials/Nodes + PerDrawUBO_LightParams + Global_ReflectionProbes |

**判定**: cold read 完了、定量 count に充分な情報量を確保。94 UBO file 全件 §10 精読は context 消費過大かつ Grep count で marker 総数把握可能ゆえ skip (= 必要時 L0-N.B trace session で pinpoint 精読)。

---

## §3. 不確実性 count + 分類

### §3.1 マーク総 count (= Grep 実測、全 doc 横断、2026-06-06)

| マーク | 件数 | WORK_ORDER 集中 | 個別 UBO file | handoff/prep doc |
|---|---|---|---|---|
| `[要追加調査]` | **258** | 178 (69%) | 76 (29%) | 4 (2%) |
| `[要 verify]` | **163** | 109 (67%) | 50 (31%) | 4 (2%) |
| `[要 AYA 判断]` | **66** | 46 (70%) | 14 (21%) | 6 (9%) |
| `[要 L0-N 結果反映]` | **23** | 6 (26%) | 17 (74%) | 0 |
| **合計** | **510** | **339 (66%)** | **157 (31%)** | **14 (3%)** |

**含意**:
- 旧版 handoff 「~100+ 件」当初想定 vs **実測 510 件** = **5 倍規模** (= F2 旧版失敗「不確実性総量未 count」の literal 確認)
- WORK_ORDER 集中 339 件 (66%) = sub-work (2) 不明事項 + sub-work (3) 調査手法に concentrated
- 個別 UBO file 157 件 (31%) = §10 不明事項 (= WORK_ORDER §3.N.M の単純複写ではなく個別 UBO 固有事項を含む)
- handoff/prep doc 14 件 (3%) = Phase 0/1 完走 record 内の残課題 cross-reference

### §3.2 解消可否別分類 (= 分類規則 = handoff §3.3.1 仕様、Claude 独自分類)

| 分類 | 件数 (推定) | 解消手段 | 解消 timing |
|---|---|---|---|
| **(a) 解消可能** = grep / Read で確定可能 | ~280 件 (55%) | sub-session B (= trace small prototype) 内で `indra/` 既存 reading のみで解消 | L0-N.B sub-session 内 |
| **(b) 実装試行必要** = small prototype 必要 | ~150 件 (29%) | sub-session B 内で 1-2 UBO pilot 実装試行 (= 1 file 実装 + cold launch 確認) | L0-N.B sub-session 内 |
| **(c) AYA literal 判断要** = 設計判断 | 66 件 (= [要 AYA 判断] literal 全件 + 一部 [要 verify]/[要追加調査] からの promote) | sub-session A (= 再精査) 末尾で AYA literal 確認 | 各 L0-N.A 完了時 |
| **(d) L0-N 結果反映** = 他 protocol 結果待ち | 23 件 (= [要 L0-N 結果反映] literal 全件) | L0-N protocol 完了後に自動解消 | L0-N.C 完了後 |

**分類は推定** (= 各 marker 個別精読を sub-session 1 範囲外と判定)。L0-N.A sub-session で各 protocol 該当 marker 詳細精読で確定数字に refine。

### §3.3 protocol 別不確実性集約 (= L0-1 / L0-2 / L0-3 / L0-4)

L0 横断 protocol は §2 (WORK_ORDER) §2.1〜§2.4 各 section 内 + §3 (各 UBO sub-work 内) に分散して影響。protocol 別 marker は **直接 §2.N に記載されたもの + §3 各 UBO sub-work (1) 前提条件 で L0-N 待ち明示されたもの** を集約 (= mapping は §2.1〜§2.4 「影響箇所」記載 + READINESS §6.4 横断 protocol 列挙ベース)。

| protocol | §2 直接 marker 件数 | 影響 UBO 数 | 解消 AYA review candidate 件数 (§2.5.2 cross-ref) | 実装試行必要件数 |
|---|---|---|---|---|
| **L0-1** name-based dispatch | 4 (= [要追加調査] 2 + [要 AYA 判断] 2) | **80 件 PerProgram cluster 全件** (= AYA review 重点 site 11 UBO = set=1 binding=0 排他 2 + set=2 binding=0 共有 6 + set=3 binding 衝突 3、実不整合 site = 全 3 set 80 件 = Phase 2.L0 sub-session 3 step 1 grep 確定) | **1 件** = set=3 binding 衝突 3 site 解消方針 (= protocol-C 候補 i/ii/iii 選択、AYA literal「(i) 新規 binding allocation」採用済 2026-06-06) | 1 件 (= 既存 pilot 通電 UBO 5 件 dispatch logic trace) |
| **L0-2** LLStaticHashedString redirect | 4 (= [要追加調査] 2 + [要 AYA 判断] 2) | **40 件 = Phase 0 計測実測** (= 06a §5.5.6 LLStaticHashedString 経由 setter 観察 40 件、当初 5+ 推定の 8 倍) | **1 件** = mapping table 構築方式 (= code-gen / introspection / ハードコード) | 1 件 (= LLGLSLShader::uniformN(LLStaticHashedString) 既存実装読解) |
| **L0-3** per-shader UBO block 拡大 | 4 (= [要追加調査] 2 + [要 AYA 判断] 2) | 50+ shader file (= FrameViewProj 主要 deferred/forward + FrameLights 全 lighting consume = 現 8 件確認 + 残 lighting shader) | **2 件** = preprocessor inject 方式 + upstream merge conflict strategy | 1 件 (= 試験的 1 shader file pilot 拡大 = 別途 Phase 1.G 候補) |
| **L0-4** cadence 再評価 | 5 (= [要追加調査] 1 + [要 AYA 判断] 3 + [要 verify] 1) | **~24 件 = READINESS §4.2 + RELATIONS §8.2 集約** (= velocity 系 7 + GLTF material 系 3 + per-frame 変化 member 系 ~8 + post-process cadence question 系 ~3 + FsObjectIdF + ObjectSkin + RadianceGen + OcclusionCube + Normgen) | **2 件** = 再分類 strategy (sliced UBO / PerDraw 移行 / stale 許容) + sliced UBO 化 Phase 3 移管判断 | 1 件 (= cadence 再分類影響範囲 verify、他 UBO 関係 RELATIONS.md §5 dirty 連動 group 20+ 全件影響可能) |

**含意**:
- §2 直接 marker は protocol あたり 4-5 件 (= 計 17 件) と相対小、しかし **影響 UBO 数で展開すると合計 194+ UBO** (= 80 + 40 + 50 shader + 24 = 重複 cross あり、L0-1 = 80 件 PerProgram cluster 全件、Phase 2.L0 step 1 grep 確定) → L0 4 protocol 一括着手 reject は妥当判断
- AYA review candidate 計 **6 件** (= §2.5.2 一致)、各 L0-N.A sub-session 末尾で個別確認
- 実装試行必要 計 **4 件** (= 各 L0-N.B sub-session 1 件、既存 pilot trace + LLGLSLShader 読解 + 1 shader file 拡大 + cadence verify)

### §3.4 不足要素 6 件 list (= AYA literal「何が足りてないのか」直接回答 + 各解消 plan)

旧版 handoff `phase2/handoff-phase2-l0-entry.md` で Claude 側 self-audit で挙げた 6 件 (= F2/F3 失敗の根拠) を **本 sub-session 1 で詳細 count + 解消 plan 提示**:

| # | 不足要素 | 規模 (= count) | 解消 plan | 解消 timing |
|---|---|---|---|---|
| **N1** | 既存 OpenGL UBO 実装の pilot trace 経験不足 | OpenGL 実働 UBO = 4 種 (= `UB_REFLECTION_PROBES` / `UB_GLTF_NODES` / `UB_GLTF_MATERIALS` / `UB_GLTF_JOINTS`、01-overview §3.3 確定) + Vulkan blueprint = 85 個 (= 当時 doc snapshot) → **94 UBO 実数** | **L0-1.B trace small prototype session** (= 既存 pilot 通電 5 UBO 読解 = Skin_GLTFJoints / Asset_GLTFMaterials/Nodes / PerDrawUBO_LightParams / Global_ReflectionProbes)、`llvkloader.cpp` PC-N-5/11/15c 5 setter site + `llgltfasset.cpp` / `llgltfnode.cpp` writer trace | sub-session 2 (= L0-1.B) |
| **N2** | upstream Firestorm setter call site の全件 trace 不足 | Phase 0 計測 (06a §5.5.2) 実測 = **union 237 unique uniform** (s2=230 + s3=231)、reserved 318 件 + hashed-path 40 件 = 計測対象 358 件。setter call site grep 未完 | **L0-2.B trace small prototype session** (= LLStaticHashedString uniformN setter call site 全件 grep) + LLGLSLShader::uniformN(LLStaticHashedString) 実装内容 trace | sub-session 5 (= L0-2.B) |
| **N3** | 3 OS 別 driver 実機検証不足 | Linux primary baseline ✅ (Phase 1.E 確立) + Win/Mac = AYA 自身 Win 実機 + @t-noami Mac 実機委任 (= Phase 5 仕事) | **Phase 4 (Win 確証) + Phase 5 (Mac 確証) = r41 milestone 内 別 Phase**、Phase 2.L0 sub-session 範囲外 (= 4 原則 OS-7 gate 「Linux validation 0」のみ sub-session C で確認) | Phase 4/5 separate (= 本 Phase 2.L0 範囲外) |
| **N4** | shader file `LL_VULKAN_GLSL` block 完備性 unknown | Vulkan blueprint 85 個 + 94 UBO の各 shader file `#ifdef LL_VULKAN_GLSL` block 配置状態未棚卸 | **L0-3.A 再精査 session** (= 全 shader file `LL_VULKAN_GLSL` block 完備性 grep)、欠落 shader file list 化 + L0-3.B で preprocessor 動作 trace 確認 | sub-session 7 (= L0-3.A) |
| **N5** | L0-3 prototype 未実装 | preprocessor inject 方式 (= protocol-B 候補 i/ii/iii) の前例なし、既存 LL shader build に同様の機能あるかも未確認 | **L0-3.B trace small prototype session** (= 試験的 1 shader file pilot 拡大、preprocessor 動作 trace = η-1〜η-28 phase で確立済 logic 再確認 + 1 file 実装試行) | sub-session 8 (= L0-3.B) |
| **N6** | L0-4 cadence Phase 0 計測反映状態 unknown | Phase 0 計測 (06a §5.5) は **2026-06-03 完走済** = §5.5.5 dead 121 件 + §5.5.6 hashed-path 40 件 + §5.5.7 per-program ↔ per-draw 境界補正 (= matrix 系 ~20 件 per-draw 補正、PBR factor ~10 件 per-program 補正) → **Phase 0 計測結果は 06a §5.5 に doc 化済、本 sub-session で WORK_ORDER §2.4 cadence 再評価 protocol に未反映** | **L0-4.A 再精査 session** (= 06a §5.5 観察結果を WORK_ORDER §2.4 L0-4 protocol-A 調査手法 D3 cadence verify に統合反映 + cadence mismatch 重大 UBO ~24 件の Phase 0 観察 cpf 値 cross-reference) | sub-session 10 (= L0-4.A) |

**含意**:
- N1/N2/N4/N5/N6 = sub-session B (= trace small prototype) で解消可能 (= 各 L0-N.B 内で実施)
- N3 = Phase 4/5 separate (= 本 Phase 2.L0 範囲外、4 原則 OS-7 gate 「Linux validation 0」のみ sub-session C で確認)
- **N6 = 既に 06a §5.5 で doc 化済**、本 sub-session 1 で「未反映」状態を確認 (= 反映作業は L0-4.A で実施)

### §3.5 sub-session 1 完了時の AYA literal 確認内容 5 件

handoff `phase2/handoff-phase2-l0-entry.md` §3.3.5 仕様準拠で AYA さんに以下 5 件提示:

**確認 1: 不確実性総量 510 件 + protocol 別内訳 + 解消可否別分類** (= §3.1 + §3.2 + §3.3)
- AYA さん納得可能か?
- 「全 marker 個別精読」「protocol 別精読」「解消可否別精読」のいずれの粒度で更に refine 必要か?

**確認 2: AYA literal 確認 candidate 6 件** (= §3.3 protocol 別、§2.5.2 cross-ref)
- L0-1: set=3 binding 衝突 3 site 解消方針 (= protocol-A/B/C のいずれ採用するか)
- L0-2: LLStaticHashedString mapping table 構築方式 (= code-gen / introspection / ハードコード のいずれ採用するか)
- L0-3: per-shader UBO block preprocessor inject 方式 (= static include / build macro / shader loader 自動 のいずれ採用するか)
- L0-4: cadence 再分類 strategy (= sliced UBO / PerDraw 移行 / stale 許容 の各 UBO 別判断)
- L0-3: upstream merge conflict 自動検出 strategy
- L0-4: sliced UBO 化が Phase 3 移管対象か Phase 2 内か
- AYA literal「推奨案で OK」自走承認は各 L0-N.A 末尾で個別確認 (= 本 sub-session 1 で一括判断要求しない) → これで OK か?

**確認 3: 不足要素 6 件の各々 解消 plan** (= §3.4)
- N1〜N6 の解消 timing (= sub-session 2/5/7/8/10) で AYA literal 納得可能か?
- N3 (= 3 OS 別 driver 実機検証) を Phase 4/5 separate 移管で OK か (= 本 Phase 2.L0 範囲外確定)?

**確認 4: sub-session 1 結論パターン判断** (= handoff §3.4)
- (A) sub-session 2 着手 OK = L0-1.B trace small prototype 着手
- (B) 設計再起案 = 不確実性 count 大きすぎる / 不足要素解消 plan 不十分 → WORK_ORDER 部分再起案 (= sub-session 1.X として継続)
- (C) protocol 設計大幅変更 = 4 protocol 自体が現実的でない → memory `project_r41_phase2_4_principles` 原則 3 再交渉

**確認 5: 本 sub-session 1 出力 doc 起案規律** (= 本 doc 内容)
- 推定値 (= §3.2 解消可否別分類 ~280/~150/66/23) を確定数字でなく推定として doc 化、L0-N.A で refine する protocol で OK か?
- 94 UBO file §10 全件精読を skip して Grep count 集約のみで判断する protocol で OK か (= memory `feedback_handoff_minimal_pre_req_read` 適用)?

---

## §4. sub-session 1 Exit 条件 (= handoff §3.4 cross-ref)

| # | Exit 項目 | 判定基準 | 状態 |
|---|---|---|---|
| 1 | cold read 完了 | §3.2 全 10 件 doc 読了 | ✅ (= 本 doc §2 状態表) |
| 2 | uncertainty audit doc 起案完了 | §3.3 全 sub-section 内容反映 | ✅ (= 本 doc §3.1〜§3.5) |
| 3 | AYA literal 確認内容提示 | §3.3.5 5 件提示 | ✅ (= 本 doc §3.5) |
| 4 | AYA literal 確認受領 | sub-session 2 着手承認 / 設計再起案指示 / protocol 設計大幅変更 のいずれか | ⏳ AYA literal 待ち |

---

## §5. sub-session 1 後の sub-session 順序確認 (= handoff §2.2 cross-ref)

| # | sub-session | scope | 着手契機 |
|---|---|---|---|
| 2 | **L0-1.B trace small prototype** | 既存 pilot 通電 UBO 5 件の dispatch logic trace | 本 sub-session 1 Exit (4) AYA literal 「sub-session 2 着手 OK」受領 |
| 3 | L0-1.C 実装 | name-based dispatch logic 実装 + cold launch | sub-session 2 Exit 後 |
| 4 | L0-2.A 再精査 | LLStaticHashedString redirect 関連 doc cold read + AYA review candidate | sub-session 3 Exit 後 |
| 5 | L0-2.B trace small prototype | LLStaticHashedString uniformN setter call site 全件 grep + 既存 dual-write 経路有無確認 | sub-session 4 Exit 後 |
| 6 | L0-2.C 実装 | LLStaticHashedString redirect mapping table + intercept + dual-write 経路実装 | sub-session 5 Exit 後 |
| 7 | L0-3.A 再精査 | per-shader UBO block 拡大関連 doc cold read + 全 shader file `LL_VULKAN_GLSL` block 完備性 grep + AYA review 2 件 | sub-session 6 Exit 後 |
| 8 | L0-3.B trace small prototype | 既存 `LL_VULKAN_GLSL` block preprocessor 動作 trace | sub-session 7 Exit 後 |
| 9 | L0-3.C 実装 | per-shader UBO block 拡大 preprocessor inject 実装 | sub-session 8 Exit 後 |
| 10 | L0-4.A 再精査 | cadence 関連 doc cold read + Phase 0 計測 (06a §5.5) 反映状態確認 + AYA review 2 件 | sub-session 9 Exit 後 |
| 11 | L0-4.B trace small prototype | 既存 cadence stale risk 実 logic trace + UBO 単位 cadence 帰属再分類提案 | sub-session 10 Exit 後 |
| 12 | L0-4.C 実装 + Phase 2.L0 Exit | cadence 再分類実装 + cold launch + READINESS.md update + Phase 2.L0 Exit handoff 起案 | sub-session 11 Exit 後 |

**手戻り protocol** (= handoff §2.3 cross-ref):
- B → A = trace 結果が protocol 案と不整合 → 設計再起案
- C → B = 実装中 cold launch reject → 整合再確認
- C → A = cold launch reject 原因が設計案破綻 → 設計大幅変更

---

## §A. 本 doc の起案規律

- `indra/` 改変ゼロ (= sub-session A 性質、memory `feedback_design_phase_no_code_write` 解除 phase だが sub-session A 内は引続 維持)
- 推論禁止、不明明示 (= memory `feedback_admit_unknown`)
- 不確実性 count 推定値は推定として明示、確定数字でない (= L0-N.A で refine)
- 4 原則 + 視覚 regression ゼロ gate 遵守 (= 本 doc 内容で violation 検知ゼロ)
- AYA literal 確認内容 5 件は省略せず提示 (= AYA literal「推奨案で OK」自走承認継続中でも、設計判断は明示確認継続)

---

## §B. 関連 commit + doc

| 種別 | 内容 |
|---|---|
| 関連 doc | `handoff/phase2/handoff-phase2-l0-entry.md` (= sub-session 1 仕様起源、commit `0bc409461d` 書き直し版) |
| 関連 doc | `handoff/phase2-prep/handoff-phase2-prep-complete.md` (= Phase 2 前提条件 work 全完走 record) |
| 関連 doc | `design/ubo/WORK_ORDER.md` §2 + §4 + §5.4 + §3 (= L0 protocol 詳細 + 4 原則 gate + visual regression policy + 94 UBO sub-work) |
| 関連 doc | `design/ubo/READINESS.md` (= A/B/C 判定 + B 31 件 + C 62 件) |
| 関連 doc | `design/ubo/RELATIONS.md` (= 関係図 8 dimension 集約) |
| 関連 doc | `design/ubo/INDEX.md` (= 全 94 UBO summary + cadence_tag mapping) |
| 関連 doc | `design/01-overview.md` §2 + §5 (= 2 大設計原則 + 13 確定事項) |
| 関連 doc | `design/06a-prep-phase0-measurement.md` §5.5 (= Phase 0 観察結果 2026-06-03 完走) |
| 関連 memory | `feedback_proactive_risk_management` / `feedback_admit_unknown` / `feedback_handoff_minimal_pre_req_read` / `project_r41_phase2_4_principles` |
| 本 doc | sub-session 1 出力 doc、commit 候補 (= AYA literal Exit 承認後) |

---

## §C. 次 sub-session 開始時の AYA 確認

「上記 audit doc 確認、§3.5 AYA literal 確認 5 件への回答受領後、**Phase 2.L0 sub-session 2 = L0-1.B trace small prototype session (= 既存 pilot 通電 UBO 5 件 dispatch logic trace、`indra/` 既存 reading のみ、改変ゼロ)** で着手 OK か?」
