# AYAstorm r32 3D Stream Channel Reader Tail 修正報告書

## 概要

AYAstorm r32 の 3D Stream linkset / multi-speaker 再生で、軽い処理落ち後にチャンネルごとの再生位置がズレ、`Stream3DEnabled` の off/on でのみ回復する事象を調査した。

結論として、主原因は `LLPositionalStreamMulti` の speaker 別 `pcmReadCallback()` で underrun が起きた際、zero-fill した speaker の logical reader tail が再生時間どおりに進まず、次回 callback で古い PCM を後追い再生できてしまう点にあると判断した。

あわせて、子プリムの 3D Stream 指定で `[3dstream:{ch:SR}{volume::1}{range:30}]` のような malformed volume を指定した場合にクラッシュする経路も確認し、数値 parse と parse error handling を修正した。

## 対象

- base branch: `fix/ayastorm-r32-3dstream-url-buffer`
- work branch: `fix/ayastorm-r32-3dstream-channel-reader-tail`
- document: `docs/specs/ayastorm-r32-3dstream-channel-reader-tail-fix-plan.ja.md`

修正対象ファイル:

- `indra/llaudio/llpositionalstreammulti.cpp`
- `indra/llaudio/llpositionalstreammulti.h`
- `indra/newview/llpositionalstreammgr.cpp`
- `indra/newview/llpositionalstreammgr.h`

## 報告された事象

- FL と M などを距離を離して配置すると、チャンネル間で聴こえ方がズレる。
- 3D Stream を enable し直すとズレが直る。
- enable 直後と、アバターまたはカメラを動かした後で聴こえ方が変わる。
- 軽い処理落ちでもチャンネルごとのズレが発生する。
- 子プリムに `[3dstream:{ch:SR}{volume::1}{range:30}]` と記述するとクラッシュする。

`Stream3DEnabled` の off/on で直る点から、距離減衰や距離による音速遅延そのものではなく、3D Stream 内部の per-channel reader state または FMOD channel state がリセットされることで回復している可能性が高い。

## 診断結果

### 1. Channel reader tail drift

3D Stream の linkset multi-speaker 再生は、speaker ごとに別 HTTP stream を開く構造ではない。`LLPositionalStreamMulti` が 1 source stream を decode thread で読み、shared multi-tail ring に PCM を書き込み、各 speaker callback が同じ ring から speaker 別 reader tail で mono PCM を読む。

処理落ちなどで特定 speaker の callback が必要 frame 数を ring から読めなかった場合、従来は不足分を zero-fill していた。ただし reader tail は実際に読めた分しか進まないため、不足分に相当する再生時間が logical tail に反映されない。その結果、次回 callback で古い PCM を読む余地が残り、その speaker だけが後追い再生になり得る。

この状態は、3D Stream を enable し直して reader tail と FMOD channel を作り直すまで固定化される可能性がある。

### 2. FMOD start sync diagnostics

開始時は paused channel を同じ future DSP clock に `setDelay()` し、その後 `setPaused(false)` して同時 start させている。今回、`getDSPClock()`、`setDelay()`、`setPaused()` の診断ログを追加し、start scheduling が成功しているか追えるようにした。

現時点の実装は診断ログ追加までで、`setDelay()` または `setPaused(false)` 失敗時に start を中止または retry する hardening は未実装である。これは残課題として扱う。

### 3. Malformed volume crash

`[3dstream:{ch:SR}{volume::1}{range:30}]` は `volume` が malformed であり、本来は parse error として child speaker を無効化すべき入力である。

従来の数値 parse は部分 parse を許容していたため、不正な文字列が後段へ進む余地があった。今回、数値文字列は全体が消費された場合のみ valid とし、root / child それぞれの parse error を明示的に処理するようにした。

子プリム側で parse error が起きた場合は、該当 child だけを skip し、linkset 全体の評価は継続する。root 側で parse error が起きた場合は、binding を teardown して通知する。

## 実装内容

### LLPositionalStreamMulti

- `SpeakerCallback` に `catchup_frames` を追加。
- speaker callback の underrun 時、zero-fill した不足 frame を logical playback time として記録。
- 次回 callback で `catchup_frames` 分を `mRing.skipFrames()` により消費し、古い PCM を後追い再生しないようにした。
- catch-up に必要な ring frame が不足している場合は、さらに silence を返して catch-up を継続する。
- upmix speaker についても、catch-up / underrun 時に silence で state を進める。

追加した主な診断ログ:

- `Stream3D sync diag: scheduling multi start`
- `Stream3D sync diag: setDelay`
- `Stream3D sync diag: unpause`
- `Stream3D sync diag: speaker underrun`
- `Stream3D sync diag: reader catch-up`
- `Stream3D sync diag: silent reader short-skip`

### LLPositionalStreamMgr

- `tryParseFloat()` を、部分 parse 許容から full parse 必須へ変更。
- `distParseErrorToNotifyKind()` を追加し、parse error と notification kind の対応を共通化。
- `evaluateBinding()` で parse error を明示的に notification kind へ変換。
- `evaluateLinkset()` で root parse error と child parse error を分けて処理。
- malformed child tag は該当 child のみ skip し、クラッシュ経路に入らないようにした。

## ログで確認できること

追加診断ログにより、以下を runtime log から確認できる。

| 確認したい内容 | 見るログ |
| --- | --- |
| multi-speaker の同時 start scheduling | `Stream3D sync diag: scheduling multi start` |
| FMOD delay 設定の成否 | `Stream3D sync diag: setDelay` |
| FMOD unpause の成否 | `Stream3D sync diag: unpause` |
| speaker 別 underrun の発生 | `Stream3D sync diag: speaker underrun` |
| underrun 後の reader tail catch-up | `Stream3D sync diag: reader catch-up` |
| catch-up frame 不足による silence 継続 | `Stream3D sync diag: silent reader short-skip` |
| malformed root tag | `[3dstream-stereo] parse error on root` |
| malformed child tag | `[3dstream-stereo] parse error on child` |

「処理落ち後にチャンネルごとのズレが発生するか」は、`speaker underrun` の後に `reader catch-up` が出ているかで確認できる。修正後は、underrun した speaker が古い PCM を後追い再生せず、catch-up または silence によって logical playback time を維持する。

`[3dstream:{ch:SR}{volume::1}{range:30}]` については、`parse error on child` と `BadVolume` 系 notification が出て、該当 child が skip されることを確認する。

## 検証結果

arm64 app の差分ビルドは成功した。

実行した build:

```sh
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild -project build-darwin-universal/SecondLife.xcodeproj build -configuration Release -target ayastorm-bin -parallelizeTargets -jobs 8 -hideShellScriptEnvironment ARCHS=arm64 ONLY_ACTIVE_ARCH=YES
```

生成物:

```text
/Users/takayukinoami/Desktop/WorkNOW/Firestorm_Develop/phoenix-firestorm-mayatonton/build-darwin-universal/newview/Release/AYAstorm.app
```

確認結果:

```text
build-darwin-universal/newview/Release/AYAstorm.app/Contents/MacOS/AYAstorm: Mach-O 64-bit executable arm64
```

app timestamp:

```text
Jun  8 22:53:07 2026 build-darwin-universal/newview/Release/AYAstorm.app
```

universal / x86_64 build は、`libopus.dylib` が arm64-only のため link できず未完了である。これは今回修正とは別件の build environment / dependency 問題として扱う。

## 残課題

- FMOD `setDelay()` / `setPaused(false)` 失敗時に、start を中止するか retry する hardening。
- linkset description の partial snapshot により一時的に child channel が欠落するケースの settle window 検討。
- 実 region で FL / M / SR などを配置し、負荷をかけた状態で underrun 後に恒久的な channel drift が再発しないことの確認。
- malformed tag `[3dstream:{ch:SR}{volume::1}{range:30}]` でクラッシュせず、parse error 通知と child skip になることの runtime 確認。

## 受け入れ確認手順

1. 正常 tag で 3D Stream を enable し、FL / M など複数 channel が同時に鳴ることを確認する。
2. Runtime log で `scheduling multi start`、`setDelay`、`unpause` が各 speaker に出ることを確認する。
3. 軽い処理落ちを発生させ、`speaker underrun` 後に `reader catch-up` または `silent reader short-skip` が出ることを確認する。
4. 処理落ち後も channel drift が固定化せず、3D Stream off/on なしで同期感が維持されることを確認する。
5. 子プリムに `[3dstream:{ch:SR}{volume::1}{range:30}]` を設定し、クラッシュせず `parse error on child` と `BadVolume` 系通知になることを確認する。
6. malformed child が skip されても、他の正常 child speaker が再生継続することを確認する。

## 結論

今回の修正は、3D Stream multi-speaker の処理落ち後に channel reader tail がズレる問題と、malformed `volume::1` 指定によるクラッシュ経路の両方を対象にしている。

arm64 app の build は成功済みであり、次の確認ポイントは実環境での runtime log と聴感確認である。ログ上は、start scheduling、underrun、catch-up、parse error の各観測点を追える状態になっている。
