# 3D Stream — 使用指南

> **Language / 言語 / 语言**: [English](./3dstream-user-guide.en.md) · [日本語](./3dstream-user-guide.ja.md) · **中文**

**搭载版本**: AYAstorm r31 及以后。r32 以后，3D Stream 默认关闭。

**对象**: 直播者、DJ、现场会场所有者，以及展览、影院、活动空间的制作者。

**相关参考**: 标签的全部键和详细规格，请参阅 [3D Stream 标签格式指南](../guides/3dstream-tag-guide.zh.md)。

---

## 1. 什么是 3D Stream?

3D Stream 可以把图元当作扬声器，将 HTTP 音频流或 Media-on-a-Prim (MOAP) 的音频作为 3D 空间定位声音播放。

普通地块音乐不管听者站在哪里，听起来都是同样的音量。3D Stream 则从扬声器图元的位置发声，听者靠近时变大，远离时变小。

主要用途：

- 现场会场的左右扬声器
- 影院、展览会场的多点扬声器
- 5.1ch 音源的空间布置
- 将 MOAP 画面的音频从画面位置或会场扬声器播放

## 2. 播放前：启用和允许

3D Stream 默认关闭。使用时，请从状态栏的 3D Stream 按钮、音量弹出窗口中的 3D Stream 复选框，或 **Preferences > Sound** 中的 3D Stream 项目启用。

使用 `{url:...}` 的 3D Stream 在第一次播放该 URL 前会显示确认对话框。只有用户允许后才会开始播放。如果用户拒绝，该 URL 在同一个 viewer session 内不会播放，也不会对同一个 URL 反复询问。

URL source 可使用的 scheme 是 `http://` 和 `https://`。其他 scheme 不会播放。

`{source:media}` 是把同一 linkset 内的 MOAP / media 面音频传给 3D Stream。它不会让 3D Stream 直接打开新的 URL，因此遵循普通 media 显示和播放的许可流程。

## 3. 最小配置：一个图元发声

在图元的 **Description** 中写入：

```text
[3dstream:{url:http://example.com/stream.mp3}]
```

这种情况下，该图元本身就是扬声器。即使指定的是立体声音源，单个图元也会混合为 mono 播放。

如需调整距离衰减：

```text
[3dstream:{url:http://example.com/stream.mp3}{min:2}{max:40}]
```

`min` 是保持 100% 音量的近距离，`max` 是音量到 0% 的远距离。

## 4. 放置左右扬声器

链接两个以上图元，并在根图元和子图元中写入各自的角色。

Root Description:

```text
[3dstream:{url:http://example.com/stream.mp3}{range:30}{ch:L}]
```

Child prim Description:

```text
[3dstream:{ch:R}]
```

这样 Root 成为左扬声器，子图元成为右扬声器。左右定位由各图元的实际位置决定，而不是由链接编号决定。

同一个声道也可以分配给多个扬声器。例如，在多个图元中写 `{ch:L}`，它们都会作为 L 扬声器播放。

## 5. 5.1ch / 多点扬声器布置

布置 5.1ch 音源时，为各声道放置对应的图元。

Root Description:

```text
[3dstream:{url:http://example.com/live_5_1.opus}{range:30}]
```

各扬声器图元：

```text
FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```

5.1ch 配信可以使用 Vorbis 6ch 或 Opus 6ch。直播时，Opus 6ch 是较实用的路径。希望以 Opus 6ch 配信的音乐人和 DJ 可以使用 t-noami 制作的 [SurroundStreamer](https://github.com/t-noami/SurroundStreamer)。

如果要把 stereo 配信展开到 6 个扬声器，在 Root 标签中追加 `{upmix:on}`。

```text
[3dstream:{url:http://example.com/stereo.ogg}{upmix:on}{range:30}]
```

即使 5.1ch 音源上写了 `{upmix:on}`，viewer 检测到 6ch source 后也会自动 bypass upmix。

## 6. 使用 MOAP / media 音频作为 3D Stream 音源

也可以不用 HTTP URL，而是把同一 linkset 内的 media 面作为音源。

Root Description:

```text
[3dstream:{source:media}{ch:L}]
```

Child prim Description:

```text
[3dstream:{ch:R}]
```

如果 linkset 中有多个 media 面，请在 Root 侧用 `{link:N}` / `{face:N}` 指定目标面。

```text
[3dstream:{source:media}{link:3}{face:2}{range:30}]
```

`{url:...}` 与 `{source:media}` 不能在同一个 3D Stream 音源声明中同时指定。如果想显示 media 画面，同时把另一个 URL stream 以 3D 方式布置，请让 3D Stream 继续使用 `{url:...}`，media 面则作为普通 MOAP 处理。

## 7. 声音调整

常用调整：

| 指定 | 用途 |
|---|---|
| `{range:N}` | 扬声器的可听距离 |
| `{volume:N}` | 每个扬声器的音量，范围 `0.0` 到 `1.0` |
| `{bin:on}` | 启用面向耳机的定位修正 |
| `{v:NAME}` | 指定会场残响 preset |
| `{wg:N}` | 残响 wet 成分量 |
| `{upmix:on}` | 将 stereo source 展开到 5.1ch 布置 |

`binaural` 未指定时为 `off`，`venue` 未指定时为 `dry`。建议先不追加处理，只确认扬声器位置；需要时再加 `{bin:on}` 或 `{v:...}`。

使用会场残响时，配信音源本身不要加入过强的 reverb，会更容易调整。

## 8. 其他 Viewer 中的表现

3D Stream 标签是 AYAstorm 专用功能。本家 Firestorm、官方 Viewer、Catznip 等不会将其解释为 3D Stream。

- `{url:...}` 的 3D Stream 音频在其他 Viewer 中不会播放
- `{source:media}` 时，media 面本身仍会作为普通 MOAP 显示和播放
- `{ch:...}` routing、upmix、binaural、venue reverb 只对 AYAstorm 用户生效
- 如果设置了地块 BGM，其他 Viewer 中仍会照常听到

## 9. 常见问题

| 现象 | 确认事项 |
|---|---|
| 没有声音 | Preferences 中 3D Stream 是否启用，音量是否不是 0 |
| 出现 URL stream 确认 | 新的 `{url:...}` source 在播放前需要允许 |
| 只有左或右一边有声音 | linkset 中是否同时存在 `{ch:L}` 和 `{ch:R}` |
| `{source:media}` 没有声音 | media 面是否在同一 linkset 内；需要时是否指定了 `{link}` / `{face}` |
| 5.1ch 的一部分无声 | 是否有对应的 `{ch:FL}` 等扬声器图元 |
| 删除标签后声音仍残留 | Description 通常会在 30 秒内重新评估；必要时可将 `Stream3DEnabled` 关闭一次 |
| 其他 Viewer 用户说听不到 | 3D Stream 是 AYAstorm 专用，和 MOAP、地块 BGM 是不同功能 |

确认布置时，启用 **Preferences > Sound > Show channel routing diagnostics in chat**，即可在 Local Chat 中查看哪个图元正在播放哪个声道。

## 10. 详细参考

- 标签全部键和错误信息: [3D Stream 标签格式指南](../guides/3dstream-tag-guide.zh.md)
- Opus 6ch 配信工具: [SurroundStreamer](https://github.com/t-noami/SurroundStreamer)
