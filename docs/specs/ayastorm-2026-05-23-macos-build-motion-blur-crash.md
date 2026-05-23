# AYAstorm macOSビルドおよびシェーダークラッシュ対応記録

日付: 2026-05-23
ブランチ: `ayastorm-release`
ビルド: `7.2.4.81164`
対象: macOS arm64

## 概要

macOS版AYAstormの再ビルド中、および実機起動確認中に発生した問題と、この段階で入れた修正を記録する。

確認された問題は以下の4件。加えて、同じ未定義プリプロセッサマクロ型の潜在リスクを静的確認で拾い、予防修正した。

1. 以前生成したappが起動直後に `SIGKILL (Code Signature Invalid)` で落ちた。
2. バイナリ後処理なしで再ビルドしたappは起動したが、BD/Cinematic系のモーションブラー経路で落ちた。
3. モーションブラー修正後、起動とインワールド描画は通ったが、AYAstormメニュー内のSSAOを有効化すると落ちた。
4. SSAO修正後、DoF/Post Shader経路で `Deferred Post Shader` がリンク失敗し、同じassertで落ちた。

2件目以降はいずれも、シェーダーのリンク失敗後に未完成のシェーダープログラムを描画パスがbindし、以下のassertに到達していた。

```text
ERROR # llrender/llglslshader.cpp(1053) bind : ASSERT (mProgramObject != 0)
```

## ビルド元の状態

ローカルチェックアウトは、再ビルド前に最新の `origin/ayastorm-release` へ合わせた。

`my_autobuild.xml` はローカルビルド設定として残し、Dullahanは `t-noami/dullahan` のaudio callback `.5` を指定した。

```text
v1.26.0-CEF_139.0.40-ayastorm-audio-callback.5
```

macOS向けに使われたDullahanアーカイブは以下。

```text
https://github.com/t-noami/dullahan/releases/download/v1.26.0-CEF_139.0.40-ayastorm-audio-callback.5/dullahan-1.26.0.202605171727_139.0.40_g465474a_chromium-139.0.7258.139-darwin64-261370827.tar.zst
```

`build-darwin-universal/packages/installed-packages.xml` でも `audio-callback.5` が入っていることを確認した。

## 問題1: Code Signature Invalid

### 症状

最初に生成したappは起動直後にmacOS側で以下のクラッシュになった。

```text
Exception Type: EXC_BAD_ACCESS (SIGKILL (Code Signature Invalid))
Termination Reason: Namespace CODESIGNING, Code 2 Invalid Page
```

### 原因

パッケージング後、ビルド成果物内の絶対パスを消す目的でMach-Oバイナリに後処理をかけていた。

これにより `codesign` 済みのページ内容が変更され、署名が壊れた。

Dullahan、CEF、ソースチェックアウト自体が直接原因ではない。

### 対応

署名後・パッケージング後のバイナリ改変をやめ、appとDMGを再ビルドした。

今後のルール:

```text
codesign後のMach-Oバイナリを直接patch/scrubしない。
```

バイナリ内に絶対ビルドパスが残る場合があるが、それはデバッグ情報やソースメタデータ由来である可能性が高い。安全に除去するには、成果物をバイト編集するのではなく、該当するサードパーティパッケージをパスリマップ付きで再ビルドする必要がある。

## 問題2: モーションブラーシェーダークラッシュ

### 症状

再ビルドしたappは起動したが、Cinematic/BD系のモーションブラー経路が有効になった後にクラッシュした。

ビューアー側のクラッシュメッセージ:

```text
ERROR # llrender/llglslshader.cpp(1053) bind : ASSERT (mProgramObject != 0)
```

macOSクラッシュログ上の主なスタック:

```text
LLGLSLShader::bind()
LLDrawPoolGLTFPBR::renderMotionBlur(int)
LLPipeline::renderGeomMotionBlur()
LLPipeline::renderDeferredLighting()
display(bool, float, int, bool)
```

### ログ根拠

`AYAstorm.log` には、クラッシュ前に以下のシェーダーリンクエラーが出ていた。

```text
Applied Cinematic BD overlay: 36 cvars set
GLSL Linker Error
Shader loading from Skinned AYAstorm Velocity Alpha Shader
ERROR: Input of fragment shader 'vary_texture_index' not written by vertex shader
Failed to link shader: Skinned AYAstorm Velocity Alpha Shader
```

### 原因

`velocityAlphaF.glsl` はindexed texture helper経路を使っており、fragment shader側で `vary_texture_index` を要求している。

通常のvelocity alpha vertex shaderでは `passTextureIndex()` が呼ばれていたが、skinned variantでは呼ばれていなかった。

対象ファイル:

```text
indra/newview/app_settings/shaders/class1/deferred/skinnedVelocityAlphaV.glsl
```

その結果、fragment shaderは `vary_texture_index` を要求する一方で、skinned vertex shaderがそれを書き出さないためリンクに失敗した。リンク失敗後は `mProgramObject == 0` のままとなり、後続の描画パスでbindされてassertに到達した。

## モーションブラー向け修正

### 1. シェーダー修正

ファイル:

```text
indra/newview/app_settings/shaders/class1/deferred/skinnedVelocityAlphaV.glsl
```

追加:

```glsl
void passTextureIndex();
```

`main()` 内で呼び出し:

```glsl
passTextureIndex();
```

これにより、skinned velocity alpha shaderもfragment shaderが期待するtexture-index varyingを書き出す。

### 2. 描画パス側の防御

ファイル:

```text
indra/newview/pipeline.cpp
```

`LLPipeline::renderGeomMotionBlur()` で、velocity passに必要な以下のシェーダーがcompleteか確認するようにした。

```text
gVelocityProgram
gVelocityAlphaProgram
gVelocityAlphaProgram.mRiggedVariant
gAvatarVelocityProgram
```

どれかがリンク失敗している場合は、未完成シェーダーをbindせず、motion blur velocity passをスキップする。

`LLPipeline::renderMotionBlurComposite()` でも `gDeferredMotionBlurProgram.isComplete()` を確認してからcomposite shaderをbindする。

### 3. ビューアー上の通知

スキップ時はログだけでなく、既存の `ChatSystemMessageTip` を使ってビューアーのシステムメッセージとして1セッションに1回だけ通知する。

表示例:

```text
AYAstorm: Motion blur was disabled because a required velocity shader failed to load. Check AYAstorm.log for shader details.
```

ログにも `LL_WARNS_ONCE("Pipeline")` で記録する。

## 問題3: SSAO有効化時のDeferred Sun Shaderクラッシュ

### 症状

モーションブラー修正後、起動とインワールド描画は通った。

その後、AYAstormメニュー内のSSAOを有効にすると、同じbind assertでクラッシュした。

```text
ERROR # llrender/llglslshader.cpp(1053) bind : ASSERT (mProgramObject != 0)
```

### ログ根拠

`AYAstorm.log` では、クラッシュ前にDeferred Sun Shaderのリンク失敗が出ていた。

```text
Shader loading from .../app_settings/shaders/class2/deferred/sunLightSSAOF.glsl
ERROR: 0:59: '' : syntax error: incorrect preprocessor directive
ERROR: 0:59: '' : syntax error: unexpected tokens following #if preprocessor directive - expected a newline
Failed to link shader: Deferred Sun Shader
```

該当行:

```glsl
#if HAS_HBAO
```

### 原因

`sunLightSSAOF.glsl` は `#if HAS_HBAO` を使っていたが、シェーダーセットアップ側で `HAS_HBAO` が定義されていなかった。

macOSのGLSLプリプロセッサはこの未定義マクロを含む `#if` を受け付けず、`Deferred Sun Shader` のリンクに失敗した。

SSAO有効時、`LLPipeline::renderDeferredLighting()` は `gDeferredSunProgram` をSSAO/shadow light pass用に選択してbindする。ここで対象シェーダーが未完成だったため、`mProgramObject == 0` のassertに到達した。

## SSAO向け修正

### 1. HBAOマクロのデフォルト定義

ファイル:

```text
indra/newview/app_settings/shaders/class2/deferred/sunLightSSAOF.glsl
```

追加:

```glsl
#ifndef HAS_HBAO
#define HAS_HBAO 0
#endif
```

これにより、将来的にHBAOを明示defineする余地は残しつつ、通常のSSAO経路ではregular AO側に倒してコンパイルできる。

### 2. SSAO/shadow描画パス側の防御

ファイル:

```text
indra/newview/pipeline.cpp
```

`LLPipeline::renderDeferredLighting()` で、SSAO/shadow light passの前に `sun_shader.isComplete()` を確認するようにした。

リンク失敗している場合は、そのpassだけをスキップし、未完成シェーダーをbindしない。deferred lighting全体は継続する。

### 3. ビューアー上のSSAOスキップ通知

SSAO/shadow passをスキップした場合も、`ChatSystemMessageTip` で1セッションに1回だけ通知する。

表示例:

```text
AYAstorm: SSAO/shadow smoothing was disabled because the deferred sun shader failed to load. Check AYAstorm.log for shader details.
```

## 問題4: DoF/Post Shaderクラッシュ

### 症状

SSAO修正後、起動とインワールド描画は通ったが、後続の描画設定で再び以下のassertが発生した。

```text
ERROR # llrender/llglslshader.cpp(1053) bind : ASSERT (mProgramObject != 0)
```

### ログ根拠

`AYAstorm.log` では、`Deferred Post Shader` のリンク失敗が出ていた。

```text
Shader loading from .../app_settings/shaders/class1/deferred/postDeferredHQDoFF.glsl
ERROR: 0:63: '' : syntax error: incorrect preprocessor directive
ERROR: 0:63: '' : syntax error: unexpected tokens following #if preprocessor directive - expected a newline
Failed to link shader: Deferred Post Shader
```

該当行:

```glsl
#if HAS_DOF_CHROMA
```

### 原因

`postDeferredHQDoFF.glsl` と `postDeferredF.glsl` は `#if HAS_DOF_CHROMA` と `#if FRONT_BLUR` を使う。

`llviewershadermgr.cpp` は該当機能がONの場合だけ `HAS_DOF_CHROMA=1` / `FRONT_BLUR=1` のpermutationを追加する設計だった。そのため、機能OFF時はマクロが未定義のまま `#if HAS_DOF_CHROMA` が残る。

macOSのGLSLプリプロセッサはこの未定義マクロを含む `#if` を受け付けず、`Deferred Post Shader` がリンク失敗した。後続の `LLPipeline::renderDoF()` が `gDeferredPostProgram` をbindし、`mProgramObject == 0` のassertに到達した。

## DoF/Post Shader向け修正

### 1. DoF系マクロのデフォルト定義

以下のDoF系シェーダーで、未指定時のデフォルトを明示した。

```text
indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl
indra/newview/app_settings/shaders/class1/deferred/postDeferredHQDoFF.glsl
indra/newview/app_settings/shaders/class1/deferred/postDeferredNoDoFF.glsl
```

`postDeferredF.glsl` と `postDeferredHQDoFF.glsl`:

```glsl
#ifndef HAS_DOF_CHROMA
#define HAS_DOF_CHROMA 0
#endif

#ifndef FRONT_BLUR
#define FRONT_BLUR 0
#endif
```

`postDeferredNoDoFF.glsl`:

```glsl
#ifndef HAS_DOF_CHROMA
#define HAS_DOF_CHROMA 0
#endif
```

これにより、permutationが指定されない場合も通常のOFF分岐としてコンパイルされる。

### 2. DoF描画パス側の防御

ファイル:

```text
indra/newview/pipeline.cpp
```

`LLPipeline::renderDoF()` で以下のシェーダーがcompleteか確認するようにした。

```text
gDeferredCoFProgram
gDeferredPostProgram
gDeferredDoFCombineProgram
```

どれかがリンク失敗している場合は、DoFをスキップして通常コピーにフォールバックする。未完成シェーダーはbindしない。

### 3. ビューアー上のDoFスキップ通知

DoFスキップ時も `ChatSystemMessageTip` で1セッションに1回だけ通知する。

表示例:

```text
AYAstorm: Depth of Field was disabled because a required post-processing shader failed to load. Check AYAstorm.log for shader details.
```

## 検証

モーションブラー修正後にapp/DMGを再ビルドした。

SSAO修正後にもappを再ビルドした。結果的にこのビルドではDMG作成まで完了しているが、実機確認対象は以下のapp。

DoF/Post Shader修正後は、DMGを作らず `viewer` schemeでapp単体を再ビルドした。

```text
build-darwin-universal/newview/Release/AYAstorm.app
```

成果物:

```text
build-darwin-universal/newview/Release/AYAstorm.app
build-darwin-universal/newview/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-81164.dmg
```

実行した検証:

```text
xcodebuild ... -scheme llpackage -configuration Release build
xcodebuild ... -scheme viewer -configuration Release build
codesign --verify --deep --strict --verbose=2 build-darwin-universal/newview/Release/AYAstorm.app
```

結果:

```text
BUILD SUCCEEDED
AYAstorm.app: valid on disk
AYAstorm.app: satisfies its Designated Requirement
```

SSAO修正済みのシェーダーがapp bundle内にパッケージされていることも確認した。

```text
build-darwin-universal/newview/Release/AYAstorm.app/Contents/Resources/app_settings/shaders/class2/deferred/sunLightSSAOF.glsl
```

DoF/Post Shader修正済みのシェーダーがapp bundle内にパッケージされていることも確認した。

```text
build-darwin-universal/newview/Release/AYAstorm.app/Contents/Resources/app_settings/shaders/class1/deferred/postDeferredF.glsl
build-darwin-universal/newview/Release/AYAstorm.app/Contents/Resources/app_settings/shaders/class1/deferred/postDeferredHQDoFF.glsl
build-darwin-universal/newview/Release/AYAstorm.app/Contents/Resources/app_settings/shaders/class1/deferred/postDeferredNoDoFF.glsl
```

## 追加静的検証: 未定義 `#if MACRO` リスク

`HAS_DOF_CHROMA` / `FRONT_BLUR` / `HAS_HBAO` で発生したmacOS GLSLプリプロセッサ問題と同じ型を、shader tree全体から機械的に確認した。

確認観点:

```text
#if SOME_MACRO
```

の形で使われているマクロについて、shader内のデフォルト定義、またはshader loader側で常時注入される定義があるかを確認した。

結果、今回の修正対象では以下の状態になった。

```text
HAS_DOF_CHROMA    shader内で未指定時 0
FRONT_BLUR        shader内で未指定時 0
HAS_HBAO          shader内で未指定時 0
HAS_NOISE         shader内で未指定時 0
GODRAYS_FADE      shader内で未指定時 0
AYASTORM_CINEMATIC llshadermgr.cpp が常時 0/1 を注入
```

追加で予防修正したファイル:

```text
indra/newview/app_settings/shaders/class1/effects/glowExtractF.glsl
indra/newview/app_settings/shaders/class3/deferred/volumetricLightF.glsl
```

`glowExtractF.glsl` は `RenderGlowNoise=0` 時に `HAS_NOISE` permutationが付かないため、macOSでは同型のpreprocessor errorになる可能性があった。未指定時 `0` を追加した。

`volumetricLightF.glsl` は `RenderVolumetricLightingDirectional=0` 時に `GODRAYS_FADE` permutationが付かないため、同型のpreprocessor errorになる可能性があった。未指定時 `0` を追加した。

また、最新版で `postDeferredHQDoFF.glsl` に入ったApple Silicon Metal向けのループ上限と同じ安全化を、標準DoFの `postDeferredF.glsl` にも追加した。通常のCoF範囲では見た目は変わらず、異常値や極端な設定でループが収束しない場合の保険になる。

## 現在の主な変更ファイル

```text
indra/newview/app_settings/shaders/class1/deferred/skinnedVelocityAlphaV.glsl
indra/newview/app_settings/shaders/class1/deferred/postDeferredF.glsl
indra/newview/app_settings/shaders/class1/deferred/postDeferredHQDoFF.glsl
indra/newview/app_settings/shaders/class1/deferred/postDeferredNoDoFF.glsl
indra/newview/app_settings/shaders/class1/effects/glowExtractF.glsl
indra/newview/app_settings/shaders/class2/deferred/sunLightSSAOF.glsl
indra/newview/app_settings/shaders/class3/deferred/volumetricLightF.glsl
indra/newview/pipeline.cpp
```

`my_autobuild.xml` はgit管理外のローカルビルド設定ファイルとして残っており、Dullahan `.5` のpackage overrideを含む。

## 残リスクと実機確認項目

今回観測されたモーションブラー側のroot causeは、skinned velocity alpha pathで修正済み。

SSAO側のroot causeも、`HAS_HBAO` のデフォルト定義追加により修正済み。

DoF/Post Shader側のroot causeも、`HAS_DOF_CHROMA` / `FRONT_BLUR` のデフォルト定義追加により修正済み。

描画パス側の防御は、今後別のシェーダーがリンク失敗した場合に、即クラッシュではなく該当passをスキップしてシステムメッセージを出すためのもの。

## Windows / Linux への影響

今回の変更はOS分岐ではなく共通shader/C++に入るため、WindowsとLinuxにも適用される。

想定される影響:

1. `#ifndef ... #define ... 0` 追加は、未指定のpermutationを明示的にOFF扱いにするだけで、ON時は従来通り `addPermutation(..., "1")` が優先される。
2. `skinnedVelocityAlphaV.glsl` の `passTextureIndex()` 追加は、fragment shader側の `vary_texture_index` 要求を満たすための修正で、Windows/Linuxでもリンク安定化方向。
3. `pipeline.cpp` の `isComplete()` guardは、shader link失敗時にassert crashせず該当passをスキップする。副作用として、shader regressionが即クラッシュではなく「効果が無効化される」挙動になる。ログと1セッション1回の通知は出る。
4. `postDeferredF.glsl` のbounded loopは、標準DoFの異常値/極端値に対する保険。通常のCoF範囲では見た目はほぼ変わらない想定。極端なDoF設定で `sc` が32pxを超えるケースでは、Windows/Linuxでもblur半径が頭打ちになる可能性がある。

総合すると、Windows/Linuxへの主な影響は「未定義macro依存をなくす」「shader失敗時にクラッシュしにくくする」方向。実質的な描画差分リスクは、極端な標準DoF blurの頭打ちに限定される見込み。

未実施:

```text
Windows 実ビルド
Linux 実ビルド
Windows/Linux 実機での shader load log 確認
```

実機では以下を確認する。

1. `AYAstorm.log` に以前の `vary_texture_index` リンクエラーが出ない。
2. Cinematic/BD系のモーションブラーで `LLDrawPoolGLTFPBR::renderMotionBlur()` クラッシュが再発しない。
3. SSAO有効化時に `sunLightSSAOF.glsl` のpreprocessor errorが出ない。
4. SSAO有効化時に `LLGLSLShader::bind()` クラッシュが再発しない。
5. DoF有効化時に `postDeferredHQDoFF.glsl` / `postDeferredF.glsl` のpreprocessor errorが出ない。
6. DoF有効化時に `Deferred Post Shader` のリンク失敗から `LLGLSLShader::bind()` クラッシュへ進まない。
7. 今後別のシェーダーがリンク失敗した場合、ビューアー上にスキップ通知が1回表示される。
8. `RenderGlowNoise=0` で `glowExtractF.glsl` の `HAS_NOISE` preprocessor errorが出ない。
9. `RenderVolumetricLightingDirectional=0` で `volumetricLightF.glsl` の `GODRAYS_FADE` preprocessor errorが出ない。
10. Windows/Linuxで同じ設定を切り替えてもshader link/preprocessor errorが出ない。
