# AYAstorm r26 — リリース告知

GitHub release ページ貼り付け用の文案。**r26 は MOAP (Media-on-a-Prim) の音声を 3D Stream の speaker routing に流せるようにするリリース** — 既存の `{ch:L}/{ch:R}/{ch:FL}/...` ベースの分散ステレオ・5.1 配置・HRTF・venue reverb・occlusion を、HTTP URL ではなく MOAP 面を音源にしてもそのまま利用できるようになります。

実装方針、現行仕様の整理、接続設計、検証計画の詳細は永続資料 (`docs/ayastorm-r26-moap-3d-stream-implementation-plan.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r26 — MOAP 音声を 3D Stream に接続

### r26 の柱: MOAP の音を 3D Stream の speaker prim から鳴らせるようにする

これまで AYAstorm の 3D Stream は HTTP オーディオストリーム (Icecast / SHOUTcast) を URL で渡す形だけを音源として扱っていました。MOAP の音は別経路 (`LLMediaAudioStream` 経由の 2D FMOD 再生) で鳴っており、3D Stream 側の speaker prim 分配・5.1 routing・HRTF・venue reverb・occlusion・volume 制御は一切経由しませんでした。

r26 では `LLPositionalStreamMulti` に **URL source と並ぶ PCM ring source** を新設し、CEF/Dullahan の audio callback が shared memory ring に書いた float PCM をそのまま 3D Stream の入力源として扱えるようにしました。MOAP video は従来どおり MOAP 面に表示しつつ、音声だけを 3D Stream の speaker prim から鳴らす、という構成が **配信者がタグを書くだけ** で組めるようになります。

### 仕組み

ルート Description に `{source:media}` (2ch) または `{source:media-5-1}` (6ch) を書くと、3D Stream は URL ではなくリンクセット内の MOAP 面を音源として bind します。スピーカープリムは従来どおり `{ch:L}/{ch:R}/{ch:FL}/{ch:FR}/{ch:C}/{ch:LFE}/{ch:SL}/{ch:SR}` を持たせるだけです。

```
ルート Description:
  [3dstream-stereo:{source:media}{ch:L}]

子 Description:
  [3dstream-stereo:{ch:R}]
```

```
MOAP video → CEF/Dullahan audio callback
  → LLPluginAudioRingHeader (shared memory PCM ring, magic="AYAA" v2)
  → LLViewerMediaImpl::getAudioRingForStream3D() ── LL_DULLAHAN_AUDIO_CALLBACK gate
  → LLPositionalStreamMulti (SourceKind::MediaRing)
     ├─ ring 読出 → 既存 LLMultiTailRing
     └─ speaker prim ごとに FMOD_OPENUSER | FMOD_3D mono sound を作って routing
  → 既存の HRTF / venue reverb / occlusion / volume / 5.1 routing をそのまま通す
```

- callback bus の実 channel 数 (CEF は 8ch 固定で返してくる) と **論理 source channel 数** (`{source:media}`=2 / `{source:media-5-1}`=6) を分離。2ch の MOAP に対する `{upmix:on}` も従来どおり機能します
- MOAP 面が複数あるリンクセットでは、ルートで `{link:N}{face:M}` を書いて bind 対象を指定。1 つしかない場合は省略可
- 同じリンクセットで `{url}` と `{source:media}` は相互排他。media 面を見せつつ URL を 3D 化する構成も従来どおり可能 (この場合 MOAP 音声は通常の 2D media として鳴ります)
- plugin 破棄時の crash 安全性: `destroyMediaSource()` → `onMediaSourceDestroying()` → `setMediaRingFor3DStream(nullptr)` → `stopDecodeThread()` (join) → ring pointer swap の順で、共有メモリが unmap される前に decode worker が必ず居なくなることを担保しています
- ring header は毎 pump iteration で magic/version を re-validate。format が変わったら reopen、ring が消えたら setFailed
- r24 で入れた `LL_DULLAHAN_AUDIO_CALLBACK` fallback switch (upstream/fork dullahan 切替) は維持。OFF ビルドでは URL 経路だけ生き、media 経路は無効化されます

### 設定

**通常運用での viewer 設定操作は不要。** 配信者がタグを書くだけで MOAP 音声が 3D Stream にルーティングされます。`{source:media}` / `{source:media-stereo}` / `{source:media-5-1}` / `{link:N}` / `{face:M}` の書き方は [3D Stream タグガイド §6.7](../guides/3dstream-tag-guide.ja.md#67-mediamoap-source-を使う-r26) を参照してください。

音量の扱いは media 面の数で変わります。

- **media 面が 1 つだけ**: その media の volume / mute が source gain として効く (single-media 同等)
- **複数 media 面**: 3D に routed された選択 media は source gain 1.0 として扱い、3D Stream 側の master volume / speaker volume で制御。選択されなかった他の media 面は従来どおりの 2D media volume で鳴る

### 移行ノート

- **配信者**: ルート Description を `[3dstream-stereo:{source:media}{ch:L}]` のように書き換えるだけ。リンクセット内に MOAP 面が複数ある場合のみ `{link:N}{face:M}` を併記
- **listener**: 操作不要。r26 を起動してタグ付きのリンクセットを見るだけで MOAP 音声が 3D Stream の speaker prim から鳴ります。UI の追加・変更はありません
- **streamer-led モデルは継続**: 配信者がタグで決定 → listener はそれを再生するだけ、という r11 以降の方針はそのまま

### 既知の制約

- **7.1 (8ch) speaker routing は出さない**: CEF/Dullahan の callback bus は 8ch 固定で受けますが、タグとして expose するのは `{source:media}` (2ch) と `{source:media-5-1}` (6ch) までです。`BL`/`BR` の enum とコード経路は forward-compat で入れてあり、7.1 routing は r27+ の課題
- **A/V sync**: prebuffer ~43ms (2048 frames@48kHz)、target buffered ~85ms (4096 frames)。動画と音声のずれは MOAP video 側の遅延と打ち消し合う想定で、knob は出していません
- **MOAP page 切替の途中再 bind**: MOAP 側で別ページに遷移して audio format (sample_rate / channels) が変わった場合は ring を reopen します。pump loop で format change を検出して reopen する設計のため、瞬断は発生し得ます
- **Linux / macOS / Windows 実機検証**: AYAstorm 側で merge 後に Linux 実機の最小 stereo MOAP 構成 (root + child) で動作確認、macOS / Windows は次の Release ビルドを切るタイミングで確認します

### 実装概要

- `indra/llcommon/llpluginaudio.h` — 新規 ring header (`LLPluginAudioRingHeader`、magic `0x41594141` "AYAA" / version 2 / max 8ch、`ll_plugin_audio_ring_supported_3d_channel_count()` で 1/2/6/8 ch を許容)
- `indra/llcommon/tests/llpluginaudio_test.cpp` — 新規 unit test (ring サイズ計算 / 対応 ch 数 / 6ch・8ch enum 順)
- `indra/llaudio/llpositionalstreammulti.{h,cpp}` — `SourceKind {Url, MediaRing}` 追加、`startMedia()` / `setMediaRingFor3DStream()` / `pumpMediaRingSource()` / CEF 7.1 channel remap (`kCef71ToStream8 = {0,1,2,3,6,7,4,5}`) を追加。`releaseSpeakerRuntime()` を抽出 (非機能変更)
- `indra/newview/llpositionalstreammgr.{h,cpp}` — `DistSourceKind::{Url, Media}` / `SourceBindingKey` / tag parser に `{source}` / `{link}` / `{face}` を追加。`onMediaSourceDestroying()` / `evaluateLinkset()` / `findMediaFor3DSource()` / `effectiveDistributedStreamVolume()` を追加。`BL`/`BR` を `ChannelKind` に追加
- `indra/newview/llviewermedia.{h,cpp}` — `getAudioRingForStream3D()` / `getStream3DAudioGain()` / `setStream3DAudioRedirected()` を追加 (すべて `LL_DULLAHAN_AUDIO_CALLBACK` ガード下)。`destroyMediaSource()` から `LLPositionalStreamMgr::onMediaSourceDestroying()` を呼んで lifecycle 安全性を担保
- `indra/llplugin/llpluginclassmedia.cpp` — `getAudioData()` に `mPlugin->isRunning()` ガード、`audio_stream_format` plugin message ハンドラ追加
- `indra/media_plugins/cef/media_plugin_cef.cpp` — start/stop/error 時に `audio_stream_format` (sample_rate / channels / max_channels) を送出
- `autobuild.xml` — Dullahan を `v1.26.0-CEF_139.0.40-ayastorm-audio-callback.4` に bump (48kHz / 8ch `GetAudioParameters` override 反映)
- `indra/cmake/FMODSTUDIO.cmake` — AYAstorm FMOD codec plugin が直接呼ぶ libopus を `ll::fmodstudio` interface にぶら下げ (FMOD lib を手動指定するビルド構成でも libopus が落ちないように)
- `indra/newview/CMakeLists.txt` — universal macOS リンクで AYAstorm FMOD codec シンボルが解決できるよう、llaudio の static archive 後に `ll::fmodstudio` を再リンク
- `docs/guides/3dstream-tag-guide.{ja,en,zh}.md` — 「Media/MOAP source (r26)」節を新設 (ja §6.7 / zh §6.9 / en §6.10)、§6.3 表に `{source}` / `{link}` / `{face}` を追加、URL との相互排他・single-media vs multi-media の volume 政策を明記
- `docs/ayastorm-r26-moap-3d-stream-implementation-plan.md` — 新規永続資料 (実装方針 / 現行仕様 / 接続設計 / 実装フェーズ / 検証計画 / 既知の課題)

### Credits

r26 の本実装 (実装計画書 / `LLPluginAudioRingHeader` 設計 / `LLPositionalStreamMulti` の MediaRing source 追加 / CEF plugin の `audio_stream_format` 送出 / `LLViewerMediaImpl` の 3D Stream 接続 / 3 言語タグガイド更新 / 単体テスト / Dullahan callback.4 bump / FMOD codec link 修正) は [t-noami](https://github.com/t-noami) さんによるものです。

AYAstorm 側では PR をそのまま取り込み、本リリースノート (3 言語) のみを追加しています。

### ドキュメント

- r26 実装計画書 / 現行仕様の整理 / 接続設計 / 実装フェーズ / 検証計画 / 既知の課題: [`docs/ayastorm-r26-moap-3d-stream-implementation-plan.md`](../ayastorm-r26-moap-3d-stream-implementation-plan.md)
- 3D Stream タグガイド (`{source:media}` の使い方は §6.7): [`docs/guides/3dstream-tag-guide.ja.md`](../guides/3dstream-tag-guide.ja.md)
- r24 の `LL_DULLAHAN_AUDIO_CALLBACK` fallback switch (upstream/fork dullahan 切替): [`docs/release/ayastorm-r24-release-note.ja.md`](./ayastorm-r24-release-note.ja.md)
