# 3D Stream タグ書式ガイド

> AYAstorm の **3D Stream** 機能で、プリムから HTTP オーディオストリームを 3D 空間定位再生するためのタグ書式リファレンスです。
>
> このドキュメントは AYAstorm `r12` 時点の最終仕様に基づきます。r12 で追加された機能 (バイノーラル / 会場残響 / stereo→5.1 upmix / タグ短縮形) も含まれます。

---

## 目次

1. [3D Stream とは](#1-3d-stream-とは)
2. [クイックスタート](#2-クイックスタート)
3. [用語](#3-用語)
4. [タグの全体像](#4-タグの全体像)
5. [モノラルタグ `[3dstream:...]`](#5-モノラルタグ-3dstream)
6. [分散ステレオ / 会場配置タグ `[3dstream-stereo:...]`](#6-分散ステレオ--会場配置タグ-3dstream-stereo)
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
   [3dstream-stereo:{url:http://example.com/stream.mp3}{range:30}]
   ```
3. **Root の Description に追記** (= Root 自身も L スピーカー):
   ```
   [3dstream-stereo:{url:http://example.com/stream.mp3}{range:30}{ch:L}]
   ```
4. **Child の Description**:
   ```
   [3dstream-stereo:{ch:R}]
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
| **音源宣言** | `{url:...}` を含むタグを書いたプリム。本書での「どのストリームを鳴らすか」を宣言する役割。**ルートプリムにのみ書ける** (子プリムに書いても無視されます) |
| **スピーカープリム** | `{ch:...}` を含むタグを書いたプリム。実際に音を鳴らすプリム。**ルート/子プリムどちらでも可** |
| **binding** | 1 つのリンクセットに対して内部で組み立てられる「音源 → スピーカー群」の対応関係。1 リンクセット = 1 binding |
| **ch (チャンネル)** | スピーカープリムが受け持つ音声チャンネル。`L` / `R` / `M` (モノラル) のほか、5.1ch 用の `FL` / `FR` / `C` / `LFE` / `SL` / `SR` |
| **rolloff** | 距離減衰。リスナーがスピーカーから離れるにつれ音量が減衰する設定 |

---

## 4. タグの全体像

### 4.1 3 種類のタグ

| タグ | 接頭辞 | 用途 |
|---|---|---|
| **モノラルタグ** | `[3dstream:...]` | 単一プリムから 1 ストリームを再生 (最小構成) |
| **分散ステレオ / 会場配置タグ** | `[3dstream-stereo:...]` | リンクセットの複数プリムから 1 ストリームを同期再生 (ステレオ / マルチスピーカー / 5.1ch) |
| **静的 occlusion タグ** (r13 新設) | `[ayastorm:occlude]` | 壁・扉・床・天井プリムを「音を遮るもの」として扱う (会場運営 / 建設者向け、§16) |

### 4.2 旧プレフィクスのエイリアス

両タグとも、旧プレフィクス (`[ayastream:...]` / `[ayastream-stereo:...]`) を **恒久エイリアス** として受け付けます。r5 (2026-05) で `ayastream` → `3dstream` にリネームした際、それ以前に配置されたプリムを再編集せずに済むよう温存しています。新規記述は **`3dstream` 系を推奨**しますが、混在しても問題ありません。

```
[3dstream:{url:...}]              ← 推奨 (canonical)
[ayastream:{url:...}]             ← 旧式、互換受付

[3dstream-stereo:{url:...}{ch:L}] ← 推奨 (canonical)
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

`[3dstream-stereo:...]` で使う **キー名のうち頻出 4 つ** と **`venue` の値 9 種** には r12 から **短縮エイリアス**を用意しています。SL の Description 127 byte 上限 (§4.4) に収めやすくするためのもので、長形式と短縮形は **完全等価** です (内部で同じ正規形に解決)。新規記述・既存記述どちらの形式で書いても動作は同じです。

#### キー名の短縮 (4 件)

| 長形式 (canonical) | 短縮形 | 機能 (詳細) |
|---|---|---|
| `binaural` | `bin` | バイノーラル ON/OFF (§7.1) |
| `venue` | `v` | 会場残響プリセット (§7.2) |
| `wetgain` | `wg` | 残響ウェット成分の強さ (§7.3) |
| `lfegain` | `lg` | LFE チャンネルゲイン倍率 (§7.4、r12.1 追加) |

その他のキー (`url` / `ch` / `range` / `volume` / `min` / `max` / `upmix`) には短縮形はありません (元々短い、または使用頻度低)。

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
[3dstream-stereo:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{ch:C}{binaural:on}{venue:hall_medium}{wetgain:1.5}{upmix:on}]
```
(133 byte — 127 上限を 6 byte 超過)

短縮形 (**127 byte 以内、書ける**):

```
[3dstream-stereo:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{ch:C}{bin:on}{v:hm}{wg:1.5}{upmix:on}]
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

## 5. モノラルタグ `[3dstream:...]`

### 5.1 書式

```
[3dstream:{url:URL}{min:N}{max:N}]
```

または旧プレフィクス:

```
[ayastream:{url:URL}{min:N}{max:N}]
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
- ステレオ音源を渡した場合は **内部で L/R をミックスしてモノラル化** されます。
- 同じリンクセット内に `[3dstream-stereo:...]` も同時に書かれている場合、`[3dstream:...]` 側がそのプリムのスピーカー指定として優先されることはありません — 両方の binding 経路は独立に評価されます。同一プリムを両用途に使うのは推奨しません (動作未定義)。

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

## 6. 分散ステレオ / 会場配置タグ `[3dstream-stereo:...]`

### 6.1 書式

```
[3dstream-stereo:{url:URL}{range:N}{ch:CH}{volume:V}]
```

または旧プレフィクス:

```
[ayastream-stereo:...]
```

このタグは **リンクセット全体で 1 つのストリーム** を扱う形式です。ルートプリムが「どのストリームを鳴らすか」を宣言し、リンクセット内の各プリムが「自分はどのチャンネルを担当するか」を宣言します。

### 6.2 プリムの役割

各プリムはタグの中身によって以下の役割を持ちます:

| Description のフィールド | 役割 |
|---|---|
| `{url:...}` を含む | **音源宣言** (ルートプリム限定。子プリムで `{url:...}` を書くと無視されます) |
| `{ch:...}` を含む | **スピーカー** (ルート/子プリムどちらでも可) |
| 両方を含む (= ルートのみ) | 音源宣言 + 自身もスピーカーを兼ねる |
| どちらも含まない | 何もしない (binding 対象外) |

リンクセット内に **音源宣言 (= `{url}` を持つルート)** と **少なくとも 1 個のスピーカー (= `{ch}` を持つプリム)** が両方あって初めて再生開始されます。スピーカー 0 個では「構造エラー」となり、エラー通知が出ます (§13)。

### 6.3 キー一覧

#### 6.3.1 ルートプリムでのみ意味があるキー

| キー | 必須 | 型 | 既定値 | 意味 |
|---|---|---|---|---|
| `url` | **必須** | 文字列 | — | ストリーム URL。空文字列はエラー |
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

> **重要**: モノラルタグの `min` / `max` キーは分散ステレオタグ側では **無視** されます。分散ステレオでは近距離は内部固定で 1.0m、遠距離は `range` キー (または既定値 `Stream3DRolloffMax`) が使われます。

### 6.4 ルート 1 つ + 子 1 つ (基本のステレオペア)

最小のステレオ配置:

```
ルート Description:
  [3dstream-stereo:{url:http://example.com/stream.mp3}{ch:L}]

子 Description:
  [3dstream-stereo:{ch:R}]
```

ルート自身が L、子が R を担当します。ルートと子の **位置関係 (= リンク番号)** は再生に影響しません。空間内のどこに置くかが定位を決めます。

### 6.5 マルチスピーカー (4 個以上の配置)

同じステレオストリームを 4 つのスピーカーから鳴らす例 (会場の 4 隅):

```
ルート Description:
  [3dstream-stereo:{url:http://example.com/stream.mp3}{range:30}]

子 #1 Description:
  [3dstream-stereo:{ch:L}{range:50}]

子 #2 Description:
  [3dstream-stereo:{ch:R}{range:50}]

子 #3 Description:
  [3dstream-stereo:{ch:L}{volume:0.7}]

子 #4 Description:
  [3dstream-stereo:{ch:R}{volume:0.7}]
```

- ルートは音源宣言だけで、自分はスピーカーとして鳴りません (`{ch}` なし)
- 子 #1, #2 は L/R それぞれ近距離 50m、音量 100%
- 子 #3, #4 は同じ L/R を 70% で鳴らす (前段の補助)
- スピーカー数の上限は `Stream3DStereoMaxSpeakers` 設定で **既定 16 個まで** (§12)

### 6.6 5.1ch 会場配置 (6 プリム)

5.1ch ソース (Opus surround / FLAC 6ch) を 6 個のスピーカーに展開:

```
ルート Description:
  [3dstream-stereo:{url:http://example.com/test_5_1.flac}{range:30}]

FL プリム:  [3dstream-stereo:{ch:FL}]
FR プリム:  [3dstream-stereo:{ch:FR}]
C プリム:   [3dstream-stereo:{ch:C}]
LFE プリム: [3dstream-stereo:{ch:LFE}]
SL プリム:  [3dstream-stereo:{ch:SL}]
SR プリム:  [3dstream-stereo:{ch:SR}]
```

- 各プリムを物理的に「会場のスピーカー位置」に配置します (ステージ前 L/R、センター、サブウーファー、サラウンド L/R)
- LFE は他の 5 個と同等に扱われます (ローパスフィルタ等の特殊処理は viewer 側に入っていません。低域フィルタリングが必要なら配信側 mix で済ませてください)
- リスナーは「映画のスイートスポット」を持ちません (= SL の自由視点モデル)。会場を歩き回ると 5.1 mix の意図した定位は当然崩れます。シネマ的サラウンド体験ではなく、会場 PA 的な多点配置として運用してください

### 6.7 同じ ch を複数のプリムに割り当てる

`{ch:L}` を 2 つ以上のプリムに書くと、両プリムから同じ L チャンネルが鳴ります。配信会場で「ステージ前列の L」と「ステージ後列の L」のように複数台のスピーカーを置く用途に使えます。

逆にどの ch も書かれていないと「スピーカー 0 個」エラーになります。

### 6.8 ルート自身もスピーカー兼用

```
ルート Description:
  [3dstream-stereo:{url:http://example.com/stream.mp3}{ch:M}{range:25}]
```

このように 1 タグに `{url}` と `{ch}` を併記すると、ルートが音源宣言 + 自身も M (モノラル) スピーカーとして機能します。子プリムが 1 つもないシンプルな mono 構成にも使えます (`[3dstream:...]` と機能的にはほぼ等価)。

### 6.9 ルートプリムの判別方法

リンクセットを編集中、Build フローターの **Object** タブで「Selected」が表示されているプリムが選択中、その linkset の親 (= ルート) は通常 **最初に選択した状態でリンクされたプリム** です。

確認するもっとも確実な方法:
- Build → Edit → 「Edit linked」を OFF にしてプリムをクリック → そのリンクセットのルートが選択される
- LSL: `llGetLinkNumber()` でルートは `1` (子プリムが存在する場合)。子プリムなしの単一プリムは `0`

ルートと子の位置関係 (リンク番号 1, 2, 3, ...) は **3D Stream の再生に影響しません**。リンク番号で L/R を決めていた仕様は r5 までで廃止され、r8 以降は `{ch:...}` タグ宣言ベースになっています。

---

## 7. バイノーラル / 会場残響 (r12)

r10 までの 3D Stream は「dry な素材を多点配置で空間に置く」だけで、ヘッドホンで聴いたときの左右定位の精細さや「会場に居る感」はリスナー側の想像力に頼っていました。r12 ではこの 2 点を viewer 内 DSP で補強する 3 つのタグキーを追加します。

| キー | 短縮形 | 既定 | 機能 |
|---|---|---|---|
| `binaural` | `bin` | `on` | lite-HRTF (ITD + air absorption) によるヘッドホン定位強化 |
| `venue` | `v` | `dry` | 9 種の会場残響プリセット (convolution reverb) |
| `wetgain` | `wg` | `0.2` | 残響ウェット成分の倍率 (0.0〜2.0、音楽的レンジ 0.1〜0.5) |
| `lfegain` | `lg` | `1.0` | LFE チャンネルのゲイン倍率 (0.0〜4.0、r12.1 追加) |

**いずれも root プリムにのみ書きます** (子プリムに書いても無視)。listener 側 UI には現れません ─ **配信者がタグで決めたものをそのまま聴く** モデルです (§7.5)。

### 7.1 `{binaural:on|off}` (短縮形 `bin`)

リスナー (ヘッドホン) の左右定位を強化する **lite-HRTF DSP** の有効化フラグです。

#### 動作

`on` のとき、各スピーカーチャンネルに対して以下の処理が掛かります:

- **ITD (interaural time delay)** ─ 左右の耳に届く時間差をスピーカー方位から計算 (Woodworth-Schlosberg 近似) し、sample-fractional delay として付与。「左の耳だけ大きい」感ではなく「左から音が来る」感に近づきます
- **air absorption (距離 HF rolloff)** ─ 距離が遠いほど高域が減衰 (`-0.5 dB/m`、上限 `-25 dB`)。50m 離れたスピーカーが暗く聴こえます

ILD (左右レベル差) は r10 までと同じく FMOD の `FMOD_3D_LINEARSQUAREROLLOFF` が担当します。

#### `off` を選ぶ場面

- **既にバイノーラル encoded された配信源** を流すとき (二重処理を避ける)
- **スピーカー視聴の listener が多い** ことが分かっているとき (ITD はヘッドホン前提の処理、スピーカーでは効果が薄く副作用のリスクがある)
- **r10 までの動作** に厳密に揃えたいとき

#### 既定が `on` の理由

既存配置 (r8/r10 で置かれた全リンクセット) を **タグ無改修で** 改善できるようにするためです。新規記述は明示的に `{bin:on}` を書くことを推奨しますが、書かなくても on 動作になります。

### 7.2 `{venue:NAME}` (短縮形 `v`)

会場残響のプリセット 9 種から 1 つを選びます。**配信側で素材を dry に保ち、viewer 側で「ホール着替え」する** 運用が基本です。

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

配信者の **明示的 opt-in** を求めるためです。`dry` のときは reverb DSP 自体が挿入されず CPU 負荷ゼロ ─ r10 まで通り素通り再生になります。

#### CPU 負荷の注意 (重い venue)

`hall_medium` 以上の 4 venue (`hm` / `hl` / `cl` / `ct`) は IR (impulse response) が長く partitioned FFT convolution の負荷が増えます。`cathedral` は r10 比 **+10.2pp** ─ 1 コアで 53% 程度を消費します。modern 多コア機では問題になりにくい (全体 CPU 換算 3〜7%) ですが、低スペック機向けの会場では `room_small` / `room_medium` / `outdoor` が無難です。

「重い venue を **削るのではなく出荷する**」のが r12 の方針です ─ 配信者が cathedral タグを付ける = 「重い長残響が欲しい」と明示 opt-in する選択であり、コストに見合う効果を返すべき、という判断です。

### 7.3 `{wetgain:N}` (短縮形 `wg`)

venue 残響の **ウェット成分** (= reverb 出力) の倍率です。dry signal はそのまま素通しされ、wet を `N` 倍してから dry に加算します。

| 値 | 効果 |
|---|---|
| `0.0` | 完全 dry (= venue=dry と同等。ただし DSP は挿入されたまま) |
| `0.1` | wet ごく薄め |
| **`0.2` (既定)** | wet 控えめ (音楽的に違和感の出ない標準) |
| `0.3〜0.5` | wet 中庸〜やや濃いめ (musical range の上限目安) |
| `1.0` 以上 | wet が dry と同等以上の濃度 (実用上はリバーブが過剰になりやすい) |
| `2.0` | wet 2 倍 (上限) |

#### 設計上のポイント

各 venue の IR は **unity-gain 正規化** されているため、`{wg:0.2}` は venue を切り替えても **「wet と dry の比」が一定** に保たれます。`room_small` も `cathedral` も `wg:0.2` で同じ「dry/wet バランス」になります (cathedral だけ wet が爆音にならない)。

> **既定値の根拠 (r12.1 で 1.0 → 0.2 に変更)**: 実装当初の `1.0` は「wet と dry が同レベル」を意味するため、ホール / カテドラルでは原音が飽和してしまい、配信用途として実用域から外れていました。実 listening で **音楽的に使えるレンジは 0.1〜0.5** であることが確認されたため、r12.1 で既定値を `0.2` に下げています。LSL UI のクイック選択も `0.1`〜`0.5` の細かい刻みに揃えています。

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

debug settings `Stream3DLfeGain` (sentinel `-1.0` = タグ通り、それ以外 `0.0〜4.0` で強制上書き) も r12.1 で追加しています (詳細 §12.2)。配信者主導モデルの例外救済枠で、一般 listener UI には載せていません。

### 7.5 配信者主導モデル

これら 4 キー (`binaural` / `venue` / `wetgain` / `lfegain`) は **root prim Description が真実 (root truth)** ─ 一般 listener の Preferences / Debug Settings には対応 UI がありません。

#### なぜ listener UI を提供しないのか

- 配信者が「この会場はホール、binaural ON」と決めた表現を、listener が勝手に変えて聴くと「同じ配信を聴いているのに人によって聴こえ方が違う」という曖昧性が増えます
- AYAstorm の方針 (`r5 命名整理` / `r11 配信者主導モデル`) は **「表現の不確定性を増やさない」** で、tuning 軸を増やすほど運用が崩れる、という判断です

#### 例外: スピーカー視聴で binaural を切りたい個人

ヘッドホンではなくスピーカーで視聴している listener が、`{binaural:on}` 配信を聴くと ITD が逆効果になる場合があります。この **救済目的** に限り Debug Settings に sentinel 1 件 ─ `Stream3DBinauralRender = 0` で listener 側強制 OFF できます (詳細 §12.2)。同様に `Stream3DVenueOverride` (空文字 = タグ通り、`"dry"` で全 reverb 強制 OFF) / `Stream3DVenueWetGain` (sentinel `-1.0` = タグ通り) / `Stream3DLfeGain` (sentinel `-1.0` = タグ通り、r12.1) も用意されていますが、いずれも一般利用者向け UI には載せていません。

### 7.6 組合せ例

#### 何も書かない (= 既定)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}]
```
→ `{bin:on}{v:d}{wg:1.0}` 相当。lite-HRTF が掛かるが残響は無し (r10 + 定位強化)。

#### ライブハウス (PA 想定、打ち込み系)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:cl}{wg:1.0}]
```
→ club preset、密な反射、dry/wet 同レベル。

#### 大ホール (オーケストラ)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:hl}{wg:0.8}]
```
→ hall_large、wet を 0.8 倍に控えめ (ホール残響が長いので素材成分を残す)。

#### カテドラル (アンビエント / 環境音)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:ct}{wg:0.6}]
```
→ cathedral、wet 0.6 倍 (RT60 ~3s と長いので濃すぎないように)。

#### 野外 (環境音 / 散歩 BGM)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:od}{wg:1.0}]
```
→ outdoor、軽い early reflection のみ、空気感を出す。

#### 既にバイノーラル済の素材 (二重処理回避)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:off}{v:d}]
```
→ binaural OFF、reverb なし (= r10 までの動作)。

---

## 8. stereo→5.1 upmix (r12)

r10 の per-channel 配置 (FL/FR/C/LFE/SL/SR の 6 spk) は **5.1 配信** 専用の機能でした。しかし SL 配信ソフト (butt / Mixxx / OBS / SAM 等) の主流は stereo 止まりで、6 spk 配置を体感できる配信者は稀です。

r12 で追加された `{upmix:on}` キーは viewer 内 DSP として **stereo 2ch を 6ch に展開** し、r10 で築いた 6 spk placement の体験を **stereo 配信にも届け** ます。配信側の機材や方法は一切変更不要で、配信者がタグを 1 つ書き加えるだけで効きます。

### 8.1 `{upmix:on|off}` (短縮形なし)

| 値 | 効果 |
|---|---|
| **`off` (既定)** | upmix 無効。stereo source は r10 までと同じく `{ch:L}` `{ch:R}` `{ch:M}` 経路に流れる |
| `on` | stereo source を 6ch 化し、`{ch:FL}` `{ch:FR}` `{ch:C}` `{ch:LFE}` `{ch:SL}` `{ch:SR}` プリムに流す |

#### 既定が `off` の理由

`venue` と同じく **配信者の明示 opt-in** を求めるためです。upmix DSP は CPU を消費し、また音像が変化するため、配信者の表現意図として明示的に有効化する設計です。

#### 既存配置への効果

r8/r10 で過去に置かれた **すべての 6 spk リンクセット** は、配信者が `{upmix:on}` を 1 文字加えるだけで 6 spk placement の体験を得ます (再配置不要)。逆に `{upmix:off}` (or 未指定) の状態では従来通り。

### 8.2 アルゴリズム (DPL2 系 matrix decode + 帯域分離)

upmix は **DPL2 (Dolby Pro Logic II) 系のマトリックスデコード** に **3 つの帯域分離処理** を組み合わせた決め打ちアルゴリズムです。配信者は `on/off` だけ判断すれば良く、アルゴリズム選択タグはありません (= r11 の流儀踏襲)。

| 出力チャンネル | 派生方法 (概要) |
|---|---|
| `C` (center) | `(L+R)/√2` (in-phase 成分) |
| `Ls` / `Rs` (rear) | `(L-R)/√2` を decorrelate (固定 delay 16ms ± jitter で L/R 分離) |
| `LFE` | `(L+R)` を 80Hz LPF (THX 推奨) |
| `FL` / `FR` (front) | `L` / `R` から center 成分を `bleed_amount` で除去 (default フル除去 = phantom center を center spk に集約) |

**3 つの帯域分離** が DPL1 純粋 matrix decode との差です:

- **LFE LPF**: 低域だけを LFE spk に分配、front L/R には低域が残らない
- **Center bleed 除去**: phantom center の二重像 (center spk + front L/R 両方から鳴る現象) を防止
- **Rear decorrelation**: surround の左右をわずかな時間差で分離、空間広がりを生成

ML / AI 系 upmix は採用しません ─ 配信者・listener どちらにも「結果が予測できる音」を保証するための判断です。

### 8.3 5.1 native 配信での自動 bypass

source の channel 数が **6 以上** (= 5.1 native 配信、Vorbis 6ch / Opus surround / FLAC 6ch) のときは、`{upmix:on}` が指定されていても **upmix を自動的に bypass** します。二重処理防止のためです。

このとき Local Chat に **1 回だけ** 通知が出ます:

```
3D Stream: 5.1 native source detected (6ch), upmix:on tag auto-bypassed
```

配信を 5.1 ↔ stereo で切り替える運用 (例: 「ライブ本番は 5.1ch、休憩中は通常のステレオ BGM」) では、`{upmix:on}` を付けたままにしておけば自動で正しく動作します。

### 8.4 微調整 (debug settings 3 件)

upmix DSP 内部のパラメータは **配信者タグには出さず、listener 側 debug settings** に出します。配信者主導モデル維持のため、配信者が選ぶのは on/off のみで、内部パラメータは「アルゴリズムの一部」として固定します。listener 側も平時は触りません ─ 実装/検証時や個人的微調整用です。

| Debug 設定キー | 既定 | 範囲 | 意味 |
|---|---|---|---|
| `Stream3DUpmixLfeCutoff` | `80.0` Hz | 20〜200 | LFE LPF の cutoff 周波数 |
| `Stream3DUpmixCenterBleed` | `1.0` | 0.0〜1.0 | front L/R から center 成分を引く割合 (`0` で DPL1 互換、`1` でフル除去) |
| `Stream3DUpmixRearDelayMs` | `16.0` ms | 0〜32 | rear decorrelation の base delay (L/R は ±2ms jitter で分離) |

別途 listener 側の強制 OFF / 強制 ON 用に sentinel 1 件:

| Debug 設定キー | 既定 | 意味 |
|---|---|---|
| `Stream3DUpmix` | `-1` (sentinel = タグ通り) | `0` でタグ無視・強制 OFF / `1` で強制 ON (※ 5.1 native の auto bypass は常に効く) |

### 8.5 組合せ例

#### 既定動作 (upmix なし)

```
[3dstream-stereo:{url:http://example/stereo.ogg}{ch:L}]
[3dstream-stereo:{ch:R}]
```
→ r10 までと同じ動作 (stereo を L/R の 2 spk に流す)。

#### 6 spk 配置 + upmix (r12 推奨フォーマット)

```
ルート Description:
  [3dstream-stereo:{url:http://example/stereo.ogg}{upmix:on}]
FL:  [3dstream-stereo:{ch:FL}]
FR:  [3dstream-stereo:{ch:FR}]
C:   [3dstream-stereo:{ch:C}]
LFE: [3dstream-stereo:{ch:LFE}]
SL:  [3dstream-stereo:{ch:SL}]
SR:  [3dstream-stereo:{ch:SR}]
```
→ stereo source が 6ch に展開され、各 spk に役割が割り当てられる。

#### upmix + binaural + venue 全部入り (r12 フル機能)

```
[3dstream-stereo:{url:http://example/stereo.ogg}{upmix:on}{bin:on}{v:hm}{wg:1.2}]
```
→ stereo source を 6ch 化、各 spk に lite-HRTF と hall_medium reverb (wet 1.2 倍) を適用。会場感とヘッドホン定位を最大限に活かす設定。

#### r10 旧配置 (`ch:L`/`ch:R` のみ) + upmix

```
ルート Description:
  [3dstream-stereo:{url:http://example/stereo.ogg}{upmix:on}{ch:L}]
子: [3dstream-stereo:{ch:R}]
```
→ `ch:L`/`ch:R` も内部的には FL/FR にマップされるため、center bleed 除去 / rear decorrelation の **一部恩恵** を受ける (ただし full surround 体感には 6 spk 配置が必要)。

#### 5.1 native 配信に upmix を付けてしまった場合

```
[3dstream-stereo:{url:http://example/stream_5_1.ogg}{upmix:on}]
```
→ 6ch source 検出 → upmix auto bypass、Local Chat に通知 1 回。誤って `{upmix:on}` を付けたまま 5.1 配信に切り替えても安全 (二重処理されない)。

---

## 9. ch (チャンネル) 値リファレンス

`{ch:値}` には以下の 9 種類が指定できます。**大文字小文字は区別しません** (`{ch:l}` も `{ch:L}` も同じ)。

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

実際にスピーカープリムから何が鳴るかは、**ソース URL のチャンネル数** と **書いた `ch` 値** の組み合わせで決まります。

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
| **Opus (Ogg)** | ✓ | ✓ | △ | 6ch は Opus channel mapping family 1。**単純 HTTP / Icecast push は seek 失敗** で開けない場合あり (§11.4) |
| **FLAC** | ✓ | ✓ | △ | 6ch は理論上対応、Opus と同じ seek 制約あり |
| AAC (ADTS / HLS) | — | — | — | 非対応 |
| AC-3 / E-AC-3 | — | — | — | Dolby ライセンス問題で非対応 |

ソース URL は `http://` / `https://` のいずれも受け付けます。HTTP/1.1 keep-alive を維持する経路 (= SHOUTcast 互換 streamer や ffmpeg の TCP 出力) のほうが、単純な静的 HTTP より安定する傾向があります。

### 11.2 1ch / 2ch の配信

ふつうの SHOUTcast / Icecast / 静的 HTTP で OK です。MP3 / Vorbis / Opus / FLAC のいずれでも問題なく動作します。`oggenc` や ffmpeg / butt 等の通常の配信ツールがそのまま使えます。

### 11.3 5.1ch (Vorbis 6ch) の配信

5.1ch を viewer 側で確実に動かす経路として **Vorbis 6ch** が推奨されます (r9 P10 で実機検証済み)。

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

### 11.4 Opus 6ch / FLAC 6ch の制約

Opus 6ch (channel mapping family 1) や FLAC 6ch を **単純な HTTP** (例 `python3 -m http.server`) や **Icecast push** で配信すると、FMOD の parser が **seek 要求** をかけるため `FMOD_ERR_FILE_COULDNOTSEEK` で開けないケースがあります。

回避策:

- **SHOUTcast 互換 streamer** を使う (keep-alive + range 対応)
- **ffmpeg primary** を経由する (TCP backpressure で解決)
- **5.1ch GUI 配信ツール `butt-aya`** (AYA さん別プロジェクトで開発中、本書執筆時点で未公開) で push する

確実に動かしたい場合は **Vorbis 6ch** を選ぶのが現状の最短経路です。

### 11.5 配信側ツールの選び方

| ツール | 用途 | 注意 |
|---|---|---|
| **ffmpeg** | 任意 codec / 任意 ch / 静的 / リアルタイム | CLI 操作が必要、最も柔軟 |
| **butt** (公式) | DJ 配信 | 1ch / 2ch のみ、5.1ch 非対応 |
| **butt-aya** (5.1ch fork) | 5.1ch GUI 配信 | AYA さん別プロジェクト、本書執筆時点で未公開 |
| **Liquidsoap** | 高度な放送オートメーション | 設定の難易度高、上級者向け |
| **Mixxx / DarkIce / ezstream** | DJ / 自動化 | stereo 前提、5.1ch 非対応 |

---

## 12. viewer 側の設定

### 12.1 Preferences 経由

**環境設定 → Sound** タブに以下のコントロールがあります:

- **3D Stream** スライダー — 全ストリームの音量倍率 (`Stream3DVolumeMaster`)
- **Enabled** チェックボックス — 機能全体の ON/OFF (`Stream3DEnabled`)
- **Show channel routing diagnostics in chat** — routing 診断通知 (`Stream3DRoutingDiagnostic`、§13.3)
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
| `Stream3DRolloffMin` | F32 (m) | `1.0` | モノラルタグ既定の近距離 (mono の `{min}` 省略時) |
| `Stream3DRolloffMax` | F32 (m) | `20.0` | 既定の遠距離 (mono の `{max}` / stereo の `{range}` 省略時の共通フォールバック) |
| `Stream3DMaxDistance` | F32 (m) | `64.0` | プリム検出のポーリング半径。`Stream3DRolloffMax` 以上に設定 |
| `Stream3DPollInterval` | F32 (秒) | `30.0` | Description ポーリング間隔。LSL 経由のタグ変更検出にこの間隔がかかる。0 で能動ポーリング無効 |
| `Stream3DVolumeMaster` | F32 [0〜1] | `0.5` | マスター音量倍率。Preferences の 3D Stream スライダーと同じ |
| `Stream3DReconnectAttempts` | S32 | `3` | ストリーム切断時の自動再接続試行回数。各リトライは 5 秒待機。0 で再接続無効 |
| `Stream3DRoutingDiagnostic` | bool | `false` | routing 診断 chat 通知の ON/OFF (§13.3)。Preferences のチェックボックスと同期 |

#### r11/r12 配信者主導モデル: listener 側 sentinel (一般 UI なし)

§7.5 / §8.4 で説明したとおり、`binaural` / `venue` / `wetgain` / `lfegain` / `upmix` は **配信者タグが真実**で、Preferences には UI を出していません。ただし救済目的に限り debug settings に sentinel を用意しています。一般 listener は触らないでください。

| 設定キー | 型 | 既定値 | 意味 |
|---|---|---|---|
| `Stream3DBinauralRender` | S32 | `-1` (sentinel = タグ通り) | `0` で listener 側強制 OFF / `1` で強制 ON。スピーカー視聴で `{binaural:on}` 配信を聴くときの救済用 (詳細 §7.5 例外節) |
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
  例: [3dstream-stereo:{ch:L}{range:30}]
```

```
3D Stream: 構造エラー (リンクセット root: "MainStage")
  音源宣言 (url) が root にあるがスピーカー (ch) が見つかりません。
  各スピーカープリムに [3dstream-stereo:{ch:L|R|M}] を記載してください。
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
2. **タグの綴り**: `[3dstream:` または `[3dstream-stereo:` が含まれているか (タイポ注意)
3. **`{url:...}` のスキームが http/https**: `file://` や相対 URL は不可
4. **`Stream3DEnabled` / `Stream3DDescriptionScan` が true**: Preferences > Sound または Debug Settings で確認
5. **ポーリング待ち**: LSL `llSetObjectDesc` 経由の変更は最大 30 秒待つ (= `Stream3DPollInterval`)
6. **チャットにエラー通知が出ていないか**: §13 のエラー文言を確認
7. **ログに `LL_INFOS("Stream3D")` の reconnect attempt が出ていないか**: ストリーム URL が落ちている可能性

### 14.2 ステレオの片方しか鳴らない

- ルートに `{url}` のみ書いて子に `{ch:R}` だけ書いた場合、L チャンネルを担当するプリムがいないので「L 片肺」状態になります。ルートに `{ch:L}` を併記するか、別のプリムに `{ch:L}` を割り当ててください
- 同じ `{ch:L}` を 2 つのプリムに書いて両方からダブって鳴らしたい場合は意図通りなので OK
- routing 診断 (`Stream3DRoutingDiagnostic`) を ON にすると、各 ch が何を再生しているかが chat に出ます

### 14.3 5.1ch ソースが開けない / 音がブツブツ切れる

- §11.4 の seek 制約: 単純 HTTP / Icecast push の Opus 6ch / FLAC 6ch で起きやすい問題。**Vorbis 6ch に切り替える** か、SHOUTcast 互換 streamer / ffmpeg primary 経由に切り替えてください
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

- 再評価のトリガが発火していない可能性。プリムを 1 度移動するか、ぐるりと回って再 polling を待ってください
- それでも止まらない場合は `Stream3DEnabled` を一旦 false にして全 binding を強制解除、再 true で再発見

---

## 15. 既知の制約 / 仕様上の注意

### 15.1 リスナー位置はカメラまたはアバター

3D Stream の音場計算に使われるリスナー位置は、Preferences > Sound の **「Hear media and sounds from:」** 設定に従います。

- `Camera` (既定): カメラ位置 / カメラ向き
- `Avatar`: アバター位置 / アバター向き

これは LSL `llPlaySound` / パーセル BGM / Media-on-a-Prim とも共通の設定です。

### 15.2 1 リンクセット = 1 ストリーム

1 つのリンクセット内に `{url}` を持つルートが「ある」/「ない」だけが意味を持ちます。**複数の `{url}` を 1 リンクセットに書くことはできません** (子プリムに `{url}` を書いても無視されます)。

複数の異なるストリームを 1 つの会場で鳴らしたい場合は、リンクセットを分けて配置してください (= `Stream3DMaxConcurrent` の枠内で複数 binding を持つ)。

### 15.3 Description 文字数 (127 byte)

LSL `llSetObjectDesc` が書き込める Description は **127 byte 上限**です。日本語を含む URL や説明文は UTF-8 で容易に超過します。

長くなる場合の対処:

- ルートに `{url}` だけ書いて子プリムに `{ch}` だけ書く分散方式 (この場合、各プリムの Description は短く保てます)
- URL を短縮 (URL shortener、または配信側のパス短縮)
- **キー名 / venue 値の短縮形を使う** (§4.5)。`binaural`/`venue`/`wetgain` は `bin`/`v`/`wg`、venue 値 9 種にも 1〜2 文字エイリアスがあり、長形式と完全等価です

### 15.4 codec 別の動作実績

| codec | 1ch / 2ch | 6ch |
|---|---|---|
| Vorbis (Ogg) | ✓ 実機検証済 | ✓ 実機検証済 (r9 P10、12 分連続 0 dropout) |
| Opus (Ogg) | ✓ 実機検証済 | △ コードレビューのみ (本番経路で動作実績、static HTTP / Icecast push は seek 失敗) |
| FLAC | ✓ 実機検証済 | △ コードレビューのみ (Opus と同制約) |
| MP3 | ✓ 実機検証済 | — |

5.1ch を確実に動かしたい場合は **Vorbis 6ch** を選んでください。

### 15.5 LFE の特殊扱いはなし

5.1ch の LFE (サブウーファー) は、viewer 側で「ローパスフィルタ」「2D 化」などの特殊処理は行われず、他の 5 チャンネルと同等に 3D 配置 + 距離減衰されます。低域フィルタリングが必要なら配信側 mix で済ませてください。

物理的なサブウーファー筐体を SL 内のプリムとして配置し、その位置から低域音を出すという運用が想定されています。

### 15.6 5.1ch の自由視点モデル

実 5.1 (映画基準・ITU-R BS.775) は **リスナーが固定位置にいる前提**で各 ch に方向感を埋め込みます。SL のリスナーは自由視点なので「sweet spot」概念は適用できません。本機能で目指すのは **「会場で 5.1 ソースを多点再生する」** という PA 的な発想であり、シネマ的サラウンド体験の再現ではありません。

リスナーが空間を歩き回ると 5.1 mix の意図した定位は当然崩れますが、「会場感」「面で鳴っている感」は十分に出ます。

### 15.7 他 Viewer での挙動

`[3dstream:...]` / `[3dstream-stereo:...]` タグは **AYAstorm 専用** です。本家 Firestorm / 公式 LL Viewer / Catznip 等の他 Viewer は完全に無視します。

- AYAstorm 利用者には 3D 定位再生される
- 他 Viewer 利用者にはタグが説明文の一部として表示されるだけで、音は鳴らない (パーセル BGM とは独立に動作するため、パーセル BGM が設定されていればそれは聞こえる)

### 15.8 同時最大数

| 上限 | 既定 |
|---|---|
| `Stream3DMaxConcurrent` (linkset 単位の binding 数) | 4 |
| `Stream3DStereoMaxSpeakers` (1 binding あたりのスピーカー数) | 16 |
| 結果として最大 同時スピーカー数 | 4 × 16 = 64 |

64 channel 程度までは FMOD の余裕があります。それ以上必要な場合は debug settings で値を上げてください (実機での CPU 負荷確認は必須)。

### 15.9 音量の合成

最終音量 = `Stream3DVolumeMaster` × `{volume:N}` × FMOD 距離減衰 × Master Audio Slider × 各種ミュート状態。

通常は `Stream3DVolumeMaster` (Preferences の 3D Stream スライダー) で全体調整、`{volume:N}` でプリム個別の補正、距離減衰は `range` (スピーカー個別) または `Stream3DRolloffMax` (全体既定) で制御します。

---

## 16. 静的 occlusion `[ayastorm:occlude]` (r13)

**会場運営 / 建設者向け** のタグです。壁・扉・床・天井などの「音を遮るプリム」にこのタグを書くと、AYAstorm はそのプリムをリスナー位置と音源プリム位置の間にある **遮蔽物** として扱い、音をこもらせます。

`[3dstream:...]` / `[3dstream-stereo:...]` (§5 / §6) が **音を出す側** のタグなのに対して、`[ayastorm:occlude]` は **音を遮る側** のタグです。両者は完全に独立で、occlude タグだけ書いたプリムからは音は鳴りません。

### 16.1 書式

```
[ayastorm:occlude]                            ← 既定値 (direct:0.7 reverb:0.5)
[ayastorm:occlude{direct:0.9}{reverb:0.7}]    ← 値指定
[ayastorm:occlude{direct:0.6}]                ← 一方だけ指定 (もう一方は既定)
```

旧プレフィクス (`[ayastream:occlude]`) は **受け付けません**。occlusion は r13 で新設の機能で、ayastream 系の遺産プリムが存在しないため。共通の書式ルール (§4.3、キー名は大文字小文字非区別 / 値は前後空白 trim / 未知キーは黙って無視) はそのまま適用されます。

### 16.2 動作モデル

#### 何が遮蔽されるか

- **`[3dstream:...]` / `[3dstream-stereo:...]` から鳴る音** (3D 定位ストリーム、スピーカープリムごと)
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
| `docs/specs/spec_stream3d_decode_thread.md` | r7 で確立した 3-thread モデル |
| `docs/specs/spec_distributed_stereo.md` | r8 分散記述ステレオ仕様 — `[3dstream-stereo:...]` の field 書式 |
| `docs/specs/spec_5_1ch_source.md` | r9 5.1ch ソース受入仕様 — Opus/FLAC 6ch decode 経路 + BS.775 ダウンミックス |
| `docs/specs/spec_5_1ch_placement.md` | r10 5.1ch 会場配置仕様 — `ch=FL/FR/C/LFE/SL/SR` 拡張 + 互換マトリクス |
| `docs/specs/spec_binaural_venue_reverb.md` | r11 バイノーラル + 会場残響仕様 (r12 と同梱配布)。lite-HRTF / 9 venue / wetgain 詳細 |
| `docs/specs/spec_stereo_upmix.md` | r12 stereo→5.1 upmix 仕様 — DPL2 系 matrix decode + 帯域分離アルゴリズム詳細 |
| `docs/ayastorm-r12-stereo-upmix.md` | r12 phase 分解 (P0-P11) と工数見積 |
| `docs/ayastorm-r13-occlusion.md` | r13 OBB occlusion 仕様 + 実装記録 — `[ayastorm:occlude]` 設計判断 / spike 実装 / 残工程 |
| `docs/ayastorm-stream3d-roadmap.md` | 3D Stream 全体ロードマップ (r5〜r13+) |

---

## 改訂履歴

- **2026-05-05 (初版)**: r10 時点の最終仕様として整備。r5 / r8 / r9 / r10 / r10.x の累積仕様をまとめて記述。r11 以降は未リリースのため対象外。
- **2026-05-08 (r12 改訂)**: r11 (バイノーラル / 会場残響 / wetgain) と r12 (stereo→5.1 upmix / タグ短縮形 `bin`/`v`/`wg` + venue 値短縮) を追記。r11 は独立リリースせず r12 に同梱配布する方針のため、ユーザー向けには r10 → r12 の 1 ジャンプとなる。§7 / §8 / §4.5 を新設、章番号 §7-§14 を §9-§16 に繰り下げ。
- **2026-05-09 (r12.1 改訂)**: `{lfegain:N}` キー (短縮形 `lg`) を §7.4 として新設、旧 §7.4 配信者主導モデルを §7.5、旧 §7.5 組合せ例を §7.6 に繰り下げ。`wetgain` の既定値を `1.0` → `0.2` に変更 (実 listening での音楽的レンジ 0.1〜0.5 反映)。§12.2 に `Stream3DLfeGain` sentinel 追加。§12.2 / §12.3 にライブチューニング修正の注記を追加 (r12 で `Stream3DUpmix*` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` がプリムタッチまで反映されなかった回帰を修正)。
- **2026-05-11 (r13 改訂)**: 静的 OBB occlusion タグ `[ayastorm:occlude]` を §16 として新設、旧 §16 関連ドキュメントを §17 に繰り下げ。§4.1 を「2 種類のタグ」→「3 種類のタグ」に拡張。debug settings (`Stream3DOcclusion` master sentinel / `Stream3DOccluderRange` 距離 cull / `Stream3DOcclusionRampMs` smoothing / `Stream3DShowOccluders` 可視化) を §16.6-§16.8 に記述。関連 spec として `docs/ayastorm-r13-occlusion.md` を §17 表に追加。
- **2026-05-11 (r13 P15 改訂)**: occlusion 判定を OBB 近似から **実プリム三角形 raycast** に拡張 (Möller-Trumbore + OBB pre-cull の 2 段判定)。Path Cut / Hollow / Mesh の実形状が遮蔽計算に反映される (§16.2)。複数プリム集計を **掛け合わせ** に修正記述 (実装は当初から掛け合わせだったが旧版で「max」と誤記)。`Stream3DShowOccluders` 表示を OBB ワイヤーフレームから **シアン三角形メッシュ** (半透明 fill + wireframe) に変更、build floater で選択中のプリムは編集中ライブ追従 (§16.8)。§16.9 に三角形数上限 2000 と OBB-only フォールバック条件を追記。§16 タイトルを「静的 OBB occlusion」→「静的 occlusion」に短縮。
