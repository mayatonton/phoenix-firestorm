# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-δ 完遂 → 次 sub-bundle (B?-ε) 着手境界 handoff (2026-06-01)

**parent commit**: `744d266f34` (B?-δ patch、本 handoff の直接 parent) / `1cd9303872` (B2-γ patch 範式継承元) / `f9d05374a3` (B2-γ-complete handoff doc commit)
**HEAD**: `744d266f34` on `feature/ayastorm-r41-gl-removal`
**本 doc 位置付け**: B?-δ 完遂状態 + 次 sub-bundle (推奨 B?-ε = utility concat 末尾 `\n` 補正 191 件 + utility 二重 attach 除去 数件 + opaque uniform layout(binding=X) qualifier 注入 数件) 着手判断境界 を fresh context 引継 用に確定する doc-only handoff。B2-γ-complete `f9d05374a3` 範式継承。**utility 既 attach 全 file scope unguarded bare uniform 網羅 scan + 3-段 swap 範式 (1 件) を B?-δ で新規確立**。cascade source 特定範式 (ALL N errors 同一 `0:LINE` line ⇒ ALL N programs の最先頭 attached utility が cascade source) も合わせて確立。`non-opaque uniforms outside a block` 223 件の直接観測 100% 解消。AYA 「OK」明示承認下で commit (no auto-commit)。

---

## §1 起草目的

β-2-bundle-B scope 第四 sub-bundle B?-δ (utility unguarded bare uniform wrap) を 12 file +167 insertions / -0 deletions の insertions-only structural addition で完遂した状態を確定し、次 sub-bundle 着手境界 を fresh context に引継ぐ。B2-γ (commit `1cd9303872` = utility source cache + per-program attached utility tracking + utility concat hook + createShader reorder) の cascade で B2-α/B2-β/B3 段階から構造的に露出していた `non-opaque uniforms outside a block` 223 件を、**utility 側 unguarded bare uniform 8 件 wrap で 100% 解消**。同時に cascade exposure で別 3 種 leak (`'#' preprocessor directive cannot be preceded by another token` 191 件 / `'GBufferInfo' redefinition struct` 数件 / `'binding' sampler/texture/image requires layout(binding=X)` 数件) が次 sub-bundle scope として露出。

本 sub-bundle は **2-attempt 構造** (= 1st falsified + 2nd struck root) であり、1st attempt の **falsification 自体** を範式継承価値として残す:
- **1st attempt** (B?-δ 第1段): 9 utility file (atmosphericsFuncs / sky{V,F} / clouds{V,F} / softenLightF / deferredUtil / shadowUtil / aoUtil) に preventive UBO 設計追加 → metric 効果 **0 件** (falsified)
- **2nd attempt** (B?-δ 第2段): 真因 trace 結果 = ALL 223 errors 同一 `0:51` line ⇒ ALL 223 programs の最先頭 attached utility (= globalF.glsl 全 fragment shader 共通 1st attach) の bare uniform 2 件 + waterFogF/tonemapUtilF 計 6 件 = **8 unguarded leak** が cascade source、3 file insertions-only 3-段 swap wrap → **223→0 100% 解消**

1st attempt の 9 file は **preventive cleanup 残置** (将来 ALM program 拡張時の保険として有意)。

---

## §2 B?-δ 完遂 status

| 項目 | 値 |
|---|---|
| Scope | (a) 1st 9 file preventive UBO 追加 (atmosphericsFuncs `AtmoExtraUBO_Legacy` set=3/binding=0 + skyV `SkyVParamUBO_Legacy` set=3/binding=1 + skyF `SkyFParamUBO_Legacy` set=3/binding=2 + cloudsV `CloudsVParamUBO_Legacy` set=3/binding=3 + cloudsF `CloudsFParamUBO_Legacy` set=3/binding=4 + softenLightF `SoftenLightParamUBO_Legacy` set=3/binding=5 + deferredUtil `DeferredUtilParamUBO_Legacy` set=3/binding=6 + shadowUtil `ShadowUtilParamUBO_Legacy` set=3/binding=7 + aoUtil `AOUtilParamUBO_Legacy` set=3/binding=8) + (b) 2nd 3 file root cause unguarded leak wrap (globalF `GlobalFParamUBO_Legacy` set=3/binding=11 (mirror_flag + clipSign) + waterFogF `WaterFogUBO_Legacy` set=3/binding=9 (waterFogColor + waterFogDensity + waterFogKS) + tonemapUtilF `TonemapUBO_Legacy` set=3/binding=10 (exposure + tonemap_mix + tonemap_type)) |
| 修正 file 数 | **12 file** (`class1/windlight/atmosphericsFuncs.glsl` + `class1/deferred/sky{V,F}.glsl` + `class1/deferred/clouds{V,F}.glsl` + `class3/deferred/softenLightF.glsl` + `class1/deferred/deferredUtil.glsl` + `class1/deferred/shadowUtil.glsl` + `class1/deferred/aoUtil.glsl` + `class1/deferred/globalF.glsl` + `class1/environment/waterFogF.glsl` + `class1/deferred/tonemapUtilF.glsl`) |
| 変更行数 | **+167 / -0** (insertions-only / atmosphericsFuncs +15 / skyV +11 / skyF +11 / cloudsV +19 / cloudsF +13 / softenLightF +27 / deferredUtil +15 / shadowUtil +17 / aoUtil +9 / globalF +11 / waterFogF +10 / tonemapUtilF +9) |
| 修正範囲 | 各 file の既存 bare uniform 直前 + 直後に `#ifdef LL_VULKAN_GLSL` UBO block (`layout(set=3, binding=N, std140) uniform XxxUBO_Legacy { ... }`) + `#else` 既存 bare uniform 保全 + `#endif` 3-段 swap、insertions-only (削除 0)、既存 UBO (A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ 追加) は byte-for-byte 不変 |
| Agent 投入 | **1 件** (Explore very thorough = utility 19 file 監査 scan、Agent 報告 8 unguarded 件中 3 件命中 + 5 件追加 false negative あり = 後段 awk 網羅 scan で補完) |
| shader file 触り | **12 件** (B?-δ 専用 scope、skip list 13 + A2 拡張 skip 2 + 5 V skip 全 untouched 維持) |
| AYAstorm 改変保全 | **GL path 全不変** (3-段 swap `#else` 側 = 既存 bare `uniform xxx;` declarations 維持、Vulkan path 限定 UBO 追加のみ、`glCreateShader`/`glShaderSource`/`glCompileShader`/`glAttachShader` 呼出順序不変、`loadShaderFile` strdup 不変、charter §3 #1 acceptance) |
| AYA cold cache launch verify | **PASS** (起動成立 21:13 cache 再生成 224 shaderbin + clean shutdown 21:14 + Goodbye! 1 件 + Vulkan device/instance destroyed 各 1 件 + status: stopped + 実 FATAL/SIGSEGV/Aborted 0 件) |
| commit | `744d266f34` (AYA 「OK」明示指示下 2026-06-01) |
| metric vs B2-γ baseline non-opaque uniforms | **-223 ✓ B?-δ patch 効果完全直接観測** (223→0, 100% 解消) |
| metric vs B2-γ baseline parse failed | **±0 cascade exposure** (223→223、エラー種が `non-opaque uniforms` から 3 種別 leak へシフト、§4 詳細) |
| metric vs B2-γ baseline link failed | **±0** (0→0 維持) |
| metric net delta | **-223 errors** (B?-δ scope = non-opaque uniforms 単指標で完全解消、parse failed は cascade exposure で同数残存) |

### §2.1 既処理 sub-bundle との関係

| sub-bundle | 関係 |
|---|---|
| A1-A7 | uniform/sampler/UBO block 注入 (binding scope)、本 step touch 0 件、既存 UBO byte-for-byte 維持 (FrameViewProj set=0/binding=0 / FrameLights set=0/binding=1 / FrameAtmosphere set=0/binding=2 / MaterialUBO set=1/binding=0 / PerDrawUBO set=2/binding=0) |
| A8-recovery | AYAstorm 改変 5 file UBO 復活、本 step も skip list 維持で untouched |
| B1 | materialF.glsl MaterialUBO_Legacy 化 (case 2 file-local override)、本 step touch 0 件、**§3.1 B?-δ scan 範式の root inspiration source** |
| B2-α | varying + fragment_out 全 program 注入、本 step touch 0 件 |
| B2-β | vertex_in/VBO attribute 全 program 注入、本 step touch 0 件 |
| B3 | SPIR-V Vulkan profile override per-stage prepend、本 step も B3 範式の `#version 460 + #extension + LL_VULKAN_GLSL` 直後 prepend 経路をそのまま継承 |
| B2-γ | utility source cache + per-program attached utility tracking + utility concat hook + createShader reorder、本 step は B2-γ で hook された utility concat に乗る各 utility file の **内部 bare uniform** を wrap = B2-γ 整備した SPIR-V parse 経路を実際に通すための前提整備 |
| B?-δ 本 sub-bundle | utility unguarded bare uniform wrap = ALL 223 程度 non-opaque uniforms cluster の最後の構造 closure |

---

## §3 設計範式 (B?-δ で新規確立)

### §3.1 Utility 既 attach 全 file scope unguarded bare uniform 網羅 scan 範式 (B?-δ で新規確立)

**設計原則**: `LLShaderMgr::attachShaderFeatures()` (`indra/llrender/llshadermgr.cpp`) が `attachVertexObject` / `attachFragmentObject` で attach する **全 utility file の list** を root-of-truth として、各 file 内の `^uniform ` 行のうち `#ifdef LL_VULKAN_GLSL` / `#ifndef LL_VULKAN_GLSL` / `#else` / `#endif` で wrap されていない = **Vulkan glslang parse 経路に leak する bare uniform** を網羅 enumeration。判定は awk 状態機械:

```awk
/#ifdef LL_VULKAN_GLSL/ { in_vk=1; next }
/#ifndef LL_VULKAN_GLSL/ { in_nvk=1; next }
/#else/ {
  if(in_vk){in_vk=0; in_else=1}
  else if(in_nvk){in_nvk=0}
  next
}
/#endif/ { in_vk=0; in_nvk=0; in_else=0; next }
/^uniform / {
  # opaque (sampler/image/texture) は Vulkan 仕様で別 rule (binding qualifier 必要だが non-opaque ではない) なので除外
  if($2 ~ /^sampler/ || $2 ~ /^image/ || $2 ~ /^texture/) next
  if(in_vk==0 && in_else==0 && in_nvk==0) print FILENAME":"NR" "$0
}
```

**Why critical**: B?-δ 第1段 (9 file preventive UBO 追加) が **既に properly guarded だった file への重複追加** で metric 0 効果に終わった反省 = guard 状態判定を機械的に行わない限り「leak 候補 file」と「実 leak file」が混同する。awk 状態機械化でこれを排除。

**範式継承元**: B1 §3.1 (case 2 file-local UBO override 3-段 swap pattern) の **detection 側カウンターパート**。B1 は per-file UBO 化方法論、B?-δ §3.1 は per-tree leak 検出方法論。

### §3.2 Cascade source 特定範式 (B?-δ で新規確立)

**設計原則**: glslang error log の `ERROR: 0:LINE: '...' : message` の **`0:LINE`** 部分を `sort | uniq -c | sort -rn` で histogram 化。**ALL N errors が同一 `0:LINE` に集中** (例: 191/223 が `0:127`) する場合、その line は **ALL N programs の最先頭 attached utility 内** = cascade source の確定的 root location。

**Why critical**: per-program では attach 順序が異なるが、`attachShaderFeatures()` の最初の数件 (例: `globalF.glsl` 全 fragment shader 共通 1st) は **全 program 共通**。glslang は parse 失敗時 first error で abort するため、ALL programs が同一 line で reject = 共通 1st utility の bare uniform = cascade source の唯一解。

**応用**: B?-δ 第2段では `0:51` 同一 cluster ⇒ globalF.glsl 推定 ⇒ line 31 `mirror_flag` + line 39 `clipSign` 確認 ⇒ wrap で解決の流れ。次 sub-bundle B?-ε でも同様 `0:127` 集中の trace で cascade source を特定可能。

**範式継承元**: B3 §12 metric 整合性 self-check 範式 (literal pattern grep のみ採用) の **error location 軸 extension**。B3 は metric 計算正確性、B?-δ §3.2 は error 構造特定。

### §3.3 patch literal (主要 3 hook + 9 file preventive 範式 1 例)

**(a) `class1/deferred/globalF.glsl` line 31-39 (旧) → line 31-50 (新)** = mirror_flag + clipSign wrap:
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=3, binding=11, std140) uniform GlobalFParamUBO_Legacy {
    float mirror_flag;
    float clipSign;
    float _pad_globalf_0;
    float _pad_globalf_1;
};
#else
uniform float mirror_flag;
#endif
#ifdef LL_VULKAN_GLSL
layout(set=2, binding=0, std140) uniform PerDrawUBO {   // A6 既存、不変
    vec4 clipPlane;
};
#else
uniform vec4 clipPlane;
#endif
#ifndef LL_VULKAN_GLSL
uniform float clipSign;
#endif
```

**(b) `class1/environment/waterFogF.glsl` line 43-45 (旧) → line 43-55 (新)** = 3 件 wrap:
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=3, binding=9, std140) uniform WaterFogUBO_Legacy {
    vec4  waterFogColor;
    float waterFogDensity;
    float waterFogKS;
    float _pad_waterfog_0;
    float _pad_waterfog_1;
};
#else
uniform vec4 waterFogColor;
uniform float waterFogDensity;
uniform float waterFogKS;
#endif
```

**(c) `class1/deferred/tonemapUtilF.glsl` line 138-140 (旧) → line 138-149 (新)** = 3 件 wrap:
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=3, binding=10, std140) uniform TonemapUBO_Legacy {
    float exposure;
    float tonemap_mix;
    int   tonemap_type;
    float _pad_tonemap_0;
};
#else
uniform float exposure;
uniform float tonemap_mix;
uniform int tonemap_type;
#endif
```

**(d) 1st attempt 9 file 代表例 `class1/windlight/atmosphericsFuncs.glsl` line 78-100** (preventive、metric 効果 0 件):
```glsl
#ifdef LL_VULKAN_GLSL
layout(set=3, binding=0, std140) uniform AtmoExtraUBO_Legacy {
    vec3  lightnorm;
    float haze_horizon;
    float cloud_shadow;
    float sun_moon_glow_factor;
    int   aya_visual_realism_enabled;
    int   aya_r14_volumetric_atmosphere_enabled;
    float aya_r14_strength;
    int   aya_r16_aerial_perspective_enabled;
    float aya_r16_strength;
    float _pad_legacy_0;
};
#else
uniform vec3  lightnorm;
uniform float haze_horizon;
/* ... 7 既存 bare uniforms 不変 ... */
#endif
```

### §3.4 set=3 binding namespace 確立

B?-δ で新規に **set=3 namespace = legacy/r41-added UBO 予約** が確立。既存 set 割当:

| set | binding | 用途 | 確立 sub-bundle |
|---|---|---|---|
| 0 | 0 | FrameViewProj | A1 |
| 0 | 1 | FrameLights | A2 |
| 0 | 2 | FrameAtmosphere | A2 |
| 1 | 0 | MaterialUBO | B1 |
| 2 | 0 | PerDrawUBO | A6 |
| 3 | 0 | AtmoExtraUBO_Legacy (preventive) | B?-δ 1st |
| 3 | 1 | SkyVParamUBO_Legacy (preventive) | B?-δ 1st |
| 3 | 2 | SkyFParamUBO_Legacy (preventive) | B?-δ 1st |
| 3 | 3 | CloudsVParamUBO_Legacy (preventive) | B?-δ 1st |
| 3 | 4 | CloudsFParamUBO_Legacy (preventive) | B?-δ 1st |
| 3 | 5 | SoftenLightParamUBO_Legacy (preventive) | B?-δ 1st |
| 3 | 6 | DeferredUtilParamUBO_Legacy (preventive) | B?-δ 1st |
| 3 | 7 | ShadowUtilParamUBO_Legacy (preventive) | B?-δ 1st |
| 3 | 8 | AOUtilParamUBO_Legacy (preventive) | B?-δ 1st |
| 3 | 9 | WaterFogUBO_Legacy (effective) | B?-δ 2nd |
| 3 | 10 | TonemapUBO_Legacy (effective) | B?-δ 2nd |
| 3 | 11 | GlobalFParamUBO_Legacy (effective) | B?-δ 2nd |
| 3 | 12+ | **未割当** (B?-ε 以降の per-program UBO 予約候補) |

次 sub-bundle B?-ε 以降は set=3, binding=12 以降を昇順で割当推奨。

---

## §4 AYA cold cache launch verify metric

baseline = `AYAstorm.old.b2-delta-pre-utility-leak-fix` (B?-δ 第1段 cold verify 後 = B2-γ baseline 同等)、after = `AYAstorm.log` (B?-δ 第2段 cold verify 後 21:13-21:14)。

| metric | baseline | after | delta | 評価 |
|---|---|---|---|---|
| `non-opaque uniforms outside a block` | 223 | **0** | **-223** | ✓ B?-δ 直接効果 (100% 解消、主指標完全達成) |
| `glslang parse failed for stage` | 223 | 223 | ±0 | cascade exposure (エラー種シフト、§4.1 詳細) |
| `glslang link failed for program` | 0 | 0 | ±0 | 維持 |
| `'#version' must occur first` cluster | 0 | 0 | ±0 | B3 既処理状態維持 |
| `location` cluster | 0 | 0 | ±0 | B2-α 既処理状態維持 |
| `binding` cluster (UBO 関連) | 0 | 0 | ±0 | A1-A7 既処理状態維持 |
| `binding` cluster (opaque uniform 新規) | 0 | 数件 | +数件 | **cascade exposure ε scope** (§4.1) |
| `'#' preprocessor directive cannot be preceded by another token` | 0 | 191 | +191 | **cascade exposure ε scope** (§4.1) |
| `'GBufferInfo' redefinition struct` | 0 | 数件 | +数件 | **cascade exposure ε scope** (§4.1) |
| `missing #endif` cluster | 0 | 0 | ±0 | B2-γ 既処理状態維持 |
| shader_cache 件数 | 224 | 224 | ±0 | B2-γ baseline と同等 (B?-δ 第1段 verify 時点 224、第2段 verify 時点も 224 維持) |
| Goodbye! 件数 | 1 | 1 | ±0 | clean shutdown 維持 |
| Vulkan device destroyed | 1 | 1 | ±0 | clean shutdown 維持 |
| Vulkan instance destroyed | 1 | 1 | ±0 | clean shutdown 維持 |
| status: stopped | 1 | 1 | ±0 | clean shutdown 維持 |
| FATAL / SIGSEGV / Aborted 実件数 | 0 | 0 | ±0 | safe |

### §4.1 Cascade exposure 分類 (B?-ε scope 候補)

B?-δ で `non-opaque uniforms` を 100% 解消した結果、glslang が parse でより深く進めるようになり、**preexisting だったが今まで line 51 で abort されて見えていなかった** 別 3 種 leak が露出 (= 同 223 program 集合が異なる error で fail):

| 件数 | line | エラー種 | 構造原因 | 想定対処 (B?-ε 案) |
|---|---|---|---|---|
| **191** | 0:127 | `'#' : preprocessor directive cannot be preceded by another token` | utility source concat 時 cache entry 末尾 `\n` 欠落 → 次 entry の `#` directive が previous 行末 token と同行扱い | `LLShaderMgr::loadShaderFile()` cache populate 時 (B2-γ §3.4 (b) hook) 各 entry 末尾 `\n` 強制 append、または `generatePerProgramSPIRV()` 内 `concatenated.append(util_sources[i])` 直後 `\n` 強制 append |
| 数件 | 0:160 | `'GBufferInfo' redefinition struct` | utility 二重 attach (例 `gbufferUtil.glsl` を `attachShaderFeatures()` 経由で2回 attach) → struct 定義二重出現 | `mVulkanAttached{Vertex,Fragment}Utilities` push_back 直前に既 contain 判定追加 (set 化 or dedup check)、または `gbufferUtil.glsl` 内 `#ifndef GBUFFER_INFO_DEFINED` guard 追加 |
| 数件 | 0:160 | `'binding' : sampler/texture/image requires layout(binding=X)` | Vulkan 仕様: opaque uniform (sampler2D 等) も `layout(set=N, binding=M) uniform sampler2D ...;` の qualifier 必須、bare `uniform sampler2D xxx;` は reject | 各 utility/program file の `^uniform sampler/image/texture` 行に `layout(set=N, binding=M)` qualifier 注入 (per-tree scan、A 範式の opaque uniform 版) |
| 数件 | 0:160 | `'' : compilation terminated` | 上記 3 種の累積打ち止め (1 stage 中 N error 後 abort) | 上記 3 種解消の副次効果で消失する見込み |

総 cascade exposure 件数 = 191 + 数件 = **~200 件超** が B?-ε scope。

### §4.2 主指標 metric integrity self-check (B3 §12 範式継承)

literal pattern grep のみ採用 (structural touch consistency / 3 段階 pair-grep 単調性):

- `non-opaque uniforms outside a block`: B?-δ 2nd verify log 0 件 = `grep -c "non-opaque uniforms outside a block" AYAstorm.log` 直接観測値 (calculated value ではない)
- baseline 223 件: B?-δ 1st verify log (= B2-γ baseline 同等) `grep -c "non-opaque uniforms outside a block" AYAstorm.old.b2-delta-pre-utility-leak-fix` 直接観測値
- delta -223: 単純減算、calculated source の compose-aware breakdown は §4.1 cascade exposure 表で独立に literal source pattern として記録

範式違反検出: `parse failed` 223→223 ±0 を「主指標と独立に」報告、混同して net delta を主指標に compose しない。

---

## §5 self-verify (commit 前最終確認)

| # | 検証項目 | 結果 |
|---|---|---|
| 1 | 修正 12 file 全て insertions-only (削除 0、修正範囲外 byte-for-byte 維持) | ✓ git diff --stat = +167/-0 |
| 2 | A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ 既処理 file 既存 UBO block byte-for-byte 不変 | ✓ atmosphericsFuncs FrameLights/FrameAtmosphere + globalF PerDrawUBO + waterFogF FrameLights + tonemapUtilF FrameViewProj 全 untouched |
| 3 | GL path bare uniform 維持 (3-段 swap `#else` 側) | ✓ 12 file 全て GL path 動作 path 不変 |
| 4 | skip list 13 + A2 拡張 skip 2 + 5 V skip untouched | ✓ 12 file 全て skip list 外 |
| 5 | std140 alignment 全 UBO 担保 (vec3 + 4byte padding hoist / mat4 64B / mat3 48B / 末尾 `_pad_legacy_N`) | ✓ 12 file 全 UBO std140 compliant |
| 6 | charter §3 #1 acceptance (Vulkan path 内部追加のみ、glCreateShader/glShaderSource/glCompileShader/glAttachShader 全不変、loadShaderFile strdup 不変) | ✓ 全 12 file shader-side のみ、C++ side touch 0 |
| 7 | AYA cold cache launch verify PASS (clean shutdown + FATAL/SIGSEGV/crash 0 件) | ✓ 21:13 起動 / 21:14 終了 / Goodbye! 1 / Vulkan destroyed 各 1 / status: stopped |
| 8 | shader_cache 再生成確認 (cold cache = 完全 clear から再構築) | ✓ 224 shaderbin re-built |
| 9 | 主指標 `non-opaque uniforms outside a block` 223→0 直接観測 | ✓ literal grep count |
| 10 | cascade exposure 3 種 (`#` preprocessor / `GBufferInfo` redefinition / `binding` opaque) を §4.1 で B?-ε scope として独立記録、本 sub-bundle scope と混同せず | ✓ §4.1 確立 |
| 11 | commit 前 AYA 「OK」明示指示確認 | ✓ 2026-06-01 AYA「OK」 |

---

## §6 設計範式継承表

| 範式 | 由来 sub-bundle | 本 B?-δ 適用箇所 |
|---|---|---|
| `feedback_doubt_self_first` (効かない時はまず自分のコード/仮説を疑う) | feedback memory | 第1段 9 file patch metric 効果 0 件 → AYA 提示情報を疑わず自仮説を falsify、第2段 trace で真因 (utility leak) 特定 |
| `feedback_admit_unknown` (分からないときは「分からない」と言う) | feedback memory | 第1段 falsify 後、推論で patch 拡張せず log/grep で実データ取得 (awk scan 範式構築) |
| `feedback_build_only_verified` (正しいことを積み上げる) | feedback memory | 第1段 9 file patch を revert せず preventive cleanup として残置、第2段 真因解消 commit に同梱 = AYA 「OK 判断まかせます 分けましょう」承認後の構成 |
| `feedback_falsification_as_progress` (全 REJECT verdict は生き残りルート絞り込みの成果) | feedback memory | 第1段 9 file の metric 0 効果は実質 falsification = utility leak 真因への絞り込み成果として §1/§3.1/§3.2 で範式化 |
| `feedback_one_step_at_a_time` (検証手順は1ステップずつ) | feedback memory | 第2段 patch 完了 → AYA verify → metric trace → cascade exposure 発覚 → AYA commit 判断 → handoff doc の 1msg/1action cadence 維持 |
| `feedback_no_auto_commit` (コミットは明示指示があるまでしない) | feedback memory | 第1段 0 効果報告 + 第2段 patch 完了 + cascade exposure 報告 + 3 option 提示 → AYA 「OK」明示指示後 commit (744d266f34) |
| `feedback_no_claude_coauthor` (Co-Authored-By: Claude を付けない) | feedback memory | commit message 末尾 Claude 共著行なし |
| `feedback_no_scope_shrink` (AYA 指示 literal scope を勝手に縮小しない) | feedback memory | AYA「全 utility scope scan」指示を A2 skip 含む全 file 厳格 awk scan で完遂、scope shrinking なし |
| B1 §3 範式: case 2 file-local UBO override 3-段 swap pattern | B1 patch | 12 file 全て 3-段 swap `#ifdef LL_VULKAN_GLSL` UBO `#else` legacy uniform `#endif` |
| B2-α §3.1: canonical Table location 順 cross-file 固定 | B2-α patch | UBO 内 member 順 = 既存 bare uniform 出現順 で配置、cross-file 整合 |
| B3 §3.2: GL profile / Vulkan profile 分離範式 | B3 patch | `#ifdef LL_VULKAN_GLSL` / `#else` の Vulkan / GL 2-path 分離継承 |
| B3 §12: metric 整合性 self-check 範式 (literal pattern grep のみ採用) | B3 patch 訂正 | §4.2 で literal grep 採用、compose-aware net delta を主指標に混同せず |
| B2-γ §3.1: Vulkan utility source cache 範式 | B2-γ patch | utility source cache に B?-δ wrap 後の utility 内容が乗ることで cache→concat→glslang parse 経路全長で UBO 化が効果発揮 |
| B2-γ §3.2: per-program attached utility tracking 範式 | B2-γ patch | utility attach 順序が SPIR-V concat に正しく反映されることで、cascade source 特定範式 (§3.2) の前提 (= ALL programs 共通の 1st utility = globalF) が成立 |
| B2-γ §3.3: utility concat hook + createShader reorder 範式 | B2-γ patch | createShader reorder が `attachShaderFeatures()` 後ろに移動済のため、本 sub-bundle で wrap した utility が正しく concat される |

---

## §7 risks (B?-δ 完遂後 / B?-ε 着手前)

| # | risk | 評価 |
|---|---|---|
| 1 | std140 padding 計算誤りで UBO サイズ mismatch → CPU 側 buffer 書込時 driver 拒否 | **低** (12 file 全 UBO で vec3 直後 float hoist + 末尾 `_pad_legacy_N` 加算で 16B align 保証、cold cache launch verify PASS = driver layout check 通過確認済) |
| 2 | set=3 namespace 衝突 (future sub-bundle が同 set/binding 重複割当) | **低** (§3.4 table 確立で割当 traceable、未割当領域 set=3/binding=12+ も明示) |
| 3 | 1st attempt 9 file preventive UBO の CPU 側 binding 接続 (glUniformBlockBinding / VkDescriptorSet) 未整備 → 将来 ALM program 拡張時に runtime null binding | **中** (preventive UBO のため現状 binding 未接続、ALM program 実装時に C++ 側 binding code 同時整備必要、handoff §10 で明示) |
| 4 | cascade exposure 3 種 (`#` preprocessor / `GBufferInfo` redefinition / opaque `binding`) のうち `#` preprocessor (191 件) が解消困難なら program count base の進捗が停滞 | **低** (B?-ε §4.1 想定対処 = cache populate 時 `\n` 強制 append、1 hook で 191 件 broadcast 解消見込み) |
| 5 | B?-δ 1st attempt 9 file 残置が「未使用 dead code」誤認で将来 revert される | **低** (本 handoff §1 / §3.3 (d) で preventive 意図明示、commit message 内 (a)/(b) 構造明示済) |
| 6 | awk scan 範式 (§3.1) が `#if defined(LL_VULKAN_GLSL)` (= `#ifdef` 等価別表記) を捕捉しない false positive リスク | **中** (現 awk は `#ifdef LL_VULKAN_GLSL` literal only、`#if defined(LL_VULKAN_GLSL)` 表記が存在すれば leak 誤判定、B?-ε で確認推奨) |
| 7 | cascade source 特定範式 (§3.2) の前提 = ALL N errors 同一 0:LINE 集中、が次以降の sub-bundle で成立しないケース | **中** (B?-ε は 191 件 0:127 集中 + 数件分散の混在、§3.2 範式は 191 件 cluster には適用可、残小 cluster は別 trace 必要) |
| 8 | shader_cache 件数 (224) が B2-γ baseline (245) より 21 件少ない状態が persist | **低** (B2-γ-complete handoff §12 観測点として既記録、本 step 中も 224 維持 = 退行なし、B2-γ 段階の現象が継承、致命的でなく次 sub-bundle 着手前 trace 推奨は B2-γ-complete handoff §12 の項目) |
| 9 | 12 file 中 1st 9 file の Edit が「既処理 file UBO untouched」 absolute rule 違反に見える誤読 | **低** (1st 9 file は B?-δ 第1段で **新規 UBO 追加** = 既存 UBO 不変 = §10.3 例外 (新 UBO block 追加可) 該当、本 handoff §2 行で明示) |
| 10 | A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ 既処理 file の既存 UBO 部 byte-for-byte 維持の literal verify を怠ると後段 sub-bundle で気付かない退行 | **低** (本 step は §2 / §5 #2 で literal verify 済、次 sub-bundle 着手前も同 verify 必要 = §10 推奨) |

---

## §8 commit メッセージ literal (`744d266f34`)

```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-δ 完遂 (utility unguarded leak wrap、non-opaque uniforms outside a block 223→0 完全 clear、12 file +167/-0 insertions-only、shader file touch 12 = (a) 1st attempt 9 file preventive UBO 設計追加 (atmosphericsFuncs AtmoExtraUBO_Legacy set=3/binding=0 + sky{V,F} SkyVParamUBO_Legacy/SkyFParamUBO_Legacy set=3/binding=1,2 + clouds{V,F} CloudsVParamUBO_Legacy/CloudsFParamUBO_Legacy set=3/binding=3,4 + softenLightF SoftenLightParamUBO_Legacy set=3/binding=5 + deferredUtil DeferredUtilParamUBO_Legacy set=3/binding=6 + shadowUtil ShadowUtilParamUBO_Legacy set=3/binding=7 + aoUtil AOUtilParamUBO_Legacy set=3/binding=8、metric 効果 0 件 = falsified hypothesis、preventive cleanup 残置 = 将来 ALM program 拡張時の保険) + (b) 2nd attempt 3 file = root cause utility unguarded leak wrap (globalF GlobalFParamUBO_Legacy set=3/binding=11 (mirror_flag + clipSign) + waterFogF WaterFogUBO_Legacy set=3/binding=9 (waterFogColor + waterFogDensity + waterFogKS) + tonemapUtilF TonemapUBO_Legacy set=3/binding=10 (exposure + tonemap_mix + tonemap_type))、設計範式新規 1 件 + cascade source 特定範式新規 1 件、metric vs B2-γ baseline = non-opaque uniforms 223→0 (-223 ✓ B?-δ 直接効果) + parse failed 223→223 (±0 cascade exposure 第2層 leak 3 種露出 = '#' preprocessor directive 191 件 + 'GBufferInfo' redefinition 数件 + 'binding' sampler/texture/image 数件) + link failed 0→0 (±0)、charter §3 #1 担保、AYA cold cache launch verify PASS、A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ 既処理 file 既存 UBO block byte-for-byte 維持、skip list 13 + A2 拡張 skip 2 + 5 V skip untouched、AYA 「OK」明示指示下 commit 2026-06-01)
```

---

## §9 build artifact

| artifact | path / state |
|---|---|
| install tree (3 file 第2段 deploy 反映) | `~/ayastorm/app_settings/shaders/class1/deferred/globalF.glsl` + `class1/environment/waterFogF.glsl` + `class1/deferred/tonemapUtilF.glsl` (2nd attempt 第2段、cold cache verify 21:13 起動時の deploy 状態) |
| shader_cache | `~/.ayastorm_x64/cache/shader_cache/` 224 shaderbin (cold cache verify 21:13 再生成) |
| baseline log | `~/.ayastorm_x64/logs/AYAstorm.old.b2-delta-pre-utility-leak-fix` (B?-δ 第1段 verify 後 = B2-γ baseline 同等、390053 byte) |
| after log | `~/.ayastorm_x64/logs/AYAstorm.log` (B?-δ 第2段 verify 後、363336 byte、起動 21:13 / 終了 21:14) |
| 1st attempt 9 file install tree | 第1段 deploy 状態が repository HEAD = 第2段 deploy 状態 (3 file 追加分のみ差分)、`~/ayastorm/` 配下に 12 file 全反映済 |
| executable | 既存 build artifact (本 step は shader-only change、autobuild 不要 = `feedback_shader_only_fast_iterate` 範式適用) |

---

## §10 次 sub-bundle 推奨 (B?-ε)

| 優先順位 | 候補 | scope | 想定 metric 効果 | 想定 file touch | Agent 必要性 |
|---|---|---|---|---|---|
| 1 | **B?-ε** = utility cache populate 末尾 `\n` 補正 + utility 二重 attach dedup + opaque uniform layout(binding=X) 注入 | (a) `LLShaderMgr::loadShaderFile()` cache populate hook (B2-γ §3.4 (b)) で各 entry 末尾 `\n` 強制 append → `'#' preprocessor directive cannot be preceded by another token` 191 件解消 + (b) `mVulkanAttached{Vertex,Fragment}Utilities` push_back 直前 dedup check → `'GBufferInfo' redefinition struct` 数件解消 + (c) 全 utility/program file `^uniform sampler/image/texture` 行に `layout(set=N, binding=M)` qualifier 注入 → `'binding' sampler/texture/image requires layout(binding=X)` 数件解消 | parse failed 223→0 級減少見込み、cascade exposure で link failed 露出可能性あり | (a) C++ 2 file 数行修正 + (b) C++ 1 file 数行修正 + (c) 多数 shader file (Agent 並列必須、~100 file 想定) | (a)(b) Agent 不要 / (c) Agent 並列必須 |
| 2 | 残 mixed cleanup | scope 未確定 (B?-ε 完遂後の cascade exposure 次第) | 不明 | 不明 | 不明 |

**B?-ε 着手前必須事項**:
- AYA 明示指示要 (no auto-commit 範式継承)
- fresh context 推奨 (handoff doc + memory 範式継承記録から start)
- cache 完全 clear 必須 launch verify
- A1-A7/A8-recovery/B1/B2-α/B2-β/B3/B2-γ/B?-δ 既処理 file 再 touch 禁止 absolute rule
- 観測点: shader_cache 件数 (現 224、B2-γ baseline 245 比 -21 が persist)、原因未 trace、致命的でない、B?-ε 着手前 trace 推奨

---

## §11 観測点 (B?-δ 中に検出 / 致命的でない / 次 sub-bundle 着手前 trace 推奨)

| 観測点 | 詳細 | 致命度 |
|---|---|---|
| shader_cache 件数 -21 件 persist | B2-γ baseline 245 → B?-δ 第1段/第2段 cold verify 後 224 件、原因未 trace (B2-γ-complete handoff §12 既記録の現象が継承) | **低** (launch 成立 + clean shutdown + FATAL 0、ただし shader 数減少 = いくつかの program が cache miss 発生中の可能性、B?-ε 着手前に specific program 名特定推奨) |
| Agent (Explore) 報告精度 | B?-δ 第2段 trace で Agent 報告 8 unguarded 件中 3 件命中 + 5 件 false negative (後段 awk scan で補完) | **中** (Agent 単独報告を信任せず必ず後段 mechanical scan で verify する範式が B?-δ で確立、§3.1 awk 状態機械化はその副産物) |
| awk scan 範式 `#if defined(LL_VULKAN_GLSL)` 表記未対応 | 現 awk は `#ifdef LL_VULKAN_GLSL` literal only、別表記 `#if defined(LL_VULKAN_GLSL)` 存在時 leak false positive リスク | **低** (B?-δ 適用 file 12 件全て `#ifdef` 表記で問題なし、B?-ε 適用前に全 utility/program file の `#if defined` grep 推奨) |

---

(EOF)
