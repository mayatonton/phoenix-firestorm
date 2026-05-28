# AYAstorm r31.2 PR #118 レビュー修正案・対応状況

**作成日**: 2026-05-29
**更新日**: 2026-05-29
**対象 PR**: <https://github.com/mayatonton/phoenix-firestorm/pull/118>
**対象 branch**: `fix/r31-2-ao-bridge-recovery`
**base branch**: `ayastorm-release`
**review fix branch**: `review/pr118-fixups`

この文書は PR #118 のレビューで見つかった懸念点、具体的な修正案、`review/pr118-fixups` での対応状況をまとめたものです。

PR の方向性自体は妥当です。AO セット削除を非破壊化すること、LSL Bridge の version drift で Firestorm 系 viewer 同士が bridge を作り直し合う状態を避けることは、どちらも必要な修正です。

一方で、現状の実装には「意図した安全策が一部の起動時経路に届かない」「soft-hide 後の同名 AO セットで UI 上の曖昧さが残る」といった境界条件があります。

## 現在の対応状況

| No. | 項目 | 状況 |
| --- | --- | --- |
| 1 | LSL Bridge: BridgeVer adopt 前に newer bridge が detach され得る | 対応済み |
| 2 | LSL Bridge: newer bridge adopt 経路だけ初回 handshake 後処理を飛ばす | 対応済み |
| 3 | AO soft-hide: 同名 AO セットの hide/restore が曖昧になる | 対応済み |
| 4 | hidden 管理画面からの完全削除 | 対応済み |
| 5 | UI localization: 日本語 panel_ao.xml の追加漏れ | 対応済み |
| 6 | UI polish: AO set hide button と hidden manager の配置調整 | 対応済み |

Hidden 管理画面からの完全削除は、通常の AO 削除を再び破壊的に戻すものではありません。Hidden 管理画面内の `Delete selected` から、確認 dialog を経た場合に限り、選択済み hidden set の実 inventory folder を完全削除します。このため、release note では「通常の Remove は実 inventory を削除しない」ことと、「Hidden 管理画面の Delete selected は明示的な完全削除である」ことを分けて説明する必要があります。

---

## 1. LSL Bridge: BridgeVer adopt 前に newer bridge が detach され得る

### 問題

PR では `FSLSLBridge::lslToViewer()` に次の安全経路が追加されています。

- bridge から受信した version が viewer 側の既知 version より新しい
- その bridge を採用する
- `recreateBridge()` を呼ばない

ただし、この経路は bridge から `<bridgeVer>` payload を受信した後にしか効きません。

起動時 attach 処理では、それより前に attachment を見て reject する可能性があります。

関連する処理:

- `FSLSLBridge::processAttach()`
  - `mpBridge == nullptr` の場合、inventory item 名が `mCurrentFullName` と完全一致しない attachment を wrong object として detach する
- `FSLSLBridge::detachOtherBridges()`
  - `mCurrentFullName` 以外の worn bridge item を detach する

発生しうる流れ:

1. AYAstorm が知っている bridge は `#Firestorm LSL Bridge v2.29`
2. Firestorm 本家が先に `#Firestorm LSL Bridge v2.30` を作っている
3. AYAstorm を起動する
4. 既存の v2.30 bridge が attachment として付いている
5. `<bridgeVer>2.30</bridgeVer>` が `lslToViewer()` に届く前に、attach 処理が v2.30 を wrong object と判断して detach する
6. newer bridge adopt 経路に到達しない

このため、現状のままだと「BridgeVer を受け取れたら recreate しない」は成立しますが、「macOS 起動時 attach が recreate なしで正常完了する」とは言い切れません。

### 期待する挙動

object 名が `FS_BRIDGE_NAME + version` 形式で、かつ version が viewer 側の既知 version 以上であれば、detach/recreate 判定より前に「受け入れ可能な既存 bridge」として扱うべきです。

### 修正案

bridge version 判定を共通 helper 化し、`lslToViewer()` だけでなく attach / detach 判定にも使います。

例:

```cpp
enum class BridgeVersionRelation
{
    Older,
    Same,
    Newer,
    Invalid
};

static bool parseBridgeObjectName(
    const std::string& object_name,
    S32& major,
    S32& minor);

static BridgeVersionRelation compareBridgeVersion(
    S32 major,
    S32 minor);

static bool isAcceptableExistingBridgeName(
    const std::string& object_name);
```

判定:

- same version: accept
- newer version: accept
- older version: 従来通り update/recreate 対象
- invalid name/version: 従来通り reject

適用先:

- `processAttach()`
  - `mpBridge == nullptr` のとき、`mCurrentFullName` 完全一致だけでなく same/newer bridge 名も受け入れる
- `detachOtherBridges()`
  - same/newer bridge candidate は detach しない
- `lslToViewer()`
  - 同じ比較 helper を使う

これにより、「古い bridge は更新するが、新しい bridge は尊重する」という狙いを起動時 attach 経路にも通せます。

### 対応状況

対応済みです。

実装内容:

- bridge version 比較を helper 化しました。
- `processAttach()` で same/newer bridge object 名を受け入れるようにしました。
- `detachOtherBridges()` で same/newer bridge candidate を detach しないようにしました。
- `startCreation()` では exact current bridge だけでなく、usable な same/newer bridge を探すようにしました。

---

## 2. LSL Bridge: newer bridge adopt 経路だけ初回 handshake 後処理を飛ばす

### 問題

通常の same-version 経路では、`URL Confirmed` を送った後に初回 setup として viewer 側の状態を bridge に送ります。

送信されるもの:

- `UseLSLFlightAssist`
- `UseMoveLock`
- `RelockMoveLockAfterMovement`
- `updateIntegrations()`
- `mIsFirstCallDone = true`

一方、newer bridge adopt 経路は `viewerToLSL("URL Confirmed")` の直後に `return true` します。

そのため、newer bridge を採用できても、通常 bridge と同じ初期状態が bridge に送られません。

### 修正案

version 判定後の handshake 完了処理を helper に切り出し、same-version 経路と newer-version 経路の両方から呼びます。

例:

```cpp
bool FSLSLBridge::completeBridgeHandshake(const std::string& url);
```

この helper に含める処理:

1. `mCurrentURL` を設定
2. `mpBridge` を確実に解決する
3. `URL Confirmed` を送る
4. 初回なら既存の first-call setup を実行する
5. `URL Confirmed` が実際に送れたかを返す

重要なのは、same-version と newer-version の bridge handshake が version 判定後に同じ処理へ合流することです。

### 対応状況

対応済みです。

実装内容:

- `confirmBridgeURLAndSendSettings()` を追加しました。
- same-version 経路と newer-version adopt 経路の両方から同じ helper を呼ぶようにしました。
- `URL Confirmed` 後の `UseLSLFlightAssist` / `UseMoveLock` / `RelockMoveLockAfterMovement` / `updateIntegrations()` が newer bridge adopt 経路でも実行されます。

---

## 3. AO soft-hide: 同名 AO セットの hide/restore が曖昧になる

### 問題

Second Life inventory 自体は UUID 管理なので、同名フォルダが複数あっても inventory 管理上は問題ありません。

ただし AYAstorm / Firestorm の AO 実装は、viewer 内部で AO set を名前で解決している箇所が複数あります。

関連箇所:

- `AOEngine::update()`
  - folder 名から `setName` を取り出す
  - `getSetByName(setName)` で既存 `AOSet` を探す
- `AOEngine::getSetByName()`
  - `set->getName() == name` で最初に一致した `AOSet` を返す
- `FloaterAO::updateList()`
  - combo box の表示 label を `currentSetName` として記憶する
  - `setName.compare(currentSetName) == 0` で選択を復元する
  - `selectSetByName(currentSetName)` を呼ぶ
- `FloaterAO::onSelectSet()`
  - combo label から `getSetByName()` で `AOSet` を引く
- `FSCurrentAOState`
  - `"CurrentSet"` として `mCurrentSet->getName()` を保存する

発生しうる流れ:

1. `My AO` を作る
2. `My AO` を Hide する
3. 一覧から `My AO` が消える
4. 同じ名前 `My AO` をもう一度作る
5. 2 個目の `My AO` も Hide する
6. Manage hidden sets には UUID 違いの `My AO` が 2 件出る
7. Restore all を押す
8. `FSAOHiddenSets` からは両方外れる
9. AO reload 時、どちらも `setName == "My AO"` になる
10. `getSetByName("My AO")` が先に見つけた `AOSet` を返す
11. UI 上は片方しか見えない、または選択対象が曖昧になる

これは inventory 破壊ではありません。viewer 側の AO UI / AOEngine 解決が曖昧になる問題です。

### 最小修正案: hidden set も含めて同名作成を禁止する

`AOEngine` に helper を追加します。

```cpp
bool AOEngine::hiddenSetNameExists(std::string_view name) const;
```

実装方針:

1. `FSAOHiddenSets` を読む
2. 各 UUID について inventory category を取得する
3. folder 名を `:` で split し、先頭を AO set name として扱う
4. requested name と比較する

使用箇所:

- `FloaterAO::newSetCallback()`
- `AOEngine::importNotecard()`

hidden set に同名がある場合は、新規作成 / import を止めて notification を出します。

文言例:

> 同じ名前の AO セットが現在非表示になっています。
> 同じ名前で新規作成する前に、非表示セットを再表示するか完全削除してください。

これにより、AOEngine 全体を UUID-first に書き換えずに、最も起きやすい同名衝突を防げます。

### restore 側の安全策

Restore 操作でも visible set との名前衝突を確認します。

`Restore selected`:

- hidden set と同名の visible set が存在する場合、unhide しない
- warning を表示する

`Restore all`:

- 衝突しないものだけ restore する
- 衝突したものは hidden のまま残す
- 何件 restore し、何件 skip したか通知する

これにより、「Restore all を押したのに片方が UI に出ない」という誤解を避けられます。

### 対応状況

対応済みです。

実装内容:

- `AOEngine::hiddenSetNameExists()` を追加しました。
- 新規 AO set 作成時に hidden set との同名衝突を拒否します。
- notecard import 時に hidden set との同名衝突を拒否します。
- `Restore selected` は visible set と同名なら restore しません。
- `Restore all` は衝突しないものだけ restore し、衝突した hidden set は hidden のまま残します。
- 衝突時の通知を `en` / `ja` / `zh` に追加しました。

### 長期修正案

AO set identity を UUID-first に寄せます。

必要になる変更:

- combo box の value に inventory UUID を持たせる
- current AO set 保存に UUID と表示名を持たせる
- `getSetByName()` を選択解決の主経路にしない
- 同名表示を許容しつつ UI で区別できるようにする

これは根本解決ですが、影響範囲が広いため r31-bugfix-2 の緊急修正には重いです。

---

## 4. 任意追加: hidden 管理画面からの完全削除

### 背景

AO floater の通常削除を soft-hide に変えるのは正しいです。

一方で、ユーザーが本当に不要な AO folder を整理したい場合、完全削除の導線がどこにもないと不便です。

完全削除を入れるなら、通常の AO floater ではなく hidden 管理画面に限定するのが安全です。

### UI 案

Hidden sets floater に以下のボタンを置きます。

- `Restore selected`
- `Restore all`
- `Delete selected`

通常 AO floater には permanent delete を置きません。

### 確認 dialog の要件

完全削除時の確認 dialog では、必ず次を明記します。

- `#Firestorm/#AO` 配下の実 inventory folder を削除する
- Firestorm 本家や他の Firestorm 派生 viewer からも消える
- AYAstorm からは元に戻せない
- backup / notecard がない場合、AO data は復元できない

ボタン文言例:

- confirm: `Delete selected`
- cancel: `キャンセル`

### 実装案

hidden list で選択された UUID に対して、既存の破壊的削除処理をこの明示的な flow からのみ呼びます。

処理:

1. 選択 UUID が `FSAOHiddenSets` に含まれることを確認する
2. その UUID が AO folder 配下の category であることを確認する
3. `purgeFolder(uuid)` を呼ぶ
4. `FSAOHiddenSets` から UUID を外す
5. AO reload
6. hidden manager を refresh

これにより、soft-hide の安全性を保ちながら、意図的な inventory 整理手段を提供できます。

### 対応状況

対応済みです。

実装内容:

- Hidden sets floater に `Delete selected` ボタンを追加しました。
- 選択行がない場合は何もしません。
- 削除前に確認 dialog を表示します。
- 削除対象は `FSAOHiddenSets` に含まれる hidden set の UUID に限定します。
- 削除対象が `#Firestorm/#AO` 直下の category であることを確認します。
- `#AO` 自体、未初期化の AO folder、`#AO` 外の category は拒否します。
- 削除後は `FSAOHiddenSets` から UUID を外し、AO reload と hidden list refresh を行います。

注意:

この機能は `purgeFolder(uuid)` を使うため、実 inventory folder を削除します。PR 本来の「通常の AO 削除は実 inventory を触らない」という目的とは別の、明示的な完全削除機能として説明する必要があります。

---

## 5. UI localization: 日本語 panel_ao.xml の追加漏れ

### 問題

release note では `xui/{en,ja,zh}/panel_ao.xml` に Manage hidden sets ボタンを追加したと書かれています。

実際の PR 差分では、英語と中国語には `ao_manage_hidden` が追加されていますが、日本語 `panel_ao.xml` には追加されていません。

XUI fallback によりボタン自体は表示される可能性がありますが、日本語 UI でも label / tooltip が英語になる可能性があります。

### 修正案

`xui/ja/panel_ao.xml` に `ao_manage_hidden` の override を追加します。

例:

```xml
<button
    name="ao_manage_hidden"
    label="非表示セットの管理"
    tool_tip="一覧から非表示にした AO セットを表示し、ここから再表示できます。" />
```

ユーザー向け guide で日本語ボタン名を書く場合は、そちらも合わせて更新します。

### 対応状況

対応済みです。

`xui/ja/panel_ao.xml` に `ao_manage_hidden` の override を追加しました。

```xml
<button
    name="ao_manage_hidden"
    label="非表示セットの管理"
    tool_tip="一覧から非表示にしたアニメーションセットを表示します。ここから再表示できます。" />
```

---

## 6. UI polish: AO set hide button と hidden manager の配置調整

### 問題

AO floater 上の AO set 削除ボタンは、PR #118 では実 inventory 削除ではなく soft-hide になっています。

ただし UI 上の icon が trash のままだと、ユーザーには「実削除される操作」に見えます。PR の狙いである「通常の AO 削除は inventory を触らず非表示にする」と表示の意味がずれます。

また、Hidden AO Sets floater では `Delete selected` が restore 系ボタンより上にあると、復元よりも完全削除が主操作に見えます。完全削除は hidden manager 内の明示的な補助操作なので、restore 系操作を先に見せる配置のほうが安全です。

さらに、restore 系ボタンと `Delete selected` の幅が不揃いだったり、横リサイズ時に `Restore all` が右端へはみ出したりすると、hidden manager 全体が不安定な UI に見えます。最小ウィンドウ幅でも収まる固定幅にする必要があります。

### 対応状況

対応済みです。

実装内容:

- AO set の soft-hide ボタン icon を `TrashItem_Press` から既存の `Profile_Group_Visibility_Off` に変更しました。
- AO set soft-hide の tooltip を `en` / `ja` / `zh` で「削除」ではなく「一覧から非表示」に合わせました。
- Hidden AO Sets floater では `Restore selected` / `Restore all` を上段、`Delete selected` を下段に配置しました。
- `Restore selected` / `Restore all` / `Delete selected` は 145px 固定幅に統一しました。
- `min_width=320` では `10 + 145 + 10 + 145 + 10 = 320` で収まるため、横幅を最小まで縮めても `Restore all` が右端へはみ出さないようにしました。
- 個別 animation を state list から外す `ao_trash` は別操作なので trash icon のままにしています。

---

## ビルド確認

2026-05-29 に Mac 版 app の差分ビルドを実施済みです。

実施内容:

- `xcodebuild -scheme ayastorm-bin -configuration Release ... build`
- app-only build。DMG は作成していません。
- 最終ビルドには hidden manager の 145px 固定幅ボタン調整を含みます。
- 成果物: `build-darwin-universal/newview/Release/AYAstorm.app`

確認結果:

- `git diff --check`: 成功
- `xcodebuild`: `** BUILD SUCCEEDED **`
- `codesign --verify --deep --strict --verbose=2 build-darwin-universal/newview/Release/AYAstorm.app`: 成功
- app bundle 内の XUI に `Profile_Group_Visibility_Off`、`Delete selected` の下段配置、hidden manager の 145px 固定幅ボタン、各 locale tooltip が反映済み

---

## 残タスク

1. Hidden 管理画面からの完全削除を追加したため、release note の「実 inventory は触らない」という説明を、通常 Remove と hidden manager の明示的削除に分けて更新する
