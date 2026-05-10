# AYAstorm r13 — 发布公告草案

用于粘贴到 GitHub release 页面的短文。r13 相对于 r12.1 (即 r10→r12.1 跳跃后首个公开 release) 是 **小幅增量**，骨干是新标签 family `[ayastorm:occlude]` + 两项独立的 UI / 启动相关修复。

功能细节常驻于用户指南 (`doc/3dstream-tag-guide.{ja,en,zh}.md`) 与规格书。本公告仅做链接 + 差分要点。

---

## AYAstorm r13 — 标签驱动 OBB 遮蔽 + UI / 启动修复

### 核心: `[ayastorm:occlude]` 静态 OBB 遮蔽 (面向会场运营 / 建造者)

引入第三个标签 family。把 **墙 / 门 / 地板 / 天花板** 等图元贴上 `[ayastorm:occlude]` 后，AYAstorm 把它们视作"阻挡声音的几何"，当听者→音源连线穿过 OBB 时施加 **音量衰减 + 低通着色**，呈现"墙的对面"质感。

- **语法**: 单独 `[ayastorm:occlude]` 使用默认值 (`direct:0.7 reverb:0.5`)，`[ayastorm:occlude{direct:0.9}{reverb:0.7}]` 可逐图元覆盖。详情 → [tag-guide §16](../doc/3dstream-tag-guide.zh.md#16-静态-obb-遮蔽-ayastormocclude-r13) / 规格 `doc/spec_obb_occlusion.md` / 实现记录 `docs/ayastorm-r13-occlusion.md`
- **哪些声音会被遮蔽**: `[3dstream:...]` / `[3dstream-stereo:...]` 的 3D 定位流 + `llPlaySound` / 附加音 / 子图元音效 (世界 SFX)。2D 流 / Voice / UI 音不受影响。
- **材质参考值**: 石墙 `0.9/0.7` / 木墙 `0.7/0.5` (默认) / 帘幕 `0.6/0.4` / 玻璃 `0.3/0.2` / 装饰 `0.1/0.05`。详情 → [tag-guide §16.4](../doc/3dstream-tag-guide.zh.md#164-材质参考值)
- **动门自动跟随**: `refreshOccluders` 每 tick 重读全部 occluder 的位置 / 旋转 / 缩放，所以 LSL 做动画的门 / 载具 / 移动图元都可以自动跟随。无需专门的"门标签"。
- **与推流者主导模型的正交性**: 既有 `[3dstream:...]{venue:NAME}` (推流者的表现选择) 与 `[ayastorm:occlude]` (建造者的物理现实) 有意做成 **独立设计**。两者不一致时 (例如洞穴风格的图元配 `venue:cathedral`)，viewer 也不会自动修正或警告。

### 同梱 debug settings

支撑 `[ayastorm:occlude]` 的实时调参开关 (无一般 UI，仅通过 debug settings):

| 键 | 默认 | 用途 |
|---|---|---|
| `Stream3DOcclusion` | `-1` (= 启用) | 主开关。`0` 时忽略所有 occlude 标签；已闷的音通过 smoothing 路径平滑回到通过状态 (无 cliff) |
| `Stream3DOccluderRange` | `64.0` m | 距离剪除。听者-音源距离超过该值时 skip raycast。`0` 表示始终 raycast |
| `Stream3DOcclusionRampMs` | `250.0` ms | direct/reverb 数值切换时的 crossfade 时长。`0` 表示瞬时跳变 (仅 debug 用) |
| `Stream3DShowOccluders` | off | 将已注册 occluder 以 OBB 线框形式显示的 debug overlay (Alt+Shift+O，与主开关独立) |

详情 → [tag-guide §16.6-§16.8](../doc/3dstream-tag-guide.zh.md#166-距离剪除-stream3doccluderrange--64m)

### r13 同梱的独立修复

- **启动时 OS"未响应"对话框的根本修复** (commits `f336d43abc` / `5c3487ff06`): r11 P10 引入的 URL 事前解析 (libcurl HEAD pre-resolve) 在 `https://` URL 上是 **同步 3 秒阻塞**。登录后大量 `[3dstream:url=https://…]` 标签图元同一帧到达时会引起 N × 3s 主线程阻塞 → OS"未响应"对话框。本版改写为 **完全异步 worker-thread API** (`LLStream3DUrlResolve::submit/poll/cancel/shutdown`)，主线程的 curl 同步阻塞彻底消失。详情 → 实现记录 `docs/ayastorm-r13-occlusion.md` §5.4
- **chat 字体实时应用修复** (`d66bdb74fc`): LL 风格 chat (FS 旧式显示) 下 `ChatFontSize` / `PlainTextChatHistory` 改动直到下次发言才生效的问题已修。**与 3dstream 经路无关** 的 chat UI 修复，在 occlusion 工作期间一并 cherry-pick 同梱。

### 既有布置的处理

- 在 r8 / r9 / r10 / r11 / r12 / r12.1 中已布置的 **全部图元无需改动标签即可继续工作**
- `[ayastorm:occlude]` 是新增的 opt-in 标签，所以不写它的既有会场体验与 r12.1 完全一致
- r12 引入的 `[3dstream:...]{venue:NAME}` (会场残响) 与新增 `[ayastorm:occlude]` (会场遮蔽) **有意正交设计** — `venue:dry` 的建筑贴 occlude、`venue:cathedral` 的图元被墙遮蔽，皆可无冲突运作

### 已知限制

- **同时 occluder 数 256** (`kMaxOccluders` hardcoded)。典型 SL 会场 (~100 图元) 余量充足。第 257 个起不会注册 (`LL_WARNS` 写入日志)。
- **OBB 近似**: 遮蔽判定基于 bounding box。复杂形状 (拱形 / 曲面 / 楼梯扶手) 有近似误差 — 如需更细粒度请拆分为面板分别贴标签。
- **同梱 FMOD 2.03.07 限制**: 内部实现为 viewer 侧 segment-vs-OBB slab test (同梱 libfmod 的 `FMOD::Geometry::createGeometry` 不可用)。对用户透明。
- **Steam Audio integration / SOFA 个人 HRTF / occluder 声透传曲线 / 按材质预设表** 未进 r13 — 推迟到 r14+。r13 的 design decision 认为 viewer 侧 raycast + 单一 lowpass 已足以呈现"墙的对面"质感。

### 文档

- 用户指南: `doc/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md` (新增 §16 OBB 遮蔽全章)
- r13 规格: `doc/spec_obb_occlusion.md`
- r13 实现记录 + 设计判断: `docs/ayastorm-r13-occlusion.md`
- 路线图: `docs/ayastorm-stream3d-roadmap.md`
