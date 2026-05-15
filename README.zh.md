[![Download](https://img.shields.io/github/v/release/mayatonton/phoenix-firestorm?label=⬇%20Download&style=for-the-badge&color=blue)](https://github.com/mayatonton/phoenix-firestorm/releases/latest)

[English](README.md) | [日本語](README.ja.md) | **中文**

<img align="left" width="100" height="100" src="indra/newview/icons/ayastorm/ayastorm_512.png" alt="AYAstorm 标志"/>

**AYAstorm 是基于 [Firestorm](https://www.firestormviewer.org) 的定制 Second Life 客户端。**
在渲染增强、界面改进和日语支持方面有独特改进。

---

## 功能

### 渲染

可在 设置 → 图形 → 渲染 标签页中配置。

![设置 - 渲染](doc/images/preferences_graphics_rendering.png)

- **阴影柔和度** — 新增滑块用于柔化阴影边缘

- **可选色调映射器** — 上游 Firestorm 在内部固定为 Khronos Neutral，AYAstorm 在界面上提供五种选择：
  - Khronos Neutral / ACES / Filmic (Uncharted 2) / Uchimura (GT) / Filmic (BD Style)

- **色彩分级控制** — 在界面上添加了饱和度 / 对比度 / 色温 / 亮度四项滑块，并提供 `Reset Color Grading` 一键重置按钮

- **Color LUT (.cube) 加载** — 通过后处理应用 `.cube` 格式的 3D LUT 进行色彩分级。内置七种预设 (`teal_orange` / `warm` / `cold_war` / `sepia` / `cool` / `cinematic` / `film_noir`)，但核心目的是 **让用户加载自己的 `.cube` 文件，自由定制画面风格**。通过 `Browse...` 选择 LUT，并使用 `LUT Intensity` 调整应用强度

### 区域

可在 设置 → Firestorm → Build 2 标签页中配置。

![设置 - Firestorm Build 2](doc/images/preferences_firestorm_build2.png)

#### 用户侧 — 隐藏区域外物体

- **`Hide objects outside your parcel`** — 不渲染当前所站区域之外的物体。**在任意区域均可启用** —— 拍摄截图时想清理掉相邻地块上碍眼的物件或招牌时非常有用。可通过 `Keep avatars visible` / `Keep my own objects visible` 保留头像 / 附件 / HUD / 自有物体

#### 区域所有者侧 — 通过描述标签强制启用

只需在 **区域 (Parcel) 的描述文本中** 写入下方标签，所有访问该区域的 **AYAstorm 用户** 都会被强制启用上述隐藏行为。**其他 Viewer (上游 Firestorm / 官方 LL Viewer 等) 不会解析此标签，因此完全不受影响** —— 也就是说，它是一个 "仅对 AYAstorm 用户生效的隐私保护标签"。区域所有者无需访问者配合即可设置。

**标签格式:**

```
[parcelhide:{key:value}{key:value}...]
```

- 可写在描述的任何位置（前后可有其他文本）
- 旧式 `[AYAstorm:...]` 格式仍可向后兼容 (r5 中重命名为 `[parcelhide:...]`)

| 键 | 默认 | 行为 |
|---|---|---|
| `hideoutside` | `true` | `false` 临时禁用此标签 |
| `keepavatars` | `false` | `true` 保留头像与 HUD |
| `keepownobject` | `false` | `true` 保留访问者自有物体 |
| `altitude` | (无) | `min-max[,min-max...]` 格式，仅当自身高度 Z 落入任一指定范围时触发 (两端 inclusive，连字符分隔，多范围以逗号分隔)。例如只对特定 skybox 楼层启用隐藏 (摄影用途) |

**示例:**

| 写入描述的字符串 | 效果 |
|---|---|
| `[parcelhide:]` | 隐藏区域外的所有内容 (包括头像和自有物体) |
| `[parcelhide:{keepavatars:true}]` | 保留头像，其他物体隐藏 |
| `[parcelhide:{keepavatars:true}{keepownobject:true}]` | 通用推荐设置 |
| `[parcelhide:{hideoutside:false}]` | 临时禁用 (例如举办活动时) |
| `[parcelhide:{altitude:1000-2000,3000-4000}]` | 仅当处于 1000-2000m 或 3000-4000m 高度时启用隐藏 (例如特定 skybox 楼层) |

**效果对比:**

<table>
<tr>
<td width="50%" align="center"><b>无标签</b><br/>(普通渲染)</td>
<td width="50%" align="center"><b>有标签</b><br/>(<code>[parcelhide:...]</code> in description)</td>
</tr>
<tr>
<td><img src="doc/images/parcel_magic_off.png" alt="无标签"/></td>
<td><img src="doc/images/parcel_magic_on.png" alt="有标签"/></td>
</tr>
</table>

### 音频

#### 3D Stream — 从图元 3D 定位播放音频流

将 HTTP 音频流 (SHOUTcast / Icecast / 静态 MP3 / Vorbis / Opus / FLAC) **以 3D 定位的方式从图元位置播放**。与 SL 标准的"地块级 2D BGM"不同，听者移动时声音的方向感和距离感会实时跟随。**适用于现场演出 PA / 环境音 / 多扬声器会场 / 5.1ch 会场展开** 等用途。仅在图元的 **Description (说明文字) 字段里写标签** 即可完成配置，无需 LSL 脚本。

**最简示例:**

```
[3dstream:{url:http://example.com/stream.mp3}]
```

写有此标签的图元会以 3D 定位播放该音频流。

**立体声 / 5.1ch / 多扬声器支持:**

为链接组中的各图元分配 `[3dstream-stereo:{ch:L|R|M|FL|FR|C|LFE|SL|SR}]`，可以把 L/R 拆到不同图元，或将 5.1ch 源展开到 6 个图元。完整参考 (全部键、兼容矩阵、推流端配方、故障排查) 请见下方各语言版指南。

**完整参考:**

- 🇯🇵 [3D Stream タグ書式ガイド (日本語)](doc/3dstream-tag-guide.ja.md)
- 🇬🇧 [3D Stream Tag Format Guide (English)](doc/3dstream-tag-guide.en.md)
- 🇨🇳 [3D Stream 标签格式指南 (简体中文)](doc/3dstream-tag-guide.zh.md)

### 聊天界面

可在 设置 → 聊天 → Chat Windows 标签页中配置。

![设置 - 聊天窗口](doc/images/preferences_chat_chatwindows.png)

- **移植 LL 风格聊天窗口** — Firestorm 原有的 Nearby Chat 提供 `FS V1 (纯文本)` 和 `FS V7 (现代头部)` 两种风格，功能强大，但 **要查看聊天范围内的用户必须打开另一个窗口**。AYAstorm 新增 **`LL style`**，将 Linden Lab 官方客户端 CONVERSATIONS 窗口的外观直接移植过来，**只需打开聊天窗口即可一览聊天范围内的用户**

- **发言者头像图标 (`Show mini icons in chat`)** — 在聊天行的用户名旁显示头像图标——仅靠名字文本难以快速识别发言者，附上头像后 **一眼就能看出是谁在说话**

  ![附近聊天中的小图标显示示例](doc/images/neaby_chat_miniicon_exapmpleshot.png)

- **聊天范围参与者过滤** — 仅在附近聊天列表中显示聊天范围（20米）内的用户

### 日语支持

- **修复 Linux + Mozc + Fcitx5 输入法候选窗口位置错误** — 上游 Firestorm 在 Linux 上使用 Mozc + Fcitx5 时存在严重 bug——**候选窗口会跳到屏幕左下角，导致输入实际上无法使用**。AYAstorm 修复了此问题，候选窗口现可正确显示在光标正下方。对在 Linux 上使用日语输入的用户尤为重要

- **内置四款日语字体家族** — 开箱即用，无需额外安装。可在偏好设置中切换：
  - **Noto Sans JP** — Google Noto 项目的日语无衬线字体，中性百搭，推荐默认使用
  - **IBM Plex Sans JP** — IBM 开源的企业字体，略带几何感，现代风格
  - **Alibaba Sans JP** — 阿里巴巴开源的无衬线字体，柔和友好
  - **LINE Seed JP** — LINE 公司开源的字体，带有现代设计个性

- **OTF字体支持修正** — 修复了 OTF 格式字体的加载问题（上述 Alibaba / IBM Plex / LINE Seed JP 均为 OTF）

---

## 下载

最新编译版可在 **[GitHub Releases](https://github.com/mayatonton/phoenix-firestorm/releases/latest)** 下载。

| OS | 文件 | 使用方法 |
|----|------|------|
| Windows (x64) | `Phoenix-FirestormOS-AYAstorm-release_AVX2-*_Setup.exe` | NSIS 安装程序，下载后运行 |
| Linux (x64) | `Phoenix-FirestormOS-AYAstorm-release_LEGACY-*.tar.xz` | 解压到任意位置，运行其中的 `install.sh` |
| macOS | （即将推出） | — |

> **若 CPU 不支持 AVX2 (仅 Windows)**: 运行上述 AVX2 版本时，安装程序会在安装开始前提示该问题。请改为下载 `Phoenix-FirestormOS-AYAstorm-release_LEGACY-*_Setup.exe`。AVX2 在 2013 年之后的 Intel / AMD CPU 上基本均支持，因此大多数用户使用 AVX2 版本即可。

**Linux 安装示例:**

```bash
tar xf Phoenix-FirestormOS-AYAstorm-release_LEGACY-*.tar.xz
cd Phoenix-FirestormOS-AYAstorm-release_LEGACY-*/
./install.sh
~/ayastorm/ayastorm
```

---

## 编译说明

AYAstorm 专用编译步骤 (Linux / Windows):

- [AYAstorm 编译指南](doc/building_ayastorm.md)

上游 Firestorm 编译指南 (Mac 请参考此处):

- [Windows](doc/building_windows.md)
- [Mac](doc/building_macos.md)
- [Linux](doc/building_linux.md)

---

## 基于

AYAstorm 是 [Phoenix Firestorm](https://www.firestormviewer.org) 的分支项目，而 Firestorm 本身是基于 [Second Life](https://github.com/secondlife/viewer) 官方客户端的开源项目，采用 LGPL 许可证。
