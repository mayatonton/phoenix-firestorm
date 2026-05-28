# AYAstorm r31-bugfix-2 — 發佈通知

> [!IMPORTANT]
> **r31-bugfix-2 是在 AYAstorm 側阻止 Firestorm 系列 viewer 整體存在的 2 件結構性行為的版本。** 防止已在約 1000 人規模觀測到的 AO 集合消失事象再次發生，並對未來 LSL Bridge 版本漂移所引發的相互破壞提供單向防禦。
>
> 並非 AYAstorm 特有的問題，而是對 Firestorm 衍生 viewer 全體共用的 inventory root 所衍生之結構性行為的對應 (屬於 bug 或仕樣的判斷由 upstream 決定)。

實作細節、影響範圍、復原手順常設於 `docs/specs/` 與 `docs/guides/` 之下。本通知為入口與差異重點。

---

## AYAstorm r31-bugfix-2 — AO 刪除事象救濟 + LSL Bridge 衝突防禦

### 標題: 將 AO 集合「刪除」改為非破壞性 Hide；LSL Bridge 相互刪除採單向防禦

在 Firestorm 系列 viewer (Firestorm 本家 / 舊版 AYAstorm / 其他 FS 衍生) 中按下 AO 視窗的「刪除」時，該 AO 集合的 Inventory 實體 (`#Firestorm/#AO` 之下的資料夾及其下所有動畫 / notecard) 會被 `purgeFolder` 永久消除。由於 `#Firestorm` root 在 Firestorm 衍生 viewer 全體共用，在某個 viewer 刪除後，之後從別的 viewer (含 Firestorm 本家) 登入仍然消失，形成跨 viewer 連鎖事象。

r31-bugfix-2 完全停止 viewer 側對 inventory 的實際操作，只以 per-account 設定的隱藏 flag 進行 UI 上的非顯示。同時修正 LSL Bridge 版本不一致時的自動再建立邏輯，對 Firestorm 本家未來 Bridge 進行 minor bump 時的 AYAstorm 側 Bridge 消失採單向防禦。

此行為並非 r31 引入。在 Firestorm 系列 viewer 中長期存在，是結構性的，包含 AYAstorm 在內的所有 FS 衍生 viewer 都受到影響 (屬於 bug 或仕樣的判斷由 upstream 決定)。

### 背景 — 為何會發生

**AO 刪除事象**:
- `aoengine.cpp::removeSet()` → `purgeFolder(catID, true)` 對 **inventory 實際資料夾遞迴刪除**
- `#Firestorm/#AO/<set name>` 的 AO 集合資料夾從 server 消失
- `#Firestorm` 是 Firestorm 衍生 viewer 全體共用的 root，刪除會傳播到所有 viewer
- 一旦發生 viewer 側無法復原 (SL server 側實體消失)

**LSL Bridge 版本衝突**:
- `fslslbridge.cpp:239` 中，只要收到的 bridge version 字串與自己的 `mCurrentFullName` 不完全一致，就會觸發 `recreateBridge()`
- `finishBridge()` → `cleanUpOldVersions()` 將比自己舊的 version 的 Bridge object 從 `#Firestorm/#LSL Bridge` 刪除
- 目前 Firestorm 本家與 AYAstorm 同為 `v2.29` 故尚未發火，但當 Firestorm bump 至 `v2.30` 時，AYAstorm 側 Bridge 將被刪除

### 修正的原理

**AO 刪除改為 soft hide**:
- 完全改寫 `removeSet()`，刪除 `purgeFolder` 呼叫
- 只在 per-account 設定 `FSAOHiddenSets` (LLSD array, Persist=1) append inventory UUID
- AO 列舉時 (`update()`) 以 hidden filter 從 UI 排除
- 新增「Manage hidden sets」浮動視窗，可由 UUID 一覽進行個別 / 全部 restore
- 刪除 Dialog 文字以 3 種語言重寫，按鈕由「Delete」改為「Hide」，明示 inventory 保留

**LSL Bridge 單向 fix**:
- 將收到的 version 字串數值 parse 為 `major.minor` 並比較大小
- 收到 > 自己 ⇒ **不刪除而 adopt** (只更新 `mBridgeUUID` / `mCurrentURL`，不呼叫 `recreateBridge`)
- 收到 == 自己 ⇒ 既有行為
- 收到 < 自己 ⇒ 既有行為 (`recreateBridge` 更新)
- parse 失敗 ⇒ 既有行為 (安全側)

### Migration note

- **不需要任何使用者端的設定變更。** r31 安裝者直接在上層安裝 r31-bugfix-2 即可
- r31 全功能 (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP routing / macOS branding / other-rigged picker) 維持原樣
- r31-bugfix-1 的 SSS pink-shadow 修正也維持
- 既有受影響使用者的 inventory **無法由 viewer 側復原**。使 AO 功能可再使用的手順常設於 [`docs/guides/ao-data-recovery-guide.zh.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)

### Known limitations / future work

- **僅單向防禦**: AYAstorm 版本領先 Firestorm 本家時，Firestorm 本家側 (未修正) 仍會破壞 AYAstorm Bridge。AYA 領先 FS 的情形罕見，長期解為對 upstream Firestorm 發 PR / root 分離 (`#Firestorm/` → `#AYAstorm/`)
- **Firestorm 本家側的 AO 刪除仍為破壞性**: 推薦運用為將 AO 編輯 / 刪除集中於 AYAstorm r31.2+，Firestorm 本家側僅作 read-only 用 AO (在 recovery guide 中明示)
- **hidden 集合的 UI fallback**: 萬一新 UI 在某環境不可用，開啟 Debug Settings (`Ctrl+Alt+Shift+S`) 將 `FSAOHiddenSets` 清為空陣列即可全 restore

### Implementation summary

- `indra/newview/aoengine.cpp` / `aoengine.h` — `removeSet()` 改為 soft hide，新增 `getHiddenSets()` / `unhideSet()` / `unhideAllSets()` / `isSetHidden()`，`update()` 加入 hidden filter
- `indra/newview/ao.cpp` / `ao.h` — `FloaterAOHiddenSets` controller + Manage hidden sets 按鈕配線
- `indra/newview/llviewerfloaterreg.cpp` — 註冊 `ao_hidden_sets` 浮動視窗
- `indra/newview/fslslbridge.cpp` — 新增 `parseBridgeVersionString()` helper + adopt path
- `indra/newview/app_settings/settings_per_account.xml` — 加入 `FSAOHiddenSets` (LLSD, Persist=1)
- `indra/newview/skins/default/xui/{en,ja,zh}/notifications.xml` — `RemoveAOSet` 文言 + 按鈕標籤改寫
- `indra/newview/skins/default/xui/{en,ja,zh}/panel_ao.xml` — 「Manage hidden sets」按鈕
- `indra/newview/skins/default/xui/{en,ja,zh}/floater_ao_hidden_sets.xml` — 新增浮動視窗 (3 語言)
- `indra/newview/skins/default/xui/en/floater_ao.xml` — 浮動視窗高度調整
- `docs/specs/ayastorm-r31-2-ao-bridge-recovery.md` — 技術 spec (新增)
- `docs/guides/ao-data-recovery-guide.{en,ja,zh}.md` — 使用者復原手順 (新增 / 3 語言)

### Credits

- [@t-noami](https://github.com/t-noami) — r31-bugfix-2 的 macOS 建置，以及對 AYAstorm 整體的持續實作貢獻 (r24 Dullahan audio callback / r25 Ogg Vorbis codec / r26 3D Stream media ring / r27 macOS branding 等)。
- [@mayatonton](https://github.com/mayatonton) — r31-bugfix-2 AO soft hide / LSL Bridge 衝突防禦的實作、影響範圍調查、3 語言復原手順 doc 整備。

### Documentation

- 技術 spec: [`docs/specs/ayastorm-r31-2-ao-bridge-recovery.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-ao-bridge-recovery.md)
- 使用者復原手順 (繁體中文): [`docs/guides/ao-data-recovery-guide.zh.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)
- 使用者復原手順 (English): [`docs/guides/ao-data-recovery-guide.en.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
- 使用者復原手順 (日本語): [`docs/guides/ao-data-recovery-guide.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
