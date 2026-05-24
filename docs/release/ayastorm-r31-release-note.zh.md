# AYAstorm r31 — 发布公告

**r31 将 3D Stream 的 linkset routing tag 统一到 `[3dstream:...]`**。此前分为单 prim mono URL stream 用的 `[3dstream:...]` 和 linkset 分布式播放用的 `[3dstream-stereo:...]` 的 tag,统合到 `[3dstream:...]`。既有 tag 全部因兼容性保留并继续受理。

> **发布形态**: r31 作为 r25–r30 + r31 一括发布 tag (`v7.2.4-ayastorm-r31+bundle-fs.80646`) 的 1 个功能出货。同捆的其他 release note 与 Firestorm upstream FS-7.1.18.80646 取入分,从 GitHub Release 页面直接 link。

规格的单一真相来源是 `docs/specs/ayastorm-r31-3dstream-unified-tag.md`,面向配信者的实用指南是 `docs/guides/3dstream-tag-guide.{en,ja,zh}.md`、面向用户的使用指南是 `docs/specs/3dstream-user-guide.{en,ja,zh}.md`。本 note 仅作入口与差异 highlight。

---

## AYAstorm r31 — `[3dstream:...]` unified linkset tag

### r31 的核心: 1 个 tag 同时书写 mono 与 linkset routing

r6 以来,3D Stream 有 2 种 tag:

- `[3dstream:{url:...}]` — 单 prim mono URL stream
- `[3dstream-stereo:...]` — linkset 分布式 stereo / 5.1 / MOAP routing

r31 将新规作成时用户需要记忆的形式 **统合到 `[3dstream:...]` 1 个**。同一 prefix 现可书写 mono 与 linkset routing。

判定规则: `[3dstream:...]` 是否作为 linkset routing 处理,取决于 linkset 内是否存在有效的 `{ch:...}` 子 prim。无 `{ch:...}` 时,root 的 `[3dstream:{url:...}]` 与以前一致,继续作为 mono 播放。

### 兼容性

以下既存形式 **全部继续受理**。既存内容无需改写:

- `[3dstream:{url:...}]` — mono URL stream
- `[ayastream:{url:...}]` — 旧 mono alias
- `[3dstream-stereo:...]` — 旧 linkset/distributed tag
- `[ayastream-stereo:...]` — 旧 linkset/distributed alias

新规作成时推荐 `[3dstream:...]`,但无需匆忙置换。

### 主要书写示例

**Mono URL (与以前一致)**

```text
[3dstream:{url:http://example.com/stream.mp3}]
```

**Stereo / distributed (linkset)**

```text
root:  [3dstream:{url:http://example.com/stream.mp3}]
left:  [3dstream:{ch:L}]
right: [3dstream:{ch:R}]
```

由于子 prim 含 `{ch:...}`,root 的 mono binding 自动昇格为 linkset routing。

**MOAP stereo / 5.1**

```text
root: [3dstream:{source:media}]
L:    [3dstream:{ch:L}]
R:    [3dstream:{ch:R}]
```

```text
root: [3dstream:{source:media-5-1}]
FL:   [3dstream:{ch:FL}]
FR:   [3dstream:{ch:FR}]
C:    [3dstream:{ch:C}]
LFE:  [3dstream:{ch:LFE}]
SL:   [3dstream:{ch:SL}]
SR:   [3dstream:{ch:SR}]
```

### 设置 — `{bin}` / `{binaural}` 默认值变更

新规 `[3dstream:...]` tag 中,**`{bin}` / `{binaural}` 未指定时的默认值变更为 off**。需要 binaural 生效的会场请明示 `{bin:on}`。

理由: 既存配置的音的播放方式,在 tag 无改修的状态下尽量不变。r12 以来的 binaural DSP 改为想启用的会场明示 opt-in 的形式。listener 侧 debug override `Stream3DBinauralRender = 0 / 1` 与以前一致可用 (`-1` = 跟随 tag)。

### 5.1ch 配信 codec 整理

5.1ch 配信可用的 codec,匹配 r9-opus codec plugin 投入后的现状整理:

- **Vorbis 6ch** — 实机验证完毕
- **Opus 6ch** — 经 r9-opus codec plugin 实机验证完毕 (不受 FMOD parser seek 制约)
- **FLAC 6ch** — codec layout 已实装。但因配信路径不同,可能需要 seek,在 `FMOD_ERR_FILE_COULDNOTSEEK` 下无法打开

live 配信中 **优先 Opus 6ch**。作为实用配信路径,推荐 [SurroundStreamer](https://github.com/t-noami/SurroundStreamer)。

### upmix 说明变更

stereo → 5.1 upmix,不使用商标名,改为 **matrix upmix + 带域分离** 描述。实装是 `LLStereoUpmix` 的固定算法 (center 为 `(L+R)/√2`、SL/SR 为延迟的 `±(L-R)/√2`、LFE 为 LPF),不以特定商用 decoder 名或方式名记述。

### 其他 Viewer / MOAP fallback

其他 Viewer 不解释 3D Stream tag:

- `{url:...}` 的 3D Stream 音频在其他 Viewer 中不播放
- `{source:media}` 的 **media 面本身**,在其他 Viewer 中作为通常的 MOAP 显示与播放
- `{ch:...}` routing、upmix、binaural、venue reverb 为 AYAstorm 专用

显示 media 画面同时播放 `{url:...}` 3D Stream 音频的配置中,在其他 Viewer 中仅 media 侧作为通常 MOAP 处理。

### 错误通知的 tag 示例

Local Chat 上输出的 tag 格式错误 (`BadCh` / `BadRange` / `BadVolume` 等) 的示例,统一为新推荐 tag 的 `[3dstream:...]` 形式。旧 `[3dstream-stereo:...]` / `[ayastream-stereo:...]` 的受理因兼容性保留,内部 log 的 `[3dstream-stereo]` 表记仅作为 subsystem/debug label 保留,不意味着面向用户的推荐书写。

### 已知制约

- **linkset 昇格仅在检出 `{ch:...}` 时**: 子 prim 仅存在不会从 mono 昇格至 linkset。至少需要 1 个含 `{ch:...}` 的子 prim
- **子 Description 取得延迟**: 子 prim Description 未取得期间,先以 root URL 的 mono playback 播放,检出 `{ch:...}` 后移行至 linkset routing
- **`{source:media}` / `{source:media-5-1}` 无 mono fallback**: 由于是 distributed source declaration,至少需要 1 个 `{ch:...}` speaker

### 实装摘要

- `LLPositionalStreamMgr::parseDistributedStereoTag()` 将 `[3dstream:...]` 作为 distributed grammar 读取
- `DistStereoTagData::unified_3dstream_prefix` 保持 tag 是来自旧 `[3dstream-stereo:...]` 还是 unified `[3dstream:...]`
- `evaluateBinding()` 将 `[3dstream:{url:...}]` 首先作为 mono 兼容处理,同 tag 或 linkset child 检出 `{ch:...}` 时,删除 mono binding 转交 `evaluateLinkset()`
- `evaluateLinkset()` 将无任何 `{ch:...}` speaker 的 unified `[3dstream:{url:...}]` root 退回 mono fallback
- `LLPositionalStreamMgr::effectiveBinaural()` 将 tag 未指定解析为 `false`

### 相关资料

- r31 unified tag spec (单一真相): `docs/specs/ayastorm-r31-3dstream-unified-tag.md`
- 配信者实用指南 (累积): `docs/guides/3dstream-tag-guide.{en,ja,zh}.md`
- 用户使用指南: `docs/specs/3dstream-user-guide.{en,ja,zh}.md`
- 过去章 (r6 以来 3D Stream 脉络): `docs/specs/ayastorm-r*-*.md`
