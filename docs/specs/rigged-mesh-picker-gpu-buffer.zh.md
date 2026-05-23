> **Language / 言語 / 语言**: [English](./rigged-mesh-picker-gpu-buffer.md) · [日本語](./rigged-mesh-picker-gpu-buffer.ja.md) · **中文**

# Rigged Mesh Picker — 解决所有 SL viewer 共通结构性缺陷的 GPU object-ID buffer 改造

**状态**: AYAstorm 已实装 (r21.1 — self pick / r28 — 其他 avatar pick)。欢迎任何 LL viewer 派生 fork 自由取用 — 无需 PR,挑你需要的部分拿走。

**Reference commits** (`mayatonton/phoenix-firestorm` 的 `ayastorm-release` 分支):

| commit | author | 角色 |
|---|---|---|
| `940b989ca5` (2026-05-14) | mayatonton (AYA) | r21.1 初始实装 — 用独立 fallback 替换上游 rigged ray-mesh |
| `f3c0829ea8` (2026-05-14) | mayatonton (AYA) | r21.1 M4.17 — `LLDrawInfo` 单位的 `mFSPickerLocalID` 解决 BoM hash collision |
| `d4fa807f00` (2026-05-14) | mayatonton (AYA) | r21.1 清理 — 废除 CPU stage,统一为 GPU 一条路径 (~746 行减少) |
| `34acea572f` (2026-05-15) | mayatonton (AYA) | r21 M5 — `FSSelfRiggedPickerGPU` 默认翻为 ON |
| `cd35ef4fd8` (2026-05-15) | **t-noami** | M6 — selection handoff 修正,在 AYA block 之后只调用一次 `handleObjectSelection()` |
| `556607465f` (2026-05-15) | **t-noami** | M7 — armed-window 模式 (仅 hover 中执行 GPU pass) |
| `7aa18dde2e` (2026-05-19) | **t-noami** | r28 — 扩展至其他 avatar |
| `c97c14a19d` / `3792ecf857` (2026-05-19/21) | **t-noami** | r28 — buffer-owner tracking、过期帧 reject |
| `8e68f83ba9` (2026-05-20) | mayatonton (AYA) | r28 P0 fixup — cvar 整合、验证日志清除 |

**深入追踪**:
- [`docs/specs/ayastorm-r21-self-rigged-picker.md`](./ayastorm-r21-self-rigged-picker.md) (self picker 设计 + canary protocol)
- [`docs/specs/ayastorm-r21-picker-armed-mode.md`](./ayastorm-r21-picker-armed-mode.md) (armed-window 性能门)
- [`docs/specs/ayastorm-r28-other-rigged-picker.md`](./ayastorm-r28-other-rigged-picker.md) (其他 avatar 扩展)
- [`docs/specs/ayastorm-r21-selection-handoff-investigation.md`](./ayastorm-r21-selection-handoff-investigation.md) (M6 handoff 修正)

---

## 1. TL;DR

当用户右键点击 **rigged mesh 装备** (Mesh body / Mesh head 上的头发、衣物、配饰) 时,所有 LL 派生 viewer 都试图在 **CPU 端的 bind-pose 顶点数据** 上做命中测试。结果出现三个结构性破坏:

1. **蒙皮偏移** — CPU 顶点是 rest pose、屏幕上看到的 GPU 顶点已经经过 idle animation 蒙皮。差异通常是几厘米,在人像距离意味着「光标永远落不到看得见的 mesh 上」。
2. **alpha discard 穿透** — GPU 在 fragment shader 中 `discard` 的头发、薄布料三角形,在 CPU 视角下仍是「实体」。屏幕上明明透明的位置点击下去会被当作实体处理,「穿过」可见 mesh 抓到后面的东西。
3. **近景 miss** — 在脸部人像级缩放下,bind-pose 误差大过屏幕上的目标尺寸。

AYAstorm 用与可视场景**同分辨率、同蒙皮矩阵、同 alpha-discard 路径**渲染一张 **GPU object-ID buffer**,把目标的 `LocalID` (32-bit) 打包到 RGBA8,右键的 pixel 由一次 `glReadPixels(1, 1)` 解析。**屏幕上的 pixel → 物体身份**,从结构上 pixel-perfect 对齐。

效果是「**Add to SSS whitelist**」的右键流真正能抓住用户看到的 mesh — 头发、薄面料、Bento head 部件、BoM body,任何缩放距离均可。这是在渲染管线层面做的结构性修复,而不是在 CPU ray 路径之上再叠 workaround。

## 2. 受影响的 viewer

此 bug **并非 AYAstorm 专属**。CPU ray-mesh intersection 源于 Linden Lab 上游 viewer,因此任何源自 LL 上游的 viewer 都继承同一条 code path。

复现不依赖 viewer 种类。给一个戴 Bento head + 任意主流 rigged hair 的 avatar 缩放到人像距离,精确右键一根可见的头发丝 — 弹出的菜单几乎一定是「下面的 body」「head」或「什么都没有」。

## 3. 现象

所有 CPU ray viewer 都能看到的用户视角破坏:

- **「头发看得见、我正点头发、菜单说我选了裙子」**
- 脸部缩放右键 Bento head → 选中的是 system avatar 骨骼
- 右键透明纱袖 → 抓到下面的 body;右键带孔帽 → 抓到后面的头发
- 多部件 BoM body (head + torso + hands 是分开的 Mesh asset 共享同一 rig) 无法区分 — picker 要么总返回同一部件,要么永远不返回你瞄准的那个

大多数 viewer 用「整个 avatar 选中」这种粗粒度选择掩饰过去。但 (像 AYAstorm Skin SSS whitelist 流程那样) 需要 **mesh asset UUID 本身** 时根本不顶用。

## 4. 根本原因

### 4.1 上游路径

上游 `LLPipeline::lineSegmentIntersectInWorld()` 遍历可见对象并调用 `LLViewerObject::lineSegmentIntersect()`。对 rigged mesh 来说,最终是用 world-space ray 测试 **CPU 端的顶点缓冲**。该缓冲里装的是 **bind-pose (rest pose) 位置** — 永远不会按帧重新蒙皮,因为蒙皮完全是 GPU 的事。

```
[ 用户点击 ]
      ↓
LLPipeline::lineSegmentIntersectInWorld()
      ↓
LLViewerObject::lineSegmentIntersect()        ← CPU
      ↓
bind-pose 顶点缓冲的 ray 测试                  ← 偏移源头
      ↓
带最大厘米级误差的 "hit" / "miss"
```

### 4.2 三个结构性破坏

| 破坏 | 原因 | 显现条件 |
|---|---|---|
| **蒙皮偏移** | CPU bind-pose 与 GPU skinned-pose 不一致 | 始终非零,人像以上缩放即可见 |
| **alpha-discard 穿透** | CPU 将所有三角形视为不透明,GPU 在 fragment shader 中 `discard` | 头发、蕾丝、网状丝袜、薄纱布料、带孔帽 |
| **近景 miss** | 误差相对屏幕目标尺寸变大 | 脸部、手、细配饰的近距离构图 |

### 4.3 identity 问题 (BoM mesh hash collision)

另一个独立但叠加的问题: AYAstorm 初版用 `LLMeshSkinInfo::mHash` (rig 的蒙皮 hash) 识别 picked rig,结果在 BoM body 上发现多个不同 Mesh asset (head / torso / hands) 共享 **同一 rig hash**。以 hash 为单一 key 的 map 把它们全部 collapse 到同一身份,picker 完全无法区分。

## 5. 修复

### 5.1 架构

```
[ 用户 hover ]
      ↓
armed window 开启 (~150ms)
      ↓
armed 中的每个可视帧:
  绑 FBO → ID buffer (WorldViewRectRaw 分辨率、RGBA8)
  目标 avatar 上每个 rigged DrawInfo:
    绑 fsObjectIDV.glsl / fsObjectIDF.glsl
    上传 skinning matrix palette  ← 与可视场景同一矩阵
    上传 object_id_packed = pack32(LocalID)  ← uniform vec4 ([0,1] 字节)
    以相同 VBO、相同 depth test、相同 alpha-discard 绘制
      ↓
[ 用户右键 ]
      ↓
fsselfriggedpicker.cpp::readObjectIDBufferLocalID(x, y)
  scaled → raw pixel (HiDPI 修正)
  window → buffer-local (WorldViewRect offset)
  glReadPixels(1, 1, RGBA, UNSIGNED_BYTE)
  unpack: id = b0 | b1<<8 | b2<<16 | b3<<24
      ↓
findAttachmentOnAvatarByLocalID(target_avatar, id)  ← avatar 作用域
      ↓
LLViewerObject* → 菜单、SSS.Add、…
```

### 5.2 Encoding (pipeline.cpp:10796–10800)

```cpp
F32 r = ((id >>  0) & 0xff) / 255.f;
F32 g = ((id >>  8) & 0xff) / 255.f;
F32 b = ((id >> 16) & 0xff) / 255.f;
F32 a = ((id >> 24) & 0xff) / 255.f;
gFSObjectIDShader.uniform4f(sObjectIDPacked, r, g, b, a);
```

U32 LocalID 在 CPU 侧拆成四个 byte 以 `vec4` uniform 上传。fragment shader 直接写出该 vec4。RGBA8 + 无过滤无损保留 byte,`glReadPixels` 以同样顺序拿回。

### 5.3 DrawInfo 单位的 identity (llvovolume.cpp:5840–5843, 5783)

为了解决 BoM hash collision,`LLDrawInfo` 携带 per-prim 的 `mFSPickerLocalID`,DrawInfo 构建时由 `LLViewerObject::getLocalID()` stamp。batch merge 逻辑扩展为要求 `mFSPickerLocalID` 一致,这样两个碰巧共享 skinning hash 的不同 Mesh asset **不会合并到同一 batch**。

### 5.4 「与可视场景同一」保证

ID-buffer pass 使用:

- 与可视场景 rigged shader 相同的 `getObjectSkinnedTransform()` GLSL helper
- 通过同一个 `LLRenderPass::uploadMatrixPalette()` 上传的 skinning matrix palette
- 相同的 VBO、相同的 depth test、相同的 alpha-discard 分支

屏幕上看到什么,ID buffer 就记录什么。不存在「另一个真相来源」。

### 5.5 鼠标坐标变换 (fsselfriggedpicker.cpp:80–116)

两个修正都必需,而且都容易忘:

1. **HiDPI**: logical (LLCoordGL) → raw pixel 变换,使用 `DisplayScale = WindowWidthRaw / WindowWidthScaled`
2. **UI chrome 偏移**: ID buffer 覆盖 `WorldViewRectRaw` (并非整个 window),需减去 `mLeft` / `mBottom`

任一遗漏会返回 `id == 0` (clear color),picker 静默回退到上游 worldray,然后以 §4 所有理由失败。开发期间「GPU picker 不生效」的报告大多最终追溯到这两点之一。

## 6. 为什么有效

§4 的结构性破坏因 ID buffer 以与可视场景同蒙皮、同 depth、同 alpha-discard 渲染而消失。

| 破坏 (§4) | ID buffer 如何解决 |
|---|---|
| 蒙皮偏移 | shader 以可视场景同一 matrix palette 重新执行 `getObjectSkinnedTransform()` → ID pixel 与 color pixel 一致 |
| alpha-discard 穿透 | fragment `discard` 在 ID shader 里同样执行 → discard 的 fragment 不会出现在 ID buffer 中,用户「穿过」alpha 孔抓到背后所见的物体,与画面一致 |
| 近景 miss | pixel 分辨率 = 可视 pixel 分辨率,1 pixel 精度 |

picker 不再有从 renderer 独立出来的几何余地 — 这就是关键。

## 7. 性能

「永远画 ID buffer」的朴素实装,仅 picker 就要 ~130 rigged draw call/frame。**armed window** (`FSSelfRiggedPickerArmedMode`,默认 ON / `FSSelfRiggedPickerArmSeconds`,默认短时间) 将 pass 限制在 **光标位于 avatar 上的帧**:

- armed window 外: 零额外 draw call,picker 处于休眠
- armed window 内 (仅 hover 中): 目标 avatar 一个的 ID-buffer pass / 帧 = 与一次普通 rigged pass 同等的 draw 数

r28 的其他 avatar 扩展加入 buffer-owner field,以防为 avatar A 构建的 buffer 被 avatar B 的点击误消费,过期帧 reject。

## 8. 验证

最直观的证据是把 ID buffer dump 到磁盘。avatar 上每个 mesh 都占一个独立颜色 (打包的 `LocalID`),清空的背景为黑。

![Picker buffer](./images/picker/picker-dump.png)

我们生成的 picker buffer 图像。

复现步骤:

1. 装备 Bento head + Mesh body + rigged hair + rigged dress
2. 缩放到人像距离
3. 直接右键可见的头发丝 → **Add to SSS whitelist** (或任意 rigged 感知的右键动作)

   - **GPU pixel-accurate picker (AYAstorm)**: 头发被选中,mesh asset UUID 被加入
   - **CPU ray-mesh-intersection picker (LL 上游派生)**: 弹出的是 body / head / "no object" 菜单

4. 透薄面料测试: 右键带孔帽或蕾丝上衣的 alpha 孔部分

   - **GPU pixel-accurate picker**: 后面的物体被 picked (ID buffer 也 `discard` 了)
   - **CPU ray-mesh-intersection picker**: 即使透得见,帽 / 上衣也被 picked

## 9. 只有这种方式才能做到的事 — 从发丝缝隙里把脸拾取出来

| 瞄准头发 | **隔着头发瞄准 head** |
|:---:|:---:|
| ![hair picked](./images/picker/pick-hair-front.png) | ![head picked through hair](./images/picker/pick-head-behind.png) |
| 直接点击发丝 → 头发 mesh 被选中 (蓝色 wireframe = 头发)。 | **点击发丝缝隙里露出的脸侧面 → 点击穿透头发,head mesh 被选中** (蓝色 wireframe = head)。ray-mesh intersect 类 picker 会在头发的三角形处停下,过去想拾到背后的头部通常需要先把头发卸下。 |

由于 picker buffer 以与可视场景**完全相同的 alpha-discard** 进行渲染,作为副产品自然得到:**带透明的 mesh 在 picker 中也是透明的**。

- 透过发丝缝隙看到的脸侧面 → 直接拾取到脸
- 透过蕾丝 / 薄纱看到的皮肤 → 拾取到皮肤
- 穿过圆环耳环或戒指的孔看到的耳朵 → 拾取到耳朵

**与 ray-mesh intersect 类 picker 的行为差异** (透明 mesh 背后):

| picker 方式 | 透明 mesh 背后 |
|---|---|
| ray-mesh intersection | ray 命中最前 mesh 的**三角形**就停下。命中点的**纹理 alpha** 对 picker 不可参照 — 发丝缝隙里能看到的脸,屏幕上可见、但 picker 触达不到。 |
| GPU pixel-accurate (本实装) | picker buffer 本身就以 alpha-discard 渲染,**与可视场景同样被打穿成孔**。可视场景显示为脸的 pixel 在 picker 里也是脸的颜色,点击直接到达。 |

ray-mesh-intersection 类 picker 原理上只能回答「ray 是否命中了某 mesh 的三角形」,无法在命中点查询**纹理 alpha**。BoM body 的皮肤 mesh 从耳到肩到腰是连成一片的三角面片,在头发 / 衣物共享同一可视占位的区域里,不存在通往「背后」的路径。

这是「在 GPU 上用与可视场景完全相同的 shader 渲染一整张 buffer」这一结构带来的副产品 — 只有走 GPU pixel-accurate 路线才能拿到的分辨率。

## 10. 本修复未解决的部分

- **non-rigged 装备** (rigid prim 配饰、单 prim 耳环) 继续使用上游 `lineSegmentIntersectInWorld`。它们原本就没有 rigged drift / discard 问题,GPU 化只会徒增 churn。GPU buffer 对点击返回 `id == 0` 时,picker 显式 **fallback** 到上游 worldray。
- **HUD 装备** 不在范围。HUD 渲染到自己的 RT 并有独立的 pick path。
- **`LLToolPie` 对 terrain / water / static prim 的粗粒度选择** 维持原样。

## 11. 取用 (Adoption)

最小取用集:

| file | 角色 |
|---|---|
| `indra/newview/fsselfriggedpicker.{h,cpp}` | 鼠标坐标变换、`glReadPixels`、scoped avatar walk |
| `indra/newview/pipeline.cpp` — `renderRiggedObjectIDBufferForAvatar()`、`renderSelfRiggedObjectIDBuffer()`、`renderOtherRiggedObjectIDBuffer()` | per-frame ID-buffer pass + armed-window gate |
| `indra/newview/llvovolume.cpp` — DrawInfo 上 `mFSPickerLocalID` stamp + merge guard | BoM body 的 per-prim identity |
| `indra/newview/app_settings/shaders/class1/deferred/fsObjectIDV.glsl` | rigged vertex shader (复用 `getObjectSkinnedTransform()`) |
| `indra/newview/app_settings/shaders/class1/deferred/fsObjectIDF.glsl` | fragment shader (单行 `frag_color = object_id_packed`) |
| `indra/newview/lltoolpie.cpp` — 指向 picker 的 handler chain | 右键解析 |
| `indra/newview/llviewercontrol.cpp` + `settings.xml` | `FSSelfRiggedPickerGPU` / `FSSelfRiggedPickerArmedMode` / `FSSelfRiggedPickerArmSeconds` |

```sh
git remote add ayastorm https://github.com/mayatonton/phoenix-firestorm.git
git fetch ayastorm ayastorm-release

# 包含 GPU path + 清理 + identity 修复的 cherry-pick 范围
git log --oneline 940b989ca5^..8e68f83ba9 -- indra/newview/fsselfriggedpicker.cpp indra/newview/pipeline.cpp indra/newview/llvovolume.cpp indra/newview/app_settings/shaders/class1/deferred/fsObjectID*.glsl
```

不计划提 upstream PR — 请按自己的节奏取用。

## 12. Attribution

- **mayatonton (AYA)**: self-picker 初期设计、GPU 替换架构、CPU stage 废除、BoM identity 修复 (M4.17)、默认 ON 切换、r28 P0 cleanup
- **t-noami**: M6 selection handoff 修正、M7 armed-mode 性能门、r28 其他 avatar 扩展、buffer-owner tracking

---

## License

本文档及 `ayastorm-release` 内的参考实装与 Phoenix-Firestorm / Linden Lab viewer 采用相同许可证 (LGPL v2.1) 发布。
