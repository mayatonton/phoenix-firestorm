# 3D Stream タグ書式ガイド

> **Language / 言語 / 语言**: [English](./3dstream-tag-guide.en.md) · **日本語** · [中文](./3dstream-tag-guide.zh.md)
>
> AYAstorm の **3D Stream** 機能で、プリムから HTTP オーディオストリームまたは Media-on-a-Prim (MOAP) 音声を 3D 空間定位再生するためのタグ書式リファレンスです。
>
> このドキュメントは AYAstorm `r31` 時点の最終仕様に基づきます。r31 では、単一プリム再生とリンクセット分散配置の新規推奨タグを `[3dstream:...]` に統一しました。旧 `[3dstream-stereo:...]` / `[ayastream-stereo:...]` も互換受付します。

---

## 目次

1. [3D Stream とは](#1-3d-stream-とは)
2. [クイックスタート](#2-クイックスタート)
3. [用語](#3-用語)
4. [タグの全体像](#4-タグの全体像)
5. [単一プリム URL 再生 `[3dstream:...]`](#5-単一プリム-url-再生-3dstream)
6. [リンクセット配置 / 分散ステレオ `[3dstream:...]`](#6-リンクセット配置--分散ステレオ-3dstream)
7. [バイノーラル / 会場残響 (r12)](#7-バイノーラル--会場残響-r12)
8. [stereo→5.1 upmix (r12)](#8-stereo51-upmix-r12)
9. [ch (チャンネル) 値リファレンス](#9-ch-チャンネル-値リファレンス)
10. [ソース ch 数 × タグ値 の互換マトリクス](#10-ソース-ch-数--タグ値-の互換マトリクス)
11. [配信側 (ソース URL の作り方)](#11-配信側-ソース-url-の作り方)
12. [viewer 側の設定](#12-viewer-側の設定)
13. [エラー通知 / 診断](#13-エラー通知--診断)
14. [トラブルシューティング](#14-トラブルシューティング)
15. [既知の制約 / 仕様上の注意](#15-既知の制約--仕様上の注意)
16. [静的 occlusion `[ayastorm:occlude]` (r13)](#16-静的-occlusion-ayastormocclude-r13)
17. [関連ドキュメント / 内部仕様書](#17-関連ドキュメント--内部仕様書)

---

## 1. 3D Stream とは

Second Life 標準の Viewer では HTTP オーディオストリーム (SHOUTcast / Icecast) は **パーセル単位の BGM** として 2D 再生されるのみで、空間内のどこから鳴っているかという情報を持ちません。

AYAstorm の **3D Stream** 機能は、プリム (オブジェクト) を「スピーカー」として扱い、ストリーム音源を **そのプリムから鳴っているように 3D 定位再生** します。リスナー (カメラまたはアバター) が動くと、音の方向と距離感がリアルタイムに追従します。

主な用途:

- **ライブ会場の PA**: ステージ前にスピーカープリムを置き、配信中の音源をその位置から鳴らす
- **環境音**: 川辺・ジュークボックス・テレビ等のオブジェクトから対応する音を鳴らす
- **ステレオ配置 / マルチスピーカー会場**: 複数のプリムに L / R / モノラルを割り当ててステレオ感を空間に広げる
- **5.1ch ソースの会場展開**: 6 個のプリムに 5.1ch 各チャンネルを配置 (FL / FR / C / LFE / SL / SR)

操作はすべて **プリムの Description (説明文) フィールドにタグを書き込む** だけで完結します。LSL スクリプト不要、SL サーバー側変更不要、相手は AYAstorm を使っているユーザーのみ。他の Viewer (本家 Firestorm / 公式 LL Viewer 等) はこのタグを無視するため互換性問題は起きません。

---

## 2. クイックスタート

### 2.1 単一プリムから音を鳴らす (もっとも簡単な例)

任意のプリムを 1 つ作り、その **Description フィールド**に以下を書き込みます:

```
[3dstream:{url:http://example.com/stream.mp3}]
```

これだけで、そのプリムの位置から `http://example.com/stream.mp3` のストリームが 3D 定位で再生されます。プリムから離れると音が小さくなり、左右に動くと自然にパンが変わります。

### 2.2 ステレオ配置 (L / R を別プリムに分離)

ステレオ音源を 2 つのプリムに分けて、空間にステレオ感を作ります。

1. 親プリム (Root) と子プリム (Child) を 1 つずつリンクします (Ctrl+L)
2. **Root の Description**:
   ```
   [3dstream:{url:http://example.com/stream.mp3}{range:30}]
   ```
3. **Root の Description に追記** (= Root 自身も L スピーカー):
   ```
   [3dstream:{url:http://example.com/stream.mp3}{range:30}{ch:L}]
   ```
4. **Child の Description**:
   ```
   [3dstream:{ch:R}]
   ```

これで Root から L、Child から R が鳴ります。

### 2.3 さらに詳しく

3 以降を読んでください。マルチスピーカー / 5.1ch / 細かいチューニング / 配信側のレシピ等を解説します。

---

## 3. 用語

| 用語 | 意味 |
|---|---|
| **ストリーム** | HTTP 経由で配信されるオーディオデータ (SHOUTcast / Icecast / 静的ファイル等)。MP3 / Vorbis / Opus / FLAC が主な対応 codec |
| **リンクセット** | SL の「リンク」(Ctrl+L) で 1 つにまとめられたプリム集合。ルート 1 個 + 子 N 個 |
| **ルートプリム** | リンクセットの親プリム。Build → Edit で「Selected linked」「Edit linked」OFF 時に最初に選択されるプリム |
| **子プリム** | ルート以外のリンクセット内プリム |
| **音源宣言** | `{url:...}` または `{source:media...}` を含むタグを書いたルートプリム。本書での「どの音源を鳴らすか」を宣言する役割。**ルートプリムにのみ書ける** (子プリムに書いても無視されます) |
| **media source** | 同じリンクセット内の Media-on-a-Prim (MOAP) 面を 3D Stream の音源として使う指定。ルートの `{source:media}` / `{source:media-stereo}` / `{source:media-5-1}` で有効になり、必要に応じて `{link:N}{face:N}` で面を選択します |
| **スピーカープリム** | `{ch:...}` を含むタグを書いたプリム。実際に音を鳴らすプリム。**ルート/子プリムどちらでも可** |
| **binding** | 1 つのリンクセットに対して内部で組み立てられる「音源 → スピーカー群」の対応関係。1 リンクセット = 1 binding |
| **ch (チャンネル)** | スピーカープリムが受け持つ音声チャンネル。`L` / `R` / `M` (モノラル) のほか、5.1ch 用の `FL` / `FR` / `C` / `LFE` / `SL` / `SR` |
| **rolloff** | 距離減衰。リスナーがスピーカーから離れるにつれ音量が減衰する設定 |

---

## 4. タグの全体像

### 4.1 3 種類の用途

| 用途 | 接頭辞 | 成立条件 |
|---|---|---|
| **単一プリム URL 再生** | `[3dstream:...]` | `{url:...}` があり、同じリンクセット内に `{ch:...}` を持つ 3D Stream タグが無い |
| **リンクセット配置 / 分散ステレオ** | `[3dstream:...]` | ルートに `{url:...}` または `{source:media...}` があり、ルートまたは子プリムに `{ch:...}` が 1 つ以上ある |
| **静的 occlusion タグ** (r13 新設) | `[ayastorm:occlude]` | 壁・扉・床・天井プリムを「音を遮るもの」として扱う (会場運営 / 建設者向け、§16) |

### 4.2 旧プレフィクスのエイリアス

r31 以降の新規記述は **`[3dstream:...]` を推奨**します。旧プレフィクス (`[ayastream:...]` / `[3dstream-stereo:...]` / `[ayastream-stereo:...]`) も互換のため受け付けます。r5 の `ayastream` → `3dstream` rename、および r31 の unified tag 以前に配置されたプリムを再編集せずに済むよう温存しています。

```
[3dstream:{url:...}]              ← 単一プリム URL 再生の推奨 (canonical)
[3dstream:{url:...}{ch:L}]        ← リンクセット配置の推奨 (canonical)

[ayastream:{url:...}]             ← 旧式、互換受付
[3dstream-stereo:{ch:L}]          ← 旧式、互換受付
[ayastream-stereo:{ch:L}]         ← 旧式、互換受付
```

### 4.3 共通の書式ルール

- **タグは Description のどこに書いてもよい**。前後に他の文章があっても無視されます (例: `お店の名前 [3dstream:{url:...}] 営業中` のように説明文と同居可)。
- フィールドは `{key:value}` 形式の集合で、フィールド間に区切り文字を要しません (空白・カンマ・連結いずれも可)。
- **キー名は大文字小文字を区別しません** (内部で小文字に正規化されます)。`{URL:...}` も `{url:...}` も同じ。
- **値の前後の空白は trim** されます。`{ url : http://example/  }` でも OK。
- **未知のキーは黙って無視** されます。例えば `{foo:bar}` は何の効果もありませんがエラーにもなりません。
- 1 つのプリムの Description に **同種タグを複数書いた場合は最初の 1 個**だけが採用されます。

### 4.4 SL の Description 制限 (127 byte)

LSL `llSetObjectDesc` が書ける Description は **127 byte 上限**です。日本語を含む場合は UTF-8 換算でこれを超えやすいので、**長い URL は短縮する** か、ルートに音源宣言だけ書いて子プリムに `{ch:...}` だけ書く分散方式 (§6) でやりくりします。**頻出キー名と venue 値には短縮形** が用意されています (§4.5)。

### 4.5 キー名・venue 値の短縮形 (r12 / r12.1)

`[3dstream:...]` で使う **キー名のうち頻出 4 つ** と **`venue` の値 9 種** には r12 から **短縮エイリアス**を用意しています。SL の Description 127 byte 上限 (§4.4) に収めやすくするためのもので、長形式と短縮形は **完全等価** です (内部で同じ正規形に解決)。新規記述・既存記述どちらの形式で書いても動作は同じです。

#### キー名の短縮 (4 件)

| 長形式 (canonical) | 短縮形 | 機能 (詳細) |
|---|---|---|
| `binaural` | `bin` | バイノーラル ON/OFF (§7.1) |
| `venue` | `v` | 会場残響プリセット (§7.2) |
| `wetgain` | `wg` | 残響ウェット成分の強さ (§7.3) |
| `lfegain` | `lg` | LFE チャンネルゲイン倍率 (§7.4、r12.1 追加) |

その他のキー (`url` / `source` / `link` / `face` / `ch` / `range` / `volume` / `min` / `max` / `upmix`) には短縮形はありません (元々短い、または使用頻度低)。

#### `venue` の値の短縮 (9 種)

| 長形式 (canonical) | 短縮形 |
|---|---|
| `dry` | `d` |
| `room_small` | `rs` |
| `room_medium` | `rm` |
| `hall_small` | `hs` |
| `hall_medium` | `hm` |
| `hall_large` | `hl` |
| `club` | `cl` |
| `cathedral` | `ct` |
| `outdoor` | `od` |

#### 書き換え例

長形式 (**127 byte 超過、Description に書けない**):

```
[3dstream:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{ch:C}{binaural:on}{venue:hall_medium}{wetgain:1.5}{upmix:on}]
```
(133 byte — 127 上限を 6 byte 超過)

短縮形 (**127 byte 以内、書ける**):

```
[3dstream:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{ch:C}{bin:on}{v:hm}{wg:1.5}{upmix:on}]
```
(110 byte — 23 byte 削減で 127 byte 制限を余裕クリア)

#### LSL から書く場合

同梱の LSL `aya_3dstream_setup.lsl` (§17 参照) は **入力時は両形式を受け付け、出力 (Description 書き込み) 時は常に短縮形** で書き出します。LSL ダイアログから設定した Description は自動的に短縮形になります。

#### 大文字小文字 / 混在

- キー名は **大文字小文字を区別しません** (§4.3 共通ルールどおり)。`{BIN:on}` も `{bin:on}` も `{binaural:on}` もすべて等価
- 同じタグ内で長形式と短縮形を **混在させても OK** (例: `{binaural:on}{v:hm}{wg:1.5}`)。ただし可読性のためどちらかに揃えることを推奨

### 4.6 タグ反映のタイミング

- AYAstorm は **30 秒間隔で範囲内のプリム Description をポーリング** します (`Stream3DPollInterval` 設定)。
- LSL `llSetObjectDesc` で Description を変更すると、次のポーリングで再評価が走り反映されます (= 通常 5〜30 秒以内)。
- プリムを手動で右クリック → Edit → Description 変更した場合は、その編集確定で即座に再評価されます (Properties 通知経由)。
- リンク / アンリンク操作も再評価のトリガになります。

---

## 5. 単一プリム URL 再生 `[3dstream:...]`

### 5.1 書式

```
[3dstream:{url:URL}{min:N}{max:N}]
```

### 5.2 キー一覧

| キー | 必須 | 型 | 既定値 | 意味 |
|---|---|---|---|---|
| `url` | **必須** | 文字列 | — | ストリーム URL (`http://` / `https://`)。空文字列はエラー |
| `min` | 任意 | F32 (m) | `Stream3DRolloffMin` (1.0) | 距離減衰の **近距離** (このプリムからこの距離以内では音量 100%) |
| `max` | 任意 | F32 (m) | `Stream3DRolloffMax` (20.0) | 距離減衰の **遠距離** (この距離以上では音量 0%) |

距離減衰モデルは FMOD の `FMOD_3D_LINEARSQUAREROLLOFF` (リニア二乗ロールオフ)。`min` と `max` の間で滑らかに減衰します。

### 5.3 動作

- リンクセットの **どのプリム** でも書けます (ルートでも子でも)。書いたプリム自身がスピーカーとして音を鳴らします。
- `{url:...}` だけを書いた `[3dstream:...]` は単一プリム URL 再生になります。
- 同じリンクセット内に `{ch:...}` を持つ 3D Stream タグが 1 つでもある場合、`[3dstream:{url:...}]` はリンクセット配置の音源宣言として扱われ、単一プリム mono ではなくなります。
- 子プリムが存在するだけではリンクセット配置に昇格しません。判定に使うのは **子プリムの有無ではなく `{ch:...}` の有無**です。
- `{source:media...}` は単一プリム再生にはなりません。media/MOAP source routing はリンクセット配置 (§6.7) として、少なくとも 1 つの `{ch:...}` スピーカーと組み合わせます。
- ステレオ音源を渡した場合は **内部で L/R をミックスしてモノラル化** されます。

### 5.4 例

#### 5.4.1 最小構成

```
[3dstream:{url:http://example.com/radio.mp3}]
```

`min` / `max` は省略され、設定既定値 (1m / 20m) で減衰します。

#### 5.4.2 距離をカスタムする

```
[3dstream:{url:http://example.com/radio.mp3}{min:2}{max:50}]
```

プリムから 2m 以内では音量最大、50m で消えます。広い屋外フィールドで遠くまで聞かせたいときに使います。

#### 5.4.3 説明文と併記

```
お店の BGM [3dstream:{url:http://radio.example.jp/8000/jazz}] よろしく
```

タグ前後に他の文があっても問題ありません。

---

## 6. リンクセット配置 / 分散ステレオ `[3dstream:...]`

### 6.1 書式

```
[3dstream:{url:URL}{range:N}{ch:CH}{volume:V}]
[3dstream:{source:media}{link:N}{face:N}{range:N}{ch:CH}{volume:V}]
```

旧プレフィクスも互換受付します:

```
[3dstream-stereo:...]
[ayastream-stereo:...]
```

この用途は **リンクセット全体で 1 つの音源** を扱う形式です。ルートプリムが「どの音源を鳴らすか」(`{url:...}` または `{source:media}`) を宣言し、リンクセット内の各プリムが「自分はどのチャンネルを担当するか」を `{ch:...}` で宣言します。

### 6.2 プリムの役割

各プリムはタグの中身によって以下の役割を持ちます:

| Description のフィールド | 役割 |
|---|---|
| `{url:...}` を含む | **URL 音源宣言** (ルートプリム限定。子プリムで `{url:...}` を書くと無視されます) |
| `{source:media...}` を含む | **media/MOAP 音源宣言** (ルートプリム限定。MOAP 面はルート/子プリムどちらにあっても可) |
| `{ch:...}` を含む | **スピーカー** (ルート/子プリムどちらでも可) |
| 両方を含む (= ルートのみ) | 音源宣言 + 自身もスピーカーを兼ねる |
| どちらも含まない | 何もしない (binding 対象外) |

リンクセット内に **音源宣言 (= `{url}` または `{source:media}` を持つルート)** と **少なくとも 1 個のスピーカー (= `{ch}` を持つプリム)** が両方あって初めてリンクセット配置として再生開始されます。子プリムがあっても `{ch}` が 1 つも無い場合、`[3dstream:{url:...}]` は従来どおり単一プリム URL 再生です。`{source:media}` は `{ch}` スピーカーが無いと再生開始できません。`{url}` と `{source:media}` は **相互排他**です。両方を書いた場合は構造エラーとして扱われます。

### 6.3 キー一覧

#### 6.3.1 ルートプリムでのみ意味があるキー

| キー | 必須 | 型 | 既定値 | 意味 |
|---|---|---|---|---|
| `url` | `source` と排他で必須 | 文字列 | — | ストリーム URL。空文字列はエラー |
| `source` | `url` と排他で必須 | 列挙値 | — | `media` / `media-stereo` = media/MOAP 面を 2ch 音源として扱う。`media-5-1` = 5.1ch / 6ch 音源として扱う |
| `link` | 任意 | S32 | 自動選択 | `{source:media}` 時の media 面があるリンク番号。複数 media 面がある場合の選択用 |
| `face` | 任意 | S32 | 自動選択 | `{source:media}` 時の media 面番号。複数 media 面がある場合の選択用 |
| `range` | 任意 | F32 (m) | `Stream3DRolloffMax` (20.0) | リンクセット内のスピーカーが個別に `range` を持たないときの既定減衰距離 |
| `binaural` | 任意 | bool | `off` | バイノーラル ON/OFF (詳細 §7.1)。短縮形 `bin` |
| `venue` | 任意 | 列挙値 | `dry` | 会場残響プリセット 9 種 (詳細 §7.2)。短縮形 `v` |
| `wetgain` | 任意 | F32 [0.0〜2.0] | `0.2` | 残響ウェット成分の倍率 (詳細 §7.3)。短縮形 `wg` |
| `lfegain` | 任意 | F32 [0.0〜4.0] | `1.0` | LFE チャンネルゲイン倍率 (詳細 §7.4、r12.1 追加)。短縮形 `lg` |
| `upmix` | 任意 | bool | `off` | stereo→5.1 アップミックス (詳細 §8)。短縮形なし |

#### 6.3.2 スピーカー宣言キー (任意のプリム)

| キー | 必須 | 型 | 既定値 | 意味 |
|---|---|---|---|---|
| `ch` | **必須** | 列挙値 | — | このプリムが受け持つチャンネル (詳細 §9) |
| `range` | 任意 | F32 (m) | ルートの `range` → `Stream3DRolloffMax` の順でフォールバック | このスピーカー個別の減衰距離 |
| `volume` | 任意 | F32 [0.0〜1.0] | 1.0 | このスピーカー個別の音量倍率 |

> **重要**: 単一プリム URL 再生の `min` / `max` キーはリンクセット配置では **無視** されます。リンクセット配置では近距離は内部固定で 1.0m、遠距離は `range` キー (または既定値 `Stream3DRolloffMax`) が使われます。

> **重要**: `source` / `link` / `face` は **ルートプリムでのみ意味がある source selection キー**です。スピーカープリムに書いても無視されます。

### 6.4 ルート 1 つ + 子 1 つ (基本のステレオペア)

最小のステレオ配置:

```
ルート Description:
  [3dstream:{url:http://example.com/stream.mp3}{ch:L}]

子 Description:
  [3dstream:{ch:R}]
```

ルート自身が L、子が R を担当します。ルートと子の **位置関係 (= リンク番号)** は再生に影響しません。空間内のどこに置くかが定位を決めます。

### 6.5 マルチスピーカー (4 個以上の配置)

同じステレオストリームを 4 つのスピーカーから鳴らす例 (会場の 4 隅):

```
ルート Description:
  [3dstream:{url:http://example.com/stream.mp3}{range:50}]

子 #1 Description:
  [3dstream:{ch:L}]

子 #2 Description:
  [3dstream:{ch:R}]

子 #3 Description:
  [3dstream:{ch:L}{volume:0.7}]

子 #4 Description:
  [3dstream:{ch:R}{volume:0.7}]
```

- ルートは音源宣言だけで、自分はスピーカーとして鳴りません (`{ch}` なし)
- ルートの `{range:50}` が 4 つの子スピーカーすべての既定減衰距離になります
- 子 #1, #2 は L/R それぞれ音量 100%
- 子 #3, #4 は同じ L/R を 70% で鳴らす (後方・補助スピーカー想定)
- 同じ `{ch}` を複数プリムに書くと、そのチャンネルを複数地点から鳴らせます。会場前方と後方に同じ L/R を置くような用途に使えます
- スピーカー数の上限は `Stream3DStereoMaxSpeakers` 設定で **既定 16 個まで** (§12)

### 6.6 5.1ch 会場配置 (6 プリム)

5.1ch ソース (Opus surround / FLAC 6ch) を 6 個のスピーカーに展開:

```
ルート Description:
  [3dstream:{url:http://example.com/test_5_1.flac}{range:30}]

FL プリム:  [3dstream:{ch:FL}]
FR プリム:  [3dstream:{ch:FR}]
C プリム:   [3dstream:{ch:C}]
LFE プリム: [3dstream:{ch:LFE}]
SL プリム:  [3dstream:{ch:SL}]
SR プリム:  [3dstream:{ch:SR}]
```

- 各プリムを物理的に「会場のスピーカー位置」に配置します (ステージ前 L/R、センター、サブウーファー、サラウンド L/R)
- LFE は他の 5 個と同等に扱われます (ローパスフィルタ等の特殊処理は viewer 側に入っていません。低域フィルタリングが必要なら配信側 mix で済ませてください)
- 映画館のように特定の座席付近を基準にしたい場合は、その位置をリスニングポイントとして想定して各スピーカーを配置できます。リスナーが会場内を歩き回る運用では、基準点から離れるほど定位は変化するため、5.1ch 各チャンネルを空間内のスピーカーとして配置する会場 PA 的な使い方になります。

### 6.7 Media/MOAP source を使う (r26 / r31 unified tag)

r26 では、HTTP URL ではなくリンクセット内の Media-on-a-Prim (MOAP) 面を 3D Stream の音源として使えるようになりました。r31 以降はルート Description に `[3dstream:{source:media}]` または `[3dstream:{source:media-5-1}]` を書き、スピーカープリムには `[3dstream:{ch:...}]` を持たせます。

```
ルート Description:
  [3dstream:{source:media}{ch:L}]

子 Description:
  [3dstream:{ch:R}]
```

media 面はルートプリム上にあっても、子プリム上にあっても構いません。media 面が 1 つだけなら `{link}` / `{face}` は省略できます。リンクセット内に media 面が複数ある場合は、どの面を 3D Stream に流すかをルートで明示します。

```
ルート Description:
  [3dstream:{source:media}{link:3}{face:2}{range:30}]

スピーカー #1 Description:
  [3dstream:{ch:L}]

スピーカー #2 Description:
  [3dstream:{ch:R}]
```

`{link:N}` は **media source を選ぶためだけ**のリンク番号です。スピーカー順や L/R の割り当てには影響しません。スピーカーの役割は常に各プリムの `{ch:...}` で決まります。複数 media 面があるのに `{link}` / `{face}` を省略して一意に決められない場合は、構造エラーになります。

URL 音源と media 表示は同居できます。ただし `{url}` と `{source:media}` は相互排他です。media 画面を見せながら別の URL ストリームを 3D 配置したい場合は、ルートに `{url:...}` だけを書き、media 面は 3D Stream source として選択しません。この場合、3D Stream のスピーカーから鳴るのは URL ストリームで、media 音声は通常の MOAP 音声として扱われます。

音量の扱いは media 面の数で変わります。media 面が 1 つだけの場合、その media の volume / mute は source gain として効きます。複数 media 面がある場合、3D に routed された選択 media は source gain 1.0 として扱われ、3D Stream 側の master volume / speaker volume で制御します。選択されなかった media 面は従来どおり通常の media volume で鳴ります。

media source のチャンネル指定:

- `{source:media}` / `{source:media-stereo}`: media を 2ch 音源として扱います。stereo media で `{upmix:on}` を使う場合もこの指定です。
- `{source:media-5-1}`: media を 5.1ch / 6ch 音源として扱います。スピーカー側は `FL / FR / C / LFE / SL / SR` を配置します。

このガイドで扱う media source は **2ch と 5.1ch (6ch)** までです。Dullahan/CEF の callback bus が 8ch で見える場合でも、3D Stream 側の 7.1ch speaker routing を実装済みとして扱うものではありません。

### 6.8 ルートプリムの判別方法

リンクセットを編集中、Build フローターの **Object** タブで「Selected」が表示されているプリムが選択中、その linkset の親 (= ルート) は通常 **最初に選択した状態でリンクされたプリム** です。

確認するもっとも確実な方法:
- Build → Edit → 「Edit linked」を OFF にしてプリムをクリック → そのリンクセットのルートが選択される
- LSL: `llGetLinkNumber()` でルートは `1` (子プリムが存在する場合)。子プリムなしの単一プリムは `0`

ルートと子の位置関係 (リンク番号 1, 2, 3, ...) は **3D Stream の再生に影響しません**。リンク番号で L/R を決めていた仕様は r5 までで廃止され、r8 以降は `{ch:...}` タグ宣言ベースになっています。

---

## 7. バイノーラル / 会場残響 (r12)

r10 までの 3D Stream でも、dry な素材を多点配置して空間内のスピーカーとして聴かせる運用は可能でした。r12 では、ヘッドホン再生時の定位補正や会場残響を viewer 内 DSP で追加できるタグキーを用意しています。

| キー | 短縮形 | 既定 | 機能 |
|---|---|---|---|
| `binaural` | `bin` | `off` | lite-HRTF (ITD + air absorption) によるヘッドホン定位強化 |
| `venue` | `v` | `dry` | 9 種の会場残響プリセット (convolution reverb) |
| `wetgain` | `wg` | `0.2` | 残響ウェット成分の倍率 (0.0〜2.0、推奨範囲 0.1〜0.5) |
| `lfegain` | `lg` | `1.0` | LFE チャンネルのゲイン倍率 (0.0〜4.0、r12.1 追加) |

これらのキーは音源全体の表現を決めるため、**音源宣言を持つ root プリムに書きます**。子プリムに書いても、そのスピーカーだけに個別適用されるわけではなく無視されます。リスナー側に個別の UI 項目はなく、会場側のタグ設定として適用されます (§7.5)。

### 7.1 `{binaural:on|off}` (短縮形 `bin`)

リスナー (ヘッドホン) の左右定位を強化する **lite-HRTF DSP** の有効化フラグです。

#### 動作

`on` のとき、各スピーカーチャンネルに対して以下の処理が掛かります:

- **ITD (interaural time delay)** ─ 左右の耳に届く時間差をスピーカー方位から計算 (Woodworth-Schlosberg 近似) し、sample-fractional delay として付与。「左の耳だけ大きい」感ではなく「左から音が来る」感に近づきます
- **air absorption (距離 HF rolloff)** ─ 距離が遠いほど高域が減衰 (`-0.5 dB/m`、上限 `-25 dB`)。50m 離れたスピーカーが暗く聴こえます

ILD (左右レベル差) は r10 までと同じく FMOD の `FMOD_3D_LINEARSQUAREROLLOFF` が担当します。

#### `off` を選ぶ場面

- 配信音源そのものが、すでにヘッドホン向けの立体音響として作られているとき
- 来場者の多くがヘッドホンではなくスピーカーで聴く前提の会場にしたいとき
- 追加の定位処理を掛けず、従来の 3D Stream と同じ素直な距離・方向表現にしたいとき

#### 既定が `off` の理由

既存配置の音の出方を、タグ無改修ではなるべく変えないためです。ヘッドホン向けの定位強化を使いたい会場では、root タグに `{bin:on}` または `{binaural:on}` を明示してください。

### 7.2 `{venue:NAME}` (短縮形 `v`)

会場残響のプリセット 9 種から 1 つを選びます。音源自体に強い残響が入っていると `venue` の残響と重なるため、3D Stream 側で会場の響きを調整したい場合は、配信音源をなるべく dry にしておくと扱いやすくなります。

#### プリセット一覧

| 値 (canonical) | 短縮形 | 想定用途 | RT60 目安 | CPU 増分 (r10 比) |
|---|---|---|---|---|
| `dry` | `d` | 残響なし (素材そのまま) | — | **0** (DSP 完全 bypass) |
| `room_small` | `rs` | 6〜10 畳の部屋 | ~0.3s | +0.1pp |
| `room_medium` | `rm` | 練習室 / 小ホール | ~0.6s | +0.1pp |
| `hall_small` | `hs` | 小規模ライブハウス | ~1.0s | ~+3pp |
| `hall_medium` | `hm` | ホール (300〜1000 席相当) | ~1.5s | **+7.7pp** |
| `hall_large` | `hl` | 大ホール | ~2.0s | **+9.6pp** |
| `club` | `cl` | クラブ / ダンスフロア (dense reflection) | ~0.8s | ~+5pp |
| `cathedral` | `ct` | カテドラル | ~3.0s | **+10.2pp** |
| `outdoor` | `od` | 野外 (軽い early reflection のみ) | ~0.2s | +0.1pp |

#### 既定が `dry` の理由

未指定では従来と同じ再生にするためです。`dry` のときは reverb DSP が挿入されないため、残響処理の CPU 負荷はありません。

#### CPU 負荷

`hall_medium` / `hall_large` / `club` / `cathedral` は IR (impulse response) が長く、partitioned FFT convolution の負荷が増えます。`cathedral` は r10 比 **+10.2pp**、1 コアで 53% 程度を消費します。多コア CPU では全体 CPU 換算で 3〜7% 程度ですが、低スペック環境を想定する会場では `room_small` / `room_medium` / `outdoor` から選ぶと負荷を抑えられます。

### 7.3 `{wetgain:N}` (短縮形 `wg`)

venue 残響の **ウェット成分** (= reverb 出力) の倍率です。dry signal はそのまま素通しされ、wet を `N` 倍してから dry に加算します。

| 値 | 効果 |
|---|---|
| `0.0` | 完全 dry (= venue=dry と同等。ただし DSP は挿入されたまま) |
| `0.1` | wet ごく薄め |
| **`0.2` (既定)** | wet 控えめ |
| `0.3〜0.5` | wet 中庸〜やや濃いめ |
| `1.0` 以上 | wet が dry と同程度以上になり、残響が目立ちやすい |
| `2.0` | wet 2 倍 (上限) |

#### 設計上のポイント

各 venue の IR は **unity-gain 正規化** されているため、`{wg:0.2}` は venue を切り替えても wet と dry の比率を保ちます。ただし RT60 が長い preset ほど残響が長く残るため、同じ `wetgain` でも聴感上は濃く感じる場合があります。

> **既定値の変更 (r12.1 で 1.0 → 0.2)**: `1.0` は wet と dry が同レベルになるため、ホール系やカテドラルでは残響が強く出すぎる場合がありました。r12.1 では通常の配信で扱いやすい値として既定値を `0.2` に下げています。LSL UI のクイック選択も `0.1`〜`0.5` を中心にしています。

#### `{venue:dry}` のとき

`venue` が `dry` のときは reverb DSP が **完全 bypass** されるため、`wetgain` は **無視** されます。

### 7.4 `{lfegain:N}` (短縮形 `lg`、r12.1 追加)

`{ch:LFE}` 経路、および `{upmix:on}` 時の **LFE 帯域** に対するゲイン倍率です。dry/wet とは独立に LFE のみを増減させます。

| 値 | 効果 |
|---|---|
| `0.0` | LFE 完全ミュート (LFE プリムから何も出ない / upmix 時は低域強調 OFF) |
| `0.5` | LFE 半減 |
| **`1.0` (既定)** | 素材ままのレベル (r12 互換動作) |
| `2.0` | LFE 2 倍 (低域を強調したい配置で常用域) |
| `4.0` | LFE 4 倍 (上限、低域 PA 想定) |

#### 想定用途

- **5.1 native 配信** (`{ch:LFE}` プリムを置く配置) で、配信側の LFE バスが控えめに収録されている素材を viewer 側で持ち上げる
- **`{upmix:on}` でステレオ → 5.1 展開** したとき、80 Hz LPF を通った成分が物足りない場合に強調する
- 逆に LFE プリムをサブウーファー筐体ではなく汎用スピーカーに割り当てる配置で、`0` にして低域漏れを止める

#### `{ch:LFE}` プリムが無い / upmix off のとき

LFE 経路自体が動作しないため、`lfegain` は **意味を持ちません** (記述しても無視)。

#### listener 側 sentinel

Debug Settings に `Stream3DLfeGain` もあります。`-1.0` はタグ通り、それ以外の `0.0〜4.0` は listener 側の上書き値として扱います (詳細 §12.2)。通常の Preferences には表示されません。

### 7.5 配信者主導モデル

これら 4 キー (`binaural` / `venue` / `wetgain` / `lfegain`) は、root prim Description のタグで指定します。通常の Preferences には、listener が個別に変更するための UI はありません。

#### なぜ listener UI を提供しないのか

- 会場側が指定した残響や binaural 設定を listener ごとに変更できると、同じ会場でも聴こえ方が大きく変わります
- AYAstorm では、会場の音作りに関わる設定はタグ側に寄せ、listener 側の通常 UI には出さない設計にしています

#### 例外: debug settings による個人側 override

Debug Settings の `Stream3DBinauralRender` で、タグに関係なく binaural を強制 OFF / ON できます。検証用の上書き設定であり、通常の Preferences には表示されません。同様に `Stream3DVenueOverride` (空文字 = タグ通り、`"dry"` で全 reverb 強制 OFF) / `Stream3DVenueWetGain` (sentinel `-1.0` = タグ通り) / `Stream3DLfeGain` (sentinel `-1.0` = タグ通り、r12.1) もあります。

### 7.6 組合せ例

#### 何も書かない (= 既定)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}]
```
→ `{bin:off}{v:d}{wg:0.2}` 相当。追加の binaural / venue reverb は掛からず、dry な 3D Stream として再生されます。

#### ライブハウス (PA 想定、打ち込み系)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:cl}{wg:0.3}]
```
→ club preset を使い、反射の多い会場として再生します。`wg:0.3` は既定より少し残響を足す設定です。

#### 大ホール (オーケストラ)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:hl}{wg:0.25}]
```
→ hall_large を使い、wet を 0.25 倍にします。ホール残響が長いため、残響量は控えめにしています。

#### カテドラル (アンビエント / 環境音)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:ct}{wg:0.15}]
```
→ cathedral を使い、wet を 0.15 倍にします。RT60 が約 3 秒あるため、長い残響が前に出すぎない値にしています。

#### 野外 (環境音 / 散歩 BGM)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:od}{wg:0.2}]
```
→ outdoor を使い、軽い early reflection を加えます。`wg:0.2` は既定値です。

#### 既にバイノーラル済の素材 (二重処理回避)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:off}{v:d}]
```
→ binaural OFF、reverb なし (= r10 までの動作)。

---

## 8. stereo→5.1 upmix (r12)

r10 の per-channel 配置 (FL/FR/C/LFE/SL/SR の 6 spk) は **5.1 配信** 用の機能でした。一方で、SL 配信で使われるソフト (butt / Mixxx / OBS / SAM 等) は stereo 配信が中心です。

> **TIPS:** 5.1ch で配信したい場合は、本 repo の contributor である t-noami が作成した [SurroundStreamer](https://github.com/t-noami/SurroundStreamer) を利用できます。

r12 で追加された `{upmix:on}` キーは、viewer 内 DSP で **stereo 2ch を 6ch に展開**します。配信側を 5.1ch に変更しなくても、6 spk 配置を使った再生ができます。

### 8.1 `{upmix:on|off}` (短縮形なし)

| 値 | 効果 |
|---|---|
| **`off` (既定)** | upmix 無効。stereo source は r10 までと同じく `{ch:L}` `{ch:R}` `{ch:M}` 経路に流れる |
| `on` | stereo source を 6ch 化し、`{ch:FL}` `{ch:FR}` `{ch:C}` `{ch:LFE}` `{ch:SL}` `{ch:SR}` プリムに流す |

#### 既定が `off` の理由

upmix DSP は CPU を消費し、音像も変化するため、未指定では従来と同じ stereo 再生にします。6 spk 展開を使う場合は `{upmix:on}` を明示します。

#### 既存配置への効果

r8/r10 で作成した 6 spk リンクセットは、root タグに `{upmix:on}` を追加すると stereo source でも 6 spk に展開されます。`{upmix:off}` または未指定の場合は従来通りです。

### 8.2 アルゴリズム (matrix upmix + 帯域分離)

upmix は stereo source から center / surround / LFE 成分を生成する固定アルゴリズムです。`(L+R)` / `(L-R)` の matrix 処理に帯域分離と rear delay を組み合わせています。配信者が指定するのは `on/off` のみで、アルゴリズムを選ぶタグはありません。

| 出力チャンネル | 派生方法 (概要) |
|---|---|
| `C` (center) | `(L+R)/√2` (in-phase 成分) |
| `Ls` / `Rs` (rear) | `(L-R)/√2` を decorrelate (固定 delay 16ms ± jitter で L/R 分離) |
| `LFE` | `(L+R)` を 80Hz LPF |
| `FL` / `FR` (front) | `L` / `R` から center 成分を `bleed_amount` で除去 (default フル除去 = phantom center を center spk に集約) |

通常の matrix decode に加えて、以下の処理を行います:

- **LFE LPF**: 低域だけを LFE spk に分配
- **Center bleed 除去**: phantom center の二重像 (center spk + front L/R 両方から鳴る現象) を防止
- **Rear decorrelation**: surround の左右をわずかな時間差で分離、空間広がりを生成

機械学習ベースの upmix は使用していません。入力に対する出力が予測しやすい方式を優先しています。

### 8.3 5.1 native 配信での自動 bypass

source の channel 数が **6 以上** (= 5.1 native 配信、Vorbis 6ch / Opus surround / FLAC 6ch) のときは、`{upmix:on}` が指定されていても **upmix を自動的に bypass** します。二重処理防止のためです。

このとき Local Chat に **1 回だけ** 通知が出ます:

```
3D Stream: 5.1 native source detected (6ch), upmix:on tag auto-bypassed
```

配信を 5.1 ↔ stereo で切り替える運用 (例: 「ライブ本番は 5.1ch、休憩中は通常のステレオ BGM」) では、`{upmix:on}` を付けたままにしておけば自動で正しく動作します。

### 8.4 微調整 (debug settings 3 件)

upmix DSP 内部のパラメータは、配信者タグではなく listener 側 Debug Settings にあります。タグで指定するのは on/off のみです。通常は変更不要で、実装確認や個別調整が必要な場合に使います。

| Debug 設定キー | 既定 | 範囲 | 意味 |
|---|---|---|---|
| `Stream3DUpmixLfeCutoff` | `80.0` Hz | 20〜200 | LFE LPF の cutoff 周波数 |
| `Stream3DUpmixCenterBleed` | `1.0` | 0.0〜1.0 | front L/R から center 成分を引く割合 (`0` で除去なし、`1` でフル除去) |
| `Stream3DUpmixRearDelayMs` | `16.0` ms | 0〜32 | rear decorrelation の base delay (L/R は ±2ms jitter で分離) |

別途 listener 側の強制 OFF / 強制 ON 用に sentinel 1 件:

| Debug 設定キー | 既定 | 意味 |
|---|---|---|
| `Stream3DUpmix` | `-1` (sentinel = タグ通り) | `0` でタグ無視・強制 OFF / `1` で強制 ON (※ 5.1 native の auto bypass は常に効く) |

### 8.5 組合せ例

#### 既定動作 (upmix なし)

```
ルート Description:
  [3dstream:{url:http://example/stereo.ogg}{ch:L}]

子 Description:
  [3dstream:{ch:R}]
```
→ stereo source の L/R を 2 個のスピーカープリムに流します。`{upmix:on}` を書かない場合、C / LFE / SL / SR は生成されません。

#### 6 spk 配置 + upmix

```
ルート Description:
  [3dstream:{url:http://example/stereo.ogg}{upmix:on}{range:30}]

FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```
→ stereo source から FL / FR / C / LFE / SL / SR を生成し、各スピーカープリムに割り当てます。

#### upmix + binaural + venue

```
ルート Description:
  [3dstream:{url:http://example/stereo.ogg}{upmix:on}{bin:on}{v:hm}{wg:0.25}{range:30}]

FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```
→ 6 spk upmix に、lite-HRTF と hall_medium reverb を組み合わせます。`wg:0.25` は hall_medium 向けの控えめな残響量です。

#### 2 spk 配置に `{upmix:on}` を書いた場合

```
ルート Description:
  [3dstream:{url:http://example/stereo.ogg}{upmix:on}{ch:L}]
子: [3dstream:{ch:R}]
```
→ `ch:L` / `ch:R` は FL / FR 相当として扱われますが、C / LFE / SL / SR を受けるプリムが無いため、それらのチャンネルは鳴りません。2 spk 配置では通常 `{upmix:on}` は不要です。center や surround を鳴らしたい場合は、上の 6 spk 配置にしてください。

#### 5.1 native 配信に upmix を付けてしまった場合

```
ルート Description:
  [3dstream:{url:http://example/stream_5_1.ogg}{upmix:on}{range:30}]

FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```
→ 6ch source を検出すると upmix は自動 bypass され、Local Chat に 1 回通知されます。各スピーカープリムには 5.1 native source の各チャンネルがそのまま流れます。

---

## 9. ch (チャンネル) 値リファレンス

`{ch:値}` には以下の値が指定できます。**大文字小文字は区別しません** (`{ch:l}` も `{ch:L}` も同じ)。

| 値 | 意味 | 主な用途 |
|---|---|---|
| `L` | 左チャンネル | ステレオの L 担当 |
| `R` | 右チャンネル | ステレオの R 担当 |
| `M` | モノラル (L+R の平均) | 「中央スピーカー」用途、または 1 点だけで鳴らす場合 |
| `FL` | Front Left | 5.1ch のフロント左 |
| `FR` | Front Right | 5.1ch のフロント右 |
| `C` | Center | 5.1ch のセンター |
| `LFE` | Low Frequency Effects (サブウーファー) | 5.1ch の低域 |
| `SL` | Surround Left | 5.1ch のサラウンド左 |
| `SR` | Surround Right | 5.1ch のサラウンド右 |

ソースの実 ch 数と書式の `ch` 値が合わない場合、「不整合」ではなく「自動フォールバック」が働きます (§10)。例えば 2ch ソースに `{ch:FL}` を書くと L が再生されます。

不正値 (`{ch:foo}` 等) は **書式エラー**として通知されます (§13)。

---

## 10. ソース ch 数 × タグ値 の互換マトリクス

実際にスピーカープリムから何が鳴るかは、**ソースのチャンネル数** と **書いた `ch` 値** の組み合わせで決まります。

### 10.1 互換マトリクス

| ソース | `{ch:L}` | `{ch:R}` | `{ch:M}` | `{ch:FL}` | `{ch:FR}` | `{ch:C}` | `{ch:LFE}` | `{ch:SL}` | `{ch:SR}` |
|---|---|---|---|---|---|---|---|---|---|
| **1ch (mono)** | M | M | M | M | M | M | 無音 | 無音 | 無音 |
| **2ch (stereo)** | L | R | (L+R)/2 | L | R | (L+R)/2 | 無音 | 無音 | 無音 |
| **6ch (5.1)** | BS.775 L | BS.775 R | (BS.775 L + R)/2 | FL | FR | C | LFE | SL | SR |

凡例:
- `L` / `R` / `FL` / `FR` / `C` / `LFE` / `SL` / `SR` はソース該当チャンネルの直接再生
- `BS.775 L` = ITU-R BS.775 ダウンミックス係数で 6ch を L/R 2ch に縮約した値 (§10.2)
- `無音` = そのスピーカーは音を出しません (binding は維持されますがプリムから音が出ない)

### 10.2 BS.775 ダウンミックス係数 (6ch ソース → L/R)

```
L_out = c × ( FL + 0.707·C + 0.707·SL + 0.5·LFE )
R_out = c × ( FR + 0.707·C + 0.707·SR + 0.5·LFE )
c = 1 / 2.914 ≒ 0.343 (clipping 防止の正規化)
```

センターは均等に左右に振り分け、サラウンドは同側に、LFE は両側に均等に混ぜます。

### 10.3 同じソースを混在配置で使う

5.1ch ソースを `{ch:L}` と `{ch:FL}` の両方に割り当てると、L プリムは BS.775 ダウンミックスで、FL プリムはダイレクトに鳴ります。混乱しやすいので、**同じソースには同じ系統の ch (`L/R/M` 系 か `FL/FR/...` 系 のどちらか) で揃える** のが推奨です。

混在状態でフォールバックが起きた場合、**routing 診断 chat 通知** (§12.3 / §13.3) で各 ch の実際の挙動を確認できます。5.1ch 会場の構築中はこれを ON にしておくと配置ミスがすぐ見つかります。

### 10.4 5.1ch 会場配置のまま 2ch / 1ch ソースを流したとき

会場に 6 個のスピーカープリム (`ch:FL` / `FR` / `C` / `LFE` / `SL` / `SR`) を配置済みの状態で、ソース URL を 5.1ch 配信から **普通のステレオ (2ch) 配信** や **モノラル (1ch) 配信** に切り替えるケース。例えば「ライブ本番は 5.1ch、休憩中は通常のステレオ BGM」「DJ セットの間に MC のモノラル音声を挟む」といった運用です。

このとき **配置変更・設定変更は一切不要** です。各スピーカープリムは自動的に以下のように振る舞います。

#### 2ch (ステレオ) ソースが流れた場合

| プリムの `ch` 値 | 鳴る内容 |
|---|---|
| `{ch:FL}` | **L** (フロント左の代わりにステレオ L 直取り) |
| `{ch:FR}` | **R** (フロント右の代わりにステレオ R 直取り) |
| `{ch:C}` | **(L+R)/2** (センターは L+R の平均 = モノラルダウンミックス) |
| `{ch:LFE}` | **無音** (LFE 信号がソースに存在しない) |
| `{ch:SL}` | **無音** (サラウンド左信号がソースに存在しない) |
| `{ch:SR}` | **無音** (サラウンド右信号がソースに存在しない) |

聴感上は「**会場前方の 3 本 (FL / FR / C) でステレオが鳴り、サラウンド 3 本 (LFE / SL / SR) は黙る**」状態になります。

#### 1ch (モノラル) ソースが流れた場合

| プリムの `ch` 値 | 鳴る内容 |
|---|---|
| `{ch:FL}` / `{ch:FR}` / `{ch:C}` | **M** (フロント 3 本ともモノラル直取り、3 箇所から同じ音が鳴る) |
| `{ch:LFE}` / `{ch:SL}` / `{ch:SR}` | **無音** |

#### 設計意図 / 補足

- **フロント 3 本 (FL / FR / C) は、どんな ch 数のソースでも必ず何かしら鳴る** ─ これにより 5.1ch ↔ 2ch ↔ 1ch とソースを切り替えても、会場前方からの音が途絶えません。
- **LFE / SL / SR はソースに該当信号がない場合は無音で固定** ─ 偽の bass や偽のサラウンドを合成して鳴らすことはしません。
- **ソースが 5.1ch に戻ると、各プリムは自動的に該当チャンネルの直取りに復帰** します (URL 切替後の reconnect で再評価)。再配置・再設定は不要です。
- 5.1ch 会場のまま日常的に 2ch BGM を流すのは **正規の運用パターン** です。「サラウンドが黙る」のは仕様であり、不具合ではありません。

#### 設定でフォールバック内容を Chat に出す (推奨: 構築・検証中)

「無音になっている prim は本当にフォールバック仕様で黙っているのか、それとも何か壊れているのか」を **Local Chat で確認できる診断スイッチ** が用意されています。

**設定の場所** (どちらでも同じ動作、両者は同期):

- **Preferences > Sound > Show channel routing diagnostics in chat** (チェックボックス)
- **Debug Settings: `Stream3DRoutingDiagnostic`** (`true` / `false`)

ON にすると、5.1 配置 × 2ch / 1ch ソースのフォールバック発生時に **Local Chat の自分自身からの発言** として以下のような行が出ます (`3D Stream:` プレフィクス付き、§13.3 の helper 経由):

**2ch ソース × 5.1 配置 (FL/FR/C/LFE/SL/SR の 6 prim) の場合**:

```
[12:34] You: 3D Stream: ch:FL prim playing L (source is 2ch)
[12:34] You: 3D Stream: ch:FR prim playing R (source is 2ch)
[12:34] You: 3D Stream: ch:C prim playing (L+R)/2 (source is 2ch)
[12:34] You: 3D Stream: ch:LFE prim silent (source is 2ch)
[12:34] You: 3D Stream: ch:SL prim silent (source is 2ch)
[12:34] You: 3D Stream: ch:SR prim silent (source is 2ch)
```

**1ch ソース × 5.1 配置 の場合**:

```
[12:35] You: 3D Stream: ch:FL prim playing M (source is 1ch)
[12:35] You: 3D Stream: ch:FR prim playing M (source is 1ch)
[12:35] You: 3D Stream: ch:C prim playing M (source is 1ch)
[12:35] You: 3D Stream: ch:LFE prim silent (source is 1ch)
[12:35] You: 3D Stream: ch:SL prim silent (source is 1ch)
[12:35] You: 3D Stream: ch:SR prim silent (source is 1ch)
```

これで「LFE / SL / SR が無音なのは仕様、FL / FR / C はフォールバック動作中」が一目で分かります。

通知は `(root_id, url, observed_channel_count, prim_set_signature)` の組をキーに throttle されるため、配置やソース ch 数が変わるまで同じ通知は再送されません。**5.1ch 会場の構築・検証中だけ ON にして、本番では OFF (`false`、これが既定値)** が推奨です。詳しくは §13.3 を参照してください。

#### 逆方向: 2ch 配置のまま 5.1ch ソースが流れた場合

参考までに反対方向も整理します。`{ch:L}` / `{ch:R}` / `{ch:M}` だけで配置したステレオ会場 (= 2ch 配置) に 5.1ch ソースが流れた場合は、

- L / R / M プリムが **BS.775 ダウンミックス** (§10.2) で 5.1ch を 2ch に縮約して鳴らします。FL / C / SL / LFE はすべて L 側に、FR / C / SR / LFE はすべて R 側に係数付きで合成されます。
- すべての ch 信号が L / R 経由で聴こえるため、**音が消えるチャンネルはありません**。
- `Stream3DRoutingDiagnostic` ON 時の Local Chat 出力例:

```
[12:36] You: 3D Stream: FL content folded into BS.775 downmix (source is 6ch, no ch:FL prim)
[12:36] You: 3D Stream: C content folded into BS.775 downmix (source is 6ch, no ch:C prim)
[12:36] You: 3D Stream: LFE content folded into BS.775 downmix (source is 6ch, no ch:LFE prim)
[12:36] You: 3D Stream: SL content folded into BS.775 downmix (source is 6ch, no ch:SL prim)
[12:36] You: 3D Stream: SR content folded into BS.775 downmix (source is 6ch, no ch:SR prim)
```

「専用 prim がないので BS.775 ダウンミックス path に折り込んだ」旨が ch 単位で通知されます。

---

## 11. 配信側 (ソース URL の作り方)

### 11.1 対応 codec / コンテナ

| codec / コンテナ | 1ch | 2ch | 6ch | 備考 |
|---|---|---|---|---|
| **MP3** | ✓ | ✓ | — | SHOUTcast / Icecast の伝統的経路 |
| **Vorbis (Ogg)** | ✓ | ✓ | ✓ | 6ch も実機検証済み (r9 P10) |
| **Opus (Ogg)** | ✓ | ✓ | ✓ | 6ch は Opus channel mapping family 1。r9-opus 補完で実機検証済み |
| **FLAC** | ✓ | ✓ | △ | 6ch の layout は実装済みだが、配信経路によって seek 制約あり (§11.4) |
| AAC (ADTS / HLS) | — | — | — | 非対応 |
| AC-3 / E-AC-3 | — | — | — | ライセンス上の理由で非対応 |

ソース URL は `http://` / `https://` のいずれも受け付けます。HTTP/1.1 keep-alive を維持する経路 (= SHOUTcast 互換 streamer や ffmpeg の TCP 出力) のほうが、単純な静的 HTTP より安定する傾向があります。

### 11.2 1ch / 2ch の配信

1ch / 2ch の音源は、SHOUTcast / Icecast / 静的 HTTP のいずれでも配信できます。codec は MP3 / Vorbis / Opus / FLAC に対応しています。配信には `oggenc`、ffmpeg、butt など一般的な配信ツールを使用できます。

### 11.3 5.1ch (Vorbis / Opus 6ch) の配信

5.1ch 配信には **Vorbis 6ch** または **Opus 6ch** を使用できます。どちらも viewer 側での 6ch 再生を確認済みです。ミュージシャンや DJ が Opus 6ch で配信する場合は、t-noami 作成の [SurroundStreamer](https://github.com/t-noami/SurroundStreamer) を利用できます。以下では、検証用の音源を作りやすい Vorbis 6ch の例を示します。

#### 11.3.1 テスト素材作成 (ffmpeg)

```bash
# 各 ch にユニーク周波数を埋めた 5.1 WAV (10 秒)
ffmpeg -f lavfi -i "sine=440:d=10" -f lavfi -i "sine=550:d=10" \
       -f lavfi -i "sine=660:d=10" -f lavfi -i "sine=110:d=10" \
       -f lavfi -i "sine=770:d=10" -f lavfi -i "sine=880:d=10" \
       -filter_complex "[0:a][1:a][2:a][3:a][4:a][5:a]amerge=inputs=6[a]" \
       -map "[a]" -ac 6 -channel_layout 5.1 test_5_1.wav

# Vorbis 6ch にエンコード
ffmpeg -i test_5_1.wav -c:a libvorbis -q:a 5 test_5_1.ogg
```

#### 11.3.2 静的 HTTP 配信 (検証用)

```bash
python3 -m http.server 8080
```

URL: `http://<host>:8080/test_5_1.ogg`

#### 11.3.3 リアルタイム配信 (ffmpeg → Icecast)

```bash
ffmpeg -re -i test_5_1.wav \
  -c:a libvorbis -q:a 5 \
  -ac 6 -ar 48000 \
  -content_type audio/ogg \
  -f ogg icecast://source:hackme@localhost:8000/aya_5_1.ogg
```

主要オプション:
- `-re` = real-time (素材長に合わせて流す。生配信のシミュレーション)
- `-content_type audio/ogg` = Icecast に MIME を申告 (これがないと MP3 と誤判定する)
- `-ac 6 -ar 48000` = 6ch 48kHz を維持

### 11.4 Opus 6ch と FLAC 6ch の扱い

Opus 6ch は r9-opus の codec plugin 経由で decode されるため、通常の Ogg/Opus 6ch 配信では FMOD parser の seek 制約を受けません。Opus channel mapping family 1 の 6ch source は実機 Icecast で再生確認済みです。ライブ配信で 5.1ch を扱う場合は、Opus 6ch が実用的な選択肢になります。

FLAC 6ch は codec layout 自体は実装済みですが、FLAC parser が seek を要求する場合があります。配信経路が seek に対応していない場合、`FMOD_ERR_FILE_COULDNOTSEEK` で開けないことがあります。FLAC 6ch を使う場合は、実際に使う配信経路で事前に再生確認してください。

運用上の目安:

- ライブ 5.1ch 配信では **Opus 6ch** を優先します
- ミュージシャン / DJ が Opus 6ch 配信を行う場合は **SurroundStreamer** を利用できます
- 検証用の静的ファイルでは **Vorbis 6ch** が扱いやすい形式です
- FLAC 6ch は、配信経路で seek できることを確認できる場合に限って使ってください

### 11.5 配信側ツールの選び方

| ツール | 用途 | 注意 |
|---|---|---|
| **SurroundStreamer** | Opus 6ch の 5.1ch 配信 | ミュージシャン / DJ 向け。t-noami 作成。詳細は [SurroundStreamer](https://github.com/t-noami/SurroundStreamer) |
| **ffmpeg** | 検証素材作成 / Vorbis 6ch 配信 / codec 変換 | CLI 操作が必要。テストや自動化に向く |
| **butt** | 1ch / 2ch のライブ配信 | 5.1ch 配信には使いません |
| **Liquidsoap** | 放送オートメーション / サーバー側処理 | 設定の難易度が高い。導入前に 6ch の維持を確認してください |
| **Mixxx / DarkIce / ezstream** | DJ / 自動化 | 基本的に stereo 前提。5.1ch 配信には使いません |

---

## 12. viewer 側の設定

### 12.1 Preferences 経由

**環境設定 → Sound** タブに以下のコントロールがあります:

- **3D Stream** スライダー — 全ストリームの音量倍率 (`Stream3DVolumeMaster`)
- **Enabled** チェックボックス — 機能全体の ON/OFF (`Stream3DEnabled`)
- **Preferences > Sound > Show channel routing diagnostics in chat** — routing 診断通知 (`Stream3DRoutingDiagnostic`、§13.3)
- **Hear media and sounds from:** — リスナー位置を Camera / Avatar から選択 (`MediaSoundsEarLocation`、§15.1)

スピーカーアイコンの Volume プルダウンにも同じ「3D Stream」スライダーが現れ、ボイスチャットの近くから音量を即時調整できます。

### 12.2 Debug Settings (詳細チューニング)

`Ctrl + Alt + D` で Advanced メニューを出し → Show Debug Settings から各キーを直接編集できます。

| 設定キー | 型 | 既定値 | 意味 |
|---|---|---|---|
| `Stream3DEnabled` | bool | `true` | 機能全体のキルスイッチ。`false` で全 binding を即座に解除、再 enable しても自動再 bind しない (次の poll で再発見) |
| `Stream3DDescriptionScan` | bool | `true` | `false` で Description タグスキャンを停止し、すべてのプリム binding を解除 (debug stream は影響受けない) |
| `Stream3DMaxConcurrent` | S32 | `4` | 同時 binding 上限 (mono + stereo の合計)。0 で無制限 |
| `Stream3DStereoMaxSpeakers` | S32 | `16` | 1 リンクセットあたりのスピーカー数上限。超過分は traversal 末尾から落ちて警告通知 |
| `Stream3DRolloffMin` | F32 (m) | `1.0` | 単一プリム URL 再生の既定近距離 (`{min}` 省略時) |
| `Stream3DRolloffMax` | F32 (m) | `20.0` | 既定の遠距離 (mono の `{max}` / stereo の `{range}` 省略時の共通フォールバック) |
| `Stream3DMaxDistance` | F32 (m) | `64.0` | プリム検出のポーリング半径。`Stream3DRolloffMax` 以上に設定 |
| `Stream3DPollInterval` | F32 (秒) | `30.0` | Description ポーリング間隔。LSL 経由のタグ変更検出にこの間隔がかかる。0 で能動ポーリング無効 |
| `Stream3DVolumeMaster` | F32 [0〜1] | `0.5` | マスター音量倍率。Preferences の 3D Stream スライダーと同じ |
| `Stream3DReconnectAttempts` | S32 | `3` | ストリーム切断時の自動再接続試行回数。各リトライは 5 秒待機。0 で再接続無効 |
| `Stream3DRoutingDiagnostic` | bool | `false` | routing 診断 chat 通知の ON/OFF (§13.3)。Preferences のチェックボックスと同期 |

#### r11/r12 の listener 側上書き設定

§7.5 / §8.4 で説明したとおり、`binaural` / `venue` / `wetgain` / `lfegain` / `upmix` はタグで指定します。通常の Preferences には UI を出していませんが、検証や個別調整用に Debug Settings から上書きできます。

| 設定キー | 型 | 既定値 | 意味 |
|---|---|---|---|
| `Stream3DBinauralRender` | S32 | `-1` (sentinel = タグ通り) | `0` で listener 側強制 OFF / `1` で強制 ON。`-1` かつ `{binaural}` 未指定なら off (詳細 §7.5 例外節) |
| `Stream3DVenueOverride` | 文字列 | `""` (sentinel = タグ通り) | `"dry"` 等の venue 名を入れると全配信を強制その venue で聴く (`"dry"` で全 reverb 強制 OFF が代表用途) |
| `Stream3DVenueWetGain` | F32 | `-1.0` (sentinel = タグ通り) | `0.0〜2.0` の値で wet gain を強制上書き |
| `Stream3DLfeGain` | F32 | `-1.0` (sentinel = タグ通り) | `0.0〜4.0` の値で LFE gain を強制上書き (r12.1 追加、詳細 §7.4) |
| `Stream3DUpmix` | S32 | `-1` (sentinel = タグ通り) | `0` でタグ無視・強制 OFF / `1` で強制 ON。5.1 native の auto bypass は常に効く (詳細 §8.1 / §8.3) |
| `Stream3DUpmixLfeCutoff` | F32 (Hz) | `80.0` | upmix DSP の LFE LPF cutoff (20〜200)。詳細 §8.4 |
| `Stream3DUpmixCenterBleed` | F32 | `1.0` | upmix DSP の center bleed 除去率 (0.0〜1.0)。詳細 §8.4 |
| `Stream3DUpmixRearDelayMs` | F32 (ms) | `16.0` | upmix DSP の rear decorrelation delay (0〜32)。詳細 §8.4 |

> **r12.1 のライブチューニング修正**: r12 リリース時、上表の `Stream3DUpmixLfeCutoff` / `Stream3DUpmixCenterBleed` / `Stream3DUpmixRearDelayMs` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` を変更しても、対象プリムに **タッチ (Description 再パース) するまで反映されない** 不具合がありました。r12.1 でポーリングループ側に push を追加し、値を変更した次フレームから即時反映されるよう修正しています (§12.3)。

#### Debug 専用 (動作確認用)

| 設定キー | 型 | 用途 |
|---|---|---|
| `Stream3DDebugUrl` | 文字列 | デバッグ用 URL |
| `Stream3DDebugPlay` | bool | `true` でアバター前方 5m にモノラルストリームを設置・再生 (タグ書き込み不要のクイックテスト) |
| `Stream3DDebugStereoPlay` | bool | 同様にステレオ版デバッグ再生 |

### 12.3 設定の永続化と即時反映

ほとんどの設定は **「Live」** = 値を変更した次フレームから反映されます。Viewer 再起動は不要です。例外:

- `Stream3DEnabled` を `false` → `true` に切替: 自動で再 bind されません。次の poll cycle (既定 30 秒以内) で再発見されます。
- `Stream3DDescriptionScan` を切替: 即時に全 binding 解除 / 再発見の挙動。

> **r12.1 で修正**: r12 リリース時、`Stream3DUpmixLfeCutoff` / `Stream3DUpmixCenterBleed` / `Stream3DUpmixRearDelayMs` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` の各設定は、値変更後に **対象プリムを一度タッチ (Description 再パース) するまで反映されない** 仕様回帰がありました。r12.1 で `LLPositionalStreamMgr::update()` のポーリングループに per-poll push を追加し、これらも他の設定同様に「次フレーム反映」になっています。

---

## 13. エラー通知 / 診断

### 13.1 通知の出方

タグの書式エラーや構造エラーは **ローカルチャットに通知** されます。先頭に「3D Stream:」が付き、システムメッセージとして表示されます。

```
3D Stream: タグ書式エラー (オブジェクト名: "MySpeaker")
  ch の値が L/R/M/FL/FR/C/LFE/SL/SR のいずれかである必要があります。
  例: [3dstream:{ch:L}{range:30}]
```

```
3D Stream: 構造エラー (リンクセット root: "MainStage")
  音源宣言 (url/source) が root にあるがスピーカー (ch) が見つかりません。
  各スピーカープリムに [3dstream:{ch:L|R|M}] を記載してください。
```

### 13.2 30 秒抑制

同じプリム × 同じエラー種別の通知は **30 秒間抑制** されます。タグを連続編集している間にチャットが洪水になるのを防ぐためです。30 秒以上経つと再度 1 回だけ通知が出ます。

抑制された通知は LL_DEBUGS ログ (debug 用ログ) には残るので、内部で何が起きているかは log で確認できます。

### 13.3 Routing 診断 (5.1ch 配置時)

5.1ch / マルチスピーカー会場の **構築・検証中** に「どのプリムが何を鳴らしているか / なぜ無音なのか」を Local Chat で確認できる診断機能です。**既定は OFF**、明示的に ON にしないと出ません。

#### 13.3.1 ON にする方法

以下のどちらでも有効化できます (両者は同期しています):

- **Preferences > Sound > Show channel routing diagnostics in chat** ─ チェックボックスを ON
- **Debug Settings: `Stream3DRoutingDiagnostic` を `true`** に設定 (Advanced > Show Debug Settings から)

設定は即時反映されます。Viewer 再起動不要です。

#### 13.3.2 出力先と書式

ON にすると、フォールバック発生時に **Local Chat の自分自身からの発言** として以下の書式で 1 行ずつ出ます。

```
[HH:MM] You: 3D Stream: <内容>
```

`3D Stream:` プレフィクスは `notifyStream3D` ヘルパが自動付与します。チャットログ (`Show in Chat`) にも当然残るため、検証後に見返せます。**他人には見えません** (自分の Local Chat にのみ表示される擬似発言)。

#### 13.3.3 通知文言一覧

| 状況 | Local Chat に出る行 |
|---|---|
| 6ch ソース × `ch:L/R/M` プリムあり (専用 prim なし) | `3D Stream: FL content folded into BS.775 downmix (source is 6ch, no ch:FL prim)` (FL/FR/C/LFE/SL/SR の各 ch について個別に出る) |
| 6ch ソース × 専用 prim も `ch:L/R/M` prim もなし | `3D Stream: FL content has no destination — dropped (source is 6ch, no ch:L/R/M prim)` |
| 2ch ソース × `ch:FL` プリム | `3D Stream: ch:FL prim playing L (source is 2ch)` |
| 2ch ソース × `ch:FR` プリム | `3D Stream: ch:FR prim playing R (source is 2ch)` |
| 2ch ソース × `ch:C` プリム | `3D Stream: ch:C prim playing (L+R)/2 (source is 2ch)` |
| 1ch ソース × `ch:FL/FR/C` プリム | `3D Stream: ch:FL prim playing M (source is 1ch)` (FR / C も同様の書式) |
| 1ch / 2ch ソース × `ch:LFE/SL/SR` プリム | `3D Stream: ch:LFE prim silent (source is 2ch)` (1ch のときは `1ch`、SL / SR も同様) |

5.1ch 配置 × 2ch ソースの具体的な出力サンプルは §10.4 の「設定でフォールバック内容を Chat に出す」を参照してください。

#### 13.3.4 throttle と再表示条件

通知は `(root_id, url, observed_channel_count, prim_set_signature)` の組をキーに throttle されます。同じ会場 / 同じソース構成のままでは **再表示されません** (Chat が洪水になるのを防ぐため)。以下のいずれかが変わると再評価され、再度通知が出ます:

- ソース URL が変わる (= 別ストリームに差し替え)
- ソースの ch 数が変わる (= 同じ URL でも 5.1ch ↔ 2ch 切替が起きた)
- 会場のスピーカープリム構成が変わる (prim 追加 / 削除 / `ch` 値変更)
- `Stream3DRoutingDiagnostic` を OFF→ON に切り替えた直後

#### 13.3.5 運用推奨

- **会場の組み立て中・配置検証中は ON** にして、各 prim のフォールバック挙動を Local Chat で確認
- **本番運用 (ライブ中など) は OFF** に戻す。チャットを綺麗に保つため
- 既定 (OFF) は本番想定。配置検証時のみ手動で ON にする運用を想定しています

### 13.4 ログ (`LL_INFOS("Stream3D")`)

詳細な動作ログは AYAstorm のログファイルに記録されます。

```
~/.ayastorm_x64/logs/AYAstorm.log
```

`Stream3D` channel で grep すると、binding の確立 / 切断 / 再接続 / source format 検出 / dropout 等が確認できます。

---

## 14. トラブルシューティング

### 14.1 タグを書いたのに音が出ない

確認順序:

1. **Description が正しく書き換わっているか**: プリムを右クリック → Edit → Description タブで現在値を確認
2. **タグの綴り**: `[3dstream:` が含まれているか。旧配置では `[3dstream-stereo:` / `[ayastream-stereo:` / `[ayastream:` も互換受付されます (タイポ注意)
3. **音源宣言が有効か**: URL 音源は `{url:http://...}` または `{url:https://...}` が必要です。media 音源は root prim の `{source:media}` と、同じリンクセット内の media/MOAP 面が必要です
4. **`Stream3DEnabled` / `Stream3DDescriptionScan` が true**: Preferences > Sound または Debug Settings で確認
5. **ポーリング待ち**: LSL `llSetObjectDesc` 経由の変更は最大 30 秒待つ (= `Stream3DPollInterval`)
6. **チャットにエラー通知が出ていないか**: §13 のエラー文言を確認
7. **ログに `LL_INFOS("Stream3D")` の reconnect attempt が出ていないか**: ストリーム URL が落ちている可能性

### 14.2 ステレオの片方しか鳴らない

- ルートに `{url}` のみ書いて子に `{ch:R}` だけ書いた場合、L チャンネルを担当するプリムがいないので「L 片肺」状態になります。ルートに `{ch:L}` を併記するか、別のプリムに `{ch:L}` を割り当ててください
- 同じ `{ch:L}` を 2 つのプリムに書いて両方からダブって鳴らしたい場合は意図通りなので OK
- routing 診断 (`Stream3DRoutingDiagnostic`) を ON にすると、各 ch が何を再生しているかが chat に出ます

### 14.3 5.1ch ソースが開けない / 音がブツブツ切れる

- §11.4 の seek 制約: FLAC 6ch の配信経路が seek に対応していない場合に起きることがあります。**Vorbis 6ch / Opus 6ch に切り替える**、Opus 6ch なら SurroundStreamer を使う、または FLAC 6ch を seek 可能な経路で配信してください
- HTTP 切替直後の最初の 5〜10 秒は prebuffer 充填中に dropout 警告が出る場合があります (LAN 環境で 408〜2045 frames/spk/s ≒ 0.8〜4% 程度)。定常運用では消えます
- ストリームのビットレートが高すぎる / ネットワークが詰まっている場合の dropout: 配信側でビットレートを下げる (256kbps 以下推奨) / 同時 binding 数を減らす

### 14.4 子プリムにタグを書いてもスピーカーとして認識されない

- 子プリムの Description は Properties 通信で取得されます。**初回のリンクセット入域時に少し時間がかかる** ことがあります (数秒〜10 秒)
- LSL `llSetObjectDesc` で子プリムの Description を変更した場合は、次の poll cycle (= 30 秒以内) で反映されます
- `{ch:...}` の値が typo になっていないか (大文字小文字は問題なし、ただしスペル間違いは無効)

### 14.5 リスナー位置がおかしい (音の方向が変)

- カメラを大きく動かすとリスナー位置がカメラ移動に追従するため、定位が変化します。Avatar 視点で固定したい場合は Preferences > Sound の **「Hear media and sounds from:」を Avatar に切替** (`MediaSoundsEarLocation = 1`)
- Camera / Avatar の切替は 3D Stream にも効きます (パーセル BGM / LSL `llPlaySound` 等と共通設定)

### 14.6 同時に複数の 3D Stream を鳴らしたい

- `Stream3DMaxConcurrent` (既定 4) を超えると新しい binding は拒否されます。同時運用したい場合は値を増やしてください (8 / 16 程度まで実用)
- ただし 1 binding = 1 デコーダスレッド + N スピーカーチャンネルで CPU を消費します。20 並走等は CPU 負荷が大きいので、必要分だけ増やしてください

### 14.7 タグを消したのに音が止まらない

- Description の変更は通常、次のポーリングで反映されます (`Stream3DPollInterval` 既定 30 秒、§4.6)。
- 手動編集の場合は、Description を保存し直すと Properties 通知で再評価されます。
- それでも止まらない場合は `Stream3DEnabled` を一旦 `false` にして全 binding を強制解除し、必要なら再度 `true` に戻してください。

### 14.8 `{source:media}` で media 面が見つからない

- media が root tag と同じリンクセット内の face に設定されているか確認してください。
- media が子プリム上にある場合は、その子プリムの SL link number を `{link:N}` で指定してください。
- 選択したプリムに media face が複数ある場合は `{face:N}` も指定してください。
- リンク直後や object 読み込み直後は、root Description より media face 情報が少し遅れて届くことがあります。数秒待つか、object を touch / edit して再評価させてください。
- 同じ root tag に `{url:...}` と `{source:media}` を同時に書かないでください。

---

## 15. 既知の制約 / 仕様上の注意

### 15.1 リスナー位置はカメラまたはアバター

3D Stream の音場計算に使われるリスナー位置は、Preferences > Sound の **「Hear media and sounds from:」** 設定に従います。

- `Camera` (既定): カメラ位置 / カメラ向き
- `Avatar`: アバター位置 / アバター向き

これは LSL `llPlaySound` / パーセル BGM / Media-on-a-Prim とも共通の設定です。

### 15.2 1 リンクセット = 1 音源

1 つのリンクセット内に音源宣言を持つルートが「ある」/「ない」だけが意味を持ちます。root は `{url:...}` または `{source:media}` のどちらか 1 つだけを持てます。**複数の `{url}` を 1 リンクセットに書いたり、`{url}` と `{source:media}` を同時に書いたりすることはできません** (子プリムに書いた source selection キーは無視されます)。

複数の異なる URL ストリームや media source を 1 つの会場で鳴らしたい場合は、リンクセットを分けて配置してください (= `Stream3DMaxConcurrent` の枠内で複数 binding を持つ)。

### 15.3 Description 文字数 (127 byte)

LSL `llSetObjectDesc` が書き込める Description は **127 byte 上限**です。日本語を含む URL や説明文は UTF-8 で容易に超過します。

長くなる場合の対処:

- ルートに `{url}` または `{source:media}` だけ書いて子プリムに `{ch}` だけ書く分散方式 (この場合、各プリムの Description は短く保てます)
- URL を短縮 (URL shortener、または配信側のパス短縮)
- **キー名 / venue 値の短縮形を使う** (§4.5)。`binaural`/`venue`/`wetgain` は `bin`/`v`/`wg`、venue 値 9 種にも 1〜2 文字エイリアスがあり、長形式と完全等価です

### 15.4 codec 別の動作実績

| codec | 1ch / 2ch | 6ch |
|---|---|---|
| Vorbis (Ogg) | ✓ 実機検証済 | ✓ 実機検証済 (r9 P10、12 分連続 0 dropout) |
| Opus (Ogg) | ✓ 実機検証済 | ✓ 実機検証済 (r9-opus 補完、Opus channel mapping family 1) |
| FLAC | ✓ 実機検証済 | △ codec layout は実装済み。配信経路によって seek 制約あり |
| MP3 | ✓ 実機検証済 | — |

5.1ch 配信では **Vorbis 6ch** または **Opus 6ch** を選んでください。ライブ配信では Opus 6ch、検証用の静的ファイルでは Vorbis 6ch が扱いやすい形式です。

### 15.5 LFE の特殊扱いはなし

5.1ch の LFE (サブウーファー) は、viewer 側で「ローパスフィルタ」「2D 化」などの特殊処理は行われず、他の 5 チャンネルと同等に 3D 配置 + 距離減衰されます。低域フィルタリングが必要なら配信側 mix で済ませてください。

物理的なサブウーファー筐体を SL 内のプリムとして配置し、その位置から低域音を出すという運用が想定されています。

### 15.6 5.1ch の自由視点モデル

5.1ch ソースは、通常は基準となるリスニング位置を想定して mix されています。AYAstorm でも、映画館の座席のように会場内の特定位置を基準にしてスピーカーを配置できます。

一方で、SL ではリスナーが会場内を自由に移動します。基準位置から離れるほど、mix が想定した定位とは聴こえ方が変わります。そのため 3D Stream の 5.1ch 配置は、固定席での視聴にも使えますが、会場内に各チャンネルを配置して鳴らす多点 PA としての性格も持ちます。

### 15.7 他 Viewer での挙動

`[3dstream:...]` タグ、および互換プレフィクスの `[3dstream-stereo:...]` / `[ayastream:...]` / `[ayastream-stereo:...]` は **AYAstorm 専用** です。本家 Firestorm / 公式 LL Viewer / Catznip 等の他 Viewer は完全に無視します。

- AYAstorm 利用者には 3D 定位再生される
- 他 Viewer 利用者にはタグが説明文の一部として表示されるだけで、`{url:...}` の 3D Stream 音声は鳴らない (パーセル BGM とは独立に動作するため、パーセル BGM が設定されていればそれは聞こえる)
- `{source:media}` を使う構成では、他 Viewer でも media 面自体は通常の MOAP として表示・再生されます。ただし 3D Stream のスピーカー配置、`{ch:...}` routing、upmix、binaural、venue reverb は適用されません
- media 画面を見せながら `{url:...}` の 3D Stream を鳴らす構成では、他 Viewer では media 側だけが通常の MOAP として扱われ、`{url:...}` の 3D Stream 音声は鳴りません

### 15.8 同時最大数

| 上限 | 既定 |
|---|---|
| `Stream3DMaxConcurrent` (linkset 単位の binding 数) | 4 |
| `Stream3DStereoMaxSpeakers` (1 binding あたりのスピーカー数) | 16 |
| 結果として最大 同時スピーカー数 | 4 × 16 = 64 |

64 channel 程度までは FMOD の余裕があります。それ以上必要な場合は debug settings で値を上げてください (実機での CPU 負荷確認は必須)。

### 15.9 音量の合成

URL 音源の最終音量は次の通りです。

```
Stream3DVolumeMaster × {volume:N} × FMOD 距離減衰 × Master Audio Slider × 各種ミュート状態
```

通常は `Stream3DVolumeMaster` (Preferences の 3D Stream スライダー) で全体調整、`{volume:N}` でプリム個別の補正、距離減衰は `range` (スピーカー個別) または `Stream3DRolloffMax` (全体既定) で制御します。

media/MOAP 音源では、media 面が 1 つだけの構成に限り media volume / mute も source gain として効きます。詳細は §6.7 の音量ルールを参照してください。

---

## 16. 静的 occlusion `[ayastorm:occlude]` (r13)

**会場運営 / 建設者向け** のタグです。壁・扉・床・天井などの「音を遮るプリム」にこのタグを書くと、AYAstorm はそのプリムをリスナー位置と音源プリム位置の間にある **遮蔽物** として扱い、音をこもらせます。

`[3dstream:...]` (§5 / §6) が **音を出す側** のタグなのに対して、`[ayastorm:occlude]` は **音を遮る側** のタグです。両者は完全に独立で、occlude タグだけ書いたプリムからは音は鳴りません。

### 16.1 書式

```
[ayastorm:occlude]                            ← 既定値 (direct:0.7 reverb:0.5)
[ayastorm:occlude{direct:0.9}{reverb:0.7}]    ← 値指定
[ayastorm:occlude{direct:0.6}]                ← 一方だけ指定 (もう一方は既定)
```

旧プレフィクス (`[ayastream:occlude]`) は **受け付けません**。occlusion は r13 で新設の機能で、ayastream 系の遺産プリムが存在しないため。共通の書式ルール (§4.3、キー名は大文字小文字非区別 / 値は前後空白 trim / 未知キーは黙って無視) はそのまま適用されます。

### 16.2 動作モデル

#### 何が遮蔽されるか

- **`[3dstream:...]` から鳴る音** (3D 定位ストリーム、スピーカープリムごと)
- **`llPlaySound` / 添付音 / 子プリム効果音** (世界 SFX)

リスナー位置 (カメラまたはアバター) と音源位置を結ぶ線分が occlude プリムの **実形状 (三角形メッシュ)** を貫いていれば「遮蔽あり」と判定し、音量を下げ + 低域強調のローパスでこもらせます。Path Cut で開けた切れ目 / Hollow でくり抜いた中空 / mesh プリムの正確な形状 が全て遮蔽計算に反映されるため、「ドーナツの穴を通る音は素通し / 壁の本体に当たる音はこもる」という直感どおりに鳴ります。

複数のプリムを同時に貫いている場合は **掛け合わせ** で減衰します — 例: `direct=0.7` のプリム 2 枚を抜けると最終 direct は `1 - (1-0.7)² ≈ 0.91`、3 枚抜けるとさらに強い、という「壁が増えるほどこもる」直感どおりの挙動。

内部的には bounding OBB で粗く pre-cull (segment-vs-AABB で ~95% のミスマッチを reject) → 残った候補のみ三角形 raycast (Möller-Trumbore)、という 2 段判定で、CPU を低く保ちつつ実形状精度を確保しています。

#### 何が遮蔽されないか

- **2D ストリーム** (parcel music のような 3D 定位なし配信)
- **Voice (Vivox / WebRTC)**
- **UI 効果音 / プレビュー音** (内部で `isForcedPriority` フィルタで除外)

### 16.3 引数の意味

| キー | 既定 | 範囲 | 効果 |
|---|---|---|---|
| `direct` | `0.7` | `0.0`-`1.0` | 直接音 (= 音量) の減衰。`0.0` = 素通し、`1.0` = ほぼ無音 |
| `reverb` | `0.5` | `0.0`-`1.0` | 残響成分の減衰。`0.0` = 残響素通し、`1.0` = 残響カット |

`direct` が大きいほど「壁の向こうで鳴っている感」が強くなり、しかも viewer 内蔵の LOWPASS_SIMPLE で **低域偏重のこもった音** に加工されます (22 kHz → 300 Hz、`direct=1.0` で最大こもり)。`reverb` は音源側プリムが `{venue:...}` で会場残響を有効化している場合のみ意味があります (§7.2)。

### 16.4 推奨セット (材質イメージ → 値)

経験的に「こんな感じ」というセット。会場でライブ確認しながら微調整してください。

| 材質イメージ | `direct` | `reverb` | 印象 |
|---|---|---|---|
| 石壁 / コンクリート | `0.9` | `0.7` | ほぼ無音、低域だけ漏れる |
| 木壁 / 内装パネル | `0.7` | `0.5` | 既定値、典型的な「壁の向こう」 |
| 軽い木板 / カーテン | `0.6` | `0.4` | こもった音が漏れる、薄い間仕切り |
| ガラス窓 / 障子 | `0.3` | `0.2` | 軽くこもる、向こう側が認識できる |
| 装飾用 (実質透過) | `0.1` | `0.05` | ほぼ素通し、シルエットだけ存在 |

### 16.5 自動追従 (動的扉も OK)

`refreshOccluders` が **毎 tick** (= `LLPositionalStreamMgr::update()` ごと) 全 occluder プリムの位置 / 回転 / スケールを再取得します。つまり:

- **動く扉** (LSL `llSetPos` / `llSetRot` でアニメーション) に `[ayastorm:occlude]` を書くだけで、開閉に追従して遮蔽が変化します
- **乗り物 / 移動プリム** に書いた場合も同じく追従
- 専用の「扉タグ」は **不要** です (r13 spec 策定時は `[ayastorm:door]` を予定していましたが、refreshOccluders で十分なため永久に drop)

### 16.6 距離 cull (`Stream3DOccluderRange` = 64m)

リスナー-音源距離が `Stream3DOccluderRange` (既定 64m) を超える場合、その音源に対する OBB raycast は **skip** されます (距離減衰で既に十分小さくなっている前提)。大規模会場で 64m 超の遮蔽が必要な場合は debug settings から値を上げる、または `0` を入れて常時 raycast にできます (§12.2)。

### 16.7 master toggle (`Stream3DOcclusion`)

トラブルシュート / 動作切り分け用の **全体 ON/OFF スイッチ** (debug setting)。

| 値 | 動作 |
|---|---|
| `-1` (既定) | 有効。すべての `[ayastorm:occlude]` タグを評価 |
| `0` | 無効。タグは全無視、すでにこもっていた音は通常 ramp で素通しに戻る |
| `1` | 明示的に有効 (将来の per-mode override 予約) |

ライブ中に live-toggle しても **cliff (突然の音量変化) は発生しません** — disabled 時も smoothing 経路は走るため、`Stream3DOcclusionRampMs` (既定 250 ms) でなめらかに bypass まで戻ります。

### 16.8 可視化 (`Stream3DShowOccluders`、Alt+Shift+O)

登録済み occluder プリムを **シアン三角形メッシュ** (半透明 fill + wireframe) として表示する debug 機能。raycast で実際に使う三角形そのものを描画するため、Path Cut / Hollow / mesh プリムの形状が**そのままシアンに見えます**。会場構築中に「タグが認識されているか」「想定どおりの形状で遮蔽されるか」を目視確認できます。

- **メニュー**: View → Highlighting and Visibility → "Show 3D Stream Occluders (AYAstorm)"
- **ホットキー**: `Alt+Shift+O` (ライブ ON/OFF)

**ライブ追従**: build floater で **選択中の occluder プリム** は、Path Cut / Hollow / Sculpt のスライダーを動かしている最中もシアン形状が即座に追従します。編集ウィンドウを閉じる前にその場で遮蔽形状を確認できます (非選択の occluder は編集ウィンドウを閉じたタイミングで sim 経由で反映)。

**フォールバック表示**: 三角形抽出ができなかった occluder (= 三角形数が上限 2000 を超えた mesh など、§16.9 参照) はシアンが描画されません。「タグは付いているのにシアンが出ない」状態は OBB-only フォールバックの目印になります。

`Stream3DOcclusion` (master toggle、§16.7) を `0` にしても可視化はそのまま見られます — 「audio off で構造だけ確認したい」会場運営側のワークフローに合わせて、両 toggle は意図的に独立しています。

### 16.9 制限 / 上限

- **同時 occluder 数 256** (`kMaxOccluders` hardcoded)。`[ayastorm:occlude]` タグ付きプリムが sim 内に 257 個以上ある場合、257 個目以降は登録されません (`LL_WARNS` がログに出ます)。典型 SL venue (~100 プリム想定) では十分な余裕。
- **三角形数上限**: 1 occluder あたり **2000 三角形** まで (`kMaxTrisPerOccluder` hardcoded)。これを超える mesh プリムは三角形抽出を諦め、bounding OBB のみでの判定にフォールバックします (`LL_WARNS_ONCE` がログに出ます、§16.8 のシアン非表示でも判別可)。典型的な SL building prim (cube / cylinder / 中空 / Path Cut) は 数十-数百三角形、建築用途の mesh プリムでも通常範囲内。
- **CPU 負荷**: 64m 距離 cull + OBB pre-cull で大半の (segment, occluder) 組は数演算で reject されるため、典型的な SL venue (~100 occluder) では 1 ms/sec 未満。三角形 raycast まで届くのは listener-source 線が実際にプリムを貫いている数枚分のみ。
- **同梱 FMOD 制約**: 内部実装は viewer 側の OBB pre-cull + Möller-Trumbore 三角形 raycast です (FMOD::Geometry::createGeometry は同梱 libfmod 2.03.07 で動作しないため)。ユーザー視点では影響なし。

---

## 17. 関連ドキュメント / 内部仕様書

本ガイドは **利用者向け** の書式リファレンスです。実装内部の詳細 (decode thread / FMOD 経路 / リング bufferr / シャットダウン順序等) は以下の仕様書を参照してください。

| ドキュメント | 内容 |
|---|---|
| `docs/specs/spec_positional_stream_audio.md` | 3D Stream 本体仕様 (r5 改訂) — 基本アーキテクチャ |
| `docs/specs/3dstream-user-guide.ja.md` | 3D Stream 使い方ガイド — 配置手順、配信形式、SurroundStreamer、他 Viewer fallback |
| `docs/specs/spec_stream3d_decode_thread.md` | r7 で確立した 3-thread モデル |
| `docs/specs/spec_distributed_stereo.md` | r8 分散記述ステレオ仕様 — 旧 `[3dstream-stereo:...]` の field 書式 |
| `docs/specs/ayastorm-r31-3dstream-unified-tag.md` | r31 3D Stream unified tag 仕様 — `[3dstream:...]` への統一、`{ch}` による linkset routing 昇格、旧 prefix 互換 |
| `docs/specs/spec_5_1ch_source.md` | r9 5.1ch ソース受入仕様 — Opus/FLAC 6ch decode 経路 + BS.775 ダウンミックス |
| `docs/specs/spec_5_1ch_placement.md` | r10 5.1ch 会場配置仕様 — `ch=FL/FR/C/LFE/SL/SR` 拡張 + 互換マトリクス |
| `docs/specs/spec_binaural_venue_reverb.md` | r11 バイノーラル + 会場残響仕様 (r12 と同梱配布)。lite-HRTF / 9 venue / wetgain 詳細 |
| `docs/specs/spec_stereo_upmix.md` | r12 stereo→5.1 upmix 仕様 — matrix upmix + 帯域分離アルゴリズム詳細 |
| `docs/ayastorm-r12-stereo-upmix.md` | r12 phase 分解 (P0-P11) と工数見積 |
| `docs/ayastorm-r13-occlusion.md` | r13 OBB occlusion 仕様 + 実装記録 — `[ayastorm:occlude]` 設計判断 / spike 実装 / 残工程 |
| `docs/ayastorm-stream3d-roadmap.md` | 3D Stream 全体ロードマップ (r5〜r13+) |

---

## 改訂履歴

- **2026-05-05 (初版)**: r10 時点の最終仕様として整備。r5 / r8 / r9 / r10 / r10.x の累積仕様をまとめて記述。r11 以降は未リリースのため対象外。
- **2026-05-08 (r12 改訂)**: r11 (バイノーラル / 会場残響 / wetgain) と r12 (stereo→5.1 upmix / タグ短縮形 `bin`/`v`/`wg` + venue 値短縮) を追記。r11 は独立リリースせず r12 に同梱配布する方針のため、ユーザー向けには r10 → r12 の 1 ジャンプとなる。§7 / §8 / §4.5 を新設、章番号 §7-§14 を §9-§16 に繰り下げ。
- **2026-05-09 (r12.1 改訂)**: `{lfegain:N}` キー (短縮形 `lg`) を §7.4 として新設、旧 §7.4 配信者主導モデルを §7.5、旧 §7.5 組合せ例を §7.6 に繰り下げ。`wetgain` の既定値を `1.0` → `0.2` に変更 (推奨範囲 0.1〜0.5 を反映)。§12.2 に `Stream3DLfeGain` sentinel 追加。§12.2 / §12.3 にライブチューニング修正の注記を追加 (r12 で `Stream3DUpmix*` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` がプリムタッチまで反映されなかった回帰を修正)。
- **2026-05-11 (r13 改訂)**: 静的 OBB occlusion タグ `[ayastorm:occlude]` を §16 として新設、旧 §16 関連ドキュメントを §17 に繰り下げ。§4.1 を「2 種類のタグ」→「3 種類のタグ」に拡張。debug settings (`Stream3DOcclusion` master sentinel / `Stream3DOccluderRange` 距離 cull / `Stream3DOcclusionRampMs` smoothing / `Stream3DShowOccluders` 可視化) を §16.6-§16.8 に記述。関連 spec として `docs/ayastorm-r13-occlusion.md` を §17 表に追加。
- **2026-05-11 (r13 P15 改訂)**: occlusion 判定を OBB 近似から **実プリム三角形 raycast** に拡張 (Möller-Trumbore + OBB pre-cull の 2 段判定)。Path Cut / Hollow / Mesh の実形状が遮蔽計算に反映される (§16.2)。複数プリム集計を **掛け合わせ** に修正記述 (実装は当初から掛け合わせだったが旧版で「max」と誤記)。`Stream3DShowOccluders` 表示を OBB ワイヤーフレームから **シアン三角形メッシュ** (半透明 fill + wireframe) に変更、build floater で選択中のプリムは編集中ライブ追従 (§16.8)。§16.9 に三角形数上限 2000 と OBB-only フォールバック条件を追記。§16 タイトルを「静的 OBB occlusion」→「静的 occlusion」に短縮。
- **2026-05-17 (r26 改訂)**: media/MOAP source routing を §6.7 として追記。root の音源宣言を `{url:...}` / `{source:media}` の相互排他に拡張し、media 面選択用の root-only キー `{link:N}` / `{face:N}`、media 音量の扱い、URL 音源 + media 表示の同居条件、5.1ch までの media callback ch 数を整理。
- **2026-05-24 (r31 改訂)**: 新規推奨タグを `[3dstream:...]` に統一。`[3dstream:{url:...}]` は `{ch}` が無いリンクセットでは単一プリム URL 再生、同じリンクセットに `{ch}` がある場合はリンクセット配置の音源宣言として扱う仕様を追記。`[3dstream:{source:media...}]` は `{ch}` スピーカーと組み合わせる linkset routing 専用であること、子プリムの有無ではなく `{ch}` の有無で判定することを明記。旧 `[3dstream-stereo:...]` / `[ayastream-stereo:...]` は互換 prefix として整理。
- **2026-05-24 (r31 ドキュメント改訂)**: `binaural` 未指定時の既定を `off` に変更し、§7 の説明と Debug Settings 表を更新。§8 の upmix 説明から商標名を除き、実装に合わせて matrix upmix + 帯域分離として整理。§11 / §15.4 の codec 実績を r9-opus 補完後の状態に更新し、Opus 6ch は実機検証済み、FLAC 6ch は配信経路によって seek 制約ありと記述。Opus 6ch 配信の実用経路として SurroundStreamer を追記。§15.6 の 5.1ch 自由視点モデル、§15.7 の他 Viewer / MOAP fallback、§14 のトラブルシュート文言を見直し。
