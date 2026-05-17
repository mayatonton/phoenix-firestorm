# 3D Stream 标签格式指南

> AYAstorm 的 **3D Stream** 功能用于把 HTTP 音频流或 Media-on-a-Prim (MOAP) 音频以 3D 空间定位的方式从图元 (prim) 播放出来。本文档是其标签格式参考手册。
>
> 本文档反映 AYAstorm `r26` 时点的最终规格，包含 r26 新增的 **media/MOAP source routing**，以及 r12 新增的功能：双耳化 (binaural) / 会场残响 (venue reverb) / stereo→5.1 上混 (upmix) / 标签短形式 (short-forms)。

---

## 目录

1. [什么是 3D Stream](#1-什么是-3d-stream)
2. [快速上手](#2-快速上手)
3. [术语](#3-术语)
4. [标签总览](#4-标签总览)
5. [单声道标签 `[3dstream:...]`](#5-单声道标签-3dstream)
6. [分散立体声 / 会场布置标签 `[3dstream-stereo:...]`](#6-分散立体声--会场布置标签-3dstream-stereo)
7. [双耳化 / 会场残响 (r12)](#7-双耳化--会场残响-r12)
8. [stereo→5.1 上混 (r12)](#8-stereo51-上混-r12)
9. [`ch` (声道) 取值参考](#9-ch-声道-取值参考)
10. [源声道数 × 标签值 兼容矩阵](#10-源声道数--标签值-兼容矩阵)
11. [推流端 (制作源 URL)](#11-推流端-制作源-url)
12. [Viewer 端设置](#12-viewer-端设置)
13. [错误通知 / 诊断](#13-错误通知--诊断)
14. [故障排查](#14-故障排查)
15. [已知限制 / 规格说明](#15-已知限制--规格说明)
16. [静态遮蔽 `[ayastorm:occlude]` (r13)](#16-静态遮蔽-ayastormocclude-r13)
17. [相关文档 / 内部规格书](#17-相关文档--内部规格书)

---

## 1. 什么是 3D Stream

Second Life 标准 Viewer 把 HTTP 音频流 (SHOUTcast / Icecast) 当作 **地块级 BGM** 进行 2D 播放，没有该声音在空间中来自何处的信息。

AYAstorm 的 **3D Stream** 功能把图元 (对象) 当作"扬声器"，让流媒体音源 **如同从该图元位置发声一样**，以 3D 定位的方式播放。当听者 (摄像机或角色) 移动时，声音的方向感和距离感会实时跟随。

主要用途：

- **现场演出 PA**: 在舞台前布置扬声器图元，让推流的音源从这些位置播放
- **环境音**: 让河畔、点唱机、电视等对象播放对应的音频
- **Media/MOAP 音源路由**: 把同一链接组内某个媒体面的音频送入 3D Stream 扬声器布置
- **立体声布置 / 多扬声器会场**: 把 L / R / 单声道分配给多个图元，把立体声铺开到空间中
- **5.1ch 源的会场展开**: 把 5.1ch 各声道分别布置到 6 个图元上 (FL / FR / C / LFE / SL / SR)

所有操作都通过 **在图元的 Description (说明文字) 字段里写标签** 完成。无需 LSL 脚本，无需 SL 服务端改动，听到 3D 音频的只有使用 AYAstorm 的用户。其他 Viewer (主线 Firestorm、官方 LL Viewer 等) 会忽略此类标签，因此不会产生兼容性问题。

---

## 2. 快速上手

### 2.1 让一个图元播放声音 (最简示例)

随意创建一个图元，在它的 **Description 字段** 里写入：

```
[3dstream:{url:http://example.com/stream.mp3}]
```

仅此而已。`http://example.com/stream.mp3` 这条流就会从该图元位置以 3D 定位播放。离开图元音量减小，左右移动时声像自然变化。

### 2.2 立体声布置 (把 L / R 拆到不同图元)

把立体声音源拆给两个图元，让立体声在空间中铺开。

1. 把根图元 (Root) 和子图元 (Child) 各 1 个链接起来 (Ctrl+L)
2. **Root 的 Description**:
   ```
   [3dstream-stereo:{url:http://example.com/stream.mp3}{range:30}]
   ```
3. **在 Root 的 Description 中追加** (Root 自身也作为 L 扬声器):
   ```
   [3dstream-stereo:{url:http://example.com/stream.mp3}{range:30}{ch:L}]
   ```
4. **Child 的 Description**:
   ```
   [3dstream-stereo:{ch:R}]
   ```

这样 Root 出 L、Child 出 R。

### 2.3 更详细的内容

请阅读第 3 节及之后。会讲到多扬声器、5.1ch、细节调优、推流端配方等。

---

## 3. 术语

| 术语 | 含义 |
|---|---|
| **流 (stream)** | 通过 HTTP 推送的音频数据 (SHOUTcast / Icecast / 静态文件等)。主要支持的 codec：MP3 / Vorbis / Opus / FLAC |
| **链接组 (linkset)** | 用 SL 的 "link" 操作 (Ctrl+L) 合并的图元集合。1 个根 + N 个子图元 |
| **根图元 (root prim)** | 链接组的父图元。在 Build → Edit 中关闭 "Edit linked" 时点击会先选中的那一个 |
| **子图元 (child prim)** | 链接组中除根之外的图元 |
| **音源声明** | Description 中含 `{url:...}` 或 `{source:media...}` 的根图元。声明 "使用哪一个音源"。**只能写在根图元上** (写在子图元上会被忽略) |
| **URL 音源** | 根图元用 `{url:...}` 声明的 HTTP 音频流 |
| **media/MOAP 音源** | 根图元用 `{source:media}` / `{source:media-stereo}` / `{source:media-5-1}` 声明的 Media-on-a-Prim 音频。媒体面可以在同一链接组内的根图元或子图元上 |
| **扬声器图元** | Description 中含 `{ch:...}` 的图元。实际发声的图元。**根图元、子图元都可以** |
| **binding (绑定)** | 内部按链接组组装的"音源 → 扬声器组"对应关系。1 个链接组 = 1 个 binding |
| **ch (声道)** | 扬声器图元负责的音频声道。`L` / `R` / `M` (单声道)，以及 5.1ch 用的 `FL` / `FR` / `C` / `LFE` / `SL` / `SR` |
| **rolloff (距离衰减)** | 听者远离扬声器时音量逐渐减小的设置 |

---

## 4. 标签总览

### 4.1 三种标签

| 标签 | 前缀 | 用途 |
|---|---|---|
| **单声道标签** | `[3dstream:...]` | 单个图元播放 1 条流 (最小配置) |
| **分散立体声 / 会场布置标签** | `[3dstream-stereo:...]` | 链接组中多个图元同步播放 1 个音源 (URL 流或 media/MOAP；立体声 / 多扬声器 / 5.1ch) |
| **静态遮蔽标签** (r13 新增) | `[ayastorm:occlude]` | 把墙 / 门 / 地板 / 天花等图元标记为"阻挡声音的物体" (面向会场运营 / 建造者，详见 §16) |

### 4.2 旧前缀的别名

两种标签都把旧前缀 (`[ayastream:...]` / `[ayastream-stereo:...]`) 作为 **永久别名** 接受。r5 (2026-05) 把 `ayastream` 重命名为 `3dstream` 时，为了让此前已布置好的图元不必重新编辑而保留了旧前缀。**新内容推荐使用 `3dstream`** 系列，但混用也没问题。

```
[3dstream:{url:...}]              ← 推荐 (canonical)
[ayastream:{url:...}]             ← 旧式，兼容接受

[3dstream-stereo:{url:...}{ch:L}] ← 推荐 (canonical)
[ayastream-stereo:{ch:L}]         ← 旧式，兼容接受
```

### 4.3 通用书写规则

- **标签可以写在 Description 的任何位置**。前后的其他文字会被忽略 (例如 `店名 [3dstream:{url:...}] 营业中` 之类的与说明文字共存均可)。
- 字段是 `{key:value}` 形式的集合，字段间不需要分隔符 (空格、逗号、连写都行)。
- **键名不区分大小写** (内部正规化为小写)。`{URL:...}` 与 `{url:...}` 等价。
- **值前后的空格会被 trim**。`{ url : http://example/  }` 也可以。
- **未知键被静默忽略**。例如 `{foo:bar}` 没有效果，但也不会报错。
- 同一个图元的 Description 中写了 **多个同种标签时只采用最先出现的那一个**。

### 4.4 SL 的 Description 字数限制 (127 字节)

LSL `llSetObjectDesc` 能写入的 Description **上限为 127 字节**。包含中文 / 日文等多字节字符时，按 UTF-8 编码很容易超过。因此长 URL 应当 **缩短**，或采用根写音源声明、子图元只写 `{ch:...}` 的分散方式 (§6) 来周转。**常用键名 / venue 取值还提供短形式** (见 §4.5)。

### 4.5 键名 / venue 取值的短形式 (r12 / r12.1)

`[3dstream-stereo:...]` 中，**4 个常用键** 与 **9 个 `venue` 取值全部** 在 r12 引入了 **短形式别名**。可让 Description 轻松塞进 SL 的 127 字节限制 (§4.4)。长形式与短形式 **完全等价** (内部规范化为同一形式)。新标签、既有标签都可任选其一，行为一致。

#### 键名短形式

| 长形式 | 短形式 | 含义 |
|---|---|---|
| `binaural` | `bin` | 双耳化 ON/OFF (§7.1) |
| `venue` | `v` | 会场残响预设 (§7.2) |
| `wetgain` | `wg` | 残响湿度等级 (§7.3) |
| `lfegain` | `lg` | LFE 通道增益倍率 (§7.4，r12.1 新增) |

其他键 (`url`, `source`, `link`, `face`, `ch`, `range`, `volume`, `min`, `max`, `upmix`) 本来已较短，未追加别名。

#### `venue` 取值短形式

| 长形式 | 短形式 | RT60 (大致) |
|---|---|---|
| `dry` | `d` | — (无残响) |
| `room_small` | `rs` | 0.3 s |
| `room_medium` | `rm` | 0.6 s |
| `hall_small` | `hs` | 1.0 s |
| `hall_medium` | `hm` | 1.5 s |
| `hall_large` | `hl` | 2.0 s |
| `club` | `cl` | 0.8 s |
| `cathedral` | `ct` | 3.0 s |
| `outdoor` | `od` | 0.2 s |

#### 字节数示例

长形式 (133 字节 — 超过 127 字节限制)：

```
[3dstream-stereo:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{binaural:on}{venue:hall_medium}{wetgain:1.2}]
```

短形式 (110 字节 — 装得下，节省 23 字节)：

```
[3dstream-stereo:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{bin:on}{v:hm}{wg:1.2}]
```

#### LSL 辅助脚本的行为

附带的 LSL `aya_3dstream_setup.lsl` (见 §17) **输入接受两种形式**、**输出 (Description 写入) 始终使用短形式**。通过 LSL 对话框配置的 Description 自动转换为短形式。

#### 注意事项

- 键名 **不区分大小写** (依 §4.3 通用规则)。`{BIN:on}`, `{bin:on}`, `{binaural:on}` 等价。
- 同一标签内同时写两种形式时 (例：`{binaural:on}{bin:off}`)，只采用 **最先出现的那一个** (§4.3 通用规则)。
- 路由诊断的 chat 输出与错误消息始终使用 **规范名 (长形式)** 以保持表述稳定。

### 4.6 标签生效时机

- AYAstorm **每隔 30 秒轮询** 一次范围内的图元 Description (`Stream3DPollInterval` 设置)。
- 通过 LSL `llSetObjectDesc` 修改 Description 后，下一次轮询会重新评估并生效 (通常 5〜30 秒内)。
- 手工右键图元 → Edit → 修改 Description 时，编辑确认后立刻重新评估 (通过 Properties 通知)。
- Link / Unlink 操作也会触发重新评估。

---

## 5. 单声道标签 `[3dstream:...]`

### 5.1 语法

```
[3dstream:{url:URL}{min:N}{max:N}]
```

或使用旧前缀：

```
[ayastream:{url:URL}{min:N}{max:N}]
```

### 5.2 键一览

| 键 | 必需 | 类型 | 默认值 | 含义 |
|---|---|---|---|---|
| `url` | **必需** | 字符串 | — | 流 URL (`http://` / `https://`)。空字符串视为错误 |
| `min` | 可选 | F32 (m) | `Stream3DRolloffMin` (1.0) | 距离衰减的 **近距离** (从该图元起此距离以内音量保持 100%) |
| `max` | 可选 | F32 (m) | `Stream3DRolloffMax` (20.0) | 距离衰减的 **远距离** (此距离以外音量为 0%) |

衰减模型为 FMOD 的 `FMOD_3D_LINEARSQUAREROLLOFF` (线性平方衰减)。在 `min` 与 `max` 之间平滑衰减。

### 5.3 行为

- 标签可以写在链接组中 **任意图元** (根或子)。写了标签的图元自身就是发声扬声器。
- 立体声音源会 **在内部混合 L/R 转为单声道** 播放。
- 如果同一链接组内同时还写了 `[3dstream-stereo:...]`，单声道标签 **不会** 优先用作该图元的扬声器指派 — 两条 binding 路径独立评估。不推荐把同一个图元用于两种用途 (行为未定义)。
- 单声道标签是 URL 音源专用。media/MOAP source routing 请使用分散立体声 / 会场布置标签 (§6.9)。

### 5.4 示例

#### 5.4.1 最小配置

```
[3dstream:{url:http://example.com/radio.mp3}]
```

省略 `min` / `max`，使用设置默认值 (1m / 20m) 衰减。

#### 5.4.2 自定义距离

```
[3dstream:{url:http://example.com/radio.mp3}{min:2}{max:50}]
```

距图元 2m 以内音量最大，50m 处归零。在大型户外场地希望声音传得远时使用。

#### 5.4.3 与说明文字共存

```
店内 BGM [3dstream:{url:http://radio.example.jp/8000/jazz}] 多谢光临
```

标签前后有其他文字也没问题。

---

## 6. 分散立体声 / 会场布置标签 `[3dstream-stereo:...]`

### 6.1 语法

```
[3dstream-stereo:{url:URL}{range:N}{ch:CH}{volume:V}]
[3dstream-stereo:{source:media}{link:N}{face:N}{range:N}{ch:CH}{volume:V}]
```

或使用旧前缀：

```
[ayastream-stereo:...]
```

此标签 **以整个链接组为单位处理 1 个音源**。根图元声明"使用哪一个音源"，链接组内各图元声明"自己负责哪个声道"。音源可以是 HTTP URL (`{url:...}`)，也可以是同一链接组内某个 media/MOAP 面 (`{source:media}`)。

### 6.2 图元的角色

每个图元根据标签内容承担以下角色：

| Description 中的字段 | 角色 |
|---|---|
| 含 `{url:...}` | **音源声明** (仅根图元有效，子图元写 `{url:...}` 会被忽略) |
| 含 `{source:media...}` | **media/MOAP 音源声明** (仅根图元有效，子图元写 `{source:media...}` 会被忽略) |
| 含 `{ch:...}` | **扬声器** (根 / 子图元都可以) |
| 根同时含音源声明与 `{ch:...}` | 音源声明 + 自身也作为扬声器 |
| 两者都没有 | 不做任何事 (不属于 binding 对象) |

链接组中同时存在 **音源声明 (= 根上有 `{url}` 或 `{source:media}`)** 和 **至少 1 个扬声器 (= 带 `{ch}` 的图元)** 时才会开始播放。扬声器为 0 个时会触发"结构错误"，并发出错误通知 (§13)。`{url:...}` 与 `{source:media}` **互斥**；同一个根标签中只能选择其中一种音源。

### 6.3 键一览

#### 6.3.1 仅在根图元上有效的键

| 键 | 必需 | 类型 | 默认值 | 含义 |
|---|---|---|---|---|
| `url` | 二选一 | 字符串 | — | HTTP 流 URL。空字符串视为错误。与 `source:media` 互斥 |
| `source` | 二选一 | 枚举 | — | `media` / `media-stereo` = 把 media/MOAP 面作为 2ch 音源。`media-5-1` = 作为 5.1ch / 6ch 音源。与 `url` 互斥 |
| `link` | 可选 | S32 | 自动选择 | `source:media` 时选择媒体面所在 link number。仅用于选择媒体音源，不决定扬声器顺序 |
| `face` | 可选 | S32 | 自动选择 | `source:media` 时选择媒体面所在 face number |
| `range` | 可选 | F32 (m) | `Stream3DRolloffMax` (20.0) | 链接组内扬声器没有自己 `range` 时使用的默认衰减距离 |
| `binaural` (`bin`) | 可选 | 枚举 | `off` | 双耳化 (耳机用 lite-HRTF) ON/OFF。详见 §7.1 |
| `venue` (`v`) | 可选 | 枚举 | `dry` | 会场残响预设 (dry / room_small / ... / outdoor 共 9 种)。详见 §7.2 |
| `wetgain` (`wg`) | 可选 | F32 [0.0〜2.0] | `0.2` | 残响湿度等级倍率。详见 §7.3 |
| `lfegain` (`lg`) | 可选 | F32 [0.0〜4.0] | `1.0` | LFE 通道增益倍率 (r12.1 新增)。详见 §7.4 |
| `upmix` | 可选 | 枚举 | `off` | stereo→5.1 上混 ON/OFF。详见 §8.1 |

#### 6.3.2 扬声器声明键 (任意图元)

| 键 | 必需 | 类型 | 默认值 | 含义 |
|---|---|---|---|---|
| `ch` | **必需** | 枚举 | — | 该图元负责的声道 (详见 §9) |
| `range` | 可选 | F32 (m) | 按 扬声器自身 → 根 `range` → `Stream3DRolloffMax` 顺序回退 | 该扬声器单独的衰减距离 |
| `volume` | 可选 | F32 [0.0〜1.0] | 1.0 | 该扬声器单独的音量倍率 |

> **重要**: 单声道标签的 `min` / `max` 键在分散立体声标签中 **会被忽略**。分散立体声内部固定近距离为 1.0m，远距离使用 `range` 键 (或默认值 `Stream3DRolloffMax`)。

### 6.4 1 个根 + 1 个子 (基础立体声对)

最小立体声布置：

```
根 Description:
  [3dstream-stereo:{url:http://example.com/stream.mp3}{ch:L}]

子 Description:
  [3dstream-stereo:{ch:R}]
```

根负责 L，子负责 R。根与子的 **链接顺序 (link number)** 不影响播放。在空间中放在哪里决定了定位。

### 6.5 多扬声器 (4 个以上)

把同一立体声流从场地 4 个角的扬声器播放：

```
根 Description:
  [3dstream-stereo:{url:http://example.com/stream.mp3}{range:30}]

子 #1 Description:
  [3dstream-stereo:{ch:L}{range:50}]

子 #2 Description:
  [3dstream-stereo:{ch:R}{range:50}]

子 #3 Description:
  [3dstream-stereo:{ch:L}{volume:0.7}]

子 #4 Description:
  [3dstream-stereo:{ch:R}{volume:0.7}]
```

- 根仅做音源声明，自身不发声 (没有 `{ch}`)
- 子 #1、#2 分别承担 L/R，近距 50m，音量 100%
- 子 #3、#4 把同一 L/R 以 70% 音量播出 (前段补充)
- 扬声器数上限由 `Stream3DStereoMaxSpeakers` 设置控制，**默认 16 个**(§12)

### 6.6 5.1ch 会场布置 (6 图元)

把 5.1ch 源 (Opus surround / FLAC 6ch) 展开到 6 个扬声器：

```
根 Description:
  [3dstream-stereo:{url:http://example.com/test_5_1.flac}{range:30}]

FL 图元:  [3dstream-stereo:{ch:FL}]
FR 图元:  [3dstream-stereo:{ch:FR}]
C 图元:   [3dstream-stereo:{ch:C}]
LFE 图元: [3dstream-stereo:{ch:LFE}]
SL 图元:  [3dstream-stereo:{ch:SL}]
SR 图元:  [3dstream-stereo:{ch:SR}]
```

- 把每个图元物理摆放到 "会场扬声器位置" (舞台前 L/R、中置、低音炮、环绕 L/R)
- LFE 与其余 5 个一视同仁 (Viewer 端不做低通滤波之类的特殊处理。如需低频限制请在推流端 mix 时完成)
- 听者没有 "影院最佳位"概念 (= SL 自由视角模型)。在场地中走动时 5.1 mix 的预定定位当然会破坏。请按 **"会场 PA 风格的多点布置"**，而不是"影院环绕体验"来运用

### 6.7 把同一 ch 分配给多个图元

把 `{ch:L}` 写到 2 个或更多图元，多个图元都会播放 L 声道。可用于"舞台前排 L"和"舞台后排 L"等多个扬声器的场景。

反之，如果没有任何图元写 `{ch}`，会触发 "扬声器 0 个" 结构错误。

### 6.8 根图元同时兼任扬声器

```
根 Description:
  [3dstream-stereo:{url:http://example.com/stream.mp3}{ch:M}{range:25}]
```

像这样在 1 条标签中并写音源声明 (`{url}` 或 `{source:media}`) 与 `{ch}` 时，根作为音源声明，自身也作为 M (单声道) 扬声器工作。也可用于完全没有子图元的简易 mono 配置 (与 `[3dstream:...]` 在功能上几乎等价；但 `[3dstream:...]` 是 URL 音源专用)。

### 6.9 Media/MOAP 音源的使用 (r26)

r26 起，分散立体声 / 会场布置标签可以把同一链接组内的 Media-on-a-Prim 音频作为 3D Stream 音源。Root 的 Description 使用 `{source:media}` 或 `{source:media-5-1}`：

```
根 Description:
  [3dstream-stereo:{source:media}{ch:L}{range:30}]

子 Description:
  [3dstream-stereo:{ch:R}]
```

media 面可以在根图元上，也可以在同一链接组内的子图元上。标签仍然写在 **根图元** 上；扬声器图元照旧只写 `{ch:...}`。如果链接组内只有 1 个 media 面，可以省略 `{link}` / `{face}`，viewer 会自动选择它。

如果同一链接组内有多个 media 面，或音源 media 面在子图元上，请在根标签中用 `{link:N}{face:N}` 明确选择。下面例子选择的是 link 3 的子图元 face 2：

```
根 Description:
  [3dstream-stereo:{source:media}{link:3}{face:2}{range:30}]

FL 图元: [3dstream-stereo:{ch:FL}]
FR 图元: [3dstream-stereo:{ch:FR}]
C 图元:  [3dstream-stereo:{ch:C}]
```

`link` / `face` 只用于选择 **哪一个 media/MOAP 面作为音源**，不决定扬声器顺序，也不替代 `{ch:...}`。有多个 media 面但没有写 `{link}` / `{face}`，或写出的组合找不到唯一 media 面时，会触发结构错误。

URL 音源与 media 显示可以共存。根上使用 `{url:...}` 时，3D Stream 扬声器播放 URL 流；对象上的 media/MOAP 音频仍按普通媒体音频播放，不会被自动改路由：

```
根 Description:
  [3dstream-stereo:{url:http://example.com/live.ogg}{ch:L}]

子 Description:
  [3dstream-stereo:{ch:R}]
```

media 音量 / mute 的作用规则：

- 链接组内只有 1 个 media 面且被 `{source:media}` 使用时，media 自身的音量 / mute 作为音源增益参与 3D Stream。
- 有多个 media 面时，被选中的 media 路由到 3D Stream 后按音源增益 `1.0` 处理，主要由 3D Stream 总音量 / 扬声器 `volume` 控制；未选中的 media 面保持普通 media 音量行为。
- `{url:...}` 与 `{source:media}` 互斥。要让扬声器播放 media/MOAP 音频就使用 `{source:media}`；要让扬声器播放 HTTP 流就使用 `{url:...}`。

media source 的声道指定：

- `{source:media}` / `{source:media-stereo}`: 把 media 当作 2ch 音源。stereo media 需要 `{upmix:on}` 时也使用这个指定。
- `{source:media-5-1}`: 把 media 当作 5.1ch / 6ch 音源。扬声器侧布置 `FL / FR / C / LFE / SL / SR`。

本指南只说明 **2ch 与 5.1ch (6ch)** 为止的 media source 行为。即使 Dullahan/CEF callback bus 显示为 8ch，也不表示 3D Stream 已实现 7.1ch speaker routing。

### 6.10 如何识别根图元

编辑链接组时，Build 浮窗的 **Object** 选项卡里 "Selected" 会显示当前选中的图元，链接组的父图元 (= 根) 通常是 **最初被选中并发起链接的那一个**。

最可靠的确认方法：
- Build → Edit → 关闭 "Edit linked" → 点击任一图元 → 选中的就是该链接组的根
- LSL: `llGetLinkNumber()` 在子图元存在时根返回 `1`。没有子图元的单一图元返回 `0`

根与子的 link number (1, 2, 3, ...) **不影响 3D Stream 的播放**。r5 之前用 link number 决定 L/R 的旧规格已废弃，r8 起改为 `{ch:...}` 标签声明制。

---

## 7. 双耳化 / 会场残响 (r12)

> r12 新增 3 个键 (`binaural` / `venue` / `wetgain`)，r12.1 再追加 `lfegain`。它们对链接组中所有扬声器一致生效，仅在根图元上有意义，在既有的 3D 定位之上叠加更自然的"现场会场 / 厅堂感"。

#### 为什么仅根有效？

这些键描述的是 **推流者对该次演出的意图** — "这套内容按厅堂混音"／"今天的直播是双耳化录音" — 因此在源端一次性决定，而非每个扬声器各自决定。**4 个键全部仅在根有效** (写在子图元上将被静默忽略)。听者侧 UI 中 **不暴露任何项**：听者按推流者通过标签所定的状态收听 (§7.5)。

### 7.1 `{binaural:on|off}` (短形式 `bin`)

对每个扬声器施加 **简化版双耳 HRTF**。配合耳机时可改善定位 (前后 / 上下的判别力)。

| 取值 | 含义 |
|---|---|
| `on` | 施加简化版 binaural HRTF (推荐用于耳机听众) |
| `off` | 跳过 HRTF (仅 vanilla 3D 定位) |

默认值：`off`。

#### "lite binaural" 做了什么

- 按 Woodworth-Schlosberg 公式计算 ITD (双耳间到达时间差) — 左右耳到达时间延迟
- 按空气吸收做高频衰减 (−0.5 dB/m, 上限 −25 dB) — 远距离声源的高频暗化
- **不做** ILD (双耳间响度差)、**不做** 频谱级 cone-of-confusion 修正 (那些属 r13+ SOFA 范畴)

简言之，是基于 ITD 的空间化 + 距离驱动的高频衰减组成的低成本 HRTF。每扬声器 CPU 开销约 +0.4 个百分点 (r10 基准机测定)。

#### 何时使用

- **耳机听众** = `on`。空间感比 vanilla 3D 明显清晰。
- **扬声器听众** = 任一皆可。扬声器播放时 ITD 偶尔会感觉相反 (本来是为耳朵设计的、不是给扬声器的)。听者侧救援见 §7.5 "例外情况"。
- **本身已为双耳混音的素材** (推流的就是预混过的 binaural 轨) = 设为 `off` 以避免双重处理。

### 7.2 `{venue:NAME}` (短形式 `v`)

从 9 种 **会场残响预设** 中选一种。每种预设的 RT60、EQ、早期反射模式、CPU 成本皆已固定 — 选择最贴合会场的那一种。

| 长形式 | 短形式 | RT60 | 用途 | CPU (增量) |
|---|---|---|---|---|
| `dry` | `d` | — | 无残响 (= r10 行为) | 0 (无 DSP) |
| `room_small` | `rs` | 0.3 s | 小型工作室、卧室 | +0.1 pp |
| `room_medium` | `rm` | 0.6 s | 中型工作室、谈话节目 | +0.1 pp |
| `hall_small` | `hs` | 1.0 s | Live house、小剧场 | +0.5 pp |
| `hall_medium` | `hm` | 1.5 s | 音乐厅、舞会厅 | +7.7 pp |
| `hall_large` | `hl` | 2.0 s | 大型礼堂、歌剧院 | +9.6 pp |
| `club` | `cl` | 0.8 s | 舞厅、密集早期反射 | +0.4 pp |
| `cathedral` | `ct` | 3.0 s | 大教堂、长尾环境 | +10.2 pp |
| `outdoor` | `od` | 0.2 s | 户外、极轻的早期反射 | +0.1 pp |

默认值：`dry`。

#### CPU 注意事项

`hall_medium` / `hall_large` / `cathedral` (长尾会场) 的 CPU 消耗明显高于 `dry` / `room_*`。请按所需会场感选择 — 比如 live house 可用 `hall_small` 或 `club`，不必硬上 `hall_large`。"增量" 列是每个 binding 的开销 (不是每扬声器 × N)。详见 `docs/specs/spec_binaural_venue_reverb.md`。

### 7.3 `{wetgain:N}` (短形式 `wg`)

**湿声 (残响成分)** 的倍率。范围：0.0–2.0。默认值：**0.2** (r12.1 由 1.0 改为 0.2)。

| 取值 | 效果 |
|---|---|
| `0.0` | 完全干声 (= 不论何种 preset 均与 `venue:dry` 等效) |
| `0.1` | 湿度极淡 |
| **`0.2` (默认)** | 湿度偏淡 — 音乐用途的实用基准 |
| `0.3〜0.5` | 中等到稍浓 (musical range 上限附近) |
| `1.0` 以上 | 湿度与干声等比或更高 — 音乐场景下通常显得过浓 |
| `2.0` | 湿度 200% — 厚重的环境淹没感 (极少使用) |

干声成分固定 1.0，仅湿声受 `wetgain` 缩放。完全干声请用 `{venue:dry}` (与 `{wetgain:0.0}` 等效，但规格上更干净)。

> **r12.1 默认值变更 (1.0 → 0.2)**：原本 `1.0` ("湿干等比") 在 hall / cathedral 等长尾预设下源声会被吞没，已脱出音乐用途的实用区间。实际试听确认 **音乐上可用的范围是 0.1〜0.5**，因此 r12.1 将默认值下调到 `0.2`。同梱 LSL UI 的快选按钮也已重排到 `0.1`〜`0.5` 的细刻度。

### 7.4 `{lfegain:N}` (短形式 `lg`，r12.1 新增)

针对 **LFE 通道** 的增益倍率 — 同时作用于 `{ch:LFE}` 路径与 `{upmix:on}` 时由 80 Hz LPF 产出的低频带。与干声 / 湿声相互独立。

| 取值 | 效果 |
|---|---|
| `0.0` | LFE 静音 (LFE 图元无输出 / upmix 时无低频强调) |
| `0.5` | LFE 减半 |
| **`1.0` (默认)** | 与素材等同 (r12 兼容行为) |
| `2.0` | LFE 双倍 (常用于希望强调低频的布局) |
| `4.0` | LFE 4 倍 (上限，sub-PA 用途) |

#### 适用场景

- **5.1 native 直播** (有 `{ch:LFE}` 图元)：当源端 LFE 总线录制偏弱时由 viewer 侧抬升
- **`{upmix:on}` 立体声 → 5.1 展开** 时，80 Hz LPF 段落感觉力道不足，可上推强调
- 反之，将 LFE 图元挂在非低音炮的普通扬声器上时，可设为 `0` 以阻止低频泄漏

#### LFE 路径未启用时

没有 `{ch:LFE}` 图元且 upmix 关闭时，LFE 路径本身不工作，`lfegain` **无效** (写了也不生效但无害)。

#### 听者侧 sentinel

debug 设置 `Stream3DLfeGain` (sentinel `-1.0` = 跟随标签，否则 `0.0〜4.0` 强制覆写) 在 r12.1 同期追加 (详见 §12.2)。位于推流者主导模型救援槽，刻意不在一般听者 UI 中暴露。

### 7.5 推流者主导模型

这 4 个键 (`binaural` / `venue` / `wetgain` / `lfegain`) 采用 **根图元 Description 即真理 (root truth)** 模型 — **一般听众没有 Preferences / Debug Settings UI**。

#### 为何无听者 UI？

- 如果推流者决定"本场为厅堂、binaural ON"，但听者却能任意覆写，就会出现"同一场直播因听者不同而声音不同"的状况 — 艺术意图变得模糊。
- AYAstorm 方针 (依 `r5 命名一致性` / `r11 推流者主导模型`) 是 **"不增加表现的不确定性"**。增加调节轴数会侵蚀运营一致性。

#### 例外：扬声器收听时的听者侧救援

使用 **扬声器而非耳机** 的听众，在收听 `{binaural:on}` 直播时可能感到 ITD 反作用。**仅为此救援目的** 提供一个 sentinel debug 设置 ─ `Stream3DBinauralRender = 0` 在听者侧强制 OFF (详见 §12.2)。同样地 `Stream3DVenueOverride` (空 = 跟随标签，`"dry"` = 强制全部残响 OFF) / `Stream3DVenueWetGain` (sentinel `-1.0` = 跟随标签) / `Stream3DLfeGain` (sentinel `-1.0` = 跟随标签，r12.1) 也作为 debug 提供。它们都不在一般用户 UI 中暴露。

### 7.6 组合示例

#### 什么都不写 (= 默认)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}]
```
→ 等价于 `{bin:on}{v:d}{wg:1.0}`。Lite-HRTF 生效，但无残响 (r10 + 定位强化)。

#### Live house (PA 取向、节奏型音乐)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:cl}{wg:1.0}]
```
→ club preset，密集反射，干湿等比。

#### 大厅 (管弦乐)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:hl}{wg:0.8}]
```
→ hall_large，湿度调到 0.8× (长厅堂残响下保留源声清晰度)。

#### 大教堂 (氛围 / 环境)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:ct}{wg:0.6}]
```
→ cathedral，湿度 0.6× (RT60 ~3s 较长，避免过厚)。

#### 户外 (环境 / 漫步 BGM)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:od}{wg:1.0}]
```
→ outdoor，仅轻量早期反射 — 露天感。

#### 已是双耳化的素材 (避免双重处理)

```
[3dstream-stereo:{url:http://example/stream.ogg}{ch:C}{bin:off}{v:d}]
```
→ binaural OFF、无残响 (= r10 基线行为)。

---

## 8. stereo→5.1 上混 (r12)

> r12 在 viewer 内 DSP 中加入 **stereo→5.1 上混 (upmix)**。把 2 声道源扩展成 6 声道 (FL / FR / C / LFE / SL / SR)，使得即便推流是普通立体声，也能享用 5.1 布置 (§6.6)。

这是 r12 单项最大的新增。由于 SL 多数推流软件 (butt / Mixxx / OBS / SAM 等) 仅支持立体声、native 5.1 推流极其稀少，**没有 upmix 时 r10 建立的 6 扬声器布置实际上只有 5.1 native 推流才能体验到**。r12 从 viewer 端补上这个缺口。

### 8.1 `{upmix:on|off}` (无短形式)

是否将立体声源 upmix 为 5.1。

| 取值 | 含义 |
|---|---|
| `on` | 通过 DPL2 系矩阵解码 + 频段分离把立体声 → 6 声道 |
| `off` | 不做 upmix (= r10 行为 — stereo 仅以 L/R 播出) |

默认值：`off`。由推流者主动开启 (opt-in)。

#### 为何默认 off (opt-in)？

- 立体声素材本来就是为立体声混音的。upmix 是诠释、不是重现。
- 如果布置了 6 prim 的听者各自决定是否 upmix，会出现"同一场直播因听者不同而声音不同" — 与 §7.5 同样的不确定性问题。
- 因此交由推流者通过标签决定。默认保留 r10 行为。

### 8.2 算法 (DPL2 系矩阵解码 + 频段分离)

内部算法固定 (NG1 — 无算法选择，不提供 "Logic 7 / SRS / ML upmix" 等选项)：

1. **DPL2 矩阵解码** — 由 L+R 推导出 C 与 S (surround) 声道，生成 L′ / R′ / C / Lₛ / Rₛ
2. **LFE 频段分离** — 对源的 mono 下混做低通 (cutoff `Stream3DUpmixLfeCutoff`，默认 80 Hz THX) 后送往 LFE
3. **中央漏出消除** — 从 L′ / R′ 中减去 `Stream3DUpmixCenterBleed` × C，避免中央成像内容同时漏到 FL/FR (默认 1.0 = 完全消除)
4. **后置 decorrelation** — 对 Lₛ / Rₛ 施加基础延迟 `Stream3DUpmixRearDelayMs` (默认 16ms)，并 ±2ms 抖动以避免后置左右间梳状滤波

结果：FL = L′, FR = R′, C, LFE, SL = Lₛ′, SR = Rₛ′。

#### 为何只有一种固定算法？

- 依 AYAstorm "不增加表现不确定性" 方针，不暴露 `{upmix:dpl2|logic7|srs|...}` 这种标签值。推流者只选 ON / OFF；其余是确定性 DSP。
- DPL2 文献完备、license 干净，跨流派结果稳定。Logic 7 / ML upmix 等留待 r13+ 在客观 FFT 与试听验证基础上重新评估。

### 8.3 5.1 native 推流的自动 bypass

源的实际声道数为 **6 或更多** 时，`{upmix:on}` 会被 **自动 bypass** (避免双重处理)，并在 chat 中通知一次：

```
3D Stream: source is already 5.1 (6ch) — upmix bypassed
```

这样推流者可以始终保留 `{upmix:on}` 标签，stereo 内容时上混生效、5.1 native 内容时自动 bypass，使用同一份 Description。切换内容时无需修改标签。

### 8.4 微调 (3 个 debug 设置)

DSP 内部参数 **不作为推流者标签暴露** — 而是作为听者侧 debug 设置。为保持推流者主导模型，推流者只选 on / off；内部参数视作"算法的一部分"固定不变。听者通常也不调整 — 仅供实现 / 验证或个人微调用。

| Debug 设置 | 默认值 | 范围 | 含义 |
|---|---|---|---|
| `Stream3DUpmixLfeCutoff` | `80.0` Hz | 20–200 | LFE LPF cutoff 频率 |
| `Stream3DUpmixCenterBleed` | `1.0` | 0.0–1.0 | 从前置 L/R 中减去的中央成分比例 (`0` = DPL1 兼容、`1` = 完全消除) |
| `Stream3DUpmixRearDelayMs` | `16.0` ms | 0–32 | 后置 decorrelation 基础延迟 (L / R 间 ±2ms 抖动) |

听者侧强制 OFF / ON 用 sentinel：

| Debug 设置 | 默认值 | 含义 |
|---|---|---|
| `Stream3DUpmix` | `-1` (sentinel = 跟随标签) | `0` 忽略标签强制 OFF / `1` 强制 ON (5.1 native 自动 bypass 仍生效) |

### 8.5 组合示例

#### 默认行为 (无 upmix)

```
[3dstream-stereo:{url:http://example/stereo.ogg}{ch:L}]
[3dstream-stereo:{ch:R}]
```
→ 与 r10 相同 (立体声源送往 L/R 2 个扬声器)。

#### 6 扬声器布置 + upmix (r12 推荐格式)

```
根 Description:
  [3dstream-stereo:{url:http://example/stereo.ogg}{upmix:on}]
FL:  [3dstream-stereo:{ch:FL}]
FR:  [3dstream-stereo:{ch:FR}]
C:   [3dstream-stereo:{ch:C}]
LFE: [3dstream-stereo:{ch:LFE}]
SL:  [3dstream-stereo:{ch:SL}]
SR:  [3dstream-stereo:{ch:SR}]
```
→ 立体声源被扩展为 6 声道，分别送往 6 个扬声器图元。

#### upmix + binaural + venue (r12 全功能)

```
[3dstream-stereo:{url:http://example/stereo.ogg}{upmix:on}{bin:on}{v:hm}{wg:1.2}]
```
→ 立体声源扩展为 6 声道，每个扬声器再施加 lite-HRTF + hall_medium 残响 (湿度 1.2×)。最大化的会场感 + 耳机定位。

#### r10 旧式布置 (`ch:L`/`ch:R` 限定) + upmix

```
根 Description:
  [3dstream-stereo:{url:http://example/stereo.ogg}{upmix:on}]
L 子: [3dstream-stereo:{ch:L}]
R 子: [3dstream-stereo:{ch:R}]
```
→ 链接组中只有 L/R 扬声器时，upmix 内部仍计算但仅 L/R 路由出去。即 "上混信号的 L/R 声道送出，C / LFE / SL / SR 计算了但未路由" → 听感上 **几乎与不开 upmix 相同** (DPL2 的 L′ / R′ ≈ L / R 减去中央漏出)。

→ 不算错，但只有 L/R 扬声器的布置 **开 upmix 没有意义**。请用 6 扬声器布置 (`ch:FL/FR/C/LFE/SL/SR`) 才能真正受益。

#### 把 upmix 加到 5.1 native 推流 (自动 bypass)

```
根 Description:
  [3dstream-stereo:{url:http://example/test_5_1.flac}{upmix:on}]
FL:  [3dstream-stereo:{ch:FL}]   # 另外 5 个扬声器
```
→ 源本就是 6 声道，因此 upmix 自动 bypass (chat 通知一次)。每个扬声器直接播放源对应的声道。

---

## 9. `ch` (声道) 取值参考

`{ch:值}` 可以指定以下取值。**不区分大小写** (`{ch:l}` 与 `{ch:L}` 等价)。

| 值 | 含义 | 主要用途 |
|---|---|---|
| `L` | 左声道 | 立体声 L |
| `R` | 右声道 | 立体声 R |
| `M` | 单声道 (L+R 平均) | "中置扬声器"用途，或单点播放 |
| `FL` | Front Left | 5.1ch 前左 |
| `FR` | Front Right | 5.1ch 前右 |
| `C` | Center | 5.1ch 中置 |
| `LFE` | Low Frequency Effects (低音炮) | 5.1ch 低频 |
| `SL` | Surround Left | 5.1ch 环绕左 |
| `SR` | Surround Right | 5.1ch 环绕右 |

源实际声道数与标签 `ch` 值不一致时，系统进行 **自动回退** 而不是报告"不一致" (§10)。例如对 2ch 源写 `{ch:FL}` 会播放 L。

非法值 (例如 `{ch:foo}`) 会触发 **格式错误** 通知 (§13)。

---

## 10. 源声道数 × 标签值 兼容矩阵

扬声器图元实际播放什么，由 **音源的声道数** 与 **写下的 `ch` 值** 的组合决定。

### 10.1 兼容矩阵

| 源 | `{ch:L}` | `{ch:R}` | `{ch:M}` | `{ch:FL}` | `{ch:FR}` | `{ch:C}` | `{ch:LFE}` | `{ch:SL}` | `{ch:SR}` |
|---|---|---|---|---|---|---|---|---|---|
| **1ch (单声道)** | M | M | M | M | M | M | 静音 | 静音 | 静音 |
| **2ch (立体声)** | L | R | (L+R)/2 | L | R | (L+R)/2 | 静音 | 静音 | 静音 |
| **6ch (5.1)** | BS.775 L | BS.775 R | (BS.775 L + R)/2 | FL | FR | C | LFE | SL | SR |

图例：
- `L` / `R` / `FL` / `FR` / `C` / `LFE` / `SL` / `SR` 表示直接播放源对应声道
- `BS.775 L` = 用 ITU-R BS.775 下混系数把 6ch 折叠到 L/R 2ch 的结果 (§10.2)
- `静音` = 该扬声器不发声 (binding 仍保持，仅图元不出声)

### 10.2 BS.775 下混系数 (6ch 源 → L/R)

```
L_out = c × ( FL + 0.707·C + 0.707·SL + 0.5·LFE )
R_out = c × ( FR + 0.707·C + 0.707·SR + 0.5·LFE )
c = 1 / 2.914 ≈ 0.343 (防削波归一化)
```

中置均匀分到左右、环绕加到同侧、LFE 平均加到两侧。

### 10.3 同一源混合布置使用

如果把 5.1ch 源同时分配给 `{ch:L}` 与 `{ch:FL}`，L 图元播 BS.775 下混、FL 图元播 直取声道。容易混淆，因此推荐 **同一源在会场内使用同一系列的 ch (`L/R/M` 系 或 `FL/FR/...` 系，二选一)**。

混合布置中发生回退时，可以通过 **routing 诊断 chat 通知** (§12.3 / §13.3) 确认每个 ch 实际行为。布置 5.1ch 会场时打开它能立刻发现配置失误。

### 10.4 5.1ch 会场布置下播放 2ch / 1ch 源

会场已部署 6 个扬声器图元 (`ch:FL` / `FR` / `C` / `LFE` / `SL` / `SR`)，把音源从 5.1ch 切到 **普通立体声 (2ch)** 或 **单声道 (1ch)** 的场景。例如 "正式演出走 5.1ch、休息时间用普通立体声 BGM"，"DJ set 之间穿插 MC 单声道语音"等运营。

此时 **完全无需重新布置或修改设置**。各扬声器图元会自动按下述方式工作。

#### 当 2ch (立体声) 源播放时

| 图元的 `ch` 值 | 播放内容 |
|---|---|
| `{ch:FL}` | **L** (代替前左，直接播放立体声 L) |
| `{ch:FR}` | **R** (代替前右，直接播放立体声 R) |
| `{ch:C}` | **(L+R)/2** (中置播放 L+R 的平均 = 单声道下混) |
| `{ch:LFE}` | **静音** (源中没有 LFE 信号) |
| `{ch:SL}` | **静音** (源中没有环绕左信号) |
| `{ch:SR}` | **静音** (源中没有环绕右信号) |

听感上："**会场前方 3 个 (FL / FR / C) 出立体声，环绕 3 个 (LFE / SL / SR) 静默**"。

#### 当 1ch (单声道) 源播放时

| 图元的 `ch` 值 | 播放内容 |
|---|---|
| `{ch:FL}` / `{ch:FR}` / `{ch:C}` | **M** (前 3 个都播放单声道，3 处发出相同的声音) |
| `{ch:LFE}` / `{ch:SL}` / `{ch:SR}` | **静音** |

#### 设计意图 / 补充

- **前方 3 个 (FL / FR / C) 无论源声道数都一定有声** ─ 因此在 5.1ch ↔ 2ch ↔ 1ch 间切换源时，会场前方不会失声。
- **当源没有对应信号时 LFE / SL / SR 固定为静音** ─ 不会合成假低音或假环绕。
- **源回到 5.1ch 时各图元自动恢复为对应声道直取** (URL 切换后重连时重新评估)。无需重新布置或重新设置。
- 5.1ch 会场长期播放 2ch BGM 是 **正规运营模式**。"环绕静音" 是规格而非故障。

#### 把回退情况输出到 Chat 的设置 (推荐: 布置 / 验证期间)

提供了一个 **可在 Local Chat 中确认** "静音的 prim 究竟是按规格回退而静音、还是出了什么问题" 的诊断开关。

**设置位置** (两者作用相同且同步):

- **Preferences > Sound > Show channel routing diagnostics in chat** (复选框)
- **Debug Settings: `Stream3DRoutingDiagnostic`** (`true` / `false`)

打开后，5.1 布置 × 2ch / 1ch 源出现回退时，会以 **Local Chat 中你自己的发言形式** 按下述格式逐行输出 (`3D Stream:` 前缀由 §13.3 helper 自动添加):

**2ch 源 × 5.1 布置 (FL/FR/C/LFE/SL/SR 6 个 prim) 时**:

```
[12:34] You: 3D Stream: ch:FL prim playing L (source is 2ch)
[12:34] You: 3D Stream: ch:FR prim playing R (source is 2ch)
[12:34] You: 3D Stream: ch:C prim playing (L+R)/2 (source is 2ch)
[12:34] You: 3D Stream: ch:LFE prim silent (source is 2ch)
[12:34] You: 3D Stream: ch:SL prim silent (source is 2ch)
[12:34] You: 3D Stream: ch:SR prim silent (source is 2ch)
```

**1ch 源 × 5.1 布置 时**:

```
[12:35] You: 3D Stream: ch:FL prim playing M (source is 1ch)
[12:35] You: 3D Stream: ch:FR prim playing M (source is 1ch)
[12:35] You: 3D Stream: ch:C prim playing M (source is 1ch)
[12:35] You: 3D Stream: ch:LFE prim silent (source is 1ch)
[12:35] You: 3D Stream: ch:SL prim silent (source is 1ch)
[12:35] You: 3D Stream: ch:SR prim silent (source is 1ch)
```

由此可一目了然地看出 "LFE / SL / SR 静音是规格行为，FL / FR / C 处于回退播放中"。

通知按 `(root_id, source_id, observed_channel_count, prim_set_signature)` 作为 throttle 的键，因此布置或源声道数不变时同一通知不会重复发送。**推荐：仅在 5.1ch 会场布置 / 验证期间打开，正式运行时关闭 (`false`，默认值)**。详见 §13.3。

#### 反方向: 2ch 布置下播放 5.1ch 源

为参考起见整理反方向。仅用 `{ch:L}` / `{ch:R}` / `{ch:M}` 布置的立体声会场 (= 2ch 布置) 播放 5.1ch 源时:

- L / R / M 图元用 **BS.775 下混** (§10.2) 把 5.1ch 折叠为 2ch 播放。FL / C / SL / LFE 都按系数加到 L 侧，FR / C / SR / LFE 都按系数加到 R 侧。
- 所有声道信号都通过 L / R 出来，因此 **没有声道会失声**。
- `Stream3DRoutingDiagnostic` 打开时 Local Chat 输出示例:

```
[12:36] You: 3D Stream: FL content folded into BS.775 downmix (source is 6ch, no ch:FL prim)
[12:36] You: 3D Stream: C content folded into BS.775 downmix (source is 6ch, no ch:C prim)
[12:36] You: 3D Stream: LFE content folded into BS.775 downmix (source is 6ch, no ch:LFE prim)
[12:36] You: 3D Stream: SL content folded into BS.775 downmix (source is 6ch, no ch:SL prim)
[12:36] You: 3D Stream: SR content folded into BS.775 downmix (source is 6ch, no ch:SR prim)
```

按声道分别通知 "没有专用 prim，所以折叠到 BS.775 下混 path"。

---

## 11. 推流端 (制作源 URL)

### 11.1 支持的 codec / 容器

| codec / 容器 | 1ch | 2ch | 6ch | 备注 |
|---|---|---|---|---|
| **MP3** | ✓ | ✓ | — | SHOUTcast / Icecast 的传统路径 |
| **Vorbis (Ogg)** | ✓ | ✓ | ✓ | 6ch 也已实机验证 (r9 P10) |
| **Opus (Ogg)** | ✓ | ✓ | △ | 6ch 用 Opus channel mapping family 1。**纯 HTTP / Icecast push 时 seek 失败** 可能无法打开 (§11.4) |
| **FLAC** | ✓ | ✓ | △ | 6ch 理论上支持，与 Opus 相同的 seek 限制 |
| AAC (ADTS / HLS) | — | — | — | 不支持 |
| AC-3 / E-AC-3 | — | — | — | 因 Dolby 授权问题不支持 |

源 URL 接受 `http://` 或 `https://`。能维持 HTTP/1.1 keep-alive 的路径 (= SHOUTcast 兼容 streamer 或 ffmpeg 的 TCP 输出) 比单纯静态 HTTP 更稳定。

### 11.2 1ch / 2ch 推流

普通 SHOUTcast / Icecast / 静态 HTTP 即可。MP3 / Vorbis / Opus / FLAC 均能正常工作。`oggenc` / ffmpeg / butt 等常规推流工具直接可用。

### 11.3 5.1ch (Vorbis 6ch) 推流

要在 Viewer 端可靠地跑 5.1ch，推荐路径是 **Vorbis 6ch** (r9 P10 已实机验证)。

#### 11.3.1 测试素材制作 (ffmpeg)

```bash
# 各声道埋入唯一频率的 5.1 WAV (10 秒)
ffmpeg -f lavfi -i "sine=440:d=10" -f lavfi -i "sine=550:d=10" \
       -f lavfi -i "sine=660:d=10" -f lavfi -i "sine=110:d=10" \
       -f lavfi -i "sine=770:d=10" -f lavfi -i "sine=880:d=10" \
       -filter_complex "[0:a][1:a][2:a][3:a][4:a][5:a]amerge=inputs=6[a]" \
       -map "[a]" -ac 6 -channel_layout 5.1 test_5_1.wav

# 编码为 Vorbis 6ch
ffmpeg -i test_5_1.wav -c:a libvorbis -q:a 5 test_5_1.ogg
```

#### 11.3.2 静态 HTTP 提供 (验证用)

```bash
python3 -m http.server 8080
```

URL: `http://<host>:8080/test_5_1.ogg`

#### 11.3.3 实时推流 (ffmpeg → Icecast)

```bash
ffmpeg -re -i test_5_1.wav \
  -c:a libvorbis -q:a 5 \
  -ac 6 -ar 48000 \
  -content_type audio/ogg \
  -f ogg icecast://source:hackme@localhost:8000/aya_5_1.ogg
```

主要选项:
- `-re` = real-time (按素材时长投递，模拟现场推流)
- `-content_type audio/ogg` = 向 Icecast 申报 MIME (没有这个会被误判为 MP3)
- `-ac 6 -ar 48000` = 维持 6ch 48kHz

### 11.4 Opus 6ch / FLAC 6ch 的限制

通过 **纯 HTTP** (例如 `python3 -m http.server`) 或 **Icecast push** 推送 Opus 6ch (channel mapping family 1) 或 FLAC 6ch 时，FMOD 的 parser 会发出 **seek 请求**，从而以 `FMOD_ERR_FILE_COULDNOTSEEK` 失败而无法打开。

变通方法:

- 使用 **SHOUTcast 兼容 streamer** (支持 keep-alive + range)
- 通过 **ffmpeg primary** 中转 (TCP backpressure 解决)
- 使用 **5.1ch GUI 推流工具 `butt-aya`** push (AYA さん的另一项目，本文撰写时尚未公开)

要确保能跑通，目前最短路径是选择 **Vorbis 6ch**。

### 11.5 推流端工具的选择

| 工具 | 用途 | 注意 |
|---|---|---|
| **ffmpeg** | 任意 codec / 任意 ch / 静态 / 实时 | 需要命令行操作，最灵活 |
| **butt** (官方) | DJ 推流 | 仅支持 1ch / 2ch，不支持 5.1ch |
| **butt-aya** (5.1ch fork) | 5.1ch GUI 推流 | AYA さん的另一项目，本文撰写时尚未公开 |
| **Liquidsoap** | 高级广播自动化 | 配置复杂，面向高阶用户 |
| **Mixxx / DarkIce / ezstream** | DJ / 自动化 | 以立体声为前提，不支持 5.1ch |

---

## 12. Viewer 端设置

### 12.1 通过 Preferences

**首选项 → Sound** 选项卡有以下控件：

- **3D Stream** 滑条 — 全部流的主音量倍率 (`Stream3DVolumeMaster`)
- **Enabled** 复选框 — 整个功能 ON/OFF (`Stream3DEnabled`)
- **Show channel routing diagnostics in chat** — routing 诊断通知 (`Stream3DRoutingDiagnostic`，§13.3)
- **Hear media and sounds from:** — 听者位置选择 Camera / Avatar (`MediaSoundsEarLocation`，§15.1)

扬声器图标的 Volume 下拉菜单中也有同样的 "3D Stream" 滑条，可以在语音聊天附近即时调整音量。

### 12.2 Debug Settings (高级调优)

`Ctrl + Alt + D` 打开 Advanced 菜单 → Show Debug Settings 可以直接编辑各键。

| 设置键 | 类型 | 默认值 | 含义 |
|---|---|---|---|
| `Stream3DEnabled` | bool | `true` | 整个功能的 kill switch。设为 `false` 立刻拆除所有 binding，重新 enable 不会自动 re-bind (下次 poll 时重新发现) |
| `Stream3DDescriptionScan` | bool | `true` | 设为 `false` 时停止 Description 标签扫描，所有图元 binding 解除 (debug stream 不受影响) |
| `Stream3DMaxConcurrent` | S32 | `4` | 同时 binding 上限 (mono + stereo 合计)。0 表示无限 |
| `Stream3DStereoMaxSpeakers` | S32 | `16` | 单链接组扬声器数上限。超过部分按 traversal 末尾被丢弃并发出警告通知 |
| `Stream3DRolloffMin` | F32 (m) | `1.0` | 单声道标签默认近距离 (mono `{min}` 省略时) |
| `Stream3DRolloffMax` | F32 (m) | `20.0` | 默认远距离 (mono `{max}` / stereo `{range}` 省略时的共同回退值) |
| `Stream3DMaxDistance` | F32 (m) | `64.0` | 图元发现 polling 半径。设为 ≥ `Stream3DRolloffMax` |
| `Stream3DPollInterval` | F32 (秒) | `30.0` | Description polling 间隔。LSL 改动标签的检测延迟由此决定。0 关闭主动 polling |
| `Stream3DVolumeMaster` | F32 [0〜1] | `0.5` | 主音量倍率。与 Preferences 的 3D Stream 滑条同步 |
| `Stream3DReconnectAttempts` | S32 | `3` | 流断开时自动重连尝试次数。每次重试等待 5 秒。0 关闭重连 |
| `Stream3DRoutingDiagnostic` | bool | `false` | routing 诊断 chat 通知 ON/OFF (§13.3)。与 Preferences 的复选框同步 |

#### r11/r12 推流者主导模型: 听者侧 sentinel (无一般 UI)

§7.5 / §8.4 已述，`binaural` / `venue` / `wetgain` / `lfegain` / `upmix` 5 个键采用 **推流者标签即真理** 模型，**没有 Preferences UI**。仅为救援目的提供以下 sentinel 性 debug 设置。一般听众 **不要触动**。

| 设置键 | 类型 | 默认值 | 含义 |
|---|---|---|---|
| `Stream3DBinauralRender` | S32 | `-1` (sentinel = 跟随标签) | `0` 在听者侧强制 OFF / `1` 强制 ON。用扬声器收听 `{binaural:on}` 直播时的救援用途 (详见 §7.5 例外) |
| `Stream3DVenueOverride` | string | `""` (sentinel = 跟随标签) | 写 `"dry"` 等 venue 名时，所有直播都按该会场播放 (`"dry"` = 强制全部残响 OFF，典型用途) |
| `Stream3DVenueWetGain` | F32 | `-1.0` (sentinel = 跟随标签) | `0.0–2.0` 范围内的值会强制覆写 wetgain |
| `Stream3DLfeGain` | F32 | `-1.0` (sentinel = 跟随标签) | `0.0–4.0` 范围内的值会强制覆写 LFE gain (r12.1 新增，详见 §7.4) |
| `Stream3DUpmix` | S32 | `-1` (sentinel = 跟随标签) | `0` 忽略标签强制 OFF / `1` 强制 ON。5.1 native 自动 bypass 仍生效 (详见 §8.1 / §8.3) |
| `Stream3DUpmixLfeCutoff` | F32 (Hz) | `80.0` | upmix DSP LFE LPF cutoff (20–200)。详见 §8.4 |
| `Stream3DUpmixCenterBleed` | F32 | `1.0` | upmix DSP 中央漏出消除比例 (0.0–1.0)。详见 §8.4 |
| `Stream3DUpmixRearDelayMs` | F32 (ms) | `16.0` | upmix DSP 后置 decorrelation 延迟 (0–32)。详见 §8.4 |

> **r12.1 实时调参修正**：r12 发布时，上表中的 `Stream3DUpmixLfeCutoff` / `Stream3DUpmixCenterBleed` / `Stream3DUpmixRearDelayMs` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` 修改后必须 **触摸目标图元 (重新解析 Description) 才生效**。r12.1 已在 polling loop 端追加 per-poll push，使这些设置和其他设置一样从值修改后的下一帧起立即生效 (§12.3)。

#### 仅供 Debug 使用 (动作确认用)

| 设置键 | 类型 | 用途 |
|---|---|---|
| `Stream3DDebugUrl` | 字符串 | Debug 用 URL |
| `Stream3DDebugPlay` | bool | `true` 时在角色前方 5m 放置并播放单声道流 (无需写标签的快速测试) |
| `Stream3DDebugStereoPlay` | bool | 立体声版的同样 debug 播放 |

### 12.3 设置的持久化与即时生效

绝大多数设置都是 **"Live"** = 修改后下一帧起生效。无需重启 Viewer。例外:

- `Stream3DEnabled` 从 `false` 切回 `true` 时不会自动 re-bind。下一个 poll cycle (默认 30 秒以内) 再发现。
- `Stream3DDescriptionScan` 切换会立刻拆除 / 重新发现所有 binding。

> **r12.1 修正**：r12 发布时，`Stream3DUpmixLfeCutoff` / `Stream3DUpmixCenterBleed` / `Stream3DUpmixRearDelayMs` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` 出现回归 — 修改后必须 **触摸目标图元 (重新解析 Description) 才生效**。r12.1 已在 `LLPositionalStreamMgr::update()` 的 polling loop 中追加 per-poll push，使这些设置遵循标准的"下一帧生效"语义。

---

## 13. 错误通知 / 诊断

### 13.1 通知的呈现方式

标签格式错误或结构错误会 **以本地 Chat 通知** 显示。前缀为 "3D Stream:"，作为系统消息呈现。

```
3D Stream: 标签格式错误 (对象名: "MySpeaker")
  ch 的取值必须是 L/R/M/FL/FR/C/LFE/SL/SR 之一。
  示例: [3dstream-stereo:{ch:L}{range:30}]
```

```
3D Stream: 结构错误 (链接组 root: "MainStage")
  根上有音源声明 (url/source:media) 但找不到扬声器 (ch)。
  请在各扬声器图元上写 [3dstream-stereo:{ch:L|R|M}]。
```

### 13.2 30 秒抑制

同一图元 × 同一错误种类的通知会被 **抑制 30 秒**。这是为了防止你连续编辑标签时聊天被刷屏。30 秒后会再发出一次。

被抑制的通知仍会写入 LL_DEBUGS 日志 (debug 用日志)，因此可以通过日志看到内部发生了什么。

### 13.3 Routing 诊断 (5.1ch 布置)

为 5.1ch / 多扬声器会场的 **构建 / 验证期间** 提供的诊断功能，在 Local Chat 中确认 "哪个 prim 在播什么 / 为什么静音"。**默认 OFF**，不显式开启就不会输出。

#### 13.3.1 开启方法

下面任一方式均可启用 (两者同步):

- **Preferences > Sound > Show channel routing diagnostics in chat** ─ 勾选复选框
- **Debug Settings: 把 `Stream3DRoutingDiagnostic` 设为 `true`** (Advanced > Show Debug Settings)

设置即时生效。无需重启 Viewer。

#### 13.3.2 输出位置和格式

打开后，回退发生时会以 **Local Chat 中你自己的发言形式** 按下述格式逐行输出:

```
[HH:MM] You: 3D Stream: <内容>
```

`3D Stream:` 前缀由 `notifyStream3D` helper 自动添加。聊天日志 (`Show in Chat`) 当然也会保留，验证后可以回看。**他人看不到** (仅在你自己的 Local Chat 中显示的伪发言)。

#### 13.3.3 通知文言一览

| 情境 | Local Chat 中输出的行 |
|---|---|
| 6ch 源 × 仅有 `ch:L/R/M` 图元 (无专用 prim) | `3D Stream: FL content folded into BS.775 downmix (source is 6ch, no ch:FL prim)` (FL/FR/C/LFE/SL/SR 各声道分别输出) |
| 6ch 源 × 既无专用 prim 也无 `ch:L/R/M` prim | `3D Stream: FL content has no destination — dropped (source is 6ch, no ch:L/R/M prim)` |
| 2ch 源 × `ch:FL` 图元 | `3D Stream: ch:FL prim playing L (source is 2ch)` |
| 2ch 源 × `ch:FR` 图元 | `3D Stream: ch:FR prim playing R (source is 2ch)` |
| 2ch 源 × `ch:C` 图元 | `3D Stream: ch:C prim playing (L+R)/2 (source is 2ch)` |
| 1ch 源 × `ch:FL/FR/C` 图元 | `3D Stream: ch:FL prim playing M (source is 1ch)` (FR / C 同样格式) |
| 1ch / 2ch 源 × `ch:LFE/SL/SR` 图元 | `3D Stream: ch:LFE prim silent (source is 2ch)` (1ch 时为 `1ch`，SL / SR 同样格式) |

5.1ch 布置 × 2ch 源的具体输出示例参见 §10.4 的 "把回退情况输出到 Chat 的设置"。

#### 13.3.4 throttle 与重新显示条件

通知按 `(root_id, source_id, observed_channel_count, prim_set_signature)` 作为 throttle 的键。同一会场 / 同一源结构持续期间 **不会重复显示** (避免聊天被刷屏)。下列任一变化时重新评估并再次输出:

- 音源改变 (= 切到不同 URL，或改选其他 media/MOAP 面)
- 源声道数改变 (= 同一 URL / 同一 media 面但发生了 5.1ch ↔ 2ch 切换)
- 会场扬声器图元结构改变 (添加 / 删除 prim 或改变 `ch` 值)
- `Stream3DRoutingDiagnostic` 由 OFF 切到 ON 的瞬间

#### 13.3.5 运营建议

- **会场搭建 / 布置验证期间打开** ON，在 Local Chat 中确认每个 prim 的回退行为
- **正式运营 (现场演出等) 时关闭** OFF，保持聊天整洁
- 默认 (OFF) 是面向正式运营的状态。仅在布置验证时手动打开。

### 13.4 日志 (`LL_INFOS("Stream3D")`)

详细的运行日志记录在 AYAstorm 的日志文件中。

```
~/.ayastorm_x64/logs/AYAstorm.log
```

按 `Stream3D` channel grep 可以看到 binding 建立 / 拆除 / 重连 / 源格式探测 / dropout 等。

---

## 14. 故障排查

### 14.1 写了标签但没声音

按顺序检查:

1. **Description 是否真的被改写**: 右键图元 → Edit → 查看 Description 选项卡的当前值
2. **标签拼写**: 是否包含 `[3dstream:` 或 `[3dstream-stereo:` (注意拼写错误)
3. **音源声明是否有效**: `{url:...}` 必须是 `http://` / `https://`；`{source:media}` 需要同一链接组内存在可唯一选择的 media/MOAP 面
4. **`Stream3DEnabled` / `Stream3DDescriptionScan` 是否都为 true**: 在 Preferences > Sound 或 Debug Settings 确认
5. **等待轮询**: LSL `llSetObjectDesc` 的修改最多等 30 秒 (= `Stream3DPollInterval`)
6. **聊天里有没有错误通知**: 参考 §13 错误文言
7. **日志中是否出现 `LL_INFOS("Stream3D")` 的 reconnect attempt**: 可能是流 URL 已断

### 14.2 立体声只出来一边

- 根只写了 `{url}` 而子只写了 `{ch:R}` 时，没有图元负责 L 声道，会变成 "L 缺失" 状态。请在根并写 `{ch:L}`，或给另一个图元分配 `{ch:L}`
- 故意把 `{ch:L}` 写到 2 个图元让两处同时出 L 是符合预期的，没问题
- 打开 routing 诊断 (`Stream3DRoutingDiagnostic`) 后，可以在 chat 中看到每个 ch 实际播放什么

### 14.3 5.1ch 源打不开 / 声音断断续续

- §11.4 的 seek 限制: Opus 6ch / FLAC 6ch 在纯 HTTP / Icecast push 上容易出现的问题。**改用 Vorbis 6ch**，或改走 SHOUTcast 兼容 streamer / ffmpeg primary
- HTTP 切换后最初 5〜10 秒在 prebuffer 充填中可能出现 dropout 警告 (LAN 环境 408〜2045 frames/spk/s ≈ 0.8〜4% 程度)。稳态运行时会消失
- 流码率过高 / 网络拥塞时的 dropout: 推流端降低码率 (推荐 ≤ 256kbps) / 减少同时 binding 数

### 14.4 子图元写了标签但识别不到为扬声器

- 子图元的 Description 通过 Properties 通信获取。**首次进入链接组所在地区时可能稍有延迟** (数秒〜10 秒)
- 通过 LSL `llSetObjectDesc` 改子图元的 Description 后，下一个 poll cycle (30 秒内) 生效
- 检查 `{ch:...}` 的值有无拼写错误 (大小写无关，但拼错就无效)

### 14.5 听者位置不对劲 (声音方向有点怪)

- 摄像机大幅移动时听者位置会跟随摄像机变化，定位也随之改变。希望固定在角色视角的话，把 Preferences > Sound 的 **"Hear media and sounds from:" 切到 Avatar** (`MediaSoundsEarLocation = 1`)
- Camera / Avatar 切换对 3D Stream 同样起作用 (与地块 BGM、LSL `llPlaySound` 等共享设置)

### 14.6 想同时播放多个 3D Stream

- 超过 `Stream3DMaxConcurrent` (默认 4) 后新 binding 会被拒绝。如需同时运行更多请提高该值 (8 / 16 量级仍实用)
- 注意 1 binding = 1 解码线程 + N 个扬声器声道占用 CPU。20 并行 CPU 负担很大，按需增加
  
### 14.7 删了标签声音还停不下来

- 重新评估的触发可能没发生。请把图元移动一下，或绕一圈等待下次 polling
- 仍然停不下来时把 `Stream3DEnabled` 暂时切到 false 强制拆除所有 binding，再切回 true 重新发现

### 14.8 `{source:media}` 找不到 media 面

- 确认 media 设置在与 root tag 相同链接组内的某个 face 上。
- 如果 media 在子图元上，请用 `{link:N}` 指定该子图元的 SL link number。
- 如果被选中的图元有多个 media face，请同时指定 `{face:N}`。
- 刚 link 或刚加载对象时，media face 信息可能比 root Description 稍晚到达。请等待几秒，或 touch / edit 对象以触发重新评估。
- 不要在同一个 root tag 中同时写 `{url:...}` 和 `{source:media}`。

---

## 15. 已知限制 / 规格说明

### 15.1 听者位置基于摄像机或角色

3D Stream 用于声场计算的听者位置遵循 Preferences > Sound 的 **"Hear media and sounds from:"** 设置。

- `Camera` (默认): 摄像机位置 / 朝向
- `Avatar`: 角色位置 / 朝向

这个设置同时作用于 LSL `llPlaySound` / 地块 BGM / Media-on-a-Prim。

### 15.2 1 链接组 = 1 音源

每个链接组只建立 **1 个音源 binding**。根图元可以用 `{url:...}` 或 `{source:media}` 声明音源，但两者互斥；子图元上的音源声明会被忽略。

要在 1 个会场里同时跑多个不同音源，请把链接组拆分摆放 (= 在 `Stream3DMaxConcurrent` 限额内持有多个 binding)。

### 15.3 Description 字数 (127 字节)

LSL `llSetObjectDesc` 能写入的 Description 上限为 **127 字节**。包含中文 / 日文 / 韩文等的 URL 或说明文字按 UTF-8 编码很容易超过。

变长时的对策:

- **使用键名 / venue 取值的短形式** (§4.5)。`binaural`/`venue`/`wetgain` 用 `bin`/`v`/`wg`，9 种 venue 取值也都有 1〜2 字符别名，长形式与短形式完全等价。
- 根上只写 `{url}` 或 `{source:media}`、子图元只写 `{ch}` 的分散方式 (这种情况下每个图元的 Description 都能保持简短)
- 缩短 URL (URL shortener，或推流端把路径缩短)

### 15.4 各 codec 的实测情况

| codec | 1ch / 2ch | 6ch |
|---|---|---|
| Vorbis (Ogg) | ✓ 实机已验证 | ✓ 实机已验证 (r9 P10，连续 12 分钟，0 dropout) |
| Opus (Ogg) | ✓ 实机已验证 | △ 仅代码审阅 (生产路径有运行实绩，static HTTP / Icecast push 上 seek 失败) |
| FLAC | ✓ 实机已验证 | △ 仅代码审阅 (与 Opus 同样限制) |
| MP3 | ✓ 实机已验证 | — |

希望可靠地跑 5.1ch 时请选择 **Vorbis 6ch**。

### 15.5 LFE 没有特殊处理

5.1ch 的 LFE (低音炮) 在 Viewer 端不做 "低通滤波"、"2D 化" 等特殊处理，与其余 5 声道同样进行 3D 布置 + 距离衰减。如需低频限制请在推流端 mix 时完成。

设想的运用方式是: 把物理意义上的低音炮形状的图元放在 SL 里的合适位置，让低频音从那里发出来。

### 15.6 5.1ch 的自由视角模型

真实 5.1 (影院基准 / ITU-R BS.775) 以 **听者处于固定位置** 为前提，在每个 ch 中嵌入方向感。SL 的听者是自由视角的，因此 "sweet spot" 概念不适用。本功能追求的是 **"在会场多点重现 5.1 源"** 的 PA 风格构想，而不是影院环绕体验的复刻。

听者在空间中走动时 5.1 mix 的预定定位当然会破坏，但 "会场感"、"面状响起的感觉" 仍然能充分体现。

### 15.7 在其他 Viewer 中的行为

`[3dstream:...]` / `[3dstream-stereo:...]` 标签是 **AYAstorm 专属**。主线 Firestorm、官方 LL Viewer、Catznip 等其他 Viewer 完全忽略它们。

- AYAstorm 用户: 按设计 3D 定位播放
- 其他 Viewer 用户: 标签只作为说明文字的一部分显示，不出声 (与地块 BGM 独立运作，地块 BGM 已设置时仍可听到)

### 15.8 同时上限

| 上限 | 默认 |
|---|---|
| `Stream3DMaxConcurrent` (链接组级 binding 数) | 4 |
| `Stream3DStereoMaxSpeakers` (1 binding 的扬声器数) | 16 |
| 由此推出的最大同时扬声器数 | 4 × 16 = 64 |

64 channel 量级处于 FMOD 余量内。需要更多请通过 debug settings 提高数值 (实机 CPU 负载确认必不可少)。

### 15.9 音量合成

URL 音源的最终音量为：

```
Stream3DVolumeMaster × {volume:N} × FMOD 距离衰减 × Master Audio Slider × 各种静音状态
```

通常用 `Stream3DVolumeMaster` (Preferences 的 3D Stream 滑条) 做整体调整、`{volume:N}` 做图元级别校正、距离衰减由 `range` (扬声器单独) 或 `Stream3DRolloffMax` (整体默认) 控制。

media/MOAP 音源在链接组内只有 1 个 media 面时，media volume / mute 也会作为 source gain 生效。详细规则见 §6.9。

---

## 16. 静态遮蔽 `[ayastorm:occlude]` (r13)

**面向会场运营 / 建造者** 的标签。把墙、门、地板、天花板等"阻挡声音的图元"贴上这个标签后，AYAstorm 会把它们当作位于听者位置与音源图元位置之间的 **遮蔽物** 来处理，从而让声音变得闷蒙。

`[3dstream:...]` / `[3dstream-stereo:...]` (§5 / §6) 是 **发出声音** 的标签，而 `[ayastorm:occlude]` 是 **阻挡声音** 的标签。两者完全独立 — 只写 occlude 标签的图元不会发出任何声音。

### 16.1 语法

```
[ayastorm:occlude]                            ← 使用默认值 (direct:0.7 reverb:0.5)
[ayastorm:occlude{direct:0.9}{reverb:0.7}]    ← 显式指定
[ayastorm:occlude{direct:0.6}]                ← 只写其中一个 (另一个使用默认)
```

旧前缀 (`[ayastream:occlude]`) **不被接受** — 遮蔽是 r13 新增功能，没有 ayastream 系的遗留图元需要保留。通用书写规则 (§4.3，键名不区分大小写 / 值前后空格 trim / 未知键静默忽略) 仍然适用。

### 16.2 行为模型

#### 哪些声音会被遮蔽

- **`[3dstream:...]` / `[3dstream-stereo:...]` 发出的声音** (3D 定位流，每个扬声器图元单独评估)
- **`llPlaySound` / 附加音 / 子图元音效** (世界 SFX)

如果听者位置 (摄像机或角色) 与音源位置的连线穿过 occlude 图元的 **真实形状 (三角形网格)**，则判定为"有遮蔽"，应用音量衰减 + 低通着色让声音变闷。Path Cut 切开的缺口 / Hollow 挖空的内部 / mesh 图元的精确形状 全部都会参与遮蔽计算 — "穿过甜甜圈的洞" 的声音直接通过，"撞到墙体本身" 的声音才会闷掉，符合直觉。

若同时穿过多个图元，衰减以 **乘法叠加** 方式累积 — 例: 两面 `direct=0.7` 的墙最终 direct = `1 - (1-0.7)² ≈ 0.91`，三面更强。"墙越多越闷" 的直觉直接反映。

内部为两阶段判定: 先用 bounding OBB 做粗剪除 (segment-vs-AABB，~95% 不相关组合被几条指令拒绝)，再对剩下的候选执行 Möller-Trumbore 三角形 raycast。在保持精确形状的同时控制 CPU 成本。

#### 哪些声音不会被遮蔽

- **2D 流** (地块 BGM 等无 3D 定位的播放)
- **Voice (Vivox / WebRTC)**
- **UI 音效 / 预览音** (内部通过 `isForcedPriority` 过滤)

### 16.3 参数说明

| 键 | 默认 | 范围 | 效果 |
|---|---|---|---|
| `direct` | `0.7` | `0.0`-`1.0` | 直达声 (= 音量) 衰减。`0.0` = 完全通过，`1.0` = 近乎静音 |
| `reverb` | `0.5` | `0.0`-`1.0` | 残响成分衰减。`0.0` = 残响通过，`1.0` = 残响切断 |

`direct` 越大，"墙的对面"的感觉越强，并且 viewer 内置的 LOWPASS_SIMPLE 还会按其值把截止频率从 22 kHz 降到 300 Hz (`direct=1.0` 时最深的闷)。`reverb` 仅在音源端图元已通过 `{venue:...}` 启用会场残响 (§7.2) 时才有意义。

### 16.4 材质参考值

经验起点，建议在会场内边听边微调。

| 材质感觉 | `direct` | `reverb` | 印象 |
|---|---|---|---|
| 石墙 / 混凝土 | `0.9` | `0.7` | 近乎静音，仅低频泄漏 |
| 木墙 / 室内装板 | `0.7` | `0.5` | 默认值 — 典型的"隔壁感" |
| 薄木板 / 帘幕 | `0.6` | `0.4` | 闷音泄漏，轻量分隔 |
| 玻璃 / 障子 | `0.3` | `0.2` | 轻微闷音，仍可辨识内容 |
| 装饰用 (实际透明) | `0.1` | `0.05` | 基本通过，仅有轮廓存在 |

### 16.5 自动跟随 (动态门也 OK)

`refreshOccluders` **每 tick** (= 每次 `LLPositionalStreamMgr::update()`) 重新读取全部 occluder 图元的位置 / 旋转 / 缩放。也就是说：

- **移动的门** (LSL `llSetPos` / `llSetRot` 做动画) 贴上 `[ayastorm:occlude]` 后，开关动作会实时反映在遮蔽变化上
- **载具 / 移动图元** 同样可以跟随
- 不需要专门的"门标签" (r13 规格初版曾计划 `[ayastorm:door]`，但 `refreshOccluders` 已足够，所以永久 drop)

### 16.6 距离剪除 (`Stream3DOccluderRange` = 64m)

听者-音源距离超过 `Stream3DOccluderRange` (默认 64m) 时，该音源的 OBB raycast 会被 **skip** (距离衰减已经足够小)。大型会场需要 64m 以上遮蔽时，可在 debug settings 中调高数值，或设为 `0` 让其始终 raycast (§12.2)。

### 16.7 主开关 (`Stream3DOcclusion`)

故障排查 / 切分动作用的 **整体 ON/OFF 开关** (debug setting)。

| 值 | 行为 |
|---|---|
| `-1` (默认) | 启用。所有 `[ayastorm:occlude]` 标签都被评估 |
| `0` | 禁用。标签全部忽略，已闷的声音会通过正常 ramp 回到通过状态 |
| `1` | 显式启用 (为将来 per-mode override 保留) |

实时切换 **不会产生 cliff (突然音量变化)** — 禁用时 smoothing 路径仍然运行，因此 DSP 会以 `Stream3DOcclusionRampMs` (默认 250 ms) 平滑地回到 bypass。

### 16.8 可视化 (`Stream3DShowOccluders`、Alt+Shift+O)

把已注册的 occluder 图元以 **青色三角形网格** (半透明 fill + wireframe) 形式显示的 debug 功能。绘制的就是 raycast 实际使用的三角形，所以 Path Cut / Hollow / mesh 图元的形状会**原样呈现**为青色。建造会场时可用来确认"标签是否被识别"、"是否按预期形状产生遮蔽"。

- **菜单**: View → Highlighting and Visibility → "Show 3D Stream Occluders (AYAstorm)"
- **快捷键**: `Alt+Shift+O` (实时切换)

**编辑中实时跟随**: 在 build floater 中 **选中的 occluder 图元**，在拖动 Path Cut / Hollow / Sculpt 滑块的过程中青色形状会实时更新 — 关闭编辑窗口前就能确认遮蔽形状。未选中的 occluder 则在关闭编辑窗口后通过 sim 回传更新。

**回退提示**: 三角形提取失败的 occluder (例: mesh 三角形数超过 2000 上限，详见 §16.9) 不会画出青色。"贴了标签但没有青色显示" 是已回退到 OBB-only 模式的视觉信号。

把 `Stream3DOcclusion` (主开关，§16.7) 设为 `0` 时可视化 **仍然可用** — 两个开关有意做成独立，以便会场运营在 audio off 的状态下也能确认遮蔽结构。

### 16.9 限制 / 上限

- **同时 occluder 数 256** (`kMaxOccluders` hardcoded)。sim 内 `[ayastorm:occlude]` 标签图元超过 256 个时，第 257 个起不会注册 (`LL_WARNS` 写入日志)。典型 SL 会场 (~100 图元) 有充足余量。
- **三角形数上限**: 每个 occluder **2000 个三角形** (`kMaxTrisPerOccluder` hardcoded)。超过上限的 mesh 图元会放弃三角形提取，回退到仅 bounding OBB 的判定 (`LL_WARNS_ONCE` 写入日志，§16.8 中该图元不显示青色作为视觉提示)。典型 SL building prim (cube / cylinder / hollow / Path Cut) 一般在数十-数百个三角形之间，建造类 mesh 图元也通常在范围内。
- **CPU 负载**: 64m 距离剪除 + OBB 粗剪除使大多数 (segment, occluder) 组合被几条指令拒绝，三角形 raycast 仅对实际相交的少数图元执行。典型 SL 会场 (~100 occluder) 下保持低于 1 ms/sec。
- **同梱 FMOD 限制**: 内部实现为 viewer 侧的 OBB 粗剪除 + Möller-Trumbore 三角形 raycast (同梱 `libfmod 2.03.07` 的 `FMOD::Geometry::createGeometry` 不可用)。对用户透明。

---

## 17. 相关文档 / 内部规格书

本指南是 **面向使用者** 的格式参考。实现内部细节 (decode thread / FMOD 路径 / 环形 buffer / 关闭顺序等) 请参考下述规格书。

| 文档 | 内容 |
|---|---|
| `docs/specs/spec_positional_stream_audio.md` | 3D Stream 主体规格 (r5 修订) — 基础架构 |
| `docs/specs/spec_stream3d_decode_thread.md` | r7 确立的 3-thread 模型 |
| `docs/specs/spec_distributed_stereo.md` | r8 分散描述立体声规格 — `[3dstream-stereo:...]` 的 field 书式 |
| `docs/specs/spec_5_1ch_source.md` | r9 5.1ch 源接收规格 — Opus/FLAC 6ch decode 路径 + BS.775 下混 |
| `docs/specs/spec_5_1ch_placement.md` | r10 5.1ch 会场布置规格 — `ch=FL/FR/C/LFE/SL/SR` 扩展 + 兼容矩阵 |
| `docs/specs/spec_binaural_venue_reverb.md` | r11 双耳 + 会场残响规格 (与 r12 一并发布) — 涵盖本指南 §7 |
| `docs/specs/spec_stereo_upmix.md` | r12 stereo→5.1 上混规格 — DPL2 + 4 步频段分离，涵盖本指南 §8 |
| `docs/ayastorm-r12-stereo-upmix.md` | r12 phase 拆分 (P0–P11) 与验证设计 |
| `docs/ayastorm-r13-occlusion.md` | r13 OBB 遮蔽规格 + 实现记录 — `[ayastorm:occlude]` 设计决策 / spike 实现 / 残工程，涵盖本指南 §16 |
| `docs/ayastorm-r26-moap-3d-stream-implementation-plan.md` | r26 media/MOAP source routing 实现计划 — `{source:media}` / `{link}` / `{face}` |
| `docs/ayastorm-stream3d-roadmap.md` | 3D Stream 整体路线图 |

---

## 修订历史

- **2026-05-05 (初版)**: 作为 r10 时点的最终规格整理。汇总记述 r5 / r8 / r9 / r10 / r10.x 的累积规格。r11 及之后未发布所以不包含。
- **2026-05-08 (r12)**: 加入 §4.5 (短形式)、§7 (双耳化 / 会场残响)、§8 (stereo→5.1 上混)。r11 与 r12 一并发布 (避免标签格式两阶段变更引起的混乱，r11 不单独发布)。后续章节改番 (§7–§14 → §9–§16)。§12.2 列出 r11/r12 听者侧 sentinel debug 设置；推流者主导模型保留 (一般用户无 Preferences UI)。
- **2026-05-09 (r12.1)**：新增 §7.4 `{lfegain:N}` (短形式 `lg`)，原 §7.4 推流者主导模型顺延为 §7.5、原 §7.5 组合示例顺延为 §7.6。`wetgain` 默认值由 `1.0` 改为 `0.2` (反映实际试听确认的音乐用途实用区间 0.1〜0.5)。§12.2 追加 `Stream3DLfeGain` sentinel；§12.2 / §12.3 加入实时调参修正说明 (覆盖 r12 中 `Stream3DUpmix*` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` 修改后必须触摸图元才生效的回归)。
- **2026-05-11 (r13)**: 新增 §16 静态 OBB 遮蔽 `[ayastorm:occlude]`，原 §16 相关文档顺延为 §17。§4.1 由"两种标签"扩展为"三种标签"。r13 debug settings (`Stream3DOcclusion` 主开关 / `Stream3DOccluderRange` 距离剪除 / `Stream3DOcclusionRampMs` smoothing / `Stream3DShowOccluders` 可视化) 在 §16.6-§16.8 中说明。§17 表追加 `docs/ayastorm-r13-occlusion.md`。
- **2026-05-11 (r13 P15)**: 遮蔽判定从 OBB 近似升级为 **真实形状三角形 raycast** (OBB 粗剪除 + Möller-Trumbore 两阶段，详见 §16.2)。Path Cut / Hollow / Mesh 的真实形状全部参与音频计算。多图元叠加方式更正为 **乘法叠加** (实现一直是乘法叠加，旧版误记为 `max`)。`Stream3DShowOccluders` 从 OBB 线框改为 **青色三角形网格** (半透明 fill + wireframe)，build floater 中选中图元支持编辑中实时跟随 (§16.8)。§16.9 中追加每 occluder 2000 三角形上限及 OBB-only 回退规则。§16 标题由"静态 OBB 遮蔽"简化为"静态遮蔽"。
- **2026-05-17 (r26)**: 加入 media/MOAP source routing。§3 / §6 追加 `{source:media}`、`{link:N}`、`{face:N}` 的根图元音源选择说明；§6.9 新增 media/MOAP 音源用法、root / child media face 示例、URL 音源与 media 显示共存示例、media volume / mute 规则，以及到 5.1ch 为止的 media callback 声道说明。
