# AYAstorm r31-bugfix-2 — 發佈通知

> [!IMPORTANT]
> **r31-bugfix-2 是在 AYAstorm 側阻止 Firestorm 系列 viewer 整體存在的 2 件結構性行為的版本。** 防止已在約 1000 人規模觀測到的 AO 集合消失事象再次發生，並對未來 LSL Bridge 版本漂移所引發的相互破壞提供單向防禦。
>
> 並非 AYAstorm 特有的問題，而是對 Firestorm 衍生 viewer 全體共用的 inventory root 所衍生之結構性行為的對應 (屬於 bug 或仕樣的判斷由 upstream 決定)。

與 AO + Bridge 修正並列的 **2 大修正項目** — 裝著物 N-BL prim alpha render-order 之 3-pass dispatch (PR #122) 也同捆於本版。此外還收錄了在 r31-bugfix-1 之後著陸的以下 3 件修正: 3D Stream URL filter + UI 更新 (PR #121)、Cinematic mode glow min-luminance bugfix (PR #123)、水中 alpha plate redirect 修正 (PR #124)。各 feature 別 section 請見本通知下方。

實作細節、影響範圍、復原手順常設於 `docs/specs/` 與 `docs/guides/` 之下。本通知為入口與差異重點。

---

## AYAstorm r31-bugfix-2 — AO 刪除事象救濟 + LSL Bridge 衝突防禦

### 標題: 將 AO 集合「刪除」改為非破壞性 Hide；LSL Bridge 相互刪除採單向防禦

在 Firestorm 系列 viewer (Firestorm 本家 / 舊版 AYAstorm / 其他 FS 衍生) 中按下 AO 視窗的「刪除」時，該 AO 集合的 Inventory 實體 (`#Firestorm/#AO` 之下的資料夾及其下所有動畫 / notecard) 會被 `purgeFolder` 永久消除。由於 `#Firestorm` root 在 Firestorm 衍生 viewer 全體共用，在某個 viewer 刪除後，之後從別的 viewer (含 Firestorm 本家) 登入仍然消失，形成跨 viewer 連鎖事象。

r31-bugfix-2 將一般 AO 集合「刪除」改為不刪除實際 inventory，只以 per-account 設定的隱藏 flag 進行 UI 上的非顯示。Hidden 管理畫面內的「刪除所選」 (`Delete selected`) 則作為經確認 dialog 後才執行的明示完全刪除保留。同時修正 LSL Bridge 版本不一致時的自動再建立邏輯，對 Firestorm 本家未來 Bridge 進行 minor bump 時的 AYAstorm 側 Bridge 消失採單向防禦。

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
- 為避免 hidden / visible 同名 AO 集合衝突，禁止以目前 hidden 中的名稱新建 / import，並在 restore 時拒絕與 visible 集合同名的 hidden set
- Hidden 管理畫面新增「刪除所選」 (`Delete selected`)。這是不同於一般 Remove 的明示完全刪除，經確認 dialog 後只刪除所選 hidden set 的實際 inventory folder
- 刪除 Dialog 文字以 3 種語言重寫，一般 AO 集合操作顯示為「Hide」而非「Delete」，明示 inventory 保留
- AO set 的 soft-hide 按鈕 icon 改為非顯示 icon，而非 trash icon

**LSL Bridge 單向 fix**:
- 將收到的 version 字串數值 parse 為 `major.minor` 並比較大小
- 收到 > 自己 ⇒ **不刪除而 adopt** (只更新 `mBridgeUUID` / `mCurrentURL`，不呼叫 `recreateBridge`)
- 收到 == 自己 ⇒ 既有行為
- 收到 < 自己 ⇒ 既有行為 (`recreateBridge` 更新)
- parse 失敗 ⇒ 既有行為 (安全側)
- 起動時 attach / detach 判定也使用同一 version 比較，避免 newer bridge 在送達 `BridgeVer` 前先被 detach
- newer bridge adopt 路徑與一般 handshake 完成處理合流，會送出 `URL Confirmed` 與初次設定同步

### Migration note

- **不需要任何使用者端的設定變更。** r31 安裝者直接在上層安裝 r31-bugfix-2 即可
- r31 全功能 (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP routing / macOS branding / other-rigged picker) 維持原樣
- r31-bugfix-1 的 SSS pink-shadow 修正也維持
- 既有受影響使用者的 inventory **無法由 viewer 側復原**。使 AO 功能可再使用的手順常設於 [`docs/guides/ao-data-recovery-guide.zh.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)

### Known limitations / future work

- **僅單向防禦**: AYAstorm 版本領先 Firestorm 本家時，Firestorm 本家側 (未修正) 仍會破壞 AYAstorm Bridge。AYA 領先 FS 的情形罕見，長期解為對 upstream Firestorm 發 PR / root 分離 (`#Firestorm/` → `#AYAstorm/`)
- **Firestorm 本家側的 AO 刪除仍為破壞性**: 推薦運用為將 AO 編輯 / 刪除集中於 AYAstorm r31.2+，Firestorm 本家側僅作 read-only 用 AO (在 recovery guide 中明示)
- **hidden 集合的 UI fallback**: 萬一新 UI 在某環境不可用，開啟 Debug Settings (`Ctrl+Alt+Shift+S`) 將 `FSAOHiddenSets` 清為空陣列即可全 restore。不過透過 Hidden 管理畫面的「刪除所選」 (`Delete selected`) 明示完全刪除的 folder 無法復原

### Implementation summary

- `indra/newview/aoengine.cpp` / `aoengine.h` — `removeSet()` 改為 soft hide，`getHiddenSets()` / `unhideSet()` / `unhideAllSets()` / `isSetHidden()`，`update()` 加入 hidden filter，hidden set 的完全刪除 / 同名衝突 helper
- `indra/newview/ao.cpp` / `ao.h` — `FloaterAOHiddenSets` controller + Manage hidden sets / Restore / Delete selected 按鈕配線
- `indra/newview/llviewerfloaterreg.cpp` — 註冊 `ao_hidden_sets` 浮動視窗
- `indra/newview/fslslbridge.cpp` / `fslslbridge.h` — bridge version 比較 helper、起動時 attach 對 newer bridge 的接受、adopt path、handshake 完成處理共通化
- `indra/newview/app_settings/settings_per_account.xml` — 加入 `FSAOHiddenSets` (LLSD, Persist=1)
- `indra/newview/skins/default/xui/{en,ja,zh}/notifications.xml` — `RemoveAOSet` 文言 + 按鈕標籤改寫、hidden set 衝突 / 完全刪除確認通知
- `indra/newview/skins/default/xui/{en,ja,zh}/panel_ao.xml` — 「Manage hidden sets」按鈕、AO set soft-hide icon / tooltip 調整
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

---

## AO 集合已消失的使用者 — AO 再設置手順

> [!IMPORTANT]
> 已被上述事象抹去的 AO 集合的 **AO 資料本身在 viewer 側或 SL 伺服器側均無法找回**。不過 AO 機能本身可以透過簡單的再設置回到正常使用狀態。**手順以 3 種語言公開，請使用符合您環境的版本:**
>
> - 🇨🇳 [**繁體中文復原指南**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)
> - 🇺🇸 [**English Recovery Guide**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
> - 🇯🇵 [**日本語復旧手順**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
>
> 安裝 r31-bugfix-2 本身 **可在 AYAstorm 側阻止相同事象再次發生**。Firestorm 本家 / 其他 FS 衍生 viewer 中的再發各別需要 viewer 端 patch，相應的規避手段在復原指南內亦有明示。

---

## 裝著物 N-BL prim alpha render-order — 3-pass dispatch (PR [#122](https://github.com/mayatonton/phoenix-firestorm/pull/122))

### 標題: r31-bugfix-2 之 2 大修正項目 其二 — POST_WATER forward pass 切分為 SIM N-BL → R-BL → 裝著物 N-BL 三段並以 per-draw discriminator 分流

這是 AO + Bridge 救濟 **並列的 2 大修正項目** 之一。r30 §5 的 render-order swap (POST_WATER 全 non-rigged → 全 rigged) 解消了 rigged hair 越過的天空空抜，但副作用是裝著物 N-BL prim (睫毛 prim 等) 反而被繪在 rigged hair 之前然後被 over-blend 蓋掉的 regression (spec 用語為 S1 / S2)。將 POST_WATER 的 forward pass 切分為以下 3 sub-pass:

- **pass 1**: `forwardRender(false, ATTACHMENT_NONE)` — 僅 SIM rezz N-BL
- **pass 2**: `forwardRender(true)` — 全 R-BL (rigged hair 等)
- **pass 3**: `forwardRender(false, ATTACHMENT_ONLY)` — 僅裝著物 N-BL prim

per-draw discriminator 為 `LLDrawInfo::mAttachedToAvatar.notNull()`。將 pass 3 移到 rigged 之後使裝著物 prim 整列於 hair 前面，同時 pass 1 仍將 SIM 側 N-BL 繪於 rigged 之前以維持 §5 swap fix (透過頭髮的天空空抜解消)。PRE_WATER 為了 water fog 整合性維持 rigged-first。HUD 僅一次 forwardRender，不在範圍內。

被 falsify 的替代案 (alpha plate 獨立 depth 等) 與採用 3-pass 的結構性比較記錄於 `docs/specs/ayastorm-double-alpha-c-plan-extension.md §3`。

### Implementation summary

- `indra/newview/lldrawpoolalpha.cpp` / `lldrawpoolalpha.h` — `AttachmentFilter` enum 與 `forwardRender(rigged, filter)` overload、POST_WATER 切分為 3 sub-pass、per-draw `LLDrawInfo::mAttachedToAvatar` discriminator
- `docs/specs/ayastorm-double-alpha-c-plan-extension.md` — 與 falsified A/B/C 案的結構性比較 (新增)
- `docs/specs/ayastorm-six-category-render-order-trace.md` — 推導 3-pass 邊界所依據的 6 類別 render order trace 全文 (新增)

### Credits

- [@mayatonton](https://github.com/mayatonton) — 3-pass dispatch 的設計、實作、falsification analysis (A/B/C 案)、spec 整備。

### Special thanks

- neria (neriamm) — alpha render-order 問題的調查與檢證協助。在多個 SL avatar 上的再現環境構築與候補修正的 hands-on 檢證，對結構性比較與最終實作選擇的縮小範圍有重大貢獻。neria 是 Second Life 的居民貢獻者 (並非 GitHub 帳號)。

### Documentation

- 結構性比較 (擴充報告書): [`docs/specs/ayastorm-double-alpha-c-plan-extension.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-double-alpha-c-plan-extension.md)
- 6 類別 render order trace: [`docs/specs/ayastorm-six-category-render-order-trace.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-six-category-render-order-trace.md)

---

## 3D Stream URL filter + UI 更新 (PR [#121](https://github.com/mayatonton/phoenix-firestorm/pull/121))

### 標題: 將 3D Stream 改為初期 OFF；URL 播放確認顯示送來方 object 名稱 / owner

3D Stream 機能 (r26 導入、r31 以 unified tag 整理) 自 r31.2 起以 `Stream3DEnabled = false` 為初期值出貨。已在 3D Stream 啟用狀態下運用中的使用者會保留 persist 值，只有新安裝才從初期 OFF 開始。in-world 物件送來 stream URL 時的播放確認 dialog 改為顯示送來方的 object 名稱 / owner，讓各 viewer 能根據送來方判斷是否接受 URL。透過 rezz 物件導致的非預期自動播放不再以 UI 後追擋下，而是於 source-of-truth 階段就被擋住。

### 修正的原理

- `settings.xml` 中 `Stream3DEnabled` 初期值改為 `false`，既有使用者的 persist 值保留
- URL 播放確認 dialog 文字加入送來方 object 名稱 / owner (3 語言)
- source-of-truth 強制: 3D Stream 為 disabled 時，rezz 物件送出的 URL 在抵達 auto-play 路徑前就被丟棄 (非 UI 後追)

### Credits

- [@mayatonton](https://github.com/mayatonton) — 3D Stream URL filter + UI 更新的實作、3 語言提示在地化。

### Documentation

- 技術報告 (日語): [`docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md)
- 使用者指南 (English): [`docs/specs/3dstream-user-guide.en.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.en.md)
- 使用者指南 (日語): [`docs/specs/3dstream-user-guide.ja.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.ja.md)
- 使用者指南 (繁體中文): [`docs/specs/3dstream-user-guide.zh.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.zh.md)

---

## Cinematic glow min-luminance bugfix (PR [#123](https://github.com/mayatonton/phoenix-firestorm/pull/123))

### 標題: 將 Cinematic mode 的 `RenderGlowMinLuminance` 從 0.0 提升到 0.5；既出貨使用者以 one-shot migration 強制矯正

Cinematic mode 套用的 BD parity overlay 帶入了 `RenderGlowMinLuminance = 0.0`，降低了 HDR linear 空間的 bloom-extract 閾值。`glowExtractF.glsl` 以 `smoothstep(min, min+1.0, x)` 計算 bloom 寄與，故 `min = 0.0` 時連 HDR `0..1.0` 的中間 lit 都會發火，且 `warmth = max(r*0.75, g*0.6, b*0.712)` 路徑會讓著色 prim (blank texture + color picker) 即使 prim 側 Glow=0 也會發光。裝著物與 SIM rez object 兩者皆有症狀回報，而 texture 套用 prim 或白色 prim 不會發火。

將閾值提升到 `0.5`。Cinematic 的強力 bloom 表現 (空 / 強反射 / 強 emissive — HDR `0.5` 以上) 仍會發火，只切掉裝著物等級的中間 lit 所致非預期 bloom。對於在 r31.0 / r31.1 已將 `0.0` 持久化的使用者，以 one-shot migration (sentinel `AYAR31GlowMinLuminanceMigrationVersion`) 強制矯正，但 **僅在下次 Cinematic mode 起動時** — Firestorm mode 專用使用者不受影響 (LL default `1.0` 維持，migration 跳過且不 bump version，於下次 Cinematic 起動時再檢查)。

### 修正的原理

- `settings_cinematic_bd.xml`: `RenderGlowMinLuminance` `0.0` → `0.5`
- `settings.xml`: 加入 `AYAR31GlowMinLuminanceMigrationVersion` sentinel (S32, Persist=1, default 0)
- `llcinematicoverlay.{h,cpp}`: 實作 `applyR31GlowMinLuminanceMigrationIfNeeded()`。當 `AYAVisualRealismEnabled != 2` (Firestorm mode) 時不 bump version 而 skip，下次 Cinematic 起動再檢查
- `llappviewer.cpp`: 起動 sequence 中在既有 `applyR15GodraysCinematicMigrationIfNeeded()` 呼叫後追加 migration 呼叫

### Implementation summary

- `indra/newview/app_settings/settings_cinematic_bd.xml` — `RenderGlowMinLuminance` 閾值提升
- `indra/newview/app_settings/settings.xml` — migration sentinel
- `indra/newview/llcinematicoverlay.cpp` / `llcinematicoverlay.h` — `applyR31GlowMinLuminanceMigrationIfNeeded()` (帶 mode==2 保護)
- `indra/newview/llappviewer.cpp` — 起動 sequence 中的整合

### Credits

- [@mayatonton](https://github.com/mayatonton) — bloom 閾值調查、修正設計、one-shot migration 實作。

---

## 水中 alpha plate redirect 修正 (PR [#124](https://github.com/mayatonton/phoenix-firestorm/pull/124))

### 標題: 將 `mAYAAlphaColor` redirect 以 `!sUnderWaterRender` 進行 gate；水中時 forward alpha 直接寫入 main RT (FS 互換動作)

r30 P5 transparent-DoF C-(a) 導入的 `mAYAAlphaColor` redirect 會將 forward alpha BLEND 寫入獨立 alpha plate，然後在 tonemap 前以 (`GL_ONE / GL_ONE_MINUS_SRC_ALPHA`) over-blend composite 至 main RT。此 redirect 的啟用條件 (`use_alpha_rt`) 並未對應 `LLPipeline::sUnderWaterRender`。水中時 main RT 已含 underwater fog 著色的 opaque scene，獨立 plate 卻被 clear 為 `(0,0,0,0)` → forward alpha 寫入 plate，pre-tonemap composite 讓 plate 覆蓋 underwater 著色的 main RT，導致睫毛 / 眉等裝著物 alpha prim 與 SIM particle 透明區域 **在水中顯示為純黑**。上浮到水面後也會數 frame 再發 (camera 上升時 redirect 一度回到水面狀態，之後又漂回水中狀態)。

修正為單一條件 gate: 在 `use_alpha_rt` 加上 `!LLPipeline::sUnderWaterRender`。水中時跳過 redirect，forward alpha 直接寫入 main RT — 水中時與 upstream FS 互換的動作。水面以上路徑與舊版完全一致。

### 修正的原理

- `indra/newview/lldrawpoolalpha.cpp`: 在 `use_alpha_rt` 條件既有的 `gPipeline.mAYAAlphaColor.isComplete()` 前加上 `!LLPipeline::sUnderWaterRender &&`
- gate 抑制 redirect 時，alpha plate 維持 clear 狀態的 `(0,0,0,0)`。pre-tonemap composite 對 main RT 變為 no-op (`A_plate * 1 + RT * 1 = RT`)，因此不需要水中專用的 composite 路徑
- 水面以上的 transparent-DoF C-(a) plate composite 效果無變化。水中是全畫面 fog 使 bokeh 結構性不可見，故水中放棄 plate composite 在知覺上無差異

### Implementation summary

- `indra/newview/lldrawpoolalpha.cpp` — `use_alpha_rt` 條件加上 `!LLPipeline::sUnderWaterRender`，並以 inline 註解記錄水中 no-op composite 的依據

### Credits

- [@mayatonton](https://github.com/mayatonton) — 水中症狀調查、單一 gate 修正、5 連線點 (`mForwardToAlphaRT` / alpha blend factors / emissive routing / plate clear / plate composite) 的影響範圍追蹤。
