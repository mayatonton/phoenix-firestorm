# AYAstorm Control Linux/Mac rendering controls fix plan

**Status**: IN PROGRESS (2026-05-23)
**Work branch**: `fix/ayastorm-cloud-postprocess-chain`
**Base branch**: `fix/macos-glsl-guards`
**Scope**: AYAstorm Control / Cinematic rendering controls の Linux/Mac 実機報告を整理し、調査・修正順序を確定する。

## 0. 背景

AYAstorm Control の Linux 版検証で、複数の描画コントロールが「効かない」「効果が見えない」「post process が壊れている疑い」として報告された。Mac 版でも同じ項目を再検証したところ、以下のように分類できる。

- Linux 固有疑い: Glow/Bloom OFF 時の UI 白塗り、LUT 適用不良
- Mac/Linux 共通疑い: 影ぼかしサイズ、SSR、Volumetric、Fullbright texture、雲描画
- Mac では効果確認済み: DoF 系、CAS、LUT、SSAO の一部
- UI レンジ/実効レンジ不一致疑い: SSAO Factor、F 値、最大散乱円など

本計画では、単に「効く/効かない」を表で終わらせず、各 cvar が UI、cached setting、pipeline、shader uniform、shader permutation のどこで途切れているかを追う。

## 1. 報告整理

### 1.1 Linux 版報告

| ID | 項目 | 報告 |
|---|---|---|
| L1 | Glow/Bloom OFF | UI 文字などに白い四角塗りが出て文字が読めない |
| L2 | 影ぼかしサイズ | 効かない |
| L3 | SSAO Scale | 効いているかわからない |
| L4 | DoF 焦点 FOV | 効果がわからない |
| L5 | F 値 | 64 より上がおそらく意味がない |
| L6 | 最大散乱円 | 効果がわからない |
| L7 | 色収差強度 | 効果がわからない |
| L8 | SSR 全項目 | 効果がわからない |
| L9 | Glow/Volumetric 暖色量 | 効果がわからない |
| L10 | Volumetric Lighting 全項目 | 効果がわからない。方向性フェード ON/OFF は確認済み |
| L11 | Fullbright texture | 効果がわからない |
| L12 | CAS sharpness | 効果がわからない |
| L13 | Clouds / 3D cloud depth | 雲が描画されず、3D cloud depth を確認できない |
| L14 | LUT | `None` 復旧後、LUT が適用されなくなった疑い。Post process が死んでいる疑い |

### 1.2 Mac 版検証

| ID | 項目 | Mac 検証結果 |
|---|---|---|
| M1 | Glow/Bloom OFF | UI 白塗りは再現せず。文字は読める |
| M2 | 影ぼかしサイズ | 効かない |
| M3 | SSAO Factor | `0.00` から `0.20` の間だけ変動。それ以上は見た目変化なし |
| M4 | SSAO Max Scale | 全範囲で変動あり |
| M5 | SSAO Scale | `Max Scale` 以下なら変動あり |
| M6 | DoF 焦点 FOV | 焦点より手前のものに対してぼかしが効く |
| M7 | F 値 | DoF 焦点 FOV との兼ね合いで効く |
| M8 | 最大散乱円 | 強い DoF 状態では効く |
| M9 | 色収差強度 | 強い DoF 状態では効く |
| M10 | SSR 全項目 | 効果がわからない |
| M11 | Glow/Volumetric 暖色量 | 効果がわからない |
| M12 | Volumetric Lighting 全項目 | 方向性フェード ON/OFF 以外は効果がわからない |
| M13 | Fullbright texture | 効果がわからない |
| M14 | CAS sharpness | うっすら効果あり。4K 以上でないと判別困難なレベル |
| M15 | Clouds / 3D cloud depth | 雲が描画されない |
| M16 | LUT | Mac では適用される |

## 2. 暫定分類

### 2.1 高優先度: 共通不具合疑い

| ID | 項目 | 理由 | 初期仮説 |
|---|---|---|---|
| P1 | Clouds が描画されない | Linux/Mac 両方で再現 | environment/cloud render pass、shader permutation、または mode gate の破損 |
| P2 | 影ぼかしサイズが効かない | Linux/Mac 両方で再現 | `RenderShadowBlurSize` または `RenderShadowGaussian` が blur shader/pass に届いていない |
| P3 | SSR 全項目が効果不明 | Linux/Mac 両方で再現 | SSR 自体が gate で無効、または UI cvar と shader uniform が未接続 |
| P4 | Volumetric Lighting の大半が効果不明 | Linux/Mac 両方で再現 | default/sample count が低すぎる、uniform 未接続、または additive pass が別 pass で上書き |
| P5 | Fullbright texture が効果不明 | Linux/Mac 両方で再現 | UI 名と実装対象が不一致、または検証 material 条件不足 |

### 2.2 中優先度: Linux 固有疑い

| ID | 項目 | 理由 | 初期仮説 |
|---|---|---|---|
| P6 | Glow/Bloom OFF 時の UI 白塗り | Mac では再現せず | Linux GL の post-process fallback、UI texture combine、または glow off path の framebuffer 初期化差 |
| P7 | LUT が適用されない | Mac では適用確認 | Linux の post-process chain が途中で no-op 化、LUT texture bind 失敗、または `None` 復旧修正の分岐差 |

### 2.3 低優先度: 効果はあるが UX/レンジ再調整候補

| ID | 項目 | 理由 | 対応方針 |
|---|---|---|---|
| P8 | SSAO Factor | Mac では `0.20` 以降が飽和 | UI max または tooltip を実効レンジに合わせる |
| P9 | DoF/F 値/最大散乱円/色収差 | Mac では条件次第で効果あり | プリセット・tooltip・検証条件を明記 |
| P10 | CAS sharpness | Mac では微弱に効果あり | UI label/tooltip で効果が微弱な条件を明示、必要なら range 再調整 |

## 3. 調査方針

各項目は以下の 5 点を順に trace する。

1. **UI binding**: `floater_aya_cinematic.xml` や Phototools の widget が正しい `control_name` / callback に接続されているか
2. **settings**: `settings.xml` の型、default、min/max、overlay 値が実装の期待値と一致するか
3. **cached setting**: `connectRefreshCachedSettingsSafe()` / `refreshCachedSettings()` / `LLCachedControl` が変更を拾うか
4. **pipeline dispatch**: 対象 pass が gate 条件を満たして実行されるか
5. **shader uniform/permutation**: uniform が bind され、shader 内で実際に出力に寄与するか

## 4. 修正ステップ案

### Step A: Cloud 描画復旧

**目的**: 3D cloud depth の検証以前に、雲そのものを復旧する。

調査対象:

- sky/cloud draw pool の mode gate
- `RenderClouds` 系 cvar
- Cinematic / AYAstorm View 切替時の environment pass
- cloud shader compile log
- depth / post-process chain で cloud が上書き・未合成になっていないか

完了条件:

- Mac/Linux で雲が表示される
- 3D cloud depth の ON/OFF 差を検証できる状態になる

進捗:

- 雲描画は cloud draw pass そのものが停止していたわけではなく、`RenderEnableEmissiveBuffer=1` 時の sky/cloud emissive output alpha が 0 になっていたことが主因と判断する。
- `LLGLSPipelineBlendSkyBox` は alpha blending で sky/cloud を合成するため、`frag_data[3]` に RGB が入っていても alpha 0 だと最終合成に寄与しない。結果として「雲が描画されていない」ように見える。
- 修正では sky domain の `HAS_EMISSIVE` 経路で、視覚出力と同じ alpha を emissive attachment に渡すようにした。具体的には `cloudsF.glsl` は `alpha1`、`skyF.glsl` は `1.0`、`starsF.glsl` / `moonF.glsl` / `sunDiscF.glsl` は各色の alpha を `frag_data[3].a` に入れる。
- このため、雲復旧は 3D cloud depth や environment pass の gate 修正ではなく、emissive buffer 経路の alpha 合成修正によって説明できる。

### Step B: Post-process chain 健全性確認

**目的**: Linux の UI 白塗り、LUT 不適用、Glow OFF path を同じ根で調査する。

調査対象:

- Glow/Bloom OFF 時の render path
- LUT pass の enable gate と texture bind
- `renderFinalize()` の pass 順序
- `mRT->screen` / ping-pong RT / post map の clear と format
- Linux only の shader compile warning/error

完了条件:

- Linux で Glow/Bloom OFF でも UI 文字が白四角化しない
- Linux で `LUT None` と任意 LUT の差が見える
- Mac の LUT 正常動作を壊さない

進捗:

- `LLPipeline::renderFinalize()` は Glow/Bloom OFF 時でも `generateGlow()` の後に必ず `combineGlow()` を実行し、`glowcombineF.glsl` は `mGlow[1]` を画面色に加算する。
- `generateGlow()` の OFF 分岐では `mGlow[1].clear()` が現在の GL clear color に依存していた。Linux で clear color が白または非ゼロのまま残ると、UI 文字やパネルに白い加算ブロックが出る説明がつく。
- 対策として OFF 分岐で `mGlow[1]` を `glClearColor(0, 0, 0, 0)` + `GL_COLOR_BUFFER_BIT` で明示クリアする。Glow ON の抽出・ぼかし経路には影響させない。
- Mac では LUT 適用済み確認あり。Linux の LUT 不適用疑いは、Glow OFF白塗り対策後に実機で再確認する。

### Step C: Shadow blur controls

**目的**: 影ぼかしサイズが Mac/Linux 共通で効かない問題を修正する。

調査対象:

- `RenderShadowBlurSize`
- `RenderShadowGaussian`
- `RenderShadowBlurDistFactor`
- shadow blur shader uniform
- Cinematic mode の per-cascade shadow path

完了条件:

- 影ぼかしサイズを最小/最大にしたスクリーンショットで差が確認できる
- sun shadow と spot/projector shadow のどちらに効く UI なのかを tooltip に反映する

進捗:

- `RenderShadowBlurSize` は UI、cached setting、`gDeferredBlurLightProgram` の `kern_scale` まで届いていた。
- ただし blur pass は `RenderDeferredSSAO && RenderDeferredBlurLight` の時だけ実行されていたため、SSAO OFF では shadow light map が生成されていても影ぼかしが走らなかった。
- Cinematic 分岐の `blurLightF.glsl` は BD original に合わせて G channel (SSAO) だけをぼかし、R/B/A の directional/spot shadow channel を素通ししていた。このため Cinematic では `RenderShadowBlurSize` が影に見えない。
- 対策として、blur pass gate を `RenderDeferredSSAO || RenderShadowDetail > 0` に広げ、Cinematic 分岐でも R/G/B/A 全 channel を同じ separable blur で処理する。
- Phototools の Shadow Blur / Blur Dist tooltip から「Ambient Occlusion 必須」の記述を外した。

### Step D: SSR controls

**目的**: SSR 全項目が効果不明な原因を切り分ける。

調査対象:

- SSR master gate
- reflection probe / screen-space reflection pass の mode gate
- roughness / material 条件
- shader permutation
- UI cvar と shader uniform の対応

完了条件:

- SSR が効く検証 material / scene を定義する
- 各 control がどの visible output に効くかを確定する
- dead control があれば削除または disabled にする

進捗:

- `RenderScreenSpaceReflections` と SSR sub controls は UI、cached setting、`bindReflectionProbes()` の uniform 送信までは届いていた。
- Cinematic shader override (`cinematic_bd/class3/deferred/screenSpaceReflUtil.glsl`) は BD 由来の `vec3` uniform を期待していたが、AYA 側 settings と C++ は scalar (`S32` / `F32`) として送っていた。型不一致により Cinematic SSR の sub uniform が正しく反映されない。
- 同 shader は `maxZDepth` / `maxRoughness` で早期 return するが、C++ 側から送信されていなかった。初期値 0 の場合、`roughness >= maxRoughness` でほぼ全 material が SSR なしになる。
- 対策として Cinematic SSR shader を AYA の scalar controls に合わせ、`maxZDepth` / `maxRoughness` を `RenderScreenSpaceReflectionMaxDepth` / `RenderScreenSpaceReflectionMaxRoughness` から送信する。
- 検証 scene は roughness 低め、metallic/specular 高めの反射 material と、反射に映る高コントラスト物体を同一画面内に置く。SSR は画面外の物体を反射できないため、比較時はカメラ内に反射対象を残す。

### Step E: Volumetric Lighting controls

**目的**: 方向性フェード以外の Volumetric controls の効果不明を解消する。

調査対象:

- `RenderVolumetricLighting`
- `RenderVolumetricLightingResolution`
- `RenderVolumetricLightingMultiplier`
- `RenderVolumetricLightingFalloffMultiplier`
- `RenderVolumetricLightingDirectional`
- `GODRAYS_FADE` permutation
- shader 内ループが default 値で実質 no-op になっていないか

注意:

過去 trace では `RenderVolumetricLightingResolution=1` の場合、for loop が実質 0 回になる疑いがある。default と UI min の再検討が必要。

デフォルト調整:

- AYA 実機確認用の初期値として、Volumetric Lighting は `ON`、`Resolution=16`、`Intensity=4.0`、`Falloff=2.0` に合わせる。
- `RenderVolumetricLighting` と `RenderVolumetricLightingResolution` は既存 default が画像値と一致していた。
- `RenderVolumetricLightingMultiplier` は `50.0 -> 4.0`、`RenderVolumetricLightingFalloffMultiplier` は `1.0 -> 2.0` に変更し、settings default / Cinematic overlay / Reset D / runtime fallback を同期する。

進捗:

- Volumetric Lighting は shader/pass が死んでいたわけではない。Cinematic mode では `renderFinalize()` から `RenderVolumetricLighting=ON` かつ `!gCubeSnapshot` の時に `renderVolumetric()` が実行され、`gVolumetricLightProgram` に `godray_res` / `godray_multiplier` / `falloff_multiplier` が送られる。
- `RenderVolumetricLightingResolution=16` は `volumetricLightF.glsl` の `for (int i=godray_res-1; i>0; --i)` により十分なサンプル数を持つ。以前懸念していた `Resolution<=1` の実質 no-op には該当しない。
- 実機所見では、朝方・夕方の低い太陽位置では `Intensity` / `Falloff` などのパラメータを動かしても描画上の変化がかなり見えにくい。一方、正午前後の太陽位置で、カメラ内に太陽を入れる構図では効果が確認できる。
- したがって現時点の問題は「Volumetric chain が直った/直っていない」というより、太陽高度・太陽の画面内位置・遮蔽物・影コントラストに強く依存して、特定条件では control の変化が視認しづらいことにある。
- 画像値の `Intensity=4.0` / `Falloff=2.0` は、初期状態で過度に飽和させずに検証するための基準値として採用した。ただし朝方・夕方の太陽位置でも変化が効くようにするには、低太陽高度時の `shaftify` / fade / falloff の扱いを追加調整する必要があるかもしれない。

完了条件:

- 各 slider の min/max で差が出る検証 scene を定義する
- 効果がない値域は UI range から外す、または tooltip に明記する

### Step F: Fullbright texture / Glow warmth / CAS / SSAO UX

**目的**: 共通不具合修正後に、効果が微弱または条件依存の controls を UX として整理する。

調査対象:

- Fullbright texture の対象 material 条件
- Glow warmth amount / weights が glow extract/combine に入っているか
- CAS sharpness の実効レンジ
- SSAO Factor の飽和点

Fullbright texture trace:

- `RenderEnableFullbright` は `LLPrimitive` の TE pack/unpack では参照されていたが、既に scene に存在する face の `LLFace::FULLBRIGHT` state や render pass 分類では `LLTextureEntry::getFullbright()` が直接参照されていた。
- そのため Cinematic floater から OFF にしても、表示中 object は fullbright pass / fullbright vertex format / fullbright shiny pass に残り、ユーザー視点では「効果がわからない」状態になりやすかった。
- 修正方針は、legacy fullbright TE の描画判定だけを `RenderEnableFullbright` で gate し、HUD attachment や light material 由来の fullbright fallback は維持する。
- `RenderEnableFullbright` 変更時は表示中の `LLVOVolume` を `updateFaceFlags()` + `markForUpdate()` で再分類し、再ログインや object reload なしで反映する。

Fullbright texture 検証条件:

- Fullbright texture を持つ legacy/blinn-phong object を用意する。PBR/GLTF emissive は別系統なのでこの toggle の主対象外。
- `RenderEnableFullbright=TRUE` で照明影響を受けにくい見た目、`FALSE` で通常 shaded surface として光源・影・環境光の影響を受けることを確認する。
- alpha/masked fullbright object と shiny fullbright object でも、OFF 時に fullbright alpha mask / fullbright shiny pass に残らないことを確認する。

完了条件:

- dead control は削除または disabled
- 条件付き control は tooltip に条件を書く
- 実効レンジと UI range を合わせる

## 5. 検証マトリクス

| 項目 | Mac | Linux | Windows | 必須 scene/material |
|---|---|---|---|---|
| Cloud 描画 | 必須 | 必須 | 推奨 | 雲あり environment |
| 3D cloud depth | 必須 | 必須 | 推奨 | 雲 + 遠近差のある地形 |
| Glow/Bloom OFF UI | 必須 | 必須 | 推奨 | UI overlay + text heavy panel |
| LUT | 必須 | 必須 | 推奨 | 強い LUT と `None` の比較 |
| Shadow blur | 必須 | 必須 | 推奨 | 斜光 + 地面 + hard shadow |
| SSR | 必須 | 必須 | 推奨 | roughness 低めの reflective material |
| Volumetric Lighting | 必須 | 必須 | 推奨 | 太陽角度低め + shadow caster |
| Fullbright texture | 必須 | 必須 | 推奨 | fullbright textured object |
| SSAO | 必須 | 必須 | 推奨 | contact shadow が見える室内/地面 |
| CAS | 必須 | 必須 | 推奨 | high frequency texture / 4K 表示 |

## 6. 実装ブランチ分割案

| Branch | 内容 | 理由 |
|---|---|---|
| `fix/ayastorm-cloud-postprocess-chain` | Cloud 復旧 + Linux post-process/LUT/Glow OFF | 環境描画と post chain は相互影響が大きいため同一ブランチで扱う |
| `fix/ayastorm-shadow-blur-controls` | shadow blur cvar trace/fix | 共通不具合だが独立しやすい |
| `fix/ayastorm-ssr-controls` | SSR controls trace/fix | material/scene 条件が特殊なので分離 |
| `fix/ayastorm-volumetric-controls` | Volumetric controls trace/fix | shader permutation と default/range 調整が絡むため分離 |
| `fix/ayastorm-control-ux-ranges` | SSAO/CAS/DoF/tooltip/range cleanup | 不具合修正後の UX 調整として分離 |

## 7. 優先順位

1. **Cloud 描画復旧**: 共通不具合かつ 3D cloud depth 検証の前提
2. **Linux post-process chain**: UI 白塗りと LUT 不適用の根が同じ可能性が高い
3. **Shadow blur**: Mac/Linux 共通で「効かない」と確認済み
4. **Volumetric Lighting**: 方向性フェード以外の no-op 疑い。default 値が弱すぎるだけの可能性もある
5. **SSR**: scene/material 条件依存が強いので、専用 scene を作ってから判定
6. **UX/range 調整**: SSAO/CAS/DoF など、効果確認済みだが見え方が弱い項目を整理

## 8. 次アクション

1. 本 document を review し、優先順位に同意する
2. `fix/ayastorm-cloud-postprocess-chain` を本ブランチまたは `fix/macos-glsl-guards` から切る
3. Cloud と post-process chain の pass trace を開始する
4. Linux 実機ログを採取する: shader compile log、OpenGL vendor/version、Glow OFF 時のスクリーンショット、LUT 切替動画
5. Mac で同じ scene/preset を使い、差分のない baseline を保存する

## 9. 進捗ログ

### 2026-05-23: Cloud/sky emissive-buffer alpha 修正

**Branch**: `fix/ayastorm-cloud-postprocess-chain`

Cloud 描画不良の第一原因候補として、`RenderEnableEmissiveBuffer=1` 時の sky-domain shader 出力を修正した。

#### 原因

`RenderEnableEmissiveBuffer` は default ON で、deferred sky/cloud は `HAS_EMISSIVE` permutation で `frag_data[3]` (= gbuffer emissive buffer) に sky/cloud color を書く。このとき skybox draw state は alpha blend (`BT_ALPHA`) なので、MRT 各 attachment の RGB 書き込みはその出力 alpha に依存する。

r30 側の SSS mask 対応で sky/cloud/moon/stars/sun が `frag_data[3].a = 0.0` を出すようになっていたため、gbuffer3 RGB も source alpha 0 で blend され、結果として sky-domain emissive color が書き込まれない。Cloud が描画されない報告はこの経路で説明できる。

SSS の sky 誤発火対策は既に `skinSSSF.glsl` 側で far-plane depth (`d_raw >= 0.9999`) により sky-domain pixels を early return しているため、sky-domain shader 側で visual alpha を 0 に潰す必要はない。

#### 修正

`HAS_EMISSIVE` path でも visual alpha を保持するように変更。

| Shader | 変更 |
|---|---|
| `cloudsF.glsl` | `frag_data[3].a = alpha1` |
| `skyF.glsl` | `frag_data[3].a = 1.0` |
| `starsF.glsl` | `frag_data[3].a = col.a` |
| `moonF.glsl` | `frag_data[3].a = c.a` |
| `sunDiscF.glsl` | `frag_data[3].a = c.a` |

#### 検証

Mac Release 差分ビルド成功:

```sh
/usr/bin/env DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer \
  xcodebuild -quiet \
  -project build-darwin-universal/SecondLife.xcodeproj \
  -scheme viewer \
  -configuration Release \
  -destination 'platform=macOS,arch=arm64' \
  -derivedDataPath build-darwin-universal/DerivedData \
  build
```

App bundle:

`build-darwin-universal/newview/Release/AYAstorm.app`

#### 残検証

- Mac 実機で雲、太陽、月、星が描画されること — **PASS (2026-05-23)**
- `AYAR20AvatarSkinSSSEnabled=ON` でも sky/cloud が SSS blur の対象にならないこと — **PASS (2026-05-23)**
- Glow/Bloom OFF UI 白塗りが出ないこと — **PASS (2026-05-23, Mac)**
- LUT が適用されること — **PASS (2026-05-23, Mac)**
- Linux 実機で Cloud 描画と Glow/Bloom OFF UI 白塗りの再確認 — **未実施**
