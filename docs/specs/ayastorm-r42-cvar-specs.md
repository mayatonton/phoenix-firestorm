# AYAstorm r42 — cvar 仕様書(全 2501 個 / GUI 種別・live-dead・影響 shader)

| | |
|---|---|
| 対象 | `settings.xml` の全 cvar **2501 個** |
| 基準 | ⚠️ **作業ツリー**(`feature/ayastorm-r42-gl-removal` HEAD `d657e586c2` + **並走者の未コミット GL 削除を含む**)。HEAD との差分 = §0.4 |
| 性格 | **機械抽出 + コードトレース**。推論・伝聞は根拠にしない |

---

# 🔴 最重要:GUI にあり死んでいる cvar — **10 件**

**画面から操作できるのに、値を読む者が誰もいない = 何も起きない no-op スイッチ。**
全 10 件を実コードで個別精査済(①cvar 名の直接参照 ②動的名連結 ③bind された widget 名での読み、いずれも読み手なし)。

**うち 9 件は r30(`b2decf0578`・2026-05-23)時点で既に死んでいた** = AYAstorm の作業とは無関係の **upstream 由来の no-op**。r41/r42 が壊したものではない。
**例外は `RenderDebugPipeline` のみ** — r30 でも現 HEAD でも LIVE で、**並走者の未コミット GL 削除が今まさに殺している最中**(→ §0.4)。

## 環境設定(Preferences)内 — 4 件

| cvar | 画面 | タブ | 内側タブ | ウィジェット | Type | r30 でも死んでいたか |
|---|---|---|---|---|---|---|
| `FSCloseChatOnReturnOnlyForNearbyChatControl` | 環境設定 (Preferences) | **Chat** | **Typing** | check_box「Only for nearby chat bar」 | Boolean | ✅ 死んでいた |
| `FSOpenInventoryAfterSnapshot` | 環境設定 (Preferences) | **Privacy** | **General** | check_box「Automatically show snapshots in inventory after upload」 | Boolean | ✅ 死んでいた |
| `MediaEnablePopups` | 環境設定 (Preferences) | **Network & Files** | **Connection** | check_box「Enable media browser pop-ups」 | Boolean | ✅ 死んでいた |
| `OverflowToastHeight` | 環境設定 (Preferences) | **User Interface** | **Toasts** | slider「Height of Overflow Toast:」 | S32 | ✅ 死んでいた |

## メニュー内 — 6 件

| cvar | 画面 | メニュー階層 | 項目 | Type | r30 でも死んでいたか |
|---|---|---|---|---|---|
| `NearbyListShowMap` | People フローター | **Nearby > 歯車 View メニュー(`nearby_view_btn`)** | menu_item_check「View Map」 | Boolean | ✅ 死んでいた |
| `FixedWeather` | メインメニュー | **Developer > World** | menu_item_check「Fixed Weather」 | Boolean | ✅ 死んでいた |
| `SkyOverrideSimSunPosition` | メインメニュー | **Developer > World** | menu_item_check「Sim Sun Override」 | Boolean | ✅ 死んでいた |
| `RenderDebugPipeline` | メインメニュー | **Developer > Rendering** | menu_item_check「Debug Pipeline」 | Boolean | 🔴 **生きていた**(r30 も HEAD も LIVE・並走者が削除中) |
| `ShowTangentBasis` | メインメニュー | **Developer > Rendering** | menu_item_check「Tangent Basis」 | Boolean | ✅ 死んでいた |
| `SaveMinidump` | メインメニュー | **Developer** | menu_item_check「Output Debug Minidump」 | Boolean | ✅ 死んでいた |

### 判断のヒント

- **9 件は r30 時点で既に no-op**(`git grep` を `b2decf0578` に対して実行し C++ 参照ゼロを確認)。= **upstream(Firestorm/LL)から引き継いだ元々の欠落**であり、r41/r42 の回帰ではない。撤去しても機能は失われない(既に失われている)。
- **`RenderDebugPipeline` だけは別物**。r30 でも HEAD でも LIVE で、`llappviewer.cpp:665` → `gDebugPipeline` → `pipeline.cpp` の `if (gDebugGL || gDebugPipeline)` で **GL の strict エラーチェック**を駆動していた。VK 等価は存在しないので r42 での撤去自体は筋が通るが、**メニュー項目 Developer > Rendering > Debug Pipeline が孤児化する**ため XUI 側の削除も要る。
- `ShowTangentBasis` は GL 時代の描画デバッグだが **r30 の時点で既に読み手なし**(GL 削除とは無関係の古い残骸)。
- 残り 8 件は描画と無関係(chat / snapshot / media / toast / radar)。ユーザーには効いているように見える **機能欠落**。

### 参考: DEAD だが「画面には存在しない」もの(XUI でコメントアウト済)— 2 件

| cvar | 所在 | 状況 |
|---|---|---|
| `GridCrossSections` | floater_build_options.xml「Grid Options」 | check_box「View cross-sections」が `<!-- -->` でコメントアウト済 = 画面に存在しない |
| `RegInClient` | menu_login.xml | menu 項目が `<!-- -->` でコメントアウト済 = 画面に存在しない |

### 参考: 精査で DEAD 認定を取り消した 5 件(widget 経由で生きていた)

| cvar | 生存経路(実コード) |
|---|---|
| `360QualitySelection` | radio_group `360_quality_selection` → `llfloater360capture.cpp:119` → `:191` `getSelectedValue()` |
| `AuctionShowFence` | check_box `fence_check` → `llfloaterauction.cpp:182` → `gForceRenderLandFence` → `pipeline.cpp:4806` |
| `LSLFindCaseInsensitivity` | check_box `case_text` → `llfloatersearchreplace.cpp:52` → `:202/:211/:220` `->get()` |
| `LSLFindDirection` | check_box `find_previous` → `llfloatersearchreplace.cpp:54` → `:202/:211` `->get()` |
| `SnapshotToProfileIncludeLocation` | check_box `add_location_cb` → `llpanelsnapshotprofile.cpp:105` `getValue()` |

---

## 0. 判定基準

### 0.1 cvar が「生きている」経路は 6 つある

単純な grep で 1 つでも見落とすと dead 誤認になる。**この資料は初版で 5 種類の見落としをやらかしている**(§0.3)。

| 状態 | 定義 | 代表例 |
|---|---|---|
| **LIVE** | C++ に `"cvar名"` のクォート文字列として出現 | `gSavedSettings.getBOOL("X")` |
| **LIVE?(動的名)** | 名前が実行時に連結される読み口に一致 | `llui.cpp:154` `getBOOL("PlayMode" + name)` → `PlayModeUISnd*` 46 個 |
| **LIVE(widget経由)** | `control_name` で widget に bind され、**C++ は cvar 名でなく widget 名で読む** | `AuctionShowFence` → `fence_check` → `llfloaterauction.cpp:182` |
| **LIVE(UI framework)** | `enabled_control` / `visibility_control` / `setting` / `sort_order_setting` / `sound` に bind = **framework が値を読んで挙動を変える** | `llstatbar.cpp:218` `getS32(mSetting)` → `DebugStatMode*` 83 個 |
| **LIVE(XML専用)** | `cmd_line.xml` / preset XML が参照 | |
| **DEAD** | 上のどれにも該当しない = **誰も読まない** | |

### 0.2 種別(GUI かどうか)

XUI の**全属性を総当たり**して値が cvar 名になる属性を機械的に確定(勘で列挙しない)。**`<!-- -->` コメント内は除外**(画面に存在しないため)。

| 属性 | 件数 | 機能的な読みか |
|---|---:|---|
| `control_name` | 1474 | ❌ 値を書く/映すだけ(誰かが読まねば no-op) |
| `parameter`(menu) | 499 | ❌ 同上 |
| `enabled_control` / `disabled_control` | 442 | ✅ framework が widget の有効/無効を決める |
| `visibility_control` / `invisibility_control` | 96 | ✅ framework が表示/非表示を決める |
| `setting`(LLStatBar/LLStatView) | 89 | ✅ `llstatbar.cpp:218` が `getS32()` |
| `control`(menu on_check) | 68 | ❌ チェック状態を映すだけ |
| `sort_order_setting` | 14 | ✅ `llinventorypanel.cpp:293-295` |
| `sound`(notifications) | 14 | ✅ `llnotifications.cpp:982` `make_ui_sound()` |

### 0.3 ⚠️ 初版で犯した dead 誤認(手法の教訓)

| 誤認 | 原因 | 修正 |
|---|---|---|
| `PlayModeUISnd*` 46 個 | `llui.cpp:154` が `getBOOL("PlayMode" + name)` と**動的に名前を組む** | 任意の `get*/set*` + 変数代入の連結まで走査 |
| `360Capture*` 6 個 | 正規表現が `"[A-Za-z_]…"` 起点で**数字始まりの識別子を弾いていた** | `"[A-Za-z0-9_]+"` に修正 |
| widget 経由 5 個 | **C++ が cvar 名でなく widget 名で読む**経路を見ていなかった | widget 名を抽出して読み手を追跡 |
| `DebugStatMode*` 等 78 個 | `setting=` / `sort_order_setting=` / `sound=` を GUI 属性リストから漏らしていた | XUI 全属性を総当たり |
| `GridCrossSections` / `RegInClient` を「GUI にあり DEAD」と誤認 | **XML コメント内の bind を拾っていた**(画面には存在しない) | `<!-- -->` を除去して再判定 |

### 0.4 ⚠️ この資料の基準は「HEAD」ではなく「作業ツリー」

生成時、**並走者が r42 の GL 削除を未コミットで進行中**だった。走査は作業ツリーを読んでいるため、判定は「並走者の GL 削除が適用された後」を先取りしている。

HEAD(`d657e586c2`)と作業ツリーを突き合わせた結果、**未コミット編集で C++ 参照が消えた cvar が 3 件**ある。

| cvar | HEAD(commit) | 作業ツリー(この資料の基準) | 影響 |
|---|---|---|---|
| `RenderDebugPipeline` | **LIVE**(`llappviewer.cpp:665` → `gDebugPipeline` → `pipeline.cpp` `if (gDebugGL \|\| gDebugPipeline)`) | 削除中 | 🔴 **メニュー項目が孤児化**(Developer > Rendering > Debug Pipeline)= XUI 側の削除が要る |
| `GridCrossSections` | **LIVE** | 削除中 | XUI は元々コメントアウト済 = 孤児化しない |
| `RenderGLContextCoreProfile` | **LIVE** | 削除中 | 非GUI = 孤児化しない |

※ 冒頭 10 件のうち 9 件は HEAD でも作業ツリーでも DEAD なので、結論は変わらない。

### 0.5 r30 との比較(いつから死んでいたか)

`git grep` を r30(`b2decf0578`・2026-05-23)に対して直接実行して確認した。

- **冒頭 10 件のうち 9 件は r30 時点で既に C++ 参照ゼロ** = upstream 由来の元々の no-op。r41/r42 の回帰ではない。
- **`RenderDebugPipeline` のみ r30 で LIVE**(参照 2 箇所)。現 HEAD でも LIVE。死ぬのは並走者の GL 削除が commit された時点。

---

## 1. 集計

| | LIVE | LIVE?(動的名) | LIVE(widget経由) | LIVE(UI framework) | LIVE(XML専用) | DEAD | 計 |
|---|---:|---:|---:|---:|---:|---:|---:|
| **GUI** | 1009 | 39 | 5 | 90 | 4 | 4 | 1151 |
| **GUI(menu)** | 68 | 0 | 0 | 0 | 0 | 6 | 74 |
| **非GUI** | 1029 | 60 | 0 | 0 | 5 | 182 | 1276 |
| **計** | **2106** | **99** | **5** | **90** | **9** | **192** | **2501** |

- **shader に影響する cvar = 97 個**(GUI 露出 77 個)= §A

---

## §A. shader に影響する cvar → 影響 shader 一覧

cvar が shader に届く経路は **3 チャネルしか無い**(全部コードで追った)。

| チャネル | 実体 |
|---|---|
| **① UBO** | cvar → `LLVKLoader::<Block>_PerProgramBind` のフィールド → `writeCurrentXxxUBO()` か `shader->mVkPerProgramUBOMapped` への memcpy |
| **② define** | `addPermutation()` / `sGlobalDefines`。define は全 shader に注入されるが、効くのは **そのマクロを GLSL で参照している shader だけ** |
| **③ 差替** | `mShaderFiles` に積む .glsl 自体が変わる |

**合成ソースの再現**: GLSL に `#include` は無い。共通 util は `LLShaderMgr::attachShaderFeatures()`(`llshadermgr.cpp:75-371`)が `mFeatures` を見て貼り付ける。よって各 shader の実ソースは `mShaderFiles` + `mFeatures` から再現(rigged variant の継承も解決)。**shader オブジェクト 160 個**。

### `AYAR14Strength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `WindlightAtmos_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwlsky.cpp:239
- **影響 shader (73)**:
  - `gAYAAlphaPlateCompositeProgram` — AYAstorm Alpha Plate Composite
  - `gAYAForwardFlipCompositeProgram` — AYAstorm Forward Flip Composite
  - `gAvatarEyeballProgram` — Avatar Eyeball Program
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredEmissiveProgram` — Deferred Emissive Shader
  - `gDeferredFullbrightAlphaMaskAlphaProgram` — Deferred Fullbright Alpha Masking Alpha Shader
  - `gDeferredFullbrightAlphaMaskProgram` — Deferred Fullbright Alpha Masking Shader
  - `gDeferredFullbrightProgram` — Deferred Fullbright Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredPostGammaCorrectProgram` — Deferred Gamma Correction Post Process
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedEmissiveProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskAlphaProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskProgram`
  - `gDeferredSkinnedFullbrightProgram`
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)
  - `gFXAAProgram` — FXAA Shader ()
  - `gGaussianProgram` — Reflection Mip Shader
  - `gHUDFullbrightAlphaMaskAlphaProgram` — HUD Fullbright Alpha Masking Alpha Shader
  - `gHUDFullbrightAlphaMaskProgram` — HUD Fullbright Alpha Masking Shader
  - `gHUDFullbrightProgram` — HUD Fullbright Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gLegacyPostGammaCorrectProgram` — Legacy Gamma Correction Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gObjectAlphaMaskNoColorProgram` — No color alpha mask Shader
  - `gReflectionMipProgram` — Reflection Mip Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gRlvSphereProgram` — RLVa Sphere Post Processing Shader
  - `gSMAABlendWeightsProgram` — SMAA Blending Weights ()
  - `gSMAAEdgeDetectProgram` — SMAA Edge Detection ()
  - `gSMAANeighborhoodBlendProgram` — SMAA Neighborhood Blending ()
  - `gSMAAResolveProgram` — SMAA T2x Resolve ()
  - `gUnderWaterProgram` — Underwater Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `AYAR14VolumetricAtmosphereInCinematicEnabled`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ① UBO `WindlightAtmos_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwlsky.cpp:232
- **影響 shader (73)**:
  - `gAYAAlphaPlateCompositeProgram` — AYAstorm Alpha Plate Composite
  - `gAYAForwardFlipCompositeProgram` — AYAstorm Forward Flip Composite
  - `gAvatarEyeballProgram` — Avatar Eyeball Program
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredEmissiveProgram` — Deferred Emissive Shader
  - `gDeferredFullbrightAlphaMaskAlphaProgram` — Deferred Fullbright Alpha Masking Alpha Shader
  - `gDeferredFullbrightAlphaMaskProgram` — Deferred Fullbright Alpha Masking Shader
  - `gDeferredFullbrightProgram` — Deferred Fullbright Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredPostGammaCorrectProgram` — Deferred Gamma Correction Post Process
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedEmissiveProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskAlphaProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskProgram`
  - `gDeferredSkinnedFullbrightProgram`
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)
  - `gFXAAProgram` — FXAA Shader ()
  - `gGaussianProgram` — Reflection Mip Shader
  - `gHUDFullbrightAlphaMaskAlphaProgram` — HUD Fullbright Alpha Masking Alpha Shader
  - `gHUDFullbrightAlphaMaskProgram` — HUD Fullbright Alpha Masking Shader
  - `gHUDFullbrightProgram` — HUD Fullbright Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gLegacyPostGammaCorrectProgram` — Legacy Gamma Correction Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gObjectAlphaMaskNoColorProgram` — No color alpha mask Shader
  - `gReflectionMipProgram` — Reflection Mip Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gRlvSphereProgram` — RLVa Sphere Post Processing Shader
  - `gSMAABlendWeightsProgram` — SMAA Blending Weights ()
  - `gSMAAEdgeDetectProgram` — SMAA Edge Detection ()
  - `gSMAANeighborhoodBlendProgram` — SMAA Neighborhood Blending ()
  - `gSMAAResolveProgram` — SMAA T2x Resolve ()
  - `gUnderWaterProgram` — Underwater Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `AYAR15GodraysPhaseExponent`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `GodraysF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12316
- **影響 shader (1)**:
  - `gDeferredGodraysProgram` — Godrays Shader

### `AYAR15GodraysStrength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `GodraysF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12317
- **影響 shader (1)**:
  - `gDeferredGodraysProgram` — Godrays Shader

### `AYAR16AerialPerspectiveEnabled`

- 種別 **非GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ① UBO `WindlightAtmos_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwlsky.cpp:256
- **影響 shader (73)**:
  - `gAYAAlphaPlateCompositeProgram` — AYAstorm Alpha Plate Composite
  - `gAYAForwardFlipCompositeProgram` — AYAstorm Forward Flip Composite
  - `gAvatarEyeballProgram` — Avatar Eyeball Program
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredEmissiveProgram` — Deferred Emissive Shader
  - `gDeferredFullbrightAlphaMaskAlphaProgram` — Deferred Fullbright Alpha Masking Alpha Shader
  - `gDeferredFullbrightAlphaMaskProgram` — Deferred Fullbright Alpha Masking Shader
  - `gDeferredFullbrightProgram` — Deferred Fullbright Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredPostGammaCorrectProgram` — Deferred Gamma Correction Post Process
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedEmissiveProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskAlphaProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskProgram`
  - `gDeferredSkinnedFullbrightProgram`
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)
  - `gFXAAProgram` — FXAA Shader ()
  - `gGaussianProgram` — Reflection Mip Shader
  - `gHUDFullbrightAlphaMaskAlphaProgram` — HUD Fullbright Alpha Masking Alpha Shader
  - `gHUDFullbrightAlphaMaskProgram` — HUD Fullbright Alpha Masking Shader
  - `gHUDFullbrightProgram` — HUD Fullbright Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gLegacyPostGammaCorrectProgram` — Legacy Gamma Correction Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gObjectAlphaMaskNoColorProgram` — No color alpha mask Shader
  - `gReflectionMipProgram` — Reflection Mip Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gRlvSphereProgram` — RLVa Sphere Post Processing Shader
  - `gSMAABlendWeightsProgram` — SMAA Blending Weights ()
  - `gSMAAEdgeDetectProgram` — SMAA Edge Detection ()
  - `gSMAANeighborhoodBlendProgram` — SMAA Neighborhood Blending ()
  - `gSMAAResolveProgram` — SMAA T2x Resolve ()
  - `gUnderWaterProgram` — Underwater Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `AYAR16AerialPerspectiveStrength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `WindlightAtmos_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwlsky.cpp:246
- **影響 shader (73)**:
  - `gAYAAlphaPlateCompositeProgram` — AYAstorm Alpha Plate Composite
  - `gAYAForwardFlipCompositeProgram` — AYAstorm Forward Flip Composite
  - `gAvatarEyeballProgram` — Avatar Eyeball Program
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredEmissiveProgram` — Deferred Emissive Shader
  - `gDeferredFullbrightAlphaMaskAlphaProgram` — Deferred Fullbright Alpha Masking Alpha Shader
  - `gDeferredFullbrightAlphaMaskProgram` — Deferred Fullbright Alpha Masking Shader
  - `gDeferredFullbrightProgram` — Deferred Fullbright Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredPostGammaCorrectProgram` — Deferred Gamma Correction Post Process
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedEmissiveProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskAlphaProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskProgram`
  - `gDeferredSkinnedFullbrightProgram`
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)
  - `gFXAAProgram` — FXAA Shader ()
  - `gGaussianProgram` — Reflection Mip Shader
  - `gHUDFullbrightAlphaMaskAlphaProgram` — HUD Fullbright Alpha Masking Alpha Shader
  - `gHUDFullbrightAlphaMaskProgram` — HUD Fullbright Alpha Masking Shader
  - `gHUDFullbrightProgram` — HUD Fullbright Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gLegacyPostGammaCorrectProgram` — Legacy Gamma Correction Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gObjectAlphaMaskNoColorProgram` — No color alpha mask Shader
  - `gReflectionMipProgram` — Reflection Mip Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gRlvSphereProgram` — RLVa Sphere Post Processing Shader
  - `gSMAABlendWeightsProgram` — SMAA Blending Weights ()
  - `gSMAAEdgeDetectProgram` — SMAA Edge Detection ()
  - `gSMAANeighborhoodBlendProgram` — SMAA Neighborhood Blending ()
  - `gSMAAResolveProgram` — SMAA T2x Resolve ()
  - `gUnderWaterProgram` — Underwater Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `AYAR18CloudVolumetricEnabled`

- 種別 **非GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ① UBO `Cloud_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwlsky.cpp:540
- **影響 shader (1)**:
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program

### `AYAR18CloudVolumetricStrength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `Cloud_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwlsky.cpp:541
- **影響 shader (1)**:
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program

### `AYAR20AvatarSkinSSSBlurRadius`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `SkinSSSF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12428 ; indra/newview/pipeline.cpp:12488
- **影響 shader (1)**:
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader

### `AYAR20AvatarSkinSSSGlowColor`

- 種別 **GUI** / 状態 **LIVE** / Type `Color4`
- チャネル: ① UBO `SkinSSSF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12421 ; indra/newview/pipeline.cpp:12422
- **影響 shader (1)**:
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader

### `AYAR20AvatarSkinSSSGlowGain`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `SkinSSSF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12424 ; indra/newview/pipeline.cpp:12484
- **影響 shader (1)**:
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader

### `AYAR20AvatarSkinSSSStrength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `SkinSSSF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12487
- **影響 shader (1)**:
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader

### `AYAVisualRealismEnabled`

- 種別 **GUI** / 状態 **LIVE** / Type `U32`
- チャネル: ① UBO `WindlightAtmos_PerProgramBind` + ① UBO `Cloud_PerProgramBind` + ① UBO `GodraysF_PerProgramBind` + ③ View mode(0/1/2)。Cinematic で post shader の file/permutation 切替
- 根拠: indra/newview/lldrawpoolwlsky.cpp:224 ; indra/newview/lldrawpoolwlsky.cpp:232 ; indra/newview/lldrawpoolwlsky.cpp:540 ; indra/newview/pipeline.cpp:12315 ; llviewershadermgr.cpp:856, :3703-3722
- **影響 shader (73)**:
  - `gAYAAlphaPlateCompositeProgram` — AYAstorm Alpha Plate Composite
  - `gAYAForwardFlipCompositeProgram` — AYAstorm Forward Flip Composite
  - `gAvatarEyeballProgram` — Avatar Eyeball Program
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredEmissiveProgram` — Deferred Emissive Shader
  - `gDeferredFullbrightAlphaMaskAlphaProgram` — Deferred Fullbright Alpha Masking Alpha Shader
  - `gDeferredFullbrightAlphaMaskProgram` — Deferred Fullbright Alpha Masking Shader
  - `gDeferredFullbrightProgram` — Deferred Fullbright Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredPostGammaCorrectProgram` — Deferred Gamma Correction Post Process
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedEmissiveProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskAlphaProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskProgram`
  - `gDeferredSkinnedFullbrightProgram`
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)
  - `gFXAAProgram` — FXAA Shader ()
  - `gGaussianProgram` — Reflection Mip Shader
  - `gHUDFullbrightAlphaMaskAlphaProgram` — HUD Fullbright Alpha Masking Alpha Shader
  - `gHUDFullbrightAlphaMaskProgram` — HUD Fullbright Alpha Masking Shader
  - `gHUDFullbrightProgram` — HUD Fullbright Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gLegacyPostGammaCorrectProgram` — Legacy Gamma Correction Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gObjectAlphaMaskNoColorProgram` — No color alpha mask Shader
  - `gReflectionMipProgram` — Reflection Mip Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gRlvSphereProgram` — RLVa Sphere Post Processing Shader
  - `gSMAABlendWeightsProgram` — SMAA Blending Weights ()
  - `gSMAAEdgeDetectProgram` — SMAA Edge Detection ()
  - `gSMAANeighborhoodBlendProgram` — SMAA Neighborhood Blending ()
  - `gSMAAResolveProgram` — SMAA T2x Resolve ()
  - `gUnderWaterProgram` — Underwater Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `CameraDoFResScale`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `DofCombineF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:10285 ; indra/newview/pipeline.cpp:10286
- **影響 shader (1)**:
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader

### `CameraFNumber`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `CofF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:10196
- **影響 shader (1)**:
  - `gDeferredCoFProgram` — Deferred CoF Shader

### `CameraFieldOfView`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `CofF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:10196 ; indra/newview/pipeline.cpp:10198
- **影響 shader (1)**:
  - `gDeferredCoFProgram` — Deferred CoF Shader

### `CameraFocalLength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `CofF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:10196 ; indra/newview/pipeline.cpp:10198
- **影響 shader (1)**:
  - `gDeferredCoFProgram` — Deferred CoF Shader

### `CameraMaxCoF`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `CofF_PerProgramBind` + ① UBO `PostF_PerProgramBind` + ① UBO `DofCombineF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:10199 ; indra/newview/pipeline.cpp:10246 ; indra/newview/pipeline.cpp:10284
- **影響 shader (3)**:
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredPostProgram` — Deferred Post Shader

### `FSRenderVignette`

- 種別 **GUI** / 状態 **LIVE** / Type `Vector3`
- チャネル: ① UBO `PostVignette_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9806 ; indra/newview/pipeline.cpp:9807
- **影響 shader (1)**:
  - `gPostVignetteProgram` — Vignette Post

### `FSSnapshotFrameBorderColor`

- 種別 **非GUI** / 状態 **LIVE** / Type `Color3`
- チャネル: ① UBO `PostSnapshotFrame_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9984 ; indra/newview/pipeline.cpp:9985
- **影響 shader (1)**:
  - `gPostSnapshotFrameProgram` — Snapshot Frame Post

### `FSSnapshotFrameBorderWidth`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `PostSnapshotFrame_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9987
- **影響 shader (1)**:
  - `gPostSnapshotFrameProgram` — Snapshot Frame Post

### `PathfindingAmbiance`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `Pathfinding_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:6037 ; indra/newview/pipeline.cpp:6078
- **影響 shader (2)**:
  - `gPathfindingNoNormalsProgram` — PathfindingNoNormals Shader
  - `gPathfindingProgram` — Pathfinding Shader

### `PathfindingXRayOpacity`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `Pathfinding_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:6271 ; indra/newview/pipeline.cpp:6283
- **影響 shader (2)**:
  - `gPathfindingNoNormalsProgram` — PathfindingNoNormals Shader
  - `gPathfindingProgram` — Pathfinding Shader

### `PathfindingXRayTint`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `Pathfinding_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:6269 ; indra/newview/pipeline.cpp:6281
- **影響 shader (2)**:
  - `gPathfindingNoNormalsProgram` — PathfindingNoNormals Shader
  - `gPathfindingProgram` — Pathfinding Shader

### `PathfindingXRayWireframe`

- 種別 **非GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ① UBO `Pathfinding_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:6282
- **影響 shader (2)**:
  - `gPathfindingNoNormalsProgram` — PathfindingNoNormals Shader
  - `gPathfindingProgram` — Pathfinding Shader

### `RenderAvatarCloth`

- 種別 **GUI(menu)** / 状態 **LIVE** / Type `Boolean`
- チャネル: ② define `AVATAR_CLOTH`
- 根拠: llviewershadermgr.cpp:917, :3110
- **影響 shader (2)**:
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarProgram` — Deferred Avatar Shader

### `RenderBufferVisualization`

- 種別 **非GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ① UBO `PostVisualizeBuffers_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:8743
- **影響 shader (1)**:
  - `gDeferredBufferVisualProgram` — Deferred Buffer Visualization Shader

### `RenderChromaStrength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `PostNoDoFF_PerProgramBind` + ① UBO `PostF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:10584 ; indra/newview/pipeline.cpp:9709 ; indra/newview/pipeline.cpp:10247
- **影響 shader (3)**:
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader

### `RenderColorBrightness`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `PostTonemap_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9030
- **影響 shader (6)**:
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapLegacyGammaCorrectProgram` — Deferred Tonemap Legacy Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process

### `RenderColorContrast`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `PostTonemap_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9028
- **影響 shader (6)**:
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapLegacyGammaCorrectProgram` — Deferred Tonemap Legacy Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process

### `RenderColorGradingLUTIntensity`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `PostTonemap_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9031
- **影響 shader (6)**:
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapLegacyGammaCorrectProgram` — Deferred Tonemap Legacy Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process

### `RenderColorSaturation`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `PostTonemap_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9027
- **影響 shader (6)**:
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapLegacyGammaCorrectProgram` — Deferred Tonemap Legacy Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process

### `RenderColorTemperature`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `PostTonemap_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9029
- **影響 shader (6)**:
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapLegacyGammaCorrectProgram` — Deferred Tonemap Legacy Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process

### `RenderDebugNormalScale`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `NormalDebug_PerProgramBind`
- 根拠: indra/newview/llspatialpartition.cpp:2166
- **影響 shader (2)**:
  - `gNormalDebugProgram` — Normal Debug Shader
  - `gSkinnedNormalDebugProgram`

### `RenderDeferredSSAO`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ③ `sunLightSSAOF.glsl` ⇄ `sunLightF.glsl` 差替 + `aoUtil.glsl` attach
- 根拠: llviewershadermgr.cpp:2215-2232 / llshadermgr.cpp:264-270
- **影響 shader (1)**:
  - `gDeferredSunProgram` — Deferred Sun Shader

### `RenderDepthOfFieldChroma`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ② define `HAS_DOF_CHROMA`
- 根拠: llviewershadermgr.cpp:3711-3715, :3786-3792, :3819-3823
- **影響 shader (3)**:
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader

### `RenderDepthOfFieldFront`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ② define `FRONT_BLUR`
- 根拠: llviewershadermgr.cpp:3717-3721
- **影響 shader (1)**:
  - `gDeferredPostProgram` — Deferred Post Shader

### `RenderDepthOfFieldHighQuality`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ③ `postDeferredHQDoFF.glsl` ⇄ `postDeferredF.glsl` 差替
- 根拠: llviewershadermgr.cpp:3706-3709
- **影響 shader (1)**:
  - `gDeferredPostProgram` — Deferred Post Shader

### `RenderDiffuseLuminanceScale`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `LuminanceF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:8793
- **影響 shader (1)**:
  - `gLuminanceProgram` — Luminance

### `RenderDynamicExposureCoefficient`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `ExposureF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:8909
- **影響 shader (2)**:
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)

### `RenderEnableEmissiveBuffer`

- 種別 **非GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ② define `HAS_EMISSIVE`
- 根拠: llviewershadermgr.cpp:280-285, :1141-1146
- **影響 shader (43)**:
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredAvatarProgram` — Deferred Avatar Shader
  - `gDeferredBumpProgram` — Deferred Bump Shader
  - `gDeferredDiffuseAlphaMaskProgram` — Deferred Diffuse Alpha Mask Shader
  - `gDeferredDiffuseProgram` — Deferred Diffuse Shader
  - `gDeferredHighlightProgram` — Deferred Highlight Shader
  - `gDeferredImpostorProgram` — Deferred Impostor Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram` — Deferred Diffuse Non-Indexed Alpha Mask No Color Shader
  - `gDeferredNonIndexedDiffuseAlphaMaskProgram` — Deferred Diffuse Non-Indexed Alpha Mask Shader
  - `gDeferredPBROpaqueProgram` — Deferred PBR Opaque Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedBumpProgram`
  - `gDeferredSkinnedDiffuseAlphaMaskProgram`
  - `gDeferredSkinnedDiffuseProgram`
  - `gDeferredSkinnedPBROpaqueProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredStarProgram` — Deferred Star Program
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredTreeProgram` — Deferred Tree Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gGLTFPBRMetallicRoughnessProgram` — GLTF PBR Metallic Roughness Shader
  - `gHUDPBROpaqueProgram` — HUD PBR Opaque Shader
  - `gHighlightNormalProgram` — Highlight Normals Shader
  - `gHighlightProgram` — Highlight Shader
  - `gHighlightSpecularProgram` — Highlight Spec Shader
  - `gImpostorProgram` — Impostor Shader
  - `gObjectBumpProgram` — Bump Shader
  - `gOcclusionCubeProgram` — Occlusion Cube Shader
  - `gOcclusionProgram` — Occlusion Shader
  - `gSkinnedHighlightProgram`
  - `gSkinnedObjectBumpProgram`
  - `gSkinnedOcclusionProgram` — Skinned Occlusion Shader

### `RenderExposure`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `TonemapUtilF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:8996
- **影響 shader (7)**:
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapLegacyGammaCorrectProgram` — Deferred Tonemap Legacy Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gWaterProgram` — Water Shader

### `RenderGlobalLightStrength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `BlurLightF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11830
- **影響 shader (1)**:
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader

### `RenderGlowLumWeights`

- 種別 **非GUI** / 状態 **LIVE** / Type `Vector3`
- チャネル: ① UBO `GlowExtract_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9163 ; indra/newview/pipeline.cpp:9164
- **影響 shader (1)**:
  - `gGlowExtractProgram` — Glow Extract Shader (Post)

### `RenderGlowMaxExtractAlpha`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `GlowExtract_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9170
- **影響 shader (1)**:
  - `gGlowExtractProgram` — Glow Extract Shader (Post)

### `RenderGlowMinLuminance`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `GlowExtract_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9166
- **影響 shader (1)**:
  - `gGlowExtractProgram` — Glow Extract Shader (Post)

### `RenderGlowNoise`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ② define `HAS_NOISE`
- 根拠: llviewershadermgr.cpp:1396, :1407
- **影響 shader (3)**:
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gGlowExtractProgram` — Glow Extract Shader (Post)

### `RenderGlowResolutionPow`

- 種別 **GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ① UBO `Glow_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9238 ; indra/newview/pipeline.cpp:9239
- **影響 shader (1)**:
  - `gGlowProgram` — Glow Shader (Post)

### `RenderGlowStrength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `Glow_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9240
- **影響 shader (1)**:
  - `gGlowProgram` — Glow Shader (Post)

### `RenderGlowWarmthAmount`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `GlowExtract_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9171
- **影響 shader (1)**:
  - `gGlowExtractProgram` — Glow Extract Shader (Post)

### `RenderGlowWarmthWeights`

- 種別 **非GUI** / 状態 **LIVE** / Type `Vector3`
- チャネル: ① UBO `GlowExtract_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9167 ; indra/newview/pipeline.cpp:9168
- **影響 shader (1)**:
  - `gGlowExtractProgram` — Glow Extract Shader (Post)

### `RenderGlowWidth`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `Glow_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:9238 ; indra/newview/pipeline.cpp:9239
- **影響 shader (1)**:
  - `gGlowProgram` — Glow Shader (Post)

### `RenderHDREnabled`

- 種別 **非GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ① UBO `WindlightAtmos_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwlsky.cpp:270
- **影響 shader (73)**:
  - `gAYAAlphaPlateCompositeProgram` — AYAstorm Alpha Plate Composite
  - `gAYAForwardFlipCompositeProgram` — AYAstorm Forward Flip Composite
  - `gAvatarEyeballProgram` — Avatar Eyeball Program
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredEmissiveProgram` — Deferred Emissive Shader
  - `gDeferredFullbrightAlphaMaskAlphaProgram` — Deferred Fullbright Alpha Masking Alpha Shader
  - `gDeferredFullbrightAlphaMaskProgram` — Deferred Fullbright Alpha Masking Shader
  - `gDeferredFullbrightProgram` — Deferred Fullbright Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredPostGammaCorrectProgram` — Deferred Gamma Correction Post Process
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedEmissiveProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskAlphaProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskProgram`
  - `gDeferredSkinnedFullbrightProgram`
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)
  - `gFXAAProgram` — FXAA Shader ()
  - `gGaussianProgram` — Reflection Mip Shader
  - `gHUDFullbrightAlphaMaskAlphaProgram` — HUD Fullbright Alpha Masking Alpha Shader
  - `gHUDFullbrightAlphaMaskProgram` — HUD Fullbright Alpha Masking Shader
  - `gHUDFullbrightProgram` — HUD Fullbright Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gLegacyPostGammaCorrectProgram` — Legacy Gamma Correction Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gObjectAlphaMaskNoColorProgram` — No color alpha mask Shader
  - `gReflectionMipProgram` — Reflection Mip Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gRlvSphereProgram` — RLVa Sphere Post Processing Shader
  - `gSMAABlendWeightsProgram` — SMAA Blending Weights ()
  - `gSMAAEdgeDetectProgram` — SMAA Edge Detection ()
  - `gSMAANeighborhoodBlendProgram` — SMAA Neighborhood Blending ()
  - `gSMAAResolveProgram` — SMAA T2x Resolve ()
  - `gUnderWaterProgram` — Underwater Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderHDRSkySunlightScale`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `WindlightAtmos_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwlsky.cpp:270
- **影響 shader (73)**:
  - `gAYAAlphaPlateCompositeProgram` — AYAstorm Alpha Plate Composite
  - `gAYAForwardFlipCompositeProgram` — AYAstorm Forward Flip Composite
  - `gAvatarEyeballProgram` — Avatar Eyeball Program
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredEmissiveProgram` — Deferred Emissive Shader
  - `gDeferredFullbrightAlphaMaskAlphaProgram` — Deferred Fullbright Alpha Masking Alpha Shader
  - `gDeferredFullbrightAlphaMaskProgram` — Deferred Fullbright Alpha Masking Shader
  - `gDeferredFullbrightProgram` — Deferred Fullbright Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredPostGammaCorrectProgram` — Deferred Gamma Correction Post Process
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedEmissiveProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskAlphaProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskProgram`
  - `gDeferredSkinnedFullbrightProgram`
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)
  - `gFXAAProgram` — FXAA Shader ()
  - `gGaussianProgram` — Reflection Mip Shader
  - `gHUDFullbrightAlphaMaskAlphaProgram` — HUD Fullbright Alpha Masking Alpha Shader
  - `gHUDFullbrightAlphaMaskProgram` — HUD Fullbright Alpha Masking Shader
  - `gHUDFullbrightProgram` — HUD Fullbright Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gLegacyPostGammaCorrectProgram` — Legacy Gamma Correction Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gObjectAlphaMaskNoColorProgram` — No color alpha mask Shader
  - `gReflectionMipProgram` — Reflection Mip Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gRlvSphereProgram` — RLVa Sphere Post Processing Shader
  - `gSMAABlendWeightsProgram` — SMAA Blending Weights ()
  - `gSMAAEdgeDetectProgram` — SMAA Edge Detection ()
  - `gSMAANeighborhoodBlendProgram` — SMAA Neighborhood Blending ()
  - `gSMAAResolveProgram` — SMAA T2x Resolve ()
  - `gUnderWaterProgram` — Underwater Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderHeroProbeResolution`

- 種別 **GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ① UBO `RadianceGen_PerProgramBind`
- 根拠: indra/newview/llheroprobemanager.cpp:458
- **影響 shader (2)**:
  - `gHeroRadianceGenProgram` — Hero Radiance Gen Shader
  - `gRadianceGenProgram` — Radiance Gen Shader

### `RenderMirrors`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ② define `HERO_PROBES`
- 根拠: llviewershadermgr.cpp:1150, :1181-1184
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderMotionBlurStrength`

- 種別 **GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ① UBO `MotionBlurF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:5012
- **影響 shader (1)**:
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader

### `RenderNormalMapScale`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `NormgenF_PerProgramBind`
- 根拠: indra/newview/lldrawpoolbump.cpp:901
- **影響 shader (1)**:
  - `gNormalMapGenProgram` — Normal Map Generation Program

### `RenderPostGreyscaleStrength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `GlowCombine_PerShaderBind`
- 根拠: indra/newview/pipeline.cpp:9758
- **影響 shader (1)**:
  - `gGlowCombineProgram` — Glow Combine Shader

### `RenderPostPosterizationSamples`

- 種別 **GUI** / 状態 **LIVE** / Type `U32`
- チャネル: ① UBO `GlowCombine_PerShaderBind`
- 根拠: indra/newview/pipeline.cpp:9760
- **影響 shader (1)**:
  - `gGlowCombineProgram` — Glow Combine Shader

### `RenderPostSepiaStrength`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `GlowCombine_PerShaderBind`
- 根拠: indra/newview/pipeline.cpp:9759
- **影響 shader (1)**:
  - `gGlowCombineProgram` — Glow Combine Shader

### `RenderReflectionProbeLevel`

- 種別 **GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ② define `REFMAP_LEVEL`
- 根拠: llviewershadermgr.cpp:1154, :1177
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderReflectionsEnabled`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ② define `REFMAP_LEVEL` / `REF_SAMPLE_COUNT`
- 根拠: llviewershadermgr.cpp:1152, :1175-1179
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderSSAOFactor`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `AoUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11398 ; indra/newview/pipeline.cpp:11399
- **影響 shader (1)**:
  - `gDeferredSunProgram` — Deferred Sun Shader

### `RenderSSAOMaxScale`

- 種別 **GUI** / 状態 **LIVE** / Type `U32`
- チャネル: ① UBO `AoUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11397
- **影響 shader (1)**:
  - `gDeferredSunProgram` — Deferred Sun Shader

### `RenderSSAOScale`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `AoUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11396
- **影響 shader (1)**:
  - `gDeferredSunProgram` — Deferred Sun Shader

### `RenderScreenSpaceReflectionAdaptiveStepMultiplier`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `SSRUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12930
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderScreenSpaceReflectionDepthRejectBias`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `SSRUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12929
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderScreenSpaceReflectionDistanceBias`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `SSRUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12928
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderScreenSpaceReflectionGlossySamples`

- 種別 **GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ① UBO `SSRUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12931
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderScreenSpaceReflectionIterations`

- 種別 **GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ① UBO `SSRUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12924
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderScreenSpaceReflectionMaxDepth`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `SSRUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12933
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderScreenSpaceReflectionMaxRoughness`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `SSRUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12934
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderScreenSpaceReflectionRayStep`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `SSRUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:12925
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderScreenSpaceReflections`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ② define `SSR`
- 根拠: llviewershadermgr.cpp:1148, :1170-1173
- **影響 shader (14)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderShaderLightingMaxLevel`

- 種別 **GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ③ `sum_lights_class` を clamp = lighting の class 段
- 根拠: llviewershadermgr.cpp:1105-1108
- **影響 shader (0)**:
  - (オブジェクト単位で列挙不可 — チャネル欄参照)

### `RenderShadowBias`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `ShadowUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11358
- **影響 shader (20)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderShadowBiasError`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `ShadowUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11358
- **影響 shader (20)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderShadowBlurDistFactor`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `BlurLightF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11524 ; indra/newview/pipeline.cpp:11561
- **影響 shader (1)**:
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader

### `RenderShadowBlurSize`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `BlurLightF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11525 ; indra/newview/pipeline.cpp:11526
- **影響 shader (1)**:
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader

### `RenderShadowDetail`

- 種別 **GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ② define `SUN_SHADOW` / `SPOT_SHADOW`
- 根拠: llviewershadermgr.cpp:1157-1168 (>=1 SUN_SHADOW / >=2 SPOT_SHADOW)
- **影響 shader (20)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderShadowOffset`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `ShadowUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11362
- **影響 shader (20)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderShadowSoftness`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `ShadowUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11367
- **影響 shader (20)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderSkyAmbientScale`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `WindlightAtmos_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwlsky.cpp:271
- **影響 shader (73)**:
  - `gAYAAlphaPlateCompositeProgram` — AYAstorm Alpha Plate Composite
  - `gAYAForwardFlipCompositeProgram` — AYAstorm Forward Flip Composite
  - `gAvatarEyeballProgram` — Avatar Eyeball Program
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredEmissiveProgram` — Deferred Emissive Shader
  - `gDeferredFullbrightAlphaMaskAlphaProgram` — Deferred Fullbright Alpha Masking Alpha Shader
  - `gDeferredFullbrightAlphaMaskProgram` — Deferred Fullbright Alpha Masking Shader
  - `gDeferredFullbrightProgram` — Deferred Fullbright Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredPostGammaCorrectProgram` — Deferred Gamma Correction Post Process
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedEmissiveProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskAlphaProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskProgram`
  - `gDeferredSkinnedFullbrightProgram`
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)
  - `gFXAAProgram` — FXAA Shader ()
  - `gGaussianProgram` — Reflection Mip Shader
  - `gHUDFullbrightAlphaMaskAlphaProgram` — HUD Fullbright Alpha Masking Alpha Shader
  - `gHUDFullbrightAlphaMaskProgram` — HUD Fullbright Alpha Masking Shader
  - `gHUDFullbrightProgram` — HUD Fullbright Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gLegacyPostGammaCorrectProgram` — Legacy Gamma Correction Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gObjectAlphaMaskNoColorProgram` — No color alpha mask Shader
  - `gReflectionMipProgram` — Reflection Mip Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gRlvSphereProgram` — RLVa Sphere Post Processing Shader
  - `gSMAABlendWeightsProgram` — SMAA Blending Weights ()
  - `gSMAAEdgeDetectProgram` — SMAA Edge Detection ()
  - `gSMAANeighborhoodBlendProgram` — SMAA Neighborhood Blending ()
  - `gSMAAResolveProgram` — SMAA T2x Resolve ()
  - `gUnderWaterProgram` — Underwater Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderSkyAutoAdjustLegacy`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ① UBO `WaterF_PerProgramBind` + ① UBO `WindlightAtmos_PerProgramBind` + ① UBO `ExposureF_PerProgramBind` + ① UBO `DeferredUtil_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwater.cpp:331 ; indra/newview/lldrawpoolwlsky.cpp:217 ; indra/newview/pipeline.cpp:8910 ; indra/newview/pipeline.cpp:8911 ; indra/newview/pipeline.cpp:11334
- **影響 shader (73)**:
  - `gAYAAlphaPlateCompositeProgram` — AYAstorm Alpha Plate Composite
  - `gAYAForwardFlipCompositeProgram` — AYAstorm Forward Flip Composite
  - `gAvatarEyeballProgram` — Avatar Eyeball Program
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredEmissiveProgram` — Deferred Emissive Shader
  - `gDeferredFullbrightAlphaMaskAlphaProgram` — Deferred Fullbright Alpha Masking Alpha Shader
  - `gDeferredFullbrightAlphaMaskProgram` — Deferred Fullbright Alpha Masking Shader
  - `gDeferredFullbrightProgram` — Deferred Fullbright Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredPostGammaCorrectProgram` — Deferred Gamma Correction Post Process
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedEmissiveProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskAlphaProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskProgram`
  - `gDeferredSkinnedFullbrightProgram`
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)
  - `gFXAAProgram` — FXAA Shader ()
  - `gGaussianProgram` — Reflection Mip Shader
  - `gHUDFullbrightAlphaMaskAlphaProgram` — HUD Fullbright Alpha Masking Alpha Shader
  - `gHUDFullbrightAlphaMaskProgram` — HUD Fullbright Alpha Masking Shader
  - `gHUDFullbrightProgram` — HUD Fullbright Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gLegacyPostGammaCorrectProgram` — Legacy Gamma Correction Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gObjectAlphaMaskNoColorProgram` — No color alpha mask Shader
  - `gReflectionMipProgram` — Reflection Mip Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gRlvSphereProgram` — RLVa Sphere Post Processing Shader
  - `gSMAABlendWeightsProgram` — SMAA Blending Weights ()
  - `gSMAAEdgeDetectProgram` — SMAA Edge Detection ()
  - `gSMAANeighborhoodBlendProgram` — SMAA Neighborhood Blending ()
  - `gSMAAResolveProgram` — SMAA T2x Resolve ()
  - `gUnderWaterProgram` — Underwater Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderSkySunlightScale`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `WindlightAtmos_PerProgramBind`
- 根拠: indra/newview/lldrawpoolwlsky.cpp:270
- **影響 shader (73)**:
  - `gAYAAlphaPlateCompositeProgram` — AYAstorm Alpha Plate Composite
  - `gAYAForwardFlipCompositeProgram` — AYAstorm Forward Flip Composite
  - `gAvatarEyeballProgram` — Avatar Eyeball Program
  - `gAvatarProgram` — Avatar Shader
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredBlurLightProgram` — Deferred Blur Light Shader
  - `gDeferredCoFProgram` — Deferred CoF Shader
  - `gDeferredDoFCombineProgram` — Deferred DoFCombine Shader
  - `gDeferredEmissiveProgram` — Deferred Emissive Shader
  - `gDeferredFullbrightAlphaMaskAlphaProgram` — Deferred Fullbright Alpha Masking Alpha Shader
  - `gDeferredFullbrightAlphaMaskProgram` — Deferred Fullbright Alpha Masking Shader
  - `gDeferredFullbrightProgram` — Deferred Fullbright Shader
  - `gDeferredFullbrightShinyProgram` — Deferred FullbrightShiny Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMotionBlurProgram` — AYAstorm Deferred Motion Blur Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gDeferredPostGammaCorrectProgram` — Deferred Gamma Correction Post Process
  - `gDeferredPostNoDoFNoiseProgram` — Deferred Post NoDoF Noise Shader
  - `gDeferredPostNoDoFProgram` — Deferred Post NoDoF Shader
  - `gDeferredPostProgram` — Deferred Post Shader
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gDeferredSkinSSSProgram` — Skin SSS Prototype Shader
  - `gDeferredSkinnedEmissiveProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskAlphaProgram`
  - `gDeferredSkinnedFullbrightAlphaMaskProgram`
  - `gDeferredSkinnedFullbrightProgram`
  - `gDeferredSkinnedFullbrightShinyProgram`
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gDeferredTerrainProgram` — Deferred Terrain Shader
  - `gDeferredWLCloudProgram` — Deferred Windlight Cloud Program
  - `gDeferredWLMoonProgram` — Deferred Windlight Moon Program
  - `gDeferredWLSkyProgram` — Deferred Windlight Sky Shader
  - `gDeferredWLSunProgram` — Deferred Windlight Sun Program
  - `gEnvironmentMapProgram` — Environment Map Program
  - `gExposureProgram` — Exposure
  - `gExposureProgramNoFade` — Exposure (no fade)
  - `gFXAAProgram` — FXAA Shader ()
  - `gGaussianProgram` — Reflection Mip Shader
  - `gHUDFullbrightAlphaMaskAlphaProgram` — HUD Fullbright Alpha Masking Alpha Shader
  - `gHUDFullbrightAlphaMaskProgram` — HUD Fullbright Alpha Masking Shader
  - `gHUDFullbrightProgram` — HUD Fullbright Shader
  - `gHUDFullbrightShinyProgram` — HUD FullbrightShiny Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gLegacyPostGammaCorrectProgram` — Legacy Gamma Correction Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gObjectAlphaMaskNoColorProgram` — No color alpha mask Shader
  - `gReflectionMipProgram` — Reflection Mip Shader
  - `gReflectionProbeDisplayProgram` — Reflection Probe Display Shader
  - `gRlvSphereProgram` — RLVa Sphere Post Processing Shader
  - `gSMAABlendWeightsProgram` — SMAA Blending Weights ()
  - `gSMAAEdgeDetectProgram` — SMAA Edge Detection ()
  - `gSMAANeighborhoodBlendProgram` — SMAA Neighborhood Blending ()
  - `gSMAAResolveProgram` — SMAA T2x Resolve ()
  - `gUnderWaterProgram` — Underwater Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderSpotShadowBias`

- 種別 **非GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `ShadowUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11368
- **影響 shader (20)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderSpotShadowOffset`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `ShadowUtil_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:11369
- **影響 shader (20)**:
  - `gDeferredAvatarAlphaProgram` — Deferred Avatar Alpha Shader
  - `gDeferredAvatarEyesProgram` — Deferred Avatar Eyes Shader
  - `gDeferredGodraysProgram` — Godrays Shader
  - `gDeferredLightProgram` — Deferred Light Shader
  - `gDeferredMaterialProgram` — Skinned Material Shader
  - `gDeferredMultiLightProgram` — Deferred MultiLight Shader
  - `gDeferredMultiSpotLightProgram` — Deferred MultiSpotLight Shader
  - `gDeferredPBRAlphaProgram` — Deferred PBR Alpha Shader
  - `gDeferredSkinnedPBRAlphaProgram`
  - `gDeferredSkinnedShadowProgram` — Deferred Skinned Shadow Shader
  - `gDeferredSoftenProgram` — Deferred Soften Shader
  - `gDeferredSpotLightProgram` — Deferred SpotLight Shader
  - `gDeferredSunProbeProgram` — Deferred Sun Probe Shader
  - `gDeferredSunProgram` — Deferred Sun Shader
  - `gHazeProgram` — Haze Shader
  - `gHazeWaterProgram` — Water Haze Shader
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader
  - `gWaterProgram` — Water Shader
  - `nullptr` — Skinned Deferred Alpha Shader
  - `shaders`

### `RenderTerrainPBRDetail`

- 種別 **GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ② define `TERRAIN_PBR_DETAIL`
- 根拠: llviewershadermgr.cpp:1191-1193, :2003/:2025
- **影響 shader (2)**:
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gPBRTerrainBakeProgram` — Terrain Bake Shader RGB%o

### `RenderTerrainPBRPlanarSampleCount`

- 種別 **GUI** / 状態 **LIVE** / Type `S32`
- チャネル: ② define `TERRAIN_PLANAR_TEXTURE_SAMPLE_COUNT`
- 根拠: llviewershadermgr.cpp:1187-1188, :2005/:2027
- **影響 shader (2)**:
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gPBRTerrainBakeProgram` — Terrain Bake Shader RGB%o

### `RenderTerrainPBRTriplanarBlendFactor`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ② define `TERRAIN_TRIPLANAR_BLEND_FACTOR`
- 根拠: llviewershadermgr.cpp:1189-1190
- **影響 shader (2)**:
  - `gDeferredPBRTerrainProgram` — Deferred PBR Terrain Shader
  - `gPBRTerrainBakeProgram` — Terrain Bake Shader RGB%o

### `RenderTonemapType`

- 種別 **GUI** / 状態 **LIVE** / Type `U32`
- チャネル: ① UBO `TonemapUtilF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:8997 ; indra/newview/pipeline.cpp:8998
- **影響 shader (7)**:
  - `gDeferredPostTonemapGammaCorrectProgram` — Deferred Tonemap Gamma Post Process
  - `gDeferredPostTonemapLegacyGammaCorrectProgram` — Deferred Tonemap Legacy Gamma Post Process
  - `gDeferredPostTonemapProgram` — Deferred Tonemap Post Process
  - `gNoPostTonemapGammaCorrectProgram` — No Post Tonemap Gamma Post Process
  - `gNoPostTonemapLegacyGammaCorrectProgram` — No Post Tonemap Legacy Gamma Post Process
  - `gNoPostTonemapProgram` — No Post Tonemap Post Process
  - `gWaterProgram` — Water Shader

### `RenderTransparentWater`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ② define `TRANSPARENT_WATER`
- 根拠: llviewershadermgr.cpp:1292, :1325
- **影響 shader (2)**:
  - `gUnderWaterProgram` — Underwater Shader
  - `gWaterProgram` — Water Shader

### `RenderVolumetricLightingDirectional`

- 種別 **GUI** / 状態 **LIVE** / Type `Boolean`
- チャネル: ② define `GODRAYS_FADE`
- 根拠: llviewershadermgr.cpp:4187-4191
- **影響 shader (1)**:
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader

### `RenderVolumetricLightingFalloffMultiplier`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `VolumetricLightF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:5061
- **影響 shader (1)**:
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader

### `RenderVolumetricLightingMultiplier`

- 種別 **GUI** / 状態 **LIVE** / Type `F32`
- チャネル: ① UBO `VolumetricLightF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:5060
- **影響 shader (1)**:
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader

### `RenderVolumetricLightingResolution`

- 種別 **GUI** / 状態 **LIVE** / Type `U32`
- チャネル: ① UBO `VolumetricLightF_PerProgramBind`
- 根拠: indra/newview/pipeline.cpp:5059
- **影響 shader (1)**:
  - `gVolumetricLightProgram` — AYAstorm Volumetric Light Shader

---

## §B. GUI cvar のうち shader に届かないもの(描画系だが CPU 側)

| cvar | Type | 状態 | 主な参照 |
|---|---|---|---|
| `AYAR15GodraysInCinematicEnabled` | Boolean | LIVE | llcinematicoverlay.cpp, pipeline.cpp |
| `AYAR16AerialPerspectiveInCinematicEnabled` | Boolean | LIVE | lldrawpoolwlsky.cpp, llviewermenu.cpp |
| `AYAR18CloudVolumetricInCinematicEnabled` | Boolean | LIVE | lldrawpoolwlsky.cpp, llviewermenu.cpp |
| `AYAR20AvatarSkinSSSEnabled` | Boolean | LIVE | llcinematicoverlay.cpp, llviewermenu.cpp, pipeline.cpp |
| `AllowSelfImpostor` | Boolean | LIVE | llvoavatar.cpp |
| `AppearanceCameraMovement` | Boolean | LIVE | llfloaterpreference.cpp, llpaneleditwearable.cpp, llpresetsmanager.cpp |
| `AudioStreamingMedia` | Boolean | LIVE | llfloaterpreference.cpp, llmediadataclient.cpp, llpanelnearbymedia.cpp |
| `AvatarNameTagMode` | S32 | LIVE | llfloaterpreference.cpp, llviewerdisplay.cpp, llvoavatar.cpp |
| `CameraAngle` | F32 | LIVE | llpresetsmanager.cpp, llstartup.cpp, lltoolcomp.cpp |
| `CameraFocusTransitionTime` | F32 | LIVE | llayaudit.cpp, llviewermenu.cpp, pipeline.cpp |
| `CameraOffset` | Boolean | LIVE | pipeline.cpp |
| `CollectFontVertexBuffers` | Boolean | LIVE | pipeline.cpp |
| `DebugBeaconLineWidth` | S32 | LIVE | fsareasearch.cpp, fsfloaterposer.cpp, llviewermenu.cpp |
| `DebugRenderHitboxes` | Boolean | LIVE | lldrawpoolavatar.cpp |
| `DebugShowTextureInfo` | Boolean | LIVE | llviewertexture.cpp, llviewerwindow.cpp |
| `EnvironmentPersistAcrossLogin` | Boolean | LIVE | llenvironment.cpp |
| `FSAvatarTurnSpeed` | F32 | LIVE | llvoavatar.cpp |
| `FSColorClienttags` | U32 | LIVE | fsdata.cpp, llfloaterpreference.cpp, llvoavatar.cpp |
| `FSColorUsername` | Boolean | LIVE | llfloaterpreference.cpp, llvoavatar.cpp |
| `FSDisableTeleportScreens` | Boolean | LIVE | llviewerdisplay.cpp |
| `FSDisableTurningAroundWhenWalkingBackwards` | Boolean | LIVE | llvoavatar.cpp |
| `FSDrawDistanceVRAMOptimization` | Boolean | LIVE | llviewerdisplay.cpp |
| `FSEnvironmentManualTransitionTime` | F32 | LIVE | llenvironment.cpp, quickprefs.cpp |
| `FSFocusPointFollowsPointer` | Boolean | LIVE | pipeline.cpp |
| `FSFocusPointLocked` | Boolean | LIVE | pipeline.cpp |
| `FSFocusPointRender` | Boolean | LIVE | pipeline.cpp |
| `FSImpostorAvatarExclude` | U32 | LIVE | llvoavatar.cpp |
| `FSLegacyNametagPosition` | Boolean | LIVE | llvoavatar.cpp |
| `FSLimitTextureVRAMUsage` | Boolean | LIVE | llappviewer.cpp, llviewertexture.cpp |
| `FSNameTagShowLegacyUsernames` | Boolean | LIVE | llfloaterpreference.cpp, llpanelgroupnotices.cpp, llstartup.cpp |
| `FSNameTagZOffsetCorrection` | S32 | LIVE | llvoavatar.cpp |
| `FSRenderFarClipSteppingInterval` | U32 | LIVE | llviewerdisplay.cpp |
| `FSRenderParcelSelectionToMaxBuildHeight` | Boolean | LIVE | llglsandbox.cpp |
| `FSResetCameraOnTP` | Boolean | LIVE | llviewerdisplay.cpp, llviewermessage.cpp |
| `FSResetSkeletonOnStandUp` | Boolean | LIVE | llvoavatar.cpp |
| `FSRevokePerms` | U32 | LIVE | llvoavatar.cpp |
| `FSShowAutorespondInNametag` | Boolean | LIVE | llvoavatar.cpp |
| `FSShowChatRangeSpheres` | Boolean | LIVE | llviewerdisplay.cpp |
| `FSShowDisplayNameUpdateNotification` | Boolean | LIVE | llviewerdisplayname.cpp |
| `FSShowMyOwnVoiceVisualizer` | Boolean | LIVE | llvoavatar.cpp |
| `FSShowTypingStateInNameTag` | Boolean | LIVE | llvoavatar.cpp |
| `FSSnapshotFrameGuideColor` | Color3 | LIVE | pipeline.cpp |
| `FSSnapshotFrameGuideWidth` | F32 | LIVE | pipeline.cpp |
| `FSSnapshotGuideStyle` | String | LIVE | pipeline.cpp |
| `FSSnapshotGuideVisibility` | F32 | LIVE | pipeline.cpp |
| `FSSnapshotShowCaptureFrame` | Boolean | LIVE | pipeline.cpp |
| `FSSnapshotShowGuides` | Boolean | LIVE | pipeline.cpp |
| `FSTagShowARW` | Boolean | LIVE | llfloaterpreference.cpp, llvoavatar.cpp |
| `FSTagShowDistance` | Boolean | LIVE | llvoavatar.cpp |
| `FSTagShowDistanceColors` | Boolean | LIVE | llvoavatar.cpp |
| `FSTagShowOwnARW` | Boolean | LIVE | llfloaterpreference.cpp, llvoavatar.cpp |
| `FSTagShowTooComplexOnlyARW` | Boolean | LIVE | llfloaterpreference.cpp, llvoavatar.cpp |
| `LimitSelectDistance` | Boolean | LIVE | fsmaniptranslatejoint.cpp, llglsandbox.cpp, llmaniprotate.cpp |
| `LoginLocation` | String | LIVE | fspanellogin.cpp, llappviewer.cpp, llpanellogin.cpp |
| `MaxSelectDistance` | F32 | LIVE | fsmaniptranslatejoint.cpp, llglsandbox.cpp, llmaniprotate.cpp |
| `MediaAutoPlayHuds` | Boolean | LIVE | llvovolume.cpp |
| `NameTagDebugAVRezState` | Boolean | LIVE | llvoavatar.cpp |
| `NameTagShowDisplayNames` | Boolean | LIVE | llvoavatar.cpp |
| `NameTagShowFriends` | Boolean | LIVE | lggcontactsets.cpp, llfloaterpreference.cpp, llvoavatar.cpp |
| `NameTagShowGroupTitles` | Boolean | LIVE | llviewerdisplay.cpp |
| `NameTagShowUsernames` | Boolean | LIVE | fschathistory.cpp, fscommon.cpp, fsradar.cpp |
| `PlayModeUISndTyping` | Boolean | LIVE | llvoavatar.cpp |
| `PreviewAmbientColor` | Color4 | LIVE | pipeline.cpp |
| `PrivatePointAtTarget` | Boolean | LIVE | llagentcamera.cpp, llviewermenu.cpp, llvoavatarself.cpp |
| `RenderAttachedLights` | Boolean | LIVE | llviewermenu.cpp, pipeline.cpp |
| `RenderAttachedParticles` | Boolean | LIVE | llayaudit.cpp, llviewermenu.cpp, pipeline.cpp |
| `RenderAutoMaskAlphaDeferred` | Boolean | LIVE | pipeline.cpp |
| `RenderAutoMaskAlphaNonDeferred` | Boolean | LIVE | pipeline.cpp |
| `RenderAvatarComplexityMode` | S32 | LIVE | llfloaterpreference.cpp, llfloaterpreferencesgraphicsadvanced.cpp, llviewercontrol.cpp |
| `RenderAvatarFriendsOnly` | Boolean | LIVE | lldrawpoolavatar.cpp, llvoavatar.cpp |
| `RenderAvatarMaxComplexity` | U32 | LIVE | fsfloaterperformance.cpp, llavatarrendernotifier.cpp, llfloaterperformance.cpp |
| `RenderAvatarMaxNonImpostors` | U32 | LIVE | llappviewer.cpp, llfloaterpreference.cpp, llfloaterpreferencesgraphicsadvanced.cpp |
| `RenderCASSharpness` | F32 | LIVE | llayaudit.cpp, pipeline.cpp |
| `RenderDefaultProbeUpdatePeriod` | F32 | LIVE | llreflectionmapmanager.cpp |
| `RenderDeferred` | Boolean | LIVE | llayaudit.cpp, llfloaterperformance.cpp, llfloaterpreference.cpp |
| `RenderDeferredBlurLight` | Boolean | LIVE | llayaudit.cpp, pipeline.cpp |
| `RenderDeferredLights` | Boolean | LIVE | pipeline.cpp |
| `RenderDepthOfField` | Boolean | LIVE | llayaudit.cpp, llviewercontrol.cpp, llviewermenu.cpp |
| `RenderDepthOfFieldAlphas` | Boolean | LIVE | lldrawpoolalpha.cpp |
| `RenderDepthOfFieldInEditMode` | Boolean | LIVE | pipeline.cpp |
| `RenderDesaturateIrradiance` | Boolean | LIVE | lldrawpoolwlsky.cpp |
| `RenderDisablePostProcessing` | Boolean | LIVE | pipeline.cpp |
| `RenderDynamicLOD` | Boolean | LIVE | llviewercontrol.cpp, pipeline.cpp |
| `RenderEnableFullbright` | Boolean | LIVE | llprimitive.cpp, llviewercontrol.cpp, llvovolume.cpp |
| `RenderFSAASamples` | U32 | LIVE | llfloaterpreference.cpp, pipeline.cpp |
| `RenderFSAAType` | U32 | LIVE | llayaudit.cpp, llfloaterpreference.cpp, llviewercontrol.cpp |
| `RenderFarClip` | F32 | LIVE | chatbar_as_cmdline.cpp, fsradar.cpp, llagent.cpp |
| `RenderGlow` | Boolean | LIVE | llayaudit.cpp, llviewercontrol.cpp, llviewershadermgr.cpp |
| `RenderGlowHDR` | Boolean | LIVE | llviewercontrol.cpp, pipeline.cpp |
| `RenderGlowIterations` | S32 | LIVE | llayaudit.cpp, pipeline.cpp |
| `RenderHDRIExposure` | F32 | LIVE | lldrawpoolwlsky.cpp |
| `RenderHDRIIrradianceOnly` | Boolean | LIVE | lldrawpoolwlsky.cpp |
| `RenderHDRIRotation` | F32 | LIVE | lldrawpoolwlsky.cpp |
| `RenderHDRISplitScreen` | F32 | LIVE | lldrawpoolwlsky.cpp |
| `RenderHeroProbeConservativeUpdateMultiplier` | S32 | LIVE | pipeline.cpp |
| `RenderHeroProbeDistance` | F32 | LIVE | llheroprobemanager.cpp |
| `RenderHeroProbeUpdateRate` | S32 | LIVE | llheroprobemanager.cpp, pipeline.cpp |
| `RenderLocalLightCount` | S32 | LIVE | llgltfmaterialpreviewmgr.cpp, pipeline.cpp |
| `RenderMaxTextureResolution` | U32 | LIVE | llappviewerwin32.cpp, llfloaterregioninfo.cpp, llviewermenu.cpp |
| `RenderMaxVRAMBudget` | U32 | LIVE | llappviewer.cpp, llfloaterpreference.cpp, llviewertexture.cpp |
| `RenderMotionBlur` | Boolean | LIVE | llayaudit.cpp, llviewercontrol.cpp, llviewermenu.cpp |
| `RenderMotionBlurOtherAvatars` | Boolean | LIVE | llayaudit.cpp, lldrawpool.cpp, lldrawpoolavatar.cpp |
| `RenderMotionBlurSelfAvatar` | Boolean | LIVE | llayaudit.cpp, lldrawpool.cpp, lldrawpoolavatar.cpp |
| `RenderNameShowSelf` | Boolean | LIVE | llvoavatar.cpp |
| `RenderNameShowTime` | F32 | LIVE | llvoavatar.cpp |
| `RenderOtherAttachedLights` | Boolean | LIVE | pipeline.cpp |
| `RenderOwnAttachedLights` | Boolean | LIVE | pipeline.cpp |
| `RenderReflectionProbeCount` | U32 | LIVE | llreflectionmapmanager.cpp, llviewercontrol.cpp |
| `RenderReflectionProbeDetail` | S32 | LIVE | llreflectionmap.cpp, llreflectionmapmanager.cpp, llviewercontrol.cpp |
| `RenderReflectionProbeDrawDistance` | F32 | LIVE | llviewerdisplay.cpp |
| `RenderReflectionProbeMaxLocalLightAmbiance` | F32 | LIVE | llreflectionmapmanager.cpp |
| `RenderReflectionProbeResolution` | U32 | LIVE | llreflectionmapmanager.cpp |
| `RenderReflectionProbeShowTransparent` | Boolean | LIVE | llviewermenu.cpp, llvovolume.cpp |
| `RenderReflectionProbeVolumes` | Boolean | LIVE | pipeline.cpp |
| `RenderSSAOIrradianceMax` | F32 | LIVE | pipeline.cpp |
| `RenderSSAOIrradianceScale` | F32 | LIVE | pipeline.cpp |
| `RenderShaderCacheEnabled` | Boolean | LIVE | llviewershadermgr.cpp |
| `RenderShadowAutomaticDistance` | Boolean | LIVE | llayaudit.cpp, pipeline.cpp |
| `RenderShadowFOVCutoff` | F32 | LIVE | pipeline.cpp |
| `RenderShadowFarClip` | F32 | LIVE | pipeline.cpp |
| `RenderShadowResolutionScale` | F32 | LIVE | llayaudit.cpp, llviewercontrol.cpp, llviewermenu.cpp |
| `RenderSkyAutoAdjustAmbientScale` | F32 | LIVE | lldrawpoolwlsky.cpp |
| `RenderSkyAutoAdjustBlueDensityScale` | F32 | LIVE | lldrawpoolwlsky.cpp |
| `RenderSkyAutoAdjustBlueHorizonScale` | F32 | LIVE | lldrawpoolwlsky.cpp |
| `RenderSkyAutoAdjustProbeAmbiance` | F32 | LIVE | llenvironment.cpp |
| `RenderSkyAutoAdjustSunColorScale` | F32 | LIVE | lldrawpoolwlsky.cpp |
| `RenderTerrainPBRScale` | F32 | LIVE | lldrawpoolterrain.cpp, llviewercontrol.cpp |
| `RenderTerrainScale` | F32 | LIVE | lldrawpoolterrain.cpp, llpanelopenregionsettings.cpp, llviewercontrol.cpp |
| `RenderTextureVRAMDivisor` | U32 | LIVE | llviewertexture.cpp |
| `RenderTonemapMix` | F32 | LIVE | llsettingsvo.cpp, pipeline.cpp |
| `RenderUnloadedAvatar` | Boolean | LIVE | llvoavatar.cpp |
| `RenderUseFarClip` | Boolean | LIVE | pipeline.cpp |
| `RenderVolumeSAFrameMax` | F32 | LIVE | llvovolume.cpp |
| `RenderVolumeSAProtection` | Boolean | LIVE | llvovolume.cpp |
| `RenderVolumetricLighting` | Boolean | LIVE | llayaudit.cpp, lldrawpoolalpha.cpp, llviewermenu.cpp |
| `ShowAxes` | Boolean | LIVE | llviewerdisplay.cpp |
| `ShowParcelOwners` | Boolean | LIVE | llappviewer.cpp, lldrawpoolterrain.cpp, llfloatertools.cpp |
| `ShowVoiceVisualizersInCalls` | Boolean | LIVE | llvoavatar.cpp |
| `Stream3DShowOccluders` | Boolean | LIVE | pipeline.cpp |
| `TextureDisable` | Boolean | LIVE | llvovolume.cpp |
| `TextureDiscardBackgroundedTime` | F32 | LIVE | llviewertexture.cpp |
| `TextureDiscardLevel` | U32 | LIVE | lltextureview.cpp, llviewertexture.cpp, llvoavatarself.cpp |
| `TextureDiscardMinimizedTime` | F32 | LIVE | llviewertexture.cpp |
| `TextureLoadFullRes` | Boolean | LIVE | llviewertexture.cpp |
| `UseChatBubbles` | Boolean | LIVE | fsconsoleutils.cpp, llfloaterimnearbychathandler.cpp, llviewermessage.cpp |
| `UseLSLBridge` | Boolean | LIVE | fslslbridge.cpp, llstartup.cpp, llvoavatarself.cpp |
| `UseOcclusion` | Boolean | LIVE | llfeaturemanager.cpp, llviewercontrol.cpp, pipeline.cpp |
| `UseTypingBubbles` | Boolean | LIVE | llvoavatar.cpp |
| `VoiceVisualizerEnabled` | Boolean | LIVE | llvoavatar.cpp, llvoicevisualizer.cpp |
| `WindLightUseAtmosShaders` | Boolean | LIVE | llfloaterperformance.cpp, llfloaterpreference.cpp, llfloaterpreferencesgraphicsadvanced.cpp |
| `fsregioncornerbeacons` | Boolean | LIVE | llviewermenu.cpp, pipeline.cpp |
| `moapbeacon` | Boolean | LIVE | llfloaterbeacons.cpp, llviewermenu.cpp, pipeline.cpp |
| `moonbeacon` | Boolean | LIVE | llenvironment.cpp, llpaneleditsky.cpp, llsky.cpp |
| `particlesbeacon` | Boolean | LIVE | llfloaterbeacons.cpp, llviewermenu.cpp, pipeline.cpp |
| `physicalbeacon` | Boolean | LIVE | llfloaterbeacons.cpp, llviewermenu.cpp, pipeline.cpp |
| `renderbeacons` | Boolean | LIVE | llfloaterbeacons.cpp, llviewermenu.cpp, pipeline.cpp |
| `renderhighlights` | Boolean | LIVE | llfloaterbeacons.cpp, llviewermenu.cpp, pipeline.cpp |
| `scriptsbeacon` | Boolean | LIVE | llfloaterbeacons.cpp, llviewermenu.cpp, pipeline.cpp |
| `scripttouchbeacon` | Boolean | LIVE | llfloaterbeacons.cpp, llviewermenu.cpp, pipeline.cpp |
| `soundsbeacon` | Boolean | LIVE | llfloaterbeacons.cpp, llviewermenu.cpp, pipeline.cpp |
| `sunbeacon` | Boolean | LIVE | llenvironment.cpp, llpaneleditsky.cpp, llsky.cpp |

---

## §C-2. 非GUI かつ DEAD — 182 個

GUI にも出ず C++ も読まない。※ 冒頭 10 件と違い**個別精査していない**(機械判定のみ)。

<details><summary>全 182 件を開く</summary>

| cvar | Type | Comment |
|---|---|---|
| `AutoMimeDiscovery` | Boolean | Enable viewer mime type discovery of media URLs |
| `BrowserPluginsEnabled` | Boolean | Enable Web plugins in the built-in Web browser? |
| `BrowserWebSecurityDisabled` | Boolean | Disable web security features in the built-in Web browser? |
| `CameraMouseWheelZoom` | S32 | Camera zooms in and out with mouse wheel |
| `CameraPreset` | U32 | (Deprecated) Preset camera position - view (0 - rear, 1 - front, 2 - group) |
| `ClickToWalk` | Boolean | (obsolete)Click in world to walk to location |
| `CloseChatOnEmptyReturn` | Boolean | Close the chat transcript floater after hitting return on an empty line |
| `CloseIMOnEmptyReturn` | Boolean | Close the IM floater after hitting return on an empty line |
| `CurlRequestTimeOut` | F32 | Max idle time of a curl request before killed (requires restart) |
| `DoubleClickAutoPilot` | Boolean | (Obsolete)Enable double-click auto pilot |
| `EnableVisualLeakDetector` | Boolean | EnableVisualLeakDetector |
| `FSContactsSortOrder` | U32 | Specifies sort order for friends (0 = by display name, 1 = username, 2 = by onli |
| `FSDestroyGLTexturesImmediately` | Boolean | If enabled, GL textures will be removed from memory immediately when its fetched |
| `FSDestroyGLTexturesThreshold` | F32 | Threshold, at what texture memory load level GL textures will be removed from me |
| `FSDisableRiggedMeshMatrixCaching` | Boolean | Disable the caching of Rigged mesh matrix pallettes.Non-persistant. |
| `FSLastSnapshotToFacebookHeight` | S32 | The height of the last Facebook snapshot, in px |
| `FSLastSnapshotToFacebookResolution` | S32 | At what resolution should snapshots be posted on Facebook. 0=Current Window, 1=3 |
| `FSLastSnapshotToFacebookWidth` | S32 | The width of the last Facebook snapshot, in px |
| `FSLastSnapshotToTwitterHeight` | S32 | The height of the last Twitter snapshot, in px |
| `FSLastSnapshotToTwitterResolution` | S32 | At what resolution should snapshots be posted on Twitter. 0=Current Window, 1=32 |
| `FSLastSnapshotToTwitterWidth` | S32 | The width of the last Twitter snapshot, in px |
| `FSLatencyOneTimeFixRun` | Boolean | One time fix has run for this install for script dialog colors on Latency |
| `FSPerfFloaterSmoothingPeriods` | U32 | Number of periods to smooth the stats over |
| `FSPoserShowBoneHighlights` | Boolean | Whether to highlight a bone with the debug beacon on selection from the UI. |
| `FSSkinClobbersColorPrefs` | Boolean | If enabled the default color scheme for newly selected skins will be overwritten |
| `FSVolumeControlsPanelOpen` | Boolean | Internal control for visibility of volume control panel. |
| `FilterItemsMaxTimePerFrameVisible` | S32 | Max time devoted to items filtering per frame for visible inventory listings (in |
| `FirstSelectedDisabledPopups` | Boolean | Return false if there is not disabled popup selected in the list of floater pref |
| `FirstSelectedEnabledPopups` | Boolean | Return false if there is not enable popup selected in the list of floater prefer |
| `FloaterActiveSpeakersSortAscending` | Boolean | Whether to sort up or down |
| `FloaterActiveSpeakersSortColumn` | String | Column name to sort on |
| `FloaterMapEast` | String | Floater Map East Label |
| `FloaterMapNorth` | String | Floater Map North Label |
| `FloaterMapNorthEast` | String | Floater Map North-East Label |
| `FloaterMapNorthWest` | String | Floater Map North-West Label |
| `FloaterMapSouth` | String | Floater Map South Label |
| `FloaterMapSouthEast` | String | Floater Map South-East Label |
| `FloaterMapSouthWest` | String | Floater Map South-West Label |
| `FloaterMapWest` | String | Floater Map West Label |
| `FloaterStatisticsRect` | Rect | Rectangle for chat transcript |
| `FlycamAbsolute` | Boolean | Treat Flycam values as absolute positions (not deltas). |
| `ForceAddressSize` | U32 | Force Windows update to 32-bit or 64-bit viewer. |
| `FullScreenAspectRatio` | F32 | Aspect ratio of fullscreen display (width / height) |
| `FullScreenAutoDetectAspectRatio` | Boolean | Automatically detect proper aspect ratio for fullscreen display |
| `GesturesMarketplaceURL` | String | URL to the Gestures Marketplace |
| `GridCrossSections` | Boolean | Highlight cross sections of prims with grid manipulation plane. |
| `HowToHelpURL` | String | URL for How To help content |
| `InventoryAutoOpenDelay` | F32 | Seconds before automatically opening inventory when mouse is over inventory butt |
| `KeepAspectForDiskSnapshot` | Boolean | Always keep width to height ratio in local snapshots the same, regardless of ima |
| `KeepAspectForEmailSnapshot` | Boolean | Always keep width to height ratio in postcard snapshots the same, regardless of  |
| `KeepAspectForInventorySnapshot` | Boolean | Always keep width to height ratio in inventory snapshots the same, regardless of |
| `KeepAspectForProfileSnapshot` | Boolean | Always keep width to height ratio in profile snapshots the same, regardless of i |
| `LocalTerrainAsset1` | String | If set to a non-null UUID, overrides the terrain asset locally for all regions w |
| `LocalTerrainAsset2` | String | If set to a non-null UUID, overrides the terrain asset locally for all regions w |
| `LocalTerrainAsset3` | String | If set to a non-null UUID, overrides the terrain asset locally for all regions w |
| `LocalTerrainAsset4` | String | If set to a non-null UUID, overrides the terrain asset locally for all regions w |
| `Marker` | String | [NOT USED] |
| `MarketplaceURL_bodypartFemale` | String | URL to the Marketplace Bodyparts Female |
| `MarketplaceURL_bodypartMale` | String | URL to the Marketplace Bodyparts Male |
| `MarketplaceURL_clothingFemale` | String | URL to the Marketplace Clothing Female |
| `MarketplaceURL_clothingMale` | String | URL to the Marketplace Clothing Male |
| `MarketplaceURL_eyesFemale` | String | URL to the Marketplace Eyes Female |
| `MarketplaceURL_eyesMale` | String | URL to the Marketplace Eyes Male |
| `MarketplaceURL_glovesFemale` | String | URL to the Marketplace Gloves Female |
| `MarketplaceURL_glovesMale` | String | URL to the Marketplace Gloves Male |
| `MarketplaceURL_hairFemale` | String | URL to the Marketplace Hair Female |
| `MarketplaceURL_hairMale` | String | URL to the Marketplace Hair Male |
| `MarketplaceURL_jacketFemale` | String | URL to the Marketplace Jacket Female |
| `MarketplaceURL_jacketMale` | String | URL to the Marketplace Jacket Male |
| `MarketplaceURL_objectFemale` | String | URL to the Marketplace Attachments Female |
| `MarketplaceURL_objectMale` | String | URL to the Marketplace Attachments Male |
| `MarketplaceURL_pantsFemale` | String | URL to the Marketplace Pants Female |
| `MarketplaceURL_pantsMale` | String | URL to the Marketplace Pants Male |
| `MarketplaceURL_physicsFemale` | String | URL to the Marketplace Bodyparts Female |
| `MarketplaceURL_physicsMale` | String | URL to the Marketplace Bodyparts Male |
| `MarketplaceURL_shapeFemale` | String | URL to the Marketplace Shape Female |
| `MarketplaceURL_shapeMale` | String | URL to the Marketplace Shape Male |
| `MarketplaceURL_shirtFemale` | String | URL to the Marketplace Shirt Female |
| `MarketplaceURL_shirtMale` | String | URL to the Marketplace Shirt Male |
| `MarketplaceURL_shoesFemale` | String | URL to the Marketplace Shoes Female |
| `MarketplaceURL_shoesMale` | String | URL to the Marketplace Shoes Male |
| `MarketplaceURL_skinFemale` | String | URL to the Marketplace Skin Female |
| `MarketplaceURL_skinMale` | String | URL to the Marketplace Skins Male |
| `MarketplaceURL_skirtFemale` | String | URL to the Marketplace Skirt Female |
| `MarketplaceURL_skirtMale` | String | URL to the Marketplace Skirt Male |
| `MarketplaceURL_socksFemale` | String | URL to the Marketplace Socks Female |
| `MarketplaceURL_socksMale` | String | URL to the Marketplace Socks Male |
| `MarketplaceURL_tattooFemale` | String | URL to the Marketplace Tattoo Female |
| `MarketplaceURL_tattooMale` | String | URL to the Marketplace Tattoo Male |
| `MarketplaceURL_underpantsFemale` | String | URL to the Marketplace Underwear Female |
| `MarketplaceURL_underpantsMale` | String | URL to the Marketplace Underwear Male |
| `MarketplaceURL_undershirtFemale` | String | URL to the Marketplace Undershirt Female |
| `MarketplaceURL_undershirtMale` | String | URL to the Marketplace Undershirt Male |
| `MePanelOpened` | Boolean | Indicates that Me Panel was opened at least once after Viewer was installed |
| `MemoryFailurePreventionEnabled` | Boolean | If set, the viewer will try to throttle memory allocations when memory is low (3 |
| `MemoryPrivatePoolEnabled` | Boolean | (Deprecated) Enable the private memory pool management |
| `MemoryPrivatePoolSize` | U32 | (Deprecated) Size of the private memory pool in MB (min. value is 256) |
| `NearbyListShowRange` | Boolean | Show range field on nearby avList? |
| `NotificationCanEmbedInIM` | S32 | Controls notification panel embedding in IMs (0 = default, 1 = focused, 2 = neve |
| `NotifyTipDuration` | F32 | Length of time that notification tips stay on screen (seconds) |
| `OpenSidePanelsInFloaters` | Boolean | If true, will always open side panel contents in a floater. |
| `PermissionsCautionNotifyBoxHeight` | S32 | Height of caution-style notification messages |
| `PicksPerSecondMouseMoving` | F32 | How often to perform hover picks while the mouse is moving (picks per second) |
| `PicksPerSecondMouseStationary` | F32 | How often to perform hover picks while the mouse is stationary (picks per second |
| `PieMenuLineWidth` | F32 | Width of lines in pie menu display (pixels) |
| `RLVaEnableCompositeFolders` | Boolean | Enables composite folders for shared inventory |
| `RegInClient` | Boolean | Experimental: Embed registration in login screen |
| `RenderAutoMuteLogging` | Boolean | Show extra information in viewer logs about avatar rendering costs |
| `RenderAutoMuteRenderWeightLimit` | U32 | OBSOLETE. This setting has been renamed RenderAvatarMaxNonImpostors. |
| `RenderAvatar` | Boolean | Render Avatars |
| `RenderBakeSunlight` | Boolean | Bake sunlight into vertex buffers for static objects. |
| `RenderBumpmapMinDistanceSquared` | F32 | Maximum distance at which to render bumpmapped primitives (distance in meters, s |
| `RenderComplexityColorMax` | Color4 | Unused obsolete setting |
| `RenderComplexityColorMid` | Color4 | Unused obsolete setting |
| `RenderComplexityColorMin` | Color4 | Unused obsolete setting |
| `RenderComplexityStaticMax` | S32 | Unused obsolete setting |
| `RenderComplexityThreshold` | S32 | Unused obsolete setting |
| `RenderDebugAlphaMask` | F32 | Test Alpha Masking Cutoffs. |
| `RenderDebugSH` | Boolean | Enable SH indirect lighting visualization. |
| `RenderDebugTextureBind` | Boolean | Enable texture bind performance test. |
| `RenderDeferredAlphaSoften` | F32 | Scalar for softening alpha surfaces (for soft particles). |
| `RenderDeferredDisplayGamma` | F32 | Gamma ramp exponent for final correction before display gamma. |
| `RenderDeferredSun` | Boolean | Execute sunlight shader in deferred renderer. |
| `RenderDelayVBUpdate` | Boolean | Delay vertex buffer updates until just before rendering |
| `RenderGLContextCoreProfile` | Boolean | Don't use a compatibility profile OpenGL context.  Requires restart. |
| `RenderHoverGlowEnable` | Boolean | DEPRECATED --- Show glow effect when hovering on interactive objects. |
| `RenderMinimumLODTriangleCount` | U32 | Triangle count threshold at which automatic LOD generation stops |
| `RenderNoAlpha` | Boolean | Disable rendering of alpha objects (render all alpha objects as alpha masks). |
| `RenderPreferStreamDraw` | Boolean | Use GL_STREAM_DRAW in place of GL_DYNAMIC_DRAW |
| `RenderReflectionRes` | S32 | Reflection map resolution. |
| `RenderScreenSpaceReflectionSplitEnd` | Vector3 | Ending splits for each SSR pass. |
| `RenderScreenSpaceReflectionSplitStart` | Vector3 | Starting splits for each SSR pass. |
| `RenderShaderLODThreshold` | F32 | Fraction of draw distance defining the switch to a different shader LOD |
| `RenderShaderParticleThreshold` | F32 | Fraction of draw distance to not use shader on particles |
| `RenderShadowBlurSamples` | U32 | Number of samples to take for each pass of shadow blur (value range 1-16).  Actu |
| `RenderShadowProjExponent` | F32 | Exponent applied to transition between ortho and perspective shadow projections  |
| `RenderShadowProjOffset` | F32 | Amount to scale distance to virtual origin of shadow perspective projection. |
| `RenderShadowSlopeThreshold` | F32 | Cutoff slope value for points to affect perspective shadow generation |
| `RenderSpecularPrecision` | U32 | Force 32-bit floating point LUT |
| `RenderUseAdvancedAtmospherics` | Boolean | Use fancy precomputed atmospherics and stuff. |
| `RenderUseTriStrips` | Boolean | DEPRECATED - now always assumed to be false - Use triangle strips for rendering  |
| `RenderUseVAO` | Boolean | [EXPERIMENTAL] Use GL Vertex Array Objects. |
| `RenderVBOMappingDisable` | Boolean | Disable VBO glMapBufferARB |
| `RenderWaterMaterials` | S32 | Water planar reflections include materials rendering. |
| `RunMultipleThreads` | Boolean | If TRUE keep background threads active during render. No longer used; always tre |
| `SaveMinidumpType` | U32 | Type of minidump that is created (0 - minimal, 1 - normal [default], 2 - extende |
| `ServerChoice` | S32 | [DO NOT MODIFY] Controls which grid you connect to |
| `ShowAdultGroups` | Boolean | Display results of find groups that are flagged as adult |
| `ShowBetaGrids` | Boolean | Display the beta grids in the grid selection control. |
| `ShowNearClip` | Boolean |  |
| `ShowPGGroups` | Boolean | Display results of find groups that are flagged as general |
| `ShowPermissions` | Boolean |  |
| `ShowProfileFloaters` | Boolean | Shows resident profiles in a floater rather than the side tray |
| `ShowVoiceChannelPopup` | Boolean | Controls visibility of the current voice channel popup above the voice tab |
| `ShowVolumeSettingsPopup` | Boolean | Show individual volume slider for voice, sound effects, etc |
| `SkinningSettingsFile` | String | Client skin color setting file name (per install). |
| `SnapshotLayers` | S32 | Which layers should be used for a snapshot. |
| `SnapshotToDiskQuality` | S32 | Quality setting of snapshot to disk JPEGs (0 = worst, 100 = best) |
| `StatsFile` | String | Filename for stats logging output |
| `StatsSessionTrackFrameStats` | Boolean | Track rendering and network statistics |
| `StatsSummaryFile` | String | Filename for stats logging summary |
| `TextureBiasUnimportantFactor` | F32 | When biasing textures to lower resolution due to lack of vram, the importance th |
| `UIFloaterHPad` | S32 | Size of UI floater horizontal pad |
| `UIFloaterTestBool` | Boolean | Example saved setting for the test floater |
| `UIImgInvisibleUUID` | String |  |
| `UIMaxComboWidth` | S32 | Maximum width of combo box |
| `UISndChatPing` | String | Sound file for chat ping(uuid for sound asset) |
| `UseWebPagesOnPrims` | Boolean | [NOT USED] |
| `VersionChannelName` | String | Version information generated by running the viewer |
| `VoiceHost` | String | Client SLVoice host to connect to |
| `VoiceImageLevel0` | String | Texture UUID for voice image level 0 |
| `VoiceImageLevel1` | String | Texture UUID for voice image level 1 |
| `VoiceImageLevel2` | String | Texture UUID for voice image level 2 |
| `VoiceImageLevel3` | String | Texture UUID for voice image level 3 |
| `VoiceImageLevel4` | String | Texture UUID for voice image level 4 |
| `VoiceImageLevel5` | String | Texture UUID for voice image level 5 |
| `VoiceImageLevel6` | String | Texture UUID for voice image level 6 |
| `VoiceLogFile` | String | Log file to use when launching the voice daemon |
| `VoicePort` | U32 | Client SLVoice port to connect to |
| `WaterEditPresets` | Boolean | Whether to be able to edit the water defaults or not |
| `WaterGLFogDensityScale` | F32 | Maps shader water fog density to gl fog density |
| `WebProfileFloaterRect` | Rect | Web profile floater dimensions |

</details>

---

## §D. 全 cvar 表(2501)

| # | cvar | Type | 種別 | 状態 | shader | Comment |
|---:|---|---|---|---|:---:|---|
| 1 | `360CaptureCameraFOV` | U32 | 非GUI | LIVE |  | Field of view of the WebGL camera that converts the cubemap to an equi |
| 2 | `360CaptureDebugSaveImage` | Boolean | 非GUI | LIVE |  | Flag if set, saves off each cube map as an image, as well as the JavaS |
| 3 | `360CaptureHideAvatars` | Boolean | GUI | LIVE |  | Flag if set, removes all the avatars from the 360 snapshot |
| 4 | `360CaptureJPEGEncodeQuality` | U32 | 非GUI | LIVE |  | Quality value to use in the JPEG encoder (0..100) |
| 5 | `360CaptureNumRenderPasses` | U32 | 非GUI | LIVE |  | Number of times to render the scene while taking a snapshot |
| 6 | `360CaptureOutputImageWidth` | U32 | 非GUI | LIVE |  | Width of the output 360 equirectangular image |
| 7 | `360QualitySelection` | U32 | GUI | LIVE(widget経由) |  | Quality level for the 360 snapshot |
| 8 | `AFKTimeout` | S32 | GUI | LIVE |  | Time before automatically setting AFK (away from keyboard) mode (secon |
| 9 | `AYAChatWindowStyle` | S32 | GUI | LIVE |  | Chat window style: 0=FS V1 (plain text), 1=FS V7 (modern headers), 2=L |
| 10 | `AYACinematicModeActive` | Boolean | GUI | LIVE |  | Helper Boolean shadow of (AYAVisualRealismEnabled==2). LL XUI enabled_ |
| 11 | `AYACinematicOverlayApplied` | Boolean | 非GUI | LIVE |  | Sentinel: settings_cinematic_bd.xml が現セッションで Cinematic mode に適用済みなら 1、 |
| 12 | `AYALLChatCompactView` | Boolean | 非GUI | LIVE |  | LL Style chat window: true=Compact View, false=Expanded View (independ |
| 13 | `AYAOpusCodecEnable` | Boolean | 非GUI | LIVE |  | AYAstorm validation switch for registering the custom FMOD Ogg Opus/Vo |
| 14 | `AYAOpusCodecPriority` | U32 | 非GUI | LIVE |  | AYAstorm validation setting for FMOD::System::registerCodec priority o |
| 15 | `AYAPipelineCacheSizeMB` | U32 | 非GUI | LIVE |  | (r41) VkPipelineCache disk persist 上限 (MB)。default=64、~/.ayastorm_x64/ |
| 16 | `AYAR14Strength` | F32 | GUI | LIVE | ✅ | r14 効果 (Volumetric Atmosphere + Sun Dazzle) の強度を 0=OFF 相当 / 1=現在の ON 相 |
| 17 | `AYAR14VolumetricAtmosphereInCinematicEnabled` | Boolean | GUI | LIVE | ✅ | r14 Volumetric Atmosphere (altitude density + scene-referred linear in |
| 18 | `AYAR15GodraysCinematicMigrationVersion` | S32 | 非GUI | LIVE |  | AYAR15GodraysInCinematicEnabled default flip (OFF -> ON) one-shot migr |
| 19 | `AYAR15GodraysInCinematicEnabled` | Boolean | GUI | LIVE |  | r15 Godrays (godraysF post-process pass) を Cinematic mode で有効化。default |
| 20 | `AYAR15GodraysPhaseExponent` | F32 | GUI | LIVE | ✅ | r15 Godrays Mie phase 関数の forward-peak exponent (pow(cos_theta, e))。1. |
| 21 | `AYAR15GodraysStrength` | F32 | GUI | LIVE | ✅ | r15 Godrays の加算強度 (light_color * accum * phase * strength)。0.05 = うっすら |
| 22 | `AYAR16AerialPerspectiveEnabled` | Boolean | 非GUI | LIVE | ✅ | r16 Aerial Perspective: scene 経路 atmosphericsFuncs の Rayleigh λ^-4 波長依 |
| 23 | `AYAR16AerialPerspectiveInCinematicEnabled` | Boolean | GUI | LIVE |  | r16 Aerial Perspective (Rayleigh λ^-4 波長依存散乱) を Cinematic mode で有効化。de |
| 24 | `AYAR16AerialPerspectiveStrength` | F32 | GUI | LIVE | ✅ | r16 Aerial Perspective (Rayleigh λ^-4) の強度を 0=OFF 相当 / 1=現在の ON 相当 で連続 |
| 25 | `AYAR17ColorTemperatureEnabled` | Boolean | 非GUI | LIVE |  | r17 Color Temperature: 太陽 elevation から派生する Kelvin (2200K horizon ～ 650 |
| 26 | `AYAR17ColorTemperatureInCinematicEnabled` | Boolean | GUI | LIVE |  | r17 Color Temperature (太陽 Kelvin modulator) を Cinematic mode で有効化。defa |
| 27 | `AYAR17Strength` | F32 | GUI | LIVE |  | r17 Color Temperature (太陽 Kelvin modulator) の強度を 0=OFF 相当 (white modul |
| 28 | `AYAR18CloudVolumetricEnabled` | Boolean | 非GUI | LIVE | ✅ | r18 Cloud Volumetric: 既存 2D cloud_noise_texture を視線方向の slab raymarch で |
| 29 | `AYAR18CloudVolumetricInCinematicEnabled` | Boolean | GUI | LIVE |  | r18 Cloud Volumetric (slab raymarch 疑似体積雲) を Cinematic mode で有効化。defau |
| 30 | `AYAR18CloudVolumetricStrength` | F32 | GUI | LIVE | ✅ | r18 Cloud Volumetric (slab raymarch) の強度を 0=OFF 相当 / 1=現在の ON 相当 で連続 l |
| 31 | `AYAR19TranslucencyEnabled` | Boolean | 非GUI | LIVE |  | r19 Translucency: softenLightF の Legacy / PBR 両分岐に wrap-around diffuse |
| 32 | `AYAR19TranslucencyInCinematicEnabled` | Boolean | 非GUI | LIVE |  | r19 Translucency (wrap-around diffuse + back-light transmission) を Cin |
| 33 | `AYAR19TranslucencyIntensity` | U32 | 非GUI | LIVE |  | r19 Translucency 強度段階: 0=OFF (完全 no-op) / 1=控えめ (default) / 2=標準 / 3=強 |
| 34 | `AYAR20AvatarSkinSSSBlurRadius` | F32 | GUI | LIVE | ✅ | r20 SSS blur 半径 (eye_dist=1m での pixel 単位、world-scale 適用)。shader 内で `r_ |
| 35 | `AYAR20AvatarSkinSSSEnabled` | Boolean | GUI | LIVE |  | r20 Avatar Skin SSS: screen-space 5-tap separable blur (wavelength-dep |
| 36 | `AYAR20AvatarSkinSSSGlowColor` | Color4 | GUI | LIVE | ✅ | r20 Phase D: glow restore additive の色。default pure red (1.0, 0.0, 0.0) |
| 37 | `AYAR20AvatarSkinSSSGlowGain` | F32 | GUI | LIVE | ✅ | r20 Phase D: SSS blur 後の lit^3 に additive を足す gain。SSS で眠くなった hi-light |
| 38 | `AYAR20AvatarSkinSSSInCinematicEnabled` | Boolean | 非GUI | LIVE |  | DEPRECATED (r30 P? r20 consolidation): AYAR20AvatarSkinSSSEnabled が mo |
| 39 | `AYAR20AvatarSkinSSSStrength` | F32 | GUI | LIVE | ✅ | r20 P0a tuning: SSS blur と元画面の mix 比率 (0.0 = OFF 相当 / 1.0 = blur 100%) |
| 40 | `AYAR20AvatarSkinSSSWhitelist` | String | GUI | LIVE |  | r20 SSS 対象 mesh asset UUID whitelist。改行区切り、各行が 1 件の mesh UUID (36 文字 d |
| 41 | `AYAR20SSSEffective` | Boolean | 非GUI | LIVE |  | Helper Boolean shadow of (AYAVisualRealismEnabled > 0). XUI enabled_co |
| 42 | `AYAR20SSSMigrationVersion` | S32 | 非GUI | LIVE |  | r20 SSS cvar migration counter。default 0。起動時 llappviewer.cpp で version |
| 43 | `AYAViewModeMigrationVersion` | S32 | 非GUI | LIVE |  | AYAVisualRealismEnabled view mode migration counter。default 0。起動時 llap |
| 44 | `AYAVisualRealismEnabled` | U32 | GUI | LIVE | ✅ | AYAstorm View Mode 切替。UI picker は 2 件 (0=Firestorm View 上流 Firestorm 標 |
| 45 | `AYAuditMode` | Boolean | 非GUI | LIVE |  | Run Cinematic cvar audit on login: sweep all bound cvars, dump snapsho |
| 46 | `AYAuditOutputDir` | String | 非GUI | LIVE |  | Directory where AYAudit snapshots are written. |
| 47 | `AbuseReportScreenshotDelay` | F32 | 非GUI | LIVE |  | Time delay before taking screenshot to avoid UI artifacts. |
| 48 | `AckCollectTime` | F32 | 非GUI | LIVE |  | Ack messages collection and grouping time |
| 49 | `ActiveFloaterTransparency` | F32 | GUI | LIVE |  | Transparency of active floaters (floaters that have focus) |
| 50 | `AdminMenu` | Boolean | 非GUI | LIVE |  | Enable the debug admin menu from the main menu.  Note: This will just  |
| 51 | `AdvanceSnapshot` | Boolean | GUI | LIVE |  | Display advanced parameter settings in snapshot interface |
| 52 | `AgentPause` | Boolean | GUI | LIVE |  | Ask the simulator to stop updating the agent while enabled |
| 53 | `AlertedUnsupportedHardware` | Boolean | 非GUI | LIVE |  | Set if there's unsupported hardware and we've already done a notificat |
| 54 | `AllowMUpose` | Boolean | GUI | LIVE |  | Allow MU* pose style in chat and IM (with ':' as a synonymous to '/me  |
| 55 | `AllowMultipleViewers` | Boolean | GUI | LIVE |  | Allow multiple viewers. |
| 56 | `AllowNoCopyRezRestoreToWorld` | Boolean | 非GUI | LIVE |  | Allow Restore to Last Position for no-copy objects on Second Life grid |
| 57 | `AllowSelectAvatar` | Boolean | GUI(menu) | LIVE |  | Allows user to select and move avatars, move is viewer sided, does not |
| 58 | `AllowSelfImpostor` | Boolean | GUI | LIVE |  | Allow own render time to impostor your avatar. |
| 59 | `AllowTapTapHoldRun` | Boolean | GUI | LIVE |  | Tapping a direction key twice and holding it down makes avatar run |
| 60 | `AnalyzePerformance` | Boolean | 非GUI | LIVE |  | Request performance analysis for a particular viewer run |
| 61 | `AnimateTextures` | Boolean | GUI(menu) | LIVE |  | Enable texture animation (debug) |
| 62 | `AnimatedObjectsAllowLeftClick` | Boolean | 非GUI | LIVE |  | Allow left-click interaction with animated objects. Uncertain how much |
| 63 | `AnimatedObjectsMaxLegalOffset` | F32 | 非GUI | LIVE |  | Max visual offset between object position and rendered position |
| 64 | `AnimatedObjectsMaxLegalSize` | F32 | 非GUI | LIVE |  | Max bounding box size for animated object's rendered position |
| 65 | `AppearanceCameraMovement` | Boolean | GUI | LIVE |  | When entering appearance editing mode, camera zooms in on currently se |
| 66 | `ApplyColorImmediately` | Boolean | 非GUI | LIVE |  | Preview selections in color picker immediately |
| 67 | `ArrowKeysAlwaysMove` | Boolean | GUI | LIVE |  | While cursor is in chat entry box, arrow keys still control your avata |
| 68 | `AssetFetchConcurrency` | U32 | 非GUI | LIVE |  | Maximum number of HTTP connections used for asset fetches |
| 69 | `AuctionShowFence` | Boolean | GUI | LIVE(widget経由) |  | When auctioning land, include parcel boundary marker in snapshot |
| 70 | `AudioLevelAmbient` | F32 | GUI | LIVE |  | Audio level of environment sounds |
| 71 | `AudioLevelMaster` | F32 | GUI | LIVE |  | Master audio level, or overall volume |
| 72 | `AudioLevelMedia` | F32 | GUI | LIVE |  | Audio level of QuickTime movies |
| 73 | `AudioLevelMic` | F32 | GUI | LIVE |  | Audio level of microphone input |
| 74 | `AudioLevelMusic` | F32 | GUI | LIVE |  | Audio level of streaming music |
| 75 | `AudioLevelSFX` | F32 | GUI | LIVE |  | Audio level of in-world sound effects |
| 76 | `AudioLevelUI` | F32 | GUI | LIVE |  | Audio level of UI sound effects |
| 77 | `AudioLevelVoice` | F32 | GUI | LIVE |  | Audio level of voice chat |
| 78 | `AudioLevelWind` | F32 | 非GUI | LIVE |  | Audio level of wind noise when standing still |
| 79 | `AudioStreamingMedia` | Boolean | GUI | LIVE |  | Enable streaming |
| 80 | `AudioStreamingMusic` | Boolean | GUI | LIVE |  | Enable streaming audio |
| 81 | `AutoAcceptNewInventory` | Boolean | GUI | LIVE |  | Automatically accept new notecards/textures/landmarks |
| 82 | `AutoCloseOOC` | Boolean | GUI | LIVE |  | Auto-close OOC chat (i.e. add "))" if not found and "((" was used) |
| 83 | `AutoDisengageMic` | Boolean | GUI | LIVE |  | Automatically turn off the microphone when ending IM calls. |
| 84 | `AutoLeveling` | Boolean | GUI | LIVE |  | Keep Flycam level. |
| 85 | `AutoLogin` | Boolean | 非GUI | LIVE |  | Login automatically using last username/password combination |
| 86 | `AutoMimeDiscovery` | Boolean | 非GUI | DEAD |  | Enable viewer mime type discovery of media URLs |
| 87 | `AutoPilotLocksCamera` | Boolean | 非GUI | LIVE |  | Keep camera position locked when avatar walks to selected position |
| 88 | `AutoQueryGridStatus` | Boolean | GUI | LIVE |  | Query status.secondlifegrid.net for latest news at login. |
| 89 | `AutoQueryGridStatusURL` | String | 非GUI | LIVE |  | URL for AutoQueryGridStatus. WordPress RSS 2.0 format. |
| 90 | `AutoReplace` | Boolean | 非GUI | LIVE |  | Replaces keywords with a configured word or phrase |
| 91 | `AutoSnapshot` | Boolean | 非GUI | LIVE |  | Update snapshot when camera stops moving, or any parameter changes |
| 92 | `AutoTuneFPS` | Boolean | GUI | LIVE |  | Allow the viewer to adjust your settings to achieve target FPS |
| 93 | `AutoTuneImpostorByDistEnabled` | Boolean | GUI | LIVE |  | Enable/disable using MaxNonImpostor to limit avatar rendering by dista |
| 94 | `AutoTuneImpostorFarAwayDistance` | F32 | GUI | LIVE |  | Avatars beyond this range will automatically be optimized |
| 95 | `AutoTuneLock` | Boolean | GUI | LIVE |  | When enabled the viewer will dynamically change settings until auto tu |
| 96 | `AutoTuneRenderFarClipMin` | F32 | GUI | LIVE |  | The lowest draw distance that auto tune is allowed to use |
| 97 | `AutoTuneRenderFarClipTarget` | F32 | GUI | LIVE |  | The draw distance that auto tune will try to achieve |
| 98 | `AutohideChatBar` | Boolean | GUI | LIVE |  | Hide the chat bar from the bottom button bar and only show it as an ov |
| 99 | `AutomaticFly` | Boolean | GUI | LIVE |  | Fly by holding jump key or using "Fly" command (FALSE = fly by using " |
| 100 | `AvatarAlignMini` | Boolean | 非GUI | LIVE |  | Use the compact mini compass floater instead of the full compass. |
| 101 | `AvatarAxisDeadZone0` | F32 | GUI | LIVE |  | Avatar axis 0 dead zone. |
| 102 | `AvatarAxisDeadZone1` | F32 | GUI | LIVE |  | Avatar axis 1 dead zone. |
| 103 | `AvatarAxisDeadZone2` | F32 | GUI | LIVE |  | Avatar axis 2 dead zone. |
| 104 | `AvatarAxisDeadZone3` | F32 | 非GUI | LIVE |  | Avatar axis 3 dead zone. |
| 105 | `AvatarAxisDeadZone4` | F32 | GUI | LIVE |  | Avatar axis 4 dead zone. |
| 106 | `AvatarAxisDeadZone5` | F32 | GUI | LIVE |  | Avatar axis 5 dead zone. |
| 107 | `AvatarAxisScale0` | F32 | GUI | LIVE |  | Avatar axis 0 scaler. |
| 108 | `AvatarAxisScale1` | F32 | GUI | LIVE |  | Avatar axis 1 scaler. |
| 109 | `AvatarAxisScale2` | F32 | GUI | LIVE |  | Avatar axis 2 scaler. |
| 110 | `AvatarAxisScale3` | F32 | 非GUI | LIVE |  | Avatar axis 3 scaler. |
| 111 | `AvatarAxisScale4` | F32 | GUI | LIVE |  | Avatar axis 4 scaler. |
| 112 | `AvatarAxisScale5` | F32 | GUI | LIVE |  | Avatar axis 5 scaler. |
| 113 | `AvatarBakedTextureUploadTimeout` | U32 | 非GUI | LIVE |  | Specifies the maximum time in seconds to wait before sending your bake |
| 114 | `AvatarExtentRefreshMaxPerBatch` | S32 | 非GUI | LIVE |  | how many avatars do we want to handle in total per batch (default is 5 |
| 115 | `AvatarExtentRefreshPeriodBatch` | S32 | 非GUI | LIVE |  | how many frames do we spread over by default when refreshing extents ( |
| 116 | `AvatarFeathering` | F32 | GUI | LIVE |  | Avatar feathering (less is softer) |
| 117 | `AvatarInspectorTooltipDelay` | F32 | GUI | LIVE |  | Seconds before displaying avatar inspector tooltip |
| 118 | `AvatarNameTagMode` | S32 | GUI | LIVE |  | Select Avatar Name Tag Mode |
| 119 | `AvatarPhysics` | Boolean | 非GUI | LIVE |  | Enable avatar physics. |
| 120 | `AvatarRotateThresholdFast` | F32 | 非GUI | LIVE |  | Angle between avatar facing and camera facing at which avatar turns to |
| 121 | `AvatarRotateThresholdSlow` | F32 | 非GUI | LIVE |  | Angle between avatar facing and camera facing at which avatar turns to |
| 122 | `AvatarSex` | U32 | GUI | LIVE |  |  |
| 123 | `AvatarSitOnAway` | Boolean | GUI | LIVE |  | Sit down when marked AFK |
| 124 | `AvatarSitRotation` | Quaternion | 非GUI | LIVE |  | Avatar real sitting rotation used in preset |
| 125 | `AvatarWelcomePack` | String | 非GUI | LIVE |  | Avatar Welcome Pack contents |
| 126 | `AzureTranslateAPIKey` | LLSD | 非GUI | LIVE |  | Azure Translation service data to use with the MS Azure Translator API |
| 127 | `BackgroundYieldTime` | S32 | GUI | LIVE |  | Amount of time to yield every frame to other applications when SL is n |
| 128 | `BasicUITooltips` | Boolean | 非GUI | LIVE |  | Show tooltips for various 2D UI elements like buttons or checkboxes, w |
| 129 | `BatchSizeAIS3` | S32 | 非GUI | LIVE |  | Amount of folder ais packs into category subset request |
| 130 | `BlockAvatarAppearanceMessages` | Boolean | 非GUI | LIVE |  | Ignores appearance messages (for simulating Ruth) |
| 131 | `BlockPeopleSortOrder` | U32 | 非GUI | LIVE |  | Specifies sort order for recent people (0 = by name, 1 = by type) |
| 132 | `BrowserEnableJSObject` | Boolean | 非GUI | LIVE |  | (WARNING: Advanced feature. Use if you are aware of the implications). |
| 133 | `BrowserFileAccessFromFileUrls` | Boolean | 非GUI | LIVE |  | Allow access to local files via file urls in the embedded browser |
| 134 | `BrowserIgnoreSSLCertErrors` | Boolean | 非GUI | LIVE |  | FOR TESTING ONLY: Tell the built-in web browser to ignore SSL cert err |
| 135 | `BrowserJavascriptEnabled` | Boolean | GUI | LIVE |  | Enable JavaScript in the built-in Web browser? |
| 136 | `BrowserPluginsEnabled` | Boolean | 非GUI | DEAD |  | Enable Web plugins in the built-in Web browser? |
| 137 | `BrowserProxyAddress` | String | GUI | LIVE |  | Address for the Web Proxy] |
| 138 | `BrowserProxyEnabled` | Boolean | GUI | LIVE |  | Use Web Proxy |
| 139 | `BrowserProxyPort` | S32 | GUI | LIVE |  | Port for Web Proxy |
| 140 | `BrowserWebSecurityDisabled` | Boolean | 非GUI | DEAD |  | Disable web security features in the built-in Web browser? |
| 141 | `BuildAxisDeadZone0` | F32 | GUI | LIVE |  | Build axis 0 dead zone. |
| 142 | `BuildAxisDeadZone1` | F32 | GUI | LIVE |  | Build axis 1 dead zone. |
| 143 | `BuildAxisDeadZone2` | F32 | GUI | LIVE |  | Build axis 2 dead zone. |
| 144 | `BuildAxisDeadZone3` | F32 | GUI | LIVE |  | Build axis 3 dead zone. |
| 145 | `BuildAxisDeadZone4` | F32 | GUI | LIVE |  | Build axis 4 dead zone. |
| 146 | `BuildAxisDeadZone5` | F32 | GUI | LIVE |  | Build axis 5 dead zone. |
| 147 | `BuildAxisScale0` | F32 | GUI | LIVE |  | Build axis 0 scaler. |
| 148 | `BuildAxisScale1` | F32 | GUI | LIVE |  | Build axis 1 scaler. |
| 149 | `BuildAxisScale2` | F32 | GUI | LIVE |  | Build axis 2 scaler. |
| 150 | `BuildAxisScale3` | F32 | GUI | LIVE |  | Build axis 3 scaler. |
| 151 | `BuildAxisScale4` | F32 | GUI | LIVE |  | Build axis 4 scaler. |
| 152 | `BuildAxisScale5` | F32 | GUI | LIVE |  | Build axis 5 scaler. |
| 153 | `BuildFeathering` | F32 | GUI | LIVE |  | Build feathering (less is softer) |
| 154 | `BulkChangeEveryoneCopy` | Boolean | GUI | LIVE |  | Bulk changed objects can be copied by everyone |
| 155 | `BulkChangeIncludeAnimations` | Boolean | GUI | LIVE |  | Bulk permission changes affect animations |
| 156 | `BulkChangeIncludeBodyParts` | Boolean | GUI | LIVE |  | Bulk permission changes affect body parts |
| 157 | `BulkChangeIncludeClothing` | Boolean | GUI | LIVE |  | Bulk permission changes affect clothing |
| 158 | `BulkChangeIncludeGestures` | Boolean | GUI | LIVE |  | Bulk permission changes affect gestures |
| 159 | `BulkChangeIncludeMaterials` | Boolean | GUI | LIVE |  | Bulk permission changes affect materials |
| 160 | `BulkChangeIncludeNotecards` | Boolean | GUI | LIVE |  | Bulk permission changes affect notecards |
| 161 | `BulkChangeIncludeObjects` | Boolean | GUI | LIVE |  | Bulk permission changes affect objects |
| 162 | `BulkChangeIncludeScripts` | Boolean | GUI | LIVE |  | Bulk permission changes affect scripts |
| 163 | `BulkChangeIncludeSettings` | Boolean | GUI | LIVE |  | Bulk permission changes affect environment settings |
| 164 | `BulkChangeIncludeSounds` | Boolean | GUI | LIVE |  | Bulk permission changes affect sounds |
| 165 | `BulkChangeIncludeTextures` | Boolean | GUI | LIVE |  | Bulk permission changes affect textures |
| 166 | `BulkChangeNextOwnerCopy` | Boolean | GUI | LIVE |  | Bulk changed objects can be copied by next owner |
| 167 | `BulkChangeNextOwnerModify` | Boolean | GUI | LIVE |  | Bulk changed objects can be modified by next owner |
| 168 | `BulkChangeNextOwnerTransfer` | Boolean | GUI | LIVE |  | Bulk changed objects can be resold or given away by next owner |
| 169 | `BulkChangeShareWithGroup` | Boolean | GUI | LIVE |  | Bulk changed objects are shared with the currently active group |
| 170 | `BulkUpload2KTextures` | Boolean | 非GUI | LIVE |  | Bulk upload scales textures to 2K if true, to 1K if false |
| 171 | `CacheLocation` | String | 非GUI | LIVE |  | Controls the location of the local disk cache |
| 172 | `CacheLocationTopFolder` | String | GUI | LIVE |  | Controls the top folder location of the local disk cache |
| 173 | `CacheSize` | U32 | GUI | LIVE |  | Controls amount of hard drive space reserved for local texture caching |
| 174 | `CacheValidateCounter` | U32 | 非GUI | LIVE |  | Used to distribute cache validation |
| 175 | `CallLogSortOrder` | U32 | 非GUI | LIVE |  | Specifies sort order for Call Log (0 = by name, 1 = by date) |
| 176 | `CameraAngle` | F32 | GUI | LIVE |  | Camera field of view angle (Radians) |
| 177 | `CameraDoFResScale` | F32 | GUI | LIVE | ✅ | Amount to scale down depth of field resolution.  Valid range is 0.25 ( |
| 178 | `CameraFNumber` | F32 | GUI | LIVE | ✅ | Camera f-number value for DoF effect |
| 179 | `CameraFieldOfView` | F32 | GUI | LIVE | ✅ | Vertical camera field of view for DoF effect (in degrees) |
| 180 | `CameraFocalLength` | F32 | GUI | LIVE | ✅ | Camera focal length for DoF effect (in millimeters) |
| 181 | `CameraFocusTransitionTime` | F32 | GUI | LIVE |  | How many seconds it takes the camera to transition between focal dista |
| 182 | `CameraMaxCoF` | F32 | GUI | LIVE | ✅ | Maximum camera circle of confusion for DoF effect |
| 183 | `CameraMouseWheelZoom` | S32 | 非GUI | DEAD |  | Camera zooms in and out with mouse wheel |
| 184 | `CameraOffset` | Boolean | GUI(menu) | LIVE |  | Render with camera offset from view frustum (rendering debug) |
| 185 | `CameraOffsetBuild` | Vector3 | 非GUI | LIVE |  | Default camera position relative to focus point when entering build mo |
| 186 | `CameraOffsetRearView` | Vector3 | 非GUI | LIVE |  | Initial camera offset from avatar in Rear View |
| 187 | `CameraOffsetScale` | F32 | GUI | LIVE |  | Scales the default offset |
| 188 | `CameraOpacity` | F32 | GUI | LIVE |  | Opacity of the Camera Controls floater |
| 189 | `CameraPosOnLogout` | Vector3D | 非GUI | LIVE |  | Camera position when last logged out (global coordinates) |
| 190 | `CameraPositionSmoothing` | F32 | GUI | LIVE |  | Smooths camera position over time |
| 191 | `CameraPreset` | U32 | 非GUI | DEAD |  | (Deprecated) Preset camera position - view (0 - rear, 1 - front, 2 - g |
| 192 | `CameraPresetType` | U32 | 非GUI | LIVE |  | Preset camera position - view (0 - rear, 1 - front, 2 - group, 3 - cus |
| 193 | `CameraZoomDistance` | F32 | 非GUI | LIVE |  | Camera distance to the zoomed in avatar |
| 194 | `CameraZoomEyeZOffset` | F32 | 非GUI | LIVE |  | Camera height offset of the camera itself on the zoomed in avatar from |
| 195 | `CameraZoomFocusZOffset` | F32 | 非GUI | LIVE |  | Camera height offset of zoomed point on the zoomed in avatar from avat |
| 196 | `CameraZoomFraction` | F32 | 非GUI | LIVE(XML専用) |  | Mousewheel driven fraction of zoom |
| 197 | `CefVerboseLog` | Boolean | 非GUI | LIVE |  | Enable/disable CEF verbose logging |
| 198 | `CertStore` | String | 非GUI | LIVE |  | Specifies the Certificate Store for certificate trust verification |
| 199 | `ChannelBottomPanelMargin` | S32 | GUI | LIVE |  | Space from a lower toast to the Bottom Tray |
| 200 | `ChatAutocompleteGestures` | Boolean | 非GUI | LIVE |  | Auto-complete gestures in nearby chat |
| 201 | `ChatBubbleOpacity` | F32 | GUI | LIVE |  | Opacity of chat bubble background (0.0 = completely transparent, 1.0 = |
| 202 | `ChatConsoleFontSize` | S32 | GUI | LIVE |  | Size of chat text in chat console (0 to 3, small to huge) |
| 203 | `ChatFontSize` | S32 | GUI | LIVE |  | Size of chat text in chat floater (0 to 3, small to huge) |
| 204 | `ChatFullWidth` | Boolean | GUI | LIVE |  | Chat console takes up full width of SL window |
| 205 | `ChatHistoryTornOff` | Boolean | 非GUI | LIVE |  | Show chat transcript window separately from Communicate window. |
| 206 | `ChatLoadGroupMaxMembers` | S32 | 非GUI | LIVE |  | Max number of active members we'll show up for an unresponsive group |
| 207 | `ChatOnlineNotification` | Boolean | GUI | LIVE |  | Provide notifications for when friend log on and off of SL |
| 208 | `ChatPersistTime` | F32 | GUI | LIVE |  | Time for which chat stays visible in console (seconds) |
| 209 | `ChatTabDirection` | S32 | GUI | LIVE |  | Toggles the direction of chat tabs between horizontal and vertical |
| 210 | `CheesyBeacon` | Boolean | GUI(menu) | LIVE |  | Enable cheesy beacon effects |
| 211 | `ClickOnAvatarKeepsCamera` | Boolean | GUI | LIVE |  | Normally, clicking on your avatar resets the camera position. This opt |
| 212 | `ClickToWalk` | Boolean | 非GUI | DEAD |  | (obsolete)Click in world to walk to location |
| 213 | `ClientSettingsFile` | String | 非GUI | LIVE |  | Client settings file name (per install). |
| 214 | `CloseChatOnEmptyReturn` | Boolean | 非GUI | DEAD |  | Close the chat transcript floater after hitting return on an empty lin |
| 215 | `CloseChatOnReturn` | Boolean | GUI | LIVE |  | Close chat after hitting return |
| 216 | `CloseIMOnEmptyReturn` | Boolean | 非GUI | DEAD |  | Close the IM floater after hitting return on an empty line |
| 217 | `CmdLineChannel` | String | 非GUI | LIVE |  | Command line specified channel name |
| 218 | `CmdLineDisableVoice` | Boolean | GUI | LIVE |  | Disable Voice. |
| 219 | `CmdLineGridChoice` | String | 非GUI | LIVE |  | The user's grid choice or ip address. |
| 220 | `CmdLineHelperURI` | String | 非GUI | LIVE |  | Command line specified helper web CGI prefix to use. |
| 221 | `CmdLineLoginLocation` | String | 非GUI | LIVE |  | Startup destination requested on command line |
| 222 | `CmdLineLoginURI` | LLSD | 非GUI | LIVE |  | Command line specified login server and CGI prefix to use. |
| 223 | `CmdLineSkipUpdater` | Boolean | 非GUI | LIVE |  | Command line skip updater check. |
| 224 | `CmdLineUpdateService` | String | 非GUI | LIVE |  | Override the url base for the update query. |
| 225 | `CollectFontVertexBuffers` | Boolean | GUI(menu) | LIVE |  | When enabled some UI elements with cache buffers generated by fonts an |
| 226 | `ColorSettingsHideDefault` | Boolean | GUI | LIVE |  | Show non-default settings only in Color Settings list |
| 227 | `ComplexityChangesPopUpDelay` | U32 | 非GUI | LIVE |  | Delay before viewer will show avatar complexity notice again |
| 228 | `ConnectAsGod` | Boolean | 非GUI | LIVE |  | Log in as god if you have god access. |
| 229 | `ConnectionPort` | U32 | GUI | LIVE |  | Custom connection port number |
| 230 | `ConnectionPortEnabled` | Boolean | GUI | LIVE |  | Use the custom connection port? |
| 231 | `ConsoleBackgroundOpacity` | F32 | GUI | LIVE |  | Opacity of chat console (0.0 = completely transparent, 1.0 = completel |
| 232 | `ConsoleBufferSize` | S32 | 非GUI | LIVE |  | Size of chat console transcript (lines of chat) |
| 233 | `ConsoleMaxLines` | S32 | GUI | LIVE |  | Max number of lines of chat text visible in console. |
| 234 | `ContactsTornOff` | Boolean | 非GUI | LIVE |  | Show contacts window separately from Communicate window. |
| 235 | `ConversationSortOrder` | U32 | 非GUI | LIVE |  | Specifies sort key for conversations |
| 236 | `CookiesEnabled` | Boolean | 非GUI | LIVE |  | Accept cookies from Web sites? |
| 237 | `CoroutineStackSize` | S32 | 非GUI | LIVE |  | Size (in bytes) for each coroutine stack |
| 238 | `CrashHostUrl` | String | 非GUI | LIVE |  | A URL pointing to a crash report handler; overrides cluster negotiatio |
| 239 | `CrashOnStartup` | Boolean | 非GUI | LIVE |  | User-requested crash on viewer startup |
| 240 | `CrashSettingsFile` | String | 非GUI | LIVE |  | Crash settings file name (per install). |
| 241 | `CreateToolCopyCenters` | Boolean | GUI | LIVE |  |  |
| 242 | `CreateToolCopyRotates` | Boolean | GUI | LIVE |  |  |
| 243 | `CreateToolCopySelection` | Boolean | GUI | LIVE |  |  |
| 244 | `CreateToolKeepSelected` | Boolean | GUI | LIVE |  | After using create tool, keep the create tool active |
| 245 | `CurlRequestTimeOut` | F32 | 非GUI | DEAD |  | Max idle time of a curl request before killed (requires restart) |
| 246 | `CurrentGrid` | String | 非GUI | LIVE |  | Currently Selected Grid |
| 247 | `CurrentMapServerURL` | String | 非GUI | LIVE |  | Current Session World map URL |
| 248 | `CurrentlyUsingBakesOnMesh` | Boolean | 非GUI | LIVE |  | Are we currently on a grid that uses bakes on mesh? Persisted to force |
| 249 | `Cursor3D` | Boolean | GUI | LIVE |  | Treat Joystick values as absolute positions (not deltas). |
| 250 | `DAEExportConsolidateMaterials` | Boolean | GUI | LIVE |  | Combine faces with same texture |
| 251 | `DAEExportSingleUVMap` | Boolean | GUI | LIVE |  | set all objects to have the same UV map name |
| 252 | `DAEExportSkipTransparent` | Boolean | GUI | LIVE |  | Skip exporting faces with default transparent texture or full transpar |
| 253 | `DAEExportTextureParams` | Boolean | GUI | LIVE |  | Apply texture params suchs as repeats to the exported UV map |
| 254 | `DAEExportTextures` | Boolean | GUI | LIVE |  | Export textures when exporting Collada |
| 255 | `DAEExportTexturesFormat` | S32 | GUI | LIVE |  | Image file format to use when exporting Collada |
| 256 | `DayCycleName` | String | 非GUI | LIVE |  | Day cycle to use. May be superseded by region settings. |
| 257 | `DebugAnimatedObjects` | Boolean | 非GUI | LIVE |  | Show info related to animated objects |
| 258 | `DebugAvatarAppearanceMessage` | Boolean | 非GUI | LIVE |  | Dump a bunch of XML files when handling appearance messages |
| 259 | `DebugAvatarAppearanceServiceURLOverride` | String | 非GUI | LIVE |  | URL to use for baked texture requests; overrides value returned by log |
| 260 | `DebugAvatarCompositeBaked` | Boolean | 非GUI | LIVE |  | Colorize avatar meshes based on baked/composite state. |
| 261 | `DebugAvatarExperimentalServerAppearanceUpdate` | Boolean | 非GUI | LIVE |  | Experiment with sending full cof_contents instead of cof_version |
| 262 | `DebugAvatarJoints` | String | 非GUI | LIVE |  | List of joints to emit additional debugging info about. |
| 263 | `DebugAvatarLocalTexLoadedTime` | Boolean | 非GUI | LIVE |  | Display time for loading avatar local textures. |
| 264 | `DebugAvatarRezTime` | Boolean | 非GUI | LIVE |  | Display times for avatars to resolve. |
| 265 | `DebugBeaconLineWidth` | S32 | GUI | LIVE |  | Size of lines for Debug Beacons |
| 266 | `DebugForceAppearanceRequestFailure` | Boolean | 非GUI | LIVE |  | Request wrong cof version to test the failure path for server appearan |
| 267 | `DebugHideEmptySystemFolders` | Boolean | GUI | LIVE |  | Hide empty system folders when on |
| 268 | `DebugLookAtShowNames` | U32 | GUI | LIVE |  | Show names with DebugLookAt. 0) None, 1) "Display Name (user.name)", 2 |
| 269 | `DebugObjectLODs` | Boolean | 非GUI | LIVE |  | Show info related to lod calculations for attached or animated objects |
| 270 | `DebugPermissions` | Boolean | GUI(menu) | LIVE |  | Log permissions for selected inventory items |
| 271 | `DebugPluginDisableTimeout` | Boolean | 非GUI | LIVE |  | Disable the code which watches for plugins that are crashed or hung |
| 272 | `DebugQualityPerformance` | U32 | 非GUI | LIVE |  | Allows to change performance quality directly from debug settings. |
| 273 | `DebugRenderHitboxes` | Boolean | GUI(menu) | LIVE |  | Renders the avatars' hitboxes (collision areas) which are unaffected b |
| 274 | `DebugSearch` | Boolean | GUI | LIVE |  | If TRUE use search url as given in SearchURLDebug |
| 275 | `DebugSelectionLODs` | S32 | 非GUI | LIVE |  | Force selection to show specific LOD, -1 for off, 0 - lowest, 4 - high |
| 276 | `DebugSession` | Boolean | 非GUI | LIVE |  | Request debugging for a particular viewer session |
| 277 | `DebugSettingsHideDefault` | Boolean | GUI | LIVE |  | Show non-default settings only in Debug Settings list |
| 278 | `DebugShowAvatarRenderInfo` | Boolean | GUI(menu) | LIVE |  | Show avatar render cost information |
| 279 | `DebugShowColor` | S32 | GUI | LIVE |  | Show color under cursor |
| 280 | `DebugShowMemory` | Boolean | GUI(menu) | LIVE |  | Show Total Allocated Memory |
| 281 | `DebugShowRenderInfo` | Boolean | GUI(menu) | LIVE |  | Show stats about current scene |
| 282 | `DebugShowRenderMatrices` | Boolean | GUI(menu) | LIVE |  | Display values of current view and projection matrices. |
| 283 | `DebugShowTextureInfo` | Boolean | GUI(menu) | LIVE |  | Show interested texture info |
| 284 | `DebugShowTime` | Boolean | GUI(menu) | LIVE |  | Show time info |
| 285 | `DebugShowXUINames` | Boolean | 非GUI | LIVE |  | Show tooltips with XUI path to widget |
| 286 | `DebugStatModeActualIn` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 287 | `DebugStatModeActualOut` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 288 | `DebugStatModeAgentUpdatesSec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 289 | `DebugStatModeAsset` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 290 | `DebugStatModeBandwidth` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 291 | `DebugStatModeBoundMem` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 292 | `DebugStatModeCachedObjs` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 293 | `DebugStatModeChildAgents` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 294 | `DebugStatModeFPS` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 295 | `DebugStatModeFormattedMem` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 296 | `DebugStatModeGLMem` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 297 | `DebugStatModeKTrisDrawnFr` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 298 | `DebugStatModeKTrisDrawnSec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 299 | `DebugStatModeLayers` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 300 | `DebugStatModeLowLODObjects` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 301 | `DebugStatModeMainAgents` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 302 | `DebugStatModeMaterials` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 303 | `DebugStatModeMemoryAllocated` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 304 | `DebugStatModeNewObjs` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 305 | `DebugStatModeObjOccluded` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 306 | `DebugStatModeObjUnoccluded` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 307 | `DebugStatModeObjects` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 308 | `DebugStatModeOcclusionQueries` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 309 | `DebugStatModePTBandwidth` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 310 | `DebugStatModePTFPS` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 311 | `DebugStatModePTKTrisDrawnFr` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 312 | `DebugStatModePTKTrisDrawnSec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 313 | `DebugStatModePTNewObjs` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 314 | `DebugStatModePTTextureCount` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 315 | `DebugStatModePTTotalObjs` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 316 | `DebugStatModePacketLoss` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 317 | `DebugStatModePacketsIn` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 318 | `DebugStatModePacketsOut` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 319 | `DebugStatModePhysicsFPS` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 320 | `DebugStatModePingSim` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 321 | `DebugStatModePinnedObjects` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 322 | `DebugStatModeRawCount` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 323 | `DebugStatModeRawMem` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 324 | `DebugStatModeSimActiveObjects` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 325 | `DebugStatModeSimActiveScripts` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 326 | `DebugStatModeSimAgentMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 327 | `DebugStatModeSimFPS` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 328 | `DebugStatModeSimFrameMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 329 | `DebugStatModeSimImagesMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 330 | `DebugStatModeSimInPPS` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 331 | `DebugStatModeSimNetMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 332 | `DebugStatModeSimObjects` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 333 | `DebugStatModeSimOutPPS` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 334 | `DebugStatModeSimPCTScriptsRun` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 335 | `DebugStatModeSimPendingDownloads` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 336 | `DebugStatModeSimPendingUploads` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 337 | `DebugStatModeSimPumpIOMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 338 | `DebugStatModeSimScriptEvents` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 339 | `DebugStatModeSimScriptMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 340 | `DebugStatModeSimSimAIStepMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 341 | `DebugStatModeSimSimOtherMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 342 | `DebugStatModeSimSimPCTSteppedCharacters` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 343 | `DebugStatModeSimSimPhysicsMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 344 | `DebugStatModeSimSimPhysicsOtherMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 345 | `DebugStatModeSimSimPhysicsShapeUpdateMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 346 | `DebugStatModeSimSimPhysicsStepMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 347 | `DebugStatModeSimSimSkippedSilhouettSteps` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 348 | `DebugStatModeSimSleepMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 349 | `DebugStatModeSimSpareMsec` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 350 | `DebugStatModeSimTotalUnackedBytes` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 351 | `DebugStatModeTexture` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 352 | `DebugStatModeTextureCount` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 353 | `DebugStatModeTimeDialation` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 354 | `DebugStatModeTotalObjs` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 355 | `DebugStatObjCacheMiss` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 356 | `DebugStatTextureCacheHits` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 357 | `DebugStatTextureCacheReadLatency` | S32 | GUI | LIVE(UI framework) |  | Mode of stat in Statistics floater |
| 358 | `DebugViews` | Boolean | GUI(menu) | LIVE |  | Display debugging info for views. |
| 359 | `DebugWindowProc` | Boolean | GUI(menu) | LIVE |  | Log windows messages |
| 360 | `DeepLTranslateAPIKey` | LLSD | 非GUI | LIVE |  | DeepL Translation service data to use with the DeepL Translator API |
| 361 | `DefaultFemaleAvatar` | String | 非GUI | LIVE |  | Default Female Avatar |
| 362 | `DefaultLoginLocation` | String | 非GUI | LIVE |  | Startup destination default (if not specified on command line) |
| 363 | `DefaultMaleAvatar` | String | 非GUI | LIVE |  | Default Male Avatar |
| 364 | `DefaultUploadPermissionsConverted` | Boolean | 非GUI | LIVE |  | Default upload permissions have been converted to default creation per |
| 365 | `DeferProfilingUntilConnected` | Boolean | GUI(menu) | LIVE |  | Enable profiling data collection only after server connects. Lowers me |
| 366 | `DestinationGuideURL` | String | 非GUI | LIVE |  | Destination guide contents |
| 367 | `DialogStackIconVisible` | Boolean | GUI | LIVE |  | Internal, volatile control that defines if the dialog stack browser ic |
| 368 | `DisableAllRenderFeatures` | Boolean | 非GUI | LIVE |  | Disables all rendering features. |
| 369 | `DisableAllRenderTypes` | Boolean | 非GUI | LIVE |  | Disables all rendering types. |
| 370 | `DisableCameraConstraints` | Boolean | GUI | LIVE |  | Disable the normal bounds put on the camera by avatar position |
| 371 | `DisableCameraJoystickCenterReset` | Boolean | GUI | LIVE |  | Disable center reset on camera joysticks (bullseye) in camera controls |
| 372 | `DisableCrashLogger` | Boolean | 非GUI | LIVE |  | Do not send crash report to Linden server |
| 373 | `DisableExternalBrowser` | Boolean | 非GUI | LIVE |  | Disable opening an external browser. |
| 374 | `DisableLookAtAnimation` | Boolean | GUI(menu) | LIVE |  | Avatar follows cursor with avatars eyes, when disabled, avatar will lo |
| 375 | `DisableTextHyperlinkActions` | Boolean | 非GUI | LIVE |  | Disable highlighting and linking of URLs in XUI text boxes |
| 376 | `DiskCacheDirName` | String | 非GUI | LIVE |  | The name of the disk cache (within the standard Viewer disk cache dire |
| 377 | `DiskCachePercentOfTotal` | F32 | 非GUI | LIVE |  | The percent of total cache size (defined by CacheSize) to use for the  |
| 378 | `DiskCacheVersion` | S32 | 非GUI | LIVE |  | Version number of disk cache |
| 379 | `DisplayTimecode` | Boolean | 非GUI | LIVE |  | Display time code on screen |
| 380 | `Disregard128DefaultDrawDistance` | Boolean | 非GUI | LIVE |  | Whether to use the auto default to 128 draw distance |
| 381 | `Disregard96DefaultDrawDistance` | Boolean | 非GUI | LIVE |  | Whether to use the auto default to 96 draw distance |
| 382 | `DoubleClickAutoPilot` | Boolean | 非GUI | DEAD |  | (Obsolete)Enable double-click auto pilot |
| 383 | `DoubleClickShowWorldMap` | Boolean | 非GUI | LIVE |  | Enable double-click to show world map from mini map |
| 384 | `DoubleClickTeleport` | Boolean | 非GUI | LIVE |  | Enable double-click to teleport where allowed (afects minimap and peop |
| 385 | `DynamicCameraStrength` | F32 | GUI | LIVE |  | Amount camera lags behind avatar motion (0 = none, 30 = avatar velocit |
| 386 | `EditCameraMovement` | Boolean | GUI | LIVE |  | When entering build mode, camera moves up above avatar |
| 387 | `EditLinkedParts` | Boolean | GUI | LIVE |  | Select individual parts of linked objects |
| 388 | `EffectScriptChatParticles` | Boolean | GUI | LIVE |  | 1 = normal behavior, 0 = disable display of swirling lights when scrip |
| 389 | `EmbeddedLandmarkCopyToInventory` | Boolean | 非GUI | LIVE |  | Copies an embedded landmark to inventory before previewing it |
| 390 | `EmbeddedTextureStealsFocus` | Boolean | 非GUI | LIVE |  | Embedded texture preview will receive focus when opened |
| 391 | `EmotesUseItalic` | Boolean | GUI | LIVE |  | Chat emotes are emphasized by using italic font style. |
| 392 | `EmulateCoreCount` | U32 | 非GUI | LIVE |  | For debugging -- number of cores to restrict the main process to, or 0 |
| 393 | `EnableAltZoom` | Boolean | 非GUI | LIVE |  | Use Alt+mouse to look at and zoom in on objects |
| 394 | `EnableAppearance` | Boolean | 非GUI | LIVE |  | Enable opening appearance from web link |
| 395 | `EnableAvatarPay` | Boolean | 非GUI | LIVE |  | Enable paying other avatars from web link |
| 396 | `EnableAvatarShare` | Boolean | 非GUI | LIVE |  | Enable sharing from web link |
| 397 | `EnableButtonFlashing` | Boolean | 非GUI | LIVE |  | Allow UI to flash buttons to get your attention |
| 398 | `EnableClassifieds` | Boolean | 非GUI | LIVE |  | Enable creation of new classified ads from web link |
| 399 | `EnableCollisionSounds` | Boolean | GUI | LIVE |  | Play sounds on collision |
| 400 | `EnableDiscord` | Boolean | 非GUI | LIVE |  | When set, connect to Discord to enable Rich Presence (UNUSED) |
| 401 | `EnableDiskCacheDebugInfo` | Boolean | 非GUI | LIVE |  | When set, display additional cache debugging information |
| 402 | `EnableGestureSounds` | Boolean | GUI | LIVE |  | Play sounds from gestures |
| 403 | `EnableGrab` | Boolean | GUI | LIVE |  | Use Ctrl+mouse to grab and manipulate objects |
| 404 | `EnableGroupChatPopups` | Boolean | GUI | LIVE |  | Enable Incoming Group Chat Popups |
| 405 | `EnableIMChatPopups` | Boolean | GUI | LIVE |  | Enable Incoming IM Chat Popups |
| 406 | `EnableInventory` | Boolean | 非GUI | LIVE |  | Enable opening inventory from web link |
| 407 | `EnableLookAtTarget` | Boolean | 非GUI | LIVE |  | Whether or not to animate the avatar head and send look at targets whe |
| 408 | `EnableMouselook` | Boolean | GUI | LIVE |  | Allow first person perspective and mouse control of camera |
| 409 | `EnablePlaceProfile` | Boolean | 非GUI | LIVE |  | Enable viewing of place profile from web link |
| 410 | `EnableSelectionHints` | Boolean | 非GUI | LIVE |  | Whether or not to send editing hints to animate the arm when editing a |
| 411 | `EnableUIHints` | Boolean | GUI | LIVE |  | Toggles UI hint popups |
| 412 | `EnableVisualLeakDetector` | Boolean | 非GUI | DEAD |  | EnableVisualLeakDetector |
| 413 | `EnableVoiceChat` | Boolean | GUI | LIVE |  | Enable talking to other residents with a microphone |
| 414 | `EnvironmentPersistAcrossLogin` | Boolean | GUI | LIVE |  | Keep Environment settings consistent across sessions |
| 415 | `EventURL` | String | 非GUI | LIVE |  | URL for Event website, displayed in the event floater |
| 416 | `EveryoneCopy` | Boolean | 非GUI | LIVE |  | (obsolete) Everyone can copy the newly created objects |
| 417 | `ExodusLookAtLines` | Boolean | GUI | LIVE |  | Render lines for LookAt focus points. |
| 418 | `ExodusMouselookIFF` | Boolean | GUI | LIVE |  | Draw tracking markers in mouselook for people in range if combat featu |
| 419 | `ExodusMouselookIFFRange` | F32 | GUI | LIVE |  | Draw tracking markers in mouselook for people in range if combat featu |
| 420 | `ExodusMouselookTextHAlign` | U32 | 非GUI | LIVE |  | Text alignment for target information text in mouselook if combat feat |
| 421 | `ExodusMouselookTextOffsetX` | F32 | 非GUI | LIVE |  | Text X offset for target information text in mouselook if combat featu |
| 422 | `ExodusMouselookTextOffsetY` | F32 | 非GUI | LIVE |  | Text Y offset for target information text in mouselook if combat featu |
| 423 | `ExternalEditor` | String | GUI | LIVE |  | Path to program used to edit LSL scripts and XUI files, e.g.: /usr/bin |
| 424 | `ExternalEditorConvertTabsToSpaces` | Boolean | GUI | LIVE |  | If enabled, the script sent to the viewer from an external editor will |
| 425 | `FMODProfilerEnable` | Boolean | 非GUI | LIVE |  | Enable profiler tool if using FMOD Studio |
| 426 | `FMODResampleMethod` | U32 | 非GUI | LIVE |  | Sets the method used for internal resampler 0(Linear), 1(Cubic), 2(Spl |
| 427 | `FSAdvancedTooltips` | Boolean | GUI | LIVE |  | Show extended information in hovertips about objects (classic Phoenix  |
| 428 | `FSAdvancedWorldmapRegionInfo` | Boolean | GUI | LIVE |  | Shows additional region infos on the world map (agent count and maturi |
| 429 | `FSAllowDoubleClickOnScriptedObjects` | Boolean | GUI | LIVE |  | If enabled, allows double-click movement action (walk/teleport) to scr |
| 430 | `FSAllowEEPWaterDerender` | Boolean | 非GUI | LIVE |  | Allow EEP assets to disable water rendering to increase FPS |
| 431 | `FSAlwaysFly` | Boolean | GUI(menu) | LIVE |  | Fly Override, for no-fly zones. Must be activated each time. |
| 432 | `FSAlwaysShowInboxButton` | Boolean | GUI | LIVE |  | If enabled, the Received Items folder aka Inbox is always shown at the |
| 433 | `FSAlwaysShowTPCancel` | Boolean | GUI | LIVE |  | Always show the TP cancel button even if the sim says it cant be cance |
| 434 | `FSAlwaysTrackPayments` | Boolean | GUI | LIVE |  | Always track payments even if the money tracker is closed |
| 435 | `FSAnimatedScriptDialogs` | Boolean | GUI | LIVE |  | Animates script dialogs V1 style. Only effective when dialogs in top r |
| 436 | `FSAnimationPreviewExpanded` | Boolean | GUI | LIVE |  | Expand or collapse the advanced animation information in the animation |
| 437 | `FSAnnounceIncomingIM` | Boolean | GUI | LIVE |  | Opens the IM floater and announces an incoming IM as soon as somebody  |
| 438 | `FSAppearanceShowHints` | Boolean | GUI | LIVE |  | If enabled, show visual hints (avatar images) in appearance editor |
| 439 | `FSAreaSearchAdvanced` | Boolean | 非GUI | LIVE |  | Displays the advanced settings tab |
| 440 | `FSAreaSearchColumnConfig` | U32 | 非GUI | LIVE |  | Stores the column visibility for the area search |
| 441 | `FSAreaSearch_ClickAction` | LLSD | 非GUI | LIVE |  | Area Search Filter: Mouse click action |
| 442 | `FSAreaSearch_ExcludeAttachments` | Boolean | 非GUI | LIVE |  | Area Search Filter: Exclude attachments |
| 443 | `FSAreaSearch_ExcludeChildPrims` | Boolean | 非GUI | LIVE |  | Area Search Filter: Exclude child prims |
| 444 | `FSAreaSearch_ExcludeNeighborRegions` | Boolean | 非GUI | LIVE |  | Area Search Filter: Exclude neighbor regions |
| 445 | `FSAreaSearch_ExcludePhysical` | Boolean | 非GUI | LIVE |  | Area Search Filter: Exclude physical objects |
| 446 | `FSAreaSearch_ExcludeReflectionProbes` | Boolean | 非GUI | LIVE |  | Area Search Filter: Exclude reflection probes |
| 447 | `FSAreaSearch_ExcludeTemporary` | Boolean | 非GUI | LIVE |  | Area Search Filter: Exclude temporary objects |
| 448 | `FSAreaSearch_FilterDistance` | Boolean | 非GUI | LIVE |  | Area Search Filter: Filter by distance |
| 449 | `FSAreaSearch_FilterForSale` | Boolean | 非GUI | LIVE |  | Area Search Filter: Only show objects that are for sale |
| 450 | `FSAreaSearch_MaximumDistance` | S32 | 非GUI | LIVE |  | Area Search Filter: Maximum distance |
| 451 | `FSAreaSearch_MaximumPrice` | S32 | 非GUI | LIVE |  | Area Search Filter: Maximum price |
| 452 | `FSAreaSearch_MinimumDistance` | S32 | 非GUI | LIVE |  | Area Search Filter: Minimum distance |
| 453 | `FSAreaSearch_MinimumPrice` | S32 | 非GUI | LIVE |  | Area Search Filter: Minimum price |
| 454 | `FSAreaSearch_OnlyAttachments` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only attachments |
| 455 | `FSAreaSearch_OnlyCopiable` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only copiable objects |
| 456 | `FSAreaSearch_OnlyCurrentParcel` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only objects in the current parcel |
| 457 | `FSAreaSearch_OnlyLocked` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only locked objects |
| 458 | `FSAreaSearch_OnlyMOAP` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only objects that have shared media applied |
| 459 | `FSAreaSearch_OnlyModifiable` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only  modifiable objects |
| 460 | `FSAreaSearch_OnlyPhantom` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only phantom objects |
| 461 | `FSAreaSearch_OnlyPhysical` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only physical objects |
| 462 | `FSAreaSearch_OnlyReflectionProbes` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only reflection probes |
| 463 | `FSAreaSearch_OnlyTemporary` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only temporary objects |
| 464 | `FSAreaSearch_OnlyTransferable` | Boolean | 非GUI | LIVE |  | Area Search Filter: Find only transferable objects |
| 465 | `FSAudioMusicFadeIn` | F32 | GUI | LIVE |  | Fade in time in seconds for music streams |
| 466 | `FSAudioMusicFadeOut` | F32 | GUI | LIVE |  | Fade out time in seconds for music streams |
| 467 | `FSAutoOrderIMTabs` | Boolean | GUI | LIVE |  | Use automatic tab ordering for IM tabs inside the conversations floate |
| 468 | `FSAutoOrderIMTabsAtTop` | Boolean | GUI | LIVE |  | Add new IM tabs as first in their chat type group. |
| 469 | `FSAutoOrderIMTabsPriorities` | String | GUI | LIVE |  | Priority for automatic ordering IM tabs. Higher priority means the tab |
| 470 | `FSAutoUnmuteAmbient` | Boolean | GUI | LIVE |  | If Ambient sounds are muted, unmute on TP. Default (false) |
| 471 | `FSAutoUnmuteSounds` | Boolean | GUI | LIVE |  | If Sound Effects are muted, unmute on TP. Default (false) |
| 472 | `FSAvatarTurnSpeed` | F32 | GUI | LIVE |  | Modify the speed at which the avatar responds to turning movements. 0- |
| 473 | `FSBeamColorFile` | String | GUI | LIVE |  | Beam file for the shape of your beam |
| 474 | `FSBeamShape` | String | GUI | LIVE |  | Beam file for the shape of your beam |
| 475 | `FSBeamShapeScale` | F32 | GUI | LIVE |  | How Big You Want to let the beam be |
| 476 | `FSBetterGroupNoticesToIMLog` | Boolean | GUI | LIVE |  | Improved logging of group notices to group IM log. |
| 477 | `FSBeyondNearbyChatColorDiminishFactor` | F32 | GUI | LIVE |  | The factor the color for nearby chat diminishes if the sender is beyon |
| 478 | `FSBlockClickSit` | Boolean | GUI | LIVE |  | Prevents sitting on left-click. |
| 479 | `FSBrowserHomePage` | String | GUI | LIVE |  | Home page for the built in web browser |
| 480 | `FSBubblesHideConsoleAndToasts` | Boolean | GUI | LIVE |  | If enabled, using bubble chat will hide the chat output in the Nearby  |
| 481 | `FSBuildPrefs_ActualRoot` | Boolean | GUI | LIVE |  | Show the axis on the actual root of a linkset instead of mass center |
| 482 | `FSBuildPrefs_Alpha` | F32 | GUI | LIVE |  | New object created default alpha |
| 483 | `FSBuildPrefs_Color` | Color4 | GUI | LIVE |  | New object created default color |
| 484 | `FSBuildPrefs_FullBright` | Boolean | GUI | LIVE |  | New object created default fullbright |
| 485 | `FSBuildPrefs_Glow` | F32 | GUI | LIVE |  | New object created default glow |
| 486 | `FSBuildPrefs_Material` | String | GUI | LIVE |  | Default Setting For New Objects to be created, physical flag |
| 487 | `FSBuildPrefs_Phantom` | Boolean | GUI | LIVE |  | New object created default of phantom |
| 488 | `FSBuildPrefs_Physical` | Boolean | GUI | LIVE |  | New object created default of physical |
| 489 | `FSBuildPrefs_PivotIsPercent` | Boolean | GUI | LIVE |  | Consider the Pivot points values as a percentage |
| 490 | `FSBuildPrefs_PivotX` | F32 | GUI | LIVE |  | Pivot point on the X axis |
| 491 | `FSBuildPrefs_PivotY` | F32 | GUI | LIVE |  | Pivot point on the Y axis |
| 492 | `FSBuildPrefs_PivotZ` | F32 | GUI | LIVE |  | Pivot point on the Z axis |
| 493 | `FSBuildPrefs_Shiny` | String | GUI | LIVE |  | New object created default shiny |
| 494 | `FSBuildPrefs_Temporary` | Boolean | GUI | LIVE |  | New object created default of temporary |
| 495 | `FSBuildPrefs_Xsize` | F32 | GUI | LIVE |  | Default Size For New Objects to be created X |
| 496 | `FSBuildPrefs_Ysize` | F32 | GUI | LIVE |  | Default Size For New Objects to be created Y |
| 497 | `FSBuildPrefs_Zsize` | F32 | GUI | LIVE |  | Default Size For New Objects to be created Z |
| 498 | `FSBuildToolDecimalPrecision` | S32 | GUI | LIVE |  | Decimal digits to display on various build tool controls (0-7) |
| 499 | `FSChatHistoryShowYou` | Boolean | GUI | LIVE |  | Show localized "You" instead of your avatar's username (like CHUI) |
| 500 | `FSChatHumanObjectTabs` | Boolean | GUI | LIVE |  | AYAstorm r22: split Nearby Chat / IM history into Human and System & O |
| 501 | `FSChatWindow` | S32 | GUI | LIVE |  | Show chat in multiple windows(by default) or in one multi-tabbed windo |
| 502 | `FSChatbarGestureAutoCompleteEnable` | Boolean | GUI | LIVE |  | Toggles gesture auto complete in chat bar |
| 503 | `FSChatbarNamePrediction` | Boolean | GUI | LIVE |  | Toggles name prediction in nearby chat |
| 504 | `FSClientTagsVisibility` | U32 | GUI | LIVE |  | Show client tags: 0=Client tags Off, 1=That are on the TPVD (needs FSU |
| 505 | `FSCloseChatOnReturnInMouselook` | Boolean | GUI | LIVE |  | When CloseChatOnReturn is enabled, only deselect nearby chat in mousel |
| 506 | `FSCloseChatOnReturnOnlyForNearbyChatControl` | Boolean | GUI | DEAD |  | When CloseChatOnReturn is enabled, only deselect nearby chat bar |
| 507 | `FSCmdLine` | Boolean | GUI | LIVE |  | Enable usage of chat bar as a command line |
| 508 | `FSCmdLineAO` | String | GUI | LIVE |  | Turn AO on/off |
| 509 | `FSCmdLineBandWidth` | String | GUI | LIVE |  | Change max. bandwidth quickly |
| 510 | `FSCmdLineCalc` | String | GUI | LIVE |  | Calculates an expression |
| 511 | `FSCmdLineClearChat` | String | GUI | LIVE |  | Clear chat transcript to stop lag from chat spam |
| 512 | `FSCmdLineCopyCam` | String | GUI | LIVE |  | Copies the current camera location to a vector of the form <x, y, z> t |
| 513 | `FSCmdLineDrawDistance` | String | GUI | LIVE |  | Change draw distance quickly |
| 514 | `FSCmdLineGround` | String | GUI | LIVE |  | Teleport to ground function command |
| 515 | `FSCmdLineHeight` | String | GUI | LIVE |  | Teleport to height function command |
| 516 | `FSCmdLineKeyToName` | String | GUI | LIVE |  | Use a fast key to name query |
| 517 | `FSCmdLineMapTo` | String | GUI | LIVE |  | Teleport to a region by name rapidly |
| 518 | `FSCmdLineMapToKeepPos` | Boolean | GUI | LIVE |  | Whether to use current local position on teleport to the new region |
| 519 | `FSCmdLineMedia` | String | GUI | LIVE |  | Chat command for setting a media url |
| 520 | `FSCmdLineMusic` | String | GUI | LIVE |  | Chat command for setting a music url |
| 521 | `FSCmdLineOfferTp` | String | GUI | LIVE |  | Offer a teleport to target avatar |
| 522 | `FSCmdLinePlatformSize` | F32 | GUI | LIVE |  | How wide the rezzed platform will appear to be. |
| 523 | `FSCmdLinePos` | String | GUI | LIVE |  | Teleport to position function command |
| 524 | `FSCmdLineRezPlatform` | String | GUI | LIVE |  | Rez a platform underneath you |
| 525 | `FSCmdLineRollDice` | String | GUI | LIVE |  | Rolls dice - cmd [number of dice] [number of faces]. Example: cmd 1 20 |
| 526 | `FSCmdLineTP2` | String | GUI | LIVE |  | Teleport to a person by name, partials work. |
| 527 | `FSCmdLineTeleportHome` | String | GUI | LIVE |  | Teleport to home function command |
| 528 | `FSCmdTeleportToCam` | String | GUI | LIVE |  | Teleport to your camera |
| 529 | `FSCollisionMessagesInChat` | Boolean | GUI | LIVE |  | Shows collision messages in nearby chat. |
| 530 | `FSColorClienttags` | U32 | GUI | LIVE |  | Color Client tags by: 0=Off, 1=Single color per Viewer, 2=User defined |
| 531 | `FSColorIMsDistinctly` | Boolean | GUI | LIVE |  | Color IM/Group messages distinctly in the console. |
| 532 | `FSColorUsername` | Boolean | GUI | LIVE |  | Color username distinctly from the rest of the tag |
| 533 | `FSComboboxSubstringSearch` | Boolean | 非GUI | LIVE |  | Allows fulltext search on comboboxes |
| 534 | `FSCommitForSaleOnChange` | Boolean | GUI | LIVE |  | Enables old SL default behavior. Objects set for sale will take effect |
| 535 | `FSConfirmPayments` | Boolean | GUI | LIVE |  | Enables confirmation dialogs for payments. |
| 536 | `FSConsoleClassicDrawMode` | Boolean | GUI | LIVE |  | Enables classic console draw mode (single background block over all li |
| 537 | `FSContactListShowSearch` | Boolean | GUI | LIVE |  | Shows the search filter in the legacy contact list. |
| 538 | `FSContactSetsColorizeChat` | Boolean | GUI | LIVE |  | Whether to color a friends chat based on their friends groups |
| 539 | `FSContactSetsColorizeFriends` | Boolean | GUI | LIVE |  | Whether to color a friends list entry based on their friends groups |
| 540 | `FSContactSetsColorizeMiniMap` | Boolean | GUI | LIVE |  | Whether to color a friends mini map icon based on their friends groups |
| 541 | `FSContactSetsColorizeNameTag` | Boolean | GUI | LIVE |  | Whether to color a friends name tag based on their friends groups |
| 542 | `FSContactSetsColorizeRadar` | Boolean | GUI | LIVE |  | Whether to color a friends name in the radar list based on their frien |
| 543 | `FSContactSetsNotificationNearbyChat` | Boolean | GUI | LIVE |  | Show the On/Offline notifications caused by Contactsets in Nearby Chat |
| 544 | `FSContactSetsNotificationToast` | Boolean | GUI | LIVE |  | Show the On/Offline notifications caused by Contactsets as Toast messa |
| 545 | `FSContactsSortOrder` | U32 | 非GUI | DEAD |  | Specifies sort order for friends (0 = by display name, 1 = username, 2 |
| 546 | `FSConversationLogLifetime` | U32 | 非GUI | LIVE |  | Number of days transcripts are preserved in the conversation log befor |
| 547 | `FSCopyObjKeySeparator` | String | 非GUI | LIVE |  | This chunk of text goes between keys when you use the Copy Key button  |
| 548 | `FSCreateCallingCards` | Boolean | 非GUI | LIVE |  | Don't create calling cards when friending other avatars (requires rest |
| 549 | `FSCreateGiveInventoryParticleEffect` | Boolean | GUI | LIVE |  | If enabled, the viewer will create particle effects around the avatar  |
| 550 | `FSCreateOctreeLog` | Boolean | 非GUI | LIVE |  | Create a log of octree operation. This can cause huge frame stalls on  |
| 551 | `FSDefaultObjectTexture` | String | GUI | LIVE |  | Default texture that will be applied to rezzed prims. (UUID texture re |
| 552 | `FSDestroyGLTexturesImmediately` | Boolean | 非GUI | DEAD |  | If enabled, GL textures will be removed from memory immediately when i |
| 553 | `FSDestroyGLTexturesThreshold` | F32 | 非GUI | DEAD |  | Threshold, at what texture memory load level GL textures will be remov |
| 554 | `FSDisableAvatarTrackerAtCloseIn` | Boolean | GUI | LIVE |  | Disables the tracking beacon if distance to target avatar is less than |
| 555 | `FSDisableBeaconAfterTeleport` | Boolean | GUI | LIVE |  | Disables the beacon of the teleport destination after a teleport. |
| 556 | `FSDisableBlockListAutoOpen` | Boolean | GUI | LIVE |  | Disables automatic opening of the block list when muting people or obj |
| 557 | `FSDisableIMChiclets` | Boolean | GUI | LIVE |  | If enabled, Firestorm will not show any group / IM chat chiclets (noti |
| 558 | `FSDisableLabeledChatLinks` | Boolean | GUI | LIVE |  | When true, do not treat wiki-style bracketed links ([https://host labe |
| 559 | `FSDisableLoginScreens` | Boolean | GUI | LIVE |  | Disable login screen progress bar |
| 560 | `FSDisableLogoutScreens` | Boolean | GUI | LIVE |  | Disable logout screen progress bar |
| 561 | `FSDisableMouseWheelCameraZoom` | Boolean | GUI | LIVE |  | If true, Firestorm will not use mouse wheel to zoom in/out the camera. |
| 562 | `FSDisableNeighbourRegionConnections` | Boolean | GUI | LIVE |  | Do not connect to neighbouring regions, only to the current region (li |
| 563 | `FSDisableReturnObjectNotification` | Boolean | GUI | LIVE |  | Disable 'Object has been returned to your inventory Lost and Found fol |
| 564 | `FSDisableRiggedMeshMatrixCaching` | Boolean | 非GUI | DEAD |  | Disable the caching of Rigged mesh matrix pallettes.Non-persistant. |
| 565 | `FSDisableTeleportScreens` | Boolean | GUI | LIVE |  | Disable teleport screens |
| 566 | `FSDisableTurningAroundWhenWalkingBackwards` | Boolean | GUI | LIVE |  | Disables your avatar turning around locally when moving backwards. |
| 567 | `FSDisableWMIProbing` | Boolean | GUI | LIVE |  | Disables VRAM detection via WMI probing on Windows systems |
| 568 | `FSDiskCacheHighWaterPercent` | F32 | 非GUI | LIVE |  | Trigger point above which we should start to clear out older cache ent |
| 569 | `FSDiskCacheLowWaterPercent` | F32 | 非GUI | LIVE |  | Level to drain cache to once it goes over the high water limit. |
| 570 | `FSDiskCacheSize` | U32 | GUI | LIVE |  | Controls amount of hard drive space reserved for local asset caching i |
| 571 | `FSDismissGroupNoticeAttachmentsToTrash` | Boolean | 非GUI | LIVE |  | If enabled, dismissing group notices with attachments will move attach |
| 572 | `FSDoNotHideMapOnTeleport` | Boolean | GUI | LIVE |  | If enabled, the world map won't be closed when teleporting |
| 573 | `FSDontIgnoreAdHocFromFriends` | Boolean | GUI | LIVE |  | Allow my friends to start conference chats with me. |
| 574 | `FSDontNagWhenPurging` | Boolean | 非GUI | LIVE |  | If enabled, emptying trash will not double check before deleting. WARN |
| 575 | `FSDoubleClickAddInventoryClothing` | Boolean | GUI(menu) | LIVE |  | Whether or not to add clothes instead of wearing them |
| 576 | `FSDoubleClickAddInventoryObjects` | Boolean | GUI | LIVE |  | Whether or not to add objects instead of wearing them |
| 577 | `FSDrawDistanceVRAMOptimization` | Boolean | GUI | LIVE |  | Enables a feature that reduces your draw distance when VRAM becomes fu |
| 578 | `FSEditGrid` | Boolean | GUI | LIVE(UI framework) |  | Allows editing a grid from the grid manager |
| 579 | `FSEmphasizeShoutWhisper` | Boolean | GUI | LIVE |  | Enables bolding shouted chat and italicizing whispered chat |
| 580 | `FSEnableAggressiveComplexityUpdates` | Boolean | 非GUI | LIVE |  | Enable active complexity calculations. This may have a significant det |
| 581 | `FSEnableEmojiWindowPopupWhileTyping` | Boolean | GUI | LIVE |  | Enables automatic opening of emoji chooser window while typing in chat |
| 582 | `FSEnableGrowl` | Boolean | GUI | LIVE |  | Enables Growl notifications |
| 583 | `FSEnableLogThrottle` | Boolean | 非GUI | LIVE |  | Enables throttling for writing to the log file to prevent spam |
| 584 | `FSEnableMovingFolderLinks` | Boolean | 非GUI | LIVE |  | Enable moving of folder links via drag and drop |
| 585 | `FSEnableObjectExports` | Boolean | GUI | LIVE |  | Enable object imports and exports (WARNING: This feature is unstable a |
| 586 | `FSEnablePerGroupSnoozeDuration` | Boolean | GUI | LIVE |  | Enables input of a snooze duration per group. |
| 587 | `FSEnableRightclickMenuInMouselook` | Boolean | GUI | LIVE |  | Enables pie or context menus on alt right click in mouselook |
| 588 | `FSEnableRightclickOnTransparentObjects` | Boolean | 非GUI | LIVE |  | If enabled, right-clicks on transparent objects will open the context  |
| 589 | `FSEnableVolumeControls` | Boolean | GUI | LIVE |  | If true, Firestorm will show volume controls (sounds, media, stream) i |
| 590 | `FSEnabledLanguages` | LLSD | 非GUI | LIVE |  | Languages that are enabled and can be used in this install. |
| 591 | `FSEnforceStrictObjectCheck` | Boolean | 非GUI | LIVE |  | Force malformed prims to be treated as invalid. This setting derenders |
| 592 | `FSEnvironmentManualTransitionTime` | F32 | GUI | LIVE |  | Timespan for blending between day/water/sky settings when manually cha |
| 593 | `FSEventPollCoreRetries` | U32 | 非GUI | LIVE |  | DEBUG: EventQueueGet llcorehttp retry count. 0 keeps current 7.2.3 beh |
| 594 | `FSExperimentalDragTexture` | Boolean | GUI | LIVE |  | If enabled, allows to click-drag or click-scale (together with caps lo |
| 595 | `FSExperimentalLostAttachmentsFix` | Boolean | 非GUI | LIVE |  | Enables the experimental fix for attachments getting detached on telep |
| 596 | `FSExperimentalLostAttachmentsFixKillDelay` | F32 | 非GUI | LIVE |  | Delay in seconds after a teleport for that kill object messages to det |
| 597 | `FSExperimentalLostAttachmentsFixReport` | Boolean | 非GUI | LIVE |  | If enabled, reports attachments that were attempted to get detached du |
| 598 | `FSExperimentalOutfitsReturn` | Boolean | 非GUI | LIVE |  | FIRE-36116 - debounce redundant refreshList calls |
| 599 | `FSExperimentalRegionCrossingMovementFix` | S32 | GUI | LIVE |  | Enables the experimental fix for region crossing movements being bogus |
| 600 | `FSExportContents` | Boolean | GUI | LIVE |  | Export object contents in linkset backups |
| 601 | `FSFadeAudioStream` | Boolean | GUI | LIVE |  | Use fading when changing the parcel audio stream. |
| 602 | `FSFadeGroupNotices` | Boolean | 非GUI | LIVE |  | Fade group notices. (V3 default: true) |
| 603 | `FSFilePickerOpenDirectory` | String | 非GUI | LIVE |  | The last used directory for opening a file |
| 604 | `FSFilePickerSaveDirectory` | String | 非GUI | LIVE |  | The last used directory for saving a file |
| 605 | `FSFilterGrowlKeywordDuplicateIMs` | Boolean | GUI | LIVE |  | Filters duplicate IMs in Growl if they have already been shown as part |
| 606 | `FSFirstRunAfterSettingsRestore` | Boolean | 非GUI | LIVE |  | Specifies that you have not run the viewer since you performed a setti |
| 607 | `FSFlashOnMessage` | Boolean | GUI | LIVE |  | Flash/Bounce the app icon when a new message is received and Firestorm |
| 608 | `FSFlashOnObjectIM` | Boolean | GUI | LIVE |  | Flash/Bounce the app icon when a new instant message from an object is |
| 609 | `FSFlashOnScriptDialog` | Boolean | GUI | LIVE |  | Flash/Bounce the app icon when a script dialog is received and Firesto |
| 610 | `FSFlyAfterTeleport` | Boolean | GUI | LIVE |  | Always fly after teleporting. |
| 611 | `FSFocusPointFollowsPointer` | Boolean | GUI | LIVE |  | Allows the Depth of Field focus to actively follow the mouse point |
| 612 | `FSFocusPointLocked` | Boolean | GUI(menu) | LIVE |  | Whether the focus point used for DoF is currently Locked in place |
| 613 | `FSFocusPointRender` | Boolean | GUI | LIVE |  | Draw a small crosshair where the DoF focus is |
| 614 | `FSFolderViewItemHeight` | S32 | GUI | LIVE |  | Controls the height of folder items, for instance in inventory |
| 615 | `FSFontChatLineSpacingPixels` | S32 | GUI | LIVE |  | Line spacing pixels for chat text (requires restart) |
| 616 | `FSFontSettingsFile` | String | GUI | LIVE |  | The font settings file with the font currently being used. |
| 617 | `FSFontSizeAdjustment` | F32 | GUI | LIVE |  | Number of points to add to the defualt font sizes |
| 618 | `FSForcedVideoMemory` | U32 | GUI | LIVE |  | Overrides the video memory detection on Windows if a value greater 0 i |
| 619 | `FSFriendListColumnShowDisplayName` | Boolean | GUI | LIVE |  | Enables the display name column in the legacy friend list. |
| 620 | `FSFriendListColumnShowFullName` | Boolean | GUI | LIVE |  | Enables the full name column in the legacy friend list. |
| 621 | `FSFriendListColumnShowPermissions` | Boolean | GUI | LIVE |  | If enabled, show permission columns in the contacts list |
| 622 | `FSFriendListColumnShowUserName` | Boolean | GUI | LIVE |  | Enables the username column in the legacy friend list. |
| 623 | `FSFriendListFullNameFormat` | S32 | GUI | LIVE |  | Defines the order of how the full name in the contacts list is shown ( |
| 624 | `FSFriendListSortOrder` | S32 | GUI | LIVE |  | Defines the sort order of the contacts list (0 = username, 1 = display |
| 625 | `FSFriendOnlineToHumanTab` | Boolean | GUI | LIVE |  | AYAstorm r22: also show friend online/offline notifications in the Hum |
| 626 | `FSGridBuilderURL` | String | 非GUI | LIVE |  | Fetch an html page of grids from this URL. |
| 627 | `FSGroupNoticesToIMLog` | Boolean | GUI | LIVE |  | Show group notices in group chats, in addition to toasts. |
| 628 | `FSGroupNotifyNoTransparency` | Boolean | GUI | LIVE |  | If true, group notices will be shown opaque and ignore the floater opa |
| 629 | `FSGrowlWhenActive` | Boolean | GUI | LIVE |  | If Growl notifications are active, show them even when the window is a |
| 630 | `FSHideHelpButtons` | Boolean | 非GUI | LIVE |  | When enabled, hides the help button from floaters (requires restart) |
| 631 | `FSHighlightGroupMods` | Boolean | GUI | LIVE |  | Enable group moderator message highlighting |
| 632 | `FSHudTextBackgroundOpacity` | F32 | GUI | LIVE |  | Opacity of floating text background (0.0 = completely transparent, 1.0 |
| 633 | `FSHudTextFadeDistance` | F32 | GUI | LIVE |  | Sets the distance where HUD text starts to fade |
| 634 | `FSHudTextFadeRange` | F32 | GUI | LIVE |  | Sets the range it takes for a HUD text to fade from fully visible to i |
| 635 | `FSHudTextShowBackground` | S32 | GUI | LIVE |  | Displays a black/white background behind the prim floating text to mak |
| 636 | `FSHudTextUseHoverHighlight` | Boolean | GUI | LIVE |  | When an object is being hovered over, highlight the HUD text by moving |
| 637 | `FSIMChatFlashOnFriendStatusChange` | Boolean | GUI | LIVE |  | Flash IM tab when friend goes online or offline. |
| 638 | `FSIMChatHistoryFade` | F32 | GUI | LIVE |  | Amount to fade IM text into the background of the chat transcript floa |
| 639 | `FSIMOpacity` | F32 | GUI | LIVE |  | Opacity of the IM floater |
| 640 | `FSIMSystemMessageBrackets` | Boolean | GUI | LIVE |  | Enables surrounding system messages with square brackets in chat trans |
| 641 | `FSIMTabNameFormat` | S32 | GUI | LIVE |  | Controls in what format the name on IM tabs will be shown: 0 = Display |
| 642 | `FSIgnoreAdHocSessions` | Boolean | GUI | LIVE |  | Automatically ignore and leave all conference (ad-hoc) chats. |
| 643 | `FSIgnoreClientsideMeshValidation` | Boolean | 非GUI | LIVE |  | Treat the errors from Clientside mesh validation as warnings and do no |
| 644 | `FSIgnoreFinishAnimation` | Boolean | GUI | LIVE |  | Disable the wait for pre-jump or landing. Credit to Zwagoth Klaar for  |
| 645 | `FSIgnoreObjectIM` | Boolean | GUI | LIVE |  | Silently discard instant messages from objects owned by other resident |
| 646 | `FSIgnoreSimulatorCameraConstraints` | Boolean | GUI | LIVE |  | Ignores the 'push' the simulator applies to your camera to keep it out |
| 647 | `FSImActiveOpacityOverride` | Boolean | GUI | LIVE |  | When enabled, uses the Active Opacity value when IM window is focused |
| 648 | `FSImageDecodeThreads` | U32 | 非GUI | LIVE |  | Amount of threads to use for image decoding. 0 = auto, >= 1 number of  |
| 649 | `FSImportBuildOffset` | Vector3 | 非GUI | LIVE |  | Distance from user the importer begins to build |
| 650 | `FSImpostorAvatarExclude` | U32 | GUI | LIVE |  | Allows for Animesh User or Control Avatars to be excluded from using i |
| 651 | `FSInspectAvatarSlurlOpensProfile` | Boolean | GUI | LIVE |  | Open the full profile of an avatar directly when clicking on its name |
| 652 | `FSInspectColumnConfig` | U32 | 非GUI | LIVE |  | Stores the column visibility of the inspect window |
| 653 | `FSInternalCanEditObjectFaces` | Boolean | GUI | LIVE |  | Internal control to show/hide object texture/material edit controls |
| 654 | `FSInternalFaceHasBPNormalMap` | Boolean | GUI | LIVE |  | Internal control to store a flag about the edited face having a Blinn- |
| 655 | `FSInternalFaceHasBPSpecularMap` | Boolean | GUI | LIVE |  | Internal control to store a flag about the edited face having a Blinn- |
| 656 | `FSInternalFontSettingsFile` | String | 非GUI | LIVE |  | The font settings file with the font currently being used in this sess |
| 657 | `FSInternalLegacyNotificationWell` | Boolean | 非GUI | LIVE |  | Internal state of FSLegacyNotificationWell |
| 658 | `FSInternalShowNavbarFavoritesPanel` | Boolean | GUI | LIVE |  | Internal control to show/hide navigation bar favorites panel |
| 659 | `FSInternalShowNavbarNavigationPanel` | Boolean | GUI | LIVE |  | Internal control to show/hide navigation bar navigation panel |
| 660 | `FSInternalSkinCurrent` | String | 非GUI | LIVE |  | The currently selected skin in the current session. |
| 661 | `FSInternalSkinCurrentTheme` | String | 非GUI | LIVE |  | The selected theme for the current skin in the current session. |
| 662 | `FSInventoryThumbnailTooltipsDelay` | F32 | GUI | LIVE |  | Sets the delay of the inventory item thumbnail tooltip |
| 663 | `FSKeepUnpackedCacheFiles` | Boolean | GUI | LIVE |  | If TRUE, the viewer won't delete unpacked cache files when logging out |
| 664 | `FSLandmarkCreatedNotification` | Boolean | GUI | LIVE |  | Display a notification if a landmark is added to your inventory. |
| 665 | `FSLargeOutfitsWarningInThisSession` | Boolean | 非GUI | LIVE |  | Internal; Suppresses the 'too many outfits' warning (does not persist  |
| 666 | `FSLastSearchTab` | S32 | 非GUI | LIVE |  | Last selected tab in search window |
| 667 | `FSLastSnapshotPanel` | String | 非GUI | LIVE |  | The last snapshot panel that was opened and will be restored the next  |
| 668 | `FSLastSnapshotToFacebookHeight` | S32 | 非GUI | DEAD |  | The height of the last Facebook snapshot, in px |
| 669 | `FSLastSnapshotToFacebookResolution` | S32 | 非GUI | DEAD |  | At what resolution should snapshots be posted on Facebook. 0=Current W |
| 670 | `FSLastSnapshotToFacebookWidth` | S32 | 非GUI | DEAD |  | The width of the last Facebook snapshot, in px |
| 671 | `FSLastSnapshotToFlickrHeight` | S32 | 非GUI | LIVE |  | The height of the last Flickr snapshot, in px |
| 672 | `FSLastSnapshotToFlickrResolution` | S32 | 非GUI | LIVE |  | At what resolution should snapshots be posted on Flickr. 0=Current Win |
| 673 | `FSLastSnapshotToFlickrWidth` | S32 | 非GUI | LIVE |  | The width of the last Flickr snapshot, in px |
| 674 | `FSLastSnapshotToPrimfeedHeight` | S32 | 非GUI | LIVE |  | The height of the last Primfeed snapshot, in px |
| 675 | `FSLastSnapshotToPrimfeedResolution` | S32 | 非GUI | LIVE |  | At what resolution should snapshots be posted on Primfeed. 0=Current W |
| 676 | `FSLastSnapshotToPrimfeedWidth` | S32 | 非GUI | LIVE |  | The width of the last Primfeed snapshot, in px |
| 677 | `FSLastSnapshotToTwitterHeight` | S32 | 非GUI | DEAD |  | The height of the last Twitter snapshot, in px |
| 678 | `FSLastSnapshotToTwitterResolution` | S32 | 非GUI | DEAD |  | At what resolution should snapshots be posted on Twitter. 0=Current Wi |
| 679 | `FSLastSnapshotToTwitterWidth` | S32 | 非GUI | DEAD |  | The width of the last Twitter snapshot, in px |
| 680 | `FSLatencyOneTimeFixRun` | Boolean | 非GUI | DEAD |  | One time fix has run for this install for script dialog colors on Late |
| 681 | `FSLegacyEdgeSnap` | Boolean | 非GUI | LIVE |  | Use old method for adjusting edge snap regions. |
| 682 | `FSLegacyMinimize` | Boolean | 非GUI | LIVE |  | Minimize floaters to bottom left instead of top left. |
| 683 | `FSLegacyNameCacheExpiration` | Boolean | 非GUI | LIVE |  | Use the legacy avatar name cache expiration (expiration at least 60 mi |
| 684 | `FSLegacyNametagPosition` | Boolean | GUI | LIVE |  | Enables the legacy nametag behavior of staying fixed at the avatar's p |
| 685 | `FSLegacyNotificationWell` | Boolean | GUI | LIVE |  | Enables the legacy notifications and system messages well |
| 686 | `FSLegacyNotificationWellAutoResize` | Boolean | GUI | LIVE |  | Enables the automatic resizing of the legacy notifications and system  |
| 687 | `FSLegacyRadarFriendColoring` | Boolean | 非GUI | LIVE |  | Use old style for friends on the radar. Uses same color as minimap. |
| 688 | `FSLegacyRadarLindenColoring` | Boolean | 非GUI | LIVE |  | Color Lindens on the radar the same as the minimap. |
| 689 | `FSLegacySearchActionOnTeleport` | U32 | 非GUI | LIVE |  | Controls what action Legacy Search should take when teleporting: 0 = N |
| 690 | `FSLetterKeysFocusNearbyChatBar` | Boolean | 非GUI | LIVE |  | If enabled, the chat bar in the Nearby Chat window will be preferred i |
| 691 | `FSLimitFramerate` | Boolean | GUI | LIVE |  | Enable framerate limitation defined by FramePerSecondLimit |
| 692 | `FSLimitTextureVRAMUsage` | Boolean | GUI | LIVE |  | If enabled, limits the amount of VRAM used for textures to the value o |
| 693 | `FSLinuxEnableWin32VoiceProxy` | Boolean | 非GUI | LIVE |  | Use Win32 SLVoice.exe for voice. Needs wine (https://www.winehq.org/)  |
| 694 | `FSLinuxEnableWin64VoiceProxy` | Boolean | 非GUI | LIVE |  | Use Win64 SLVoice.exe for voice. Needs wine (https://www.winehq.org/)  |
| 695 | `FSLocalMeshApplyJointOffsets` | Boolean | GUI | LIVE |  | use joint offsets if they are present |
| 696 | `FSLocalMeshAutoReload` | Boolean | GUI | LIVE |  | Automatically reload local mesh files when they change on disk. |
| 697 | `FSLocalMeshAutoReloadPeriod` | F32 | GUI | LIVE |  | How often, in seconds, the viewer should scan local mesh files for cha |
| 698 | `FSLocalMeshScaleAlwaysMeters` | Boolean | GUI | LIVE |  | Ignore the units specified by the Collada file. Useful when importing  |
| 699 | `FSLogAutoAcceptInventoryToChat` | Boolean | GUI | LIVE |  | If enabled, auto-accepted inventory items will be logged to nearby cha |
| 700 | `FSLogGroupImToChatConsole` | Boolean | GUI | LIVE |  | Defines if group IM notifications should be sent to the nearby chat co |
| 701 | `FSLogIMInChatHistory` | Boolean | GUI | LIVE |  | If true, IM will also be logged in the nearby chat transcript if loggi |
| 702 | `FSLogImToChatConsole` | Boolean | GUI | LIVE |  | Defines if IM notifications should be sent to the nearby chat console  |
| 703 | `FSLogSnapshotsToLocal` | Boolean | GUI | LIVE |  | Log filename of saved snapshots in to chat history |
| 704 | `FSLoginDontSavePassword` | Boolean | 非GUI | LIVE |  | Internal setting used to indicate that passwords shouldn't be saved if |
| 705 | `FSLookAtTargetLimitDistance` | Boolean | GUI | LIVE |  | Limit look at target distance from head to value of LookAtTargetMaxDis |
| 706 | `FSLookAtTargetMaxDistance` | F32 | GUI | LIVE |  | Max lookat distance in meters |
| 707 | `FSManipRotateJointUseNaturalDirection` | Boolean | GUI | LIVE |  | use the natural bone direction instead of world rotation |
| 708 | `FSManipShowJointMarkers` | Boolean | GUI | LIVE |  | Show small markers where the selectable joints are. |
| 709 | `FSMarkObjects` | Boolean | GUI | LIVE |  | Mark unnamed objects with (No Name) |
| 710 | `FSMaxAnimationPriority` | S32 | 非GUI | LIVE |  | Allow uploading animations with higher priority (up to 6) NOTE: Only p |
| 711 | `FSMaxBeamsPerSecond` | F32 | GUI | LIVE |  | How many selection beam updates to send in a second |
| 712 | `FSMaxPendingIMMessages` | S32 | 非GUI | LIVE |  | Maximum number of pending IM or group messages before a minimized or n |
| 713 | `FSMenuBackgroundAlpha` | F32 | GUI | LIVE |  | Alpha (opacity) of menu backgrounds including context menus and the to |
| 714 | `FSMeshHighLodSuffix` | String | GUI | LIVE |  | Suffix to use for High LOD models and files (DAE). |
| 715 | `FSMeshImportScaleFixup` | Boolean | 非GUI | LIVE |  | Adjust normals on import, fixes underlying issue, not recommended in c |
| 716 | `FSMeshLowLodSuffix` | String | GUI | LIVE |  | Suffix to use for Low LOD models and files (DAE). |
| 717 | `FSMeshLowestLodSuffix` | String | GUI | LIVE |  | Suffix to use for Lowest LOD models and files (DAE). |
| 718 | `FSMeshMediumLodSuffix` | String | GUI | LIVE |  | Suffix to use for Medium LOD models and files (DAE). |
| 719 | `FSMeshPhysicsSuffix` | String | GUI | LIVE |  | Suffix to use for Physics models and files (DAE). |
| 720 | `FSMeshPreviewUVGuideFile` | String | 非GUI | LIVE |  | filename of the texture to use as a UV guide for mesh preview. |
| 721 | `FSMeshUploadAutoEnableWeights` | Boolean | GUI | LIVE |  | Automatically set weights enabled for meshes with rigging info |
| 722 | `FSMeshUploadAutoShowWeightsWhenEnabled` | Boolean | GUI | LIVE |  | Automatically show weights in preview for meshes with rigging info |
| 723 | `FSMeshUploadUseGLODAsDefault` | Boolean | GUI | LIVE |  | Use the 'reliable' (GLOD) method for LODs as the default when opening  |
| 724 | `FSMilkshakeRadarToasts` | Boolean | 非GUI | LIVE |  | When enabled, radar alerts will be sent as notification toasts. |
| 725 | `FSMiniMapChatRing` | Boolean | GUI | LIVE |  | Show chat range ring on minimap |
| 726 | `FSMiniMapOpacity` | F32 | GUI | LIVE |  | The opacity for the minimap background |
| 727 | `FSMiniMapShoutRing` | Boolean | GUI | LIVE |  | Show shout range ring on minimap |
| 728 | `FSMiniMapWhisperRing` | Boolean | GUI | LIVE |  | Show whisper range ring on minimap |
| 729 | `FSMinimapPickScale` | F32 | GUI | LIVE |  | Controls the pick radius on the minimap |
| 730 | `FSModNameStyle` | U32 | GUI | LIVE |  | Font style settings for moderators' name if FSHighlightGroupMods enabl |
| 731 | `FSModTextStyle` | U32 | GUI | LIVE |  | Font style settings for moderators' name if FSHighlightGroupMods enabl |
| 732 | `FSMouselookCombatFeatures` | Boolean | GUI | LIVE |  | Enable combat features (target distance etc.) when in mouselook |
| 733 | `FSMuteAllGroups` | Boolean | GUI | LIVE |  | Disable ALL group chats. |
| 734 | `FSMuteGroupWhenNoticesDisabled` | Boolean | GUI | LIVE |  | When 'Receive group notices' is disabled, disable group chat as well. |
| 735 | `FSNameTagShowLegacyUsernames` | Boolean | GUI | LIVE |  | Show legacy name (Firstname Lastname) in user tags instead of username |
| 736 | `FSNameTagZOffsetCorrection` | S32 | GUI | LIVE |  | Changes the default Z-offset of the avatar nametags. |
| 737 | `FSNearbyChatToastsOffset` | S32 | GUI | LIVE |  | Vertical offset of the nearby chat toasts |
| 738 | `FSNearbyChatbar` | Boolean | GUI | LIVE |  | Set to true to add a chat bar to the Nearby Chat window |
| 739 | `FSNetMapDoubleClickAction` | S32 | GUI | LIVE |  | Defines the action happening if the a double click occurs on a minimap |
| 740 | `FSNetMapPhantomOpacity` | U32 | 非GUI | LIVE |  | Percentage of opacity for phantom objects on netmap. |
| 741 | `FSNetMapPhysical` | Boolean | GUI(menu) | LIVE |  | Accent physical objects on netmap in different colors. |
| 742 | `FSNetMapScripted` | Boolean | GUI(menu) | LIVE |  | Accent scripted objects on netmap in different colors. |
| 743 | `FSNetMapTempOnRez` | Boolean | GUI(menu) | LIVE |  | Accent temp on rez objects on netmap in different colors. |
| 744 | `FSNoScreenShakeOnRegionRestart` | Boolean | GUI | LIVE |  | Don't shake my screen when region restart alert message is shown |
| 745 | `FSNoVersionPopup` | Boolean | 非GUI | LIVE |  | Disables version popup on the Firestorm Login page |
| 746 | `FSNotecardFontName` | String | GUI | LIVE |  | The name of the font used for the notecard editor |
| 747 | `FSNotecardFontSize` | String | GUI | LIVE |  | The size of the font used for the notecard editor |
| 748 | `FSNotifyIMFlash` | Boolean | GUI | LIVE |  | Flash FUI button if new (group) IMs arrived and conversations floater  |
| 749 | `FSNotifyIncomingObjectSpam` | Boolean | GUI | LIVE |  | Notify about throttled incoming object offers from objects. |
| 750 | `FSNotifyIncomingObjectSpamFrom` | Boolean | GUI | LIVE |  | Notify about throttled incoming object offers from named sources. |
| 751 | `FSNotifyNearbyChatFlash` | Boolean | GUI | LIVE |  | Flash FUI button if new nearby chat arrived and conversations floater  |
| 752 | `FSNotifyUnreadChatMessages` | Boolean | GUI | LIVE |  | Notify about new unread chat messages in history if scrolled back |
| 753 | `FSNotifyUnreadIMMessages` | Boolean | GUI | LIVE |  | Notify about new unread IM messages in history if scrolled back |
| 754 | `FSOOCPostfix` | String | 非GUI | LIVE |  | Postfix to mark OOC chat |
| 755 | `FSOOCPrefix` | String | 非GUI | LIVE |  | Prefix to mark OOC chat |
| 756 | `FSOfferThrottleMaxCount` | U32 | GUI | LIVE |  | The number of objects offered within a second duration before throttli |
| 757 | `FSOpenIMContainerOnOfflineMessage` | Boolean | GUI | LIVE |  | Open the IM container at login when an offline message is present. |
| 758 | `FSOpenInventoryAfterSnapshot` | Boolean | GUI | DEAD |  | If enabled, the inventory window will open and show the snapshot after |
| 759 | `FSOpenSimAlwaysForceShowGrid` | Boolean | 非GUI | LIVE |  | Soft enable/disable the startup behaviour of OpenSim builds to allow g |
| 760 | `FSOtherRiggedPickerArmSeconds` | F32 | 非GUI | LIVE |  | AYAstorm r28: seconds to keep the other-avatar rigged attachment ID bu |
| 761 | `FSOtherRiggedPickerEnable` | Boolean | 非GUI | LIVE |  | AYAstorm r28: master enable for the GPU picker for one hovered non-sel |
| 762 | `FSOtherRiggedPickerGPU` | Boolean | 非GUI | LIVE |  | AYAstorm r28: GPU stage for resolving rigged attachments on the single |
| 763 | `FSOutputDeviceUUID` | String | 非GUI | LIVE |  | UUID of the output device used for inworld sound playback |
| 764 | `FSOverrideVRAMDetection` | Boolean | GUI | LIVE |  | Allow user to override the vRAM detection, use the FSForcedVideoMemory |
| 765 | `FSParcelMusicAutoPlay` | Boolean | GUI | LIVE |  | Auto play parcel music when available |
| 766 | `FSParcelStreamQuality` | U32 | 非GUI | LIVE |  | Parcel music stream quality. 0=Original FS path. 1=AYAstorm enhanced:  |
| 767 | `FSParticleChat` | Boolean | GUI | LIVE |  | Send Selection Info on channel 9000 when editing objects (does not wor |
| 768 | `FSPaymentConfirmationThreshold` | S32 | GUI | LIVE |  | Threshold when payment confirmation dialogs are triggered |
| 769 | `FSPaymentInfoInChat` | Boolean | GUI | LIVE |  | If true, L$ balance changes will be shown in nearby chat instead of to |
| 770 | `FSPerfFloaterSmoothingPeriods` | U32 | 非GUI | DEAD |  | Number of periods to smooth the stats over |
| 771 | `FSPermissionDebitDefaultDeny` | Boolean | 非GUI | LIVE |  | If enabled, LSL script debit permission dialogs will default to deny.  |
| 772 | `FSPhysicsPresetUser1` | String | GUI | LIVE |  | full system path to a user provided physics mesh (DAE). |
| 773 | `FSPlayDefaultBentoAnimation` | Boolean | GUI | LIVE |  | If enabled, the viewer will run a default, priority 0 bento animation  |
| 774 | `FSPoseStandLastSelectedPose` | String | 非GUI | LIVE |  | Last selected pose in the pose stand |
| 775 | `FSPoseStandLock` | Boolean | GUI | LIVE |  | When enabled, posestand will lock the avatar to the ground. |
| 776 | `FSPoserOnSaveConfirmOverwrite` | Boolean | GUI | LIVE |  | Whether to confirm overwriting a save file. |
| 777 | `FSPoserPelvisUnlockedForBvhSave` | Boolean | GUI | LIVE |  | Whether the mPelvis joint should be position/rotationally locked when  |
| 778 | `FSPoserSaveExternalFileAlso` | Boolean | GUI | LIVE |  | Whether to save to an external format (like BVH or ANIM) file as well  |
| 779 | `FSPoserShowBoneHighlights` | Boolean | 非GUI | DEAD |  | Whether to highlight a bone with the debug beacon on selection from th |
| 780 | `FSPoserStopPosingWhenClosed` | Boolean | GUI | LIVE |  | Whether to stop animating with the poser when the poser window is clos |
| 781 | `FSPoserTrackpadSensitivity` | F32 | GUI | LIVE |  | The relative sensitivity of the poser trackpad. |
| 782 | `FSPrettyEmojiButtonCode` | U32 | 非GUI | LIVE |  | Decimal code for the emoji button. Try 128569 or 128571 for example |
| 783 | `FSPrimfeedViewerApiKey` | String | 非GUI | LIVE |  | Viewer key for API login. |
| 784 | `FSRadarColorNamesByDistance` | Boolean | GUI | LIVE |  | Colors avatar nametags by distance in the radar. |
| 785 | `FSRadarColumnConfig` | U32 | 非GUI | LIVE |  | Stores the column visibility of the radar |
| 786 | `FSRadarEnhanceByBridge` | Boolean | GUI | LIVE |  | Enhance radar functionality by using client LSL Bridge. |
| 787 | `FSRadarShowMutedAndDerendered` | Boolean | GUI | LIVE |  | If enabled, show muted or derendered avatars in radar list |
| 788 | `FSRegionCrossingAngleErrorLimit` | F32 | 非GUI | LIVE |  | Region crossing angle error limit in degrees |
| 789 | `FSRegionCrossingPositionErrorLimit` | F32 | 非GUI | LIVE |  | Region crossing position error limit in meters |
| 790 | `FSRegionCrossingSmoothingTime` | F32 | 非GUI | LIVE |  | Region crossing smoothing filter time in seconds |
| 791 | `FSRegionRestartAnnounceChannel` | S32 | GUI | LIVE |  | Chat channel where region restart info is announced to. |
| 792 | `FSRemapLinuxShortcuts` | Boolean | GUI | LIVE |  | If enabled, use the special shortcuts on Linux to remap shortcuts that |
| 793 | `FSRememberUsername` | Boolean | GUI | LIVE |  | Stores the username used for logging in. |
| 794 | `FSRemoveFlyHeightLimit` | Boolean | 非GUI | LIVE |  | Remove the 4096m high fly limit |
| 795 | `FSRemoveScriptBlockButton` | Boolean | GUI | LIVE |  | Removes the "block" from script dialogs. |
| 796 | `FSRenderBeaconText` | Boolean | GUI | LIVE |  | Show beacon text in the viewer window if beacons are enabled |
| 797 | `FSRenderFarClipStepping` | Boolean | GUI | LIVE |  | Set to TRUE to increase performance via progressive draw distance step |
| 798 | `FSRenderFarClipSteppingInterval` | U32 | GUI | LIVE |  | Interval in seconds between each draw distance increment |
| 799 | `FSRenderParcelSelectionToMaxBuildHeight` | Boolean | GUI | LIVE |  | Shows the parcel boundary up to the maximum build height instead of ju |
| 800 | `FSRenderVignette` | Vector3 | GUI | LIVE | ✅ | Amount of vignette to apply (X), power of vignette shading (Y), and mu |
| 801 | `FSRepeatedEnvTogglesShared` | Boolean | GUI | LIVE |  | Whether repeated presses of sky preset shortcuts should revert to shar |
| 802 | `FSReportBlockToNearbyChat` | Boolean | GUI | LIVE |  | Reports changes to the blocklist in nearby chat |
| 803 | `FSReportCollisionMessages` | Boolean | GUI | LIVE |  | Report collision messages to scripts. |
| 804 | `FSReportCollisionMessagesChannel` | S32 | GUI | LIVE |  | The channel used to report collision messages to scripts. |
| 805 | `FSReportIgnoredAdHocSession` | Boolean | GUI | LIVE |  | Reports to nearby chat if a conference (ad-hoc) has been ignored. |
| 806 | `FSReportMutedGroupChat` | Boolean | GUI | LIVE |  | Reports to nearby chat if a group chat has been muted. |
| 807 | `FSReportRegionRestartToChat` | Boolean | GUI | LIVE |  | Announces region restart to a defined chat channel. |
| 808 | `FSReportTotalScriptCountChanges` | Boolean | GUI | LIVE |  | Reports if the change of total number of active scripts in a region ex |
| 809 | `FSReportTotalScriptCountChangesThreshold` | U32 | GUI | LIVE |  | Minimum change of total active scripts in a region before reporting. |
| 810 | `FSResetCameraOnMovement` | Boolean | GUI | LIVE |  | If true, Firestorm will reset camera on avatar movement. |
| 811 | `FSResetCameraOnTP` | Boolean | GUI | LIVE |  | If true the camera will be reset to behind the avatar on teleporting |
| 812 | `FSResetSkeletonOnStandUp` | Boolean | GUI | LIVE |  | Resets own avatar skeleton upon standing up and sends the reset to all |
| 813 | `FSRestoreOpenIMs` | Boolean | GUI | LIVE |  | Restore open IM windows from the previous session on startup. |
| 814 | `FSRevokePerms` | U32 | GUI | LIVE |  | Revokes objects anim perms on your avatar on: 0) never, 1) on sit, 2)  |
| 815 | `FSRowsPerScriptDialog` | S32 | GUI | LIVE |  | The number of rows visible in a script dialog |
| 816 | `FSSaveInventoryScriptsAsMono` | Boolean | GUI | LIVE |  | Saves scripts edited directly from inventory as Mono instead of LSL |
| 817 | `FSSavedRenderFarClip` | F32 | 非GUI | LIVE |  | Saved draw distance (used in case of logout during progressive draw di |
| 818 | `FSScriptDebugWindowClearOnClose` | Boolean | 非GUI | LIVE |  | Clear [ALL SCRIPTS] tab of script debug/error window on close. |
| 819 | `FSScriptDialogNoTransparency` | Boolean | GUI | LIVE |  | If true, script dialogs will be shown opaque and ignore the floater op |
| 820 | `FSScriptEditorRecompileButton` | Boolean | GUI | LIVE |  | Enables the save button to recompile scripts when no change in the ope |
| 821 | `FSScriptInfoExtended` | Boolean | GUI | LIVE |  | If enabled, extend basic script info feature with various details usef |
| 822 | `FSScriptingFontName` | String | GUI | LIVE |  | The name of the font used for the LSL script editor |
| 823 | `FSScriptingFontSize` | String | GUI | LIVE |  | The size of the font used for the LSL script editor |
| 824 | `FSScrollWheelExitsMouselook` | Boolean | GUI | LIVE |  | If enabled, mouselook can be left by turning the scroll wheel |
| 825 | `FSSearchGroupMaturity` | U32 | GUI | LIVE |  | Setting for the user's group search maturity level (consts in indra_co |
| 826 | `FSSecondsinChatTimestamps` | Boolean | GUI | LIVE |  | Show seconds in chat timestamps, in the chat window and logs |
| 827 | `FSSelectCopyableOnly` | Boolean | GUI | LIVE |  | Only include copyable objects during selection |
| 828 | `FSSelectIncludeGroupOwned` | Boolean | GUI | LIVE |  | Includes group-owned objects during selection |
| 829 | `FSSelectLocalSearchEditorOnShortcut` | Boolean | 非GUI | LIVE |  | If enabled, pressing the shortcut for search (CTRL-F) will focus the s |
| 830 | `FSSelectLockedOnly` | Boolean | GUI | LIVE |  | Select only objects that are locked |
| 831 | `FSSelfRiggedPickerArmSeconds` | F32 | 非GUI | LIVE |  | AYAstorm r21.1 experimental: seconds to keep the GPU self rigged-attac |
| 832 | `FSSelfRiggedPickerArmedMode` | Boolean | 非GUI | LIVE |  | AYAstorm r21.1 experimental: only render the GPU self rigged-attachmen |
| 833 | `FSSelfRiggedPickerEnable` | Boolean | 非GUI | LIVE |  | AYAstorm r21.1: master enable for the GPU self rigged-attachment picke |
| 834 | `FSSelfRiggedPickerGPU` | Boolean | 非GUI | LIVE |  | AYAstorm r21.1: GPU stage for the self rigged-attachment picker. When  |
| 835 | `FSSendTypingState` | Boolean | GUI | LIVE |  | Send typing start and typing stop state notifications to other clients |
| 836 | `FSShowAutoAcceptInventoryInNotifications` | Boolean | GUI | LIVE |  | If enabled, auto-accepted inventory items will be shown in notificatio |
| 837 | `FSShowAutorespondInNametag` | Boolean | GUI | LIVE |  | Does the user want to see autorespond mode in his own nametag? |
| 838 | `FSShowBackSLURL` | Boolean | GUI | LIVE |  | Report the SLURL of the region you completed a teleport from |
| 839 | `FSShowChatChannel` | Boolean | GUI | LIVE |  | Shows/Hides the channel selector in the Nearby Chat command line |
| 840 | `FSShowChatRangeSpheres` | Boolean | GUI | LIVE |  | Show chat range spheres (whisper, say, shout) in 3D world around avata |
| 841 | `FSShowChatType` | Boolean | GUI | LIVE |  | Shows/Hides the chat type selector (Whisper, Say, Shout) |
| 842 | `FSShowConversationVoiceStateIndicator` | Boolean | GUI | LIVE |  | Show the voice state indicator in the conversation floater tabs |
| 843 | `FSShowConvoAndRadarInML` | Boolean | GUI | LIVE |  | Conversations and Radar windows stays visible when entering mouselook  |
| 844 | `FSShowCurrencyBalanceInStatusbar` | Boolean | GUI | LIVE |  | Show the current balance in the statusbar if enabled |
| 845 | `FSShowDisplayNameUpdateNotification` | Boolean | GUI | LIVE |  | Show system notifications if somebody changes their display name. |
| 846 | `FSShowDummyAVsinRadar` | Boolean | 非GUI | LIVE |  | If true, shows dummy (preview) avatars in radar. |
| 847 | `FSShowEmojiButton` | Boolean | GUI | LIVE(UI framework) |  | Show or hide the emoji selection button in chat/IM windows |
| 848 | `FSShowGroupNameLength` | S32 | GUI | LIVE |  | Max length of group name to be printed in chat (-1 for full group name |
| 849 | `FSShowGroupTitleInTooltip` | Boolean | GUI | LIVE |  | Shows the group title of an avatar in the tooltip. |
| 850 | `FSShowIMInChatHistory` | Boolean | GUI | LIVE |  | If true, IM will also be shown in the nearby chat transcript. |
| 851 | `FSShowIMSendButton` | Boolean | GUI | LIVE(UI framework) |  | Shows the send chat button in IM session windows |
| 852 | `FSShowInboxFolder` | Boolean | GUI | LIVE |  | If enabled, the Received Items folder aka Inbox is shown in the invent |
| 853 | `FSShowInterfaceInMouselook` | Boolean | GUI | LIVE |  | If true, Firestorm will show user interface in mouselook mode. |
| 854 | `FSShowInventoryThumbnailTooltips` | Boolean | GUI | LIVE |  | Shows the inventory item thumbnail as tooltip |
| 855 | `FSShowJoinedGroupInvitations` | Boolean | GUI | LIVE |  | If enabled, invitations to groups you are already a member in will be  |
| 856 | `FSShowMessageCountInWindowTitle` | Boolean | GUI | LIVE |  | Displays the number of unread IMs in the application window title. |
| 857 | `FSShowMouselookInstructions` | Boolean | GUI | LIVE |  | If true, instructions about leaving Mouseview are displayed. |
| 858 | `FSShowMutedChatHistory` | Boolean | GUI | LIVE |  | Shows the muted text in nearby chat transcript if enabled. |
| 859 | `FSShowMyOwnVoiceVisualizer` | Boolean | GUI | LIVE |  | Show voice visualizer over my own avatar. |
| 860 | `FSShowOnscreenConsole` | Boolean | GUI | LIVE |  | Displays the on-screen console |
| 861 | `FSShowSelectedInBlinnPhong` | Boolean | 非GUI | LIVE |  | Show BlinnPhong while selected (non-persistent).This setting is driven |
| 862 | `FSShowServerVersionChangeNotice` | Boolean | GUI | LIVE |  | Shows a notice if the simulator version is different after a region cr |
| 863 | `FSShowStatsBarInMouselook` | Boolean | 非GUI | LIVE |  | Makes it so that the statistics bar stays visible when entering mousel |
| 864 | `FSShowTimestampsIM` | Boolean | GUI | LIVE |  | Show timestamps in IM |
| 865 | `FSShowTimestampsNearbyChat` | Boolean | GUI | LIVE |  | Show timestamps in nearby chat |
| 866 | `FSShowTimestampsTranscripts` | Boolean | GUI | LIVE |  | Show timestamps in transcripts |
| 867 | `FSShowToastsInFront` | Boolean | GUI | LIVE |  | Show toasts in front of other floaters if enabled |
| 868 | `FSShowTypingStateInNameTag` | Boolean | GUI | LIVE |  | Shows in the nametag of an avatar if they are typing |
| 869 | `FSShowUploadPaymentToast` | Boolean | GUI | LIVE |  | Show UploadPayment Notifications |
| 870 | `FSShowVoiceVisualizerWithDot` | Boolean | GUI | LIVE |  | Shows the voice dot over avatars as part of the voice visualizer. |
| 871 | `FSShowWhitelistReminder` | Boolean | 非GUI | LIVE |  | Show the whitelist reminder on first install. |
| 872 | `FSSkinClobbersColorPrefs` | Boolean | 非GUI | DEAD |  | If enabled the default color scheme for newly selected skins will be o |
| 873 | `FSSkinClobbersToolbarPrefs` | Boolean | GUI | LIVE |  | If enabled the default toolbar layout for newly selected skins will be |
| 874 | `FSSkinCurrentReadableName` | String | 非GUI | LIVE |  | The readable name of the currently selected skin. |
| 875 | `FSSkinCurrentThemeReadableName` | String | 非GUI | LIVE |  | The readable name of the selected theme for the current skin. |
| 876 | `FSSnapshotFrameBorderColor` | Color3 | 非GUI | LIVE | ✅ | The color of the border for the Snapshot frame |
| 877 | `FSSnapshotFrameBorderWidth` | F32 | 非GUI | LIVE | ✅ | The thickness of the line drawn around the snapshot frame |
| 878 | `FSSnapshotFrameGuideColor` | Color3 | GUI | LIVE |  | The color of the Snapshot composition guides |
| 879 | `FSSnapshotFrameGuideWidth` | F32 | GUI | LIVE |  | The thickness of the lines drawn for the composition guides |
| 880 | `FSSnapshotGuideStyle` | String | GUI | LIVE |  | The framing guide layout to display inside the snapshot frame |
| 881 | `FSSnapshotGuideVisibility` | F32 | GUI | LIVE |  | The blend factor used to color the snapshot framing guides |
| 882 | `FSSnapshotLocalFormat` | S32 | 非GUI | LIVE |  | Save snapshots to disk in this format (0 = PNG, 1 = JPEG, 2 = BMP) |
| 883 | `FSSnapshotLocalNamesWithTimestamps` | Boolean | GUI | LIVE |  | include a timestamp in the filename when saving snapshots locally |
| 884 | `FSSnapshotShowCaptureFrame` | Boolean | GUI | LIVE |  | If enabled, masks the main screen according to the capture frame. |
| 885 | `FSSnapshotShowGuides` | Boolean | GUI | LIVE |  | If enabled, shows composition guides inside the snapshot frame. |
| 886 | `FSSortAttachmentSpotsAlphabetically` | Boolean | GUI | LIVE |  | Sorts the attachment spots in the "Attach to" menus alphabetically |
| 887 | `FSSortDeferalFrames` | U32 | 非GUI | LIVE |  | How many frames after an update should we wait before sorting |
| 888 | `FSSortFSFoldersToTop` | Boolean | 非GUI | LIVE |  | Sorts the #FS and #RLV folders to the top like system folders. |
| 889 | `FSSoundCacheLocation` | String | GUI | LIVE |  | Location for caching sound files (.DSF); Uses default cache directory  |
| 890 | `FSSplashScreenHideBlogs` | Boolean | GUI | LIVE |  | Hide the blogs section on the splash screen |
| 891 | `FSSplashScreenHideDestinations` | Boolean | GUI | LIVE |  | Hide the destinations section on the splash screen |
| 892 | `FSSplashScreenHideTopBar` | Boolean | GUI | LIVE |  | Hide the top bar section on the splash screen |
| 893 | `FSSplashScreenNoTransparency` | Boolean | GUI | LIVE |  | Disable transparency effects on the splash screen |
| 894 | `FSSplashScreenUseAllCaps` | Boolean | GUI | LIVE |  | Enable all caps mode on the splash screen |
| 895 | `FSSplashScreenUseGrayMode` | Boolean | GUI | LIVE |  | Enable grayscale mode on the splash screen |
| 896 | `FSSplashScreenUseHighContrast` | Boolean | GUI | LIVE |  | Enable high contrast mode on the splash screen |
| 897 | `FSSplashScreenUseLargerFonts` | Boolean | GUI | LIVE |  | Use larger fonts on the splash screen |
| 898 | `FSSplitInventorySearchOverTabs` | Boolean | GUI | LIVE |  | If enabled, the search terms for inventory can be entered for each tab |
| 899 | `FSStartupClearBrowserCache` | Boolean | 非GUI | LIVE |  | Clear internal browser cache on next startup. |
| 900 | `FSStatbarLegacyMeanPerSec` | Boolean | GUI | LIVE |  | Use legacy period mean per second display for stat bars. |
| 901 | `FSStaticEyesUUID` | String | 非GUI | LIVE |  | Animation UUID to used to stop idle eye moment (Default is priority 2: |
| 902 | `FSStatisticsNoFocus` | Boolean | GUI | LIVE |  | If enabled, the statistics bar will never gain focus (i.e. from closin |
| 903 | `FSStatusBarMenuButtonPopupOnRollover` | Boolean | GUI | LIVE |  | Enable rollover popups on top status bar menu icons: Quick Graphics Pr |
| 904 | `FSStatusBarShowFPS` | Boolean | GUI(menu) | LIVE |  | If enabled, shows the current FPS in the main menu bar |
| 905 | `FSStatusBarShowFPSColors` | Boolean | GUI(menu) | LIVE |  | If enabled, display FPS number in a color based on the current status. |
| 906 | `FSStatusBarTimeFormat` | String | GUI | LIVE |  | Which time format to use for the status bar clock, e.g. 12 Hours, 24 H |
| 907 | `FSStatusbarShowSimulatorVersion` | Boolean | GUI | LIVE |  | If enabled, the simulator version is included in the V1-like statusbar |
| 908 | `FSStreamList` | LLSD | 非GUI | LIVE |  | Saved list of media streams |
| 909 | `FSSupportGroupChatPrefix3` | Boolean | GUI | LIVE |  | Adds (FS 1.2.3) to support group chat |
| 910 | `FSSupportGroupChatPrefixTesting` | Boolean | GUI | LIVE |  | Adds (W 56789f* os) to testing group chat |
| 911 | `FSTPHistoryTZ` | String | 非GUI | LIVE |  | Select the timezone to be used with Teleport History. ('utc' = default |
| 912 | `FSTagShowARW` | Boolean | GUI | LIVE |  | If enabled, the avatar complexity will be shown in the nametag for eve |
| 913 | `FSTagShowDistance` | Boolean | GUI | LIVE |  | If enabled, show distance to other avatars in their nametag. |
| 914 | `FSTagShowDistanceColors` | Boolean | GUI | LIVE |  | If enabled, color other avatars' nametags based on their distance |
| 915 | `FSTagShowOwnARW` | Boolean | GUI | LIVE |  | If enabled, the avatar complexity for the own avatar will be shown in  |
| 916 | `FSTagShowTooComplexOnlyARW` | Boolean | GUI | LIVE |  | If enabled, the avatar complexity will be shown in the nametag only fo |
| 917 | `FSTeleportHistoryShowDate` | Boolean | GUI | LIVE(UI framework) |  | Shows the exact date and time in the teleport history. |
| 918 | `FSTeleportHistoryShowPosition` | Boolean | GUI | LIVE(UI framework) |  | Shows the local position within a region in the teleport history. |
| 919 | `FSTeleportToOffsetLateral` | F32 | 非GUI | LIVE |  | Horizontal distance from the target avatar that is used for teleportin |
| 920 | `FSTeleportToOffsetVertical` | F32 | 非GUI | LIVE |  | Vertical distance from the target avatar that is used for teleporting  |
| 921 | `FSTempDerenderUntilTeleport` | Boolean | GUI | LIVE |  | If enabled, temporary derendered objects will stay derendered until te |
| 922 | `FSTextureDefaultSaveAsFormat` | Boolean | GUI | LIVE |  | The default "save as" format for textures, in the texture preview floa |
| 923 | `FSToolbarsResetOnModeChange` | Boolean | GUI | LIVE |  | If enabled the user's current toolbar layout for newly selected modes  |
| 924 | `FSToolboxExpanded` | Boolean | 非GUI | LIVE |  | Whether to show additional build tool controls |
| 925 | `FSTrimLegacyNames` | Boolean | GUI | LIVE |  | Trim "Resident" from Legacy Names. |
| 926 | `FSTurnAvatarToSelectedObject` | Boolean | GUI | LIVE |  | If enabled, the avatar turns towards an selected object |
| 927 | `FSTypeDuringEmote` | Boolean | GUI | LIVE |  | Enables the typing animation even while emoting |
| 928 | `FSTypingChevronPrefix` | Boolean | GUI | LIVE |  | Adds an additional chevron prefix to the IM window as typing indicator |
| 929 | `FSUndeformUUID` | String | 非GUI | LIVE |  | Animation UUID to use for the undeform |
| 930 | `FSUnfocusChatHistoryOnReturn` | Boolean | GUI | LIVE |  | De-focus chat history after sending a message |
| 931 | `FSUnlinkConfirmEnabled` | Boolean | GUI | LIVE |  | Unlink confirmation dialog functionality enabled? |
| 932 | `FSUploadAnimationOnOwnAvatar` | Boolean | GUI | LIVE |  | Uploading an animation preview on own avatar if set to true, preview o |
| 933 | `FSUseAis3Api` | Boolean | 非GUI | LIVE |  | Option to disable the use of the AISv3 inventory API. NOTE: This setti |
| 934 | `FSUseAltOOC` | Boolean | GUI | LIVE |  | Set to TRUE to use the keyboard shortcut Alt+Enter to send ((OOC)) mes |
| 935 | `FSUseAntiSpamMine` | Boolean | GUI | LIVE |  | Use the Anti-Spam System even for the user's own objects |
| 936 | `FSUseBWEmojis` | Boolean | GUI | LIVE |  | Use Black and White Emojis. |
| 937 | `FSUseBuiltInHistory` | Boolean | GUI | LIVE |  | Open the conversation transcript in the built in log viewer. |
| 938 | `FSUseChatMentionAutoComplete` | Boolean | GUI | LIVE |  | Enable auto-completion when typing @ mentions in chat. |
| 939 | `FSUseCtrlShout` | Boolean | GUI | LIVE |  | Set to TRUE to use the keyboard shortcut Ctrl+Enter to Shout in Nearby |
| 940 | `FSUseFSLegacySearch` | Boolean | 非GUI | LIVE |  | Uses Firestorm's legacy search if enabled - the Linden Lab version ins |
| 941 | `FSUseLegacyClienttags` | U32 | GUI | LIVE |  | 0=Off, 1=Local Client tags, 2=Download Client tags (needs relog) |
| 942 | `FSUseLegacyCursors` | Boolean | GUI | LIVE |  | Use 1.x style cursors instead |
| 943 | `FSUseLegacyInventoryAcceptMessages` | Boolean | GUI | LIVE |  | If enabled, the viewer will send accept/decline response for inventory |
| 944 | `FSUseLegacyLoginPanel` | Boolean | 非GUI | LIVE |  | If enabled, the legacy layout version of the login panel will be used |
| 945 | `FSUseLegacyObjectProperties` | Boolean | GUI | LIVE |  | If enabled, the legacy object profile floater will be used when openin |
| 946 | `FSUseLegacyUnsupportedHardwareChecks` | Boolean | 非GUI | LIVE |  | If enabled, Firestorm will perform utterly pointless checks that proba |
| 947 | `FSUseNearbyChatConsole` | Boolean | GUI | LIVE |  | Display popup chat embedded into the read-only world console (v1-style |
| 948 | `FSUseNewRegionRestartNotification` | Boolean | GUI | LIVE |  | Use the new region restart notification instead of the old one with to |
| 949 | `FSUseNewTexturePanel` | Boolean | GUI | LIVE |  | Use the new Texture/Material editor panel in build mode |
| 950 | `FSUsePrettyEmojiButton` | Boolean | GUI | LIVE |  | Use an emoji for the emoji button. |
| 951 | `FSUseReadOfflineMsgsCap` | Boolean | 非GUI | LIVE |  | If enabled, use the ReadOfflineMsgsCap to request offline messages at  |
| 952 | `FSUseShiftWhisper` | Boolean | GUI | LIVE |  | Set to TRUE to use the keyboard shortcut Shift+Enter to Whisper in Nea |
| 953 | `FSUseSingleLineChatEntry` | Boolean | 非GUI | LIVE |  | Use single line chat entry instead of auto-expanding chat entry |
| 954 | `FSUseSmallCameraFloater` | Boolean | GUI | LIVE |  | If enabled, the camera floater will be smaller and not contain the cam |
| 955 | `FSUseStandaloneBlocklistFloater` | Boolean | GUI | LIVE |  | If enabled, Firestorm will use a standalone floater for the blocklist. |
| 956 | `FSUseStandaloneGroupFloater` | Boolean | GUI | LIVE |  | If enabled, Firestorm will use a standalone floater for each group pro |
| 957 | `FSUseStandalonePlaceDetailsFloater` | Boolean | GUI | LIVE |  | If enabled, Firestorm will use a standalone floater for each landmark  |
| 958 | `FSUseStandaloneTeleportHistoryFloater` | Boolean | GUI | LIVE |  | If enabled, Firestorm will use a standalone floater for the teleport h |
| 959 | `FSUseStatsInsteadOfLagMeter` | Boolean | GUI | LIVE |  | Clicking on traffic indicator (upper right) toggles Statistics window, |
| 960 | `FSUseV2Friends` | Boolean | GUI | LIVE |  | Makes Comm->Friends and Comm->Groups open the v2 based windows. |
| 961 | `FSVolumeControlsPanelOpen` | Boolean | 非GUI | DEAD |  | Internal control for visibility of volume control panel. |
| 962 | `FSWearableFavoritesSortOrder` | U32 | 非GUI | LIVE |  | The sort order for the wearable favorites item list |
| 963 | `FSWorldMapDoubleclickTeleport` | Boolean | GUI | LIVE |  | If enabled, double click teleports on the world map will be enabled (d |
| 964 | `FSdataQAtest` | Boolean | 非GUI | LIVE |  | Enable testing fsdata instead of the normal fsdata |
| 965 | `FSllOwnerSayToScriptDebugWindowRouting` | U32 | 非GUI | LIVE |  | Routing options for FSllOwnerSayToScriptDebugWindow (0 = both tabs, 1  |
| 966 | `FakeInitialOutfitName` | String | 非GUI | LIVE |  | Pretend that this is first time login and specified name was chosen |
| 967 | `FastCacheFetchEnabled` | Boolean | 非GUI | LIVE |  | Enable texture fast cache fetching if set |
| 968 | `FilterItemsMaxTimePerFrameVisible` | S32 | 非GUI | DEAD |  | Max time devoted to items filtering per frame for visible inventory li |
| 969 | `FindLandArea` | Boolean | GUI | LIVE |  | Enables filtering of land search results by area |
| 970 | `FindLandPrice` | Boolean | GUI | LIVE |  | Enables filtering of land search results by price |
| 971 | `FindLandType` | String | GUI | LIVE |  | Controls which type of land you are searching for in Find Land interfa |
| 972 | `FindOriginalOpenWindow` | Boolean | GUI | LIVE |  | Sets the action for 'Find original' and 'Show in Inventory' (0 - shows |
| 973 | `FirstLoginThisInstall` | Boolean | 非GUI | LIVE |  | Specifies that you have not logged in with the viewer since you perfor |
| 974 | `FirstName` | String | 非GUI | LIVE |  | Login first name |
| 975 | `FirstPersonAvatarVisible` | Boolean | GUI | LIVE |  | Display avatar and attachments below neck while in mouse look |
| 976 | `FirstRunThisInstall` | Boolean | 非GUI | LIVE |  | Specifies that you have not run the viewer since you performed a clean |
| 977 | `FirstSelectedDisabledPopups` | Boolean | 非GUI | DEAD |  | Return false if there is not disabled popup selected in the list of fl |
| 978 | `FirstSelectedEnabledPopups` | Boolean | 非GUI | DEAD |  | Return false if there is not enable popup selected in the list of floa |
| 979 | `FirstUseFlyOverride` | Boolean | 非GUI | LIVE |  | Whether the next use of the Fly Override would be the first use |
| 980 | `FixedWeather` | Boolean | GUI(menu) | DEAD |  | Weather effects do not change over time |
| 981 | `FlashCount` | S32 | GUI | LIVE |  | Number of flashes of item. Requires restart. |
| 982 | `FlashPeriod` | F32 | GUI | LIVE |  | Period at which item flash (seconds). Requires restart. |
| 983 | `FloaterActiveSpeakersSortAscending` | Boolean | 非GUI | DEAD |  | Whether to sort up or down |
| 984 | `FloaterActiveSpeakersSortColumn` | String | 非GUI | DEAD |  | Column name to sort on |
| 985 | `FloaterMapEast` | String | 非GUI | DEAD |  | Floater Map East Label |
| 986 | `FloaterMapNorth` | String | 非GUI | DEAD |  | Floater Map North Label |
| 987 | `FloaterMapNorthEast` | String | 非GUI | DEAD |  | Floater Map North-East Label |
| 988 | `FloaterMapNorthWest` | String | 非GUI | DEAD |  | Floater Map North-West Label |
| 989 | `FloaterMapSouth` | String | 非GUI | DEAD |  | Floater Map South Label |
| 990 | `FloaterMapSouthEast` | String | 非GUI | DEAD |  | Floater Map South-East Label |
| 991 | `FloaterMapSouthWest` | String | 非GUI | DEAD |  | Floater Map South-West Label |
| 992 | `FloaterMapWest` | String | 非GUI | DEAD |  | Floater Map West Label |
| 993 | `FloaterStatisticsRect` | Rect | 非GUI | DEAD |  | Rectangle for chat transcript |
| 994 | `FlycamAbsolute` | Boolean | 非GUI | DEAD |  | Treat Flycam values as absolute positions (not deltas). |
| 995 | `FlycamAxisDeadZone0` | F32 | GUI | LIVE |  | Flycam axis 0 dead zone. |
| 996 | `FlycamAxisDeadZone1` | F32 | GUI | LIVE |  | Flycam axis 1 dead zone. |
| 997 | `FlycamAxisDeadZone2` | F32 | GUI | LIVE |  | Flycam axis 2 dead zone. |
| 998 | `FlycamAxisDeadZone3` | F32 | GUI | LIVE |  | Flycam axis 3 dead zone. |
| 999 | `FlycamAxisDeadZone4` | F32 | GUI | LIVE |  | Flycam axis 4 dead zone. |
| 1000 | `FlycamAxisDeadZone5` | F32 | GUI | LIVE |  | Flycam axis 5 dead zone. |
| 1001 | `FlycamAxisDeadZone6` | F32 | GUI | LIVE |  | Flycam axis 6 dead zone. |
| 1002 | `FlycamAxisScale0` | F32 | GUI | LIVE |  | Flycam axis 0 scaler. |
| 1003 | `FlycamAxisScale1` | F32 | GUI | LIVE |  | Flycam axis 1 scaler. |
| 1004 | `FlycamAxisScale2` | F32 | GUI | LIVE |  | Flycam axis 2 scaler. |
| 1005 | `FlycamAxisScale3` | F32 | GUI | LIVE |  | Flycam axis 3 scaler. |
| 1006 | `FlycamAxisScale4` | F32 | GUI | LIVE |  | Flycam axis 4 scaler. |
| 1007 | `FlycamAxisScale5` | F32 | GUI | LIVE |  | Flycam axis 5 scaler. |
| 1008 | `FlycamAxisScale6` | F32 | GUI | LIVE |  | Flycam axis 6 scaler. |
| 1009 | `FlycamBuildModeScale` | F32 | 非GUI | LIVE |  | Scale factor to apply to flycam movements when in build mode. |
| 1010 | `FlycamFeathering` | F32 | GUI | LIVE |  | Flycam feathering (less is softer) |
| 1011 | `FlyingAtExit` | Boolean | 非GUI | LIVE |  | Was flying when last logged out, so fly when logging in |
| 1012 | `FocusOffsetRearView` | Vector3D | 非GUI | LIVE |  | Initial focus point offset relative to avatar for the camera preset Re |
| 1013 | `FocusPosOnLogout` | Vector3D | 非GUI | LIVE |  | Camera focus point when last logged out (global coordinates) |
| 1014 | `FolderAutoOpenDelay` | F32 | 非GUI | LIVE |  | Seconds before automatically expanding the folder under the mouse when |
| 1015 | `FontScreenDPI` | F32 | 非GUI | LIVE |  | Font resolution, higher is bigger (pixels per inch) |
| 1016 | `ForceAddressSize` | U32 | 非GUI | DEAD |  | Force Windows update to 32-bit or 64-bit viewer. |
| 1017 | `ForceAssetFail` | U32 | 非GUI | LIVE |  | Force wearable fetches to fail for this asset type. |
| 1018 | `ForceInitialCOFDelay` | F32 | 非GUI | LIVE |  | Number of seconds to delay initial processing of COF contents |
| 1019 | `ForceLoginURL` | String | 非GUI | LIVE |  | Force a specified URL for login page content - used if exists |
| 1020 | `ForceMissingType` | U32 | 非GUI | LIVE |  | Force this wearable type to be missing from COF |
| 1021 | `ForceShowGrid` | Boolean | GUI | LIVE |  | Always show grid dropdown on login screen |
| 1022 | `FramePerSecondLimit` | U32 | GUI | LIVE |  | Controls upper limit of frames per second |
| 1023 | `FreezeTime` | Boolean | 非GUI | LIVE |  |  |
| 1024 | `FriendsListHideUsernames` | Boolean | 非GUI | LIVE |  | Show both Display name and Username in Friend list |
| 1025 | `FriendsListShowIcons` | Boolean | GUI(menu) | LIVE |  | Show/hide online and all friends icons in the friend list |
| 1026 | `FriendsListShowPermissions` | Boolean | GUI(menu) | LIVE |  | Show/hide permission icons in the friend list |
| 1027 | `FriendsSortOrder` | U32 | 非GUI | LIVE |  | Specifies sort order for friends (0 = by name, 1 = by online status) |
| 1028 | `FullScreen` | Boolean | GUI | LIVE |  | Run a fullscreen session. MacOS not supported |
| 1029 | `FullScreenAspectRatio` | F32 | 非GUI | DEAD |  | Aspect ratio of fullscreen display (width / height) |
| 1030 | `FullScreenAutoDetectAspectRatio` | Boolean | 非GUI | DEAD |  | Automatically detect proper aspect ratio for fullscreen display |
| 1031 | `GLTFEnabled` | Boolean | 非GUI | LIVE |  | Enable GLTF support.  Set to true by simulator if the simulator you ar |
| 1032 | `GenericErrorPageURL` | String | 非GUI | LIVE |  | URL to set as a property on LLMediaControl to navigate to if the a pag |
| 1033 | `GesturesEveryoneCopy` | Boolean | GUI | LIVE?(動的名) |  | Everyone can copy the newly created gesture |
| 1034 | `GesturesMarketplaceURL` | String | 非GUI | DEAD |  | URL to the Gestures Marketplace |
| 1035 | `GesturesNextOwnerCopy` | Boolean | GUI | LIVE?(動的名) |  | Newly created gestures can be copied by next owner |
| 1036 | `GesturesNextOwnerModify` | Boolean | GUI | LIVE?(動的名) |  | Newly created gestures can be modified by next owner |
| 1037 | `GesturesNextOwnerTransfer` | Boolean | GUI | LIVE?(動的名) |  | Newly created gestures can be resold or given away by next owner |
| 1038 | `GesturesShareWithGroup` | Boolean | GUI | LIVE?(動的名) |  | Newly created gestures are shared with the currently active group |
| 1039 | `GoogleTranslateAPIKey` | String | 非GUI | LIVE |  | Google Translate API key |
| 1040 | `GridCrossSections` | Boolean | 非GUI | DEAD |  | Highlight cross sections of prims with grid manipulation plane. |
| 1041 | `GridDrawSize` | F32 | GUI | LIVE |  | Visible extent of 2D snap grid (meters) |
| 1042 | `GridListDownload` | Boolean | 非GUI | LIVE |  | Whether to fetch a grid list from the URL specified in GridListDownloa |
| 1043 | `GridListDownloadURL` | String | 非GUI | LIVE |  | Fetch a grid list from this URL. |
| 1044 | `GridMode` | S32 | 非GUI | LIVE |  | Snap grid reference frame (0 = world, 1 = local, 2 = reference object) |
| 1045 | `GridOpacity` | F32 | GUI | LIVE |  | Grid line opacity (0.0 = completely transparent, 1.0 = completely opaq |
| 1046 | `GridResolution` | F32 | GUI | LIVE |  | Size of single grid step (meters) |
| 1047 | `GridStatusFloaterRect` | Rect | 非GUI | LIVE |  | Web profile floater dimensions |
| 1048 | `GridStatusRSS` | String | 非GUI | LIVE |  | URL that points to SL Grid Status RSS |
| 1049 | `GridStatusUpdateDelay` | F32 | 非GUI | LIVE |  | Timer delay for updating Grid Status RSS. |
| 1050 | `GridSubUnit` | Boolean | GUI | LIVE |  | Display fractional grid steps, relative to grid size |
| 1051 | `GridSubdivision` | S32 | 非GUI | LIVE |  | Maximum number of times to divide single snap grid unit when GridSubUn |
| 1052 | `GroupListShowIcons` | Boolean | GUI(menu) | LIVE |  | Show/hide group icons in the group list |
| 1053 | `GroupMembersSortOrder` | String | 非GUI | LIVE |  | The order by which group members will be sorted (name\|donated\|online |
| 1054 | `GroupSnoozeTime` | S32 | GUI | LIVE |  | Amount of time (in seconds) group chat will be snoozed for |
| 1055 | `GuidebookURL` | String | 非GUI | LIVE |  | URL for Guidebook content |
| 1056 | `HUDScaleFactor` | F32 | GUI | LIVE |  | Scale of HUD attachments |
| 1057 | `HeadlessClient` | Boolean | 非GUI | LIVE |  | Run in headless mode by disabling GL rendering, keyboard, etc |
| 1058 | `HeightUnits` | Boolean | 非GUI | LIVE |  | Determines which metric units are used: 1(TRUE) for meter and 0(FALSE) |
| 1059 | `HelpFloaterOpen` | Boolean | 非GUI | LIVE |  | Show Help Floater on login? |
| 1060 | `HelpURLFormat` | String | 非GUI | LIVE |  | URL pattern for help page; arguments will be encoded; see llviewerhelp |
| 1061 | `HideSelectedObjects` | Boolean | GUI(menu) | LIVE |  | Hide Selected Objects |
| 1062 | `HideUIControls` | Boolean | 非GUI | LIVE |  | Hide all menu items and buttons |
| 1063 | `HighResSnapshot` | Boolean | GUI(menu) | LIVE |  | Double resolution of snapshot from current window resolution |
| 1064 | `HomeSidePanelURL` | String | 非GUI | LIVE |  | URL for the web page to display in the Home side panel |
| 1065 | `HostID` | String | 非GUI | LIVE |  | Machine identifier for hosted Second Life instances |
| 1066 | `HoverHeightAffectsCamera` | Boolean | GUI | LIVE |  | Camera view is affected by Hover Height setting |
| 1067 | `HowToHelpURL` | String | 非GUI | DEAD |  | URL for How To help content |
| 1068 | `HttpPipelining` | Boolean | 非GUI | LIVE |  | If true, viewer will attempt to pipeline HTTP requests. |
| 1069 | `HttpProxyType` | String | GUI | LIVE |  | Proxy type to use for HTTP operations |
| 1070 | `HttpRangeRequestsDisable` | Boolean | 非GUI | LIVE |  | If true, viewer will not issue GET requests with 'Range:' headers for  |
| 1071 | `IMShowControlPanel` | Boolean | 非GUI | LIVE |  | Show IM Control Panel |
| 1072 | `IMShowNamesForP2PConv` | Boolean | GUI | LIVE |  | Enable(disable) showing of a names in the chat. |
| 1073 | `IMShowTime` | Boolean | GUI(menu) | LIVE |  | Enable(disable) timestamp showing in the chat. |
| 1074 | `IgnoreAllNotifications` | Boolean | 非GUI | LIVE |  | Ignore all notifications so we never need user input on them. |
| 1075 | `IgnoreFOVZoomForLODs` | Boolean | 非GUI | LIVE |  | Ignore zoom effect(CTRL+0) when calculating lods. |
| 1076 | `IgnorePixelDepth` | Boolean | 非GUI | LIVE |  | Ignore pixel depth settings. |
| 1077 | `ImagePipelineUseHTTP` | Boolean | GUI(menu) | LIVE |  | If TRUE use HTTP GET to fetch textures from the server |
| 1078 | `ImporterDebugMode` | U32 | 非GUI | LIVE |  | At 0 does nothing, at 1 dumps skinning data near orifinal file, at 2 d |
| 1079 | `ImporterDebugVerboseLogging` | Boolean | GUI | LIVE |  | Enable debug output to more precisely identify sources of import error |
| 1080 | `ImporterLegacyMatching` | Boolean | 非GUI | LIVE |  | Enable index based model matching. |
| 1081 | `ImporterModelLimit` | U32 | 非GUI | LIVE |  | Limits amount of importer generated (when over 8 faces) models for dae |
| 1082 | `ImporterPreprocessDAE` | Boolean | 非GUI | LIVE |  | Enable preprocessing for DAE files to fix some ColladaDOM related prob |
| 1083 | `InBandwidth` | F32 | 非GUI | LIVE(XML専用) |  | Incoming bandwidth throttle (bps) |
| 1084 | `InactiveFloaterTransparency` | F32 | GUI | LIVE |  | Transparency of inactive floaters (floaters that have no focus) |
| 1085 | `IndirectMaxComplexity` | U32 | GUI | LIVE |  | Controls RenderAvatarMaxComplexity in a non-linear fashion (do not set |
| 1086 | `IndirectMaxNonImpostors` | U32 | GUI | LIVE |  | Controls RenderAvatarMaxNonImpostors in a non-linear fashion (do not s |
| 1087 | `InstallLanguage` | String | 非GUI | LIVE |  | Language passed from installer (for UI) |
| 1088 | `InternalShowGroupNoticesTopRight` | Boolean | 非GUI | LIVE |  | Holds the information if group notifications should be shown in top ri |
| 1089 | `InterpolationPhaseOut` | F32 | 非GUI | LIVE |  | Seconds to phase out interpolated motion |
| 1090 | `InterpolationTime` | F32 | 非GUI | LIVE |  | How long to extrapolate object motion after last packet received |
| 1091 | `InventoryAddAttachmentBehavior` | Boolean | 非GUI | LIVE |  | Defines behavior when hitting return on an inventory item (unused in f |
| 1092 | `InventoryAutoOpenDelay` | F32 | 非GUI | DEAD |  | Seconds before automatically opening inventory when mouse is over inve |
| 1093 | `InventoryDebugSimulateLateOpRate` | F32 | 非GUI | LIVE |  | Rate at which we simulate late-completing copy/link requests in some o |
| 1094 | `InventoryDebugSimulateOpFailureRate` | F32 | 非GUI | LIVE |  | Rate at which we simulate failures of copy/link requests in some opera |
| 1095 | `InventoryDisplayInbox` | Boolean | 非GUI | LIVE |  | UNUSED - Controlled via FSShowInboxFolder and FSAlwaysShowInboxButton: |
| 1096 | `InventoryExposeFolderID` | Boolean | 非GUI | LIVE |  | Allows copying folder id from context menu |
| 1097 | `InventoryFavoritesColorText` | Boolean | GUI | LIVE |  | render favorite items using InventoryFavoriteText as color |
| 1098 | `InventoryFavoritesUseHollowStar` | Boolean | GUI | LIVE |  | Show star near folders that contain favorites |
| 1099 | `InventoryFavoritesUseStar` | Boolean | GUI | LIVE |  | Show star near favorited items in inventory |
| 1100 | `InventoryLinking` | Boolean | 非GUI | LIVE |  | Enable ability to create links to folders and items via "Paste as link |
| 1101 | `InventoryOutboxLogging` | Boolean | 非GUI | LIVE |  | Enable debug output associated with the Merchant Outbox. |
| 1102 | `InventoryOutboxMakeVisible` | Boolean | 非GUI | LIVE |  | Enable making the Merchant Outbox visible in the inventory for debug p |
| 1103 | `InventoryOutboxMaxFolderCount` | U32 | 非GUI | LIVE |  | Maximum number of subfolders allowed in a listing in the merchant outb |
| 1104 | `InventoryOutboxMaxFolderDepth` | U32 | 非GUI | LIVE |  | Maximum number of nested levels of subfolders allowed in a listing in  |
| 1105 | `InventoryOutboxMaxItemCount` | U32 | 非GUI | LIVE |  | Maximum number of items allowed in a listing in the merchant outbox. |
| 1106 | `InventoryOutboxMaxStockItemCount` | U32 | 非GUI | LIVE |  | Maximum number of items allowed in a stock folder. |
| 1107 | `InventorySortOrder` | U32 | GUI | LIVE |  | Specifies sort key for inventory items (+0 = name, +1 = date, +2 = fol |
| 1108 | `InventoryTrashMaxCapacity` | U32 | 非GUI | LIVE |  | Maximum capacity of the Trash folder. User will be offered to clean it |
| 1109 | `InvertMouse` | Boolean | GUI | LIVE |  | When in mouselook, moving mouse up looks down and vice verse (FALSE =  |
| 1110 | `JoystickAvatarEnabled` | Boolean | GUI | LIVE |  | Enables the Joystick to control Avatar movement. |
| 1111 | `JoystickAxis0` | S32 | GUI | LIVE |  | Flycam hardware axis mapping for internal axis 0 ([0, 5]). |
| 1112 | `JoystickAxis1` | S32 | GUI | LIVE |  | Flycam hardware axis mapping for internal axis 1 ([0, 5]). |
| 1113 | `JoystickAxis2` | S32 | GUI | LIVE |  | Flycam hardware axis mapping for internal axis 2 ([0, 5]). |
| 1114 | `JoystickAxis3` | S32 | GUI | LIVE |  | Flycam hardware axis mapping for internal axis 3 ([0, 5]). |
| 1115 | `JoystickAxis4` | S32 | GUI | LIVE |  | Flycam hardware axis mapping for internal axis 4 ([0, 5]). |
| 1116 | `JoystickAxis5` | S32 | GUI | LIVE |  | Flycam hardware axis mapping for internal axis 5 ([0, 5]). |
| 1117 | `JoystickAxis6` | S32 | GUI | LIVE |  | Flycam hardware axis mapping for internal axis 6 ([0, 5]). |
| 1118 | `JoystickBuildEnabled` | Boolean | GUI | LIVE |  | Enables the Joystick to move edited objects. |
| 1119 | `JoystickDeviceUUID` | LLSD | 非GUI | LIVE |  | Preffered device ID. |
| 1120 | `JoystickEnabled` | Boolean | GUI | LIVE |  | Enables Joystick Input. |
| 1121 | `JoystickFlycamEnabled` | Boolean | GUI | LIVE |  | Enables the Joystick to control the flycam. |
| 1122 | `JoystickInitialized` | String | 非GUI | LIVE |  | Whether or not a joystick has been detected and initialized. |
| 1123 | `JoystickMouselookYaw` | Boolean | 非GUI | LIVE |  | Pass joystick yaw to scripts in mouse look. |
| 1124 | `JoystickRunThreshold` | F32 | 非GUI | LIVE |  | Input threshold to initiate running |
| 1125 | `Jpeg2000AdvancedCompression` | Boolean | 非GUI | LIVE |  | Use advanced Jpeg2000 compression options (precincts, blocks, ordering |
| 1126 | `Jpeg2000BlocksSize` | S32 | 非GUI | LIVE |  | Size of encoding blocks. Assumed square and same for all levels. Must  |
| 1127 | `Jpeg2000PrecinctsSize` | S32 | 非GUI | LIVE |  | Size of image precincts. Assumed square and same for all levels. Must  |
| 1128 | `KeepAspectForDiskSnapshot` | Boolean | 非GUI | DEAD |  | Always keep width to height ratio in local snapshots the same, regardl |
| 1129 | `KeepAspectForEmailSnapshot` | Boolean | 非GUI | DEAD |  | Always keep width to height ratio in postcard snapshots the same, rega |
| 1130 | `KeepAspectForInventorySnapshot` | Boolean | 非GUI | DEAD |  | Always keep width to height ratio in inventory snapshots the same, reg |
| 1131 | `KeepAspectForProfileSnapshot` | Boolean | 非GUI | DEAD |  | Always keep width to height ratio in profile snapshots the same, regar |
| 1132 | `KeepAspectForSnapshot` | Boolean | 非GUI | LIVE |  | Always keep width to height ratio in snapshots the same, regardless of |
| 1133 | `KeepAutoTuneLock` | U32 | GUI | LIVE |  | When enabled the AutoTuneLock will be maintainted all following sessio |
| 1134 | `LSLFindCaseInsensitivity` | Boolean | GUI | LIVE(widget経由) |  | Use case insensitivity when searching for text |
| 1135 | `LSLFindDirection` | Boolean | GUI | LIVE(widget経由) |  | Direction text will be searched for a match (0: down ; 1: up) |
| 1136 | `LSLFontSizeName` | String | 非GUI | LIVE |  | UNUSED - Text font size in LSL editor |
| 1137 | `LSLHelpURL` | String | 非GUI | LIVE |  | URL that points to LSL help files, with [LSL_STRING] corresponding to  |
| 1138 | `LagMeterShrunk` | Boolean | 非GUI | LIVE |  | Last large/small state for lag meter |
| 1139 | `LandBrushForce` | F32 | 非GUI | LIVE |  | Multiplier for land modification brush force. |
| 1140 | `LandBrushSize` | F32 | GUI | LIVE |  | Size of affected region when using terraform tool |
| 1141 | `LandmarksSortedByDate` | Boolean | 非GUI | LIVE |  | Reflects landmarks panel sorting order. |
| 1142 | `Language` | String | GUI | LIVE |  | Specifies language (for UI) |
| 1143 | `LanguageIsPublic` | Boolean | GUI | LIVE |  | Let other residents see our language information |
| 1144 | `LastAppearanceTab` | S32 | 非GUI | LIVE |  | Last selected tab in appearance floater |
| 1145 | `LastConnectedGrid` | String | 非GUI | LIVE |  | Last grid with successful connection |
| 1146 | `LastFeatureVersion` | S32 | 非GUI | LIVE |  | [DO NOT MODIFY] Feature Table Version number for tracking rendering sy |
| 1147 | `LastGPUString` | String | 非GUI | LIVE |  | [DO NOT MODIFY] previous GPU id string for tracking hardware changes |
| 1148 | `LastJ2CVersion` | String | 非GUI | LIVE |  | Last used J2C engine version |
| 1149 | `LastMediaSettingsTab` | S32 | 非GUI | LIVE |  | Last selected tab in media settings window |
| 1150 | `LastName` | String | 非GUI | LIVE |  | Login last name |
| 1151 | `LastPrefTab` | S32 | 非GUI | LIVE |  | Last selected tab in preferences window |
| 1152 | `LastRunVersion` | String | 非GUI | LIVE |  | Version number of last instance of the viewer that you ran |
| 1153 | `LastSelectedGrass` | String | 非GUI | LIVE?(動的名) |  | The last selected grass option from the build tools |
| 1154 | `LastSelectedTree` | String | 非GUI | LIVE?(動的名) |  | The last selected tree option from the build tools |
| 1155 | `LastSnapshotToDiskHeight` | S32 | 非GUI | LIVE |  | The height of the last disk snapshot, in px |
| 1156 | `LastSnapshotToDiskResolution` | S32 | 非GUI | LIVE |  | At what resolution should snapshots be taken to disk. 0=Current Window |
| 1157 | `LastSnapshotToDiskWidth` | S32 | 非GUI | LIVE |  | The width of the last disk snapshot, in px |
| 1158 | `LastSnapshotToEmailHeight` | S32 | 非GUI | LIVE |  | The height of the last email snapshot, in px |
| 1159 | `LastSnapshotToEmailResolution` | S32 | 非GUI | LIVE |  | At what resolution should snapshots be taken as postcards. 0=Current W |
| 1160 | `LastSnapshotToEmailWidth` | S32 | 非GUI | LIVE |  | The width of the last email snapshot, in px |
| 1161 | `LastSnapshotToInventoryHeight` | S32 | 非GUI | LIVE |  | The height of the last texture snapshot, in px |
| 1162 | `LastSnapshotToInventoryResolution` | S32 | 非GUI | LIVE |  | At what resolution should snapshots be taken into inventory. 0=Current |
| 1163 | `LastSnapshotToInventoryWidth` | S32 | 非GUI | LIVE |  | The width of the last texture snapshot, in px |
| 1164 | `LastSnapshotToProfileHeight` | S32 | 非GUI | LIVE |  | The height of the last profile snapshot, in px |
| 1165 | `LastSnapshotToProfileResolution` | S32 | 非GUI | LIVE |  | At what resolution should snapshots be taken to the profile. 0=Current |
| 1166 | `LastSnapshotToProfileWidth` | S32 | 非GUI | LIVE |  | The width of the last profile snapshot, in px |
| 1167 | `LastUIFeatureVersion` | LLSD | 非GUI | LIVE |  | UI Feature Version number for tracking feature notification between vi |
| 1168 | `LeapCommand` | LLSD | 非GUI | LIVE |  | Zero or more command lines to run LLSD Event API Plugin programs. |
| 1169 | `LeapPlaybackEventsCommand` | LLSD | 非GUI | LIVE |  | Command line to use leap to launch playback of event recordings |
| 1170 | `LeaveMouselook` | Boolean | 非GUI | LIVE |  | Exit Mouselook mode via S or Down Arrow keys while sitting |
| 1171 | `LetterKeysAffectsMovementNotFocusChatBar` | Boolean | GUI | LIVE |  | When printable characters keys (possibly with Shift held) are pressed, |
| 1172 | `LimitDragDistance` | Boolean | GUI | LIVE |  | Limit translation of object via translate tool |
| 1173 | `LimitLookAtTarget` | Boolean | 非GUI | LIVE |  | Whether or not to clamp the look at targets around the avatar head bef |
| 1174 | `LimitLookAtTargetDistance` | F32 | 非GUI | LIVE |  | Distance to limit look at target to (UNUSED) |
| 1175 | `LimitRadarByRange` | Boolean | GUI | LIVE |  | Restrict Radar to a range near you |
| 1176 | `LimitSelectDistance` | Boolean | GUI | LIVE |  | Disallow selection of objects beyond max select distance |
| 1177 | `LinkReplaceBatchPauseTime` | F32 | 非GUI | LIVE |  | The time in seconds between two batches in a link replace operation |
| 1178 | `LinkReplaceBatchSize` | U32 | 非GUI | LIVE |  | The maximum size of a batch in a link replace operation |
| 1179 | `LipSyncAah` | String | 非GUI | LIVE |  | Aah (jaw opening) babble loop |
| 1180 | `LipSyncAahPowerTransfer` | String | 非GUI | LIVE |  | Transfer curve for Voice Interface power to aah lip sync amplitude |
| 1181 | `LipSyncEnabled` | Boolean | GUI | LIVE |  | 0 disable lip-sync, 1 enable babble loop |
| 1182 | `LipSyncOoh` | String | 非GUI | LIVE |  | Ooh (mouth width) babble loop |
| 1183 | `LipSyncOohAahRate` | F32 | 非GUI | LIVE |  | Rate to babble Ooh and Aah (/sec) |
| 1184 | `LipSyncOohPowerTransfer` | String | 非GUI | LIVE |  | Transfer curve for Voice Interface power to ooh lip sync amplitude |
| 1185 | `LocalCacheVersion` | S32 | 非GUI | LIVE |  | Version number of cache |
| 1186 | `LocalFileSystemBrowsingEnabled` | Boolean | 非GUI | LIVE |  | Enable/disable access to the local file system via the file picker |
| 1187 | `LocalTerrainAsset1` | String | 非GUI | DEAD |  | If set to a non-null UUID, overrides the terrain asset locally for all |
| 1188 | `LocalTerrainAsset2` | String | 非GUI | DEAD |  | If set to a non-null UUID, overrides the terrain asset locally for all |
| 1189 | `LocalTerrainAsset3` | String | 非GUI | DEAD |  | If set to a non-null UUID, overrides the terrain asset locally for all |
| 1190 | `LocalTerrainAsset4` | String | 非GUI | DEAD |  | If set to a non-null UUID, overrides the terrain asset locally for all |
| 1191 | `LocalTerrainPaintEnabled` | Boolean | 非GUI | LIVE |  | Enables local paintmap if LocalTerrainAsset1, etc are set |
| 1192 | `LocalTerrainTransform1OffsetU` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset1 is set |
| 1193 | `LocalTerrainTransform1OffsetV` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset1 is set |
| 1194 | `LocalTerrainTransform1Rotation` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset1 is set |
| 1195 | `LocalTerrainTransform1ScaleU` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset1 is set |
| 1196 | `LocalTerrainTransform1ScaleV` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset1 is set |
| 1197 | `LocalTerrainTransform2OffsetU` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset2 is set |
| 1198 | `LocalTerrainTransform2OffsetV` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset2 is set |
| 1199 | `LocalTerrainTransform2Rotation` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset2 is set |
| 1200 | `LocalTerrainTransform2ScaleU` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset2 is set |
| 1201 | `LocalTerrainTransform2ScaleV` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset2 is set |
| 1202 | `LocalTerrainTransform3OffsetU` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset3 is set |
| 1203 | `LocalTerrainTransform3OffsetV` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset3 is set |
| 1204 | `LocalTerrainTransform3Rotation` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset3 is set |
| 1205 | `LocalTerrainTransform3ScaleU` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset3 is set |
| 1206 | `LocalTerrainTransform3ScaleV` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset3 is set |
| 1207 | `LocalTerrainTransform4OffsetU` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset4 is set |
| 1208 | `LocalTerrainTransform4OffsetV` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset4 is set |
| 1209 | `LocalTerrainTransform4Rotation` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset4 is set |
| 1210 | `LocalTerrainTransform4ScaleU` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset4 is set |
| 1211 | `LocalTerrainTransform4ScaleV` | F32 | 非GUI | LIVE?(動的名) |  | KHR texture transform component if LocalTerrainAsset4 is set |
| 1212 | `LockToolbars` | Boolean | GUI | LIVE |  | If enabled, toolbars are locked and buttons can not be dragged around, |
| 1213 | `LogInventoryDecline` | Boolean | 非GUI | LIVE |  | Log in system chat whenever an inventory offer is declined |
| 1214 | `LogMessages` | Boolean | 非GUI | LIVE |  | Log network traffic |
| 1215 | `LogMetrics` | String | 非GUI | LIVE |  | Log viewer metrics |
| 1216 | `LogPerformance` | Boolean | 非GUI | LIVE |  | Log performance analysis for a particular viewer run |
| 1217 | `LogTextureDownloadsToSimulator` | Boolean | 非GUI | LIVE |  | Send a digest of texture info to the region |
| 1218 | `LogTextureDownloadsToViewerLog` | Boolean | 非GUI | LIVE |  | Send texture download details to the viewer log |
| 1219 | `LogTextureNetworkTraffic` | Boolean | 非GUI | LIVE |  | Log network traffic for textures |
| 1220 | `LogWearableAssetSave` | Boolean | 非GUI | LIVE |  | Save copy of saved wearables to log dir |
| 1221 | `LoginContentVersion` | String | 非GUI | LIVE |  | Version of login page web based content to display |
| 1222 | `LoginLocation` | String | GUI | LIVE |  | Default Login location ('last', 'home') preference |
| 1223 | `LoginPage` | String | 非GUI | LIVE |  | Login authentication page. |
| 1224 | `LoginSRVTimeout` | F32 | 非GUI | LIVE |  | Duration in seconds of the login SRV request timeout |
| 1225 | `LosslessJ2CUpload` | Boolean | 非GUI | LIVE |  | Use lossless compression for small image uploads |
| 1226 | `MFAHash` | String | 非GUI | LIVE |  | Override MFA state hash for authentication |
| 1227 | `MainChatbarVisible` | Boolean | GUI | LIVE |  | Internal, volatile control variable to enable/disable the chat button  |
| 1228 | `MainWorkTime` | F32 | 非GUI | LIVE |  | Max time per frame devoted to mainloop work queue (in milliseconds) |
| 1229 | `MainloopTimeoutDefault` | F32 | 非GUI | LIVE |  | Timeout duration for mainloop lock detection during teleports, login a |
| 1230 | `MainloopTimeoutStarted` | F32 | 非GUI | LIVE |  | Timeout duration for mainloop lock detection when logged in and not te |
| 1231 | `MapScale` | F32 | 非GUI | LIVE |  | World map zoom level (pixels per region) |
| 1232 | `MapServerURL` | String | 非GUI | LIVE |  | World map URL template for locating map tiles |
| 1233 | `MapShowEvents` | Boolean | GUI | LIVE |  | Show events on world map |
| 1234 | `MapShowGridCoords` | Boolean | GUI | LIVE |  | Shows/hides the grid coordinates of each region on the world map. |
| 1235 | `MapShowInfohubs` | Boolean | GUI | LIVE |  | Show infohubs on the world map |
| 1236 | `MapShowLandForSale` | Boolean | GUI | LIVE |  | Show land for sale on world map |
| 1237 | `MapShowPeople` | Boolean | GUI | LIVE |  | Show other users on world map |
| 1238 | `MapShowTelehubs` | Boolean | 非GUI | LIVE |  | Show telehubs on world map |
| 1239 | `Marker` | String | 非GUI | DEAD |  | [NOT USED] |
| 1240 | `MarketplaceListingsLogging` | Boolean | 非GUI | LIVE |  | Enable debug output associated with the Marketplace Listings (SLM) API |
| 1241 | `MarketplaceListingsSortOrder` | U32 | 非GUI | LIVE |  | Specifies sort for marketplace listings |
| 1242 | `MarketplaceURL` | String | 非GUI | LIVE |  | URL to the Marketplace |
| 1243 | `MarketplaceURL_bodypartFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Bodyparts Female |
| 1244 | `MarketplaceURL_bodypartMale` | String | 非GUI | DEAD |  | URL to the Marketplace Bodyparts Male |
| 1245 | `MarketplaceURL_clothingFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Clothing Female |
| 1246 | `MarketplaceURL_clothingMale` | String | 非GUI | DEAD |  | URL to the Marketplace Clothing Male |
| 1247 | `MarketplaceURL_eyesFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Eyes Female |
| 1248 | `MarketplaceURL_eyesMale` | String | 非GUI | DEAD |  | URL to the Marketplace Eyes Male |
| 1249 | `MarketplaceURL_glovesFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Gloves Female |
| 1250 | `MarketplaceURL_glovesMale` | String | 非GUI | DEAD |  | URL to the Marketplace Gloves Male |
| 1251 | `MarketplaceURL_hairFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Hair Female |
| 1252 | `MarketplaceURL_hairMale` | String | 非GUI | DEAD |  | URL to the Marketplace Hair Male |
| 1253 | `MarketplaceURL_jacketFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Jacket Female |
| 1254 | `MarketplaceURL_jacketMale` | String | 非GUI | DEAD |  | URL to the Marketplace Jacket Male |
| 1255 | `MarketplaceURL_objectFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Attachments Female |
| 1256 | `MarketplaceURL_objectMale` | String | 非GUI | DEAD |  | URL to the Marketplace Attachments Male |
| 1257 | `MarketplaceURL_pantsFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Pants Female |
| 1258 | `MarketplaceURL_pantsMale` | String | 非GUI | DEAD |  | URL to the Marketplace Pants Male |
| 1259 | `MarketplaceURL_physicsFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Bodyparts Female |
| 1260 | `MarketplaceURL_physicsMale` | String | 非GUI | DEAD |  | URL to the Marketplace Bodyparts Male |
| 1261 | `MarketplaceURL_shapeFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Shape Female |
| 1262 | `MarketplaceURL_shapeMale` | String | 非GUI | DEAD |  | URL to the Marketplace Shape Male |
| 1263 | `MarketplaceURL_shirtFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Shirt Female |
| 1264 | `MarketplaceURL_shirtMale` | String | 非GUI | DEAD |  | URL to the Marketplace Shirt Male |
| 1265 | `MarketplaceURL_shoesFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Shoes Female |
| 1266 | `MarketplaceURL_shoesMale` | String | 非GUI | DEAD |  | URL to the Marketplace Shoes Male |
| 1267 | `MarketplaceURL_skinFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Skin Female |
| 1268 | `MarketplaceURL_skinMale` | String | 非GUI | DEAD |  | URL to the Marketplace Skins Male |
| 1269 | `MarketplaceURL_skirtFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Skirt Female |
| 1270 | `MarketplaceURL_skirtMale` | String | 非GUI | DEAD |  | URL to the Marketplace Skirt Male |
| 1271 | `MarketplaceURL_socksFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Socks Female |
| 1272 | `MarketplaceURL_socksMale` | String | 非GUI | DEAD |  | URL to the Marketplace Socks Male |
| 1273 | `MarketplaceURL_tattooFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Tattoo Female |
| 1274 | `MarketplaceURL_tattooMale` | String | 非GUI | DEAD |  | URL to the Marketplace Tattoo Male |
| 1275 | `MarketplaceURL_underpantsFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Underwear Female |
| 1276 | `MarketplaceURL_underpantsMale` | String | 非GUI | DEAD |  | URL to the Marketplace Underwear Male |
| 1277 | `MarketplaceURL_undershirtFemale` | String | 非GUI | DEAD |  | URL to the Marketplace Undershirt Female |
| 1278 | `MarketplaceURL_undershirtMale` | String | 非GUI | DEAD |  | URL to the Marketplace Undershirt Male |
| 1279 | `MaterialsEveryoneCopy` | Boolean | GUI | LIVE?(動的名) |  | Everyone can copy the newly created GLTF material |
| 1280 | `MaterialsNextOwnerCopy` | Boolean | GUI | LIVE?(動的名) |  | Newly created GLTF material can be copied by next owner |
| 1281 | `MaterialsNextOwnerModify` | Boolean | GUI | LIVE?(動的名) |  | Newly created GLTF material can be modified by next owner |
| 1282 | `MaterialsNextOwnerTransfer` | Boolean | GUI | LIVE?(動的名) |  | Newly created GLTF material can be resold or given away by next owner |
| 1283 | `MaterialsShareWithGroup` | Boolean | GUI | LIVE?(動的名) |  | Newly created GLTF materials are shared with the currently active grou |
| 1284 | `MaxAttachmentComplexity` | F32 | 非GUI | LIVE |  | Attachment's render weight limit |
| 1285 | `MaxDragDistance` | F32 | GUI | LIVE |  | Maximum allowed translation distance in a single operation of translat |
| 1286 | `MaxHeapSize` | F32 | 非GUI | LIVE |  | Maximum heap size on 32-bit builds (GB) |
| 1287 | `MaxHeapSize64` | F32 | 非GUI | LIVE |  | Maximum heap size on 64-bit builds (GB) |
| 1288 | `MaxPersistentNotifications` | S32 | 非GUI | LIVE |  | Maximum amount of persistent notifications |
| 1289 | `MaxSelectDistance` | F32 | GUI | LIVE |  | Maximum allowed selection distance (meters from avatar) |
| 1290 | `MaxWearableWaitTime` | F32 | 非GUI | LIVE |  | Max seconds to wait for wearable assets to fetch. |
| 1291 | `MePanelOpened` | Boolean | 非GUI | DEAD |  | Indicates that Me Panel was opened at least once after Viewer was inst |
| 1292 | `MediaAutoPlayHuds` | Boolean | GUI | LIVE |  | Automatically play HUD media |
| 1293 | `MediaControlFadeTime` | F32 | 非GUI | LIVE |  | Amount of time (in seconds) that the media control fades |
| 1294 | `MediaControlTimeout` | F32 | 非GUI | LIVE |  | Amount of time (in seconds) for media controls to fade with no mouse a |
| 1295 | `MediaEnableFilter` | Boolean | GUI | LIVE |  | Enable media domain filtering |
| 1296 | `MediaEnablePopups` | Boolean | GUI | DEAD |  | If true, enable targeted links and JavaScript in media to open new med |
| 1297 | `MediaFilterSinglePrompt` | Boolean | 非GUI | LIVE |  | Use a single legacy style dialog for media filter prompt, instead of t |
| 1298 | `MediaFirstClickInteract` | S32 | 非GUI | LIVE |  | This setting controls which media (once loaded) does not require a fir |
| 1299 | `MediaOnAPrimUI` | Boolean | 非GUI | LIVE |  | Whether or not to show the "link sharing" UI |
| 1300 | `MediaPerformanceManagerDebug` | Boolean | 非GUI | LIVE |  | Whether to show debug data for the media performance manager in the ne |
| 1301 | `MediaPluginDebugging` | Boolean | 非GUI | LIVE |  | Turn on debugging messages that may help diagnosing media issues (WARN |
| 1302 | `MediaRollOffMax` | F32 | GUI | LIVE |  | Distance at which media volume is set to 0 |
| 1303 | `MediaRollOffMin` | F32 | GUI | LIVE |  | Adjusts the distance at which media attenuation starts |
| 1304 | `MediaRollOffRate` | F32 | 非GUI | LIVE |  | Multiplier to change rate of media attenuation |
| 1305 | `MediaShowOnOthers` | Boolean | GUI | LIVE |  | Whether or not to show media on other avatars |
| 1306 | `MediaShowOutsideParcel` | Boolean | 非GUI | LIVE |  | Whether or not to show media from outside the current parcel |
| 1307 | `MediaShowWithinParcel` | Boolean | 非GUI | LIVE |  | Whether or not to show media within the current parcel |
| 1308 | `MediaSoundsEarLocation` | S32 | GUI | LIVE |  | Location of the virtual ear for media and sounds |
| 1309 | `MediaTentativeAutoPlay` | Boolean | 非GUI | LIVE |  | This is a tentative flag that may be temporarily set off by the user,  |
| 1310 | `MemoryFailurePreventionEnabled` | Boolean | 非GUI | DEAD |  | If set, the viewer will try to throttle memory allocations when memory |
| 1311 | `MemoryLogFrequency` | F32 | 非GUI | LIVE |  | Seconds between display of Memory in log (0 for never) |
| 1312 | `MemoryPrivatePoolEnabled` | Boolean | 非GUI | DEAD |  | (Deprecated) Enable the private memory pool management |
| 1313 | `MemoryPrivatePoolSize` | U32 | 非GUI | DEAD |  | (Deprecated) Size of the private memory pool in MB (min. value is 256) |
| 1314 | `MenuAccessKeyTime` | F32 | 非GUI | LIVE |  | Time (seconds) in which the menu key must be tapped to move focus to t |
| 1315 | `MenuSearch` | Boolean | GUI | LIVE |  | Show/hide 'Search menus' field |
| 1316 | `Mesh2MaxConcurrentRequests` | U32 | 非GUI | LIVE |  | Number of connections to use for loading meshes. |
| 1317 | `MeshBytesPerTriangle` | U32 | 非GUI | LIVE |  | Approximation of bytes per triangle to use for determining mesh stream |
| 1318 | `MeshEnabled` | Boolean | 非GUI | LIVE |  | Expose UI for mesh functionality (may require restart to take effect). |
| 1319 | `MeshImportUseSLM` | Boolean | 非GUI | LIVE |  | Use cached copy of last upload for a dae if available instead of loadi |
| 1320 | `MeshMaxConcurrentRequests` | U32 | 非GUI | LIVE |  | Number of connections to use for loading meshes (legacy system). |
| 1321 | `MeshMetaDataDiscount` | U32 | 非GUI | LIVE |  | Number of bytes to deduct for metadata when determining streaming cost |
| 1322 | `MeshMinimumByteSize` | U32 | 非GUI | LIVE |  | Minimum number of bytes per LoD block when determining streaming cost. |
| 1323 | `MeshPreviewBaseColor` | Color4 | 非GUI | LIVE |  | base diffuse colour for the Mesh uploader |
| 1324 | `MeshPreviewBrightnessColor` | Color3 | 非GUI | LIVE |  | Brightness modifier |
| 1325 | `MeshPreviewCanvasColor` | Color4 | GUI | LIVE |  | Canvas colour for the Mesh uploader |
| 1326 | `MeshPreviewDegenerateEdgeColor` | Color4 | GUI | LIVE |  | Edge colour for the Mesh uploader Degenerate preview |
| 1327 | `MeshPreviewDegenerateEdgeWidth` | F32 | 非GUI | LIVE |  | line thickness used when display Degenerate is selected in mesh previe |
| 1328 | `MeshPreviewDegenerateFillColor` | Color4 | GUI | LIVE |  | Fill colour for the Mesh uploader Degenerate preview |
| 1329 | `MeshPreviewDegeneratePointSize` | F32 | 非GUI | LIVE |  | Large point size used to highlight degenerate triangle vertices in Mes |
| 1330 | `MeshPreviewEdgeColor` | Color4 | GUI | LIVE |  | Edge colour for the Mesh uploader preview |
| 1331 | `MeshPreviewEdgeWidth` | F32 | 非GUI | LIVE |  | line thickness used when display edges is selected in mesh preview |
| 1332 | `MeshPreviewPhysicsEdgeColor` | Color4 | GUI | LIVE |  | Edge colour for the Mesh uploader physics preview |
| 1333 | `MeshPreviewPhysicsEdgeWidth` | F32 | 非GUI | LIVE |  | line thickness used when display physics is selected in mesh preview |
| 1334 | `MeshPreviewPhysicsFillColor` | Color4 | GUI | LIVE |  | Fill colour for the Mesh uploader physics preview |
| 1335 | `MeshPreviewZoomLimit` | F32 | 非GUI | LIVE |  | Maximum Zoom level in preview |
| 1336 | `MeshTriangleBudget` | U32 | 非GUI | LIVE |  | Target visible triangle budget to use when estimating streaming cost. |
| 1337 | `MeshUploadFakeErrors` | S32 | 非GUI | LIVE |  | Force upload errors (for testing) |
| 1338 | `MeshUploadLogXML` | Boolean | 非GUI | LIVE |  | Verbose XML logging on mesh upload |
| 1339 | `MeshUploadTimeOut` | S32 | 非GUI | LIVE |  | Maximum time in seconds for llcurl to execute a mesh uoloading request |
| 1340 | `MeshUseGetMesh1` | Boolean | 非GUI | LIVE |  | If TRUE, use the legacy GetMesh capability for mesh download requests. |
| 1341 | `MeshUseHttpRetryAfter` | Boolean | 非GUI | LIVE |  | If TRUE, use Retry-After response headers when rescheduling a mesh req |
| 1342 | `MigrateCacheDirectory` | Boolean | 非GUI | LIVE |  | Check for old version of disk cache to migrate to current location |
| 1343 | `MinObjectsForUnlinkConfirm` | S32 | GUI | LIVE |  | Minimum amount of objects in linkset for showing confirmation dialog |
| 1344 | `MinWindowHeight` | U32 | 非GUI | LIVE |  | SL viewer minimum window height in pixels |
| 1345 | `MinWindowWidth` | U32 | 非GUI | LIVE |  | SL viewer minimum window width in pixels |
| 1346 | `MiniMapAutoCenter` | Boolean | GUI | LIVE |  | Center the focal point of the minimap. |
| 1347 | `MiniMapChatRing` | Boolean | GUI | LIVE |  | Display chat distance ring on mini map |
| 1348 | `MiniMapCollisionParcels` | Boolean | 非GUI | LIVE |  | Show collision parcels on the mini-map as they become available |
| 1349 | `MiniMapForSaleParcels` | Boolean | GUI | LIVE |  | Show for-sale parcels with a yellow highlight on the mini-map |
| 1350 | `MiniMapObjects` | Boolean | GUI | LIVE |  | Show object layers on the mini-map |
| 1351 | `MiniMapPrimMaxRadius` | F32 | 非GUI | LIVE |  | Radius of the largest prim to show on the MiniMap. Increasing beyond 2 |
| 1352 | `MiniMapPrimMaxVertDistance` | F32 | 非GUI | LIVE |  | Max height difference between avatar and prim to be shown on the MiniM |
| 1353 | `MiniMapRotate` | Boolean | GUI | LIVE |  | Rotate miniature world map to avatar direction |
| 1354 | `MiniMapScale` | F32 | 非GUI | LIVE |  | Miniature world map zoom level (pixels per region) |
| 1355 | `MiniMapShowPropertyLines` | Boolean | GUI | LIVE |  | Whether or not to show parcel borders on the MiniMap. |
| 1356 | `MouseLookEnabled` | Boolean | GUI | LIVE |  | Internal, volatile control variable to show if we are currently in mou |
| 1357 | `MouseMoon` | Boolean | 非GUI | LIVE |  |  |
| 1358 | `MouseSensitivity` | F32 | GUI | LIVE |  | Controls responsiveness of mouse when in mouselook mode (fraction or m |
| 1359 | `MouseSmooth` | Boolean | GUI | LIVE |  | Smooths out motion of mouse when in mouselook mode. |
| 1360 | `MouseSun` | Boolean | 非GUI | LIVE |  |  |
| 1361 | `MouseWarpMode` | S32 | GUI | LIVE |  | Controls warping of the mouse to the center of the screen during alt-z |
| 1362 | `MultiModeDoubleClickFolder` | U32 | GUI | LIVE |  | Sets the action for Double-click on folder in multi-folder view (0 - e |
| 1363 | `MuteAmbient` | Boolean | GUI | LIVE |  | Ambient sound effects, such as wind noise, play at 0 volume |
| 1364 | `MuteAudio` | Boolean | GUI | LIVE |  | All audio plays at 0 volume (streaming audio still takes up bandwidth, |
| 1365 | `MuteListLimit` | S32 | 非GUI | LIVE |  | Maximum number of entries in the mute list |
| 1366 | `MuteMedia` | Boolean | GUI | LIVE |  | Media plays at 0 volume (streaming audio still takes up bandwidth) |
| 1367 | `MuteMusic` | Boolean | GUI | LIVE |  | Music plays at 0 volume (streaming audio still takes up bandwidth) |
| 1368 | `MuteSounds` | Boolean | GUI | LIVE |  | Sound effects play at 0 volume |
| 1369 | `MuteUI` | Boolean | GUI | LIVE |  | UI sound effects play at 0 volume |
| 1370 | `MuteVoice` | Boolean | GUI | LIVE |  | Voice plays at 0 volume (streaming audio still takes up bandwidth) |
| 1371 | `MuteWhenMinimized` | Boolean | GUI | LIVE |  | Mute audio when SL window is minimized |
| 1372 | `NameTagDebugAVRezState` | Boolean | GUI(menu) | LIVE |  | Show Avatar Rez state in name tag |
| 1373 | `NameTagShowDisplayNames` | Boolean | GUI | LIVE |  | Show display names in name labels |
| 1374 | `NameTagShowFriends` | Boolean | GUI | LIVE |  | Highlight the name tags of your friends |
| 1375 | `NameTagShowGroupTitles` | Boolean | GUI | LIVE |  | Show group titles in name labels |
| 1376 | `NameTagShowUsernames` | Boolean | GUI | LIVE |  | Show usernames in avatar name tags |
| 1377 | `NavBarShowCoordinates` | Boolean | GUI | LIVE |  | Show coordinates in navigation bar |
| 1378 | `NavBarShowParcelProperties` | Boolean | GUI | LIVE |  | Show parcel property icons in navigation bar |
| 1379 | `NearMeRange` | F32 | GUI | LIVE |  | Search radius for nearby avatars |
| 1380 | `NearbyListHideUsernames` | Boolean | 非GUI | LIVE |  | Show both Display name and Username in Nearby list |
| 1381 | `NearbyListShowIcons` | Boolean | GUI(menu) | LIVE |  | Show/hide people icons in nearby list |
| 1382 | `NearbyListShowMap` | Boolean | GUI(menu) | DEAD |  | Show/hide map above nearby people list (unused by firestorm) |
| 1383 | `NearbyListShowRange` | Boolean | 非GUI | DEAD |  | Show range field on nearby avList? |
| 1384 | `NearbyPeopleSortOrder` | U32 | 非GUI | LIVE |  | Specifies sort order for nearby people (0 = by name, 3 = by distance,  |
| 1385 | `NearbyToastFadingTime` | S32 | GUI | LIVE |  | Number of seconds while a nearby chat toast is fading |
| 1386 | `NearbyToastLifeTime` | S32 | GUI | LIVE |  | Number of seconds while a nearby chat toast exists |
| 1387 | `NearbyToastWidth` | S32 | GUI | LIVE |  | Width of a the nearby chat toasts in percent of screen width |
| 1388 | `NewCacheLocation` | String | 非GUI | LIVE |  | Change the location of the local disk cache to this |
| 1389 | `NewCacheLocationTopFolder` | String | 非GUI | LIVE |  | Change the top folder location of the local disk cache to this |
| 1390 | `NewObjectCreationThrottle` | S32 | 非GUI | LIVE |  | maximum number of new objects created per frame, -1 to disable this th |
| 1391 | `NewObjectCreationThrottleDelayTime` | F32 | 非GUI | LIVE |  | time in seconds NewObjectCreationThrottle to take effect after the pro |
| 1392 | `NextLoginLocation` | String | GUI | LIVE |  | Location to log into for this session - set from command line or the l |
| 1393 | `NextOwnerCopy` | Boolean | 非GUI | LIVE |  | (obsolete) Newly created objects can be copied by next owner |
| 1394 | `NextOwnerModify` | Boolean | 非GUI | LIVE |  | (obsolete) Newly created objects can be modified by next owner |
| 1395 | `NextOwnerTransfer` | Boolean | 非GUI | LIVE |  | (obsolete) Newly created objects can be resold or given away by next o |
| 1396 | `NoAudio` | Boolean | 非GUI | LIVE |  | Disable audio playback. |
| 1397 | `NoHardwareProbe` | Boolean | 非GUI | LIVE |  | Disable hardware probe. |
| 1398 | `NoInventoryLibrary` | Boolean | 非GUI | LIVE |  | (Deprecated) Do not request inventory library. |
| 1399 | `NoPreload` | Boolean | 非GUI | LIVE |  | Disable sound and image preload. |
| 1400 | `NoQuickTime` | Boolean | 非GUI | LIVE(XML専用) |  | Disable QuickTime for a particular viewer run |
| 1401 | `NoVerifySSLCert` | Boolean | 非GUI | LIVE |  | Do not verify SSL peers. |
| 1402 | `NonInteractive` | Boolean | 非GUI | LIVE |  | Run in a semi-headless mode where only logging in and logging out need |
| 1403 | `NonvisibleObjectsInMemoryTime` | U32 | 非GUI | LIVE |  | Number of frames non-visible objects stay in memory before being remov |
| 1404 | `NotMovingHintTimeout` | F32 | 非GUI | LIVE |  | Number of seconds to wait for resident to move before displaying move  |
| 1405 | `NotecardsEveryoneCopy` | Boolean | GUI | LIVE?(動的名) |  | Everyone can copy the newly created notecard |
| 1406 | `NotecardsNextOwnerCopy` | Boolean | GUI | LIVE?(動的名) |  | Newly created notecards can be copied by next owner |
| 1407 | `NotecardsNextOwnerModify` | Boolean | GUI | LIVE?(動的名) |  | Newly created notecards can be modified by next owner |
| 1408 | `NotecardsNextOwnerTransfer` | Boolean | GUI | LIVE?(動的名) |  | Newly created notecards can be resold or given away by next owner |
| 1409 | `NotecardsShareWithGroup` | Boolean | GUI | LIVE?(動的名) |  | Newly created notecards are shared with the currently active group |
| 1410 | `NotificationCanEmbedInIM` | S32 | 非GUI | DEAD |  | Controls notification panel embedding in IMs (0 = default, 1 = focused |
| 1411 | `NotificationChannelHeightRatio` | F32 | 非GUI | LIVE |  | Notification channel and World View ratio(0.0 - always show 1 notifica |
| 1412 | `NotificationChannelRightMargin` | S32 | 非GUI | LIVE |  | Space between toasts and a right border of an area where they can appe |
| 1413 | `NotificationConferenceIMOptions` | String | 非GUI | LIVE |  | Specifies how the UI responds to Conference IM Notifications.          |
| 1414 | `NotificationFriendIMOptions` | String | 非GUI | LIVE |  | Specifies how the UI responds to Friend IM Notifications.         Allo |
| 1415 | `NotificationGroupChatOptions` | String | 非GUI | LIVE |  | Specifies how the UI responds to Group Chat Notifications.         All |
| 1416 | `NotificationNearbyChatOptions` | String | 非GUI | LIVE |  | Specifies how the UI responds to Nearby Chat Notifications.         Al |
| 1417 | `NotificationNonFriendIMOptions` | String | 非GUI | LIVE |  | Specifies how the UI responds to Non Friend IM Notifications.          |
| 1418 | `NotificationObjectIMOptions` | String | 非GUI | LIVE |  | Specifies how the UI responds to Object IM Notifications.         Allo |
| 1419 | `NotificationTipToastLifeTime` | S32 | GUI | LIVE |  | Number of seconds while a notification tip toast exist |
| 1420 | `NotificationToastLifeTime` | S32 | GUI | LIVE |  | Number of seconds while a notification toast exists |
| 1421 | `NotifyMoneyChange` | Boolean | GUI | LIVE |  | Pop up notifications for all L$ transactions |
| 1422 | `NotifyMoneyReceived` | Boolean | 非GUI | LIVE |  | Pop up notifications when receiving L$ |
| 1423 | `NotifyMoneySpend` | Boolean | 非GUI | LIVE |  | Pop up notifications when spending L$ |
| 1424 | `NotifyTipDuration` | F32 | 非GUI | DEAD |  | Length of time that notification tips stay on screen (seconds) |
| 1425 | `NumSessions` | S32 | 非GUI | LIVE |  | Number of successful logins to Second Life |
| 1426 | `NvAPICreateApplicationProfile` | Boolean | 非GUI | LIVE |  | Create NVIDIA application profile for optimized settings |
| 1427 | `ObjectCacheEnabled` | Boolean | 非GUI | LIVE |  | Enable the object cache. |
| 1428 | `ObjectCostHighColor` | Color4 | 非GUI | LIVE |  | Color for object a high object cost. |
| 1429 | `ObjectCostHighThreshold` | F32 | 非GUI | LIVE |  | Threshold at which object cost is considered high (displayed in red). |
| 1430 | `ObjectCostLowColor` | Color4 | 非GUI | LIVE |  | Color for object with a low object cost. |
| 1431 | `ObjectCostMidColor` | Color4 | 非GUI | LIVE |  | Color for object with a medium object cost. |
| 1432 | `ObjectInspectorTooltipDelay` | F32 | GUI | LIVE |  | Seconds before displaying object inspector tooltip |
| 1433 | `ObjectsEveryoneCopy` | Boolean | GUI | LIVE?(動的名) |  | Everyone can copy the newly created object |
| 1434 | `ObjectsNextOwnerCopy` | Boolean | GUI | LIVE |  | Newly created objects can be copied by next owner |
| 1435 | `ObjectsNextOwnerModify` | Boolean | GUI | LIVE |  | Newly created objects can be modified by next owner |
| 1436 | `ObjectsNextOwnerTransfer` | Boolean | GUI | LIVE |  | Newly created objects can be resold or given away by next owner |
| 1437 | `ObjectsShareWithGroup` | Boolean | GUI | LIVE?(動的名) |  | Newly created objects are shared with the currently active group |
| 1438 | `ObscureBalanceInStatusBar` | Boolean | 非GUI | LIVE |  | If true, balance will be shows as '*' (UNUSED) |
| 1439 | `OctreeAlphaDistanceFactor` | Vector3 | 非GUI | LIVE |  | Multiplier on alpha object distance for determining octree node size.  |
| 1440 | `OctreeAttachmentSizeFactor` | S32 | 非GUI | LIVE |  | Multiplier on attachment size for determining octree node size |
| 1441 | `OctreeDistanceFactor` | Vector3 | 非GUI | LIVE |  | Multiplier on distance for determining octree node size |
| 1442 | `OctreeMaxNodeCapacity` | U32 | 非GUI | LIVE |  | Maximum number of elements to store in a single octree node |
| 1443 | `OctreeMinimumNodeSize` | F32 | 非GUI | LIVE |  | Minimum size of any octree node |
| 1444 | `OctreeStaticObjectSizeFactor` | S32 | 非GUI | LIVE |  | Multiplier on static object size for determining octree node size |
| 1445 | `OmnifilterEnabled` | Boolean | GUI | LIVE |  | The operating state of the Omnifilter. |
| 1446 | `OnlineOfflinetoNearbyChat` | Boolean | GUI | LIVE |  | Send online/offline notifications to Nearby Chat panel (v1-style behav |
| 1447 | `OnlineOfflinetoNearbyChatHistory` | Boolean | GUI | LIVE |  | Show online/offline notifications only in chat transcript |
| 1448 | `OnlyShowSelectedNormals` | Boolean | 非GUI | LIVE |  | Only render the normals for selected objects. in conjunction with rend |
| 1449 | `OpenDebugStatAdvanced` | Boolean | GUI | LIVE(UI framework) |  | Expand Advanced performance stats display |
| 1450 | `OpenDebugStatBasic` | Boolean | GUI | LIVE(UI framework) |  | Expand Basic performance stats display |
| 1451 | `OpenDebugStatMaterials` | Boolean | GUI | LIVE(UI framework) |  | Expand Materials performance stats display |
| 1452 | `OpenDebugStatNet` | Boolean | GUI | LIVE(UI framework) |  | Expand Network performance stats display |
| 1453 | `OpenDebugStatPathfinding` | Boolean | GUI | LIVE(UI framework) |  | Expand Pathfinding performance stats display |
| 1454 | `OpenDebugStatPhysicsDetails` | Boolean | GUI | LIVE(UI framework) |  | Expand Physics Details performance stats display |
| 1455 | `OpenDebugStatRender` | Boolean | GUI | LIVE(UI framework) |  | Expand Render performance stats display |
| 1456 | `OpenDebugStatSim` | Boolean | GUI | LIVE(UI framework) |  | Expand Simulator performance stats display |
| 1457 | `OpenDebugStatSimTime` | Boolean | GUI | LIVE(UI framework) |  | Expand Simulator Time performance stats display |
| 1458 | `OpenDebugStatSimTimeDetails` | Boolean | GUI | LIVE(UI framework) |  | Expand Simulator Time Details performance stats display |
| 1459 | `OpenDebugStatTexture` | Boolean | GUI | LIVE(UI framework) |  | Expand Texture performance stats display |
| 1460 | `OpenIMOnVoice` | Boolean | 非GUI | LIVE |  | Open the corresponding IM window when connecting to a voice call. |
| 1461 | `OpenRegionSettingsEnableDrawDistance` | Boolean | 非GUI | LIVE |  | Obey the forced max draw distance setting in aurora-sim |
| 1462 | `OpenSidePanelsInFloaters` | Boolean | 非GUI | DEAD |  | If true, will always open side panel contents in a floater. |
| 1463 | `OutBandwidth` | F32 | 非GUI | LIVE(XML専用) |  | Outgoing bandwidth throttle (bps) |
| 1464 | `OutfitGallerySortOrder` | S32 | 非GUI | LIVE |  | Gallery sorting: 0 - sort outfits by name, 1 - images frst, 2 - favori |
| 1465 | `OutfitListFilterFullList` | Boolean | 非GUI | LIVE |  | 0 - show only matches. 1 - show all items in outfit as long as outfit  |
| 1466 | `OutfitListSortOrder` | S32 | 非GUI | LIVE |  | How outfit list in Avatar's floater is sorted. 0 - by name 1 - favorit |
| 1467 | `OutfitOperationsTimeout` | S32 | 非GUI | LIVE |  | Timeout for outfit related operations. |
| 1468 | `OverflowToastHeight` | S32 | GUI | DEAD |  | Height of an overflow toast |
| 1469 | `OverlayTitle` | String | 非GUI | LIVE |  | Controls watermark text message displayed on screen when "ShowOverlayT |
| 1470 | `OverridePieColors` | Boolean | GUI | LIVE |  | Override the pie menu color defined by the currently selected skin |
| 1471 | `PTTCurrentlyEnabled` | Boolean | 非GUI | LIVE |  | Use Push to Talk mode |
| 1472 | `PacketDropPercentage` | F32 | 非GUI | LIVE |  | Percentage of packets dropped by the client. |
| 1473 | `ParcelHideEnabled` | Boolean | GUI | LIVE |  | Hide objects that are outside the agent's current parcel |
| 1474 | `ParcelHideKeepAvatars` | Boolean | GUI | LIVE |  | When ParcelHideEnabled is on, still render avatars, attachments and HU |
| 1475 | `ParcelHideKeepOwn` | Boolean | GUI | LIVE |  | When ParcelHideEnabled is on, still render objects owned by the agent  |
| 1476 | `ParcelMediaAutoPlayEnable` | S32 | GUI | LIVE |  | Auto play parcel media when available. 0 - Do not autoplay; 1- Autopla |
| 1477 | `ParticipantListShowIcons` | Boolean | GUI(menu) | LIVE |  | Show/hide people icons in participant list |
| 1478 | `PathfindingAmbiance` | F32 | 非GUI | LIVE | ✅ | Ambiance of lit pathfinding navmesh displays. |
| 1479 | `PathfindingBoundaryEdge` | Color4 | 非GUI | LIVE |  | Color of a boundary (non-crossable) edge when displaying pathfinding n |
| 1480 | `PathfindingConnectedEdge` | Color4 | 非GUI | LIVE |  | Color of a connected (crossable) edge when displaying pathfinding navm |
| 1481 | `PathfindingExclusion` | Color4 | 非GUI | LIVE |  | Color of exclusion volumes when displaying pathfinding navmesh object  |
| 1482 | `PathfindingFaceColor` | Color4 | 非GUI | LIVE |  | Color of the faces when displaying the default view of the pathfinding |
| 1483 | `PathfindingHeatColorBase` | Color4 | 非GUI | LIVE |  | Color of the least walkable value when displaying the pathfinding navm |
| 1484 | `PathfindingHeatColorMax` | Color4 | 非GUI | LIVE |  | Color of the most walkable value when displaying the pathfinding navme |
| 1485 | `PathfindingLineOffset` | F32 | 非GUI | LIVE |  | Depth offset of volume outlines in pathfinding display. |
| 1486 | `PathfindingLineWidth` | F32 | 非GUI | LIVE |  | Width of volume outlines in pathfinding navmesh display. |
| 1487 | `PathfindingMaterial` | Color4 | 非GUI | LIVE |  | Color of material volumes when displaying pathfinding navmesh object t |
| 1488 | `PathfindingNavMeshClear` | Color4 | 非GUI | LIVE |  | Background color when displaying pathfinding navmesh. |
| 1489 | `PathfindingObstacle` | Color4 | 非GUI | LIVE |  | Color of static obstacle objects when displaying pathfinding navmesh o |
| 1490 | `PathfindingRetrieveNeighboringRegion` | U32 | 非GUI | LIVE |  | Download a neighboring region when visualizing a pathfinding navmesh ( |
| 1491 | `PathfindingTestPathColor` | Color4 | 非GUI | LIVE |  | Color of the pathfinding test-path when the path is valid. |
| 1492 | `PathfindingTestPathInvalidEndColor` | Color4 | 非GUI | LIVE |  | Color of the pathfinding test-pathing tool end-point when the path is  |
| 1493 | `PathfindingTestPathValidEndColor` | Color4 | 非GUI | LIVE |  | Color of the pathfinding test-pathing tool end-point when the path is  |
| 1494 | `PathfindingWalkable` | Color4 | 非GUI | LIVE |  | Color of walkable objects when displaying pathfinding navmesh object t |
| 1495 | `PathfindingWaterColor` | Color4 | 非GUI | LIVE |  | Color of water plane when displaying pathfinding navmesh. |
| 1496 | `PathfindingXRayOpacity` | F32 | 非GUI | LIVE | ✅ | Opacity of xray lines in pathfinding display. |
| 1497 | `PathfindingXRayTint` | F32 | 非GUI | LIVE | ✅ | Amount to darken/lighten x-ray lines in pathfinding display. |
| 1498 | `PathfindingXRayWireframe` | Boolean | 非GUI | LIVE | ✅ | Render pathfinding navmesh xray as a wireframe. |
| 1499 | `PerAccountSettingsFile` | String | GUI | LIVE |  | Persisted client settings file name (per user). |
| 1500 | `PerfStatsCaptureEnabled` | Boolean | 非GUI | LIVE |  | Enable/disable render time data to support autotune. |
| 1501 | `PermAllowScriptedMedia` | Boolean | GUI | LIVE |  | Allow scripts to control media |
| 1502 | `PermissionsCautionEnabled` | Boolean | 非GUI | LIVE |  | When enabled, changes the handling of script permission requests to he |
| 1503 | `PermissionsCautionNotifyBoxHeight` | S32 | 非GUI | DEAD |  | Height of caution-style notification messages |
| 1504 | `PickerContextOpacity` | F32 | 非GUI | LIVE |  | Controls overall opacity of context frustrum connecting color and text |
| 1505 | `PicksPerSecondMouseMoving` | F32 | 非GUI | DEAD |  | How often to perform hover picks while the mouse is moving (picks per  |
| 1506 | `PicksPerSecondMouseStationary` | F32 | 非GUI | DEAD |  | How often to perform hover picks while the mouse is stationary (picks  |
| 1507 | `PieMenuFade` | F32 | GUI | LIVE |  | Fade out for the pie menu background towards the edges |
| 1508 | `PieMenuLineWidth` | F32 | 非GUI | DEAD |  | Width of lines in pie menu display (pixels) |
| 1509 | `PieMenuOpacity` | F32 | GUI | LIVE |  | Opacity for the pie menu background |
| 1510 | `PieMenuOuterRingShade` | Boolean | GUI | LIVE |  | If enabled, a shade around the outside of the pie menu will be drawn,  |
| 1511 | `PieMenuPopupFontEffect` | Boolean | GUI | LIVE |  | If enabled, the labels in the pie menu slices are affected by the popu |
| 1512 | `PingInterpolate` | Boolean | GUI(menu) | LIVE |  | Extrapolate object position along velocity vector based on ping delay |
| 1513 | `PitchFromMousePosition` | F32 | GUI | LIVE |  | Vertical range over which avatar head tracks mouse position (degrees o |
| 1514 | `PlainTextChatHistory` | Boolean | GUI | LIVE |  | Enable/Disable plain text chat transcript style |
| 1515 | `PlayChatAnim` | Boolean | GUI | LIVE |  | Your avatar plays the chat animation whenever you say, shout or whispe |
| 1516 | `PlayModeUISndAlert` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndAlert. |
| 1517 | `PlayModeUISndBadKeystroke` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndBadKeystroke. |
| 1518 | `PlayModeUISndChatMention` | Boolean | 非GUI | LIVE |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndChatMention. |
| 1519 | `PlayModeUISndClick` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndClick. |
| 1520 | `PlayModeUISndClickRelease` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndClickRelease. |
| 1521 | `PlayModeUISndFootsteps` | Boolean | 非GUI | LIVE |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndFootsteps. |
| 1522 | `PlayModeUISndFriendOffline` | Boolean | GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndFriendOffline. |
| 1523 | `PlayModeUISndFriendOnline` | Boolean | GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndFriendOnline. |
| 1524 | `PlayModeUISndFriendshipOffer` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndFriendshipOffer |
| 1525 | `PlayModeUISndGroupInvitation` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndGroupInvitation |
| 1526 | `PlayModeUISndGroupNotice` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndGroupNotice. |
| 1527 | `PlayModeUISndHealthReductionF` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndHealthReduction |
| 1528 | `PlayModeUISndHealthReductionM` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndHealthReduction |
| 1529 | `PlayModeUISndIncomingVoiceCall` | Boolean | 非GUI | LIVE?(動的名) |  | Plays a sound when have an incoming voice call. Holds state for Prefs  |
| 1530 | `PlayModeUISndInvalidOp` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndInvalidOp. |
| 1531 | `PlayModeUISndInventoryOffer` | Boolean | 非GUI | LIVE |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndInventoryOffer. |
| 1532 | `PlayModeUISndMicToggle` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndMicToggle. |
| 1533 | `PlayModeUISndMoneyChangeDown` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndMoneyChangeDown |
| 1534 | `PlayModeUISndMoneyChangeUp` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndMoneyChangeUp. |
| 1535 | `PlayModeUISndMovelockToggle` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndMovelockToggle. |
| 1536 | `PlayModeUISndNearbyChat` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndNearbyChat. |
| 1537 | `PlayModeUISndNewIncomingConfIMSession` | U32 | 非GUI | LIVE |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndNewIncomingConf |
| 1538 | `PlayModeUISndNewIncomingGroupIMSession` | U32 | 非GUI | LIVE |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndNewIncomingGrou |
| 1539 | `PlayModeUISndNewIncomingIMSession` | U32 | 非GUI | LIVE |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndNewIncomingIMSe |
| 1540 | `PlayModeUISndObjectCreate` | Boolean | 非GUI | LIVE |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndObjectCreate. |
| 1541 | `PlayModeUISndObjectDelete` | Boolean | 非GUI | LIVE |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndObjectDelete. |
| 1542 | `PlayModeUISndObjectRezIn` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndObjectRezIn. |
| 1543 | `PlayModeUISndObjectRezOut` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndObjectRezOut. |
| 1544 | `PlayModeUISndPieMenuAppear` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndPieMenuAppear. |
| 1545 | `PlayModeUISndPieMenuHide` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndPieMenuHide. |
| 1546 | `PlayModeUISndPieMenuSliceHighlight0` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndPieMenuSliceHig |
| 1547 | `PlayModeUISndPieMenuSliceHighlight1` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndPieMenuSliceHig |
| 1548 | `PlayModeUISndPieMenuSliceHighlight2` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndPieMenuSliceHig |
| 1549 | `PlayModeUISndPieMenuSliceHighlight3` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndPieMenuSliceHig |
| 1550 | `PlayModeUISndPieMenuSliceHighlight4` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndPieMenuSliceHig |
| 1551 | `PlayModeUISndPieMenuSliceHighlight5` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndPieMenuSliceHig |
| 1552 | `PlayModeUISndPieMenuSliceHighlight6` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndPieMenuSliceHig |
| 1553 | `PlayModeUISndPieMenuSliceHighlight7` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndPieMenuSliceHig |
| 1554 | `PlayModeUISndQuestionExperience` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndQuestionExperie |
| 1555 | `PlayModeUISndRadarAgeAlert` | Boolean | GUI | LIVE?(動的名) |  | If enabled: plays the sound when the age alert for an avatar is trigge |
| 1556 | `PlayModeUISndRadarChatEnter` | Boolean | GUI | LIVE?(動的名) |  | If enabled: plays the sound when avatars enter chat range. Also depend |
| 1557 | `PlayModeUISndRadarChatLeave` | Boolean | GUI | LIVE?(動的名) |  | If enabled: plays the sound when avatars leave chat range. Also depend |
| 1558 | `PlayModeUISndRadarDrawEnter` | Boolean | GUI | LIVE?(動的名) |  | If enabled: plays the sound when avatars enter draw distance. Also dep |
| 1559 | `PlayModeUISndRadarDrawLeave` | Boolean | GUI | LIVE?(動的名) |  | If enabled: plays the sound when avatars leave draw distance. Also dep |
| 1560 | `PlayModeUISndRadarSimEnter` | Boolean | GUI | LIVE?(動的名) |  | If enabled: plays the sound when avatars enter the region. Also depend |
| 1561 | `PlayModeUISndRadarSimLeave` | Boolean | GUI | LIVE?(動的名) |  | If enabled: plays the sound when avatars leave the region. Also depend |
| 1562 | `PlayModeUISndRestart` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndRestart. |
| 1563 | `PlayModeUISndRestartOpenSim` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndRestartOpenSim. |
| 1564 | `PlayModeUISndScriptFloaterClose` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndScriptFloaterCl |
| 1565 | `PlayModeUISndScriptFloaterOpen` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndScriptFloaterOp |
| 1566 | `PlayModeUISndSnapshot` | Boolean | GUI(menu) | LIVE |  | Take snapshots to disk without playing animation or sound. Originally  |
| 1567 | `PlayModeUISndStartIM` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndStartIM. |
| 1568 | `PlayModeUISndTeleportOffer` | Boolean | 非GUI | LIVE |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndTeleportOffer. |
| 1569 | `PlayModeUISndTeleportOut` | Boolean | GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndTeleportOut. Pl |
| 1570 | `PlayModeUISndTrackerBeacon` | Boolean | 非GUI | LIVE |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndTrackerBeacon. |
| 1571 | `PlayModeUISndTyping` | Boolean | GUI | LIVE |  | Hear the typing sound when others type in to local chat. Originally FS |
| 1572 | `PlayModeUISndWindowClose` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndWindowClose. |
| 1573 | `PlayModeUISndWindowOpen` | Boolean | 非GUI | LIVE?(動的名) |  | Holds state for Prefs > Sound/Media > UI Sounds - UISndWindowOpen. |
| 1574 | `PlaySoundChatMention` | Boolean | 非GUI | LIVE |  | Plays a sound when got mentioned in a chat - UNUSED IN FS - see PlayMo |
| 1575 | `PlaySoundConferenceIM` | Boolean | 非GUI | LIVE |  | Plays a sound when conference IM received. |
| 1576 | `PlaySoundFriendIM` | Boolean | 非GUI | LIVE |  | Plays a sound when friend's IM received. |
| 1577 | `PlaySoundGroupChatIM` | Boolean | 非GUI | LIVE |  | Plays a sound when group chat IM received. |
| 1578 | `PlaySoundNearbyChatIM` | Boolean | 非GUI | LIVE |  | Plays a sound when nearby chat IM received. |
| 1579 | `PlaySoundNonFriendIM` | Boolean | 非GUI | LIVE |  | Plays a sound when non-friend's IM received. |
| 1580 | `PlaySoundObjectIM` | Boolean | 非GUI | LIVE |  | Plays a sound when IM fom an object received. |
| 1581 | `PlayTypingAnim` | Boolean | GUI | LIVE |  | Your avatar plays the typing animation whenever you type in the chat b |
| 1582 | `PluginAttachDebuggerToPlugins` | Boolean | 非GUI | LIVE |  | If true, attach a debugger session to each plugin process as it's laun |
| 1583 | `PluginInstancesCPULimit` | F32 | 非GUI | LIVE |  | Amount of total plugin CPU usage before inworld plugins start getting  |
| 1584 | `PluginInstancesLow` | U32 | 非GUI | LIVE |  | Limit on the number of inworld media plugins that will run at "low" pr |
| 1585 | `PluginInstancesNormal` | U32 | 非GUI | LIVE |  | Limit on the number of inworld media plugins that will run at "normal" |
| 1586 | `PluginInstancesTotal` | U32 | 非GUI | LIVE |  | Hard limit on the number of plugins that will be instantiated at once  |
| 1587 | `PluginUseReadThread` | Boolean | GUI(menu) | LIVE |  | Use a separate thread to read incoming messages from plugins |
| 1588 | `PoolSizeAIS` | U32 | 非GUI | LIVE |  | Coroutine Pool size for AIS |
| 1589 | `PoolSizeAssetStorage` | U32 | 非GUI | LIVE?(動的名) |  | Coroutine Pool size for AssetStorage requests |
| 1590 | `PoolSizeUpload` | U32 | 非GUI | LIVE?(動的名) |  | Coroutine Pool size for Upload |
| 1591 | `PostFirstLoginIntroURL` | String | 非GUI | LIVE |  | URL of intro presentation after first time users first login |
| 1592 | `PostFirstLoginIntroViewed` | Boolean | 非GUI | LIVE |  | Flag indicating if user has seen intro presentation after first time u |
| 1593 | `PrecachingDelay` | F32 | GUI | LIVE |  | Delay when logging in to load world before showing it (seconds) |
| 1594 | `PreferredBrowserBehavior` | U32 | GUI | LIVE |  | Use system browser for any links (0), use builtin browser for SL links |
| 1595 | `PreferredMaturity` | U32 | GUI | LIVE |  | Setting for the user's preferred maturity level (consts in indra_const |
| 1596 | `PresetCameraActive` | String | 非GUI | LIVE |  | Name of currently selected preference |
| 1597 | `PresetGraphicActive` | String | 非GUI | LIVE |  | Name of currently selected preference |
| 1598 | `PreviewAmbientColor` | Color4 | GUI | LIVE |  | Ambient color of preview render. |
| 1599 | `PreviewDiffuse0` | Color4 | 非GUI | LIVE |  | Diffuse color of preview light 0. |
| 1600 | `PreviewDiffuse1` | Color4 | 非GUI | LIVE |  | Diffuse color of preview light 1. |
| 1601 | `PreviewDiffuse2` | Color4 | 非GUI | LIVE |  | Diffuse color of preview light 2. |
| 1602 | `PreviewDirection0` | Vector3 | 非GUI | LIVE |  | Direction of light 0 for preview render. |
| 1603 | `PreviewDirection1` | Vector3 | 非GUI | LIVE |  | Direction of light 1 for preview render. |
| 1604 | `PreviewDirection2` | Vector3 | 非GUI | LIVE |  | Direction of light 2 for preview render. |
| 1605 | `PreviewRenderSize` | S32 | 非GUI | LIVE |  | Resolution of the image rendered for the mesh upload preview (must be  |
| 1606 | `PreviewSpecular0` | Color4 | 非GUI | LIVE |  | Diffuse color of preview light 0. |
| 1607 | `PreviewSpecular1` | Color4 | 非GUI | LIVE |  | Diffuse color of preview light 1. |
| 1608 | `PreviewSpecular2` | Color4 | 非GUI | LIVE |  | Diffuse color of preview light 2. |
| 1609 | `PreviousInstallChecked` | Boolean | 非GUI | LIVE |  | Whether viewer checked previous install on the same channel for NSIS |
| 1610 | `PrimMediaControlsUseHoverControlSet` | Boolean | 非GUI | LIVE |  | Whether or not hovering over prim media uses minimal "hover" controls  |
| 1611 | `PrimMediaDragNDrop` | Boolean | 非GUI | LIVE |  | Enable drag and drop of URLs onto prim faces |
| 1612 | `PrimMediaMasterEnabled` | Boolean | 非GUI | LIVE |  | Whether or not Media on a Prim is enabled. |
| 1613 | `PrimMediaMaxRetries` | U32 | 非GUI | LIVE |  | Maximum number of retries for media queries. |
| 1614 | `PrimMediaMaxRoundRobinQueueSize` | U32 | 非GUI | LIVE |  | Maximum number of objects the viewer will continuously update media fo |
| 1615 | `PrimMediaMaxSortedQueueSize` | U32 | 非GUI | LIVE |  | Maximum number of objects the viewer will load media for initially |
| 1616 | `PrimMediaRequestQueueDelay` | F32 | 非GUI | LIVE |  | Timer delay for fetching media from the queue (in seconds). |
| 1617 | `PrimMediaRetryTimerDelay` | F32 | 非GUI | LIVE |  | Timer delay for retrying on media queries (in seconds). |
| 1618 | `PrimTextMaxDrawDistance` | F32 | 非GUI | LIVE |  | Maximum draw distance beyond which PRIM_TEXT won't be rendered |
| 1619 | `PrivateLocalLookAtTarget` | Boolean | GUI | LIVE |  | If true, your avatar's lookat target will not affect your local displa |
| 1620 | `PrivateLookAtTarget` | Boolean | GUI | LIVE |  | If true, viewer shows simulated look-at behavior to others. |
| 1621 | `PrivatePointAtTarget` | Boolean | GUI | LIVE |  | If true, viewer won't show the editing arm motion. |
| 1622 | `ProbeHardwareOnStartup` | Boolean | 非GUI | LIVE |  | Query current hardware configuration on application startup |
| 1623 | `ProfilingActive` | Boolean | GUI(menu) | LIVE |  | notes whether profiling is currently active. Used by the dev menu item |
| 1624 | `PurgeCacheOnNextStartup` | Boolean | 非GUI | LIVE |  | Clear local file cache next time viewer is run |
| 1625 | `PurgeCacheOnStartup` | Boolean | 非GUI | LIVE |  | Clear local file cache every time viewer is run |
| 1626 | `PushToTalkButton` | String | 非GUI | LIVE |  | (Obsolete)Which button or keyboard key is used for push-to-talk |
| 1627 | `PushToTalkToggle` | Boolean | GUI | LIVE |  | Should the push-to-talk toolbar button behave as a toggle |
| 1628 | `QAMode` | Boolean | GUI | LIVE |  | Enable Testing Features. |
| 1629 | `QAModeEventHostPort` | S32 | 非GUI | LIVE |  | (Deprecated) Port on which lleventhost should listen |
| 1630 | `QAModeFakeSystemFolderIssues` | Boolean | 非GUI | LIVE |  | Simulates system folder issues in inventory |
| 1631 | `QAModeMetrics` | Boolean | 非GUI | LIVE |  | Enables QA features (logging, faster cycling) for metrics collector |
| 1632 | `QAModeTermCode` | S32 | 非GUI | LIVE |  | On LL_ERRS, terminate with this code instead of OS message box |
| 1633 | `QuickBuyCurrency` | Boolean | 非GUI | LIVE |  | Toggle between HTML based currency purchase floater and legacy XUI ver |
| 1634 | `QuickPrefsEditMode` | Boolean | GUI | LIVE |  | Internal, volatile control that defines if the quickprefs floater is i |
| 1635 | `QuickPrefsSelectedControl` | String | GUI | LIVE |  | Internal, volatile control that holds the currently selected control.  |
| 1636 | `QuitAfterSeconds` | F32 | 非GUI | LIVE |  | The duration allowed before quitting. |
| 1637 | `QuitAfterSecondsOfAFK` | F32 | GUI | LIVE |  | The duration allowed after being AFK before quitting. |
| 1638 | `QuitOnLoginActivated` | Boolean | 非GUI | LIVE |  | Quit if login page is activated (used when auto login is on and users  |
| 1639 | `RLVaBlockedExperiences` | String | 非GUI | LIVE |  | List of experiences blocked from interacting with RLVa |
| 1640 | `RLVaCompatibilityModeList` | String | 非GUI | LIVE |  | Contains a list of creators or partial items names that require compat |
| 1641 | `RLVaDebugDeprecateExplicitPoint` | Boolean | 非GUI | LIVE |  | Ignore attachment point names on inventory items and categories (incom |
| 1642 | `RLVaDebugHideUnsetDuplicate` | Boolean | GUI(menu) | LIVE |  | Suppresses reporting "unset" or "duplicate" command restrictions when  |
| 1643 | `RLVaEnableCompositeFolders` | Boolean | 非GUI | DEAD |  | Enables composite folders for shared inventory |
| 1644 | `RLVaEnableIMQuery` | Boolean | 非GUI | LIVE |  | Enables a limited number of configuration queries via IM (e.g. @versio |
| 1645 | `RLVaEnableLegacyNaming` | Boolean | GUI(menu) | LIVE |  | Enables legacy naming convention for folders |
| 1646 | `RLVaEnableSharedWear` | Boolean | GUI(menu) | LIVE |  | Attachments in the shared #RLV folder can be force-attached without ne |
| 1647 | `RLVaEnableTemporaryAttachments` | Boolean | GUI(menu) | LIVE |  | Allows temporary attachments (regardless of origin) to issue RLV comma |
| 1648 | `RLVaExperienceMaturityThreshold` | S32 | 非GUI | LIVE |  | Specifies the minimum maturity an experience has to be before it can i |
| 1649 | `RLVaExperimentalCommands` | Boolean | 非GUI | LIVE |  | Enables the experimental command set |
| 1650 | `RLVaHideLockedAttachments` | Boolean | GUI(menu) | LIVE |  | Hides non-detachable worn attachments from @getattach |
| 1651 | `RLVaHideLockedLayers` | Boolean | GUI(menu) | LIVE |  | Hides "remove outfit" restricted worn clothing layers from @getoufit |
| 1652 | `RLVaSharedInvAutoRename` | Boolean | GUI(menu) | LIVE |  | Automatically renames shared inventory items when worn |
| 1653 | `RLVaShowAssertionFailures` | Boolean | GUI(menu) | LIVE |  | Notify the user when an assertion fails |
| 1654 | `RLVaShowRedirectChatTyping` | Boolean | GUI(menu) | LIVE |  | Sends typing start messages (and optionally plays the typing animation |
| 1655 | `RLVaSplitRedirectChat` | Boolean | GUI(menu) | LIVE |  | Splits long nearby chat lines across multiple messages when @redir* re |
| 1656 | `RLVaTopLevelMenu` | Boolean | GUI(menu) | LIVE |  | Show the RLVa specific menu as a top level menu |
| 1657 | `RLVaWearReplaceUnlocked` | Boolean | GUI(menu) | LIVE |  | Don't block wear replace when at least one attachment on the target at |
| 1658 | `RadarAlertChannel` | S32 | 非GUI | LIVE |  | Channel for whispering radar alerts |
| 1659 | `RadarAvatarAgeAlert` | Boolean | GUI | LIVE |  | Toggles whether radar sends out chat alerts when it detects an avatar  |
| 1660 | `RadarAvatarAgeAlertValue` | S32 | GUI | LIVE |  | Defines how old an avatar may be at maximum for the age alert to get t |
| 1661 | `RadarEnterChannelAlert` | Boolean | GUI | LIVE |  | Toggles whether radar sends out chat alerts when it detects a new avat |
| 1662 | `RadarLeaveChannelAlert` | Boolean | GUI | LIVE |  | Toggles whether radar sends out chat alerts when it detects an avatar  |
| 1663 | `RadarLegacyChannelAlertRefreshUUID` | String | 非GUI | LIVE |  | UUID of sound asset that when detected, will request a full radar chan |
| 1664 | `RadarNameFormat` | U32 | GUI | LIVE |  | 0=DisplayName,1=Username,2=Displayname/Username,3=Username/Displayname |
| 1665 | `RadarReportChatRangeEnter` | Boolean | GUI | LIVE |  | Display a chat message when avatar enters chat distance |
| 1666 | `RadarReportChatRangeLeave` | Boolean | GUI | LIVE |  | Display a chat message when avatar leaves chat distance |
| 1667 | `RadarReportDrawRangeEnter` | Boolean | GUI | LIVE |  | Display a chat message when avatar enters draw distance |
| 1668 | `RadarReportDrawRangeLeave` | Boolean | GUI | LIVE |  | Display a chat message when avatar leaves draw distance |
| 1669 | `RadarReportSimRangeEnter` | Boolean | GUI | LIVE |  | Display a chat message when avatar enteres local region |
| 1670 | `RadarReportSimRangeLeave` | Boolean | GUI | LIVE |  | Display a chat message when avatar leaves local region |
| 1671 | `RadioLandBrushAction` | S32 | 非GUI | LIVE |  | Last selected land modification operation (0 = flatten, 1 = raise, 2 = |
| 1672 | `RecentItemsSortOrder` | U32 | GUI | LIVE |  | Specifies sort key for recent inventory items (+0 = name, +1 = date, + |
| 1673 | `RecentJumpThresholdSecs` | F32 | 非GUI | LIVE |  | Seconds after jump input during which landing finish-anim is suppresse |
| 1674 | `RecentListShowIcons` | Boolean | GUI(menu) | LIVE |  | Show/hide people icons in recent list |
| 1675 | `RecentPeopleSortOrder` | U32 | 非GUI | LIVE |  | Specifies sort order for recent people (0 = by name, 2 = by most recen |
| 1676 | `RectangleSelectInclusive` | Boolean | GUI | LIVE |  | Select objects that have at least one vertex inside selection rectangl |
| 1677 | `RegInClient` | Boolean | 非GUI | DEAD |  | Experimental: Embed registration in login screen |
| 1678 | `RegionCheckTextureHeights` | Boolean | 非GUI | LIVE |  | Don't allow user to set low heights greater than high |
| 1679 | `RegionCrossingInterpolationTime` | F32 | 非GUI | LIVE |  | How long to extrapolate object motion after crossing regions |
| 1680 | `RegionTextureSize` | U32 | 非GUI | LIVE |  | Terrain texture dimensions (power of 2) |
| 1681 | `RelockMoveLockAfterRegionChange` | Boolean | GUI | LIVE |  | TRUE: Re-lock movelock after region change - refresh avatar position a |
| 1682 | `RememberPassword` | Boolean | GUI | LIVE |  | Keep password (in encrypted form) for next login |
| 1683 | `RememberUser` | Boolean | 非GUI | LIVE |  | Keep user name for next login |
| 1684 | `RenderAnimateRes` | Boolean | 非GUI | LIVE |  | (Obsolete) Does nothing Animate rezing prims. |
| 1685 | `RenderAnisotropic` | Boolean | GUI | LIVE |  | Render textures using anisotropic filtering |
| 1686 | `RenderAppleUseMultGL` | Boolean | 非GUI | LIVE |  | Whether we want to use multi-threaded OpenGL on Apple hardware (requir |
| 1687 | `RenderAttachedLights` | Boolean | GUI | LIVE |  | Render lighted prims that are attached to avatars |
| 1688 | `RenderAttachedParticles` | Boolean | GUI | LIVE |  | Render particle systems that are attached to avatars |
| 1689 | `RenderAutoHideSurfaceAreaLimit` | F32 | 非GUI | LIVE |  | Maximum surface area of a set of proximal objects inworld before autom |
| 1690 | `RenderAutoMaskAlphaDeferred` | Boolean | GUI | LIVE |  | Use alpha masks where appropriate in the Advanced Lighting Model |
| 1691 | `RenderAutoMaskAlphaNonDeferred` | Boolean | GUI | LIVE |  | Use alpha masks where appropriate when not using the Advanced Lighting |
| 1692 | `RenderAutoMuteLogging` | Boolean | 非GUI | DEAD |  | Show extra information in viewer logs about avatar rendering costs |
| 1693 | `RenderAutoMuteRenderWeightLimit` | U32 | 非GUI | DEAD |  | OBSOLETE. This setting has been renamed RenderAvatarMaxNonImpostors. |
| 1694 | `RenderAutoMuteSurfaceAreaLimit` | F32 | 非GUI | LIVE |  | Maximum surface area of attachments before an avatar is     rendered a |
| 1695 | `RenderAvatar` | Boolean | 非GUI | DEAD |  | Render Avatars |
| 1696 | `RenderAvatarCloth` | Boolean | GUI(menu) | LIVE | ✅ | Controls if system avatar clothes use wavy cloth |
| 1697 | `RenderAvatarComplexityMode` | S32 | GUI | LIVE |  | 0 - complexity limit applies to everyone, 1 - always show friends, 2 - |
| 1698 | `RenderAvatarFriendsOnly` | Boolean | GUI(menu) | LIVE |  | When enabled hides all avatars that aren't friends. Does not affect in |
| 1699 | `RenderAvatarLODFactor` | F32 | GUI | LIVE |  | Controls level of detail of avatars (multiplier for current screen are |
| 1700 | `RenderAvatarMaxART` | F32 | GUI | LIVE |  | Render Time Limit in microseconds (0.0 = no limit) |
| 1701 | `RenderAvatarMaxComplexity` | U32 | GUI | LIVE |  | Maximum Avatar Complexity; above this value, the avatar is     rendere |
| 1702 | `RenderAvatarMaxNonImpostors` | U32 | GUI | LIVE |  | Maximum number of avatars to fully render at one time;     over this l |
| 1703 | `RenderAvatarMaxVisible` | S32 | 非GUI | LIVE |  | OBSOLETE and UNUSED. See RenderAvatarMaxNonImpostors |
| 1704 | `RenderAvatarPhysicsLODFactor` | F32 | GUI | LIVE |  | Controls level of detail of avatar physics (such as breast physics). |
| 1705 | `RenderBackend` | U32 | 非GUI | LIVE |  | r41 Phase E++ atomic 3 RenderBackend cvar: 0 = GL only (= initVulkan() |
| 1706 | `RenderBakeSunlight` | Boolean | 非GUI | DEAD |  | Bake sunlight into vertex buffers for static objects. |
| 1707 | `RenderBalanceInSnapshot` | Boolean | GUI | LIVE |  | Display L$ balance in snapshot |
| 1708 | `RenderBufferVisualization` | S32 | 非GUI | LIVE | ✅ | Outputs a selected buffer to the screen.  -1 = final render buffer.  0 |
| 1709 | `RenderBumpmapMinDistanceSquared` | F32 | 非GUI | DEAD |  | Maximum distance at which to render bumpmapped primitives (distance in |
| 1710 | `RenderCASSharpness` | F32 | GUI | LIVE |  | Level of sharpening to apply via Contrast Adaptive Sharpening (0.0(off |
| 1711 | `RenderCPUBasis` | F32 | 非GUI | LIVE |  | Reference CPU clockspeed to use to bias GPU class (in MHz). |
| 1712 | `RenderCanUseGLTFPBROpaqueShaders` | Boolean | 非GUI | LIVE |  | Hardware has support for GLTF scene shaders |
| 1713 | `RenderCanUseTerrainBakeShaders` | Boolean | 非GUI | LIVE |  | Hardware has support for Terrain Bake shaders |
| 1714 | `RenderChromaStrength` | F32 | GUI | LIVE | ✅ | AYAstorm r30 P4 (new render engine borrow): radial chromatic aberratio |
| 1715 | `RenderClass1MemoryBandwidth` | F32 | 非GUI | LIVE |  | Memory bandwidth at which to default to Class 1 in gigabytes per secon |
| 1716 | `RenderColorBrightness` | F32 | GUI | LIVE | ✅ | Color grading brightness offset (-1.0=dark, 0.0=neutral, 1.0=bright) |
| 1717 | `RenderColorContrast` | F32 | GUI | LIVE | ✅ | Color grading contrast (1.0=neutral, 0.5=flat, 2.0=high contrast) |
| 1718 | `RenderColorGradingLUTIntensity` | F32 | GUI | LIVE | ✅ | Blend intensity of the 3D color grading LUT (0.0=off, 1.0=full) |
| 1719 | `RenderColorGradingLUTName` | String | 非GUI | LIVE |  | Name of the 3D LUT .cube file to apply for color grading |
| 1720 | `RenderColorSaturation` | F32 | GUI | LIVE | ✅ | Color grading saturation (0.0=grayscale, 1.0=neutral, 2.0=vivid) |
| 1721 | `RenderColorTemperature` | F32 | GUI | LIVE | ✅ | Color grading temperature offset (-1.0=cool/blue, 0.0=neutral, 1.0=war |
| 1722 | `RenderComplexityColorMax` | Color4 | 非GUI | DEAD |  | Unused obsolete setting |
| 1723 | `RenderComplexityColorMid` | Color4 | 非GUI | DEAD |  | Unused obsolete setting |
| 1724 | `RenderComplexityColorMin` | Color4 | 非GUI | DEAD |  | Unused obsolete setting |
| 1725 | `RenderComplexityStaticMax` | S32 | 非GUI | DEAD |  | Unused obsolete setting |
| 1726 | `RenderComplexityThreshold` | S32 | 非GUI | DEAD |  | Unused obsolete setting |
| 1727 | `RenderCompressTextures` | Boolean | GUI | LIVE |  | Enable texture compression on OpenGL 3.0 and later implementations (EX |
| 1728 | `RenderCubeMap` | Boolean | 非GUI | LIVE |  | Whether we can render the cube map or not |
| 1729 | `RenderDebugAlphaMask` | F32 | 非GUI | DEAD |  | Test Alpha Masking Cutoffs. |
| 1730 | `RenderDebugGLSession` | Boolean | GUI(menu) | LIVE |  | Enable strict GL debugging on the start of next session. |
| 1731 | `RenderDebugNormalScale` | F32 | 非GUI | LIVE | ✅ | Scale of normals in debug display. |
| 1732 | `RenderDebugPipeline` | Boolean | GUI(menu) | DEAD |  | Enable strict pipeline debugging. |
| 1733 | `RenderDebugSH` | Boolean | 非GUI | DEAD |  | Enable SH indirect lighting visualization. |
| 1734 | `RenderDebugTextureBind` | Boolean | 非GUI | DEAD |  | Enable texture bind performance test. |
| 1735 | `RenderDefaultProbeUpdatePeriod` | F32 | GUI | LIVE |  | When RenderReflectionProbeLevel is 0, amount of time in seconds to wai |
| 1736 | `RenderDeferred` | Boolean | GUI | LIVE |  | DEPRECATED (only true supported) - Use deferred rendering pipeline (Ad |
| 1737 | `RenderDeferredAlphaSoften` | F32 | 非GUI | DEAD |  | Scalar for softening alpha surfaces (for soft particles). |
| 1738 | `RenderDeferredAtmospheric` | Boolean | 非GUI | LIVE |  | Execute atmospheric shader in deferred renderer. |
| 1739 | `RenderDeferredBlurLight` | Boolean | GUI | LIVE |  | Execute shadow softening shader in deferred renderer. |
| 1740 | `RenderDeferredDisplayGamma` | F32 | 非GUI | DEAD |  | Gamma ramp exponent for final correction before display gamma. |
| 1741 | `RenderDeferredLights` | Boolean | GUI | LIVE |  | If false, in-world (non-attached) point/spot lights are suppressed. Ci |
| 1742 | `RenderDeferredNoise` | F32 | 非GUI | LIVE |  | Noise scalar to hide banding in deferred render. |
| 1743 | `RenderDeferredSSAO` | Boolean | GUI | LIVE | ✅ | Execute screen space ambient occlusion shader in deferred renderer. |
| 1744 | `RenderDeferredSpotShadowBias` | F32 | 非GUI | LIVE |  | Bias value for spot shadows (prevent shadow acne). |
| 1745 | `RenderDeferredSpotShadowOffset` | F32 | 非GUI | LIVE |  | Offset value for spot shadows (prevent shadow acne). |
| 1746 | `RenderDeferredSun` | Boolean | 非GUI | DEAD |  | Execute sunlight shader in deferred renderer. |
| 1747 | `RenderDeferredSunWash` | F32 | 非GUI | LIVE |  | Amount local lights are washed out by sun. |
| 1748 | `RenderDeferredTreeShadowBias` | F32 | 非GUI | LIVE |  | Bias value for tree shadows (prevent shadow acne). |
| 1749 | `RenderDeferredTreeShadowOffset` | F32 | 非GUI | LIVE |  | Offset value for tree shadows (prevent shadow acne). |
| 1750 | `RenderDelayCreation` | Boolean | 非GUI | LIVE |  | Throttle creation of drawables. |
| 1751 | `RenderDelayVBUpdate` | Boolean | 非GUI | DEAD |  | Delay vertex buffer updates until just before rendering |
| 1752 | `RenderDepthOfField` | Boolean | GUI | LIVE |  | Whether to use depth of field effect when Advanced Lighting Model is e |
| 1753 | `RenderDepthOfFieldAlphas` | Boolean | GUI | LIVE |  | If true, transparent surfaces contribute to depth-of-field depth (cuto |
| 1754 | `RenderDepthOfFieldChroma` | Boolean | GUI | LIVE | ✅ | AYAstorm r30 P4 (new render engine borrow): use DoF-coupled chromatic  |
| 1755 | `RenderDepthOfFieldFront` | Boolean | GUI | LIVE | ✅ | AYAstorm r30 P4 (new render engine borrow): enable blurring the foregr |
| 1756 | `RenderDepthOfFieldHighQuality` | Boolean | GUI | LIVE | ✅ | AYAstorm r30 P4: enable the new render engine's high quality Depth of  |
| 1757 | `RenderDepthOfFieldInEditMode` | Boolean | GUI | LIVE |  | Whether to use depth of field effect when in edit mode |
| 1758 | `RenderDepthPrePass` | Boolean | 非GUI | LIVE |  | EXPERIMENTAL: Prime the depth buffer with simple prim geometry before  |
| 1759 | `RenderDesaturateIrradiance` | Boolean | GUI | LIVE |  | Desaturate irradiance to remove blue tint |
| 1760 | `RenderDiffuseLuminanceScale` | F32 | GUI | LIVE | ✅ | Luminance adjustment for diffuse surfaces to aid auto-exposure behavio |
| 1761 | `RenderDisablePostProcessing` | Boolean | GUI | LIVE |  | Disable tone mapping and exposure correction when build floater is ope |
| 1762 | `RenderDisableVintageMode` | Boolean | GUI | LIVE |  | Enables additional rendering pipeline features on newer machines such  |
| 1763 | `RenderDownScaleMethod` | U32 | 非GUI | LIVE |  | Method to use to downscale images.  0 - FBO, 1 - PBO |
| 1764 | `RenderDynamicExposureCoefficient` | F32 | GUI | LIVE | ✅ | Luminance coefficient for dynamic exposure |
| 1765 | `RenderDynamicLOD` | Boolean | GUI | LIVE |  | Dynamically adjust level of detail. |
| 1766 | `RenderEdgeDepthCutoff` | F32 | 非GUI | LIVE |  | Cutoff for depth difference that amounts to an edge. |
| 1767 | `RenderEdgeNormCutoff` | F32 | 非GUI | LIVE |  | Cutoff for normal difference that amounts to an edge. |
| 1768 | `RenderEnableEmissiveBuffer` | Boolean | 非GUI | LIVE | ✅ | Enable emissive buffer in gbuffer.  Should only be disabled in GL3 mod |
| 1769 | `RenderEnableFullbright` | Boolean | GUI | LIVE |  | If false, fullbright textures are rendered as regular shaded surfaces. |
| 1770 | `RenderExposure` | F32 | GUI | LIVE | ✅ | Exposure value to send to tonemapper. |
| 1771 | `RenderFSAASamples` | U32 | GUI | LIVE |  | Quality of antialiasing: 0 = Low, 1 = Medium, 2 = High, 3 = Ultra |
| 1772 | `RenderFSAAType` | U32 | GUI | LIVE |  | Type of Antialiasing to use: 0 = None, 1 = FXAA, 2 = SMAA |
| 1773 | `RenderFarClip` | F32 | GUI | LIVE |  | Distance of far clip plane from camera (meters) |
| 1774 | `RenderFlexTimeFactor` | F32 | GUI | LIVE |  | Controls level of detail of flexible objects (multiplier for amount of |
| 1775 | `RenderFogRatio` | F32 | GUI | LIVE |  | DEPRECATED - Distance from camera where fog reaches maximum density (f |
| 1776 | `RenderGLContextCoreProfile` | Boolean | 非GUI | DEAD |  | Don't use a compatibility profile OpenGL context.  Requires restart. |
| 1777 | `RenderGLMultiThreadedMedia` | Boolean | 非GUI | LIVE |  | Allow OpenGL to use multiple render contexts for playing media (may re |
| 1778 | `RenderGLMultiThreadedTextures` | Boolean | 非GUI | LIVE |  | Allow OpenGL to use multiple render contexts for loading textures (may |
| 1779 | `RenderGamma` | F32 | 非GUI | LIVE |  | DEPRECATED - Sets gamma exponent for renderer |
| 1780 | `RenderGammaFull` | Boolean | 非GUI | LIVE |  | Use fully controllable gamma correction, instead of faster, hard-coded |
| 1781 | `RenderGlobalLightStrength` | F32 | GUI | LIVE | ✅ | Global multiplier applied to point/spot light contribution in the defe |
| 1782 | `RenderGlow` | Boolean | GUI | LIVE |  | Render bloom post effect. |
| 1783 | `RenderGlowHDR` | Boolean | GUI | LIVE |  | Enable HDR for glow map |
| 1784 | `RenderGlowIterations` | S32 | GUI | LIVE |  | Number of times to iterate the glow (higher = wider and smoother but s |
| 1785 | `RenderGlowLumWeights` | Vector3 | 非GUI | LIVE | ✅ | Weights for each color channel to be used in calculating luminance (sh |
| 1786 | `RenderGlowMaxExtractAlpha` | F32 | GUI | LIVE | ✅ | Max glow alpha value for brightness extraction to auto-glow. |
| 1787 | `RenderGlowMinLuminance` | F32 | GUI | LIVE | ✅ | Min luminance intensity necessary to consider an object bright enough  |
| 1788 | `RenderGlowNoise` | Boolean | GUI | LIVE | ✅ | Enables glow noise (dithering). Reduces banding from glow in certain c |
| 1789 | `RenderGlowResolutionPow` | S32 | GUI | LIVE | ✅ | Glow map resolution power of two. |
| 1790 | `RenderGlowStrength` | F32 | GUI | LIVE | ✅ | Additive strength of glow. |
| 1791 | `RenderGlowWarmthAmount` | F32 | GUI | LIVE | ✅ | Amount of warmth extraction to use (versus luminance extraction). 0 =  |
| 1792 | `RenderGlowWarmthWeights` | Vector3 | 非GUI | LIVE | ✅ | Weight of each color channel used before finding the max warmth |
| 1793 | `RenderGlowWidth` | F32 | GUI | LIVE | ✅ | Glow sample size (higher = wider and softer but eventually more pixela |
| 1794 | `RenderHDREnabled` | Boolean | 非GUI | LIVE | ✅ | Enable HDR rendering. |
| 1795 | `RenderHDRIExposure` | F32 | GUI | LIVE |  | Exposure adjustment of HDRI when previewing an HDRI.  Units are EV.  S |
| 1796 | `RenderHDRIIrradianceOnly` | Boolean | GUI | LIVE |  | Only use HDRI sky for irradiance map when RenderHDRISplitScreen is 0 |
| 1797 | `RenderHDRIRotation` | F32 | GUI | LIVE |  | Rotation (in degrees) of environment when previewing an HDRI. |
| 1798 | `RenderHDRISplitScreen` | F32 | GUI | LIVE |  | What percentage of screen to render using HDRI vs EEP sky. |
| 1799 | `RenderHDRSkySunlightScale` | F32 | 非GUI | LIVE | ✅ | Sunlight scale fudge factor for matching with pre-PBR viewer when HDR  |
| 1800 | `RenderHUDInSnapshot` | Boolean | GUI | LIVE |  | Display HUD attachments in snapshot |
| 1801 | `RenderHUDObjectsWarning` | U32 | 非GUI | LIVE |  | Viewer will warn user about HUD containing to many objects if objects  |
| 1802 | `RenderHUDOversizedTexturesWarning` | U32 | 非GUI | LIVE |  | How many textures with size 1024 * 1024 or bigger HUD can contain befo |
| 1803 | `RenderHUDParticles` | Boolean | 非GUI | LIVE |  | Display particle systems in HUD attachments (experimental) |
| 1804 | `RenderHUDTexturesMemoryWarning` | U32 | 非GUI | LIVE |  | Viewer will warn user about HUD textures using memory above this value |
| 1805 | `RenderHUDTexturesWarning` | U32 | 非GUI | LIVE |  | Viewer will warn user about HUD containing to many textures if texture |
| 1806 | `RenderHeroProbeConservativeUpdateMultiplier` | S32 | GUI | LIVE |  | How many probe updates to wait until it's time to update faces that ar |
| 1807 | `RenderHeroProbeDistance` | F32 | GUI | LIVE |  | Distance in meters for hero probes to render out to. |
| 1808 | `RenderHeroProbeResolution` | S32 | GUI | LIVE | ✅ | Resolution to render hero probes used for mirrors, water, etc. |
| 1809 | `RenderHeroProbeUpdateRate` | S32 | GUI | LIVE |  | How many frames to wait for until it's time to render the probe. E.g., |
| 1810 | `RenderHiDPI` | Boolean | GUI | LIVE |  | Enable support for HiDPI displays, like Retina (macOS ONLY, requires r |
| 1811 | `RenderHiddenSelections` | Boolean | GUI | LIVE |  | Show selection lines on objects that are behind other objects |
| 1812 | `RenderHideGroupTitle` | Boolean | GUI | LIVE |  | Don't show my group title in my name label |
| 1813 | `RenderHighMemMinDiscardDecrement` | F32 | 非GUI | LIVE |  | Minimum decrement of discard bias if excess texture memory is availabl |
| 1814 | `RenderHighlightBrightness` | F32 | 非GUI | LIVE |  | Brightness of mouseover highlights. |
| 1815 | `RenderHighlightColor` | Color4 | 非GUI | LIVE |  | Brightness of mouseover highlights. |
| 1816 | `RenderHighlightFadeTime` | F32 | 非GUI | LIVE |  | Transition time for mouseover highlights. |
| 1817 | `RenderHighlightSelections` | Boolean | GUI | LIVE |  | Show selection outlines on objects |
| 1818 | `RenderHighlightThickness` | F32 | 非GUI | LIVE |  | Thickness of mouseover highlights. |
| 1819 | `RenderHoverGlowEnable` | Boolean | 非GUI | DEAD |  | DEPRECATED --- Show glow effect when hovering on interactive objects. |
| 1820 | `RenderInitError` | Boolean | 非GUI | LIVE |  | Error occured while initializing GL |
| 1821 | `RenderJellyDollsAsImpostors` | Boolean | 非GUI | LIVE |  | Use an impostor instead of a JellyDoll for better visuals (true) |
| 1822 | `RenderLightRadius` | Boolean | GUI | LIVE |  | Render the radius of selected lights |
| 1823 | `RenderLocalLightCount` | S32 | GUI | LIVE |  | Number of local lights to render. |
| 1824 | `RenderLowMemMinDiscardIncrement` | F32 | 非GUI | LIVE |  | Minimum increment of discard bias if available texture memory gets low |
| 1825 | `RenderMaxNodeSize` | S32 | 非GUI | LIVE |  | Maximum size of a single node's vertex data (in KB). |
| 1826 | `RenderMaxOpenGLVersion` | F32 | 非GUI | LIVE |  | Maximum OpenGL version to attempt use (minimum 3.1 maximum 4.6).  Requ |
| 1827 | `RenderMaxPartCount` | S32 | GUI | LIVE |  | Maximum number of particles to display on screen |
| 1828 | `RenderMaxTextureIndex` | U32 | 非GUI | LIVE |  | Maximum texture index to use for indexed texture rendering. |
| 1829 | `RenderMaxTextureResolution` | U32 | GUI | LIVE |  | Maximum texture resolution to download for non-boosted textures. |
| 1830 | `RenderMaxVBOSize` | S32 | 非GUI | LIVE |  | Maximum size of a vertex buffer (in KB). |
| 1831 | `RenderMaxVRAMBudget` | U32 | GUI | LIVE |  | Maximum amount of texture memory to budget for (in MB), or autodetect  |
| 1832 | `RenderMinFreeMainMemoryThreshold` | U32 | 非GUI | LIVE |  | If available free physical memory is below this value textures get agr |
| 1833 | `RenderMinimumLODTriangleCount` | U32 | 非GUI | DEAD |  | Triangle count threshold at which automatic LOD generation stops |
| 1834 | `RenderMirrors` | Boolean | GUI | LIVE | ✅ | Renders realtime mirrors. |
| 1835 | `RenderMotionBlur` | Boolean | GUI | LIVE |  | Enable per-object motion blur velocity buffer generation. |
| 1836 | `RenderMotionBlurOtherAvatars` | Boolean | GUI | LIVE |  | AYAstorm r30 P2: include other avatars in the velocity buffer so motio |
| 1837 | `RenderMotionBlurSelfAvatar` | Boolean | GUI | LIVE |  | AYAstorm r30 P2: include the self avatar in the velocity buffer so mot |
| 1838 | `RenderMotionBlurStrength` | S32 | GUI | LIVE | ✅ | Maximum motion blur length in pixels (higher = stronger blur). New ren |
| 1839 | `RenderNameFadeDuration` | F32 | 非GUI | LIVE |  | Time interval over which to fade avatar names (seconds) |
| 1840 | `RenderNameShowSelf` | Boolean | GUI | LIVE |  | Display own name above avatar |
| 1841 | `RenderNameShowTime` | F32 | GUI | LIVE |  | Fade avatar names after specified time (seconds) |
| 1842 | `RenderNoAlpha` | Boolean | 非GUI | DEAD |  | Disable rendering of alpha objects (render all alpha objects as alpha  |
| 1843 | `RenderNormalMapScale` | F32 | GUI | LIVE | ✅ | Scaler applied to height map when generating normal maps |
| 1844 | `RenderNsightDebugSupport` | Boolean | 非GUI | LIVE |  | Disable features which prevent nVidia nSight from being usable with SL |
| 1845 | `RenderObjectBump` | Boolean | 非GUI | LIVE |  | DEPRECATED (only TRUE supported) Show bumpmapping on primitives |
| 1846 | `RenderOcclusionTimeout` | U32 | 非GUI | LIVE |  | Maximum number of frames to wait on an occlusion query before forcibly |
| 1847 | `RenderOtherAttachedLights` | Boolean | GUI | LIVE |  | If false, lights attached to other avatars are suppressed. Cinematic m |
| 1848 | `RenderOwnAttachedLights` | Boolean | GUI | LIVE |  | If false, lights attached to your own avatar are suppressed. Cinematic |
| 1849 | `RenderParcelSelection` | Boolean | GUI | LIVE |  | Display selected parcel outline |
| 1850 | `RenderPerformanceTest` | Boolean | 非GUI | LIVE |  | Disable rendering of everything but in-world content for               |
| 1851 | `RenderPostGreyscaleStrength` | F32 | GUI | LIVE | ✅ | Set the percentual amount of greyscale's strength, 1.0 means fully gre |
| 1852 | `RenderPostPosterizationSamples` | U32 | GUI | LIVE | ✅ | Set the number of color shades used for rendering the scene. 1 = poste |
| 1853 | `RenderPostSepiaStrength` | F32 | GUI | LIVE | ✅ | Set the percentual amount of sepia's strength, 1.0 means a full sepia  |
| 1854 | `RenderPreferStreamDraw` | Boolean | 非GUI | DEAD |  | Use GL_STREAM_DRAW in place of GL_DYNAMIC_DRAW |
| 1855 | `RenderProjectorShadowResolution` | Vector2 | 非GUI | LIVE |  | Raw projector shadow width of the two available spot shadow maps. Cine |
| 1856 | `RenderQualityPerformance` | U32 | GUI | LIVE |  | Which graphics settings you've chosen. Don't use this setting to chang |
| 1857 | `RenderReflectionDetail` | S32 | GUI | LIVE |  | DEPRECATED -- use RenderTransparentWater and RenderReflectionProbeDeta |
| 1858 | `RenderReflectionProbeAmbiance` | F32 | GUI | LIVE(XML専用) |  | Amount reflection probes contribute to ambient light. |
| 1859 | `RenderReflectionProbeCount` | U32 | GUI | LIVE |  | Number of probes to render.  Maximum of 256.  Clamps to the nearest po |
| 1860 | `RenderReflectionProbeDetail` | S32 | GUI | LIVE |  | Detail of reflections. (-1 - Disabled, 0 - Static Only, 1 - Static + D |
| 1861 | `RenderReflectionProbeDrawDistance` | F32 | GUI | LIVE |  | Camera far clip to use when updating reflection probes. |
| 1862 | `RenderReflectionProbeDynamicAllocation` | S32 | 非GUI | LIVE |  | Enable dynamic allocation of reflection probes. -1 means no dynamic al |
| 1863 | `RenderReflectionProbeLevel` | S32 | GUI | LIVE | ✅ | Reflection probes control.  0 - disable (one probe to rule them all),  |
| 1864 | `RenderReflectionProbeMaxLocalLightAmbiance` | F32 | GUI | LIVE |  | Maximum effective probe ambiance for local lights |
| 1865 | `RenderReflectionProbeResolution` | U32 | GUI | LIVE |  | Resolution of reflection probe radiance maps (requires restart).  Will |
| 1866 | `RenderReflectionProbeShowTransparent` | Boolean | GUI(menu) | LIVE |  | Show reflection probes in the transparency debug view |
| 1867 | `RenderReflectionProbeVolumes` | Boolean | GUI | LIVE |  | Render influence volumes of Reflection Probes |
| 1868 | `RenderReflectionRes` | S32 | 非GUI | DEAD |  | Reflection map resolution. |
| 1869 | `RenderReflectionsEnabled` | Boolean | GUI | LIVE | ✅ | Enable/disable reflection probes - Deprecated, disabling this removes  |
| 1870 | `RenderReservedTextureIndices` | S32 | 非GUI | LIVE |  | Count of texture indices to reserve for shadow and reflection maps whe |
| 1871 | `RenderResolutionDivisor` | U32 | 非GUI | LIVE |  | Divisor for rendering 3D scene at reduced resolution. |
| 1872 | `RenderResolutionMultiplier` | F32 | 非GUI | LIVE |  | Multiplier for rendering 3D scene at reduced resolution. Valid values: |
| 1873 | `RenderSSAOEffect` | Vector3 | 非GUI | LIVE |  | Multiplier for (1) value and (2) saturation (HSV definition), for area |
| 1874 | `RenderSSAOFactor` | F32 | GUI | LIVE | ✅ | Occlusion sensitivity factor for ambient occlusion (larger is more) |
| 1875 | `RenderSSAOIrradianceMax` | F32 | GUI | LIVE |  | Max factor for irradiance input to SSAO |
| 1876 | `RenderSSAOIrradianceScale` | F32 | GUI | LIVE |  | Scaling factor for irradiance input to SSAO |
| 1877 | `RenderSSAOMaxScale` | U32 | GUI | LIVE | ✅ | Maximum screen radius for sampling (pixels) |
| 1878 | `RenderSSAOScale` | F32 | GUI | LIVE | ✅ | Scaling factor for the area to sample for occluders (pixels at 1 meter |
| 1879 | `RenderScreenSpaceReflectionAdaptiveStepMultiplier` | F32 | GUI | LIVE | ✅ | Multiplier to scale adaptive stepping. |
| 1880 | `RenderScreenSpaceReflectionDepthRejectBias` | F32 | GUI | LIVE | ✅ | Bias against the depth buffer before rejecting a sample. |
| 1881 | `RenderScreenSpaceReflectionDistanceBias` | F32 | GUI | LIVE | ✅ | Distance bias to apply when rejecting a potential sample. |
| 1882 | `RenderScreenSpaceReflectionGlossySamples` | S32 | GUI | LIVE | ✅ | Maximum number of samples to apply for glossy SSR. |
| 1883 | `RenderScreenSpaceReflectionIterations` | S32 | GUI | LIVE | ✅ | Number of times the ray march algorithm runs to find a potential hit. |
| 1884 | `RenderScreenSpaceReflectionMaxDepth` | F32 | 非GUI | LIVE | ✅ | Maximum depth from the camera to attempt a trace. |
| 1885 | `RenderScreenSpaceReflectionMaxRoughness` | F32 | 非GUI | LIVE | ✅ | Maximum permitted roughness for screen space reflections. |
| 1886 | `RenderScreenSpaceReflectionRayStep` | F32 | GUI | LIVE | ✅ | How big the step is between each run. |
| 1887 | `RenderScreenSpaceReflectionSplitEnd` | Vector3 | 非GUI | DEAD |  | Ending splits for each SSR pass. |
| 1888 | `RenderScreenSpaceReflectionSplitStart` | Vector3 | 非GUI | DEAD |  | Starting splits for each SSR pass. |
| 1889 | `RenderScreenSpaceReflections` | Boolean | GUI | LIVE | ✅ | Renders screen space reflections to better account for dynamic objects |
| 1890 | `RenderSculptSAThreshold` | F32 | 非GUI | LIVE |  | The surface area at which sculpts begin to be considered for being mad |
| 1891 | `RenderShaderCacheEnabled` | Boolean | GUI(menu) | LIVE |  | Enable binary shader cache |
| 1892 | `RenderShaderCacheVersion` | String | 非GUI | LIVE |  | Current version hash for shader cache |
| 1893 | `RenderShaderLODThreshold` | F32 | 非GUI | DEAD |  | Fraction of draw distance defining the switch to a different shader LO |
| 1894 | `RenderShaderLightingMaxLevel` | S32 | GUI | LIVE | ✅ | Max lighting level to use in the shader (class 3 is default, 2 is less |
| 1895 | `RenderShaderParticleThreshold` | F32 | 非GUI | DEAD |  | Fraction of draw distance to not use shader on particles |
| 1896 | `RenderShadowAutomaticDistance` | Boolean | GUI | LIVE |  | If true, shadow cascade distances are computed automatically; if false |
| 1897 | `RenderShadowBias` | F32 | GUI | LIVE | ✅ | Bias value for shadows (prevent shadow acne). |
| 1898 | `RenderShadowBiasError` | F32 | 非GUI | LIVE | ✅ | Error scale for shadow bias (based on altitude). |
| 1899 | `RenderShadowBlurDistFactor` | F32 | GUI | LIVE | ✅ | Distance scaler for shadow blur. |
| 1900 | `RenderShadowBlurSamples` | U32 | 非GUI | DEAD |  | Number of samples to take for each pass of shadow blur (value range 1- |
| 1901 | `RenderShadowBlurSize` | F32 | GUI | LIVE | ✅ | Scale of shadow softening kernel. |
| 1902 | `RenderShadowDetail` | S32 | GUI | LIVE | ✅ | Detail of shadows. |
| 1903 | `RenderShadowDistance` | Vector4 | 非GUI | LIVE |  | Per-cascade sun shadow far clip range addition (closest X / mid Y / fa |
| 1904 | `RenderShadowErrorCutoff` | F32 | 非GUI | LIVE |  | Cutoff error value to use ortho instead of perspective projection. |
| 1905 | `RenderShadowFOVCutoff` | F32 | GUI | LIVE |  | Cutoff FOV to use ortho instead of perspective projection. |
| 1906 | `RenderShadowFarClip` | F32 | GUI | LIVE |  | Single far-clip distance for the sun shadow cascade pyramid when Rende |
| 1907 | `RenderShadowGaussian` | Vector3 | 非GUI | LIVE |  | Gaussian coefficients for the two shadow/SSAO blurring passes (z compo |
| 1908 | `RenderShadowNoise` | F32 | 非GUI | LIVE |  | Magnitude of noise on shadow samples. |
| 1909 | `RenderShadowOffset` | F32 | 非GUI | LIVE | ✅ | Offset value for shadows (prevent shadow acne). |
| 1910 | `RenderShadowOffsetError` | F32 | 非GUI | LIVE |  | Error scale for shadow offset (based on altitude). |
| 1911 | `RenderShadowProjExponent` | F32 | 非GUI | DEAD |  | Exponent applied to transition between ortho and perspective shadow pr |
| 1912 | `RenderShadowProjOffset` | F32 | 非GUI | DEAD |  | Amount to scale distance to virtual origin of shadow perspective proje |
| 1913 | `RenderShadowResolution` | Vector4 | 非GUI | LIVE |  | Raw shadow width and height of each of the four shadow maps, closest ( |
| 1914 | `RenderShadowResolutionScale` | F32 | GUI | LIVE |  | Scale of shadow map resolution vs. screen resolution (only positivie v |
| 1915 | `RenderShadowSlopeThreshold` | F32 | 非GUI | DEAD |  | Cutoff slope value for points to affect perspective shadow generation |
| 1916 | `RenderShadowSoftness` | F32 | GUI | LIVE | ✅ | PCF shadow softness multiplier (1.0 = sharp, 3.0 = soft). Scales the s |
| 1917 | `RenderShadowSplitExponent` | Vector3 | 非GUI | LIVE |  | Near clip plane split distances for shadow map frusta (x=perspective,  |
| 1918 | `RenderShadowSplits` | S32 | 非GUI | LIVE |  | Amount of shadow map splits to render (0 - 3). |
| 1919 | `RenderSkyAmbientScale` | F32 | GUI | LIVE | ✅ | Ambient scale fudge factor for matching with pre-PBR viewer |
| 1920 | `RenderSkyAutoAdjustAmbientScale` | F32 | GUI | LIVE |  | Amount to scale ambient when auto-adjusting legacy skies |
| 1921 | `RenderSkyAutoAdjustBlueDensityScale` | F32 | GUI | LIVE |  | Blue Horizon Scale value to use when auto-adjusting legacy skies |
| 1922 | `RenderSkyAutoAdjustBlueHorizonScale` | F32 | GUI | LIVE |  | Blue Horizon Scale value to use when auto-adjusting legacy skies |
| 1923 | `RenderSkyAutoAdjustHDRScale` | F32 | GUI | LIVE |  | HDR Scale value to use when auto-adjusting legacy skies |
| 1924 | `RenderSkyAutoAdjustLegacy` | Boolean | GUI | LIVE | ✅ | If true, automatically adjust legacy skies (those without a probe ambi |
| 1925 | `RenderSkyAutoAdjustProbeAmbiance` | F32 | GUI | LIVE |  | Probe ambiance value when auto-adjusting legacy skies |
| 1926 | `RenderSkyAutoAdjustSunColorScale` | F32 | GUI | LIVE |  | Sun color scalar when auto-adjusting legacy skies |
| 1927 | `RenderSkySunlightScale` | F32 | GUI | LIVE | ✅ | Sunlight scale fudge factor for matching with pre-PBR viewer when HDR  |
| 1928 | `RenderSnapshotNoPost` | Boolean | 非GUI | LIVE |  | Disable tone mapping and exposure correction when snapshot is being re |
| 1929 | `RenderSpecularExponent` | F32 | 非GUI | LIVE |  | Specular exponent for generating spec map |
| 1930 | `RenderSpecularPrecision` | U32 | 非GUI | DEAD |  | Force 32-bit floating point LUT |
| 1931 | `RenderSpecularResX` | U32 | 非GUI | LIVE |  | Spec map resolution. |
| 1932 | `RenderSpecularResY` | U32 | 非GUI | LIVE |  | Spec map resolution. |
| 1933 | `RenderSpotLightsInNondeferred` | Boolean | 非GUI | LIVE |  | Whether to support projectors as spotlights when Advanced Lighting Mod |
| 1934 | `RenderSpotShadowBias` | F32 | 非GUI | LIVE | ✅ | Bias value for shadows (prevent shadow acne). |
| 1935 | `RenderSpotShadowOffset` | F32 | GUI | LIVE | ✅ | Offset value for shadows (prevent shadow acne). |
| 1936 | `RenderSunDynamicRange` | F32 | GUI | LIVE |  | Defines what percent brighter the sun is than local point lights (1.0  |
| 1937 | `RenderTerrainDetail` | S32 | GUI | LIVE(XML専用) |  | Detail applied to terrain texturing (0 = none, 1 = full) |
| 1938 | `RenderTerrainLODFactor` | F32 | GUI | LIVE |  | Controls level of detail of terrain (multiplier for current screen are |
| 1939 | `RenderTerrainPBRDetail` | S32 | GUI | LIVE | ✅ | Detail level for PBR terrain. 0 is full detail. Negative values drop r |
| 1940 | `RenderTerrainPBREnabled` | Boolean | 非GUI | LIVE |  | Enable PBR Terrain features. |
| 1941 | `RenderTerrainPBRNormalsEnabled` | Boolean | 非GUI | LIVE |  | EXPERIMENTAL: Change normal gen for PBR Terrain. |
| 1942 | `RenderTerrainPBRPlanarSampleCount` | S32 | GUI | LIVE | ✅ | How many UV planes to sample PBR terrain textures from. 1 is "flat", 3 |
| 1943 | `RenderTerrainPBRScale` | F32 | GUI | LIVE |  | PBR terrain detail texture scale (meters) |
| 1944 | `RenderTerrainPBRTransformsEnabled` | Boolean | 非GUI | LIVE |  | EXPERIMENTAL: Enable PBR Terrain texture transforms. |
| 1945 | `RenderTerrainPBRTriplanarBlendFactor` | F32 | GUI | LIVE | ✅ | Higher values create sharper transitions, but are more likely to produ |
| 1946 | `RenderTerrainScale` | F32 | GUI | LIVE |  | Terrain detail texture scale (meters) |
| 1947 | `RenderTextureVRAMDivisor` | U32 | GUI | LIVE |  | Divisor for maximum amount of VRAM the viewer will use for textures. 1 |
| 1948 | `RenderTonemapMix` | F32 | GUI | LIVE |  | Mix between linear and tonemapped colors (0.0(Linear) - 1.0(Tonemapped |
| 1949 | `RenderTonemapType` | U32 | GUI | LIVE | ✅ | What tonemapper to use: 0 = Khronos Neutral, 1 = ACES, 2 = Filmic |
| 1950 | `RenderTrackerBeacon` | Boolean | 非GUI | LIVE |  | Display tracking arrow and beacon to target avatar, teleport destinati |
| 1951 | `RenderTransparentWater` | Boolean | GUI | LIVE | ✅ | Render water as transparent.  Setting to false renders water as opaque |
| 1952 | `RenderTreeLODFactor` | F32 | GUI | LIVE |  | Controls level of detail of vegetation (multiplier for current screen  |
| 1953 | `RenderUIBuffer` | Boolean | 非GUI | LIVE |  | Cache ui render in a screen aligned buffer. |
| 1954 | `RenderUIInSnapshot` | Boolean | GUI | LIVE |  | Display user interface in snapshot |
| 1955 | `RenderUnloadedAvatar` | Boolean | GUI | LIVE |  | Show avatars which haven't finished loading |
| 1956 | `RenderUseAdvancedAtmospherics` | Boolean | 非GUI | DEAD |  | Use fancy precomputed atmospherics and stuff. |
| 1957 | `RenderUseExposureSkySettings` | Boolean | 非GUI | LIVE |  | Use exposure sky settings instead of deriving from HDR scale. |
| 1958 | `RenderUseFarClip` | Boolean | GUI | LIVE |  | If false, frustum culling will ignore far clip plane. |
| 1959 | `RenderUseStreamVBO` | Boolean | GUI | LIVE(XML専用) |  | Use VBO's for stream buffers |
| 1960 | `RenderUseTriStrips` | Boolean | 非GUI | DEAD |  | DEPRECATED - now always assumed to be false - Use triangle strips for  |
| 1961 | `RenderUseVAO` | Boolean | 非GUI | DEAD |  | [EXPERIMENTAL] Use GL Vertex Array Objects. |
| 1962 | `RenderVBOEnable` | Boolean | GUI | LIVE |  | Use GL Vertex Buffer Objects |
| 1963 | `RenderVBOMappingDisable` | Boolean | 非GUI | DEAD |  | Disable VBO glMapBufferARB |
| 1964 | `RenderVSyncEnable` | Boolean | GUI | LIVE |  | Update frames between display scans (FALSE = Update frames as fast as  |
| 1965 | `RenderVolumeLODFactor` | F32 | GUI | LIVE |  | Influences the distance at which the viewer will display a lower detai |
| 1966 | `RenderVolumeSAFrameMax` | F32 | GUI | LIVE |  | The limit(per frame) at which the sum of all volumes above the surface |
| 1967 | `RenderVolumeSAProtection` | Boolean | GUI | LIVE |  | Enables automatic derendering of prims with high surface area (can pro |
| 1968 | `RenderVolumeSAThreshold` | F32 | 非GUI | LIVE |  | The surface area at which volumes begin to be considered for being mad |
| 1969 | `RenderVolumetricLighting` | Boolean | GUI | LIVE |  | AYAstorm r30 P3: enable volumetric lighting (godrays) in Cinematic mod |
| 1970 | `RenderVolumetricLightingDirectional` | Boolean | GUI | LIVE | ✅ | AYAstorm r30 P3: fade volumetric lighting out when the sun is below th |
| 1971 | `RenderVolumetricLightingFalloffMultiplier` | F32 | GUI | LIVE | ✅ | AYAstorm r30 P3: distance falloff multiplier for volumetric lighting ( |
| 1972 | `RenderVolumetricLightingMultiplier` | F32 | GUI | LIVE | ✅ | AYAstorm r30 P3: intensity multiplier for the volumetric lighting comp |
| 1973 | `RenderVolumetricLightingResolution` | U32 | GUI | LIVE | ✅ | AYAstorm r30 P3: godray sample count per pixel for the volumetric ligh |
| 1974 | `RenderWater` | Boolean | 非GUI | LIVE |  | Display water |
| 1975 | `RenderWaterMaterials` | S32 | 非GUI | DEAD |  | Water planar reflections include materials rendering. |
| 1976 | `RenderWaterMipNormal` | Boolean | 非GUI | LIVE |  | Use mip maps for water normal map. |
| 1977 | `RenderWaterRefResolution` | S32 | GUI | LIVE(XML専用) |  | Water planar reflection resolution. |
| 1978 | `RenderWindlightInterpolateTime` | F32 | GUI | LIVE |  | Seconds over which to interpolate when a preset matching the new rende |
| 1979 | `ReplaySession` | Boolean | 非GUI | LIVE |  | Request replay of previously-recorded pilot file |
| 1980 | `ReportBugURL` | String | 非GUI | LIVE |  | URL used for filing bugs from viewer |
| 1981 | `RequestFullRegionCache` | Boolean | 非GUI | LIVE |  | If set, ask sim to send full region object cache. Needs to restart vie |
| 1982 | `ResetToolbarSettings` | Boolean | 非GUI | LIVE |  | If enabled, reset some skin-specific settings next relog |
| 1983 | `ResetUIScaleOnFirstRun` | Boolean | 非GUI | LIVE |  | Resets the UI scale factor on first run due to changed display scaling |
| 1984 | `ResetViewTurnsAvatar` | Boolean | GUI | LIVE |  | This option keeps the camera direction and turns the avatar when Reset |
| 1985 | `RestoreCameraPosOnLogin` | Boolean | 非GUI | LIVE |  | Reset camera position to location at logout |
| 1986 | `RestoreGlobalSettings` | Boolean | GUI | LIVE |  | Defines if the global settings should be restored when doing a setting |
| 1987 | `RestorePerAccountSettings` | Boolean | GUI | LIVE |  | Defines if the per account settings should be restored when doing a se |
| 1988 | `RestrainedLove` | Boolean | GUI | LIVE |  | Toggles the RestrainedLove features (BDSM lockable toys support). Need |
| 1989 | `RestrainedLoveCanOOC` | Boolean | GUI(menu) | LIVE |  | Allows sending OOC chat when send chat restricted, or seeing OOC chat  |
| 1990 | `RestrainedLoveDebug` | Boolean | GUI(menu) | LIVE |  | Toggles the RestrainedLove debug mode (displays the commands when in d |
| 1991 | `RestrainedLoveForbidGiveToRLV` | Boolean | GUI(menu) | LIVE |  | When TRUE, forbids to give sub-folders to the #RLV RestrainedLove fold |
| 1992 | `RestrainedLoveNoSetEnv` | Boolean | 非GUI | LIVE |  | When TRUE, forbids to set the environment (time of day and Windlight s |
| 1993 | `RestrainedLoveReplaceWhenFolderBeginsWith` | String | 非GUI | LIVE |  | If a folder name begins with this string, its attach behavior will alw |
| 1994 | `RestrainedLoveShowEllipsis` | Boolean | GUI(menu) | LIVE |  | When TRUE, show "..." when someone speaks, while the avatar is prevent |
| 1995 | `RestrainedLoveStackWhenFolderBeginsWith` | String | 非GUI | LIVE |  | If a folder name begins with this string, its attach behavior will alw |
| 1996 | `RevokePermsOnStopAnimation` | Boolean | 非GUI | LIVE |  | Clear animation permssions when choosing "Stop Animating Me" |
| 1997 | `RezUnderLandGroup` | Boolean | GUI | LIVE |  | Rez objects under land group when possible. |
| 1998 | `RotateRight` | Boolean | 非GUI | LIVE |  | Make the agent rotate to its right. |
| 1999 | `RotationStep` | F32 | GUI | LIVE |  | All rotations via rotation tool are constrained to multiples of this u |
| 2000 | `RunMultipleThreads` | Boolean | 非GUI | DEAD |  | If TRUE keep background threads active during render. No longer used;  |
| 2001 | `SDL2IMEChatHistoryVerticalOffset` | S32 | 非GUI | LIVE |  | Chat History: Vertical offset to apply to the international input meth |
| 2002 | `SDL2IMEDefaultVerticalOffset` | S32 | 非GUI | LIVE |  | Default vertical offset to apply to the international input method edi |
| 2003 | `SDL2IMEEnabled` | Boolean | GUI | LIVE |  | Enable the international input method editor for Japanese, Chinese, et |
| 2004 | `SDL2IMEMediaVerticalOffset` | S32 | 非GUI | LIVE |  | Media: Vertical offset to apply to the international input method edit |
| 2005 | `SLURLDragNDrop` | Boolean | 非GUI | LIVE |  | Enable drag and drop of SLURLs onto the viewer |
| 2006 | `SLURLPassToOtherInstance` | Boolean | 非GUI | LIVE |  | Pass execution to prevoius viewer instances if there is a given slurl |
| 2007 | `SLURLTeleportDirectly` | Boolean | 非GUI | LIVE |  | Clicking on a slurl will teleport you directly instead of opening plac |
| 2008 | `SafeMode` | Boolean | 非GUI | LIVE(XML専用) |  | Reset preferences, run in safe mode. |
| 2009 | `SaveMinidump` | Boolean | GUI(menu) | DEAD |  | Save minidump for developer debugging on crash |
| 2010 | `SaveMinidumpType` | U32 | 非GUI | DEAD |  | Type of minidump that is created (0 - minimal, 1 - normal [default], 2 |
| 2011 | `ScaleShowAxes` | Boolean | 非GUI | LIVE |  | Show indicator of selected scale axis when scaling |
| 2012 | `ScaleStretchTextures` | Boolean | GUI | LIVE |  | Stretch textures along with object when scaling |
| 2013 | `ScaleUniform` | Boolean | GUI | LIVE |  | Scale selected objects evenly about center of selection |
| 2014 | `SceneLoadFrontPixelThreshold` | F32 | 非GUI | LIVE |  | in pixels, all objects in view frustum whose screen area is greater th |
| 2015 | `SceneLoadHighMemoryBound` | U32 | 非GUI | LIVE |  | in MB, when total memory usage above this threshold, minimum invisible |
| 2016 | `SceneLoadLowMemoryBound` | U32 | 非GUI | LIVE |  | in MB, when total memory usage above this threshold, start to reduce i |
| 2017 | `SceneLoadMinRadius` | F32 | 非GUI | LIVE |  | in meters, all objects (visible or invisible) within this radius will  |
| 2018 | `SceneLoadRearMaxRadiusFraction` | F32 | 非GUI | LIVE |  | a fraction of draw distance beyond which all objects outside of view f |
| 2019 | `SceneLoadRearPixelThreshold` | F32 | 非GUI | LIVE |  | in pixels, all objects out of view frustum whose screen area is greate |
| 2020 | `SceneLoadingMonitorEnabled` | Boolean | 非GUI | LIVE |  | Enabled scene loading monitor if set |
| 2021 | `SceneLoadingMonitorPixelDiffThreshold` | F32 | 非GUI | LIVE |  | Amount of pixels changed required to consider the scene as still loadi |
| 2022 | `SceneLoadingMonitorSampleTime` | F32 | 非GUI | LIVE |  | Time between screen samples when monitor scene load (seconds) |
| 2023 | `ScriptDialogLimitations` | U32 | GUI | LIVE |  | Limits amount of dialogs per script (0 - per object, 1 - per channel,  |
| 2024 | `ScriptDialogsPosition` | S32 | GUI | LIVE |  | Holds the position where script llDialog floaters will show up. 1 = do |
| 2025 | `ScriptHelpFollowsCursor` | Boolean | 非GUI | LIVE |  | Scripting help window updates contents based on script editor contents |
| 2026 | `ScriptsCanShowUI` | Boolean | GUI | LIVE |  | Allow LSL calls (such as LLMapDestination) to spawn viewer UI |
| 2027 | `ScriptsEveryoneCopy` | Boolean | GUI | LIVE?(動的名) |  | Everyone can copy the newly created script |
| 2028 | `ScriptsNextOwnerCopy` | Boolean | GUI | LIVE?(動的名) |  | Newly created scripts can be copied by next owner |
| 2029 | `ScriptsNextOwnerModify` | Boolean | GUI | LIVE?(動的名) |  | Newly created scripts can be modified by next owner |
| 2030 | `ScriptsNextOwnerTransfer` | Boolean | GUI | LIVE?(動的名) |  | Newly created scripts can be resold or given away by next owner |
| 2031 | `ScriptsShareWithGroup` | Boolean | GUI | LIVE?(動的名) |  | Newly created scripts are shared with the currently active group |
| 2032 | `SearchFromAddressBar` | Boolean | 非GUI | LIVE |  | Can enter search queries into navigation address bar |
| 2033 | `SearchURL` | String | 非GUI | LIVE |  | URL for Search website, displayed in the Find floater |
| 2034 | `SearchURLDebug` | String | GUI | LIVE |  | Debug URL for Search website. Overrides any other search URL if DebugS |
| 2035 | `SearchURLOpenSim` | String | 非GUI | LIVE |  | Fallback URL for Search website if the loginservice doesn't provide a  |
| 2036 | `SelectInvisibleObjects` | Boolean | GUI | LIVE |  | Select invisible objects |
| 2037 | `SelectMovableOnly` | Boolean | GUI | LIVE |  | Select only objects you can move |
| 2038 | `SelectOwnedOnly` | Boolean | GUI | LIVE |  | Select only objects you own |
| 2039 | `SelectReflectionProbes` | Boolean | GUI | LIVE |  | Select reflection probes |
| 2040 | `SelectionHighlightAlpha` | F32 | 非GUI | LIVE |  | Opacity of selection highlight (0.0 = completely transparent, 1.0 = co |
| 2041 | `SelectionHighlightAlphaTest` | F32 | 非GUI | LIVE |  | Alpha value below which pixels are displayed on selection highlight li |
| 2042 | `SelectionHighlightThickness` | F32 | 非GUI | LIVE |  | Thickness of selection highlight line (fraction of view distance) |
| 2043 | `SelectionHighlightUAnim` | F32 | 非GUI | LIVE |  | Rate at which texture animates along U direction in selection highligh |
| 2044 | `SelectionHighlightUScale` | F32 | 非GUI | LIVE |  | Scale of texture display on selection highlight line (fraction of text |
| 2045 | `SelectionHighlightVAnim` | F32 | 非GUI | LIVE |  | Rate at which texture animates along V direction in selection highligh |
| 2046 | `SelectionHighlightVScale` | F32 | 非GUI | LIVE |  | Scale of texture display on selection highlight line (fraction of text |
| 2047 | `ServerChoice` | S32 | 非GUI | DEAD |  | [DO NOT MODIFY] Controls which grid you connect to |
| 2048 | `SessionSettingsFile` | String | 非GUI | LIVE |  | Settings that are a applied per session (not saved). |
| 2049 | `SettingsBackupPath` | String | GUI | LIVE |  | Path where settings were last backed up. |
| 2050 | `SettingsNextOwnerModify` | Boolean | GUI | LIVE?(動的名) |  | Newly created Environment setting can be modified by next owner |
| 2051 | `SettingsNextOwnerTransfer` | Boolean | GUI | LIVE?(動的名) |  | Newly created Environment setting can be resold or given away by next  |
| 2052 | `ShareWithGroup` | Boolean | 非GUI | LIVE |  | (obsolete) Newly created objects are shared with the currently active  |
| 2053 | `ShowAdultClassifieds` | Boolean | GUI | LIVE |  | Display results of find classifieds that are flagged as adult |
| 2054 | `ShowAdultEvents` | Boolean | GUI | LIVE |  | Display results of find events that are flagged as adult |
| 2055 | `ShowAdultGroups` | Boolean | 非GUI | DEAD |  | Display results of find groups that are flagged as adult |
| 2056 | `ShowAdultLand` | Boolean | GUI | LIVE |  | Display results of find land sales that are flagged as adult |
| 2057 | `ShowAdultSims` | Boolean | GUI | LIVE |  | Display results of find places or find popular that are in adult sims |
| 2058 | `ShowAllObjectHoverTip` | Boolean | GUI | LIVE |  | Show descriptive tooltip when mouse hovers over non-interactive and in |
| 2059 | `ShowAxes` | Boolean | GUI(menu) | LIVE |  | Render coordinate frame at your position |
| 2060 | `ShowBanLines` | S32 | 非GUI | LIVE |  | Show in-world ban/access borders, 0 - do not show, 1 - show on collisi |
| 2061 | `ShowBetaGrids` | Boolean | 非GUI | DEAD |  | Display the beta grids in the grid selection control. |
| 2062 | `ShowChatMiniIcons` | Boolean | GUI | LIVE |  | Toggles the display of mini icons in chat transcript |
| 2063 | `ShowConsoleWindow` | Boolean | GUI(menu) | LIVE |  | Show log in separate OS window |
| 2064 | `ShowCrosshairs` | Boolean | GUI | LIVE |  | Display crosshairs when in mouselook mode |
| 2065 | `ShowDebugConsole` | Boolean | 非GUI | LIVE |  | Show log in SL window |
| 2066 | `ShowDeviceSettings` | Boolean | GUI | LIVE |  | Show device settings |
| 2067 | `ShowDiscordActivityDetails` | Boolean | 非GUI | LIVE |  | When set, show avatar name on Discord Rich Presence (UNUSED) |
| 2068 | `ShowDiscordActivityState` | Boolean | 非GUI | LIVE |  | When set, show location on Discord Rich Presence (UNUSED) |
| 2069 | `ShowEventRecorderMenuItems` | Boolean | 非GUI | LIVE |  | Whether or not Event Recorder menu choices - Start / Stop event record |
| 2070 | `ShowGroupNoticesTopRight` | Boolean | GUI | LIVE |  | Show group notifications to the top right corner of the screen. |
| 2071 | `ShowHelpOnFirstLogin` | Boolean | 非GUI | LIVE |  | Show Help Floater on first login |
| 2072 | `ShowHoverTips` | Boolean | GUI | LIVE |  | Show descriptive tooltip when mouse hovers over items in world |
| 2073 | `ShowInInventory` | Boolean | GUI | LIVE |  | Automatically opens inventory to show accepted objects |
| 2074 | `ShowLandHoverTip` | Boolean | GUI | LIVE |  | Show descriptive tooltip when mouse hovers over land |
| 2075 | `ShowMatureClassifieds` | Boolean | GUI | LIVE |  | Display results of find classifieds that are flagged as moderate |
| 2076 | `ShowMatureEvents` | Boolean | GUI | LIVE |  | Display results of find events that are flagged as moderate |
| 2077 | `ShowMatureGroups` | Boolean | 非GUI | LIVE |  | Display results of find groups that are flagged as moderate |
| 2078 | `ShowMatureLand` | Boolean | GUI | LIVE |  | Display results of find land sales that are flagged as moderate |
| 2079 | `ShowMatureSims` | Boolean | GUI | LIVE |  | Display results of find places or find popular that are in moderate si |
| 2080 | `ShowMenuBarLocation` | Boolean | GUI | LIVE |  | Show/Hide location info in the menu bar |
| 2081 | `ShowMiniLocationPanel` | Boolean | 非GUI | LIVE |  | Show/hide mini-location panel |
| 2082 | `ShowMyComplexityChanges` | U32 | 非GUI | LIVE |  | How long to show notices about avatar complexity (set to zero to disab |
| 2083 | `ShowNavbarFavoritesPanel` | Boolean | GUI | LIVE |  | Show/hide navigation bar favorites panel |
| 2084 | `ShowNavbarNavigationPanel` | Boolean | GUI | LIVE |  | Show/hide navigation bar navigation panel |
| 2085 | `ShowNearClip` | Boolean | 非GUI | DEAD |  |  |
| 2086 | `ShowNetStats` | Boolean | GUI | LIVE |  | Show the Status Indicators for the Viewer and Network Usage in the Sta |
| 2087 | `ShowNewInventory` | Boolean | GUI | LIVE |  | Automatically views new notecards/textures/landmarks |
| 2088 | `ShowObjectRenderingCost` | Boolean | 非GUI | LIVE |  | Show the object rendering cost in build tools |
| 2089 | `ShowObjectUpdates` | Boolean | 非GUI | LIVE |  | Show when update messages are received for individual objects |
| 2090 | `ShowOfferedInventory` | Boolean | 非GUI | LIVE |  | Show inventory window with last inventory offer selected when receivin |
| 2091 | `ShowOverlayTitle` | Boolean | 非GUI | LIVE |  | Prints watermark text message on screen |
| 2092 | `ShowPGClassifieds` | Boolean | GUI | LIVE |  | Display results of find classifieds that are flagged as general |
| 2093 | `ShowPGEvents` | Boolean | GUI | LIVE |  | Display results of find events that are flagged as general |
| 2094 | `ShowPGGroups` | Boolean | 非GUI | DEAD |  | Display results of find groups that are flagged as general |
| 2095 | `ShowPGLand` | Boolean | GUI | LIVE |  | Display results of find land sales that are flagged as general |
| 2096 | `ShowPGSims` | Boolean | GUI | LIVE |  | Display results of find places or find popular that are in general sim |
| 2097 | `ShowParcelOwners` | Boolean | GUI | LIVE |  |  |
| 2098 | `ShowPermissions` | Boolean | 非GUI | DEAD |  |  |
| 2099 | `ShowPhysicsShapeInEdit` | Boolean | GUI | LIVE |  | show physics shapes while building |
| 2100 | `ShowProfileFloaters` | Boolean | 非GUI | DEAD |  | Shows resident profiles in a floater rather than the side tray |
| 2101 | `ShowPropertyLines` | Boolean | GUI | LIVE |  | Show line overlay demarking property boundaries |
| 2102 | `ShowRadarMinimap` | Boolean | GUI | LIVE |  | Toggle visibility of the embedded minimap in the radar panel |
| 2103 | `ShowScriptErrors` | Boolean | GUI | LIVE |  | Show script errors |
| 2104 | `ShowScriptErrorsLocation` | S32 | GUI | LIVE |  | Show script error in chat (0) or window (1). |
| 2105 | `ShowSearchTopBar` | Boolean | GUI | LIVE(UI framework) |  | Toggles whether the search field is displayed at the top of the viewer |
| 2106 | `ShowSelectionBeam` | Boolean | GUI | LIVE |  | Show selection particle beam when selecting or interacting with object |
| 2107 | `ShowSpecificLODInEdit` | S32 | 非GUI | LIVE |  | Force the display of a specific LOD while editing |
| 2108 | `ShowStartLocation` | Boolean | GUI | LIVE |  | Display starting location menu on login screen |
| 2109 | `ShowStreamMetadata` | U32 | GUI | LIVE |  | Shows stream metadata (artist, title) notifications. (0 = Off, 1 = Not |
| 2110 | `ShowTangentBasis` | Boolean | GUI(menu) | DEAD |  | Render normal and binormal (debugging bump mapping) |
| 2111 | `ShowTunedART` | Boolean | GUI | LIVE |  | Show the current render time not the pre-tuning render time in the ava |
| 2112 | `ShowTutorial` | Boolean | 非GUI | LIVE |  | Show tutorial window on login |
| 2113 | `ShowVoiceChannelPopup` | Boolean | 非GUI | DEAD |  | Controls visibility of the current voice channel popup above the voice |
| 2114 | `ShowVoiceVisualizersInCalls` | Boolean | GUI | LIVE |  | Enables in-world voice visuals, voice gestures and lip-sync while in g |
| 2115 | `ShowVolumeSettingsPopup` | Boolean | 非GUI | DEAD |  | Show individual volume slider for voice, sound effects, etc |
| 2116 | `SimulateFBOFailure` | Boolean | 非GUI | LIVE |  | [DEBUG] Make allocateScreenBuffer return false.  Used to test error ha |
| 2117 | `SingleModeDoubleClickOpenWindow` | Boolean | GUI | LIVE |  | Sets the action for Double-click on folder in single-folder view (0 -  |
| 2118 | `SkinCurrent` | String | 非GUI | LIVE |  | The currently selected skin. |
| 2119 | `SkinCurrentTheme` | String | 非GUI | LIVE |  | The selected theme for the current skin. |
| 2120 | `SkinningSettingsFile` | String | 非GUI | DEAD |  | Client skin color setting file name (per install). |
| 2121 | `SkipBenchmark` | Boolean | 非GUI | LIVE |  | if true, disables running the GPU benchmark at startup       (default  |
| 2122 | `SkyAmbientScale` | F32 | 非GUI | LIVE |  | Controls strength of ambient, or non-directional light from the sun an |
| 2123 | `SkyMoonDefaultPosition` | Vector3 | 非GUI | LIVE |  | Default position of sun in sky (direction in world coordinates) |
| 2124 | `SkyNightColorShift` | Color3 | 非GUI | LIVE |  | Controls moonlight color (base color applied to moon as light source) |
| 2125 | `SkyOverrideSimSunPosition` | Boolean | GUI(menu) | DEAD |  |  |
| 2126 | `SkyPresetName` | String | 非GUI | LIVE |  | Sky preset to use. May be superseded by region settings or by a day cy |
| 2127 | `SkySunDefaultPosition` | Vector3 | 非GUI | LIVE |  | Default position of sun in sky (direction in world coordinates) |
| 2128 | `SnapEnabled` | Boolean | GUI | LIVE |  | Enable snapping to grid |
| 2129 | `SnapMargin` | S32 | 非GUI | LIVE |  | Controls maximum distance between windows before they auto-snap togeth |
| 2130 | `SnapToMouseCursor` | Boolean | 非GUI | LIVE |  | When snapping to grid, center object on nearest grid point to mouse cu |
| 2131 | `SnapshotFormat` | S32 | 非GUI | LIVE |  | Save snapshots in this format (0 = PNG, 1 = JPEG, 2 = BMP) |
| 2132 | `SnapshotLayers` | S32 | 非GUI | DEAD |  | Which layers should be used for a snapshot. |
| 2133 | `SnapshotQuality` | S32 | 非GUI | LIVE |  | Quality setting of postcard JPEGs (0 = worst, 100 = best) |
| 2134 | `SnapshotToDiskQuality` | S32 | 非GUI | DEAD |  | Quality setting of snapshot to disk JPEGs (0 = worst, 100 = best) |
| 2135 | `SnapshotToProfileIncludeLocation` | Boolean | GUI | LIVE(widget経由) |  | Include location information with a snapshot taken to profile. |
| 2136 | `Socks5AuthType` | String | GUI | LIVE |  | Selected Auth mechanism for Socks5 |
| 2137 | `Socks5ProxyEnabled` | Boolean | GUI | LIVE |  | Use Socks5 Proxy |
| 2138 | `Socks5ProxyHost` | String | GUI | LIVE |  | Socks 5 Proxy Host |
| 2139 | `Socks5ProxyPort` | U32 | GUI | LIVE |  | Socks 5 Proxy Port |
| 2140 | `SortFriendsFirst` | Boolean | 非GUI | LIVE |  | Specifies whether friends will be sorted first in Call Log |
| 2141 | `SpeakerParticipantDefaultOrder` | U32 | 非GUI | LIVE |  | Order for displaying speakers in voice controls.  0 = alphabetical. 1  |
| 2142 | `SpeakerParticipantRemoveDelay` | F32 | 非GUI | LIVE |  | Timeout to remove participants who is not in channel before removed fr |
| 2143 | `SpeedTest` | Boolean | 非GUI | LIVE |  | Performance testing mode, no network |
| 2144 | `SpellCheck` | Boolean | GUI | LIVE |  | Enable spellchecking on line and text editors |
| 2145 | `SpellCheckDictionary` | String | 非GUI | LIVE |  | Current primary and secondary dictionaries used for spell checking |
| 2146 | `StarLightShowMapDetails` | Boolean | GUI | LIVE(UI framework) |  | Show the details panel on the side of the World Map |
| 2147 | `StartUpToastLifeTime` | S32 | GUI | LIVE |  | Number of seconds while a Startup toast exist |
| 2148 | `StatsAutoRun` | Boolean | 非GUI | LIVE |  | Play back autopilot |
| 2149 | `StatsFile` | String | 非GUI | DEAD |  | Filename for stats logging output |
| 2150 | `StatsFrametimeEventThreshold` | F32 | 非GUI | LIVE |  | The percentage that the frametime difference must exceed in order to r |
| 2151 | `StatsFrametimeSampleSeconds` | S32 | 非GUI | LIVE |  | The number of seconds to sample extended frametime data (percentiles,  |
| 2152 | `StatsNumRuns` | S32 | 非GUI | LIVE |  | Loop autopilot playback this number of times |
| 2153 | `StatsPilotFile` | String | 非GUI | LIVE |  | Filename for stats logging autopilot path |
| 2154 | `StatsPilotXMLFile` | String | 非GUI | LIVE |  | Filename for stats logging extended autopilot path |
| 2155 | `StatsQuitAfterRuns` | Boolean | 非GUI | LIVE |  | Quit application after this number of autopilot playback runs |
| 2156 | `StatsReportFileInterval` | F32 | 非GUI | LIVE |  | Interval to save viewer stats file data |
| 2157 | `StatsReportMaxDuration` | F32 | 非GUI | LIVE |  | Maximum seconds for viewer stats file data, prevents huge file |
| 2158 | `StatsReportSkipZeroDataSaves` | Boolean | 非GUI | LIVE |  | In viewer stats data file, skip saving entry if there is no data |
| 2159 | `StatsSessionTrackFrameStats` | Boolean | 非GUI | DEAD |  | Track rendering and network statistics |
| 2160 | `StatsSummaryFile` | String | 非GUI | DEAD |  | Filename for stats logging summary |
| 2161 | `Stream3DBinauralRender` | S32 | 非GUI | LIVE |  | AYAstorm r11/r31: Debug-only override for the lite-HRTF DSP on positio |
| 2162 | `Stream3DDebugPlay` | Boolean | 非GUI | LIVE |  | AYAstorm: Toggle to start/stop the 3D Stream debug source. URL is read |
| 2163 | `Stream3DDebugStereoPlay` | Boolean | 非GUI | LIVE |  | AYAstorm M5-a: Toggle to start/stop a stereo 3D Stream debug source. U |
| 2164 | `Stream3DDebugUrl` | String | 非GUI | LIVE |  | AYAstorm: HTTP audio stream URL used for the 3D Stream debug toggle (s |
| 2165 | `Stream3DDescriptionScan` | Boolean | 非GUI | LIVE |  | AYAstorm M8: When false, prim Description tags are no longer scanned ( |
| 2166 | `Stream3DEnabled` | Boolean | GUI | LIVE |  | AYAstorm M8: Master kill switch for the positional stream feature. Whe |
| 2167 | `Stream3DLfeGain` | F32 | 非GUI | LIVE |  | AYAstorm r12.1: Debug-only override for the LFE channel gain multiplie |
| 2168 | `Stream3DMaxConcurrent` | S32 | 非GUI | LIVE |  | AYAstorm M8: Maximum number of simultaneously bound positional streams |
| 2169 | `Stream3DMaxDistance` | F32 | 非GUI | LIVE |  | AYAstorm M3b: Maximum distance in meters within which prims are polled |
| 2170 | `Stream3DOccluderRange` | F32 | 非GUI | LIVE |  | AYAstorm r13: Maximum listener-to-source distance (m) at which OBB occ |
| 2171 | `Stream3DOcclusion` | S32 | 非GUI | LIVE |  | AYAstorm r13: Master sentinel for tag-based OBB occlusion on positiona |
| 2172 | `Stream3DOcclusionRampMs` | F32 | 非GUI | LIVE |  | AYAstorm r13: Ramp time (ms) for the OBB-occlusion direct/reverb facto |
| 2173 | `Stream3DPollInterval` | F32 | 非GUI | LIVE |  | AYAstorm M3b: Seconds between RequestObjectPropertiesFamily re-polls o |
| 2174 | `Stream3DReconnectAttempts` | S32 | 非GUI | LIVE |  | AYAstorm M7: Number of times the manager will try to reopen a position |
| 2175 | `Stream3DRolloffMax` | F32 | 非GUI | LIVE |  | AYAstorm: Distance in meters at which a positional stream is fully att |
| 2176 | `Stream3DRolloffMin` | F32 | 非GUI | LIVE |  | AYAstorm: Distance in meters within which a positional stream plays at |
| 2177 | `Stream3DRoutingDiagnostic` | Boolean | GUI | LIVE |  | AYAstorm r10.x: When true, emit one chat line per fallback case descri |
| 2178 | `Stream3DShowOccluders` | Boolean | GUI | LIVE |  | AYAstorm r13: Render every registered [ayastorm:occlude] prim as an or |
| 2179 | `Stream3DStereoMaxSpeakers` | S32 | 非GUI | LIVE |  | AYAstorm r8: Maximum number of speaker prims allowed in a single distr |
| 2180 | `Stream3DUpmix` | S32 | 非GUI | LIVE |  | AYAstorm r12: Debug-only override for the 2ch→5.1 upmix dispatch on po |
| 2181 | `Stream3DUpmixCenterBleed` | F32 | 非GUI | LIVE |  | AYAstorm r12: Front L/R center-bleed-removal amount in the 2ch→5.1 upm |
| 2182 | `Stream3DUpmixLfeCutoff` | F32 | 非GUI | LIVE |  | AYAstorm r12: Cutoff frequency (Hz) for the LFE-channel low-pass filte |
| 2183 | `Stream3DUpmixRearDelayMs` | F32 | 非GUI | LIVE |  | AYAstorm r12: Base delay (ms) for the rear-channel decorrelation tap i |
| 2184 | `Stream3DUrlPreResolve` | S32 | 非GUI | LIVE |  | AYAstorm r11: viewer-side URL pre-resolve toggle. -1 (default, sentine |
| 2185 | `Stream3DVenueOverride` | String | 非GUI | LIVE |  | AYAstorm r11: Debug-only override for the venue convolution reverb on  |
| 2186 | `Stream3DVenueWetGain` | F32 | 非GUI | LIVE |  | AYAstorm r11: Debug-only override for the venue reverb wet-mix multipl |
| 2187 | `Stream3DVolumeMaster` | F32 | GUI | LIVE |  | AYAstorm M7: Master volume multiplier (0.0 - 1.0) applied to every pri |
| 2188 | `StreamMetadataAnnounceChannel` | S32 | GUI | LIVE |  | Chat channel where stream metadata is announced to. |
| 2189 | `StreamMetadataAnnounceToChat` | Boolean | GUI | LIVE |  | Announces stream metadata to a defined chat channel. |
| 2190 | `SyncMaterialSettings` | Boolean | GUI | LIVE |  | SyncMaterialSettings |
| 2191 | `SysinfoButtonInIM` | Boolean | GUI(menu) | LIVE |  | Shows or hides the system info button in IM floaters. Used to send you |
| 2192 | `SystemLanguage` | String | 非GUI | LIVE |  | Language indicated by system settings (for UI) |
| 2193 | `TabToTextFieldsOnly` | Boolean | 非GUI | LIVE |  | TAB key takes you to next text entry field, instead of next widget |
| 2194 | `TargetFPS` | U32 | GUI | LIVE |  | Desired minimum FPS |
| 2195 | `TeleportArrivalDelay` | F32 | 非GUI | LIVE |  | Time to wait before displaying world during teleport (seconds) |
| 2196 | `TeleportLocalDelay` | F32 | 非GUI | LIVE |  | Delay to prevent teleports after starting an in-sim teleport. (seconds |
| 2197 | `TempAllowScriptedMedia` | Boolean | 非GUI | LIVE |  | Allow scripts to control media |
| 2198 | `TemporaryUpload` | Boolean | GUI | LIVE |  | Temporary texture upload flag (volatile) |
| 2199 | `TerrainColorHeightRange` | F32 | 非GUI | LIVE |  | Altitude range over which a given terrain texture has effect (meters) |
| 2200 | `TerrainColorStartHeight` | F32 | 非GUI | LIVE |  | Starting altitude for terrain texturing (meters) |
| 2201 | `TerrainPaintBitDepth` | U32 | 非GUI | LIVE |  | Bit depth for future terrain paint map operations. Min: 1. Max: 8. Tak |
| 2202 | `TerrainPaintResolution` | U32 | 非GUI | LIVE |  | Resolution of the terrain paint map in pixels. Rounded to a power of t |
| 2203 | `TestGridStatusRSSFromFile` | Boolean | 非GUI | LIVE |  | For testing only: Don't update rss xml file from server. |
| 2204 | `TextureBiasUnimportantFactor` | F32 | 非GUI | DEAD |  | When biasing textures to lower resolution due to lack of vram, the imp |
| 2205 | `TextureCameraBoost` | F32 | 非GUI | LIVE |  | Amount to boost resolution of textures that are important to the camer |
| 2206 | `TextureDecodeDisabled` | Boolean | 非GUI | LIVE |  | If TRUE, do not fetch and decode any textures |
| 2207 | `TextureDisable` | Boolean | GUI(menu) | LIVE |  | If TRUE, do not load textures for in-world content |
| 2208 | `TextureDiscardBackgroundedTime` | F32 | GUI | LIVE |  | Specify how long to wait before discarding texture data after viewer i |
| 2209 | `TextureDiscardLevel` | U32 | GUI | LIVE |  | Specify texture resolution (0 = highest, 5 = lowest) |
| 2210 | `TextureDiscardMinimizedTime` | F32 | GUI | LIVE |  | Specify how long to wait before discarding texture data after viewer i |
| 2211 | `TextureFetchConcurrency` | U32 | 非GUI | LIVE |  | Maximum number of HTTP connections used for texture fetches |
| 2212 | `TextureFetchFakeFailureRate` | F32 | 非GUI | LIVE |  | Simulate HTTP fetch failures for some server bake textures. |
| 2213 | `TextureFetchMinTimeToLog` | F32 | 非GUI | LIVE |  | If texture fetching time exceeds this value, texture fetch tester will |
| 2214 | `TextureFetchUpdateMinCount` | S32 | 非GUI | LIVE |  | Minimum number of textures to update per frame |
| 2215 | `TextureLivePreview` | Boolean | 非GUI | LIVE |  | Preview selections in texture picker or material picker immediately |
| 2216 | `TextureLoadFullRes` | Boolean | GUI(menu) | LIVE |  | If TRUE, always load textures at full resolution (discard = 0). Not pe |
| 2217 | `TextureLoggingThreshold` | U32 | 非GUI | LIVE |  | Specifies the byte threshold at which texture download data should be  |
| 2218 | `TextureNewByteRange` | Boolean | 非GUI | LIVE |  | Use the new more accurate byte range computation for j2c discard level |
| 2219 | `TextureReverseByteRange` | S32 | 非GUI | LIVE |  | Minimal percent of the optimal byte range allowed to render a given di |
| 2220 | `TextureSaveLocation` | String | 非GUI | LIVE |  | Current location for bulk saving textures to disk |
| 2221 | `TextureScaleMaxAreaFactor` | F32 | 非GUI | LIVE |  | Limits how texture scale affects area calculation. |
| 2222 | `TextureScaleMinAreaFactor` | F32 | 非GUI | LIVE |  | Limits how texture scale affects area calculation. |
| 2223 | `ThreadPoolSizes` | LLSD | 非GUI | LIVE |  | Map of size overrides for specific thread pools. |
| 2224 | `ThrottleBandwidthKBPS` | F32 | GUI | LIVE |  | Maximum allowable downstream bandwidth (kilo bits per second) |
| 2225 | `TipToastMessageLineCount` | S32 | 非GUI | LIVE |  | Max line count of text message on tip toast. |
| 2226 | `ToastButtonWidth` | S32 | 非GUI | LIVE |  | Default width of buttons in the toast.  Notes: If required width will  |
| 2227 | `ToastFadingTime` | S32 | GUI | LIVE |  | Number of seconds while a toast is fading |
| 2228 | `ToastGap` | S32 | GUI | LIVE |  | Gap between toasts on a screen (min. value is 5) |
| 2229 | `ToolTipDelay` | F32 | GUI | LIVE |  | Seconds before displaying tooltip when mouse stops over UI element |
| 2230 | `ToolTipFadeTime` | F32 | 非GUI | LIVE |  | Seconds over which tooltip fades away |
| 2231 | `ToolTipFastDelay` | F32 | 非GUI | LIVE |  | Seconds before displaying tooltip when mouse stops over UI element (wh |
| 2232 | `ToolTipVisibleTimeFar` | F32 | 非GUI | LIVE |  | Fade tooltip after time passes (seconds) while mouse not near tooltip |
| 2233 | `ToolTipVisibleTimeNear` | F32 | 非GUI | LIVE |  | Fade tooltip after time passes (seconds) while mouse near tooltip or o |
| 2234 | `ToolTipVisibleTimeOver` | F32 | 非GUI | LIVE |  | Fade tooltip after time passes (seconds) while mouse over tooltip |
| 2235 | `ToolboxAutoMove` | Boolean | 非GUI | LIVE |  | [NOT USED] |
| 2236 | `TrackFocusObject` | Boolean | 非GUI | LIVE |  | Camera tracks last object zoomed on |
| 2237 | `TranslateChat` | Boolean | GUI | LIVE |  | Translate incoming chat messages |
| 2238 | `TranslateLanguage` | String | 非GUI | LIVE |  | Translate specified language |
| 2239 | `TranslationService` | String | 非GUI | LIVE |  | Translation API to use. (google\|azure) |
| 2240 | `TuningFPSStrategy` | U32 | GUI | LIVE |  | Strategy to use when tuning FPS. 0=Tune avatar rendering only, 1=Tune  |
| 2241 | `TutorialURL` | String | 非GUI | LIVE |  | URL for tutorial menu item, set automatically during login |
| 2242 | `TypeAheadTimeout` | F32 | 非GUI | LIVE |  | Time delay before clearing type-ahead buffer in lists (seconds) |
| 2243 | `UIAutoScale` | Boolean | 非GUI | LIVE |  | Keep UI scale consistent across different resolutions |
| 2244 | `UIButtonOrigHPad` | S32 | 非GUI | LIVE |  | UI Button Original Horizontal Pad |
| 2245 | `UICheckboxctrlHPad` | S32 | 非GUI | LIVE |  | UI Checkbox Control Horizontal Pad |
| 2246 | `UICheckboxctrlSpacing` | S32 | 非GUI | LIVE |  | UI Checkbox Control Spacing |
| 2247 | `UICheckboxctrlVPad` | S32 | 非GUI | LIVE |  | UI Checkbox Control Vertical Pad |
| 2248 | `UICloseBoxFromTop` | S32 | 非GUI | LIVE |  | Distance from top of floater to top of close box icon, pixels |
| 2249 | `UIExtraTriangleHeight` | S32 | 非GUI | LIVE |  | UI extra triangle height |
| 2250 | `UIExtraTriangleWidth` | S32 | 非GUI | LIVE |  | UI extra triangle width |
| 2251 | `UIFloaterCloseBoxSize` | S32 | 非GUI | LIVE |  | Size of UI floater close box size |
| 2252 | `UIFloaterHPad` | S32 | 非GUI | DEAD |  | Size of UI floater horizontal pad |
| 2253 | `UIFloaterTestBool` | Boolean | 非GUI | DEAD |  | Example saved setting for the test floater |
| 2254 | `UIFloaterTitleVPad` | S32 | 非GUI | LIVE |  | Distance from top of floater to top of title string, pixels |
| 2255 | `UIImgDefaultAlphaUUID` | String | 非GUI | LIVE |  |  |
| 2256 | `UIImgDefaultEyesUUID` | String | 非GUI | LIVE |  |  |
| 2257 | `UIImgDefaultGlovesUUID` | String | 非GUI | LIVE |  |  |
| 2258 | `UIImgDefaultHairUUID` | String | 非GUI | LIVE |  |  |
| 2259 | `UIImgDefaultJacketUUID` | String | 非GUI | LIVE |  |  |
| 2260 | `UIImgDefaultPantsUUID` | String | 非GUI | LIVE |  |  |
| 2261 | `UIImgDefaultShirtUUID` | String | 非GUI | LIVE |  |  |
| 2262 | `UIImgDefaultShoesUUID` | String | 非GUI | LIVE |  |  |
| 2263 | `UIImgDefaultSkirtUUID` | String | 非GUI | LIVE |  |  |
| 2264 | `UIImgDefaultSocksUUID` | String | 非GUI | LIVE |  |  |
| 2265 | `UIImgDefaultUnderwearUUID` | String | 非GUI | LIVE |  |  |
| 2266 | `UIImgInvisibleUUID` | String | 非GUI | DEAD |  |  |
| 2267 | `UIImgTransparentUUID` | String | 非GUI | LIVE |  |  |
| 2268 | `UILineEditorCursorThickness` | S32 | 非GUI | LIVE |  | UI Line Editor Cursor Thickness |
| 2269 | `UIMaxComboWidth` | S32 | 非GUI | DEAD |  | Maximum width of combo box |
| 2270 | `UIMinimizedWidth` | S32 | 非GUI | LIVE |  | Size of UI floater minimized width |
| 2271 | `UIMultiSliderctrlSpacing` | S32 | 非GUI | LIVE |  | UI multi slider ctrl spacing |
| 2272 | `UIMultiTrackHeight` | S32 | 非GUI | LIVE |  | UI multi track height |
| 2273 | `UIPreeditMarkerBrightness` | F32 | 非GUI | LIVE |  | UI Preedit Marker Brightness |
| 2274 | `UIPreeditMarkerGap` | S32 | 非GUI | LIVE |  | UI Preedit Marker Gap |
| 2275 | `UIPreeditMarkerPosition` | S32 | 非GUI | LIVE |  | UI Preedit Marker Position |
| 2276 | `UIPreeditMarkerThickness` | S32 | 非GUI | LIVE |  | UI Preedit Marker Thickness |
| 2277 | `UIPreeditStandoutBrightness` | F32 | 非GUI | LIVE |  | UI Preedit Standout Brightness |
| 2278 | `UIPreeditStandoutGap` | S32 | 非GUI | LIVE |  | UI Preedit Standout Gap |
| 2279 | `UIPreeditStandoutPosition` | S32 | 非GUI | LIVE |  | UI Preedit Standout Position |
| 2280 | `UIPreeditStandoutThickness` | S32 | 非GUI | LIVE |  | UI Preedit Standout Thickness |
| 2281 | `UIPreviewMaterial` | Boolean | 非GUI | LIVE |  | Whether or not PBR material swatch is enabled |
| 2282 | `UIResizeBarHeight` | S32 | 非GUI | LIVE |  | Size of UI resize bar height |
| 2283 | `UIScaleFactor` | F32 | GUI | LIVE |  | Size of UI relative to default layout on 1024x768 screen |
| 2284 | `UIScrollbarSize` | S32 | 非GUI | LIVE |  | UI scrollbar size |
| 2285 | `UISliderctrlHeight` | S32 | 非GUI | LIVE |  | UI slider ctrl height |
| 2286 | `UISliderctrlSpacing` | S32 | 非GUI | LIVE |  | UI slider ctrl spacing |
| 2287 | `UISndAlert` | String | GUI | LIVE |  | Sound file for alerts (uuid for sound asset) |
| 2288 | `UISndBadKeystroke` | String | 非GUI | LIVE |  | Sound file for invalid keystroke (uuid for sound asset) |
| 2289 | `UISndChatMention` | String | 非GUI | LIVE |  | Sound file for chat mention(uuid for sound asset) |
| 2290 | `UISndChatPing` | String | 非GUI | DEAD |  | Sound file for chat ping(uuid for sound asset) |
| 2291 | `UISndClick` | String | 非GUI | LIVE |  | Sound file for mouse click (uuid for sound asset) |
| 2292 | `UISndClickRelease` | String | 非GUI | LIVE |  | Sound file for mouse button release (uuid for sound asset) |
| 2293 | `UISndDebugSpamToggle` | Boolean | 非GUI | LIVE |  | Log UI sound effects as they are played |
| 2294 | `UISndFootsteps` | String | 非GUI | LIVE |  | Sound file for default footsteps (uuid for sound asset). Change requir |
| 2295 | `UISndFriendOffline` | String | 非GUI | LIVE |  | Sound file for friends going offline (uuid for sound asset) |
| 2296 | `UISndFriendOnline` | String | 非GUI | LIVE |  | Sound file for friends coming online (uuid for sound asset) |
| 2297 | `UISndFriendshipOffer` | String | 非GUI | LIVE |  | Sound file for friendship offer (uuid for sound asset) |
| 2298 | `UISndGroupInvitation` | String | 非GUI | LIVE |  | Sound file for group invitation (uuid for sound asset) |
| 2299 | `UISndGroupNotice` | String | 非GUI | LIVE |  | Sound file for group notice (uuid for sound asset) |
| 2300 | `UISndHealthReductionF` | String | 非GUI | LIVE |  | Sound file for female pain (uuid for sound asset) |
| 2301 | `UISndHealthReductionM` | String | 非GUI | LIVE |  | Sound file for male pain (uuid for sound asset) |
| 2302 | `UISndHealthReductionThreshold` | F32 | 非GUI | LIVE |  | Amount of health reduction required to trigger "pain" sound |
| 2303 | `UISndIncomingVoiceCall` | String | 非GUI | LIVE |  | Sound file for incoming voice call (uuid for sound asset) |
| 2304 | `UISndInvalidOp` | String | 非GUI | LIVE |  | Sound file for invalid operations (uuid for sound asset) |
| 2305 | `UISndInventoryOffer` | String | GUI | LIVE |  | Sound file for inventory offer (uuid for sound asset) |
| 2306 | `UISndMicToggle` | String | 非GUI | LIVE |  | Sound file for microphone toggle (uuid for sound asset) |
| 2307 | `UISndMoneyChangeDown` | String | 非GUI | LIVE |  | Sound file for L$ balance decrease (uuid for sound asset) |
| 2308 | `UISndMoneyChangeThreshold` | F32 | GUI | LIVE |  | Amount of change in L$ balance required to trigger "money" sound |
| 2309 | `UISndMoneyChangeUp` | String | 非GUI | LIVE |  | Sound file for L$ balance increase (uuid for sound asset) |
| 2310 | `UISndMovelockToggle` | String | 非GUI | LIVE |  | Sound file for toggling movelock (uuid for sound asset) |
| 2311 | `UISndNearbyChat` | String | 非GUI | LIVE |  | Sound file for nearby chat messages from other avatars (uuid for sound |
| 2312 | `UISndNewIncomingConfIMSession` | String | 非GUI | LIVE |  | Sound file for new conference instant message session(uuid for sound a |
| 2313 | `UISndNewIncomingGroupIMSession` | String | 非GUI | LIVE |  | Sound file for new group instant message session(uuid for sound asset) |
| 2314 | `UISndNewIncomingIMSession` | String | 非GUI | LIVE |  | Sound file for new instant message session(uuid for sound asset) |
| 2315 | `UISndObjectCreate` | String | 非GUI | LIVE |  | Sound file for object creation (uuid for sound asset) |
| 2316 | `UISndObjectDelete` | String | 非GUI | LIVE |  | Sound file for object deletion (uuid for sound asset) |
| 2317 | `UISndObjectRezIn` | String | 非GUI | LIVE |  | Sound file for rezzing objects (uuid for sound asset) |
| 2318 | `UISndObjectRezOut` | String | 非GUI | LIVE |  | Sound file for derezzing objects (uuid for sound asset) |
| 2319 | `UISndPieMenuAppear` | String | 非GUI | LIVE |  | Sound file for opening pie menu (uuid for sound asset) |
| 2320 | `UISndPieMenuHide` | String | 非GUI | LIVE |  | Sound file for closing pie menu (uuid for sound asset) |
| 2321 | `UISndPieMenuSliceHighlight0` | String | 非GUI | LIVE |  | Sound file for selecting pie menu item 0 (uuid for sound asset) |
| 2322 | `UISndPieMenuSliceHighlight1` | String | 非GUI | LIVE |  | Sound file for selecting pie menu item 1 (uuid for sound asset) |
| 2323 | `UISndPieMenuSliceHighlight2` | String | 非GUI | LIVE |  | Sound file for selecting pie menu item 2 (uuid for sound asset) |
| 2324 | `UISndPieMenuSliceHighlight3` | String | 非GUI | LIVE |  | Sound file for selecting pie menu item 3 (uuid for sound asset) |
| 2325 | `UISndPieMenuSliceHighlight4` | String | 非GUI | LIVE |  | Sound file for selecting pie menu item 4 (uuid for sound asset) |
| 2326 | `UISndPieMenuSliceHighlight5` | String | 非GUI | LIVE |  | Sound file for selecting pie menu item 5 (uuid for sound asset) |
| 2327 | `UISndPieMenuSliceHighlight6` | String | 非GUI | LIVE |  | Sound file for selecting pie menu item 6 (uuid for sound asset) |
| 2328 | `UISndPieMenuSliceHighlight7` | String | 非GUI | LIVE |  | Sound file for selecting pie menu item 7 (uuid for sound asset) |
| 2329 | `UISndQuestionExperience` | String | 非GUI | LIVE |  | Sound file for new experience notification (uuid for sound asset) |
| 2330 | `UISndRadarAgeAlert` | String | GUI | LIVE |  | Sound file played when the age alert for an avatar is triggered (uuid  |
| 2331 | `UISndRadarChatEnter` | String | GUI | LIVE |  | Sound file played when avatars enter chat range (uuid for sound asset) |
| 2332 | `UISndRadarChatLeave` | String | GUI | LIVE |  | Sound file played when avatars leave chat range (uuid for sound asset) |
| 2333 | `UISndRadarDrawEnter` | String | GUI | LIVE |  | Sound file played when avatars enter draw distance (uuid for sound ass |
| 2334 | `UISndRadarDrawLeave` | String | GUI | LIVE |  | Sound file played when avatars leave draw distance (uuid for sound ass |
| 2335 | `UISndRadarSimEnter` | String | GUI | LIVE |  | Sound file played when avatars enter the region (uuid for sound asset) |
| 2336 | `UISndRadarSimLeave` | String | GUI | LIVE |  | Sound file played when avatars leave the region (uuid for sound asset) |
| 2337 | `UISndRestart` | String | 非GUI | LIVE |  | Sound file for region restarting, Second Life grid (uuid for sound ass |
| 2338 | `UISndRestartOpenSim` | String | 非GUI | LIVE |  | Sound file for region restarting, OpenSim grid (uuid for sound asset) |
| 2339 | `UISndScriptFloaterClose` | String | 非GUI | LIVE |  | Sound file for closing a script dialog (uuid for sound asset) |
| 2340 | `UISndScriptFloaterOpen` | String | 非GUI | LIVE |  | Sound file for opening a script dialog (uuid for sound asset) |
| 2341 | `UISndSnapshot` | String | 非GUI | LIVE |  | Sound file for taking a snapshot (uuid for sound asset) |
| 2342 | `UISndStartIM` | String | 非GUI | LIVE |  | Sound file for starting a new IM session (uuid for sound asset) |
| 2343 | `UISndTeleportOffer` | String | GUI | LIVE |  | Sound file for teleport offer (uuid for sound asset) |
| 2344 | `UISndTeleportOut` | String | 非GUI | LIVE |  | Sound file for teleporting (uuid for sound asset) |
| 2345 | `UISndTrackerBeacon` | String | 非GUI | LIVE |  | Sound file for tracker beacon (uuid for sound asset) |
| 2346 | `UISndTyping` | String | 非GUI | LIVE |  | Sound file for starting to type a chat message (uuid for sound asset) |
| 2347 | `UISndWindowClose` | String | 非GUI | LIVE |  | Sound file for closing a window (uuid for sound asset) |
| 2348 | `UISndWindowOpen` | String | 非GUI | LIVE |  | Sound file for opening a window (uuid for sound asset) |
| 2349 | `UISpinctrlBtnHeight` | S32 | 非GUI | LIVE |  | UI spin control button height |
| 2350 | `UISpinctrlBtnWidth` | S32 | 非GUI | LIVE |  | UI spin control button width |
| 2351 | `UISpinctrlSpacing` | S32 | 非GUI | LIVE |  | UI spin control spacing |
| 2352 | `UITabCntrArrowBtnSize` | S32 | 非GUI | LIVE |  | UI Tab Container Arrow Button Size |
| 2353 | `UITabCntrButtonPanelOverlap` | S32 | 非GUI | LIVE |  | UI Tab Container Button Panel Overlap |
| 2354 | `UITabCntrCloseBtnSize` | S32 | 非GUI | LIVE |  | UI Tab Container Close Button Size |
| 2355 | `UITabCntrTabHPad` | S32 | 非GUI | LIVE |  | UI Tab Container Tab Horizontal Pad |
| 2356 | `UITabCntrTabPartialWidth` | S32 | 非GUI | LIVE |  | UI Tab Container Tab Partial Width |
| 2357 | `UITabCntrVertTabMinWidth` | S32 | 非GUI | LIVE |  | UI Tab Container Vertical Tab Minimum Width |
| 2358 | `UITabCntrvArrowBtnSize` | S32 | 非GUI | LIVE |  | UI Tab Container V Arrow Button Size |
| 2359 | `UITabCntrvPad` | S32 | 非GUI | LIVE |  | UI Tab Container V Pad |
| 2360 | `UITabPadding` | S32 | 非GUI | LIVE |  | UI Tab Padding |
| 2361 | `UpdateAppWindowTitleBar` | Boolean | 非GUI | LIVE |  | Updates the application window title bar with brief information about  |
| 2362 | `UpdateRememberPasswordSetting` | Boolean | 非GUI | LIVE |  | Save 'rememeber password' setting for current user. |
| 2363 | `UpdaterServiceSetting` | U32 | 非GUI | LIVE |  | Configure updater service. |
| 2364 | `UpdaterShowReleaseNotes` | S32 | 非GUI | LIVE |  | Enables displaying of the Release notes in a web floater after update. |
| 2365 | `UpdaterWillingToTest` | Boolean | 非GUI | LIVE |  | Whether or not the updater should offer Beta upgrades. |
| 2366 | `UploadBakedTexOld` | Boolean | 非GUI | LIVE |  | Forces the baked texture pipeline to upload using the old method. |
| 2367 | `UploadsEveryoneCopy` | Boolean | GUI | LIVE |  | Everyone can copy the newly uploaded item |
| 2368 | `UploadsNextOwnerCopy` | Boolean | GUI | LIVE |  | Newly uploaded items can be copied by next owner |
| 2369 | `UploadsNextOwnerModify` | Boolean | GUI | LIVE |  | Newly uploaded items can be modified by next owner |
| 2370 | `UploadsNextOwnerTransfer` | Boolean | GUI | LIVE |  | Newly uploaded items can be resold or given away by next owner |
| 2371 | `UploadsShareWithGroup` | Boolean | GUI | LIVE |  | Newly uploaded items are shared with the currently active group |
| 2372 | `Use24HourClock` | Boolean | 非GUI | LIVE |  | 12 vs 24. At the moment coverage is partial |
| 2373 | `UseAltKeyForMenus` | Boolean | 非GUI | LIVE |  | Access menus via keyboard by tapping Alt |
| 2374 | `UseAnimationTimeSteps` | Boolean | 非GUI | LIVE |  | Enable the use of animation timesteps to reduce render load for distan |
| 2375 | `UseAntiSpam` | Boolean | GUI | LIVE |  | Activate the Anti-Spam System |
| 2376 | `UseChatBubbles` | Boolean | GUI | LIVE |  | Show chat above avatars head in chat bubbles |
| 2377 | `UseCircuitCodeMaxRetries` | S32 | 非GUI | LIVE |  | Max timeout count for the initial UseCircuitCode message |
| 2378 | `UseCircuitCodeTimeout` | F32 | 非GUI | LIVE |  | Timeout duration in seconds for the initial UseCircuitCode message |
| 2379 | `UseDebugMenus` | Boolean | GUI | LIVE |  | Turns on "Debug" menu |
| 2380 | `UseDefaultColorPicker` | Boolean | 非GUI | LIVE |  | Use color picker supplied by operating system |
| 2381 | `UseDisplayNames` | Boolean | GUI | LIVE |  | Use new, changeable, unicode names |
| 2382 | `UseEnergy` | Boolean | 非GUI | LIVE |  |  |
| 2383 | `UseFreezeFrame` | Boolean | 非GUI | LIVE |  | Freeze time when taking snapshots. |
| 2384 | `UseGroupMemberPagination` | Boolean | 非GUI | LIVE |  | Enable pagination of group memeber list 50 members at a time. |
| 2385 | `UseHTTPInventory` | Boolean | GUI(menu) | LIVE |  | Allow use of http inventory transfers instead of UDP |
| 2386 | `UseLSLBridge` | Boolean | GUI | LIVE |  | Use the client LSL bridge for lsl functionality |
| 2387 | `UseLegacyIMLogNames` | Boolean | GUI | LIVE |  | Use legacy filenames for P2P IMs logs |
| 2388 | `UseMediaPluginsForStreamingAudio` | Boolean | 非GUI | LIVE |  | Use media plugins (VLC) for streaming audio. |
| 2389 | `UseNewWalkRun` | Boolean | 非GUI | LIVE |  | Replace standard walk/run animations with new ones. |
| 2390 | `UseObjectCacheOcclusion` | Boolean | 非GUI | LIVE |  | Enable object cache level object culling based on occlusion (coverage) |
| 2391 | `UseOcclusion` | Boolean | GUI | LIVE |  | Enable object culling based on occlusion (coverage) by other objects |
| 2392 | `UsePeopleAPI` | Boolean | 非GUI | LIVE |  | Use the people API cap for avatar name fetching, use old legacy protoc |
| 2393 | `UsePhysicsCostOnly` | Boolean | 非GUI | LIVE |  | When displaying physics shapes in edit mode colour ramp based on physi |
| 2394 | `UsePieMenu` | Boolean | GUI | LIVE |  | Use the classic V1.x circular menu instead of the rectangular context  |
| 2395 | `UseStartScreen` | Boolean | 非GUI | LIVE |  | Whether to load a start screen image or not. |
| 2396 | `UseTypingBubbles` | Boolean | GUI | LIVE |  | Show typing indicator in avatar nametags |
| 2397 | `UseWebPagesOnPrims` | Boolean | 非GUI | DEAD |  | [NOT USED] |
| 2398 | `UserConnectionPort` | U32 | 非GUI | LIVE |  | Port that this client transmits on. |
| 2399 | `UserLogFile` | String | 非GUI | LIVE |  | User specified log file name. |
| 2400 | `UserLoginInfo` | String | 非GUI | LIVE |  | User login data. |
| 2401 | `UserLoginInfoCmdLine` | LLSD | 非GUI | LIVE |  | Command line supplied user login data. |
| 2402 | `UserSessionSettingsFile` | String | 非GUI | LIVE |  | User settings that are a applied per session (not saved). |
| 2403 | `VelocityInterpolate` | Boolean | GUI(menu) | LIVE |  | Extrapolate object motion from last packet based on received velocity |
| 2404 | `VersionChannelName` | String | 非GUI | DEAD |  | Version information generated by running the viewer |
| 2405 | `VivoxAutoPostCrashDumps` | Boolean | 非GUI | LIVE |  | If true, SLVoice will automatically send crash dumps directly to Vivox |
| 2406 | `VivoxDebugLevel` | String | 非GUI | LIVE |  | Logging level to use when launching the vivox daemon |
| 2407 | `VivoxDebugSIPURIHostName` | String | 非GUI | LIVE |  | Hostname portion of vivox SIP URIs (empty string for the default). |
| 2408 | `VivoxDebugVoiceAccountServerURI` | String | 非GUI | LIVE |  | URI to the vivox account management server (empty string for the defau |
| 2409 | `VivoxLogDirectory` | String | 非GUI | LIVE |  | Default log path is Application Support/SecondLife/logs specify altern |
| 2410 | `VivoxShutdownTimeout` | String | 非GUI | LIVE |  | shutdown timeout in miliseconds.  The amount of time to wait for the s |
| 2411 | `VivoxVadAuto` | U32 | 非GUI | LIVE |  | A flag indicating if the automatic VAD is enabled (1) or disabled (0). |
| 2412 | `VivoxVadHangover` | U32 | 非GUI | LIVE |  | The time (in milliseconds) that it takes or the VAD to switch back to  |
| 2413 | `VivoxVadNoiseFloor` | U32 | 非GUI | LIVE |  | A dimensionless value between 0 and 20000 (default 576) that controls  |
| 2414 | `VivoxVadSensitivity` | U32 | 非GUI | LIVE |  | A dimensionless value between 0 and 100, indicating the 'sensitivity o |
| 2415 | `VivoxVoiceHost` | String | 非GUI | LIVE |  | Client SLVoice host to connect to |
| 2416 | `VivoxVoicePort` | U32 | 非GUI | LIVE |  | Client SLVoice port to connect to |
| 2417 | `VoiceAutomaticGainControl` | Boolean | GUI | LIVE |  | Voice Automatic Gain Control |
| 2418 | `VoiceCallsRejectAdHoc` | Boolean | GUI | LIVE |  | Silently reject all incoming AdHoc (conference) voice calls. |
| 2419 | `VoiceCallsRejectGroup` | Boolean | GUI | LIVE |  | Silently reject all incoming group voice calls. |
| 2420 | `VoiceCallsRejectP2P` | Boolean | GUI | LIVE |  | Silently reject all incoming P2P (avatar with avatar) voice calls. |
| 2421 | `VoiceDisableMic` | Boolean | 非GUI | LIVE |  | Completely disable the ability to open the mic. |
| 2422 | `VoiceEarLocation` | S32 | GUI | LIVE |  | Location of the virtual ear for voice |
| 2423 | `VoiceEchoCancellation` | Boolean | GUI | LIVE |  | Voice Echo Cancellation |
| 2424 | `VoiceHost` | String | 非GUI | DEAD |  | Client SLVoice host to connect to |
| 2425 | `VoiceImageLevel0` | String | 非GUI | DEAD |  | Texture UUID for voice image level 0 |
| 2426 | `VoiceImageLevel1` | String | 非GUI | DEAD |  | Texture UUID for voice image level 1 |
| 2427 | `VoiceImageLevel2` | String | 非GUI | DEAD |  | Texture UUID for voice image level 2 |
| 2428 | `VoiceImageLevel3` | String | 非GUI | DEAD |  | Texture UUID for voice image level 3 |
| 2429 | `VoiceImageLevel4` | String | 非GUI | DEAD |  | Texture UUID for voice image level 4 |
| 2430 | `VoiceImageLevel5` | String | 非GUI | DEAD |  | Texture UUID for voice image level 5 |
| 2431 | `VoiceImageLevel6` | String | 非GUI | DEAD |  | Texture UUID for voice image level 6 |
| 2432 | `VoiceInputAudioDevice` | String | GUI | LIVE |  | Audio input device to use for voice |
| 2433 | `VoiceLogFile` | String | 非GUI | DEAD |  | Log file to use when launching the voice daemon |
| 2434 | `VoiceMorphingEnabled` | Boolean | 非GUI | LIVE |  | Whether or not to enable Voice Morphs and show the UI. |
| 2435 | `VoiceNoiseSuppressionLevel` | U32 | GUI | LIVE |  | Voice Noise Suppression Level |
| 2436 | `VoiceOutputAudioDevice` | String | GUI | LIVE |  | Audio output device to use for voice |
| 2437 | `VoiceParticipantLeftRemoveDelay` | S32 | 非GUI | LIVE |  | Timeout to remove participants who has left Voice chat from the list i |
| 2438 | `VoicePort` | U32 | 非GUI | DEAD |  | Client SLVoice port to connect to |
| 2439 | `VoiceServerType` | String | 非GUI | LIVE |  | The type of voice server to use for group, conference, and p2p calls. |
| 2440 | `VoiceVisualizerEnabled` | Boolean | GUI | LIVE |  | Display voice dot indicator above an avatar |
| 2441 | `WLSkyDetail` | U32 | GUI | LIVE |  | Controls vertex detail on the WindLight sky.  Lower numbers will give  |
| 2442 | `WarningsAsChat` | Boolean | 非GUI | LIVE |  | Display warning messages in chat transcript |
| 2443 | `WatchdogEnabled` | S32 | 非GUI | LIVE |  | Controls whether the thread watchdog timer is activated. Value is S32. |
| 2444 | `WaterEditPresets` | Boolean | 非GUI | DEAD |  | Whether to be able to edit the water defaults or not |
| 2445 | `WaterFogColor` | Color4 | 非GUI | LIVE |  | Water fog color |
| 2446 | `WaterFogDensity` | F32 | 非GUI | LIVE |  | Water fog density |
| 2447 | `WaterGLFogDensityScale` | F32 | 非GUI | DEAD |  | Maps shader water fog density to gl fog density |
| 2448 | `WaterGLFogDepthFloor` | F32 | 非GUI | LIVE |  | Controls how dark water gl fog can get |
| 2449 | `WaterGLFogDepthScale` | F32 | 非GUI | LIVE |  | Controls how quickly gl fog gets dark under water |
| 2450 | `WaterPresetName` | String | 非GUI | LIVE |  | Water preset to use. May be superseded by region settings. |
| 2451 | `WearFolderLimit` | U32 | 非GUI | LIVE |  | Limits number of items in the folder that can be replaced/added to cur |
| 2452 | `WearablesEveryoneCopy` | Boolean | GUI | LIVE?(動的名) |  | Everyone can copy the newly created clothing or body part |
| 2453 | `WearablesNextOwnerCopy` | Boolean | GUI | LIVE?(動的名) |  | Newly created clothing or body part can be copied by next owner |
| 2454 | `WearablesNextOwnerModify` | Boolean | GUI | LIVE?(動的名) |  | Newly created clothing or body part can be modified by next owner |
| 2455 | `WearablesNextOwnerTransfer` | Boolean | GUI | LIVE?(動的名) |  | Newly created clothing or body part can be resold or given away by nex |
| 2456 | `WearablesShareWithGroup` | Boolean | GUI | LIVE?(動的名) |  | Newly created clothing or body part is shared with the currently activ |
| 2457 | `WebContentWindowLimit` | S32 | GUI | LIVE |  | Maximum number of web browser windows that can be open at once in the  |
| 2458 | `WebProfileFloaterRect` | Rect | 非GUI | DEAD |  | Web profile floater dimensions |
| 2459 | `WindLightUseAtmosShaders` | Boolean | GUI | LIVE |  | DEPRECATED (only true supported) - Whether to enable or disable WindLi |
| 2460 | `WindowHeight` | U32 | 非GUI | LIVE |  | SL viewer window height |
| 2461 | `WindowMaximized` | Boolean | 非GUI | LIVE |  | SL viewer window maximized on login |
| 2462 | `WindowWidth` | U32 | 非GUI | LIVE |  | SL viewer window width |
| 2463 | `WindowX` | S32 | 非GUI | LIVE |  | X coordinate of upper left corner of SL viewer window, relative to upp |
| 2464 | `WindowY` | S32 | 非GUI | LIVE |  | Y coordinate of upper left corner of SL viewer window, relative to upp |
| 2465 | `WorldmapFilterDuplicateLandmarks` | Boolean | GUI | LIVE |  | Filter duplicate Landmarks in the Landmark list on the world map. |
| 2466 | `XferThrottle` | F32 | 非GUI | LIVE |  | Maximum allowable downstream bandwidth for asset transfers (bits per s |
| 2467 | `YawFromMousePosition` | F32 | GUI | LIVE |  | Horizontal range over which avatar head tracks mouse position (degrees |
| 2468 | `YieldTime` | S32 | 非GUI | LIVE |  | Yield some time to the local host. |
| 2469 | `YouAreHereDistance` | F32 | 非GUI | LIVE |  | Radius of distance for banner that indicates if the resident is "on" t |
| 2470 | `ZoomDirect` | Boolean | GUI | LIVE |  | Map Joystick zoom axis directly to camera zoom. |
| 2471 | `ZoomTime` | F32 | GUI | LIVE |  | Time of transition between different camera modes (seconds) |
| 2472 | `_NACL_AntiSpamAmount` | U32 | GUI | LIVE |  | Number of messages needed over _NACL_AntiSpamTime seconds to trigger t |
| 2473 | `_NACL_AntiSpamGlobalQueue` | Boolean | 非GUI | LIVE |  | Do not track types of spam seperately, all actions from a single owner |
| 2474 | `_NACL_AntiSpamNewlines` | U32 | GUI | LIVE |  | Messages with more than these number of lines will be squelched and se |
| 2475 | `_NACL_AntiSpamSoundMulti` | U32 | GUI | LIVE |  | Multiplier for _NACL_AntiSpamTime for sounds heard in _NACL_AntiSpamTi |
| 2476 | `_NACL_AntiSpamSoundPreloadMulti` | U32 | GUI | LIVE |  | Multiplier for _NACL_AntiSpamTime for sound preloads heard in _NACL_An |
| 2477 | `_NACL_AntiSpamTime` | U32 | 非GUI | LIVE |  | Time inverval in seconds by which to track incoming messages. Within t |
| 2478 | `_NACL_LSLPreprocessor` | Boolean | GUI | LIVE |  | LSL Preprocessor |
| 2479 | `_NACL_MLFovValues` | Vector3 | 非GUI | LIVE |  | NaCl Fov zoom values |
| 2480 | `_NACL_PreProcEnableHDDInclude` | Boolean | GUI | LIVE |  | Enable #include from local disk |
| 2481 | `_NACL_PreProcHDDIncludeLocation` | String | GUI | LIVE |  | Path for local disk includes |
| 2482 | `_NACL_PreProcLSLLazyLists` | Boolean | GUI | LIVE |  | LSL Lazy Lists |
| 2483 | `_NACL_PreProcLSLOptimizer` | Boolean | GUI | LIVE |  | LSL Optimizer |
| 2484 | `_NACL_PreProcLSLSwitch` | Boolean | GUI | LIVE |  | LSL Switch Statements |
| 2485 | `_NACL_PreProcLSLTextCompress` | Boolean | 非GUI | LIVE |  | LSL Text Compress |
| 2486 | `always_showable_floaters` | LLSD | 非GUI | LIVE |  | Floaters that can be shown despite mouselook mode |
| 2487 | `fsregioncornerbeacons` | Boolean | GUI | LIVE |  | Show beacons at region corners to help avoid region boundary disconnec |
| 2488 | `max_texture_dimension_X` | S32 | 非GUI | LIVE |  | Maximum texture width for user uploaded textures |
| 2489 | `max_texture_dimension_Y` | S32 | 非GUI | LIVE |  | Maximum texture height for user uploaded textures |
| 2490 | `moapbeacon` | Boolean | GUI | LIVE |  | Beacon / Highlight media on a prim sources |
| 2491 | `moonbeacon` | Boolean | GUI | LIVE |  | Show direction to the Moon |
| 2492 | `particlesbeacon` | Boolean | GUI | LIVE |  | Beacon / Highlight particle generators |
| 2493 | `physicalbeacon` | Boolean | GUI | LIVE |  | Beacon / Highlight physical objects |
| 2494 | `renderbeacons` | Boolean | GUI | LIVE |  | Beacon / Highlight particle generators |
| 2495 | `renderhighlights` | Boolean | GUI | LIVE |  | Beacon / Highlight scripted objects with touch function |
| 2496 | `scriptsbeacon` | Boolean | GUI | LIVE |  | Beacon / Highlight scripted objects |
| 2497 | `scripttouchbeacon` | Boolean | GUI | LIVE |  | Beacon / Highlight scripted objects with touch function |
| 2498 | `soundsbeacon` | Boolean | GUI | LIVE |  | Beacon / Highlight sound generators |
| 2499 | `sourceid` | String | 非GUI | LIVE |  | Identify referring agency to Linden web servers |
| 2500 | `sunbeacon` | Boolean | GUI | LIVE |  | Show direction to the Sun |
| 2501 | `teleport_offer_invitation_max_length` | S32 | 非GUI | LIVE |  | Maximum length of teleport offer invitation line editor. 254 - max_loc |

---

## 付録. OPEN

- 🔴 **`RenderDebugPipeline` のメニュー項目撤去**: 並走者の GL 削除が commit された時点で `gDebugPipeline` の読み手が消え、Developer > Rendering > Debug Pipeline が孤児化する。**`menu_viewer.xml` の項目削除 + settings.xml からの cvar 削除**を GL 削除と同じ commit に含めること(§0.4)。
- **この資料は作業ツリー基準**(並走者の未コミット GL 削除を含む)。commit 後に再生成すれば基準と一致する。
- **§C-2(非GUI DEAD 182 個)は個別精査していない**。冒頭 10 件の確定までに 5 種の誤認が出た以上、機械判定だけで dead を断定してはならない。撤去前に同じ精査(直接参照 / 動的名 / widget 経由 / XML コメント)を必ず通すこと。
- **XUI の属性名 typo**: `invisiblity_control=`(6件)/ `visiblity_control=`(3件)が実在。正は `invisibility_control` / `visibility_control`。LLInitParam が未知属性を落とすなら **bind 無効** = 実挙動未確認。
- **`RenderTonemapMix`**: `pipeline.cpp:8989-8991` で三項演算子を跨いで UBO に入る(手動確認済)。式を跨ぐ代入は自動抽出から漏れるため **§A は下限**。
- **bind 側ゲート**(`RenderDeferred` / `RenderGlow` / `RenderLocalLightCount` / `RenderFSAAType`): shader の中身は変えず「その pass を bind するか」を決める。§A に含めていない = 別軸で要整理。
