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
- **sun halo / haze_glow の生成経路** (`skyV.glsl` / `skyF.glsl`): r14 P0 round 1 で解明済み (`docs/archive/r14/sun_rendering_survey.md`)
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
| **B. 物質色** | 薄物の透過 / アルベド忠実度 (frozen) / material response | r19+ |
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

### r17: 時間帯色温度 — **REVERTED — P1.a 復活 (実装完了)**
- 細部: `docs/ayastorm-r17-color-temperature.md` (revert 記録)
- ねらい: 朝・昼・夕・夜の色温度が物理的に正確 (Sun/Ambient/Cloud の Kelvin 解釈)
- **経緯**: P1.a 実装 (`c3d6aee734`) → instant A/B で「効きがわからない」 → drop (`380f5dc245`) → drop 後の sustained viewing で AYA が「Dropしたオレンジの夕焼けの世界は失われた」と評価 → **revert で復活**
- **revert の根拠**: instant A/B では tone mapping 飽和や ambient 副作用で perceptual threshold 以下に見えたが、drop 後の世界を比較すると Sunset preset の orange 夕焼けが cumulative に効いていたことが判明。`feedback_doubt_self_first.md` を自分の drop 判断にも適用、AYA の「効きがわからない」発言を「機能 NG」と一般化したのが早計だった
- **実装結果**: `getR17SunModulator` (太陽 elevation smoothstep 0..0.4 → Kelvin 2200..6500 → Tanner Helland 2012 公式 → mod RGB) + sky path (SUNLIGHT_COLOR/CLOUD_COLOR/ambient) + scene path (mSunDiffuse/ambient) の 3 注入点 modulate + `KNOWN_SKY_LEGACY_MIDDAY` asset UUID pinpoint 除外。個別 switch `AYAR17ColorTemperatureEnabled` (default TRUE) を sentinel として保持
- **運用方針**: 「Day cycle と Sunset/Sunset 系 fixed preset で効くツール」として位置付け、Sunrise/Midnight 等の helper が空回りする preset があることは release note で説明、`feedback_release_with_user_feedback.md` 流で受け入れる
- **teach-back**: 「instant A/B と sustained viewing の効きは別軸で評価する」を新規 lesson として獲得。`feedback_feature_value_in_main_usecase.md` の「動いた ≠ 効いた」を判定するときは A/B 文脈と sustained 文脈の両方で観測してから drop 判断する

### r18: 雲の体積化 + 色温度連動 — **実装完了 (A+B 軸 PASS)**
- 細部: `docs/ayastorm-r18-cloud-volumetric.md`
- ねらい: 既存 flat texture cloud に体積感を付与 + 体積化した雲面に sun の色温度が乗る cinematic 効果、写真撮るに値する空の核 (A 軸完走)
- スコープ変遷: 当初「**雲の体積化 (A 軸) + 色温度連動 (B 軸)**」セットで起票 → r17 drop で B 軸も同伴 drop、「A 軸単独」に re-scope → r17 revert で B 軸復活、**最終的に当初通り A 軸 + B 軸セットで出荷**
- スコープ (確定): `cloudsF.glsl` で既存 2D `cloud_noise_texture` を視線方向 slab raymarch (N=4 step、Beer-Lambert 風 transmittance 累積) で体積化 (A 軸) + `applySpecial` で CLOUD_COLOR を `getR17SunModulator` で乗算 (B 軸)
- 含まない: 雲影 (地表に雲の縞模様) は r19+ 候補、heavy raymarch (全画面 ray-march) は永久 drop
- **実装結果**: 個別 switch `AYAR18CloudVolumetricEnabled` (default TRUE) を新設、`AYA_R18_CLOUD_VOLUMETRIC_ENABLED` shader enum を `llshadermgr.{h,cpp}` に追加。`applySpecial` で master + r18 sentinel + `KNOWN_SKY_LEGACY_MIDDAY` pinpoint 除外で `r18_on` を gate、ON で shader uniform=1 push、OFF で 0。CLOUD_COLOR は `getR17SunModulator` で乗算 (r17 OFF/Legacy で identity)。`cloudsF.glsl` で uniform 分岐、ON 経路は 4 step slab raymarch (UV offset 0.013/0.008、各 slab 45% 透過)、OFF 経路は既存式と数式上完全一致 (preset 互換維持)。3D noise asset 不要、class1/deferred 単一 shader で完結。**AYA 実機 Linux PASS「とても素晴らしい」(A 軸) + 「cinematic な orange 夕焼け」(B 軸 revert 後)**
- **View Mode UI 追加**: Preferences → Graphics → Shaders に `AYAViewMode` combo_box (`Firestorm View / AYAstorm View`) を panel_preferences_graphics1.xml (en/ja) に配線、master cvar `AYAVisualRealismEnabled` を **Boolean → U32** に変更 (combo_box value="0"/"1" 文字列 ↔ Boolean cvar の LLSD 型 coercion 不安定を回避)
- 工数感: 3〜4 日 → 実績は P0 Survey + P1.a 実装 + 実機検証 + r17 drop + r17 revert + UI 配線 + master cvar U32 化で 1.5 日 (Linux 体感 PASS まで)

---

## 5. r19+ (B 軸: 物質色) / r21+ (C 軸: カメラ表現)

A 軸を積み上げ切った段階で詳細化。現時点では骨子のみ:

### B 軸候補 (r19+)
- **薄物の透過 (r19 Linux P1.b PASS)**: 葉 / 白布 / 耳の subsurface 風透過。`class3/deferred/softenLightF.glsl` の PBR / Legacy 両分岐に wrap-around diffuse + back-light transmission を注入、強度 4 段階 (`AYAR19TranslucencyIntensity` 0-3) + 機能 switch (`AYAR19TranslucencyEnabled`) + master gate (`AYAVisualRealismEnabled`)。Linux 実機で板プリム instant A/B + Intensity 段階差 + 厚み変化 (scol 経由副次効果) + sustained / preset 互換 / r14-r18 無干渉 / OFF 復元 / pinpoint 無効化 全 PASS。3 OS ビルドと tag/release は B 軸完走時 (r19-r21) で一括公開判断 (`v7.2.4-ayastorm-r21` 想定)。spec: `docs/ayastorm-r19-translucency.md`、routing リファレンス: `docs/ayastorm-deferred-shader-routing.md`
- **アルベド忠実度 (frozen archive)**: PBR base color の sRGB/linear 取り扱い見直し。Round 3 実装で `HAS_ATMOS_LINEAR` flag scheme は technical PASS したが主流ユースケースで instant A/B 視認困難 (数学的差分 5-6%)、将来 sustained 検証で復活可能。コードは `feature/aya-r19-albedo-fidelity-spec-draft` branch に温存。spec: `docs/ayastorm-r19-albedo-fidelity.md`
- **material response (r20+ 候補)**: roughness / metallic の物理整合性

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
| r17 時間帯色温度 | 1〜2 日 | **REVERTED — 実装完了** (P1.a `c3d6aee734` 実装 → drop `380f5dc245` → drop 後の sustained viewing で「夕焼けの世界が失われた」評価で revert 復活。Kelvin modulator + sky/scene/cloud 3 注入点 + Legacy Midday pinpoint 除外。実績は P0 + P1.a + drop + revert で 1 日強。詳細は `docs/ayastorm-r17-color-temperature.md`) |
| r18 雲の体積化 + 色温度連動 | 3〜4 日 | **実装完了** (P0 Survey `636e163a3c` + P1.a 実装、AYAR18CloudVolumetricEnabled + AYA_R18_CLOUD_VOLUMETRIC_ENABLED uniform 配線、cloudsF.glsl で 2D noise の 4-step slab raymarch、CLOUD_COLOR × r17 sun mod、KNOWN_SKY_LEGACY_MIDDAY pinpoint 除外、OFF パス preset 互換、**AYA 実機 Linux PASS「とても素晴らしい」+「cinematic な orange 夕焼け」**。あわせて `AYAViewMode` combo_box UI + master cvar U32 化。3 OS ビルド / tag は A 軸完走 = r18 close-out で `v7.2.4-ayastorm-r18` 一括公開判断) |
| r14-r18 (A 軸完走) | 12〜19 日 | 参考値 (r14 + r15 + r16 + r17 + r18 で実績 1 + 1 + 1 + 1 + 1.5 日、A 軸 5 リリース完走) |
| r19 薄物の透過 (B 軸第 1 弾) | 2〜3 日 | **Linux P1.b PASS** (P0R1 アルゴリズム決定 + P0R3 全 deferred routing trace + P1.a 実装 + P1.b 受入で実績 1 日、AYAR19TranslucencyEnabled + AYAR19TranslucencyIntensity 0-3、softenLightF PBR/Legacy 両分岐注入、板プリム instant A/B + Intensity 段階差 + 厚み変化 + sustained + preset 互換 + r14-r18 無干渉 + OFF 復元 + pinpoint 無効化 全 PASS、3 OS ビルド / tag は B 軸完走 = r19-r21 一括公開判断) |

---

## 8. 更新履歴

- 2026-05-12: 初版作成。旧 `ayastorm-light-expression-roadmap.md` (3 層モデル + r14 sun dazzle) を全面書き換え。きっかけは r14 P1 (sun disc HDR boost) が体感ゼロで unground し、その unground を契機に AYA から「光表現章ではなく視覚的リアリティ章」「LUT/Tone では届かない」「AAA の暗がり偽装は採用しない」の章 thesis が明示されたこと。3 層モデル (光源/大気/カメラ) を 3 軸モデル (大気・空気 / 物質色 / カメラ表現) に再編、A 軸を r14-r17 で積む計画に再構成。背景は memory `project_ayastorm_visual_realism_chapter.md` 参照
- 2026-05-12 (A 軸第 1 弾 + 第 2 弾 実装完了反映): r14 volumetric atmosphere (`fb76b8391b`〜`e69f2a98b1`) と r15 godrays (`fd027e7475`〜`edaed0fe6a`) を Linux 体感 PASS まで実装完了。§4 r14 / r15 の entry に実装結果を埋め、§7 工数感 table を「実装完了」表示に更新。3 OS フルビルドと tag/release は r13+r14+r15 一括で実施 (`v7.2.4-ayastorm-r15` 想定)。r15 P1 デバッグ中に観測した「scene buffer additive で `frag_color.a=1` を積むと sky mask が破壊される」挙動は memory `project_aya_visual_realism_alpha_protect.md` に永続化 (r16+ で再利用必須の知見)
- 2026-05-12 (A 軸第 4 弾 r17 実装完了反映): r17 時間帯色温度を Linux 体感 PASS まで実装完了。共通 helper `LLSettingsVOSky::getR17SunModulator` + sky / scene / ambient の 3 注入点 modulate + 「昼間(レガシー)」(`KNOWN_SKY_LEGACY_MIDDAY`) の asset UUID pinpoint 除外で着地。実装中の知見として、SL の `canAutoAdjust()` axis は preset の「PBR 互換有無」を表しており「PBR 前再現意図」と一致しないため広い gate には使えない (SL 標準 menu 5 preset 全部が canAutoAdjust=TRUE)。§4 r17 の entry に実装結果を埋め、§7 工数感 table の r17 行を「実装完了」へ更新。tag/release は r13+r14+r15+r16+r17 一括 (`v7.2.4-ayastorm-r17` 想定)、r18 (雲の体積化) 完了で公開判断する流れを継続
- 2026-05-12 (A 軸第 5 弾 r18 起票): r17 close-out (`c3d6aee734`) 直後に r18 を起票。AYA との対話で scope を「**雲の体積化 (B) + 色温度連動 (A)**」のセットに確定 (cloud shadow C は r19+ に分離、scope 膨張回避)。決定理由: 体積化された雲面に r17 Kelvin modulator が乗ると cinematic 効果が桁違いに上がるため別リリース分割では単体評価が難しい、cloud shadow は実装重め (SL の shadow path 改造 or cloud noise projection lighting で 1〜3 日上乗せ) で scope 膨張リスク。§4 r18 entry をスコープ確定で書き換え、§7 工数感 table の r18 行を「**着手**」へ更新。r18 spec は `docs/ayastorm-r18-cloud-volumetric.md`、tag/release 想定は `v7.2.4-ayastorm-r18` (A 軸完走 + 一括公開判断)
- 2026-05-12 (A 軸第 5 弾 r18 P0 Survey 完了): `docs/archive/r18/cloud_volumetric_survey.md` (`636e163a3c`) で 6 P0 ターゲットすべて PASS。class1/deferred の cloudsV/cloudsF 2 ファイルのみ存在 (class2/windlight 配下に同名 shader 無し、R5 解消)、CLOUD_COLOR 注入点は `llsettingsvo.cpp:890` の 1 箇所のみ (r17 SUNLIGHT/AMBIENT の 3 注入点と違って B 軸単純)、cloud は完全 2D `cloud_noise_texture` (sampler2D × 4 UV)、3D noise asset 新規同梱不要 (R2 解消)、HAS_METAL 0 hits / `.metal` cloud shader 無し、drawpool bind 経路不触。P1.a 方針: 既存 2D noise を視線方向 4-step slab raymarch (Beer-Lambert 風 transmittance 45%/slab) で疑似体積化、新規 asset 配布なし、OFF パスは既存式と数式一致で preset 互換維持
- 2026-05-12 (A 軸第 5 弾 r18 P1.a 実装完了): `llshadermgr.{h,cpp}` に `AYA_R18_CLOUD_VOLUMETRIC_ENABLED` enum + uniform 名追加、`settings.xml` に `AYAR18CloudVolumetricEnabled` Boolean (default TRUE)、`llsettingsvo.cpp::applySpecial` で master + r18 sentinel + `KNOWN_SKY_LEGACY_MIDDAY` pinpoint 除外を統合した `r18_on` で gate (B 軸 CLOUD_COLOR を `r17_sun_mod` で乗算 + A 軸 shader uniform push)、`cloudsF.glsl` で `aya_r18_cloud_volumetric_enabled` 分岐の 4-step slab raymarch (UV offset 0.013/0.008、各 slab 45% 透過)。A 軸 / B 軸ともに r17 と同思想の Legacy Midday pinpoint 除外を入れ、OFF パスは既存式と数式上完全一致。§4 r18 entry に実装結果反映、§7 工数感 table の r18 行を「**P1.a 実装完了**」へ更新。AYA 実機確認 (Linux ビルド + 4 preset × AYAR18/AYAR17 ON/OFF) 待ち
- 2026-05-12 (A 軸第 3 弾 r16 実装完了 + r17/r18 分割): r16 aerial perspective (`7427fbcb8d` P1.a + `fc08ffeebc` close-out) を Linux PASS まで実装完了 — scene 経路 `atmosphericsFuncs.glsl` に Rayleigh λ^-4 波長依存 in-scatter を導入、遠景青味シフト体感 PASS、P1.b Preetham 球面近似は体感差なしで drop。あわせて旧 r17 (時間帯色温度 + 雲のリアリティ) を r17 (色温度) / r18 (雲の体積化) に分割、B 軸を r19+、C 軸を r21+ に繰り下げ (A 軸 4 リリース → 5 リリース構成)。色温度を先にする理由は「雲は色温度の影響を受ける側 (sun color が物理的に決まらないと雲の体積感も浮く)」のため
- 2026-05-12 (r17 全体 drop + r18 を A 軸単独に re-scope + A 軸完走): r18 P1.a 実機検証で AYAR18 (slab raymarch) は **AYA「とても素晴らしい」PASS** 、AYAR17 (色温度) は「効きがわからない」と評価。診断ログ (`AYA_R17` tag) で SL の 5 menu preset (朝/昼/夕/夜/レガシー) の太陽 elevation と Kelvin mod を採取、Sunrise preset で elev=0.996 (zenith) → K=6500=identity (no-op)、Sunset で warm preset + R 飽和 → 知覚閾値以下、Midnight で elev=0 → ambient warm 偏り副作用、Estate day cycle (preset=00000000) でのみ effective、という「fixed preset では機能せず」の偏りを確認。根本原因は「SL preset の lightnorm と絵作り色 (SunlightColor/CloudColor) は preset 制作者の経験で独立に設定されており、Sunrise=低い太陽 / Sunset=水平の太陽 という物理的整合が SL 慣行に存在しない」こと。`feedback_feature_value_in_main_usecase.md` (動いた ≠ 効いた) を厳格適用し r17 全体を drop — `getR17SunModulator` / `kelvinToRGB` / `AYAR17ColorTemperatureEnabled` / 診断ログ / sky path mod / scene path mod / ambient mod / cloud path mod (r18 B 軸) をすべて巻き戻し。r18 は **A 軸 (slab raymarch) 単独でリリース確定**。`docs/ayastorm-r17-color-temperature.md` を drop 記録へ書き換え、`docs/ayastorm-r18-cloud-volumetric.md` を「A 軸単独」に re-scope (タイトル変更、§1/§2/§3/§4/§5/§6/§7 更新)。§4 r17 entry を DROP 記録、§4 r18 entry を A 軸単独実装完了、§7 工数感 table を r17→DROPPED / r18→実装完了 へ更新。次は AYA 実機 P2 (macOS / Windows ビルド) を A 軸完走の `v7.2.4-ayastorm-r18` tag に向けて実施
- 2026-05-12 (r17 revert + B 軸復活 + View Mode UI 追加 + master cvar U32 化): drop commit `380f5dc245` 後の sustained viewing で AYA が「Dropしたオレンジの夕焼けの世界は失われた」「夕焼けの美しさはなくなってしまった」と評価。**instant A/B では perceptual threshold 以下に見えた r17 が cumulative には効いていた**ことが drop 後の比較で判明、`feedback_doubt_self_first.md` を自分の drop 判断にも適用して r17 全体を revert。`getR17SunModulator` / `kelvinToRGB` / `AYAR17ColorTemperatureEnabled` を `c3d6aee734` の内容で復活、sky / scene / cloud (r18 B 軸) の 3 注入点 modulator を復元。あわせて (1) Preferences → Graphics → Shaders に `AYAViewMode` combo_box (`Firestorm View / AYAstorm View`) を panel_preferences_graphics1.xml (en/ja) で配線、(2) combo_box value="0"/"1" 文字列 ↔ Boolean cvar の LLSD 型 coercion が「差が殆どない」現象を起こしたため master cvar `AYAVisualRealismEnabled` を **Boolean → U32** に変更 (default=1)、C++ 3 サイト (`llsettingsvo.cpp::applySpecial` 2 箇所、`pipeline.cpp::doGodrays`) を `LLCachedControl<U32>` + `() != 0` 判定に統一、(3) r16 master gate leak (個別 switch だけで gate されていた r16 効果) を `(aya_visual_realism && aya_r16_aerial)` の AND で修正 — Firestorm View で aerial perspective も完全 off に。`docs/ayastorm-r17-color-temperature.md` を「REVERTED — P1.a 復活」へ書き換え、`docs/ayastorm-r18-cloud-volumetric.md` を「雲の体積化 + 色温度連動」(当初スコープ復活) へ書き戻し。§4 r17 entry を REVERTED 実装完了、§4 r18 entry を A+B 軸セット実装完了、§7 工数感 table を r17→REVERTED 実装完了 / r18→A+B 軸 実装完了 + UI 追加へ更新。**新 lesson**: instant A/B と sustained viewing の効きは別軸で評価する、`feedback_feature_value_in_main_usecase.md` を厳格適用するときは A/B 文脈と sustained 文脈の両方で観測してから drop 判断する
- 2026-05-13 (r19 albedo fidelity drop + B 軸第 1 弾を「薄物の透過」に差し替え): r19 albedo fidelity P1.b 実機検証で BoM avatar には `materialF` 経由で writer 修正効くと確認したが、bare prim は別 writer path (`bumpF` / `diffuseAlphaMaskF` 系) を踏むため scope 外、主流ユースケースで instant A/B 視認困難 (数学的差分 5-6%)。Round 3 の `HAS_ATMOS_LINEAR` flag scheme (GBuffer flag 5 値化) は technical PASS、コードは `feature/aya-r19-albedo-fidelity-spec-draft` に **frozen archive** として温存 (commit `d9a6580eab`、将来 sustained 検証で復活可能)。**B 軸第 1 弾を「薄物の透過 (translucency / subsurface 風)」に差し替え**、新 spec `docs/ayastorm-r19-translucency.md` を `feature/aya-r19-translucency-spec-draft` で起票。差し替え理由は (1) 写真撮影テーマ「物質が見せる本来の色と空気」直球、(2) instant A/B で誰でも分かる視覚インパクト (葉が太陽に透ける / 白布カーテン裏に光が回る / 人物の耳が透ける)、(3) r19 albedo の「数学的に正しいが視認困難」問題を構造的に解消。§3 表の B 軸 entry を「薄物の透過 / アルベド忠実度 (frozen) / material response」へ書き換え、§5 B 軸候補で薄物の透過を起票中・アルベド忠実度を frozen archive へ降格。**学習**: GBuffer writer path の scope は survey で最初に全棚卸しすべきだった (机上 listing では `bumpF`/`diffuseAlphaMaskF` 系/`avatarF`/`terrainF`/`treeF` も legacy GBuffer に書く事実が抜けた)。bare prim が実機で踏む path は survey 時点で diagnostic 着色 (`frag_color.rgb = vec3(1,0,0)`) で実機トレース確認すべき。B 軸テーマ選定では「主流ユースケースで instant A/B で見えるか」を起票時点でゲート化、translucency spec §3 P0 Survey で実機トレース必須化として組み込み済み
- 2026-05-13 (B 軸第 1 弾 r19 薄物の透過 Linux P1.b PASS): r19 translucency 起票 (同日) → P0R1 (アルゴリズム = Wrap+Back combo、注入点 = `class3/deferred/softenLightF.glsl` 1 ヶ所) → **P0R3** (canary 着色で全 deferred 経路の writer shader → softenLightF 4 分岐 (PBR/HDRI/SKIP_ATMOS/Legacy) の対応を 2 時間 trace、結果として PBR 服 = `pbropaqueF`→HAS_PBR 分岐、壁/avatar/耳/旧服 = `materialF`→HAS_ATMOS Legacy 分岐の 2 ヶ所注入で完走可能と確定、routing 全分岐を `docs/ayastorm-deferred-shader-routing.md` に永続化) → **P1.a 実装** (`settings.xml` に `AYAR19TranslucencyEnabled` Boolean + `AYAR19TranslucencyIntensity` U32 0-3、`softenLightF.glsl` に uniform 3 件 + ヘルパー 2 件 + PBR 分岐 / Legacy 分岐への注入、`pipeline.cpp` の `renderDeferredLighting` で tier table から uniform push、OFF は strength=0 で shader 短絡) → **P1.b Linux 受入 PASS** (板プリム instant A/B、Intensity 0/1/2/3 段階差、厚み変化を `scol` 経由副次効果として確認、sustained 違和感なし、WindLight 5 preset 互換、r14-r18 維持、master OFF で r13 復元、`AYAR19TranslucencyEnabled = 0` で pinpoint 無効化 全 PASS)。検証中に判明した scope 外: prim transparency > 0 は SL の `alphaF` forward alpha 経路に逃げ softenLightF を経由しない (Intensity 無関係)、forward 経路への注入は P2 候補 / r20 検討。**学習**: (1) 「平面 prim では subsurface 出ない」と初期説明したが `scol` self-shadow 経由で副次的に厚み依存性が機能していた、(2) 検証で動かした debug settings (Persist=1) は最後に default に戻す案内を必ず入れる (memory `feedback_restore_debug_settings.md` に永続化)、(3) routing trace は永続化することで再解析 2 時間を回避 (memory `reference_deferred_shader_routing.md`)。§5 B 軸候補 / §7 工数感 table を P1.b PASS に更新。3 OS ビルドと tag/release は B 軸完走 = r19-r21 で一括公開
