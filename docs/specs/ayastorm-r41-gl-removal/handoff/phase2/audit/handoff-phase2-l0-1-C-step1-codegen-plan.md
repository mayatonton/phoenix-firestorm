# handoff = r41 Phase 2.L0 sub-session 3 = L0-1.C 実装 step 1 = codegen pipeline 修正起案

**起案日**: 2026-06-06
**位置付け**: Phase 2.L0 sub-session 3 step 1 (= codegen pipeline 修正起案) 出力 doc。`scripts/ubo_codegen/` + `build-linux-x86_64/codegen/ubo/` 配下のみ cold read、`indra/` 改変ゼロ。
**起案契機**: entry handoff `handoff-phase2-l0-1-C-entry.md` §2.2.1 step 1 scope + §7 着手内容受領。
**起案規律**:
- memory `feedback_admit_unknown` 適用 (= 推論禁止、不明明示)
- memory `feedback_doubt_self_first` 適用 (= 仮説の前に literal 取得)
- memory `feedback_ubo_migration_one_at_a_time` 適用 (= 大塊一括 reject 評価)
- memory `feedback_self_verify_before_handoff` 適用 (= AYA literal 確認前 self-trace)
- `indra/` 改変ゼロ (= step 1 規律)

---

## §1. step 1 着手目的

L0-1 protocol-C 採用案 (= AYA literal「(i) 新規 binding allocation」) 実装のため、codegen pipeline 修正起案 + Legacy UBO PerProgram cluster 現状値 grep + 80 slot 割当 plan 策定 + AYA literal 確認 candidate 提示。

---

## §2. codegen pipeline 構造 (= cold read 結果)

### §2.1 pipeline 全体 (= `scripts/ubo_codegen/`)

| file | 行数 | 役割 |
|---|---|---|
| `main.py` | 438 | entry point、parse → layout → reflection verify → 6-file emit pipeline |
| `glsl_parser.py` | 505 | mini-parser (= GLSL `layout(set=N, binding=M) uniform <NAME> { ... }` decl 抽出) |
| `std140.py` | 210 | std140 offset/size 計算 (= 256B device alignment 込) |
| `spirv_reflect.py` | 305 | SPIR-V double-verification (`spirv-cross` 経由) |
| `perfect_hash.py` | 839 | CHD perfect hash + per-block layout emit + ubo_metadata.inl 出力 |
| `glslang_preproc.py` | 99 | `glslangValidator` preprocess wrapper |
| `build_cache.py` | 379 | incremental cache (B4a hash + mtime) |
| `codegen_error.py` | 51 | CodegenError 例外 |
| 合計 | 2826 | |

### §2.2 set/binding 値の codegen 経路 (= 重要 finding)

**codegen は GLSL `layout(set=N, binding=M)` 宣言を直読みするのみ、自動 allocation logic 無し**。

| 経路 | source literal | logic |
|---|---|---|
| GLSL parse | `glsl_parser.py:282` `_parse_layout_qual` → `UboBlockDecl.layout_qual: dict` | GLSL `layout(...)` qualifier を dict 抽出 (= `{"std140": True, "set": N, "binding": M}`) |
| BlockSpec 構築 | `main.py:211-221` `_ubo_to_block_spec` | `descriptor_set = int(ubo.layout_qual.get("set", 0))`、`binding = int(ubo.layout_qual.get("binding", 0))` (= 直読み、override 無し) |
| subset 計算 | `main.py:200-208` `_derive_subset` | `set==1 and binding>=40 → 1`、`他 → 0` (= **PC-7α' 通電済**) |
| cadence_tag 計算 | `main.py:161-168` `_derive_cadence` | block_name prefix/suffix matching (= `UBO_Legacy` suffix → `CADENCE_PER_PROGRAM`) |
| metadata emit | `perfect_hash.py:686-712` `emit_metadata_inl` | `BlockSpec.descriptor_set` / `binding` / `subset` / `cadence_tag` をそのまま `g_block_metadata[]` literal に出力 |

⇒ **codegen pipeline 自体に set/binding 自動 allocation logic は無い**。`ubo_metadata.inl` 出力の set/binding 値 = shader 側 GLSL の `layout(set=N, binding=M)` 宣言値そのまま。

### §2.3 subset 列の dispatch 経路使用 (= verify 3 件 #3 解消)

**`meta->subset` は `llvkloader.cpp:5348-5385` `registerProgramUbo` 内で使用済**:

```cpp
// indra/llrender/llvkloader.cpp:5368-5385 literal
if (meta->subset == 0)
{
    set_array = sProgramUboSetA;  // set=1a
    dst_binding = src_binding;
}
else if (meta->subset == 1)
{
    set_array = sProgramUboSetB;  // set=1b
    dst_binding = src_binding - V3A_PROGRAM_SET_A_BINDINGS;
}
else
{
    LL_WARNS_ONCE("Vulkan") << "PC-7α' (e) registerProgramUbo: meta.subset " << meta->subset
                            << " out of V3a {0:1a, 1:1b} range, skip vkUpdateDescriptorSets";
    set_array = nullptr;
}
```

⇒ **subset 列は既に PC-7α' で通電済** (= dispatch 経路 active)、ただし現状 80 件 PerProgram cluster 全件 subset=0 ゆえ subset=1 (set=1b) 経路は実質未通電、Legacy UBO 再配置 → 初めて active 化。

---

## §3. Legacy UBO PerProgram cluster 現状値 (= grep 結果)

### §3.1 cadence_tag 別 breakdown

| cadence_tag | 値 | 件数 |
|---|---|---|
| 0 PerFrame | - | 3 |
| **1 PerProgram** | - | **80** |
| 2 PerDraw | - | 7 |
| 3 PerAsset | - | 2 |
| 4 PerSkin | - | 1 |
| 5 Singleton | - | 1 |
| **合計** | | **94** |

⇒ **PerProgram cluster = 80 件** (= sub-session 2 doc 88 件 claim **訂正**、+8 件は不存在)。

### §3.2 現状 set/binding 配置 80 件

| set | binding 範囲 | 件数 | UBO 例 | 状態 |
|---|---|---|---|---|
| **set=1** | binding=0 | 2 | MaterialUBO + MaterialUBO_Legacy | **排他衝突** (= AYA review §2.5 影響 1 件目) |
| **set=2** | binding=2..25 | 24 | PerProgramUBO_* (= AlphaParams, BlrLightF, CofF, ColorGrading, ...) | **実 pipeline V3A_DRAW_SET_BINDINGS=4 と不整合 (= 4 slot のみ確保、24 件 overflow)** |
| **set=3** | binding=0..62 | 54 | *_Legacy 大多数 (= AtmoExtra, SkyV, SkyF, AOUtilParam, ...) | **実 pipeline V3A_ASSET_SET_BINDINGS=3 と不整合 (= 3 slot のみ Asset/Skin 専有、54 件 overflow)** |
| 合計 | | 80 | | |

### §3.3 実 pipeline layout 容量 (= `llvkloader.cpp:858-868` literal)

```cpp
constexpr U32 V3A_FRAME_SET_BINDINGS     = 4;   // set=0
constexpr U32 V3A_PROGRAM_SET_A_BINDINGS = 40;  // set=1a
constexpr U32 V3A_PROGRAM_SET_B_BINDINGS = 40;  // set=1b
constexpr U32 V3A_DRAW_SET_BINDINGS      = 4;   // set=2
constexpr U32 V3A_ASSET_SET_BINDINGS     = 3;   // set=3 (Asset_GLTFNodes + Asset_GLTFMaterials + Skin_GLTFJoints 専有)
```

⇒ PerProgram cluster 80 件の **目的地** = set=1a (40) + set=1b (40) = **80 slot 丁度収まる、overflow 0 件**。

---

## §4. L0-1 protocol-C 採用案 (= AYA literal「(i) 新規 binding allocation」) の具体 plan

### §4.1 修正範囲確定

- **80 件全件**を `layout(set=N, binding=M)` 宣言を **set=1a (binding=0..39) or set=1b (binding=40..79)** に再配置
- 配置完了で `_derive_subset` 自動判定 = binding<40 → subset=0 (set=1a) / binding>=40 → subset=1 (set=1b)

### §4.2 修正 site = 「shader 側 GLSL」 vs 「codegen pipeline」

#### §4.2.1 物理的整合の必須要件 (= 二者択一不可)

GLSL `layout(set=N, binding=M)` 宣言は SPIR-V binary に焼き込まれる (= Vulkan 仕様)。host C++ pipeline layout (= `sProgramUboLayout` etc.) は SPIR-V binary 内の set/binding と整合する必要 → **shader 側 GLSL の宣言が物理的 single source of truth**。codegen pipeline が `ubo_metadata.inl` で別 set/binding 値を出力しても、SPIR-V binary 内の set/binding 宣言と乖離したら Vulkan validation layer error + dispatch 失敗。

⇒ **shader 側 GLSL `layout(set=N, binding=M)` 宣言修正は necessary**。codegen pipeline は GLSL 直読みゆえ追従。

#### §4.2.2 修正方式の選択肢 (= AYA literal 確認 candidate 1)

shader 側 GLSL 80 件 `layout(set=N, binding=M)` 宣言値書換は必須、ただし書換 site (= 手動 vs 自動) は選択肢あり:

| 案 | 内容 | pros | cons |
|---|---|---|---|
| **(i)** | **shader 側 GLSL 80 件手動編集** | minimal tooling、SPIR-V binary に正しい set/binding 焼き込み、Vulkan validation 完全整合、shader 直読みで分かりやすい | 80 件 file 編集 = ミスリスク、`indra/newview/app_settings/shaders/` 配下大量変更 |
| **(ii)** | **codegen pipeline に shader file 書換機能追加** = UBO 名 → set=1a/1b binding mapping table を新規 file (= 例 `scripts/ubo_codegen/ubo_binding_map.yaml`) で集中管理、codegen が shader file 内 `layout(set=N, binding=M)` を自動書換 | mapping table 集中管理、80 件一括書換可能、UBO 増減時 mapping table のみ更新 | codegen 責任範囲拡大 (= shader source 改変 = 副作用)、shader file 改変が automated → review しにくい、`indra/newview/` 配下 shader file commit phase での原本性低下 |
| **(iii)** | **shader_loader 段階で preprocessor 経由 override** = shader source 不変、build phase で UBO 名 → set/binding mapping を inject | shader source 直接改変なし、L0-3 (per-shader UBO block 拡大) protocol と統合可能 | shader_loader 改修要、L0-3 protocol との scope 混在、Phase 2 範囲外 |

**Claude 推奨**: **案 (i) 手動編集**。

理由:
- 案 (ii) の codegen 駆動 shader rewrite は **副作用が大きい** (= shader source = ユーザー可視 file が automated edit 対象になる、L0-2 LLStaticHashedString redirect 等の後続 protocol で「shader source は source of truth」前提が崩れる)
- 案 (iii) は L0-3 protocol と scope 混在、Phase 2 範囲外で別 protocol 統合判断要
- 案 (i) は 80 件編集だが、shader file 内 `layout(set=N, binding=M)` 1 行書換 = mechanical で誤りリスク低 (= grep/sed 補助可能)、commit phase で 1 file 1 review 可能

**AYA literal 確認要請**: 案 (i) 採用で OK か?

### §4.3 80 slot 割当方針 (= AYA literal 確認 candidate 2)

80 件を set=1a (binding=0..39) + set=1b (binding=40..79) にどう並べるか:

| Plan | 並べ方 | pros | cons |
|---|---|---|---|
| **Plan-A** | **UBO 名 alphabetical sort で 0..79** | deterministic、自動生成可能、UBO 増減時 regenerate しやすい | 関連 UBO 隣接せず、debug 不便 |
| **Plan-B** | **機能 group 隣接** (= shader pass / 機能別 group) | 関連 UBO 連続、cache 局所性 | group 定義に主観混入、AYA literal 判断要 |
| **Plan-C** | **shader stage 別** (V/F suffix) | V (vertex) → set=1a、F (fragment) → set=1b 分離 | V/F 分類が UBO 名から推測 (= 慣習的判断)、混在 UBO 困る |
| **Plan-D** | **現状 set/binding 値からの最小移動** = set=3 binding=0..62 (54 件) → set=1b binding=0..53、set=2 binding=2..25 (24 件) → set=1a binding=2..25、set=1 binding=0 排他 2 件 → set=1a binding=0..1 | 修正差分 minimal、現状値からの mental mapping 容易 | 並びが偶然的 (= 現状の偶然的配置を維持) |

**Claude 推奨**: **Plan-A alphabetical sort**。

理由:
- deterministic = codegen 再生成で同一結果保証 (= UBO 増減時の reproducibility)
- alphabetical = 主観排除、AYA literal 判断負荷最小
- 関連 UBO 隣接 (= cache 局所性) の効果は **PerProgram cadence では register-once + bind-many 動作ゆえ runtime cache hit 影響極小** (= shader bind 時 1 回 vkUpdateDescriptorSets で済む、frame 毎の cache miss なし)
- Plan-D (= 最小差分) は **24 件 set=2 → set=1a + 54 件 set=3 → set=1b + 2 件 set=1 → set=1a の mixing で実質 80 件全件移動と等価**、deterministic 利得無し

**AYA literal 確認要請**: Plan-A 採用で OK か? 他 Plan に強い preference あるか?

### §4.4 修正 batch 単位 (= AYA literal 確認 candidate 3)

memory `feedback_ubo_migration_one_at_a_time` (= 「1 UBO ずつ、cold launch 検証挟む、大塊バッチ禁止」) との整合判断:

| batch 案 | 件数 / batch | commit 数 | cold launch 回数 | pros | cons |
|---|---|---|---|---|---|
| **batch-1** | 1 件 / batch | 80 | 80 | memory 文言厳格遵守 | 非実用的 (= 1 commit/cold launch あたり 10 分とすると 13 時間以上) |
| **batch-10** | 10 件 / batch | 8 | 8 | バランス、batch あたり 30 分目処 | UBO group 境界調整要 |
| **batch-80** | 80 件 / 1 batch | 1 | 1 | minimal commit、minimal cold launch | 一括 batch、memory 文言違反候補、ロールバック粒度大 |
| **batch-stage** | shader stage / batch (= V/F/Compute) | 3-4 | 3-4 | stage 単位で意味 group、debug 容易 | stage 内多数件 (= F = 50 件超) で粒度大 |

**Claude 推奨**: **batch-stage** (= shader stage 別 batch、3-4 batch、各 batch 内全件は alphabetical sort)。

理由:
- `feedback_ubo_migration_one_at_a_time` は **新規 UBO 化 + host C++ redirect 層整備** の文脈、本 step 2 は **既通電 UBO の layout 宣言修正のみ** (= 動作 logic 変更なし、register/write 経路無改変) ゆえ batch 化のリスクは低い
- ただし「memory 文言違反候補」を完全回避する batch-1 は非実用的
- shader stage 別 = 意味 group 明確、batch 内全件 cold launch で同 stage の shader compile + validation を一度に確認可能
- batch-10 は機械的 group 化で stage 混在しうる → debug 不便

**AYA literal 確認要請**: batch-stage 採用で OK か? batch-1 (= memory 文言厳格遵守) を強く推奨するか?

### §4.5 overflow strategy (= 残 0 件)

**現状**: 80 件 = 80 slot 丁度収まる、overflow 0 件。

**将来 UBO 追加時の overflow strategy** (= 参考、本 step 1 範囲外):

| strategy | 内容 | 原則 整合 |
|---|---|---|
| (a) set=1a/1b 容量拡大 | `V3A_PROGRAM_SET_A_BINDINGS=40 → 48`、`V3A_PROGRAM_SET_B_BINDINGS=40 → 48` | 原則 OS-2 (= descriptor set 数 5 維持) ✅、追加 binding は SPIR-V binary 内 layout 宣言と 1:1 整合要 |
| (b) set=4 新規追加 | 新規 descriptor set 確保 | 原則 OS-2 違反候補 (= descriptor set 数 5 → 6 の場合)、要 verify (= 現状 descriptor set 5 か?) |
| (c) UBO 統合 | cadence 一致 UBO 同士を block merge | data model 影響大、L0-4 cadence 再評価 protocol と統合判断要 |

**Claude 推奨**: **将来時に再判断、本 step 1 範囲外**。

**AYA literal 確認要請**: 将来 UBO 追加時の overflow strategy を本 step 1 内で確定するか、将来時に再判断するか?

---

## §5. step 1 出力サマリ

### §5.1 確定事項

| # | 確定内容 |
|---|---|
| 1 | codegen pipeline は GLSL `layout(set=N, binding=M)` 直読み、自動 allocation logic 無し (= `main.py:211-221` `_ubo_to_block_spec`) |
| 2 | subset 列は PC-7α' で dispatch 経路通電済 (= `llvkloader.cpp:5368-5385` `registerProgramUbo`)、sub-session 2 verify 3 件 #3 解消 |
| 3 | PerProgram cluster = **80 件** (= sub-session 2 doc 88 件 claim 訂正) |
| 4 | 80 件 = set=1a (40) + set=1b (40) = **80 slot 丁度収まる、overflow 0 件** |
| 5 | shader 側 GLSL `layout(set=N, binding=M)` 修正は **物理的必須** (= SPIR-V binary single source of truth)、codegen pipeline 単独修正では Vulkan validation 整合不能 |

### §5.2 AYA literal 確認 candidate 4 件

| # | 確認内容 | Claude 推奨 |
|---|---|---|
| 1 | shader 側 GLSL 修正方式 = (i) 手動編集 / (ii) codegen 駆動 rewrite / (iii) shader_loader override | **(i) 手動編集** |
| 2 | 80 slot 割当方針 = Plan-A alphabetical / Plan-B 機能 group / Plan-C V-F stage / Plan-D 最小移動 | **Plan-A alphabetical** |
| 3 | 修正 batch 単位 = batch-1 (memory 厳格) / batch-10 / batch-80 / batch-stage | **batch-stage** |
| 4 | overflow strategy = 本 step 1 内確定 / 将来時再判断 | **将来時再判断** |

### §5.3 step 2 着手条件 (= 本 step 1 Exit 条件)

| # | Exit 条件 | 状態 |
|---|---|---|
| 1 | codegen pipeline cold read 完了 | ✅ §2 |
| 2 | Legacy UBO PerProgram cluster 現状 set/binding 値 grep 完了 | ✅ §3 |
| 3 | 実 pipeline layout 容量 verify (= 80 slot 丁度) | ✅ §3.3 |
| 4 | subset 列 dispatch 経路 verify (= verify 3 件 #3 解消) | ✅ §2.3 |
| 5 | 80 slot 割当 plan + overflow strategy 起案 | ✅ §4 |
| 6 | step 1 出力 doc 起案 | ✅ 本 doc |
| 7 | AYA literal 確認受領 | ⏳ AYA literal 待ち |

---

## §6. 手戻り protocol (= entry handoff §5 cross-ref)

- **step 1 内手戻り** = AYA literal 確認 1-4 件で「推奨案 reject」受領 → 採用案で §4 再起案
- **step 1 → sub-session 2 戻り** = AYA literal 「protocol-C 採用案変更」指示 → sub-session 2 doc §6 確認 1 再起案
- **step 1 → sub-session 1 戻り** = AYA literal 「L0-1 protocol 自体再考」指示 → sub-session 1 戻り

---

## §7. AYA literal 確認 (= 本 step 1 Exit 条件 7)

**確認要請**: §5.2 AYA literal 確認 candidate 4 件への回答お願いします:

1. shader 側 GLSL 修正方式 = **(i) 手動編集** で OK か? 他案採用するか?
2. 80 slot 割当方針 = **Plan-A alphabetical** で OK か? 他 Plan 採用するか?
3. 修正 batch 単位 = **batch-stage** で OK か? batch-1 (memory 厳格) を強く推奨するか?
4. overflow strategy = **将来時再判断** で OK か? 本 step 1 内確定するか?

**回答受領後**:
- 全 4 件 OK → step 2 (= shader 側 GLSL 80 件書換 batch-stage 着手) 着手
- 一部 reject → §4 該当 candidate 再起案、step 1 内で再 cycle
- 全 reject → sub-session 2 戻り (= protocol-C 採用案変更) or sub-session 1 戻り (= L0-1 protocol 再考)

---

## §A. 関連 commit + doc

| 種別 | 内容 |
|---|---|
| 関連 doc (本 step 1 entry) | `handoff/phase2/audit/handoff-phase2-l0-1-C-entry.md` (= commit `0af8ac4bdb`) |
| 関連 doc (sub-session 2 出力) | `handoff/phase2/audit/handoff-phase2-l0-1-B-dispatch-trace.md` (= commit `0af8ac4bdb`、§5 protocol 整合判定) |
| 関連 doc (sub-session 1 出力) | `handoff/phase2/audit/handoff-phase2-l0-uncertainty-audit.md` (= commit `cecb55ceb1`) |
| 関連 doc (設計) | `design/ubo/WORK_ORDER.md` §2.1 (= L0-1 protocol-A/B/C/D 詳細) |
| 関連 source | `scripts/ubo_codegen/main.py:161-221` (= `_derive_cadence` / `_derive_subset` / `_ubo_to_block_spec`) |
| 関連 source | `scripts/ubo_codegen/perfect_hash.py:95-108` (= `BlockSpec` struct) + `:686-712` (= `emit_metadata_inl`) |
| 関連 source | `build-linux-x86_64/codegen/ubo/ubo_metadata.inl:25-120` (= literal 94 件 g_block_metadata) |
| 関連 source | `indra/llrender/llvkloader.cpp:858-868` (= V3A_*_BINDINGS literal) + `:5348-5385` (= subset 経路 dispatch) |
| 関連 memory | `project_r41_phase1b_vulkan_host_gate` / `project_r41_phase2_4_principles` / `project_r41_design_principles` / `feedback_ubo_migration_one_at_a_time` / `feedback_admit_unknown` / `feedback_self_verify_before_handoff` |
| 本 doc | step 1 出力 doc、commit 候補 (= AYA literal 確認受領後) |

---

## §B. 起案規律 (= memory 適用結果)

- `indra/` 改変ゼロ ✅ (= step 1 規律遵守、本 doc 内 source literal 引用のみ)
- 推論禁止、不明明示 ✅ (= §4.5 overflow strategy 将来時再判断 = 現時点不明部分明示)
- AYA literal 確認 candidate 全件 (= §5.2 4 件) 省略せず提示 ✅
- 推奨案明示 + 採用根拠記載 ✅ (= §4.2.2 / §4.3 / §4.4 / §4.5 各 Claude 推奨)
- 大塊一括 default reject 評価 ✅ (= §4.4 batch-1 案併記 + memory 文言整合判断)
- 視覚 regression ゼロ死守 = step 2 以降で cold launch validation 担保 (= 本 step 1 内では起案のみ)

---

## §C. 次 step 着手契機

**step 2 着手契機** = AYA literal 確認 4 件全件 OK 受領 (= §7 全 4 件)。

**step 2 scope**:
- shader 側 GLSL Legacy UBO 80 件 `layout(set=N, binding=M)` 書換 (= batch-stage 想定)
- batch 別 cold launch validation (= Linux validation layer warnings 0 件 + AYA live verify)
- step 2 出力 doc = `handoff-phase2-l0-1-C-step2-shader-rewrite.md` 起案
- step 3 着手承認 or step 2 内 batch 再起案
