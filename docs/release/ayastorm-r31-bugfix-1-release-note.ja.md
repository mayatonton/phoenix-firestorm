# AYAstorm r31-bugfix-1 — リリースアナウンス

**r31-bugfix-1 は FullBright prim 越しに後ろのアバター形状の SSS pink shadow が透ける構造的バグを修正します** — r20+ の視覚的リアリティ章で SSS を入れた時点から構造的に存在していたバグで、r31 で導入されたものではありません。

実装詳細、routing 調査、設計ノートは `docs/specs/` 配下の 4 本の effect 軸 routing 地図に常設しています。本ノートは入口と差分ハイライトです。

---

## AYAstorm r31-bugfix-1 — FullBright prim 越し SSS pink shadow 透け修正

### 見出し: FullBright prim 越しに浮かんでいたアバター形状の pink shadow を消す

SSS 肌のアバターが FullBright prim の後ろに立っていると、その prim 表面に**アバター形状の** pink shadow が透けていました。空 (sky) の同様の透けバリアントは別途修正済でしたが、SSS バリアントは過去 2 回 (3-4 時間ずつ) の調査でも解明できず先送りされていました。r31-bugfix-1 でこれを構造レベルで解消しました。

このバグは r31 で導入されたものではありません。r20+ の視覚的リアリティ章で SSS を初めて配線した時点から存在しており、SSS を有効にして FullBright prim がシーン内にある AYAstorm のあらゆるバージョンが影響を受けます。

### 背景 — なぜ起きていたか

4 つの single-RT 出力経路が `gbuffer3.a` (SSS skin flag channel) に一切書き込みません:

- `POOL_FULLBRIGHT`
- `POOL_FULLBRIGHT_ALPHA_MASK`
- `POOL_BUMP` FB Shiny
- `POOL_ALPHA` 内 FB 経路

結果として、既に framebuffer に書かれているアバターの上にこれらの pass が走った後、アバターの先行 opaque pass で書かれた **stale な `aya_sss_skin_flag = 1.0`** が `gbuffer3.a` に残ります。SSS pass (`skinSSSF.glsl`) はその `gbuffer3.a` を読み「ここは肌」と判定し、`mRT->screen` を blur します — その時点で `mRT->screen` には FullBright prim の表面色が書かれており、アバター形状の flag mask で blur されることで、occluded アバターの形をした pink shadow が FullBright prim 越しに浮かんで見えていました。

### 修正の仕組み

SSS dispatch 位置を移動しました:

- **修正前**: `POOL_ALPHA_POST_WATER` 到達時に SSS が走る (FB が scene color を上書きした後)
- **修正後**: `POOL_FULLBRIGHT` 到達直前に SSS が走る (FB が scene color を書く前)

これにより SSS は FB 上書き後の出力ではなく、softenLight 直後の素の skin 色を sample します。FB shader は無改修、`indra/newview/pipeline.cpp::renderGeomPostDeferred` の dispatch を 1 ブロック移動して `done_sss` / `sss_pass` 独立 flag/変数を導入したのみで、可逆な変更です。

検討した 4 案 (案 A: FB shader の MRT 化 + C++ pool 側 MRT bind、3 OS 込みで半日コース / 案 B/C: 別 compositing / 案 D: dispatch 並び替え) のうち、**FB shader や pool の MRT bind に一切触らずに SSS 経路を完全解消できる最小リスクの案 D を採用**しました。

### アプローチの転換 (技術ハイライト)

このバグは過去 2 回、SSS pass 内の場当たり `if` 分岐で対応しようとして失敗していました。r31-bugfix-1 では一歩退き、コードを書く前に **4 本の effect 軸 routing 地図を並列で整備**することから始めました:

- SSS rendering routing
- FullBright rendering routing
- Glow rendering routing
- Environment rendering routing

これらは既存の object 軸 routing 資料 (deferred shader / attachment / rez-object / gbuffer3-trace) と相補的で、「fragment がどのオブジェクト種別から来たか」ではなく「どの視覚 effect に着地するか」で同じ pipeline を辿ります。4 本の独立 agent 調査がすべて同じ根本原因 — single-RT FB pass 後の `gbuffer3.a` staleness — に収束したことで、pipeline を編集する前に診断の信頼度が確定しました。

4 本の routing 地図 (合計 1894 行) は本リリースに同梱され、今後の描画系バグ調査の再利用資産として永続保持されます。

### Migration note

- **ユーザー側の設定変更は不要です。** r31 install 済の方は r31-bugfix-1 を上書き install するだけで動作します
- r31 の全機能 (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP routing / macOS branding / other-rigged picker) はそのまま動作します
- SSS を実際に使っている方は、FullBright prim 越しの pink shadow 透けが消えます

### Known limitations / future work

- `gbuffer3.a` の構造的二重意味 (SSS skin mask と emissive MRT blend factor) は本 fix では解消していません。single-RT pass 後に `gbuffer3.a` を読む将来の effect で同型のバグが再発する可能性があります
- より構造的な fix (案 A: FB shader を MRT 化してクリーンな `gbuffer3.a` を書かせる) は将来課題として残っています。SSS については本 fix で完了です
- **macOS build は r32 章で同梱予定**です (r10.x の前例どおり、bugfix-N リリースは Linux/Windows のみ提供)

### Implementation summary

- `indra/newview/pipeline.cpp` (+43 / −8 行) — SSS dispatch 分離
- `docs/specs/ayastorm-sss-rendering-routing.md` (335 行、新規)
- `docs/specs/ayastorm-fullbright-rendering-routing.md` (542 行、新規)
- `docs/specs/ayastorm-glow-rendering-routing.md` (534 行、新規)
- `docs/specs/ayastorm-environment-rendering-routing.md` (483 行、新規)
- PR [#112](https://github.com/mayatonton/phoenix-firestorm/pull/112)

### Credits

[@mayatonton](https://github.com/mayatonton) — 実装、routing 地図整備、バグ解析。

### Documentation

- SSS rendering routing: [`docs/specs/ayastorm-sss-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-sss-rendering-routing.md)
- FullBright rendering routing (案 D 議論: §9.1): [`docs/specs/ayastorm-fullbright-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-fullbright-rendering-routing.md)
- Glow rendering routing: [`docs/specs/ayastorm-glow-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-glow-rendering-routing.md)
- Environment rendering routing: [`docs/specs/ayastorm-environment-rendering-routing.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-1/docs/specs/ayastorm-environment-rendering-routing.md)
- PR #112 (merge 済): [https://github.com/mayatonton/phoenix-firestorm/pull/112](https://github.com/mayatonton/phoenix-firestorm/pull/112)
