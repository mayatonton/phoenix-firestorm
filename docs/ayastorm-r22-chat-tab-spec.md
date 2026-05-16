# AYAstorm r22 — Chat tab split (Human vs System & Object) 仕様

**作成日**: 2026-05-15
**最終更新**: 2026-05-16 (M8 実装 + 3 OS 受入完了、r23 同梱リリース確定)
**ステータス**: M1〜M8 完了 (3 OS 受入 PASS)。**単独タグは発行せず、r23 リリースに同梱配信予定**
**対象ブランチ**: `feat/ayastorm-r22-chat-tab-split`

このドキュメントは r22 で追加予定の「Chat 表示の人間/Object タブ分離」機能の仕様。AYA さんとの対話で確定した内容を集約。M1 セッション (2026-05-15) で主要論点をすべて確定、2026-05-16 に spec change (System & Object 再グループ化 + フレンド online 例外 cvar) を追加。

---

## 1. 狙い

Nearby Chat / IM の表示において、**人間アバター発言** と **System / Object 系通知 (LSL 由来 + システム通知 + TP / Region メッセージ)** を別タブに分離する。

人間同士の会話に LSL からの警告 / 広告 / HUD 通知 / TP / Region 通知が混入して「会話の流れが途切れる」状況を解消するのが主目的。これらの通知を完全に消すのではなく、別タブに退避させて見落としは防ぐ。

ただし「フレンドがオンラインに来た」通知だけは "会話したい相手の登場" 性質が強いため、cvar 制御で Human タブにも複製可能とする (System & Object タブには常に表示)。

---

## 2. スコープ

| 項目 | 対象 / 対象外 |
|---|---|
| Nearby Chat (Local Chat) | ✅ 対象 |
| IM (1 on 1) — 通常 | ✅ 対象 (受信側で判定) |
| IM (1 on 1) — `IM_FROM_TASK` (Object 発信) | ✅ Object タブ |
| Group chat | ✅ 基本 Human (Object 発言が来ない経路) |
| 対象 style: FS V1 / V7 / LL | ✅ **3 style 同時対応** (M1 確定) |

---

## 3. 表示構造 (GUI)

### 3.1 確定: タブ切替式 + Preferences スイッチ

各 chat ウィンドウの history 領域上部に **タブ** を配置する。

```
┌──────────────────────────────────────┐
│ [Human] [System & Object (3)]        │ ← 非アクティブ側に未読件数バッジ
├──────────────────────────────────────┤
│  (選択中タブの history)              │
├──────────────────────────────────────┤
│  > 入力欄                            │ ← 共有 1 つ
└──────────────────────────────────────┘
```

**仕様**:
- タブは `[Human]` `[System & Object]` の 2 つ
- 非アクティブ側に **未読件数バッジ** を表示し見落としを防ぐ
  - 形式は `[System & Object (3)]` の件数表示 (M5 で確定)
  - タブ切替で 0 にリセットしラベルを `System & Object` に戻す
  - **セッション内のみ。再起動でリセット** (履歴ファイルには残るので件数自体は意味薄、と判断)
- 入力欄は 1 つ共有 (どちらのタブを見ていても Local Chat に送信)
- 自分の Local Chat 発言は **Human タブに記録**
- V1 / V7 / LL の 3 style とも同じ形状で揃える (UX 一貫性)

**ラベル順序** (spec change 2026-05-16): `System & Object` の順に並べる。`Object & System` ではない (AYA さん談: "Object が偉そうだから")。

**設定スイッチ** (M1 確定 + M8 spec change で追加):

| Name | Type | Default | Persist | 関係 |
|---|---|---|---|---|
| `FSChatHumanObjectTabs` | Boolean | `true` | 1 | master switch |
| `FSFriendOnlineToHumanTab` | Boolean | `true` | 1 | 子 (master ON 時のみ意味あり) |

- `FSChatHumanObjectTabs` (master): **opt-out (default ON)** — AYAstorm の目玉機能として default で有効
  - 配置: `Preferences → Chat → Chat Windows` タブ (LL chat style 切替と同じタブ)
  - ラベル: `Split chat into Human / System & Object tabs`
  - tool_tip: `Separate human avatar speech from system/script messages into two tabs. Keeps human conversations free of notifications.`
  - `false` にすると旧 1-widget 挙動に戻る (escape hatch 兼用、別 cvar 不要)
- `FSFriendOnlineToHumanTab` (子、M8 spec change で追加): **default ON**
  - 配置: `Preferences → Chat → Chat Windows`、master の直下にインデント
  - ラベル: `Show friend online/offline in Human tab`
  - tool_tip: `Also show friend online/offline notifications in the Human tab. They always appear in the System & Object tab regardless.`
  - master OFF 時は UI 上グレーアウト (enabled_control で連動)
  - フレンド online/offline は **System & Object には cvar 値に関わらず常に出力**、Human への複製可否だけがこの cvar で決まる

### 3.2 見た目フォールバック案 (実機検証で α が詰まった時の二の矢)

実機で「タブが見た目的に変」と判明したら、以下の順に検討:

| Plan | 場所 | 形 | 採用しやすさ |
|---|---|---|---|
| **A タブ (本命)** | history 上部 | `[Human] [Object (3)]` 横並び | — |
| **B-1 ツールバートグル** | 既存 Muted ボタンの隣 | `[H/O]` トグル | ◎ (既存 Muted パターン踏襲) |
| B-2 右上ミニタブ | ヘッダ右端 | 小さく `[Human] [Object]` | ○ |
| B-3 combo_box | ツールバー | `[Show: Human ▼]` ドロップダウン | △ (ステップ数増) |

→ **B-1 が最有力フォールバック**。既存 `chat_history` / `chat_history_muted` の visibility_control 切替パターン (§5.3 参照) と完全一致するため、実装/見た目とも違和感が出にくい。

### 3.3 必須要件: 表示色維持 (タブ切替で文字色を変えない)

Firestorm の `fs_chat_history` widget は行ごとに色が固定される:

| 行の出所 | 色 |
|---|---|
| 起動時に history load した過去ログ | `ChatHistoryTextColorPersisted` (グレー) |
| session 中に受信した発言 | `ChatHistoryTextColor` (通常) |

色は **append された時点で決まる** ので、後から変わらない。問題が起きるのは「タブ切替時に widget を reload する」粗い実装をした場合のみ。

→ **必須実装方針**:

- Human / Object の `fs_chat_history` widget を **最初から両方常駐** させる
- 起動時の history load は両 widget に同時振り分け (どちらも persisted color = グレー)
- chat 受信 hook で source_type を見て対応 widget だけに append (通常色)
- **タブ切替は visible/invisible swap のみ** — widget 内容は一切触らない

これで「タブ往復で色がグレーになる」事故を構造的に回避できる。既存の `chat_history` / `chat_history_muted` パターンと同じ理屈。

---

## 4. 判定境界 (M1 確定)

`EChatSourceType` (llchat.h:35) の 6 値を以下に振り分ける (spec change 2026-05-16 で System/TP/Region を System & Object 側へ移動):

| Source Type | 例 | 振り分け |
|---|---|---|
| `CHAT_SOURCE_AGENT` | アバター発言 | **Human** |
| `CHAT_SOURCE_OBJECT` | LSL `llSay` / `llOwnerSay` / HUD / 自分の attachment | **System & Object** |
| `CHAT_SOURCE_SYSTEM` | "You are now logged in", item received, friend online 等 | **System & Object** (※フレンド online は例外、下記参照) |
| `CHAT_SOURCE_TELEPORT` | TP offer / arrival | **System & Object** |
| `CHAT_SOURCE_REGION` | sim restart 通知等 | **System & Object** |
| `CHAT_SOURCE_UNKNOWN` | 不明 (fallback) | **Human** |

**判定方針**:
- System & Object タブには **「会話以外の通知すべて (LSL 発言・システム通知・TP・Region)」** を集める
- 旧仕様 (r22 初版〜M7) では System / Teleport / Region は Human に流していたが、「TP したときのログが Human タブに表示される」のは会話の流れを切る — の AYA 判断で System & Object 側へ移動 (spec change 2026-05-16)
- `CHAT_SOURCE_UNKNOWN` だけは **Human フォールバック** を維持: 判別不能 = 人間性のヒントがゼロなので「会話を見落とすほうがダメージ大」と判断、default safe を保つ

**フレンド online/offline 例外**:
- `llcallingcard.cpp:891-909` あたりで発火する friend online/offline 通知は `mSourceType = CHAT_SOURCE_SYSTEM` だが、「会話したい相手の登場」として Human タブにも届けたい性質がある
- 実装: chat 構築時に `LLChat::mFriendOnlineNotification` (新フラグ、M8 で追加) を立てる
- ルーティング層でこのフラグが立っていれば:
  - **System & Object タブには `FSFriendOnlineToHumanTab` の値に関わらず常に出力**
  - **Human タブには `FSFriendOnlineToHumanTab=true` (デフォルト) のときのみ複製出力**
- 「フレンドオンライン通知」は SL の `OnlineOfflinetoNearbyChat` cvar (デフォルト OFF、Nearby Chat に表示するか) とは独立判定: AYAstorm の Chat tab split が ON のときは、上記フラグ経由で常に Nearby Chat に届く扱い
- 履歴ファイルの suffix marker は `<!--src:system-->` のまま (Human 複製の判定は live ルーティング時のフラグであり、起動時の history load 後はフラグ復元不可なので System & Object タブにだけ載る。これは仕様として許容: 過去のフレンドオンライン履歴を Human で見直したい需要は薄い)

---

## 5. データ層

### 5.1 履歴ファイルは 1 本のまま維持

| 種類 | ファイル | 方針 |
|---|---|---|
| Nearby Chat 履歴 | `chat-LOCAL-CHAT.txt` | **1 本のまま維持**、物理分離しない |
| IM 履歴 | `chat-{user_uuid}.txt` | **1 本のまま維持**、物理分離しない |

理由:
- 既存利用者の履歴アーカイブと外部 viewer / grep の連続性を壊さない
- Object タブを後から消したり Human と統合する自由度が残る (ファイル構造を一切変えない)
- 履歴ファイル数が倍増しない
- 上流 Firestorm が同じファイルを開いても破綻しない

### 5.2 行末メタマーカー

新規書き込み行の末尾に source_type を埋め込む。

**フォーマット例**:

```
[2026-05-15 10:23] Alice: hello\t<!--src:avatar-->
[2026-05-15 10:24] PetBox: greetings, traveler!\t<!--src:task-->
[2026-05-15 10:25] Second Life: You are now logged in.\t<!--src:system-->
```

**source_type の値** (`EChatSourceType` 既存定義からマッピング):

| Marker 値 | 対応する EChatSourceType |
|---|---|
| `avatar` | `CHAT_SOURCE_AGENT` |
| `task` | `CHAT_SOURCE_OBJECT` |
| `system` | `CHAT_SOURCE_SYSTEM` |
| `teleport` | `CHAT_SOURCE_TELEPORT` |
| `region` | `CHAT_SOURCE_REGION` |
| `unknown` | `CHAT_SOURCE_UNKNOWN` |

→ ファイル上にすべての種類を残しておくことで、将来 Human/Object 以外の細分化 (例: System タブを別に分ける) に対応できる柔軟性を確保。

**HTML コメント風 (`<!-- -->`) にした理由**:
- 外部 viewer / エディタで直接開いても **意味のあるコメント記法に見え害が薄い** (`[TASK]` プレフィックスより目立たない)
- `grep "src:task"` のような検索 noise になりにくい
- **上流 Firestorm で同じ履歴ファイルを開いても破綻しない** (ただの末尾文字列として見える)

### 5.3 読み込み時の挙動

`LLChatLogParser::parse()` が行を読む際:

1. 行末の `<!--src:(\w+)-->` を **正規表現で抽出して LLSD に `source` キーとして詰める**
2. 本文文字列からはマーカーを **剥がしてから** 後続パイプラインに渡す
3. メタに従って Human / Object widget に振り分け

→ **AYAstorm を介する限り、ユーザー画面にマーカー文字列は絶対出ない**。

### 5.4 旧履歴 (マーカーなし) のフォールバック

マーカーがない行は判別不能 → **Human タブにフォールバック**。

理由:
- 過去ログは大半が人間会話なので Human 側に流すのが安全
- Object 側に巻き込むと検索性が悪化する
- 「判別つかないものは Human」が default safe

---

## 6. 実装スケッチ

### 6.1 影響ファイル

**XUI** (M3 着手時に構造再調査した実態を反映、M8 spec change で追加分あり):

- `indra/newview/skins/default/xui/en/floater_fs_nearby_chat.xml` — FS V1/V7 用、`tab_container` で `tab_human` / `tab_object` を抱える
- `indra/newview/skins/default/xui/en/floater_im_session.xml` — LL style 用 (Nearby Chat と 1:1 IM の両方が同じ XUI を共有)、同じく `tab_container` を持つ
- `indra/newview/skins/default/xui/{en,ja,zh}/floater_fs_nearby_chat.xml` / `floater_im_session.xml` — タブラベル `Object` → `System & Object` に変更 (3 言語、M8 spec change)
- `indra/newview/skins/default/xui/{en,ja,zh}/panel_preferences_chat.xml` — `FSChatHumanObjectTabs` スイッチ + `FSFriendOnlineToHumanTab` 子チェックボックス (M8 spec change で追加) + AYAChatWindowStyle の (requires restart) ラベル
- `indra/newview/skins/default/xui/{en,ja,zh}/notifications.xml` — `ChangeChatLayoutSetting` モーダル (M4-extra)
- ~~`panel_nearby_chat.xml`~~ — **dead/未参照と判明** (M3 着手時に確認)、touch しない

**C++** (実装後の実態、M8 spec change で追加分あり):

- `indra/llui/llchat.h` — `LLChat::mSourceType` (既存 `EChatSourceType`) をそのまま利用 + `LLChat::mFriendOnlineNotification` フラグを追加 (M8 spec change)
- `indra/newview/lllogchat.cpp`:
  - `LLChatLogFormatter::format()` — suffix marker 出力
  - `LLChatLogParser::parse()` — suffix marker 抽出 + 本文剥がし。マーカーがある行だけ `LL_IM_SOURCE_TYPE` を立てる (legacy 行は未設定のまま、load 側の heuristic に委ねる)
- `indra/newview/llcallingcard.cpp` — friend online/offline 通知 chat 構築時に `LLChat::mFriendOnlineNotification = true` を立てる (M8 spec change、§4 「フレンド online/offline 例外」参照)
- `indra/newview/fsfloaternearbychat.cpp` / `.h` — FS 側の addMessage で source_type 振り分け (System/TP/Region → System & Object、spec change) + フレンドオンライン例外パス + 未読バッジ更新
- `indra/newview/llfloaterimsessiontab.cpp` / `.h` — LL Nearby Chat と 1:1 IM の基底クラス、appendMessage で同じく振り分け + バッジ更新 (System & Object 再グループ化、フレンド例外、M8)
- `indra/newview/llfloaterimnearbychat.cpp` — load path で `LL_IM_SOURCE_TYPE` を尊重
- `indra/llui/lltabcontainer.h` — `setTabsHidden()` を public へ昇格 (`FSChatHumanObjectTabs=false` 時のタブ strip 抑止に使用)
- `indra/newview/llviewercontrol.cpp` — `AYAChatWindowStyle` / `FSChatHumanObjectTabs` 切替時の `ChangeChatLayoutSetting` モーダル発火 + 旧スタイルの IM コンテナ自動クローズ + `floater_vis_*` クリア (M4-extra)
- `indra/newview/llagent.cpp` — TP セパレーター送出時に `LLFloaterIMNearbyChat` (LL style) と `FSFloaterNearbyChat` (FS V1/V7) の両方に dispatch (M8 spec change、上流 FS で commented out されていた LL style 側を `findTypedInstance` でガードしつつ復活させ、3 style 一貫性を確保)
- `indra/newview/llnotificationhandlerutil.cpp` — `LLHandlerUtil::logToNearbyChat` で notification tip (`ChatSystemMessageTip` 等、SLURL からのテレポート完了 / simulator version 差異など) を FS / LL 両方の nearby chat に dispatch (M8 spec change、同じく Ansariel コメントアウトを復活)

**Settings** (M8 spec change で追加分あり):

- `indra/newview/app_settings/settings.xml`:
  - `FSChatHumanObjectTabs` Boolean default true persist 1
  - `FSFriendOnlineToHumanTab` Boolean default true persist 1 (M8 spec change)

### 6.2 既存の参考パターン (viewer 内)

- **`chat_history` / `chat_history_muted` 2 widget + `visibility_control` 切替** — `floater_fs_nearby_chat.xml` に既存。「複数の chat_history を切り替える」設計の前例
- **`tab_container` widget** — viewer 内で 20+ ファイル使用 (Preferences の Chat/Graphics/Privacy 等)。熟成済みなので技術的リスクは低い
- **`EChatSourceType`** — `indra/llui/llchat.h:35` で既に 6 値定義済み、15+ファイルで使用 (`fschathistory.cpp` / `llimprocessing.cpp` / `llviewermessage.cpp` 等)

### 6.3 構造調査メモ (M3 で確定)

XUI / C++ を覗いた結果:

- `panel_nearby_chat.xml` は **dead/未参照** だった (上流ですでに切り離されていた)。本 r22 では touch しない
- `floater_fs_nearby_chat.xml` は既に 2 つの chat_history を切り替える設計を採用済み、ここに `tab_container` を被せて Human/Object パネルに分割
- LL style の Nearby Chat と 1:1 IM はどちらも `floater_im_session.xml` を共有してロードされる (Nearby Chat も IM session の一種扱い) ため、ここに `tab_container` を入れることで **M4 単発で M6 (IM 1:1 対応) も実質カバーされる**
- `EChatSourceType` が既に LLChat に存在しており、source_type の判定ロジックは新規実装不要
- `LLLogChat::saveHistory()` のシグネチャに source_type が無いので拡張要

---

## 7. 受入基準 (実装後)

**初版 (M1〜M7) で完了済み**:

- [x] Human タブに人間アバター発言、Object タブに LSL/Object 発言が分離して表示される (※タブラベルは M8 で `System & Object` に rename 予定)
- [x] 旧履歴 (マーカーなし) は Human タブにフォールバック
- [x] タブ切替で session 中の発言色が維持される (グレーにならない)
- [x] 起動時の history load は両タブともグレー (persisted color) で表示
- [x] 非アクティブタブに未読件数バッジ `(N)` が表示される (セッション内のみ、再起動でリセット)
- [x] 自分の Local Chat 発言は Human タブに記録される
- [x] AYAstorm を介してユーザー画面にマーカー文字列が漏れない
- [x] 上流 Firestorm で同じ履歴ファイルを開いても破綻しない (末尾文字列として見える)
- [x] FS V1 / V7 / LL の 3 style すべてでタブが機能する (Linux / Win / Mac 全 OS で確認)
- [x] IM (1 on 1) の `IM_FROM_TASK` 由来発言が System & Object タブに分離される (M4 で共有 XUI 経由実装、M6 で確認)
- [x] `Preferences → Chat → Chat Windows` に `FSChatHumanObjectTabs` スイッチが表示される
- [x] `FSChatHumanObjectTabs=false` で旧 1-widget 挙動に戻る (escape hatch)
- [x] AYAChatWindowStyle / FSChatHumanObjectTabs 切替時に再起動誘導モーダルが出る (M4-extra)
- [x] AYAChatWindowStyle 切替時に旧スタイルの IM コンテナが自動で閉じる (M4-extra)
- [x] 3 OS (Linux / Win / Mac) でビルド通過 + 動作確認 (M7)

**M8 spec change で追加** (全て 3 OS で PASS 確認済):

- [x] タブラベルが 3 言語 (en/ja/zh) すべてで `System & Object` (順序固定、`Object & System` ではない) に表示される
- [x] System / Teleport / Region 発言は **System & Object** タブに流れる (spec change 2026-05-16、旧仕様では Human タブだった)
- [x] `CHAT_SOURCE_UNKNOWN` は Human タブにフォールバック (default safe、変更なし)
- [x] フレンド online/offline 通知は System & Object タブには `FSFriendOnlineToHumanTab` の値に関わらず**常に**出力される
- [x] `FSFriendOnlineToHumanTab=true` (デフォルト) のとき、フレンド online/offline 通知は Human タブにも複製出力される
- [x] `FSFriendOnlineToHumanTab=false` で Human への複製が止まる (System & Object には残る)
- [x] `Preferences → Chat → Chat Windows` に `FSFriendOnlineToHumanTab` チェックボックスが master のインデント子要素として表示される
- [x] `FSChatHumanObjectTabs=false` のとき、`FSFriendOnlineToHumanTab` チェックボックスがグレーアウトする (enabled_control 連動)
- [x] TP 着地時のセパレーター (`CHAT_SOURCE_TELEPORT` + `CHAT_STYLE_TELEPORT_SEP`) が **FS V1 / V7 / LL の 3 style すべてで** System & Object タブに表示される (M8 で `llagent.cpp` の commented-out LL 経路を復活、上流 FS の latent bug 修正を同梱)
- [x] TP 着地時にも非アクティブタブの未読バッジが bump される (history replay と区別するため `args["is_replay"]` 専用フラグを導入、`do_not_log` は履歴抑止のみの意味に戻す)
- [x] LL style でも TP 完了時の SLURL 通知 (`FSShowBackSLURL` で出る "secondlife://... からのテレポートが完了しました") と simulator version 差異通知が System & Object タブに表示される (`llnotificationhandlerutil.cpp` の `logToNearbyChat` で LL 経路を復活)
- [x] M8 後に 3 OS (Linux / Win / Mac) で再ビルド + 動作確認 PASS

---

## 8. 残る未確定論点

| 論点 | 優先 | 対処タイミング | 状況 |
|---|---|---|---|
| バッジ詳細 (件数表示 vs ドット) | 低 | M5 実装中に決める | ✅ 件数表示 `(N)` で確定、セッション内のみ |
| `floater_im_container.xml` の panel_nearby_chat 埋め込み確認 | 中 | M2 着手前に最終確認 | ✅ `panel_nearby_chat.xml` は dead と判明 (M3) |
| HUD allow list (自分の HUD だけ Human に流す) | 低 | **r22 スコープ外**、r23+ で検討 | 据え置き |

`FSChatHumanObjectTabs=false` で 1-widget 旧挙動に戻れるため、別の escape hatch cvar は **不要**。

---

## 9. リスク登録

| リスク | 影響 | 対策 |
|---|---|---|
| タブの見た目が実機で詰む | 中 | フォールバック B-1〜B-3 を §3.2 に明記済み |
| 履歴ファイルの行末マーカー regex が既存パース処理 (URL 抽出等) と衝突 | 低 | 実装時に URL/highlight regex と末尾マーカー regex の優先順位を確認 |
| LL style の `floater_im_container.xml` / `floater_im_session.xml` の Nearby Chat 統合構造が想定と違う | 中 | M2 着手前に再確認、想定外なら影響範囲を再評価 |
| マーカー付き履歴を上流 Firestorm で開いた時の挙動 | 低 | 末尾文字列なので表示には漏れる可能性あり (パースされないので無害)、§5.2 参照 |
| `LLChatLogFormatter` / `LLChatLogParser` の上流互換性 | 中 | suffix marker は optional、なくても既存 parse が壊れない設計を保つ |

---

## 10. マイルストーン

| M | 内容 | ステータス |
|---|---|---|
| M1 | 仕様確定 + 構造調査 | ✅ 完了 (2026-05-15) |
| M2 | データ層実装 (LLLogChat + LLChat suffix marker) | ✅ 完了 |
| M3 | XUI — `floater_fs_nearby_chat.xml` / `floater_im_session.xml` に `tab_container` | ✅ 完了 (panel_nearby_chat.xml は dead/未参照と判明、touch しない方針に変更) |
| M4 | C++ 受信 hook で widget 振り分け + M4-extra (restart-only モーダル + 旧スタイル floater 自動クローズ) | ✅ 完了 |
| M5 | 未読バッジ (`(N)` 件数表示、セッション内のみ、再起動でリセット) | ✅ 完了 |
| M6 | IM (1 on 1) — `floater_im_session.xml` 対応、`IM_FROM_TASK` 判定 | ✅ 完了 (LSL `llInstantMessage` 経由で IM_FROM_TASK が Object タブに分離されることを確認) |
| M7 | 3 OS ビルド (Linux → Win → Mac) | ✅ 完了 (2026-05-16、3 OS ビルド + 動作確認 PASS) |
| M8 | spec change 実装: System & Object 再グループ化 + フレンド online → Human 例外 cvar (`FSFriendOnlineToHumanTab`) + タブラベル `Object` → `System & Object` (3 言語) + 上流 LL style 通知 dispatch 復活 + 未読バッジの `is_replay` 分離 | ✅ 完了 (2026-05-16、3 OS 受入 PASS) |
| M9 | Release — **r22 単独タグは発行せず、r23 リリースに同梱配信**。本 spec doc finalize 済、release note 3 言語 finalize 済、r23 release page から本 spec へリンク | ✅ 完了 (2026-05-16、r23 同梱方針確定) |

---

## 11. 履歴

- **2026-05-15 (初版)**: ドラフト作成。AYA さんとの設計検討対話 (GUI 案 α 採用、データ層 1 本維持 + suffix marker、表示色維持の必須要件) を 1 ページにまとめ。判定境界とスコープ詳細は §7 に未確定として記録。
- **2026-05-15 (M1 完了)**: M1 で以下を確定:
  - 対象 style: FS V1 / V7 / LL の **3 style 同時対応**
  - 判定境界: `CHAT_SOURCE_AGENT` → Human、`CHAT_SOURCE_OBJECT` → Object、`SYSTEM` / `TELEPORT` / `REGION` / `UNKNOWN` → Human
  - opt-in/out: **opt-out (default ON)**
  - 設定 cvar: `FSChatHumanObjectTabs` (Boolean, default true, persist 1)、配置 `Preferences → Chat → Chat Windows`
  - UI ラベル: `Split chat into Human / Object tabs`
  - escape hatch: 上記 cvar の false 化で代用、別 cvar 不要
  - HUD allow list: r22 スコープ外、r23+ で検討
  - 構造調査の結果、`EChatSourceType` が既存定義済みで判定ロジックは新規不要、`LLLogChat::saveHistory()` シグネチャ拡張要、LL style は `panel_container` で session 切替する構造
- **2026-05-16 (M7 完了)**: Windows + macOS でもビルド通過 + 動作確認 PASS。3 OS で受入基準を全項目クリア、M8 (Release) に移行。
- **2026-05-16 (spec change: System & Object 再グループ化)**: AYA さんから「TP したときのログが Human タブに表示されないようにしてほしい」要求。3-tab (Human / Object / System) 案を一旦検討したが、「タブ多いと切り替えて見ない」との判断で **2-tab 再グループ化** に決定。
  - **判定境界の変更**: `CHAT_SOURCE_SYSTEM` / `TELEPORT` / `REGION` を Human → **System & Object** へ移動。`CHAT_SOURCE_UNKNOWN` だけは Human フォールバックを維持 (default safe)
  - **タブラベル**: `Object` → `System & Object` (順序固定。AYA さん談: "Object が偉そうだから" — `Object & System` ではない)
  - **フレンド online/offline 例外**: 「会話したい相手の登場」性質が強いため、新 cvar `FSFriendOnlineToHumanTab` (デフォルト ON) で Human タブにも複製可能とする。System & Object には cvar 値に関わらず常に出力。実装は `llcallingcard.cpp` で chat 構築時に `LLChat::mFriendOnlineNotification` フラグを立てる方式。
  - **据え置き**: `FSChatHumanObjectTabs` cvar 名 (rename しない) / 履歴 suffix marker フォーマット (semantic shift のみ、loader のタブ振り分けロジックが変わる)
  - **マイルストーン再採番**: 旧 M8 (Release) → M9、新規 M8 = spec change 実装
  - **背景**: AYA さん自身が「Human 開いてたらわからない」=「会話したい相手の登場は会話の流れに残したい」と説明、設定で出す/出さないも選べるようにしておく方が複数ユーザーの嗜好をカバーできるとの判断 (デフォルトは Human にも出す、cvar OFF で旧仕様派にも対応)
- **2026-05-16 (M8 追加: LL style TP セパレーター復活)**: Sandbox で region 通知が LL style に出ないという AYA さんからの指摘を起点に調査。原因は Firestorm の `llagent.cpp:5029` で 2021-02-03 (Ansariel, commit `a1c46dc4125`) に `LLFloaterIMNearbyChat::addMessage()` 呼び出しが commented out されており、TP セパレーターが FS style にしか届かない上流 latent bug と判明。r22 で LL style を一級化した結果として顕在化した。
  - **方針**: コメントアウトを復活させ、`findTypedInstance` でガードした上で 3 style に dispatch (option b、AYA 判断 "正しくは出るべき")
  - **実装**: `llagent.cpp` の `#include "llfloaterimnearbychat.h"` を復活、`LLAgent::addRegionChangedCallback` 内で `FSFloaterNearbyChat` と `LLFloaterIMNearbyChat` の両方に `addMessage(chat, true, args)` を呼ぶ。`findTypedInstance` は inactive style では nullptr を返すので二重通知にはならず、`args["do_not_log"] = true` で履歴二重書きも防止
- **2026-05-16 (M8 追加: TP 着地時の未読バッジ修正)**: AYA さんから「TP したログは未読カウントされてない」との追加指摘。原因は M5 で未読バッジの抑止条件に `args["do_not_log"]` を流用していたこと。TP セパレーターは「履歴に残さないが session 内の新規イベント」なので `do_not_log=true` で送られるが、未読カウントは bump すべき性質。
  - **方針**: history reload 経路に専用フラグ `args["is_replay"]` を導入し、未読バッジ条件を `!args["is_replay"]` に切り替える。`do_not_log` は本来の「履歴ファイルに書かない」意味のみに戻す。
  - **実装**: `fsfloaternearbychat.cpp` の `updateChatHistoryStyle` / `reloadMessages` / `loadHistory` および `llfloaterimnearbychat.cpp` の `reloadMessages` / `loadHistory` の各経路で `do_not_log["is_replay"] = true;` を併設。`fsfloaternearbychat.cpp` / `llfloaterimsessiontab.cpp` の `log_active` 条件を `!args["is_replay"].asBoolean()` に変更。
  - **影響範囲**: TP 着地時のセパレーター, region change 系通知などで未読バッジが正しく bump されるようになる。history reload は引き続き抑止される (今までと同じ挙動)。
- **2026-05-16 (M8 追加: LL style notification tip 復活)**: AYA さんから「V7 では `secondlife://... からのテレポートが完了しました` と `現在のシミュレータ / 以前のシミュレータ` の通知が出るが LL 版では出ない」と指摘。調査結果、`llnotificationhandlerutil.cpp:273` の `LLHandlerUtil::logToNearbyChat` で notification tip (`ChatSystemMessageTip` 等) を chat に流す処理が Ansariel 改造で FS 専用 (`FSFloaterNearbyChat`) のみに dispatch されていた。`llagent.cpp:5029` と同パターンの上流 FS 改造、LL style 一級化で顕在化。
  - **方針**: `llagent.cpp` の修正と同様に、`LLFloaterIMNearbyChat` も `findTypedInstance` でガードしつつ並列 dispatch。3 style 一貫性確保。
  - **実装**: `llnotificationhandlerutil.cpp` の `#include "llfloaterimnearbychat.h"` を復活、`logToNearbyChat` 内で `FSFloaterNearbyChat::addMessage` と `LLFloaterIMNearbyChat::addMessage` の両方を呼ぶ。
  - **副次効果**: TP 完了 SLURL (`FSShowBackSLURL=true` 時) / region simulator version 差異 / RLV 系通知など、`ChatSystemMessageTip` 経由の system tip が全て LL style にも届くようになる。`mSourceType = CHAT_SOURCE_SYSTEM` で送られるので System & Object タブにルーティングされる。
- **2026-05-16 (M8 完了 + 3 OS 受入 PASS)**: Linux / Windows / macOS の 3 OS で M8 込みの再ビルド + 動作確認すべて PASS。§7 の M8 追加受入項目 (11 件) を全てクリア。
- **2026-05-16 (r23 同梱リリース確定)**: r22 単独タグは発行せず、r23 (parcel-bound 3D stream) リリースに同梱配信することを決定。理由: r22 / r23 とも 3 OS テスト PASS 済で配信遅延要因が無く、独立 tag を 2 本切るより 1 リリースにまとめた方が release page / 周知の整理に有利。本 spec doc と release note 3 言語 (`docs/ayastorm-r22-release-note.{en,ja,zh}.md`) は単体で完結する形のまま保持し、r23 release page からリンクする運用とする。
