# AYAstorm r30 P3 — リリース告知

**r30 P3 は Cinematic mode に volumetric lighting (godrays / 薄明光線) を導入するリリース** — 撮影描画章 (r30 chapter) の表現力強化として、建物や樹木のエッジを太陽光が抜ける時の放射状の光条 / 空中の靄を本実装します。

実装トレース・改修ポイント・受入観測 (4 段階 canary 解析含む) ・上流参照行は永続資料 (`docs/specs/ayastorm-r30-p3-volumetric-lighting-bd-trace.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r30 P3 — Volumetric Lighting (Godrays)

### r30 P3 の柱: 太陽光が空気中で散乱する画を入れる

r30 章 (撮影描画) は P2 で per-object motion blur と SMAA T2x を入れて Cinematic mode の描画基盤を立てました。P3 はその Cinematic mode に **volumetric lighting (godrays)** を本実装します。

godrays (神の光・薄明光線・光芒) は、建物の屋根や樹木の枝で太陽光が一部遮られた時、空気中の塵やもやで散乱して見える放射状の光の筋です。AYAstorm r30 の核心テーゼ「写真を撮るに値する空気と空間」(`docs/specs/ayastorm-r30-cinematic-chapter.md`) を、光の物質感として可視化する一段です。

通常 (Standard) / リアリズム (AYAstorm View) モードでは hook 自体が dispatch されないため、追加コストはゼロです。

### 仕組み

post-process pass として `renderFinalize()` 内、`generateGlow()` と `combineGlow()` の間に挿入されます:

```
deferredScreen (lighting 後の最終色)
  → renderVolumetric(src, dst)
       ├─ Cinematic mode + RenderVolumetricLighting で gate
       ├─ volumetricLightF.glsl で sun 方向に shadow march (16 sample default)
       ├─ depth weighting (pow(depth, 100)) で空 pixel のみ寄与
       ├─ haze_weight + sunlight_color で大気散乱として合成
       └─ GODRAYS_FADE permutation (任意 OFF 可) で太陽が画面前方の時のみ shaftify
  → ping-pong swap → combineGlow に渡す
```

shadow sampling は AYAstorm/Firestorm 標準の `sampleDirectionalShadow` (`shadowUtil.glsl`) を再利用、`sun_dir` を surrogate normal として PCF bias を効かせます (中空 sample 点に法線が無いので、`class1/deferred/godraysF.glsl` と同じトリック)。

### 設定

**通常運用での操作は不要。** Cinematic mode (`AYAVisualRealismEnabled = 2`) を起動すれば volumetric lighting が自動で有効化されます。チューニング用の cvar は以下:

| Cvar | 既定値 | 用途 |
|---|---|---|
| `RenderVolumetricLighting` | `1` (ON) | godrays composite 全体の ON/OFF。Cinematic 起動時のみ有効 |
| `RenderVolumetricLightingResolution` | `16` | 太陽方向の shadow march サンプル数。上げる ほどスムーズ、GPU コスト線形増 |
| `RenderVolumetricLightingMultiplier` | `50.0` | 光条の強度。`50` で「現実世界に近い」自然な薄明光線。`100` でしっかり主張、`200` で PV/Cinematic 寄り。BD 既定 `1.0` は AYAstorm の ACES tone mapping + HDR scene buffer で不可視 (§8 受入観測 §8.2 参照) |
| `RenderVolumetricLightingFalloffMultiplier` | `1.0` | 距離減衰の強さ。上げると godrays が遠景で速くフェード |
| `RenderVolumetricLightingDirectional` | `1` (ON) | 太陽が画面前方にある時のみ shaftify が残るゲート (GODRAYS_FADE permutation)。OFF にすると太陽が画面外でも全画面 godrays が乗る (= 空中に光線が浮き建物が godrays に飲まれる、不自然)。permutation 変更なので **AYAstorm 再起動が必要** |

### 見え方の目安

- **時間帯**: 昼〜午後早め (太陽が高い時間) が一番出やすい。SL の sunlight clamp で夕方は sunlight_color が下がり godrays も dim になる (BD の物理近似の限界)
- **構図**: 太陽を画面のほぼ中央に、太陽の手前に建物のエッジ / 樹木 / 細枝 などのシルエットを置く。エッジで太陽が半分隠れる構図が一番強く出る
- **モード**: Cinematic mode 必須。Standard / AYAstorm View では一切走らない

### 移行ノート

- 撮影モード (Cinematic) で volumetric lighting が新たに使えます
- 通常 / リアリズムモードには一切影響なし (hook が dispatch されない)
- Cinematic ↔ 他モードの切替は **viewer 再起動が必要** (r30 P1 で確定した設計)
- `RenderVolumetricLightingDirectional` の切替も **viewer 再起動が必要** (shader permutation 変更)
- 既存の Linden / Firestorm 標準実装 (`gDeferredGodraysProgram` などの既存 godrays) とは独立した shader プログラム (`gVolumetricLightProgram`) を使うため、上流との設定衝突は発生しません

### 既知の制約

- **default Multiplier の BD 乖離**: BD 既定 `1.0` で出ていた表現が AYAstorm では `50.0` 相当になります。AYAstorm の ACES tone mapping + HDR scene buffer が加算分を BD の sRGB 直書きより強く圧縮するためと推測 (BD 側オリジナル挙動の実機検証は未実施)
- **GODRAYS_FADE 窓の狭さ**: 既定 (`Directional=1`) では太陽が画面中心から ~30° 以内に入っていないと shaftify が消えます。広範囲に godrays を出したい場合 `Directional=0` で再起動
- **空 pixel のみ寄与**: `depth *= pow(depth, 100.0)` の重み付けで地面や近景の depth は 0 に落とされるため、godrays は主に空 pixel (sky 含む大気) に乗ります。地面に godrays の影が伸びる効果は出ません (将来 phase で検討の余地あり)
- **macOS / Windows 実機検証**: AYAstorm 側 Linux ビルドで動作確認 PASS、Mac/Win の Release ビルドは tag 切り出し時に実施

### 実装概要

- shader (`indra/newview/app_settings/shaders/class3/deferred/`):
  - `volumetricLightF.glsl` (Black Dragon Viewer から借用、AYAstorm 側で shadow helper を `sampleDirectionalShadow` に差し替え + `HAS_SUN_SHADOW` permutation gate 追加)
  - vertex shader は `deferred/postDeferredNoTCV.glsl` 既存を共用
- C++ pipeline (`indra/newview/`):
  - `pipeline.{cpp,h}`: `renderVolumetric()` 新規 (Cinematic gate + ping-pong)、`renderFinalize()` 内 hook 追加
  - `llviewershadermgr.{cpp,h}`: `gVolumetricLightProgram` extern + register + `mShaderList.push_back()` で atmosphere uniform auto-bind
  - `lldrawpoolalpha.cpp`: forward pass alpha の depth-write gate を Cinematic + `RenderVolumetricLighting` でも有効化 (godrays 用の depth 整備)
  - `llshadermgr.{cpp,h}`: `GODRAY_RES` / `GODRAY_MULTIPLIER` / `FALLOFF_MULTIPLIER` uniform 文字列追加
- `indra/newview/app_settings/settings.xml`: P3 cvar 5 件 追加
- Cinematic 起動時のみ hook dispatch → 通常 / リアリズムモードはコストゼロ

### Credits

volumetric lighting (godrays) の実装パターンは [Black Dragon Viewer](https://github.com/NiranV/Black-Dragon-Viewer) (NiranV Dean) 由来です。AYAstorm では BD `995a1354d8` (2026-04-19) を上流参照点として `volumetricLightF.glsl` をライセンス継承 (LGPL-2.1-only) で取り込み、AYAstorm 側で以下の改修を加えました:

- shadow sampling を BD-only `nonpcfShadowAtPos` から AYAstorm/Firestorm 標準 `sampleDirectionalShadow` に差し替え (`shadowUtil.glsl`、`sun_dir` を surrogate normal 化)
- `HAS_SUN_SHADOW` permutation gate で godrays 全計算を `RenderShadowDetail > 0` 時のみ走らせる (sun shadow OFF 時の自動 passthrough)
- `gVolumetricLightProgram` を `mShaderList` 登録で atmosphere uniform (`sunlight_color` / `sun_dir` / `blue_density` / `haze_density`) auto-bind
- Cinematic mode gate (`AYAVisualRealismEnabled == 2` でのみ hook dispatch)
- `mPostPingMap` / `mPostPongMap` ping-pong による GPU read-after-write 安全化
- default `RenderVolumetricLightingMultiplier` を BD `1.0` から `50.0` に retune (4 段階 canary 解析 + AYA 主観評価で確定、§8.2 参照)

### ドキュメント

- r30 P3 full trace / file:line 改修マップ / step 1〜6 実装 commit log / 受入観測 (canary 4 段階 bisect 含む): [`docs/specs/ayastorm-r30-p3-volumetric-lighting-bd-trace.md`](../specs/ayastorm-r30-p3-volumetric-lighting-bd-trace.md)
- 親 spec (r30 章): [`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- 前段 (r30 P2, velocity buffer + motion blur + SMAA T2x): [`docs/release/ayastorm-r30-p2-release-note.ja.md`](ayastorm-r30-p2-release-note.ja.md)
