# aya_r41_blueprints/ = AYAstorm r41 UBO Codegen **入力 source of truth**

**位置付け**: AYAstorm r41 UBO Codegen toolchain (= `scripts/ubo_codegen/main.py`) の **唯一の入力 source of truth**。Phase 1.A PA-8 (= 2026-06-03) で起案、AYA 指示 #5 で **discard 禁止 = 保護指示** が確定済 (= `design/01-overview.md:146` 「既存 85 GLSL UBO blueprint は **discard しない** (parse error 解消の蓄積を温存)」literal)。

**「parse error 解消の蓄積」literal の真意** (= 2026-06-06 案 X 確定で literal 真意確定):
- 各 .glsl は `#version 450` + 自己完結 1 UBO declaration として **AYAstorm shader runtime prepend chain (= `loadShaderFile()` 経由 `addPermutation` / `#define FRAGMENT_SHADER` / `AYASTORM_CINEMATIC` 等) を一切必要としない最小単位**
- 結果として codegen Python tool は `glslang -E` で **bare preprocess** のみで全 94 file を parse 可能、`MAX_JOINTS_PER_MESH_OBJECT` 等の runtime dynamic #define を emulate する必要なし
- これが「parse error 解消の蓄積」の構造的意味 = runtime prepend chain emulation を **設計時に blueprint dir 内に hardcode した snapshot 群**

**AYAstorm shader runtime path との関係** (= 別 GLSL 並列 build process、設計 doc 04 §2.2 旧 literal 「同じ GLSL 入力」は誤り、2026-06-06 案 X 確定で訂正):
- **blueprint dir (本 dir)** = codegen Python tool 入力 = build-time に `ubo_metadata.inl` 等を generate
- **`class*/` + `cinematic_bd/` 配下 actual shader** = AYAstorm shader runtime compile target = viewer 起動時に `loadShaderFile()` 経由で prepend chain 適用後 glCompileShader / SPIR-V 化
- 両者は **別 GLSL 系統 (= 別 file)** で並列 build process、**同 UBO declaration の二重 source として整合 verify が必要** (= main.py `_verify_block_match` 拡張で blueprint と actual の対称的構造的整合 verify)

---

## §1. 本 dir の現在の役割 (= 案 X 確定後 2026-06-06)

| 軸 | 内容 |
|---|---|
| Codegen 入力対象 | **✅ 唯一の source of truth** (= `AyaUboCodegen.cmake` `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 経由、`scripts/ubo_codegen/main.py --input` の入力 path) |
| source of truth | **✅ 唯一** (= 80 UBO declaration の binding/std140 layout/member は本 dir 内 .glsl で確定、`ubo_metadata.inl` 等 codegen 出力の original source) |
| AYAstorm shader runtime compile target | **× 違う** (= runtime compile は `class*/` + `cinematic_bd/` の actual shader、本 dir 内 .glsl は viewer 起動時 read されない) |
| 編集対象 | **○ 設計時改修可能** (= UBO declaration の binding/std140 layout/member 改修は本 dir 内で実施、AYAstorm runtime compile path = `class*/` + `cinematic_bd/` への同 UBO 同期書換も必要) |
| 削除対象 | **× 禁止** (= AYA 指示 #5 literal「discard しない」整合) |

**現状 file 数**: set0/set1/set2/set3 = 4/2/31/57 = **94 .glsl** (= 80 UBO declaration + Phase 1 起案時の余剰 14 file、後者は将来 inventory cross-ref で確認後 廃止 / consolidation 判定)。

---

## §2. AYAstorm shader runtime path との二重 source 同期 protocol (= 案 X 確定後)

設計 doc 04 §4.4 literal 「同名 UBO 複数 GLSL 宣言の扱い」 = blueprint dir 内 declaration と `class*/` + `cinematic_bd/` 内 actual shader declaration が **同 UBO 同 layout で整合必須**、不整合時は build-time check で abort。

**verify 実装** (= Phase 2.α α-2 commit `b66ec99f72` `_verify_block_match` 拡張、本 README は実装拡張の design intent record):
- main.py 内 `_verify_block_match` を **blueprint と actual の対称的整合 verify** に拡張
- 同名 UBO の set/binding/std140 layout/member 全件一致を build-time check
- 不一致時 `CodegenError` abort = 二重 source 同期断裂を構造的に検知

**手動同期 protocol (= 過渡期)**:
- blueprint dir 内 declaration の set/binding 改修時 = 対応する `class*/` + `cinematic_bd/` 内 declaration も同期書換
- 逆も同様 = actual shader 内 declaration 改修時 = 対応する blueprint dir 内 declaration も同期書換
- `_verify_block_match` 拡張で **build-time に同期断裂を抽出 + abort**、手動同期の漏れを構造的に検知

**Phase 2.L0 sub-session 5 step 2-batch-0-a 7 commit の整合性**:
- sub-session 5 で `class*/` + `cinematic_bd/` 内 actual shader 14 file の set/binding 書換完了 (= 7 UBO 新 binding 反映)
- 対応する blueprint dir 内 7 UBO 14 file は **未書換** = **二重 source 同期断裂** = 案 X 採用後 blueprint dir 側同期書換が必要 (= Phase 2.α α-3 phase E、本 README 内 §2 protocol 適用)

---

## §3. Phase 履歴 (= 本 dir 成立経緯 + 案変遷)

| sub-step | 期間 | 成果 |
|---|---|---|
| Phase 1.A PA-1〜PA-6 | 2026-05-31〜2026-06-02 | 85 UBO 設計判断 + std140 layout 確定 + set/binding 割付 |
| Phase 1.A PA-7 | 2026-06-02 | Codegen toolchain (`scripts/ubo_codegen/`) 起案 + 本 dir を Codegen 入力 source として確立 |
| Phase 1.A PA-8 | 2026-06-03 | 85 UBO blueprint 配置 (= 本 dir set0-3 = 4/2/31/57 ファイル) + parse error 解消蓄積 (= AYAstorm runtime prepend chain 不要の自己完結 GLSL) |
| Phase 2.L0 | 2026-06-03〜 | 80 UBO mapping + set/binding 再配 + `class*/` + `cinematic_bd/` 配下 actual shader 改修開始 (= sub-session 5 step 2-batch-0-a 7 commit 完了 = actual 14 file) |
| Phase 2.α 起案 | 2026-06-06 | 二重 source 構造の同期断裂発覚 (= sub-session 5 で actual 14 file 改修済 + blueprint dir 未改修) → Phase 2.L0 freeze + Phase 2.α 独立起案 |
| Phase 2.α 案 Y (= 撤回) | 2026-06-06 | 案 Y = blueprint 完全廃止 → AYA 指示 #5 違反確定で撤回 |
| Phase 2.α 案 Z (= 撤回) | 2026-06-06 | 案 Z = `class*/` + `cinematic_bd/` 入力切替 + blueprint reference 降格 → improvement 1/2 commit 後 codegen 単独走行 verify で parse error 発覚 (= `MAX_JOINTS_PER_MESH_OBJECT` unresolved) → 設計 doc literal 整合だが実装不整合確定で撤回 |
| Phase 2.α 案 Z' (= 撤回) | 2026-06-06 | 案 Z' = 案 Z + C++ runtime emulation 層追加 (= dump file + `--defines-file` + cmake DEPENDS) → improvement 1.5.b/c commit 後 codegen 単独走行 verify で parse error 第 2 階層発覚 (= `#version` + `AYASTORM_CINEMATIC` 等 prepend chain 全件 emulate scope 超過) → 構造的破綻確定で撤回 |
| **Phase 2.α 案 X (= 確定、本 README 起案点)** | **2026-06-06** | **blueprint dir = codegen 入力 source of truth、`class*/` + `cinematic_bd/` = runtime compile target、別 GLSL 並列 build process + 二重 source 同期 protocol formal化** |

詳細は `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.10 (= 案 X 確定 source of truth、Phase 2.α α-3 phase C で起案) 参照。

---

## §4. 参照優先順位 (= 設計議論時)

1. **本 dir `aya_r41_blueprints/<set>/<name>.glsl`** (= **UBO declaration source of truth**、codegen 入力)
2. **`design/ubo/<UBO_NAME>.md`** (= per-UBO doc、本 dir 内 declaration の説明 doc)
3. **`indra/newview/app_settings/shaders/class*/` + `cinematic_bd/` 配下 actual shader** (= runtime compile target、本 dir と同 UBO 同 layout で整合維持必須)

**1 と 3 で食い違いがある場合は build-time `_verify_block_match` で abort**。手動同期 protocol で両者を同期維持。

---

## §5. 編集規律

- 本 dir 配下 file の **編集 (= UBO declaration 改修)** は可能、ただし対応する `class*/` + `cinematic_bd/` 内 declaration も **同期書換必須** (= §2 protocol)
- 本 dir 配下 file の **削除** は禁止 (= AYA 指示 #5 違反、削除 PR は reject)
- 本 dir 配下 file の **新規追加** = UBO 新規追加時、blueprint dir 内に 1 file 1 UBO で起案後、`class*/` + `cinematic_bd/` 内に対応 declaration を追加 = 順序逆も可、ただし build-time `_verify_block_match` で整合 confirm
- 本 dir を `AYA_UBO_CODEGEN_BLUEPRINT_DIR` (= AyaUboCodegen.cmake) 以外で参照する PR / commit は **reject** (= source of truth 単一化、二重 source 構造の正当な共存以外の参照は構造混乱の元)

---

## §A. 関連 doc

| 種別 | path |
|---|---|
| AYA 指示 #5 (= discard しない literal) | `docs/specs/ayastorm-r41-gl-removal/design/01-overview.md:146` |
| blueprint 再利用方針 | `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md:967` |
| 同名 UBO 複数 GLSL 整合 verify 設計 | `docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md` §4.4 (= 本 dir + actual shader 二重 source 整合) |
| Codegen 入力 source 確定 (= 案 X) | `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.10 |
| Phase 2.α 案 X 確定 entry | `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-3-cmake-blueprint-readme-propagation.md` (= 案 X 用に再起案予定 phase C) |
| CMake 配線 | `indra/cmake/AyaUboCodegen.cmake` (= `AYA_UBO_CODEGEN_BLUEPRINT_DIR` = 本 dir、復元状態) |
| Codegen toolchain | `scripts/ubo_codegen/` (= main.py + glsl_parser.py + std140.py + perfect_hash.py + spirv_reflect.py 等、α-2 commit `b66ec99f72` で `_verify_block_match` 実装済) |
