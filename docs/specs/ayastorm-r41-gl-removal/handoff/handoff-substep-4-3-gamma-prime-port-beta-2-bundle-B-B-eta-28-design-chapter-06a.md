# sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-28 handoff: 設計 chapter 06a + 06a-prep 起案 + design-phase 規律確立 + ルール境界明確化

**完了日**: 2026-06-03 (= 3 sub-session 連続更新、本 doc は 06a 起案 session + 06a-prep 起案 session + design-phase ルール境界明確化 session の合成 state)
**位置付け**: 前 handoff (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-28-design-chapter-04-05.md`、chapter 04-05 起案完了) を受けて、**chapter 06 を 06a/06b/06c の 3 sub-chapter に分割 + 06a 起案 + Phase 0 (D)(H1a) 計測完了** (= 第 1 sub-session) + **AYA 指示 (= 設計起案中に program 改変提案は誤り、資料化すべき) で `06a-prep-phase0-measurement.md` を独立 doc 起案 + design-phase 規律 (memory `feedback_design_phase_no_code_write`) 確立** (= 第 2 sub-session) + **AYA 二次指摘 (= spec doc 内の file 言及は削除不要、ルールは program 実改変のみ禁止) を反映して 06a §7 復元 + memory にルール境界 section 追加** (= 第 3 sub-session、本 session 最終) した state。次 session は chapter 06b 起案へ進む。

---

## §0 本 session で完了した作業

### §0.1 議論先行 (= chapter 06 起案方針確定)

| 項目 | 確定内容 |
|---|---|
| chapter 06 分割 | **3 sub-chapter 方式 (06a / 06b / 06c)** を採用 — 単一 chapter では context 1 session 超過の懸念、scope 軸分離で 1 chapter ≈ 1 session 規律維持 |
| 06a scope | cache 構造 + 16 method setter Vulkan path 分岐 (= host C++ 側 dispatch 機構) |
| 06b scope | cadence 別 update site + dirty flag + flush timing (= 5 cadence 各 upload 位置 + `mValue` cache 継承) |
| 06c scope | descriptor set bind 配線 (= set=0/1/2/3 帯 cadence 別 rebind + UB_* 4 binding vs 84 blueprint 接合) |
| Phase 0 task 順序 | 06a 起案前に (D)(H1a) を消化、(H1b) は 06a 起案後の AYA build run 待ち、(E')(F) は 06b 起案前に消化 |

### §0.2 Phase 0 計測 (= 2 件消化)

| # | task | 手段 | 結論 |
|---|---|---|---|
| **(D)** | shader 内動的 uniform 名存在確認 (3 軸) | C++ setter call site grep + GLSL array uniform 数値性確認 + shader link 時解決 path 追跡 | **compile-time perfect hash 事前 enumerate 完全成立 GO** (3 軸全て static、runtime 動的名前生成 0 件、fallback 設計不要) |
| **(H1a)** | bare uniform 集合 C++ caller 完全 enumerate | `LLShaderMgr::mReservedUniforms` static 配列読込 + `LLStaticHashedString` literal grep + cadence 推定 | **318 (index 経由) + 67 (hashed string 経由) = 385 names、UBO 化対象 267 + sampler 49 + hashed 67 + cadence 不明 16**。chapter 05 §7.3 集約表 = 267 行確定 |

### §0.3 設計 chapter 起案

| chapter | file | 規模 | 主内容 |
|---|---|---|---|
| 06a | `design/06a-cache-structure-and-setter-redirect.md` | 約 370 行、§0-§10 (11 section) | Phase 0 (D)(H1a) 結果記録 / scope vs 非 scope / chapter 04 入力契約 / `mUniformUBOLoc[index]` cache struct + CadenceTag enum / shader link 時 pre-cache flow (`mapUniforms()` Vulkan path 拡張) / 16 method setter Vulkan path 分岐 code shape (`#ifdef LL_VULKAN_GLSL` + `if (mUseUBO)` 分岐 + sampler/INVALID early-out) / 06b/06c/chapter 07 への bridge / (H1b) LL_INFOS hook 提案 / chapter 入出力契約 / 持越 item / update 規律 |

### §0.4 06a 起案の波及 reflect (= 整合 update)

| update 先 | 内容 | 状態 |
|---|---|---|
| chapter 01 §4 (進捗表) | chapter 06 を 06a/06b/06c に 3 分割、06a を ✅ 起案済、06b/06c を 未起案 | unstaged |

(他 chapter への直接波及なし — 06a は内部設計 doc、chapter 01-05 の確定事項を消費する側)

### §0.5 第 2 sub-session 追加完了作業

| 項目 | 詳細 | 出典 |
|---|---|---|
| AYA 指示 = 設計起案中の program 改変提案 NG | 「設計書書いてるのにプログラム書き換えるバカがいるとは思いませんでした。それを資料化すべきじゃないですか？」 | 2026-06-03 AYA 直接指示 |
| memory 保存 (`feedback_design_phase_no_code_write`) | 設計 chapter 起案 phase 中は `indra/` 配下改変禁止、Phase 0 計測 (hook/grep/log) も spec doc 化、実装は別 phase の別 session | memory index に bold 追加 |
| 新 doc `06a-prep-phase0-measurement.md` 起案 (= 案 B 採用) | §0 起案経緯 / §1 入出力契約 / §2 (H1b) LL_INFOS hook spec (= 17 method + 13 LLStaticHashedString + frame counter + CMake gate + log 仕様 + 3 計測 scenario + 除去 protocol) / §3 (E') grep spec / §4 (F) 比較 spec / §5 解析 spec / §6 反映 flow / §7 持越 / §8 update 規律 | 約 380 行 |
| 06a §7 縮約 (= 第 2 sub-session で実施、第 3 で再展開) | hook 提案 70 行 → 2 段落 pointer に縮約 (= `06a-prep` §2 への link)、後に第 3 sub-session で過剰自粛として修正 | 06a §7 update |
| chapter 01 §4 進捗表 | 新行追加 (06a-prep ✅ 起案済) | 01 §4 update |
| 06a §9 持越 (S1) 解消反映 | LLStaticHashedString global registry 仮 API → 本 session で `llstaticstringtable.h:35-66` 直 read 確認、value type / registry 無し確定、`06a-prep` §2.2.2 で `uniform.String()` 経由実装に方針確定 | 本 handoff §2.1 update |
| 本 handoff doc §1 (= 次 session 作業) | (H1b)(E')(F) Phase 0 計測実施 → `06a-prep` spec 化 + 実装 phase 移管、次 session 着手 = chapter 06b 起案 (= 計測結果を待たず着手可) | 本 handoff §1.0 / §1.1 update |

### §0.6 第 3 sub-session (= 本 session 最終) 追加完了作業 — ルール境界明確化

| 項目 | 詳細 | 出典 |
|---|---|---|
| AYA 二次指摘 = ルール境界 over-correction | 「§7.2 hook 配線位置 (= 16 method setter body 入口、`indra/llrender/llglslshader.cpp`) この項けしてるけど、わたしが言ったのは設計書の作成段階で実プログラムを書き換えるなと言っただけで、資料からソースコードの記述を消せなんて言ってないよ どっちでもいいけど」 | 2026-06-03 AYA 直接指摘 |
| ルール境界の明確化 | 禁止 = `indra/` 配下 program file の **実改変** (Edit/Write/MultiEdit) のみ / 禁止 NOT = spec doc 内の file path / line / code shape / 配線位置記述 (= 設計書の本質) | memory `feedback_design_phase_no_code_write` に「ルール境界 (= 過剰自粛防止)」section 追加 |
| 06a §7 復元 | 第 2 sub-session で 2 段落 pointer 化した §7 を §7.1 目的 / §7.2 配線位置 (= `indra/llrender/llglslshader.cpp` line 2141-2538 + 2617-2844、`llappviewer.cpp` idle() に file path / line / 件数明記) / §7.3 build flag / §7.4 scenario / §7.5 完全 spec への pointer の 5 sub-section 構成に restore | 06a §7 update |
| 本 handoff §0.5 注記 | §0.5 「06a §7 縮約」行に「第 3 sub-session で再展開」追記 | 本 handoff §0.5 update |

---

## §1 次 session の作業 (= 06b 起案、Phase 0 計測は実装 phase 入口に移管)

### §1.0 重要転換 (= 2026-06-03 本 session 中の AYA 指示反映)

**design phase 中は `indra/` 配下 program 改変を行わない** (= memory `feedback_design_phase_no_code_write` 由来、本 session 起案の `06a-prep-phase0-measurement.md` で書面化)。これに伴い:

| 旧計画 (= 本 doc 初版) | 新計画 (= 2026-06-03 update) |
|---|---|
| (H1b) hook 実装 → AYA build run → log 解析 を 06b 起案前 prerequisite として実施 | (H1b) hook は **`06a-prep-phase0-measurement.md` §2 に spec 化済**、実装は **設計 chapter 01-10 全件起案完了後の実装 phase 入口** で実施 |
| (E')(F) grep を 06b 起案前 prerequisite として実施 | (E') / (F) は **`06a-prep-phase0-measurement.md` §3 / §4 に spec 化済**、実施は **実装 phase 入口** |
| 06b 起案は (H1b)(E')(F) 確定後 | 06b 起案は **計測結果を待たず着手可** = cadence 5 分類 + 確定済 cadence uniform を前提に書く、不明 uniform は `06a-prep` §6 反映 flow に従って計測後追い update |

### §1.1 次 session = chapter 06b 起案着手

| 項目 | 詳細 |
|---|---|
| chapter 06b scope | per-frame / per-program / per-draw / per-asset / per-skin の **5 cadence 別 update site 設計** + dirty 判定機構 (= `mValue` cache を Vulkan UBO upload 側に乗せ替え) + flush timing (= upload → bind → draw 順序) + `forwardToUboUpload(loc, data, size)` interface 詳細 |
| 着手前提 | `01-overview.md` 〜 `06a-prep-phase0-measurement.md` 全 chapter 読込済 + memory `feedback_design_phase_no_code_write` 遵守 (= `indra/` 配下改変ゼロ) |
| 規模見込み | 06a と同等 (~370 行) — cadence 5 種 × update site / dirty / flush の 3 軸 = 15 セル + interface 仕様 + 06c / chapter 07 への bridge |
| 起案完了判定 | 06b 起案完了 = 設計 chapter 01-06 (06a/06a-prep/06b) 完了、残 06c / 07-10 |

### §1.2 06b 起案後の Phase 0 計測 spec への bridge

06b 起案時、cadence 不明 16 件 / per-program ↔ per-draw 境界 ambiguous の uniform が 06b cadence 別 update site 設計に **直接影響しない** ことを確認:

- 06b は cadence **分類** が確定していれば書ける (= chapter 03 で 5 分類確定済)
- 個別 uniform の cadence 割当は `06a-prep` §6 反映 flow で後追い update する live 表 (= chapter 05 §7.3 集約表 = 別 file `05a-bare-uniform-mapping.md` 切出し)
- = 06b 起案で **計測結果を block しない**

### §1.3 chapter 06b (cadence-update-site-and-dirty) scope (= §1.1 補足、参考保存)

| 項目 | 詳細 |
|---|---|
| per-frame update site | frame loop 内 1 回 update 位置 (e.g., `LLPipeline::render*` 入口) / FrameViewProj / FrameGlobal の upload thread |
| per-program update site | shader bind 時 1 回 update 位置 (= `LLGLSLShader::bind()` 内) / `Program_*` UBO upload |
| per-draw update site | draw call ごと update 位置 (= `renderGeom*` 内) + `forwardToUboUpload` 実装 |
| per-asset update site | GLTF asset state 変化点 (= `gltf::Asset::commitChanges()` 等) / `mNodesUBO` / `mMaterialsUBO` 配線 |
| per-skin update site | rigged animation 毎 frame update (= `LLVOAvatar::updateMeshTextures` 周辺) / matrix palette upload |
| dirty 判定機構 | 既存 `mValue` cache (= `LLGLSLShader::mValue[]`) を Vulkan UBO upload 側 dirty flag に乗せ替え、Material* 切替も per-draw cadence 内 dirty flag で吸収 (= G1 確定の実装) |
| flush timing | 各 cadence の upload → bind → draw の順序保証 (= memory barrier / Vulkan flush) の論理仕様 (Vulkan API 詳細は chapter 07) |

### §1.4 chapter 06c (descriptor-set-bind-wiring) scope

| 項目 | 詳細 |
|---|---|
| set=0 (per-frame 帯) | Frame* UBO の descriptor set 配置 + bind タイミング |
| set=1 (per-program 帯) | Program_* UBO の descriptor set 配置 + shader bind 連動 |
| set=2 (per-draw 帯) | per-draw UBO の descriptor set + dynamic offset 戦略 |
| set=3 (per-asset/per-skin 帯) | GLTF Asset/Skin owner UBO の descriptor set + 動的個数対応 |
| UB_* 4 binding ↔ 84 blueprint 接合 | inventory §3.3 の 4 種論理 binding と 84 GLSL blueprint の最終配線表 |
| `mUseUBO` flag 確定 | 06a で配置だけした flag の initial 設定方針 (= cold launch 時の Vulkan path 全 ON or shader 単位 phase migration) |
| sampler / opaque 系 binding | UBO 化対象外 49 sampler の descriptor set 経由 binding (= chapter 07 vulkan-api-state と接続) |

### §1.5 chapter 06b 起案で AYA 判断を仰ぐ可能性のある論点 (= 暫定)

| # | 論点 | 影響 |
|---|---|---|
| (K) | dirty 判定の粒度 = member 単位 / UBO 単位 / cadence 単位 | upload 粒度 / overhead トレードオフ |
| (L) | per-draw cadence の Vulkan 最適化 = ring buffer / dynamic offset / sub-allocation | 数百〜数千 / frame upload を Vulkan で捌く方式 (06b で論理仕様、chapter 07 で API 接合) |

### §1.6 chapter 06c 起案で AYA 判断を仰ぐ可能性のある論点 (= 暫定)

| # | 論点 | 影響 |
|---|---|---|
| (M) | descriptor set 4 帯 (set=0/1/2/3) bind 戦略 | PSO compatibility / chapter 07 直接接続 |
| (N) | `mUseUBO` initial 設定 = shader 単位 phase migration vs Vulkan path 全 ON | Phase 9 (`09-phase-roadmap`) migration scope と連動 |

### §1.7 次 session 着手前の前提読み込み (= pre-requisite)

順序固定:

1. `design/01-overview.md` (用語定義 + 設計原則 + 確定事項 13 件)
2. `design/02-naming-convention.md` (命名規則 + rename 表)
3. `design/03-cadence-classification.md` (cadence 5 分類)
4. `design/04-codegen-ubo.md` (Codegen-UBO 機構)
5. `design/05-existing-inventory-link.md` (既存 inventory link + bare uniform 集約 framework)
6. `design/06a-cache-structure-and-setter-redirect.md` (cache 構造 + setter redirect)
7. `design/06a-prep-phase0-measurement.md` (本 session 起案、Phase 0 計測 spec、実装 phase 入口手順書)
8. `ayastorm-r41-ubo-current-state-inventory.md` (現状棚卸し、live doc)
9. 本 handoff doc (本 session 完了状態 + design-phase 規律 + 06b scope)

**= この 9 件で次 session の文脈は完全 reconstruct 可能**。

### §1.8 次 session 推奨進め方

- chapter 06b 起案着手 (= cadence 5 種 update site + dirty + flush timing 設計)、`indra/` 配下改変ゼロ厳守
- 06b 起案完了で 1 chapter ≈ 1 session の規律に合致、context 残量を見て /clear 判断
- 06c → 07-10 を順次別 session で起案、design phase 完了 (= 全 chapter 起案済)
- design phase 完了後の **実装 phase 入口で `06a-prep-phase0-measurement.md` を再読込** → Phase 0 計測 (H1b)(E')(F) 実施 → 結果を chapter 05 / 06a / 06b に反映

---

## §2 残持越 item (= 全 chapter の §10 / 本 handoff からの累積)

### §2.1 chapter 06a §9 持越

| # | 項目 | 解消先 |
|---|---|---|
| (H1b) | LL_INFOS hook 実装 + AYA build run + 不明 16 件 cadence 確定 | **`06a-prep-phase0-measurement.md` §2 に spec 化済 → 実装 phase 入口で実施** |
| (Q1) | `mUseUBO` flag の initial 設定方針 (cold launch 全 ON vs shader 単位 phase migration) | 06c §1.6 (N) |
| (Q2) | `forwardToUboUpload(loc, data, size)` 本体実装 (memcpy / ring buffer / dynamic offset / thread 配線) | 06b / chapter 07 |
| (R1) | LLStaticHashedString 経由 setter 67 個の core UBO 化対象外確定根拠の chapter 05 §7.3 への反映 | chapter 05 §7.3 集約表別 file 切出し時 |
| (S1) | LLStaticHashedString global registry API 存在確認 | **本 session 解消** (= `llstaticstringtable.h:35-66` 直 read で value type / registry 無し確定、`06a-prep` §2.2.2 で `uniform.String()` 経由実装に方針確定) |
| (T1) | `mapUniforms()` 内 Vulkan path 拡張で OpenGL path との二重維持 cost (e.g., `glGetUniformLocation` 不要化判断) | chapter 09 Phase 進行中 |

### §2.2 chapter 04 §10 持越 (= 前 handoff から継続)

| # | 項目 | 解消先 |
|---|---|---|
| (A1) | std140 offset 計算 (Codegen 独自 vs SPIR-V reflection 抽出) | chapter 08 |
| (G_codegen) | perfect hash generator (gperf / 独自 / frozen) | chapter 08 |
| (P) | parse 手段 (独自 mini-parser vs glslang reflection) | chapter 08 |

### §2.3 chapter 05 §10 持越 (= 前 handoff から継続、本 session で (D)(H1a) 消化)

| # | 項目 | 解消先 |
|---|---|---|
| (E') | inventory §3.3.1 同一 binding 複数 UBO 名疑い | 次 session 06b 起案前 grep (= §1.2) |
| (F) | MaterialUBO vs MaterialUBO_Legacy 処遇 | 次 session 06b 起案前 grep (= §1.2) |
| (H1) | bare uniform 完全 enumerate | ✅ 本 session (H1a) 消化、(H1b) は §2.1 |
| (H2) | chapter 05 §7.3 集約表の owner (inline / 別 file 切出し) | 267 行確定 → `05a-bare-uniform-mapping.md` に切出し対象 (次 session 別作業 or 06b 内で並行) |
| (H3) | 集約判定 conflict 時の AYA 判断ループ | chapter 09 Phase 進行中 case-by-case |

### §2.4 前 handoff §2 持越の本 session 消化状態

| # | 項目 | 状態 |
|---|---|---|
| A1 | std140 offset 計算 | → chapter 08 譲り継続 |
| G_codegen | perfect hash generator | → chapter 08 譲り継続 |
| **D** | 動的 uniform 名存在確認 | **✅ 本 session 消化、(D) GO 結論** |
| P | parse 手段 | → chapter 08 譲り継続 |
| E' | inventory §3.3.1 binding 重複 | → 次 session 06b 起案前 (継続持越) |
| F | MaterialUBO vs Legacy member 比較 | → 次 session 06b 起案前 (継続持越) |
| **H1** | bare uniform enumerate | **✅ 本 session (H1a) 消化、(H1b) は次 session** |
| H2 | 集約表 owner 判定 | ✅ 267 行確定 → 別 file 切出し対象 |
| H3 | 集約 conflict AYA loop | → chapter 09 |

**= 前 handoff 持越 9 件のうち、本 session で 2 件 (D / H1a) が確定、1 件 (H2) が判定確定、6 件は他 chapter / 次 session 譲り継続**。

---

## §3 unstaged / untracked 状態 (= AYA 判断対象)

### §3.1 累積状態

| 種別 | 内訳 |
|---|---|
| staged (前々 session) | `git mv` で `handoff/` 配下に移動した 97 件 |
| unstaged (前 + 前々 session の累積に本 session 追加) | inventory doc / chapter 01 (§4 / §5) / chapter 02 (§2.1 / §3.4) / chapter 03 (§2 / §4.3) + **本 session: chapter 01 §4 進捗表 (06 → 06a/06b/06c 3 分割 + 06a-prep 追加) / 06a §7 (第 2 sub-session 縮約 + 第 3 sub-session 復元の最終形)** |
| untracked (前 + 前々 session の累積に本 session 追加) | design/ 配下: 01-overview.md / 02-naming-convention.md / 03-cadence-classification.md / 04-codegen-ubo.md / 05-existing-inventory-link.md / **06a-cache-structure-and-setter-redirect.md (本 session 新規) / 06a-prep-phase0-measurement.md (本 session 第 2 sub-session 新規)** / handoff/ 配下: 過去 2 件 + **本 handoff doc 1 件 (本 session 新規)** |
| memory file (`~/.claude/projects/.../memory/`) | **`feedback_design_phase_no_code_write.md` (本 session 第 2 sub-session 新規、第 3 sub-session でルール境界 section 追記) / `MEMORY.md` (index 1 行追加)** |

### §3.2 commit / push / clear 判断は AYA 側

memory `feedback_no_auto_commit` + 既存方針継続。Claude 側から proactive に commit 提案しない。

push / session clear / (H1b) hook 実装 session タイミングも AYA 側で判断。次 session 開始時は §1.7 の 8 件読込から再開可能。

---

## §4 本 session の感触 (= 設計議論結果の要約)

1. **chapter 06 を 3 sub-chapter に分割した判断が正解** = 06a だけで 370 行、単一 chapter にしていたら確実に context 1 session 超過。scope 軸 (cache 構造 / cadence update / descriptor bind) で綺麗に分離できた
2. **Phase 0 (D) GO 結論で fallback 設計負債が完全に消えた** = 「動的 uniform 名 0 件」が 3 軸 (C++ setter / GLSL array / shader link) で確定、compile-time perfect hash が本気の銀弾として機能する確信が固まった
3. **Phase 0 (H1a) で集約表規模 267 行が確定** = chapter 05 §7.3 が確実に別 file 切出し対象 (50 行 rule 抵触)、運用上 `05a-bare-uniform-mapping.md` 切出しが時間問題に
4. **cadence 不明 16 件が (H1b) hook 待ち** = caller context 解析だけで掴めない frame 内呼出回数 / per-program/per-draw 境界 / RLV 条件 path は実機 build run データ必須、grep だけで終わらない設計議論段階に入った
5. **(Q1)(Q2)(R1)(S1)(T1) で 06a §9 が 6 件の持越を生んだ** = 06b/06c/chapter 07-09 への配分が必要、特に (Q1)(Q2) は 06b/06c 起案で集中処理対象。持越 6 件は単一 chapter で消化不能の signal
6. **設計 doc 群が 5 + 06a = 6 chapter 相当に達した** = chapter 01 §4 進捗表は 5+1/12 (06b/06c 追加で分母 12)、残 6 chapter (06b / 06c / 07-10)。chapter 07-10 は短めに収束する見込みだが、06b は cadence update site 5 種 + dirty 機構で 06a 並みの規模見込み

---

## §5 本 handoff doc の更新規律

- 次 session で本 doc を入口として読み込む際、§1.1 (H1b) hook 消化状況 + §1.2 (E')(F) 消化状況を本 doc に上書き反映
- chapter 06b 起案完了時は本 doc を superseded mark、新 handoff doc (`...-design-chapter-06b.md` 等) に引継ぎ
- §3 unstaged / untracked 状態は AYA の commit 判断に追従して更新

---

**= 本 handoff doc を次 session の入口として、(H1b) LL_INFOS hook 実装 → AYA build run → log 解析 → (E')(F) 並行消化 → chapter 06b 起案 の流れで再開する**。
