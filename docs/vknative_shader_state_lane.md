# shader object record 状態の lane 化(台帳 #3・2026-08-11)

台帳 = `vknative_parallel_record_feasibility.md` #3 / §6.6 系統②。gate = 直列(lane=1)挙動恒等の純リファクタ。

## 改修形

`LLGLSLShader` の record 相中可変 member を `VkRecordLaneState mVkRecordLane[LLVKLoader::MAX_RECORD_LANES]`(llglslshader.h)へ集約:

| 旧 member | lane struct 内 |
|---|---|
| `mVkFragPC[16]` / `mVkFragPCMask` | `fragPC[]` / `fragPCMask` |
| `mVkActivePerProgramUBO` / `mVkActivePerProgramUBOMapped` | `activePerProgramUBO` / `activePerProgramUBOMapped` |
| `mVkPerProgramUBORing[3]` / `mVkPerProgramRingIdx[3]` / `mVkPerProgramRingFrame[3]` | `ring[3]` / `ringIdx[3]` / `ringFrame[3]`(ring 本体ごと per-lane = rotate の cursor 競合を構造排除) |
| `mVkPerProgramShadow` | `shadow`(arena path の CPU staging) |

- lane index は既存 idiom `const U32 lane = 0;`(lldrawpool.cpp:767 と同形)。#11 復配線時に TL lane id へ差し替える縫い目。
- 外部 access は accessor に集約: **Type A**(rotate→active 書き・32 site)= `vkPerProgramActiveWritePtr()` / **Type B**(base 直書き・rotate なし・28 site)= `vkPerProgramBaseWritePtr()`(arena = 当該 lane shadow / ring = GPU base mapped)。null guard(~80 site)は `mVkPerProgramUBOMapped`(= GPU base mapped 固定に再定義・null 性は従前と同一)のまま無変更。
- arena 判定は `vkPerProgramArenaActive(lane)`(旧 resolve :3459 条件と同値)に一本化。

## 削除(dead 確定)

- `mVkPerProgramUBOGeneration` = write-only(read 0 件・repo 全域 grep)
- `mVkPerProgramUBOBaseMapped` = Mapped の GPU base 固定化で冗長
- `rotatePerProgramUBOSlot` 末尾の arena_path offsets-dirty 分岐 = 早期 return 後に条件恒偽の到達不能 dead

## 意味論保存の要点(検証済)

- arena path: base==shadow==active の等式は lane init(createVkPipeline)で lane 内に再現。Type B 書き→resolve の per-draw slice copy(#22)の流れ不変。
- ring path: rotate が lane の ring cursor を進め active を差し替え。Type B は非 rotate slot(GPU base)へ = 従前どおり。
- lossy fallback(`C_PP_FALLBACK_LOSSY`)の書き先 = GPU base = 旧 BaseMapped と同一。

## Stage 1 接続(義務)

- #11 復配線 = `MAX_RECORD_LANES` 拡張 + `const U32 lane = 0` 群を TL lane id に差し替え。
- **fork prologue の lane seed 契約(新規義務)**: pass setup(main)で書かれた fragPC / active / shadow は lane0 にしか無い。fork 時に worker lane へ lane0 から複製(または PassContext 供給 = #23/#24)しないと worker bind の `vkReassertFragPC` が空 PC を再生する。

## 残余(#3 外・帰属明示)

- ring path の base GPU buffer は単一実体のまま(Type B ring 書きの lane 分離は未実施・fork 面と非交差の post-process 系が主)。
- `mVkPipelineCache` = #10 / `mMatHash`・`mLightHash` = #2/#18 行列族 / immediate 計数 = #26/#28。
- `mVkEnumBoundView`(bind 時 capture)= 台帳帰属未確認 → 要台帳審査。
- lltexlayer(bake worker)の `setMinimumAlpha` = 例外 thread からの lane0 書き = Stage 1 の lane 割当で構造解消予定。
