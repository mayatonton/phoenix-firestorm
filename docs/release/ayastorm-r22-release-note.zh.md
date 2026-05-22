# AYAstorm r22 — 发布公告

**r22 是聊天 UX 改进版本**,将 Nearby Chat / IM 的历史记录分离为「人类阿凡达发言」和「System & Object 系通知 (LSL + 系统 + TP / Region)」两个标签页。

> **发布形态**: r22 与 **r23 发布版本一同捆绑发布** (不单独发行 r22 tag)。r23 发布页面会回链至本说明及 r22 规格文档。

实现细节 / 已知限制 / 配置参考都保留在永久规格文档 (`docs/ayastorm-r22-chat-tab-spec.md`) 中。本说明仅作为该文档的入口及差异亮点。

---

## AYAstorm r22 — Chat tab split (Human vs System & Object)

### r22 主轴: 不让对话和通知互相打断

在 r21 之前,Nearby Chat / IM 的历史记录是单一 widget,LSL 来源的警告 / 广告 / HUD 通知,再加上 TP 着陆、region restart 等系统通知,全都和人与人的对话混在一起,打断对话节奏。完全屏蔽这些通知会导致漏看,因此 r22 采用 **分离为两个标签页** 的策略,让两类信息互不干扰。

- `[Human]` 标签: 仅阿凡达发言 (对话)
- `[System & Object]` 标签: LSL `llSay` / `llRegionSay` / `IM_FROM_TASK` / 系统通知 / TP / Region 通知 等,**所有非对话信息**
- 非激活标签会显示 **未读数量徽章 `(N)`** (切换标签时归零,仅在当前会话内)
- 自己的 Local Chat 发言记录在 **Human 标签**
- 输入框共用 (无论查看哪个标签都发送到 Local Chat)

标签同时适用于 Nearby Chat (FS V1 / V7 / LL 三种 style 全部) 和 IM (1:1)。Group IM 原本就没有 Object 发言路径,因此只用 Human 标签运作。

标签顺序固定为 `System & Object` (不是 `Object & System`)。

详情 → spec `docs/ayastorm-r22-chat-tab-spec.md`

### 好友上线通知也会出现在 Human 标签 (新增 cvar)

好友上线 / 离线通知带有「想要交谈的对象登场了」的性质,因此被 **额外复制到 Human 标签** (在 System & Object 中始终显示)。只看 Human 标签的用户也不会错过好友登录。

- 由 `FSFriendOnlineToHumanTab` (Boolean, 默认 **ON**) 控制
- 设为 OFF 后停止向 Human 复制;通知仍出现在 System & Object 中
- 此功能与 SL 既有 cvar `OnlineOfflinetoNearbyChat` (默认 OFF,控制是否在 Nearby Chat 显示通知本身) 独立判定 — 当 r22 的标签分离启用时,好友上线 / 离线通知通过此专用路径始终送达 Nearby Chat

### 历史文件格式: 仍为单文件,通过末尾 suffix marker 分流

`chat_*.txt` 保持与 r21 相同的单文件结构,每行末尾附加 **suffix marker**,加载时据此判断该行属于 Human 还是 System & Object 标签。

- 由于 marker 位于行末,使用上游 Firestorm 打开同一文件时,**仅会显示为末尾字符串** (解析不会被破坏)
- AYAstorm 端在 append 至 history widget 前会剥离该 marker,**不会泄漏到用户画面**
- 旧历史 (无 marker) 回退到 Human 标签
- 启动时加载的过往好友上线通知仅出现在 System & Object 标签 (Human 复制是 live routing 时的决定,history reload 不会复原)

### 启动时的显示颜色

与 Firestorm 原有行为保持一致:

- 启动时加载的历史记录在两个标签页中均使用 **`ChatHistoryTextColorPersisted` (灰色)**
- 会话中实时接收的发言使用 `ChatHistoryTextColor` (常规色) 追加
- 标签切换仅做 **widget 的 visible/invisible 切换** — 完全不触碰内容,因此不会出现变灰或颜色错乱

### 设置

位于 `Preferences → Chat → Chat Windows`:

| 键 | 默认 | 用途 |
|---|---|---|
| `FSChatHumanObjectTabs` | `1` (ON) | 标签分离的主开关。设为 `0` 则完全回到旧版单 widget 行为 (兼作 escape hatch) |
| `FSFriendOnlineToHumanTab` | `1` (ON) | 同时将好友上线/离线通知复制到 Human 标签。无论该 cvar 值如何,通知都会出现在 System & Object 标签 |

UI 上 `FSFriendOnlineToHumanTab` 作为主开关的缩进子项排列,主开关 OFF 时会自动变灰。

> **不另外提供 escape hatch 用的独立 cvar。** 将主 cvar 设为 false 即可回到旧版行为,没有理由再多设一个开关 (依据 memory `feedback_prefer_defaults_over_config.md` — 比起众多 tuning 键,更优先选择 1 个妥当的默认值)。

变更 `FSChatHumanObjectTabs` 或 `AYAChatWindowStyle` (V1 / V7 / LL 切换) 时,会弹出 **重启确认模态框**,旧 style 的 IM 容器会自动关闭 (M4-extra)。`FSFriendOnlineToHumanTab` 无需重启,实时生效。

### LL style 的通知统一到 3 style 一致 (M8 同捆)

实现过程中发现上游 Firestorm 的 latent bug,在同一版本中一并修复:

- **TP 着陆分隔符** (`secondlife://... 的传送已完成` 前后的分隔线) 在 **FS V1 / V7 / LL 全 3 style** 的 Nearby Chat (System & Object 标签) 中均会显示
- **TP 完成通知 / region simulator 版本差异通知 / RLV 系 system tip** 等经由 `ChatSystemMessageTip` 路由的消息,也开始在 LL style 中送达
- 将 2021 年在上游被注释掉的 `LLFloaterIMNearbyChat` 路径以 `findTypedInstance` 守卫的形式恢复,与既有的 `FSFloaterNearbyChat` 路径并行 dispatch

### 未读徽章在 TP 着陆时也能正确递增

M5 实现的未读徽章误用了 `do_not_log` 标志作为抑制条件,导致 TP 分隔符这类「不写入历史但属于 session 内新事件」的消息无法递增徽章。M8 引入了 history reload 专用的 `is_replay` 标志,将两种关注点正确分离。

### 已知限制

- **未读徽章仅限当前会话**: 重启后会重置。在历史文件中保留计数意义不大 (重新打开历史就能看到消息),因此未采用持久化。
- **HUD allow list (将自己的 HUD 输出归入 Human) 不在 r22 范围内**: 「我自己挂的 HUD 通知应当算人类发言」的需求计划在 r23+ 评估。目前 HUD 的 `llSay` 等也会进入 System & Object 标签。
- **过往好友上线通知不会在 history reload 时复制到 Human**: 复制是 live routing 时的决定,reloaded history 仅出现在 System & Object 中。
- **未触碰 `panel_nearby_chat.xml`**: M3 着手时的结构调研发现该文件已 dead / 未被引用,因此实现仅修改 `floater_fs_nearby_chat.xml` 和 `floater_im_session.xml` (通过 `tab_container`)。

### 文档

- r22 spec / GUI 设计 / 数据层 / 验收标准 / 风险登记: `docs/ayastorm-r22-chat-tab-spec.md`
- 历史文件 suffix marker 格式: spec §5 (`docs/ayastorm-r22-chat-tab-spec.md#5-データ層`)
- 路由表 (哪种 source type 进哪个标签): spec §4
- 好友上线/离线例外路径实现: spec §4 「フレンド online/offline 例外」
- AYAChatWindowStyle (V1 / V7 / LL 切换) 周边的重启模态框 / floater 自动关闭: spec §6 (M4-extra)
