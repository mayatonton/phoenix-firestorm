# AYAstorm R42 macOS arm64 / MoltenVK 検証手順

## 目的

`feature/ayastorm-r42-phase2` を起点にした開発用 app で、MoltenVK の shadow
multiview 経路、描画品質、および P0 の性能・安定性計器を再現可能な形で確認する。

- 対象 app: `build-darwin-universal/newview/Release/AYAstorm.app`
- 対象環境: Apple Silicon（arm64）の macOS
- 開発用 profile: `~/Library/Application Support/AYAstorm-dev/`
- 対象外: DMG 配布物、Intel Mac、OpenGL fallback

通常の開発 app の configure / build は
[macOS ビルド手順](../build/building_ayastorm_macos.md)を、bundle runtime の成立条件は
[MoltenVK 実行時ブートストラップ仕様](../specs/ayastorm-r42-macos-moltenvk-runtime-bootstrap.md)を参照する。

## 1. ビルド成果物の確認

開発 app をビルドしてから、次を確認する。DMG / `llpackage` は使わない。

```bash
export REPO="/path/to/phoenix-firestorm-mayatonton"
export APP="$REPO/build-darwin-universal/newview/Release/AYAstorm.app"

test -x "$APP/Contents/MacOS/AYAstorm"
lipo -archs "$APP/Contents/MacOS/AYAstorm"
codesign --verify --deep --strict --verbose=2 "$APP"
test -e "$APP/Contents/Frameworks/libvulkan.dylib"
test -e "$APP/Contents/Frameworks/libMoltenVK.dylib"
test -f "$APP/Contents/Resources/vulkan/icd.d/MoltenVK_icd.json"
```

期待値は executable が `arm64` であり、codesign と 3 つの runtime artifact
確認が成功することである。

## 2. 起動前に必ず cache を消去する

すべての AYAstorm 開発 app を終了してから実行する。今回 shader が変わっているため、
旧 SPIR-V や pipeline cache を残したままの起動は未定義動作として扱い、検証結果に
用いてはならない。

```bash
export AYA_DEV_PROFILE="$HOME/Library/Application Support/AYAstorm-dev"
rm -rf "$AYA_DEV_PROFILE/cache/shader_cache"
rm -f "$AYA_DEV_PROFILE/cache/pipeline_cache.bin"
```

削除対象は上記 2 項目だけである。`AYAstorm-dev` profile 全体、通常版の
`AYAstorm` profile、または他の cache を削除しない。

## 3. 診断起動とログ採取

`AYASTORM_PERF_LOG=5` を付け、Vulkan / MoltenVK の Loader 環境変数を追加せずに
開発 app を起動する。ログイン後、影を含む場面でカメラを動かしながら 2〜3 分使用する。

```bash
export REPO="/path/to/phoenix-firestorm-mayatonton"
export APP="$REPO/build-darwin-universal/newview/Release/AYAstorm.app"
export AYA_DEV_PROFILE="$HOME/Library/Application Support/AYAstorm-dev"

AYASTORM_PERF_LOG=5 "$APP/Contents/MacOS/AYAstorm"
```

ログは次に出力される。

```bash
export LOG="$AYA_DEV_PROFILE/logs/AYAstorm.log"
rg '#VkPerf#|FRAMETIME ms:|recreateSwapchain|WARNING|ERROR|VUID|device lost' "$LOG"
```

`AYASTORM_PERF_LOG=5` は `#VkPerf#` を約 5 秒周期で出す。P0 の `FRAMETIME ms:`
は 10 秒周期で、平均値・p95・p99・最大値を記録する。

## 4. `shsite` による shadow 経路判定

`#VkPerf#` 行に含まれる `shsite` 欄を、最低 1 行以上そのまま記録する。判定は次の通り。

| `shsite` の観測 | 判定 |
| --- | --- |
| `mv.am=...` がある | multiview フル経路 |
| `mv.op=...` はあるが `rest.am` が非ゼロ | bindless なしへの降格 |
| `fb.` で始まる項目がある | multiview なし fallback |

単一のカウンタだけで「正常」としない。下記の視覚確認と、P0 の性能・安定性ログを
同じ run から揃える。

## 5. 視覚確認チェックリスト

影が十分に見える場所・時刻・カメラ角度を選び、次を確認する。

- 髪の透過影が正しく出る。
- 植生・木の葉の影が正しく出る。
- 格子・金網など alpha mask material の影が正しく出る。
- 草の影が正しく出る。
- 半透明オブジェクトの影が点描（dither）状に正常に出る。
- カメラを回しても影が消えない。
- spot light の影が正しく出る。

各項目を `OK` / `NG` / `未確認` で記録し、`NG` では再現場所、時刻、カメラ操作、
スクリーンショットの有無を添える。

### 実施結果 — 2026-08-02

| 確認項目 | 結果 |
| --- | --- |
| 髪の透過影 | OK |
| 植生・木の葉の影 | OK |
| 格子・金網など alpha mask material の影 | OK |
| 草の影 | OK |
| 半透明オブジェクトの影が点描（dither）状に出ること | **NG** |
| カメラを回しても影が消えないこと | OK |
| spot light の影 | OK |

半透明オブジェクトの NG は、透明度段階を並べた検証シーンで、期待する点描状ではない影を
確認したもの。スクリーンショットは有り（この task に添付された
`codex-clipboard-5ef5d7d3-9b79-401b-a0ae-faaae31d421e.png`）。再現場所、ワールド時刻、
カメラ位置・回転操作の詳細は未記録であるため、修正確認時はこれらを採取して同一条件で
再試験する。この時点では、この NG により視覚受入を **FAIL** とした。

### 再確認結果 — 2026-08-10

半透明オブジェクトの影が点描（dither）状に出ることを、検証者の目視で **OK** と確認した。
したがって同項目の現時点の結果は **OK** とし、2026-08-02 の NG は過去の再現記録として残す。

この PR には `pbrShadowAlphaBlendF.glsl` 等の半透明 shadow shader 差分を含めていないため、
今回の OK をこの PR の device lost 修正による効果とは断定しない。再発防止のため、同一の透明度
段階・light・カメラ条件で screenshot と log を採取する再試験は引き続き推奨する。

### 追加視覚事象: メッシュボディ alpha の再有効化 — 2026-08-02

メッシュボディの alpha 関連操作のうち、**alpha を有効にして透明度を 100% に戻す操作**が
その場では反映されなかった。alpha を無効にする操作ではなく、この「再有効化」の反映が
失敗することが主事象である。しかしテレポート後には反映された。従って「完全に描画
されない」ではなく、外観更新の即時反映に失敗し、テレポートが更新を回復させる事象として
記録する。

- 結果: **NG（テレポートで一時回復）**。特に alpha を有効化して透明度 100% に戻す操作が、
  テレポートを必要とせず反映されるまで受入にしない。
- 視覚証拠: [Gyazo capture](https://gyazo.com/9aca27980c7b8cfe7d32c78580340724)
  （2026-08-02 21:35 JST にアップロード）。
- 関連ログ: 12:34:27--12:34:29 UTC に `forced full rebake`、COF
  `210360 -> 210361 -> 210362`、旧 appearance `#210360` の破棄が連続している。
  これは操作に伴う外観更新と時刻が整合するが、alpha 再有効化失敗を直接示すログではない。
- 再試験時の合格条件: alpha を無効化した後、alpha を有効にして透明度 100% に戻す。これが
  テレポートせずに反映されること。操作開始・終了時刻、COF version、`Stale appearance` の
  有無を併記すること。

## 6. 半透明 dither shadow: 過去の NG と修正候補（PR 対象外）

この節は §5 で過去に記録した半透明オブジェクト shadow NG に対する**修正候補**であり、shader の
修正がこの branch に含まれることを示すものではない。2026-08-10 の目視再確認では dither shadow は
OK であり、ここに記載する候補は再発時に検討するものとする。

### 現状の確認結果

Vulkan の PBR alpha-blend shadow shader
[`pbrShadowAlphaBlendF.glsl`](../../indra/newview/app_settings/shaders/class1/deferred/pbrShadowAlphaBlendF.glsl)
では、texture alpha を `0.05` 未満で discard する判定に使う一方、Bayer 点描の密度は
`object_alpha` だけで決めている。`object_alpha` は `LLDrawInfo::mObjectAlpha` 経由の face
alpha である。したがって、材質または texture の alpha だけで半透明にした物体では、その
透明度が点描密度に反映されない可能性がある。

この確認は source 上の事実であり、今回の NG の直接原因がこれであることはまだ**未確定**
である。特に `vertex_color.a` と `object_alpha` が同じ face alpha を重複して表すかは、
実装前に draw data の生成経路で確認する。

### 修正案

対象は OpenGL との見た目合わせではなく、Vulkan の alpha-blend shadow における有効な
不透明度を正しく Bayer 判定へ渡すことである。PBR alpha-blend shadow shader で、相互に
独立していることを確認できた alpha 要素だけから `effective_shadow_alpha` を作り、次の
順序で使う。

1. `effective_shadow_alpha < 0.05` は discard する。
2. `effective_shadow_alpha < 0.996` の場合、4×4 Bayer threshold と比較して discard する。
3. それ以外は shadow caster として残す。

候補となる入力は texture/material alpha、face alpha (`object_alpha`)、および独立した
vertex alpha である。値が二重に含まれる経路では乗算しない。alpha mask material の
cutout shadow は対象外とし、輪郭を保つ既存経路を変更しない。

この shader は `LL_VULKAN_GLSL` 経路で共有されるため、修正時は macOS MoltenVK だけでなく
Linux Vulkan と Windows Vulkan を同一 scope とする。

### 再発時の受入条件

同じ light、地面、カメラ距離で、次の4行を別々に比較する。

| 行 | 透明度の与え方 | 検証値 |
| --- | --- | --- |
| A | face alpha | 25% / 50% / 75% |
| B | texture/material alpha | 25% / 50% / 75% |
| C | 独立した vertex alpha（存在する場合） | 25% / 50% / 75% |
| D | alpha mask control | cutout が従来どおり明瞭であること |

各行で、透明度に応じて shadow の点描密度が変わること、camera 回転で影が消えないこと、
`AYASTORM_PERF_LOG=5` の `shsite` が `mv.am` を維持し `rest.am` / `fb.*` を出さないことを
確認する。修正前後の screenshot と run 全体の log を保存し、VUID / device lost の有無も
併記する。

## 7. 共通 texture type 修正の Linux / Windows 影響

この検証ブランチでは、`indra/llrender/llrender.cpp` の `LLImageGL::getTarget()`
との比較を `GL_TEXTURE_2D` から `LLTexUnit::TT_TEXTURE` に直している。
`getTarget()` の戻り値は全プラットフォーム共通の `LLTexUnit::eTextureType` であり、
OpenGL の数値 enum ではない。従って旧比較は、診断する compiler では定数比較警告を
`-Werror` として build failure にし、実行時には通常の 2D texture でも
`mCurrVkHeapSlot` を invalid のままにしていた。

ここでの `GL_TEXTURE_2D` は比較誤りの元になった OpenGL の数値定数であり、GL fallback
を対象にする記述ではない。この変更は Vulkan 側の `mCurrVkHeapSlot` の記録だけを直す。
実行時の影響範囲は **LLDrawInfo を伴わない即時 draw(scratch 経路・establishPerDrawId の
params==nullptr 分岐)に限られる** — 通常の draw(LLDrawInfo 経由)は texture から直接
heap slot を引くため、この比較誤りの影響を受けておらず従来から正常である。また
`vkHeapSlotOrDefault` 内部に GL target と residency の二重チェックが残るため、修正後も
不正な slot が漏れる経路はない。もう 1 つの実効修正は `non2d_bind` 検出器の条件是正で、
旧条件は型違いにより**正常な 2D texture へ恒常誤発火**していた(修正後は本物の非 2D
bind のみ報告される)。macOS だけの shadow 結果を Linux / Windows の実行証拠と
みなしてはならない点は変わらない。

| プラットフォーム | 予想される影響 | プラットフォーム担当者の確認項目 | 状態 |
| --- | --- | --- | --- |
| Linux | `eTextureType` と OpenGL 数値 enum の不正比較による compiler failure を回避する。Vulkan + bindless では scratch 経路の 2D texture heap slot が有効になる。 | Vulkan 構成の Linux build と Vulkan 起動を行い、device / presentation surface の初期化、texture、alpha / mask、shadow を確認する。 | **検証済 2026-08-03**(本流 merge `b02b13e4aa2`・build 0 error・診断走行 = texture / alpha mask / alpha blend / shadow 視覚正常・fail-closed 検出器沈黙・`non2d_bind` の旧誤発火署名(tgt=0x0)消滅を確認) |
| Windows | Linux と同じ共通ソースを compile する。Vulkan + bindless 時の heap slot 選択が変わる。 | `build_ayastorm.bat` による Vulkan 構成の build と Vulkan 起動を行い、device / presentation surface の初期化、texture、alpha / mask、shadow を確認する。 | OPEN |

Linux / Windows とも Vulkan 前提で検証する。Vulkan + bindless run では、default texture
への意図しない置換、欠落 texture、alpha mask / alpha blend の欠落、shadow の変化がない
ことを確認する。各プラットフォームの build log と検証結果を、この macOS 検証記録とは
別に残す。

## 8. 提出物

次をひとまとまりで共有する。ログ全体が最優先である。

1. `AYAstorm.log` 全体（最低でもこの run の開始から終了まで）。
2. `#VkPerf#` 行は全欄をそのまま（`shsite`・`e3`・`fam`・`shamdi`・`mdi` を含む行全体を、
   加工せず最低 1 行転記する。欄の取捨選択をしない — 2026-08-02 の実測記録では
   `shsite` のみ抜粋したため `e3 pal` が失われた）。
3. `FRAMETIME ms:` の全行（10 秒周期、p95 / p99 を含む）。
4. `recreateSwapchain` 行。存在する場合は必ず `reason=` を残す。
5. `WARNING`、`ERROR`、`VUID`、`device lost` の有無。存在する場合は該当行の前後も含める。
6. 視覚確認チェックリストの結果と、NG があれば再現情報。

起動確認では、`initialized device=...`、`Vulkan presentation surface initialized`、
`Initializing Login Screen` が順に出ることも確認する。これらが不足している場合は、
影の検証を開始せず、起動失敗として log 全体を共有する。

## 9. 実測記録 — 2026-08-02 macOS arm64 / MoltenVK

この記録は `feature/ayastorm-r42-macos-arm64-moltenvk-validation` の開発 app を、
shader cache と `pipeline_cache.bin` を削除してから
`AYASTORM_PERF_LOG=5` で起動した run のものである。この run は終了時に log rotate
されたため、原本は次に残る（その後の再起動分は `AYAstorm.log`）。

```text
~/Library/Application Support/AYAstorm-dev/logs/AYAstorm.old
```

### 起動成立

次の順序で記録されたため、MoltenVK の Loader / surface / login screen 初期化は
**VERIFIED** とする。

```text
2026-08-02T12:00:19Z initialized device=Apple M2 Pro
2026-08-02T12:00:20Z Vulkan presentation surface initialized
2026-08-02T12:00:22Z Initializing Login Screen
```

### shadow 経路

`#VkPerf#` は 5 秒周期で出力された。次は 2026-08-02T12:06:43Z より前の最新の
`shsite` 抜粋である。

```text
shsite mv.op=850 mv.opR=374 mv.am=102 mv.ab=646 mv.gm=102 mv.gmR=8874 mv.gaR=34 mv.pbrR=136
```

`mv.am` が非ゼロで、`rest.am` と `fb.*` はこの観測にない。従って shadow multiview
フル経路は **VERIFIED**。ただし、これは visual acceptance の代替ではない。

### P0 / 安定性計器

10 秒周期 `FRAMETIME ms:` は run 全体で 50 行出た。p95 の全範囲は
50.92--216.34 ms、p99 の全範囲は 52.40--302.82 ms である。下表は、性能が悪化した
12:02 UTC の連続観測と、終了直前の観測を併記する。従来記載した 12:06 UTC の 4 行だけを
この run 全体の P0 値として扱ってはならない。

| 時刻 (UTC) | avg ms | p95 ms | p99 ms | max ms |
| --- | ---: | ---: | ---: | ---: |
| 12:02:17 | 52.54 | 179.74 | 211.06 | 213.73 |
| 12:02:27 | 43.43 | 155.13 | 168.73 | 187.79 |
| 12:02:37 | 41.74 | 149.58 | 170.97 | 223.34 |
| 12:02:47 | 46.64 | 150.58 | 157.11 | 161.22 |
| 12:08:49 | 41.89 | 111.67 | 118.61 | 135.39 |
| 12:08:59 | 37.62 | 111.90 | 138.67 | 143.66 |
| 12:09:09 | 34.60 | 102.12 | 123.68 | 286.99 |

`recreateSwapchain` は継続して出ており、理由はいずれも
`acq-suboptimal,present-suboptimal` だった。これは **OPEN** であり、通常の受入結果と
混同しない。直近例:

```text
2026-08-02T12:06:42Z recreateSwapchain: reason=acq-suboptimal,present-suboptimal old=2560x1387 new=2560x1387 frames_since_last=30 drain_us=0 wait_idle_us=37 ok=1
```

### 警告・未完了項目

- `VUID` とログレベル `ERROR` は検出していない。ただし 12:09:10 UTC に
  `PresentEngine: device lost`、続いて `GPU device lost (VK_ERROR_DEVICE_LOST)` が出て
  graceful shutdown した。したがって、この run の安定性は **FAIL** であり、
  「device lost なし」と報告してはならない。
- `VKGeo stateSort` の `visible empty-drawmap groups` 警告は継続している。
- HTTP 403/503 と、それに続く GLTF material asset 取得失敗、Voice account provisioning
  失敗がある。これらは network / asset 配信系の警告として分離し、shadow 経路の合否には
  用いない。
- 視覚確認は [§5](#5-視覚確認チェックリスト) に記録済み。髪、植生・木の葉、金網、草、
  カメラ回転、spot light は OK だが、半透明 dither shadow は NG のため、視覚受入は
  **FAIL** である。

## 10. device lost 再現・submit 診断記録 — 2026-08-09

`feature/ayastorm-r42-macos-device-lost-diagnostics` の開発 app で、次の環境変数を付けて
実行した run で device lost を再現した。この branch には Retina でのログイン画面 1/4 表示を
防ぐ修正 (`1bfa09760d`) と、Darwin 限定の submit 履歴診断
(`c43f8d4320`) を含む。

```bash
AYASTORM_VKC=1 AYASTORM_PERF_LOG=5 "$APP/Contents/MacOS/AYAstorm"
```

この run については shader cache / pipeline cache を起動直前に消去した記録がない。そのため
shadow の視覚受入結果には使わず、device lost の診断 run としてだけ扱う。

### 結果

- 起動から約 271 秒後の `2026-08-09T14:41:02Z` に `VK_ERROR_DEVICE_LOST` を再現した。
- 直接失敗したのは `queue-submit` の timeline `57962`、`result=-4`
  (`VK_ERROR_DEVICE_LOST`) である。
- 新設した `VKC-DEVLOST` 診断では、失敗 submit を含む直近 16 件が**すべて
  `one-shot`**だった。timeline `57947`--`57961` は成功、`57962` だけが失敗した。
- `PresentEngine submit failed` の後、Viewer は `requestQuit` を呼び、logout / cleanup を
  完走して `status: stopped` へ到達した。OS による即時 abort ではなく、device lost を検出した
  Viewer の graceful shutdown である。
- 終了直前の P0 は `FRAMETIME ms: avg 101.81, p95 275.99, p99 319.35, max 319.35, n 99`。
  この run の安定性判定は **FAIL**。

`VKC-DEVLOST` の原本は次の Viewer log に残る。

```text
~/Library/Application Support/AYAstorm-dev/logs/AYAstorm.log
```

要点は次のとおり。

```text
VKC-DEVLOST trigger=queue-submit submit_history=16
VKC-DEVLOST submit[14]: type=one-shot result=0  timeline=57961 ...
VKC-DEVLOST submit[15]: type=one-shot result=-4 timeline=57962 ...
PresentEngine: device lost — all further submits skipped (first skipped: is_frame=0 is_oneshot=1)
GPU device lost (VK_ERROR_DEVICE_LOST) — requesting graceful shutdown.
```

### shader / pipeline cache が空の状態での再現 — 2026-08-09

同じ開発用 profile で、起動前に次の 2 項目を確認した。両方とも既に存在せず、profile 内に
別位置の同名項目もなかったため、cache が空の状態であることを確認してから起動した。

```text
~/Library/Application Support/AYAstorm-dev/cache/shader_cache/
~/Library/Application Support/AYAstorm-dev/cache/pipeline_cache.bin
```

起動には再び `AYASTORM_VKC=1 AYASTORM_PERF_LOG=5` を用いた。`15:12:02Z` に Vulkan
presentation surface / `2560x1387` swapchain の成立を確認したが、run time 約 68 秒後の
`15:13:10Z` に再度 `VK_ERROR_DEVICE_LOST` が発生した。

- `VKC-DEVLOST trigger=queue-submit`、失敗は one-shot timeline `16606` (`result=-4`)。
- 直前 15 件 (`16591`--`16605`) もすべて one-shot で、成功していた。
- 最終 P0 行は `15:13:01Z`: `FRAMETIME ms: avg 106.80, p95 237.44, p99 310.23,
  max 310.23, n 75`。
- この場合も `requestQuit` を経由して `status: stopped` まで cleanup した。

したがって shader cache / pipeline cache の残存は、この device lost の必要条件ではない。
ただし cache を消しても device lost を防げないことを示すだけで、one-shot command buffer 内の
どの操作が GPU 異常を引き起こしたかは未確定である。

### 現時点の切り分け

device lost を返した submit と直前 15 件が one-shot であるため、通常 frame submit、swapchain
present、shadow multiview 経路ではなく、one-shot のリソース転送・画像レイアウト遷移・mipmap /
cubemap 処理のいずれかが関与する可能性が高い。ただし GPU の異常は先行コマンドに起因して
後続 submit で検出され得るため、timeline `57962` の command buffer 自体を原因とは断定しない。

MoltenVK / Apple GPU のこの run では GPU breadcrumb と `VK_EXT_device_fault` がともに
`enabled=0` であり、driver 側 fault 情報は取得できない。HTTP 403・asset retry・network timeout
は同時期に出ているが、device lost への因果を示すログはない。

次の再現では one-shot submit ごとに呼び出し元種別（buffer upload / image upload / mipmap /
cubemap / layout transition）と staging bytes を記録し、timeline `57962` / `16606` のような
失敗 submit に対応する操作まで特定する。

### one-shot 発生元を特定した再現 — 2026-08-09

Darwin 限定の発生元・staging byte 診断 (`7f8b2cfd83`) を含む開発 app を、同じ環境変数で起動した。
Viewer log 上の起動時刻は `15:30:17Z`、device lost は約 42 秒後の `15:30:59Z` だった。

- 失敗した submit は timeline `17119`、`type=one-shot`、
  `oneshot_source=image-upload-2d`、`staging_bytes=4096`、`result=-4`
  (`VK_ERROR_DEVICE_LOST`) である。
- 直前 15 件もすべて one-shot で、`image-upload-2d`（各 4,096 bytes）と
  `image-mip-blit` が交互に並んだ。`17104`--`17118` は成功し、`17119` だけが失敗した。
- `image-upload-2d` は静的な呼び出し元として
  `LLImageGL::syncVulkanMip0Image()` から `LLVKLoader::uploadImageDataVk()` へ入る一般的な
  テクスチャ upload 経路である。今回の診断は texture asset ID、画像寸法、format、mip 数を
  出していないため、対象テクスチャ自体は未確定である。
- `image-mip-blit` は `LLVKLoader::generateMipChainBlitVk()` の経路である。最後に
  `image-upload-2d` が失敗を返したことは、当該 upload 自体が原因である証拠にはならない。
  先行 upload / mip blit の GPU 異常が後続 submit で検出される可能性を残す。
- 終了直前の P0 は `15:30:57Z`: `FRAMETIME ms: avg 66.77, p95 171.10, p99 188.98,
  max 342.55, n 121`。この run も `requestQuit` を経由して終了し、安定性判定は **FAIL**。

従って、次の修正・診断対象は swapchain や shadow multiview ではなく、
`LLImageGL::syncVulkanMip0Image()` -- `uploadImageDataVk()` -- `generateMipChainBlitVk()` の
小規模テクスチャ upload / mipmap 作成経路である。次の診断では image handle、幅・高さ、format、
mip 数および texture asset ID を submit 履歴へ加え、対象リソースを確定してから layout と
GPU 完了前の寿命管理を修正する。

## 11. image upload / mip blit の調査と修正方針 — 2026-08-10

### 結論

`image-upload-2d` と `image-mip-blit` が交互に記録されたこと自体は、device lost の原因確定を
意味しない。Present Engine は one-shot job に単調な timeline 値を振り、単一 graphics queue へ
FIFO で submit するため、通常の `upload -> mip blit` の順序は維持される。

今回の履歴は、先行 texture の mip blit の後に次 texture の upload が続く形であり、timeline
`17119` の `image-upload-2d` が `VK_ERROR_DEVICE_LOST` を返した。Vulkan / MoltenVK では先行する
GPU command の異常が後続 submit で表面化し得るため、最後の upload だけを原因とは断定しない。

### VERIFIED: 修正すべき仕様不備

`LLVKLoader::generateMipChainBlitVk()` は `VK_FILTER_LINEAR` 用に
`VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT` だけを検査している。しかし
`vkCmdBlitImage` には source の `VK_FORMAT_FEATURE_BLIT_SRC_BIT` と destination の
`VK_FORMAT_FEATURE_BLIT_DST_BIT` も必要である。

従って次の修正を行う。

1. mip generation 可否を format ごとに `BLIT_SRC`、`BLIT_DST`、および linear filter の全条件で
   判定する。
2. 非対応 format は mip blit command を一切記録せず、最初から 1 mip の texture として確保し、
   non-mip sampler を使う fallback にする。複数 mip を確保したまま未初期化の level を sample
   してはならない。
3. 失敗時の診断には image handle、幅・高さ、format、mip 数、および format feature bits を残す。

これは log の 4 KiB texture が当該非対応 format だったことをまだ示すものではないが、現行コードの
`vkCmdBlitImage` 発行条件が不十分であることは source と Vulkan の valid usage から確認済みである。

### VERIFIED: bindless descriptor の寿命上の危険

Vulkan + bindless が有効な run で、`LLImageGL::updateVkHeapSlot()` は image view または sampler が
変わると、既存 bindless slot を `vkUpdateDescriptorSets()` で即時上書きする。descriptor binding は
`UPDATE_AFTER_BIND` と `UPDATE_UNUSED_WHILE_PENDING` を付けているが、pending command buffer が実際に
その slot を sample する場合、その descriptor を更新してよいことにはならない。

修正は、view または sampler が変わるたびに新しい bindless slot を取得し、旧 slot は既存の
`bindlessReleaseSlotDeferred()` 経路で GPU 完了後に解放することとする。これにより既に submit 済みの
frame は旧 slot と旧 image view を維持し、以後 record する frame だけが新 slot を参照する。

これは device lost の直接原因としては未確定だが、今回の run では bindless heap が有効であり、texture
streaming 中にこの更新が起きるため、format feature 修正と同じ優先度で是正する。

### 構造改善: upload と mip chain を一つの one-shot に統合する

現状は `syncVulkanMip0Image()` が base level upload を submit した直後、別 command buffer / 別
one-shot job として mip chain blit を submit する。FIFO により通常は順序を保てるものの、mip family の
layout 遷移を呼び手内で閉じる規約に対して不必要な中間状態を作る。

auto-generated mip の経路は、次を一つの command buffer に記録して一度だけ submit する。

```text
UNDEFINED -> TRANSFER_DST (mip 0)
copy staging buffer -> mip 0
TRANSFER_DST / TRANSFER_SRC を使った mip 1..N の blit
全 mip -> SHADER_READ_ONLY
```

submit 発生元は `image-upload-mips-2d` として診断に残す。asset 側が既に全 mip を持つ upload、cube /
3D image、readback などは対象外とし、挙動を変えない。

### 補助的な是正と検証

- one-shot の staging byte 上限は、通常の `image-upload-2d` で回収キューに `0` が渡されるため、
  256 MiB の byte 上限が実効していない。今回の直接原因とは未確定だが、実バイト数を保持して
  backpressure を有効化する。
- `sCommandPool` は one-shot と frame command buffer で共有している。Vulkan の command pool は
  host access の外部同期が必要である。現行の静的確認では PE thread は submit 専任であり、この共有が
  本件の race である証拠はない。ただし one-shot を将来 worker から記録する経路がないかを確認し、
  必要なら専用 pool または pool mutex を導入する。

修正順序は、(1) format feature gate と 1 mip fallback、(2) bindless slot の世代分離、(3) upload + mip
統合、(4) staging backpressure と command pool 監査とする。各段階で cache を消去した状態から
`AYASTORM_VKC=1 AYASTORM_PERF_LOG=5` で複数回起動し、device lost / VUID の不在、`#VkPerf#`、
`FRAMETIME ms:`、および通常の texture・shadow 表示を記録する。

## 12. device lost 修正実装と初回実用検証 — 2026-08-09

### 実装済みの変更

`feature/ayastorm-r42-macos-device-lost-repair-wip` で、§11 のうち command pool 監査を除く
修正を次の 3 コミットに分離して実装した。この branch はローカル検証用であり、既存 PR には
まだ含めていない。

| コミット | 内容 |
| --- | --- |
| `5ced502490` | auto-generated mip の base level upload、全 mip の layout 遷移、blit、shader-read 遷移を単一 one-shot command buffer / submit に統合。format feature は `BLIT_SRC`、`BLIT_DST`、linear filter の全条件を確認し、非対応 format は 1 mip に制限する。 |
| `f247b52742` | one-shot deferred-free queue に実際の staging byte 数を引き渡し、256 MiB の backpressure を再び実効させる。byte 数の型を `U64` に揃える。 |
| `d9da0126e9` | image view / sampler の変更時に bindless descriptor を in-place 更新せず、新 slot を取得して旧 slot を GPU 完了後に解放する。 |

この変更により、旧来の `image-upload-2d` と `image-mip-blit` の別 submit は、auto-generated mip
対象では `image-upload-mips-2d` の単一 submit に置き換わる。asset が既に mip を持つ upload、cube / 3D
image、readback は変更対象外である。

### 初回実用検証の記録

同一ソースから arm64 開発 app を build し、shader cache と `pipeline_cache.bin` を削除してから、
次で起動した。

```bash
AYASTORM_VKC=1 AYASTORM_PERF_LOG=5 "$APP/Contents/MacOS/AYAstorm"
```

原本 log は次にある。

```text
~/Library/Application Support/AYAstorm-dev/logs/AYAstorm.log
```

| 確認項目 | 実測結果 |
| --- | --- |
| Vulkan 起動 | `initialized device=Apple M2 Pro`、`Vulkan presentation surface initialized`、`Initializing Login Screen` を順に確認。 |
| 実行時間 | `Run time: 1069.259 seconds`（約 17 分 49 秒）。ユーザー操作による終了後に `status: stopped`。 |
| device lost / VUID | `device lost`、`VK_ERROR_DEVICE_LOST`、`VUID` は検出なし。 |
| GPU shadow 経路 | `#VkPerf#` に `shsite mv.am=...` を確認。観測した同一行に `rest.am` および `fb.*` はなく、alpha-mask の multiview 経路は維持された。 |
| P0 | 終盤は負荷により `FRAMETIME ms:` の p95 が約 190--290 ms、p99 が約 262--308 ms。安定性通過と性能受入を混同しない。 |
| 警告 | HTTP 403 等の asset/network 系 WARNING は継続。今回の log に device lost、VUID、ログレベル ERROR はない。 |

この run は、従来 42--271 秒で再現していた `VK_ERROR_DEVICE_LOST` がこの操作範囲では再現せず、
修正コードを**実用検証へ進める候補として採用できる**根拠である。一方で、単一 run が GPU/driver
問題の恒久解決を証明するものではない。安定性の最終受入は次を満たすまで **OPEN** とする。

1. shader cache / pipeline cache を毎回消去した cache-cold run を少なくとも 3 回行う。
2. 各 run を 10--30 分以上継続し、login、teleport、texture streaming、カメラ回転を含める。
3. 各 run の log 全体から `device lost` / `VK_ERROR_DEVICE_LOST` / `VUID` / ERROR の有無、`#VkPerf#`、
   `FRAMETIME ms:`、`recreateSwapchain` を記録する。
4. macOS の結果を Linux / Windows の実行証拠とみなさず、共有 Vulkan TU に触れる本修正は各
   プラットフォーム担当者が build と Vulkan run を別途確認する。
