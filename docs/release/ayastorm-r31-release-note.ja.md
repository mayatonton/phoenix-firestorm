# AYAstorm r31 — リリース告知

**r31 は 3D Stream の linkset routing タグを `[3dstream:...]` に一本化するリリース**。これまで mono URL stream 用の `[3dstream:...]` と、linkset 分散再生用の `[3dstream-stereo:...]` に分かれていたタグを、`[3dstream:...]` に統合します。既存タグはすべて互換性のため引き続き受け付けます。

> **配信形態**: r31 は r25〜r30 + r31 を一括配信する tag (`v7.2.4-ayastorm-r31+bundle-fs.80646`) の 1 機能として出荷されます。同梱される他リリースの release note と Firestorm upstream FS-7.1.18.80646 取込分は GitHub Release ページから直接リンクされます。

仕様の単一の真実は `docs/specs/ayastorm-r31-3dstream-unified-tag.md`、配信者向け実用ガイドは `docs/guides/3dstream-tag-guide.{en,ja,zh}.md` と `docs/specs/3dstream-user-guide.{en,ja,zh}.md` です。本ノートは差分ハイライトに徹します。

---

## AYAstorm r31 — `[3dstream:...]` unified linkset tag

### r31 の柱: タグ 1 つで mono も linkset routing も書ける

r6 以降、3D Stream には 2 種類のタグがありました:

- `[3dstream:{url:...}]` — 単一プリム mono URL stream
- `[3dstream-stereo:...]` — linkset 分散ステレオ / 5.1 / MOAP routing

r31 では、新規にタグを書くユーザーが覚える形を **`[3dstream:...]` 1 つ** にまとめます。同じ prefix で mono も linkset routing も書けるようになります。

判定ルール: linkset 内に有効な `{ch:...}` 子プリムがあるかどうかで mono / distributed が決まります。`{ch:...}` が無ければ root の `[3dstream:{url:...}]` は従来どおり mono として再生されます。

### 互換性

以下の既存形式は **すべて引き続き受け付けます**。既存コンテンツに書き換えは不要です:

- `[3dstream:{url:...}]` — mono URL stream
- `[ayastream:{url:...}]` — 旧 mono alias
- `[3dstream-stereo:...]` — 旧 linkset/distributed タグ
- `[ayastream-stereo:...]` — 旧 linkset/distributed alias

新規作成では `[3dstream:...]` を推奨しますが、急いで置き換える必要はありません。

### 主な書き方の例

**Mono URL (従来どおり)**

```text
[3dstream:{url:http://example.com/stream.mp3}]
```

**Stereo / distributed (linkset)**

```text
root:  [3dstream:{url:http://example.com/stream.mp3}]
left:  [3dstream:{ch:L}]
right: [3dstream:{ch:R}]
```

子プリムに `{ch:...}` があるため、root の mono binding は自動的に linkset routing に昇格します。

**MOAP stereo / 5.1**

```text
root: [3dstream:{source:media}]
L:    [3dstream:{ch:L}]
R:    [3dstream:{ch:R}]
```

```text
root: [3dstream:{source:media-5-1}]
FL:   [3dstream:{ch:FL}]
FR:   [3dstream:{ch:FR}]
C:    [3dstream:{ch:C}]
LFE:  [3dstream:{ch:LFE}]
SL:   [3dstream:{ch:SL}]
SR:   [3dstream:{ch:SR}]
```

### 設定 — `{bin}` / `{binaural}` の既定値変更

新規 `[3dstream:...]` タグで **`{bin}` / `{binaural}` の指定が無い場合の既定が off** に変わります。binaural を効かせたい会場は `{bin:on}` を明示してください。

理由: 既存配置の音の出方を、タグ無改修ではなるべく変えないため。r12 由来の binaural DSP は有効化したい会場が明示的に opt-in する形に揃えます。listener 側 debug override `Stream3DBinauralRender = 0 / 1` は従来どおり利用可能です (`-1` = タグ通り)。

### 5.1ch 配信 codec の整理

5.1ch 配信に使える codec を r9-opus codec plugin 投入後の現状に合わせて整理しました:

- **Vorbis 6ch** — 実機検証済み
- **Opus 6ch** — r9-opus codec plugin 経由で実機検証済み (FMOD parser の seek 制約を受けません)
- **FLAC 6ch** — codec layout は実装済み。ただし配信経路によっては seek が必要になり、`FMOD_ERR_FILE_COULDNOTSEEK` で開けない場合があります

ライブ配信では **Opus 6ch を優先** してください。実用配信経路として [SurroundStreamer](https://github.com/t-noami/SurroundStreamer) を案内しています。

### upmix の説明変更

stereo → 5.1 upmix は、商標名を使わず **matrix upmix + 帯域分離** として説明します。実装は `LLStereoUpmix` による固定アルゴリズム (center は `(L+R)/√2`、SL/SR は `±(L-R)/√2` を遅延、LFE は LPF) で、特定の商用 decoder 名や方式名としては記述しません。

### 他 Viewer / MOAP fallback

他 Viewer では 3D Stream タグは解釈されません:

- `{url:...}` の 3D Stream 音声は他 Viewer では鳴らない
- `{source:media}` の **media 面自体** は、他 Viewer でも通常の MOAP として表示・再生される
- `{ch:...}` routing、upmix、binaural、venue reverb は AYAstorm 専用

media 画面を見せながら `{url:...}` の 3D Stream を鳴らす構成では、他 Viewer では media 側だけが通常 MOAP として扱われます。

### エラー通知文のタグ例

Local Chat に出るタグ書式エラー (`BadCh` / `BadRange` / `BadVolume` 等) の例示は、新規推奨タグの `[3dstream:...]` 形式に統一しています。旧 `[3dstream-stereo:...]` / `[ayastream-stereo:...]` の受付は互換性のため残しますが、内部ログの `[3dstream-stereo]` 表記は subsystem/debug label として残るのみで、ユーザー向けの推奨記法を意味しません。

### 既知の制約

- **linkset 昇格は `{ch:...}` 検出時のみ**: 子プリムが存在するだけでは mono → linkset に昇格しません。`{ch:...}` を持つ子プリムが 1 つ以上必要です
- **子 Description 取得遅延**: 子プリム Description が未取得の間は、root URL の mono playback で先に再生され、`{ch:...}` 検出後に linkset routing へ移行します
- **`{source:media}` / `{source:media-5-1}` には mono fallback なし**: distributed source declaration なので、少なくとも 1 つの `{ch:...}` speaker が必要です

### 実装サマリ

- `LLPositionalStreamMgr::parseDistributedStereoTag()` が `[3dstream:...]` を distributed grammar として読む
- `DistStereoTagData::unified_3dstream_prefix` で、旧 `[3dstream-stereo:...]` 由来か unified `[3dstream:...]` 由来かを保持
- `evaluateBinding()` は `[3dstream:{url:...}]` をまず mono 互換として扱い、同タグまたは linkset child に `{ch:...}` が見つかった時点で mono binding を消して `evaluateLinkset()` に渡す
- `evaluateLinkset()` は `{ch:...}` speaker が 1 つもない unified `[3dstream:{url:...}]` root を mono fallback に戻す
- `LLPositionalStreamMgr::effectiveBinaural()` は tag 未指定を `false` として解決

### 関連資料

- r31 unified tag spec (単一の真実): `docs/specs/ayastorm-r31-3dstream-unified-tag.md`
- 配信者向け実用ガイド (累積): `docs/guides/3dstream-tag-guide.{en,ja,zh}.md`
- ユーザー向け使い方ガイド: `docs/specs/3dstream-user-guide.{en,ja,zh}.md`
- 過去章 (r6 以降の 3D Stream 系譜): `docs/specs/ayastorm-r*-*.md`
