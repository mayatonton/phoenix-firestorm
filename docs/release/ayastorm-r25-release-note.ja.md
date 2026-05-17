# AYAstorm r25 — リリース告知

GitHub release ページ貼り付け用の文案。**r25 は AYAstorm r10.x-bugfix-1 以降で壊れていた parcel music の Ogg Vorbis live stream 再生を修正するリリース** — Opus support を維持したまま、Ogg Vorbis の Icecast live stream を AYAstorm 上でも正しく再生できるようにします。

実装・調査ログ・検証 URL の詳細は永続資料 (`docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md`) に常駐させ、本ノートはそこへの誘導と差分ハイライトに徹します。

---

## AYAstorm r25 — Parcel music Ogg Vorbis live stream 再生修正

### r25 の柱: AYAstorm が壊していた Ogg Vorbis 土地音を取り戻す

AYAstorm は r9-opus 系列で独自の FMOD codec plugin (Ogg Opus 再生用) を追加しました。r10.x-bugfix-1 でこの codec の登録 priority を `0` (FMOD 最優先) に変更し、4-byte "OggS" capture probe で **非 Ogg stream (MP3 など) を早期 reject** することで built-in HTTP codec への影響を回避していました。

ただしこの設計には Ogg Vorbis live stream に対する盲点がありました。Vorbis stream は `OggS` 4-byte gate を通過したあと、custom codec が Ogg 最初の packet を読んで `OpusHead` ではない (`\x01vorbis`) と判定した時点で `FMOD_ERR_FORMAT` を返します。FMOD は次に built-in Vorbis codec に fallback しようとしますが、HTTP live stream は先頭まで巻き戻せないため `FMOD_ERR_FILE_COULDNOTSEEK` で死にます。結果として **r10.x-bugfix-1 から r24 まで、AYAstorm では Ogg Vorbis 土地音が再生不能** でした (vanilla Firestorm では built-in Vorbis が直接処理するため問題なし)。

r25 では custom FMOD codec 自体を **Ogg Opus / Ogg Vorbis 両対応** に拡張し、fallback 経路を踏まない設計に変更しました。

### 仕組み

custom codec が Ogg first packet を一度だけ読み、内容で分岐:

```
parcel music URL
  → LLStreamingAudio_FMODSTUDIO
  → FMOD::System::createStream(url)
  → AYAstorm Ogg Opus/Vorbis codec (priority 0)
     ├─ first packet "OpusHead"  → opus_decoder / opus_multistream_decoder (既存 path)
     └─ first packet "\x01vorbis" → libvorbis decoder (新規 path)
  → PCMFLOAT を FMOD mixer に渡す
```

- non-Ogg stream (MP3 など) は従来どおり 4-byte `OggS` gate で reject、built-in codec が処理
- Vorbis stream は **custom codec 内で完結** するため、fallback 経由の seek failure は発生しない
- Opus mapping family 0 (mono/stereo) / family 1 (multistream 5.1ch surround) の既存 path は変更なし

### 設定

**通常運用での操作は不要。** 診断用に 2 件の cvar を追加しています:

| Cvar | 既定値 | 用途 |
|---|---|---|
| `AYAOpusCodecEnable` | `1` | OFF にすると custom codec を一切登録せず built-in のみで動作。Opus 配信は再生不可になるが、Vorbis 周りの挙動切り分け診断用 |
| `AYAOpusCodecPriority` | `0` (最優先) | FMOD codec dispatch 順の数値。`0` 以外は built-in との順序競合で Opus / Vorbis が壊れる可能性があるため、診断時以外は触らない |

両者とも変更には viewer 再起動が必要です。**通常運用では既定値のまま** にしてください。

### 移行ノート

- 配信者側の操作は一切不要。Icecast Ogg Vorbis のままで AYAstorm listener から聞こえるようになります
- listener 側の操作も一切不要。r25 を起動するだけで Ogg Vorbis 土地音が戻ります
- vanilla Firestorm / 他 viewer で Ogg Vorbis 土地音が聞けていた配信が、AYAstorm でも同様に聞けるようになる、というのが正しい挙動の戻り

### 既知の制約

- **chained Ogg / serial change**: 同一 HTTP stream の途中で Ogg logical stream が切り替わる (serial number 変更) ケースは未対応。Icecast の通常運用では稀な遷移ですが、配信側で長時間運用する場合に発生し得ます。検出時の自動再初期化は r26 以降の課題
- **Opus 以外の非 Vorbis Ogg family** (Theora / Speex 等): 同一 codec で対応しない方針。Ogg なら何でも吸う設計にはしない (誤判定リスクを抑える)
- **macOS / Windows 実機検証**: PR 著者の macOS arm64 + AYAstorm 側の Linux ビルドで動作確認 PASS、Mac/Win の Release ビルドは tag 切り出し時に実施

### 実装概要

- `indra/llaudio/fmod_codec_ogg.{cpp,h}` — Ogg Opus/Vorbis 両対応 codec (PR #75 で `fmod_codec_opus.{cpp,h}` から rename)
- `indra/llaudio/llaudioengine_fmodstudio.cpp` — codec 登録 1 本、登録名 "AYAstorm Ogg Opus/Vorbis codec"
- `indra/llaudio/llpositionalstreammulti.cpp` — 3D Stream 経路で plugin codec name `"Ogg Vorbis"` を `FMOD_SOUND_TYPE_OGGVORBIS` に昇格
- `indra/newview/app_settings/settings.xml` — 診断 cvar 2 件追加
- 既存 Opus (mono/stereo/multistream 5.1ch) の path は touch 無し、5.1ch surround Opus source の再生も検証済

### Credits

r25 の本実装 (Ogg Vorbis decoder 取り込み + Opus 維持 + fallback 経路回避設計 + 調査資料) は [t-noami](https://github.com/t-noami) さんによるものです。AYAstorm 側ではファイル / 関数 / header guard の rename cleanup (`fmod_codec_opus` → `fmod_codec_ogg`、`FMODGetCodecDescriptionOpus()` → `FMODGetCodecDescriptionOgg()`) を取り込み済みです。

### ドキュメント

- r25 full investigation log / 検証 URL / 失敗仮説の整理 / 設計メモ / 6ch multichannel 取り扱い: [`docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md`](./ayastorm-r25-parcel-music-ogg-vorbis-investigation.md)
- 過去の Opus codec 関連 (r9-opus 系列): [`docs/specs/spec_5_1ch_opus_decode.md`](../specs/spec_5_1ch_opus_decode.md)
