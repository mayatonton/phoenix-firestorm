# AYAstorm r31-bugfix-2 — リリースアナウンス

> [!IMPORTANT]
> **r31-bugfix-2 は Firestorm 系 viewer 全体に存在する 2 件の構造的な振る舞いを AYAstorm 側で食い止めるリリースです。** 1000 人規模で観測されている AO セット消失事象の再発防止と、将来の LSL Bridge version drift による相互破壊への片方向防御。
>
> AYAstorm 固有の問題ではなく、Firestorm 派生 viewer 全体で共有される inventory root に起因する構造的な振る舞いへの対応です (バグか仕様かの判断は upstream にあります)。

実装詳細、影響範囲、復旧手順は `docs/specs/` および `docs/guides/` 配下に常設しています。本ノートは入口と差分ハイライトです。

---

## AYAstorm r31-bugfix-2 — AO 削除事象救済 + LSL Bridge 衝突防御

### 見出し: AO セット「削除」を非破壊 hide に置き換え、LSL Bridge 相互削除を片方向防御

Firestorm 系 viewer (Firestorm 本家 / 旧版 AYAstorm / その他 FS 派生) で AO ウィンドウの「削除」を押すと、その AO セットの Inventory 実体 (`#Firestorm/#AO` 配下のフォルダと配下の全アニメーション / notecard) が `purgeFolder` で永久消去されていました。`#Firestorm` root が Firestorm 派生 viewer 全体で共有されているため、ある viewer で削除すると、後で別の viewer (含 Firestorm 本家) でログインしても消えたままという cross-viewer 連鎖事故になっていました。

r31-bugfix-2 では viewer 側で実 inventory 操作を完全に止め、per-account 設定の隠しフラグで UI 上の非表示のみを行います。同時に LSL Bridge の version 不一致時の自動再作成ロジックを修正し、Firestorm 本家が将来 Bridge をマイナーバンプした際の AYAstorm 側 Bridge 消失を片方向で防御します。

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
- 削除 Dialog の文言を 3 言語で書き直し、ボタンを「Delete」→「Hide」に変更、inventory が残ることを明示

**LSL Bridge 片方向 fix**:
- 受信 version 文字列を数値 parse し、`major.minor` で大小比較
- 受信 > 自分 ⇒ **削除せず adopt** (`mBridgeUUID` / `mCurrentURL` のみ更新、`recreateBridge` 呼ばず)
- 受信 == 自分 ⇒ 既存挙動
- 受信 < 自分 ⇒ 既存挙動 (`recreateBridge` で更新)
- parse 失敗 ⇒ 既存挙動 (安全側)

### Migration note

- **ユーザー側の設定変更は不要です。** r31 install 済の方は r31-bugfix-2 を上書き install するだけで動作します
- r31 の全機能 (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP routing / macOS branding / other-rigged picker) はそのまま動作します
- r31-bugfix-1 の SSS pink-shadow 修正もそのまま継承します
- 既存被害ユーザーの inventory は **viewer 側で復旧不可能** です。AO 機能を再び使えるようにする手順は [`docs/guides/ao-data-recovery-guide.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md) に分離して常設しています

### Known limitations / future work

- **片方向防御のみ**: AYAstorm が Firestorm 本家より version 先行する場合、Firestorm 本家側 (未修正) は AYAstorm Bridge を引き続き削除します。実害は AYA が FS より先行する状況に限られ稀ですが、長期的には upstream Firestorm への PR / root 分離 (`#Firestorm/` → `#AYAstorm/`) を検討
- **Firestorm 本家側の AO 削除は依然破壊的**: 推奨運用は AO 編集 / 削除を AYAstorm r31.2 以降に集約し、Firestorm 本家側からは「使う」だけにする (recovery guide に明記)
- **hidden 機能の UI フォールバック**: UI が機能不全になった場合、Debug Settings (`Ctrl+Alt+Shift+S`) で `FSAOHiddenSets` を空配列にすれば全 restore 可能

### Implementation summary

- `indra/newview/aoengine.cpp` / `aoengine.h` — `removeSet()` soft hide 化、`getHiddenSets()` / `unhideSet()` / `unhideAllSets()` / `isSetHidden()` 新規、`update()` に hidden filter
- `indra/newview/ao.cpp` / `ao.h` — `FloaterAOHiddenSets` controller + Manage hidden sets ボタン配線
- `indra/newview/llviewerfloaterreg.cpp` — `ao_hidden_sets` フロータ登録
- `indra/newview/fslslbridge.cpp` — `parseBridgeVersionString()` helper 新規 + adopt path
- `indra/newview/app_settings/settings_per_account.xml` — `FSAOHiddenSets` (LLSD, Persist=1) 追加
- `indra/newview/skins/default/xui/{en,ja,zh}/notifications.xml` — `RemoveAOSet` 文言 + ボタンラベル書き換え
- `indra/newview/skins/default/xui/{en,ja,zh}/panel_ao.xml` — 「Manage hidden sets」ボタン
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
