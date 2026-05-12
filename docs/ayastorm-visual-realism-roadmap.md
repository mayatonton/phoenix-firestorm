# AYAstorm 視覚的リアリティ章 長期ロードマップ (r14 →)

**作成日**: 2026-05-12 (初版、旧 `ayastorm-light-expression-roadmap.md` を全面書き換え)
**対象**: AYAstorm r14+ の視覚表現拡張シリーズ
**位置づけ**: r13 で完結した音響表現章 (`docs/ayastorm-stream3d-roadmap.md`) の後継章

---

## 1. 章の核 (core thesis)

r14+ で AYAstorm が追うのは「**写真を撮るに値する空気と空間**」「**物質が見せる本来の色と空気と雰囲気**」。

- 現状の SL も含めて他のゲームのスクリーンショットは絵画的で、写真や映画の世界に対して実にゲーム的、という観察が出発点
- **LUT や Tone では届かないもっと根本的なもの**: scene-referred な物理を内部で組み直す必要がある
- **AAA は影や暗がりのライトでリアリティを偽装する方向に寄せがち**: AYAstorm はこの逆を取る。明るい場所・曇り・雨でも空気感が出る表現を狙う、暗くするだけの解決策は採用しない
- r13 までで音という空間演出を現実的にする工程をやり切った。その自然な延長として **物質が見せる本来の色・空気・雰囲気** を視覚側でも見直す

光表現は入口にすぎず、本丸は **物質・空気・雰囲気の総体としての写真的リアリティ**。

---

## 2. 設計制約 (boundary conditions)

「莫大な資源を捨てない」「必要なら既存 Firestorm の枠を踏み越える」の両立点として、**外側の契約は保ち、内側の計算は書き換える**。

### 保つもの (= 共通基盤、外したら fork 維持コスト爆発)
- **WindLight / Environment preset 互換**: 朝・夕方・夜・region 個別 preset 等の数値 (Sun color, Ambient, Blue Horizon, Haze, Cloud 等) はそのまま input として受け取る。estate operator / 配信者 / AYA 自身の preset 資産は無効化しない
- **deferred → HDR scene buffer → tonemap → LDR の骨格**: Linden 本家との merge 経路。これを断つと AYAstorm 単独 fork のメンテが破綻
- **PBR / deferred の shader interface**: frag_data layout、GBuffer flag、uniform 命名

### 書き換えていいもの (= 内側、流用しても得しない)
- **atmospheric scattering の計算式** (`atmosphericsFuncs.glsl` 等): 経験式ベースから scene-referred な物理ベースへ
- **sun halo / haze_glow の生成経路** (`skyV.glsl` / `skyF.glsl`): r14 P0 round 1 で解明済み (`doc/r14/sun_rendering_survey.md`)
- **fog の適用方法**: 単純 lerp から depth-driven Beer-Lambert へ
- **tonemap / postDeferredGammaCorrect の中身**: 物理露出 + scene-referred で組み直し

### 境界線の言い換え
> **WindLight preset を input として受け取り、scene-referred で物理的に再解釈する**

input (preset 値) と output (HDR scene buffer の信号特性、tonemap 取付け点) の契約は保つ。中の計算は r14-r17 で自由に書き換える。

### preset との関係 (FAQ)
- preset データは **失われない・ゼロから作り直しでもない**
- 朝 preset は朝らしく (暖色・低い太陽・霞)、夕方は夕方らしく出る、ただし「空気感」が物理的に改善された朝・夕方になる
- 過去 SS との完全一致は取れない、ただし master switch (`AYAVisualRealismEnabled`) で r13 までの見え方に戻せる設計を r14 から維持

---

## 3. 3 軸モデル

視覚的リアリティを構成する 3 軸を直交に積む。各軸は独立に効果が出て、重なると掛け算で効く。

| 軸 | 何を解く問題か | 担当リリース候補 |
|---|---|---|
| **A. 大気・空気** | 空気の体積感 / 距離感 / 時間帯の色 / 雲の体積感 | r14-r18 |
| **B. 物質色** | アルベド忠実度 / 薄物の subsurface / material response | r19+ |
| **C. カメラ表現** | 自然な DoF / scene-referred 露出階調 / grain・vignette・収差の抑制活用 | r21+ |

3 軸は AYA の章 thesis (memory `project_ayastorm_visual_realism_chapter.md`) から導出。AAA が偏重する Layer C (post-process で誤魔化す) ではなく、A → B → C の順で **物理的な土台から積む**。

---

## 4. r14-r18 (A 軸: 大気・空気)

A 軸を 5 リリースに分割。各リリースは独立に完結し体感が出る単位。

### r14: volumetric atmosphere (空気の体積感) — **実装完了**
- 細部: `docs/ayastorm-r14-volumetric-atmosphere.md`
- ねらい: 距離と高度に応じて空気が体積として見える。今の「色付きフィルター」状の霞を、depth-driven Beer-Lambert + altitude density で再解釈
- スコープ: 既存 atmospherics shader の内側を書き換え、preset の Haze Horizon / Haze Density / Blue Density 値を物理パラメタとして再解釈
- 含まない: heavy raymarch (r15)、godrays (r15)、距離 aerial perspective (r16)、時間帯色温度 (r17)
- **実装結果**: master switch `AYAVisualRealismEnabled` (default TRUE) + altitude density + scene-referred 積分 (sky shader を blue / haze 分割で linear 空間合成) で着地。P2.a refined で完了、P2.b/c (Preetham 方向経路長 / sun disc HDR boost) は太陽 disc 消失副作用のため deferred。commit 範囲 `fb76b8391b`〜`e69f2a98b1`
- 工数感: 3〜5 日 → 実績は P0 (5/12) → P2.a refined (5/12) で 1 日 (Linux 体感調整完了まで)

### r15: godrays (光線が空間を貫く) — **実装完了**
- 細部: `docs/ayastorm-r15-godrays.md`
- ねらい: 雲間 / 木漏れ日 / 窓から差す光線が空間を貫く体感
- スコープ: shadow map driven の screen-space godrays (radial blur ではなく ray-march 系)
- **実装結果**: `renderGeomPostDeferred` の `doAtmospherics` 直後に挿入する fullscreen pass。既存 cascaded sun shadow を流用 (`sampleDirectionalShadow` + cascade 範囲外 skip + NaN/Inf ガード)、N=16 サンプルの shadow-driven ray-march + Mie 前方ピーク phase (cos^8) + strength=0.10。**alpha 保護必須** (`frag_color.a = 0`、`ONE/ONE` additive で alpha=1 を積むと scene buffer の sky mask が破壊され空が真っ白に潰れる; memory `project_aya_visual_realism_alpha_protect.md`)。master switch `AYAVisualRealismEnabled` を r14 と共有。commit 範囲 `fd027e7475`〜`edaed0fe6a`
- 工数感: 2〜4 日 → 実績は P0 → P1 で 1 日 (Linux 体感 PASS まで)

### r16: aerial perspective (距離による色変化) — **実装完了**
- 細部: `docs/ayastorm-r16-aerial-perspective.md`
- ねらい: 遠景が距離と共に減衰・色相変化、空気の遠近感が物理的に出る
- スコープ: scene 経路 `atmosphericsFuncs.glsl` に Rayleigh λ^-4 波長依存重み (`rayleigh_w = (1.0, 2.33, 5.71)`) を `combined_haze` と `blue_weight` に乗算。`light_atten` (太陽光路) には適用しない (= aerial perspective ≠ 夕焼けの物理分離)
- **実装結果**: P1.a (波長依存 in-scatter) で Linux PASS — 遠景青味シフト明確、近景変化なし、sun disc 健在、preset 互換維持。P1.b (Preetham 球面近似) は数値検証 PASS だが体感 perceptual threshold 以下のため drop。個別 switch `AYAR16AerialPerspectiveEnabled` を新設 (master `AYAVisualRealismEnabled` 単独切替では r14/r15 も同時 off になり r16 単独体感評価不能の事情から不可避と判明)。commit `7427fbcb8d` (P1.a) + `fc08ffeebc` (close-out)
- 工数感: 2〜3 日 → 実績は P0 → P1.a → P1.b 検証 → drop で 1 日 (Linux 体感 PASS まで)

### r17: 時間帯色温度
- ねらい: 朝・昼・夕・夜の色温度が物理的に正確 (Sun/Ambient の Kelvin 解釈)
- スコープ: Sun/Ambient color の色温度 (Kelvin) 解釈、現状の preset 経験式色を物理的に再解釈。preset 互換維持 (input は経験式 RGB のまま、内側で色温度として解釈)
- **実装結果**: P1.a (Kelvin modulator × preset 色の C++ 乗算 path) で Linux PASS — 夕方 amber 明瞭、朝方 subtle、昼 OFF と差なし、preset 切替健全、sun disc 健在。共通 helper `LLSettingsVOSky::getR17SunModulator(lightnorm, psky)` で sky / scene / ambient の 3 注入点を整合。「昼間(レガシー)」(`KNOWN_SKY_LEGACY_MIDDAY`) は PBR 前 noon 再現 preset として asset UUID 完全一致で r17 modulator から pinpoint 除外 (`canAutoAdjust()` での広い gate は SL 標準 5 menu preset 全部を巻き込むため不可)。個別 switch `AYAR17ColorTemperatureEnabled` (default TRUE) を新設、master `AYAVisualRealismEnabled` の下に並列
- 工数感: 2〜3 日 → 実績は P0 Survey Round 1 + Round 2 補遺 (legacy noon UUID gate) + P1.a 実装で 1 日 (Linux 体感 PASS まで)

### r18: 雲のリアリティ (体積感)
- ねらい: 既存 flat texture cloud に体積感を付与、写真撮るに値する空の核
- スコープ: cloud shader の volumetric 化 (軽量、heavy raymarch ではない、analytic + 軽量 raymarch の組合せ)
- 工数感: 3〜4 日 (3 OS 込み、未確定)

---

## 5. r19+ (B 軸: 物質色) / r21+ (C 軸: カメラ表現)

A 軸を積み上げ切った段階で詳細化。現時点では骨子のみ:

### B 軸候補 (r19+)
- アルベド忠実度: PBR base color の sRGB/linear 取り扱い見直し
- 薄物の subsurface scattering: 葉・カーテン・薄い布
- material response: roughness / metallic の物理整合性

### C 軸候補 (r21+)
- 自然な DoF: 物理レンズベース
- scene-referred 露出階調: auto-exposure / tonemap 全面再考
- grain / vignette / chromatic aberration: 抑制的活用 (LUT 系の「写真風 look」は本章で明示的に拒否)

---

## 6. 採用しない方針 (drop)

- **LUT / tonemap / color grade による安易な「写真風 look」**: AYA が明示的に拒否。scene-referred pipeline 全体の精緻化を取る
- **AAA 風の暗がり依存**: 明るい場所 / 曇り / 雨でも空気感が出る表現を狙う、暗くするだけの解決策は採用しない
- **重い raymarch volumetric (毎フレーム全画面 ray-march)**: 配布負債と GPU 負荷の両面で AYAstorm の流儀 (1 viewer で完結、3 OS) に合わない。analytic + 軽量 raymarch の組合せで近似
- **軸 / 機能ごとの個別 debug settings 量産**: master switch 1 本 (`AYAVisualRealismEnabled`) で章全体を ON/OFF、`feedback_prefer_defaults_over_config.md` に従う

---

## 7. 工数感 (参考)

`project_release_workload_norms.md` の物差し (r8 実績 2 日、r9 見積 3〜4 日) で。

| リリース | 工数感 (3 OS 込み) | 状態 |
|---|---|---|
| r14 volumetric atmosphere | 3〜5 日 | **実装完了** (`fb76b8391b`〜`e69f2a98b1`、Linux 体感 PASS、3 OS ビルド / tag は r13+r14+r15+r16 一括で実施予定) |
| r15 godrays | 2〜4 日 | **実装完了** (`fd027e7475`〜`edaed0fe6a`、Linux 体感 PASS、3 OS ビルド / tag は r13+r14+r15+r16 一括で実施予定) |
| r16 aerial perspective | 2〜3 日 | **実装完了** (`7427fbcb8d` P1.a + `fc08ffeebc` close-out、Linux 体感 PASS、3 OS ビルド / tag は r13+r14+r15+r16+r17 一括で実施予定) |
| r17 時間帯色温度 | 2〜3 日 | **実装完了** (P1.a + KNOWN_SKY_LEGACY_MIDDAY UUID gate、Linux 体感 PASS、3 OS ビルド / tag は r13+r14+r15+r16+r17 一括で実施予定) |
| r18 雲の体積化 | 3〜4 日 | 候補 |
| r14-r18 (A 軸完走) | 12〜19 日 | 参考値 (r14 + r15 + r16 + r17 は実績 1 + 1 + 1 + 1 日でかなり前倒し) |

---

## 8. 更新履歴

- 2026-05-12: 初版作成。旧 `ayastorm-light-expression-roadmap.md` (3 層モデル + r14 sun dazzle) を全面書き換え。きっかけは r14 P1 (sun disc HDR boost) が体感ゼロで unground し、その unground を契機に AYA から「光表現章ではなく視覚的リアリティ章」「LUT/Tone では届かない」「AAA の暗がり偽装は採用しない」の章 thesis が明示されたこと。3 層モデル (光源/大気/カメラ) を 3 軸モデル (大気・空気 / 物質色 / カメラ表現) に再編、A 軸を r14-r17 で積む計画に再構成。背景は memory `project_ayastorm_visual_realism_chapter.md` 参照
- 2026-05-12 (A 軸第 1 弾 + 第 2 弾 実装完了反映): r14 volumetric atmosphere (`fb76b8391b`〜`e69f2a98b1`) と r15 godrays (`fd027e7475`〜`edaed0fe6a`) を Linux 体感 PASS まで実装完了。§4 r14 / r15 の entry に実装結果を埋め、§7 工数感 table を「実装完了」表示に更新。3 OS フルビルドと tag/release は r13+r14+r15 一括で実施 (`v7.2.4-ayastorm-r15` 想定)。r15 P1 デバッグ中に観測した「scene buffer additive で `frag_color.a=1` を積むと sky mask が破壊される」挙動は memory `project_aya_visual_realism_alpha_protect.md` に永続化 (r16+ で再利用必須の知見)
- 2026-05-12 (A 軸第 4 弾 r17 実装完了反映): r17 時間帯色温度を Linux 体感 PASS まで実装完了。共通 helper `LLSettingsVOSky::getR17SunModulator` + sky / scene / ambient の 3 注入点 modulate + 「昼間(レガシー)」(`KNOWN_SKY_LEGACY_MIDDAY`) の asset UUID pinpoint 除外で着地。実装中の知見として、SL の `canAutoAdjust()` axis は preset の「PBR 互換有無」を表しており「PBR 前再現意図」と一致しないため広い gate には使えない (SL 標準 menu 5 preset 全部が canAutoAdjust=TRUE)。§4 r17 の entry に実装結果を埋め、§7 工数感 table の r17 行を「実装完了」へ更新。tag/release は r13+r14+r15+r16+r17 一括 (`v7.2.4-ayastorm-r17` 想定)、r18 (雲の体積化) 完了で公開判断する流れを継続
- 2026-05-12 (A 軸第 3 弾 r16 実装完了 + r17/r18 分割): r16 aerial perspective (`7427fbcb8d` P1.a + `fc08ffeebc` close-out) を Linux PASS まで実装完了 — scene 経路 `atmosphericsFuncs.glsl` に Rayleigh λ^-4 波長依存 in-scatter を導入、遠景青味シフト体感 PASS、P1.b Preetham 球面近似は体感差なしで drop。あわせて旧 r17 (時間帯色温度 + 雲のリアリティ) を r17 (色温度) / r18 (雲の体積化) に分割、B 軸を r19+、C 軸を r21+ に繰り下げ (A 軸 4 リリース → 5 リリース構成)。色温度を先にする理由は「雲は色温度の影響を受ける側 (sun color が物理的に決まらないと雲の体積感も浮く)」のため
