🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-github-release-page.zh.md)

# AYAstorm r31-bugfix-1 — FullBright prim 越し SSS pink shadow 透け修正

r31-bugfix-1 は **bug 1 件単独修正リリース**です。SSS 肌を有効にしているとき FullBright prim 越しに浮かんでいたアバター形状の pink shadow を消します。本バグは r20+ 視覚的リアリティ章で SSS を初めて配線した時点から構造的に存在しており、過去 2 回 (各 3-4 時間) の調査でも解明できずに残っていました。

修正は SSS pass 内の場当たり `if` 分岐から一歩退き、**4 本の effect 軸 rendering routing 地図 (SSS / FullBright / Glow / Environment) を並列で先に整備**することで導出しました。4 本の独立調査がいずれも同じ根本原因 — single-RT FB pass 後の `gbuffer3.a` staleness — に収束したことを確認した上で、pipeline に手を入れています。実 fix は `pipeline.cpp` の dispatch 1 ブロック移動のみで、FB shader は無改修です。

## Release notes

- 🇺🇸 English: [docs/release/ayastorm-r31-bugfix-1-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-bugfix-1-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-bugfix-1-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/release/ayastorm-r31-bugfix-1-release-note.zh.md)

## Key documents (tag pinned)

- SSS rendering routing: [docs/specs/ayastorm-sss-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-sss-rendering-routing.md)
- FullBright rendering routing (案 D 議論 §9.1): [docs/specs/ayastorm-fullbright-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-fullbright-rendering-routing.md)
- Glow rendering routing: [docs/specs/ayastorm-glow-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-glow-rendering-routing.md)
- Environment rendering routing: [docs/specs/ayastorm-environment-rendering-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-environment-rendering-routing.md)
- PR #112 (merge 済): [https://github.com/mayatonton/phoenix-firestorm/pull/112](https://github.com/mayatonton/phoenix-firestorm/pull/112)

## 既存環境との互換性

r31 環境を乱さずに出荷:

- **SSS pink-shadow 透け fix**: 自動適用。ユーザー側の設定変更不要。r31 install 済の方は r31-bugfix-1 を上書き install するだけで FB prim 越しの pink-shadow 透けが消えます
- **r31 の全機能** (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP audio routing / macOS branding / GPU other-rigged picker / chat tab split / venue reverb 等): そのまま保持されます
- **SSS を使わない方**: 見た目の変化はありません。SSS 肌 pixel がシーン内に無いとき、dispatch 並び替えは機能的に no-op です
- **FB shader**: 一切触っていません。本 fix は可逆です

## IR licence

r11 で同梱し以降も出荷中の venue IR は OpenAIR (CC-BY 4.0) 由来です。出典: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/indra/newview/app_settings/venue_ir/CREDITS.md)

## Downloads

> [!IMPORTANT]
> **r31-bugfix-1 の binary は r31-bugfix-2 の release page に集約しています。**
> r31-bugfix-1 の SSS pink-shadow 修正は r31-bugfix-2 にもそのまま継承されています。下記の最新版 release page からダウンロードしてください:
>
> 👉 **[AYAstorm r31-bugfix-2 Release page](https://github.com/mayatonton/phoenix-firestorm/releases/tag/v7.2.4-ayastorm-r31-bugfix-2)**

## Contributors

@t-noami @mayatonton
