# AYAstorm r15 — 发布公告

**r15 是继 r12.1 (即 r10→r12.1 跳跃后首个公开 release) 之后的下一个公开 release，把 r13 + r14 + r15 三步以一次跳跃合并发布** — 同时携带音响表现章的最后一根支柱 (标签驱动遮蔽) 与新章「视觉真实感」的第 1+2 弹 (空气体积感 + 光线穿空)。

功能细节常驻于用户指南 (`docs/guides/3dstream-tag-guide.{ja,en,zh}.md`) 与规格 / 路线图。本公告仅做链接 + 差分要点。

---

## AYAstorm r15 — 标签驱动遮蔽 + 视觉真实感章 第 1+2 弹 (空气体积感 + 光线)

以 r12.1 → r15 的单次跳跃合并提供以下内容。**r13 / r14 并未单独发布**，把 r13 收束的音响表现章与 r14+r15 开启的视觉真实感章集中到这一个 release。

### r13 来源 — 标签驱动遮蔽 (真实形状 mesh raycast) + UI / 启动相关修复

#### r13 核心: `[ayastorm:occlude]` 静态遮蔽 (面向会场运营 / 建造者)

引入第三个标签 family。把 **墙 / 门 / 地板 / 天花板** 等图元贴上 `[ayastorm:occlude]` 后，AYAstorm 把它们视作"阻挡声音的几何"，当听者→音源连线穿过 **图元真实形状 (path cut / hollow / mesh 的三角形网格)** 时施加 **音量衰减 + 低通着色**，呈现"墙的对面"质感。

- **语法**: 单独 `[ayastorm:occlude]` 使用默认值 (`direct:0.7 reverb:0.5`)，`[ayastorm:occlude{direct:0.9}{reverb:0.7}]` 可逐图元覆盖。详情 → [tag-guide §16](../guides/3dstream-tag-guide.zh.md#16-静态遮蔽-ayastormocclude-r13) / 规格 `docs/specs/spec_obb_occlusion.md` / 实现记录 `docs/ayastorm-r13-occlusion.md`
- **哪些声音会被遮蔽**: `[3dstream:...]` / `[3dstream-stereo:...]` 的 3D 定位流 + `llPlaySound` / 附加音 / 子图元音效 (世界 SFX)。2D 流 / Voice / UI 音不受影响。
- **按真实形状判定**: 通过 Path Cut 切口的声音照样穿过 / Hollow 内空腔内不被遮蔽 / mesh 图元按精确形状判定。先用 bounding OBB 粗剪除 (~95% reject) → 再对剩余候选执行 Möller-Trumbore 三角形 raycast 的两阶段方案，兼顾精度与 CPU 成本 (详情 → [tag-guide §16.2](../guides/3dstream-tag-guide.zh.md#162-行为模型))。
- **多图元叠加**: direct / reverb 以 **乘法叠加** 方式累积 — 例: 两面 `direct=0.7` 的墙最终 `1 - (1-0.7)² ≈ 0.91`，"墙越多越闷" 的直觉符合实现。
- **材质参考值**: 石墙 `0.9/0.7` / 木墙 `0.7/0.5` (默认) / 帘幕 `0.6/0.4` / 玻璃 `0.3/0.2` / 装饰 `0.1/0.05`。详情 → [tag-guide §16.4](../guides/3dstream-tag-guide.zh.md#164-材质参考值)
- **动门自动跟随**: `refreshOccluders` 每 tick 重读全部 occluder 的位置 / 旋转 / 缩放，所以 LSL 做动画的门 / 载具 / 移动图元都可以自动跟随。无需专门的"门标签"。在 build floater 中 **选中的图元** 还会在拖动 Path Cut / Hollow / Sculpt 滑块的过程中实时重新提取形状，青色可视化和音频 raycast 都不需要等待关闭编辑窗口。
- **与推流者主导模型的正交性**: 既有 `[3dstream:...]{venue:NAME}` (推流者的表现选择) 与 `[ayastorm:occlude]` (建造者的物理现实) 有意做成 **独立设计**。两者不一致时 (例如洞穴风格的图元配 `venue:cathedral`)，viewer 也不会自动修正或警告。

#### 同梱 debug settings (遮蔽相关)

支撑 `[ayastorm:occlude]` 的实时调参开关 (无一般 UI，仅通过 debug settings):

| 键 | 默认 | 用途 |
|---|---|---|
| `Stream3DOcclusion` | `-1` (= 启用) | 主开关。`0` 时忽略所有 occlude 标签；已闷的音通过 smoothing 路径平滑回到通过状态 (无 cliff) |
| `Stream3DOccluderRange` | `64.0` m | 距离剪除。听者-音源距离超过该值时 skip raycast。`0` 表示始终 raycast |
| `Stream3DOcclusionRampMs` | `250.0` ms | direct/reverb 数值切换时的 crossfade 时长。`0` 表示瞬时跳变 (仅 debug 用) |
| `Stream3DShowOccluders` | off | 将已注册 occluder 以 **青色三角形网格** (半透明 fill + wireframe，raycast 实际使用的三角形) 形式显示的 debug overlay (Alt+Shift+O，与主开关独立)。选中图元在 build floater 编辑期间实时跟随 |

详情 → [tag-guide §16.6-§16.8](../guides/3dstream-tag-guide.zh.md#166-距离剪除-stream3doccluderrange--64m)

#### r13 同梱的独立修复

- **启动时 OS"未响应"对话框的根本修复** (commits `f336d43abc` / `5c3487ff06`): r11 P10 引入的 URL 事前解析 (libcurl HEAD pre-resolve) 在 `https://` URL 上是 **同步 3 秒阻塞**。登录后大量 `[3dstream:url=https://…]` 标签图元同一帧到达时会引起 N × 3s 主线程阻塞 → OS"未响应"对话框。本版改写为 **完全异步 worker-thread API** (`LLStream3DUrlResolve::submit/poll/cancel/shutdown`)，主线程的 curl 同步阻塞彻底消失。详情 → 实现记录 `docs/ayastorm-r13-occlusion.md` §5.4
- **chat 字体实时应用修复** (`d66bdb74fc`): LL 风格 chat (FS 旧式显示) 下 `ChatFontSize` / `PlainTextChatHistory` 改动直到下次发言才生效的问题已修。**与 3dstream 经路无关** 的 chat UI 修复，在 occlusion 工作期间一并 cherry-pick 同梱。
- **V3 皮肤 on-screen chat console 初始显示对齐** (`617716ced8`): 仅 V3 皮肤的 `FSUseNearbyChatConsole` 默认值为 `0` (初始关闭)，本版对齐其他皮肤 (firestorm / phoenix / text / hybrid) 改为 `1` (初始开启)。全新安装与皮肤切换时的行为现在跨皮肤一致。

#### r13 同梱的独立新功能

- **`[parcelhide]` 高度门** (`1dfa52d0e9`): 在 parcel description 中写入 `[parcelhide:{altitude:1000-2000,3000-4000}]` 后，仅当自身高度 (Z) 落入任一指定范围时才触发隐藏。两端 inclusive，连字符分隔，多范围以逗号分隔。例如只对特定 skybox 楼层启用隐藏 (摄影用途)。无参数的传统 `parcelhide` 行为保持不变。
- **一键忽略其他住民物体的 IM (`FSIgnoreObjectIM`)** (`e99d7c9abf`): 全局开关，把其他住民 rez 的物体 (例如商家广告 / 钓鱼广播) 发来的 IM 静默丢弃。**自己拥有的 HUD / rezzer 发出的 IM 不受影响** (依据 `permYouOwner()` 判定)。在 Preferences → Notifications → People 中新增勾选项，default 关闭。

### r14 来源 — 视觉真实感章 第 1 弹: 空气的体积感 (volumetric atmosphere)

收束音响表现章 (r8〜r13) 后，从 r14 开启视觉真实感章。**章节核心命题是「值得拍下来的空气与空间」**「物质本来所呈现的颜色、空气、氛围」。LUT / Tone 的「写真感 look」蒙混与 AAA 风「靠变暗伪装真实感」在本章被 **明确拒绝**，方针是在内部重建 scene-referred 物理计算 (章节 thesis → `docs/ayastorm-visual-realism-roadmap.md` §1)。

r14 作为 A 轴 (大气・空气) 的第 1 弹，目标是 **「让空气随距离与高度作为体积可见」**。

- **主开关 `AYAVisualRealismEnabled`** (default TRUE): 一根开关统一切换视觉真实感章 (r14 起) 的全部开关。OFF 时回到 r13 之前的画面。本章不为各轴 / 各功能量产 cvar (`feedback_prefer_defaults_over_config.md`)。
- **altitude density (高度衰减的物理化)**: 在 `calcAtmosphericVars` 中插入以 scale-height 表达「大气密度随高度指数衰减」的模型。从高处远景及低空雾霭从「带色滤镜」状变为自然的体积衰减。详情 → `docs/ayastorm-r14-volumetric-atmosphere.md` §3-§4
- **scene-referred 积分** (sky shader 内侧。在 `skyV.glsl` 中把 haze 颜色合成拆为 blue / haze 两路并在 linear 空间积分): 朝・夕的暖色 / 蓝天 / 多云时的色相，把 preset 数值 (Blue Density / Haze Density / Haze Horizon) 当作「物理参数」重新解释后再堆栈。preset 兼容性 (朝 preset 仍是朝的暖色、夕仍是夕) 保持不变，但空气感的物理合理性得到提升。
- **WindLight preset 不会失效**: 朝・夕・夜・region 个别 preset 等数值继续作为 input。estate operator / 推流者 / AYA 自己的 preset 资产不会被作废。
- **deferred → HDR scene buffer → tonemap → LDR 骨架保持**: 与 Linden 上游的 merge 通道保留，避免 AYAstorm 单独 fork 的维护成本爆炸。改写的只是内侧 (atmospherics 算式、sun halo / haze_glow 生成路径)。

#### r14 中有意排除在外的项目

- **Preetham (1999) 近似的太阳方向路径长度物理化** (`docs/ayastorm-r14-volumetric-atmosphere.md` §4 P2.b/c): 出现了 太阳 disc 消失的副作用，暂时 deferred。重新设计时以「sun disc 保护」共存的方式回归。
- **sun disc HDR boost** (alpha-driven halo 前提有误，已在 `69cf280f43` revert): r14 P1 试过一次但体感为零并 unground。r14 中不再尝试，计划在 r17 (时间带色温 + 云) 中合流。

### r15 来源 — 视觉真实感章 第 2 弹: 光线贯穿空间 (godrays)

在 r14 「空气作为体积可见」的基础上，r15 加入了 **光线在那空气中穿行** 的体感。

- **新增 shadow-map driven 的 screen-space godrays pass**: 在 `renderGeomPostDeferred` 的 atmospherics 紧接其后插入的 fullscreen pass。复用既有 cascaded sun shadow (不新增 shadow buffer)，沿视线方向以 16 采样 ray-march 积分「太阳光是否被遮挡」，用 Mie 前向峰 phase (`cos^8`) 整形为沿太阳方向的薄纱，最后以 additive 方式叠加到 HDR scene buffer。详情 → `docs/ayastorm-r15-godrays.md`
- **强度固定在 `strength = 0.10`**: 不引入调参 cvar (整章统一仅由主开关 ON/OFF)。AYA 的体感调节自 0.5 → 0.2 → 0.15 → 0.10 逐级收敛，目标是「在太阳方向铺一层克制的纱、不破坏天空与地面颜色」。
- **主开关与 r14 共享**: `AYAVisualRealismEnabled = FALSE` 时 godrays pass 同时跳过。OFF 时回到 r13 之前的画面。
- **不新增 cvar / 不新增 UI**: 推流者主导模型 (r11+) 的流派被原样应用到视觉侧。本 release 不来自 r15 的 Preferences UI 新增 **均为 无**。
- **不与既有 glow / bloom 冲突**: godrays 工作于 HDR scene buffer 上；glow 是 tonemap 之后的 bright pass。逻辑上无合成冲突，二者以加法方式共存。

#### r15 P1 中确认并永续化的知识

向 scene buffer 以 additive (`blendFunc ONE/ONE`) 写入 post-pass 时，shader 侧 **必须** 强制 `frag_color.a = 0.0`。若推 alpha=1，scene buffer 的 alpha 通道 (doAtmospherics / sky 合成将其用作 sky mask) 会被累积破坏，tonemap 之后 **天空会被刷白**。r15 P1 首次投入时观测到该症状 → 定位根因 → 已永续化为 memory `project_aya_visual_realism_alpha_protect.md` (r16+ 视觉表现章中复用必备)。

### 既有布置的处理

- 在 r8 / r9 / r10 / r11 / r12 / r12.1 中已布置的 **全部图元无需改动标签即可继续工作**
- `[ayastorm:occlude]` 是新增的 opt-in 标签，所以不写它的既有会场体验与 r12.1 完全一致 (遮蔽方面)
- r14 / r15 视觉真实感章通过主开关 `AYAVisualRealismEnabled` (default TRUE) 全局生效，但 **OFF 时画面完全回到 r13 之前**。WindLight / Environment / region 个别 preset 继续作为 input — 不会作废 preset 资产。
- r12 引入的 `[3dstream:...]{venue:NAME}` (会场残响) 与 r13 新增的 `[ayastorm:occlude]` (会场遮蔽) **有意正交设计** — `venue:dry` 的建筑贴 occlude、`venue:cathedral` 的图元被墙遮蔽，皆可无冲突运作。

### 已知限制

- **遮蔽相关 (r13 来源):**
  - **同时 occluder 数 256** (`kMaxOccluders` hardcoded)。典型 SL 会场 (~100 图元) 余量充足。第 257 个起不会注册 (`LL_WARNS` 写入日志)。
  - **每 occluder 三角形数上限 2000** (`kMaxTrisPerOccluder` hardcoded)。超过上限的 mesh 图元会跳过三角形提取，回退到仅 bounding OBB 的判定 (`LL_WARNS_ONCE` 写入日志，§16.8 中该图元不显示青色作为视觉提示)。标准 SL 图元与建造类 mesh 图元通常都在范围内。
  - **同梱 FMOD 2.03.07 限制**: 内部实现为 viewer 侧 OBB 粗剪除 + Möller-Trumbore 三角形 raycast (同梱 libfmod 的 `FMOD::Geometry::createGeometry` 不可用)。对用户透明。
- **视觉真实感章 (r14 / r15 来源):**
  - **依赖 shadow detail**: r15 godrays 以 cascaded sun shadow (RenderShadowDetail ≥ 1) 为前提。完全 OFF shadow 的设定下 godrays pass 实质贡献为零 (并非画面崩坏，只是不可见)。
  - **限定于太阳方向 cone**: phase = `cos^8` 的指数意味着仅在距太阳方向大约 30° 以内的 view ray 上有有效贡献。完全背对太阳的视线 (按设计) 不会有 godrays。
  - **与过去 screenshot 像素一致无法保留**: r14 改写 atmospherics 内侧导致颜色合成方式变化，r13 之前的 screenshot 不会逐像素一致。主开关 OFF 可回到 r13 之前的画面。

### 有意排除在外 (r14 / r15 design decision)

以下并非"待实现"，而是 **明确决定不放入** 的设计判断 (完整背景 → [`docs/ayastorm-visual-realism-roadmap.md`](./ayastorm-visual-realism-roadmap.md))。

- **LUT / tonemap / color grade 的"写真感 look"蒙混**: 取 scene-referred 物理计算的精细化路径。本章明确拒绝。
- **AAA 风"靠变暗伪装真实感"**: AYAstorm 追求在亮处 / 阴天 / 雨天也有空气感的表现。"只把画面变暗"的解决方案不予采用。
- **重型 per-frame 全屏 volumetric ray-march**: 分发负担与 GPU 负荷两方面都不适合 AYAstorm 的流派 (1 viewer / 3 OS)。r15 godrays 同样是 16 采样 shadow-driven 的轻量实现。
- **按轴 / 按功能量产 debug settings**: 整章统一仅用一根主开关 `AYAVisualRealismEnabled` 来 ON/OFF (`feedback_prefer_defaults_over_config.md`)。

将在 r16+ 中依次堆栈:
- **r16 aerial perspective** (随距离色变化): depth-based scattering integration，把 preset 的 Distance Multiplier 当作物理系数重新解释
- **r17 时间带色温 + 云的真实感**: Sun/Ambient color 的色温解释、云的 volumetric 化 (轻量、非 heavy raymarch)
- **r18+ 物质色 (B 轴)** / **r20+ 相机表现 (C 轴)**

### 文档

- 用户指南: `docs/guides/3dstream-tag-guide.ja.md` / `.en.md` / `.zh.md` (新增 §16 静态遮蔽全章)
- r13 规格: `docs/specs/spec_obb_occlusion.md`
- r13 实现记录 + 设计判断: `docs/ayastorm-r13-occlusion.md`
- r14 spec: `docs/ayastorm-r14-volumetric-atmosphere.md`
- r15 spec: `docs/ayastorm-r15-godrays.md`
- 视觉真实感章路线图: `docs/ayastorm-visual-realism-roadmap.md` (r14-r17 A 轴, r18+ B 轴, r20+ C 轴 长期计划)
- 音响表现章路线图 (r13 收束): `docs/ayastorm-stream3d-roadmap.md`
- 渲染性能调查笔记 (讨论草稿): `docs/ayastorm-render-perf-survey.md` — 跨 LL 本体 + Firestorm + AYAstorm 的渲染热点观测笔记。并非 r13-r15 功能，作为今后改修讨论的基线一并随版同梱。
