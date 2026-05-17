🌐 Language: [🇺🇸 English](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r24-github-release-page.en.md) | [🇯🇵 日本語](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r24-github-release-page.ja.md) | [🇨🇳 中文](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r24-github-release-page.zh.md)

# AYAstorm r24 — MOAP audio to FMOD 2D channel + Parcel-bound 3D stream + 视觉真实感章 (r14–r20) + 基于 tag 的 OBB 遮蔽 + GPU self-rigged picker + chat 标签拆分

从 r12.1 直接跳到 r24, 一并打包 12 个 release (r13〜r24) 至此 tag。各 release 的详细请参考 repo 内的语言别 release note (tag pin 的 permalink — 后续 docs 更新也不会断链)。

## Release notes (各 release × 语言)
- 🇺🇸 English: [docs/ayastorm-r24-release-note.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r24-release-note.en.md)
- 🇯🇵 日本語: [docs/ayastorm-r24-release-note.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r24-release-note.ja.md)
- 🇨🇳 中文: [docs/ayastorm-r24-release-note.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r24-release-note.zh.md)

### Media audio (CEF/MOAP)
- **r24** — MOAP audio 接入 viewer 内 FMOD 2D channel (本 tag 的主功能、@t-noami 实现 + AYAstorm 侧 build-time fallback switch): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r24-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r24-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r24-release-note.zh.md)

### 音响章 close-out
- **r13** — 基于 tag 的 OBB 遮蔽 (音响章旗舰): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r13-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r13-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r13-release-note.zh.md)
- **r23** — Parcel-bound 3D stream: [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r23-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r23-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r23-release-note.zh.md)

### 视觉真实感章 (r14–r20)
- **r14** — Volumetric atmosphere (章节开幕、`AYAVisualRealismEnabled` master switch): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r14-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r14-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r14-release-note.zh.md)
- **r15** — Godrays: [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r15-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r15-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r15-release-note.zh.md)
- **r16** — Aerial perspective (Rayleigh λ⁻⁴ 波长依存 haze): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r16-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r16-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r16-release-note.zh.md)
- **r17** — 时间带色温 (Kelvin 曲线、夕烧暖色): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r17-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r17-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r17-release-note.zh.md)
- **r18** — Cloud volumetric + 色温联动 (A 轴完走): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r18-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r18-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r18-release-note.zh.md)
- **r19** — Translucency (薄物透过、B 轴入口): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r19-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r19-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r19-release-note.zh.md)
- **r20** — Avatar 皮肤 SSS (B 轴完走、右键学习): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r20-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r20-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r20-release-note.zh.md)

### Chat UX & picker
- **r21** — GPU self-rigged picker: [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r21-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r21-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r21-release-note.zh.md)
- **r22** — Chat 标签拆分 (Human vs System & Object): [🇺🇸 en](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r22-release-note.en.md) / [🇯🇵 ja](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r22-release-note.ja.md) / [🇨🇳 zh](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r22-release-note.zh.md)

## 主要文档 (tag pin)

### Media audio (CEF/MOAP)
- r24 MOAP audio to FMOD 2D channel spec: [docs/ayastorm-r24-moap-audio-to-fmod-2d.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r24-moap-audio-to-fmod-2d.md)
- Dullahan fork release (audio callback API): [t-noami/dullahan v1.26.0-CEF_139.0.40-ayastorm-audio-callback.3](https://github.com/t-noami/dullahan/releases/tag/v1.26.0-CEF_139.0.40-ayastorm-audio-callback.3)

### 音响章
- r13 OBB occlusion spec: [docs/ayastorm-r13-occlusion.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r13-occlusion.md) / [docs/specs/spec_obb_occlusion.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/specs/spec_obb_occlusion.md)
- r23 parcel-bound 3D stream spec: [docs/ayastorm-r23-parcel-bound-3d-stream.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r23-parcel-bound-3d-stream.md)
- 累积 3D stream tag 指南 (r6 以降): [docs/guides/3dstream-tag-guide.en.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/guides/3dstream-tag-guide.en.md) / [.ja.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/guides/3dstream-tag-guide.ja.md) / [.zh.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/guides/3dstream-tag-guide.zh.md)

### 视觉真实感章
- 视觉真实感章路线图 (A/B 轴 overview): [docs/ayastorm-visual-realism-roadmap.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-visual-realism-roadmap.md)
- r14 volumetric atmosphere spec: [docs/ayastorm-r14-volumetric-atmosphere.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r14-volumetric-atmosphere.md)
- r15 godrays spec: [docs/ayastorm-r15-godrays.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r15-godrays.md)
- r16 aerial perspective spec: [docs/ayastorm-r16-aerial-perspective.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r16-aerial-perspective.md)
- r17 color temperature spec: [docs/ayastorm-r17-color-temperature.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r17-color-temperature.md)
- r18 cloud volumetric spec: [docs/ayastorm-r18-cloud-volumetric.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r18-cloud-volumetric.md)
- r19 translucency spec: [docs/ayastorm-r19-translucency.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r19-translucency.md)
- r20 avatar skin SSS spec: [docs/ayastorm-r20-avatar-skin-sss.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r20-avatar-skin-sss.md) / [docs/specs/spec_avatar_skin_sss.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/specs/spec_avatar_skin_sss.md)
- Deferred shader routing 参考: [docs/ayastorm-deferred-shader-routing.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-deferred-shader-routing.md)

### Chat UX & picker
- r21 GPU self-rigged picker spec: [docs/ayastorm-r21-self-rigged-picker.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r21-self-rigged-picker.md)
- r22 chat 标签拆分 spec: [docs/ayastorm-r22-chat-tab-spec.md](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/docs/ayastorm-r22-chat-tab-spec.md)

## 与既有环境的兼容性

全部功能在不破坏 r12.1 环境的前提下出货:

- **r24 MOAP audio to FMOD 2D channel**: 发布二进制以 ON 模式 (t-noami fork Dullahan + FMOD 2D 路径) 构建。Viewer 的 Media volume / mute 现在能直接对 MOAP / Web / HTML5 / YouTube 等 CEF media 生效。macOS 实机验证完成,Windows / Linux 的 ON 模式构建与播放验证作为 release 后续逐步进行。OFF 模式构建则回到 upstream `secondlife/dullahan` + 旧 native output 路径。
- **r23 parcel-bound 3D stream**: 未立 `PARCEL_FLAG_SOUND_LOCAL` 的配信者无变化。立有 SOUND_LOCAL ON 的 parcel 现在会正确停止漏到邻接 parcel — 这与 gesture / object sound 一直以来的行为一致。
- **r14〜r20 视觉真实感**: master switch `AYAVisualRealismEnabled` (default ON)。设为 OFF 即可回到 r12.1 的视觉基准。
- **r21 self-rigged picker**: GPU object-ID buffer 替换 CPU self-rigged picker。无需设置, macOS 用 kill-switch `FSSelfRiggedPickerGPU` 保留。
- **r22 chat 标签拆分**: 既有 Nearby Chat / IM history 保持不变, Human / System & Object 标签各自有未读徽章。Nearby Chat default ON, IM default off (用户反馈反映)。
- **r13 OBB occlusion**: 通过 build tag 配信者 opt-in, listener 侧除非 venue 立了 tag 否则无变化。

## IR 许可

r11 同捆的 venue IR (现在仍在出货) 来自 OpenAIR (CC-BY 4.0)。来源参见 [`app_settings/venue_ir/CREDITS.md`](https://github.com/mayatonton/phoenix-firestorm/blob/v7.2.4-ayastorm-r24/indra/newview/app_settings/venue_ir/CREDITS.md)。

## 下载

_3 OS 构建完成后, @mayatonton 填写。_

- [Windows Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r24/Phoenix-FirestormOS-AYAstorm-release_AVX2-7-2-4-261360346_Setup.exe)
- [macOS Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r24/Phoenix-FirestormOS-AYAstorm-release_arm64-7-2-4-261361150.dmg)
- [Linux Installer](https://github.com/mayatonton/phoenix-firestorm/releases/download/v7.2.4-ayastorm-r24/Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-261361212.tar.xz)

## Contributer

@t-noami @mayatonton
