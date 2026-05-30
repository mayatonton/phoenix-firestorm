🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-github-release-page.zh.md)

# AYAstorm r31-bugfix-2 — AO 刪除事象救濟 + LSL Bridge 衝突防禦 + 3D Stream URL filter + 裝著物 alpha 順序 + Cinematic glow + 水中 alpha

> [!IMPORTANT]
> **r31-bugfix-2 是在 AYAstorm 側阻止 Firestorm 系列 viewer 整體存在的 2 件結構性行為的版本。**
>
> 並非 AYAstorm 特有的問題，而是對 Firestorm 衍生 viewer 全體共用的 inventory root 所衍生之結構性行為的對應 (屬於 bug 或仕樣的判斷由 upstream 決定)。

1. **AO 刪除事象**: 在 Firestorm 系列 viewer (Firestorm 本家 / 舊版 AYAstorm / 其他 FS 衍生) 中按下 AO 集合「刪除」時，共用 inventory root `#Firestorm` 之下的 AO 資料會被永久消除，即使從別的 viewer 登入也仍然消失，形成跨 viewer 連鎖事象。已在約 1000 人規模觀測。r31-bugfix-2 將一般 AO 集合刪除改為 per-account 隱藏 flag 的非破壞性 Hide。唯一的破壞性路徑是 Hidden 管理畫面中經確認 dialog 後的「刪除所選」 (`Delete selected`)
2. **LSL Bridge 版本衝突**: `fslslbridge.cpp` 在 version 不一致時自動再建立的邏輯，會在 Firestorm 本家未來 minor bump 時造成 AYAstorm 側 Bridge 一同被刪除。目前同為 v2.29 故尚未發火，作為單向防禦導入「收到 version > 自己時則 adopt」的邏輯，並在起動時 attach 階段也接受 newer bridge，避免 `BridgeVer` 送達前先 detach

修正是對 Firestorm 系列 viewer 全體所存在的結構性行為的對應。AYAstorm 側會從一般 AO 刪除路徑移除對 `#Firestorm` root 的破壞性操作，Firestorm 本家 / 其他衍生 viewer 中的再發各別 viewer 需另行 patch (推薦運用與規避手段在 recovery guide 中明示)。

## AO 集合已消失的使用者 — AO 再設置手順

> [!IMPORTANT]
> 已被上述事象抹去的 AO 集合的 **AO 資料本身在 viewer 側或 SL 伺服器側均無法找回**。不過 AO 機能本身可以透過簡單的再設置回到正常使用狀態。**手順以 3 種語言公開，請使用符合您環境的版本:**
>
> - 🇨🇳 [**繁體中文復原指南**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.zh.md)
> - 🇺🇸 [**English Recovery Guide**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.en.md)
> - 🇯🇵 [**日本語復旧手順**](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/guides/ao-data-recovery-guide.ja.md)
>
> 安裝 r31-bugfix-2 本身 **可在 AYAstorm 側阻止相同事象再次發生**。Firestorm 本家 / 其他 FS 衍生 viewer 中的再發各別需要 viewer 端 patch，相應的規避手段在復原指南內亦有明示。

## 裝著物 alpha render-order — 3-pass dispatch (PR [#122](https://github.com/mayatonton/phoenix-firestorm/pull/122))

這是 r31-bugfix-2 的 **與 AO 救濟並列的 2 大修正項目** 之一。r30 §5 出貨的「rigged hair / SIM N-BL render-order swap」雖然解消了透過頭髮的天空空抜，但副作用是裝著物 N-BL prim (睫毛 prim 等) 反而被繪在 rigged hair 之前，然後被 over-blend 蓋掉，形成可見的 regression — 在使用裝著物 alpha prim 的 avatar (眼 / 眉 / 睫毛由 prim 構成的 head) 上顯現為「眉與睫毛變淡 / 缺失」的 regression。

將 POST_WATER 的 forward pass 切分為 3 sub-pass (SIM N-BL → 全 R-BL → 裝著物 N-BL)，使用 `LLDrawInfo::mAttachedToAvatar` 作為 per-draw discriminator 兩立。§5 swap fix 維持的同時恢復裝著物 prim 的前面整列。自動套用 — 不需變更設定。

與被 falsify 的 A/B/C 案的結構性比較請參照 [`docs/specs/ayastorm-double-alpha-c-plan-extension.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-double-alpha-c-plan-extension.md)。

### Special thanks (PR #122 — 裝著物 alpha render-order)

此修正得到 **neria (neriamm)** 的大力協助。在多個 SL avatar 上的再現環境構築與候補修正的 hands-on 檢證，大幅縮短了結構性比較與最終實作選擇的範圍。neria 是 Second Life 的居民貢獻者 (並非 GitHub 帳號)，以 SL 名稱記載。

## 其他同捆修正

r31-bugfix-2 也同捆了在 r31-bugfix-1 之後著陸的以下 3 件修正:

- **3D Stream URL filter + UI 更新** (PR [#121](https://github.com/mayatonton/phoenix-firestorm/pull/121)): 3D Stream 機能改為初期狀態 OFF。URL 播放確認 dialog 改為顯示送來方的 object 名稱 / owner，讓各 viewer 能依據送來方判斷是否播放。透過 rezz 物件導致的非預期自動播放於 source-of-truth 階段就被擋住。使用者向解說請參照 [`docs/specs/3dstream-user-guide.zh.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/3dstream-user-guide.zh.md)
- **Cinematic glow min-luminance 修正** (PR [#123](https://github.com/mayatonton/phoenix-firestorm/pull/123)): BD parity port 帶入的 `RenderGlowMinLuminance = 0.0` 使 blank texture + 著色 prim (裝著物 / SIM rez object 兩者皆然) 即使 prim 側 Glow=0 也會誤發 post-process bloom。將閾值提升到 `0.5`，r31.0 / r31.1 中已被 persist 為 `0.0` 的使用者在下次 Cinematic mode 起動時透過 one-shot migration 強制矯正。Firestorm mode 不受影響 (LL default `1.0` 保持，migration 跳過並於下次 Cinematic 起動時再檢查)
- **水中 alpha plate 修正** (PR [#124](https://github.com/mayatonton/phoenix-firestorm/pull/124)): r30 P5 transparent-DoF C-(a) 導入的 `mAYAAlphaColor` redirect 並未對應 `LLPipeline::sUnderWaterRender`。水中時 main RT 已含 underwater fog 著色的 opaque scene，獨立 plate 卻被 clear 為 `(0,0,0,0)` → forward alpha 寫入 plate，pre-tonemap composite (`GL_ONE / GL_ONE_MINUS_SRC_ALPHA`) 讓 plate 覆蓋 underwater 著色的 main RT，導致睫毛 / 眉等裝著物 alpha prim 與 SIM particle 透明區域在水中顯示為純黑。上浮到水面後也會數 frame 再發。`use_alpha_rt` 條件加上 `!sUnderWaterRender`，水中時跳過 plate redirect，forward alpha 直接寫入 main RT = upstream FS 互換動作。水面以上路徑與舊版完全一致

## Release notes

- 🇺🇸 English: [docs/release/ayastorm-r31-bugfix-2-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r31-bugfix-2-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r31-bugfix-2-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/release/ayastorm-r31-bugfix-2-release-note.zh.md)

## Key documents (tag pinned)

- AO + Bridge 技術 spec: [docs/specs/ayastorm-r31-2-ao-bridge-recovery.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-ao-bridge-recovery.md)
- AO 使用者復原手順 (3 種語言): [docs/guides/ao-data-recovery-guide.{en,ja,zh}.md](https://github.com/mayatonton/phoenix-firestorm/tree/v7.2.4-ayastorm-r31-bugfix-2/docs/guides)
- 3D Stream URL filter 報告書 (日語): [docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-r31-2-3dstream-url-filter-and-ui-update-report.ja.md)
- 3D Stream 使用者指南 (3 種語言): [docs/specs/3dstream-user-guide.{en,ja,zh}.md](https://github.com/mayatonton/phoenix-firestorm/tree/v7.2.4-ayastorm-r31-bugfix-2/docs/specs)
- Alpha render-order 擴充報告書: [docs/specs/ayastorm-double-alpha-c-plan-extension.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-double-alpha-c-plan-extension.md)
- 6 類別 render order trace: [docs/specs/ayastorm-six-category-render-order-trace.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/docs/specs/ayastorm-six-category-render-order-trace.md)

## 與既有環境的相容性

不擾亂 r31 / r31-bugfix-1 環境而出貨:

- **AO 刪除事象 fix**: 自動套用。不需變更設定。已安裝 r31 / r31-bugfix-1 者，僅需在上層覆蓋安裝 r31-bugfix-2，一般 AO 集合刪除就會變成非破壞性 Hide
- **LSL Bridge 衝突防禦**: 自動套用。防止 Firestorm 本家對 Bridge 進行 minor bump 時 AYAstorm 側 Bridge 被刪除的事象 (目前尚未發火，為將來防禦)
- **3D Stream 初期 OFF + URL filter**: 已在 3D Stream 啟用狀態下運用中的配信者會保留 `Stream3DEnabled = true` 設定。新安裝會從初期 OFF 開始
- **Alpha render-order 3-pass dispatch**: 對全 avatar 自動套用。裝著物 N-BL prim (睫毛 prim 等) 會正確排在 rigged hair 之前，§5 swap 對透過頭髮的天空空抜修正仍然維持
- **Cinematic glow min-luminance**: 對於將 `0.0` 持久化的使用者，**下次 Cinematic mode 起動時** 會執行 one-shot migration。Firestorm mode 專用使用者不受影響 (LL default `1.0` 維持)
- **水中 alpha plate**: 自動套用。水中時 alpha BLEND 直接寫入 main RT 而非獨立 plate，underwater fog 著色得以透過 composite 保持。副作用: 水中時 transparent-DoF C-(a) plate composite 效果停用，但水中本來就是全畫面 fog，bokeh 結構性上看不見，視覺差幾乎為零。水面以上路徑不變
- **r31 / r31-bugfix-1 的全部機能** (3D Stream unified tag / AYAstorm View / parcel music Vorbis fix / MOAP audio routing / macOS branding / GPU other-rigged picker / chat tab split / venue reverb / SSS pink-shadow 修正等): 原樣保持
- **不編輯 / 刪除 AO 的使用者**: 幾乎沒有可見變化。AO set 的 trash icon 會變成非顯示 icon，Dialog 文字改為 Hide 前提
- **需要整理 AO inventory 的使用者**: Hidden 管理畫面提供「刪除所選」 (`Delete selected`)，確認後會永久刪除所選 hidden inventory folder。此操作無法復原
- **已有 AO 集合消失的使用者**: 安裝 r31-bugfix-2 **可防止相同事象再次發生**。已消失的 AO 資料本身無法找回，請以上述再設置手順讓 AO 機能立即回到正常使用狀態

## IR licence

r11 起同捆並持續出貨的 venue IR 來自 OpenAIR (CC-BY 4.0)。出處: [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r31-bugfix-2/indra/newview/app_settings/venue_ir/CREDITS.md)

## Downloads

- [Windows Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_AVX2-7-2-4-261492019_Setup.exe)
- [macOS Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-81339.dmg)
- [Linux Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r31-bugfix-2/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261500427.tar.xz)

## Contributors

@t-noami @mayatonton

(關於 neria (neriamm) 對 PR #122 的協助，請參照上方「裝著物 alpha render-order」章節中的「Special thanks」說明)
