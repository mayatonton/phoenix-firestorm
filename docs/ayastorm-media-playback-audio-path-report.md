# AYAstorm MOAP media playback and audio path report

検証ブランチ: `feature/media-playback-validation-release`

検証日: 2026-05-15

## 目的

現在の AYAstorm / Firestorm 系コードで、MOAP (Media on a Prim) / Shared Media / Web page media の再生と音声がどの経路を通っているかを整理する。

ここで主対象にする「メディア再生」は、基本的に Web ページや動画をプリム面に表示する MOAP 系の media playback を指す。

次の経路は、MOAP の位置づけを明確にするための比較対象として確認した。

- Parcel Music / Streaming Music
- Media on a Prim / Shared Media / Browser media
- AYAstorm 3D Stream
- それぞれの音量制御と macOS での注意点

この報告は現時点ではコード調査ベースであり、実機再生テスト結果ではない。

## 開発計画の柱

この調査を踏まえた開発計画は、次の 2 段階に分けて扱う。

### Phase 1: macOS の MOAP 音量制御を有効化する

目的は、macOS で MOAP / Web media の音量を Viewer 側から調整できるようにすること。

現状では、macOS の CEF media plugin が `mac_volume_catcher_null.cpp` を使っているため、CEF 経路の音声に対する Media volume が実際の出力音量へ反映されない可能性が高い。

この段階では、音声を 3D Stream に接続することはしない。まずは従来の MOAP 音声経路のまま、Viewer の Media volume / mute が macOS でも正しく効く状態を目標にする。

対象:

- `media_plugin_cef` の macOS 音量制御
- `AudioLevelMedia`
- `AudioStreamingMedia`
- `MuteMedia`
- Web page / HTML5 video / HTML5 audio / YouTube などの CEF 経路

### Phase 2: MOAP 音声を 3D Stream 経路へ接続する

目的は、MOAP で再生される Web media の音声を、AYAstorm の 3D Stream と同じようにプリムスピーカーから鳴らせるようにすること。

現状の MOAP 音声は media plugin process から OS audio device へ直接出ており、FMOD の Stream3D bus には入っていない。

この段階では、MOAP 音声を viewer 側で受け取り、既存の `LLPositionalStreamMulti` / Stream3D speaker routing に接続できる構造を検討する。

対象:

- media plugin からの音声取り出し方法
- Viewer / FMOD 側への PCM 受け渡し
- MOAP source と speaker prim の対応付け
- 既存 Stream3D の range / volume / position / speaker role との統合
- 従来の media plugin 音声出力との排他、または併用設計

## 結論

このコードベースでは、「音楽・メディア再生」と呼ばれ得るものが少なくとも 3 系統に分かれている。

1. Parcel Music は FMOD の `LLStreamingAudio_FMODSTUDIO` で再生される。
2. MOAP / Shared Media / Web / Movie は `SLPlugin` 経由の media plugin で再生される。
3. AYAstorm 3D Stream は `LLPositionalStreamMgr` と `LLPositionalStreamMulti` による独自の FMOD 3D 経路で再生される。

今回の主対象である MOAP / Web media の音声は、現状では FMOD の Stream3D bus には入っていない。

そのため、Media on a Prim の音声は AYAstorm 3D Stream の upmix、LFE、venue reverb、speaker placement とは別系統である。

開発上は、まず macOS でこの既存 MOAP 音声経路の volume control を直し、その次に MOAP 音声を Stream3D 側へ接続する、という順序が妥当である。

## 全体構造

大きく見ると、音声経路は次のようになる。

```text
Parcel Music URL
  -> LLViewerParcelMgr / LLViewerMedia
  -> LLViewerAudio::startInternetStreamWithAutoFade()
  -> LLAudioEngine::startInternetStream()
  -> LLStreamingAudio_FMODSTUDIO
  -> FMOD 2D stream channel
  -> OS audio device

MOAP / Shared Media / Web / Movie URL
  -> LLViewerMediaImpl
  -> MIME 判定
  -> LLPluginClassMedia
  -> SLPlugin
  -> media_plugin_cef or media_plugin_libvlc
  -> plugin process audio output
  -> OS audio device

AYAstorm 3D Stream tag
  -> prim Description
  -> LLPositionalStreamMgr
  -> LLPositionalStreamMulti
  -> FMOD source stream + OPENUSER per-speaker channels
  -> Stream3D FMOD channel group
  -> OS audio device
```

## 比較対象: Parcel Music / Streaming Music

Parcel Music は通常の media plugin 経由ではなく、デフォルトでは FMOD ネイティブのストリーミング実装を使う。

初期化では `UseMediaPluginsForStreamingAudio` が false の場合、FMOD の実装が選ばれる。

関連箇所:

- `indra/newview/llstartup.cpp`
  - `UseMediaPluginsForStreamingAudio == false` なら `createDefaultStreamingAudioImpl()` を使用
- `indra/llaudio/llaudioengine_fmodstudio.cpp`
  - `LLAudioEngine_FMODSTUDIO::createDefaultStreamingAudioImpl()`
  - `LLStreamingAudio_FMODSTUDIO` を生成
- `indra/llaudio/llstreamingaudio_fmodstudio.cpp`
  - `LLStreamingAudio_FMODSTUDIO::start()`
  - `FMOD::System::createStream()`
  - `FMOD::System::playSound()`

Parcel Music の開始は主に次の経路で行われる。

```text
LLViewerParcelMgr::optionallyStartMusic()
  -> LLViewerAudio::startInternetStreamWithAutoFade()
  -> LLAudioEngine::startInternetStream()
  -> LLStreamingAudio_FMODSTUDIO::start()
```

`LLStreamingAudio_FMODSTUDIO` は `FMOD_2D | FMOD_NONBLOCKING | FMOD_IGNORETAGS` で `createStream()` している。つまり Parcel Music は 2D のストリーミング音声であり、スピーカー位置を持たない。

音量は `LLViewerAudio::audio_update_volume()` から `gAudiop->setInternetStreamGain()` に入り、最終的に FMOD channel volume に反映される。

## Media on a Prim / Shared Media

MOAP やブラウザ、動画再生は `LLViewerMediaImpl` が入口になる。

関連箇所:

- `indra/newview/llviewermedia.cpp`
  - `LLViewerMediaImpl::newSourceFromMediaType()`
  - `LLViewerMediaImpl::initializePlugin()`
  - `LLViewerMediaImpl::updateVolume()`
  - `LLViewerMediaImpl::preMediaTexUpdate()`
- `indra/llplugin/llpluginclassmedia.cpp`
  - `LLPluginClassMedia::init()`
  - `LLPluginClassMedia::setVolume()`
  - shared memory texture update handling
- `indra/media_plugins/cef/media_plugin_cef.cpp`
- `indra/media_plugins/libvlc/media_plugin_libvlc.cpp`

MIME type から media plugin が選ばれる。

macOS の `mime_types_mac.xml` では、おおむね次の割り当てになっている。

- `text/html`, `text/plain`, `image/*` など: `media_plugin_cef`
- `audio/*`, `video/*`, `video/mp4`, `video/quicktime` など: `media_plugin_libvlc`
- default implementation: `media_plugin_cef`

映像は media plugin が shared memory にピクセルを書き、viewer 側が dirty rect を拾って media texture に転送する。

音声はこの shared memory 経路ではなく、media plugin process 側の CEF / libVLC がそれぞれ OS audio device へ出す。

## MOAP 音声の距離処理

`LLViewerMediaImpl::updateVolume()` には `mProximityCamera` による距離減衰がある。

これは `MediaRollOffMin`, `MediaRollOffMax`, `MediaRollOffRate` を使って、media plugin に渡す volume を viewer 側で計算する仕組みである。

ただし、これは FMOD の 3D channel ではない。実体は media plugin process の音声出力であり、viewer 側で計算した音量を plugin に `set_volume` で渡している。

したがって、MOAP 音声は「距離で音量が変わる」ことはあっても、AYAstorm 3D Stream のような per-speaker placement、upmix、LFE、venue reverb の対象ではない。

## Media plugin 別の音声経路

### CEF

CEF plugin の音量制御は次の流れ。

```text
LLViewerMediaImpl::updateVolume()
  -> LLPluginClassMedia::setVolume()
  -> media time message: set_volume
  -> MediaPluginCEF::setVolume()
  -> VolumeCatcher::setVolume()
```

ただし macOS では `media_plugins/cef/CMakeLists.txt` で `mac_volume_catcher_null.cpp` が使われている。

`mac_volume_catcher_null.cpp` は値を保持するだけで、実際の AudioUnit / OS 出力音量へ反映しない null 実装である。

このため、macOS の CEF media audio については、Viewer の Media volume が実音量に効かない可能性がある。これは実機で確認すべき重要ポイントである。

### libVLC

libVLC plugin の音量制御は次の流れ。

```text
LLViewerMediaImpl::updateVolume()
  -> LLPluginClassMedia::setVolume()
  -> media time message: set_volume
  -> MediaPluginLibVLC::setVolume()
  -> libvlc_audio_set_volume()
```

`media_plugin_libvlc.cpp` では `libvlc_audio_set_volume()` が呼ばれているため、動画ファイルや `video/mp4` / `audio/*` のような libVLC 経路では plugin 内で音量が反映される構造になっている。

Windows では追加で `waveOutSetVolume()` の回避処理があるが、macOS では libVLC の音量 API が主経路になる。

## Streaming audio の media plugin fallback

`UseMediaPluginsForStreamingAudio` を true にすると、Parcel Music も `LLStreamingAudio_MediaPlugins` 経由に切り替わる可能性がある。

関連箇所:

- `indra/newview/llviewermedia_streamingaudio.cpp`
- `LLStreamingAudio_MediaPlugins::start()`

この場合は `audio/mpeg` として media plugin を初期化し、`LLPluginClassMedia` 経由で再生する。

ただし現在の設定デフォルトは `UseMediaPluginsForStreamingAudio = false` なので、通常の Parcel Music は FMOD 経路である。

## 接続先候補: AYAstorm 3D Stream

AYAstorm 3D Stream は通常の MOAP / media plugin ではない。

ただし Phase 2 では、MOAP 音声を接続する先の候補になる。

入口は prim Description の `[3dstream:...]` / `[3dstream-stereo:...]` / legacy `[ayastream:...]` 系タグである。

関連箇所:

- `indra/newview/llselectmgr.cpp`
  - ObjectProperties 受信時に Description を `LLPositionalStreamMgr` へ渡す
- `indra/newview/llappviewer.cpp`
  - frame update で `LLPositionalStreamMgr::update()` を呼ぶ
- `indra/newview/llpositionalstreammgr.cpp`
  - Description scan、tag parse、binding 管理
- `indra/llaudio/llpositionalstreammulti.cpp`
  - FMOD source stream、per-speaker OPENUSER channel
- `indra/llaudio/llaudioengine_fmodstudio.cpp`
  - `Stream3D` channel group と venue reverb DSP

構造は次の通り。

```text
prim Description
  -> LLPositionalStreamMgr::onObjectPropertiesReceived()
  -> tag parse / linkset binding
  -> LLPositionalStreamMulti::start()
  -> FMOD::System::createStream(source)
  -> source PCM read
  -> per-speaker OPENUSER FMOD::Sound
  -> per-speaker FMOD::Channel
  -> set3DAttributes / set3DMinMaxDistance
  -> Stream3D channel group
```

`LLPositionalStreamMulti` は source stream をそのまま FMOD channel として鳴らすのではなく、source を読み出し、各 speaker 用の OPENUSER sound / channel に分配する。

このため、speaker ごとの位置、range、volume、upmix、LFE、occlusion、lite-HRTF、venue reverb を扱える。

## 音量カテゴリ

UI 上の音量カテゴリは、おおむね次のように分かれている。

| UI / setting | 主な対象 | 主経路 |
| --- | --- | --- |
| `AudioLevelMusic` / `AudioStreamingMusic` | Parcel Music | FMOD streaming audio |
| `AudioLevelMedia` / `AudioStreamingMedia` | MOAP / Browser / Movie | media plugin |
| `Stream3DVolumeMaster` / `Stream3DEnabled` | AYAstorm 3D Stream | FMOD Stream3D channel group |
| SFX / UI / Ambient | 通常の viewer sounds | FMOD audio engine |
| Voice | voice client | 別系統 |

## 現時点の注意点

### この報告の主対象は MOAP

Parcel Music と既存 AYAstorm 3D Stream は、この報告では主対象ではない。

Parcel Music は、MOAP と異なる既存ストリーミング音楽経路を説明するための比較対象である。

AYAstorm 3D Stream は、将来的に MOAP 音声をプリムスピーカーへ接続するための接続先候補として扱う。

### macOS CEF media volume

macOS では CEF volume catcher が null 実装である。

そのため、HTML5 video、Web audio、YouTube のような CEF 経路の media audio では、viewer の Media volume が期待通り効かない可能性がある。

libVLC 経路の動画 / 音声は `libvlc_audio_set_volume()` があるため、CEF とは別に検証する必要がある。

### 現状の MOAP と 3D Stream は混ぜて考えない

MOAP media audio は media plugin process の音声であり、Stream3D bus には入らない。

そのため、現状の AYAstorm 3D Stream 機能を検証する場合、MOAP の Web/Video 再生ではなく、prim Description の `[3dstream...]` タグ経路を使う必要がある。

一方、Phase 2 の開発では、この分離されている MOAP 音声経路を Stream3D 側へ接続することを目標にする。

### Parcel Music と 3D Stream も別系統

Parcel Music は FMOD だが 2D streaming music である。

FMOD を使っている点は共通だが、Stream3D の per-speaker 3D channel group とは別である。

## 実機検証で見るべき項目

次のケースを分けて確認するのがよい。

1. MOAP text/html / YouTube / HTML5 audio
   - `media_plugin_cef` 経路になる想定
   - macOS で Media volume が効くか
   - `mac_volume_catcher_null.cpp` の影響を確認

2. MOAP video/mp4
   - macOS では `media_plugin_libvlc` 経路になる想定
   - Media volume が効くか
   - `libvlc_audio_set_volume()` 経路のログ追加も検討

3. Parcel Music
   - `AudioStreamingMusic` on/off
   - Music volume が効くか
   - `AudioImpl`, `AudioEngine`, `ParcelMgr` ログ

4. AYAstorm 3D Stream
   - `[3dstream-stereo:...]` タグの検出
   - `Stream3D` ログ
   - speaker channel / range / volume
   - `Stream3DVolumeMaster`
   - upmix / LFE / venue reverb / occlusion の有無

## 追加調査候補

- Phase 1: macOS の CEF 音量制御を null 実装のままにするか、別の音量制御経路を実装するか。
- Phase 2: MOAP media audio を Stream3D へ載せるために、media plugin audio をどの段階で取り出すか。
- Phase 2: 取り出した MOAP 音声を `LLPositionalStreamMulti` 相当の speaker fan-out に流すか、別の MOAP 専用 3D media stream wrapper を作るか。
- Phase 2: MOAP の従来音声出力を mute し、Stream3D 側だけを鳴らす制御が必要か。
- `UseMediaPluginsForStreamingAudio` を有効にした場合の Parcel Music 挙動。現在の通常経路とは異なるため、必要なら別テストにする。
- CEF と libVLC で Media volume の効き方が一致しているか。
