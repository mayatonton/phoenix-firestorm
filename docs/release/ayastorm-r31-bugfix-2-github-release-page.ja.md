🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.zh.md)

# AYAstorm r31-bugfix-2 — AO 削除事象救済 + LSL Bridge 衝突防御 + 3D Stream URL filter + 装着物アルファ順序 + Cinematic glow + 水中アルファ

> [!IMPORTANT]
> **r31-bugfix-2 は Firestorm 系 viewer 全体に存在する 2 件の構造的な振る舞いを AYAstorm 側で食い止めるリリースです。**
>
> AYAstorm 固有の問題ではなく、Firestorm 派生 viewer 全体で共有される inventory root に起因する構造的な振る舞いへの対応です (バグか仕様かの判断は upstream にあります)。

1. **AO 削除事象**: Firestorm 系 viewer (Firestorm 本家 / 旧版 AYAstorm / その他 FS 派生) で AO セットを「削除」すると、共有 inventory root `#Firestorm` 配下の AO データが永久消去され、別の viewer でログインしても消えたままになる cross-viewer 連鎖事象。1000 人規模で観測済。r31-bugfix-2 では通常の AO セット削除を per-account 設定の隠しフラグによる UI 非表示に置き換え、実 inventory を削除しません。Hidden 管理画面の「選択を削除」 (`Delete selected`) だけは、確認 dialog 後の明示的な完全削除として残します
2. **LSL Bridge version 衝突**: `fslslbridge.cpp` の version 不一致時自動再作成ロジックが、Firestorm 本家のマイナーバンプ時に AYAstorm Bridge を巻き添えで削除しうる構造。現状 v2.29 同一で未発火だが、片方向防御として `受信 version > 自分なら adopt` ロジックを導入し、起動時 attach でも newer bridge を `BridgeVer` 受信前に detach しないようにしました

修正は Firestorm 系 viewer 全体に存在する構造的振る舞いへの対応です。AYAstorm 側では通常の AO 削除から `#Firestorm` root への破壊的操作を外します。Firestorm 本家 / 他派生 viewer での再発はそれぞれの viewer が patch される必要があります (推奨運用と回避策は recovery guide に明記)。

## すでに AO セットが消えてしまった方 — AO 再セットアップ手順

> [!IMPORTANT]
> 上に書いた事象によりすでに AO セットが消えてしまった方の **AO データそのものは viewer 側でも SL サーバ側でも取り戻せません**。ただし AO 機能自体は簡単な再セットアップで普通に使える状態に戻せます。**手順を 3 言語で公開していますので、ご自身の環境に合うものをご利用ください:**
>
> - 🇯🇵 [**日本語復旧手順**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
> - 🇺🇸 [**English Recovery Guide**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
> - 🇨🇳 [**繁體中文復原指南**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)
>
> r31-bugfix-2 を install していただくと **以降 AYAstorm 側では同じ事象は起きません**。Firestorm 本家 / 他 FS 派生 viewer での再発は各 viewer 側で patch される必要がありますが、その場合の回避策も復旧 guide 内に明記されています。

## 装着物アルファ render-order — 3-pass dispatch (PR [#122](https://github.com/mayatonton/phoenix-firestorm/pull/122))

これは r31-bugfix-2 の **AO 救済と並ぶ 2 大修正項目** の 1 つです。r30 §5 で出荷した「rigged hair / SIM N-BL render-order swap」は髪越し空抜けを解消しましたが、副作用として装着物 N-BL prim (まつ毛 prim 等) が rigged hair の前ではなく後ろに描かれ、その後 over-blend で潰される regression を出していました — 装着物 alpha prim を使う avatar (目 / 眉 / まつ毛が prim 構成のヘッド) で「眉まつ毛が薄い / 欠ける」として可視化される regression でした。

POST_WATER の forward pass を 3 sub-pass (SIM N-BL → 全 R-BL → 装着物 N-BL) に分割し、`LLDrawInfo::mAttachedToAvatar` を per-draw discriminator として両立。§5 swap fix を維持しつつ装着物 prim の前面整列を回復します。自動適用 — 設定変更は不要です。

falsified 案 A/B/C との構造的比較は [`docs/specs/ayastorm-double-alpha-c-plan-extension.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-double-alpha-c-plan-extension.md) を参照。

### Special thanks (PR #122 — 装着物アルファ render-order)

この修正には **neria (neriamm)** さんに多大なご協力をいただきました。複数の SL avatar での再現環境構築と候補修正の hands-on 検証により、構造的比較と最終的な実装選択の絞り込みが大きく短縮されました。neria さんは Second Life のレジデント貢献者で (GitHub アカウントではありません)、SL 名で表記しています。

## その他の同梱修正

r31-bugfix-2 では r31-bugfix-1 以降に着地した次の 3 件の修正も同梱しています:

- **3D Stream URL filter + UI 更新** (PR [#121](https://github.com/mayatonton/phoenix-firestorm/pull/121)): 3D Stream 機能を初期状態で OFF にしました。プロンプトの URL 再生確認ダイアログには送信元の object 名 / owner を表示し、誰が送ってきた URL なのかを各 viewer が判断できるようにします。rezz されたオブジェクト経由の意図しない自動再生は source-of-truth の段階でブロックします。ユーザー向け解説は [`docs/specs/3dstream-user-guide.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.ja.md) を参照
- **Cinematic glow min-luminance 修正** (PR [#123](https://github.com/mayatonton/phoenix-firestorm/pull/123)): BD parity port で持ち込まれた `RenderGlowMinLuminance = 0.0` が、blank texture + 色付きプリム (装着物 / SIM rez object 両方) で prim 側 Glow=0 設定でも post-process bloom を誤発火させていました。閾値を `0.5` に引き上げ、r31.0 / r31.1 で `0.0` を persist 焼きしてしまったユーザーは次回 Cinematic mode 起動時に one-shot migration で強制矯正します。Firestorm mode は影響なし (LL default `1.0` のまま、migration は skip され次回 Cinematic 起動で再検査)
- **水中アルファ plate 修正** (PR [#124](https://github.com/mayatonton/phoenix-firestorm/pull/124)): r30 P5 transparent-DoF C-(a) で導入した `mAYAAlphaColor` redirect が `LLPipeline::sUnderWaterRender` に対応していませんでした。水中では main RT に underwater fog 着色済みの opaque scene があるのに、独立 plate は `(0,0,0,0)` で clear → forward alpha が plate に書き込み、pre-tonemap composite (`GL_ONE / GL_ONE_MINUS_SRC_ALPHA`) で plate が underwater 着色 main RT を上書きしてしまい、まつ毛 / 眉などの装着物アルファプリムと SIM particle の透過部分が **水中で真っ黒** で描画されていました。水上に出た後にも数フレーム再発する症状あり。`use_alpha_rt` 条件に `!sUnderWaterRender` を追加して水中時は plate redirect を skip、forward alpha は main RT に直書き = upstream FS 互換挙動。水上時の振る舞いは旧と完全一致

## Release notes

- 🇺🇸 English: [docs/release/ayastorm-r31-bugfix-2-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-bugfix-2-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-bugfix-2-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.zh.md)

## Key documents (tag pinned)

- AO + Bridge 技術 spec: [docs/specs/ayastorm-r31-2-ao-bridge-recovery.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-ao-bridge-recovery.md)
- AO ユーザー復旧手順 (3 言語): [docs/guides/ao-data-recovery-guide.{en,ja,zh}.md](https://github.com/mayatonton/phoenix-firestorm/tree/v7.2.4-ayastorm-r31-bugfix-2/docs/guides)
- 3D Stream URL filter 報告書 (日本語): [docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md)
- 3D Stream ユーザーガイド (3 言語): [docs/specs/3dstream-user-guide.{en,ja,zh}.md](https://github.com/mayatonton/phoenix-firestorm/tree/v7.2.4-ayastorm-r31-bugfix-2/docs/specs)
- アルファ render-order 拡張報告書: [docs/specs/ayastorm-double-alpha-c-plan-extension.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-double-alpha-c-plan-extension.md)
- 6 カテゴリ render order trace: [docs/specs/ayastorm-six-category-render-order-trace.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-six-category-render-order-trace.md)

## 既存環境との互換性

r31 / r31-bugfix-1 環境を乱さずに出荷:

- **AO 削除事象 fix**: 自動適用。設定変更不要。r31 / r31-bugfix-1 install 済の方は r31-bugfix-2 を上書き install するだけで、通常の AO セット削除は非破壊の Hide になります
- **LSL Bridge 衝突防御**: 自動適用。Firestorm 本家のマイナーバンプ時に AYAstorm Bridge が削除される事象を防止 (現状未発火、将来防御)
- **3D Stream 初期 OFF + URL filter**: すでに 3D Stream 有効で運用中の配信者は `Stream3DEnabled = true` の設定を持ち越します。新規 install では初期 OFF からの開始です
- **アルファ render-order 3-pass dispatch**: 全 avatar に自動適用。装着物 N-BL prim (まつ毛 prim 等) が rigged hair の前面に正しく整列するようになり、§5 swap による髪越し空抜け修正は維持されます
- **Cinematic glow min-luminance**: `0.0` を persist 焼きしていたユーザーの **次回 Cinematic mode 起動時** に one-shot migration が走ります。Firestorm mode 専用ユーザーは影響なし (LL default `1.0` のまま)
- **水中アルファ plate**: 自動適用。水中時は alpha BLEND が独立 plate を経由せず main RT に直書きされ、underwater fog 着色が composite 越しに保持されます。副作用: 水中での transparent-DoF C-(a) plate composite 効果は無効化されますが、水中はそもそも全画面 fog で bokeh 構造的に不可視のため視覚差ほぼなし。水上時は変更なし
- **r31 / r31-bugfix-1 の全機能** (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP audio routing / macOS branding / GPU other-rigged picker / chat tab split / venue reverb / SSS pink-shadow 修正等): そのまま保持されます
- **AO を編集 / 削除しない方**: 見た目の変化はほぼありません。AO set の Trash icon は非表示 icon になり、Dialog 文言が Hide 前提に変わります
- **AO を整理したい方**: Hidden 管理画面の「選択を削除」 (`Delete selected`) から、確認後に選択済み hidden set の実 inventory folder を完全削除できます。これは元に戻せません
- **すでに AO セットが消えてしまった方**: r31-bugfix-2 を入れていただくと **以降は同じ事象は起きません**。すでに消えた AO データ自体は戻せませんが、上の再セットアップ手順で AO 機能はすぐ使える状態に戻せます

## IR licence

r11 で同梱し以降も出荷中の venue IR は OpenAIR (CC-BY 4.0) 由来です。出典: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/indra/newview/app_settings/venue_ir/CREDITS.md)

## Downloads

- [Windows Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_AVX2-7-2-4-261492019_Setup.exe)
- [macOS Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-81339.dmg)
- [Linux Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261500427.tar.xz)

## Contributors

@t-noami @mayatonton

(PR #122 への neria (neriamm) さんのご協力については、上記「装着物アルファ render-order」セクション内の「Special thanks」を参照)
