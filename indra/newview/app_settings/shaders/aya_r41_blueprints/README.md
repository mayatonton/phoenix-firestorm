# aya_r41_blueprints/ = AYAstorm r41 UBO 設計時参考資料 (= reference 降格、編集禁止)

**位置付け**: Phase 1 (= 2026-05-31〜2026-06-03) で起案した **85 GLSL UBO blueprint 群の履歴 snapshot**。Phase 2 以降の **source of truth ではなく**、設計時参考資料として **物理保持** する dir。

**discard しない根拠** (= 編集禁止 / 削除禁止):
- `docs/specs/ayastorm-r41-gl-removal/design/01-overview.md:146` AYA 指示 #5 = 「**既存 85 GLSL UBO blueprint は discard しない** (parse error 解消の蓄積を温存)」 literal
- `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md:967` = 「blueprint は **discard せず再利用** (chapter 01 §5 #5)」

---

## §1. 本 dir の現在の役割 (= Phase 2.α 以降)

| 軸 | 内容 |
|---|---|
| Codegen 入力対象 | **× 対象外** (= `AyaUboCodegen.cmake` SHADER_SOURCE_DIRS 指定外、`scripts/ubo_codegen/` は本 dir を読まない) |
| source of truth | **× 違う** (= Phase 2 以降の UBO declaration source of truth は `class*/` + `cinematic_bd/` 配下 actual shader file 群) |
| 編集対象 | **× 禁止** (= 設計時 snapshot として固定、AYA 指示 #5 discard しない literal 整合) |
| 削除対象 | **× 禁止** (= 同上、AYA 指示 #5 literal 整合) |
| 参照用途 | **○ 設計時参考** (= Phase 1 で確立した std140 layout / binding / member 構造の history reference、設計議論時の照合に使用) |

**現状 file 数**: set0/set1/set2/set3 = 4/2/31/57 = **94 .glsl** (= AYA 指示 #5 literal「85 GLSL UBO blueprint」は設計時 declaration 数 = inventory §6.x 集約後 count、physical file 数 94 はその superset)。

---

## §2. Codegen 入力 source の正史 (= Phase 2.α 案 Z 確定、2026-06-06)

Phase 2.α α-3 (= 案 Z 確定 commit `64122994c1` + α-2 main.py commit `b66ec99f72` + α-3 CMake 改修) で Codegen 入力 source を **設計時想定 (= `design/08-build-codegen-pipeline.md:72-74/96` literal) に整合修復**:

| Codegen 入力 source | 位置 | 役割 |
|---|---|---|
| `indra/newview/app_settings/shaders/class1/` | actual shader | UBO declaration source of truth (低品質 path) |
| `indra/newview/app_settings/shaders/class2/` | actual shader | UBO declaration source of truth (中品質 path) |
| `indra/newview/app_settings/shaders/class3/` | actual shader | UBO declaration source of truth (高品質 path) |
| `indra/newview/app_settings/shaders/cinematic_bd/` | actual shader | UBO declaration source of truth (cinematic_bd 上書き path、class*/ と同 layout 整合性 verify 対象) |
| **`indra/newview/app_settings/shaders/aya_r41_blueprints/`** | **本 dir** | **× Codegen 入力対象外 (= reference 降格、設計時参考のみ)** |

同名 UBO が複数 file (= class*/ + cinematic_bd/) で再宣言されている場合は `scripts/ubo_codegen/main.py` の `_verify_block_match` (= α-2 commit `b66ec99f72`) が set/binding + subset/cadence + member 全件 layout 一致を構造的に verify し、不一致時は `CodegenError` で abort、整合時は cinematic_bd/ 上書き path として PASS する。

---

## §3. Phase 1 履歴 (= 本 dir 成立経緯)

| sub-step | 期間 | 成果 |
|---|---|---|
| Phase 1.A PA-1〜PA-6 | 2026-05-31〜2026-06-02 | 85 UBO 設計判断 + std140 layout 確定 + set/binding 割付 |
| Phase 1.A PA-7 | 2026-06-02 | Codegen toolchain (`scripts/ubo_codegen/`) 起案 + 本 dir を Codegen 入力 source として確立 |
| Phase 1.A PA-8 | 2026-06-03 | 85 UBO blueprint 配置 (= 本 dir set0-3 = 4/2/31/57 ファイル) + parse error 解消蓄積 |
| Phase 2.L0 | 2026-06-03〜 | 80 UBO mapping + set/binding 再配 + class*/ + cinematic_bd/ 配下 actual shader 改修開始 (= sub-session 5 step 2-batch-0-a 7 commit 完了) |
| Phase 2.α | 2026-06-06 | 案 Y (= blueprint 完全廃止) 撤回 → 案 Z (= 設計 doc 整合修復) 確定 = blueprint dir **reference 降格保持** + Codegen 入力を `class*/` + `cinematic_bd/` に切替 (= 本 README 起案点) |

詳細は `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D 参照。

---

## §4. 参照優先順位 (= 設計議論時)

1. **`design/ubo/<UBO_NAME>.md`** (= per-UBO doc、Phase 2 以降の確定 layout source of truth)
2. **`indra/newview/app_settings/shaders/class*/` + `cinematic_bd/` 配下 actual shader** (= Codegen 入力 source、build 時 layout source of truth)
3. **本 dir `aya_r41_blueprints/<set>/<name>.glsl`** (= Phase 1 設計時参考、history reference)

**1 と 2 で食い違いがある場合は 2 が正**。本 dir (3) は Phase 1 時点の判断履歴であり、Phase 2 以降の変更を反映していない可能性がある。

---

## §5. 編集禁止の運用

- 本 dir 配下 file を編集する PR / commit は **reject**
- 本 dir 配下 file を Codegen 入力に再追加する PR / commit は **reject** (= AyaUboCodegen.cmake `AYA_UBO_CODEGEN_SHADER_SOURCE_DIRS` に本 dir を加えない、設計 doc 整合性違反)
- 本 dir を削除する PR / commit は **reject** (= AYA 指示 #5 違反)
- Phase 1 の追加調査で本 dir に新規 file を作成したい場合は事前に design doc 起案 + AYA literal 承認後実施

---

## §A. 関連 doc

| 種別 | path |
|---|---|
| AYA 指示 #5 (= discard しない literal) | `docs/specs/ayastorm-r41-gl-removal/design/01-overview.md:146` |
| blueprint 再利用方針 | `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md:967` |
| Codegen 入力 source 確定 (= 案 Z) | `docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md:72-74` |
| Phase 2.α 案 Z source of truth | `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D |
| Phase 2.α α-3 entry handoff | `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-3-cmake-blueprint-readme-propagation.md` |
| CMake 配線 (= Codegen 入力切替) | `indra/cmake/AyaUboCodegen.cmake` |
| Codegen toolchain | `scripts/ubo_codegen/` |
