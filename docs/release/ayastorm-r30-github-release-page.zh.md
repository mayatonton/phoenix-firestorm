🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-github-release-page.zh.md)

# AYAstorm r30 — 摄影级渲染引擎作为新 AYAstorm View 发布 + parcel music Ogg Vorbis fix + MOAP 3D stream routing + macOS 品牌统一 + 他人 avatar rigged picker

从 r24 到 r30 的一括发布 tag。将 5 个 release (r25 / r26 / r27 / r28 / r30) 同捆为 1 个 tag。r29 在章节切换中跳过。核心是 r30 —— r30 章 (P1–P6) 构建的摄影级渲染引擎作为新的 "AYAstorm View" 出货,r14–r20 的视觉真实感层重新部署为新引擎之上的 opt-in 追加功能。各 release 的完整 note 参见 repo 内的 per-language release note (tag-pinned permalink —— 即使 docs 后续更新也不会失效)。

## Release notes (release × 语言)
- 🇺🇸 English: [docs/release/ayastorm-r30-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.en.md)
- 🇯🇵 日本語: [docs/release/ayastorm-r30-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.ja.md)
- 🇨🇳 中文: [docs/release/ayastorm-r30-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.zh.md)

### 摄影渲染章 (r30)
- **r30** — View Mode picker reshuffle: Cinematic 提升为新的 AYAstorm View。velocity buffer / SMAA T2x / Volumetric Light / BD 级 DoF chain / Motion Blur / Chromatic Aberration / 35 cvar 的 AYAstorm Controls floater。模式切换需要重启,r14–r20 AYAstorm View 的 1-shot migration: [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r30-release-note.zh.md)

### Media audio (parcel music / MOAP)
- **r25** — parcel music Ogg Vorbis live stream 播放修复 (r10.x-bugfix-1 以来的 regression,Ogg Opus 支持保留): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r25-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r25-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r25-release-note.zh.md)
- **r26** — MOAP audio 通过 3D Stream 的 speaker routing 路由 (`{ch:L}/{ch:R}/{ch:FL}/...` 分布式 stereo / 5.1 定位 / HRTF / venue reverb / occlusion 在 MOAP 面上也工作): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r26-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r26-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r26-release-note.zh.md)

### Cross-platform polish
- **r27** — macOS 品牌统一 (menu bar / window title / Apple About panel 显示 "AYAstorm",About 内 `(based on Firestorm)` 归属保留): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r27-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r27-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r27-release-note.zh.md)

### UX & picker
- **r28** — 他人 rigged picker (将 r21 GPU object-ID picker 扩展到他人 avatar 的 rigged attachment,同时 armed 1 人,by @t-noami): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r28-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r28-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/release/ayastorm-r28-release-note.zh.md)

## 主要文档 (tag-pinned)

### 摄影渲染章 (r30)
- r30 release 决定的单一真相来源: [docs/specs/ayastorm-r30-view-mode-reshuffle.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r30-view-mode-reshuffle.md)
- r30 章 status block (P1–P6 脉络,前指 reshuffle): [docs/specs/ayastorm-r30-cinematic-chapter.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r30-cinematic-chapter.md)
- P1 重启切换基础设施: [docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r30-p1-view-mode-restart-switch.md)
- BD live cvar port reference: [docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md)
- Cinematic Controls floater audit: [docs/specs/ayastorm-r30-cinematic-controls-cleanup.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r30-cinematic-controls-cleanup.md)
- Skin SSS 使用指南 (面向 avatar 摄影): [docs/specs/skin-sss-user-guide.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/skin-sss-user-guide.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/skin-sss-user-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/skin-sss-user-guide.zh.md)

### Media audio
- r25 parcel music Ogg Vorbis investigation: [docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/ayastorm-r25-parcel-music-ogg-vorbis-investigation.md)
- r26 MOAP 3D stream implementation plan: [docs/ayastorm-r26-moap-3d-stream-implementation-plan.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/ayastorm-r26-moap-3d-stream-implementation-plan.md)
- 3D stream tag guide (r6 以来累积): [docs/guides/3dstream-tag-guide.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/guides/3dstream-tag-guide.en.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/guides/3dstream-tag-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/guides/3dstream-tag-guide.zh.md)

### UX & picker
- r28 他人 rigged picker spec: [docs/specs/ayastorm-r28-other-rigged-picker.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r28-other-rigged-picker.md)
- r21 self rigged picker spec (姐妹功能): [docs/specs/ayastorm-r21-self-rigged-picker.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/ayastorm-r21-self-rigged-picker.md)
- Rigged Mesh Picker — GPU object-ID buffer 技术资料 (供其他 viewer fork 取用): [docs/specs/rigged-mesh-picker-gpu-buffer.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/rigged-mesh-picker-gpu-buffer.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/rigged-mesh-picker-gpu-buffer.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/docs/specs/rigged-mesh-picker-gpu-buffer.zh.md)

## 公开披露: 二重 alpha block fix (其他 SL viewer 通用 bug 公开资料)

LL / Firestorm / Alchemy / BlackDragon 所有 viewer 共通存在的 forward alpha BLEND 渲染 bug ("二重 alpha block") 与 AYAstorm 采用的 2 行修复方案,以 3 语言公开资料 + 验证截图的形式公开,以便其他 viewer fork 无需 PR 即可取入。专用 reference branch `fix/double-alpha-block` 作为永久参照保持 (HEAD 跟踪最新 doc revision):

- 一次资料 (英语): [docs/specs/double-alpha-block-fix.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.md)
- 日本語: [docs/specs/double-alpha-block-fix.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.ja.md)
- 中文: [docs/specs/double-alpha-block-fix.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/fix/double-alpha-block/docs/specs/double-alpha-block-fix.zh.md)
- Reference branch: [fix/double-alpha-block](https://github.com/mayatonton/phoenix-firestorm/tree/fix/double-alpha-block)

## 与现有 setup 的兼容性

所有功能不破坏 r24 setup 出货:

- **r30 view-mode reshuffle**: r24 时代的 AYAstorm View 用户 (`AYAVisualRealismEnabled = 1`) 在首次启动时通过幂等 one-shot migration 静默迁移到新的 AYAstorm View (`= 2`)。无用户提示。r14–r20 视觉真实感层通过 AYAstorm Controls floater (`Alt+C`) 作为新引擎之上的 opt-in 追加功能到达。Firestorm View 切换继续用于直播/低资源用途。模式切换需要重启
- **r25 parcel music Ogg Vorbis fix**: Icecast Ogg Vorbis live stream 正常播放。Ogg Opus 支持完整保留。无需设置
- **r26 MOAP 3D stream routing**: 既有 3D Stream tag 约定 (`{ch:L}/{ch:R}/{ch:FL}/...`) 适用于 MOAP 面 URL。未使用 tag 的配信者不受影响
- **r27 macOS 品牌**: macOS 上纯表面 rename。无功能变更,非 macOS build 不受影响
- **r28 他人 rigged picker**: GPU object-ID buffer pass 扩展到他人 avatar。主开关 `FSOtherRiggedPickerEnable` 默认 ON,`FSOtherRiggedPickerGPU` kill-switch 保留。默认相机 gate 和 1.0 秒 arm window 约束负载。r21 self picker 不变

## IR 许可

r11 同捆以来持续出货的 venue IR 来源于 OpenAIR (CC-BY 4.0)。来源在 [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r30/indra/newview/app_settings/venue_ir/CREDITS.md)。

## 渲染引擎 credit

新的 AYAstorm View pipeline 大量借鉴自 NiranV Dean 的 Black Dragon viewer (LGPL-2.1,与 viewerlgpl 兼容)。credit 在 `floater_about.xml` 中明示,port spec 的 header 中保留 BD repository commit ref。

## Downloads

_3 OS 构建完成后由 @mayatonton 填写。_

- Windows Installer
- macOS Installer
- Linux Installer

## Contributors

@t-noami @mayatonton
