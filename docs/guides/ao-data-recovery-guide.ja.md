# AO データ復旧ガイド (AYAstorm r31.2)

このガイドは「Firestorm 系 viewer (Firestorm 本家 / AYAstorm 旧 build / その他派生) で AO セットを削除したら、別の viewer でも AO が消えていた」という被害に遭った方向けの復旧手順書です。

## まず最初に知っておいてほしいこと

### 削除済の AO データは戻りません

残念ながら、一度 viewer が削除した AO データ (AO セット内の各アニメーション、設定 notecard) は **どうやっても元の状態に戻すことはできません**。Second Life サーバ側で完全に消えています。Linden Lab に問い合わせても復旧はできません。

ただし **AO 機能をもう一度使えるようにすることは可能** です。このガイドの「やること 2」で説明します。

### なぜこんなことが起きたのか

Second Life の Inventory のうち、AO セットの保管場所は Firestorm 系 viewer 全部で共通になっています。Firestorm 本家、AYAstorm、その他 FS 派生 viewer は、SL アカウントの同じ場所に AO データを置きます。

これは設計上「どの viewer でログインしても同じ AO が使える」ようにするためですが、副作用として「ある viewer で AO を削除すると、別の viewer でも AO が消える」状態になっていました。

AYAstorm r31.2 では、AO を削除してもデータを消さないように直しました。

---

## やること 1: AYAstorm r31.2 以降を入れる (再発防止)

1. AYAstorm の Web サイトから r31.2 (またはそれ以降) のインストーラをダウンロード
2. インストール → 起動 → SL アカウントでログイン
3. 以降、AO ウィンドウでセットを「削除」しても **データは残ります** (= 非表示にするだけ)

これだけで再発は防げます。

---

## やること 2: 失った AO をもう一度使えるようにする (復元手順)

すでに削除済の AO セットそのものは戻せませんが、AO 機能をもう一度動かすことはできます。以下のどちらかの方法で AO を入れ直してください。

### A. AO の notecard backup を持っている場合

SL では古くから「AO 設定を notecard でバックアップする」習慣があります。心当たりがあれば:

1. Inventory ウィンドウで `Notecards` フォルダ (または保管場所) を開く
2. 該当する AO 設定 notecard を探す
3. AYAstorm の AO ウィンドウ (アバターメニュー → アニメーションオーバーライダ) を開く
4. 「+」ボタンで新しいセットを作成
5. notecard を AO ウィンドウにドラッグ&ドロップで import
6. インポートしたセットを選んで動作確認

### B. backup が無い場合 (大半の方)

SL Marketplace で AO を入手し直してください。無料のものでもよいです:

- **Vista Free AO** ── 古くから定番の無料 AO
- **Animare Free AO** ── 同じく無料
- **ZHAO-II** ── notecard 標準形式の元祖
- 検索キーワード: "AO HUD", "Animation Override", "AO Free"

入手したら:
1. Marketplace から購入 (無料品でも「購入」ボタンで Inventory に入ります)
2. AO 商品に含まれる notecard を rez (地面に出す) または Inventory から取り出す
3. AYAstorm の AO ウィンドウを開く → 「+」 → notecard をドラッグ&ドロップ
4. 動作確認

これで AO 機能は元通り使えるようになります。

### 補足: Linden 配布のプリセット AO を失ったと思われる方へ

もし元々 Linden Lab が配布していたプリセット (Library 由来の AO 等) を失ったと思われ、**さらにそれ自体の回復を望まれる場合** は、viewer サイドからは復元できないため Linden Lab Support へお問い合わせください。

AO 機能を「再び使える状態に戻す」だけが目的の場合は、上記の B. Free AO 取り直しで十分です。

---

## やること 3: 非表示にしたセットを管理する (AYAstorm r31.2 以降の新機能)

AYAstorm r31.2 では「削除」の代わりに「非表示」になります。非表示にしたセットの確認と復元:

1. AO ウィンドウを開く
2. **「Manage hidden sets」** ボタンを押す (画面下部)
3. 非表示にしたセットの一覧が出る
4. 個別に戻したい → セットを選択 → 「Restore selected」
5. 全部戻したい → 「Restore all」

データ自体は触られていないので、Firestorm 本家など他の viewer でログインしても、これらのセットは残ったままです。

---

## Firestorm 本家を使うときの注意

Firestorm 本家側 (本ガイド作成時点で未修正) で AO セットの「削除」操作を行うと、依然として AO データが消えてしまいます。

**もし Firestorm 本家でうっかり削除してしまった場合**:
→ 上記の「やること 2」と同じ手順で AO を入れ直せば、機能としては元通り使えます。

**推奨運用**:
- AO の編集・削除操作は AYAstorm r31.2 以降で行う
- Firestorm 本家側からは AO セットを「使う」だけにする (削除しない)
- AYAstorm で「非表示」にしたセットは Firestorm 本家側からは普通に見え、使えます

---

## それでも困ったら

- AYAstorm の不具合報告先: [GitHub Issues](https://github.com/mayatonton/phoenix-firestorm/issues)
- SL 公式: AO データはサーバ側に存在しないので Linden Lab に問い合わせても復旧はできません

---

技術的な詳細は [docs/specs/ayastorm-r31-2-ao-bridge-recovery.md](../specs/ayastorm-r31-2-ao-bridge-recovery.md) を参照。
