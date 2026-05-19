# AYAstorm r30 BD 完全移植 — Phase 0 Inventory

**Phase 名**: r30 BD 完全移植 (BD full port) — Phase 0 Inventory
**Reference BD**: `995a1354d8` (Black Dragon Viewer, Version 5.6.2, 2026-04-19)
**Inventory commit branch**: `feature/ayastorm-r30-bd-full-port-inventory`
**作成日**: 2026-05-19
**作成方針**: ground truth enumeration のみ、判定は行わない (`memory/feedback_bd_full_port_only.md` の唯一の道に基づき、推論で「移植不要」「省略可」を入れない)

---

## §0 本 inventory の位置付け

### §0.1 なぜ inventory が最初か

r30 P2-P5 (velocity buffer / volumetric / DoF chain / parity spec) で「BD から該当機能だけ borrow → AYAstorm 既存パイプラインに足す」approach を 4 回繰り返した結果、

- cvar default / shader 中身 / pipeline 順序 / BD-only 機能のいずれの軸でも BD parity に届かない
- 個別 audit (P5 step 4) を始めても「軽い axis だけで結論」「推論で穴埋め」失敗パターンに戻る

ことが実証された (`memory/feedback_bd_full_port_only.md`)。

唯一の到達手段は **BD 描画処理を完全移植**。その第一歩として、BD と AYAstorm の描画系全 file inventory を取り、移植単位を ground truth で確定する。

### §0.2 出力物の使い方

本 inventory は次工程 (Phase 1 retract 判定 / Phase 2 移植 spec / Phase 3 実装) の **入力**。

- 「BD-only」項目 = AYAstorm に持って来る必要がある (= 移植対象)
- 「AY-only」項目 = BD に無い項目。Cinematic では disable / 除外する候補 (純 BD パスを汚染しない、`memory/feedback_bd_full_port_only.md`)
- 「common かつ diff あり」項目 = BD と AYAstorm で中身が違う。Cinematic では BD 側の中身を採用 = 移植対象 (= AYAstorm 側で BD 中身を bind し直す、または preset で切替)
- 「common かつ diff なし」項目 = そのまま、移植不要

本 inventory 内では **判定はしない**。「BD-only だが省略可」等の punt は本 inventory に書かない (escape 化を避ける)。Phase 1 以降で個別に判定する。

### §0.3 スコープ範囲

5 bucket で網羅:

| Bucket | 範囲 | スクリプト |
|---|---|---|
| 1 | shader (`indra/newview/app_settings/shaders/**/*.glsl`) | `/tmp/bd_inventory_shaders.py` |
| 2 | 描画系 cpp/h (`indra/llrender/**` 全件 + `indra/newview/**` キーワード filter) | `/tmp/bd_inventory_cpp.py` |
| 3 | 描画系 cvar (`indra/newview/app_settings/settings.xml` の Render*/DepthOfField*/Cinematic* etc.) | `/tmp/bd_inventory_cvars.py` |
| 4 | 描画系 UI XML (`indra/newview/skins/default/xui/en/*.xml` キーワード filter) | `/tmp/bd_inventory_ui.py` |
| 5 | environment / camera / filter presets (`indra/newview/app_settings/{windlight,filters,camera}/`) | `/tmp/bd_inventory_presets.py` |

スコープ外 (= 本 inventory に含まれない、別途扱う):

- `indra/newview/character/` (avatar mesh assets) — 描画ロジックではなく data
- `indra/newview/textures/` (engine textures) — 描画ロジックではなく data
- `indra/newview/skins/default/textures/` (UI textures) — 描画ロジックではなく UI 素材
- `lloutfit*` / `llinventory*` / `llappearance*` 等の non-render ロジック
- `indra/llmath`, `indra/llcommon` 等の共通 library (描画固有ではない)
- 3rd party (`autobuild.xml` / `indra/cmake/3rd_party/` 等)

スコープ外 file が描画に影響する場合は、Phase 1 以降の retract/spec 段階で個別に bucket 追加判断する (= 本 inventory も後段で追補可)。

### §0.4 ground truth は sha256 16 文字 prefix

中身 diff の有無は SHA-256 hash の先頭 16 文字で判定。完全一致 = byte 一致。差分の中身解析 (= 行 diff) は Phase 1 以降。

### §0.5 集計 summary (5 bucket 合算)

| Bucket | BD 件数 | AY 件数 | common | BD-only (= 新規移植) | AY-only (= 除外候補) | common diff (= BD 中身採用) |
|---|---:|---:|---:|---:|---:|---:|
| 1. shader | 233 | 245 | 232 | 1 | 13 | 49 |
| 2.A llrender | 52 | 52 | 52 | 0 | 0 | 23 |
| 2.B newview render | 167 | 174 | 161 | 6 | 13 | 92 |
| 3. cvar | 266 | 838 | 259 | 7 | 579 | 33 (value) + 7 (type) |
| 4. UI XML | 37 | 41 | 28 | 9 | 13 | 25 |
| 5. windlight skies | 111 | 769 | 48 | 63 | 721 | (個別表参照) |
| 5. windlight water | (個別表参照) | | | | | |
| 5. windlight days | (個別表参照) | | | | | |
| 5. filters | (個別表参照) | | | | | |
| 5. camera | (個別表参照) | | | | | |

**ハイレベル所感** (= 判定ではなく観察、本文中に escape 化させないため §0 終わりに 1 段引いて記録):

- BD-only 移植対象は 5 bucket 合算で数十〜数百ファイル規模
- AY-only 「Cinematic で除外」候補は 600+ cvar / 13 shader / 13 cpp / 13 UI / 約 1500 windlight preset
- common 中身 diff (= BD 中身を bind し直す要) は 280+ ファイル
- 「数日〜数ヶ月スコープ」と AYA 確定 (`memory/feedback_bd_full_port_only.md`) の根拠が定量化された

---

# Bucket 1: shaders (BD 995a1354d8 vs AYAstorm)

- BD shaders total: **233**
- AY shaders total: **245**
- common: **232**, BD-only: **1**, AY-only: **13**

## 1.1 BD-only shaders (BD にあって AYAstorm に無い、新規移植対象)

- `class1/deferred/motionBlurV.glsl` (1,216 B)

## 1.2 AY-only shaders (AYAstorm にあって BD に無い、移植判断: Cinematic では disable 候補)

- `class1/deferred/SMAAResolveF.glsl` (2,625 B)
- `class1/deferred/SMAAResolveV.glsl` (1,363 B)
- `class1/deferred/fsObjectIDF.glsl` (1,195 B)
- `class1/deferred/fsObjectIDV.glsl` (928 B)
- `class1/deferred/godraysF.glsl` (6,001 B)
- `class1/deferred/godraysV.glsl` (724 B)
- `class1/deferred/rlvF.glsl` (6,258 B)
- `class1/deferred/rlvV.glsl` (814 B)
- `class1/deferred/skinSSSF.glsl` (7,218 B)
- `class1/deferred/skinSSSV.glsl` (746 B)
- `class1/post/exoPostBaseV.glsl` (479 B)
- `class1/post/exoVignetteF.glsl` (731 B)
- `class1/post/snapshotFrameF.glsl` (1,992 B)

## 1.3 common shaders (中身 diff 有無を sha256 で判定)

| # | shader | BD sha (16) | AY sha (16) | diff |
|---|---|---|---|---|
| 1 | `class1/avatar/avatarF.glsl` | `3fff248a9500da1d` | `3fff248a9500da1d` | no |
| 2 | `class1/avatar/avatarSkinV.glsl` | `f7851860139e8a96` | `f7851860139e8a96` | no |
| 3 | `class1/avatar/avatarV.glsl` | `0a18e0ec43473b8c` | `0a18e0ec43473b8c` | no |
| 4 | `class1/avatar/eyeballF.glsl` | `d509ddf18ba89aa5` | `d509ddf18ba89aa5` | no |
| 5 | `class1/avatar/eyeballV.glsl` | `26da3fed15bdf262` | `26da3fed15bdf262` | no |
| 6 | `class1/avatar/objectSkinV.glsl` | `d154e72c54e955d3` | `1f94ed74dd2e0c6a` | yes |
| 7 | `class1/deferred/CASF.glsl` | `d3377af5de23c2a7` | `d3377af5de23c2a7` | no |
| 8 | `class1/deferred/SMAA.glsl` | `c8f506ea72e591ce` | `5b623c77b5ba4992` | yes |
| 9 | `class1/deferred/SMAABlendWeightsF.glsl` | `073e28fdc70df41a` | `9c922deffd1c3de1` | yes |
| 10 | `class1/deferred/SMAABlendWeightsV.glsl` | `e2810170a2ee2d2a` | `e2810170a2ee2d2a` | no |
| 11 | `class1/deferred/SMAAEdgeDetectF.glsl` | `75705164e44e81d2` | `75705164e44e81d2` | no |
| 12 | `class1/deferred/SMAAEdgeDetectV.glsl` | `3b860e61e9ea7ffd` | `3b860e61e9ea7ffd` | no |
| 13 | `class1/deferred/SMAANeighborhoodBlendF.glsl` | `936e6308f56973e1` | `936e6308f56973e1` | no |
| 14 | `class1/deferred/SMAANeighborhoodBlendV.glsl` | `7c979abc5aa53e5b` | `7c979abc5aa53e5b` | no |
| 15 | `class1/deferred/alphaV.glsl` | `d1441f3ec9e41e77` | `d1441f3ec9e41e77` | no |
| 16 | `class1/deferred/aoUtil.glsl` | `a87921f9aea324cb` | `dcabe640b287ba8d` | yes |
| 17 | `class1/deferred/avatarAlphaMaskShadowF.glsl` | `cd811a15b28611f3` | `88620de21145523b` | yes |
| 18 | `class1/deferred/avatarAlphaShadowF.glsl` | `9d74bf04a6c77613` | `1b945262105056f4` | yes |
| 19 | `class1/deferred/avatarAlphaShadowV.glsl` | `184f9545ab83b72c` | `184f9545ab83b72c` | no |
| 20 | `class1/deferred/avatarEyesV.glsl` | `287eb3de55c71561` | `287eb3de55c71561` | no |
| 21 | `class1/deferred/avatarF.glsl` | `992950614133a024` | `f06f26dec51046ab` | yes |
| 22 | `class1/deferred/avatarShadowF.glsl` | `ffdd3ddd278e5ea4` | `ffdd3ddd278e5ea4` | no |
| 23 | `class1/deferred/avatarShadowV.glsl` | `692ca73fa8ff2c92` | `692ca73fa8ff2c92` | no |
| 24 | `class1/deferred/avatarV.glsl` | `4e9ead48083c5d24` | `4e9ead48083c5d24` | no |
| 25 | `class1/deferred/avatarVelocityF.glsl` | `edf3850795b0ece6` | `8e9fdd64bad4ad89` | yes |
| 26 | `class1/deferred/avatarVelocityV.glsl` | `83ce1f6963ce002f` | `6516aa0ac7c29f72` | yes |
| 27 | `class1/deferred/blurLightF.glsl` | `0de390dfd897be41` | `4856ba8d86f28011` | yes |
| 28 | `class1/deferred/blurLightV.glsl` | `b73b4c97d49cca5c` | `b73b4c97d49cca5c` | no |
| 29 | `class1/deferred/bumpF.glsl` | `f3544acaec66376f` | `f3544acaec66376f` | no |
| 30 | `class1/deferred/bumpV.glsl` | `f5ddba692578113e` | `f5ddba692578113e` | no |
| 31 | `class1/deferred/cloudsF.glsl` | `ad9ae56d8abf6c6e` | `e4a781c6db711028` | yes |
| 32 | `class1/deferred/cloudsV.glsl` | `35355923341ec2ba` | `35355923341ec2ba` | no |
| 33 | `class1/deferred/cofF.glsl` | `02c3a04f88cd0713` | `02c3a04f88cd0713` | no |
| 34 | `class1/deferred/deferredUtil.glsl` | `aaa73a1743633b32` | `aaa73a1743633b32` | no |
| 35 | `class1/deferred/diffuseAlphaMaskF.glsl` | `6aff072fb79e946c` | `6aff072fb79e946c` | no |
| 36 | `class1/deferred/diffuseAlphaMaskIndexedF.glsl` | `4b6007b305441816` | `4b6007b305441816` | no |
| 37 | `class1/deferred/diffuseAlphaMaskNoColorF.glsl` | `a897a189109c2a9d` | `a897a189109c2a9d` | no |
| 38 | `class1/deferred/diffuseF.glsl` | `62ac4166901300e1` | `62ac4166901300e1` | no |
| 39 | `class1/deferred/diffuseIndexedF.glsl` | `7cecd4c976feab34` | `7cecd4c976feab34` | no |
| 40 | `class1/deferred/diffuseNoColorV.glsl` | `234983c358b0e302` | `234983c358b0e302` | no |
| 41 | `class1/deferred/diffuseV.glsl` | `9f87409a08d992f3` | `9f87409a08d992f3` | no |
| 42 | `class1/deferred/dofCombineF.glsl` | `911def1f0e594ca9` | `911def1f0e594ca9` | no |
| 43 | `class1/deferred/emissiveF.glsl` | `8dc75b39e8a42c2e` | `8dc75b39e8a42c2e` | no |
| 44 | `class1/deferred/emissiveV.glsl` | `cb4b27eb442db599` | `cb4b27eb442db599` | no |
| 45 | `class1/deferred/exposureF.glsl` | `e7de75b0783c8e76` | `e7de75b0783c8e76` | no |
| 46 | `class1/deferred/fullbrightF.glsl` | `230a61f81f2cb14f` | `230a61f81f2cb14f` | no |
| 47 | `class1/deferred/fullbrightShinyV.glsl` | `d3ffcbb1000f3ec4` | `d3ffcbb1000f3ec4` | no |
| 48 | `class1/deferred/fullbrightV.glsl` | `964907eedc5bd456` | `964907eedc5bd456` | no |
| 49 | `class1/deferred/fxaaF.glsl` | `c196a225c9d75e1d` | `c196a225c9d75e1d` | no |
| 50 | `class1/deferred/gbufferUtil.glsl` | `cda8c78e0f0a4322` | `cda8c78e0f0a4322` | no |
| 51 | `class1/deferred/genbrdflutF.glsl` | `1376aeba21a7361f` | `1376aeba21a7361f` | no |
| 52 | `class1/deferred/genbrdflutV.glsl` | `4e6af2792eab1616` | `4e6af2792eab1616` | no |
| 53 | `class1/deferred/globalF.glsl` | `efefb3fede7ef04b` | `dfc03734d858cda3` | yes |
| 54 | `class1/deferred/highlightF.glsl` | `e6efaa922ab9a960` | `e6efaa922ab9a960` | no |
| 55 | `class1/deferred/impostorF.glsl` | `e6c75406dc1ffab9` | `e6c75406dc1ffab9` | no |
| 56 | `class1/deferred/impostorV.glsl` | `55fc2ea2d67e6d0b` | `55fc2ea2d67e6d0b` | no |
| 57 | `class1/deferred/luminanceF.glsl` | `e4fbe18753260cff` | `e4fbe18753260cff` | no |
| 58 | `class1/deferred/materialF.glsl` | `ce08d349264921fd` | `ce08d349264921fd` | no |
| 59 | `class1/deferred/materialV.glsl` | `5e8eb1af324fb514` | `5e8eb1af324fb514` | no |
| 60 | `class1/deferred/moonF.glsl` | `e83979cd5b6f1cda` | `99988081976e3aa4` | yes |
| 61 | `class1/deferred/moonV.glsl` | `9a5ec1398c2158f6` | `9a5ec1398c2158f6` | no |
| 62 | `class1/deferred/motionBlurF.glsl` | `e2abe833150a4e7e` | `43339b53e7e67880` | yes |
| 63 | `class1/deferred/normgenF.glsl` | `9607eff333d0bab5` | `9607eff333d0bab5` | no |
| 64 | `class1/deferred/normgenV.glsl` | `bf57cbc4deec9a11` | `bf57cbc4deec9a11` | no |
| 65 | `class1/deferred/pbrShadowAlphaBlendF.glsl` | `2509ccc11e3de09b` | `44c0eb1fcc94325c` | yes |
| 66 | `class1/deferred/pbrShadowAlphaMaskF.glsl` | `8430b4d2df9117f9` | `8430b4d2df9117f9` | no |
| 67 | `class1/deferred/pbrShadowAlphaMaskV.glsl` | `9a3242801c0bc8cb` | `9a3242801c0bc8cb` | no |
| 68 | `class1/deferred/pbralphaF.glsl` | `47d3c90f4e596232` | `47d3c90f4e596232` | no |
| 69 | `class1/deferred/pbralphaV.glsl` | `c588976a62e195ac` | `c588976a62e195ac` | no |
| 70 | `class1/deferred/pbrglowF.glsl` | `7cf11c7d402759a4` | `7cf11c7d402759a4` | no |
| 71 | `class1/deferred/pbrglowV.glsl` | `60751aad006286f2` | `60751aad006286f2` | no |
| 72 | `class1/deferred/pbropaqueF.glsl` | `27f4639b6de6a59a` | `47b22751ca46149c` | yes |
| 73 | `class1/deferred/pbropaqueV.glsl` | `2904d651f6043579` | `2904d651f6043579` | no |
| 74 | `class1/deferred/pbrterrainF.glsl` | `ff5342c98ac8d6e8` | `ff5342c98ac8d6e8` | no |
| 75 | `class1/deferred/pbrterrainUtilF.glsl` | `dbad781c9b30a34d` | `dbad781c9b30a34d` | no |
| 76 | `class1/deferred/pbrterrainV.glsl` | `f556c3549e6d09f5` | `f556c3549e6d09f5` | no |
| 77 | `class1/deferred/postDeferredF.glsl` | `48cbb81fe025ee40` | `5d163d691220d8c6` | yes |
| 78 | `class1/deferred/postDeferredGammaCorrect.glsl` | `c405aeaabe3a4bd7` | `c405aeaabe3a4bd7` | no |
| 79 | `class1/deferred/postDeferredHQDoFF.glsl` | `3ed617f664a9561b` | `7ea2d8b1730ea778` | yes |
| 80 | `class1/deferred/postDeferredNoDoFF.glsl` | `82cc6c74101040cf` | `28e4b0fca0b64c59` | yes |
| 81 | `class1/deferred/postDeferredNoTCV.glsl` | `9cf24ae97bb2d26b` | `9cf24ae97bb2d26b` | no |
| 82 | `class1/deferred/postDeferredTonemap.glsl` | `4c3ecc4d0cba0f89` | `45264767dbd63cdd` | yes |
| 83 | `class1/deferred/postDeferredV.glsl` | `74f0428926441ece` | `74f0428926441ece` | no |
| 84 | `class1/deferred/postDeferredVisualizeBuffers.glsl` | `701ee778ddf28db6` | `7decce8479539dca` | yes |
| 85 | `class1/deferred/screenSpaceReflUtil.glsl` | `fb73938f2411d2b4` | `fb73938f2411d2b4` | no |
| 86 | `class1/deferred/shadowAlphaMaskF.glsl` | `37568d0860ca15ea` | `5e7700f1e56164d1` | yes |
| 87 | `class1/deferred/shadowAlphaMaskV.glsl` | `aeb0af003ff866b6` | `aeb0af003ff866b6` | no |
| 88 | `class1/deferred/shadowCubeV.glsl` | `7755c40f7e43c4e2` | `7755c40f7e43c4e2` | no |
| 89 | `class1/deferred/shadowF.glsl` | `0847671b81c0315f` | `0847671b81c0315f` | no |
| 90 | `class1/deferred/shadowSkinnedV.glsl` | `ca782959e7c6b887` | `ca782959e7c6b887` | no |
| 91 | `class1/deferred/shadowUtil.glsl` | `be468e39680d7ed7` | `793646cdb311a4b1` | yes |
| 92 | `class1/deferred/shadowV.glsl` | `bc4c10ab23cbcf7c` | `bc4c10ab23cbcf7c` | no |
| 93 | `class1/deferred/skinnedVelocityAlphaV.glsl` | `4ba48cc42bfe7846` | `8240ed9935ac4fa5` | yes |
| 94 | `class1/deferred/skinnedVelocityV.glsl` | `253069145dfe46ad` | `813578edaa5b7eea` | yes |
| 95 | `class1/deferred/skyF.glsl` | `b314184e58d3dc12` | `5c0be63d8f129ddf` | yes |
| 96 | `class1/deferred/skyV.glsl` | `aca388fcb948fc10` | `27d33ee098011f1b` | yes |
| 97 | `class1/deferred/starsF.glsl` | `396acf13dc5e177b` | `8c908cadb461c2b8` | yes |
| 98 | `class1/deferred/starsV.glsl` | `2c038079ec622c1a` | `2c038079ec622c1a` | no |
| 99 | `class1/deferred/sunDiscF.glsl` | `d67f0c45c02da589` | `1c82d89b719cdcc9` | yes |
| 100 | `class1/deferred/sunDiscV.glsl` | `52e90d73fec8108d` | `52e90d73fec8108d` | no |
| 101 | `class1/deferred/terrainF.glsl` | `b8273228f12de674` | `b8273228f12de674` | no |
| 102 | `class1/deferred/terrainV.glsl` | `20d408824a96cbc1` | `20d408824a96cbc1` | no |
| 103 | `class1/deferred/textureUtilV.glsl` | `0bb7b2ff004685ec` | `0bb7b2ff004685ec` | no |
| 104 | `class1/deferred/tonemapUtilF.glsl` | `33bdd821336fbdde` | `8bdff7c75403e4ed` | yes |
| 105 | `class1/deferred/treeF.glsl` | `976e2918ecd19418` | `976e2918ecd19418` | no |
| 106 | `class1/deferred/treeShadowF.glsl` | `6804242e8bb74b46` | `6804242e8bb74b46` | no |
| 107 | `class1/deferred/treeShadowSkinnedV.glsl` | `438df5df4b85e61b` | `438df5df4b85e61b` | no |
| 108 | `class1/deferred/treeShadowV.glsl` | `039de53ba0766666` | `039de53ba0766666` | no |
| 109 | `class1/deferred/treeV.glsl` | `dbe77e5198bad093` | `dbe77e5198bad093` | no |
| 110 | `class1/deferred/velocityAlphaF.glsl` | `ecba19d20b3040e2` | `930ade3ce652aa5b` | yes |
| 111 | `class1/deferred/velocityAlphaV.glsl` | `58b5d46156742c5e` | `3537102b85a74403` | yes |
| 112 | `class1/deferred/velocityF.glsl` | `7a403c0ff3711fcc` | `a8c583e5fb7fe1f3` | yes |
| 113 | `class1/deferred/velocityFuncV.glsl` | `6c69a0b1c4a16afb` | `c636f51b0a6c3ea0` | yes |
| 114 | `class1/deferred/velocityV.glsl` | `6bdd1b03f8774780` | `654d0af317aa970f` | yes |
| 115 | `class1/deferred/volumetricLightF.glsl` | `9fcde93ae00233c1` | `8bf17e097e1d152f` | yes |
| 116 | `class1/effects/glowExtractF.glsl` | `eca93ab36142c803` | `eca93ab36142c803` | no |
| 117 | `class1/effects/glowExtractV.glsl` | `9d736a442992e135` | `9d736a442992e135` | no |
| 118 | `class1/effects/glowF.glsl` | `4ebf4be8e14324be` | `4ebf4be8e14324be` | no |
| 119 | `class1/effects/glowV.glsl` | `7443a9b1f1da9f21` | `7443a9b1f1da9f21` | no |
| 120 | `class1/environment/srgbF.glsl` | `f76a95c7c9018247` | `f76a95c7c9018247` | no |
| 121 | `class1/environment/waterF.glsl` | `29562365dbdb5db7` | `29562365dbdb5db7` | no |
| 122 | `class1/environment/waterFogF.glsl` | `6f87d9d5c47e2562` | `6f87d9d5c47e2562` | no |
| 123 | `class1/environment/waterV.glsl` | `f68c7ce3ec373690` | `f68c7ce3ec373690` | no |
| 124 | `class1/gltf/pbrmetallicroughnessF.glsl` | `59f494ebb9e4e7a3` | `59f494ebb9e4e7a3` | no |
| 125 | `class1/gltf/pbrmetallicroughnessV.glsl` | `2cecdc9d6c969e6c` | `2cecdc9d6c969e6c` | no |
| 126 | `class1/interface/alphamaskF.glsl` | `7bee3764389e1d30` | `7bee3764389e1d30` | no |
| 127 | `class1/interface/alphamaskV.glsl` | `c251258cfd013dc0` | `c251258cfd013dc0` | no |
| 128 | `class1/interface/benchmarkF.glsl` | `542989c4c4fa3373` | `542989c4c4fa3373` | no |
| 129 | `class1/interface/benchmarkV.glsl` | `ed21cb91456fa45e` | `ed21cb91456fa45e` | no |
| 130 | `class1/interface/clipF.glsl` | `415aed7731c07654` | `415aed7731c07654` | no |
| 131 | `class1/interface/clipV.glsl` | `b04e007fd34bdebc` | `b04e007fd34bdebc` | no |
| 132 | `class1/interface/copyF.glsl` | `ba4712d30a315892` | `ba4712d30a315892` | no |
| 133 | `class1/interface/copyV.glsl` | `ec0ec13f4739a863` | `ec0ec13f4739a863` | no |
| 134 | `class1/interface/debugF.glsl` | `d3519ee8f12dfc85` | `d3519ee8f12dfc85` | no |
| 135 | `class1/interface/debugV.glsl` | `37420d08f507e7b3` | `37420d08f507e7b3` | no |
| 136 | `class1/interface/gaussianF.glsl` | `dd8cd5b1ca57f885` | `dd8cd5b1ca57f885` | no |
| 137 | `class1/interface/glowcombineF.glsl` | `06a5e23ecd1e96d3` | `f73fcce8bf889e8b` | yes |
| 138 | `class1/interface/glowcombineFXAAF.glsl` | `bae98b97fdeceb92` | `bae98b97fdeceb92` | no |
| 139 | `class1/interface/glowcombineFXAAV.glsl` | `25061bc51b002900` | `25061bc51b002900` | no |
| 140 | `class1/interface/glowcombineV.glsl` | `4b8c1226872d007b` | `4b8c1226872d007b` | no |
| 141 | `class1/interface/highlightF.glsl` | `cf515b605211d302` | `cf515b605211d302` | no |
| 142 | `class1/interface/highlightNormV.glsl` | `6a8324db68841b67` | `6a8324db68841b67` | no |
| 143 | `class1/interface/highlightSpecV.glsl` | `08e5940f7dec27d2` | `08e5940f7dec27d2` | no |
| 144 | `class1/interface/highlightV.glsl` | `deb7d8b26820de49` | `deb7d8b26820de49` | no |
| 145 | `class1/interface/irradianceGenV.glsl` | `eb10a55723d75961` | `eb10a55723d75961` | no |
| 146 | `class1/interface/normaldebugF.glsl` | `210fdb35e0855e13` | `210fdb35e0855e13` | no |
| 147 | `class1/interface/normaldebugG.glsl` | `4c1c9f79d7decae7` | `4c1c9f79d7decae7` | no |
| 148 | `class1/interface/normaldebugV.glsl` | `184ca52833be409e` | `184ca52833be409e` | no |
| 149 | `class1/interface/occlusionCubeV.glsl` | `625c70d449c87564` | `625c70d449c87564` | no |
| 150 | `class1/interface/occlusionF.glsl` | `c8932d623e100103` | `c8932d623e100103` | no |
| 151 | `class1/interface/occlusionSkinnedV.glsl` | `597b93bd01b9bfd5` | `597b93bd01b9bfd5` | no |
| 152 | `class1/interface/occlusionV.glsl` | `63d02007a647ab8a` | `63d02007a647ab8a` | no |
| 153 | `class1/interface/onetexturefilterF.glsl` | `d735fc62ee472d64` | `d735fc62ee472d64` | no |
| 154 | `class1/interface/onetexturefilterV.glsl` | `d6c4adfefe2e6a6e` | `d6c4adfefe2e6a6e` | no |
| 155 | `class1/interface/pathfindingF.glsl` | `7688b91c7a721b8f` | `7688b91c7a721b8f` | no |
| 156 | `class1/interface/pathfindingNoNormalV.glsl` | `29375f92b99524fd` | `29375f92b99524fd` | no |
| 157 | `class1/interface/pathfindingV.glsl` | `ab236dc814c15129` | `ab236dc814c15129` | no |
| 158 | `class1/interface/pbrTerrainBakeF.glsl` | `49e18ad5f3dbdb89` | `49e18ad5f3dbdb89` | no |
| 159 | `class1/interface/pbrTerrainBakeV.glsl` | `3ef584e9b66836e2` | `3ef584e9b66836e2` | no |
| 160 | `class1/interface/radianceGenF.glsl` | `b896baa21d67262a` | `b896baa21d67262a` | no |
| 161 | `class1/interface/radianceGenV.glsl` | `2cb3bee988d913bb` | `2cb3bee988d913bb` | no |
| 162 | `class1/interface/reflectionmipF.glsl` | `45949f350ed60dfa` | `45949f350ed60dfa` | no |
| 163 | `class1/interface/solidcolorF.glsl` | `dd18cb6cf1ff697a` | `dd18cb6cf1ff697a` | no |
| 164 | `class1/interface/solidcolorV.glsl` | `8451eba0a5012e2e` | `8451eba0a5012e2e` | no |
| 165 | `class1/interface/splattexturerectV.glsl` | `26118606f53fbec4` | `26118606f53fbec4` | no |
| 166 | `class1/interface/twotexturecompareF.glsl` | `0e042ec8bf9db265` | `0e042ec8bf9db265` | no |
| 167 | `class1/interface/twotexturecompareV.glsl` | `eebe8c88e4cf30ad` | `eebe8c88e4cf30ad` | no |
| 168 | `class1/interface/uiF.glsl` | `93cdfc8e6b3b604a` | `93cdfc8e6b3b604a` | no |
| 169 | `class1/interface/uiV.glsl` | `cd19b4abe04be0b9` | `cd19b4abe04be0b9` | no |
| 170 | `class1/lighting/lightAlphaMaskF.glsl` | `7805eebf180dbe67` | `7805eebf180dbe67` | no |
| 171 | `class1/lighting/lightAlphaMaskNonIndexedF.glsl` | `49b5b76090189c16` | `49b5b76090189c16` | no |
| 172 | `class1/lighting/lightF.glsl` | `59bd7e7aa2e345f5` | `59bd7e7aa2e345f5` | no |
| 173 | `class1/lighting/lightFuncSpecularV.glsl` | `72a464542d3f0fab` | `72a464542d3f0fab` | no |
| 174 | `class1/lighting/lightFuncV.glsl` | `5c44786934dac655` | `5c44786934dac655` | no |
| 175 | `class1/lighting/lightNonIndexedF.glsl` | `9aa8ac74e20da815` | `9aa8ac74e20da815` | no |
| 176 | `class1/lighting/lightSpecularV.glsl` | `62a0312fe771f0f2` | `62a0312fe771f0f2` | no |
| 177 | `class1/lighting/sumLightsSpecularV.glsl` | `d94b8cf0a1e9453a` | `d94b8cf0a1e9453a` | no |
| 178 | `class1/lighting/sumLightsV.glsl` | `e1eeffc1212365cb` | `e1eeffc1212365cb` | no |
| 179 | `class1/objects/bumpF.glsl` | `09f3e16c61c760af` | `09f3e16c61c760af` | no |
| 180 | `class1/objects/bumpV.glsl` | `cfc7f3494f0f45ba` | `cfc7f3494f0f45ba` | no |
| 181 | `class1/objects/impostorF.glsl` | `0165bd180cdcf342` | `0165bd180cdcf342` | no |
| 182 | `class1/objects/impostorV.glsl` | `5ef722684d3aa40a` | `5ef722684d3aa40a` | no |
| 183 | `class1/objects/indexedTextureV.glsl` | `901ab651ca22bf6d` | `901ab651ca22bf6d` | no |
| 184 | `class1/objects/nonindexedTextureV.glsl` | `0b975424aa711330` | `0b975424aa711330` | no |
| 185 | `class1/objects/previewF.glsl` | `5d96367d69442614` | `5d96367d69442614` | no |
| 186 | `class1/objects/previewPhysicsF.glsl` | `fdfb15f329ee11a9` | `fdfb15f329ee11a9` | no |
| 187 | `class1/objects/previewPhysicsV.glsl` | `3a72c081c2804145` | `3a72c081c2804145` | no |
| 188 | `class1/objects/previewV.glsl` | `e813ba8dff67a4db` | `32c74abcc371ec5b` | yes |
| 189 | `class1/objects/simpleColorF.glsl` | `cf027b6c5ebe3205` | `cf027b6c5ebe3205` | no |
| 190 | `class1/objects/simpleF.glsl` | `4ba44c4c04c161c2` | `4ba44c4c04c161c2` | no |
| 191 | `class1/objects/simpleNoAtmosV.glsl` | `9ac5fb6b070cdb83` | `9ac5fb6b070cdb83` | no |
| 192 | `class1/objects/simpleNoColorV.glsl` | `648203af6a80760c` | `648203af6a80760c` | no |
| 193 | `class1/windlight/atmosphericsF.glsl` | `7076154bb55a26d8` | `7076154bb55a26d8` | no |
| 194 | `class1/windlight/atmosphericsFuncs.glsl` | `c2a7078956b4ecfb` | `8cd8235003ca694d` | yes |
| 195 | `class1/windlight/atmosphericsHelpersF.glsl` | `519fb1bf197b2259` | `519fb1bf197b2259` | no |
| 196 | `class1/windlight/atmosphericsHelpersV.glsl` | `b033abbc5a3e6caf` | `b033abbc5a3e6caf` | no |
| 197 | `class1/windlight/atmosphericsV.glsl` | `c1f2b057433b7e7e` | `c1f2b057433b7e7e` | no |
| 198 | `class1/windlight/atmosphericsVarsF.glsl` | `6f25f37b13d6443e` | `6f25f37b13d6443e` | no |
| 199 | `class1/windlight/atmosphericsVarsV.glsl` | `944e1ba3a6b041c6` | `944e1ba3a6b041c6` | no |
| 200 | `class1/windlight/gammaF.glsl` | `bfff3d109bb6e7f1` | `bfff3d109bb6e7f1` | no |
| 201 | `class2/deferred/alphaF.glsl` | `b31f91d81f1cbaa4` | `b31f91d81f1cbaa4` | no |
| 202 | `class2/deferred/pbralphaF.glsl` | `316738cdadda91b9` | `316738cdadda91b9` | no |
| 203 | `class2/deferred/reflectionProbeF.glsl` | `e3deda45d3571268` | `e3deda45d3571268` | no |
| 204 | `class2/deferred/softenLightV.glsl` | `4bf221d03026de74` | `4bf221d03026de74` | no |
| 205 | `class2/deferred/sunLightF.glsl` | `a5bf4c4f348f775b` | `a5bf4c4f348f775b` | no |
| 206 | `class2/deferred/sunLightSSAOF.glsl` | `1ad7ce3324cd06d1` | `637beb5c7bf59d28` | yes |
| 207 | `class2/deferred/sunLightV.glsl` | `ebf147dc5820ddcf` | `ebf147dc5820ddcf` | no |
| 208 | `class2/interface/irradianceGenF.glsl` | `71d14ceda80d3fb6` | `71d14ceda80d3fb6` | no |
| 209 | `class2/interface/reflectionprobeF.glsl` | `52f18887f773dbbc` | `52f18887f773dbbc` | no |
| 210 | `class2/interface/reflectionprobeV.glsl` | `3b5722836989e9e9` | `3b5722836989e9e9` | no |
| 211 | `class3/deferred/fullbrightShinyF.glsl` | `6ee57a5bf436904c` | `6ee57a5bf436904c` | no |
| 212 | `class3/deferred/hazeF.glsl` | `29847931a7e2f58c` | `29847931a7e2f58c` | no |
| 213 | `class3/deferred/materialF.glsl` | `5f7a9d59a2c0a98f` | `7595e5352baac944` | yes |
| 214 | `class3/deferred/multiPointLightF.glsl` | `f1d315e4c8537a4b` | `d9b4b100cc3e3439` | yes |
| 215 | `class3/deferred/multiPointLightV.glsl` | `e4fdc38f1fb6b718` | `e4fdc38f1fb6b718` | no |
| 216 | `class3/deferred/pointLightF.glsl` | `bc418b368a2660f8` | `a49b14fa107bdae1` | yes |
| 217 | `class3/deferred/pointLightV.glsl` | `c34341da34b53254` | `c34341da34b53254` | no |
| 218 | `class3/deferred/reflectionProbeF.glsl` | `b164b92d252e7fd7` | `0df0a3f2e2478823` | yes |
| 219 | `class3/deferred/screenSpaceReflPostF.glsl` | `beef69fd1e344764` | `beef69fd1e344764` | no |
| 220 | `class3/deferred/screenSpaceReflPostV.glsl` | `fa227779f9907180` | `fa227779f9907180` | no |
| 221 | `class3/deferred/screenSpaceReflUtil.glsl` | `969f523bfa3be094` | `c1d025efd9418a7a` | yes |
| 222 | `class3/deferred/softenLightF.glsl` | `f39255476b221770` | `d11728ad978901a6` | yes |
| 223 | `class3/deferred/spotLightF.glsl` | `ad46a2d2630341f0` | `abc46a89c1038b7f` | yes |
| 224 | `class3/deferred/volumetricLightF.glsl` | `d2cf3341a56337b9` | `0710e66584fd7ee7` | yes |
| 225 | `class3/deferred/waterHazeF.glsl` | `cede595cd4095a71` | `cede595cd4095a71` | no |
| 226 | `class3/deferred/waterHazeV.glsl` | `451e87d506ff0bbc` | `451e87d506ff0bbc` | no |
| 227 | `class3/environment/underWaterF.glsl` | `5327aeaf5ac8c1f2` | `5327aeaf5ac8c1f2` | no |
| 228 | `class3/environment/waterF.glsl` | `de4c89aacf1935b4` | `685c777c99cff079` | yes |
| 229 | `class3/lighting/lightV.glsl` | `68b5a1cf81dd207c` | `68b5a1cf81dd207c` | no |
| 230 | `class3/lighting/sumLightsSpecularV.glsl` | `00685342307943a2` | `00685342307943a2` | no |
| 231 | `errorF.glsl` | `654a3c065b1f784d` | `654a3c065b1f784d` | no |
| 232 | `errorV.glsl` | `f97df05c372db44b` | `f97df05c372db44b` | no |

**common 中身 diff 集計**: 49 / 232 件で中身差分あり
# Bucket 2: render cpp/h (BD 995a1354d8 vs AYAstorm)

Two sub-buckets:

- **2.A llrender library** — full enumeration (no filter)
- **2.B newview render-related cpp/h** — filename keyword filter, see script header

## 2.A llrender library

- BD files: **52**, AY files: **52**
- common: **52**, BD-only: **0**, AY-only: **0**

### 2.A llrender library BD-only

(none)

### 2.A llrender library AY-only

(none)

### 2.A llrender library common (sha256 diff)

| # | file | BD sha | AY sha | diff |
|---|---|---|---|---|
| 1 | `llrender/llcubemap.cpp` | `1f633a6635ebbe44` | `1f633a6635ebbe44` | no |
| 2 | `llrender/llcubemap.h` | `85aff9603f8b6edb` | `85aff9603f8b6edb` | no |
| 3 | `llrender/llcubemaparray.cpp` | `bdb5115dcefa66ef` | `29d4a0991576cea5` | yes |
| 4 | `llrender/llcubemaparray.h` | `2f956683ee99f79e` | `2f956683ee99f79e` | no |
| 5 | `llrender/llfontbitmapcache.cpp` | `5a7b1736bb112d05` | `5a7b1736bb112d05` | no |
| 6 | `llrender/llfontbitmapcache.h` | `026027cc4098e0de` | `026027cc4098e0de` | no |
| 7 | `llrender/llfontfreetype.cpp` | `861cefb67d847e9d` | `3b6a40f1cc6c1905` | yes |
| 8 | `llrender/llfontfreetype.h` | `1bb8e004d52f44eb` | `42ce7f82ec59bb8b` | yes |
| 9 | `llrender/llfontfreetypesvg.cpp` | `1fec4d4d7d1bc029` | `1fec4d4d7d1bc029` | no |
| 10 | `llrender/llfontfreetypesvg.h` | `1242ca08e06f0d59` | `1242ca08e06f0d59` | no |
| 11 | `llrender/llfontgl.cpp` | `d6dc500721fa7a39` | `dd6bbb7a53654009` | yes |
| 12 | `llrender/llfontgl.h` | `c8e5a6c1a2240515` | `f2fd9b5c61a0aacb` | yes |
| 13 | `llrender/llfontregistry.cpp` | `758668c4e3a2738a` | `90f65991d868f4ea` | yes |
| 14 | `llrender/llfontregistry.h` | `5fd2a9dad0ac223d` | `b75650b2c6653fad` | yes |
| 15 | `llrender/llfontvertexbuffer.cpp` | `5cc4422bf1612d10` | `5cc4422bf1612d10` | no |
| 16 | `llrender/llfontvertexbuffer.h` | `7830d5dfbbcdc8aa` | `7830d5dfbbcdc8aa` | no |
| 17 | `llrender/llgl.cpp` | `ac576dbc1cad2d78` | `55457b5ec28adc0d` | yes |
| 18 | `llrender/llgl.h` | `3b17ff15b5310af9` | `d349494a77125369` | yes |
| 19 | `llrender/llglcommonfunc.cpp` | `8b7c024bb1ae528f` | `8b7c024bb1ae528f` | no |
| 20 | `llrender/llglcommonfunc.h` | `00bd934f5a376480` | `00bd934f5a376480` | no |
| 21 | `llrender/llglheaders.h` | `afc03ef44257fb38` | `8dc32c062ca88bd1` | yes |
| 22 | `llrender/llglslshader.cpp` | `c95105a98c1befc5` | `c95105a98c1befc5` | no |
| 23 | `llrender/llglslshader.h` | `4f73491f116bcaaa` | `2cf1cf2617bcb668` | yes |
| 24 | `llrender/llglstates.h` | `bbb2370bb8e98b47` | `bbb2370bb8e98b47` | no |
| 25 | `llrender/llgltexture.cpp` | `e0273a038061bd8c` | `8676b8162a9a258d` | yes |
| 26 | `llrender/llgltexture.h` | `a93332ed9005f465` | `9449aced76890e40` | yes |
| 27 | `llrender/llgltypes.h` | `7689b9b0751904ac` | `7689b9b0751904ac` | no |
| 28 | `llrender/llimagegl.cpp` | `8dfe5f7eb8a7d94c` | `ca04da48342f3a25` | yes |
| 29 | `llrender/llimagegl.h` | `d9a05ec0efb7bc8f` | `6ec9e60e30e0ccec` | yes |
| 30 | `llrender/llpostprocess.cpp` | `09fc6a1e6a5139ee` | `09fc6a1e6a5139ee` | no |
| 31 | `llrender/llpostprocess.h` | `253869d35609119c` | `253869d35609119c` | no |
| 32 | `llrender/llrender.cpp` | `d0b810d8be989a77` | `73218f1e6a3269d5` | yes |
| 33 | `llrender/llrender.h` | `243aecd789f39028` | `9d036ac43fe77821` | yes |
| 34 | `llrender/llrender2dutils.cpp` | `781ff813b7f6e4ae` | `edff1a7fb76fb406` | yes |
| 35 | `llrender/llrender2dutils.h` | `331c43df6ca6a52b` | `331c43df6ca6a52b` | no |
| 36 | `llrender/llrendernavprim.cpp` | `f5cdcc85a7c5ab8c` | `f5cdcc85a7c5ab8c` | no |
| 37 | `llrender/llrendernavprim.h` | `56b4601ad992a4ae` | `56b4601ad992a4ae` | no |
| 38 | `llrender/llrendersphere.cpp` | `f4aa8441ffbd109c` | `f4aa8441ffbd109c` | no |
| 39 | `llrender/llrendersphere.h` | `965748bbcb04c56e` | `965748bbcb04c56e` | no |
| 40 | `llrender/llrendertarget.cpp` | `b8e8d779c21c4739` | `50e3ac958ea1251d` | yes |
| 41 | `llrender/llrendertarget.h` | `374c0f2097e202a5` | `374c0f2097e202a5` | no |
| 42 | `llrender/llshadermgr.cpp` | `2c71b52f3b4aa7c8` | `3a30e75cefe2d7d5` | yes |
| 43 | `llrender/llshadermgr.h` | `7c3adc7080d49801` | `77b72c9aa238ff9f` | yes |
| 44 | `llrender/lltexture.cpp` | `7f0ce02bcf3a321d` | `7f0ce02bcf3a321d` | no |
| 45 | `llrender/lltexture.h` | `08d54b775a072bcc` | `08d54b775a072bcc` | no |
| 46 | `llrender/lltexturemanagerbridge.cpp` | `044fd8059cb0df5b` | `044fd8059cb0df5b` | no |
| 47 | `llrender/lltexturemanagerbridge.h` | `c324a66d0cb8cf11` | `c324a66d0cb8cf11` | no |
| 48 | `llrender/lluiimage.cpp` | `4cff7252a18ed052` | `4cff7252a18ed052` | no |
| 49 | `llrender/lluiimage.h` | `e6c65e393645def0` | `e6c65e393645def0` | no |
| 50 | `llrender/lluiimage.inl` | `23516e33ad281a93` | `23516e33ad281a93` | no |
| 51 | `llrender/llvertexbuffer.cpp` | `3c79ae02d9c2c83a` | `f337cc98a0b6554e` | yes |
| 52 | `llrender/llvertexbuffer.h` | `43411cf233d1901d` | `0a57d038c3b095b0` | yes |

**common 中身 diff 集計**: 23 / 52 件で差分あり

## 2.B newview render-related cpp/h

- BD files: **167**, AY files: **174**
- common: **161**, BD-only: **6**, AY-only: **13**

### 2.B newview render-related cpp/h BD-only

- `newview/llfloatereditsky.cpp` (22,134 B)
- `newview/llfloatereditwater.cpp` (17,124 B)
- `newview/llfloaterenvironmentsettings.cpp` (7,408 B)
- `newview/llfloaterenvironmentsettings.h` (2,205 B)
- `newview/llfloaterwateradjust.cpp` (18,680 B)
- `newview/llfloaterwateradjust.h` (4,008 B)

### 2.B newview render-related cpp/h AY-only

- `newview/fsavatarrenderpersistence.cpp` (3,912 B)
- `newview/fsavatarrenderpersistence.h` (2,430 B)
- `newview/fsfloateravatarrendersettings.cpp` (8,526 B)
- `newview/fsfloateravatarrendersettings.h` (2,754 B)
- `newview/fsfloatersplashscreensettings.cpp` (4,955 B)
- `newview/fsfloatersplashscreensettings.h` (1,895 B)
- `newview/llfloaterpostprocess.cpp` (9,462 B)
- `newview/llfloaterpostprocess.h` (2,329 B)
- `newview/llocclusiongeometrymgr.cpp` (29,258 B)
- `newview/llocclusiongeometrymgr.h` (6,079 B)
- `newview/llpanelopenregionsettings.cpp` (7,743 B)
- `newview/rlvenvironment.cpp` (40,411 B)
- `newview/rlvenvironment.h` (3,315 B)

### 2.B newview render-related cpp/h common (sha256 diff)

| # | file | BD sha | AY sha | diff |
|---|---|---|---|---|
| 1 | `newview/SMAAAreaTex.h` | `0dda685e5f63b9d5` | `0dda685e5f63b9d5` | no |
| 2 | `newview/SMAASearchTex.h` | `162be140f48eda88` | `162be140f48eda88` | no |
| 3 | `newview/gltf/llgltfloader.cpp` | `05bd83a41ed8017b` | `8449218d909fccd9` | yes |
| 4 | `newview/gltf/llgltfloader.h` | `dfbc34a74ce16dbd` | `dfbc34a74ce16dbd` | no |
| 5 | `newview/llavatarrenderinfoaccountant.cpp` | `9075823de22054a0` | `9075823de22054a0` | no |
| 6 | `newview/llavatarrenderinfoaccountant.h` | `293f0c1139e651b7` | `293f0c1139e651b7` | no |
| 7 | `newview/llavatarrendernotifier.cpp` | `730037bc7fcb9c91` | `505203224fc39782` | yes |
| 8 | `newview/llavatarrendernotifier.h` | `ef470e1ffe1ff5f4` | `c25d74a2494d93ec` | yes |
| 9 | `newview/llcontrolavatar.cpp` | `87193fd98959febb` | `6ceac133a92a9000` | yes |
| 10 | `newview/llcontrolavatar.h` | `3c77f2594c5451a5` | `ff75d29f45ed763a` | yes |
| 11 | `newview/lldeferredsounds.cpp` | `53cea00768d27627` | `53cea00768d27627` | no |
| 12 | `newview/lldeferredsounds.h` | `aa7ee8e8335bf9d6` | `aa7ee8e8335bf9d6` | no |
| 13 | `newview/lldrawable.cpp` | `f9acbb3b78d40f43` | `dd914b08644ff1fd` | yes |
| 14 | `newview/lldrawable.h` | `faaf0569eb766053` | `658446a5ee25bf47` | yes |
| 15 | `newview/lldrawpool.cpp` | `cb9338f712e78faa` | `1b4ce6c0125efd6f` | yes |
| 16 | `newview/lldrawpool.h` | `48ecc16c56efb789` | `0e3661c45bfee731` | yes |
| 17 | `newview/lldrawpoolalpha.cpp` | `faaa0d995177428a` | `f2c4881cf519910d` | yes |
| 18 | `newview/lldrawpoolalpha.h` | `91ed66ccba7a0153` | `4ab824862c468875` | yes |
| 19 | `newview/lldrawpoolavatar.cpp` | `ec4f9dc4ef6855b3` | `6df95538bd2b1658` | yes |
| 20 | `newview/lldrawpoolavatar.h` | `0f4478739b51d8e4` | `3425b70641469644` | yes |
| 21 | `newview/lldrawpoolbump.cpp` | `1035148afe85265e` | `53e861cb1add2e29` | yes |
| 22 | `newview/lldrawpoolbump.h` | `5ce49d97d96daf3c` | `93dbf768bed3b8fb` | yes |
| 23 | `newview/lldrawpoolmaterials.cpp` | `2e8fbd340655e497` | `8d73be1268150ad7` | yes |
| 24 | `newview/lldrawpoolmaterials.h` | `67fe79de99bf2d69` | `f9561350e11ea6bb` | yes |
| 25 | `newview/lldrawpoolpbropaque.cpp` | `53a98abd784988b0` | `026bfbeb32b357f6` | yes |
| 26 | `newview/lldrawpoolpbropaque.h` | `c669ec151a181a3a` | `3a5151ca9ef4dafa` | yes |
| 27 | `newview/lldrawpoolsimple.cpp` | `369a8d3267e536be` | `54a8be34d2fd1c8e` | yes |
| 28 | `newview/lldrawpoolsimple.h` | `d209a47b17e1b321` | `ec3c5ab142cc1e43` | yes |
| 29 | `newview/lldrawpoolsky.cpp` | `6dae7d34244f3ffd` | `6dae7d34244f3ffd` | no |
| 30 | `newview/lldrawpoolsky.h` | `a42f0870733b6235` | `a42f0870733b6235` | no |
| 31 | `newview/lldrawpoolterrain.cpp` | `fa20a048cd05f969` | `572b84135549dbdd` | yes |
| 32 | `newview/lldrawpoolterrain.h` | `4132811770a11166` | `11ff0b7b954e0b50` | yes |
| 33 | `newview/lldrawpooltree.cpp` | `d27f4049ce67487e` | `a82c79a13aabb0d3` | yes |
| 34 | `newview/lldrawpooltree.h` | `8d0a476f28708c57` | `db2eb6fd7d5dcac8` | yes |
| 35 | `newview/lldrawpoolwater.cpp` | `08ce965c03784e25` | `47eab13a8f618a53` | yes |
| 36 | `newview/lldrawpoolwater.h` | `ac5a4195da55d0a7` | `86db146fbb394def` | yes |
| 37 | `newview/lldrawpoolwaterexclusion.cpp` | `ba5726c2543d95a0` | `ba5726c2543d95a0` | no |
| 38 | `newview/lldrawpoolwaterexclusion.h` | `e360121f33a14caf` | `e360121f33a14caf` | no |
| 39 | `newview/lldrawpoolwlsky.cpp` | `7f9a9e9171a0f4cf` | `7f9a9e9171a0f4cf` | no |
| 40 | `newview/lldrawpoolwlsky.h` | `2a75e4b5a862b2ca` | `2a75e4b5a862b2ca` | no |
| 41 | `newview/llenvironment.cpp` | `30384d69d11cf846` | `c8106bfa49d256ee` | yes |
| 42 | `newview/llenvironment.h` | `400c0a93b0c43828` | `f7bd6f8ac3406256` | yes |
| 43 | `newview/llface.cpp` | `aa355866acb40d02` | `bc75e955bf66a3b5` | yes |
| 44 | `newview/llface.h` | `4e6caa3eae643257` | `7b487702b3a12f44` | yes |
| 45 | `newview/llflexibleobject.cpp` | `2dfcc485355b4059` | `2dfcc485355b4059` | no |
| 46 | `newview/llflexibleobject.h` | `2e1d28ddf8d5f841` | `2e1d28ddf8d5f841` | no |
| 47 | `newview/llfloaterautoreplacesettings.cpp` | `ad4e8daa56b0f85e` | `8ff7bd52395aa4e3` | yes |
| 48 | `newview/llfloaterautoreplacesettings.h` | `3cd2783a8b04bad7` | `3cd2783a8b04bad7` | no |
| 49 | `newview/llfloateravatarrendersettings.cpp` | `f621f9f05cbe39b2` | `4a1ad85e1f25d017` | yes |
| 50 | `newview/llfloateravatarrendersettings.h` | `949901d40d2d7676` | `eefeb623d1a65a24` | yes |
| 51 | `newview/llfloatercamerapresets.cpp` | `9757f3ab4a7d5a2c` | `4ea60b15f3d75665` | yes |
| 52 | `newview/llfloatercamerapresets.h` | `0d2de875521daf52` | `0d2de875521daf52` | no |
| 53 | `newview/llfloatereditenvironmentbase.cpp` | `b76f3e18f0355e5b` | `b76f3e18f0355e5b` | no |
| 54 | `newview/llfloatereditenvironmentbase.h` | `3f219dab24410b7c` | `3f219dab24410b7c` | no |
| 55 | `newview/llfloaterenvironmentadjust.cpp` | `3e2f10dab4a5d4f7` | `86159170c024c80b` | yes |
| 56 | `newview/llfloaterenvironmentadjust.h` | `65b2822859ca2b9b` | `25e24efb0efb2ea4` | yes |
| 57 | `newview/llfloaterfixedenvironment.cpp` | `c7a07c6e008fe7f6` | `48bd967777695e9e` | yes |
| 58 | `newview/llfloaterfixedenvironment.h` | `d8c0bb1863dae8d2` | `67cb0ba4946604c0` | yes |
| 59 | `newview/llfloaterinventorysettings.cpp` | `55cd7c4d8332b28d` | `55cd7c4d8332b28d` | no |
| 60 | `newview/llfloaterinventorysettings.h` | `82c1997a0e8941e5` | `82c1997a0e8941e5` | no |
| 61 | `newview/llfloatermediasettings.cpp` | `420fa97dab878313` | `420fa97dab878313` | no |
| 62 | `newview/llfloatermediasettings.h` | `ff952298ffd695f7` | `ff952298ffd695f7` | no |
| 63 | `newview/llfloatermyenvironment.cpp` | `f709234436a89f9a` | `f709234436a89f9a` | no |
| 64 | `newview/llfloatermyenvironment.h` | `dc702e939b64ed9e` | `dc702e939b64ed9e` | no |
| 65 | `newview/llfloatersettingscolor.cpp` | `cd868358ae8cfd55` | `cd868358ae8cfd55` | no |
| 66 | `newview/llfloatersettingscolor.h` | `018e77318325ab10` | `018e77318325ab10` | no |
| 67 | `newview/llfloatersettingsdebug.cpp` | `c4c5f9a6accf73b7` | `ddf5f73eb7659606` | yes |
| 68 | `newview/llfloatersettingsdebug.h` | `122926b5aa269ea9` | `1c4bd4d0dd8281e3` | yes |
| 69 | `newview/llfloaterspellchecksettings.cpp` | `3f3cce3cd890c187` | `bcb5e555969c923b` | yes |
| 70 | `newview/llfloaterspellchecksettings.h` | `ed09e4c319c94e3b` | `9e10cc1dc860b6a0` | yes |
| 71 | `newview/llfloatertranslationsettings.cpp` | `a2aa6dab7f8643b4` | `23551551e92487ec` | yes |
| 72 | `newview/llfloatertranslationsettings.h` | `6c85f6ea09536698` | `6c85f6ea09536698` | no |
| 73 | `newview/llglsandbox.cpp` | `b59397e402028c13` | `041c4bb404ae9fc0` | yes |
| 74 | `newview/llgltffolderitem.cpp` | `3dabc14724ad0326` | `3dabc14724ad0326` | no |
| 75 | `newview/llgltffolderitem.h` | `9102a72b0b021703` | `3770675715e19d94` | yes |
| 76 | `newview/llgltffoldermodel.cpp` | `d7984e19c790d464` | `d7984e19c790d464` | no |
| 77 | `newview/llgltffoldermodel.h` | `9ab735f3dcdd0952` | `9ab735f3dcdd0952` | no |
| 78 | `newview/llgltfmateriallist.cpp` | `e93788431ca7d704` | `28138616845f64dd` | yes |
| 79 | `newview/llgltfmateriallist.h` | `0287cbedf7688183` | `0287cbedf7688183` | no |
| 80 | `newview/llgltfmaterialpreviewmgr.cpp` | `3e9b06bb283e3d52` | `3e9b06bb283e3d52` | no |
| 81 | `newview/llgltfmaterialpreviewmgr.h` | `df2a51c606babbcb` | `df2a51c606babbcb` | no |
| 82 | `newview/llhudrender.cpp` | `ee650414aebe7cf9` | `ee650414aebe7cf9` | no |
| 83 | `newview/llhudrender.h` | `74c3e84e5ad3fcbb` | `74c3e84e5ad3fcbb` | no |
| 84 | `newview/lllegacyatmospherics.cpp` | `ec43ec4bb8a5fa65` | `ec43ec4bb8a5fa65` | no |
| 85 | `newview/lllegacyatmospherics.h` | `01d7c485654817ab` | `01d7c485654817ab` | no |
| 86 | `newview/llmaterialeditor.cpp` | `0fab7afef544c6df` | `90ef9d3199204a53` | yes |
| 87 | `newview/llmaterialeditor.h` | `9a9df540b0dbcb0e` | `9a9df540b0dbcb0e` | no |
| 88 | `newview/llmaterialmgr.cpp` | `d61ab208cba605ec` | `d61ab208cba605ec` | no |
| 89 | `newview/llmaterialmgr.h` | `61310ec2a88f058c` | `70165aab756bdd53` | yes |
| 90 | `newview/llpaneleditsky.cpp` | `4c55ca864ab786b9` | `584f4f7942b4b11b` | yes |
| 91 | `newview/llpaneleditsky.h` | `923bbe040448c191` | `054237955a8611ab` | yes |
| 92 | `newview/llpaneleditwater.cpp` | `7acafd16a95acf6e` | `d6b5fdc7ed968252` | yes |
| 93 | `newview/llpaneleditwater.h` | `6af68d4077a37228` | `9698d0dfb6dfe1c8` | yes |
| 94 | `newview/llpanelenvironment.cpp` | `36042f0fdf4337f1` | `64ec042159c04969` | yes |
| 95 | `newview/llpanelenvironment.h` | `20e68c85c2727b8d` | `20e68c85c2727b8d` | no |
| 96 | `newview/llpanelmediasettingsgeneral.cpp` | `00396fe62aca342e` | `00396fe62aca342e` | no |
| 97 | `newview/llpanelmediasettingsgeneral.h` | `2bf48fbbffd7acfd` | `2bf48fbbffd7acfd` | no |
| 98 | `newview/llpanelmediasettingspermissions.cpp` | `ac7f70a414cb14bd` | `ac7f70a414cb14bd` | no |
| 99 | `newview/llpanelmediasettingspermissions.h` | `05d1cba2f968e2e7` | `05d1cba2f968e2e7` | no |
| 100 | `newview/llpanelmediasettingssecurity.cpp` | `0a867d814b4095bb` | `0a867d814b4095bb` | no |
| 101 | `newview/llpanelmediasettingssecurity.h` | `fe078998646def92` | `fe078998646def92` | no |
| 102 | `newview/llpanelpresetscamerapulldown.cpp` | `c73741c240827801` | `9bb490bd9339ee8e` | yes |
| 103 | `newview/llpanelpresetscamerapulldown.h` | `46c00440fe95ddd1` | `46c00440fe95ddd1` | no |
| 104 | `newview/llpanelpresetspulldown.cpp` | `aa5b3a6e0d011855` | `ee63ecdedd237a55` | yes |
| 105 | `newview/llpanelpresetspulldown.h` | `42ae605c6ebc93c5` | `d514f39af206a9a2` | yes |
| 106 | `newview/llpanelsnapshotpostcard.cpp` | `7139384af40eb262` | `3672165e45403cb0` | yes |
| 107 | `newview/llpanelvoicedevicesettings.cpp` | `6857b1901566a59d` | `1710aec1f4e00372` | yes |
| 108 | `newview/llpanelvoicedevicesettings.h` | `6961cdca6d15e0a7` | `6961cdca6d15e0a7` | no |
| 109 | `newview/llpipelinelistener.cpp` | `796dd654c129e4d5` | `796dd654c129e4d5` | no |
| 110 | `newview/llpipelinelistener.h` | `06b2fc26b3c7f6bb` | `06b2fc26b3c7f6bb` | no |
| 111 | `newview/llpostcard.cpp` | `d2af628e707a3615` | `c44dfc17b45d2f48` | yes |
| 112 | `newview/llpostcard.h` | `f48b297256bb6aa5` | `edf3e07b459910b1` | yes |
| 113 | `newview/llpresetsmanager.cpp` | `78fa134115232902` | `9dd172792de44b40` | yes |
| 114 | `newview/llpresetsmanager.h` | `b18245cf73fd02f0` | `0b128859786d84d2` | yes |
| 115 | `newview/llsettingspicker.cpp` | `ec045191a7228a55` | `ec045191a7228a55` | no |
| 116 | `newview/llsettingspicker.h` | `09a7f67fffb8a2c3` | `09a7f67fffb8a2c3` | no |
| 117 | `newview/llsettingsvo.cpp` | `91edd21d9954d84c` | `1b8748ef7dcc8e6c` | yes |
| 118 | `newview/llsettingsvo.h` | `ddcd9c8621e624ed` | `aee173483d9b36ef` | yes |
| 119 | `newview/llsky.cpp` | `7904beda201d3bd8` | `7904beda201d3bd8` | no |
| 120 | `newview/llsky.h` | `f0a2121d960352d5` | `f0a2121d960352d5` | no |
| 121 | `newview/llspatialpartition.cpp` | `11d565252ff7f790` | `8b2b6abaee4e7e29` | yes |
| 122 | `newview/llspatialpartition.h` | `bb8bd18c2a24441b` | `ce184f5ef0bf2799` | yes |
| 123 | `newview/llviewershadermgr.cpp` | `75375333c3c36568` | `583ddd2bb58346e9` | yes |
| 124 | `newview/llviewershadermgr.h` | `683a012b5b52f4f1` | `8f1650d8374d52ec` | yes |
| 125 | `newview/llviewertexture.cpp` | `79efc225e232f1b3` | `2672eaf47058d5be` | yes |
| 126 | `newview/llviewertexture.h` | `e775b493401a08d7` | `0e48cb49ddeab53b` | yes |
| 127 | `newview/llviewertextureanim.cpp` | `49da63ca9b429810` | `49da63ca9b429810` | no |
| 128 | `newview/llviewertextureanim.h` | `0f989512331ec004` | `0f989512331ec004` | no |
| 129 | `newview/llviewertexturelist.cpp` | `33dec8d5fa6b33e0` | `52816daf1a4c2437` | yes |
| 130 | `newview/llviewertexturelist.h` | `dbbfd1e51255a119` | `39c49763babf1efd` | yes |
| 131 | `newview/llviewerwindow.cpp` | `23022a093921cac7` | `74041892c924a82b` | yes |
| 132 | `newview/llviewerwindow.h` | `392a57677c8e4063` | `db24e2a3d5845d5c` | yes |
| 133 | `newview/llviewerwindowlistener.cpp` | `84dfd573d3913dcf` | `84dfd573d3913dcf` | no |
| 134 | `newview/llviewerwindowlistener.h` | `bb3329e394dbbb07` | `bb3329e394dbbb07` | no |
| 135 | `newview/llvoavatar.cpp` | `1e370f87503724af` | `3129460d41430b46` | yes |
| 136 | `newview/llvoavatar.h` | `1cc49867f89b9a84` | `d6a108a146e198a2` | yes |
| 137 | `newview/llvoavatarself.cpp` | `801e949db8a71f1d` | `82060809a4e513d8` | yes |
| 138 | `newview/llvoavatarself.h` | `dde7674cf7044252` | `2de9c94e8e398024` | yes |
| 139 | `newview/llvograss.cpp` | `bfdd7f668eb9167a` | `373cd63c44b944a3` | yes |
| 140 | `newview/llvograss.h` | `fc80e17199631da0` | `ab2ae02287765e28` | yes |
| 141 | `newview/llvopartgroup.cpp` | `a228b1ce238b0624` | `6c79bff4d03fdfe1` | yes |
| 142 | `newview/llvopartgroup.h` | `5a962eab3e58942f` | `5a962eab3e58942f` | no |
| 143 | `newview/llvosky.cpp` | `02b450ff1e4d7575` | `12609d295dc4f4de` | yes |
| 144 | `newview/llvosky.h` | `0787a8b2abe649d8` | `0787a8b2abe649d8` | no |
| 145 | `newview/llvosurfacepatch.cpp` | `ed9bfa87e4726098` | `251736fe64eed0b0` | yes |
| 146 | `newview/llvosurfacepatch.h` | `b4179af127dbcad4` | `b4179af127dbcad4` | no |
| 147 | `newview/llvotree.cpp` | `d7e506feb7772b14` | `bd80319571f57df1` | yes |
| 148 | `newview/llvotree.h` | `931d58244d0f5f3b` | `ae77c1e05f8f7a71` | yes |
| 149 | `newview/llvovolume.cpp` | `656e4db1ce76c6de` | `66e8855450e81a8d` | yes |
| 150 | `newview/llvovolume.h` | `3f799849820526ce` | `2427fd3998a65728` | yes |
| 151 | `newview/llvowater.cpp` | `818de5999724c8a0` | `4e03553a1cfc38ff` | yes |
| 152 | `newview/llvowater.h` | `eec9f9a4f3e2fae3` | `eec9f9a4f3e2fae3` | no |
| 153 | `newview/llvowlsky.cpp` | `378ba9da843cceab` | `f4e72fd4943126c6` | yes |
| 154 | `newview/llvowlsky.h` | `fc31ea45e800e735` | `fc31ea45e800e735` | no |
| 155 | `newview/pipeline.cpp` | `eb3c1d8e32180500` | `faf4817b5b804a04` | yes |
| 156 | `newview/pipeline.h` | `0006aae1dbd725a1` | `68519d92d1c14d0b` | yes |
| 157 | `newview/tests/llglslshader_stub.cpp` | `7635a909df9c1343` | `7635a909df9c1343` | no |
| 158 | `newview/tests/llpipeline_stub.cpp` | `f73fe49a08ca1e84` | `f73fe49a08ca1e84` | no |
| 159 | `newview/tests/llsky_stub.cpp` | `cf7cbc6b4675fc5b` | `cf7cbc6b4675fc5b` | no |
| 160 | `newview/tests/llviewershadermgr_stub.cpp` | `11bd6a73756ff8fd` | `11bd6a73756ff8fd` | no |
| 161 | `newview/tests/llviewertexture_stub.cpp` | `7620fca29a4ea051` | `7620fca29a4ea051` | no |

**common 中身 diff 集計**: 92 / 161 件で差分あり

# Bucket 3: render cvars (BD 995a1354d8 vs AYAstorm)

- BD render cvars: **266**, AY render cvars: **838**
- common: **259**, BD-only: **7**, AY-only: **579**
- common 完全一致 (type+value): **219**
- common value 差分: **33**, type 差分: **7**

## 3.1 BD-only cvars (BD にあって AYAstorm に無い、新規移植対象)

- `RenderAvatar` (Boolean) = `1`
- `RenderDelayVBUpdate` (Boolean) = `0`
- `RenderMotionBlur` (Boolean) = `0`
- `RenderScreenSpaceReflectionMaxDepth` (F32) = `256`
- `RenderScreenSpaceReflectionMaxRoughness` (F32) = `1.0`
- `RenderScreenSpaceReflectionSplitEnd` (Vector3) = `25`
- `RenderScreenSpaceReflectionSplitStart` (Vector3) = `0`

## 3.2 AY-only cvars (AYAstorm にあって BD に無い、Cinematic では default 不適用 候補)

- `AYAChatWindowStyle` (S32) = `2`
- `AYALLChatCompactView` (Boolean) = `1`
- `AYAOpusCodecEnable` (Boolean) = `1`
- `AYAOpusCodecPriority` (U32) = `0`
- `AYAR16AerialPerspectiveEnabled` (Boolean) = `1`
- `AYAR17ColorTemperatureEnabled` (Boolean) = `1`
- `AYAR18CloudVolumetricEnabled` (Boolean) = `1`
- `AYAR19TranslucencyEnabled` (Boolean) = `1`
- `AYAR19TranslucencyIntensity` (U32) = `1`
- `AYAR20AvatarSkinSSSBlurRadius` (F32) = `1.0`
- `AYAR20AvatarSkinSSSEnabled` (Boolean) = `1`
- `AYAR20AvatarSkinSSSGlowColor` (Color4) = `1.0`
- `AYAR20AvatarSkinSSSGlowGain` (F32) = `1.0`
- `AYAR20AvatarSkinSSSStrength` (F32) = `0.7`
- `AYAR20AvatarSkinSSSWhitelist` (String) = ``
- `AYAVisualRealismEnabled` (U32) = `1`
- `FSAdvancedTooltips` (Boolean) = `1`
- `FSAdvancedWorldmapRegionInfo` (Boolean) = `1`
- `FSAllowDoubleClickOnScriptedObjects` (Boolean) = `1`
- `FSAllowEEPWaterDerender` (Boolean) = `0`
- `FSAlwaysFly` (Boolean) = `0`
- `FSAlwaysShowInboxButton` (Boolean) = `0`
- `FSAlwaysShowTPCancel` (Boolean) = `0`
- `FSAlwaysTrackPayments` (Boolean) = `0`
- `FSAnimatedScriptDialogs` (Boolean) = `0`
- `FSAnimationPreviewExpanded` (Boolean) = `0`
- `FSAnnounceIncomingIM` (Boolean) = `0`
- `FSAppearanceShowHints` (Boolean) = `1`
- `FSAreaSearchAdvanced` (Boolean) = `0`
- `FSAreaSearchColumnConfig` (U32) = `1023`
- `FSAreaSearch_ClickAction` (LLSD) = ``
- `FSAreaSearch_ExcludeAttachments` (Boolean) = `1`
- `FSAreaSearch_ExcludeChildPrims` (Boolean) = `1`
- `FSAreaSearch_ExcludeNeighborRegions` (Boolean) = `1`
- `FSAreaSearch_ExcludePhysical` (Boolean) = `1`
- `FSAreaSearch_ExcludeReflectionProbes` (Boolean) = `0`
- `FSAreaSearch_ExcludeTemporary` (Boolean) = `1`
- `FSAreaSearch_FilterDistance` (Boolean) = `0`
- `FSAreaSearch_FilterForSale` (Boolean) = `0`
- `FSAreaSearch_MaximumDistance` (S32) = `999999`
- `FSAreaSearch_MaximumPrice` (S32) = `999999`
- `FSAreaSearch_MinimumDistance` (S32) = `0`
- `FSAreaSearch_MinimumPrice` (S32) = `0`
- `FSAreaSearch_OnlyAttachments` (Boolean) = `0`
- `FSAreaSearch_OnlyCopiable` (Boolean) = `0`
- `FSAreaSearch_OnlyCurrentParcel` (Boolean) = `0`
- `FSAreaSearch_OnlyLocked` (Boolean) = `0`
- `FSAreaSearch_OnlyMOAP` (Boolean) = `0`
- `FSAreaSearch_OnlyModifiable` (Boolean) = `0`
- `FSAreaSearch_OnlyPhantom` (Boolean) = `0`
- `FSAreaSearch_OnlyPhysical` (Boolean) = `0`
- `FSAreaSearch_OnlyReflectionProbes` (Boolean) = `0`
- `FSAreaSearch_OnlyTemporary` (Boolean) = `0`
- `FSAreaSearch_OnlyTransferable` (Boolean) = `0`
- `FSAudioMusicFadeIn` (F32) = `3.0`
- `FSAudioMusicFadeOut` (F32) = `2.0`
- `FSAutoOrderIMTabs` (Boolean) = `0`
- `FSAutoOrderIMTabsAtTop` (Boolean) = `0`
- `FSAutoOrderIMTabsPriorities` (String) = `210`
- `FSAutoUnmuteAmbient` (Boolean) = `0`
- `FSAutoUnmuteSounds` (Boolean) = `0`
- `FSAvatarTurnSpeed` (F32) = `0.0`
- `FSBeamColorFile` (String) = `Rainbow`
- `FSBeamShape` (String) = `Phoenix`
- `FSBeamShapeScale` (F32) = `1.3`
- `FSBetterGroupNoticesToIMLog` (Boolean) = `1`
- `FSBeyondNearbyChatColorDiminishFactor` (F32) = `0.8`
- `FSBlockClickSit` (Boolean) = `0`
- `FSBrowserHomePage` (String) = `https://duckduckgo.com`
- `FSBubblesHideConsoleAndToasts` (Boolean) = `1`
- `FSBuildPrefs_ActualRoot` (Boolean) = `0`
- `FSBuildPrefs_Alpha` (F32) = `0.0`
- `FSBuildPrefs_Color` (Color4) = `1.0`
- `FSBuildPrefs_FullBright` (Boolean) = `0`
- `FSBuildPrefs_Glow` (F32) = `0.0`
- `FSBuildPrefs_Material` (String) = `Wood`
- `FSBuildPrefs_Phantom` (Boolean) = `0`
- `FSBuildPrefs_Physical` (Boolean) = `0`
- `FSBuildPrefs_PivotIsPercent` (Boolean) = `1`
- `FSBuildPrefs_PivotX` (F32) = `50`
- `FSBuildPrefs_PivotY` (F32) = `50`
- `FSBuildPrefs_PivotZ` (F32) = `50`
- `FSBuildPrefs_Shiny` (String) = `None`
- `FSBuildPrefs_Temporary` (Boolean) = `0`
- `FSBuildPrefs_Xsize` (F32) = `0.5`
- `FSBuildPrefs_Ysize` (F32) = `0.5`
- `FSBuildPrefs_Zsize` (F32) = `0.5`
- `FSBuildToolDecimalPrecision` (S32) = `5`
- `FSChatHistoryShowYou` (Boolean) = `0`
- `FSChatHumanObjectTabs` (Boolean) = `1`
- `FSChatWindow` (S32) = `1`
- `FSChatbarGestureAutoCompleteEnable` (Boolean) = `1`
- `FSChatbarNamePrediction` (Boolean) = `0`
- `FSClientTagsVisibility` (U32) = `0`
- `FSCloseChatOnReturnInMouselook` (Boolean) = `1`
- `FSCloseChatOnReturnOnlyForNearbyChatControl` (Boolean) = `0`
- `FSCmdLine` (Boolean) = `1`
- `FSCmdLineAO` (String) = `cao`
- `FSCmdLineBandWidth` (String) = `bw`
- `FSCmdLineCalc` (String) = `calc`
- `FSCmdLineClearChat` (String) = `clrchat`
- `FSCmdLineCopyCam` (String) = `cpcampos`
- `FSCmdLineDrawDistance` (String) = `dd`
- `FSCmdLineGround` (String) = `flr`
- `FSCmdLineHeight` (String) = `gth`
- `FSCmdLineKeyToName` (String) = `key2name`
- `FSCmdLineMapTo` (String) = `mapto`
- `FSCmdLineMapToKeepPos` (Boolean) = `0`
- `FSCmdLineMedia` (String) = `/media`
- `FSCmdLineMusic` (String) = `/music`
- `FSCmdLineOfferTp` (String) = `offertp`
- `FSCmdLinePlatformSize` (F32) = `30`
- `FSCmdLinePos` (String) = `gtp`
- `FSCmdLineRezPlatform` (String) = `rezplat`
- `FSCmdLineRollDice` (String) = `rolld`
- `FSCmdLineTP2` (String) = `tp2`
- `FSCmdLineTeleportHome` (String) = `tph`
- `FSCmdTeleportToCam` (String) = `tp2cam`
- `FSCollisionMessagesInChat` (Boolean) = `0`
- `FSColorClienttags` (U32) = `2`
- `FSColorIMsDistinctly` (Boolean) = `0`
- `FSColorUsername` (Boolean) = `0`
- `FSComboboxSubstringSearch` (Boolean) = `1`
- `FSCommitForSaleOnChange` (Boolean) = `0`
- `FSConfirmPayments` (Boolean) = `1`
- `FSConsoleClassicDrawMode` (Boolean) = `0`
- `FSContactListShowSearch` (Boolean) = `1`
- `FSContactSetsColorizeChat` (Boolean) = `0`
- `FSContactSetsColorizeFriends` (Boolean) = `0`
- `FSContactSetsColorizeMiniMap` (Boolean) = `0`
- `FSContactSetsColorizeNameTag` (Boolean) = `0`
- `FSContactSetsColorizeRadar` (Boolean) = `0`
- `FSContactSetsNotificationNearbyChat` (Boolean) = `1`
- `FSContactSetsNotificationToast` (Boolean) = `0`
- `FSContactsSortOrder` (U32) = `3`
- `FSConversationLogLifetime` (U32) = `120`
- `FSCopyObjKeySeparator` (String) = `,`
- `FSCreateCallingCards` (Boolean) = `1`
- `FSCreateGiveInventoryParticleEffect` (Boolean) = `1`
- `FSCreateOctreeLog` (Boolean) = `0`
- `FSDefaultObjectTexture` (String) = `89556747-24cb-43ed-920b-47caed15465f`
- `FSDestroyGLTexturesImmediately` (Boolean) = `0`
- `FSDestroyGLTexturesThreshold` (F32) = `0.9`
- `FSDisableAvatarTrackerAtCloseIn` (Boolean) = `1`
- `FSDisableBeaconAfterTeleport` (Boolean) = `0`
- `FSDisableBlockListAutoOpen` (Boolean) = `0`
- `FSDisableIMChiclets` (Boolean) = `0`
- `FSDisableLoginScreens` (Boolean) = `0`
- `FSDisableLogoutScreens` (Boolean) = `0`
- `FSDisableMouseWheelCameraZoom` (Boolean) = `0`
- `FSDisableNeighbourRegionConnections` (Boolean) = `0`
- `FSDisableReturnObjectNotification` (Boolean) = `0`
- `FSDisableRiggedMeshMatrixCaching` (Boolean) = `0`
- `FSDisableTeleportScreens` (Boolean) = `0`
- `FSDisableTurningAroundWhenWalkingBackwards` (Boolean) = `0`
- `FSDisableWMIProbing` (Boolean) = `0`
- `FSDiskCacheHighWaterPercent` (F32) = `95.0`
- `FSDiskCacheLowWaterPercent` (F32) = `70.0`
- `FSDiskCacheSize` (U32) = `2048`
- `FSDismissGroupNoticeAttachmentsToTrash` (Boolean) = `0`
- `FSDoNotHideMapOnTeleport` (Boolean) = `0`
- `FSDontIgnoreAdHocFromFriends` (Boolean) = `0`
- `FSDontNagWhenPurging` (Boolean) = `0`
- `FSDoubleClickAddInventoryClothing` (Boolean) = `0`
- `FSDoubleClickAddInventoryObjects` (Boolean) = `0`
- `FSDrawDistanceVRAMOptimization` (Boolean) = `0`
- `FSEditGrid` (Boolean) = `0`
- `FSEmphasizeShoutWhisper` (Boolean) = `1`
- `FSEnableAggressiveComplexityUpdates` (Boolean) = `0`
- `FSEnableEmojiWindowPopupWhileTyping` (Boolean) = `1`
- `FSEnableGrowl` (Boolean) = `0`
- `FSEnableLogThrottle` (Boolean) = `1`
- `FSEnableMovingFolderLinks` (Boolean) = `1`
- `FSEnableObjectExports` (Boolean) = `1`
- `FSEnablePerGroupSnoozeDuration` (Boolean) = `0`
- `FSEnableRightclickMenuInMouselook` (Boolean) = `0`
- `FSEnableRightclickOnTransparentObjects` (Boolean) = `1`
- `FSEnableVolumeControls` (Boolean) = `1`
- `FSEnabledLanguages` (LLSD) = `az`
- `FSEnforceStrictObjectCheck` (Boolean) = `1`
- `FSEnvironmentManualTransitionTime` (F32) = `0.0`
- `FSEventPollCoreRetries` (U32) = `0`
- `FSExperimentalDragTexture` (Boolean) = `0`
- `FSExperimentalLostAttachmentsFix` (Boolean) = `1`
- `FSExperimentalLostAttachmentsFixKillDelay` (F32) = `3.0`
- `FSExperimentalLostAttachmentsFixReport` (Boolean) = `0`
- `FSExperimentalOutfitsReturn` (Boolean) = `0`
- `FSExperimentalRegionCrossingMovementFix` (S32) = `0`
- `FSExportContents` (Boolean) = `1`
- `FSFadeAudioStream` (Boolean) = `1`
- `FSFadeGroupNotices` (Boolean) = `1`
- `FSFilePickerOpenDirectory` (String) = ``
- `FSFilePickerSaveDirectory` (String) = ``
- `FSFilterGrowlKeywordDuplicateIMs` (Boolean) = `0`
- `FSFirstRunAfterSettingsRestore` (Boolean) = `0`
- `FSFlashOnMessage` (Boolean) = `0`
- `FSFlashOnObjectIM` (Boolean) = `1`
- `FSFlashOnScriptDialog` (Boolean) = `0`
- `FSFlyAfterTeleport` (Boolean) = `0`
- `FSFocusPointFollowsPointer` (Boolean) = `0`
- `FSFocusPointLocked` (Boolean) = `0`
- `FSFocusPointRender` (Boolean) = `0`
- `FSFolderViewItemHeight` (S32) = `20`
- `FSFontChatLineSpacingPixels` (S32) = `2`
- `FSFontSettingsFile` (String) = `fonts.xml`
- `FSFontSizeAdjustment` (F32) = `0.0`
- `FSForcedVideoMemory` (U32) = `0`
- `FSFriendListColumnShowDisplayName` (Boolean) = `0`
- `FSFriendListColumnShowFullName` (Boolean) = `1`
- `FSFriendListColumnShowPermissions` (Boolean) = `1`
- `FSFriendListColumnShowUserName` (Boolean) = `0`
- `FSFriendListFullNameFormat` (S32) = `1`
- `FSFriendListSortOrder` (S32) = `0`
- `FSFriendOnlineToHumanTab` (Boolean) = `1`
- `FSGridBuilderURL` (String) = `https://phoenixviewer.com/app/fsdata/fs_grid_builder.html`
- `FSGroupNoticesToIMLog` (Boolean) = `1`
- `FSGroupNotifyNoTransparency` (Boolean) = `0`
- `FSGrowlWhenActive` (Boolean) = `0`
- `FSHideHelpButtons` (Boolean) = `0`
- `FSHighlightGroupMods` (Boolean) = `1`
- `FSHudTextBackgroundOpacity` (F32) = `0.75`
- `FSHudTextFadeDistance` (F32) = `8.0`
- `FSHudTextFadeRange` (F32) = `4.0`
- `FSHudTextShowBackground` (S32) = `0`
- `FSHudTextUseHoverHighlight` (Boolean) = `0`
- `FSIMChatFlashOnFriendStatusChange` (Boolean) = `0`
- `FSIMChatHistoryFade` (F32) = `0.5`
- `FSIMOpacity` (F32) = `1.0`
- `FSIMSystemMessageBrackets` (Boolean) = `0`
- `FSIMTabNameFormat` (S32) = `0`
- `FSIgnoreAdHocSessions` (Boolean) = `0`
- `FSIgnoreClientsideMeshValidation` (Boolean) = `0`
- `FSIgnoreFinishAnimation` (Boolean) = `0`
- `FSIgnoreObjectIM` (Boolean) = `0`
- `FSIgnoreSimulatorCameraConstraints` (Boolean) = `0`
- `FSImActiveOpacityOverride` (Boolean) = `0`
- `FSImageDecodeThreads` (U32) = `0`
- `FSImportBuildOffset` (Vector3) = `5.0`
- `FSImpostorAvatarExclude` (U32) = `0`
- `FSInspectAvatarSlurlOpensProfile` (Boolean) = `0`
- `FSInspectColumnConfig` (U32) = `1023`
- `FSInternalCanEditObjectFaces` (Boolean) = `1`
- `FSInternalFaceHasBPNormalMap` (Boolean) = `1`
- `FSInternalFaceHasBPSpecularMap` (Boolean) = `1`
- `FSInternalFontSettingsFile` (String) = ``
- `FSInternalLegacyNotificationWell` (Boolean) = `0`
- `FSInternalShowNavbarFavoritesPanel` (Boolean) = `1`
- `FSInternalShowNavbarNavigationPanel` (Boolean) = `0`
- `FSInternalSkinCurrent` (String) = ``
- `FSInternalSkinCurrentTheme` (String) = ``
- `FSInventoryThumbnailTooltipsDelay` (F32) = `0.7`
- `FSKeepUnpackedCacheFiles` (Boolean) = `0`
- `FSLandmarkCreatedNotification` (Boolean) = `0`
- `FSLargeOutfitsWarningInThisSession` (Boolean) = `0`
- `FSLastSearchTab` (S32) = `0`
- `FSLastSnapshotPanel` (String) = ``
- `FSLastSnapshotToFacebookHeight` (S32) = `768`
- `FSLastSnapshotToFacebookResolution` (S32) = `4`
- `FSLastSnapshotToFacebookWidth` (S32) = `1024`
- `FSLastSnapshotToFlickrHeight` (S32) = `768`
- `FSLastSnapshotToFlickrResolution` (S32) = `4`
- `FSLastSnapshotToFlickrWidth` (S32) = `1024`
- `FSLastSnapshotToPrimfeedHeight` (S32) = `768`
- `FSLastSnapshotToPrimfeedResolution` (S32) = `4`
- `FSLastSnapshotToPrimfeedWidth` (S32) = `1024`
- `FSLastSnapshotToTwitterHeight` (S32) = `768`
- `FSLastSnapshotToTwitterResolution` (S32) = `3`
- `FSLastSnapshotToTwitterWidth` (S32) = `1024`
- `FSLatencyOneTimeFixRun` (Boolean) = `0`
- `FSLegacyEdgeSnap` (Boolean) = `0`
- `FSLegacyMinimize` (Boolean) = `0`
- `FSLegacyNameCacheExpiration` (Boolean) = `0`
- `FSLegacyNametagPosition` (Boolean) = `1`
- `FSLegacyNotificationWell` (Boolean) = `0`
- `FSLegacyNotificationWellAutoResize` (Boolean) = `0`
- `FSLegacyRadarFriendColoring` (Boolean) = `0`
- `FSLegacyRadarLindenColoring` (Boolean) = `0`
- `FSLegacySearchActionOnTeleport` (U32) = `1`
- `FSLetterKeysFocusNearbyChatBar` (Boolean) = `1`
- `FSLimitFramerate` (Boolean) = `1`
- `FSLimitTextureVRAMUsage` (Boolean) = `0`
- `FSLinuxEnableWin32VoiceProxy` (Boolean) = `0`
- `FSLinuxEnableWin64VoiceProxy` (Boolean) = `0`
- `FSLocalMeshApplyJointOffsets` (Boolean) = `0`
- `FSLocalMeshAutoReload` (Boolean) = `0`
- `FSLocalMeshAutoReloadPeriod` (F32) = `3.0`
- `FSLocalMeshScaleAlwaysMeters` (Boolean) = `0`
- `FSLogAutoAcceptInventoryToChat` (Boolean) = `1`
- `FSLogGroupImToChatConsole` (Boolean) = `0`
- `FSLogIMInChatHistory` (Boolean) = `0`
- `FSLogImToChatConsole` (Boolean) = `0`
- `FSLogSnapshotsToLocal` (Boolean) = `0`
- `FSLoginDontSavePassword` (Boolean) = `0`
- `FSLookAtTargetLimitDistance` (Boolean) = `0`
- `FSLookAtTargetMaxDistance` (F32) = `1`
- `FSManipRotateJointUseNaturalDirection` (Boolean) = `1`
- `FSManipShowJointMarkers` (Boolean) = `1`
- `FSMarkObjects` (Boolean) = `0`
- `FSMaxAnimationPriority` (S32) = `4`
- `FSMaxBeamsPerSecond` (F32) = `40`
- `FSMaxPendingIMMessages` (S32) = `25`
- `FSMenuBackgroundAlpha` (F32) = `1.0`
- `FSMeshHighLodSuffix` (String) = ``
- `FSMeshImportScaleFixup` (Boolean) = `0`
- `FSMeshLowLodSuffix` (String) = `LOD1`
- `FSMeshLowestLodSuffix` (String) = `LOD0`
- `FSMeshMediumLodSuffix` (String) = `LOD2`
- `FSMeshPhysicsSuffix` (String) = `PHYS`
- `FSMeshPreviewUVGuideFile` (String) = `salt_and_pepper.jpg`
- `FSMeshUploadAutoEnableWeights` (Boolean) = `1`
- `FSMeshUploadAutoShowWeightsWhenEnabled` (Boolean) = `1`
- `FSMeshUploadUseGLODAsDefault` (Boolean) = `0`
- `FSMilkshakeRadarToasts` (Boolean) = `0`
- `FSMiniMapChatRing` (Boolean) = `1`
- `FSMiniMapOpacity` (F32) = `0.66`
- `FSMiniMapShoutRing` (Boolean) = `1`
- `FSMiniMapWhisperRing` (Boolean) = `1`
- `FSMinimapPickScale` (F32) = `3.0`
- `FSModNameStyle` (U32) = `1`
- `FSModTextStyle` (U32) = `1`
- `FSMouselookCombatFeatures` (Boolean) = `0`
- `FSMuteAllGroups` (Boolean) = `0`
- `FSMuteGroupWhenNoticesDisabled` (Boolean) = `0`
- `FSNameTagShowLegacyUsernames` (Boolean) = `0`
- `FSNameTagZOffsetCorrection` (S32) = `0`
- `FSNearbyChatToastsOffset` (S32) = `20`
- `FSNearbyChatbar` (Boolean) = `1`
- `FSNetMapDoubleClickAction` (S32) = `2`
- `FSNetMapPhantomOpacity` (U32) = `90`
- `FSNetMapPhysical` (Boolean) = `0`
- `FSNetMapScripted` (Boolean) = `0`
- `FSNetMapTempOnRez` (Boolean) = `0`
- `FSNoScreenShakeOnRegionRestart` (Boolean) = `0`
- `FSNoVersionPopup` (Boolean) = `0`
- `FSNotecardFontName` (String) = `SansSerif`
- `FSNotecardFontSize` (String) = `Medium`
- `FSNotifyIMFlash` (Boolean) = `1`
- `FSNotifyIncomingObjectSpam` (Boolean) = `1`
- `FSNotifyIncomingObjectSpamFrom` (Boolean) = `1`
- `FSNotifyNearbyChatFlash` (Boolean) = `1`
- `FSNotifyUnreadChatMessages` (Boolean) = `1`
- `FSNotifyUnreadIMMessages` (Boolean) = `1`
- `FSOOCPostfix` (String) = `))`
- `FSOOCPrefix` (String) = `((`
- `FSOfferThrottleMaxCount` (U32) = `5`
- `FSOpenIMContainerOnOfflineMessage` (Boolean) = `0`
- `FSOpenInventoryAfterSnapshot` (Boolean) = `1`
- `FSOpenSimAlwaysForceShowGrid` (Boolean) = `1`
- `FSOutputDeviceUUID` (String) = `00000000-0000-0000-0000-000000000000`
- `FSOverrideVRAMDetection` (Boolean) = `0`
- `FSParcelMusicAutoPlay` (Boolean) = `0`
- `FSParcelStreamQuality` (U32) = `0`
- `FSParticleChat` (Boolean) = `0`
- `FSPaymentConfirmationThreshold` (S32) = `200`
- `FSPaymentInfoInChat` (Boolean) = `0`
- `FSPerfFloaterSmoothingPeriods` (U32) = `50`
- `FSPermissionDebitDefaultDeny` (Boolean) = `1`
- `FSPhysicsPresetUser1` (String) = ``
- `FSPlayDefaultBentoAnimation` (Boolean) = `0`
- `FSPoseStandLastSelectedPose` (String) = ``
- `FSPoseStandLock` (Boolean) = `0`
- `FSPoserOnSaveConfirmOverwrite` (Boolean) = `0`
- `FSPoserPelvisUnlockedForBvhSave` (Boolean) = `0`
- `FSPoserSaveExternalFileAlso` (Boolean) = `0`
- `FSPoserShowBoneHighlights` (Boolean) = `1`
- `FSPoserStopPosingWhenClosed` (Boolean) = `1`
- `FSPoserTrackpadSensitivity` (F32) = `0.5`
- `FSPrettyEmojiButtonCode` (U32) = `128578`
- `FSPrimfeedViewerApiKey` (String) = `xAcXYt8SBius3Lor4wHle8L96PDHYlAZuWYXIYQUdW4b09mjhQUAwiqmWp5UNYXLpq5GSUtuKHuDYLwaueACPkew93l6MRY8jfBKSH09kv0zyGglpky07X7X7Sp4Rzin`
- `FSRadarColorNamesByDistance` (Boolean) = `0`
- `FSRadarColumnConfig` (U32) = `1023`
- `FSRadarEnhanceByBridge` (Boolean) = `1`
- `FSRadarShowMutedAndDerendered` (Boolean) = `1`
- `FSRegionCrossingAngleErrorLimit` (F32) = `20.0`
- `FSRegionCrossingPositionErrorLimit` (F32) = `0.25`
- `FSRegionCrossingSmoothingTime` (F32) = `10.0`
- `FSRegionRestartAnnounceChannel` (S32) = `473405`
- `FSRemapLinuxShortcuts` (Boolean) = `0`
- `FSRememberUsername` (Boolean) = `1`
- `FSRemoveFlyHeightLimit` (Boolean) = `1`
- `FSRemoveScriptBlockButton` (Boolean) = `0`
- `FSRenderBeaconText` (Boolean) = `1`
- `FSRenderFarClipStepping` (Boolean) = `0`
- `FSRenderFarClipSteppingInterval` (U32) = `20`
- `FSRenderParcelSelectionToMaxBuildHeight` (Boolean) = `0`
- `FSRenderVignette` (Vector3) = `0.0`
- `FSRepeatedEnvTogglesShared` (Boolean) = `0`
- `FSReportBlockToNearbyChat` (Boolean) = `0`
- `FSReportCollisionMessages` (Boolean) = `0`
- `FSReportCollisionMessagesChannel` (S32) = `-25000`
- `FSReportIgnoredAdHocSession` (Boolean) = `0`
- `FSReportMutedGroupChat` (Boolean) = `0`
- `FSReportRegionRestartToChat` (Boolean) = `0`
- `FSReportTotalScriptCountChanges` (Boolean) = `0`
- `FSReportTotalScriptCountChangesThreshold` (U32) = `100`
- `FSResetCameraOnMovement` (Boolean) = `1`
- `FSResetCameraOnTP` (Boolean) = `1`
- `FSResetSkeletonOnStandUp` (Boolean) = `0`
- `FSRestoreOpenIMs` (Boolean) = `0`
- `FSRevokePerms` (U32) = `0`
- `FSRowsPerScriptDialog` (S32) = `20`
- `FSSaveInventoryScriptsAsMono` (Boolean) = `1`
- `FSSavedRenderFarClip` (F32) = `0.0`
- `FSScriptDebugWindowClearOnClose` (Boolean) = `0`
- `FSScriptDialogNoTransparency` (Boolean) = `0`
- `FSScriptEditorRecompileButton` (Boolean) = `0`
- `FSScriptInfoExtended` (Boolean) = `0`
- `FSScriptingFontName` (String) = `Scripting`
- `FSScriptingFontSize` (String) = `Scripting`
- `FSScrollWheelExitsMouselook` (Boolean) = `1`
- `FSSecondsinChatTimestamps` (Boolean) = `0`
- `FSSelectCopyableOnly` (Boolean) = `0`
- `FSSelectIncludeGroupOwned` (Boolean) = `1`
- `FSSelectLocalSearchEditorOnShortcut` (Boolean) = `1`
- `FSSelectLockedOnly` (Boolean) = `0`
- `FSSelfRiggedPickerArmSeconds` (F32) = `3.0`
- `FSSelfRiggedPickerArmedMode` (Boolean) = `1`
- `FSSelfRiggedPickerEnable` (Boolean) = `1`
- `FSSelfRiggedPickerGPU` (Boolean) = `1`
- `FSSendTypingState` (Boolean) = `1`
- `FSShowAutoAcceptInventoryInNotifications` (Boolean) = `1`
- `FSShowAutorespondInNametag` (Boolean) = `0`
- `FSShowBackSLURL` (Boolean) = `1`
- `FSShowChatChannel` (Boolean) = `0`
- `FSShowChatRangeSpheres` (Boolean) = `0`
- `FSShowChatType` (Boolean) = `1`
- `FSShowConversationVoiceStateIndicator` (Boolean) = `1`
- `FSShowConvoAndRadarInML` (Boolean) = `0`
- `FSShowCurrencyBalanceInStatusbar` (Boolean) = `1`
- `FSShowDisplayNameUpdateNotification` (Boolean) = `1`
- `FSShowDummyAVsinRadar` (Boolean) = `0`
- `FSShowEmojiButton` (Boolean) = `1`
- `FSShowGroupNameLength` (S32) = `0`
- `FSShowGroupTitleInTooltip` (Boolean) = `1`
- `FSShowIMInChatHistory` (Boolean) = `0`
- `FSShowIMSendButton` (Boolean) = `1`
- `FSShowInboxFolder` (Boolean) = `0`
- `FSShowInterfaceInMouselook` (Boolean) = `0`
- `FSShowInventoryThumbnailTooltips` (Boolean) = `1`
- `FSShowJoinedGroupInvitations` (Boolean) = `0`
- `FSShowMessageCountInWindowTitle` (Boolean) = `0`
- `FSShowMouselookInstructions` (Boolean) = `1`
- `FSShowMutedChatHistory` (Boolean) = `0`
- `FSShowMyOwnVoiceVisualizer` (Boolean) = `1`
- `FSShowOnscreenConsole` (Boolean) = `1`
- `FSShowSelectedInBlinnPhong` (Boolean) = `0`
- `FSShowServerVersionChangeNotice` (Boolean) = `1`
- `FSShowStatsBarInMouselook` (Boolean) = `0`
- `FSShowTimestampsIM` (Boolean) = `1`
- `FSShowTimestampsNearbyChat` (Boolean) = `1`
- `FSShowTimestampsTranscripts` (Boolean) = `1`
- `FSShowToastsInFront` (Boolean) = `0`
- `FSShowTypingStateInNameTag` (Boolean) = `0`
- `FSShowUploadPaymentToast` (Boolean) = `1`
- `FSShowVoiceVisualizerWithDot` (Boolean) = `1`
- `FSShowWhitelistReminder` (Boolean) = `1`
- `FSSkinClobbersColorPrefs` (Boolean) = `1`
- `FSSkinClobbersToolbarPrefs` (Boolean) = `0`
- `FSSkinCurrentReadableName` (String) = `Firestorm`
- `FSSkinCurrentThemeReadableName` (String) = `Grey`
- `FSSnapshotFrameBorderColor` (Color3) = `0.0`
- `FSSnapshotFrameBorderWidth` (F32) = `2.0`
- `FSSnapshotFrameGuideColor` (Color3) = `1.0`
- `FSSnapshotFrameGuideWidth` (F32) = `1.0`
- `FSSnapshotGuideStyle` (String) = `rule_of_thirds`
- `FSSnapshotGuideVisibility` (F32) = `0.5`
- `FSSnapshotLocalFormat` (S32) = `0`
- `FSSnapshotLocalNamesWithTimestamps` (Boolean) = `1`
- `FSSnapshotShowCaptureFrame` (Boolean) = `0`
- `FSSnapshotShowGuides` (Boolean) = `0`
- `FSSortAttachmentSpotsAlphabetically` (Boolean) = `1`
- `FSSortDeferalFrames` (U32) = `5`
- `FSSortFSFoldersToTop` (Boolean) = `1`
- `FSSoundCacheLocation` (String) = ``
- `FSSplashScreenHideBlogs` (Boolean) = `0`
- `FSSplashScreenHideDestinations` (Boolean) = `0`
- `FSSplashScreenHideTopBar` (Boolean) = `0`
- `FSSplashScreenNoTransparency` (Boolean) = `0`
- `FSSplashScreenUseAllCaps` (Boolean) = `0`
- `FSSplashScreenUseGrayMode` (Boolean) = `0`
- `FSSplashScreenUseHighContrast` (Boolean) = `0`
- `FSSplashScreenUseLargerFonts` (Boolean) = `0`
- `FSSplitInventorySearchOverTabs` (Boolean) = `0`
- `FSStartupClearBrowserCache` (Boolean) = `0`
- `FSStatbarLegacyMeanPerSec` (Boolean) = `0`
- `FSStaticEyesUUID` (String) = `592a9fb4-5e02-77bb-e1e5-da4d17d27336`
- `FSStatisticsNoFocus` (Boolean) = `0`
- `FSStatusBarMenuButtonPopupOnRollover` (Boolean) = `1`
- `FSStatusBarShowFPS` (Boolean) = `1`
- `FSStatusBarShowFPSColors` (Boolean) = `1`
- `FSStatusBarTimeFormat` (String) = `Language`
- `FSStatusbarShowSimulatorVersion` (Boolean) = `0`
- `FSStreamList` (LLSD) = `?`
- `FSSupportGroupChatPrefix3` (Boolean) = `1`
- `FSSupportGroupChatPrefixTesting` (Boolean) = `1`
- `FSTPHistoryTZ` (String) = `utc`
- `FSTagShowARW` (Boolean) = `1`
- `FSTagShowDistance` (Boolean) = `0`
- `FSTagShowDistanceColors` (Boolean) = `0`
- `FSTagShowOwnARW` (Boolean) = `0`
- `FSTagShowTooComplexOnlyARW` (Boolean) = `1`
- `FSTeleportHistoryShowDate` (Boolean) = `0`
- `FSTeleportHistoryShowPosition` (Boolean) = `0`
- `FSTeleportToOffsetLateral` (F32) = `0.0`
- `FSTeleportToOffsetVertical` (F32) = `2.0`
- `FSTempDerenderUntilTeleport` (Boolean) = `1`
- `FSTextureDefaultSaveAsFormat` (Boolean) = `0`
- `FSToolbarsResetOnModeChange` (Boolean) = `1`
- `FSToolboxExpanded` (Boolean) = `1`
- `FSTrimLegacyNames` (Boolean) = `1`
- `FSTurnAvatarToSelectedObject` (Boolean) = `1`
- `FSTypeDuringEmote` (Boolean) = `0`
- `FSTypingChevronPrefix` (Boolean) = `0`
- `FSUndeformUUID` (String) = `44e98907-3764-119f-1c13-cba9945d2ff4`
- `FSUnfocusChatHistoryOnReturn` (Boolean) = `1`
- `FSUnlinkConfirmEnabled` (Boolean) = `1`
- `FSUploadAnimationOnOwnAvatar` (Boolean) = `1`
- `FSUseAis3Api` (Boolean) = `1`
- `FSUseAltOOC` (Boolean) = `1`
- `FSUseAntiSpamMine` (Boolean) = `0`
- `FSUseBWEmojis` (Boolean) = `0`
- `FSUseBuiltInHistory` (Boolean) = `1`
- `FSUseChatMentionAutoComplete` (Boolean) = `1`
- `FSUseCtrlShout` (Boolean) = `1`
- `FSUseFSLegacySearch` (Boolean) = `0`
- `FSUseLegacyClienttags` (U32) = `2`
- `FSUseLegacyCursors` (Boolean) = `0`
- `FSUseLegacyInventoryAcceptMessages` (Boolean) = `0`
- `FSUseLegacyLoginPanel` (Boolean) = `0`
- `FSUseLegacyObjectProperties` (Boolean) = `0`
- `FSUseLegacyUnsupportedHardwareChecks` (Boolean) = `0`
- `FSUseNearbyChatConsole` (Boolean) = `1`
- `FSUseNewRegionRestartNotification` (Boolean) = `1`
- `FSUseNewTexturePanel` (Boolean) = `1`
- `FSUsePrettyEmojiButton` (Boolean) = `1`
- `FSUseReadOfflineMsgsCap` (Boolean) = `1`
- `FSUseShiftWhisper` (Boolean) = `1`
- `FSUseSingleLineChatEntry` (Boolean) = `0`
- `FSUseSmallCameraFloater` (Boolean) = `0`
- `FSUseStandaloneBlocklistFloater` (Boolean) = `0`
- `FSUseStandaloneGroupFloater` (Boolean) = `1`
- `FSUseStandalonePlaceDetailsFloater` (Boolean) = `0`
- `FSUseStandaloneTeleportHistoryFloater` (Boolean) = `0`
- `FSUseStatsInsteadOfLagMeter` (Boolean) = `0`
- `FSUseV2Friends` (Boolean) = `0`
- `FSVolumeControlsPanelOpen` (Boolean) = `0`
- `FSWearableFavoritesSortOrder` (U32) = `3`
- `FSWorldMapDoubleclickTeleport` (Boolean) = `1`
- `FSdataQAtest` (Boolean) = `0`
- `FSllOwnerSayToScriptDebugWindowRouting` (U32) = `0`
- `RenderChromaStrength` (F32) = `5.0`
- `RenderColorBrightness` (F32) = `0.0`
- `RenderColorContrast` (F32) = `1.0`
- `RenderColorGradingLUTIntensity` (F32) = `1.0`
- `RenderColorGradingLUTName` (String) = ``
- `RenderColorSaturation` (F32) = `1.0`
- `RenderColorTemperature` (F32) = `0.0`
- `RenderDebugSH` (Boolean) = `0`
- `RenderDepthOfFieldChroma` (Boolean) = `1`
- `RenderDepthOfFieldFront` (Boolean) = `1`
- `RenderDepthOfFieldHighQuality` (Boolean) = `0`
- `RenderJellyDollsAsImpostors` (Boolean) = `1`
- `RenderMotionBlurOtherAvatars` (Boolean) = `1`
- `RenderMotionBlurSelfAvatar` (Boolean) = `1`
- `RenderResolutionMultiplier` (F32) = `1.0`
- `RenderSMAAT2x` (Boolean) = `0`
- `RenderSculptSAThreshold` (F32) = `150.0`
- `RenderShadowSoftness` (F32) = `1.0`
- `RenderVolumeSAFrameMax` (F32) = `5000.0`
- `RenderVolumeSAProtection` (Boolean) = `0`
- `RenderVolumeSAThreshold` (F32) = `75.0`
- `RenderVolumetricLighting` (Boolean) = `1`
- `RenderVolumetricLightingDirectional` (Boolean) = `1`
- `RenderVolumetricLightingFalloffMultiplier` (F32) = `1.0`
- `RenderVolumetricLightingMultiplier` (F32) = `50.0`
- `RenderVolumetricLightingResolution` (U32) = `16`
- `WaterEditPresets` (Boolean) = `0`
- `WaterFogColor` (Color4) = `0.0863`
- `WaterFogDensity` (F32) = `16.0`

## 3.3 common cvars (default 値差分)

| # | cvar | type | BD default | AY default |
|---|---|---|---|---|
| 1 | `RenderAutoHideSurfaceAreaLimit` | F32 | `0` | `10.0E6` |
| 2 | `RenderAutoMaskAlphaDeferred` | Boolean | `0` | `1` |
| 3 | `RenderAutoMaskAlphaNonDeferred` | Boolean | `0` | `1` |
| 4 | `RenderAutoMuteSurfaceAreaLimit` | F32 | `0` | `1000.0` |
| 5 | `RenderAvatarMaxComplexity` | U32 | `250000` | `0` |
| 6 | `RenderDeferredSpotShadowOffset` | F32 | `0.0` | `0.8` |
| 7 | `RenderFSAAType` | U32 | `2` | `0` |
| 8 | `RenderFarClip` | F32 | `96.0` | `256.0` |
| 9 | `RenderGammaFull` | Boolean | `1` | `1.0` |
| 10 | `RenderGlowIterations` | S32 | `5` | `2` |
| 11 | `RenderGlowLumWeights` | Vector3 | `0.4` | `1.0` |
| 12 | `RenderGlowMaxExtractAlpha` | F32 | `0.03` | `0.25` |
| 13 | `RenderGlowMinLuminance` | F32 | `0.0` | `1.0` |
| 14 | `RenderGlowResolutionPow` | S32 | `10` | `9` |
| 15 | `RenderGlowStrength` | F32 | `0.233` | `0.325` |
| 16 | `RenderGlowWarmthAmount` | F32 | `16.0` | `0.0` |
| 17 | `RenderGlowWarmthWeights` | Vector3 | `0.75` | `1.0` |
| 18 | `RenderGlowWidth` | F32 | `3.6` | `1.3` |
| 19 | `RenderMaxVRAMBudget` | U32 | `0` | `768` |
| 20 | `RenderSSAOFactor` | F32 | `0.05` | `0.30` |
| 21 | `RenderSSAOMaxScale` | U32 | `300` | `200` |
| 22 | `RenderShadowBias` | F32 | `-0.001` | `-0.002` |
| 23 | `RenderShadowBiasError` | F32 | `0.1` | `-0.007` |
| 24 | `RenderShadowBlurDistFactor` | F32 | `0.01` | `0` |
| 25 | `RenderShadowBlurSize` | F32 | `1.0` | `1.4` |
| 26 | `RenderShadowDetail` | S32 | `1` | `2` |
| 27 | `RenderShadowErrorCutoff` | F32 | `0.0` | `5.0` |
| 28 | `RenderShadowFOVCutoff` | F32 | `0.0` | `0.8` |
| 29 | `RenderShadowGaussian` | Vector3 | `1.25` | `3.0` |
| 30 | `RenderShadowOffset` | F32 | `0.002` | `0.01` |
| 31 | `RenderTerrainScale` | F32 | `6.0` | `12.0` |
| 32 | `RenderTreeLODFactor` | F32 | `1.0` | `0.5` |
| 33 | `RenderWaterRefResolution` | S32 | `768` | `512` |

## 3.4 common cvars (type 差分 — 要注意)

- `RenderMotionBlurStrength`: BD=S32/`32`, AY=U32/`32`
- `RenderSSAOEffect`: BD=F32/`-0.5`, AY=Vector3/`0.80`
- `RenderScreenSpaceReflectionAdaptiveStepMultiplier`: BD=Vector3/`1.13`, AY=F32/`1.6`
- `RenderScreenSpaceReflectionDepthRejectBias`: BD=Vector3/`1.0`, AY=F32/`0.001`
- `RenderScreenSpaceReflectionDistanceBias`: BD=Vector3/`10.0`, AY=F32/`0.015`
- `RenderScreenSpaceReflectionIterations`: BD=Vector3/`64`, AY=S32/`25`
- `RenderScreenSpaceReflectionRayStep`: BD=Vector3/`0.025`, AY=F32/`0.1`

# Bucket 4: render UI XML (BD 995a1354d8 vs AYAstorm)

- BD UI files: **37**, AY UI files: **41**
- common: **28**, BD-only: **9**, AY-only: **13**

## 4.1 BD-only UI (BD にあって AYAstorm に無い、新規移植対象)

- `floater_adjust_water.xml` (10,939 B)
- `floater_edit_sky_preset.xml` (33,514 B)
- `floater_edit_water_preset.xml` (12,259 B)
- `floater_environment_settings.xml` (7,077 B)
- `panel_machinima.xml` (82,832 B)
- `panel_preferences_render_settings.xml` (2,778 B)
- `panel_preferences_ui_colors.xml` (22,448 B)
- `panel_settings_water_image.xml` (2,121 B)
- `panel_settings_water_settings.xml` (5,811 B)

## 4.2 AY-only UI (AYAstorm にあって BD に無い)

- `floater_aya_cinematic.xml` (14,539 B)
- `floater_beamcolor.xml` (2,454 B)
- `floater_fs_avatar_render_settings.xml` (1,750 B)
- `floater_fs_fixedenvironment.xml` (5,315 B)
- `floater_post_process.xml` (12,243 B)
- `floater_quickprefs.xml` (14,903 B)
- `menu_fs_avatar_render_setting.xml` (741 B)
- `menu_perf_avatar_rendering_settings.xml` (1,391 B)
- `panel_fs_settings_sky_atmos.xml` (7,156 B)
- `panel_fs_settings_sky_clouds.xml` (6,490 B)
- `panel_fs_settings_sky_sunmoon.xml` (7,234 B)
- `panel_fs_settings_water.xml` (6,561 B)
- `panel_quickprefs_item.xml` (3,422 B)

## 4.3 common UI (sha256 diff)

| # | file | BD sha | AY sha | diff |
|---|---|---|---|---|
| 1 | `floater_adjust_environment.xml` | `872e57cac09faca0` | `58059a534829a740` | yes |
| 2 | `floater_avatar_render_settings.xml` | `1b3cb2c9384eee81` | `7324001990f3538e` | yes |
| 3 | `floater_camera_presets.xml` | `c6f2da3499a4d09c` | `09629e0fe8b7cbdd` | yes |
| 4 | `floater_color_picker.xml` | `438b921f4b82e6d0` | `f7546e00dd301f7a` | yes |
| 5 | `floater_display_name.xml` | `77da0c1010c28d7e` | `f28b5c553155d2d8` | yes |
| 6 | `floater_fbc_web.xml` | `cd1f8129a64db8d8` | `c41a19d5df9357f1` | yes |
| 7 | `floater_fixedenvironment.xml` | `2a5f6e0939a9ead2` | `bfbfe39c7ffb5de9` | yes |
| 8 | `floater_my_environments.xml` | `202f5acfaccee30e` | `c296ea376f850f5c` | yes |
| 9 | `floater_preferences_graphics_advanced.xml` | `97792fb3c790401d` | `2651cfdb59a00c2d` | yes |
| 10 | `floater_settings_color.xml` | `9d4a35e9964f3cad` | `c10e3eeadb7d75af` | yes |
| 11 | `menu_avatar_rendering_settings.xml` | `dab9695dac91643e` | `2373f23a9c3a25be` | yes |
| 12 | `menu_avatar_rendering_settings_add.xml` | `de3b25f93d7a9c46` | `48f21b453cedf5ae` | yes |
| 13 | `menu_copy_paste_color.xml` | `7e6dd0ea5f0b2843` | `7e6dd0ea5f0b2843` | no |
| 14 | `panel_postcard_message.xml` | `7c7dd9857f4192ad` | `7c7dd9857f4192ad` | no |
| 15 | `panel_postcard_settings.xml` | `c16c39a691fe209b` | `ef6133027d9801c2` | yes |
| 16 | `panel_preferences_colors.xml` | `ca35e9ff6c7e7c68` | `3aab0080b4b69943` | yes |
| 17 | `panel_preferences_graphics1.xml` | `048762e204f53c06` | `ad7cf383188c8ed6` | yes |
| 18 | `panel_presets_camera_pulldown.xml` | `15b6f6185d7bde93` | `dd78223b06136da9` | yes |
| 19 | `panel_presets_pulldown.xml` | `440ecf4b0d098c52` | `8e5ee7f22572c940` | yes |
| 20 | `panel_region_environment.xml` | `5cd5d5824d2b778c` | `c937bd81a10229d0` | yes |
| 21 | `panel_settings_sky_atmos.xml` | `8b28b03453332cfd` | `89430a1927cc24f8` | yes |
| 22 | `panel_settings_sky_clouds.xml` | `92114c17981d7a82` | `5fc6a5af33bd4c4c` | yes |
| 23 | `panel_settings_sky_density.xml` | `cb9971b6936b0fa5` | `e21613561e538680` | yes |
| 24 | `panel_settings_sky_sunmoon.xml` | `8b7af39834975b7f` | `6f63cb0ab159559b` | yes |
| 25 | `panel_settings_water.xml` | `760f897a45d9bfc9` | `5d6e620e3178474d` | yes |
| 26 | `panel_snapshot_postcard.xml` | `05beedc8ff628a56` | `87441e02eb026f57` | yes |
| 27 | `widgets/color_swatch.xml` | `7bd6d51d3e7f793b` | `1b194e0d6387f013` | yes |
| 28 | `widgets/sun_moon_trackball.xml` | `eee0fcdddbb421d3` | `eee0fcdddbb421d3` | no |

**common 中身 diff 集計**: 25 / 28 件で差分あり
# Bucket 5: graphic / environment presets and assets

Scope: windlight (sky/water/day presets) + filters + camera presets.
BD has no `app_settings/presets/` (= Firestorm graphic preset file system).

## 5.0 トップレベル ディレクトリ差分

- BD top-level dirs: ['camera', 'filters', 'shaders', 'windlight']
- AY top-level dirs: ['beams', 'beamsColors', 'camera', 'filters', 'fs_static_assets', 'luts', 'poses', 'shaders', 'static_assets', 'venue_ir', 'windlight']
- BD-only dirs: []
- AY-only dirs: ['beams', 'beamsColors', 'fs_static_assets', 'luts', 'poses', 'static_assets', 'venue_ir']

## 5.x `windlight/skies/`

- BD files: **111**, AY files: **769**
- common: **48**, BD-only: **63**, AY-only: **721**

### `windlight/skies/` BD-only

- `%5BTarnix%5D%20Dramatic%20Fog%202.xml` (4,405 B)
- `%5BTarnix%5D%20Dramatic%20Fog.xml` (4,415 B)
- `%5BTarnix%5D%20End%2Dof%2Dworld.xml` (4,003 B)
- `%5BTarnix%5D%20Mystical%20Forest.xml` (4,019 B)
- `%5BTarnix%5D%20Rainy%20Forest.xml` (3,988 B)
- `%5BTarnix%5D%20Rusted%20Perception.xml` (4,034 B)
- `%5BTarnix%5D%20Shine.xml` (4,146 B)
- `%5BTarnix%5D%20Smooth%20spot.xml` (3,991 B)
- `%5BTarnix%5D%20Ultrabright%20Day.xml` (4,318 B)
- `%5BTarnix%5D%20Ultrabright%20Morning.xml` (3,959 B)
- `%5BTarnix%5D%20Ultrabright%20Night.xml` (4,308 B)
- `%5BTarnix%5D%20Xealot.xml` (4,215 B)
- `Amsterdam.xml` (4,065 B)
- `AnaLutetia%20%2D%20Default.xml` (4,332 B)
- `AnaLutetia%20%2D%20Studio%202.xml` (3,983 B)
- `AnaLutetia%20%2D%20Studio.xml` (3,934 B)
- `Bleu.xml` (4,039 B)
- `Bright.xml` (3,910 B)
- `Burn.xml` (4,036 B)
- `Crazy%20Midnight.xml` (3,960 B)
- `DaytimeShadows.xml` (4,058 B)
- `Dramatic%20Fog.xml` (4,157 B)
- `Dusty%20Bright.xml` (4,047 B)
- `E%2D12AM%20C.xml` (4,059 B)
- `E%2D12AM.xml` (4,125 B)
- `E%2D3AM.xml` (4,125 B)
- `E%2D3PM%20B.xml` (4,040 B)
- `E%2D5AM.xml` (4,092 B)
- `E%2D6AM%20B.xml` (4,009 B)
- `E%2D6PM%20B.xml` (4,060 B)
- `E%2D7PM%20C.xml` (4,059 B)
- `E%2D9AM%20B.xml` (4,043 B)
- `E%2D9PM.xml` (4,006 B)
- `F%2D12AM.xml` (4,125 B)
- `F%2D12PM.xml` (4,058 B)
- `F%2D3AM.xml` (4,125 B)
- `F%2D3PM.xml` (4,025 B)
- `F%2D5AM.xml` (4,092 B)
- `F%2D6AM.xml` (4,009 B)
- `F%2D6PM.xml` (4,060 B)
- `F%2D7PM.xml` (4,059 B)
- `F%2D9AM.xml` (4,041 B)
- `F%2D9PM.xml` (4,006 B)
- `Freakangels%202.xml` (4,031 B)
- `Freakangels%205.xml` (4,024 B)
- `Insilico.xml` (4,004 B)
- `Itsgonnarain.xml` (4,079 B)
- `KL%20Purple.xml` (4,084 B)
- `London%202027.xml` (3,975 B)
- `Luna%20Jubilee%20%2D%20Snowday.xml` (4,047 B)
- `Luna%20Jubilee%20%2D%20Springscape.xml` (4,024 B)
- `Overcast.xml` (4,063 B)
- `Pitch%20Black%20Nights.xml` (3,840 B)
- `Rainy%20Foggy.xml` (3,880 B)
- `Rainy.xml` (3,905 B)
- `Realistic%20Night.xml` (3,830 B)
- `Recording%20Hippotropolis.xml` (4,049 B)
- `Reflection%20Abstract.xml` (3,955 B)
- `Shadow.xml` (3,977 B)
- `ShadowsBrightDay.xml` (4,024 B)
- `SkyeHigh.xml` (4,163 B)
- `TorleyRise.xml` (4,077 B)
- `Zest.xml` (4,133 B)

### `windlight/skies/` AY-only

- `%28SS%29%20Atmos%2000%3A00%202.xml` (3,935 B)
- `%28SS%29%20Atmos%2001%3A00%201.xml` (3,965 B)
- `%28SS%29%20Atmos%2002%3A00%202.xml` (3,965 B)
- `%28SS%29%20Atmos%2004%3A00%201.xml` (3,979 B)
- `%28SS%29%20Atmos%2006%3A00%201.xml` (3,935 B)
- `%28SS%29%20Atmos%2007%3A00%201.xml` (3,970 B)
- `%28SS%29%20Atmos%2008%3A00%201.xml` (3,986 B)
- `%28SS%29%20Atmos%2012%3A00%201.xml` (4,008 B)
- `%28SS%29%20Atmos%2013%3A00%201.xml` (4,025 B)
- `%28SS%29%20Atmos%2014%3A00%201.xml` (4,011 B)
- `%28SS%29%20Atmos%2015%3A00%201.xml` (4,011 B)
- `%28SS%29%20Atmos%2016%3A00%201.xml` (4,011 B)
- `%28SS%29%20Atmos%2017%3A00%201.xml` (4,011 B)
- `%28SS%29%20Atmos%2018%3A00%201.xml` (4,035 B)
- `%28SS%29%20Atmos%2019%3A00%201.xml` (3,970 B)
- `%28SS%29%20Atmos%2020%3A00%201.xml` (3,980 B)
- `%28SS%29%20Atmos%2023%2E30%202.xml` (3,949 B)
- `%28SS%29%20Atmospheric%2000%3A00b.xml` (4,016 B)
- `%28SS%29%20Atmospheric%2000%3A00cloudy.xml` (4,017 B)
- `%28SS%29%20Atmospheric%2000%3A00cloudy2.xml` (4,002 B)
- `%28SS%29%20Atmospheric%2004%3A00b.xml` (3,938 B)
- `%28SS%29%20Atmospheric%2004%3A00cloudy.xml` (3,939 B)
- `%28SS%29%20Atmospheric%2004%3A00cloudy2.xml` (4,014 B)
- `%28SS%29%20Atmospheric%2006%3A00%206b.xml` (4,036 B)
- `%28SS%29%20Atmospheric%2006%3A00cloudy.xml` (4,037 B)
- `%28SS%29%20Atmospheric%2006%3A00cloudy2.xml` (4,022 B)
- `%28SS%29%20Atmospheric%2007%3A00b.xml` (3,963 B)
- `%28SS%29%20Atmospheric%2007%3A00cloudy.xml` (3,964 B)
- `%28SS%29%20Atmospheric%2007%3A00cloudy2.xml` (3,967 B)
- `%28SS%29%20Atmospheric%2008%3A00b.xml` (3,981 B)
- `%28SS%29%20Atmospheric%2008%3A00cloudy.xml` (3,982 B)
- `%28SS%29%20Atmospheric%2008%3A00cloudy2.xml` (3,983 B)
- `%28SS%29%20Atmospheric%2012%3A00%20midday6b.xml` (3,997 B)
- `%28SS%29%20Atmospheric%2012%3A00cloudy.xml` (3,998 B)
- `%28SS%29%20Atmospheric%2012%3A00cloudy2.xml` (4,035 B)
- `%28SS%29%20Atmospheric%2013%3A00b.xml` (3,996 B)
- `%28SS%29%20Atmospheric%2013%3A00cloudy.xml` (3,997 B)
- `%28SS%29%20Atmospheric%2013%3A00cloudy2.xml` (3,982 B)
- `%28SS%29%20Atmospheric%2014%3A00b.xml` (3,978 B)
- `%28SS%29%20Atmospheric%2014%3A00cloudy.xml` (3,979 B)
- `%28SS%29%20Atmospheric%2014%3A00cloudy2.xml` (3,964 B)
- `%28SS%29%20Atmospheric%2015%3A00b.xml` (3,978 B)
- `%28SS%29%20Atmospheric%2015%3A00cloudy.xml` (3,979 B)
- `%28SS%29%20Atmospheric%2015%3A00cloudy2.xml` (3,964 B)
- `%28SS%29%20Atmospheric%2016%3A00b.xml` (3,978 B)
- `%28SS%29%20Atmospheric%2016%3A00cloudy.xml` (3,979 B)
- `%28SS%29%20Atmospheric%2016%3A00cloudy2.xml` (3,964 B)
- `%28SS%29%20Atmospheric%2017%3A00b.xml` (3,978 B)
- `%28SS%29%20Atmospheric%2017%3A00cloudy.xml` (3,979 B)
- `%28SS%29%20Atmospheric%2017%3A00cloudy2.xml` (3,964 B)
- `%28SS%29%20Atmospheric%2018%3A00b.xml` (3,996 B)
- `%28SS%29%20Atmospheric%2018%3A00cloudy.xml` (3,997 B)
- `%28SS%29%20Atmospheric%2018%3A00cloudy2.xml` (3,982 B)
- `%28SS%29%20Atmospheric%2019%3A00b.xml` (3,992 B)
- `%28SS%29%20Atmospheric%2019%3A00cloudy.xml` (3,993 B)
- `%28SS%29%20Atmospheric%2019%3A00cloudy2.xml` (3,978 B)
- `%28SS%29%20Atmospheric%2020%3A00b.xml` (4,004 B)
- `%28SS%29%20Atmospheric%2020%3A00cloudy2.xml` (3,989 B)
- `%28SS%29%20Atmospheric20%3A00cloudy.xml` (4,005 B)
- `%2ACanimod.xml` (3,949 B)
- `%2AStarley%2A%20Settings%202.xml` (3,926 B)
- `%5BAnaLu%5D%20AvatarOpt%20%28Caliah%29%20whiter.xml` (3,981 B)
- `%5BAnaLu%5D%20AvatarOpt%20%28Caliah%29.xml` (3,949 B)
- `%5BAnaLu%5D%20Studio%20Light%20%28Gillian%29.xml` (3,956 B)
- `%5BAnaLu%5D%20default1.xml` (4,332 B)
- `%5BAnaLu%5D%20default2.xml` (4,103 B)
- `%5BAnaLu%5D%20default3.xml` (4,104 B)
- `%5BAnaLu%5D%20neutral.xml` (4,106 B)
- `%5BAnaLu%5D%20outdoor%20city%20weirdlights.xml` (4,056 B)
- `%5BAnaLu%5D%20outdoor%20city.xml` (4,055 B)
- `%5BAnaLu%5D%20outdoor%20night.xml` (3,986 B)
- `%5BAnaLu%5D%20shadows.xml` (3,914 B)
- `%5BAnaLu%5D%20studio1.xml` (3,935 B)
- `%5BAnaLu%5D%20studio2.xml` (3,983 B)
- `%5BAnaLu%5D%20studio3.xml` (3,984 B)
- `%5BAnaLu%5D%20studio4.xml` (4,318 B)
- `%5BAnaLu%5D%20studio5.xml` (3,993 B)
- `%5BAnaLu%5D%20studio6.xml` (4,034 B)
- `%5BAnaLu%5D%20studio7.xml` (4,049 B)
- `%5BAnaLu%5D%20studio8.xml` (4,034 B)
- `%5BAnaLu%5D%20studio9.xml` (4,033 B)
- `%5BCB%5D%20Rouge%201.xml` (4,135 B)
- `%5BCB%5D%20Rouge%202.xml` (4,139 B)
- `%5BCB%5D%20Rouge%203.xml` (4,148 B)
- `%5BCB%5D%20Rouge%204.xml` (4,134 B)
- `%5BCB%5D%20Rouge%205.xml` (4,069 B)
- `%5BCB%5D%20Rouge%206.xml` (4,070 B)
- `%5BEUPHORIA%5D%20Plum%20sunset.xml` (4,048 B)
- `%5BEUPHORIA%5D%20air%20pollution%202.xml` (3,750 B)
- `%5BEUPHORIA%5D%20bergamot%20%26%20rosemary.xml` (4,110 B)
- `%5BEUPHORIA%5D%20day%20of%20the%20end.xml` (3,975 B)
- `%5BEUPHORIA%5D%20fin%20de%20siecle.xml` (3,995 B)
- `%5BEUPHORIA%5D%20hello%20gaia.xml` (4,069 B)
- `%5BEUPHORIA%5D%20low%20saturation%20dull%20deposit.xml` (4,139 B)
- `%5BEUPHORIA%5D%20monochrome.xml` (3,926 B)
- `%5BEUPHORIA%5D%20natural%20high.xml` (4,258 B)
- `%5BEUPHORIA%5D%20realm.xml` (4,111 B)
- `%5BEUPHORIA%5D%20rotten%20melon.xml` (4,083 B)
- `%5BEUPHORIA%5D%20sleep%20warm.xml` (3,994 B)
- `%5BEUPHORIA%5D%20smoky%20blue%20sky%20%3A%20reverse.xml` (4,038 B)
- `%5BEUPHORIA%5D%20smoky%20blue%20sky.xml` (4,110 B)
- `%5BEUPHORIA%5D%20sunset%201.xml` (4,053 B)
- `%5BEUPHORIA%5D%20sunset%202.xml` (4,033 B)
- `%5BEUPHORIA%5D%20that%20day.xml` (4,160 B)
- `%5BEUPHORIA%7D%20air%20pollution%201.xml` (3,837 B)
- `%5BNB%5D%20Aftermath%200000.xml` (4,399 B)
- `%5BNB%5D%20Aftermath%200500.xml` (4,391 B)
- `%5BNB%5D%20Aftermath%201200.xml` (4,275 B)
- `%5BNB%5D%20Aftermath%201630.xml` (4,278 B)
- `%5BNB%5D%20Aftermath%201750.xml` (4,303 B)
- `%5BNB%5D%20Aftermath%202000.xml` (4,343 B)
- `%5BNB%5D%20Alpine%2Dskinlight%20RGB.xml` (4,221 B)
- `%5BNB%5D%20P%2DHaze%200000.xml` (4,335 B)
- `%5BNB%5D%20P%2DHaze%200430.xml` (4,160 B)
- `%5BNB%5D%20P%2DHaze%200800.xml` (4,202 B)
- `%5BNB%5D%20P%2DHaze%201200.xml` (4,128 B)
- `%5BNB%5D%20P%2DHaze%201700.xml` (4,171 B)
- `%5BNB%5D%20P%2DHaze%201800.xml` (4,007 B)
- `%5BNB%5D%20P%2DHaze%201900.xml` (4,051 B)
- `%5BNB%5D%20P%2DHaze%202000.xml` (4,090 B)
- `%5BNB%5D%20Sepia%200000.xml` (4,118 B)
- `%5BNB%5D%20Sepia%200430.xml` (4,141 B)
- `%5BNB%5D%20Sepia%200800.xml` (4,134 B)
- `%5BNB%5D%20Sepia%201200.xml` (4,085 B)
- `%5BNB%5D%20Sepia%201700.xml` (4,127 B)
- `%5BNB%5D%20Sepia%201800.xml` (4,036 B)
- `%5BNB%5D%20Sepia%202000.xml` (4,044 B)
- `%5BNB%5D%2DMistyDay%2D12am.xml` (3,966 B)
- `%5BNB%5D%2DMistyDay%2D12pm.xml` (4,170 B)
- `%5BNB%5D%2DMistyDay%2D4am.xml` (4,071 B)
- `%5BNB%5D%2DMistyDay%2D4pm.xml` (4,141 B)
- `%5BNB%5D%2DMistyDay%2D5am.xml` (4,081 B)
- `%5BNB%5D%2DMistyDay%2D5pm.xml` (4,155 B)
- `%5BNB%5D%2DMistyDay%2D6am.xml` (4,123 B)
- `%5BNB%5D%2DMistyDay%2D6pm.xml` (4,097 B)
- `%5BNB%5D%2DMistyDay%2D7pm.xml` (4,054 B)
- `%5BNB%5D%2DMistyDay%2D8am.xml` (4,125 B)
- `%5BNB%5D%2DMistyDay%2D9pm.xml` (4,104 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Awwyeah.xml` (4,123 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Eats%20night.xml` (3,707 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Good%20for%20marriage.xml` (4,045 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Good%20for%20warm%20sailing.xml` (4,091 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Impires.xml` (4,050 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Raylanta.xml` (4,003 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Safarion.xml` (3,988 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Slastlevania.xml` (4,010 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Slips%20through%20walls%201.xml` (3,932 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Slips%20through%20walls%202.xml` (3,962 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Slips%20through%20walls%203.xml` (4,012 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Slips%20through%20walls%204.xml` (4,029 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Slips%20through%20walls%205.xml` (4,026 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Slips%20through%20walls%206.xml` (3,949 B)
- `%5BTOR%5D%20BIG%20SUN%20%2D%20Variationz.xml` (4,139 B)
- `%5BTOR%5D%20DUSK%20%2D%20Apts.xml` (4,057 B)
- `%5BTOR%5D%20DUSK%20%2D%20Beachin%27.xml` (4,082 B)
- `%5BTOR%5D%20DUSK%20%2D%20Blue%20hour.xml` (4,112 B)
- `%5BTOR%5D%20DUSK%20%2D%20Briony.xml` (3,956 B)
- `%5BTOR%5D%20DUSK%20%2D%20Burnt%20hope.xml` (4,159 B)
- `%5BTOR%5D%20DUSK%20%2D%20Fairytale%20glow.xml` (4,100 B)
- `%5BTOR%5D%20DUSK%20%2D%20Patriotix.xml` (4,019 B)
- `%5BTOR%5D%20DUSK%20%2D%20Sepia%27ed.xml` (4,055 B)
- `%5BTOR%5D%20DUSK%20%2D%20Siennarel.xml` (4,173 B)
- `%5BTOR%5D%20DUSK%20%2D%20Smolder.xml` (4,104 B)
- `%5BTOR%5D%20DUSK%20%2D%20Somber%20rose.xml` (4,123 B)
- `%5BTOR%5D%20DUSK%20%2D%20Strummer.xml` (4,058 B)
- `%5BTOR%5D%20DUSK%20%2D%20Vision%20of%20something.xml` (3,979 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Catastrophe.xml` (4,101 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Chouchou%20rockets.xml` (4,177 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Das%20fog.xml` (3,570 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Fogvari.xml` (3,974 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Golden%20glow.xml` (4,131 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Lollitudes.xml` (4,069 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Melon%20chapterhouse.xml` (4,152 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Mint%20teahouse.xml` (3,987 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Peach%20Peche.xml` (3,974 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Rose%2Dtinted.xml` (3,659 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Sad%20purple.xml` (4,120 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Silent%20heck.xml` (3,952 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Southern%20delite.xml` (4,091 B)
- `%5BTOR%5D%20FOGGY%20%2D%20Terpsa%20bonne.xml` (4,151 B)
- `%5BTOR%5D%20FOGGY%20%2D%20The%20pink%20tower.xml` (4,145 B)
- `%5BTOR%5D%20HORROR%20%2D%20Asian%20red.xml` (3,938 B)
- `%5BTOR%5D%20HORROR%20%2D%20Bloody%20moon.xml` (4,085 B)
- `%5BTOR%5D%20HORROR%20%2D%20Castle%20vain.xml` (4,166 B)
- `%5BTOR%5D%20HORROR%20%2D%20Commie%20skies.xml` (4,055 B)
- `%5BTOR%5D%20HORROR%20%2D%20Darkside%20tales.xml` (4,105 B)
- `%5BTOR%5D%20HORROR%20%2D%20Dawnbreaker.xml` (4,072 B)
- `%5BTOR%5D%20HORROR%20%2D%20Dullard.xml` (4,120 B)
- `%5BTOR%5D%20HORROR%20%2D%20Evil%20machinery.xml` (4,029 B)
- `%5BTOR%5D%20HORROR%20%2D%20Land%20of%20rising.xml` (4,176 B)
- `%5BTOR%5D%20HORROR%20%2D%20Monochrome%20redflog.xml` (3,660 B)
- `%5BTOR%5D%20HORROR%20%2D%20Namob%20islet.xml` (4,127 B)
- `%5BTOR%5D%20HORROR%20%2D%20Profondo%20rosso.xml` (4,064 B)
- `%5BTOR%5D%20HORROR%20%2D%20Rancid%20milk.xml` (4,067 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20An%20incongruent%20truth.xml` (4,106 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Anime%20Ciel.xml` (4,061 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Azure%20desertation.xml` (4,034 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Baskaholic.xml` (4,046 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Beachin%27.xml` (4,018 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Character.xml` (4,145 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Cheery%20cyan.xml` (4,195 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Coral%20reef.xml` (4,134 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Herr%20Gracken.xml` (4,173 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Londonisk.xml` (4,099 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Maldives.xml` (4,021 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Morally%20satisfied.xml` (4,107 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20My%20noon.xml` (4,124 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Nostalgika.xml` (4,169 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Pleasantrie.xml` (4,164 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Precision%20blue.xml` (4,157 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Quiet%20confidence.xml` (4,027 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Rebranded%20skies.xml` (4,176 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Salmon.xml` (4,178 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Touch%20o%27%20purple.xml` (4,134 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Vintage%20Village.xml` (4,119 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Why%20so%20blue%3F%201.xml` (4,121 B)
- `%5BTOR%5D%20MIDDAY%20%2D%20Why%20so%20blue%3F%202.xml` (4,145 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Anwar.xml` (4,101 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Bright%20blue%20horizon.xml` (4,156 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Brighter.xml` (3,966 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Dark%20came%20over.xml` (4,172 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Faux%20moon.xml` (4,005 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Flyer.xml` (4,173 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Mighty%20moon.xml` (3,844 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Moony.xml` (4,152 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Nocturne.xml` (3,938 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Our%20together.xml` (4,090 B)
- `%5BTOR%5D%20NIGHT%20%2D%20That%20spells%20moon%201.xml` (4,116 B)
- `%5BTOR%5D%20NIGHT%20%2D%20That%20spells%20moon%202.xml` (4,115 B)
- `%5BTOR%5D%20NIGHT%20%2D%20That%20spells%20moon%203.xml` (4,171 B)
- `%5BTOR%5D%20NIGHT%20%2D%20That%20spells%20moon%204.xml` (4,190 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Twilight%20rider.xml` (4,174 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Under%20a%20yellow%20moon.xml` (4,144 B)
- `%5BTOR%5D%20NIGHT%20%2D%20Wuxia.xml` (3,925 B)
- `%5BTOR%5D%20NIGHT%2D%20Northern%20lite.xml` (4,128 B)
- `%5BTOR%5D%20SCIFI%20%2D%202012.xml` (4,096 B)
- `%5BTOR%5D%20SCIFI%20%2D%202013.xml` (4,210 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Albedo%200%2E39.xml` (3,740 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Alien%20planet.xml` (4,136 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Arid%20nestler.xml` (4,150 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Arrakissed%201.xml` (4,042 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Arrakissed%202.xml` (4,041 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Awesome%20atmo%201.xml` (4,044 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Awesome%20atmo%202.xml` (4,044 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Awesome%20atmo%203.xml` (4,045 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Awesome%20atmo%204.xml` (4,026 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Beautiful%20again.xml` (4,081 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Big%20pink.xml` (4,083 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Blue%20highline.xml` (4,105 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Blue%20skies%20LOL.xml` (4,016 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Blue%20sun%20warmed.xml` (3,791 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Bridge%20opera%201.xml` (3,923 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Bridge%20opera%202.xml` (3,824 B)
- `%5BTOR%5D%20SCIFI%20%2D%20CGAtrope.xml` (4,152 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Chinese%20legacy.xml` (4,004 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Climacontrasty.xml` (4,168 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Conciergist.xml` (4,050 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Cragelica.xml` (4,144 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Cranched.xml` (4,160 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Eno%20would%20be%20proud.xml` (4,193 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Evocrads%27lime.xml` (3,920 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Fog%20mystic%201.xml` (4,128 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Fog%20mystic%202.xml` (4,108 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Gelatto.xml` (3,970 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Gelding%20morose.xml` (4,001 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Gimler%20gronchi.xml` (4,165 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Golden.xml` (3,937 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Green%20whirl.xml` (4,072 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Grit%20%26%20shame.xml` (4,008 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Hong%20Kong%20planet.xml` (4,189 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Kelf%20shelf.xml` (3,920 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Lilack.xml` (4,197 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Lords%20of%20Barsoom.xml` (4,080 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Lunar%20kuwang.xml` (4,159 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Lydian%20dinosaurs.xml` (4,128 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Martian%20meat.xml` (3,953 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Metal%20Gear%20Solid.xml` (3,970 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Military%20camo.xml` (4,149 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Moon%20goons.xml` (4,055 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Morphirizon.xml` (4,053 B)
- `%5BTOR%5D%20SCIFI%20%2D%20New%20romantic.xml` (4,163 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Non%2Dgeneric%20fantasy.xml` (4,031 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Noxumic.xml` (4,134 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Orange%20balm.xml` (4,077 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Oyster%20Bay.xml` (3,892 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Phlogiston.xml` (4,083 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Planet%20Xmas.xml` (4,070 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Polaron%20charge%201.xml` (4,106 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Polaron%20charge%202.xml` (4,133 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Port%20skies.xml` (4,124 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Purple%20wisps%20%26%20egg%20yolk.xml` (4,161 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Railingz%201.xml` (4,107 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Railingz%202.xml` (4,079 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Rainbowtek.xml` (4,040 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Ramshagguld%201.xml` (3,952 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Ramshagguld%202.xml` (3,959 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Rezzable%20Hallucinogen.xml` (4,025 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Rimmerthal.xml` (4,108 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Sailor%20moon.xml` (4,138 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Scanning%20aliens.xml` (3,997 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Shilarto.xml` (4,169 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Shine%20on%20my%20friends.xml` (4,140 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Sky%20as%20canvas.xml` (3,946 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Sparz%20puft.xml` (4,171 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Spiceflow.xml` (3,955 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Stay%20stonkin%27.xml` (4,091 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Suppression%20band.xml` (4,045 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Teak%20weak.xml` (4,002 B)
- `%5BTOR%5D%20SCIFI%20%2D%20This%20alien%20life.xml` (3,934 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Touchness.xml` (4,065 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Un%20Peinture.xml` (4,061 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Unorthodox%20happy.xml` (4,083 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Use%20on%20fine%20ships.xml` (4,180 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Watermelon%2Dish.xml` (4,164 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Whirld%20spinnin.xml` (4,147 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Wild%20Palms.xml` (3,751 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Yo%20pirates%21.xml` (4,051 B)
- `%5BTOR%5D%20SCIFI%20%2D%20Yolka%20loka.xml` (4,157 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Blacklight.xml` (3,698 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Dreamwalker.xml` (3,528 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Green%20highlighter.xml` (3,515 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Heaven.xml` (3,662 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Light%20thru%20veins.xml` (3,775 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Posture%20eyes.xml` (3,849 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Puur.xml` (3,638 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Rightvision.xml` (3,487 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Threshold.xml` (3,666 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Use%20with%20full%20bright.xml` (3,468 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20Watermelon.xml` (3,672 B)
- `%5BTOR%5D%20SPECIAL%20%2D%20What%20I%20am%20thinking.xml` (4,039 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Across%20process.xml` (4,094 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Asiatix.xml` (3,984 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Baffin.xml` (3,998 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Barcelon%201.xml` (4,062 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Barcelon%202.xml` (4,126 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Breaking%20firmament.xml` (4,067 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Brillianter%20sunrise.xml` (4,184 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Bubble%20sun%20severance.xml` (4,125 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Canyon%20dreams%20red.xml` (4,189 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Canyon%20dreams.xml` (4,192 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Chernoble.xml` (4,062 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Clouds%20indifferent.xml` (4,106 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Coastal%201.xml` (4,085 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Coastal%202.xml` (4,083 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Coastal%203.xml` (4,105 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Cobraring%201.xml` (3,962 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Cobraring%202.xml` (4,012 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Deep%20blue%20sky%202.xml` (4,150 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Defenderz%201.xml` (4,170 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Defenderz%202.xml` (4,117 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Desert.xml` (4,071 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Eye%20of%20Mowron.xml` (4,175 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Farmatronic%20sepia.xml` (4,092 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Fine%20Scottish%20day.xml` (4,173 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Frenlite.xml` (4,159 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Hot.xml` (4,177 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Island%20cusp.xml` (3,981 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Lemon%2Dpeach%20smoothie.xml` (4,121 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Liz%20gluft.xml` (4,171 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Lonely%20cyberpunks.xml` (4,144 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Moon%20berries.xml` (4,073 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Teaching.xml` (4,223 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Tortoise%20isle.xml` (4,073 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Turtle%20island.xml` (4,140 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Tusken.xml` (4,074 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Twisted%20pixels.xml` (4,195 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Ultimate%21.xml` (4,156 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20We%27re%20so%20vane.xml` (3,909 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Whisked.xml` (4,129 B)
- `%5BTOR%5D%20SUNRISE%20%2D%20Wonderzan.xml` (4,070 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Azure%20desertation%202.xml` (4,035 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Bangko%201.xml` (3,997 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Bangko%202.xml` (4,001 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Blue%20inspiration.xml` (4,057 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Brouhatta%20charme.xml` (3,912 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Buccaneers%20of%20the%20coast.xml` (4,119 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Camp%20champ.xml` (4,078 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Damage%20down.xml` (4,068 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Desert.xml` (4,091 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Early%20warning.xml` (4,180 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Eggdrop.xml` (4,126 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Endgame.xml` (4,155 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Garrigal%20moscheles%201.xml` (4,010 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Garrigal%20moscheles%202.xml` (4,008 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Holy%20romance.xml` (3,982 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Ivory%20flowers.xml` (4,158 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Kyrosonatine.xml` (4,190 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Langnun.xml` (3,860 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Malts.xml` (4,107 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Ominox.xml` (4,028 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Oriental%20delight%201.xml` (4,159 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Oriental%20delight%202.xml` (4,174 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Pale%20incabon.xml` (4,093 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Pevensie.xml` (4,107 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Pink%20sink.xml` (4,175 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Pinker.xml` (4,119 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Ravenelle%27s%20choice.xml` (4,048 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Red%20city.xml` (3,950 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Silhouetta.xml` (4,056 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Stark%20epiphany.xml` (4,006 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Study%20vessels.xml` (4,139 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Vacation.xml` (4,134 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Vehicle%20Sandbox.xml` (4,090 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Verdancy.xml` (4,051 B)
- `%5BTOR%5D%20SUNSET%20%2D%20Warmer.xml` (3,977 B)
- `80%27s%20Wave.xml` (4,047 B)
- `AM%20Radio%27s%20Nostalgia.xml` (2,692 B)
- `Alchemy%20Immortalis%20%2D%20Dark%20Misty%20Night.xml` (3,906 B)
- `Alchemy%20Immortalis%20%2D%20Fog%20Lifting.xml` (4,066 B)
- `Alchemy%20Immortalis%20%2D%20Foggy%20Morning.xml` (3,831 B)
- `Ambient%20Dark.xml` (3,614 B)
- `Ambient%20Grey.xml` (3,806 B)
- `Ambient%20White.xml` (3,737 B)
- `AnaLu%20%2Astudio%2A%205.xml` (3,992 B)
- `AnaLu%20%2D%20outdoor%20city%20night.xml` (4,056 B)
- `AnaLu%20%2D%20outdoor%20city.xml` (4,054 B)
- `AnaLu%20%2D%20outdoor%20night.xml` (3,986 B)
- `AnaLutetia%20%2D%20STUDIO2.xml` (3,983 B)
- `AnaLutetia%20%2D%20STUDIO3.xml` (3,984 B)
- `AnaLutetia%20%2D%20outdoor.xml` (4,059 B)
- `AnaLutetia%2Ddefault.xml` (4,332 B)
- `AnaLutetia%2Doutdoor2.xml` (4,056 B)
- `AnaLutetia%2Dstudio.xml` (3,934 B)
- `Anime%20%2D%2012AM.xml` (3,964 B)
- `Anime%20%2D%2012PM.xml` (3,939 B)
- `Anime%20%2D%203AM.xml` (4,065 B)
- `Anime%20%2D%203PM.xml` (3,974 B)
- `Anime%20%2D%205%3A30AM.xml` (4,029 B)
- `Anime%20%2D%206%3A30AM.xml` (3,992 B)
- `Anime%20%2D%206%3A30PM.xml` (3,856 B)
- `Anime%20%2D%206AM.xml` (3,956 B)
- `Anime%20%2D%206PM.xml` (3,928 B)
- `Anime%20%2D%209AM.xml` (3,950 B)
- `Anime%20%2D%209PM.xml` (3,975 B)
- `Annan%20Adored%20Dark%20Red%20sky.xml` (3,953 B)
- `Annan%20Adored%20Darkness.xml` (4,035 B)
- `Annan%20Adored%20Dusty.xml` (4,067 B)
- `Annan%20Adored%20Light%20Explosion%20II.xml` (4,039 B)
- `Annan%20Adored%20Light%20Explosion%20III.xml` (3,930 B)
- `Annan%20Adored%20Light%20Explosion.xml` (3,958 B)
- `Annan%20Adored%20Optimal%20Skin%20%28no%20shadows%29.xml` (4,067 B)
- `Annan%20Adored%20Realistic%20ambient.xml` (4,013 B)
- `Annan%20Adored%20Red%20moments.xml` (4,072 B)
- `Annan%20Adored%20Tan%20Skin.xml` (3,913 B)
- `Annyka%27s%20Soft%20Lavender%20Day.xml` (3,969 B)
- `AvatarOpt.xml` (3,932 B)
- `B5%2DShadowDancing.xml` (4,051 B)
- `Bree%27s%20appleblossom.xml` (2,867 B)
- `Bright2.xml` (3,910 B)
- `Bryn%20Oh%27s%20Annas%20Many%20Murders.xml` (4,099 B)
- `Bryn%20Oh%27s%20BLUNIVERSE.xml` (4,118 B)
- `Bryn%20Oh%27s%20BOX%20MetaLES.xml` (3,997 B)
- `Bryn%20Oh%27s%20Condos%20in%20Heaven%20sky%20%232.xml` (4,042 B)
- `Bryn%20Oh%27s%20Condos%20in%20Heaven%20sky.xml` (3,963 B)
- `Bryn%20Oh%27s%20Immersiva%20Grey%20Dust.xml` (4,097 B)
- `Bryn%20Oh%27s%20Mayfly.xml` (4,075 B)
- `Bryn%20Oh%27s%20Rusted%20Gears.xml` (3,688 B)
- `Bryn%20Oh%27s%20Virginia%20Alone.xml` (4,032 B)
- `CB%27s%20Rouge%201.xml` (4,135 B)
- `CB%27s%20Rouge%202.xml` (4,139 B)
- `CB%27s%20Rouge%203.xml` (4,148 B)
- `CB%27s%20Rouge%204.xml` (4,134 B)
- `CB%27s%20Rouge%205.xml` (4,069 B)
- `CB%27s%20Rouge%206.xml` (4,070 B)
- `CalWL.xml` (4,028 B)
- `Creepy%20Pyri.xml` (4,182 B)
- `Daytime%20shadows.xml` (4,058 B)
- `Doomed%20Spaceship.xml` (4,112 B)
- `Dusty.xml` (4,029 B)
- `FALLOUT%20%2D%2012AM.xml` (4,085 B)
- `FALLOUT%20%2D%2012PM.xml` (3,997 B)
- `FALLOUT%20%2D%203AM.xml` (4,125 B)
- `FALLOUT%20%2D%203PM.xml` (4,011 B)
- `FALLOUT%20%2D%205AM.xml` (4,082 B)
- `FALLOUT%20%2D%206AM.xml` (4,009 B)
- `FALLOUT%20%2D%206PM.xml` (4,010 B)
- `FALLOUT%20%2D%209AM.xml` (4,023 B)
- `FALLOUT%20%2D%209PM.xml` (4,012 B)
- `FSOriginal.xml` (4,077 B)
- `Fairy%20blue%20%28Paulina%29.xml` (4,111 B)
- `Fairy%20dark%20blue%20%28Paulina%29.xml` (4,103 B)
- `Fairy%20light%20pink%20%28Paulina%29.xml` (4,106 B)
- `Fairy%20warm%20pinks%20%28Paulina%29.xml` (4,065 B)
- `Free%27s%20Sunset.xml` (4,053 B)
- `Glowing%20Sea.xml` (4,040 B)
- `Gwen%27s%20Light.xml` (4,044 B)
- `Hyborian%20Age%20%2D%20Water%20E%20%2D%20evening.xml` (4,078 B)
- `Hyborian%20Age%20%2D%20Water%20E%20%2D%20morning.xml` (4,080 B)
- `Hyborian%20Age%20%2D%20Water%20F%20%2D%20noon.xml` (4,085 B)
- `Hyborian%20Dawn%20B%20%2D%20midnight.xml` (4,072 B)
- `Jean%27s%20Gothic%20Moon%2001.xml` (3,901 B)
- `Jean%27s%20Gothic%20Moon%2002.xml` (3,944 B)
- `Jean%27s%20Gothic%20Wasteland.xml` (3,972 B)
- `Jean%27s%20Gothic%20Winter.xml` (4,102 B)
- `Lunar%20Morning%207.xml` (3,714 B)
- `Magic%20Hour.xml` (4,042 B)
- `MorningGlory.xml` (4,041 B)
- `Nacon%27s%20Afternoon.xml` (3,940 B)
- `Nacon%27s%20Dawn.xml` (3,970 B)
- `Nacon%27s%20Day%20Mood.xml` (3,954 B)
- `Nacon%27s%20Day%20Mood2.xml` (3,956 B)
- `Nacon%27s%20Fog.xml` (3,988 B)
- `Nacon%27s%20Ghost.xml` (3,920 B)
- `Nacon%27s%20Morning.xml` (3,872 B)
- `Nacon%27s%20Morning2.xml` (4,302 B)
- `Nacon%27s%20Natural%2010am.xml` (3,978 B)
- `Nacon%27s%20Natural%204%3A30am.xml` (3,938 B)
- `Nacon%27s%20Natural%205am%20END.xml` (3,925 B)
- `Nacon%27s%20Natural%205am%20START.xml` (3,815 B)
- `Nacon%27s%20Natural%205am.xml` (3,831 B)
- `Nacon%27s%20Natural%207%3A30pm.xml` (3,939 B)
- `Nacon%27s%20Natural%208%3A30am.xml` (4,106 B)
- `Nacon%27s%20Natural%208am.xml` (3,978 B)
- `Nacon%27s%20Natural%20Dark%20START.xml` (3,797 B)
- `Nacon%27s%20Natural%20Midnight.xml` (3,925 B)
- `Nacon%27s%20Natural%20Night.xml` (3,939 B)
- `Nacon%27s%20Natural%20Noon.xml` (3,979 B)
- `Nacon%27s%20Natural%20Sunrise%3A%20%2DA.xml` (3,922 B)
- `Nacon%27s%20Natural%20Sunrise%3A%20A.xml` (3,883 B)
- `Nacon%27s%20Natural%20Sunrise%3A%20B.xml` (3,945 B)
- `Nacon%27s%20Natural%20Sunrise%3A%20C.xml` (3,990 B)
- `Nacon%27s%20Natural%20Sunrise%3A%20D.xml` (3,974 B)
- `Nacon%27s%20Natural%20Sunrise%3A%20E.xml` (3,990 B)
- `Nacon%27s%20Natural%20Sunrise%3A%20F.xml` (4,025 B)
- `Nacon%27s%20Natural%20Sunset%3A%20A.xml` (4,024 B)
- `Nacon%27s%20Natural%20Sunset%3A%20B.xml` (3,952 B)
- `Nacon%27s%20Natural%20Sunset%3A%20C.xml` (3,933 B)
- `Nacon%27s%20Natural%20Sunset%3A%20D.xml` (3,920 B)
- `Nacon%27s%20Nighty%20Fog.xml` (3,939 B)
- `Nacon%27s%20O%2D6am.xml` (4,164 B)
- `Nacon%27s%20Rise.xml` (3,995 B)
- `Nacon%27s%20Rosy%20Morning.xml` (3,990 B)
- `Nacon%27s%20Sweet%20Dawn.xml` (4,046 B)
- `Nacon%27s%20Sweet%20Morning.xml` (4,046 B)
- `Nacon%27s%20Test.xml` (4,046 B)
- `Nam%27s%20Beach%20Scene.xml` (3,921 B)
- `Nam%27s%20Optimal%20Skin%201.xml` (3,982 B)
- `Nam%27s%20Optimal%20Skin%202.xml` (4,052 B)
- `Nam%27s%20Optimal%20Skin%20and%20Prim.xml` (4,067 B)
- `Nam%27s%20Robots%20of%20Dawn.xml` (3,997 B)
- `Oceane%27s%20Body%20Designs.xml` (4,278 B)
- `Orac%20%2D%20Black%20fog%201.xml` (3,738 B)
- `Orac%20%2D%20Black%20fog%202.xml` (3,733 B)
- `Orac%20%2D%20Drawing%20blue.xml` (3,589 B)
- `Orac%20%2D%20Drawing%20extreme.xml` (3,576 B)
- `Orac%20%2D%20Drawing%20green.xml` (3,589 B)
- `Orac%20%2D%20Drawing%20red.xml` (3,589 B)
- `Orac%20%2D%20Drawing%20underground%20comic.xml` (3,771 B)
- `Orac%20%2D%20fog.xml` (3,748 B)
- `Orac%20%2D%20gray.xml` (3,594 B)
- `Orac%20%2D%20green.xml` (3,697 B)
- `Orange%20Incubus.xml` (3,979 B)
- `PaperSnow.xml` (3,972 B)
- `Phototools%2D%20Absinthe%20Light%20.xml` (4,257 B)
- `Phototools%2D%20Andi%20Light%20.xml` (3,757 B)
- `Phototools%2D%20Angles%20Light%2001.xml` (3,995 B)
- `Phototools%2D%20B%2FW%20Light%20%2008.xml` (3,736 B)
- `Phototools%2D%20B%2FW%20Light%2001.xml` (3,736 B)
- `Phototools%2D%20B%2FW%20Light%2002.xml` (3,753 B)
- `Phototools%2D%20B%2FW%20Light%2003.xml` (3,978 B)
- `Phototools%2D%20B%2FW%20Light%2004.xml` (3,856 B)
- `Phototools%2D%20B%2FW%20Light%2005.xml` (4,066 B)
- `Phototools%2D%20B%2FW%20Light%2006.xml` (4,018 B)
- `Phototools%2D%20B%2FW%20Light%2007.xml` (3,660 B)
- `Phototools%2D%20B%2FW%20Light%2008.xml` (3,748 B)
- `Phototools%2D%20B%2FW%20Light%2009.xml` (3,766 B)
- `Phototools%2D%20B%2FW%20Light%2010.xml` (3,709 B)
- `Phototools%2D%20B%2FW%20Light%2011.xml` (3,768 B)
- `Phototools%2D%20B%2FW%20Light%2012.xml` (3,786 B)
- `Phototools%2D%20B%2FW%20Light%2013.xml` (3,673 B)
- `Phototools%2D%20Boken%20Lines%20Light%2001.xml` (3,652 B)
- `Phototools%2D%20Breakwave%20Building%20Light.xml` (3,823 B)
- `Phototools%2D%20Build%20002%20Light.xml` (3,777 B)
- `Phototools%2D%20Build%20003%20Light.xml` (3,877 B)
- `Phototools%2D%20Build%20005%20Light.xml` (3,757 B)
- `Phototools%2D%20Build%20007%20Light.xml` (3,752 B)
- `Phototools%2D%20Cafe%20Light%2001.xml` (3,940 B)
- `Phototools%2D%20Calima%20Light%2001.xml` (3,915 B)
- `Phototools%2D%20Charolotte%20Light.xml` (3,772 B)
- `Phototools%2D%20Cloud%20Credit%20Light.xml` (3,668 B)
- `Phototools%2D%20Cloud%20Light%2001.xml` (3,687 B)
- `Phototools%2D%20Dead%20End%20Sky%2001.xml` (3,895 B)
- `Phototools%2D%20Dorm%20Light%2001.xml` (3,982 B)
- `Phototools%2D%20Dream%20Book%20Light%2001.xml` (4,055 B)
- `Phototools%2D%20Dream%20Book%20Light%2002.xml` (4,073 B)
- `Phototools%2D%20Dream%20Book%20Light%2003.xml` (4,070 B)
- `Phototools%2D%20Dream%20Book%20Light%2004.xml` (3,970 B)
- `Phototools%2D%20Epi%20Vintage%20Light.xml` (3,972 B)
- `Phototools%2D%20Fashion%20Path%20Light%2001.xml` (3,839 B)
- `Phototools%2D%20Got%20It%20Light%20.xml` (3,924 B)
- `Phototools%2D%20Horizon%20Building%20Light%2001.xml` (3,978 B)
- `Phototools%2D%20Horizon%20Building%20Light%2002.xml` (3,823 B)
- `Phototools%2D%20Horizon%20Building%20Light.xml` (3,823 B)
- `Phototools%2D%20Hospital%20Light%2001.xml` (3,806 B)
- `Phototools%2D%20Hufflepuff%20Light%2001.xml` (4,090 B)
- `Phototools%2D%20Hufflepuff%20Light%2002.xml` (3,966 B)
- `Phototools%2D%20Hufflepuff%20Light%2003.xml` (4,106 B)
- `Phototools%2D%20Jessica%20Light%2001.xml` (3,962 B)
- `Phototools%2D%20Jessica%20Light%2002.xml` (3,962 B)
- `Phototools%2D%20Jessica%20Light%2003.xml` (3,885 B)
- `Phototools%2D%20Jessica%20Light%2004.xml` (3,903 B)
- `Phototools%2D%20Jim%20Light%2001.xml` (3,876 B)
- `Phototools%2D%20Jim%20Light%2002.xml` (3,840 B)
- `Phototools%2D%20July%20Light%2001.xml` (4,157 B)
- `Phototools%2D%20July%20Light%2002.xml` (3,976 B)
- `Phototools%2D%20July%20Light%2003.xml` (3,924 B)
- `Phototools%2D%20Landar%20Light%2001.xml` (3,764 B)
- `Phototools%2D%20Lo%20%20Gun%20Light.xml` (4,048 B)
- `Phototools%2D%20Lo%20Light%2001.xml` (3,779 B)
- `Phototools%2D%20Lo%20Light%2002.xml` (3,693 B)
- `Phototools%2D%20Lo%20Light%2003.xml` (4,103 B)
- `Phototools%2D%20Lo%20Light%2004.xml` (4,095 B)
- `Phototools%2D%20Lo%20Light%2005.xml` (3,798 B)
- `Phototools%2D%20Lo%20Moves%20Light.xml` (3,740 B)
- `Phototools%2D%20Lo%20Music%20Light%2001.xml` (3,865 B)
- `Phototools%2D%20Mavi%20Light%2001.xml` (4,046 B)
- `Phototools%2D%20Mavi%20Light%2002.xml` (4,018 B)
- `Phototools%2D%20Mavi%20Light%2003.xml` (4,046 B)
- `Phototools%2D%20Me%20Love%20Light.xml` (3,773 B)
- `Phototools%2D%20Me%20Mine%20Light.xml` (4,187 B)
- `Phototools%2D%20Meni%20Light%2001.xml` (4,065 B)
- `Phototools%2D%20Miaa%20light%2001.xml` (3,769 B)
- `Phototools%2D%20Moon%20Light%2001.xml` (4,097 B)
- `Phototools%2D%20Moon%20Light%2002.xml` (3,907 B)
- `Phototools%2D%20Moon%20Light%2003.xml` (4,072 B)
- `Phototools%2D%20Moon%20Light%2004.xml` (4,048 B)
- `Phototools%2D%20Moon%20Light%2005.xml` (4,065 B)
- `Phototools%2D%20Moon%20Light%2006.xml` (3,976 B)
- `Phototools%2D%20Moon%20Light%2007.xml` (4,071 B)
- `Phototools%2D%20Moon%20Light%2008.xml` (4,070 B)
- `Phototools%2D%20Never%20Night%20Light.xml` (4,050 B)
- `Phototools%2D%20No%20Light.xml` (4,013 B)
- `Phototools%2D%20Owlery%20Light.xml` (4,105 B)
- `Phototools%2D%20Puppy%20Light.xml` (3,898 B)
- `Phototools%2D%20Queen%20Light%2001.xml` (4,064 B)
- `Phototools%2D%20Quidditch%20Light.xml` (3,941 B)
- `Phototools%2D%20Save%20Me%20Light%20.xml` (3,672 B)
- `Phototools%2D%20Shadow%20Testing%20Light.xml` (3,987 B)
- `Phototools%2D%20Still%20Life.xml` (4,018 B)
- `Phototools%2D%20Thalia%20Light%2001.xml` (3,731 B)
- `Phototools%2D%20Thalia%20Light%2002.xml` (4,109 B)
- `Phototools%2D%20Trilogy%20Rain%2001.xml` (3,945 B)
- `Phototools%2D%20Trilogy%20Rain%2002.xml` (3,909 B)
- `Phototools%2D%20White%20Fire%20Sky%2001.xml` (3,839 B)
- `Phototools%2D%20White%20Fire%20Sky%2002.xml` (3,840 B)
- `Phototools%2D%20White%20Fire%20Sky%2003.xml` (3,840 B)
- `Phototools%2D%20Yellow%20Stars%2001.xml` (3,867 B)
- `Phototools%2D%20rara%20Fall%20Home%20Light.xml` (3,992 B)
- `Places%20Abracadabra.xml` (4,004 B)
- `Places%20Abracadabra2.xml` (4,039 B)
- `Places%20Abracadabra3.xml` (4,038 B)
- `Places%20Annamaria.xml` (4,091 B)
- `Places%20Astryls%20Wild.xml` (4,037 B)
- `Places%20Babbage.xml` (3,980 B)
- `Places%20Beach%20Cay%20Surreal.xml` (4,040 B)
- `Places%20Beach%20Cay.xml` (4,032 B)
- `Places%20Bentham.xml` (4,027 B)
- `Places%20Cornfield.xml` (4,051 B)
- `Places%20Cromac.xml` (4,240 B)
- `Places%20Crucible.xml` (4,076 B)
- `Places%20District8.xml` (4,022 B)
- `Places%20Duskwood.xml` (4,108 B)
- `Places%20Eridu.xml` (3,969 B)
- `Places%20Erie.xml` (4,015 B)
- `Places%20Eugene%202.xml` (4,069 B)
- `Places%20Eugene%20BL.xml` (4,051 B)
- `Places%20Greed.xml` (4,035 B)
- `Places%20Greed2.xml` (4,034 B)
- `Places%20Imagine.xml` (4,043 B)
- `Places%20Kingsport.xml` (3,943 B)
- `Places%20Kunming.xml` (3,973 B)
- `Places%20Las%20Legunas.xml` (4,067 B)
- `Places%20Legacies.xml` (4,060 B)
- `Places%20Midian.xml` (4,070 B)
- `Places%20Mother.xml` (4,105 B)
- `Places%20Old%20New%20York.xml` (4,077 B)
- `Places%20Paris%202.xml` (4,101 B)
- `Places%20Paris.xml` (4,100 B)
- `Places%20Pathfinder.xml` (4,059 B)
- `Places%20Sand.xml` (3,994 B)
- `Places%20Terre%20Des%20Mortes.xml` (4,041 B)
- `Places%20Urbania.xml` (4,033 B)
- `Places%20Wiccan.xml` (4,123 B)
- `Places%20alirium.xml` (4,089 B)
- `Places%2DEmbryo.xml` (4,027 B)
- `Raymond%27s%20Bright%20%26%20Hazy%20Day.xml` (4,377 B)
- `Raymond%27s%20Brighter%20Day.xml` (4,506 B)
- `Raymond%27s%20Day.xml` (4,429 B)
- `Raymond%27s%20Night.xml` (4,275 B)
- `Raymond%27s%20Sunrise.xml` (4,403 B)
- `Riverrock%20Night.xml` (4,066 B)
- `Shadow1.xml` (3,977 B)
- `Shadows%20Bright%20day.xml` (4,024 B)
- `Silent%20Hill.xml` (3,884 B)
- `StrawberrySingh%2Ecom%20%2D%20Closeups.xml` (3,876 B)
- `StrawberrySingh%2Ecom%20%2D%20Headshots.xml` (4,023 B)
- `Studio%20Light.xml` (3,970 B)
- `Sunset%20Pink%20%28Paulina%29.xml` (4,094 B)
- `Surreal%20%2D%20Brazil%20%28Paulina%29.xml` (4,064 B)
- `Surreal%20%2D%20Fire%20%28Paulina%29.xml` (3,979 B)
- `Surreal%20%2D%20Flirt%20%28Paulina%29.xml` (3,908 B)
- `Surreal%20%2D%20Night%20%28Paulina%29.xml` (3,923 B)
- `Surreal%20%2D%20Summer%20%28Paulina%29.xml` (4,050 B)
- `Synthwave.xml` (4,059 B)
- `The%20Shed.xml` (3,919 B)
- `Torie%20Senne%20WL%20settings.xml` (4,002 B)
- `Tron%20Legacy%20clean.xml` (4,088 B)
- `Tron%20Legacy%20extreme.xml` (4,090 B)
- `Tron%20Legacy%20hard.xml` (4,091 B)
- `Tron%20Legacy%20soft.xml` (4,091 B)
- `Wastes%2012pm.xml` (4,025 B)
- `Wastes%20Midnight.xml` (3,935 B)
- `Wastes%20Morning.xml` (3,991 B)
- `blackskymoon.xml` (4,071 B)
- `greyskymoon.xml` (4,207 B)
- `names.txt` (1,440 B)
- `neutral.xml` (4,371 B)
- `pinkpurple.xml` (4,260 B)
- `predawnmoon.xml` (4,383 B)
- `purplebluenight.xml` (4,362 B)
- `wastelands.xml` (3,962 B)

### `windlight/skies/` common (sha256 diff)

| # | file | BD sha | AY sha | diff |
|---|---|---|---|---|
| 1 | `A%2D12AM.xml` | `d7f329175210f2e1` | `d7f329175210f2e1` | no |
| 2 | `A%2D12PM.xml` | `031ef816b5f42c22` | `031ef816b5f42c22` | no |
| 3 | `A%2D3AM.xml` | `cd3086a6e29f2c9f` | `cd3086a6e29f2c9f` | no |
| 4 | `A%2D3PM.xml` | `1e5e13e5db11743b` | `1e5e13e5db11743b` | no |
| 5 | `A%2D6AM.xml` | `2189d3a51b860ada` | `2189d3a51b860ada` | no |
| 6 | `A%2D6PM.xml` | `22546900f9141c3b` | `22546900f9141c3b` | no |
| 7 | `A%2D9AM.xml` | `057e097f92ba48aa` | `057e097f92ba48aa` | no |
| 8 | `A%2D9PM.xml` | `a17359d6076b82c4` | `a17359d6076b82c4` | no |
| 9 | `AnaLutetia%20%2D%20AvatarOpt.xml` | `8a896349fe873e6f` | `8a896349fe873e6f` | no |
| 10 | `AnaLutetia%20%2D%20AvatarOpt2%20whiter.xml` | `07055694aa6b8252` | `07055694aa6b8252` | no |
| 11 | `AnaLutetia%20%2D%20Studio%20Light.xml` | `3a7472c8d1f8d6f9` | `3a7472c8d1f8d6f9` | no |
| 12 | `AnaLutetia.xml` | `225a4a3513f18b82` | `225a4a3513f18b82` | no |
| 13 | `Barcelona.xml` | `620ffe8774189589` | `620ffe8774189589` | no |
| 14 | `Blizzard.xml` | `1c9d98c281806087` | `1c9d98c281806087` | no |
| 15 | `Blue%20Midday.xml` | `2131f8b95df370d0` | `2131f8b95df370d0` | no |
| 16 | `Bristol.xml` | `6241b4a6a9eac199` | `6241b4a6a9eac199` | no |
| 17 | `Coastal%20Afternoon.xml` | `bfe192faf34bf487` | `bfe192faf34bf487` | no |
| 18 | `Coastal%20Sunset.xml` | `d60f2529efc1b23b` | `d60f2529efc1b23b` | no |
| 19 | `Default.xml` | `986194f531469ba5` | `986194f531469ba5` | no |
| 20 | `Desert%20Sunset.xml` | `4ea569c0f25f6176` | `4ea569c0f25f6176` | no |
| 21 | `Fine%20Day.xml` | `11c8b30fba906e63` | `11c8b30fba906e63` | no |
| 22 | `Fluffy%20Big%20Clouds.xml` | `3164c6d2e1f1dbf5` | `3164c6d2e1f1dbf5` | no |
| 23 | `Foggy.xml` | `f01ec09a8f86c8d3` | `f01ec09a8f86c8d3` | no |
| 24 | `Funky%20Funky%20Funky.xml` | `ef33819916be858d` | `ef33819916be858d` | no |
| 25 | `Funky%20Funky.xml` | `42ed2113d1babc8c` | `42ed2113d1babc8c` | no |
| 26 | `Gelatto.xml` | `08dc393ac06e47ef` | `08dc393ac06e47ef` | no |
| 27 | `Ghost.xml` | `c355667429b1346f` | `c355667429b1346f` | no |
| 28 | `Incongruent%20Truths.xml` | `dbc13263fb3c0379` | `dbc13263fb3c0379` | no |
| 29 | `London2026.xml` | `804329a3d8b2db27` | `804329a3d8b2db27` | no |
| 30 | `London2050.xml` | `4e42e226ce9fc656` | `4e42e226ce9fc656` | no |
| 31 | `Maroon.xml` | `95fda0a7a3d64312` | `95fda0a7a3d64312` | no |
| 32 | `Midday%201.xml` | `986194f531469ba5` | `986194f531469ba5` | no |
| 33 | `Midday%202.xml` | `7e6be74dc21ffb73` | `7e6be74dc21ffb73` | no |
| 34 | `Midday%203.xml` | `7ee001ae067e4e46` | `7ee001ae067e4e46` | no |
| 35 | `Midday%204.xml` | `2a871aa381ef8826` | `2a871aa381ef8826` | no |
| 36 | `Midday.xml` | `031ef816b5f42c22` | `031ef816b5f42c22` | no |
| 37 | `Midnight.xml` | `d7f329175210f2e1` | `d7f329175210f2e1` | no |
| 38 | `Night.xml` | `dc85f73d21de9910` | `dc85f73d21de9910` | no |
| 39 | `Pirate.xml` | `e62e89170a99081f` | `e62e89170a99081f` | no |
| 40 | `Purple.xml` | `d087e95c956c6af8` | `d087e95c956c6af8` | no |
| 41 | `Rot.xml` | `02e27107ec829dba` | `02e27107ec829dba` | no |
| 42 | `Sailor%27s%20Delight.xml` | `915fac5db67a2b5e` | `915fac5db67a2b5e` | no |
| 43 | `ShadowSet.xml` | `a2c439d4f828137a` | `a2c439d4f828137a` | no |
| 44 | `Sheer%20Surreality.xml` | `9eb8239abcb11983` | `9eb8239abcb11983` | no |
| 45 | `ShepherdsDelight.xml` | `d1558d377aa88506` | `d1558d377aa88506` | no |
| 46 | `Sunrise.xml` | `2189d3a51b860ada` | `2189d3a51b860ada` | no |
| 47 | `Sunset.xml` | `22546900f9141c3b` | `22546900f9141c3b` | no |
| 48 | `Verdigris.xml` | `c1699d4a7ed95831` | `c1699d4a7ed95831` | no |

**windlight/skies common 中身 diff 集計**: 0 / 48 件で差分あり

## 5.x `windlight/water/`

- BD files: **11**, AY files: **58**
- common: **7**, BD-only: **4**, AY-only: **51**

### `windlight/water/` BD-only

- `%5BML%5D%20Caribbean%20Water.xml` (1,364 B)
- `Crazy%20Water.xml` (1,177 B)
- `High%20Definition.xml` (1,327 B)
- `Snake.xml` (1,260 B)

### `windlight/water/` AY-only

- `%5BNB%5D%20Hidden%20Depths.xml` (1,292 B)
- `%5BNB%5D%20Sparkling%20Depths.xml` (1,329 B)
- `%5BNB%5D%20Turbid%20Reflections.xml` (1,357 B)
- `%5BTOR%5D%20Arrakissed%20variation.xml` (1,228 B)
- `%5BTOR%5D%20Atomist%202.xml` (1,166 B)
- `%5BTOR%5D%20Atomist.xml` (1,185 B)
- `%5BTOR%5D%20Bayouette.xml` (1,230 B)
- `%5BTOR%5D%20Chivandria%20clair.xml` (1,086 B)
- `%5BTOR%5D%20Claira.xml` (1,251 B)
- `%5BTOR%5D%20Coral%20reef.xml` (1,172 B)
- `%5BTOR%5D%20Crosshatched%20plans.xml` (1,195 B)
- `%5BTOR%5D%20Eyeballin%27.xml` (1,124 B)
- `%5BTOR%5D%20Fantastuck.xml` (1,208 B)
- `%5BTOR%5D%20Featuresque.xml` (1,198 B)
- `%5BTOR%5D%20Freak%2DA%2Ddermia.xml` (1,032 B)
- `%5BTOR%5D%20Glaznost.xml` (1,215 B)
- `%5BTOR%5D%20Grantamount.xml` (1,113 B)
- `%5BTOR%5D%20Hpmod.xml` (1,235 B)
- `%5BTOR%5D%20Ice%2Dlike.xml` (1,075 B)
- `%5BTOR%5D%20Impure.xml` (1,134 B)
- `%5BTOR%5D%20Linear%20puddles.xml` (1,175 B)
- `%5BTOR%5D%20Listeryne.xml` (1,177 B)
- `%5BTOR%5D%20Liz%20Taylor.xml` (1,220 B)
- `%5BTOR%5D%20Maldives.xml` (1,204 B)
- `%5BTOR%5D%20Meridian%20verde.xml` (1,197 B)
- `%5BTOR%5D%20Negative%20ultraspace.xml` (1,114 B)
- `%5BTOR%5D%20Placida%20brite.xml` (1,136 B)
- `%5BTOR%5D%20Pretty%20placid.xml` (1,232 B)
- `%5BTOR%5D%20Raw%20sewage.xml` (1,144 B)
- `%5BTOR%5D%20Showing%20age.xml` (1,193 B)
- `%5BTOR%5D%20Soldier%27s%20legacy.xml` (1,209 B)
- `%5BTOR%5D%20Subtleties.xml` (1,247 B)
- `%5BTOR%5D%20Trandshan.xml` (1,125 B)
- `%5BTOR%5D%20Watermelon%20juice.xml` (1,140 B)
- `%5BTOR%5D%20Waterslides.xml` (1,107 B)
- `%5BTOR%5D%20What%27s%20on%20TV.xml` (1,104 B)
- `%5BTOR%5D%20Wickedly.xml` (1,141 B)
- `%5BTOR%5D%20You%27re%20in%20luck.xml` (1,196 B)
- `%5BTOR%5D%20Yung%20buk.xml` (1,206 B)
- `Blackwater.xml` (1,181 B)
- `Lassies%20Clearwater.xml` (1,175 B)
- `Nacon%27s%20Lake%20Water.xml` (1,201 B)
- `Nacon%27s%20Water.xml` (1,285 B)
- `Nam%27s%20beach%20Scene.xml` (1,285 B)
- `Phototools%2D%20Black%20Default%20.xml` (1,165 B)
- `Phototools%2D%20Breakwave%20Building%20Water.xml` (1,182 B)
- `Phototools%2D%20Chandra%20Sea.xml` (1,224 B)
- `Phototools%2D%20Gallery%20Water%2001.xml` (1,183 B)
- `Phototools%2D%20Ship%20Light.xml` (1,212 B)
- `Raymond%27s%20Water%20At%20Night.xml` (1,277 B)
- `Raymond%27s%20Water.xml` (1,287 B)

### `windlight/water/` common (sha256 diff)

| # | file | BD sha | AY sha | diff |
|---|---|---|---|---|
| 1 | `Default.xml` | `bd16c1e34dfcd702` | `bd16c1e34dfcd702` | no |
| 2 | `Glassy.xml` | `edfe290add80201a` | `edfe290add80201a` | no |
| 3 | `Murky.xml` | `635aaf6e97a042dd` | `635aaf6e97a042dd` | no |
| 4 | `Pond.xml` | `9e44a6ea2d8d282f` | `9e44a6ea2d8d282f` | no |
| 5 | `SNAKE%21%21%21.xml` | `c38e0ba92c703fd1` | `c38e0ba92c703fd1` | no |
| 6 | `Second%20Plague.xml` | `1d8bfcd5a792a076` | `1d8bfcd5a792a076` | no |
| 7 | `Valdez.xml` | `3f7423f727f6dbaa` | `3f7423f727f6dbaa` | no |

**windlight/water common 中身 diff 集計**: 0 / 7 件で差分あり

## 5.x `windlight/days/`

- BD files: **10**, AY files: **18**
- common: **6**, BD-only: **4**, AY-only: **12**

### `windlight/days/` BD-only

- `%5BTarnix%5D%20UltraBright.xml` (576 B)
- `Fantasy%20Overworld.xml` (1,153 B)
- `Penny%20Day%201C.xml` (1,166 B)
- `Weird%2DO.xml` (1,573 B)

### `windlight/days/` AY-only

- `%28SS%29%20Atmos%206.xml` (2,166 B)
- `%28SS%29%20Atmospheric%20Daycycle%20Clear.xml` (1,865 B)
- `%28SS%29%20Atmospheric%20Daycycle%20Cloudy.xml` (1,924 B)
- `%28SS%29%20Atmospheric%20Daycycle%20Cloudy2.xml` (1,939 B)
- `%5BNB%5D%20Aftermath.xml` (846 B)
- `%5BNB%5D%20P-Haze.xml` (957 B)
- `%5BNB%5D%20Sepia.xml` (950 B)
- `%5BNB%5D-MistyDay.xml` (1,356 B)
- `Anime%20Daze.xml` (1,341 B)
- `FALLOUT%20SL%201.0.xml` (1,112 B)
- `Hyborian%20Coast.xml` (573 B)
- `Weird-O.xml` (1,573 B)

### `windlight/days/` common (sha256 diff)

| # | file | BD sha | AY sha | diff |
|---|---|---|---|---|
| 1 | `Colder%20Tones.xml` | `6ff0d69c32cee81a` | `6ff0d69c32cee81a` | no |
| 2 | `Default.xml` | `2a512017fb4df982` | `2a512017fb4df982` | no |
| 3 | `Dynamic%20Richness.xml` | `6039b34531615a18` | `6039b34531615a18` | no |
| 4 | `Pirate%27s%20Dream.xml` | `ff21ee275ef16dd5` | `ff21ee275ef16dd5` | no |
| 5 | `Psycho%20Strobe%21.xml` | `e8e30b2cad352851` | `e8e30b2cad352851` | no |
| 6 | `Tropicalia.xml` | `370650afc678440f` | `370650afc678440f` | no |

**windlight/days common 中身 diff 集計**: 0 / 6 件で差分あり

## 5.x `filters/`

- BD files: **11**, AY files: **30**
- common: **11**, BD-only: **0**, AY-only: **19**

### `filters/` AY-only

- `Antique.xml` (1,475 B)
- `Badtrip.xml` (873 B)
- `Blownhighlights.xml` (611 B)
- `Brighten.xml` (227 B)
- `Cartoon.xml` (554 B)
- `Darken.xml` (225 B)
- `Edges.xml` (553 B)
- `Focus.xml` (999 B)
- `Heatwave.xml` (976 B)
- `Julesverne.xml` (462 B)
- `Lightleak.xml` (2,100 B)
- `Linearize.xml` (228 B)
- `Negative.xml` (229 B)
- `Overcast.xml` (574 B)
- `Posterize.xml` (856 B)
- `Rotatecolors180.xml` (140 B)
- `Sharpen.xml` (110 B)
- `Softfocus.xml` (107 B)
- `Thematrix.xml` (1,035 B)

### `filters/` common (sha256 diff)

| # | file | BD sha | AY sha | diff |
|---|---|---|---|---|
| 1 | `Autocontrast.xml` | `e442b111ab06ab19` | `e442b111ab06ab19` | no |
| 2 | `BlackAndWhite.xml` | `f2583e59ceef535e` | `f2583e59ceef535e` | no |
| 3 | `Colors1970.xml` | `d541c997fb2b9098` | `d541c997fb2b9098` | no |
| 4 | `Intense.xml` | `7e4a814da6954af9` | `7e4a814da6954af9` | no |
| 5 | `LensFlare.xml` | `a1c5166e94dc43d8` | `a1c5166e94dc43d8` | no |
| 6 | `Miniature.xml` | `a837d007923f1f3c` | `a837d007923f1f3c` | no |
| 7 | `Newspaper.xml` | `57a2b21e75e266df` | `57a2b21e75e266df` | no |
| 8 | `Sepia.xml` | `5aa5bd261e230ca2` | `5aa5bd261e230ca2` | no |
| 9 | `Spotlight.xml` | `f1e9ae0200c5d065` | `f1e9ae0200c5d065` | no |
| 10 | `Toycamera.xml` | `fd5b0c29637aaf27` | `fd5b0c29637aaf27` | no |
| 11 | `Video.xml` | `f1e113ac7f879bc5` | `f1e113ac7f879bc5` | no |

**filters common 中身 diff 集計**: 0 / 11 件で差分あり

## 5.x `camera/`

- BD files: **8**, AY files: **4**
- common: **0**, BD-only: **8**, AY-only: **4**

### `camera/` BD-only

- `Front%20View.xml` (194 B)
- `Group%20View.xml` (197 B)
- `Left%20Shoulder%20View.xml` (199 B)
- `Mouselook.xml` (186 B)
- `RLVa%20View.xml` (231 B)
- `Rear%20View.xml` (194 B)
- `Right%20Shoulder%20View.xml` (201 B)
- `Top%20View.xml` (194 B)

### `camera/` AY-only

- `Front.xml` (3,546 B)
- `Rear.xml` (3,542 B)
- `Side.xml` (3,545 B)
- `TPP.xml` (3,556 B)


---

## §6 次工程への引き継ぎ

本 inventory が確定したことで、次の判断が可能になる:

### §6.1 Phase 1 — 既存 r30 commit (P2-P5) の retract 判定

各 commit が BD-only / common diff のどれを borrow しているかを inventory に照合し、

- BD と完全一致まで持って行ければ「移植進捗の一部」として残す
- 中途半端な状態 (= BD と差分残り) のまま積み上がっているなら retract → 完全移植 spec に組み込み直す

Phase 1 は本 inventory に対する mechanical な点検。推論で「これは残せる」と判定しない。

### §6.2 Phase 2 — 完全移植 spec 起こし

bucket 別に移植 task を切る:

- BD-only ファイル → AYAstorm への 1:1 ファイル新規追加 + bind
- common diff ファイル → AYAstorm 側を BD 中身に reset、または Cinematic 経路で BD 中身を bind
- AY-only ファイル / cvar / UI → Cinematic では disable / 不可視化 (純 BD パス保持)

### §6.3 Phase 3 — 実装 (BD パイプラインを Cinematic として走らせる)

完全移植が動いた段階で AYA self-judgment を取り、`memory/feedback_bd_full_port_only.md` の scope (BD parity 達成まで) を抜けて P6+ に移行する。

### §6.4 本 inventory の更新ポリシー

- スコープ外 file が描画に影響することが分かった時点で bucket 追加 → 本 doc に追補 commit
- BD 側 reference commit を更新する場合 (現在 `995a1354d8`)、全 bucket 再生成し本 doc を上書き
- 「判定」を含むコメントは本 doc に書かない、Phase 1+ の別 doc で扱う

---

## §A 付録: 生成スクリプト

5 bucket とも `/tmp/bd_inventory_*.py` に置いた。各スクリプトはこの commit には含めず (= 一回限りの生成器)、本 inventory doc に表として固定する方針。再生成が必要になった時点で同等スクリプトを書き直す。

スクリプト本体は本 doc 確定後の参照のため `docs/specs/ayastorm-r30-bd-full-port-inventory-scripts/` に固定する想定 (Phase 1 着手時点で判断、本 inventory 自体には含めない)。

---

**End of Phase 0 Inventory.**
