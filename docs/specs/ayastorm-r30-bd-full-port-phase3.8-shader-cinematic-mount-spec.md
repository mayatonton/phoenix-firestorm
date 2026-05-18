# AYAstorm r30 BD 完全移植 — Phase 3.8 shader Cinematic mount spec

**Phase 名**: r30 BD 完全移植 Phase 3.8 — bucket 1.shader common-diff 49 file の Cinematic mount 計画
**前提**: `docs/specs/ayastorm-r30-bd-full-port-inventory.md` (Phase 0) / `docs/specs/ayastorm-r30-bd-full-port-phase1-audit.md` (Phase 1) / `docs/specs/ayastorm-r30-bd-full-port-phase2-spec.md` (Phase 2 D1-D4) / `docs/specs/ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md` (Phase 3.2)
**作成日**: 2026-05-19
**作成方針**: Phase 1 audit bucket 1.shader の REDO 49 file について、Cinematic mode (2) で BD-equivalent shader が走るよう mount 方針を **per-file 確定**。Phase 2 §1.4 D1 で「shader 側 predicate は中央集権 uniform 経由のため別途修正不要」と判断済の path (atmosphericsFuncs / godraysF / skinSSSF / skyV) は Phase 3.7 で C++ 側 uniform 給餌が既に BD default 化されている前提で、本 spec は **shader file 内の構造差分 (code path / output layout / 定数値)** を扱う (`memory/feedback_no_escape_full_bd_coverage.md` 準拠)。

---

## §0 本 spec の出力意図

Phase 2 §1.4 (D1) と Phase 3.2 §1 (cat 03 shadermgr) の handoff を受けて、shader file 単位の mount 行動を確定する。

- 49 REDO shader を **non-substantive (cosmetic)** と **substantive (code-path 差)** に二分し、後者にだけ mount 作業を割り当てる
- substantive 群に対しては「BD 中身を AY に取り込む方法」を 3 strategy (overwrite / preprocessor permutation / dual-file) から file ごとに割り当てる
- mount を割り当てた file は AYAstorm View mode (1) で従来動作を保つ責務も同時に確定する (= AY 拡張を潰さない)
- 検証は per-shader compile + 両 mode 起動 + frame regression で行う

Phase 3.7 で C++ 側 dispatch (uniform 値の BD default 化) は揃った。本 phase 後は Phase 3.9 (UI floater 移植) → Phase 4 (検証) → Phase 5 (cleanup) で r30 Cinematic release に到達する。

---

## §1 49 REDO shader の二段分類

### §1.1 一段目: cosmetic vs substantive

49 file を「コメント / 空白以外の論理差」の有無で二分する。判定ルール:

> 各 file から行コメント (`//`)、ブロックコメント行 (`/*`, `*/`, ` * `)、空白行、行末空白を除去した後、AY 版と BD 版が完全一致なら **cosmetic**、それ以外は **substantive**。

| 分類 | 件数 | 行動方針 |
|---|---:|---|
| cosmetic | **9** | 行動不要 (mode 1/2 とも現状動作が BD-equivalent。AY 由来の attribution コメントは残す価値あるので overwrite しない) |
| substantive | **40** | §1.2 以降で diff 規模別に sub-bucket 化 + mount strategy 確定 |

cosmetic 9 file の確定一覧 (差分は comment 追加・空白整理のみ、論理 BD 同一):

- `class1/deferred/avatarVelocityF.glsl`
- `class1/deferred/avatarVelocityV.glsl`
- `class1/deferred/postDeferredF.glsl` (r30 P4 step 1 で BD chroma uniform を既に取り込み済、annotation のみ追加状態)
- `class1/deferred/postDeferredHQDoFF.glsl`
- `class1/deferred/velocityAlphaV.glsl`
- `class1/deferred/velocityF.glsl`
- `class1/deferred/velocityFuncV.glsl`
- `class1/deferred/velocityV.glsl` (LGPL 提供元 attribution 4行付与)
- `class1/deferred/volumetricLightF.glsl`

### §1.2 二段目: substantive 40 file を diff 行数で sub-bucket 化

行数は `grep -vE '^\s*(//|\*|/\*|\*/)' | sed -E 's/\s+$//; s/^\s+//' | grep -v '^$'` 正規化後の双方向 diff 行数 (`<>` 合算)。大まかな移植難度の proxy。

| sub-bucket | 行数範囲 | 件数 | mount 方針候補 |
|---|---|---:|---|
| S | 2-3 行 (定数 / uniform 増減) | **15** | A (uniform 給餌で吸収) → B (overwrite) を試す |
| M | 4-13 行 (関数 1 個 / 分岐 1 個追加) | **9** | B (overwrite) 優先、AY 拡張併存なら C (permutation) |
| L | 20-31 行 (関数複数 / pass 内 sub-pipeline) | **9** | C (permutation) 必須 |
| XL | 40-76 行 (大規模 path 追加 / 構造変更) | **5** | C (permutation) + per-file walkthrough |
| XXL | 147+ 行 (file 全体書き直し級) | **2** | D (dual-file mount: BD copy を別 path 配置 + shadermgr 切替) |

合計 40 file。

---

## §2 mount strategy 4 種

### §2.1 strategy A: uniform-fed 吸収 (= 何もしない)

AY 拡張が uniform 値で gate されており、Phase 3.7 で uniform が BD default に強制される shader は **shader file を触らない** で完成。Phase 2 §1.4 D1 の「中央集権 uniform 経由」path と同じ思想。

判定基準: AY 拡張のすべての code 差が `if (uniform_name != 0) { ... }` 型で囲まれ、かつ uniform_name が Phase 3.7 で BD default = 0 を流す側になっていること。

S-bucket の 2-3 行差は多くがこれに該当する想定 (定数 1 個 / 0 への degenerate)。per-file 検証で確定する。

### §2.2 strategy B: overwrite (= BD 版で AY 版を置き換え)

AY 拡張が同 file 内に存在せず、AY 版は単に古い BD 版である shader は **AY 版を BD 版で上書き** する。両 mode で同じ shader が走り、両方 BD-equivalent になる (AY 機能を失うものはこの bucket に入らない)。

判定基準: AY 拡張 (FS:AYA / AYAstorm marker) が file 内に**無く**、diff は BD 進化を AY が反映していないだけ。

M-bucket の motionBlurF / skinnedVelocityV / velocityAlphaF 等が候補。

### §2.3 strategy C: preprocessor permutation (#define AYASTORM_CINEMATIC で AY/BD 切替)

AY 拡張と BD 差分が同 file 内に共存している場合、shader 上部に `#define AYASTORM_CINEMATIC 0` (デフォルト)、Cinematic 時のみ shadermgr が `#define AYASTORM_CINEMATIC 1` を inject。AY 機能ブロックと BD-only ブロックを `#if AYASTORM_CINEMATIC` で切替。

判定基準: AY 拡張 (FS:AYA marker / r14-r20 追加 path) が file 内に明らかにあり、Cinematic では BD オリジナル path、AYAstorm View では AY 拡張 path を踏み分けたい。

L, XL の大半はこちらになる想定。

### §2.4 strategy D: dual-file mount (BD copy を別 path 配置)

XXL bucket (shadowUtil 147 行差 / screenSpaceReflUtil 463 行差) は AY 側 file 全体が AY 独自設計に書き直されており、#if での共存が読めなくなるため、`indra/newview/app_settings/shaders/cinematic_bd/class*/...` 等の別 path に BD 版を **そのまま配置**し、llviewershadermgr が Cinematic mode のとき loadShaderFile() のセグメント探索順に cinematic_bd を最優先で混ぜる。

判定基準: file 全体が AY 設計で BD と構造的に共存不能。

実装影響: llviewershadermgr.cpp の loadShaderFile / loadShaderText 周りに cinematic_bd ディレクトリ探索 hook が必要 (Phase 3.2 §2.3.2 で「shader load gate NOT added」と punt したのを本 phase で実施)。

---

## §3 per-file mount 表 (40 substantive shader)

### §3.0 step 1 audit signature

各 file の diff signature を機械抽出 (step 1 audit 出力):

- **AY+=N**: BD baseline に対して AY が追加した行数 (normalized: コメント/空白除外)
- **BD+=N**: BD baseline にあって AY で削除された行数
- **ay_uni**: AY が追加した `uniform` 宣言の数 (uniform-gated extension の indicator)
- **bd_rm**: AY が削除した BD 関数の数

判定 heuristic:

| signature | 解釈 | strategy |
|---|---|---|
| AY+=0, BD+=N | AY が BD 機能削減のみ | **B** (overwrite で BD 復活) |
| AY+=N, BD+=0, ay_uni>0 | AY が uniform-gated 拡張追加 | **A** (neutral 給餌で collapse 確認) / 不十分なら **C** |
| AY+=N, BD+=0, ay_uni=0 | AY が hard-coded 拡張追加 | **C** (permutation 必須) |
| AY+=N, BD+=N | 双方変更 | **C** (permutation) |
| AY+= 100+ AND BD+= 100+ | 構造乖離 | **D** (dual-file) |

### §3.1 sub-bucket S (2-3 行差、15 件) — 確定 strategy

| # | file | AY+= | BD+= | ay_uni | bd_rm | strategy | 確定理由 |
|---|---|---:|---:|---:|---:|---|---|
| 1 | `class1/deferred/SMAA.glsl` | 1 | 1 | 0 | 0 | **C** | 1-line 双方差、permutation |
| 2 | `class1/deferred/moonF.glsl` | 3 | 1 | 0 | 0 | **C** | r20 SSS .a=0 write、permutation |
| 3 | `class1/deferred/postDeferredVisualizeBuffers.glsl` | 1 | 1 | 0 | 0 | **C** | 1-line 差、permutation |
| 4 | `class1/deferred/skyF.glsl` | 4 | 1 | 0 | 0 | **C** | r20 SSS .a=0 write、permutation |
| 5 | `class1/deferred/starsF.glsl` | 3 | 1 | 0 | 0 | **C** | r20 SSS .a=0 write、permutation |
| 6 | `class1/deferred/sunDiscF.glsl` | 3 | 1 | 0 | 0 | **C** | r20 SSS .a=0 write、permutation |
| 7 | `class3/deferred/multiPointLightF.glsl` | 0 | 7 | 0 | 0 | **B** | AY が BD 削減のみ、overwrite |
| 8 | `class3/deferred/pointLightF.glsl` | 0 | 7 | 0 | 0 | **B** | 同上 |
| 9 | `class3/deferred/spotLightF.glsl` | 0 | 7 | 0 | 0 | **B** | 同上 |
| 10 | `class3/environment/waterF.glsl` | 1 | 1 | 0 | 0 | **C** | 1-line 双方差、permutation |
| 11 | `class1/deferred/SMAABlendWeightsF.glsl` | 1 | 2 | 0 | 0 | **C** | 1-line AY tweak、permutation |
| 12 | `class1/deferred/avatarF.glsl` | 9 | 1 | 1 | 0 | **C** | r20 Phase C SSS skin marker uniform-gated、permutation |
| 13 | `class1/deferred/velocityAlphaF.glsl` | 9 | 2 | 0 | 1 | **C** | AY P2 で BD bayerDither helper を alpha cutoff に意図置換、permutation |
| 14 | `class1/objects/previewV.glsl` | 2 | 1 | 1 | 0 | **C** | FS:Beq ambient color 拡張、permutation |
| 15 | `class3/deferred/materialF.glsl` | 9 | 1 | 1 | 0 | **C** | r20 SSS skin marker、permutation |

S-bucket 集計: B = 3, C = 12

### §3.2 sub-bucket M (4-13 行差、10 件) — 確定 strategy

| # | file | AY+= | BD+= | ay_uni | bd_rm | strategy | 確定理由 |
|---|---|---:|---:|---:|---:|---|---|
| 1 | `class1/deferred/pbropaqueF.glsl` | 9 | 4 | 1 | 0 | **C** | r20 Phase C SSS skin marker uniform-gated、permutation |
| 2 | `class1/deferred/skinnedVelocityAlphaV.glsl` | 9 | 2 | 1 | 0 | **C** | AY P2 A2.2 skinning approach 意図差、permutation |
| 3 | `class1/deferred/skinnedVelocityV.glsl` | 11 | 2 | 1 | 0 | **C** | AY が BD original skinning math を AY 用 helper に置換、permutation |
| 4 | `class2/deferred/sunLightSSAOF.glsl` | 0 | 5 | 0 | 1 | **B** | AY が BD SSAO 関数削除、overwrite |
| 5 | `class3/deferred/reflectionProbeF.glsl` | 5 | 2 | 0 | 0 | **C** | 双方差、permutation |
| 6 | `class3/deferred/volumetricLightF.glsl` | 33 | 14 | 0 | 2 | **C** | AY P3 で BD nonpcfShadowAtPos 想定を AY shadow helper に remap、permutation |
| 7 | `class1/deferred/avatarAlphaMaskShadowF.glsl` | 12 | 3 | 0 | 1 | **C** | shadow path AY 拡張、permutation |
| 8 | `class1/deferred/avatarAlphaShadowF.glsl` | 12 | 3 | 0 | 1 | **C** | 同上 |
| 9 | `class1/deferred/pbrShadowAlphaBlendF.glsl` | 12 | 3 | 0 | 1 | **C** | 同上 |
| 10 | `class1/deferred/shadowAlphaMaskF.glsl` | 12 | 3 | 0 | 1 | **C** | 同上 |

M-bucket 集計: B = 1, C = 9

### §3.3 sub-bucket L (20-31 行差、9 件) — 確定 strategy

| # | file | AY+= | BD+= | ay_uni | bd_rm | strategy | 確定理由 |
|---|---|---:|---:|---:|---:|---|---|
| 1 | `class1/deferred/blurLightF.glsl` | 7 | 15 | 0 | 0 | **C** | 双方差、permutation |
| 2 | `class1/deferred/globalF.glsl` | 0 | 29 | 0 | 1 | **B** | AY が BD 機能削減、overwrite |
| 3 | `class1/deferred/motionBlurF.glsl` | 48 | 6 | 0 | 0 | **C** | AY P2 step 5b 拡張 (NaN guard / vel guard / threshold) は意図差、permutation |
| 4 | `class1/interface/glowcombineF.glsl` | 1 | 27 | 0 | 0 | **B** | AY 削減のみ、overwrite |
| 5 | `class1/deferred/cloudsF.glsl` | 35 | 7 | 1 | 0 | **A→C** | r18 cloud volumetric uniform-gated (aya_r18_cloud_volumetric_enabled、Phase 3.7 で 0 給餌済)、collapse 確認後 A 確定、不十分なら C |
| 6 | `class3/deferred/softenLightF.glsl` | 36 | 0 | 2 | 0 | **A→C** | r14+ atmospheric uniform-gated、neutral collapse 確認 |
| 7 | `class1/avatar/objectSkinV.glsl` | 0 | 34 | 0 | 0 | **B** | AY が BD 機能削減、overwrite |
| 8 | `class1/deferred/postDeferredTonemap.glsl` | 35 | 0 | 7 | 0 | **A** | neutral uniform (sat=1/con=1/temp=0/bri=0/lut=off) で applyColorGrading() no-op、Phase 3.7 cat 01 で給餌済、shader 触らず |
| 9 | `class1/windlight/atmosphericsFuncs.glsl` | 63 | 3 | 2 | 0 | **A→C** | aya_visual_realism_enabled / aya_r16_aerial_perspective_enabled で gated、Phase 3.7 で 0 給餌時 collapse 確認 |

L-bucket 集計: B = 3, A = 1, A→C = 3, C = 2

### §3.4 sub-bucket XL (40-76 行差、4 件) — 確定 strategy

| # | file | AY+= | BD+= | ay_uni | bd_rm | strategy | 確定理由 |
|---|---|---:|---:|---:|---:|---|---|
| 1 | `class1/deferred/postDeferredNoDoFF.glsl` | 27 | 42 | 1 | 0 | **C** | 双方大幅変更、AY chroma uniform 拡張 + BD pipeline 更新、permutation |
| 2 | `class1/deferred/skyV.glsl` | 61 | 4 | 1 | 0 | **A→C** | aya_visual_realism_enabled で gated、neutral collapse 確認 |
| 3 | `class1/deferred/tonemapUtilF.glsl` | 41 | 37 | 0 | 3 | **C** | 双方大幅変更、permutation |
| 4 | `class1/deferred/aoUtil.glsl` | 0 | 110 | 0 | 2 | **B** | AY が BD HBAO (作者本人「Shitty ass local」) 削除、overwrite で復活、AY mode 影響なし (caller 既に削除済) |

XL-bucket 集計: B = 1, A→C = 1, C = 2

(§1.2 で XL = 5 と書いたが正は 40-76 行帯 4 件 + 100+ 帯 2 件 = 合計 40)

### §3.5 sub-bucket XXL (147+ 行差、2 件) — 確定 strategy

| # | file | AY+= | BD+= | ay_uni | bd_rm | strategy | 確定理由 |
|---|---|---:|---:|---:|---:|---|---|
| 1 | `class1/deferred/shadowUtil.glsl` | 22 | 165 | 1 | 4 | **B→D** | AY が大幅削減 + 少量追加。overwrite で AY mode 回帰なければ B、回帰あれば D dual-file mount |
| 2 | `class3/deferred/screenSpaceReflUtil.glsl` | 307 | 221 | 11 | 5 | **D** | 双方大幅再設計、構造乖離、dual-file mount 必須 |

XXL-bucket 集計: B/D = 1 (検証で確定), D = 1

### §3.6 step 1 audit 集計

| strategy | 確定数 | 検証要 | 合計 |
|---|---:|---:|---:|
| A (uniform 吸収) | 1 (postDeferredTonemap) | 4 (cloudsF, softenLightF, atmosphericsFuncs, skyV) | 5 |
| B (overwrite) | 8 (multiPointLightF, pointLightF, spotLightF, sunLightSSAOF, globalF, glowcombineF, objectSkinV, aoUtil) | 1 (shadowUtil) | 9 |
| C (permutation) | 26 | 0 | 26 |
| D (dual-file) | 1 (screenSpaceReflUtil) | 0 (shadowUtil 重複候補) | 1 |
| **合計** | **36** | **5** | **41 (重複 1 = shadowUtil)** |

実数 40 = A(1) + B(8) + C(26) + D(1) + 不確定(4 件は A 検証後 A or C、1 件は shadowUtil B or D)。検証 step を経て A 5/C 28/B 8/D 2 等に再 fix する。

---

## §4 実装手順

Phase 3.8 は次の 4 step で進める:

### §4.1 step 1: per-file AY 拡張 marker audit (§3 表の TBD 行を埋める)

- 40 substantive file × `grep -E 'FS:AYA|AYAstorm|AYA_'` で AY marker 有無を確定
- marker なし → strategy B 候補に確定
- marker あり → diff の各 hunk が「BD 進化を AY が反映」か「AY が新規追加」か判別 → C/D 確定
- §3 表を確定状態に書き換え、本 spec を再 commit

### §4.2 step 2: strategy B (overwrite) を一括実施

- 確定した B 群を `cp /tmp/bd-baseline/<path> <path>` で順次上書き
- 上書き対象ごとに 1 commit (`r30 BD full port Phase 3.8 step 2: <file> overwrite (strategy B)`)
- 全 B 終わりで build 1 回、両 mode 起動確認

### §4.3 step 3: strategy C (permutation) を per-file 実施

- 各 C 対象 shader の上部に `#ifndef AYASTORM_CINEMATIC` `#define AYASTORM_CINEMATIC 0` `#endif` を挿入
- AY 拡張 block を `#if AYASTORM_CINEMATIC == 0` で囲い、BD original block を `#else` 側に同等量配置
- llviewershadermgr が Cinematic 時に `#define AYASTORM_CINEMATIC 1` を loadShaderText 前に inject (LLGLSLShader::createShader の preamble 経路)
- per-shader build + 両 mode visual check、1 file 1 commit

### §4.4 step 4: strategy D (dual-file) を実施

- `indra/newview/app_settings/shaders/cinematic_bd/class*/<path>` ディレクトリ作成
- BD 版を該当 path に配置
- llviewershadermgr loadShaderFile に "Cinematic mode の時は cinematic_bd/ を優先" 探索 logic を追加
- 既存の class3 → class2 → class1 fallback chain を尊重して、cinematic_bd 内も class3→class2→class1 探索を保つ

---

## §5 commit unit

| step | 単位 | 例 |
|---|---|---|
| 1 (audit 更新) | spec re-commit 1 回 | `r30 BD full port Phase 3.8 step 1: per-file AY marker audit 完了` |
| 2 (overwrite) | per-file | `r30 BD full port Phase 3.8 step 2: motionBlurF overwrite (strategy B)` |
| 3 (permutation) | per-file | `r30 BD full port Phase 3.8 step 3: postDeferredTonemap permutation (strategy C)` |
| 4 (dual-file) | per-file | `r30 BD full port Phase 3.8 step 4: shadowUtil dual-file mount (strategy D)` |

build verify は各 step の終わりに 1 回、両 mode 動作確認は step 2/3/4 ごと。

---

## §6 punt 禁止項目 (自検)

`memory/feedback_bd_full_port_only.md` / `feedback_no_escape_full_bd_coverage.md` 適用:

- ❌ 「shader は時間かかるので Cinematic でも AY 版で十分」punt → **不採用** (BD parity が崩れる)
- ❌ 「permutation 複雑なので uniform 給餌で全部済ます」推論 → **要 per-file 検証**、推論禁止
- ❌ 「XXL は別 phase に回す」punt → **本 phase で D 戦略まで完遂**
- ❌ 「cosmetic 9 file を念のため overwrite」過剰行動 → **AY attribution を消す行為**、不採用

---

## §7 完了基準

- 49 REDO shader すべてが §3 表で確定 strategy を持つ
- A/B/C/D それぞれの完了 commit が landed
- 両 mode (AYAstorm View / Cinematic) で main scene + α (sky/water/avatar/particles) の visual regression なし
- build green
- 完了 commit message: `r30 BD full port Phase 3.8: shader Cinematic mount 完了 (49 file、A=N, B=N, C=N, D=N)`

---
