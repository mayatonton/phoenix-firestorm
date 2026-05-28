# [Bug Fix] AYAstorm r31.2: AO 破壊事故救済 + LSL Bridge 衝突修正

**作成日**: 2026-05-28
**対象**: AYA + Claude Code
**作業ブランチ**: `fix/r31-2-ao-bridge-recovery` (予定)
**ベース**: `ayastorm-release`
**関連仕様**:
- `docs/specs/ayastorm-r31-2-sss-fullbright-glow-fix-report.md` (同 r31.2 release の既 merged 分)

**Memory 参照**:
- `memory/project_ayastorm_inventory_shared_local_separate.md` — AYAstorm の inventory 共有 / local 分離構造
- `memory/project_ayastorm_rlv_user_base.md` — AYAstorm のユーザー層に RLV ヘビーが多い可能性

---

## 1. 背景

### 1.1 発端: AO 破壊事故 (2026-05-28 報告)

Windows 環境のユーザーから報告:

> AYASTORM の builtin AO を編集した。現在の AO を削除して新規ロードしようとしたところ
> "loading" のまま反応しない。再起動しても直らず、Firestorm 本家の AO まで動かなくなった。
> アンインストール + 再インストールでも復旧しない。

macOS で再現を試みた他開発者 (@t-noami) では再現せず。Windows ユーザー 1000 人スケールでの影響が懸念される重篤バグ。

### 1.2 原因マップ

SL inventory はサーバ側 account 共有 state。AYAstorm は Firestorm 派生として `#Firestorm/` という inventory root を Firestorm 本家と同じ場所に作る。**同じ account でログインすればどの viewer も同じものを見る**。

| 階層 | 例 | 共有度 |
|---|---|---|
| SL 標準 (LL 由来) | `My Outfits/`, `Clothing/`, `Animations/` etc | 全 viewer 共通 (仕様) |
| Firestorm 派生 | `#Firestorm/#AO`, `#LSL Bridge`, `#Wearable Favorites` | FS 派生間で共通 |
| RLV コミュニティ標準 | `#RLV` | RLV 対応 viewer 全部 |

`AOEngine::removeSet()` (`aoengine.cpp:1341`) が呼び出す `purgeFolder()` (`aoengine.cpp:1313-1339`) は 3 段階削除 (`removeCategory` → `purge_descendents_of` → `remove_inventory_object`) で AO セットを **trash 経由なし完全消去**。AYAstorm 側でセット削除 → Firestorm 側でも同じデータが消える。

復旧不可の三重苦:
- viewer に内蔵 default AO プリセット **無し** (Firestorm/AYAstorm 共通)
- `#Firestorm` の server-side 自動再生成 **無し** (`ensureCategoryForTypeExists()` は `FT_FIRESTORM` 非対応)
- Settings reset での復旧 **不可** (`FSCurrentAOState` は asset UUID のみ保存、inventory structure 情報なし)

「内蔵 default AO セットの自動投入」案 (2026-05-28 検討) は **技術検証で drop**。詳細は §8 を参照。被害者向け復旧は §5 の手動手順 (marketplace 入手) のみ。

### 1.3 これは AYAstorm 固有のバグか? (no)

調査結果 (2026-05-28):

- `aoengine.cpp` の初期 commit `3a616b9546 "First draft for a viewer side animation overrider"` は **Firestorm 由来**。`removeSet()` / `purgeFolder()` は Firestorm から継承したコード。
- `removeSet()` の唯一の呼び出し元は `ao.cpp:576` (`FloaterAO::removeSetCallback`) — これも Firestorm 由来の旧 AO floater (Trash ボタン handler)。
- 結論: **Firestorm 本家でも同じ操作で同じ被害が出る**。AYAstorm 固有のコードバグではない。

ではなぜ AYAstorm で先に発覚したか:

| 要因 | AYAstorm | Firestorm 本家 |
|---|---|---|
| 内蔵 AO セット提供 | **あり** | 無し (user 導入) |
| 「編集→削除→新規ロード」発生確率 | **高** | 低 (自作 AO は Trash しない) |
| 1000 人スケールの集約報告 | あり | silent (個別事例のみ) |

AYAstorm の内蔵 AO 提供が「ユーザーが Trash ボタンを押すユースケース」を産み、Firestorm 由来の潜在バグを顕在化させた。AYAstorm 側で soft hide 化して fix する責務はある (継承元バグでも下流の責任)。upstream PR で FS 本家にも fix を送ることで全 FS 派生ユーザーを救済できる (§8 参照)。

### 1.4 横展開チェック結果

同型の inventory 自動操作リスクを全数チェック (background agent 2 回実施):

| 領域 | リスク | 詳細 |
|---|---|---|
| AO | **致命的** | `purgeFolder()` で完全削除、復旧不可 |
| LSL Bridge | **時限爆弾** | `fslslbridge.cpp:239` で version 不一致時 `recreateBridge()`、現在 FS と AYA 同 v2.29 で未発生 |
| Wearable Favorites | 低 | 並び順は per-account settings、inventory はリンクのみ |
| `#Firestorm` 連鎖削除 | 無 | `purgeFolder()` は子のみ削除、親は touch しない |
| その他 (`#RLV`, Outfit 等) | 無 | viewer が自動削除しない、user 手動操作のみ |

→ 致命的なのは **AO のみ**、Bridge は時限爆弾、他は安全。

## 2. 設計判断の履歴

### 2.1 検討した 3 案

| 案 | 内容 | 工数 |
|---|---|---|
| A | AO soft hide + 内蔵 default AO + Bridge 片方向 fix | 3-4 日 |
| B | A + `#Firestorm/` → `#AYAstorm/` root 分離 + migration | 5-7 日 |
| C | A から Bridge 片方向 fix を抜き、`UseLSLBridge` default=false | 3 日 |

### 2.2 採用: 案 A (内蔵 default AO 同梱は技術検証で drop)

- **案 B (root 分離)**: 抜本対策だが migration コストと r31.2 緊急 release への重さで見送り。次 release (r32+) で再検討の選択肢として残す。
- **案 C (Bridge 無効化)**: AYA さん自身は Bridge を使わないが、AYAstorm ユーザー層には RLV ヘビーが一定数いる可能性 (memory: `project_ayastorm_rlv_user_base.md`)。Bridge 経由 RLV 機能 (`@adjustheight=force` 等) を default で殺すリスクを取らない。
- **案 A**: 採用するが、当初案 A に含めていた「内蔵 default AO 同梱」は技術検証 (`override()` 実装確認、2026-05-28) の結果 **drop**。LL 公式 `ANIM_AGENT_*` を override target に置いても、それ自体が base animation なので「base を base で override」となり override 効果ゼロ、UI が反応する錯覚を生むだけと判明 (詳細 §8)。結果的に r31.2 scope は AO soft hide + Bridge 片方向 fix + 復旧手順 doc の 3 本。

## 3. 修正内容

### 3.1 AO 削除を soft hide 化

#### 3.1.1 関連コード
- `indra/newview/aoengine.cpp:1341` `AOEngine::removeSet()` — セット削除 entry point
- `indra/newview/aoengine.cpp:1313-1339` `AOEngine::purgeFolder()` — 3 段階 inventory 完全削除

現状実装:

```cpp
bool AOEngine::removeSet(AOSet* set)
{
    purgeFolder(set->getInventoryUUID());
    mTimerCollection.enableReloadTimer(true);
    return true;
}
```

#### 3.1.2 修正方針
`removeSet()` を「inventory には触らず、viewer-local 設定の hidden flag のみ立てる」に変更。列挙時に hidden flag セットを除外。

```cpp
// 修正後 (concept)
bool AOEngine::removeSet(AOSet* set)
{
    if (!set) return false;

    // 旧: purgeFolder(set->getInventoryUUID());
    // 新: hidden marker のみ
    LLSD hidden = gSavedPerAccountSettings.getLLSD("FSAOHiddenSets");
    if (!hidden.isArray()) hidden = LLSD::emptyArray();
    hidden.append(LLSD(set->getInventoryUUID().asString()));
    gSavedPerAccountSettings.setLLSD("FSAOHiddenSets", hidden);

    // mSets から外す (UI 上は消えたように見える)
    auto it = std::find(mSets.begin(), mSets.end(), set);
    if (it != mSets.end()) mSets.erase(it);

    mUpdatedSignal();
    return true;
}
```

#### 3.1.3 列挙時のフィルタ
`AOEngine::update()` (`aoengine.cpp:1557` 周辺) で `#AO` 配下を走査するとき、`FSAOHiddenSets` に含まれる UUID をスキップ。

#### 3.1.4 「再表示」UI 実装

問い合わせ激増の懸念 (症状未発生のユーザーが「何を確認すれば?」と問い合わせる) を考慮し、r31.2 で UI を実装する。

設計:
- AO floater (`floater_ao.xml` または `fs_floater_ao.xml`) に「Manage hidden sets」ボタンまたは context menu を追加
- 押下で hidden sets 一覧 floater を開く
- 各 hidden set に "Restore" ボタン (`FSAOHiddenSets` から該当 UUID を削除)、または「全 restore」ボタン
- 1000 人スケールの問い合わせを「UI に置いてある」で吸収

実装範囲 (詳細はコード着手時に確定):
- XUI: hidden sets 管理用 panel または floater (`floater_ao_hidden_sets.xml` 新規)
- C++: 一覧取得 / restore action (`AOEngine::getHiddenSets()` / `unhideSet()` 等)
- AO floater 本体に「Manage hidden sets」trigger 追加

#### 3.1.5 削除操作時の警告 Dialog 強化 (3 言語)

ユーザーが Trash アイコンを押した時に「inventory は触らず hidden 化のみ」であることを明示する Dialog に差し替える。1000 人被害の引き金になった「軽い気持ちで削除」を 1 段抑止し、AYAstorm 上は消えるが Firestorm 本家からは引き続き見える、というギャップを事前に説明する。

対象: 既存 `RemoveAOSet` notification (`ao.cpp:567` の `LLNotificationsUtil::add("RemoveAOSet", ...)`)。notification 名は維持 (callsite 1 箇所 / 他言語フォールバック挙動連続性)。

書き換える言語: **en / ja / zh** の 3 言語。他 11 言語 (az/da/de/es/fr/it/pl/pt/ru/tr) は en フォールバック (既存 AYAstorm 改修の標準パターン)。

ボタン label: `Remove`/`削除する`/`刪除` → `Hide`/`非表示にする`/`隱藏`。Trash アイコンの tooltip も同様に変更。

##### en (`indra/newview/skins/default/xui/en/notifications.xml:11173-11183`)

```xml
<notification
   icon="alertmodal.tga"
   name="RemoveAOSet"
   type="alertmodal">
"[AO_SET_NAME]" will be hidden from AYAstorm.

The inventory data in #Firestorm/#AO is NOT deleted —
it remains visible and usable in Firestorm and other
Firestorm-derived viewers using the same account.

You can restore it later via "Manage hidden sets".
    <usetemplate name="okcancelbuttons" notext="Cancel" yestext="Hide"/>
</notification>
```

##### ja (`indra/newview/skins/default/xui/ja/notifications.xml:4232-4235`)

```xml
<notification name="RemoveAOSet">
ＡＯセット「[AO_SET_NAME]」をＡＹＡｓｔｏｒｍ上で非表示にします。

ＳＬインベントリ内の #Firestorm/#AO のデータは削除されません。
同じアカウントで Firestorm や他の Firestorm 派生ビューアからログインすると引き続き表示・使用できます。

後で再表示するには「非表示にしたＡＯセットの管理」から復元できます。
    <usetemplate name="okcancelbuttons" notext="キャンセル" yestext="非表示にする"/>
</notification>
```

##### zh (`indra/newview/skins/default/xui/zh/notifications.xml:4398-4401`, 繁體中文)

```xml
<notification name="RemoveAOSet">
動畫覆蓋集「[AO_SET_NAME]」將從 AYAstorm 介面中隱藏。

SL 庫存中 #Firestorm/#AO 內的資料不會被刪除，
使用相同帳號從 Firestorm 或其他 Firestorm 衍生瀏覽器登入時，仍可正常顯示與使用。

如需復原，可透過「管理已隱藏的動畫覆蓋集」進行還原。
    <usetemplate name="okcancelbuttons" notext="取消" yestext="隱藏"/>
</notification>
```

### 3.2 (削除) 内蔵 default AO セット同梱

当初の案 A に含めていたが、技術検証 (2026-05-28) の結果 **drop**。詳細は §8 を参照。

被害者向け復旧は「viewer 側で何かを自動投入する」ことは行わず、`#AO` 空フォルダ再生成 (既存挙動、`AOEngine::tick()` の `ensureFolder()` で発生) + 復旧手順 doc 経由の marketplace 案内に切替。

### 3.3 LSL Bridge version 衝突 片方向 fix

#### 3.3.1 関連コード
`indra/newview/fslslbridge.cpp:237-244`:

```cpp
// Verify Version
std::string receivedBridgeVersion = llformat("%s%s", FS_BRIDGE_NAME.c_str(), bVer.c_str());
if (receivedBridgeVersion != mCurrentFullName)
{
    LL_WARNS("FSLSLBridge") << "BridgeVer message received from ("<< bAuth <<") was ("
                            << receivedBridgeVersion <<"), but it should be different ("
                            << mCurrentFullName <<"). Recreating." << LL_ENDL;
    recreateBridge();
    return true;
}
```

現状: 文字列単純比較 (`!=`) で、自分の `mCurrentFullName` と完全一致しない限り全て `recreateBridge()` が走る。**新版でも旧版でも区別なく削除**される。

#### 3.3.2 修正方針 (採用: 数値 parse 方式)

検討した 2 方式:
- (a) **数値 parse**: 受信した version 文字列を parse して major/minor を数値抽出、`受信 > 自分` のみ adopt、`受信 < 自分` は既存通り recreate
- (b) **シンプル**: `mCurrentFullName` と異なる version は全て adopt (削除しない)

→ **(a) を採用**。理由: (b) だと「過去に AYA が rez した古い Bridge object (例 v2.28) が inventory に残っている場合、新 AYA (v2.29) でも古いものを尊重して使い続けてしまう」リスクがある。(a) なら「古いものは update、新しいものは尊重」という正しい update flow を保てる。parse コードの追加分は数十行で許容範囲。

- 受信 > 自分 ⇒ **削除せず adopt** (新しい Bridge を尊重)、`mBridgeUUID = fromID` だけ更新
- 受信 == 自分 ⇒ 既存挙動 (そのまま採用)
- 受信 < 自分 ⇒ 既存挙動 (`recreateBridge()` で更新)
- parse 失敗 ⇒ 既存挙動 (`recreateBridge()`、安全側)

```cpp
// 修正後 (concept)
std::string receivedBridgeVersion = llformat("%s%s", FS_BRIDGE_NAME.c_str(), bVer.c_str());
if (receivedBridgeVersion != mCurrentFullName)
{
    // version 数値を parse
    S32 recvMajor, recvMinor;
    if (parseBridgeVersionString(bVer, recvMajor, recvMinor) &&
        (recvMajor > FS_BRIDGE_MAJOR_VERSION ||
         (recvMajor == FS_BRIDGE_MAJOR_VERSION && recvMinor > FS_BRIDGE_MINOR_VERSION)))
    {
        // 新しい version の Bridge を発見、削除せず尊重する
        LL_INFOS("FSLSLBridge") << "Found newer bridge v" << recvMajor << "." << recvMinor
                                 << " (we know v" << FS_BRIDGE_MAJOR_VERSION << "."
                                 << FS_BRIDGE_MINOR_VERSION << "), adopting it." << LL_ENDL;
        mBridgeUUID = fromID;
        mCurrentURL = bURL;
        return true;
    }

    // 古い version (or parse 失敗) → 既存挙動
    LL_WARNS("FSLSLBridge") << "..." << LL_ENDL;
    recreateBridge();
    return true;
}
```

新規 helper:

```cpp
// fslslbridge.cpp に追加
static bool parseBridgeVersionString(const std::string& bVer, S32& major, S32& minor)
{
    // bVer は "2.29" のような形式
    size_t dot = bVer.find('.');
    if (dot == std::string::npos) return false;
    try {
        major = std::stoi(bVer.substr(0, dot));
        minor = std::stoi(bVer.substr(dot + 1));
        return true;
    } catch (...) {
        return false;
    }
}
```

#### 3.3.3 防御方向と限界
- ✅ FS 本家が新 Bridge を rez → AYA でログイン: AYA は FS の新 Bridge を尊重、削除しない (片方向防御 成功)
- ❌ AYA が新 Bridge を rez → FS でログイン: FS 本家は patch されていないので AYA の新 Bridge を削除する
- 実害は「AYA が FS より version 先行している時のみ」発生。AYA は通常 FS から派生して同期するので version 先行は稀。長期的には FS 本家に upstream PR を出すか、案 B (root 分離) で抜本対策。

#### 3.3.4 「Adopt した Bridge を使う」の意味
`mBridgeUUID` に新 Bridge の UUID を保存することで AYA 起動中はその Bridge を使用。`FS_BRIDGE_MAJOR_VERSION` / `FS_BRIDGE_MINOR_VERSION` は AYA が知っている古い数値のままなので、新版 Bridge の **追加機能** (もしあれば) は使えない。基本機能 (RLV `@adjustheight=force` 等) は version 互換で動く前提。

## 4. 実装範囲

| ファイル | 変更内容 | 規模 |
|---|---|---|
| `indra/newview/aoengine.cpp` | `removeSet()` soft hide 化 / 列挙時 hidden filter / `getHiddenSets()` / `unhideSet()` 新規 | +60 行 / -2 行 |
| `indra/newview/aoengine.h` | `getHiddenSets()`, `unhideSet()` 宣言 | +6 行 |
| `indra/newview/skins/default/xui/en/floater_ao.xml` (or `fs_floater_ao.xml`) | "Manage hidden sets" trigger 追加 | +10 行 |
| `indra/newview/skins/default/xui/en/floater_ao_hidden_sets.xml` | hidden sets 管理 floater (新規) | +60 行 |
| `indra/newview/ao.cpp` (旧 FloaterAO controller) | trigger handler + hidden sets floater controller | +60 行 |
| `indra/newview/app_settings/settings_per_account.xml` | `FSAOHiddenSets` (LLSD) 追加 | +10 行 |
| `indra/newview/skins/default/xui/en/notifications.xml` | `RemoveAOSet` 文言書き換え + label `Hide` | -3 / +12 行 |
| `indra/newview/skins/default/xui/ja/notifications.xml` | `RemoveAOSet` 日本語訳書き換え + label 「非表示にする」 | -3 / +9 行 |
| `indra/newview/skins/default/xui/zh/notifications.xml` | `RemoveAOSet` 繁體中文訳書き換え + label `隱藏` | -3 / +9 行 |
| `indra/newview/fslslbridge.cpp` | version 比較ロジック書き換え + parse helper 追加 | +30 行 / -2 行 |
| `docs/specs/ayastorm-r31-2-ao-bridge-recovery.md` | 本 spec | new file |
| Release Notes (該当 location) | r31.2 release notes 追記 (本 spec へのリンク) | +20 行 |

ヘッダ変更は最小、設定追加 1 件 (per-account, Persist=1)、新規 file 2 (本 spec + hidden sets XUI)。

## 5. 既存被害ユーザー向け復旧手順 (公開 doc 同居)

被害ユーザーの inventory (`#Firestorm/#AO` 配下の AO セット) は viewer 側で復旧不可能。viewer 側で「内蔵 default AO を自動投入する」案は技術検証で drop (§8 参照)。よって復旧は **手動入手** が前提。

### 5.1 r31.2 をインストール → 再発防止
1. AYAstorm の Web サイトから r31.2 build をダウンロード
2. インストール、起動、SL アカウントでログイン
3. 以後、AO セット削除操作 (Trash) は soft hide 化されており **inventory は破壊されない**

### 5.2 notecard backup を持っている場合
SL では昔から AO 設定を notecard に backup する文化がある。backup を持っているなら:
1. inventory の `Notecards` 配下から AO note を探す
2. AO floater (Avatar メニュー → Animation Override) で `+` (新規セット) → notecard を drop して import
3. (詳細は Firestorm AO Manual を参照)

### 5.3 notecard backup がない場合 (= 大半のユーザー)
1. SL Marketplace で無料 / 有料 AO を探す:
   - `Vista Free AO` (代表的な無料 AO)
   - `Animare Free AO`
   - `ZHAO-II` (notecard 標準形式の元祖)
   - その他「AO Hud」「Animation Override」キーワードで検索
2. AO 商品を購入 / 取得後、含まれる notecard を rez (もしくは inventory から直接 drop)
3. AO floater で `+` → notecard を drop して import

### 5.4 Firestorm 本家側で AO を見る場合
本 r31.2 は AYAstorm 側からの修正のみ。Firestorm 本家側 (未 patch) で AO セット削除を行うと依然として inventory が破壊される。
- 推奨: AYAstorm に統一するか、Firestorm 本家でも AO セット削除は行わない (rename か hidden 運用)
- AYAstorm の soft hide で hidden にしたセットは inventory にそのまま残っているので、Firestorm 本家でも通常通り見える / 使える

### 5.5 hidden を解除したい場合
1. AO floater を開く
2. 「Manage hidden sets」ボタン (またはコンテキストメニュー) を押す
3. hidden set 一覧から該当セットを選択し "Restore"
4. AO セット一覧に戻る

(Debug Settings 経由のフォールバック手順は §7.3 を参照)

## 6. 検証手順

### 6.1 修正前の再現確認
1. AYAstorm を Linux で **修正前** のコードでビルド
2. SL Beta grid (Aditi) に test account でログイン (memory: `project_ayaudit_account.md`)
3. AO floater でセット作成 → 削除 → 「Loading...」のまま固まることを確認
4. Firestorm でも同じ account にログイン → AO データが消失していることを確認

### 6.2 修正後の検証
1. r31.2 修正版をビルド、インストール
2. test account で AO セットを 1 つ用意 (notecard import)
3. AO floater で当該セットの「削除」操作 → UI 上は消えるが、inventory (`#Firestorm/#AO/<set name>`) は **残っている** ことを確認
4. AYAstorm を再起動 → hidden 状態が維持される (削除したセットが UI に出てこない) ことを確認
5. Firestorm 本家で同じ account にログイン → 「削除」したはずの AO セットが `#Firestorm/#AO` に **無傷で存在** することを確認 (= 破壊しなくなった)
6. AYAstorm の AO floater で「Manage hidden sets」→ restore → セットが再び UI に出ることを確認

### 6.3 Bridge 衝突の検証
**実機での完全な再現は難しい** (Bridge の LSL script 内部 version 文字列まで偽装する必要)。確認できる範囲:
1. AYAstorm v2.29 で Bridge を rez
2. `fslslbridge.cpp` のテストとして、受信 `bVer` が `"2.30"` の場合を mock で発火させ、`recreateBridge()` が呼ばれず adopt パスに入ることを log で確認
3. 既存挙動 (`bVer="2.28"` 古い version) では `recreateBridge()` が走ることを確認 (regression なし)

### 6.4 3 OS 確認
| OS | 担当 | 内容 |
|---|---|---|
| Linux x86_64 | AYA | build, install, §6.1〜6.3 実施 |
| Windows x86_64 | AYA or @t-noami | build, install, §6.1〜6.2 実施 (1000 人被害者の OS) |
| macOS arm64 | @t-noami | build, install, §6.2 実施 (再現未経験だが回帰確認) |

### 6.5 Regression 確認
- Outfit / 服 / 持ち物 / Trash / Sound / Animation 等 SL 標準フォルダは触られないことを確認
- RLV pass-through (`@adjustheight=force` 等) が引き続き動作することを確認 (Bridge fix 後も Bridge は維持)
- LSL Bridge 機能 (Z 位置精密取得、Teleport history 補助等) が引き続き動作

## 7. リスクと留意点

### 7.1 Bridge 片方向 fix の弱点
逆方向 (AYA が新 → FS で削除) は AYA からは防げない。AYA が FS より version 先行する状況は稀 (AYA は FS から派生) なので実害小だが、長期的には:
- 案 X: FS 本家に同じ patch を upstream PR で出す
- 案 Y: 案 B (root 分離 `#Firestorm/` → `#AYAstorm/`) で抜本対策

### 7.2 既存被害者の inventory は復旧されない
soft hide 化は **再発防止** のみ。既に `purgeFolder` で破壊された 1000 人スケールの inventory は viewer 側で復旧不可能。復旧は §5.2 / §5.3 の手動入手のみ。release notes / 復旧手順 doc で明示的に告知する。

### 7.3 hidden set UI の Debug Settings フォールバック
UI が機能不全になった場合のフォールバック手順:
1. Debug Settings (`Ctrl+Alt+Shift+S`) を開く
2. `FSAOHiddenSets` を検索
3. 値を空配列 `[]` に設定 → 全 hidden 解除
4. AO floater を再読込

`FSAOHiddenSets` は per-account, Persist=1 の LLSD array なので、AYA 再起動後も hidden 状態は維持される。Debug Settings 経由でいつでも全 restore 可能。

### 7.4 既存 1000 人のうち「Bridge 衝突」未経験者
本 fix は Bridge 衝突を AYA 側で防御するが、現状 FS と AYA は同 v2.29 で衝突未発生。FS が version bump した時に「AYA 側 fix が効いた結果削除されなかった」ことに気づくのは難しい (silent fix)。release notes で「将来の FS Bridge アップデートに備えた防御」と説明。

## 8. 検討した代替案

### 案 B: root 分離 (`#Firestorm/` → `#AYAstorm/`)
未破壊ユーザーの AO セットを FS → AYA に migration するなど抜本対策。5-7 日工数で r31.2 緊急 release には重すぎる。r32+ で再検討の選択肢として残す。

### 案 C: AYA で Bridge を作らない (`UseLSLBridge` default=false)
衝突問題が根本消滅するが、AYAstorm ユーザー層に RLV ヘビーが一定数いる可能性 (memory: `project_ayastorm_rlv_user_base.md`) を考慮し、機能維持を選択。

### 案: 内蔵 default AO セット同梱 (drop 経緯)

当初案 A に含めていたが、技術検証 (2026-05-28) の結果 **drop**。

検証した方針 (UUID 調査 agent 結果):
- (a) LL 公式 `ANIM_AGENT_*` UUID をハードコード → **無効**
- (b) SL Library inventory から link → 同じ animation asset しか存在せず無効
- (c) AYAstorm 同梱 `.bvh` を initial upload → 工数 2-3 週間、r31.2 緊急 release に不可能
- (d) default 同梱諦め → 採用

(a) drop の根拠 (`aoengine.cpp::override()` 実装確認):

AO の override 動作は `state->mAnimations[].mAssetUUID` を `gAgent.sendAnimationRequest()` で再生 (`aoengine.cpp:716, 759`)。ここに LL 公式 `ANIM_AGENT_STAND` 等を入れても、それは元々 server が base animation として再生する UUID と同一。「base を base で override」となり、override 効果ゼロ。UI 上はセット選択が機能し AO が動いているように見えるが実体は何も変わらない = misleading。

`stateUUIDs[]` (`aoset.cpp:77-104`) は `mRemapID` (= server から来る base animation を識別するトリガー側 UUID) に入るもので、override target ではないことも確認。

結果として r31.2 では「viewer 側で被害者に何かを自動投入する」案を全て drop。復旧は marketplace 等での手動入手のみ (§5.3)。

### 案: notecard 自動 backup
AO セット保存時に notecard 自動 backup を inventory に作成する案。既存被害者は救えないので r31.2 緊急 release では skip。将来 release (r33+) で検討。

### upstream PR 候補

§1.3 で確認した通り、`removeSet()` → `purgeFolder()` は Firestorm から継承したコードで、本バグは **Firestorm 共通**。AYAstorm 側で soft hide 化した patch を Firestorm 本家に upstream PR として送ることで、全 FS 派生 viewer ユーザー (AYAstorm を含む) が同じ恩恵を受けられる。r31.2 リリース後、AYA 判断で upstream 提出を検討。

## 9. 改訂履歴
- **v1 (2026-05-28)**: 初版。1000 人被害報告に基づく緊急 r31.2 spec drafting。Background agent 2 回による横展開チェック + 3 案比較 + 案 A 採用までの判断履歴を含む。
- **v2 (2026-05-28)**: UUID 調査 agent 結果 + `override()` 実装確認に基づき「内蔵 default AO 同梱」案を drop (§3.2 / §5 / §6 / §8 整合更新)。AYA 質問「AYAstorm 固有のバグか?」に対し Firestorm 共通バグ判定を §1.3 に追記、upstream PR 候補を §8 末尾に追加。
