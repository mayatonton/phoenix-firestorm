🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.zh.md)

# AYAstorm r31-bugfix-2 — AO 刪除事象救濟 + LSL Bridge 衝突防禦

> [!IMPORTANT]
> **r31-bugfix-2 是在 AYAstorm 側阻止 Firestorm 系列 viewer 整體存在的 2 件結構性行為的版本。**
>
> 並非 AYAstorm 特有的問題，而是對 Firestorm 衍生 viewer 全體共用的 inventory root 所衍生之結構性行為的對應 (屬於 bug 或仕樣的判斷由 upstream 決定)。

1. **AO 刪除事象**: 在 Firestorm 系列 viewer (Firestorm 本家 / 舊版 AYAstorm / 其他 FS 衍生) 中按下 AO 集合「刪除」時，共用 inventory root `#Firestorm` 之下的 AO 資料會被永久消除，即使從別的 viewer 登入也仍然消失，形成跨 viewer 連鎖事象。已在約 1000 人規模觀測。r31-bugfix-2 在 viewer 側完全停止實際的 inventory 操作，只以 per-account 設定的隱藏 flag 進行 UI 上的非顯示
2. **LSL Bridge 版本衝突**: `fslslbridge.cpp` 在 version 不一致時自動再建立的邏輯，會在 Firestorm 本家未來 minor bump 時造成 AYAstorm 側 Bridge 一同被刪除。目前同為 v2.29 故尚未發火，作為單向防禦導入「收到 version > 自己時則 adopt」的邏輯

修正是對 Firestorm 系列 viewer 全體所存在的結構性行為的對應。只在 AYAstorm 側停止對 `#Firestorm` root 的破壞性操作，Firestorm 本家 / 其他衍生 viewer 中的再發各別 viewer 需另行 patch (推薦運用與規避手段在 recovery guide 中明示)。

## 已有 AO 集合消失的使用者 — AO 機能再次設置手順

在 Firestorm 系列 viewer (Firestorm 本家 / 舊版 AYAstorm / 其他 FS 衍生) 中已按下「刪除」的 AO 集合的 **AO 資料本身在 viewer 側或 SL 伺服器側均無法找回**。不過 AO 機能本身可以透過簡單的再設置回到正常使用狀態。手順以 3 種語言公開:

- 🇨🇳 [繁體中文復原指南](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)
- 🇺🇸 [English Recovery Guide](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
- 🇯🇵 [日本語復旧手順](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)

## Release notes

- 🇺🇸 English: [docs/release/ayastorm-r31-bugfix-2-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-bugfix-2-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-bugfix-2-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.zh.md)

## Key documents (tag pinned)

- 技術 spec: [docs/specs/ayastorm-r31-2-ao-bridge-recovery.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-ao-bridge-recovery.md)
- 使用者復原手順 (3 種語言): [docs/guides/ao-data-recovery-guide.{en,ja,zh}.md](https://github.com/mayatonton/phoenix-firestorm/tree/v7.2.4-ayastorm-r31-bugfix-2/docs/guides)

## 與既有環境的相容性

不擾亂 r31 / r31-bugfix-1 環境而出貨:

- **AO 刪除事象 fix**: 自動套用。不需變更設定。已安裝 r31 / r31-bugfix-1 者，僅需在上層覆蓋安裝 r31-bugfix-2 即可防止再次發生
- **LSL Bridge 衝突防禦**: 自動套用。防止 Firestorm 本家對 Bridge 進行 minor bump 時 AYAstorm 側 Bridge 被刪除的事象 (目前尚未發火，為將來防禦)
- **r31 / r31-bugfix-1 的全部機能** (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP audio routing / macOS branding / GPU other-rigged picker / chat tab split / venue reverb / SSS pink-shadow 修正等): 原樣保持
- **不編輯 / 刪除 AO 的使用者**: 沒有可見變化。只是「Delete」按鈕變為「Hide」，Dialog 文字稍有變更
- **已有 AO 集合消失的使用者**: 安裝 r31-bugfix-2 **可防止相同事象再次發生**。已消失的 AO 資料本身無法找回，請以上述再設置手順讓 AO 機能立即回到正常使用狀態

## IR licence

r11 起同捆並持續出貨的 venue IR 來自 OpenAIR (CC-BY 4.0)。出處: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/indra/newview/app_settings/venue_ir/CREDITS.md)

## Downloads

- [Windows Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_AVX2-7-2-4-261481144_Setup.exe)
- [macOS Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-81209.dmg)
- [Linux Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261481144.tar.xz)

## Contributors

@t-noami @mayatonton
