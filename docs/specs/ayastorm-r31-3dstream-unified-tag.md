# AYAstorm r31: 3D Stream unified tag

**Status**: implementation branch draft
**Target branch**: `feat/ayastorm-r31-3dstream-unified-tag`
**Scope**: `[3dstream:...]` を mono と linkset routing の共通タグとして扱うための仕様

## 目的

これまで 3D Stream には、単一プリム用の `[3dstream:...]` と、リンクセット分散再生用の `[3dstream-stereo:...]` があった。

r31 ではユーザーが覚えるタグを `[3dstream:...]` に寄せる。ただし既存コンテンツを壊さないため、`[3dstream:{url:...}]` 単独は従来通り mono として動作する。

あわせて、ユーザーが `[3dstream:...]` を新規に書くときの既定動作を保守的にするため、`{bin}` / `{binaural}` 未指定時は off とする。定位補正を使う場合は `{bin:on}` を明示する。

## 判定ルール

`[3dstream:...]` を linkset routing として扱うかどうかは、リンクセット内に有効な `{ch:...}` 指定があるかで決める。

子プリムが存在するだけでは stereo / distributed には昇格しない。`{ch:...}` のない子プリムがあるだけなら、root の `[3dstream:{url:...}]` は mono のまま。

### Mono のままになる例

root:

```text
[3dstream:{url:http://example.com/stream.mp3}]
```

子プリムなし、または `{ch:...}` のない子プリムだけなら mono 再生になる。

```text
子プリム Description:
看板
```

### Stereo / distributed に昇格する例

root:

```text
[3dstream:{url:http://example.com/stream.mp3}]
```

left speaker:

```text
[3dstream:{ch:L}]
```

right speaker:

```text
[3dstream:{ch:R}]
```

この場合、root の mono binding は解除され、linkset routing の binding が作られる。

### Root 自身を mono speaker にする例

root:

```text
[3dstream:{url:http://example.com/stream.mp3}{ch:M}]
```

`{ch:M}` があるため、これは旧 mono path ではなく distributed path として扱う。

## Media source

`{source:media}` / `{source:media-5-1}` は distributed source declaration であり、mono fallback はない。少なくとも 1 つの `{ch:...}` speaker が必要。

MOAP stereo:

```text
root: [3dstream:{source:media}]
L:    [3dstream:{ch:L}]
R:    [3dstream:{ch:R}]
```

MOAP 5.1:

```text
root: [3dstream:{source:media-5-1}]
FL:   [3dstream:{ch:FL}]
FR:   [3dstream:{ch:FR}]
C:    [3dstream:{ch:C}]
LFE:  [3dstream:{ch:LFE}]
SL:   [3dstream:{ch:SL}]
SR:   [3dstream:{ch:SR}]
```

## 互換性

次の既存形式は引き続き受け付ける。

- `[3dstream:{url:...}]`: 単一プリム mono URL stream
- `[ayastream:{url:...}]`: 旧 mono alias
- `[3dstream-stereo:...]`: 旧 distributed/linkset tag
- `[ayastream-stereo:...]`: 旧 distributed/linkset alias

新規作成では `[3dstream:...]` を推奨する。ただし、既存の `[3dstream-stereo:...]` を急いで置き換える必要はない。

## 実装メモ

主な変更点:

- `LLPositionalStreamMgr::parseDistributedStereoTag()` が `[3dstream:...]` も distributed grammar として読む。
- `DistStereoTagData::unified_3dstream_prefix` で、旧 `3dstream-stereo` ではなく unified `[3dstream:...]` から読まれたタグかを保持する。
- `evaluateBinding()` は `[3dstream:{url:...}]` をまず mono 互換として扱う。
- ただし同じタグ、または既知の linkset child に `{ch:...}` がある場合は mono binding を消して `evaluateLinkset()` に渡す。
- `evaluateLinkset()` は `{ch:...}` speaker が 1 つもない unified `[3dstream:{url:...}]` root を mono fallback に戻す。
- distributed binding が成立した場合、root に残っている mono binding は削除する。

重要な注意:

- 子プリムの存在だけでは昇格条件にしない。
- 昇格条件は `{ch:...}` が実際に見つかること。
- 子 Description が未取得でも、`{ch:...}` が見つかるまでは root URL の mono playback を維持してよい。

## 動作確認メモ

r31 branch で次を確認済み。

- `[3dstream:{url:...}]` 単独が mono になる。
- root `[3dstream:{url:...}]` + child `{ch:L}` / `{ch:R}` が stereo/distributed になる。
- `{ch:...}` のない子プリムがあるだけでは mono のまま。
- 修正後の macOS app build が成功する。

Build artifact:

```text
build-darwin-universal/newview/RelWithDebInfo/AYAstorm.app
```

## 2026-05-24 追補: r31 ドキュメント / 仕様調整

このブランチでの動作確認後、ユーザー向け説明と実装差分を次のように整理した。

### binaural 既定値

`{bin}` / `{binaural}` 未指定時の既定は **off**。

- `LLPositionalStreamMgr::effectiveBinaural()` は tag 未指定を `false` として解決する。
- `Stream3DBinauralRender = -1` は「タグ通り」の sentinel のまま。ただしタグ未指定時の解決結果は off。
- `Stream3DBinauralRender = 0` / `1` は従来どおり listener 側の debug override として扱う。

理由: 既存配置の音の出方を、タグ無改修ではなるべく変えないため。r12 由来の binaural DSP は有効化したい会場が明示的に opt-in する。

### upmix の説明

stereo→5.1 upmix は、商標名を使わず **matrix upmix + 帯域分離** として説明する。

実装は `LLStereoUpmix` による固定アルゴリズムで、概略は次の通り。

```text
C   = (L + R) / sqrt(2)
SL  = delay(+ (L - R) / sqrt(2))
SR  = delay(- (L - R) / sqrt(2))
LFE = LPF((L + R) / 2)
FL/FR は center 成分を指定量だけ差し引く
```

特定の商用 decoder 名や方式名としては記述しない。

### codec / 配信経路

日本語タグガイドとユーザーガイドの codec 説明を、r9-opus 補完後の状態に合わせた。

- Vorbis 6ch: 実機検証済み。
- Opus 6ch: r9-opus codec plugin 経由で実機検証済み。通常の Ogg/Opus 6ch 配信では FMOD parser の seek 制約を受けない。
- FLAC 6ch: codec layout は実装済み。ただし配信経路によって seek が必要になり、`FMOD_ERR_FILE_COULDNOTSEEK` で開けない場合がある。
- 5.1ch 配信では Vorbis 6ch または Opus 6ch を案内する。ライブ配信では Opus 6ch を優先し、ミュージシャン / DJ 向けの実用経路として [SurroundStreamer](https://github.com/t-noami/SurroundStreamer) を記載する。

### 他 Viewer / MOAP fallback

他 Viewer では 3D Stream タグは解釈されない。

- `{url:...}` の 3D Stream 音声は他 Viewer では鳴らない。
- `{source:media}` の media 面自体は、他 Viewer でも通常の MOAP として表示・再生される。
- `{ch:...}` routing、upmix、binaural、venue reverb は AYAstorm 専用。
- media 画面を見せながら `{url:...}` の 3D Stream を鳴らす構成では、他 Viewer では media 側だけが通常 MOAP として扱われる。

### エラー通知文のタグ例

Local Chat に出るタグ書式エラー / 構造エラーの例示は、新規推奨タグの `[3dstream:...]` に統一した。

- `BadCh` / `BadRange` / `BadVolume` などの speaker tag 例は `[3dstream:{ch:L}{range:30}]` 形式。
- URL source 例は `[3dstream:{url:http://example/stream.mp3}{range:30}]` 形式。
- media source 例は `[3dstream:{source:media}{link:2}{face:0}{ch:L}{range:30}]` 形式。

旧 `[3dstream-stereo:...]` / `[ayastream-stereo:...]` の受付は互換性のため残す。内部ログの `[3dstream-stereo]` 表記も subsystem/debug label として残っており、ユーザー向けの推奨記法を意味しない。

### 関連ドキュメント

最終的に日本語版を基準として、英語版・中国語版も同内容で整備した。

- `docs/guides/3dstream-tag-guide.ja.md`: r31 unified tag、binaural 既定 off、upmix 表現、codec 実績、MOAP fallback、troubleshooting を反映。
- `docs/guides/3dstream-tag-guide.en.md` / `docs/guides/3dstream-tag-guide.zh.md`: 日本語版との差異が出ないよう、同じ仕様内容で更新。
- `docs/specs/3dstream-user-guide.ja.md`: 使い方ガイドを新規作成。配置手順、5.1ch 配信、SurroundStreamer、他 Viewer fallback を記載。
- `docs/specs/3dstream-user-guide.en.md` / `docs/specs/3dstream-user-guide.zh.md`: 日本語版と同じ構成・仕様内容で作成。
- `docs/README.md`: `docs/specs/3dstream-user-guide.ja.md` を docs 構成例に追加。
