# 3D Stream 标签格式指南

> **Language / 言語 / 语言**: [English](./3dstream-tag-guide.en.md) · [日本語](./3dstream-tag-guide.ja.md) · **中文**
>
> AYAstorm 的 **3D Stream** 功能用于把 HTTP 音频流或 Media-on-a-Prim (MOAP) 音频以 3D 空间定位的方式从图元 (prim) 播放出来。本文档是其标签格式参考手册。
>
> 本文档反映 AYAstorm `r31` 时点的最终规格。r31 起，单图元播放与链接组分散布置的新写法统一推荐使用 `[3dstream:...]`。旧的 `[3dstream-stereo:...]` / `[ayastream-stereo:...]` 前缀仍作为兼容写法接受。

---

## 目录

1. [什么是 3D Stream](#1-什么是-3d-stream)
2. [快速上手](#2-快速上手)
3. [术语](#3-术语)
4. [标签总览](#4-标签总览)
5. [单图元 URL 播放 `[3dstream:...]`](#5-单图元-url-播放-3dstream)
6. [链接组布置 / 分散立体声 `[3dstream:...]`](#6-链接组布置--分散立体声-3dstream)
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
   [3dstream:{url:http://example.com/stream.mp3}{range:30}]
   ```
3. **在 Root 的 Description 中追加** (Root 自身也作为 L 扬声器):
   ```
   [3dstream:{url:http://example.com/stream.mp3}{range:30}{ch:L}]
   ```
4. **Child 的 Description**:
   ```
   [3dstream:{ch:R}]
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
| **media source** | 将同一链接组内的 Media-on-a-Prim (MOAP) 面作为 3D Stream 音源使用的指定。由根图元上的 `{source:media}` / `{source:media-stereo}` / `{source:media-5-1}` 启用，必要时用 `{link:N}{face:N}` 选择媒体面 |
| **扬声器图元** | Description 中含 `{ch:...}` 的图元。实际发声的图元。**根图元、子图元都可以** |
| **binding (绑定)** | 内部按链接组组装的"音源 → 扬声器组"对应关系。1 个链接组 = 1 个 binding |
| **ch (声道)** | 扬声器图元负责的音频声道。`L` / `R` / `M` (单声道)，以及 5.1ch 用的 `FL` / `FR` / `C` / `LFE` / `SL` / `SR` |
| **rolloff (距离衰减)** | 听者远离扬声器时音量逐渐减小的设置 |

---

## 4. 标签总览

### 4.1 三种用途

| 用途 | 前缀 | 成立条件 |
|---|---|---|
| **单图元 URL 播放** | `[3dstream:...]` | 有 `{url:...}`，且同一链接组内没有带 `{ch:...}` 的 3D Stream 标签 |
| **链接组布置 / 分散立体声** | `[3dstream:...]` | Root 有 `{url:...}` 或 `{source:media...}`，且 Root 或子图元中至少有一个 `{ch:...}` |
| **静态遮蔽标签** (r13 新增) | `[ayastorm:occlude]` | 把墙 / 门 / 地板 / 天花等图元标记为"阻挡声音的物体" (面向会场运营 / 建造者，详见 §16) |

### 4.2 旧前缀的别名

r31 以后的新内容推荐使用 **`[3dstream:...]`**。旧前缀 (`[ayastream:...]` / `[3dstream-stereo:...]` / `[ayastream-stereo:...]`) 仍作为兼容写法接受，以便 r5 改名和 r31 unified tag 以前布置的图元无需重新编辑。

```
[3dstream:{url:...}]              ← 推荐 (canonical)
[3dstream:{url:...}{ch:L}]        ← 链接组布置的推荐写法 (canonical)

[ayastream:{url:...}]             ← 旧式，兼容接受
[3dstream-stereo:{ch:L}]          ← 旧式，兼容接受
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

`[3dstream:...]` 中，**4 个常用键** 与 **9 个 `venue` 取值全部** 在 r12 引入了 **短形式别名**。可让 Description 轻松塞进 SL 的 127 字节限制 (§4.4)。长形式与短形式 **完全等价** (内部规范化为同一形式)。新标签、既有标签都可任选其一，行为一致。

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
[3dstream:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{ch:C}{binaural:on}{venue:hall_medium}{wetgain:1.5}{upmix:on}]
```

短形式 (110 字节 — 装得下，节省 23 字节)：

```
[3dstream:{url:http://stream.example.jp:8000/aya/live_set_a.ogg}{ch:C}{bin:on}{v:hm}{wg:1.5}{upmix:on}]
```

#### LSL 辅助脚本的行为

附带的 LSL `aya_3dstream_setup.lsl` (见 §17) **输入接受两种形式**、**输出 (Description 写入) 始终使用短形式**。通过 LSL 对话框配置的 Description 自动转换为短形式。

#### 注意事项

- 键名 **不区分大小写** (依 §4.3 通用规则)。`{BIN:on}`, `{bin:on}`, `{binaural:on}` 等价。
- 同一标签内也可以混用长形式与短形式 (例：`{binaural:on}{v:hm}{wg:1.5}`)。但为了可读性，建议统一使用其中一种形式。

### 4.6 标签生效时机

- AYAstorm **每隔 30 秒轮询** 一次范围内的图元 Description (`Stream3DPollInterval` 设置)。
- 通过 LSL `llSetObjectDesc` 修改 Description 后，下一次轮询会重新评估并生效 (通常 5〜30 秒内)。
- 手工右键图元 → Edit → 修改 Description 时，编辑确认后立刻重新评估 (通过 Properties 通知)。
- Link / Unlink 操作也会触发重新评估。

---

## 5. 单图元 URL 播放 `[3dstream:...]`

### 5.1 语法

```
[3dstream:{url:URL}{min:N}{max:N}]
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
- 只写 `{url:...}` 的 `[3dstream:...]` 会作为单图元 URL 播放。
- 如果同一链接组内有任何带 `{ch:...}` 的 3D Stream 标签，则 `[3dstream:{url:...}]` 会作为链接组布置的音源声明，而不是单图元 mono 播放。
- 是否进入链接组布置模式，不看有没有子图元，而看同一链接组内是否存在 `{ch:...}`。
- `{source:media...}` 不作为单图元播放处理。media/MOAP source routing 是链接组布置功能 (§6.7)，必须与至少一个 `{ch:...}` 扬声器配合使用。
- 立体声音源会 **在内部混合 L/R 转为单声道** 播放。

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

## 6. 链接组布置 / 分散立体声 `[3dstream:...]`

### 6.1 语法

```
[3dstream:{url:URL}{range:N}{ch:CH}{volume:V}]
[3dstream:{source:media}{link:N}{face:N}{range:N}{ch:CH}{volume:V}]
```

或使用旧前缀：

```
[3dstream-stereo:...]
[ayastream-stereo:...]
```

此标签 **以整个链接组为单位处理 1 个音源**。根图元声明"使用哪一个音源"，链接组内各图元声明"自己负责哪个声道"。音源可以是 HTTP URL (`{url:...}`)，也可以是同一链接组内某个 media/MOAP 面 (`{source:media}`)。

### 6.2 图元的角色

每个图元根据标签内容承担以下角色：

| Description 中的字段 | 角色 |
|---|---|
| 含 `{url:...}` | **音源声明** (仅根图元有效，子图元写 `{url:...}` 会被忽略) |
| 含 `{source:media...}` | **media/MOAP 音源声明** (仅根图元有效。MOAP 面可以在根图元或子图元上) |
| 含 `{ch:...}` | **扬声器** (根 / 子图元都可以) |
| 同时包含两者 (= 仅根图元) | 音源声明 + 自身也作为扬声器 |
| 两者都没有 | 不做任何事 (不属于 binding 对象) |

链接组中同时存在 **音源声明 (= 根上有 `{url}` 或 `{source:media}`)** 和 **至少 1 个扬声器 (= 带 `{ch}` 的图元)** 时才会按链接组布置播放。即使有子图元，只要没有 `{ch}`，`[3dstream:{url:...}]` 仍按单图元 URL 播放处理。`{source:media}` 没有 `{ch}` 扬声器时不能开始播放。`{url:...}` 与 `{source:media}` **互斥**；同一个根标签中只能选择其中一种音源。

### 6.3 键一览

#### 6.3.1 仅在根图元上有效的键

| 键 | 必需 | 类型 | 默认值 | 含义 |
|---|---|---|---|---|
| `url` | 与 `source` 互斥，必需 | 字符串 | — | 流 URL。空字符串为错误 |
| `source` | 与 `url` 互斥，必需 | 枚举值 | — | `media` / `media-stereo` = 将 media/MOAP 面作为 2ch 音源。`media-5-1` = 作为 5.1ch / 6ch 音源 |
| `link` | 可选 | S32 | 自动选择 | `{source:media}` 时 media 面所在的 link number。用于在多个 media 面中选择 |
| `face` | 可选 | S32 | 自动选择 | `{source:media}` 时的 media 面编号。用于在多个 media 面中选择 |
| `range` | 可选 | F32 (m) | `Stream3DRolloffMax` (20.0) | 链接组内扬声器没有自己 `range` 时使用的默认衰减距离 |
| `binaural` | 可选 | bool | `off` | 双耳化 ON/OFF (详见 §7.1)。短形式 `bin` |
| `venue` | 可选 | 枚举值 | `dry` | 会场残响 preset 9 种 (详见 §7.2)。短形式 `v` |
| `wetgain` | 可选 | F32 [0.0〜2.0] | `0.2` | 残响 wet 成分倍率 (详见 §7.3)。短形式 `wg` |
| `lfegain` | 可选 | F32 [0.0〜4.0] | `1.0` | LFE 通道增益倍率 (详见 §7.4，r12.1 新增)。短形式 `lg` |
| `upmix` | 可选 | 枚举 | `off` | stereo→5.1 上混 ON/OFF。详见 §8.1 |

#### 6.3.2 扬声器声明键 (任意图元)

| 键 | 必需 | 类型 | 默认值 | 含义 |
|---|---|---|---|---|
| `ch` | **必需** | 枚举 | — | 该图元负责的声道 (详见 §9) |
| `range` | 可选 | F32 (m) | 按 扬声器自身 → 根 `range` → `Stream3DRolloffMax` 顺序回退 | 该扬声器单独的衰减距离 |
| `volume` | 可选 | F32 [0.0〜1.0] | 1.0 | 该扬声器单独的音量倍率 |

> **重要**: 单图元 URL 播放的 `min` / `max` 键在链接组布置中 **会被忽略**。链接组布置内部固定近距离为 1.0m，远距离使用 `range` 键 (或默认值 `Stream3DRolloffMax`)。

### 6.4 1 个根 + 1 个子 (基础立体声对)

最小立体声布置：

```
根 Description:
  [3dstream:{url:http://example.com/stream.mp3}{ch:L}]

子 Description:
  [3dstream:{ch:R}]
```

根负责 L，子负责 R。根与子的 **链接顺序 (link number)** 不影响播放。在空间中放在哪里决定了定位。

### 6.5 多扬声器 (4 个以上)

把同一立体声流从场地 4 个角的扬声器播放：

```
根 Description:
  [3dstream:{url:http://example.com/stream.mp3}{range:50}]

子 #1 Description:
  [3dstream:{ch:L}]

子 #2 Description:
  [3dstream:{ch:R}]

子 #3 Description:
  [3dstream:{ch:L}{volume:0.7}]

子 #4 Description:
  [3dstream:{ch:R}{volume:0.7}]
```

- 根仅做音源声明，自身不发声 (没有 `{ch}`)
- 根的 `{range:50}` 会成为 4 个子扬声器共同的默认衰减距离
- 子 #1、#2 分别承担 L/R，音量 100%
- 子 #3、#4 把同一 L/R 以 70% 音量播出 (后方或辅助扬声器)
- 同一个 `{ch}` 可以写在多个图元上，该声道会从多个位置发声。可用于把同一组 L/R 放在会场前方和后方
- 扬声器数上限由 `Stream3DStereoMaxSpeakers` 设置控制，**默认 16 个**(§12)

### 6.6 5.1ch 会场布置 (6 图元)

把 5.1ch 源 (Opus surround / FLAC 6ch) 展开到 6 个扬声器：

```
根 Description:
  [3dstream:{url:http://example.com/test_5_1.flac}{range:30}]

FL 图元:  [3dstream:{ch:FL}]
FR 图元:  [3dstream:{ch:FR}]
C 图元:   [3dstream:{ch:C}]
LFE 图元: [3dstream:{ch:LFE}]
SL 图元:  [3dstream:{ch:SL}]
SR 图元:  [3dstream:{ch:SR}]
```

- 把每个图元物理摆放到 "会场扬声器位置" (舞台前 L/R、中置、低音炮、环绕 L/R)
- LFE 与其余 5 个一视同仁 (Viewer 端不做低通滤波之类的特殊处理。如需低频限制请在推流端 mix 时完成)
- 如果想按影院座位那样的听音位置来布置，可以先在会场内决定一个基准点，再围绕该点放置各声道扬声器。另一方面，SL 中的听者可以自由移动；离开基准点越远，感受到的定位就越会变化。在这种运用中，它也会表现为把 5.1ch 各声道作为空间扬声器来播放的多点 PA 布置。

### 6.7 Media/MOAP 音源的使用 (r26 / r31 unified tag)

r26 起，可以把链接组内的 Media-on-a-Prim (MOAP) 面作为 3D Stream 音源，而不是直接使用 HTTP URL。r31 以后，在 Root Description 写 `[3dstream:{source:media}]` 或 `[3dstream:{source:media-5-1}]`，扬声器图元则写 `[3dstream:{ch:...}]`。

```
Root Description:
  [3dstream:{source:media}{ch:L}]

Child Description:
  [3dstream:{ch:R}]
```

media 面可以在根图元上，也可以在子图元上。media 面只有 1 个时可以省略 `{link}` / `{face}`。链接组中有多个 media 面时，请在 root tag 中明确选择哪一个面要路由到 3D Stream。

```
Root Description:
  [3dstream:{source:media}{link:3}{face:2}{range:30}]

Speaker #1 Description:
  [3dstream:{ch:L}]

Speaker #2 Description:
  [3dstream:{ch:R}]
```

`{link:N}` 只是选择 **media source** 的 link number。不影响扬声器顺序，也不决定 L/R 分配。扬声器角色始终由各图元的 `{ch:...}` 决定。有多个 media 面但 `{link}` / `{face}` 不能唯一确定目标时，会成为结构错误。

URL 音源与 media 显示可以共存。不过 `{url}` 与 `{source:media}` 互斥。想显示 media 画面，同时把另一个 URL stream 以 3D 方式布置时，只在 root 写 `{url:...}`，不要把 media 面选为 3D Stream source。这种情况下，3D Stream 扬声器播放 URL stream，media 音声按普通 MOAP 音声处理。

音量规则取决于 media 面数量。media 面只有 1 个时，该 media 的 volume / mute 作为 source gain 生效。media 面有多个时，路由到 3D 的被选中 media 作为 source gain `1.0` 处理，由 3D Stream master volume / speaker volume 控制。未选中的 media 面保持普通 media volume 行为。

media source 的声道指定：

- `{source:media}` / `{source:media-stereo}`: 把 media 当作 2ch 音源。stereo media 需要 `{upmix:on}` 时也使用这个指定。
- `{source:media-5-1}`: 把 media 当作 5.1ch / 6ch 音源。扬声器侧布置 `FL / FR / C / LFE / SL / SR`。

本指南只说明 **2ch 与 5.1ch (6ch)** 为止的 media source 行为。即使 Dullahan/CEF callback bus 显示为 8ch，也不表示 3D Stream 已实现 7.1ch speaker routing。

### 6.8 如何识别根图元

编辑链接组时，Build 浮窗的 **Object** 选项卡里 "Selected" 会显示当前选中的图元，链接组的父图元 (= 根) 通常是 **最初被选中并发起链接的那一个**。

最可靠的确认方法：
- Build → Edit → 关闭 "Edit linked" → 点击任一图元 → 选中的就是该链接组的根
- LSL: `llGetLinkNumber()` 在子图元存在时根返回 `1`。没有子图元的单一图元返回 `0`

根与子的 link number (1, 2, 3, ...) **不影响 3D Stream 的播放**。r5 之前用 link number 决定 L/R 的旧规格已废弃，r8 起改为 `{ch:...}` 标签声明制。

---

## 7. 双耳化 / 会场残响 (r12)

r10 之前的 3D Stream 已经可以把 dry 素材作为空间中的多点扬声器播放。r12 追加了可选的 viewer 内 DSP 标签，用于耳机定位修正和会场残响。

| 键 | 短形式 | 默认 | 功能 |
|---|---|---|---|
| `binaural` | `bin` | `off` | 通过 lite-HRTF (ITD + air absorption) 增强耳机定位 |
| `venue` | `v` | `dry` | 9 种会场残响 preset (convolution reverb) |
| `wetgain` | `wg` | `0.2` | 残响 wet 成分倍率 (0.0-2.0，推荐 0.1-0.5) |
| `lfegain` | `lg` | `1.0` | LFE 通道增益倍率 (0.0-4.0，r12.1 新增) |

这些键决定整个音源的表现，因此写在 **带音源声明的 root 图元** 上。写在子图元上不会只作用于该扬声器，而是被忽略。普通听者 UI 中没有这些控制项，会场侧标签设定就是听到的结果 (§7.5)。

### 7.1 `{binaural:on|off}` (短形式 `bin`)

启用用于增强耳机左右定位的 **lite-HRTF DSP**。

#### 动作

`on` 时，会对每个扬声器通道应用以下处理:

- **ITD (interaural time delay)** — 根据扬声器方向用 Woodworth-Schlosberg 近似计算左右耳到达时间差，并作为 sample-fractional delay 施加。它更接近“声音从左侧到来”，而不是单纯让左耳更响。
- **air absorption (距离 HF rolloff)** — 距离越远，高频越衰减 (`-0.5 dB/m`，上限 `-25 dB`)。50m 外的扬声器会听起来更暗。

ILD (左右电平差) 与 r10 以前相同，由 FMOD 的 `FMOD_3D_LINEARSQUAREROLLOFF` 负责。

#### 何时选择 `off`

- 配信源本身已经是面向耳机的空间音频
- 会场主要假定来场者用扬声器而不是耳机收听
- 不想追加定位处理，希望保持既有 3D Stream 的自然距离和方向表现

#### 默认值为 `off` 的理由

为了避免未修改标签的既有布置声音发生变化。希望使用耳机定位增强的会场，请在 root 标签中明确写 `{bin:on}` 或 `{binaural:on}`。

### 7.2 `{venue:NAME}` (短形式 `v`)

从 9 种会场残响 preset 中选择一种。音源本身已经带有强残响时，会与 `venue` 残响叠加。如果希望在 3D Stream 侧调整会场响度，配信源应尽量保持 dry。

#### Preset 一览

| 长形式 | 短形式 | RT60 | 用途 | CPU (增量) |
|---|---|---|---|---|
| `dry` | `d` | — | 无残响 (= r10 行为) | 0 (无 DSP) |
| `room_small` | `rs` | 0.3 s | 小型工作室、卧室 | +0.1 pp |
| `room_medium` | `rm` | 0.6 s | 中型工作室、谈话节目 | +0.1 pp |
| `hall_small` | `hs` | 1.0 s | 小型 live house / 小剧场 | ~+3 pp |
| `hall_medium` | `hm` | 1.5 s | 音乐厅、舞会厅 | +7.7 pp |
| `hall_large` | `hl` | 2.0 s | 大型礼堂、歌剧院 | +9.6 pp |
| `club` | `cl` | 0.8 s | 舞厅、密集早期反射 | ~+5 pp |
| `cathedral` | `ct` | 3.0 s | 大教堂、长尾环境 | +10.2 pp |
| `outdoor` | `od` | 0.2 s | 户外、极轻的早期反射 | +0.1 pp |

#### 默认值为 `dry` 的理由

未指定时保持与既有播放相同。`dry` 时不会插入 reverb DSP，因此没有残响处理的 CPU 负荷。

#### CPU 注意事项

`hall_medium` / `hall_large` / `club` / `cathedral` 的 IR (impulse response) 较长，会增加 partitioned FFT convolution 的负荷。`cathedral` 相比 r10 约 **+10.2 pp**，消耗约一个核心的 53%。在多核 CPU 上换算为整体 CPU 约 3〜7%，但面向低规格环境的会场建议从 `room_small` / `room_medium` / `outdoor` 开始选择。

### 7.3 `{wetgain:N}` (短形式 `wg`)

**湿声 (残响成分)** 的倍率。范围：0.0–2.0。默认值：**0.2** (r12.1 由 1.0 改为 0.2)。

| 取值 | 效果 |
|---|---|
| `0.0` | 完全干声 (= 与 venue=dry 等效。不过 DSP 仍保持插入状态) |
| `0.1` | 湿度极淡 |
| **`0.2` (默认)** | 湿度偏淡 — 音乐用途的实用基准 |
| `0.3〜0.5` | 中等到稍浓 (musical range 上限附近) |
| `1.0` 以上 | 湿度与干声等比或更高 — 音乐场景下通常显得过浓 |
| `2.0` | 湿度 200% — 厚重的环境淹没感 (极少使用) |

#### 设计要点

各 venue 的 IR 均经过 **unity-gain 规范化**，因此 `{wg:0.2}` 在切换 venue 时仍会保持 wet/dry 比例。不过 RT60 较长的 preset 残响拖尾更长，所以同样的 `wetgain` 也可能听起来更浓。

> **r12.1 默认值变更 (1.0 → 0.2)**：原本 `1.0` ("湿干等比") 在 hall / cathedral 等长尾预设下源声会被吞没，已脱出音乐用途的实用区间。实际试听确认 **音乐上可用的范围是 0.1〜0.5**，因此 r12.1 将默认值下调到 `0.2`。同梱 LSL UI 的快选按钮也已重排到 `0.1`〜`0.5` 的细刻度。

#### `{venue:dry}` 时

`venue` 为 `dry` 时，reverb DSP 会被 **完全 bypass**，因此 `wetgain` 会被 **忽略**。

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

这 4 个键 (`binaural` / `venue` / `wetgain` / `lfegain`) 由 root prim 的标签指定。普通 Preferences 中不提供听者侧控制项。

#### 为何无听者 UI？

- 如果每个听者都能分别更改 venue reverb 和 binaural，同一个会场在不同听者那里会变成不同声音。
- AYAstorm 将会场音作相关控制放在标签侧，而不是普通听者 UI。

#### 例外：扬声器收听时的听者侧救援

验证或救援用途可以通过 Debug Settings 覆写。`Stream3DBinauralRender = 0` 强制 binaural off，`Stream3DVenueOverride = "dry"` 强制关闭 reverb，`Stream3DVenueWetGain` / `Stream3DLfeGain` 可覆写对应增益。这些是 debug 控制，不是普通 Preferences 控制。

### 7.6 组合示例

#### 什么都不写 (= 默认)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}]
```
→ 等价于 `{bin:off}{v:d}{wg:0.2}`。不会追加 binaural / venue reverb，作为 dry 的 3D Stream 播放。

#### Live house (PA 取向、节奏型音乐)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:cl}{wg:0.3}]
```
→ 使用 club preset。`wg:0.3` 比默认值稍微增加残响。

#### 大厅 (管弦乐)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:hl}{wg:0.25}]
```
→ 使用 hall_large，wet 为 0.25 倍。长残响 preset 因此保持较克制。

#### 大教堂 (氛围 / 环境)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:ct}{wg:0.15}]
```
→ 使用 cathedral，wet 为 0.15 倍，避免长尾残响压过源声。

#### 户外 (环境 / 漫步 BGM)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:on}{v:od}{wg:0.2}]
```
→ 使用 outdoor，加入轻量 early reflection。`wg:0.2` 是默认值。

#### 已是双耳化的素材 (避免双重处理)

```
[3dstream:{url:http://example/stream.ogg}{ch:C}{bin:off}{v:d}]
```
→ binaural OFF、无残响 (= r10 基线行为)。

---

## 8. stereo→5.1 上混 (r12)

r10 的 per-channel 布置 (FL/FR/C/LFE/SL/SR 六个扬声器) 原本是 **5.1ch 配信** 用功能。另一方面，SL 常用的推流软件 (butt / Mixxx / OBS / SAM 等) 多数以 stereo 配信为中心。

> **TIPS:** 如果想以 5.1ch 配信，可以使用本 repo contributor t-noami 制作的 [SurroundStreamer](https://github.com/t-noami/SurroundStreamer)。

r12 追加的 `{upmix:on}` 键会在 viewer 内 DSP 中把 **stereo 2ch 展开为 6ch**。即使配信侧不是 native 5.1ch，也可以使用 6 扬声器布置播放。

### 8.1 `{upmix:on|off}` (无短形式)

是否将立体声源 upmix 为 5.1。

| 取值 | 含义 |
|---|---|
| **`off` (默认)** | upmix 无效。stereo source 按既有行为流向 `{ch:L}` `{ch:R}` `{ch:M}` |
| `on` | 将 stereo source 6ch 化，并流向 `{ch:FL}` `{ch:FR}` `{ch:C}` `{ch:LFE}` `{ch:SL}` `{ch:SR}` 图元 |

#### 为何默认 off (opt-in)？

upmix DSP 会消耗 CPU，也会改变音像；未指定时保持既有 stereo 播放。使用 6 扬声器展开时，请明确写 `{upmix:on}`。

#### 对既有布置的影响

r8/r10 制作的 6 扬声器链接组，只要在 root 标签中追加 `{upmix:on}`，stereo source 也会展开到 6 个扬声器。`{upmix:off}` 或未指定时则保持既有行为。

### 8.2 算法 (matrix upmix + 频段分离)

upmix 是从 stereo source 生成 center / surround / LFE 成分的固定算法。它将 `(L+R)` / `(L-R)` 的 matrix 处理与频段分离、rear delay 组合使用。配信者只能指定 `on/off`，没有选择算法的标签。

| 输出声道 | 生成方式 |
|---|---|
| `C` (center) | `(L+R)/sqrt(2)` 的同相成分 |
| `Ls` / `Rs` (rear) | `(L-R)/sqrt(2)` 加固定 16ms delay +/- jitter 做 decorrelate |
| `LFE` | `(L+R)` 通过 80Hz low-pass filter |
| `FL` / `FR` (front) | 从 `L` / `R` 中按 `bleed_amount` 去除 center 成分 |

追加处理：

- **LFE LPF**: 只把低频段分配给 LFE speaker
- **Center bleed removal**: 防止 center speaker 与 front L/R 同时形成双重 phantom center
- **Rear decorrelation**: 用很小的时间差分离 surround L/R

不使用机器学习 upmix。实现优先保证输入对应输出的可预测性。

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
| `Stream3DUpmixCenterBleed` | `1.0` | 0.0–1.0 | 从前置 L/R 中减去的中央成分比例 (`0` = 不去除、`1` = 完全去除) |
| `Stream3DUpmixRearDelayMs` | `16.0` ms | 0–32 | 后置 decorrelation 基础延迟 (L / R 间 ±2ms 抖动) |

听者侧强制 OFF / ON 用 sentinel：

| Debug 设置 | 默认值 | 含义 |
|---|---|---|
| `Stream3DUpmix` | `-1` (sentinel = 跟随标签) | `0` 忽略标签强制 OFF / `1` 强制 ON (5.1 native 自动 bypass 仍生效) |

### 8.5 组合示例

#### 默认行为 (无 upmix)

```
根 Description:
  [3dstream:{url:http://example/stereo.ogg}{ch:L}]

子 Description:
  [3dstream:{ch:R}]
```
→ stereo source 的 L/R 流向 2 个扬声器图元。未写 `{upmix:on}` 时，不生成 C / LFE / SL / SR。

#### 6 扬声器布置 + upmix

```
根 Description:
  [3dstream:{url:http://example/stereo.ogg}{upmix:on}{range:30}]

FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```
→ 立体声源被扩展为 6 声道，分别送往 6 个扬声器图元。

#### upmix + binaural + venue

```
根 Description:
  [3dstream:{url:http://example/stereo.ogg}{upmix:on}{bin:on}{v:hm}{wg:0.25}{range:30}]

FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```
→ 6 扬声器 upmix 上再组合 lite-HRTF 与 hall_medium reverb。`wg:0.25` 是 hall_medium 向的克制残响量。

#### 2 扬声器布置中写 `{upmix:on}` 的情况

```
根 Description:
  [3dstream:{url:http://example/stereo.ogg}{upmix:on}{ch:L}]
子: [3dstream:{ch:R}]
```
→ `ch:L` / `ch:R` 作为 FL / FR 相当处理，但 C / LFE / SL / SR 没有接收图元，因此不会发声。2 扬声器布置通常不需要 `{upmix:on}`。需要 center 或 surround 时，请使用上面的 6 扬声器布置。

#### 把 upmix 加到 5.1 native 推流 (自动 bypass)

```
根 Description:
  [3dstream:{url:http://example/stream_5_1.ogg}{upmix:on}{range:30}]

FL:  [3dstream:{ch:FL}]
FR:  [3dstream:{ch:FR}]
C:   [3dstream:{ch:C}]
LFE: [3dstream:{ch:LFE}]
SL:  [3dstream:{ch:SL}]
SR:  [3dstream:{ch:SR}]
```
→ 检测到 6ch source 后，upmix 自动 bypass，并在 Local Chat 通知一次。各扬声器图元直接播放 5.1 native source 的对应声道。

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
| **Opus (Ogg)** | ✓ | ✓ | ✓ | 6ch 使用 Opus channel mapping family 1。已通过 r9-opus 补完路径实机验证 |
| **FLAC** | ✓ | ✓ | △ | codec layout 已实现；配信路径可能有 seek 制约 |
| AAC (ADTS / HLS) | — | — | — | 不支持 |
| AC-3 / E-AC-3 | — | — | — | 不支持 |

源 URL 接受 `http://` 或 `https://`。能维持 HTTP/1.1 keep-alive 的路径 (= SHOUTcast 兼容 streamer 或 ffmpeg 的 TCP 输出) 比单纯静态 HTTP 更稳定。

### 11.2 1ch / 2ch 推流

1ch / 2ch 音源可以通过普通 SHOUTcast、Icecast 或静态 HTTP 配信。codec 支持 MP3 / Vorbis / Opus / FLAC，因此 `oggenc`、ffmpeg、butt 等既有工具可以继续使用。

### 11.3 5.1ch (Vorbis / Opus 6ch) 推流

5.1ch 配信可以使用 **Vorbis 6ch** 或 **Opus 6ch**。两者都已在 viewer 侧确认 6ch 播放。希望以 Opus 6ch 配信的音乐人和 DJ 可以使用 t-noami 制作的 [SurroundStreamer](https://github.com/t-noami/SurroundStreamer)。下面示例使用的是便于用 ffmpeg 制作测试素材的 Vorbis 6ch。

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

### 11.4 Opus 6ch 与 FLAC 6ch 的处理

Opus 6ch 通过 r9-opus codec plugin 路径 decode，因此通常的 Ogg/Opus 6ch 配信不会进入导致 seek 问题的 FMOD parser 路径。Opus channel mapping family 1 的 6ch source 已通过 Icecast 实机确认。直播 5.1ch 时，Opus 6ch 是实用选项。

FLAC 6ch 的 codec layout 已实现，但 FLAC parser 可能要求 seek。如果配信路径不能 seek，可能以 `FMOD_ERR_FILE_COULDNOTSEEK` 失败。使用 FLAC 6ch 时，请事先用实际配信路径确认播放。

运用上的目安：

- 直播 5.1ch 配信优先使用 **Opus 6ch**
- 音乐人 / DJ 进行 Opus 6ch 配信时可使用 **SurroundStreamer**
- 验证用静态文件使用 **Vorbis 6ch** 较容易处理
- FLAC 6ch 仅在确认配信路径可按需 seek 后使用

### 11.5 推流端工具的选择

| 工具 | 用途 | 注意 |
|---|---|---|
| **SurroundStreamer** | Opus 6ch 的 5.1ch 配信 | 面向音乐人 / DJ。t-noami 制作。详见 [SurroundStreamer](https://github.com/t-noami/SurroundStreamer) |
| **ffmpeg** | 测试素材制作 / Vorbis 6ch 配信 / codec 转换 | 需要 CLI 操作，适合测试与自动化 |
| **butt** | 1ch / 2ch 直播配信 | 不用于 5.1ch 配信 |
| **Liquidsoap** | 广播自动化 / 服务器侧处理 | 配置难度高。导入前请确认能保持 6ch |
| **Mixxx / DarkIce / ezstream** | DJ / 自动化 | 基本以 stereo 为前提，不用于 5.1ch 配信 |

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

§7.5 / §8.4 已述，`binaural` / `venue` / `wetgain` / `lfegain` / `upmix` 由标签指定。普通 Preferences 中没有对应 UI，但可通过 Debug Settings 用于验证或个人调整。

| 设置键 | 类型 | 默认值 | 含义 |
|---|---|---|---|
| `Stream3DBinauralRender` | S32 | `-1` (sentinel = 跟随标签) | `0` 在听者侧强制 OFF / `1` 强制 ON。为 `-1` 且未指定 `{binaural}` 时结果为 off (详见 §7.5 例外) |
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
  示例: [3dstream:{ch:L}{range:30}]
```

```
3D Stream: 结构错误 (链接组 root: "MainStage")
  根上有音源声明 (url/source:media) 但找不到扬声器 (ch)。
  请在各扬声器图元上写 [3dstream:{ch:L|R|M}]。
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
2. **标签拼写**: 是否包含 `[3dstream:`。旧配置也兼容 `[3dstream-stereo:` / `[ayastream-stereo:` / `[ayastream:` (注意拼写错误)
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

- §11.4 的 seek 制约: FLAC 6ch 走不能 seek 的路径时可能发生。请改用 **Vorbis 6ch / Opus 6ch**，Opus 6ch 可使用 SurroundStreamer，或让 FLAC 走可 seek 的配信路径
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

- Description 的修改通常会在下一次 polling 时反映 (`Stream3DPollInterval` 默认 30 秒，§4.6)。
- 手动编辑时，重新保存 Description 会发送 Properties 通知并触发重新评估。
- 仍然停不下来时，请先把 `Stream3DEnabled` 暂时设为 `false` 强制解除所有 binding，必要时再设回 `true`。

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
| Opus (Ogg) | ✓ 实机已验证 | ✓ 实机已验证 (r9-opus 补完、Opus channel mapping family 1) |
| FLAC | ✓ 实机已验证 | △ codec layout 已实现。配信路径可能有 seek 制约 |
| MP3 | ✓ 实机已验证 | — |

5.1ch 配信请使用 **Vorbis 6ch** 或 **Opus 6ch**。直播配信中 Opus 6ch 较实用，验证用静态文件则 Vorbis 6ch 更容易处理。

### 15.5 LFE 没有特殊处理

5.1ch 的 LFE (低音炮) 在 Viewer 端不做 "低通滤波"、"2D 化" 等特殊处理，与其余 5 声道同样进行 3D 布置 + 距离衰减。如需低频限制请在推流端 mix 时完成。

设想的运用方式是: 把物理意义上的低音炮形状的图元放在 SL 里的合适位置，让低频音从那里发出来。

### 15.6 5.1ch 的自由视角模型

5.1ch source 通常会假定一个基准听音位置进行 mix。AYAstorm 中也可以像影院座席一样，在会场内指定某个位置作为基准来布置扬声器。

另一方面，SL 中的听者可以自由移动。离开基准点越远，感受到的定位就越会偏离 mix 的假定。3D Stream 的 5.1ch 布置既可以用于固定席视听，也会作为把 5.1ch 各声道作为空间扬声器播放的多点 PA 布置来工作。

### 15.7 在其他 Viewer 中的行为

`[3dstream:...]` 标签，以及兼容前缀 `[3dstream-stereo:...]` / `[ayastream:...]` / `[ayastream-stereo:...]` 是 **AYAstorm 专属**。主线 Firestorm、官方 LL Viewer、Catznip 等其他 Viewer 完全忽略它们。

- AYAstorm 用户: 按设计 3D 定位播放
- 其他 Viewer 用户: 标签只作为说明文字的一部分显示，`{url:...}` 的 3D Stream 音声不会播放。地块 BGM 已设置时仍可听到
- 使用 `{source:media}` 的构成中，其他 Viewer 也会把 media 面作为普通 MOAP 显示和播放。但 3D Stream 的扬声器布置、`{ch:...}` routing、upmix、binaural、venue reverb 不会适用
- 显示 media 画面同时使用 `{url:...}` 3D Stream 的构成中，其他 Viewer 只会得到普通 MOAP 侧，`{url:...}` 的 3D Stream 音声不会播放

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

media/MOAP 音源在链接组内只有 1 个 media 面时，media volume / mute 也会作为 source gain 生效。详细规则见 §6.7。

---

## 16. 静态遮蔽 `[ayastorm:occlude]` (r13)

**面向会场运营 / 建造者** 的标签。把墙、门、地板、天花板等"阻挡声音的图元"贴上这个标签后，AYAstorm 会把它们当作位于听者位置与音源图元位置之间的 **遮蔽物** 来处理，从而让声音变得闷蒙。

`[3dstream:...]` (§5 / §6) 是 **发出声音** 的标签，而 `[ayastorm:occlude]` 是 **阻挡声音** 的标签。两者完全独立 — 只写 occlude 标签的图元不会发出任何声音。

### 16.1 语法

```
[ayastorm:occlude]                            ← 使用默认值 (direct:0.7 reverb:0.5)
[ayastorm:occlude{direct:0.9}{reverb:0.7}]    ← 显式指定
[ayastorm:occlude{direct:0.6}]                ← 只写其中一个 (另一个使用默认)
```

旧前缀 (`[ayastream:occlude]`) **不被接受** — 遮蔽是 r13 新增功能，没有 ayastream 系的遗留图元需要保留。通用书写规则 (§4.3，键名不区分大小写 / 值前后空格 trim / 未知键静默忽略) 仍然适用。

### 16.2 行为模型

#### 哪些声音会被遮蔽

- **`[3dstream:...]` 发出的声音** (3D 定位流，每个扬声器图元单独评估)
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
| `docs/specs/3dstream-user-guide.zh.md` | 3D Stream 使用指南 — 布置步骤、配信形式、SurroundStreamer、其他 Viewer fallback |
| `docs/specs/spec_stream3d_decode_thread.md` | r7 确立的 3-thread 模型 |
| `docs/specs/spec_distributed_stereo.md` | r8 分散描述立体声规格 — 旧 `[3dstream-stereo:...]` 的 field 书式 |
| `docs/specs/ayastorm-r31-3dstream-unified-tag.md` | r31 3D Stream unified tag 规格 — 统一到 `[3dstream:...]`、通过 `{ch}` 升格为 linkset routing、旧前缀兼容 |
| `docs/specs/spec_5_1ch_source.md` | r9 5.1ch 源接收规格 — Opus/FLAC 6ch decode 路径 + BS.775 下混 |
| `docs/specs/spec_5_1ch_placement.md` | r10 5.1ch 会场布置规格 — `ch=FL/FR/C/LFE/SL/SR` 扩展 + 兼容矩阵 |
| `docs/specs/spec_binaural_venue_reverb.md` | r11 双耳 + 会场残响规格 (与 r12 一并发布) — lite-HRTF / 9 venue / wetgain 详细 |
| `docs/specs/spec_stereo_upmix.md` | r12 stereo→5.1 上混规格 — matrix upmix + 频段分离算法详细 |
| `docs/ayastorm-r12-stereo-upmix.md` | r12 phase 拆分 (P0-P11) 与工数估算 |
| `docs/ayastorm-r13-occlusion.md` | r13 OBB 遮蔽规格 + 实现记录 — `[ayastorm:occlude]` 设计决策 / spike 实现 / 残工程 |
| `docs/ayastorm-stream3d-roadmap.md` | 3D Stream 整体路线图 |

---

## 修订历史

- **2026-05-05 (初版)**: 作为 r10 时点的最终规格整理。汇总记述 r5 / r8 / r9 / r10 / r10.x 的累积规格。r11 及之后未发布所以不包含。
- **2026-05-08 (r12)**: 加入 §4.5 (短形式)、§7 (双耳化 / 会场残响)、§8 (stereo→5.1 上混)。r11 与 r12 一并发布 (避免标签格式两阶段变更引起的混乱，r11 不单独发布)。后续章节改番 (§7–§14 → §9–§16)。§12.2 列出 r11/r12 听者侧 sentinel debug 设置；推流者主导模型保留 (一般用户无 Preferences UI)。
- **2026-05-09 (r12.1)**：新增 §7.4 `{lfegain:N}` (短形式 `lg`)，原 §7.4 推流者主导模型顺延为 §7.5、原 §7.5 组合示例顺延为 §7.6。`wetgain` 默认值由 `1.0` 改为 `0.2` (反映实际试听确认的音乐用途实用区间 0.1〜0.5)。§12.2 追加 `Stream3DLfeGain` sentinel；§12.2 / §12.3 加入实时调参修正说明 (覆盖 r12 中 `Stream3DUpmix*` / `Stream3DVenueOverride` / `Stream3DVenueWetGain` / `Stream3DLfeGain` / `Stream3DVolumeMaster` 修改后必须触摸图元才生效的回归)。
- **2026-05-11 (r13)**: 新增 §16 静态 OBB 遮蔽 `[ayastorm:occlude]`，原 §16 相关文档顺延为 §17。§4.1 由"两种标签"扩展为"三种标签"。r13 debug settings (`Stream3DOcclusion` 主开关 / `Stream3DOccluderRange` 距离剪除 / `Stream3DOcclusionRampMs` smoothing / `Stream3DShowOccluders` 可视化) 在 §16.6-§16.8 中说明。§17 表追加 `docs/ayastorm-r13-occlusion.md`。
- **2026-05-11 (r13 P15)**: 遮蔽判定从 OBB 近似升级为 **真实形状三角形 raycast** (OBB 粗剪除 + Möller-Trumbore 两阶段，详见 §16.2)。Path Cut / Hollow / Mesh 的真实形状全部参与音频计算。多图元叠加方式更正为 **乘法叠加** (实现一直是乘法叠加，旧版误记为 `max`)。`Stream3DShowOccluders` 从 OBB 线框改为 **青色三角形网格** (半透明 fill + wireframe)，build floater 中选中图元支持编辑中实时跟随 (§16.8)。§16.9 中追加每 occluder 2000 三角形上限及 OBB-only 回退规则。§16 标题由"静态 OBB 遮蔽"简化为"静态遮蔽"。
- **2026-05-17 (r26)**: 加入 media/MOAP source routing。§3 / §6 追加 `{source:media}`、`{link:N}`、`{face:N}` 的根图元音源选择说明；§6.7 新增 media/MOAP 音源用法、root / child media face 示例、URL 音源与 media 显示共存示例、media volume / mute 规则，以及到 5.1ch 为止的 media callback 声道说明。
- **2026-05-24 (r31)**: 新规推荐标签统一为 `[3dstream:...]`。补充 `[3dstream:{url:...}]` 在同一链接组没有 `{ch}` 时是单图元 URL 播放，而同一链接组有 `{ch}` 时作为链接组布置的音源声明处理。明确 `[3dstream:{source:media...}]` 是与 `{ch}` 扬声器组合使用的 linkset routing 专用，判定依据不是是否有子图元，而是是否存在 `{ch}`。旧 `[3dstream-stereo:...]` / `[ayastream-stereo:...]` 作为兼容 prefix 整理。
- **2026-05-24 (r31 文档修订)**: 将 `binaural` 未指定时的默认值改为 `off`，并更新 §7 与 Debug Settings 表。删除 upmix 说明中的商标性命名，按实现整理为 matrix upmix + 频段分离。更新 §11 / §15.4 的 codec 状态：Opus 6ch 在 r9-opus 补完后为实机验证済，FLAC 6ch 则根据配信路径可能有 seek 制约。追加 SurroundStreamer 作为 Opus 6ch 配信的实用路径。调整 §15.6 的 5.1ch 自由视点模型、§15.7 的其他 Viewer / MOAP fallback，以及 §14 的 troubleshooting 文言。
