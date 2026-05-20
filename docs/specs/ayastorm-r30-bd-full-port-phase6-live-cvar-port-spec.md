# AYAstorm r30 BD full port — Phase 6 live cvar port spec

**Phase 名**: r30 BD 完全移植 Phase 6 — Phase 3.9 で stub 残置した未配線/未実装 cvar の 1:1 移植
**前提**: Phase 5 cleanup 完了 (commit `b33a6b6c4c`)、BD remote `bd/master` (995a1354d8) を fetch 済
**作成日**: 2026-05-20
**ブランチ**: `feature/ayastorm-r30-bd-full-port-inventory`
**章方針**: `feedback_bd_full_port_only.md` (BD 1:1 verbatim) / `feedback_no_escape_full_bd_coverage.md` / `feedback_match_bd_defaults_on_borrow.md`

---

## §0 目的 / 非目的 / 経緯

### 目的
Phase 3.9 UI mount 時に Cinematic Controls floater (`floater_aya_cinematic.xml`) に「未実装 — BD cvar X 未移植」として stub 残置していた UI 行を、BD upstream 995a1354d8 の **live (実消費されている) cvar** のみ 1:1 移植して有効化する。同時に設定登録のみ済んでいた per-channel shadow 系の placeholder 3 cvar を AY pipeline に消費配線する。

### 非目的
- BD で dead code 化されている UI 系の移植 (§1.3 参照、X1b 判断)
- 追加 borrow / 推論移植 (`feedback_bd_full_port_only.md`)
- AY 既存 tone mapping / 視覚表現章 (r14-r20) 経路の改修

### 経緯 (X1a / X1b 判断)
audit で BD 995a1354d8 が次の 3 系列を **dead code 状態** で持っていることが判明:
- `RenderGodrays` + 4 関連 cvar (`llfloaterpreference.cpp:1437-1441 / 1970-1975 / 2110` 全 `//` コメントアウト、pipeline.cpp consumer 無し)
- `RenderLensFlare` (`pipeline.cpp:691` `connectRefreshCachedSettingsSafe` のみ、実 consumer 無し)
- Exodus tone mapping sub-system 15 cvar (`exoPostProcess` class 未定義、`tonemapUtilF.glsl` は LL ACES 標準で Exodus 経路無し)

`feedback_bd_full_port_only.md` の趣旨は「BD と並走する pipeline」であり、動かない UI を引き写すのは趣旨に反するため、**X1b (BD で live な cvar のみ 1:1 移植)** を採用 (AYA 承認 2026-05-20)。

---

## §1 移植対象 (Phase 6 scope)

### §1.1 Category A — placeholder cvar の AY pipeline 配線 (3 件)

`indra/newview/app_settings/settings.xml` line 12388-12390 で「現状未配線」と注記されている 3 件:

| AY cvar | Type | BD default | BD 上流 settings_blackdragon.xml | BD consumer (pipeline.cpp) |
|---|---|---|---|---|
| `RenderShadowResolution` | Vector4 | `[2048, 2048, 1024, 1024]` | :779 (closest X / mid Y / far Z / furthest W) | :247 (member), :1107-1109 (shadow buffer allocation per-cascade), :1310 (read) |
| `RenderShadowDistance` | Vector4 | `[2, 8, 16, 64]` | :914 ("Shadow Far Clip range addition") | :243 (member `RenderShadowFarClipVec`), :1298 (read), :10708-10709 (clip plane setup) |
| `RenderProjectorShadowResolution` | Vector2 | `[1024, 1024]` | :1241 | :248 (member), :1131 (spot shadow allocation), :1309 (read) |

**移植内容**:
- AY 既存の `RenderShadowResolutionScale` (単一スカラー) と並列に BD per-channel 配線を追加
- pipeline.cpp shadow buffer allocation + clip plane計算で per-channel 配列を消費
- settings.xml の comment block (line 12388-12390) から「現状未配線」表記を除去

### §1.2 Category B core — live BD cvar (7 件)

`bd/master` で pipeline.cpp 等が実消費している cvar:

| cvar | Type | BD default | BD consumer | 移植先 file (AY 側) |
|---|---|---|---|---|
| `RenderShadowFarClip` | F32 | `200` | pipeline.cpp:240/1295/10699 (sun clip plane near+far計算) | pipeline.cpp (Cinematic dispatch 経路) |
| `RenderDepthOfFieldAlphas` | Boolean | `0` | lldrawpoolalpha.cpp (alpha pass 2 DoF) | lldrawpoolalpha.cpp |
| `RenderEnableFullbright` | Boolean | `1` | **`indra/llprimitive/llprimitive.cpp:1256/1349/1474`** + llviewercontrol.cpp:991/1238 (fullbright flag suppression) | llprimitive.cpp |
| `RenderDeferredLights` | Boolean | `1` | pipeline.cpp:387/483/5888/6085/9251 (deferred light pass skip) | pipeline.cpp |
| `RenderOwnAttachedLights` | Boolean | `1` | pipeline.cpp:386/482/5880/6077/9243 | pipeline.cpp |
| `RenderOtherAttachedLights` | Boolean | `1` | pipeline.cpp:385/481/5879/6076/9242 | pipeline.cpp |
| `RenderGlobalLightStrength` | F32 | `1.0` | pipeline.cpp:241/1296/8965 (`LLShaderMgr::DEFERRED_LIGHT_STRENGTH` uniform) | pipeline.cpp + llshadermgr (new uniform) |

**移植内容**:
- settings.xml: 各 cvar 登録 (BD default 採用、Comment は BD 上流文 verbatim、Cinematic 既定 mode で BD default が効くよう Cinematic overlay 該当行追加)
- pipeline.h: static 宣言 (BD の declaration 順を尊重)
- pipeline.cpp: ctor 読み込み + connectRefreshCachedSettings + 各 consumer site の dispatch (Cinematic mode の時 BD 経路、それ以外は AY 既存値)
- llprimitive.cpp: fullbright suppression は global (per-mode dispatch 入れない、BD verbatim)
- llshadermgr.cpp/.h: `DEFERRED_LIGHT_STRENGTH` uniform name 追加

### §1.3 Category B Post FX — live BD cvar (3 件)

| BD cvar | Type | BD default | BD internal名 (pipeline.h) | uniform name |
|---|---|---|---|---|
| `RenderPostSepiaStrength` | F32 | `0.0` | `RenderSepiaStrength` | `DEFERRED_SEPIA_STRENGTH` |
| `RenderPostGreyscaleStrength` | F32 | `0.0` | `RenderGreyscaleStrength` | `DEFERRED_GREYSCALE_STRENGTH` |
| `RenderPostPosterizationSamples` | U32 | `1` | `RenderNumColors` | `DEFERRED_NUM_COLORS` |

**BD consumer**:
- pipeline.cpp:236-238 (member 宣言), :1314-1316 (read), :8218-8220 (gGlowCombineProgram uniform), :8970-8972 (deferred shader uniform)
- lldrawpoolwater.cpp:258-260 (water shader uniform)
- pipeline.h:1129-1131 (declaration)

**移植内容**:
- settings.xml 登録 (BD default、Comment verbatim)
- pipeline.h/cpp 上記 BD と同一構造で member + ctor + uniform send
- lldrawpoolwater.cpp: BD と同じ 3 uniform send 追加 (Cinematic mode 限定で良いか要検証 — water shader が AY 側で異なる場合 dispatch 入れる)
- llshadermgr.cpp/.h: 3 uniform name 追加
- shader 側 (glowcombineF.glsl / waterF.glsl 等): uniform 受け + post FX 演算追加 (BD shader 1:1)

### §1.4 dead BD UI の drop 範囲 (X1b 適用)

BD で dead 化されている次の UI 行は floater_aya_cinematic.xml から削除 (移植しない):

| dead 群 | 削除対象 (en xml line / ja xml line) |
|---|---|
| BD Godrays section | en:335-338, ja:対応 BD Godrays section |
| Lens / Flare section | en:360-363 (`cb_LensFlare` / `d_LensFlare`), ja:269-270 |
| Tone mapping (Color grade tech / Tone mapping op) | en:386-392 (`T_Tone` / `T_ColorGradeTech` / `T_ToneMapping` 行群), ja:294-297 |

Post FX section (Greyscale / Sepia / Posterize) は live なので **有効 UI 化** (slider 等追加、削除はしない)。

### §1.5 Category C — floater branding 残漏れ (副次)

floater_aya_cinematic.xml の tool_tip / section header に残る "BD cvar X 未移植" / "BD Godrays" / "BD RenderGodrays" 文字列を 「新しい描画エンジン」 統一形 (Phase 5 §案 a) に置換、または §1.4 dead UI 削除に伴い消滅させる。

---

## §2 移植戦略

### §2.1 mode dispatch 方針

`feedback_bd_full_port_only.md` × `feedback_match_bd_defaults_on_borrow.md` × Phase 5 R2 で確立した Cinematic overlay 機構の延長で実装:

- **mode 0/1 (Firestorm / AYAstorm View)**: AY/LL 既存挙動を温存
- **mode 2 (Cinematic)**: BD verbatim 動作

ただし以下は global 採用 (mode dispatch 入れない、BD 上流が global 適用しているため):
- `RenderEnableFullbright` (llprimitive.cpp 経路、avatar 関連 fullbright flag に直接効く)
- Post FX 3 件 (BD は global mode 概念無し、AY 既存 noop 値 = `0.0` / `1` で実害なし)

### §2.2 settings.xml overlay 反映

Phase 5 R2 で確立した Cinematic overlay 機構を使用。各 BD cvar の **Cinematic 上での値 = BD default** とし、mode 0/1 では AY 既存値を維持。AY 既存値の無い新規 cvar (今回 13 件すべて) は BD default を mode 0/1 でも採用 (BD default が「無効化」状態の物が多いため実害なし)。

### §2.3 build / shader rebuild 要否

- shader 改造あり: §1.3 Post FX 群 (glowcombineF.glsl / waterF.glsl)
- C++ 改造のみ: §1.1 (Category A) / §1.2 (Category B core) — pipeline.cpp / lldrawpoolalpha.cpp / llprimitive.cpp / llshadermgr.cpp/.h

→ shader 改造分は cache clear 必要。C++ 改造分は autobuild rebuild 必要。

---

## §3 commit unit

| step | 状態 | commit |
|---|---|---|
| 1 (Category A 配線) | ✅ | `45a71362d7` r30 BD full port Phase 6 step 1: per-channel shadow 3 cvar を AY pipeline に配線 |
| 2 (Category B core) | ✅ | step 2 関連 commit 群 (Shadow/DoF/Fullbright/Lights/GlobalLight, 7 件) |
| 3 (Category B Post FX) | ✅ | `1306241cc4` r30 BD full port Phase 6 step 3: Post FX 3 cvar (Greyscale/Sepia/Posterize) 1:1 移植 |
| 4 (Category C UI cleanup) | ✅ | `94abf64fab` r30 BD full port Phase 6 step 4: floater dead UI 削除 + live cvar UI 化 + branding 残漏れ rename |
| 5 (build / 検証) | ✅ | step 4 build green / install OK, AYA hands-on は別作業 |
| FINAL | ✅ | `r30 BD full port Phase 6: live cvar 13 件 1:1 移植 完了` |

---

## §4 punt 禁止項目 (自検)

`feedback_bd_full_port_only.md` / `feedback_no_escape_full_bd_coverage.md`:

- ❌ 「dead 化されている BD cvar も完全 verbatim 移植 (案 X1a)」punt → X1b 採用 (動かない UI は移植しない、dead code 引き写しは "並走" の趣旨外)
- ❌ 「Cinematic mode のみで効かせるべき AY 既存路線」punt → BD は global 適用している cvar (Fullbright / Post FX) は global 採用が verbatim
- ❌ 「Post FX shader を AY 既存 post-pass にマージ」punt → BD と同じ glowcombine / water 経路で 1:1 配線、AY 既存 post-pass は触らない

---

## §5 完了基準

- ✅ §1.1 Category A 3 件: pipeline.cpp で shadow buffer + clip plane が per-channel 配列から正しく構成される
- ✅ §1.2 Category B core 7 件: BD 同等 site で同じ dispatch が走る (RenderShadowFarClip / RenderGlobalLightStrength / RenderOtherAttachedLights / RenderOwnAttachedLights / RenderDeferredLights / RenderDepthOfFieldAlphas / RenderEnableFullbright)
- ✅ §1.3 Category B Post FX 3 件: Cinematic mode で Sepia/Greyscale/Posterize の slider が UI から実効 (combineGlow + bindDeferredShader 両 site で BD 1:1 dispatch、neutral fallback 付き)
- ✅ §1.4 dead UI 削除: floater_aya_cinematic.xml から Godrays / LensFlare / Tone mapping section が消えている、live cvar 11 件は active UI に昇格
- ✅ §1.5 branding cleanup: floater_aya_cinematic.xml に user-visible "BD" 文字列が消滅 (LGPL attribution は source comment に残す)
- ✅ build green / cache clear / install 完了 — AYA hands-on で Cinematic mode の新 UI 機能を確認 (別作業)
- ✅ 最終 commit: `r30 BD full port Phase 6: live cvar 13 件 1:1 移植 完了`

---

## §6 Post-close polish (2026-05-20)

Phase 6 close 直後に AYA hands-on で発見された UI 整理。**配線スコープ外**の純粋な見栄え / 帰属表示の polish。

- **BD LGPL-2.1 attribution の置き場所変更**: `floater_aya_cinematic.xml` 下部の `T_Credits` テキスト ("DoF / Motion Blur / SSR / Cinematic chain ported from Black Dragon viewer (LGPL-2.1).") を撤去し、`floater_about.xml` Licenses タブに `Black Dragon viewer (LGPL-2.1) Copyright (C) NiranV Dean — DoF / Motion Blur / SSR / Cinematic post-process chain ported into AYAstorm Cinematic mode.` を追記 (en/ja 両方)。floater 高さは 560→520 に縮小。LGPL-2.1 attribution は引続き user-visible (場所が About に移っただけ)。
- **check_box の D ボタン左移動**: 各タブの check_box 23 件で D ボタンが画面右端 (`left=555`) に配置されており、label との対応が視覚的に取りづらかったため、D ボタンを check_box 直前 (`left=12`) に移動し check_box を `left=36 width=506` にずらした。slider 系 D ボタン (`left=445`) は spinner との並びで関係が明確なため現状維持。Cinematic Control の cvar default / slider range / saturation curve の本格 tuning は引続き別 phase (`memory: project_r30_cinematic_control_tuning_deferred.md`)。

両件とも live cvar 配線本体には触れていないため、Phase 6 の「BD 1:1 verbatim port」完了基準には影響しない。
