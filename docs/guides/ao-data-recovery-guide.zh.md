# AO 資料復原指南 (AYAstorm r31.2)

本指南是為以下情形的使用者所撰寫:「在 Firestorm 系列 viewer (Firestorm 本家 / 舊版 AYAstorm / 其他 FS 衍生 viewer) 中刪除了 AO 集合，結果在其他 viewer 中 AO 也一併消失了」。

## 首先請了解這些重點

### 已刪除的 AO 資料無法復原

很遺憾，一旦 viewer 刪除了 AO 資料 (AO 集合內的動畫、設定 notecard)，**已沒有任何方法可以還原**。資料已從 Second Life 伺服器上完全消失。即使聯絡 Linden Lab 也無法復原。

不過，**讓 AO 功能再次能使用是可行的**。請參閱下方「步驟 2」。

### 為什麼會發生這種狀況？

在 Second Life 的 Inventory 中，AO 集合的儲存位置被 Firestorm 系列 viewer 全部共用。Firestorm 本家、AYAstorm、其他 FS 衍生 viewer 都將 AO 資料放在 SL 帳號的同一個位置。

這個設計原本是為了「無論用哪個 viewer 登入都能使用相同的 AO」，但副作用是「在某個 viewer 中刪除 AO，會造成所有其他 viewer 中的 AO 也消失」。

AYAstorm r31.2 已修正此問題，將一般 AO 集合「刪除」改為非破壞性的隱藏。例外是 Hidden 管理畫面中的 **「刪除所選」** (`Delete selected`): 這是經確認後永久刪除實際 inventory folder 的整理用操作。

---

## 步驟 1: 安裝 AYAstorm r31.2 或更新版 (防止再次發生)

1. 從 AYAstorm 網站下載 r31.2 (或更新版) 的安裝程式
2. 安裝 → 啟動 → 用 SL 帳號登入
3. 之後，在 AO 視窗中「刪除」一個集合 **不會破壞資料** (僅變更為隱藏)

只要做到這一步就能防止再次發生。

注意: Hidden 管理畫面中的 **「刪除所選」** (`Delete selected`) 是例外。它會永久刪除所選 hidden set 的實際 inventory folder，只有在您明確要整理 inventory 時才應使用。

---

## 步驟 2: 讓 AO 再次能使用 (復原手順)

已刪除的 AO 集合本身無法找回，但 AO 功能可以重新運作。請使用下列任一方法重新匯入 AO。

### A. 有 AO 的 notecard 備份時

在 SL 中，使用 notecard 備份 AO 設定是長久以來的習慣。如有備份:

1. 開啟 Inventory 視窗，找出該 AO 設定 notecard (通常在 `Notecards` 資料夾或您自己保存的位置)
2. 開啟 AYAstorm 的 AO 視窗 (Avatar 選單 → Animation Overrider)
3. 點擊「+」按鈕建立新集合
4. 將 notecard 拖放至 AO 視窗匯入
5. 選擇匯入的集合並確認運作

### B. 沒有備份時 (大部分使用者)

在 SL Marketplace 重新取得 AO。免費的也可以:

- **Vista Free AO** ── 長期以來的經典免費 AO
- **Animare Free AO** ── 同為免費
- **ZHAO-II** ── notecard 標準格式的元祖
- 搜尋關鍵字: "AO HUD", "Animation Override", "AO Free"

取得後:

1. 在 Marketplace 購買 (免費商品也是用「Buy」按鈕加入 Inventory)
2. 將 AO 商品中包含的 notecard rez 到地面，或從 Inventory 取出
3. 開啟 AYAstorm 的 AO 視窗 → 「+」 → 將 notecard 拖放
4. 確認運作

這樣 AO 功能就能恢復使用。

### 補充: 認為自己失去了 Linden 配發的預設 AO 的使用者

若您認為自己失去了原本 Linden Lab 配發的預設 (Library 來源的 AO 等)，**且特別希望復原該預設本身**，則 viewer 側無法復原，請向 Linden Lab Support 詢問。

若目的僅為「讓 AO 功能再次能使用」，上方 B. 重新取得免費 AO 即足夠。

---

## 步驟 3: 管理已隱藏的集合 (AYAstorm r31.2 新功能)

AYAstorm r31.2 將「刪除」改為「隱藏」。檢視與還原已隱藏的集合:

1. 開啟 AO 視窗
2. 點擊 **「Manage hidden sets」** 按鈕 (視窗下方)
3. 出現已隱藏集合的清單
4. 還原單一集合 → 選取 → **「還原所選」** (`Restore selected`)
5. 還原全部 → **「全部還原」** (`Restore all`)

一般 Hide 不會觸動資料本身，因此這些集合在其他 viewer (包括 Firestorm 本家) 登入時仍可正常看見、使用。

### 關於「刪除所選」

Hidden 管理畫面也有 **「刪除所選」** (`Delete selected`)。這不是一般 Hide。

- 它會從 `#Firestorm/#AO` 永久刪除所選 hidden AO set 的實際 inventory folder
- 從 Firestorm 本家或其他 Firestorm 衍生 viewer 登入時，該 AO set 也會消失
- AYAstorm 無法復原此操作
- 若沒有 backup / notecard，被刪除的 AO 資料無法復原

如果只是想讓 AO set 從 AO 視窗中消失，請**不要**使用「刪除所選」。請使用一般 Hide 與 Restore。

---

## 使用 Firestorm 本家時的注意事項

Firestorm 本家 (本指南撰寫時點尚未修正) 仍會在「刪除」AO 集合時破壞資料。

**若不小心在 Firestorm 本家刪除了 AO**:
→ 請使用上方「步驟 2」的手順重新匯入 AO，功能即可恢復。

**建議用法**:
- AO 的編輯 / 刪除操作請在 AYAstorm r31.2 或更新版進行
- 在 Firestorm 本家僅「使用」AO 集合，不要刪除
- 在 AYAstorm「隱藏」的集合，在 Firestorm 本家側可正常看見、使用

---

## 仍有問題時

- AYAstorm 問題回報: [GitHub Issues](https://github.com/mayatonton/phoenix-firestorm/issues)
- SL 官方: AO 資料已不存在於伺服器，向 Linden Lab 反映也無法復原

---

技術細節請參閱 [docs/specs/ayastorm-r31-2-ao-bridge-recovery.md](../specs/ayastorm-r31-2-ao-bridge-recovery.md)。
