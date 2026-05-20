# AYAstorm r26 media-5-1 callback routing fix

作成日: 2026-05-21

対象:

- 3D Stream media source
- `{source:media-5-1}`
- Dullahan / CEF audio callback が 8ch bus を返す macOS build

## 背景

Dullahan audio callback package `.5` 適用後、CEF audio callback は viewer log 上で次のように見える。

```text
CEF audio stream started: 48000 Hz x 8 ch (ring max 8 ch)
Media multi source ready: ... 48000 Hz x 6 logical ch (callback bus=8 ch), layout=FL/FR/C/LFE/SL/SR
```

この状態で `[3dstream-stereo:{source:media-5-1}]` を使うと、FL / FR は鳴るが、SL / SR が speaker prim に出ない症状が出た。

C / LFE については、今回確認した `https://mp3-proxy.onrender.com/http%3A%2F%2Fgo%2Dstream%2Dlive%2Ecom%3A8030%2Fstream` の約 12 秒サンプルでは source 側の channel 3 / 4 が完全な無音だった。そのため、この症状とは別問題として扱う。

## 原因

`LLPositionalStreamMulti::pumpMediaRingSource()` が、callback bus が 8ch の場合に media-5-1 の SL / SR を CEF 7.1 layout の index 6 / 7 から読んでいた。

修正前の考え方:

```text
CEF 7.1 callback layout: FL / FR / C / LFE / BL / BR / SL / SR
3D Stream internal 8ch: FL / FR / C / LFE / SL / SR / BL / BR
media-5-1 SL/SR <- callback index 6/7
```

これは full 7.1 source では必要な並べ替えだが、今回の WebAudio media-5-1 経路には合わない。

`vj_051726_A.html` / `vj_051726_B.html` は `ChannelSplitter` から `ChannelMerger` へ channel 0..N をそのまま接続する。`source:media-5-1` では viewer 側の logical source channel 数を 6ch として扱うため、callback bus が 8ch でも、3D Stream が読むべき 5.1 logical channel は 0..5 である。

つまり、この構成では次のように扱う。

```text
media-5-1 on 8ch callback bus: FL / FR / C / LFE / SL / SR / unused / unused
```

修正前は SL / SR を `6/7` から読んでいたため、WebAudio が `4/5` に出した SL / SR を取り逃がしていた。

## 修正

`indra/llaudio/llpositionalstreammulti.cpp` の media ring copy で、6ch logical media source は callback bus channel 0..5 をそのまま読むようにした。

8ch logical source の場合だけ、CEF 7.1 layout から 3D Stream internal 8ch layout へ並べ替える。

修正後の rule:

| 条件 | 読み方 |
| --- | --- |
| `mSourceChannels == 1` | ch0 を全 track へ複製 |
| `mSourceChannels == 2` | ch0 / ch1 をそのまま読む |
| `mSourceChannels == 6` | ch0..ch5 を `FL/FR/C/LFE/SL/SR` としてそのまま読む |
| `mSourceChannels == 8` かつ callback bus 8ch | CEF 7.1 `FL/FR/C/LFE/BL/BR/SL/SR` を internal `FL/FR/C/LFE/SL/SR/BL/BR` へ並べ替える |

実装上の要点:

```text
media-5-1:
  src_channel = c

media-7-1 / full 8ch:
  src_channel = { 0, 1, 2, 3, 6, 7, 4, 5 }[c]
```

URL source path は FMOD decode path のままであり、この修正の影響を受けない。

## 検証

確認済み:

- 対象 branch: `fix/dullahan-audio-callback-5-macos-helper`
- Dullahan package: `v1.26.0-CEF_139.0.40-ayastorm-audio-callback.5`
- `LL_DULLAHAN_AUDIO_CALLBACK=TRUE`
- macOS arm64 viewer build 成功

実行した検証:

```text
xcodebuild -project build-darwin-universal/Firestorm.xcodeproj \
  -configuration Release \
  -target viewer \
  ARCHS=arm64 \
  ONLY_ACTIVE_ARCH=YES \
  build
```

生成 app:

```text
build-darwin-universal/newview/Release/AYAstorm.app
```

補足確認:

- `ffprobe` では対象 stream が Opus 6ch / 48 kHz として認識される。
- `ffmpeg astats` では取得サンプルの channel 3 / 4 が無音、channel 5 / 6 には信号あり。
- そのため C / LFE の無音は source 側の内容に由来する。
- SL / SR の無音は viewer 側の media-5-1 callback bus mapping が原因。

## 注意点

root prim の media 5.1 source 指定:

```text
[3dstream-stereo:{source:media-5-1}{...}]
```

child speaker prim の channel 指定:

```text
[3dstream-stereo:{ch:SL}{range:80}{volume:1.0}]
[3dstream-stereo:{ch:SR}{range:80}{volume:1.0}]
```

`[3dstream:{ch:SR}...]` ではなく、distributed 3D Stream の child prim description は `[3dstream-stereo:...]` を使う。

## 今後の確認

- C / LFE に実信号が入った 5.1 test source で、FL / FR / C / LFE / SL / SR の全 speaker prim を個別確認する。
- `media-7-1` / full 8ch source を使う場合は、CEF 7.1 layout の `BL/BR` と `SL/SR` の並べ替えが必要なため、今回の 6ch rule と混同しない。
