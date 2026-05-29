# 3D Stream — 使い方ガイド

> **Language / 言語 / 语言**: [English](./3dstream-user-guide.en.md) · **日本語** · [中文](./3dstream-user-guide.zh.md)

**搭載**: AYAstorm r31 以降。r32 以降では 3D Stream は初期状態で無効です。

**対象**: 配信者、DJ、ライブ会場オーナー、展示・映画館・イベント会場を作る人。

**関連リファレンス**: タグの全キーと細かい仕様は [3D Stream タグ書式ガイド](../guides/3dstream-tag-guide.ja.md) を参照してください。

---

## 1. 3D Stream とは

3D Stream は、プリムをスピーカーとして使い、HTTP オーディオストリームまたは Media-on-a-Prim (MOAP) の音声を 3D 空間に配置して再生する機能です。

通常のパーセル BGM は、場所に関係なく同じ音量で聞こえます。3D Stream では、スピーカープリムの位置から音が鳴り、リスナーが近づくと大きく、離れると小さくなります。

主な用途:

- ライブ会場の左右スピーカー
- 映画館や展示会場の多点スピーカー
- 5.1ch ソースの空間配置
- MOAP 画面の音声を、画面位置や会場スピーカーから鳴らす構成

## 2. 再生前の準備と許可確認

3D Stream は初期状態では無効です。使う場合は、ステータスバーの 3D Stream ボタン、音量ポップアップの 3D Stream チェック、または **Preferences > Sound** の 3D Stream 項目で有効にします。

`{url:...}` を使う 3D Stream は、初回再生時に接続先の確認ダイアログを表示します。ユーザーが許可した場合のみ、その URL の再生が始まります。拒否した場合、その URL は同じ viewer セッション内では再生されず、同じ URL で確認を繰り返しません。

URL source で利用できる scheme は `http://` と `https://` です。その他の scheme は再生対象になりません。

`{source:media}` は、リンクセット内の MOAP / media 面の音声を 3D Stream に渡す指定です。3D Stream が新しい URL を直接開くものではないため、通常の media 表示・再生の許可処理に従います。

## 3. 最小構成: 1 個のプリムから鳴らす

プリムの **Description** に次のように書きます。

```text
[3dstream:{url:http://example.com/stream.mp3}]
```

この場合、そのプリム自身がスピーカーになります。ステレオ音源を指定しても、単一プリムでは mono にまとめて再生されます。

距離を調整したい場合:

```text
[3dstream:{url:http://example.com/stream.mp3}{min:2}{max:40}]
```

`min` は音量 100% の近距離、`max` は音量 0% になる遠距離です。

## 4. 左右スピーカーを置く

2 個以上のプリムをリンクし、ルートと子プリムに役割を書きます。

ルート Description:

```text
[3dstream:{url:http://example.com/stream.mp3}{range:30}{ch:L}]
```

子プリム Description:

```text
[3dstream:{ch:R}]
```

これでルートが左、子プリムが右のスピーカーになります。左右の定位はリンク番号ではなく、各プリムの実際の位置で決まります。

複数のスピーカーから同じチャンネルを鳴らすこともできます。たとえば `{ch:L}` を複数プリムに書けば、すべてが L スピーカーとして鳴ります。

## 5. 5.1ch / 多点スピーカー配置

5.1ch ソースを配置する場合は、各チャンネル用のプリムを置きます。

ルート Description:

```text
[3dstream:{url:http://example.com/live_5_1.opus}{range:30}]
```

各スピーカープリム:

```text
FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```

5.1ch 配信には Vorbis 6ch または Opus 6ch を使用できます。ライブ配信では Opus 6ch が実用的です。ミュージシャンや DJ が Opus 6ch で配信する場合は、t-noami 作成の [SurroundStreamer](https://github.com/t-noami/SurroundStreamer) を利用できます。

ステレオ配信を 6 スピーカーに展開したい場合は、ルートに `{upmix:on}` を追加します。

```text
[3dstream:{url:http://example.com/stereo.ogg}{upmix:on}{range:30}]
```

5.1ch ソースに `{upmix:on}` が付いていても、viewer は 6ch source を検出すると upmix を自動的に bypass します。

## 6. MOAP / media 音声を 3D Stream に使う

HTTP URL ではなく、リンクセット内の media 面を音源にすることもできます。

ルート Description:

```text
[3dstream:{source:media}{ch:L}]
```

子プリム Description:

```text
[3dstream:{ch:R}]
```

media 面が複数ある場合は、ルート側で `{link:N}` / `{face:N}` を指定して対象の面を選びます。

```text
[3dstream:{source:media}{link:3}{face:2}{range:30}]
```

`{url:...}` と `{source:media}` は同時に指定できません。media 画面を表示しつつ別の URL ストリームを 3D 配置したい場合は、3D Stream 側は `{url:...}` のままにし、media 面は通常の MOAP として扱います。

## 7. 音の調整

よく使う調整:

| 指定 | 用途 |
|---|---|
| `{range:N}` | スピーカーの届く距離 |
| `{volume:N}` | スピーカーごとの音量。`0.0` から `1.0` |
| `{bin:on}` | ヘッドホン向けの定位補正を有効化 |
| `{v:NAME}` | 会場残響 preset を指定 |
| `{wg:N}` | 残響の wet 成分量 |
| `{upmix:on}` | stereo source を 5.1ch 配置に展開 |

`binaural` は未指定では `off`、`venue` は未指定では `dry` です。まずは何も足さずに配置を確認し、必要に応じて `{bin:on}` や `{v:...}` を追加してください。

会場残響を使う場合は、配信音源側に強い reverb を入れすぎない方が調整しやすくなります。

## 8. 他 Viewer での見え方

3D Stream のタグは AYAstorm 専用です。本家 Firestorm、公式 Viewer、Catznip などでは 3D Stream としては解釈されません。

- `{url:...}` の 3D Stream 音声は、他 Viewer では鳴りません
- `{source:media}` の場合、media 面自体は通常の MOAP として表示・再生されます
- ただし `{ch:...}` routing、upmix、binaural、venue reverb は AYAstorm 利用者にだけ適用されます
- パーセル BGM が設定されている場合、それは他 Viewer でも通常どおり聞こえます

## 9. よくあるトラブル

| 症状 | 確認すること |
|---|---|
| 音が鳴らない | Preferences の 3D Stream が有効か、音量が 0 でないか |
| URL stream の確認が出る | 初めての `{url:...}` source は再生前に許可が必要 |
| 左右の片方しか鳴らない | `{ch:L}` と `{ch:R}` の両方がリンクセット内にあるか |
| `{source:media}` で鳴らない | media 面が同じリンクセット内にあるか、必要なら `{link}` / `{face}` を指定しているか |
| 5.1ch の一部が無音 | 対応する `{ch:FL}` などのスピーカープリムがあるか |
| タグを消したのに音が残る | Description の再評価は通常 30 秒以内。必要なら `Stream3DEnabled` を一度 OFF にする |
| 他 Viewer の人から聞こえないと言われる | 3D Stream は AYAstorm 専用。MOAP やパーセル BGMとは別機能 |

配置確認中は **Preferences > Sound > Show channel routing diagnostics in chat** を有効にすると、どのプリムがどのチャンネルを鳴らしているかを Local Chat で確認できます。

## 10. 詳細リファレンス

- タグの全キーとエラー文言: [3D Stream タグ書式ガイド](../guides/3dstream-tag-guide.ja.md)
- Opus 6ch 配信ツール: [SurroundStreamer](https://github.com/t-noami/SurroundStreamer)
