# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 Phase 2d-β prep handoff

**作成日**: 2026-06-03
**branch**: `feature/ayastorm-r41-gl-removal`
**前 handoff (= 直前 phase 完了)**: `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-complete.md`
**設計 source of truth**: `design/09-phase-roadmap.md` §2-§3 (= Phase 0 entry 起点) + `design/06a-prep-phase0-measurement.md` (= Phase 0 計測 spec source)
**第二次査読 report**: `design-review-2026-06-03-second-pass.md` (= 致命傷 2 + 修正推奨 20 + AYA 判断仰ぎ 4)
**状態**: **prep 起草**。AYA 判断 4 件 (Q22-NUM / Q23-K / Q24-S1 / Q25-21CNT) 完了 + 設計 chapter 群 修正推奨 20 件 反映完了の **2 前提が揃った時点で着手**。本 phase 自体は docs 化のみ、`indra/` 改変ゼロ、memory `feedback_design_phase_no_code_write` 継承。

---

## §0 本 handoff の目的 (= 次 session 着手地点)

Phase 2d-α verify 2 周完了 + Phase 2d-α 起因 5 root 全消化 + 5 program SPIR-V PASS (= Phase 2d-α complete handoff §1.5 / §8.2) を経て、η-28 sub-bundle の **GLSL parse pass 系作業**は完了。次の作業地点は **設計 source of truth (`design/09-phase-roadmap.md`) に従う migration roadmap への乗替**であり、その **Phase 0 (= 計測 phase、η-29 で実機実施予定) の entry-ready 状態を docs 化**するのが本 Phase 2d-β scope。

**本 phase の literal scope** = docs 化のみ:
1. **setter 30 entry point cookbook 反映** (= 06a-prep §2.2.1 / §2.2.2 を Phase 0 entry doc として整形 + cross-ref 整合)
2. **既存 GL state cookbook 反映** (= 04-frame-context §1.3 帯構成 + 06a §0.2 cadence 推定表 + inventory §3 / §7 を Phase 0 入力契約として整理)
3. **既存 vulkanize C++ bug 2 件 (triplanar 系) の Phase 0 計測対象追加候補化** (= Phase 2d-α complete §5.2 で確定した独立 bug を 06a-prep §3 / §4 と並列の (G') 計測 task として追記)

**本 phase の非 scope**:
- 実機 hook 実装 / build / log 取得 / 解析 (= `indra/` 改変、Phase 1+ 持越、memory `feedback_design_phase_no_code_write` 継承)
- 設計 chapter 群 修正推奨 20 件の本体反映 (= main session で並行処理、本 phase 完了条件ではない、§5.3)
- AYA 判断 4 件への返答待ち (= main session で受領、本 phase 入口前提)

---

## §1 pre-requisite 必読 file (= 最低限 3 件、memory `feedback_handoff_minimal_pre_req_read` 準拠)

新規 session 開始時に **最低限 3 件のみ** を Read で先読み、追加は pinpoint Read で必要時に切替。本 prep は docs 化 task のため、必読は spec doc 3 件 (= 全件 design-phase 確定済) を厳守。

### §1.1 必読 1: 直前 phase complete handoff
- `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-complete.md`
- §1 verify 結果 + §5 既存 vulkanize bug 2 件 + §9 引き継ぎ事項 (= 本 phase scope の根拠)

### §1.2 必読 2: 設計 Phase 0 spec source (= roadmap 起点)
- `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` §2 (= Phase マップ + 依存図) + §3 (= Phase 0 entry scope + Exit Criteria)

### §1.3 必読 3: Phase 0 計測 spec 詳細
- `docs/specs/ayastorm-r41-gl-removal/design/06a-prep-phase0-measurement.md` §0 (= design-phase / implementation-phase 分離) + §2 (= setter 30 entry point hook spec) + §6 (= 反映 flow 出力契約)

### §1.4 pinpoint Read (= docs 化作業中に必要時のみ)
- `design/06a-cache-structure-and-setter-redirect.md` §0.2 (= cadence 推定表、不明 16 件起点)
- `04-frame-context.md` §1.3 (= descriptor set 5 帯構成、UBO pivot 反映済 工程表)
- `ayastorm-r41-ubo-current-state-inventory.md` §3 (= 既存 UBO 一覧、binding 配置) + §7 (= 残課題)
- `design-review-2026-06-03-second-pass.md` §7 / §8 (= AYA 判断 4 件 + 修正 20 件、本 phase 着手前提)
- `audit-past-b-work-vs-design-2026-06-03.md` §1-§3 (= 過去 B 作業整合性 100% / blueprint 不在 0、Phase 0 入力契約の信頼性 source)

---

## §2 Phase 2d-β literal scope (= 本 phase deliverable 3 件)

### §2.1 Deliverable A: setter 30 entry point cookbook 反映 (→ `06a-prep` 内 update)

**目的**: Phase 0 (η-29) 実機実施時に **手順書として 1 doc から駆動可能** な状態を作る。

**対象 doc**: `design/06a-prep-phase0-measurement.md`

**反映内容**:
| sub-task | 反映先 § | 内容 |
|---|---|---|
| A-1 | §2.2.1 (17 method 表) | 第二次査読 §2.4 由来の 06a §5.1 16 method 表との不一致を **本 prep 完了をもって 06a-prep 側を正式 source of truth と確定** = §2.2.1 表の注 line 106-107 を「06a §5.1 は本 phase 2d-β で 17 method に修正完了 (修正推奨 §8.1 #06a/3 反映済)」に書換、cross-ref 整合 |
| A-2 | §2.2.2 (13 method 表) | 13 method 確定状態を Phase 0 entry-ready マーク、§2.2.2 注の getGlobalRegistry() 不存在確認 (= Q24-S1 解消方針追従) を §2.2 末尾に「S1-代替案確定済 (= Q24-S1 AYA 判断結果反映)」note 追記 |
| A-3 | §2.3.3 (frame counter 公開方式) | §7 (P3)(P4) 残候補 (= helper 公開 / 既存 frame counter 流用) を **Phase 0 入口で grep + AYA 判断**前提を明文化、本 prep 完了後の Phase 0 着手 1 文書のみで判断可能化 |
| A-4 | §2.4 (CMake build flag) | §7 (P2) CMake patch 配置先未確定を **Phase 0 入口 grep 範囲 = `indra/cmake/00-Common.cmake` + `indra/cmake/LLRender.cmake` 2 候補**と pin 化、Phase 0 着手 1 step で配置先確定可能に絞込 |
| A-5 | §2.5 (log 出力仕様) | LL_INFOS class 名「UBO_CADENCE」衝突有無 (= §7 (P1)) を **Phase 0 入口 grep の 1 行 command pinned** で記述、衝突時の代替名 `AYA_UBO_CADENCE` を明文化 |

**注**: A-1 / A-2 は **Q24-S1 / Q22-NUM AYA 判断結果が `06a` 側 修正に反映完了している前提**。前提未充足時の handling は §6.1。

### §2.2 Deliverable B: 既存 GL state cookbook 反映 (→ `06a-prep` 内 §1.1 入力 source 拡張)

**目的**: Phase 0 計測の **入力契約 (= 何を計測するか、どの既存 state を比較対象とするか) を 1 表に集約**、現状 06a-prep §1.1 が 5 行 listing のみで散発のため、Phase 0 entry 1 文書からの逆引きが困難。

**対象 doc**: `design/06a-prep-phase0-measurement.md` §1.1 (= 入力 source 表) を拡張

**反映内容**:
| sub-task | 反映先 § | 内容 |
|---|---|---|
| B-1 | §1.1 入力 source 表 | 現状 5 行 (06a §0.2 / 05 §7.3 / inventory §3.3.1 / inventory §3.2 / 04 §6) に **04-frame-context §1.3 (= descriptor set 5 帯構成、工程表反映済)** を 6 行目として追加、Phase 0 計測時の「帯構成と uniform cadence の整合 verify」を入力契約化 |
| B-2 | §1.1 入力 source 表 | **`audit-past-b-work-vs-design-2026-06-03.md` §1-§3** (= 過去 B 作業整合性 100% / blueprint 不在 0 確定) を 7 行目として追加、Phase 0 計測結果の baseline (= 整合性 100% state) を入力契約化 |
| B-3 | §1.2 出力契約 | 現状の **chapter 06b 起案 prerequisite** 行を **chapter 06b + 09 §3 Exit Criteria 双方 prerequisite** に拡張 (= 09 §3.3 Exit Criteria 6 項目を 1.2 表末尾に link 列追加) |
| B-4 | §1.1 末尾に新 §1.1.1 追加 | **「既存 GL state cookbook」概念定義 inline** = 04-frame-context §1.3 descriptor set 5 帯 + 06a §0.2 cadence 推定表 + inventory §3 既存 UBO 一覧 の 3 軸を Phase 0 計測の比較基準 (= migration 前 state) と確定明示、Phase 0 計測結果との diff が migration 着手判断 source となる関係を 1 段落で記述 |

### §2.3 Deliverable C: 既存 vulkanize C++ bug 2 件 (triplanar 系) の Phase 0 計測対象追加 (→ `06a-prep` 内 §5 新規 (G') 追記)

**目的**: Phase 2d-α complete §5.2 で「Phase 2d-α 編集と独立した既存 vulkanize C++ slot allocation bug」と source-tree trace で確定した 2 件 (= paintmap triplanar parse fail + heightmap-with-noise triplanar link fail) を、**Phase 0 計測対象に正式追加** = Phase 0 着手時に取りこぼさない措置。

**対象 doc**: `design/06a-prep-phase0-measurement.md` 新規 §5.5 として追記 (= §5 解析 spec の末尾、(G') vulkanize C++ slot allocation 計測 task として)

**反映内容**:
| sub-task | 反映先 § | 内容 |
|---|---|---|
| C-1 | §5.5 (G') 新規追記 | trigger 条件 = `pbrterrainF.glsl` paintmap × triplanar permutation で `vary_texcoord` location override 28→29 + `vary_coords` location 29 衝突、log の確定証拠 (= `vulkanizeStageSource: F stage layout in 'vary_texcoord' location override 28 -> 29 (V↔F pair alignment)`) を listing |
| C-2 | §5.5 (G') | trigger 条件 = `pbrterrainF.glsl` heightmap-with-noise × triplanar permutation で vert `vary_coords` location=30 vs frag location=29 不整合 link fail、log の確定証拠 (= `ERROR: Linking vertex and fragment stages: Layout location qualifier must match`) を listing |
| C-3 | §5.5 (G') | 計測 protocol = `llglslshader.cpp` 内 `vulkanizeStageSource` の location override / V↔F pair alignment 経路を Phase 0 (H1b) hook と同 build に LL_INFOS hook 追加 (= class 名 `VULKANIZE_SLOT`)、override 発生 frequency + permutation 別 V/F location 値を histogram 化 |
| C-4 | §6 反映 flow | (G') 結果の反映先 = `inventory §7 残課題` に新規 #N (= slot allocation bug fix scope 確定) として追記、修正 phase は Phase 1.B (redirect 層実装と同 phase で host C++ 内修正) と紐付け |
| C-5 | §0.3 計測軸 表 | (D) (H1a) (H1b) (E') (F) の 5 軸表に (G') を 6 軸目として追加、軸 = static analysis + runtime measurement の混合 (= log で frequency 取得 + source-tree trace で slot allocation 経路確定) |

---

## §3 setter 30 entry point cookbook 集約表 (= Deliverable A-1 の source、本 prep 自己完結化)

§2.1 Deliverable A の反映作業を **本 prep 1 doc から逆引き可能**にするため、setter 30 entry point の集約表を本 §3 に inline 保持。本表は `06a-prep §2.2.1 / §2.2.2` から複製、修正は 06a-prep 側を source of truth として実施 (= 本 §3 は読み込み専用 snapshot)。

### §3.1 integer index 経由 setter (= 17 method、`llglslshader.cpp`)

| # | method | line (HEAD) | 引数 type | UBO 化対象 |
|---|---|---|---|---|
| 1 | `uniform1i` | 2141 | (U32, GLint) | × (sampler binding setter、独立 band) |
| 2 | `uniform1f` | 2166 | (U32, GLfloat) | ○ |
| 3 | `fastUniform1f` | 2192 | (U32, GLfloat) | ○ |
| 4 | `uniform2f` | 2202 | (U32, GLfloat×2) | ○ |
| 5 | `uniform3f` | 2229 | (U32, GLfloat×3) | ○ |
| 6 | `uniform4f` | 2256 | (U32, GLfloat×4) | ○ |
| 7 | `uniform1iv` | 2283 | (U32, U32, GLint*) | ○ |
| 8 | `uniform4iv` | 2310 | (U32, U32, GLint*) | ○ |
| 9 | `uniform1fv` | 2338 | (U32, U32, GLfloat*) | ○ |
| 10 | `uniform2fv` | 2365 | (U32, U32, GLfloat*) | ○ |
| 11 | `uniform3fv` | 2392 | (U32, U32, GLfloat*) | ○ |
| 12 | `uniform4fv` | 2419 | (U32, U32, GLfloat*) | ○ |
| 13 | `uniform4uiv` | 2447 | (U32, U32, GLuint*) | ○ |
| 14 | `uniformMatrix2fv` | 2475 | (U32, U32, GLboolean, GLfloat*) | ○ |
| 15 | `uniformMatrix3fv` | 2496 | 同上 | ○ |
| 16 | `uniformMatrix3x4fv` | 2517 | 同上 | ○ |
| 17 | `uniformMatrix4fv` | 2538 | 同上 | ○ |

### §3.2 LLStaticHashedString 経由 setter (= 13 method、`llglslshader.cpp`)

| # | method | line (HEAD) | 引数 type |
|---|---|---|---|
| 1 | `uniform1i` | 2617 | (LLStaticHashedString&, GLint) |
| 2 | `uniform1iv` | 2634 | (LLStaticHashedString&, U32, GLint*) |
| 3 | `uniform4iv` | 2652 | 同上 |
| 4 | `uniform2i` | 2670 | (LLStaticHashedString&, GLint×2) |
| 5 | `uniform1f` | 2688 | (LLStaticHashedString&, GLfloat) |
| 6 | `uniform2f` | 2705 | (LLStaticHashedString&, GLfloat×2) |
| 7 | `uniform3f` | 2723 | (LLStaticHashedString&, GLfloat×3) |
| 8 | `uniform4f` | 2740 | (LLStaticHashedString&, GLfloat×4) |
| 9 | `uniform1fv` | 2757 | (LLStaticHashedString&, U32, GLfloat*) |
| 10 | `uniform2fv` | 2774 | 同上 |
| 11 | `uniform3fv` | 2791 | 同上 |
| 12 | `uniform4fv` | 2808 | 同上 |
| 13 | `uniform4uiv` | 2826 | (LLStaticHashedString&, U32, GLuint*) |
| 14 | `uniformMatrix4fv` | 2844 | (LLStaticHashedString&, U32, GLboolean, GLfloat*) |

**注**: §3.2 は 14 行記述だが、`uniformMatrix4fv` を含めると 14 method、含めない場合 13 method。06a-prep §2.2.2 表は **14 method を listing しているが §0.3 注で「13 method」と記述**= 整合性軽微違反候補、本 phase Deliverable A-2 で確認 + 修正対象に追加候補化。

### §3.3 frame counter 配線箇所 (= 1 箇所)

| 箇所 | source | 根拠 |
|---|---|---|
| `LLAppViewer::idle()` 入口 (= main loop top) | `indra/newview/llappviewer.cpp` | per-frame 1 回確定、idle frame も拾える、shader bind ゼロ frame の anomaly 検知用 |

合計 entry point = **17 (index) + 14 (hashed) + 1 (frame counter) = 32 entry point** (= 06a §5.5 旧記述「32 entry point」と数値一致、ただし旧記述は 16+16+0 = 32 で内訳異なる、Deliverable A-1 で 06a §5.5 の内訳 17+13+? と整合させる必要)

---

## §4 既存 GL state cookbook 集約表 (= Deliverable B の source、本 prep 自己完結化)

§2.2 Deliverable B の反映作業を本 prep 1 doc から逆引き可能にするため、既存 GL state cookbook 3 軸 (= descriptor set 帯構成 / cadence 推定表 / 既存 UBO 一覧) の集約表を本 §4 に inline 保持。

### §4.1 軸 1: descriptor set 5 帯構成 (= 04-frame-context §1.3 工程表反映済 + 第二次査読 §6.1.1 数値確認済)

| set | binding 帯 | 用途 | 物理 instance 数 | source of truth |
|---|---|---|---|---|
| set=0 | 0-2 (3 個) | per-frame UBO | 1 (= GPU 共有 singleton) | `01-overview.md` §3.3 |
| set=1 | 0-1 (2 個) | per-asset UBO (= MaterialUBO 系 + GLTFNodes 等) | 1 + 2N (= 物理 instance 式) | `01-overview.md` §3.3 |
| set=2 | 0-25 or 0-26 (= **Q22-NUM 判断待ち**) | per-program / per-draw UBO | 1 + 2N (= 物理 instance 式) | `02-naming-convention.md` §3.3 (= 26) / `01` `05` `inventory` (= 25) 矛盾、Q22-NUM 確定後 source of truth 確定 |
| set=3 | 0-53 (54 個) | per-skin UBO / Legacy 系 | M (= 可変 instance 数) | `01-overview.md` §3.3 |
| (samp) | 0-48 (49 個) | sampler binding | - | `07-vulkan-api-state.md` §3 |

**Q22-NUM 解消方針依存箇所**: 本表 set=2 行の数値が **AYA 判断結果 (25 採用 or 26 採用) で確定**、本 phase Deliverable B-1 反映時に 04-frame-context §1.3 の対応行と整合させる。

### §4.2 軸 2: cadence 推定表 (= 06a §0.2 sub-doc に確定済、本 prep snapshot)

| cadence | 推定 uniform 数 (06a §0.2) | 確定状態 |
|---|---|---|
| per-frame | 85 | static analysis (= caller 関数名) のみで暫定確定、Phase 0 (H1b) で確証取得 |
| per-program | 92 | 同上、per-program ↔ per-draw 境界に ambiguous 候補あり |
| per-draw | 68 | 同上 |
| per-asset | 45 | 同上 |
| per-skin | 12 | 同上 |
| sampler binding (= UBO 化対象外、独立 band) | 49 | 06a-prep §2.7 表で独立報告軸 |
| **不明 16 件** | **16** | **Phase 0 (H1b) で確定対象** |

合計 = 85 + 92 + 68 + 45 + 12 + 16 = **318** (= 06a §0.2 unique uniform 261 + 重複検出 + 16 不明、最終確定は Phase 0 完了後)。

### §4.3 軸 3: 既存 UBO 一覧 (= inventory §3 baseline、84 個総数 = Q22-NUM 解消方針依存)

| set | UBO 数 (現行 source of truth、Q22-NUM 依存) |
|---|---|
| set=0 | 3 |
| set=1 | 2 |
| set=2 | 25 (= 01/05/inventory) or 26 (= 02) |
| set=3 | 54 |
| **総数** | **84** (= 01/05/inventory) or **85** (= 02 集計) |

**第二次査読 §6.1.1 数値突合 agent 結果**: 4 種 UB_* / cadence 5 分類 / 79 binding (40/39 split) / 物理 instance 式 = ✓ 全 chapter 一貫 (= 本 phase で再 verify 不要)。Q22-NUM 解消方針追従のみ Deliverable B 影響範囲。

### §4.4 (G') 既存 vulkanize C++ slot allocation bug 2 件 (= §2.3 Deliverable C 起点)

| # | program | permutation | error 種別 | log 確定証拠 |
|---|---|---|---|---|
| G'-1 | Deferred PBR Terrain Shader 0 | paintmap triplanar | parse fail (location 29 overlap) | `vulkanizeStageSource: F stage layout in 'vary_texcoord' location override 28 -> 29 (V↔F pair alignment)` |
| G'-2 | Deferred PBR Terrain Shader 0 | heightmap-with-noise triplanar | link fail (vert/frag slot allocation 不整合) | `ERROR: Linking vertex and fragment stages: Layout location qualifier must match: vertex stage: vary_coords "layout( location=30) smooth out" / fragment stage: vary_coords "layout( location=29) smooth in"` |

**root**: `indra/llrender/llglslshader.cpp` 内 `vulkanizeStageSource` の location override + V↔F pair alignment ロジック、Phase 2d-α 編集と独立 (= Phase 2d-α complete §5.3 source-tree trace で確定)。

---

## §5 Phase 2d-β 着手前提条件 (= 2 件、main session 受領)

### §5.1 前提 1: AYA 判断 4 件完了 (= 第二次査読 §7)

| ID | 内容 | 待ち項目 |
|---|---|---|
| **Q22-NUM** | set=2 帯 = 25 か 26 か (= 総数 84 か 85 か) | A 案 (25 + 84 採用) / B 案 (26 + 85 採用) のいずれ |
| **Q23-K** | chapter 09 §2.1 K placeholder と §5.2 template 矛盾 | A 案 (§5.2 を template 例に書直) / B 案 (§5.2 を一般化に書直) のいずれ |
| **Q24-S1** | chapter 06a §9 (S1) 持越 vs 06a-prep §2.2.2「(S1) 解消」chapter 間矛盾 | (S1-存在) 解消マーク + (S1-代替) AYA 採用案 (S1-A/B/C/D) のいずれ |
| **Q25-21CNT** | chapter 10 §1「21 件」 double-count 検証 (= 分類表 chapter 別追加) | 追加要 / 不要のいずれ |

### §5.2 前提 2: 設計 chapter 群 修正推奨 20 件 反映完了 (= 第二次査読 §8.1 + 本 audit §3.1)

- 第二次査読 §8.1: chapter 01-1 + 02-2 + 05-3 + 06a-4 + 09-5 + 10-2 + inventory-1 + handoff-1 = **18 件**
- 本 audit §3.1: Ch05 §3.3 25→26 (= Q22-NUM 解消方針追従) + `FrameAtmosphere_Lighting` → `Frame_Atmosphere` rename = **2 件**
- = **計 20 件**、main session で AYA 判断後実施 → 全件「反映済」マーク後本 phase 着手

### §5.3 main session vs 本 phase の関係

| 区分 | 主体 | scope |
|---|---|---|
| AYA 判断仰ぎ (4 件) | main session | AYA 受領 → chapter 10 §1 反映 |
| 修正推奨 20 件 反映 | main session | AYA 判断後の chapter 本体 edit |
| **本 phase (Phase 2d-β prep 実施)** | **本 prep doc 駆動 session** | **上記完了を確認 + Deliverable A/B/C 反映** |

→ 本 phase は main session 完了 **後** に着手、main session と **並行しない** (= memory `feedback_one_step_at_a_time` 準拠、prep 並行起草禁止)。

---

## §6 前提未充足時の handling (= 部分着手禁止)

### §6.1 AYA 判断 4 件のうち Q22-NUM / Q24-S1 が未受領の場合

→ **本 phase 着手不可、prep 段階で停止**:
- Q22-NUM 未確定 = Deliverable B-1 / B-3 / §4.1 / §4.3 の数値が確定不可
- Q24-S1 未確定 = Deliverable A-2 反映先 (06a-prep §2.2 末尾の S1-代替案確定 note) が確定不可
- → 着手判定 = AYA 判断 4 件全完了確認、ない場合は main session に「Q22-NUM / Q24-S1 受領待ち」を申し送り、本 phase 入口で待機

### §6.2 修正推奨 20 件のうち 06a / 06a-prep 関連 (= 第二次査読 §8.1 chapter 06a 4 件) が未反映の場合

→ **本 phase 着手不可、main session 完了待ち**:
- §8.1 chapter 06a 修正 4 件 = §9 (S1) 持越分割 + §4.3 cross-ref §2.2.1→§2.2.2 + §5.1 setter 表 uniform1i 追加 (16→17 method) + §5.5「32→30 entry point」
- これら未反映で本 phase Deliverable A 着手すると、修正対象 file が main session と本 phase で **同時 edit 競合**
- → 着手判定 = main session で 06a 4 件全反映確認 (= git log で commit hash 確認)、ない場合は本 phase 入口で待機

### §6.3 部分着手 (= Deliverable A のみ実施 等) の扱い

- **禁止**: memory `feedback_no_scope_shrink` 準拠、本 phase literal scope = A + B + C の 3 件、A のみ等の縮小着手は scope shrink 違反
- 例外: Deliverable C (= §2.3 vulkanize bug Phase 0 計測対象追加) は前提条件 (§5.1 / §5.2) に**依存しない**ため、AYA 判断 4 件未受領状態でも Deliverable C のみ先行実施は **不可** (= §6 全 deliverable 一括着手の literal scope を守る)

---

## §7 Phase 2d-β 完了条件 + 次 step (= η-29 Phase 0 入口)

### §7.1 Phase 2d-β 完了条件 (= Deliverable A/B/C 全件反映)

- [ ] Deliverable A (= setter 30 entry point cookbook): 06a-prep §2.2 / §2.3.3 / §2.4 / §2.5 内の 5 sub-task (A-1〜A-5) 全件 edit + diff 検証
- [ ] Deliverable B (= 既存 GL state cookbook): 06a-prep §1.1 / §1.2 / 新規 §1.1.1 の 4 sub-task (B-1〜B-4) 全件 edit + diff 検証
- [ ] Deliverable C (= vulkanize bug 計測追加): 06a-prep 新規 §5.5 + §0.3 表 + §6 反映 flow の 5 sub-task (C-1〜C-5) 全件 edit + diff 検証
- [ ] **self-verify**: 06a-prep 全文を Read で再読 → 17 method / 14 method / Q22-NUM 解消反映 / G' 計測追加 の整合確認 (= memory `feedback_self_verify_before_handoff`)
- [ ] **handoff doc 起草**: Phase 2d-β complete handoff doc を起案、本 prep を Source of truth として完了状態を記述
- [ ] **commit**: docs commit (= `feedback_no_auto_commit` 準拠、AYA 明示指示後)

### §7.2 完了後の次 step = η-29 Phase 0 (= 実機計測 phase、`indra/` 改変解禁)

- `design/09-phase-roadmap.md` §3 Phase 0 sub-task 構成 (= 3.2.1 〜 3.2.6) に従い実機計測着手
- Phase 0 entry 1 doc = 本 phase で update 完了の `design/06a-prep-phase0-measurement.md` 1 件のみで駆動可能 state 到達
- memory `feedback_design_phase_no_code_write` **解除点**: η-29 Phase 0 着手時、`indra/` 配下に LL_INFOS hook + CMake patch + frame counter 配線が **本 phase で確定済 spec に従って** 実装される
- 本 phase は **design-phase ↔ implementation-phase 境界 bridge** の役割を果たし完了

---

## §8 範式継承 + 本 phase 適用範式

### §8.1 継承 (Phase 2c / 2d-α / 設計起案 phase から)

- `feedback_design_phase_no_code_write` (= 本 phase は docs 化のみ、`indra/` 改変ゼロ)
- `feedback_handoff_minimal_pre_req_read` (= 必読 3 件 + pinpoint Read で context 圧迫回避)
- `feedback_no_scope_shrink` (= Deliverable A + B + C の 3 件一括 literal scope、部分着手禁止 §6.3)
- `feedback_one_step_at_a_time` (= main session 完了待ち、本 phase と並行しない §5.3)
- `feedback_self_verify_before_handoff` (= 完了条件 §7.1 で self-verify step 明示)
- `feedback_no_auto_commit` (= commit は AYA 明示指示後)
- `feedback_doubt_self_first` (= 前提未充足時 (§6) は「着手して走りながら確認」を抑止、待機を選択)
- `feedback_ubo_migration_one_at_a_time` (= 本 phase 完了後の η-29 Phase 0 → η-30 Phase 1+ で発動)

### §8.2 本 phase 新規適用範式 (= design-phase ↔ implementation-phase bridge)

- **bridge phase 範式 (= 本 phase 起源、Phase 2d-β で確定)**:
  - design-phase で起案された複数 spec doc (= 06a-prep / 04-frame-context / inventory) を **実装 phase entry の単一駆動 doc** に集約する phase を **bridge phase** として明示
  - bridge phase 中は `indra/` 改変ゼロ、deliverable は **既存 spec doc 内の整合反映 + cross-ref 補完 + 入口 1 doc 駆動性確立** のみ
  - bridge phase が完了することで、実装 phase 入口 (= 本件 η-29 Phase 0) で **AYA + Claude が同一 doc を pre-requisite として共有可能** 状態に到達
  - 将来の r41 milestone 内 chapter 移行 (= Phase 1 codegen 入口 / Phase K+1 3 OS 確証入口 / Phase K+5 release 入口) でも同型 bridge phase 起案可能性、本 phase で範式確立

---

## §9 関連 doc / commit / handoff cross-ref

### §9.1 関連 doc (= 本 handoff から参照)

- `design/09-phase-roadmap.md` (= Phase 0 entry 起点、本 phase 完了後の駆動 doc)
- `design/06a-prep-phase0-measurement.md` (= Phase 0 計測 spec、本 phase Deliverable A/B/C 反映先)
- `design/06a-cache-structure-and-setter-redirect.md` (= setter 整備 spec、§0.2 cadence 推定表 cross-ref 元)
- `04-frame-context.md` §1.3 (= descriptor set 5 帯構成、Deliverable B-1 入力)
- `ayastorm-r41-ubo-current-state-inventory.md` §3 / §7 (= 既存 UBO 一覧 + 残課題、Deliverable B / C 入力)
- `audit-past-b-work-vs-design-2026-06-03.md` (= 整合性 100% baseline、Deliverable B-2 入力)
- `design-review-2026-06-03-second-pass.md` §7 / §8 (= AYA 判断 4 + 修正 20、§5 前提条件)

### §9.2 直近 commit (= 2026-06-03 Phase 2d-α 完了 〜 本 prep)

| hash | 種別 | 内容 |
|---|---|---|
| `0587c574da` | feat | Phase 2d-α 初版 5 GLSL file 適用 (= pivot doc §4.1 由来) |
| `9a576884c0` | docs | reference doc 更新 (= binding 10 note + η-28-E/F 範式) |
| `e886fa92c7` | feat | Phase 2d-α Fix (= PBRMix guard 補完 + SpotLight cross-stage UBO 共有) |
| `c7cdaa279a` | docs | Phase 2d-α complete handoff 起草 + Fix 内訳 |
| (本 commit) | docs | Phase 2d-β prep 起草 (= 本 doc) |

### §9.3 関連 handoff (= 過去 → 本 handoff)

- `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-complete.md` (= 直前 phase 完了、§5 残 bug 2 件 + §9 引き継ぎ事項が本 phase 起点)
- `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-alpha-verify-prep.md` (= 直前 phase verify-prep、§3.1 で本 phase scope 起案宣言)
- `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-pivot-to-ubo-design.md` (= 設計 pivot、Phase 2 / 3 構造分離 確定 source)

### §9.4 関連 memory

- `project_ayastorm_r41_vulkan_migration.md` (= r41 milestone active)
- `project_ayastorm_r41_design_principles.md` (= 2 大設計原則)
- `feedback_design_phase_no_code_write` (= 本 phase の規律根幹)
- `feedback_ubo_migration_one_at_a_time` (= 本 phase 完了後の η-29 / η-30 進行ルール)
- `feedback_handoff_minimal_pre_req_read` (= §1 必読 3 件規律)
- `feedback_self_verify_before_handoff` (= §7.1 完了条件)
- `feedback_no_scope_shrink` (= §6.3 部分着手禁止)

---

## §10 本 handoff の制約 (= 次 session 開始時に Claude が忘れがちな点)

- **本 phase は docs 化のみ**: `indra/` 配下を 1 行も触らない (= memory `feedback_design_phase_no_code_write` 継承、Phase 0 hook 実装は η-29 で解禁)
- **着手前提は 2 件**: AYA 判断 4 件完了 (§5.1) + 修正推奨 20 件反映完了 (§5.2)、いずれか未充足なら本 phase 入口で待機 (§6)
- **literal scope は 3 deliverable 一括**: 部分着手禁止 (§6.3)、memory `feedback_no_scope_shrink` 違反となる
- **本 phase の出力は `06a-prep` 内 update のみ**: 新規 doc 起案ではなく既存 spec の整合反映 + cross-ref 補完が deliverable
- **完了後の次 step は η-29 Phase 0**: bridge phase 完了 → implementation-phase 入口、`indra/` 改変解禁点を本 phase 完了が成立
- **commit は AYA 明示指示後**: memory `feedback_no_auto_commit`、本 prep / 完了 handoff の起草自体は context 進行のみで commit しない
- **`feedback_doubt_self_first`**: 前提未充足を「走りながら確認」で乗り越えようとしない、本 phase は **待機を選択する** ことが正解

---

**本 handoff は Phase 2d-β bridge phase 完遂までの永続参照**。完了後は `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-phase2d-beta-complete.md` (= 起案予定) に役割移管。次 phase は η-29 Phase 0 (= 計測 phase、`design/09-phase-roadmap.md` §3) として実装-phase 入口へ遷移。
