> **Language / 言語 / 语言**: [English](./skin-sss-user-guide.md) · **日本語** · [中文](./skin-sss-user-guide.zh.md)

# Skin SSS (サブサーフェス・スキャタリング) — 使い方ガイド

**搭載**: AYAstorm r20 以降。UI は r30 で **AYAstorm Controls** に移動。

**場所**: トップメニュー → **AYAstorm → AYAstorm Controls...** (ショートカット `Alt+C`) → **Skin SSS** タブ。

![AYAstorm Controls の Skin SSS パネル](./images/skin-sss/panel-overview.png)

### 適用例 (同じシーン・同じ照明・同じカメラ)

| Skin SSS **OFF** | Skin SSS **ON** |
|:---:|:---:|
| ![SSS off](./images/skin-sss/example-off.png) | ![SSS on](./images/skin-sss/example-on.png) |
| 肌のハイライトが硬く、頬から影への移行がエッジとして読める。 | ハイライトが柔らかくなり、頬から影への移行はなだらかに、頬と鼻まわりにわずかな暖色感が立つ。 |

---

## 1. Skin SSS とは

実際の肌は硬い面ではなく、光が少しだけ内部に入り込み、散乱して柔らかく・少し色付いて出てきます (耳が逆光で赤く透ける、ポートレート写真で頬や鼻まわりに「あたたかい柔らかさ」が出る、あれです)。SL では多くの Skin ブランドが描き込みと質感表現の力でこの課題に長年取り組んでこられていて、いま見られる肌表現は本当に素晴らしい域に育っています。**Skin SSS** はその Skin の力を **レンダラ側からさらに下支えしたい** という思いで作った機能です。

アバターの **肌部分にのみ** 画面空間サブサーフェス・スキャタリング近似を適用し、衣服・髪・小物には触れません。見た目の変化:

- 頬・耳・鼻の「明暗の境目」が柔らかく、わずかに暖色寄りに
- 硬いハイライトピークが暖色のにじみへと変わり、光が「硬い面で反射した光」ではなく「肌の上に灯る光」として読める
- オプションで「赤寄りの glow」を足し戻し、ぼかしで失われがちな小さなハイライトピークを血色感として復活

動いている間は控えめ・止まると確実に分かる効果として、ポートレート / シネマティック撮影向けに設計されています。

## 2. 前提条件

| 必要条件 | 補足 |
|---|---|
| View Mode | **AYAstorm View** (Preferences → グラフィック)。Firestorm View では SSS pass は走りません。 |
| Deferred Rendering | 必須 (AYAstorm View では常時 ON)。 |
| アバター Mesh body / head | 対象 Mesh の asset UUID が whitelist (§4) に登録されているもの。**出荷時の whitelist は空** なので、最初に自分の使っている Mesh body / head を登録する必要があります。 |

## 3. コントロール一覧

すべて AYAstorm Controls の **Skin SSS** タブ内。各行右端の **D** ボタンで個別 default に戻せます。

| コントロール | 動作 | Default |
|---|---|---|
| **Enabled SSS** | SSS pass のマスタースイッチ。OFF で SL 標準の肌ライティングに戻る。 | ON |
| **Blur radius** | 肌の中に光が「染み込む」距離 (1m 視点距離基準)。shader が距離に応じて自動縮小するので、カメラ距離が変わってもルックが安定します。 | 1.0 |
| **Strength** | 元の照明結果と SSS でぼかした結果の混合比。0.0 = SSS 不可視 / 1.0 = SSS 100%。ポートレートは 0.4 〜 0.6 あたりが自然。 | 0.5 |
| **Glow gain** | ぼかし後に赤寄り highlight を足し戻す強さ。強い blur で眠くなりがちなハイライトピークを血色感として補償。0.0 = 純 SSS / 上げるほど血色寄り。 | 0.2 |
| **Glow color** | 足し戻す highlight の色味。default 純赤 (1, 0, 0) は血色感として読みやすい。オレンジ寄りで暖色肌、マゼンタ寄りでクール / 白肌寄り。 | 純赤 |
| **Reset all to defaults** | 上記 5 値 + whitelist (§4) を一括で AYAstorm default に戻す。 | — |

おすすめ手順: **Blur radius** は 1.0 のまま、**Strength** で頬・鼻の柔らかさを決め、**Glow gain** で峰のハイライトを少しだけ戻す。

## 4. Whitelist — viewer に「これは肌」と教える仕組み

SL には「この mesh は肌か服か?」を問い合わせる API がありません — renderer から見ると Mesh body / head / hands は全部同じ顔をしています。目・歯・爪・アクセサリーまで誤って blur しないように、Skin SSS は **明示的に登録した Mesh asset UUID のみ** に適用されます。

### Mesh の登録

1. 対象の Mesh body / Mesh head を **装着**。
2. インワールドで自分のアバターの装着オブジェクトを **右クリック** → **Add to SSS whitelist**。
   - 多パーツ body (頭 + 胴 + 手) は同メニューの **Add entire linkset to SSS whitelist** で linkset 全体を一括登録できます。
3. Mesh の asset UUID が **Skin SSS** タブの whitelist テキストエリアに追加されます。

**出荷時の whitelist は空** です。SSS を有効化したら、上記の手順で自分の使っている Mesh body / head を登録してから効果が見えるようになります。

### 手動編集

whitelist は素のテキスト — 1 行 1 UUID (36 文字 dash 区切り)。誤編集防止のため初期状態は **ロック** (読み取り専用):

- **Lock editing (prevent accidental changes)** を外すと編集可能。
- ノートカード等から UUID を 1 行ずつ貼り付け。
- 編集後はロックを戻す。リストは自動保存。

### Mesh の登録解除

装着オブジェクトを右クリック → **Remove from SSS whitelist** (linkset 一括解除は **Remove entire linkset…**)。

## 5. クイックスタート

1. AYAstorm → AYAstorm Controls (`Alt+C`)。
2. **Skin SSS** タブを開く。
3. View Mode が **AYAstorm View** であることを確認 (違えば Preferences → グラフィック で切替)。
4. **Enabled SSS** を ON。
5. アバターの顔を見る。ハイライトが硬いまま変化が無いようなら Mesh head が whitelist に未登録 — 頭を右クリック → **Add to SSS whitelist**。
6. もっと柔らかくしたい場合は **Strength** を 0.7 へ。血色感が抜けて見えるようなら **Glow gain** を 0.3 程度に。

## 6. Tips

- **ポートレート**: **Strength** を少し上げて (0.6 – 0.7)、**Glow gain** は 0.2 – 0.3。柔らかいが平坦に見えない。
- **全身 / 風景込み**: default 0.5 / 0.2 で基本 OK。距離が出れば blur radius は自動縮小されるので過剰には乗らない。
- **目や歯までぼやける**: その Mesh の UUID が肌として誤登録されています。該当装着物を右クリック → **Remove from SSS whitelist**。
- **Enabled トグルで何も変わらない**: View Mode が AYAstorm View か、装着 Mesh の **少なくとも 1 つ** の UUID が whitelist にあるかを確認。
- **負荷**: SSS は単一 screen-space pass で SSAO 同等。フレーム時間の主因にはなりません。

## 7. Default に戻す

- **1 項目だけ**: 該当行の **D** ボタン。
- **このタブ全部 (whitelist 含む)**: **Reset all to defaults**。

whitelist の default は **空リスト** なので、リセットすると登録済みの Mesh body / head はすべて消えます。再度 §4 の手順で登録し直す必要があるので、承知のうえで使ってください。

## 8. トラブルシューティング

| 症状 | 原因 | 対処 |
|---|---|---|
| SSS ON/OFF で見た目が変わらない | View Mode が Firestorm View | Preferences → グラフィック で AYAstorm View に切替 |
| 顔だけ柔らかく、手足はハイライトの硬さが残る | 手足が別 Mesh asset で whitelist 未登録 | 該当パーツを右クリック → Add to SSS whitelist |
| 目・歯・爪までぼやける | それらが body と同じ Mesh UUID を共有、または誤って whitelist に追加 | 右クリック → Remove from SSS whitelist (またはテキストから該当 UUID を削除) |
| whitelist テキストエリアが編集できない | ロック ON (default の安全状態) | **Lock editing** を外す |
| 効果が強すぎ / 弱すぎ | **Strength** の値 | 0.4 – 0.6 の範囲で自然 |

## 9. さらに掘り下げる

- SSS shader と whitelist 機構のエンジニアリング spec: [`docs/specs/ayastorm-r20-avatar-skin-sss.md`](./ayastorm-r20-avatar-skin-sss.md)
- UI 配置変更 (r30): [`docs/specs/ayastorm-r30-aya-controls-tab-overhaul.md`](./ayastorm-r30-aya-controls-tab-overhaul.md)
