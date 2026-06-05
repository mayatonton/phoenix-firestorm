# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A 全終了

**作成日**: 2026-06-04
**前 session commit**: `46959adbb0` (= PA-8 set=3 完了、本 session entry 時点)
**本 session 物理出力** (= 全て **未 commit**、AYA さん明示指示後 batch commit):
- `indra/cmake/AyaUboCodegen.cmake` 4 line path 修正 (= `${CMAKE_SOURCE_DIR}` semantics 補正 = α 案、L52/L57/L91/L115)
- 本 handoff doc 新規

**次 session 着手**: **AYA 判断不要** (= Phase 1.A Exit Criteria (i)(ii)(iii) 全充足、Phase 1.A 全終了 marker 確定) → **Phase 1.B entry** = host C++ redirect 層着手 (= 09 §5 + entry handoff §4)。Phase 1.B PB-1 (= 06a §3-§5 = 30 setter Vulkan path 分岐 + mUniformUBOLoc cache) entry handoff doc 起案。副次 task = `_PREFIX_TO_CADENCE` 拡張 (= main.py 3-5 line + unittest 2-3 件) は Phase 1.B entry 直前 or 直後 独立 commit (= PA-7.6 fix と同 weight)。

---

## §0 state 一行 summary

η-30 **Phase 1.A 全終了 state**:
- **Phase 1.A Exit Criteria (i)+(ii) は前 session (= PA-8 set=3 完了、commit 46959adbb0) で smoke 経路で達成済**
- **Phase 1.A Exit Criteria (iii) bind 不変動作確認 は本 session で 実 indra/ build + viewer launch + 経路非到達 3 観点 verify で literal 充足**:
  - 実 indra/ build phase で `codegen_ubo` target が trigger ([1%] AYAstorm r41 (PA-7): generating UBO codegen artifacts → [27%] Built target codegen_ubo)
  - 90 .glsl input discover + 95 file emit + 90 block + 382 member + 0 hash collision + 5102 ms (cache miss path)
  - llrender depend 経由 [53%] 以降 .cpp compile 進行 → [100%] Built target llpackage + tar.xz package 化完了
  - install + cache clear + viewer launch PASS = login + region entry + asset fetch + AOEngine + Stream3D tick 全部正常、Phase 1.A 起因 fail 0 件
- **副次 path fix 4 line** = `indra/cmake/AyaUboCodegen.cmake` の `${CMAKE_SOURCE_DIR}` semantics 補正 (= PA-7 当時 /tmp/smoke project で CMAKE_SOURCE_DIR を repo root に set した盲点、実 autobuild では CMAKE_SOURCE_DIR == `indra/` ゆえ `indra/` 重複 + `scripts/` 誤位置で blueprint count = 0 になっていた、α 案 minimal 差分採用)
- **bind 不変動作 literal 確証**: 経路 (A) blueprint .glsl は viewer の名指し load 規約 (= `loadShaderFile()` で `class/` `cinematic_bd/` のみ) で拾われない / (B) 既存 .cpp は codegen header (`codegen/ubo/ubo_index.inl` 等) を一切 #include していない (= `grep -rn` 0 件) / (C) `aya_attach_ubo_codegen()` は `add_dependencies` + `target_include_directories(PUBLIC)` のみで `target_link_libraries` 不変 = 3 経路全部塞がっており runtime 到達不能。これは 09 §4.2 注「Phase 1 完了時点では既存 program 動作 unchanged」literal 充足
- **画面描画 = 100% OpenGL**: `SDL_GL_SwapBuffers()` (Linux) / `SwapBuffers()` (Win32) で OpenGL frame buffer を画面提示、Vulkan path は 3.4-β-1 placeholder smoke (= `vkCmdDraw(3,1,0,0)` を `sFramebuffer` offscreen target に fire) で結果は捨てる、画面到達なし

**設計判断 0 件** (= AYA 判断項なし、自走完了)。

**残 1 件 未確定 (= 副次 finding、Phase 1.B 行き = 前 session §2.6 から累積)**: set=2 31 件 + set=3 54 件 = **85 UBO 全 cadence_tag が 1 (PerProgram default fallback)** = `main.py:65-72` `_PREFIX_TO_CADENCE` 表が `Frame*` / `Program_*` / `Draw_*` / `Asset_*` / `Skin_*` / `Global_*` の 6 prefix のみ、AYAstorm naming `PerDrawUBO_*` / `PerProgramUBO_*` / `<Name>UBO_Legacy` は **match せず default 1=PerProgram に fallback**。Phase 1.A Exit Criteria literal 充足 (= bind 不変 = host code 無関与、cadence_tag は Phase 1.B redirect 層の dispatch hint として使う想定だが Phase 1.A では unused) ゆえ blocker でない。Phase 1.B host wiring で `_PREFIX_TO_CADENCE` 拡張 (= main.py 3-5 line + unittest 2-3 件追加 = PA-7.6 fix と同 weight trivial) で解消。

**副次 finding 2 件 (= 本 session 発覚、Phase 1.B 行きまたは将来の chapter 拡張候補)**:
- (a) **CMakeCache.txt persistence**: AyaUboCodegen.cmake の `set(... CACHE PATH ...)` は configure 1 回目で値を CMakeCache.txt に永続化、cmake module 側 default 変更しても次回 configure で cache hit して旧値 reload = path fix 反映には `cmake -U <var>` で entry を unset 必要。本 session では `cmake -U AYA_UBO_CODEGEN_BLUEPRINT_DIR -U AYA_UBO_CODEGEN_SCRIPT .` で対処、blueprint count = 90 で正常化確認。Phase 1.B/1.C/2 で cmake module を更新する際は同様の cache invalidate 手順が必要、または `set(... FORCE)` 化検討余地あり (= 但し FORCE 化は user override を block するため一般 cmake 慣例では避ける)
- (b) **spirv-cross not in PATH**: 本 build host (= Linux Ubuntu 24.04) に system `spirv-cross` 不在、`AYA_CODEGEN_SKIP_SPIRV_CHECK=1` fallback 路径で SPIR-V binding 番号 cross-check skip。Phase 1.A wiring (= PA-1) の真 scope は「spirv-cross 取込」だが、本 build host で取込未完で fallback path で動作した = build host setup の補完 task が Phase 1.A entry handoff §3 PA-1 cell の literal 「Linux build PASS」と整合は取れる (= Linux build PASS した) が、cross-check 機能は未稼働。Phase 1.B/1.C/2 着手前に spirv-cross system install (= `sudo apt install spirv-cross` 等) で skip 解除推奨、または 06a §3.4 inventory 更新時に再確認

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-complete.md`) | 全文 | Phase 1.A 全終了 state + 副次 finding 3 件 (cadence default fallback / CMakeCache persistence / spirv-cross PATH) + Phase 1.B entry 起点 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/09-phase-roadmap.md` | §5 (= Phase 2..K 構成、参考) + §4.3 紐付け持越項目 (= Phase 1.B/1.C 行き 3 件 = `sAssetUboPool` prealloc / ring buffer / PSC) | Phase 1.B 全体像 + Phase 1.B/1.C 持越項目把握 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` | §3 (= cache 構造 mUniformUBOLoc) + §4 (= setter 内 Vulkan path 分岐) + §5 (= name → offset 解決 dispatch) | Phase 1.B 実装 chapter (source of truth)、30 setter 内部の Vulkan path 分岐 + perfect hash table dispatch 詳細 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-a-entry.md` | §3 PA-X 構成表 (= Phase 1.A 全 PA-1〜PA-8 完了状態確認用) + §4 後続 Phase 1.B/1.C/2 送り出し項目 |
| `scripts/ubo_codegen/main.py:65-72` | `_PREFIX_TO_CADENCE` 表 (= 副次 finding (1) `_PREFIX_TO_CADENCE` 拡張 Phase 1.B 着手前 副次 task) |
| `indra/cmake/AyaUboCodegen.cmake:52,57,91,115` | 本 session 4 line path fix 箇所 (= `${CMAKE_SOURCE_DIR}/../scripts/...` + `${CMAKE_SOURCE_DIR}/newview/...` + `--project-root "${CMAKE_SOURCE_DIR}/.."`) |
| `indra/newview/app_settings/shaders/aya_r41_blueprints/set{0,1,2,3}/` | 全 90 .glsl (= Phase 1.B host wiring の literal source、UBO 名 + member 列 + binding) |
| `indra/llrender/CMakeLists.txt:10,120` | `include(AyaUboCodegen)` + `aya_attach_ubo_codegen(llrender)` = Phase 1.A wiring 既設、Phase 1.B でも不変 |
| `${CMAKE_BINARY_DIR}/codegen/ubo/ubo_*.inl` (= build 後生成) | 5 aggregated (= ubo_index/ubo_perfect_hash/ubo_metadata/ubo_dummy_init/ubo_host_loader) + 90 per-block layout (= ubo_layout_<name>.inl) = Phase 1.B redirect 層が consume する artifact |

---

## §2 本 session 成果

### §2.1 真 scope 達成: Phase 1.A Exit Criteria (iii) 充足

| 出力契約 | 実装箇所 | 内容 |
|---|---|---|
| 実 indra/ build + codegen 起動 + 95 file emit | `autobuild build` 全 phase | `[1%] AYAstorm r41 (PA-7): generating UBO codegen artifacts` → `[codegen_ubo] INFO: 90 .glsl input(s) discovered` → `[codegen_ubo] INFO: emitted 95 file(s) for 90 block(s) / 382 member(s) in 5102 ms` → `[27%] Built target codegen_ubo` → llrender depend 経由 `[53%]` 以降 .cpp compile → `[100%] Built target llpackage` |
| install + cache clear PASS | `~/ayastorm/` + `~/.ayastorm_x64/cache/` | tar.xz package + `./install.sh` (= 5 backup retain + `/home/ishikawa/ayastorm` install + menu entry install) + `rm -rf ~/.ayastorm_x64/cache/` 全 PASS |
| viewer launch + bind 不変動作確認 | AYA さん起動 + 目視確認 | 起動 PASS、log で Phase 1.A 起因 fail 0 件、AYA さん「変化していないと思う」確認 + 経路非到達 3 観点 verify (= §2.3 詳細) で確証付き |
| Phase 1.A Exit Criteria (i)(ii)(iii) 全充足 | 前 session smoke + 本 session 実 build + viewer | (i) codegen 実行 PASS = ✅ (= 前 smoke + 本 session 実 build 両方 PASS) / (ii) 既存 program 1 個 include = ✅ (= 前 sanity_check.cpp + 本 session llrender include path 経路) / (iii) bind 不変動作 = ✅ (= viewer launch + 経路非到達 verify) |

### §2.2 副次 path fix 4 line (= `indra/cmake/AyaUboCodegen.cmake` α 案)

PA-7 当時 /tmp/smoke project で verify した時の `CMAKE_SOURCE_DIR` は smoke project root (= repo root と一致) だったが、実 autobuild では `indra/` (= `indra/CMakeLists.txt` が top-level cmake source) になる semantics 差分が configure phase で発覚 = blueprint count = 0 件で codegen artifact 0 emit になっていた。AYA さん「α」確定で 4 line minimal 差分修正:

| line | 修正前 | 修正後 | 修正理由 |
|---|---|---|---|
| L52 | `"${CMAKE_SOURCE_DIR}/scripts/ubo_codegen/main.py"` | `"${CMAKE_SOURCE_DIR}/../scripts/ubo_codegen/main.py"` | `scripts/` は repo root 直下、`indra/` の parent ゆえ `..` 経由 |
| L57 | `"${CMAKE_SOURCE_DIR}/indra/newview/app_settings/shaders/aya_r41_blueprints"` | `"${CMAKE_SOURCE_DIR}/newview/app_settings/shaders/aya_r41_blueprints"` | `CMAKE_SOURCE_DIR` 既に `indra/` ゆえ `indra/` 重複削除 |
| L91 | `"${CMAKE_SOURCE_DIR}/scripts/ubo_codegen/*.py"` | `"${CMAKE_SOURCE_DIR}/../scripts/ubo_codegen/*.py"` | L52 と同理由、Python module 群 glob |
| L115 | `--project-root "${CMAKE_SOURCE_DIR}"` | `--project-root "${CMAKE_SOURCE_DIR}/.."` | main.py の `_detect_project_root` 既定 = `script_path.parent.parent.parent` = repo root と整合、cache key normalization 用 root を repo root 化 |

option β (= `AYA_PROJECT_ROOT` 中央定義による頑健化) は退けた = 1 cmake module の 4 line minimal 差分で済む scope に対し中央定義導入は scope overshoot、AYA 確定 α 路線。

### §2.3 bind 不変動作 literal 確証: 経路非到達 3 観点 verify

AYA さん起動目視 (= 「変化していないと思う」) を **理論で裏付け** = 3 経路全部塞がっており runtime 到達不能:

| 経路 | 確認結果 | 根拠 |
|---|---|---|
| (A) blueprint .glsl が viewer の shader loader に拾われる | ❌ 拾われない | `llviewershadermgr.cpp` は `make_pair("deferred/textureUtilV.glsl", 1)` のような **名指し list** で `loadShaderFile()` 経由 load (= 全 shader を 1 個ずつ明示)、`getShaderDirPrefix()` (= L4034) は `"shaders/class"` のみ返し、`getCinematicBDShaderDirPrefix()` (= L4040) は `"shaders/cinematic_bd"` のみ返す。`aya_r41_blueprints/` は install dir に物理存在 (= 54 file 物理確認) するが viewer code が一切名指ししない |
| (B) 既存 .cpp が codegen header を #include | ❌ 誰も include していない | `grep -rn "codegen/ubo\|ubo_index\|ubo_metadata\|ubo_perfect_hash\|ubo_host_loader\|ubo_dummy_init" indra/llrender/*.cpp *.h` = **0 件 match**。include path は `aya_attach_ubo_codegen(llrender)` で追加されたが consumer 0 |
| (C) llrender library link 関係変化 | ❌ 不変 | `aya_attach_ubo_codegen()` の中身は `add_dependencies(${TARGET_NAME} codegen_ubo)` + `target_include_directories(${TARGET_NAME} PUBLIC "${AYA_UBO_CODEGEN_INCLUDE_DIR}")` のみ、`target_link_libraries` なし = build order を強制するだけで symbol/link は不変 |

**= Phase 1.A は build phase に generated artifact (95 .inl) を増やしただけで runtime 経路に一切到達していない完全 inactive shim**。設計 09 §4.2 注「Phase 1 完了時点では既存 program 動作 unchanged」literal 充足。

**画面描画 = 100% OpenGL** 補足:
- `SDL_GL_SwapBuffers()` (Linux = `llwindowsdl.cpp:1241`) / `SwapBuffers(mhDC)` (Win32 = `llwindowwin32.cpp:3874`) で OpenGL frame buffer を画面提示、`vkQueuePresent` への path は `llvkloader.cpp` に存在しない
- Vulkan placeholder draw (= `recordPlaceholderPoolDraw` = `vkCmdDraw(3,1,0,0)` / `recordAvatarPlaceholderDraw`) は 3.4-β-1 placeholder smoke、`sFramebuffer` offscreen target に fire、結果捨て、画面到達なし
- Phase 1.A は上記状態に何も変化加えていない

### §2.4 設計判断 0 件 (= AYA 判断項なし、自走完了)

本 session 唯一の AYA 判断項 = 「path fix α/β どちらか」だけ。α 確定後は build/install/cache clear/log 分析/経路非到達 verify 全 Claude 自走、起動目視のみ AYA さん立ち合い (= 前 handoff §3.5 protocol 通り)。

### §2.5 副次 finding 3 件 (= 次 session で 1 件 Phase 1.B 直前 task、2 件は Phase 1.B/1.C/2 着手前 setup 候補)

**(1) cadence_tag default fallback 累計 85 件** (= 前 session §2.6 から継承継続、Phase 1.B 行き):
- set=2 31 件 + set=3 54 件 = 85 UBO 全 cadence_tag=1 (PerProgram default fallback)
- `main.py:65-72` `_PREFIX_TO_CADENCE` 表に 3 prefix 追加 (= `PerDrawUBO_` → 2 = Draw / `PerProgramUBO_` → 1 = Program / `UBO_Legacy` suffix-match → 1 = Program) で解消
- weight = main.py 3-5 line + unittest 2-3 件 = PA-7.6 fix と同 weight trivial
- 本 handoff 時点判定 = **Phase 1.B entry 直前 or 直後 独立 commit**

**(2) CMakeCache.txt persistence** (= 本 session 発覚、Phase 1.B/1.C/2 cmake 更新時の共通注意):
- AyaUboCodegen.cmake L52/L57 の `set(... CACHE PATH ...)` は configure 1 回目で値を CMakeCache.txt に永続化
- cmake module 側 default 変更しても次回 configure で cache hit して旧値 reload = 反映には `cmake -U <var>` で entry を unset 必要
- 本 session では `cd build-linux-x86_64 && cmake -U AYA_UBO_CODEGEN_BLUEPRINT_DIR -U AYA_UBO_CODEGEN_SCRIPT .` で対処、blueprint count = 90 で正常化確認
- Phase 1.B/1.C/2 で cmake module 更新時は同様の cache invalidate 手順が必要
- 検討余地 = `set(... FORCE)` 化 → 但し FORCE 化は user override を block するため一般 cmake 慣例では避ける、AYAstorm 内部 cmake module ゆえ FORCE 化合理ありの判断は別 phase

**(3) spirv-cross not in PATH** (= 本 session 発覚、build host setup 補完候補):
- 本 build host (= Linux Ubuntu 24.04) に system `spirv-cross` 不在、`AYA_CODEGEN_SKIP_SPIRV_CHECK=1` fallback で SPIR-V binding 番号 cross-check skip
- 09 §4.2 Phase 1.A Exit Criteria literal「build error 0 + 名前解決衝突 0」は SPIR-V cross-check なしでも充足 (= mini-parser の std140 calculator 出力が canonical、cross-check は二重保証用)
- Phase 1.B/1.C/2 着手前に system install (= `sudo apt install spirv-cross` 等) で skip 解除推奨、または 06a §3.4 inventory 更新時に再確認
- AYA 判断不要、Phase 1.B 着手前 setup task 候補

### §2.6 引き継ぐべき protocol (= 前 session §2.7 から継承、本 session 拡張なし)

PA-7.5/PA-7.6/PA-8 set=0/set=1/set=2/set=3 で確立した P-1〜P-5 protocol、本 session は blueprint 著作 phase ではないため未発動。Phase 1.B/1.C/2 で blueprint 拡張・修正発生時に再適用:

- **(P-1)** divergence 検出 = `grep "uniform <Name>\s*{"` で site 列挙 → Agent 並列 + 直接 Read で二重 verify
- **(P-2)** binding 一意性 = smoke 後 metadata で `(set, binding, subset)` 重複確認
- **(P-3)** cadence_tag 推定 = `_PREFIX_TO_CADENCE` 確認 (= 副次 finding (1) で 85 件 default fallback)
- **(P-4)** hash collision = `ubo_perfect_hash.inl` で `g_chd_values[N]` 配列 perfect (= 0 collision)
- **(P-5)** macro literal 置換 = compile-time macro 出現時 source 出典 trace → permutation max 値採用 → comment header 明記

---

## §3 次 session 着手 (= Phase 1.B entry = host C++ redirect 層着手)

### §3.1 着手 1 line

「前 session で Phase 1.A 全終了 (= Exit Criteria (i)(ii)(iii) 全充足 = 実 indra/ build PASS + 95 file emit + viewer launch + 経路非到達 3 観点 verify) + 副次 path fix 4 line 反映完了。本 session = **Phase 1.B entry handoff doc 起案** = host C++ redirect 層着手 (= 30 setter Vulkan path 分岐 + mUniformUBOLoc cache + name → offset 解決 dispatch) → sub-task PB-1〜PB-N 構成 (= 06a §3-§5 chapter からの literal 継承) → strict 線形最終。副次 task 候補 = `_PREFIX_TO_CADENCE` 拡張 (= main.py 3-5 line + unittest 2-3 件、PA-7.6 fix と同 weight) は Phase 1.B entry 直前 or 直後 独立 commit。」

### §3.2 Phase 1.B scope (= 09 §4.1 から literal 継承)

**redirect 層実装** = 30 setter method 内部に Vulkan path 分岐 + name → offset 解決 dispatch + cache 構造 mUniformUBOLoc。

該当 chapter = **06a 全章** (= `06a-cache-structure-and-setter-redirect.md`、source of truth)。

### §3.3 Phase 1.B Exit Criteria (= 09 §4.2 から literal 継承)

**30 setter Vulkan path 分岐の call site から見て transparent** = 既存 program 1 個の動作 unchanged。

補足:
- 30 setter (= `glUniform*` 系 API の wrapper) 全てで Vulkan path 分岐 working、OpenGL path 既存挙動 unchanged
- 1 setter call 1 path 決定論的、build flag で全 path 確認可能
- 09 §4.2 注「Phase 1 完了時点では既存 program 動作 unchanged = Vulkan path 分岐 ON でも OpenGL path 経路を選ぶ default 動作」literal 充足

### §3.4 Phase 1.B sub-task 構成 (= 次 session entry handoff doc で起案、本 handoff では概要のみ)

Phase 1.B entry handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-entry.md`) で **PB-1〜PB-N** strict 線形構成を起案予定。次 session 第 1 着手は本 entry handoff doc 起案 = AYA 判断項先に確認 → 確定後 sub-task 着手。

Phase 1.B 主要 sub-task 候補 (= 06a chapter §3-§5 から逆算):
- **PB-1**: mUniformUBOLoc cache 構造実装 (= 06a §3) = `LLGLSLShader` 等の既存 uniform location cache に UBO offset 解決 entry を追加
- **PB-2**: name → offset 解決 dispatch 実装 (= 06a §5) = codegen `ubo_perfect_hash.inl` の CHD lookup を runtime で呼出、name string → (UBO id, offset) 取得
- **PB-3**: setter 内 Vulkan path 分岐 (= 06a §4) = 30 setter (= `setUniform1i` / `setUniform3fv` 等) 内に `if (mUseVulkanPath) { memcpy(UBO_buffer + offset, ...) } else { glUniform*(...) }` 分岐追加
- **PB-N**: Exit Criteria 充足検証 = 既存 program 1 個 (= 前 PA-8 sanity_check.cpp 経路で確定済 program 1 個) で 30 setter call が Vulkan path/OpenGL path 両方で動作確認

詳細は次 session entry handoff doc で。

### §3.5 Phase 1.B entry 直前 副次 task 候補 (= 累計 finding 3 件)

| # | 内容 | 着手判定 |
|---|---|---|
| (1) `_PREFIX_TO_CADENCE` 拡張 | `main.py:65-72` に `PerDrawUBO_` / `PerProgramUBO_` / `UBO_Legacy` suffix-match 3 prefix 追加 + unittest 2-3 件追加 = 85 UBO の cadence_tag default fallback 解消 | Phase 1.B entry 直前 or 直後 独立 commit、AYA 判断不要 trivial fix |
| (2) CMakeCache.txt invalidate 手順記録 | Phase 1.B/1.C/2 で cmake module 更新時の共通注意、本 handoff §2.5 (2) に literal 記録済、次 session で必要時参照 | 都度発生時に `cmake -U <var>` 適用、永続記録不要 |
| (3) spirv-cross system install | `sudo apt install spirv-cross` で skip 解除推奨 | Phase 1.B 着手前 setup task 候補、AYA さん setup 余力ある時に実施 |

### §3.6 Exit Criteria (iii) 時の注意 (= 前 session §3.5 から継承、Phase 1.B/1.C/2 でも同流)

- **3 OS 統一 scope 外**: Phase 1.B Exit Criteria literal 充足は Linux first-class baseline 1 platform で十分、3 OS 統一は Phase 1.B/1.C/2 完了後の release 直前 phase
- **`build_only_verified` 適用**: viewer 起動目視 PASS 前に Phase 1.B 完了 commit しない
- **`feedback_remove_verification_logs` 適用**: PB-X 実装中の検証用 LL_INFOS hook は commit 前に必ず除去
- **`feedback_restore_debug_settings` 適用**: viewer launch 検証で debug settings 一時変更があれば AYA に「戻す値表」提示

---

## §4 self-verify (= 9 観点、本 handoff 起案時点)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) `indra/cmake/AyaUboCodegen.cmake` 4 line path fix 物理確認 | L52 `${CMAKE_SOURCE_DIR}/../scripts/...` / L57 `${CMAKE_SOURCE_DIR}/newview/...` / L91 `${CMAKE_SOURCE_DIR}/../scripts/...` / L115 `--project-root "${CMAKE_SOURCE_DIR}/.."` 各 line 修正後物理 grep 一致 | ✅ |
| (2) 実 indra/ build phase で codegen 起動 + 95 file emit | build log で `[1%] AYAstorm r41 (PA-7): generating UBO codegen artifacts` → `90 .glsl input(s) discovered` → `emitted 95 file(s) for 90 block(s) / 382 member(s)` → `[27%] Built target codegen_ubo` 物理確認 | ✅ |
| (3) llrender depend 経由 build [53%] 以降 .cpp compile → [100%] llpackage | build log で `[53%] Building CXX object newview/CMakeFiles/ayastorm-bin.dir/lldelayedgestureerror.cpp.o` (= codegen_ubo 完了後の最初の .cpp compile) → 最終 `[100%] Built target llpackage` 物理確認 | ✅ |
| (4) install + cache clear PASS | `./install.sh` で `Installation completed successfully` 物理確認 + `rm -rf ~/.ayastorm_x64/cache/` 完了 + `~/ayastorm/` 配下 file 列挙 PASS | ✅ |
| (5) viewer launch + login + region entry PASS | log で `createSharedDescriptorPool` / `createPerFrameDescriptorSetLayout` / `createPerFrameUbos` / `createSkySmokePipeline` / `writeCurrentPerFrameMatrixUBO` / `writeCurrentTextureMatrixUBO` 既存 Vulkan placeholder 全 path 正常稼働 + `Inventory validate done, fatal errors: 0` + `LogViewerStatsPacket` 送信 (= login PASS marker) + `AOEngine basic folder structure intact` + `Stream3D occlude tick` 物理確認、Phase 1.A 起因 fail 0 件 (= log 内 `aya_ubo` / `ubo_*` keyword 検索 0 件) | ✅ |
| (6) 経路非到達 3 観点 verify PASS | (A) `aya_r41_blueprints` は `llviewershadermgr.cpp` で 0 件 match + viewer は名指し load 規約 / (B) `codegen/ubo` / `ubo_*` include は `indra/llrender/*.cpp *.h` で 0 件 match / (C) `aya_attach_ubo_codegen` 中身は `add_dependencies` + `target_include_directories(PUBLIC)` のみで `target_link_libraries` なし | ✅ |
| (7) 画面描画 = 100% OpenGL 確証 | `SDL_GL_SwapBuffers()` (= `llwindowsdl.cpp:1241`) / `SwapBuffers(mhDC)` (= `llwindowwin32.cpp:3874`) 物理確認 + `vkQueuePresent` への path は `llvkloader.cpp` 内 0 件 + Vulkan placeholder draw は `sFramebuffer` offscreen target (= `rp_begin.framebuffer = sFramebuffer`) で結果捨て | ✅ |
| (8) AYA さん起動目視「変化していないと思う」 + 「変化する可能性は?」確認応答 | AYA 「起動しました」 + 「変化していないと思う」 + 「変化する可能性は?」 → Claude 「3 経路全部塞がっており理論的に変化する経路ゼロ」回答 → AYA 「OpenGL で描画してると思って良い?」 → Claude 「100% OpenGL、画面 present は SDL_GL_SwapBuffers / SwapBuffers のみ、Vulkan は裏 placeholder で画面到達なし」回答 → AYA 「handoff 起案して commit して」 = 全質問応答 closed | ✅ |
| (9) git working tree 状態 = `indra/cmake/AyaUboCodegen.cmake` 4 line modified + 本 handoff doc 新規、indra/ 他改変なし + scripts/ 改変なし + Co-Authored-By: Claude 行不在 + handoff doc 命名対称 (= 前 handoff `…-phase1-a-PA-8-set3-complete.md` の次 = `…-phase1-a-complete.md` で Phase 1.A 全終了 marker) | `git status` で確認 = `modified: indra/cmake/AyaUboCodegen.cmake` + `?? docs/.../handoff/…-phase1-a-complete.md` のみ | ✅ |

---

## §5 引き継ぎ済 memory (= 次 session で active)

- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ
- `feedback_design_phase_no_code_write` — 解禁済 (Phase 1.A 完了、Phase 1.B 着手後も実装 phase 継続)
- `feedback_no_scope_shrink` — 本 session Exit Criteria (iii) 経路 (A)(B)(C) 全 verify、部分検証で済まさない厳守
- `feedback_self_verify_before_handoff` — 本 session 発動 (= AYA 起動目視回答「変化していないと思う」を経路 (A)(B)(C) 3 観点 theoretical proof で裏付け、AYA 1 サイクル浪費回避)
- `feedback_no_claude_coauthor` — 本 handoff doc 含め全 commit 共著行不在
- `feedback_one_step_at_a_time` — path fix α/β 提示 → AYA「α」確定 → 4 line 実施 → configure → blueprint count = 0 発覚 → CMakeCache invalidate → configure 再走 → blueprint count = 90 確定 → build → install → viewer launch → log 分析 → AYA 質問応答 → handoff 起案、各 step 単独進行
- `feedback_doubt_self_first` — 本 session 発動 (= configure 1 回目で blueprint count = 0 発覚、smoke の CMAKE_SOURCE_DIR semantics 盲点を自疑い、AyaUboCodegen.cmake 4 line 自己診断、AYA 判断仰ぐ前に root cause 特定)
- `feedback_admit_unknown` — 本 session 発動 (= AYA「変化する可能性ある?」質問に「不変」即答せず、経路 (A)(B)(C) 3 観点 verify を一段深掘りしてから確証付き回答)
- `feedback_proactive_handoff` — 本 session 発動 (= Phase 1.A 全終了 marker で次 session に handoff)
- `feedback_no_auto_commit` — 本 handoff doc + cmake 4 line fix は AYA 明示指示「commit して」後 batch commit
- `feedback_remove_verification_logs` — 本 session 追加 log/diagnostic 不在 (= 該当なし、cmake module + handoff のみ)
- `feedback_build_only_verified` — Exit Criteria (iii) viewer launch PASS + 経路非到達 verify PASS 後 commit (= 本 session で達成、AYA 明示指示後 batch commit)
- `feedback_tests_dir_never_commit` — 本 session test 新規追加なし
- `feedback_restore_debug_settings` — Exit Criteria (iii) で debug settings 一時変更不在 (= 本 session viewer launch 検証は default 設定で完了)
- `feedback_build` — Exit Criteria (iii) full build フローは Claude 全権実行 (= `project_build_procedure` 参照済)
- `project_build_procedure` — 本 session で参照済 (= configure → build → install → cache clear フル実行)
- `project_ayastorm_r41_vulkan_migration` — Phase 1.A 全終了 milestone = 次 Phase 1.B entry 状態
- `project_ayastorm_r41_design_principles` — Vulkan std140 layout-compat 慣用は本 session 未発動 (= Phase 1.A は blueprint 拡張なし)
- `feedback_ubo_migration_one_at_a_time` — Phase 1.B host wiring も 1 setter 1 commit を default (= 30 setter 一括禁止)
- `project_ayastorm_three_platforms` — Exit Criteria (iii) は Linux first-class baseline 1 platform で literal 充足、3 OS 統一は別 phase
- `feedback_use_agents_proactively` — 本 session 経路非到達 verify は grep 直接実行で完了 (= Agent 不要範囲)
- `feedback_explanation_lead_with_conclusion` — 本 session AYA 質問応答で発動 (= 「変化する可能性は?」「OpenGL で描画?」両方とも結論先 + 表で根拠提示)

---

## §6 次 session 着手 1 line

**「前 session で Phase 1.A 全終了 (= Exit Criteria (i)(ii)(iii) 全充足 = 実 indra/ build PASS + 95 file emit + viewer launch + 経路非到達 3 観点 verify + 画面描画 100% OpenGL 確証) + 副次 path fix 4 line 反映完了 + commit 1 件 (= cmake fix + 本 handoff)。本 session = **Phase 1.B entry handoff doc 起案** = host C++ redirect 層着手 (= 30 setter Vulkan path 分岐 + mUniformUBOLoc cache + name → offset 解決 dispatch) → sub-task PB-1〜PB-N strict 線形構成を 06a §3-§5 から literal 継承して提示 → AYA 判断項先に確認 → 確定後 PB-1 着手。副次 task 候補 = `_PREFIX_TO_CADENCE` 拡張 (= main.py 3-5 line + unittest 2-3 件、PA-7.6 fix と同 weight) + spirv-cross system install (= setup 補完) は Phase 1.B entry 直前 or 直後 独立判定。AYA 判断不要、Claude 自走可。」**
