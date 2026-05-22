# AYAstorm r30 P4 — リリース告知

**r30 P4 は Cinematic mode に BD DoF chain (高品質 DoF + 色収差 + 前ボケ) を導入するリリース** — 撮影描画章 (r30 chapter) の表現力強化として、Black Dragon Viewer から DoF パイプライン全体を移植し、専用の Cinematic Controls floater (新設 AYAstorm 上部メニュー → `Cinematic Controls...` / `Alt+C`) で撮影中のライブ調整を可能にします。

実装トレース・改修ポイント・受入観測 (chroma 数式 hotfix 含む) ・上流参照行は永続資料 (`docs/specs/ayastorm-r30-p4-bd-dof-chain-trace.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r30 P4 — BD DoF Chain (HQ DoF + Chromatic Aberration + Front Blur)

### r30 P4 の柱: 写真レンズの「ボケと色収差」を入れる

r30 章 (撮影描画) は P2 で per-object motion blur + SMAA T2x、P3 で volumetric lighting を入れて Cinematic mode の描画基盤と光の物質感を立てました。P4 はその Cinematic mode に **写真レンズの光学特性 (高品質 DoF + 色収差 + 前ボケ)** を本実装します。

色収差 (chromatic aberration) は、ガラスレンズが R/G/B 各波長を異なる屈折率で曲げるために生じる光学現象で、画面周縁部の輪郭が赤と青に分離する「古いレンズで撮ったような」絵になります。AYAstorm r30 の核心テーゼ「写真を撮るに値する空気と空間」(`docs/specs/ayastorm-r30-cinematic-chapter.md`) を、レンズの物質感として可視化する一段です。

通常 (Standard) / リアリズム (AYAstorm View) モードでは shader register 段で permutation が付かないため、追加コストはゼロです。

### 仕組み

post-process pass として既存 DoF 経路に permutation 追加で挿入されます:

```
deferredScreen (lighting 後の最終色)
  → gDeferredPostProgram (HQ DoF 経路)
       ├─ Cinematic mode + RenderDepthOfFieldHighQuality で gate
       ├─ HAS_DOF_CHROMA permutation で CoF (Circle of Confusion) ベースの色収差
       ├─ FRONT_BLUR permutation で前ボケ経路有効化
       └─ DEFERRED_CHROMA_STRENGTH uniform で強度動的制御
  → gDeferredPostNoDoFProgram (NoDoF 経路)
       ├─ HAS_DOF_CHROMA permutation で radial offset 色収差
       └─ vary_fragcoord ベース radial 数式 (画面中央 0 → 周縁部最大)
  → gDeferredPostNoDoFNoiseProgram (最終 present 経路)
       └─ 同 NoDoF 経路 + film grain noise
```

色収差は per-channel texcoord offset 方式で、R チャネルは radial 方向に「内側」、B チャネルは「外側」へずらしてサンプリングします (G はずらさない)。

### 設定

**通常運用での操作は不要。** Cinematic mode (`AYAVisualRealismEnabled = 2`) を起動すれば BD DoF chain が自動で有効化されます。撮影中のライブ調整は新設 **AYAstorm メニュー → Cinematic Controls...** (`Alt+C`) で。チューニング用の cvar は以下:

| Cvar | 既定値 | 用途 |
|---|---|---|
| `RenderDepthOfFieldHighQuality` | `0` (OFF) | 4× CoF サンプル + depth-gated chroma の高品質 DoF post-pass。GPU コスト高め、user opt-in。Cinematic 起動時のみ有効 |
| `RenderDepthOfFieldChroma` | `1` (ON) | 色収差機能の compile 取り込み。OFF にすると vignette only (画面端のみ薄く色ずれ)、ON で per-pixel に DoF blur 量に連動した色収差。Cinematic 起動時のみ有効 |
| `RenderChromaStrength` | `5.0` | 色収差の強度 (0〜100 range)。`5` で subtle (「気付く」range)、`10` で目で見て明らかな分離、`30` で映画的な強い収差、`>50` でスタイライズド表現。BD 既定 `0.0` は BD の UI に slider が露出する前提なので、AYAstorm では floater で同じ動線を出した上で default を上げる |
| `RenderDepthOfFieldFront` | `1` (ON) | 前ボケ (焦点面より手前の物体もボケる) を有効化。BD default 完全再現。HQ DoF user opt-in 時のみ effective。Cinematic 起動時のみ有効 |

### 見え方の目安

- **色収差の出方**: 画面中央は元の絵のまま、画面周縁部に向かって輪郭が R/B に分離していきます。古いレンズの「fringe」と呼ばれる現象を模した radial 数式 (中心 r=0 → 周縁 r=1) です
- **HQ DoF の効き所**: 標準 DoF は forward pass の限られた sample 数でボケを作るのに対し、HQ DoF は 4× sample で post-pass に再構成するため、ボケ円の輪郭が滑らかに、bokeh の dot 形状が消えます。被写界深度の浅い撮影 (顔の前で焦点、奥がボケる) で差が顕著
- **前ボケ**: 焦点面より手前の物体 (例: 自分のアバターの手) が焦点合っているところより遠景方向にボケます。標準 DoF (前ボケ無し) との差は被写体の手前に小物を置いたカットで一目瞭然
- **モード**: Cinematic mode 必須。Standard / AYAstorm View では shader permutation が付かないため一切走らない

### 移行ノート

- 撮影モード (Cinematic) で BD DoF chain (HQ DoF + 色収差 + 前ボケ) が新たに使えます
- 通常 / リアリズムモードには一切影響なし (permutation gate)
- Cinematic ↔ 他モードの切替は **viewer 再起動が必要** (r30 P1 で確定した設計)
- 既存 FIRE-16728 free-aim DoF mechanism (フォーカスをどこに合わせるか) は保持。P4 で借りたのは「DoF 内部の sample 精度 + 色収差 + 前ボケ」のみで、ピント機構は Firestorm 既存を踏襲する hybrid 構成
- 新設 AYAstorm 上部メニュー (Build と Help の間) は P5+ の Cinematic 機能拡張時にも sibling 項目が増える予定
- Cinematic Controls floater は撮影中のライブ調整専用で、Preferences には反映しません (永続設定の場でなく撮影 workflow 側に置く判断)

### 既知の制約

- **NoDoF 経路は subtle**: 標準 DoF (= 非 HQ) の色収差は CoF が無いため radial 数式での弱い vignette となり、画面中央付近は元の絵のまま。強い色収差を狙う場合は `RenderDepthOfFieldHighQuality=1` (HQ DoF) を併用すると CoF ベースで全画面に色収差が乗ります
- **`RenderChromaStrength` default の BD 乖離**: BD 既定 `0.0` (= 効果ゼロ) で出ていた表現を AYAstorm では `5.0` に上げて出荷します。BD は UI に slider が露出して user が値を上げる前提だったため。AYAstorm でも floater で同等動線を出している以上、初見で「機能が無いように見える」状態を避ける judgment (§5.7 / `feedback_match_bd_defaults_on_borrow.md`)
- **lldrawpoolwater 経由の water chroma は取り込まず**: BD `lldrawpoolwater.cpp:257` で water shader にも chroma uniform を push しているが、Firestorm の water pipeline は BD と差分が大きく未知の regression リスクがあるため P4 スコープ外
- **多言語**: P4 では floater UI は英語のみ。日本語/中国語 lproj は P5+ で Cinematic 系機能拡張時にまとめて翻訳
- **macOS / Windows 実機検証**: AYAstorm 側 Linux ビルドで動作確認 PASS、Mac/Win の Release ビルドは tag 切り出し時に実施

### 実装概要

- shader (`indra/newview/app_settings/shaders/class1/deferred/`):
  - `postDeferredHQDoFF.glsl` (新規、BD から借用 + AYAstorm 改修): HQ DoF + CoF ベース色収差 + 前ボケ block を含む
  - 既存 `postDeferredF.glsl`: `dofSample()` 末尾に `#if HAS_DOF_CHROMA` ガード付き chroma block を追加 (標準 DoF 経路にも色収差を載せる)
  - 既存 `postDeferredNoDoFF.glsl`: NoDoF 経路に radial offset 数式の chroma block を追加 (受入過程で当初の edge-gated vignette 案を radial offset に書き直し、§9.2 参照)
- C++ pipeline (`indra/newview/`):
  - `pipeline.{cpp,h}`: `LLPipeline::RenderChromaStrength` static + `renderDoF` / `renderFinalize` / `bindDeferredShader` 3 site での uniform push
  - `llviewershadermgr.cpp`: 3 program (`gDeferredPostProgram` / `gDeferredPostNoDoFProgram` / `gDeferredPostNoDoFNoiseProgram`) に Cinematic + cvar 2 段 gate + `HAS_DOF_CHROMA` / `FRONT_BLUR` permutation 付与、cvar signal listener で再起動を最小化
  - `llshadermgr.{cpp,h}`: `DEFERRED_CHROMA_STRENGTH` reserved uniform enum + 文字列追加
- XUI / UI 配線:
  - `menu_viewer.xml`: AYAstorm 上部メニュー新設 (Build と Help の間)、`Cinematic Controls...` 項目 (`Alt+C`) で floater toggle
  - `floater_aya_cinematic.xml` (新規): width 320 / height 320 / single_instance, DoF + Chromatic Aberration 2 section の控えめな構成
  - `llviewerfloaterreg.cpp`: floater 登録 (`FloaterQuickPrefs` 派生 generic class を流用)
- `indra/newview/app_settings/settings.xml`: P4 cvar 4 件 追加
- Cinematic 起動時のみ permutation 付与 → 通常 / リアリズムモードはコストゼロ

### Credits

BD DoF chain (HQ DoF + 色収差 + 前ボケ) の実装パターンは [Black Dragon Viewer](https://github.com/NiranV/Black-Dragon-Viewer) (NiranV Dean) 由来です。AYAstorm では BD `995a1354d8` (2026-04-19) を上流参照点として `postDeferredHQDoFF.glsl` をライセンス継承 (LGPL-2.1-only) で取り込み、AYAstorm 側で以下の改修を加えました:

- shader file header の `@file` 修正 + provenance コメント (BD `995a1354d8`、LGPL-2.1-only) 追加
- Cinematic mode gate (`AYAVisualRealismEnabled == 2` 時のみ permutation 付与)
- BD の独立 settings file (`settings_blackdragon.xml`) を取り込まず、cvar 4 件を Firestorm 標準 `settings.xml` に統合
- NoDoF 経路の chroma 数式を BD の固定 offset 方式から radial per-channel offset 方式へ書き直し (vary_fragcoord ベース、画面中心 0 → 周縁部最大)
- default `RenderChromaStrength` を BD `0.0` から `5.0` に retune (受入過程で chroma_str=0 だと「機能が無いように見える」と判定、`feedback_match_bd_defaults_on_borrow.md` を実体験ベースで再適用)
- BD UI (`panel_preferences_graphics1.xml` / `panel_machinima.xml`) を取り込まず、AYAstorm 独自の top menu + 専用 Cinematic Controls floater で動線を提供 (chapter §1.2 通り)
- BD の FIRE-16728 と独立な focus 機構 (`CameraFreeDoFFocus` 静的) は取り込まず、Firestorm 既存 (FIRE-16728 free-aim DoF) を維持
- BD `lldrawpoolwater.cpp:257` の water chroma uniform push は取り込まず (Firestorm water pipeline との差分が大きく深層 regression リスク)

### ドキュメント

- r30 P4 full trace / file:line 改修マップ / step 1〜7 実装 commit log / 受入観測 (chroma 数式 hotfix + UI revise 経過): [`docs/specs/ayastorm-r30-p4-bd-dof-chain-trace.md`](../specs/ayastorm-r30-p4-bd-dof-chain-trace.md)
- 親 spec (r30 章): [`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- 前段 (r30 P3, volumetric lighting): [`docs/release/ayastorm-r30-p3-release-note.ja.md`](ayastorm-r30-p3-release-note.ja.md)
