# AYAstorm r30 BD 完全移植 — Phase 1 Audit

**Phase 名**: r30 BD 完全移植 Phase 1 — 既存 r30 commit + r14-r20 gating の audit
**前提**: `docs/specs/ayastorm-r30-bd-full-port-inventory.md` (Phase 0)
**作成日**: 2026-05-19
**作成方針**: Phase 0 inventory に対する mechanical な照合のみ。判定列 (KEEP / REDO / DISABLE_IN_CINEMATIC / NEW_PORT_REQUIRED) は inventory 状態から導出、推論で覆さない (`memory/feedback_bd_full_port_only.md`)

---

## §0 Phase 1 の出力意図

Phase 0 inventory が ground truth を出した。Phase 1 はそれに対して 2 方向で照合:

- **1a (commit 照合)**: r30 P1-P5 で AYAstorm に積んだ commit が touched したファイル群 (56 件) を inventory のどの分類に該当するか mechanical に当てる
- **1b (predicate 照合)**: `AYAVisualRealismEnabled` を read している全 site (C++ 12 + shader 6) を列挙し、mode 2 (Cinematic) で BD pure path のみ動かすために修正必要な site を mechanical に挙げる

両方の結果を Phase 2 (= 完全移植 spec 起こし) の入力にする。

### §0.1 verdict 語彙

- **KEEP**: AY 現状が BD と整合 = そのまま残す
- **REDO**: AY 現状が BD と不一致 = BD 中身に下げる、または BD 経路を bind し直す
- **DISABLE_IN_CINEMATIC**: AY 固有 (BD に無い) = Cinematic mode (2) では gate off / dispatch off / UI 不可視化
- **NEW_PORT_REQUIRED**: BD にあり AY に無い = AY に新規追加移植

### §0.2 verdict の読み方注意 (C++ 系 file 限定)

bucket 2.B (newview render cpp/h) で「REDO」が出ているファイル (pipeline.cpp / pipeline.h / lldrawpool*.cpp/h / llviewershadermgr.cpp/h など) は **byte 単位で BD に一致させる意味ではない**。AY 側には r14-r29 期の独自実装 (audio chapter / 視覚表現章 r14+ / r21-r22 picker/chat 等) が積まれており、それを潰せば AYAstorm View mode が壊れる。

C++ 系の「REDO」の意味:

> mode 2 (Cinematic) を実行する code path が BD-equivalent な dispatch を呼ぶように仕立てる (= 中央集権 dispatch + Cinematic 用 BD-borrowed branch を整備)。AY の r14+ 実装は AYAstorm View mode (1) でのみ走るように残す。

shader / cvar / UI XML は **個別 file 単位** で REDO 判定が機能する (= file 自体を BD 内容に reset するか、Cinematic 専用に複製する)。

---

# Phase 1a: r30 P1-P5 touched files vs BD inventory

対象: 56 files

| # | file | bucket | status | verdict | note |
|---|---|---|---|---|---|
| 1 | `indra/llrender/llglslshader.h` | 2.A llrender | common (diff) | **REDO** | BD=4f73491f116bcaaa AY=2cf1cf2617bcb668 → BD 中身採用 |
| 2 | `indra/llrender/llshadermgr.cpp` | 2.A llrender | common (diff) | **REDO** | BD=2c71b52f3b4aa7c8 AY=3a30e75cefe2d7d5 → BD 中身採用 |
| 3 | `indra/llrender/llshadermgr.h` | 2.A llrender | common (diff) | **REDO** | BD=7c3adc7080d49801 AY=77b72c9aa238ff9f → BD 中身採用 |
| 4 | `indra/newview/app_settings/settings.xml` | 3.cvar (settings.xml — 個別 cvar は bucket 3 表参照) | common (diff) | **REDO** | BD=fed887ed9cc963e6 AY=458660d9b3b0c919 → BD 中身採用 |
| 5 | `indra/newview/app_settings/shaders/class1/deferred/SMAAResolveF.glsl` | 1.shader | AY-only | **DISABLE_IN_CINEMATIC** | BD に無い → 純 BD パス汚染 |
| 6 | `indra/newview/app_settings/shaders/class1/deferred/SMAAResolveV.glsl` | 1.shader | AY-only | **DISABLE_IN_CINEMATIC** | BD に無い → 純 BD パス汚染 |
| 7 | `indra/newview/app_settings/shaders/class1/deferred/avatarVelocityF.glsl` | 1.shader | common (diff) | **REDO** | BD=edf3850795b0ece6 AY=8e9fdd64bad4ad89 → BD 中身採用 |
| 8 | `indra/newview/app_settings/shaders/class1/deferred/avatarVelocityV.glsl` | 1.shader | common (diff) | **REDO** | BD=83ce1f6963ce002f AY=6516aa0ac7c29f72 → BD 中身採用 |
| 9 | `indra/newview/app_settings/shaders/class1/deferred/motionBlurF.glsl` | 1.shader | common (diff) | **REDO** | BD=e2abe833150a4e7e AY=43339b53e7e67880 → BD 中身採用 |
| 10 | `indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl` | 1.shader | common (diff) | **REDO** | BD=48cbb81fe025ee40 AY=5d163d691220d8c6 → BD 中身採用 |
| 11 | `indra/newview/app_settings/shaders/class1/deferred/postDeferredHQDoFF.glsl` | 1.shader | common (diff) | **REDO** | BD=3ed617f664a9561b AY=7ea2d8b1730ea778 → BD 中身採用 |
| 12 | `indra/newview/app_settings/shaders/class1/deferred/postDeferredNoDoFF.glsl` | 1.shader | common (diff) | **REDO** | BD=82cc6c74101040cf AY=28e4b0fca0b64c59 → BD 中身採用 |
| 13 | `indra/newview/app_settings/shaders/class1/deferred/skinnedVelocityAlphaV.glsl` | 1.shader | common (diff) | **REDO** | BD=4ba48cc42bfe7846 AY=8240ed9935ac4fa5 → BD 中身採用 |
| 14 | `indra/newview/app_settings/shaders/class1/deferred/skinnedVelocityV.glsl` | 1.shader | common (diff) | **REDO** | BD=253069145dfe46ad AY=813578edaa5b7eea → BD 中身採用 |
| 15 | `indra/newview/app_settings/shaders/class1/deferred/velocityAlphaF.glsl` | 1.shader | common (diff) | **REDO** | BD=ecba19d20b3040e2 AY=930ade3ce652aa5b → BD 中身採用 |
| 16 | `indra/newview/app_settings/shaders/class1/deferred/velocityAlphaV.glsl` | 1.shader | common (diff) | **REDO** | BD=58b5d46156742c5e AY=3537102b85a74403 → BD 中身採用 |
| 17 | `indra/newview/app_settings/shaders/class1/deferred/velocityF.glsl` | 1.shader | common (diff) | **REDO** | BD=7a403c0ff3711fcc AY=a8c583e5fb7fe1f3 → BD 中身採用 |
| 18 | `indra/newview/app_settings/shaders/class1/deferred/velocityFuncV.glsl` | 1.shader | common (diff) | **REDO** | BD=6c69a0b1c4a16afb AY=c636f51b0a6c3ea0 → BD 中身採用 |
| 19 | `indra/newview/app_settings/shaders/class1/deferred/velocityV.glsl` | 1.shader | common (diff) | **REDO** | BD=6bdd1b03f8774780 AY=654d0af317aa970f → BD 中身採用 |
| 20 | `indra/newview/app_settings/shaders/class1/deferred/volumetricLightF.glsl` | 1.shader | common (diff) | **REDO** | BD=9fcde93ae00233c1 AY=8bf17e097e1d152f → BD 中身採用 |
| 21 | `indra/newview/app_settings/shaders/class3/deferred/volumetricLightF.glsl` | 1.shader | common (diff) | **REDO** | BD=d2cf3341a56337b9 AY=0710e66584fd7ee7 → BD 中身採用 |
| 22 | `indra/newview/lldrawable.h` | 2.B newview render | common (diff) | **REDO** | BD=faaf0569eb766053 AY=658446a5ee25bf47 → BD 中身採用 |
| 23 | `indra/newview/lldrawpool.cpp` | 2.B newview render | common (diff) | **REDO** | BD=cb9338f712e78faa AY=1b4ce6c0125efd6f → BD 中身採用 |
| 24 | `indra/newview/lldrawpool.h` | 2.B newview render | common (diff) | **REDO** | BD=48ecc16c56efb789 AY=0e3661c45bfee731 → BD 中身採用 |
| 25 | `indra/newview/lldrawpoolalpha.cpp` | 2.B newview render | common (diff) | **REDO** | BD=faaa0d995177428a AY=f2c4881cf519910d → BD 中身採用 |
| 26 | `indra/newview/lldrawpoolalpha.h` | 2.B newview render | common (diff) | **REDO** | BD=91ed66ccba7a0153 AY=4ab824862c468875 → BD 中身採用 |
| 27 | `indra/newview/lldrawpoolavatar.cpp` | 2.B newview render | common (diff) | **REDO** | BD=ec4f9dc4ef6855b3 AY=6df95538bd2b1658 → BD 中身採用 |
| 28 | `indra/newview/lldrawpoolavatar.h` | 2.B newview render | common (diff) | **REDO** | BD=0f4478739b51d8e4 AY=3425b70641469644 → BD 中身採用 |
| 29 | `indra/newview/lldrawpoolbump.cpp` | 2.B newview render | common (diff) | **REDO** | BD=1035148afe85265e AY=53e861cb1add2e29 → BD 中身採用 |
| 30 | `indra/newview/lldrawpoolbump.h` | 2.B newview render | common (diff) | **REDO** | BD=5ce49d97d96daf3c AY=93dbf768bed3b8fb → BD 中身採用 |
| 31 | `indra/newview/lldrawpoolmaterials.cpp` | 2.B newview render | common (diff) | **REDO** | BD=2e8fbd340655e497 AY=8d73be1268150ad7 → BD 中身採用 |
| 32 | `indra/newview/lldrawpoolmaterials.h` | 2.B newview render | common (diff) | **REDO** | BD=67fe79de99bf2d69 AY=f9561350e11ea6bb → BD 中身採用 |
| 33 | `indra/newview/lldrawpoolpbropaque.cpp` | 2.B newview render | common (diff) | **REDO** | BD=53a98abd784988b0 AY=026bfbeb32b357f6 → BD 中身採用 |
| 34 | `indra/newview/lldrawpoolpbropaque.h` | 2.B newview render | common (diff) | **REDO** | BD=c669ec151a181a3a AY=3a5151ca9ef4dafa → BD 中身採用 |
| 35 | `indra/newview/lldrawpoolsimple.cpp` | 2.B newview render | common (diff) | **REDO** | BD=369a8d3267e536be AY=54a8be34d2fd1c8e → BD 中身採用 |
| 36 | `indra/newview/lldrawpoolsimple.h` | 2.B newview render | common (diff) | **REDO** | BD=d209a47b17e1b321 AY=ec3c5ab142cc1e43 → BD 中身採用 |
| 37 | `indra/newview/lldrawpoolterrain.cpp` | 2.B newview render | common (diff) | **REDO** | BD=fa20a048cd05f969 AY=572b84135549dbdd → BD 中身採用 |
| 38 | `indra/newview/lldrawpoolterrain.h` | 2.B newview render | common (diff) | **REDO** | BD=4132811770a11166 AY=11ff0b7b954e0b50 → BD 中身採用 |
| 39 | `indra/newview/lldrawpooltree.cpp` | 2.B newview render | common (diff) | **REDO** | BD=d27f4049ce67487e AY=a82c79a13aabb0d3 → BD 中身採用 |
| 40 | `indra/newview/lldrawpooltree.h` | 2.B newview render | common (diff) | **REDO** | BD=8d0a476f28708c57 AY=db2eb6fd7d5dcac8 → BD 中身採用 |
| 41 | `indra/newview/llspatialpartition.h` | 2.B newview render | common (diff) | **REDO** | BD=bb8bd18c2a24441b AY=ce184f5ef0bf2799 → BD 中身採用 |
| 42 | `indra/newview/llviewercamera.cpp` | 2.B newview render | common (diff) | **REDO** | BD=decbcc27c9e1b4e3 AY=7c16123edcb6eff1 → BD 中身採用 |
| 43 | `indra/newview/llviewercontrol.cpp` | 2.B newview render | common (diff) | **REDO** | BD=9559a2a78c3cd22c AY=4f54b212bad8ebe1 → BD 中身採用 |
| 44 | `indra/newview/llviewerfloaterreg.cpp` | 2.B newview render | common (diff) | **REDO** | BD=42ce9efc554a8987 AY=a2da0c13258db1c6 → BD 中身採用 |
| 45 | `indra/newview/llviewershadermgr.cpp` | 2.B newview render | common (diff) | **REDO** | BD=75375333c3c36568 AY=583ddd2bb58346e9 → BD 中身採用 |
| 46 | `indra/newview/llviewershadermgr.h` | 2.B newview render | common (diff) | **REDO** | BD=683a012b5b52f4f1 AY=8f1650d8374d52ec → BD 中身採用 |
| 47 | `indra/newview/llvoavatar.cpp` | 2.B newview render | common (diff) | **REDO** | BD=1e370f87503724af AY=3129460d41430b46 → BD 中身採用 |
| 48 | `indra/newview/llvoavatar.h` | 2.B newview render | common (diff) | **REDO** | BD=1cc49867f89b9a84 AY=d6a108a146e198a2 → BD 中身採用 |
| 49 | `indra/newview/llvovolume.cpp` | 2.B newview render | common (diff) | **REDO** | BD=656e4db1ce76c6de AY=66e8855450e81a8d → BD 中身採用 |
| 50 | `indra/newview/pipeline.cpp` | 2.B newview render | common (diff) | **REDO** | BD=eb3c1d8e32180500 AY=faf4817b5b804a04 → BD 中身採用 |
| 51 | `indra/newview/pipeline.h` | 2.B newview render | common (diff) | **REDO** | BD=0006aae1dbd725a1 AY=68519d92d1c14d0b → BD 中身採用 |
| 52 | `indra/newview/skins/default/xui/en/floater_aya_cinematic.xml` | 4.UI | AY-only | **DISABLE_IN_CINEMATIC** | BD に無い → 純 BD パス汚染 |
| 53 | `indra/newview/skins/default/xui/en/menu_viewer.xml` | 4.UI | common (diff) | **REDO** | BD=4f737c438be1a86e AY=00bdcca7a5425f17 → BD 中身採用 |
| 54 | `indra/newview/skins/default/xui/en/notifications.xml` | 4.UI | common (diff) | **REDO** | BD=5d863e5842abf1c5 AY=a76a6205b8ec0832 → BD 中身採用 |
| 55 | `indra/newview/skins/default/xui/en/panel_preferences_graphics1.xml` | 4.UI | common (diff) | **REDO** | BD=048762e204f53c06 AY=ad7cf383188c8ed6 → BD 中身採用 |
| 56 | `indra/newview/skins/default/xui/ja/notifications.xml` | 4.UI | common (diff) | **REDO** | BD=f870a21b4526c09a AY=7650c5c57b991542 → BD 中身採用 |

## 集計

- **KEEP**: 0
- **REDO**: 53
- **DISABLE_IN_CINEMATIC**: 3
- **NEW_PORT_REQUIRED**: 0
- **MANUAL**: 0
# Phase 1b: AYAVisualRealismEnabled gating predicate audit

## §1b.1 全 read site 列挙

| # | site | predicate / 用途 | mode 0 | mode 1 | mode 2 (Cinematic) | category |
|---|---|---|---|---|---|---|
| 1 | `indra/newview/lldrawpoolalpha.cpp:219` | `aya_view_mode_dpa == 2 && volumetric_enable` → alpha pool depth-write extension (P3 godrays 用) | OFF | OFF | ON | A (Cinematic-only) |
| 2 | `indra/newview/llviewershadermgr.cpp:2999` | `aya_view_mode_post == 2` → DEFERRED_CHROMA / HQ_DOF / FRONT_BLUR permutation (P4 postDeferred) | OFF | OFF | ON | A (Cinematic-only) |
| 3 | `indra/newview/llviewershadermgr.cpp:3067` | `aya_view_mode_nodof != 2 \|\| dof_chroma_nodof` → noiseless NoDoF permutation (P4) | (P4 select 経路) | (P4 select 経路) | (P4 select 経路) | A (Cinematic-only) |
| 4 | `indra/newview/llviewershadermgr.cpp:3096` | `aya_view_mode_noise != 2 \|\| dof_chroma_noise` → noisy NoDoF permutation (P4 兄弟) | (P4 select 経路) | (P4 select 経路) | (P4 select 経路) | A (Cinematic-only) |
| 5 | `indra/newview/pipeline.cpp:1053` | `aya_view_mode == 2` → mVelocityMap / mSMAAHistory alloc (P2) | OFF | OFF | ON | A (Cinematic-only) |
| 6 | `indra/newview/pipeline.cpp:9675` | `aya_view_mode == 2 && volumetric_enable && !cubeSnapshot` → renderVolumetric() dispatch (P3 godrays) | OFF | OFF | ON | A (Cinematic-only) |
| 7 | `indra/newview/pipeline.cpp:10502` | `aya_realism_r19()` truthy → r19 Translucency params push (mode 1 OR 2 で active) | OFF | **ON** | **ON** ❌ | **B (master-on)** |
| 8 | `indra/newview/pipeline.cpp:10942` | `realism_enabled() == 0` early-return → r15/r14/r18 godrays dispatch | OFF | **ON** | **ON** ❌ | **B (master-on)** |
| 9 | `indra/newview/pipeline.cpp:10991` | `realism_enabled() == 0 \|\| !r20_enabled()` early-return → r20 SSS dispatch | OFF | **ON** | **ON** ❌ | **B (master-on)** |
| 10 | `indra/newview/llsettingsvo.cpp:779` | `aya_visual_realism() == 0 \|\| !aya_r17` early-return → r17 Sun Kelvin modulator | OFF (= white) | **ON** | **ON** ❌ | **B (master-on)** |
| 11 | `indra/newview/llsettingsvo.cpp:901` | `(aya_master() != 0) && aya_r18_cloud_vol && !is_legacy_midday` → r18 Cloud Volumetric shader uniform | OFF | **ON** | **ON** ❌ | **B (master-on)** |
| 12 | `indra/newview/llsettingsvo.cpp:939-940` | `aya_view = (aya_visual_realism() != 0)` → AYA_VISUAL_REALISM_ENABLED uniform push to atmospherics/sky shaders | 0 | **1** | **1** ❌ | **B (master-on / 中央集権)** |

### Shader 側 read site (uniform 経由、独立 cvar 読まない)

| # | shader | predicate | gating 対象 | mode 2 動作 |
|---|---|---|---|---|
| s1 | `class1/windlight/atmosphericsFuncs.glsl:111` | `aya_visual_realism_enabled > 0` | r14 altitude density | uniform=1 のため ON ❌ |
| s2 | `class1/windlight/atmosphericsFuncs.glsl:163` | `aya_visual_realism_enabled > 0` | r14 新 linear path | uniform=1 のため ON ❌ |
| s3 | `class1/deferred/godraysF.glsl:60` | `aya_visual_realism_enabled <= 0` early-out | r15 godrays | uniform=1 のため ON ❌ |
| s4 | `class1/deferred/skinSSSF.glsl:77` | `aya_visual_realism_enabled <= 0 \|\| aya_r20_skin_sss_enabled <= 0` | r20 SSS | uniform=1 のため ON ❌ |
| s5 | `class1/deferred/skyV.glsl:170` | `aya_visual_realism_enabled > 0` | r14 sky path | uniform=1 のため ON ❌ |
| s6 | `class1/deferred/skyV.glsl:198` | `aya_visual_realism_enabled > 0` | r14 add_below_cloud | uniform=1 のため ON ❌ |

(s1-s6 は llsettingsvo.cpp:939-940 の中央集権 uniform 経由なので、L12 を修正すれば一括で mode 2 OFF になる)

## §1b.2 純 BD Cinematic 化に必要な predicate 変更点

mode 2 (Cinematic) で BD pure path のみ動かすには、category B の 6 site (#7-12) を以下に変更する:

| # | 現状 | 純 BD 化のため | 効果 |
|---|---|---|---|
| 7 | `pipeline.cpp:10502` r19 truthy check | `aya_realism_r19() == 1` に | mode 2 で r19 translucency params 押下 skip |
| 8 | `pipeline.cpp:10942` `== 0` early-out | `!= 1` early-out に | mode 2 で r15 godrays dispatch skip |
| 9 | `pipeline.cpp:10991` `== 0` early-out | `!= 1` early-out に | mode 2 で r20 SSS dispatch skip |
| 10 | `llsettingsvo.cpp:779` `== 0` early-out | `!= 1` early-out に | mode 2 で r17 Kelvin modulator = white |
| 11 | `llsettingsvo.cpp:901` `!= 0` 判定 | `== 1` 判定に | mode 2 で r18 cloud volumetric uniform=0 |
| 12 | `llsettingsvo.cpp:939-940` `!= 0` 中央集権 | `== 1` 中央集権に | mode 2 で uniform=0 → s1-s6 すべて skip (一括) |

L12 修正 1 点で shader 側 6 site が連動。C++ 側 5 site (#7-11) は独立修正必要。**計 6 行変更**。

## §1b.3 verdict (Phase 1b mechanical 結論)

- **category A**: 6 site = 既に Cinematic-only gating、純 BD パス保護要件と一致 → **KEEP**
- **category B**: 6 site = r14-r20 効果が mode 2 で漏れる、純 BD パス汚染 → **REDO (predicate を `== 1` に)**

修正方針:

- (Phase 2 spec で確定 / Phase 3 で実装) llsettingsvo.cpp / pipeline.cpp の 6 site を `== 1` 判定に統一
- r14-r20 効果は AYAstorm View mode (= mode 1) でのみ active、Cinematic mode (= mode 2) では一切 active にしない
- `feedback_bd_full_port_only.md` の方針通り、Cinematic は純 BD パス


---

## §2 Phase 2 への引き継ぎ

Phase 1a + 1b の verdict を受けて、Phase 2 (完全移植 spec) で扱うべき項目:

### §2.1 Phase 2 spec のスコープ (要素別)

| 要素 | Phase 1a/1b 出力 | Phase 2 で書くべきこと |
|---|---|---|
| shader (bucket 1) | REDO 49 + DISABLE_IN_CINEMATIC 13 + NEW_PORT_REQUIRED 0 (motionBlurV は既に P2 で取り込み済) | shader 別の BD reset / Cinematic-only mount 戦略、AY-only shader (godrays/SSS/SMAA/rlv) の mode 2 dispatch 除外 |
| llrender (bucket 2.A) | REDO 23 / 共通 lib なので Cinematic 限定にできない、AY 統一実装で BD-equivalent 動作させる | 各 llrender file の BD diff 行内容を walk、AY 側に積む必要があれば最小 patch を Phase 2 spec に書く |
| newview render cpp/h (bucket 2.B) | REDO 92 / mode 2 path を BD-equivalent に揃える | pipeline.cpp / lldrawpool* の Cinematic dispatch を BD pipeline 順序に揃える spec (= P3/P4 で漏れた順序を全部洗う) |
| cvar (bucket 3) | value diff 33 + type diff 7 + AY-only 579 (Cinematic では default 不適用) | Cinematic preset 機能ではなく、Cinematic mode 時に「BD default で読む」 dispatch を pipeline 内に組む方針 (= preset 不採用、`memory/feedback_bd_full_port_only.md`) |
| UI XML (bucket 4) | REDO 25 + DISABLE_IN_CINEMATIC 9 + NEW_PORT_REQUIRED 9 (BD-only floater) | BD-only floater の AY 移植 (= BD 撮影 UI を Cinematic mode 用に追加)、AY-only floater (floater_aya_cinematic.xml 等) の Cinematic mode 不可視化 |
| presets (bucket 5) | windlight 一致 48 / BD-only 63 / AY-only 721 | BD-only windlight preset の AY 取り込み (= BD-style 自然環境 preset)、Cinematic mode で AY-only preset を選択不可にするか、preset は data layer として残し render side が BD path で扱う方針 |
| r14+ gating (1b cat B) | REDO 6 site (`== 1` 判定に統一) | mode 2 で r14-r20 全機能 OFF、predicate 変更 6 行を Phase 3 で実装 |

### §2.2 Phase 2 spec で確定すべき architectural decision

Phase 1 の audit からは下記の追加判断が要る:

- D1. mode 2 (Cinematic) は AYAstorm View r14+ 効果を **全部** off する (= `feedback_bd_full_port_only.md` 直接帰結) → Phase 1b §1b.2 修正で確定
- D2. C++ 系 file は AY 単一実装で mode 別 dispatch (= `if (cinematic) {BD path} else {AY path}`) か、別 file 化か → Phase 2 で確定
- D3. cvar は Cinematic preset 機能で扱わず、mode 2 検出時に `LLCachedControl` の代わりに hard-coded BD default を読む dispatch 層を pipeline / dispatcher に追加するか、または mode 切替時に Cinematic-specific defaults を gSavedSettings に push する起動 logic を入れるか → Phase 2 で確定
- D4. windlight preset (bucket 5) は data layer のため、Cinematic では BD-only preset のみ選択可にする UI gate を入れるか、AY-only preset の data を BD path 互換で render する logic を入れるか → Phase 2 で確定

### §2.3 Phase 3 への punt 禁止

「Phase 3 着手時に判定」「実装時に決める」等の punt は Phase 2 spec に書かない (`memory/feedback_no_escape_full_bd_coverage.md`)。判断軸を Phase 2 内で明示確定する。

---

## §3 Phase 1 から判明した r30 既存 commit の総括

- **AY 単独で BD と byte-一致しているファイル: 0** (= P2-P5 で touch した全 56 file が何らかの形で BD と divergent)
- **完全に新規取り込み (BD-only) で AY に足したファイル: motionBlurV.glsl 1 件のみ** (P2 step 1 commit dbe4fc508b)
- **AY 独自追加 (BD に無い、Cinematic で除外要) ファイル: SMAAResolve{F,V}.glsl + floater_aya_cinematic.xml 3 件**
- 残り 52 file は「BD と異なる中身で AY に積まれた」(= REDO 対象)

= P2-P5 の borrow-then-modify approach は inventory レベルで 1 件しか「純 BD」を達成していない (motionBlurV のみ、しかも本体 motionBlurF は AY mod が +42 行入って divergent)。

Phase 2 spec はこの事実を踏まえて、**borrow 単位ではなく BD pipeline 全体単位** で移植戦略を組む。

---

**End of Phase 1 Audit.**
