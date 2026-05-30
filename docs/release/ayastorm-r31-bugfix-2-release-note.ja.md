# AYAstorm r31-bugfix-2 — リリースアナウンス

> [!IMPORTANT]
> **r31-bugfix-2 は Firestorm 系 viewer 全体に存在する 2 件の構造的な振る舞いを AYAstorm 側で食い止めるリリースです。** 1000 人規模で観測されている AO セット消失事象の再発防止と、将来の LSL Bridge version drift による相互破壊への片方向防御。
>
> AYAstorm 固有の問題ではなく、Firestorm 派生 viewer 全体で共有される inventory root に起因する構造的な振る舞いへの対応です (バグか仕様かの判断は upstream にあります)。

AO + Bridge 修正と並ぶ **2 大修正項目** として、装着物 N-BL prim アルファ render-order の 3-pass dispatch (PR #122) も本リリースに同梱します。それに加え r31-bugfix-1 以降に着地した次の 3 件の修正も含んでいます: 3D Stream URL filter + UI 更新 (PR #121)、Cinematic mode glow min-luminance bugfix (PR #123)、水中アルファ plate redirect 修正 (PR #124)。各 feature 別の section は本ノート下部を参照。

実装詳細、影響範囲、復旧手順は `docs/specs/` および `docs/guides/` 配下に常設しています。本ノートは入口と差分ハイライトです。

---

## AYAstorm r31-bugfix-2 — AO 削除事象救済 + LSL Bridge 衝突防御

### 見出し: AO セット「削除」を非破壊 hide に置き換え、LSL Bridge 相互削除を片方向防御

Firestorm 系 viewer (Firestorm 本家 / 旧版 AYAstorm / その他 FS 派生) で AO ウィンドウの「削除」を押すと、その AO セットの Inventory 実体 (`#Firestorm/#AO` 配下のフォルダと配下の全アニメーション / notecard) が `purgeFolder` で永久消去されていました。`#Firestorm` root が Firestorm 派生 viewer 全体で共有されているため、ある viewer で削除すると、後で別の viewer (含 Firestorm 本家) でログインしても消えたままという cross-viewer 連鎖事故になっていました。

r31-bugfix-2 では通常の AO セット「削除」を実 inventory 削除ではなく、per-account 設定の隠しフラグによる UI 上の非表示に置き換えます。Hidden 管理画面内の「選択を削除」 (`Delete selected`) だけは、確認 dialog を経た明示的な完全削除として残します。同時に LSL Bridge の version 不一致時の自動再作成ロジックを修正し、Firestorm 本家が将来 Bridge をマイナーバンプした際の AYAstorm 側 Bridge 消失を片方向で防御します。

この振る舞いは r31 で導入されたものではありません。Firestorm 系 viewer に長く存在してきた構造的なものであり、AYAstorm を含む全 FS 派生 viewer が影響を受けていました (バグか仕様かの判断は upstream にあります)。

### 背景 — なぜ起きていたか

**AO 削除事象**:
- `aoengine.cpp::removeSet()` → `purgeFolder(catID, true)` で **inventory 実フォルダを再帰削除**
- `#Firestorm/#AO/<set name>` の AO セットフォルダごと server から消える
- `#Firestorm` は Firestorm 派生 viewer 全体で共有される root のため、削除は全 viewer に伝播
- 一度発生すると viewer 側からは復旧不可能 (SL サーバ側で実体消失)

**LSL Bridge version 衝突**:
- `fslslbridge.cpp:239` で受信 bridge version 文字列が自分の `mCurrentFullName` と完全一致しない限り `recreateBridge()` が走る
- `finishBridge()` → `cleanUpOldVersions()` で自分より古い version の Bridge object を `#Firestorm/#LSL Bridge` から削除
- 現状 Firestorm 本家と AYAstorm はいずれも `v2.29` のため発火していないが、Firestorm が `v2.30` にバンプした時点で AYAstorm 側 Bridge が削除される

### 修正の仕組み

**AO 削除を soft hide 化**:
- `removeSet()` を完全に書き換え、`purgeFolder` 呼出を削除
- per-account 設定 `FSAOHiddenSets` (LLSD array, Persist=1) に inventory UUID を append するだけに変更
- AO 列挙時 (`update()`) に hidden filter で UI から除外
- 「Manage hidden sets」フロータを新規追加し、UUID 一覧から個別 / 全 restore 可能
- 同名 AO セットの hidden / visible 衝突を避けるため、hidden 中の名前との新規作成 / import 衝突と restore 時の visible 名衝突を拒否
- Hidden 管理画面に「選択を削除」 (`Delete selected`) を追加。これは通常の Remove とは別の明示的な完全削除であり、確認 dialog 後に選択済み hidden set の実 inventory folder だけを削除
- 削除 Dialog の文言を 3 言語で書き直し、通常の AO セット操作は「Delete」ではなく「Hide」として表示、inventory が残ることを明示
- AO set の soft-hide ボタン icon を trash ではなく非表示 icon に変更

**LSL Bridge 片方向 fix**:
- 受信 version 文字列を数値 parse し、`major.minor` で大小比較
- 受信 > 自分 ⇒ **削除せず adopt** (`mBridgeUUID` / `mCurrentURL` のみ更新、`recreateBridge` 呼ばず)
- 受信 == 自分 ⇒ 既存挙動
- 受信 < 自分 ⇒ 既存挙動 (`recreateBridge` で更新)
- parse 失敗 ⇒ 既存挙動 (安全側)
- 起動時 attach でも newer bridge を `BridgeVer` 受信前に detach しないよう、attach / detach 判定にも同じ version 比較を適用
- newer bridge adopt 経路も通常経路と同じ handshake 後処理に合流し、`URL Confirmed` と初回設定同期を送信

### Migration note

- **ユーザー側の設定変更は不要です。** r31 install 済の方は r31-bugfix-2 を上書き install するだけで動作します
- r31 の全機能 (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP routing / macOS branding / other-rigged picker) はそのまま動作します
- r31-bugfix-1 の SSS pink-shadow 修正もそのまま継承します
- 既存被害ユーザーの inventory は **viewer 側で復旧不可能** です。AO 機能を再び使えるようにする手順は [`docs/guides/ao-data-recovery-guide.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md) に分離して常設しています

### Known limitations / future work

- **片方向防御のみ**: AYAstorm が Firestorm 本家より version 先行する場合、Firestorm 本家側 (未修正) は AYAstorm Bridge を引き続き削除します。実害は AYA が FS より先行する状況に限られ稀ですが、長期的には upstream Firestorm への PR / root 分離 (`#Firestorm/` → `#AYAstorm/`) を検討
- **Firestorm 本家側の AO 削除は依然破壊的**: 推奨運用は AO 編集 / 削除を AYAstorm r31.2 以降に集約し、Firestorm 本家側からは「使う」だけにする (recovery guide に明記)
- **hidden 機能の UI フォールバック**: UI が機能不全になった場合、Debug Settings (`Ctrl+Alt+Shift+S`) で `FSAOHiddenSets` を空配列にすれば全 restore 可能。ただし Hidden 管理画面の「選択を削除」 (`Delete selected`) で明示的に完全削除した folder は復元できません

### Implementation summary

- `indra/newview/aoengine.cpp` / `aoengine.h` — `removeSet()` soft hide 化、`getHiddenSets()` / `unhideSet()` / `unhideAllSets()` / `isSetHidden()`、`update()` の hidden filter、hidden set の完全削除 / 同名衝突 helper
- `indra/newview/ao.cpp` / `ao.h` — `FloaterAOHiddenSets` controller + Manage hidden sets / Restore / Delete selected ボタン配線
- `indra/newview/llviewerfloaterreg.cpp` — `ao_hidden_sets` フロータ登録
- `indra/newview/fslslbridge.cpp` / `fslslbridge.h` — bridge version 比較 helper、起動時 attach の newer bridge 受け入れ、adopt path、handshake 後処理の共通化
- `indra/newview/app_settings/settings_per_account.xml` — `FSAOHiddenSets` (LLSD, Persist=1) 追加
- `indra/newview/skins/default/xui/{en,ja,zh}/notifications.xml` — `RemoveAOSet` 文言 + ボタンラベル書き換え、hidden set 衝突 / 完全削除確認通知
- `indra/newview/skins/default/xui/{en,ja,zh}/panel_ao.xml` — 「Manage hidden sets」ボタン、AO set soft-hide icon / tooltip 調整
- `indra/newview/skins/default/xui/{en,ja,zh}/floater_ao_hidden_sets.xml` — 新規フロータ (3 言語)
- `indra/newview/skins/default/xui/en/floater_ao.xml` — フロータ高さ調整
- `docs/specs/ayastorm-r31-2-ao-bridge-recovery.md` — 技術 spec (新規)
- `docs/guides/ao-data-recovery-guide.{en,ja,zh}.md` — ユーザー復旧手順 (新規 / 3 言語)

### Credits

- [@t-noami](https://github.com/t-noami) — r31-bugfix-2 の macOS ビルドに加え、AYAstorm 全体への継続的な実装貢献 (r24 Dullahan audio callback / r25 Ogg Vorbis codec / r26 3D Stream media ring / r27 macOS branding ほか)。
- [@mayatonton](https://github.com/mayatonton) — r31-bugfix-2 AO soft hide / LSL Bridge 衝突防御の実装、影響範囲調査、3 言語復旧手順 doc 整備。

### Documentation

- 技術 spec: [`docs/specs/ayastorm-r31-2-ao-bridge-recovery.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-ao-bridge-recovery.md)
- ユーザー復旧手順 (日本語): [`docs/guides/ao-data-recovery-guide.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
- ユーザー復旧手順 (English): [`docs/guides/ao-data-recovery-guide.en.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
- ユーザー復旧手順 (繁體中文): [`docs/guides/ao-data-recovery-guide.zh.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)

---

## すでに AO セットが消えてしまった方 — AO 再セットアップ手順

> [!IMPORTANT]
> 上に書いた事象によりすでに AO セットが消えてしまった方の **AO データそのものは viewer 側でも SL サーバ側でも取り戻せません**。ただし AO 機能自体は簡単な再セットアップで普通に使える状態に戻せます。**手順を 3 言語で公開していますので、ご自身の環境に合うものをご利用ください:**
>
> - 🇯🇵 [**日本語復旧手順**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
> - 🇺🇸 [**English Recovery Guide**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
> - 🇨🇳 [**繁體中文復原指南**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)
>
> r31-bugfix-2 を install していただくと **以降 AYAstorm 側では同じ事象は起きません**。Firestorm 本家 / 他 FS 派生 viewer での再発は各 viewer 側で patch される必要がありますが、その場合の回避策も復旧 guide 内に明記されています。

---

## 装着物 N-BL prim アルファ render-order — 3-pass dispatch (PR [#122](https://github.com/mayatonton/phoenix-firestorm/pull/122))

### 見出し: r31-bugfix-2 の 2 大修正項目その 2 — POST_WATER forward pass を SIM N-BL → R-BL → 装着物 N-BL の 3 段に分割し per-draw discriminator で振り分け

これは AO + Bridge 救済と **並ぶ 2 大修正項目** の 1 つです。r30 §5 の render-order swap (POST_WATER 全 non-rigged → 全 rigged) は rigged hair 越しの空抜けを解消しましたが、副作用として装着物 N-BL prim (まつ毛 prim 等) が rigged hair の前ではなく後に描かれて over-blend で潰される regression (spec 用語で S1 / S2) を出していました。POST_WATER の forward pass を以下 3 sub-pass に分割します:

- **pass 1**: `forwardRender(false, ATTACHMENT_NONE)` — SIM rezz N-BL のみ
- **pass 2**: `forwardRender(true)` — 全 R-BL (rigged hair 等)
- **pass 3**: `forwardRender(false, ATTACHMENT_ONLY)` — 装着物 N-BL prim のみ

per-draw discriminator は `LLDrawInfo::mAttachedToAvatar.notNull()`。pass 3 を rigged の後にずらすことで装着物 prim が hair の前面に整列し、同時に pass 1 で SIM 側 N-BL を rigged 前に描いて §5 swap fix (髪越し空抜け解消) は維持します。PRE_WATER は water fog 整合性のため rigged-first 維持。HUD は forwardRender 1 回のみで対象外。

falsified した代替案 (alpha plate の独立 depth など) と 3-pass 採用の構造的比較は `docs/specs/ayastorm-double-alpha-c-plan-extension.md §3` に記録。

### Implementation summary

- `indra/newview/lldrawpoolalpha.cpp` / `lldrawpoolalpha.h` — `AttachmentFilter` enum と `forwardRender(rigged, filter)` overload、POST_WATER の 3 sub-pass 分割、per-draw `LLDrawInfo::mAttachedToAvatar` discriminator
- `docs/specs/ayastorm-double-alpha-c-plan-extension.md` — falsified 案 A/B/C との構造的比較 (新規)
- `docs/specs/ayastorm-six-category-render-order-trace.md` — 3-pass 境界の根拠となった 6 カテゴリ render order trace 全文 (新規)

### Credits

- [@mayatonton](https://github.com/mayatonton) — 3-pass dispatch の設計・実装、falsification analysis (A/B/C 案)、spec 整備。

### Special thanks

- neria (neriamm) — アルファ render-order 問題の調査と検証協力。複数の SL avatar での再現環境構築と候補修正の hands-on 検証が、構造的比較と最終的な実装選択の絞り込みに大きく貢献しました。neria さんは Second Life のレジデント貢献者です (GitHub アカウントではありません)。

### Documentation

- 構造的比較 (拡張報告書): [`docs/specs/ayastorm-double-alpha-c-plan-extension.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-double-alpha-c-plan-extension.md)
- 6 カテゴリ render order trace: [`docs/specs/ayastorm-six-category-render-order-trace.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-six-category-render-order-trace.md)

---

## 3D Stream URL filter + UI 更新 (PR [#121](https://github.com/mayatonton/phoenix-firestorm/pull/121))

### 見出し: 3D Stream を初期 OFF に。URL 再生確認に送信元の object 名 / owner を表示

3D Stream 機能 (r26 で導入、r31 で unified tag として整理) を r31.2 以降は `Stream3DEnabled = false` 初期値で出荷します。すでに 3D Stream を有効で運用していたユーザーは persist 値を引き継ぎます。新規 install のみ初期 OFF からの開始です。in-world オブジェクトから stream URL が送られたときの再生確認 dialog には送信元の object 名 / owner を表示し、誰が送ってきた URL なのかを各 viewer が判断できるようにします。rezz されたオブジェクト経由の意図しない自動再生は UI で後追いブロックするのではなく source-of-truth の段階で落とします。

### 修正の仕組み

- `settings.xml` で `Stream3DEnabled` 初期値を `false` に。既存ユーザーの persist 値は保持
- URL 再生確認 dialog 文言に送信元の object 名 / owner を含める (3 言語)
- source-of-truth 強制: 3D Stream が disabled のとき、rezz されたオブジェクトからの URL emit は auto-play 経路に届く前に落とす (UI 後追いではない)

### Credits

- [@mayatonton](https://github.com/mayatonton) — 3D Stream URL filter + UI 更新の実装、3 言語プロンプトローカライズ。

### Documentation

- 技術報告 (日本語): [`docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md)
- ユーザーガイド (English): [`docs/specs/3dstream-user-guide.en.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.en.md)
- ユーザーガイド (日本語): [`docs/specs/3dstream-user-guide.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.ja.md)
- ユーザーガイド (繁體中文): [`docs/specs/3dstream-user-guide.zh.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.zh.md)

---

## Cinematic glow min-luminance bugfix (PR [#123](https://github.com/mayatonton/phoenix-firestorm/pull/123))

### 見出し: Cinematic mode `RenderGlowMinLuminance` を 0.0 → 0.5 に。既出荷ユーザーは one-shot migration で強制矯正

Cinematic mode で適用される BD parity overlay が `RenderGlowMinLuminance = 0.0` を持ち込み、HDR linear 空間の bloom-extract 閾値を下げていました。`glowExtractF.glsl` は bloom 寄与を `smoothstep(min, min+1.0, x)` で計算するため、`min = 0.0` だと HDR `0..1.0` の中間 lit でも発火し、`warmth = max(r*0.75, g*0.6, b*0.712)` 経路で色付きプリム (blank texture + color picker) が prim 側 Glow=0 設定でも光ってしまいます。装着物と SIM rez object の両方で症状が報告され、texture 適用プリムや白色プリムでは発火しないという観測でした。

閾値を `0.5` に引き上げます。Cinematic の強い bloom 表現 (空 / 強い反射 / 強い emissive — HDR `0.5` 以上) は引き続き発火し、装着物クラスの中間 lit による意図しない bloom だけを切ります。r31.0 / r31.1 で `0.0` を persist 焼きしてしまったユーザーは one-shot migration (sentinel `AYAR31GlowMinLuminanceMigrationVersion`) で強制矯正しますが、**次回 Cinematic mode 起動時に限ります** — Firestorm mode 専用ユーザーは影響なし (LL default `1.0` のまま、migration は skip し version も bump せず、次回 Cinematic 起動で再検査)。

### 修正の仕組み

- `settings_cinematic_bd.xml`: `RenderGlowMinLuminance` `0.0` → `0.5`
- `settings.xml`: `AYAR31GlowMinLuminanceMigrationVersion` sentinel (S32, Persist=1, default 0) 追加
- `llcinematicoverlay.{h,cpp}`: `applyR31GlowMinLuminanceMigrationIfNeeded()` 実装。`AYAVisualRealismEnabled != 2` (Firestorm mode) の場合は version を bump せず skip、次回 Cinematic 起動で再検査
- `llappviewer.cpp`: 起動 sequence の既存 `applyR15GodraysCinematicMigrationIfNeeded()` 呼出後に migration 呼出を追加

### Implementation summary

- `indra/newview/app_settings/settings_cinematic_bd.xml` — `RenderGlowMinLuminance` 閾値引き上げ
- `indra/newview/app_settings/settings.xml` — migration sentinel
- `indra/newview/llcinematicoverlay.cpp` / `llcinematicoverlay.h` — `applyR31GlowMinLuminanceMigrationIfNeeded()` (mode==2 ガード付き)
- `indra/newview/llappviewer.cpp` — 起動 sequence への組込み

### Credits

- [@mayatonton](https://github.com/mayatonton) — bloom 閾値の調査、修正設計、one-shot migration 実装。

---

## 水中アルファ plate redirect 修正 (PR [#124](https://github.com/mayatonton/phoenix-firestorm/pull/124))

### 見出し: `mAYAAlphaColor` redirect を `!sUnderWaterRender` で gate、水中は forward alpha を main RT へ直書き (FS 互換挙動)

r30 P5 transparent-DoF C-(a) で導入した `mAYAAlphaColor` redirect は、forward alpha BLEND を独立 alpha plate に書かせて tonemap 前に main RT に over-blend (`GL_ONE / GL_ONE_MINUS_SRC_ALPHA`) composite します。この redirect 有効化条件 (`use_alpha_rt`) が `LLPipeline::sUnderWaterRender` に対応していませんでした。水中時は main RT に underwater fog 着色済みの opaque scene があるのに、独立 plate は `(0,0,0,0)` で clear → forward alpha が plate に書き込み、pre-tonemap composite で plate が underwater 着色 main RT を上書きし、まつ毛 / 眉などの装着物アルファプリムと SIM particle の透過部分が **水中で真っ黒** で描画されていました。水上に出た後にも数フレーム再発する症状あり (カメラ上昇に伴い redirect が一旦水上状態に戻り、その後また水中状態に drift する)。

修正は単一条件 gate: `use_alpha_rt` に `!LLPipeline::sUnderWaterRender` を追加します。水中時は redirect を skip して forward alpha を main RT に直書き — 水中での upstream FS 互換挙動。水上時の振る舞いは旧と完全一致。

### 修正の仕組み

- `indra/newview/lldrawpoolalpha.cpp`: `use_alpha_rt` 条件の既存 `gPipeline.mAYAAlphaColor.isComplete()` 直前に `!LLPipeline::sUnderWaterRender &&` を追加
- gate が redirect を抑えると alpha plate は clear 済の `(0,0,0,0)` のまま。pre-tonemap composite は main RT に対して no-op (`A_plate * 1 + RT * 1 = RT`) となるため、水中専用 composite 経路は不要
- 水上時の transparent-DoF C-(a) plate composite 効果は変更なし。水中は全画面 fog で bokeh 構造的に不可視のため、水中で plate composite を落としても知覚的差はない

### Implementation summary

- `indra/newview/lldrawpoolalpha.cpp` — `use_alpha_rt` 条件に `!LLPipeline::sUnderWaterRender` を追加、inline コメントで水中時 no-op composite の根拠を明記

### Credits

- [@mayatonton](https://github.com/mayatonton) — 水中症状の調査、単一 gate 修正、5 connection point (`mForwardToAlphaRT` / alpha blend factors / emissive routing / plate clear / plate composite) の影響範囲トレース。
