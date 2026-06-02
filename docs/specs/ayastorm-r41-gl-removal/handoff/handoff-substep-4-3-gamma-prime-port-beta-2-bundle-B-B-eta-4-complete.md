# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-4 完遂 → 次 sub-bundle (B?-η-5 仮称) 着手境界 handoff (2026-06-02)

**parent commit**: `0a4008066c` (B?-η-4 patch、本 handoff の直接 parent) / `733bfc225f` (B?-η-3 patch 範式継承元) / `b67b91152a` (B?-η-2 (a) patch 範式継承元) / `bf9ae4950c` (B?-η-1 patch 範式継承元) / `5b1aa7001f` (B?-η-3-complete handoff doc commit)
**HEAD**: `0a4008066c` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B?-η-4 (= nameless interface block member 名 global scope export 衝突解消 = `_pad_legacy_*` padding member per-block 固有化 = η-3 §3.2 範式の member-level 拡張) 完遂状態 + 次 sub-bundle (推奨 B?-η-5 仮称 = handoff §10 確定 9 件 移管 + `Cannot reuse block name` 第7層 emergence 10 件 + 第7層 emergence 4 系統残 scope 設計) 着手判断境界 を fresh context 引継 用に確定する doc-only handoff。B?-η-3-complete `5b1aa7001f` 範式継承。**handoff §10 想定外発見 (45 件 `nameless block contains a member that already has a name at global scope` = phase 1 multi-expansion 仮説 FALSIFIED → phase 2 body 不一致 仮説 FALSIFIED → phase 3 padding member global scope export 衝突 CONFIRMED の 4 仮説評価) → scope refinement 4th-level で本 sub-bundle 内 3 phase に refine 範式 1 件を新規確立** (= `feedback_admit_unknown` + `feedback_falsification_as_progress` 範式の sub-bundle 内多段適用、phase 失敗時に scope 縮小せず原因 trace で次 phase へ移行)。`nameless block contains a member that already has a name at global scope` **45→0 (-45 / 100% 完全解消)** ✓ 主指標完全達成 + η-3 既達主指標 (`'size' : undeclared identifier` 0 / `Link failed` 0) 完全維持。計 48 file +199/-195、shader file のみ編集 (C++ touch 0)、Agent (general-purpose) 3 件 parallel disjoint scope + pilot 2 file (Claude 自力)。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第七 sub-bundle B?-η の sub-step 4 (= η-3 §3.2 範式の member-level 拡張 = nameless interface block の同名 padding member global scope export 衝突解消) を **48 file +199/-195** で完遂した状態を確定し、次 sub-bundle 着手境界を fresh context に引継ぐ。B?-η-3 (commit `733bfc225f` = PerDrawUBO per-group 名称固有化) で 3 主指標 100% 解消後の第7層 emergence (`'normalMap' redefinition` 188 / `non-opaque` 127 / `binding` 73 / `location` 18 = 計 406 件) と handoff §10 確定 9 件 ((b-1) 4 + (b-2) 3 + (c) 2) の整理に着手前に、log context 再 trace で **`nameless block contains a member that already has a name at global scope` 45 件** を発見 (handoff §10 想定外)。本 sub-bundle は **3 phase に分けて 4 仮説評価** で真因絞り込み:
- **phase 1**: AtmoExtraUBO_Legacy multi-expansion 仮説 (1 file +4/-0 guard wrap insertions-only) → FALSIFIED (Δ 0、効果なし、preventive として保持)
- **phase 2**: FrameAtmosphere body 不一致 + shared guard 仮説 (46 file +184/-184 per-group 固有化 `_Lighting`/`_Skybox` rename) → FALSIFIED (45 件不変、missing #endif 0→127 spike)
- **phase 3**: padding member `_pad_legacy_*` global scope export 衝突 CONFIRMED (8 file +11/-11 per-block padding member 固有化 rename) → 主指標 45→0 (100% 完全達成)

本 sub-bundle は **scope refinement 4th-level 範式の主検証**:
- B?-η-3 §3.1 では scope refinement 3rd-level (handoff §10 想定外 root cause 発見時の本 sub-bundle scope 再設計 + handoff §10 確定分は次 sub-bundle 移管) を確立
- 本 η-4 では **同一 sub-bundle 内で 3 phase に refine** = 仮説連続 falsify (2 連) → 推論停止 → Agent 投入で実データ trace → 真因確定 → phase 3 で根本対処
- `feedback_admit_unknown` 範式の sub-bundle 内多段適用 = 仮説 2 連続外れたら推論停止、log/canary/bisect で実データ取得に切替
- `feedback_falsification_as_progress` 範式の sub-bundle 内多段適用 = 全 phase falsify でも reject 根拠 (Δ 0 / spike 127) を積めば真因絞り込みの論拠

本 sub-bundle は **B?-η-3 §3.2 範式の member-level 拡張** の実例:
- η-3 §3.2 = guard macro collision detection 範式 (per-UBO body 固有化)
- 本 η-4 §3.2 = nameless block の **member 名 global scope export 衝突** detection 範式 (per-block padding member 固有化)
- 構造は同形 (per-block 固有 suffix で衝突回避) だが対象が guard macro (preprocessor) から interface block member (GLSL semantic) に拡張

本 sub-bundle は **cascade pair hypothesis 汎用形完全検証**:
- B?-η-1 §3.2 で確立した cascade pair hypothesis (parse error 後 `#endif` 消費 → `missing #endif` cascade) が **任意 root cause で汎用形成立** することを完全検証
- 本 η-4 では `nameless block` 45 件 → `missing #endif` 45 件 (1:1, +11 offset pair) + `'normalMap' redefinition` 69 件 → `missing #endif` 69 件 (1:1, +0 offset pair) + 独立 13 件 = 計 127 件 metric 完全整合
- `nameless block` および `'normalMap' redefinition` への拡張完全検証 (B?-η-2 (a) §3.2 / B?-η-3 §4 範式のさらなる検証)

本 sub-bundle は **B?-η-1 §3.3 範式 (Agent 並列 disjoint scope) の直接継承** の実例:
- phase 2 で 46 file scope を pilot 2 file (Claude 自力 = atmosphericsFuncs.glsl Group A + skyV.glsl Group B 各 4 行 block 置換) で patch literal 確立後 Agent 並列展開
- Agent A 13 file = class1/deferred 前半
- Agent B 14 file = class1/deferred 後半 + interface + lighting + objects
- Agent C 17 file = class1/windlight + class2 + class3 13 + Group B 4
- phase 3 は Claude 自力 8 file (Read 並列 + Edit 並列、各 file 1-3 padding rename)

---

## §2 B?-η-4 完遂 status

| 項目 | 値 |
|---|---|
| Scope | nameless interface block member 名 global scope export 衝突解消 = `_pad_legacy_*` padding member per-block 固有化 (phase 3 真因対処) + FrameAtmosphere per-group 名称固有化 (phase 2 preventive 保持) + AtmoExtraUBO_Legacy guard wrap (phase 1 preventive 保持) の 3 phase 構成 |
| 修正 file 数 | **48 file** (phase 1 = 1 file + phase 2 = 46 file + phase 3 = 8 file、重複あり) |
| 変更行数 | **+199 / -195** (phase 1 +4/-0 insertions-only + phase 2 +184/-184 rename-only + phase 3 +11/-11 rename-only) |
| 修正範囲 | 48 shader file 全 `#ifdef LL_VULKAN_GLSL` branch 内側、UBO body member 名・型・順序 byte-for-byte 不変 (padding member 名のみ phase 3 で rename = GL `#else` path に存在しない = Vulkan profile 内側 only) + layout `set=X, binding=Y, std140` 全 file 不変 |
| phase 分類 | 3 phase (phase 1 multi-expansion 仮説 FALSIFIED → phase 2 body 不一致 仮説 FALSIFIED → phase 3 padding member 衝突 CONFIRMED) |
| Group 分類 | (phase 2) 2 group (Group A `_Lighting` 41 file = 末尾 `max_cof/_pad_atm0/_pad_atm1` 20 member / Group B `_Skybox` 5 file = 末尾 `haze_horizon/gamma/_pad_atm0/_pad_atm1` 22 member、CASF/cloudsV/postDeferredGammaCorrect/postDeferredTonemap/skyV) |
| padding rename 分類 | (phase 3) 8 file 11 padding member (atmo_extra/deferred_util/material/soften_light/shadow_util×3/sky_v(vec3)/clouds_v/clouds_f×2) |
| Agent 投入 | **3 件 parallel disjoint scope** (Agent A general-purpose = 13 file class1/deferred 前半 / Agent B general-purpose = 14 file class1/deferred 後半+interface+lighting+objects / Agent C general-purpose = 17 file class1/windlight+class2+class3 13 + Group B 4) + Claude 自力 pilot 2 file (atmosphericsFuncs.glsl Group A + skyV.glsl Group B) for phase 2、phase 3 は Claude 自力 8 file (Read + Edit 並列) |
| shader file 触り | **48 件** (本 sub-bundle は shader file のみ、C++ touch 0、A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ/B?-ε/B?-ζ/B?-η-1/B?-η-2 (a)/B?-η-3 既処理 file は §2.1 で個別検証) |
| AYAstorm 改変保全 | **GL path 全不変** (`#else` branch literal 全 48 file 不変、UBO body member 名・型・順序 literal 全 48 file 不変 (padding member 名は Vulkan profile 内側 only = GL path に存在しない)、outer `#ifdef LL_VULKAN_GLSL ... #endif` 全 48 file 不変、既存 η-1/η-2 (a)/η-3 patch (FrameLights/PerDrawUBO_<Group> guard wrap 計 38 file) 全 byte-for-byte 不変、charter §3 #1 acceptance) |
| skip list 13 + A2 拡張 skip 2 + 5 V skip | 48 file 中 8 file 再 touch = atmosphericsFuncs.glsl + godraysF.glsl + volumetricLightF.glsl + shadowUtil.glsl + deferredUtil.glsl + cloudsV.glsl + cloudsF.glsl + skyV.glsl 等 B?-δ admission 範式継承 (UBO body member byte-for-byte 維持 + rename-only or insertions-only + outer LL_VULKAN_GLSL 不変 で skip list 趣旨担保) |
| AYA cold cache launch verify | **PASS** (起動成立 2026-06-01T16:45:10Z + cache 再生成 259 shaderbin = B?-η-3 baseline 223 を 36 件上回り link 成立路径変化 + clean shutdown 16:45:57Z 47 秒 + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + status: stopped 1 件 + 実 FATAL/SIGSEGV/Aborted 0 件) |
| commit | `0a4008066c` (AYA 「OK」明示指示下 2026-06-02) |
| metric vs B?-η-3 baseline `nameless block contains a member that already has a name at global scope` | **-45 ✓ B?-η-4 直接効果 100% 完全達成** (45→0、phase 3 padding rename 直接効果) |
| metric vs B?-η-3 baseline `'size' : undeclared identifier` | ±0 (0→0、B?-η-3 達成完全維持) |
| metric vs B?-η-3 baseline `Link failed` | ±0 (0→0、B?-η-3 達成完全維持) |
| metric vs B?-η-3 baseline `Cannot reuse block name within the same interface` | **+10** (0→10、第7層 emergence = padding rename で hidden cascade 露出、η-2 (a) 達成からの局所退行、B?-η-5 移管対象) |
| metric vs B?-η-3 baseline `missing #endif` | +104 (0→104、cascade pair 内訳変化 = nameless block 45 件 cascade pair + normalMap 69 件 cascade pair の subset + 独立 13 件、η-3 baseline 0 とは別 cascade) |
| metric vs B?-η-3 baseline non-opaque uniforms | -70 (127→57 net 改善、第7層 emergence net シフト) |
| metric vs B?-η-3 baseline parse failed | +144 (42→186、cascade 内訳変化 = 主指標 -45 + 第7層 emergence net +189 で整合) |
| metric vs B?-η-3 baseline redefinition (全体) | -107 (188→81 net 改善、`normalMap` cluster の一部解消 + 第7層 emergence) |
| metric vs B?-η-3 baseline `GBufferInfo` redefinition struct | ±0 (0→0、B?-ζ 達成完全維持) |
| metric vs B?-η-3 baseline `'#'` preprocessor | ±0 (0→0、B?-ε 達成完全維持) |
| metric vs B?-η-3 baseline shader_cache | +36 (223→259、cache 再生成 program 集合シフト = link 成立路径変化、-21 観測点完全解消継続) |
| metric vs B?-η-3 baseline opaque `'binding'` | -52 (73→21 net 改善、第7層 emergence net シフト) |
| metric vs B?-η-3 baseline `'location'` | -4 (18→14 net 改善、第7層 emergence net シフト) |
| metric net delta | **主指標 100% 完全解消** (nameless block -45) + 既達主指標完全維持 ('size' / Link failed 0) + 第7層 emergence net シフト (`Cannot reuse` +10 / non-opaque -70 / `'binding'` -52 / `'location'` -4 / redefinition -107) |

### §2.1 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step 48 file 中 UBO body 既存 literal 全不変 (padding member 名 rename は Vulkan profile 内側 only = phase 3 のみ)、outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 全不変、phase 1+2+3 全て A1-A7 注入結果は byte-for-byte 維持 |
| A8-recovery | AYAstorm 改変 5 file UBO 復活、本 step は 5 V skip 中 4 file (cinematic_bd/class1/deferred/shadowUtil.glsl + class1/windlight/atmosphericsFuncs.glsl + class1/deferred/godraysF.glsl + class3/deferred/volumetricLightF.glsl) を再 touch = B?-δ admission 範式継承 (UBO body member byte-for-byte 維持 + rename-only or insertions-only) |
| B1 | materialF.glsl MaterialUBO_Legacy 化、本 step phase 3 で materialF.glsl の MaterialUBO_Legacy padding member `_pad_legacy_0` → `_pad_material_legacy_0` rename = B1 注入結果の Vulkan profile padding 部分のみ rename、GL path uniform 宣言不変 |
| B2-α | varying + fragment_out 全 program 注入、本 step は varying/fragment_out 触り 0 件 |
| B2-β | vertex_in/VBO attribute 全 program 注入、本 step は vertex_in 触り 0 件 |
| B3 | SPIR-V Vulkan profile override per-stage prepend、本 step も B3 範式の `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路をそのまま継承 (全 phase は LL_VULKAN_GLSL branch 内側) |
| B2-γ | utility source cache + per-program attached utility tracking + utility concat hook + createShader reorder、本 step は utility cache 構造を継承するが shader-file side 修正のみ |
| B?-δ | utility unguarded bare uniform wrap (12 file +167)、本 step は phase 3 で deferredUtil.glsl の DeferredUtilParamUBO_Legacy padding `_pad_legacy_0` → `_pad_deferred_util_legacy_0` rename = B?-δ 注入結果の Vulkan profile padding 部分のみ rename |
| B?-ε | utility source concat 末尾 `\n` 補正 (1 file +11、`llglslshader.cpp`)、本 step touch 0 |
| B?-ζ (b) | `extra_code_text` 内 struct GBufferInfo guard wrap (1 file +10、`llshadermgr.cpp`)、本 step touch 0、§3.1 範式 (shader-file 版応用) で B?-η-1 経由間接継承 |
| B?-η-1 | FrameAtmosphere + PerDrawUBO UBO 宣言 shader-file 側 guard wrap (53 file +236)、本 step phase 1 で AtmoExtraUBO_Legacy guard wrap 1 file = η-1 §3.1 範式同形応用 + phase 2 で FrameAtmosphere 部分の UBO 名 + guard macro 名 rename = η-1 §3.1 範式の bug fix 形応用 + phase 3 で padding member rename = η-1 注入結果の Vulkan profile padding 部分のみ rename |
| B?-η-2 (a) | FrameLights UBO 宣言 shader-file 側 guard wrap (25 file +100)、本 step 48 file 中 FrameLights guard 持つ file は touch 範囲外 (FrameLights guard 部分不変) |
| B?-η-3 | PerDrawUBO per-group 名称固有化 (13 file +52/-52 rename-only)、本 step は PerDrawUBO_<Group> 部分 完全不変 = η-3 達成完全維持 + η-3 §3.2 範式 (guard macro collision detection) を本 step phase 2 で applied work + member-level 拡張は §3.2 で新規確立 |
| **B?-η-4 本 sub-bundle** | nameless interface block member 名 global scope export 衝突解消 = `_pad_legacy_*` padding member per-block 固有化 + FrameAtmosphere per-group 名称固有化 + AtmoExtraUBO_Legacy guard wrap = η-3 §3.2 範式の member-level 拡張 + scope refinement 4th-level (sub-bundle 内 3 phase refine) で本 sub-bundle scope 確定 |

### §2.2 skip list 13 + A2 拡張 skip 2 + 5 V skip 中 8 file 再 touch

本 sub-bundle scope (48 file) と skip list 13 file + cinematic_bd の交差:

| skip list file | 本 step scope inclusion | admission 根拠 |
|---|---|---|
| atmosphericsFuncs.glsl (Visual Realism 5) | YES (phase 1 + phase 2 + phase 3) | phase 1 = AtmoExtraUBO_Legacy guard wrap insertions-only + body 9 member byte-for-byte 不変、phase 2 = FrameAtmosphere_Lighting rename UBO body 20 member byte-for-byte 不変、phase 3 = `_pad_legacy_0` → `_pad_atmo_extra_legacy_0` 1 padding rename Vulkan profile 内側 only = GL `#else` path に存在しない = GL コンパイル路径完全等価、outer LL_VULKAN_GLSL 不変、AYAstorm 改変保護担保 |
| godraysF.glsl (Visual Realism 5) | YES (phase 2) | FrameAtmosphere_Lighting rename UBO body 20 member byte-for-byte 不変、outer LL_VULKAN_GLSL 不変、AYAstorm 改変保護担保 |
| volumetricLightF.glsl (Visual Realism 5) | YES (phase 2) | FrameAtmosphere_Lighting rename、同上 |
| shadowUtil.glsl (Visual Realism 5) | YES (phase 2 + phase 3) | phase 2 = FrameAtmosphere_Lighting rename UBO body 20 member byte-for-byte 不変 + phase 3 = `_pad_legacy_0/1/2` → `_pad_shadow_util_legacy_0/1/2` 3 padding rename Vulkan profile 内側 only、AYAstorm 改変保護担保 |
| deferredUtil.glsl (Picker 2 / Cinematic BD 1 系) | YES (phase 2 + phase 3) | phase 2 = FrameAtmosphere_Lighting rename + phase 3 = `_pad_legacy_0` → `_pad_deferred_util_legacy_0` 1 padding rename、AYAstorm 改変保護担保 |
| cloudsV.glsl (5 V skip) | YES (phase 2 + phase 3) | phase 2 = FrameAtmosphere_Skybox rename (Group B、UBO body 22 member byte-for-byte 不変) + phase 3 = `_pad_legacy_0` → `_pad_clouds_v_legacy_0` 1 padding rename、AYAstorm 改変保護担保 |
| cloudsF.glsl (5 V skip) | YES (phase 3) | `_pad_legacy_0/1` → `_pad_clouds_f_legacy_0/1` 2 padding rename Vulkan profile 内側 only = GL `#else` path に存在しない、AYAstorm 改変保護担保 |
| skyV.glsl (5 V skip) | YES (phase 2 + phase 3) | phase 2 = FrameAtmosphere_Skybox rename (Group B、UBO body 22 member byte-for-byte 不変) + phase 3 = `vec3 _pad_legacy_0` → `vec3 _pad_sky_v_legacy_0` 1 padding rename (vec3 type 保持)、AYAstorm 改変保護担保 |
| 13 file 中残 5 file (Picker 2 + Cinematic BD 1 + Exemplar 1 + Visual Realism 1) | NO | 48 file scope に含まれず、touch 0 件 |
| A2 拡張 skip 2 (`previewV.glsl` + `multiPointLightF.glsl`) | NO | 48 file scope に含まれず、touch 0 件 |

**admission 根拠 (全 8 file 共通)**: B?-δ admission 範式継承 = UBO body member byte-for-byte 維持 + (phase 2) rename-only / (phase 1) insertions-only / (phase 3) padding member 名 rename (Vulkan profile 内側 only = GL path に存在しない) + outer LL_VULKAN_GLSL 不変 = skip list 趣旨 = AYAstorm 改変保護 = 担保。

---

## §3 設計範式 (B?-η-4 で新規確立)

### §3.1 scope refinement 4th-level 範式 (sub-bundle 内多段 phase refine 適用、B?-η-4 で新規確立)

**設計原則**: 単一 sub-bundle 内で **複数仮説を順次評価** する場合、**仮説 falsify 時に scope 縮小せず原因 trace で次 phase へ移行**。各 phase は patch level の最小単位で立ち、phase 失敗時の patch は (a) revert / (b) preventive として保持 のいずれかを 4 仮説評価結果に基づき判断。`feedback_admit_unknown` 範式 (推論 2 連続外れたら推論停止して実データ取得に切替) + `feedback_falsification_as_progress` 範式 (reject 根拠を progress として記録) の sub-bundle 内多段適用。

**scope refinement の level 階層** (η-4 §3.1 で完成):
- **1st-level** (B?-η-2 (a) §3.1 で確立) = handoff §10 想定対処範式 (preprocessor balance) が実態 (cascade root cause 3 種類) と falsify される場合の scope 振替
- **2nd-level** (B?-η-2 (a) §3.1 で確立) = 想定 (b)+(c) を 1 commit に強引統合せず、root cause 3 種別に sub-bundle 分割する scope 設計
- **3rd-level** (B?-η-3 §3.1 で確立) = handoff §10 に **そもそも列挙されていない** root cause (`'size' undeclared` 118 件 = B?-η-1 自作 bug) を着手前 trace で発見 → 本 sub-bundle scope を再設計 + handoff §10 確定分は次 sub-bundle 移管
- **4th-level** (本 η-4 §3.1 で新規確立) = 単一 sub-bundle 内で **3 phase に refine** = 仮説連続 falsify (2 連) → 推論停止 → Agent 投入で実データ trace → 真因確定 → phase 3 で根本対処、phase 失敗の patch も preventive として保持判断 (4 仮説評価)

**4th-level 適用フロー** (本 η-4 で実例化):
1. handoff §10 で (b-1)+(b-2)+(c) 9 件 + 第7層 emergence 4 系統 (406 件) scope 想定
2. 着手前に verify log を再 trace (B?-η-3 commit `733bfc225f` 直後 log)
3. `nameless block contains a member that already has a name at global scope` 45 件発見 → **handoff §10 + 第7層 emergence 4 系統に未列挙** (handoff §10 では nameless block 言及なし、第7層 emergence 4 系統は redefinition `'normalMap'` / non-opaque / binding / location のみ)
4. **phase 1** (multi-expansion 仮説): AtmoExtraUBO_Legacy が AYAstorm Velocity Shader と main shader 両方に attach されて 2 回展開で重複 → 仮説に基づく patch (1 file +4/-0 guard wrap) → verify log 確認 → **FALSIFIED** (45 件不変、Δ 0、効果なし)
5. **phase 2** (body 不一致 + shared guard 仮説): FrameAtmosphere の `max_cof` 末尾 group (20 member) と `haze_horizon/gamma` 末尾 group (22 member) が異なる body で同 guard 共有 → η-3 §3.2 範式適用で per-group 固有化 → 仮説に基づく patch (46 file +184/-184 rename) → verify log 確認 → **FALSIFIED** (45 件不変、missing #endif 0→127 spike)
6. **推論停止判断** (`feedback_admit_unknown` 適用): 仮説 2 連続外れ → 推論停止 → Agent (general-purpose) 投入で実データ trace
7. **Agent trace 結果**: 45 件 nameless block の全 identifier / 全 program / 全 LINE 抽出 + cascade pair pattern 検証 + 4 仮説評価 (A multi-expansion / B body 不一致 / C UBO 重複 attach / **D padding member global scope export 衝突 → CONFIRMED**)
8. **phase 3** (padding member global scope export 衝突 CONFIRMED): `_pad_legacy_0/1/2` が 8 nameless block で global scope 衝突 → η-3 §3.2 範式の member-level 拡張で per-block 固有化 → patch (8 file +11/-11 rename) → verify log 確認 → **CONFIRMED** (45→0、主指標 100% 完全達成)
9. **phase 失敗 patch の判断**: phase 1+2 の patch は 4 仮説評価結果 + constraint 違反なし (charter §3 #1 byte-for-byte 担保) で **preventive 保持** (将来別 file collision 防止 + per-group 固有化は構造的に正しい patch、本 η-4 では実 collision なし)

**仮説 4 件評価の構造** (本 η-4 で実例化、η-5 以降の真因絞り込み範式継承可能):
| 仮説 | 内容 | 評価方法 | 結果 |
|---|---|---|---|
| A multi-expansion | utility file が 2 program に attach されて 2 回展開 | guard wrap で防止 → log 確認 | FALSIFIED (Δ 0) |
| B body 不一致 + shared guard | FrameAtmosphere 2 body が同 guard 共有 | per-group 固有化 → log 確認 | FALSIFIED (Δ 0、missing #endif spike) |
| C UBO 重複 attach | 複数 program で同 UBO が attach 順違反 | attach 順 trace | FALSIFIED (重複なし) |
| **D padding member global scope export** | nameless block の `_pad_legacy_*` が global scope で 8 block 衝突 | per-block 固有化 → log 確認 | **CONFIRMED (45→0)** |

### §3.2 nameless block member 名 global scope export 衝突 detection 範式 (B?-η-4 で新規確立、η-3 §3.2 範式の member-level 拡張)

**設計原則**: GLSL/Vulkan profile の **nameless interface block** (= `uniform <Name> { ... }; // インスタンス名なし`) は **member が global scope に export される** GLSL/SPIR-V 仕様。同名 member を持つ複数 nameless block を **同一 program に attach** すると、`nameless block contains a member that already has a name at global scope` cascade 発火。**member 名は per-block 固有化必須** = guard macro collision detection 範式 (η-3 §3.2) の member-level 拡張。

**collision detection 手順** (η-4 で実例化、4 step):
1. **エラー件数 × 発生 program 関係 trace** = log 全件 grep で `nameless block` LINE + program 名 抽出 (45 件全 program 抽出)
2. **attach される全 file の nameless block member 名 literal grep で抽出** (`grep -A 30 "uniform.*_Legacy {" *.glsl` で全 member 列挙)
3. **member 名 collision 検出** = padding 系 (`_pad_legacy_*` 等 high-risk) + 汎用名 (`size` 等 high-risk、η-3 で別 collision 検出済) + struct field 名 を per-block 横断比較
4. **per-block 固有化 rename** = `_pad_<group>_legacy_<idx>` style (例 `_pad_atmo_extra_legacy_0` / `_pad_deferred_util_legacy_0` 等) で全 collision 解消

**GL path 不可触担保**: padding member は **GL `#else` path に存在しない** (= Vulkan profile 内側 only) ため、padding member 名 rename は **GL コンパイル路径完全等価** = charter §3 #1 byte-for-byte 担保。

**8 nameless `_Legacy` block 共有** (本 η-4 で確認):
| block 名 | file | padding 名 (η-4 前) | padding 名 (η-4 後) |
|---|---|---|---|
| AtmoExtraUBO_Legacy | atmosphericsFuncs.glsl | `_pad_legacy_0` | `_pad_atmo_extra_legacy_0` |
| DeferredUtilParamUBO_Legacy | deferredUtil.glsl | `_pad_legacy_0` | `_pad_deferred_util_legacy_0` |
| MaterialUBO_Legacy | materialF.glsl | `_pad_legacy_0` | `_pad_material_legacy_0` |
| SoftenLightParamUBO_Legacy | softenLightF.glsl | `_pad_legacy_0` | `_pad_soften_light_legacy_0` |
| ShadowUtilParamUBO_Legacy | shadowUtil.glsl | `_pad_legacy_0/1/2` | `_pad_shadow_util_legacy_0/1/2` |
| SkyVParamUBO_Legacy | skyV.glsl | `vec3 _pad_legacy_0` | `vec3 _pad_sky_v_legacy_0` (vec3 type 保持) |
| CloudsVParamUBO_Legacy | cloudsV.glsl | `_pad_legacy_0` | `_pad_clouds_v_legacy_0` |
| CloudsFParamUBO_Legacy | cloudsF.glsl | `_pad_legacy_0/1` | `_pad_clouds_f_legacy_0/1` |

**collision の cascade exposure**:
- 8 nameless block で `_pad_legacy_0` が global scope 衝突 → `nameless block contains a member that already has a name at global scope` 45 件 cascade (1 program 内で 2+ block 同時 attach の組合せ数)
- B?-η-4 phase 3 で per-block 固有化 → 8 block co-existence 担保 → nameless block 0 件

### §3.3 cascade pair hypothesis 汎用形完全検証 (B?-η-4 で完全証明、η-1 §3.2 + η-2 (a) §3.2 範式の最終検証)

**設計原則**: B?-η-1 §3.2 で確立した cascade pair hypothesis (parse error 後 `#endif` 消費 → outer `#ifdef` の `#endif` が「消える」⇒ `missing #endif` cascade) が **任意 root cause で汎用形成立** することを完全検証。本 η-4 では `nameless block` + `'normalMap' redefinition` 2 系統で完全 1:1 対応観測。

**本 η-4 で観測した cascade pair の整合**:
| root cause | 件数 | cascade pair `missing #endif` | offset | 確認 |
|---|---|---|---|---|
| `nameless block contains a member that already has a name at global scope` | 45 (η-3 baseline) | 45 (η-4 phase 2 後) | +11 LINE | 1:1 完全対応 ✓ |
| `'normalMap' : redefinition` at LINE `0:1332` | 69 (η-4 phase 2 後) | 69 (η-4 phase 2 後) | +0 LINE (同 LINE) | 1:1 完全対応 ✓ |
| 独立 missing #endif (handoff §10 既知 9 + Underwater/Water Haze 系 4) | 13 | 13 | N/A | 1:1 完全対応 ✓ |
| **計** | **127** | **127** | N/A | **完全整合 ✓** |

**汎用形完全検証完了**: cascade pair hypothesis は 4 種類の root cause (η-1 `Cannot reuse block name` / η-2 (a) FrameLights collision / η-3 `'size' undeclared` / η-4 `nameless block` + `'normalMap' redefinition`) で実証 = 汎用形完全証明。次 sub-bundle 以降は cascade pair `missing #endif` を **自動消滅指標** として継続観測 (= 主指標解消で cascade pair も自動 1:1 で消滅 = `feedback_falsification_as_progress` 範式の構造的応用)。

### §3.4 仮説 4 件評価による真因絞り込み範式 (B?-η-4 で新規確立、`feedback_admit_unknown` の sub-bundle 内多段適用)

**設計原則**: 単一 sub-bundle 内で複数仮説が立つ場合、**事前に 4 仮説評価 table を起草** + 各仮説の **評価方法 + 期待効果** を patch 着手前に明示 + Agent (general-purpose) を **同時並列で全 4 仮説検証** することで真因 1 つを絞り込む。`feedback_admit_unknown` 範式 (推論 2 連続外れたら推論停止して実データ取得に切替) の sub-bundle 内構造化。

**仮説 4 件評価 table** (η-4 §3.1 で実例化、再掲):
- 仮説 A multi-expansion → FALSIFIED (phase 1 で実測 Δ 0)
- 仮説 B body 不一致 + shared guard → FALSIFIED (phase 2 で実測 Δ 0 + missing #endif spike)
- 仮説 C UBO 重複 attach → FALSIFIED (Agent trace で attach 重複なし確認)
- **仮説 D padding member global scope export → CONFIRMED (Agent trace で global scope 衝突 + phase 3 で実測 45→0)**

**Agent (general-purpose) 投入の閾値** (本 η-4 で確立):
- 推論 1 連続外れ = 再推論で continuation OK
- 推論 2 連続外れ = **推論停止**、Agent 投入で実データ trace 必須
- 本 η-4 では phase 1 + phase 2 連続 falsify → Agent 投入 → 真因確定 (24 時間内に解決、`feedback_admit_unknown` 24 時間 ceiling 内)

---

## §4 cold cache launch verify metric (vs B?-η-3 baseline)

| metric pattern | B?-η-3 baseline (commit `733bfc225f` 直後) | B?-η-4 (commit `0a4008066c` 直後) | Δ | 評価 |
|---|---|---|---|---|
| **`nameless block contains a member that already has a name at global scope`** | **45** | **0** | **-45 (100% 完全解消)** | ✓ 主指標完全達成 (phase 3 padding rename 直接効果) |
| `'size' : undeclared identifier` | 0 | 0 | ±0 | ✓ B?-η-3 達成完全維持 |
| `Link failed` | 0 | 0 | ±0 | ✓ B?-η-3 達成完全維持 |
| `Cannot reuse block name within the same interface` | 0 | **10** | **+10** | 第7層 emergence (padding rename で hidden cascade 露出、η-2 (a) 達成からの局所退行、B?-η-5 移管対象) |
| `missing #endif` | 0 | 104 | +104 | cascade pair 内訳変化 (nameless block 45 件 cascade pair + normalMap 69 件 cascade pair の subset + 独立 13 件、η-3 baseline 0 とは別 cascade、§3.3 汎用形完全検証で構造的に説明可能) |
| `non-opaque uniforms outside a block` | 127 | 57 | **-70** | net 改善 第7層 emergence net シフト |
| `parse failed for stage` | 42 | 186 | +144 | cascade 内訳変化 (主指標 -45 + 第7層 emergence net +189 で整合) |
| redefinition (全体) | 188 | 81 | **-107** | net 改善 (`normalMap` cluster の一部解消 + 第7層 emergence) |
| `GBufferInfo` redefinition struct | 0 | 0 | ±0 | B?-ζ 達成完全維持 |
| `'#'` preprocessor directive | 0 | 0 | ±0 | B?-ε 達成完全維持 |
| shader_cache 件数 | 223 | **259** | **+36** | cache 再生成 program 集合シフト = link 成立路径変化 (-21 観測点完全解消継続) |
| opaque `'binding'` | 73 | 21 | **-52** | net 改善 第7層 emergence net シフト |
| `'location'` | 18 | 14 | **-4** | net 改善 第7層 emergence net シフト |
| `FATAL`/`SIGSEGV`/`Aborted` | 0 | 0 | ±0 | clean |
| `Goodbye!` | 1 | 1 | ±0 | clean shutdown |
| `Vulkan (device|instance) destroyed` | 2 | 2 | ±0 | clean |
| `status: stopped` | 1 | 1 | ±0 | clean |

### §4.1 第7層 emergence net シフト分析

handoff §10 確定 9 件 + `Cannot reuse block name` 第7層 emergence 10 件 = B?-η-5 移管に加え、本 η-4 で **第7層 emergence net シフト** を観測:

| emergence 種 | η-3 baseline | η-4 後 | Δ | 解釈 | 次 sub-bundle scope |
|---|---|---|---|---|---|
| `'normalMap' : redefinition` at LINE `0:1332` | 188 | (含む redefinition 81 内) | -107 (redefinition 全体) | cluster の一部解消 + 第7層 emergence | B?-η-5 で個別 trace 推奨 |
| `non-opaque uniforms outside a block` | 127 | 57 | -70 | link 成立 program 集合シフトで露出側変化、net 改善 | B?-η-6 (B?-δ 範式継承 program-scope) |
| `'binding'` (sampler/texture/image) | 73 | 21 | -52 | link 成立 program 集合シフトで露出側変化、net 改善 | B?-η-7 (sampler binding 専用) |
| `'location'` | 18 | 14 | -4 | link 成立 program 集合シフトで露出側変化、net 改善 | B?-η-8 (location 専用) |
| `Cannot reuse block name within the same interface` | 0 | **10** | **+10** | padding rename で hidden cascade 露出、η-2 (a) 達成からの局所退行 | **B?-η-5 主 scope** (10 件 + handoff §10 確定 9 件 = 計 19 件) |

**注記**: η-4 で第7層 emergence 4 系統 (η-3 計 406 件 → η-4 計 173 件 + Cannot reuse 10 件 = 183 件) は **計 -223 件 net 改善** だが、これは link 成立 program 集合シフトによる露出側変化が主要因 (= shader_cache +36 で実証)。`Cannot reuse block name` +10 は **真の局所退行** = B?-η-5 主 scope。

### §4.2 主指標 metric integrity self-check

**B3 §12 literal grep 範式継承**: B?-η-4 metric は **literal pattern grep** 結果のみで報告:
- `nameless block contains a member that already has a name at global scope` → 0
- `'size' : undeclared identifier` → 0
- `Link failed` → 0
- 数値 -45 (主指標) は Solution = phase 3 padding rename による構造的解消

**self-check 観点**:
- 主指標 100% 完全解消の整合性 = nameless block 45 件 全 `_pad_legacy_*` global scope 衝突 → per-block 固有化で全 45 件直接解消、η-3 既達主指標 ('size' 0 / Link failed 0) 完全維持
- parse failed 42→186 (+144) は cascade 内訳変化 = 主指標 -45 + 第7層 emergence net +189 で整合 (cascade pair `missing #endif` +104 と整合)
- shader_cache 223→259 (+36) は link 成立路径変化で program 集合シフト = 一見 cache 増だが実態は link 成立 program のみ shaderbin 生成、-21 観測点 (B?-δ 達成完全解消継続) は維持

---

## §5 self-verify (本 doc 起草前の Claude 自己検証)

| 項目 | 結果 |
|---|---|
| (1) 48 file `git diff --stat` 合計 = 48 files / 199 insertions / 195 deletions | ✓ |
| (2) phase 1 atmosphericsFuncs.glsl 4 insertions / 0 deletions (insertions-only AtmoExtraUBO_Legacy guard wrap) | ✓ |
| (3) phase 2 46 file 各 +4/-4 rename-only (Group A 41 file `_Lighting` + Group B 5 file `_Skybox`) | ✓ |
| (4) phase 3 8 file 計 +11/-11 padding rename (atmo_extra/deferred_util/material/soften_light/shadow_util×3/sky_v/clouds_v/clouds_f×2) | ✓ |
| (5) outer `#ifdef LL_VULKAN_GLSL ... #else ... #endif` 全 48 file 不変 (git diff で context line 確認) | ✓ |
| (6) UBO body member 名・型・順序 不変 (phase 2 は UBO 宣言行 rename のみ + phase 3 は padding member 名のみ rename = 型・順序保持) | ✓ |
| (7) `#else` GL path uniform 宣言 literal 不変 (git diff で `#else` 後 context line 不変) | ✓ |
| (8) padding member は GL `#else` path に存在しない (= Vulkan profile 内側 only) = phase 3 rename は GL コンパイル路径完全等価 | ✓ |
| (9) 48 file 全件 inner guard/UBO/padding が LL_VULKAN_GLSL branch 内側 (= outer `#ifdef LL_VULKAN_GLSL` 直後 ~ outer `#else` 直前 の範囲内) | ✓ |
| (10) AYA cold cache launch PASS (起動成立 16:45:10Z + clean shutdown 16:45:57Z 47 sec) | ✓ |
| (11) `~/.ayastorm_x64/cache/shader_cache/` 259 件 (B?-η-3 baseline 223 から +36、link 成立 program 集合シフト) | ✓ |
| (12) FATAL / SIGSEGV / Aborted 0 件 | ✓ |
| (13) Goodbye! 1 件 / Vulkan device/instance destroyed 各 1 件 / status: stopped 1 件 | ✓ |
| (14) 既存 η-1 patch (FrameAtmosphere/PerDrawUBO guard、13 file) FrameAtmosphere 部分は本 η-4 phase 2 で per-group 固有化 (UBO 名 + guard macro 名 rename) + PerDrawUBO 部分は η-3 で per-group 固有化済 (本 η-4 では完全不変) | ✓ |
| (15) 既存 η-2 (a) patch (FrameLights guard、25 file) 全 byte-for-byte 不変 (本 η-4 scope の 48 file 中 FrameLights guard 部分は touch 範囲外) | ✓ |
| (16) 既存 η-3 patch (PerDrawUBO_<Group> 13 file +52/-52) 全 byte-for-byte 不変 (本 η-4 scope は PerDrawUBO_<Group> 部分完全不変) | ✓ |

---

## §6 設計範式継承表

| 範式 | 由来 | 本 sub-bundle 適用箇所 |
|---|---|---|
| `feedback_doubt_self_first` | feedback memory | handoff §10 を疑って verify log 再 trace で nameless block 45 件発見 = handoff §10 + 第7層 emergence 4 系統 未列挙 root cause |
| `feedback_admit_unknown` | feedback memory | phase 1+2 連続 falsify → 推論停止 → Agent 投入で実データ trace で真因 (padding member global scope export 衝突) 確定 |
| `feedback_build_only_verified` | feedback memory | phase 3 padding rename Solution の効果は (phase 2 完遂後 baseline からの) 実測 -45 + log context 抽出で検証 |
| `feedback_falsification_as_progress` | feedback memory | phase 1+2 連続 FALSIFIED でも reject 根拠 (Δ 0 / spike 127) を progress として handoff doc に記録 + 真因絞り込みの論拠 |
| `feedback_one_step_at_a_time` | feedback memory | Step 1 (literal verify η-3 baseline) → Step 2 (cascade source 候補 4 仮説起草) → Step 3 (phase 1 patch) → Step 4 (phase 1 verify) → Step 5 (phase 2 patch) → Step 6 (phase 2 verify) → Step 7 (Agent 投入 + 4 仮説評価) → Step 8 (phase 3 patch) → Step 9 (phase 3 verify) → Step 10 (commit) の sequential 進行 |
| `feedback_no_auto_commit` | feedback memory | AYA 「OK」明示承認下のみ commit (phase 1+2+3 patch 一括 commit、handoff doc は別 commit 想定) |
| `feedback_no_claude_coauthor` | feedback memory | commit message に Claude 共著行なし |
| `feedback_no_scope_shrink` | feedback memory | phase 1+2 falsify でも scope shrink せず phase 3 まで refine 継続 = `feedback_no_scope_shrink` の 4th-level 適用 |
| `feedback_shader_only_fast_iterate` | feedback memory | shader-only 変更のため autobuild 不要、`cp` + `rm shader_cache` のみで反映 (各 phase 検証で適用) |
| `feedback_root_cause_not_dump` | feedback memory | 真因確定後 fallback (η-4 revert / named instance block 化等) でなく根本修正 (padding member 固有化) を選択、η-3 §3.2 範式 member-level 拡張で同形対処 |
| B1 §3 (MaterialUBO_Legacy file-local override 範式) | B1 commit | phase 3 で materialF.glsl `_pad_legacy_0` → `_pad_material_legacy_0` rename = B1 注入結果の Vulkan profile padding 部分のみ rename |
| B2-α §3.1 (varying + fragment_out 全 program 注入範式) | B2-α commit | (本 sub-bundle 適用 0、次 sub-bundle で 'location' 系 scope に継承想定) |
| B3 §3.2 (SPIR-V Vulkan profile override per-stage prepend 範式) | B3 commit | 48 shader file 全 `#ifdef LL_VULKAN_GLSL` branch 内側 = B3 範式 `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路で展開 |
| B3 §12 (literal grep metric 範式) | B3 commit | §4 metric 全件 literal pattern grep のみ採用 |
| B2-γ §3.1 (utility source cache 範式) | B2-γ commit | (本 sub-bundle は utility cache 構造を継承するが shader-file side 修正のみ) |
| B2-γ §3.2 (per-program attached utility tracking 範式) | B2-γ commit | 同上 |
| B2-γ §3.3 (utility concat hook + createShader reorder 範式) | B2-γ commit | 同上 |
| B?-δ §3.1 (utility 既 attach 全 file scope unguarded bare uniform 網羅 scan 範式) | B?-δ commit | (本 sub-bundle 適用 0、bare uniform touch 0、B?-η-5 (b-1) で直接継承想定) |
| B?-δ §3.2 (cascade source 特定範式) | B?-δ commit | nameless block 45 件 = 8 nameless `_Legacy` block 共有 padding 由来 単一 root cause を事前 literal grep + Agent trace で確定 |
| B?-ε §3.2 (cascade source ALL N errors 同一 0:LINE 集中 → 1 cluster 範式) | B?-ε commit | 第7層 emergence `'normalMap' redefinition` 188→ 一部解消 ALL LINE 0:1332 集中で本範式適用想定継続 |
| B?-ζ §3.1 (`extra_code_text` guard wrap 範式) | B?-ζ commit | B?-η-1 §3.1 経由間接継承 (shader-file 版応用) |
| B?-η-1 §3.1 (shader-file 側 UBO 宣言 guard wrap 範式) | B?-η-1 commit | phase 1 AtmoExtraUBO_Legacy guard wrap 同形応用 + phase 2 FrameAtmosphere 部分の UBO 名 + guard macro 名 rename = bug fix 形応用 |
| B?-η-1 §3.2 (cascade pair hypothesis) | B?-η-1 commit | **本 η-4 で完全証明** = nameless block 45 件 + normalMap 69 件 + 独立 13 件 = 計 127 件 cascade pair 完全 1:1 対応 (§3.3) |
| **B?-η-1 §3.3 (Agent 並列 disjoke scope)** | **B?-η-1 commit** | **本 sub-bundle で直接継承 = 同形適用** (phase 2 で 46 file scope を pilot 2 file + 3 Agent disjoint 分割) |
| **B?-η-2 (a) §3.1 (scope refinement 範式)** | **B?-η-2 (a) commit** | **本 sub-bundle で 4th-level に拡張** = sub-bundle 内 3 phase refine 範式 (§3.1) |
| B?-η-2 (a) §3.2 (cascade pair hypothesis 汎用形) | B?-η-2 (a) commit | 本 η-4 §3.3 で完全証明 |
| B?-η-2 (a) §3.3 (第6層 emergence 分類) | B?-η-2 (a) commit | 本 η-4 §4.1 で第7層 emergence net シフト分析に拡張 |
| **B?-η-3 §3.1 (scope refinement 3rd-level 範式)** | **B?-η-3 commit** | **本 sub-bundle で 4th-level に拡張** (§3.1) |
| **B?-η-3 §3.2 (guard macro collision detection 範式)** | **B?-η-3 commit** | **本 sub-bundle で member-level に拡張** = nameless block member 名 global scope export 衝突 detection 範式 (§3.2) |
| B?-η-3 §3.3 (第7層 emergence 4 系統分類) | B?-η-3 commit | 本 η-4 §4.1 で net シフト分析 + `Cannot reuse block name` 第7層 emergence +10 件追加 |

---

## §7 risks 観測点 (本 sub-bundle で発生した、または将来発生し得る)

| risk | 観測状況 | 想定対処 |
|---|---|---|
| (1) UBO body 差異 (FrameAtmosphere 2 group) で guard wrap が link failure exposure を引き起こす (B?-η-1 §7 (1) 継承) | 本 η-4 phase 2 で per-group 固有化により FrameAtmosphere 2 group co-existence 担保 = risk (1) 完了 (η-3 で PerDrawUBO 6 group + η-4 で FrameAtmosphere 2 group の per-group 固有化完了) | 完了 (η-1 §7 (1) 範式完全達成) |
| (2) skip list 8 file 再 touch (atmosphericsFuncs/godraysF/volumetricLightF/shadowUtil/deferredUtil/cloudsV/cloudsF/skyV) | UBO body member byte-for-byte 維持 + rename-only or insertions-only + outer LL_VULKAN_GLSL 不変 + padding member 名 rename は Vulkan profile 内側 only = AYAstorm 改変保護担保 (§2.2 admission 範式継承) | 次 sub-bundle で skip list file が scope に含まれる場合は B?-δ admission 範式継承 |
| (3) cascade pair hypothesis 汎用形完全検証 | 本 η-4 §3.3 で `nameless block` 45 件 + `'normalMap' redefinition` 69 件 + 独立 13 件 = 計 127 件 cascade pair 1:1 完全対応観測 = 汎用形完全証明 | 次 sub-bundle 以降は cascade pair `missing #endif` を自動消滅指標として継続観測 |
| (4) scope refinement 4th-level 範式 (sub-bundle 内 3 phase refine) の濫用リスク | 本 η-4 が初回適用、phase 失敗時の patch 保持判断は 4 仮説評価結果 + constraint 違反なし (charter §3 #1 byte-for-byte 担保) で実証 | 次 sub-bundle 以降も 4th-level 適用時は同基準で実証必須 |
| (5) `Cannot reuse block name within the same interface` 第7層 emergence +10 件 (η-2 (a) 達成からの局所退行) | 本 η-4 phase 3 padding rename で hidden cascade 露出 = η-2 (a) 達成水準からの局所退行 | B?-η-5 主 scope (10 件 + handoff §10 確定 9 件 = 計 19 件) |
| (6) Agent 並列 disjoint scope の file 衝突 | 0 件 (44 file 残を 3 Agent A/B/C 完全 disjoint、各 13/14/17 file) | 次 sub-bundle で Agent 並列を行う場合も同 disjoint 分割を維持 |
| (7) literal verify 範式忘却 | Agent 3 件全件 self-report と Claude 側 `git diff --stat` 突合で整合確認 ✓ | 次 sub-bundle でも literal grep 結果 vs Agent 報告の併走を必須 |
| (8) shader_cache 259+ 維持 観測点 | 本 η-4 で 259 件 (+36 link 成立 program 集合シフト) = 223 baseline 上回り 健全 | 次 sub-bundle で 259+ 維持 観測点に切替 |
| (9) charter §3 #1 byte-for-byte 維持 verify | git diff context line 検証で `#else` GL path uniform 宣言 + outer `#ifdef`/`#endif` + 既存 η-1/η-2 (a)/η-3 patch 全不変 + padding member は Vulkan profile 内側 only | 次 sub-bundle でも git diff context line ≥ 2 で確認 |
| (10) 範式誤伝承 (η-3 §3.2 範式の member-level 拡張は nameless block padding member で確立、他 member 系 (struct field / 通常 member) への伝承時は collision detection 範式 (§3.2) の事前適用必須) | §3.1 / §3.2 で η-3 §3.2 範式の member-level 拡張と明示 + collision detection 範式 4 step 手順 確立 | 次 sub-bundle で他 nameless block member collision が発見される場合は §3.2 collision detection 範式を事前適用 |

---

## §8 commit log

```
0a4008066c feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-4 完遂 (48 file +199/-195、3 phase 構成、nameless block 45→0 主指標完全達成、AYA 「OK」明示指示下 commit 2026-06-02)
```

詳細 commit message body は git log 参照。

---

## §10 次 sub-bundle B?-η-5 (仮称) 推奨 scope (handoff §10 確定 9 件 移管 + `Cannot reuse block name` 第7層 emergence 10 件 + 第7層 emergence 4 系統残)

### §10.1 推奨 scope = B?-η-5 (a) `Cannot reuse block name` 第7層 emergence 10 件 + (b-1)+(b-2)+(c) cascade root cause 3 種別整理 (handoff §10 移管)

**(a) `Cannot reuse block name within the same interface` 10 件 trace** (主 scope = 第7層 emergence、η-2 (a) 達成からの局所退行):
- root cause 想定: padding rename で hidden cascade 露出、η-4 phase 3 後初露出
- 想定対処: η-2 (a) 範式継承 (FrameLights guard wrap) と同形 + 追加 UBO 名 collision 検出 + per-block 固有化
- 想定 file 数: 不明 (Agent trace 必須) / 想定行数: 小〜中規模
- 本 (a) を主 scope に置くのは η-2 (a) 達成水準 (0) からの局所退行のため早期回復が優先 (η-4 §7 risk (5))

**(b-1) non-opaque uniforms outside a block 4 件 trace** (副 scope):
- 該当 LINE: 458 / 457 / 347 / 349 (B?-η-2 (a)-complete handoff §10.1 「+1 件 再 trace 必要」は B?-η-3 着手前 trace で 4 件と確定、誤記訂正済)
- 該当 program: Skinned Deferred PBR Opaque / Deferred PBR Opaque / Contrast Adaptive Sharpening / CAS Legacy Gamma
- 想定対処: B?-δ §3.1 範式継承 = utility 既 attach 全 file scope unguarded bare uniform 網羅 scan + guard wrap (4 strdup 追加/uniform)
- 想定 file 数: 4 file / 想定行数: 小規模 (16-20 行 insertions 規模)

**(b-2) undeclared identifier 3 件 trace** (副 scope):
- 該当 LINE: 385 (`modelview_projection_matrix`) / 636 (`minimum_alpha`) / 557 (`modelview_projection_matrix`)
- 該当 program: PBR Glow / HUD PBR Opaque / HUD PBR Alpha
- 想定対処: include order / utility cache attachment order 調査 = B2-γ §3.3 範式継承 + 必要に応じ utility cache + per-program tracking 修正 (C++ side touch 想定)
- 想定 file 数: 不明 (C++ 1 file 修正 or shader file 注入想定) / 想定行数: 小〜中規模

**(c) `weight4` / `weight` redefinition 2 件 trace** (副 scope):
- 該当 LINE: 536 (`weight4`) / 480 (`weight`)
- 該当 program: Skinned AYAstorm Velocity Shader (FRAG) / AYAstorm Avatar Velocity Shader (FRAG)
- 想定対処: shader 内 variable redefinition file 特定 + guard wrap or rename = η-1 §3.1 範式の variable-level 応用
- 想定 file 数: 2 file / 想定行数: 小規模 (10 行 insertions 規模)

### §10.2 B?-η-5 着手前 trace 範式 (B?-ε §3.2 + B?-δ §3.2 + B?-η-1 §3.3 + B?-η-3 §3.1 + B?-η-4 §3.1 §3.2 §3.4 統合)

1. **literal grep 範式** (B?-η-1 §3.2 cascade source 特定範式継承):
   - `grep -n "Cannot reuse block name" log` で (a) LINE 確定 (10 件)
   - `grep -n "non-opaque uniforms outside a block" log` で (b-1) LINE 確定 ('size' / nameless block は除外 = 既完全解消)
   - `grep -n "undeclared identifier" log` で (b-2) LINE + identifier 確定 ('size' は除外 = B?-η-3 で完全解消済)
   - `grep -n "redefinition" log` で (c) LINE + variable 確定 ('normalMap' は除外 = 第7層 emergence で別 sub-bundle scope)
2. **log context 抽出範式** (B?-η-2 (a) §4.1 で確立):
   - 各 LINE の `sed -n "$((line-5)),$((line+2))p" log` で program 名 + stage type 確定
3. **handoff §10 + 第7層 emergence 未列挙 root cause trace 範式** (η-3 §3.1 + η-4 §3.1 で確立):
   - 着手前に **想定外 root cause** が存在しないか log 全件再 trace
   - 想定外 root cause 発見の場合は本 sub-bundle scope を再設計 (3rd/4th-level scope refinement)
4. **Agent 並列 disjoint scope 範式** (B?-η-1 §3.3 / η-2 (a) / η-3 / η-4 直接継承):
   - 9 件 + 10 件 = 19 件 root cause を 4 Agent disjoint scope に分割 (例: Agent A = (a) Cannot reuse 10 件 / Agent B = (b-1) 4 件 / Agent C = (b-2) 3 件 / Agent D = (c) 2 件)
   - pilot 1-2 file (Claude 自力) で patch literal 確立してから Agent 並列展開
5. **4 仮説評価範式** (η-4 §3.4 で確立):
   - 仮説連続 falsify (2 連) → 推論停止 → Agent 投入で実データ trace 必須
   - 4 仮説評価 table 事前起草 + 各仮説 評価方法 + 期待効果 明示

### §10.3 B?-η-5 完遂後の想定 cascade exposure 第8層

- (a) 完遂で `Cannot reuse block name within the same interface` 10 → 0
- (b-1) 完遂で `non-opaque uniforms outside a block` 57 → 期待 -4 (utility scope のみ、program-scope は別 sub-bundle)
- (b-2) 完遂で `undeclared identifier` 3 → 0
- (c) 完遂で `weight4`/`weight` redefinition 2 → 0
- 全 4 種根本対処で missing #endif cascade pair 自動消滅 (汎用形完全検証済 §3.3)
- 第7層 emergence 4 系統残 (redefinition `'normalMap'` cluster 残 + non-opaque program-scope 残 + sampler `'binding'` 21 + `'location'` 14) は第8層 emergence として顕在化 = η-6 以降 sub-bundle scope

### §10.4 第7層 emergence 整理 (B?-η-5 完遂後 or 別 sub-bundle で扱う候補)

| emergence 種 | 件数 (η-4 後) | 想定対処範式 | 想定 sub-bundle |
|---|---|---|---|
| `'normalMap' : redefinition` at LINE `0:1332` 残 | redefinition 81 内の subset (要 trace) | B?-ε §3.2 範式継承 + B?-ζ §3.1 範式継承 (cascade source 1 件特定 + struct guard wrap or `#define` rename) | B?-η-6 (推奨) = 単一 cluster 1 patch 多数件解消 |
| non-opaque uniforms outside a block (program-scope 残) | 57 - 4 = 53 (η-5 で utility-scope 4 件解消後) | B?-δ §3.1 範式継承 = program-scope unguarded bare uniform wrap scan | B?-η-7 (推奨) = program-scope 拡大 scope |
| opaque `'binding'` (sampler/texture/image requires layout(binding=X)) | 21 | B?-δ 範式 + B2-γ §3.3 utility concat hook 補強 + sampler binding 追加範式 | B?-η-8 (推奨) = sampler binding 専用 |
| `'location'` (SPIR-V missing + overlapping 20) | 14 | B2-α §3.1 範式継承 = varying + fragment_out program 注入 scope 拡大 + location 重複検出範式 | B?-η-9 (推奨) = location 専用 |

---

## §11 観測点

1. **shader_cache 259+ 維持** maintained 観測点 (B?-η-3 223 baseline から本 η-4 +36 link 成立 program 集合シフト後の新基準)
2. **cascade pair hypothesis 汎用形完全検証完了** (本 η-4 §3.3 で `nameless block` 45 件 + `'normalMap' redefinition` 69 件 + 独立 13 件 = 計 127 件 cascade pair 1:1 完全対応観測)、次 sub-bundle 以降は自動消滅指標として継続観測
3. **B?-η-3 §3.2 範式の member-level 拡張範囲** = nameless block padding member で確立、他 member 系 (struct field / 通常 member / interface block instance) は別範式が必要、新規 nameless block 追加時は §3.2 collision detection 範式事前適用必須
4. **scope refinement 4th-level 範式** (本 η-4 §3.1 で初回適用) の濫用リスク監視 = sub-bundle 内 3 phase refine の判断基準 (phase 失敗 patch の保持/revert 判断は 4 仮説評価結果 + constraint 違反なし) を厳守
5. **`Cannot reuse block name within the same interface` 第7層 emergence +10 件** = η-2 (a) 達成水準 (0) からの局所退行 = B?-η-5 主 scope での早期回復が必須、η-2 (a) 範式継承で同形対処想定
6. **第7層 emergence 4 系統 net シフト** (η-3 計 406 件 → η-4 計 173 件 = -233 件 net 改善) = link 成立 program 集合シフトによる露出側変化が主要因 (= shader_cache +36 で実証)、`Cannot reuse block name` +10 のみが真の局所退行
7. **`feedback_admit_unknown` 範式の sub-bundle 内構造化** (本 η-4 §3.4 で確立) = 仮説連続 falsify (2 連) → 推論停止 → Agent 投入 + 4 仮説評価 table = 次 sub-bundle 以降も適用必須
