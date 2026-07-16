# AYAstorm r42 — GUI にあり死んでいる cvar(r43 復元対象・9 件)

| | |
|---|---|
| 性格 | **この 9 件の発見のみが本資料の成果物**。9 件は個別に実コード精査済(①cvar 名の直接参照 ②動的名連結 ③bind された widget 名での読み、いずれも読み手なし)|
| 刈込 | 2026-07-16 AYA 決定: 初版にあった全 2501 cvar の生死表・shader 影響表は **単純 grep ベースで信用できないため削除**。cvar の生死は事前の全数表でなく **削除時にその経路を実トレースして個別判定**する(基準 = GUI 到達性・docs/vknative_architecture.md §5.1)。初版自身が 5 種類の grep 見落とし(動的名連結・数字始まり識別子・widget 名経由・UI framework 属性・XML コメント内)を記録していた = grep で追えないことの実証 |

**9 件は r30(`b2decf0578`・2026-05-23)時点で既に C++ 参照ゼロ** = upstream 由来の元々の no-op。r41/r42 の回帰ではない。復元は「元に戻す」でなく**読み手の新規実装**(r43・先送り台帳)。

## 環境設定(Preferences)内 — 4 件

| cvar | 画面 | タブ | 内側タブ | ウィジェット | Type |
|---|---|---|---|---|---|
| `FSCloseChatOnReturnOnlyForNearbyChatControl` | 環境設定 (Preferences) | **Chat** | **Typing** | check_box「Only for nearby chat bar」 | Boolean |
| `FSOpenInventoryAfterSnapshot` | 環境設定 (Preferences) | **Privacy** | **General** | check_box「Automatically show snapshots in inventory after upload」 | Boolean |
| `MediaEnablePopups` | 環境設定 (Preferences) | **Network & Files** | **Connection** | check_box「Enable media browser pop-ups」 | Boolean |
| `OverflowToastHeight` | 環境設定 (Preferences) | **User Interface** | **Toasts** | slider「Height of Overflow Toast:」 | S32 |

## メニュー内 — 5 件

| cvar | 画面 | メニュー階層 | 項目 | Type |
|---|---|---|---|---|
| `NearbyListShowMap` | People フローター | **Nearby > 歯車 View メニュー(`nearby_view_btn`)** | menu_item_check「View Map」 | Boolean |
| `FixedWeather` | メインメニュー | **Developer > World** | menu_item_check「Fixed Weather」 | Boolean |
| `SkyOverrideSimSunPosition` | メインメニュー | **Developer > World** | menu_item_check「Sim Sun Override」 | Boolean |
| `ShowTangentBasis` | メインメニュー | **Developer > Rendering** | menu_item_check「Tangent Basis」 | Boolean |
| `SaveMinidump` | メインメニュー | **Developer** | menu_item_check「Output Debug Minidump」 | Boolean |

※ 初版で 10 件目だった `RenderDebugPipeline`(唯一 LIVE だったもの)は決裁どおり GL 削除と同時に **撤去済**(commit `bf50a7f0b5`: gDebugPipeline + menu_viewer.xml + settings.xml + deploy 同期)= 本リストから除外。

### 補足

- 8 件は描画と無関係(chat / snapshot / media / toast / radar / weather)。ユーザーには効いているように見える**機能欠落**。
- `ShowTangentBasis` は GL 時代の描画デバッグだが r30 時点で既に読み手なし(GL 削除とは無関係の古い残骸)。
