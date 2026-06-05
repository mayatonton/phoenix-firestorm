# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B1 完遂 → B2 着手境界 handoff (2026-06-01)

**parent commit**: `5614494f56` (B1 patch、本 handoff の直接 parent) / `8d2cae435b` (A7-complete handoff doc commit 範式継承元 source-of-truth)
**HEAD**: `5614494f56` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B1 完遂状態 + B2 (location 195 件解消) 着手境界 を fresh context 引継 用に確定する doc-only handoff。A7-complete `8d2cae435b` 範式継承。**案 2 = file-local override MaterialUBO_Legacy 範式 (A6 PerDrawUBO 6 layout pattern 範式継承) を本 B1 で適用**。bundle-B/C scope (per-program SPIR-V parse error 根本解消) の最初の sub-bundle として、materialF.glsl bare uniform 7 件を MaterialUBO_Legacy に集約し non-opaque error 32 件を完全消滅。AYA 「commit して」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 初 sub-bundle B1 (materialF.glsl bare uniform UBO 化) を 1 file +21 行に注入完遂した状態を確定し、B2 (Material Shader 32 program が cascade した location error 195 件解消) 着手境界 を fresh context に引継ぐ。materialF.glsl (class3/deferred、Material Shader 0-31 全 32 program 共有) の bare uniform 7 件 (`emissive_brightness` / `morphFactor` / `camPosLocal` / `is_mirror` / `env_intensity` / `specular_color` / `aya_sss_skin_flag` = AYAstorm 改変) を `set=1/binding=0` MaterialUBO_Legacy 専用 block (canonical MaterialUBO とは別 std140 layout) に集約。**案 2 = file-local override 範式 (A6 PerDrawUBO 6 layout pattern 範式継承)** を採用し、canonical MaterialUBO (A4 確立) には影響を与えず materialF.glsl のみ独自 layout で対応。**non-opaque error 32 → 0 完全消滅** = B1 patches 効果完全直接観測 (Material Shader first-error semantics により 1 件 bare uniform 解消で 32 program 全 non-opaque error 消滅)。location は +32 cascade (32→32 program が次層 error に進行)、bundle-B 全体閾値で評価。

---

## §2 B1 完遂 status

| 項目 | 値 |
|---|---|
| Scope | materialF.glsl bare uniform UBO 化 (Material Shader 0-31 全 32 program 共有 file) |
| 注入 file 数 | **1 file** (class3/deferred/materialF.glsl のみ、class1 は debug stub で uniform 0 件) |
| 変更行数 | **+21 / 0** (insertions-only、deletion 0 件) |
| 3-段 swap pattern balanced | ifdef/ifndef/if 26→31 (+5 pair) / endif 26→31 (+5 pair) 整合、1 新規 ifdef-else-endif + 4 新規 ifndef-endif pair |
| variant pattern `defined(LL_VULKAN_GLSL)` | **0 件** |
| Agent 投入 | 直接 patch (file 1 件、bare uniform 7 件 = manual patch で十分) |
| canonical MaterialUBO (A4 確立) untouched | **違反 0 件** (case 2 file-local override で canonical 不変) |
| skip list 13 file untouched | **全 untouched** |
| AYAstorm 改変 `aya_sss_skin_flag` 保全 | **GL path #else 分岐に bare uniform 保全 + Vulkan path UBO 内 member 化** (A8-recovery 範式継承) |
| AYA cold cache launch verify | **PASS** (起動成立 06:45:39 → 06:46:25 ~46 秒 + clean shutdown + shader_cache 224 件再生成 + crash 0 + GL shader compile/link fail 0) |
| commit | `5614494f56` (AYA 「commit して」明示指示下) |
| metric vs A7 baseline non-opaque | **-32 ✓ B1 patches 効果完全直接観測** = materialF.glsl bare uniform 7 件 UBO 化 → Material Shader 0-31 全 32 program non-opaque error 完全消滅 |
| metric net delta | **0 errors** (non-opaque -32 + location +32 cascade) = controlled cascade (root cause 完全解消、next layer に進行) |

### §2.1 patch 内訳 (5 edit、1 file)

| # | Line | 操作 | 対象 uniform |
|---|---|---|---|
| 1 | 37 | 3-段 swap (MaterialUBO_Legacy 新規 + #else bare 保全) | `emissive_brightness` |
| 2 | 129-130 | `#ifndef LL_VULKAN_GLSL` wrap | `morphFactor` + `camPosLocal` |
| 3 | 135 | `#ifndef LL_VULKAN_GLSL` wrap | `is_mirror` |
| 4 | 278-279 | `#ifndef LL_VULKAN_GLSL` wrap | `env_intensity` + `specular_color` |
| 5 | 284 | `#ifndef LL_VULKAN_GLSL` wrap (AYAstorm 改変) | `aya_sss_skin_flag` |

### §2.2 既処理 sub-bundle との関係

materialF.glsl は A4 (MaterialUBO sampler set=1/binding=1, 3, 6) + A5 (extension sampler) + A6 (PerDrawUBO?) で **sampler のみ touch**、bare uniform は未 touch だった (A4 当時の case 2 file-local override 範式未確立)。B1 で bare uniform 7 件を補正。

| sub-bundle | materialF.glsl touch 内容 |
|---|---|
| A4 | `diffuseMap` (set=1/binding=1) + `bumpMap` (set=1/binding=6) + `specularMap` (set=1/binding=3) sampler 3-段 swap |
| A5 | extension sampler (`environmentMap` set=0/binding=6 + `lightFunc` set=0/binding=5) 3-段 swap |
| A6 | PerDrawUBO は materialF.glsl 内未参照のため untouched |
| A7 | bare uniform 漏れ candidate に含まれず (case 1(b) screen_res 系のみ補正) |
| **B1** | **bare uniform 7 件 → MaterialUBO_Legacy 集約 (file-local override)** |

---

## §3 binding rule §2 補正 literal canonical (B1 で確定)

### §3.1 案 2 = file-local override MaterialUBO_Legacy 範式 (A6 PerDrawUBO 範式継承)

**設計原則**: file 固有の uniform set を canonical UBO に追加せず、file-local な独自 std140 layout で `set=1/binding=0` を上書き定義。per-program descriptor set のため runtime binding 衝突なし。canonical MaterialUBO (A4 確立) に member を追加すると全 PBR file に影響するが、materialF.glsl 専用 uniform (legacy 系 fullbright/morph/specular/mirror/env/AYAstorm SSS marker) を canonical に追加するのは設計汚染。案 2 で隔離。

**範式継承元**: A6 PerDrawUBO 6 layout pattern (α/β/γ/δ/ε/ζ) で file-local override が確立済 (skinning matrixPalette vec4[45] / per-light color vec4 / clipPlane vec4 等が file-local layout で `set=2/binding=0` を共有)。

### §3.2 MaterialUBO_Legacy literal canonical (B1 で確立)

```glsl
#ifdef LL_VULKAN_GLSL
layout(set=1, binding=0, std140) uniform MaterialUBO_Legacy {
    vec4  morphFactor;          // offset 0,  size 16
    vec4  specular_color;       // offset 16, size 16
    vec3  camPosLocal;          // offset 32, size 12
    float emissive_brightness;  // offset 44, size  4 (vec3 padding slot 活用)
    float is_mirror;            // offset 48, size  4
    float env_intensity;        // offset 52, size  4
    float aya_sss_skin_flag;    // offset 56, size  4 (AYAstorm 改変)
    float _pad_legacy_0;        // offset 60, size  4 (16-byte align 担保)
};
#else
uniform float emissive_brightness;  // fullbright flag, 1.0 == fullbright, 0.0 otherwise
#endif
// + 4 別箇所 #ifndef LL_VULKAN_GLSL wrap で morphFactor/camPosLocal/is_mirror/env_intensity/specular_color/aya_sss_skin_flag を GL path 保全
```

**std140 layout 検算**:
- vec4 morphFactor: alignment 16, size 16, offset 0-16
- vec4 specular_color: alignment 16, size 16, offset 16-32
- vec3 camPosLocal: alignment 16, size 12, offset 32-44
- float emissive_brightness: alignment 4, size 4, offset 44-48 (vec3 直後 4-byte padding slot に float 配置可)
- float is_mirror: 48-52
- float env_intensity: 52-56
- float aya_sss_skin_flag: 56-60
- float _pad_legacy_0: 60-64

**total 64 byte** (16-byte 倍数で次 struct 配置可)

### §3.3 3-段 swap pattern 範式継承

A1-A7 共通の 3-段 swap pattern を B1 でも適用:

```glsl
// 案 1: 1 件 UBO 新規 + bare 保全
#ifdef LL_VULKAN_GLSL
layout(...) uniform MaterialUBO_Legacy { ... };
#else
uniform float emissive_brightness;
#endif

// 案 2: 既 bare uniform を GL path のみ保全
#ifndef LL_VULKAN_GLSL
uniform vec4 morphFactor;
uniform vec3 camPosLocal;
#endif
```

variant `defined(LL_VULKAN_GLSL)` 0 件、`#ifdef LL_VULKAN_GLSL` / `#ifndef LL_VULKAN_GLSL` のみ使用。

---

## §4 cold cache launch verify metric (2026-06-01)

### §4.1 base metric

| metric | A7 baseline | B1 (current) | Δ |
|---|---|---|---|
| hook fire (parse 試行) | 226 | 224 | -2 |
| parse failed | 224 | 224 | ±0 (Material Shader 32 program は次層 cascade、他 program 不変) |
| location | 163 | **195** | **+32 cascade** (Material Shader 32 program が next-error 層 location に進行) |
| binding | 29 | 29 | ±0 (B1 scope 外) |
| non-opaque | **32** | **0** | **-32 ✓ B1 target 完全達成** |
| missing #endif | 8 | 8 | ±0 (cascade error、location 解消で自然消滅予定) |
| shader_cache 再生成 | 224 | **224** | ±0 (GL path regression 0) |
| link failed | 0 | 0 | ±0 |
| FATAL | 0 | 0 | ±0 |
| SIGSEGV | 0 | 0 | ±0 |
| crash | 0 (benign 4) | 0 (benign 3) | ±0 |

### §4.2 起動成立 + clean shutdown

- 起動: `2026-06-01T06:45:39Z` → 起動完了 ~06:45:46 (~7 秒)
- shutdown 開始: `2026-06-01T06:46:25Z`
- 全 session: ~46 秒
- `Vulkan device destroyed` + `Vulkan instance destroyed` + `Goodbye!` 全 trace 確認
- charter §3 #1 acceptance 担保

### §4.3 net delta 解釈

**non-opaque -32 + location +32 = net ±0** だが意味は重要:
- materialF.glsl root cause = bare uniform は **完全消滅** (Material Shader first-error semantics で 1 件解消 → 32 program 全消滅)
- glslang parser は次 error 層 (location declaration) に進行 = Material Shader 32 program が next error に cascade
- cascade は bundle-B 設計通り = B1 で root cause を 1 層解決、B2 で次層 (location) を解決
- net 0 = controlled cascade、improvement の段階的進行 (bundle-A A7 で binding -30 + location +24 と同 pattern)

### §4.4 crash 内訳 (benign 3 件)

| line | source | 性質 |
|---|---|---|
| 11 | `CrashSettings` group load INFO | benign 設定読込 |
| 28 | `CrashSettings` group load INFO (User location) | benign 設定読込 |
| 39 | `CrashSettings` group load INFO (User location 2nd) | benign 設定読込 |

SIGSEGV / FATAL 実体 **0 件**、A7 baseline 同 pattern。

### §4.5 binding 数 discrepancy 観察 (handoff doc claim vs current log)

A7-complete handoff doc は binding 30→0 (-30) と記録、current B1 log は binding 29、A7 measurement と B1 measurement の差は cache/measurement design 影響と推定。本 B1 verify では A7 → B1 で binding 29→29 ±0 が直接観測値であり、B1 scope (non-opaque) と直交、bundle-C 全体閾値で再評価予定。差異原因究明 task #2 で完了済。

---

## §5 self-verify 結果

| 項目 | 検査 | 結果 |
|---|---|---|
| (a) bare uniform 残検出 (LL_VULKAN_GLSL guard 外) | awk depth-aware scan | **0 件** ✓ |
| (b) ifdef/ifndef/if vs endif count | grep -cE | 31 = 31 ✓ balanced |
| (c) insertions-only | git diff --stat | **+21 / 0** ✓ deletion 0 |
| (d) GL path #else 分岐 byte-for-byte 維持 | git diff 確認 | ✓ 既存 declaration 全保全 (charter §3 #1 担保) |
| (e) canonical MaterialUBO (A4) 影響 | grep MaterialUBO across 全 PBR file | ✓ 0 件影響 (case 2 file-local override) |
| (f) AYAstorm 改変 `aya_sss_skin_flag` 保全 | コメント `<FS:AYA r20 Phase C>` 隣接 untouched | ✓ wrap 注入のみ、機能特性 untouched (A8-recovery 範式) |
| (g) skip list 13 file untouched | git diff -- skip list paths | ✓ 全 untouched |
| (h) std140 layout 検算 | offset 0-64 byte 計算 | ✓ alignment rule 準拠 (vec3 + float padding slot 活用) |

---

## §6 設計範式 (A7 継承 + B1 で確認)

### §6.1 案 2 = file-local override 範式 (B1 で適用、A6 範式継承)

- file 固有 uniform 集合 (legacy/AYAstorm 改変 等 canonical 対象外) は canonical UBO に追加せず、file-local な独自 std140 layout で binding 上書き
- per-program descriptor set のため runtime 衝突なし
- canonical UBO への副作用 0 = 他 file 影響 0
- AYAstorm 改変 uniform を UBO 内 member 化することで GL path 保全と Vulkan 通過両立 (A8-recovery 範式継承)

### §6.2 案 D vs 案 2 範式選択指針

| 範式 | 適用条件 | 例 |
|---|---|---|
| 案 D (canonical 拡張) | 複数 file が同 entity を参照、canonical 拡張で全 file 一括対応可 | A7 FrameAtmosphere haze_horizon/gamma (5 file) / MaterialUBO metallicFactor (2 file) |
| 案 2 (file-local override) | 単一 file 固有 uniform、canonical 追加は設計汚染 | B1 materialF.glsl 7 uniform (Material Shader 32 program 共有 1 file) |

### §6.3 範式継承確認

- 3-段 swap pattern (A1-A7 共通): ✓ B1 適用
- conditional injection (A2-A7): ✓ B1 では全 7 uniform が全 program で参照されるため conditional 不要
- AYAstorm 改変保全 (A8-recovery): ✓ B1 適用 (`aya_sss_skin_flag`)
- variant pattern 禁止 (A1-A7): ✓ B1 0 件
- charter §3 #1 GL path byte-for-byte (A1-A7): ✓ B1 +21/-0

---

## §7 risks/caveats

### §7.1 location +32 cascade

Material Shader 32 program が non-opaque で early-fail していたものが next error layer (location) に進行。B1 scope では handled、B2 で location 解消時に Material Shader 32 program も同時解消予定。bundle-B 全体閾値で評価。

### §7.2 案 2 case-by-case 設計判断

案 2 は file-local 隔離だが、将来 maintainer が canonical MaterialUBO と MaterialUBO_Legacy の 2 種類が存在することに混乱する可能性あり。本 handoff doc + binding rule artifact §2 (E) で literal 明示記録要。

### §7.3 MaterialUBO_Legacy binding=0 共有

set=1/binding=0 は canonical MaterialUBO (A4) と同 binding 値。per-program descriptor set のため runtime 衝突なし (Vulkan descriptor set は program 単位で binding mapping)。documentation 上は同 binding 値共用を明示要。

### §7.4 vec3 + float padding slot 活用 (std140)

`camPosLocal` (vec3, offset 32-44) 直後の 4-byte padding slot を `emissive_brightness` (float, offset 44-48) で活用。GL 4.6 spec 準拠だが、driver implementation によっては layout 違反扱いの可能性 → cold cache launch verify で実機確認済 (NVIDIA 570.211.01 PASS)。

### §7.5 AYAstorm 改変 `aya_sss_skin_flag` UBO 内同居

A8-recovery 範式は AYAstorm 改変を canonical UBO 末尾に member 追加で同居させる pattern を確立。B1 では canonical 不変 (案 2) のため、独自 MaterialUBO_Legacy 内に AYAstorm member を同居。A8-recovery 範式の variant として案 2-AYAstorm 同居 pattern を本 B1 で確立。

### §7.6 cold cache launch 必須

`rm -rf ~/.ayastorm_x64/cache/shader_cache/` を sub-bundle 毎 verify 前必須化。本 B1 でも実施済 (削除前 224 file → 削除確認済 → 再生成後 224 file)。

### §7.7 起動時間 短縮 (46s vs A7 64s)

texture/asset cache 残存差で起動時間が短縮、shader 計測には影響なし (shader_cache 224 file 再生成 + hook fire 224 で同等条件確認)。

### §7.8 case 2 file-local override の影響範囲

A6 PerDrawUBO 6 layout pattern + B1 MaterialUBO_Legacy = file-local override 範式は今後も bundle-B/C で増加予定。binding rule artifact §2 に file-local layout 一覧 section 追加要 (bundle-B 全体完遂時に整理)。

---

## §8 B2 着手境界

### §8.1 B2 scope

**location SPIR-V layout 注入 (195 件)** = bundle-B 第 2 sub-bundle

- error pattern: `'location' : SPIR-V requires location for user input/output`
- 直接観測: line 36-42 dominant (top-of-fragment-shader `in`/`out` declarations)
- 対象 program: ~150-180 program (Material Shader 32 + Deferred Diffuse/Alpha/Fullbright 多数 + UI/Highlight/Pathfinding 系)
- 解消策: GLSL `in`/`out` declarations に `layout(location=N)` 注入 (3-段 swap pattern)

### §8.2 B2 prep 着手前チェックリスト

1. ~~bundle-A 全 7 sub-bundle 完遂確認~~ ✓ A1-A7 + A8-recovery
2. ~~B1 完遂確認~~ ✓ `5614494f56`
3. location 195 件の file × program × in/out variable 分類 (Agent 並列 scan 推奨)
4. SPIR-V `layout(location=N)` 番号割当 ruleset 設計 (vertex out → fragment in pairing、N 番号は file 間整合要)
5. 3-段 swap pattern 適用 (`#ifdef LL_VULKAN_GLSL` `layout(location=N) in/out` `#else` `in/out` `#endif`)
6. case 1(a) / case 1(b) 範式継承 (A1 trace 範式)
7. canonical layout=N 番号 binding rule artifact §3 (新規) に literal 記録

### §8.3 B2 cadence 8 step (A7-prep §10 範式継承)

1. **trace**: location 195 件の file × program × in/out 変数分類 (Agent 並列 scan 推奨)
2. **prep**: layout(location=N) 番号割当 ruleset + 3-段 swap pattern 適用方針 AYA 提示
3. **patch** [AYA OK]: file 単位 patch (in/out pair 整合確認込み)
4. **verify**: shader cp + cache clear + AYA cold cache launch verify
5. **handoff**: error metric 取得 (location -195 期待、cascade で次層 binding 露出可能性あり)
6. **measurement**: vs B1 baseline delta 取得 (location -X / binding +Y cascade / non-opaque ±0 / missing #endif ±0)
7. **commit** [AYA OK]
8. **handoff doc draft**: B2-complete handoff doc 起草 (本 B1-complete 範式継承)

### §8.4 B2 推定規模 (B1 比 大規模)

- file 数: ~100-150 unique GLSL file (vertex/fragment shader 全般)
- 変更行数推定: +500-1000 (各 file +5-10 行の in/out wrap)
- Agent 並列 scan 推奨 (B1 単独 patch とは規模差)
- sub-bundle 分割可能性: B2 自体を B2-α (Deferred/PBR shader) + B2-β (UI/Highlight/Post shader) 等に細分化検討

---

## §9 charter §3 #1 acceptance 担保

- GL path (`#else` 分岐内 既存 uniform 列 byte-for-byte 維持): ✓ git diff で確認、deletion 0 件
- AYAstorm 独自改造意図保全 (`aya_sss_skin_flag` r20 Phase C SSS marker 機能特性そのまま): ✓ コメント untouched + wrap 注入のみ
- canonical MaterialUBO (A4 確立) untouched: ✓ case 2 file-local override で 0 件影響
- skip list 13 file untouched: ✓ 全 untouched
- 段階 1-4.3-γ'-port-β-2-bundle-A-A7 動作維持: ✓ cold cache launch verify PASS (起動成立 + clean shutdown + shader_cache 224 再生成 + crash 0 + GL shader compile/link fail 0)

---

## §10 cross reference

### §10.1 handoff doc 系譜

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A1-complete.md` 〜 `A7-complete.md` (A1-A7 完遂 handoff 系譜)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A8-recovery-complete.md` (AYAstorm 改変 recovery)
- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md` (bundle-A 全体 prep)
- `handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md` `fcf2b6c508` (β-2-hook 完遂 handoff)
- **本 doc**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B1-complete.md` (B1 完遂、本 handoff)

### §10.2 patch commit 系譜

- `6f941c0a48` (A1) / `9f77f875db` (A2) / `ebd5e2b16d` (A3) / `f703710f8d` (A4) / `eddcc7ac40` (A5) / `b80d90bea4` (A6) / `862f7dc8bd` (A7) / `aed1438936` (A8-recovery) / `8d2cae435b` (A7-complete handoff doc) / **`5614494f56` (B1)** ← 本 commit

### §10.3 spec sub-doc

- `docs/specs/ayastorm-r41-gl-removal/sub-doc-06-bundle-A-design.md` §1.2.4 set=1/binding=0 baseline (canonical MaterialUBO)
- `docs/specs/ayastorm-r41-gl-removal/sub-doc-07-bundle-B-design.md` §3.1 sub-step 7.3 location SPIR-V layout
- `docs/specs/ayastorm-r41-gl-removal/charter.md` §3 #1 acceptance + §7.5

### §10.4 artifact

- `/tmp/bundle-A-binding-rules.md` (36906 bytes、A1-A7 binding rule canonical persist)
- `docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt` (1500 bytes、13 file skip list)
- **新規予定**: binding rule artifact §2 (G) MaterialUBO_Legacy literal canonical (file-local override 一覧 section)

### §10.5 feedback rules 適用確認

- `feedback_proactive_handoff` ✓ context 圧迫前に B1-complete doc 起草
- `feedback_self_verify_before_handoff` ✓ self-verify 8 項目全実施 + AYA cold cache launch verify
- `feedback_use_agents_proactively` (B1 は単独 patch 妥当規模、B2 は Agent 並列 scan 必要)
- `feedback_no_scope_shrink` ✓ AYA 「案 2 で進めて」明示指示 → 全 7 uniform 一括 patch
- `feedback_doubt_self_first` ✓ cache 削除疑念 → 即 verify (shader_cache 削除済 + 再生成 224 確認)
- `feedback_admit_unknown` (該当事象なし)
- `feedback_falsification_as_progress` ✓ non-opaque -32 = root cause 完全消滅 + location +32 cascade = controlled improvement
- `feedback_explanation_lead_with_conclusion` ✓ 全 status 報告で結論先行
- `feedback_no_claude_coauthor` ✓ commit message に Claude 共著行なし
- `feedback_no_auto_commit` ✓ AYA 「commit して」明示指示下で commit
- `feedback_one_step_at_a_time` ✓ trace → prep → patch → verify → commit を順次実行
- `feedback_remove_verification_logs` (B1 では verification log 追加なし)
- `feedback_build_only_verified` ✓ cold cache launch verify で実測確認後 commit

---

## §11 次 session 投入 prompt (fresh context 推奨)

```
AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2 (location SPIR-V layout 注入 195 件解消) に着手境界。

【必須読了 handoff doc】
1. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B1-complete.md (本 doc、B1 完遂 + B2 着手境界)
2. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-A7-complete.md (A7 完遂 + bundle-A 全体完遂 + 案 D 範式確立)
3. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-A-prep.md (bundle-A 全体 prep + binding rule (A)-(H))
4. docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-hook-complete.md (per-program SPIR-V hook 仕組)

【必須参照 artifact】
- /tmp/bundle-A-binding-rules.md (A1-A7 binding rule canonical)
- docs/specs/ayastorm-r41-gl-removal/bundle-A-skip-list.txt (13 file skip)

【B1 完遂状態】
- HEAD: `5614494f56` (B1 commit) / `8d2cae435b` (A7-complete handoff doc)
- B1 patch: class3/deferred/materialF.glsl +21/-0 (MaterialUBO_Legacy 案 2 file-local override 範式)
- 効果直接観測: non-opaque error 32 → 0 完全消滅、Material Shader 32 program 全 non-opaque 解消
- cascade: location 163 → 195 (+32、Material Shader 32 program が次層 location error に進行)

【B2 scope】
- location SPIR-V layout 注入 195 件解消 (line 36-42 dominant = top-of-fragment in/out declarations)
- 対象 ~150-180 program (Material Shader 32 + Deferred Diffuse/Alpha/Fullbright 多数 + UI/Highlight/Pathfinding 系)
- 3-段 swap pattern (`#ifdef LL_VULKAN_GLSL` `layout(location=N) in/out` `#else` `in/out` `#endif`)
- file 数推定 100-150、+500-1000 行規模 (Agent 並列 scan 推奨、B2-α/β 細分化検討)

【hard rule 7 件 (A7-complete §6 + B1-complete §6 継承)】
1. 3-段 swap pattern only (variant `defined(LL_VULKAN_GLSL)` 禁止)
2. GL path #else 分岐 byte-for-byte 維持 (insertions-only)
3. AYAstorm 改変 untouched + UBO 内 member 化で wrap (A8-recovery 範式)
4. skip list 13 file untouched
5. 既処理 file の既存 UBO block byte-for-byte 維持 (std140 末尾追加 ABI 互換例外のみ可、案 D 範式)
6. canonical UBO (A1-A7 確立) は新規 entity 追加で拡張可、file-local override (案 2 範式) は file 固有 uniform 集合に適用
7. cold cache launch verify 必須 (`rm -rf ~/.ayastorm_x64/cache/shader_cache/`)

【cadence 8 step (B2 適用)】
1. trace → 2. prep → 3. patch [AYA OK] → 4. verify → 5. handoff (shader cp + cache clear + AYA launch verify) → 6. measurement → 7. commit [AYA OK] → 8. handoff doc draft

【feedback rules 13 件】 user_address / feedback_proactive_handoff / feedback_self_verify_before_handoff / feedback_use_agents_proactively / feedback_no_scope_shrink / feedback_doubt_self_first / feedback_admit_unknown / feedback_falsification_as_progress / feedback_explanation_lead_with_conclusion / feedback_no_claude_coauthor / feedback_no_auto_commit / feedback_one_step_at_a_time / feedback_remove_verification_logs / feedback_build_only_verified

【1st action】
B2 scope 着手前に AYA「OK」確認、B1 完遂状態 + B2 scope (location 195 件解消) 認識 1 行 status 報告。
```

---

**本 doc 完遂で B1 → B2 引継準備完了。AYA 明示指示下で本 handoff doc commit、その後 fresh context で B2 着手。**
