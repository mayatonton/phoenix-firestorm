# AYAstorm r27 — 发布公告

用于粘贴到 GitHub release 页面的简短文案。**r27 将 macOS 上残留的 "Firestorm" 表记统一为 "AYAstorm"** — menu bar (应用菜单 / Hide / Quit) / 初始窗口标题 / Apple 标准 About 面板统一为 AYAstorm 表记,About 版本字符串中保留派生来源致谢 `(based on Firestorm)`。

实装为 2 文件的 surgical 修正,因此不另立永久文档,本说明本身即为 first-class 文档。

---

## AYAstorm r27 — macOS 的 Firestorm 表记统一为 AYAstorm

### r27 主轴: 清除仅在 macOS 残留的 "Firestorm" 表记

Linux / Windows 由于经 `VIEWER_CHANNEL = "AYAstorm Release"` 路径,窗口标题早就显示为 AYAstorm。但 macOS 上 menu bar (应用菜单 / Hide / Quit) 与 Apple 标准 About 面板仍然显示 "Firestorm"。

CMake 侧 (`MACOSX_BUNDLE_BUNDLE_NAME` / `MACOSX_EXECUTABLE_NAME` / `MACOSX_BUNDLE_INFO_STRING`) 已设为 AYAstorm,但 menu bar 仍是 Firestorm 的原因是: **macOS 在 runtime 时优先采用 `English.lproj/InfoPlist.strings` 的 `CFBundleName` 而不是 Info.plist 中的值** 的本地化规范 — .strings 文件的 1 行覆盖了 CMake 设置的全部内容。

r27 仅改写该 .strings 与 main menu nib 的源文件 `Firestorm.xib` 共 2 文件,统一 macOS 表记为 AYAstorm。

### 工作原理

```
indra/newview/English.lproj/InfoPlist.strings
  CFBundleName               : "Firestorm" → "AYAstorm"
  CFBundleShortVersionString : "Firestorm version X.X.X" → "AYAstorm version X.X.X (based on Firestorm)"
  CFBundleGetInfoString      : Firestorm → AYAstorm (并记派生来源致谢)

indra/newview/Firestorm.xib
  main menu 的 "Firestorm" 标题            → "AYAstorm"
  Apple submenu 的 "Firestorm" 标题        → "AYAstorm"
  "About Firestorm"                        → "About AYAstorm"
  "Hide Firestorm"                         → "Hide AYAstorm"
  "Quit Firestorm"                         → "Quit AYAstorm"
  初始 window title="Firestorm"            → "AYAstorm"
  window frameAutosaveName="Firestorm"     → "AYAstorm"
```

- 其他语言 .lproj (`Japanese.lproj/` `German.lproj/` `Korean.lproj/` 等) 仅持有 `language.txt`,不带 `InfoPlist.strings`。因此 macOS 会 fallback 到 CFBundleDevelopmentRegion (= English) 的本 .strings — 结果是 **所有 locale 的 menu bar 都显示 AYAstorm**
- 派生来源致谢 `(based on Firestorm)` 保留于 Apple 标准 About 面板的版本字符串内,license / 派生归属表示完整保留

### Firestorm 开发组致谢与联系方式不动

r27 改写的范围仅限 **macOS 的 OS 级别 branding 字符串**,AYAstorm About 浮窗 (Avatar 菜单 → Help → About AYAstorm 的内容,`indra/newview/skins/default/xui/en/floater_about.xml`) 1 个字符也不 touch。具体来说:

- Firestorm Development Team / Additional Contributors / Translators / UI Artists 名单 → 原样保留
- 「最新信息请见 <https://www.firestormviewer.org>」support URL → 原样保留
- Linden Lab credits / Licenses / Starlight skin 致谢 → 原样保留

Apple 标准 About 面板 (App menu → About AYAstorm) 是 **icon + 应用名 + 版本字符串 + 1 行 Copyright** 的 OS 固定 layout,本来就不是放开发者名或联系方式的位置。联系方式与开发致谢全部集中在 Firestorm About 浮窗一侧,r27 不会造成「AYAstorm 起因的 bug 误投 Firestorm 团队,或反之」的结构。

### 设置

**无需用户操作。** 启动 viewer 即可看到 macOS 表记切换为 AYAstorm。Linux / Windows 用户无感 (原本即为 AYAstorm)。

### 迁移备注

- **macOS 用户**: 启动 r27 后 menu bar / Hide / Quit / 标准 About / window title 全部切换为 AYAstorm 表记
- **Linux / Windows 用户**: 本次发布无可见变化
- **配信者 / listener 任一侧均无需操作**

### 已知限制

- **macOS 既有用户的 window 位置会 reset 一次**: `frameAutosaveName="Firestorm"` → `"AYAstorm"` 的变更导致 NSWindow 自动保存 window 位置 / 尺寸的 key 改变。r27 首次启动时新 key (`AYAstorm`) 没有存储,因此从默认位置打开。第 2 次启动起按新 key 再保存,恢复正常运用
- **Linux / Windows 实机验证不需要**: 不在本 PR 范围
- **macOS 实机验证**: 留到下次切 Release binary 时,确认 Apple 标准 About 面板 / menu bar / window title 全部为 AYAstorm 表记

### 实现概要

- `indra/newview/English.lproj/InfoPlist.strings` — CFBundleName / CFBundleShortVersionString / CFBundleGetInfoString 改写为 AYAstorm,版本字符串并记 `(based on Firestorm)`
- `indra/newview/Firestorm.xib` — main menu 的 menu item title (`Firestorm` / `About Firestorm` / `Hide Firestorm` / `Quit Firestorm`) 与 Apple submenu title、初始 window title、`frameAutosaveName` 改写为 AYAstorm
- 验证: `plutil -lint indra/newview/English.lproj/InfoPlist.strings` (.strings 语法) 与 `xmllint --noout indra/newview/Firestorm.xib` (xib XML 完整性) 由 PR 作者执行
- 不动的部分: `indra/newview/Info-Firestorm.plist` (template,经 CMake 变量已为 AYAstorm)、`indra/newview/CMakeLists.txt` 的 `MACOSX_BUNDLE_*` (已为 AYAstorm)、`indra/newview/skins/default/xui/en/floater_about.xml` (保留 Firestorm 开发组致谢 / support URL)

### Credits

r27 的本实装 (macOS 表记 audit / 修正对象 2 文件的定位 / `(based on Firestorm)` 保留的 attribution 设计 / `plutil`・`xmllint` 验证) 由 [t-noami](https://github.com/t-noami) 完成。

AYAstorm 侧将 PR 原样取入,仅追加本发布说明 (3 语言)。

### 文档

- 至 r25 的 parcel music Ogg Vorbis 修复: [`docs/release/ayastorm-r25-release-note.zh.md`](./ayastorm-r25-release-note.zh.md)
- r26 的 MOAP 音声 → 3D Stream 接入: [`docs/release/ayastorm-r26-release-note.zh.md`](./ayastorm-r26-release-note.zh.md)
