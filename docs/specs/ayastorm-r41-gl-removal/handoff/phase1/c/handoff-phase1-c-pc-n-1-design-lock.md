# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-N-1 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-1 (= `flushDrawUbos` real per-draw data write 通電 = PC-6ε-3 持越 = `forwardToUboUpload` PER_DRAW case 通電 + `writeDrawUbo` helper 新設 + `recordPlaceholderPoolDraw` dummy zero memset → `writeDrawUbo` 経由 zero write 置換) の **design-lock phase 完了** marker = ambiguity (N1-1)..(N1-9) 9 件 全 AYA literal「OK」record (2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。

> **本 doc 位置付け**: PC-N decomposition design-lock (= 直前 commit `cf7b0b99b0`) で確定した 5 sub-step (PC-N-1..PC-N-5) の **着手 1 番目 sub-step** の詳細 design-lock。PC-N-1 単独 sub-step の literal scope + ambiguity 解消 + 実装計画 + Exit Criteria を固定、別 session で実装 phase 着手。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「PC-N-1 design-lock 着手お願いします」literal 受領 (2026-06-05) → 直前 commit `cf7b0b99b0` (PC-N decomposition design-lock) §6 着手手順に従い、必読 1 件 (PC-N decomp doc) Read + pinpoint reference 4 件 (PC-7ε complete + design 07 §7/§8 + design 06b §4.1/§5.3) pinpoint Read → Explore agent 8 項現状調査 (= `forwardToUboUpload` PER_DRAW case + `writeFrameUbo`/`writeSingletonUbo` pattern + `recordPlaceholderPoolDraw` dummy memset + `flushDrawUbos` + `sDrawUboRingBufferMgr` API + `UniformLocation` field + dirty 管理 + codegen set=2 binding 配置) → ambiguity 9 件 + 重要 gap 2 件 (= (N1-5) scope 縮小該当の可能性 + (N1-7) codegen 配置現状 vs design 07 §7.4 spec 差異) 発見 → AYA literal「OK」一括確認受領 (2026-06-05) で本 design-lock doc 起案。

**PC-N-1 literal scope** (= PC-N decomp doc §0 + §4.1 継承):

1. **`forwardToUboUpload` PER_DRAW case 通電** = 現状 `LL_WARNS_ONCE` diagnostic + return (= PC-7γ-1 tag) を `writeDrawUbo(loc.block_hash, loc.offset, data, size, out_dynamic_offset)` 経由 real write 配線
2. **`writeDrawUbo` helper 新設** = `writeFrameUbo` / `writeSingletonUbo` 同形 signature + 内部 `sDrawUboRingBufferMgr->allocate` + `sDrawUboRingBufferRecords[buffer].mapped` lookup + `std::memcpy` + dynamic offset 返却 (= (N1-1) A + (N1-2) A + (N1-3) A + (N1-4) A + (N1-8) B 採用)
3. **`recordPlaceholderPoolDraw` dummy zero memset → `writeDrawUbo` 経由 zero write 置換** = 既存 dummy memset 0 を `writeDrawUbo(block_hash, 0, zero_buf, 256, out_offset)` 経由置換 = API 経路通電確認 + zero data 維持 (= (N1-5) B 採用、real value 構築は PC-N-2 set=2 復活時 `recordAvatarPlaceholderDraw` 側で本格化)
4. **`flushDrawUbos` first-fire log 維持** + PC-N-4 grow flag 集約 hook 用 placeholder (= (N1-6) B 採用)
5. **codegen set=2 binding 配置現状 (= binding=0/1) 維持** = PC-N-1 では再配置せず、binding=2/3 配置は別 sub-step (= (N1-7) A 採用、design 07 §7.4 spec の 4 binding 独立は PC-7α'' or 別 sub-step で扱う)

**`indra/` 改変想定 3 file** (= 実装 phase で別 session):
- `indra/llrender/llglslshader.cpp` (= `forwardToUboUpload` PER_DRAW case 通電)
- `indra/llrender/llvkloader.cpp` (= `writeDrawUbo` helper 新設 + `recordPlaceholderPoolDraw` 内 `writeDrawUbo` 経由化 + `flushDrawUbos` first-fire log 整理)
- `indra/llrender/llvkloader.h` (= `writeDrawUbo` 公開宣言)

---

## §1. 必読 1 件 + pinpoint reference

**次 session 必読 (= PC-N-1 実装 phase 着手前)**:

1. **本 PC-N-1 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-1-design-lock.md`

**pinpoint reference (実装 phase で必要分のみ)**:

- **PC-N decomposition design-lock doc**: `handoff-...-pc-n-decomposition-design-lock.md` = PC-N 5 sub-step 全体構図 + 依存関係 + Phase 境界 + (N-1)..(N-9) 9 件 AYA record
- **PC-7ε complete doc**: `handoff-...-pc-7-epsilon-complete.md` = `recordPlaceholderPoolDraw` allocate-chain pattern (= step (d) line 5010-5054) + `bindV3aStatic` signature 拡張参考実装
- **design 07 §7**: `design/07-vulkan-api-state.md:361-410` = dynamic offset (L2) 実 Vulkan 配線 + ring buffer 容量
- **design 06b §4.1 + §5.2 + §5.3**: `design/06b-cadence-update-site-and-dirty.md:234-373` = cadence 別 flush 順序 + `forwardToUboUpload` PER_DRAW routing + L1+L2 ring buffer + dynamic offset 設計
- **`writeFrameUbo` / `writeSingletonUbo` 実装**: `indra/llrender/llvkloader.cpp:4611-4676` = 既存 helper pattern 参考実装

---

## §2. 現状調査結果 (= Explore agent 8 項要約)

### §2.1 現 code 状態

| # | 項目 | file:line | 現状要約 |
|---|------|-----------|---------|
| 1 | `forwardToUboUpload` PER_DRAW case | `llglslshader.cpp:2151-2157` | `LL_WARNS_ONCE("Vulkan") << "PC-7γ-1: PER_DRAW forwardToUboUpload not wired yet (PC-7ε scope), block_hash=..."` + return = 未通電 |
| 2 | `writeFrameUbo` 既存 pattern | `llvkloader.cpp:4611-4640` | `void writeFrameUbo(U32 block_hash, U32 offset, const void* data, size_t size)` = `sFrameUboInstances[block_hash]` lookup → guard (data/size/offset range) → `std::memcpy(mapped_ptr[sFrameIndex] + offset, data, size)` → `dirty.store(true, release)` |
| 3 | `writeSingletonUbo` 既存 pattern | `llvkloader.cpp:4642-4676` | `writeFrameUbo` 同形 signature、`sSingletonUboInstances[block_hash]` lookup、`mapped_ptr` single instance |
| 4 | `recordPlaceholderPoolDraw` dummy memset | `llvkloader.cpp:5028-5054` | `sDrawUboRingBufferMgr->allocate(256)` → `alloc.success` guard → `alloc.grew` `LL_WARNS_ONCE` (= PC-N-4 持越) → `sDrawUboRingBufferRecords[alloc.buffer].mapped` lookup → `std::memset(mapped + offset, 0, alloc.size)` → `dynamic_offsets[4] = {offset, offset, offset, offset}` → `bindV3aStatic(cmd_buf, sFrameIndex, dynamic_offsets)` |
| 5 | `flushDrawUbos` | `llvkloader.cpp:4376-4383` | `flushDummyUboWrite("flushDrawUbos")` 1 行呼出のみ、helper は anonymous namespace `:4281-4318` で `allocate(256)` → `memset 0` → first-fire marker log |
| 6 | `sDrawUboRingBufferMgr` public API | `lluboringbuffer.h:54-132` | `allocate(size_bytes)` → `AllocateResult{buffer, offset, size, success, grew}`、`beginFrame()`、`getBuffer()`、`tryGrow()` 内部使用 |
| 7 | `UniformLocation` struct | `ubo_perfect_hash.inl:13-18` | `{U32 block_hash, U32 offset, U32 size, U32 cadence_tag}`、`cadence_tag` enum (= `llglslshader.cpp:94-100`) 0=PerFrame / 1=PerProgram / **2=PerDraw** / 3=PerAsset / 4=PerSkin / 5=Singleton |
| 8 | dirty 管理 pattern | UboInstance struct (= per-frame/program/singleton) | writer 側 `dirty.store(true, std::memory_order_release)` + flush 側 `dirty.exchange(false, std::memory_order_acq_rel)`、per-draw は ring buffer per-allocate ゆえ dirty 概念不要 |

### §2.2 codegen 出力 set=2 binding 配置現状 (= 🚨 重要 gap 発見)

`build-linux-x86_64/codegen/ubo/ubo_metadata.inl:64-70` (= build dir tracked 外、source = `scripts/ubo_codegen/main.py`):

| Block Name | block_hash | set | binding | cadence_tag | size |
|------------|-----------|-----|---------|-------------|------|
| PerDrawUBO_AvatarSkin | `0x667c5023u` | 2 | 0 | 2 | 768 B |
| PerDrawUBO_AvatarVelocity | `0xfa009835u` | 2 | 0 | 2 | 768 B |
| PerDrawUBO_ClipPlane | `0x142da0d9u` | 2 | 0 | 2 | 256 B |
| PerDrawUBO_LightParams | `0x9ebc071fu` | 2 | 0 | 2 | 256 B |
| PerDrawUBO_MultiLight | `0x77f0114cu` | 2 | **1** | 2 | 768 B |
| PerDrawUBO_ObjectSkin | `0x12c7004du` | 2 | 0 | 2 | 10752 B |
| PerDrawUBO_SkinnedVelocity | `0x2531887eu` | 2 | 0 | 2 | 5376 B |

**設計 spec との差異**:
- design 07 §7.4 spec = 4 binding 独立 (= DrawTransform / DrawLights / MaterialPBR / MaterialDithering)
- 現 codegen 出力 = binding=0 に 6 block 集中 + binding=1 に 1 block (= MultiLight)、binding=2/3 0 block
- 採用案 (= (N1-7) A): PC-N-1 では現配置維持、binding=2/3 配置 + 4 binding 再分配は別 sub-step (= 別 design-lock + 実装 + AYA literal 確認段階)

### §2.3 design doc 章

| # | 章 | 出典 | 該当 |
|---|----|------|------|
| 9 | per-draw cadence Vulkan 最適化 (L1+L2 ring + dynamic offset) | `design/06b §5.3` (line 354-373) | (N1-2) A + (N1-4) A 根拠 |
| 10 | dynamic offset (L2) 実 Vulkan 配線 + ring buffer 容量 | `design/07 §7` (line 361-410) | (N1-7) A 根拠 (= 4 binding spec 出典) |
| 11 | cadence 別 flush 順序 (= `flushDrawUbos`) | `design/06b §4.1` (line 234-244) | (N1-6) B 根拠 (= log marker 維持) |
| 12 | `forwardToUboUpload` CADENCE_PER_DRAW routing | `design/06b §5.2` (line 318-323) | (N1-2) A 根拠 (= setter 内 immediate ring buffer write) |

---

## §3. ambiguity (N1-1)..(N1-9) 9 件 AYA literal「OK」record (2026-06-05) + 採用根拠

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| **(N1-1)** | `writeDrawUbo` helper signature | **A**: `void writeDrawUbo(U32 block_hash, U32 offset, const void* data, size_t size, U32& out_dynamic_offset)` = `writeFrameUbo` 同形 + offset 返却引数 | OK (2026-06-05) | 既存 `writeFrameUbo` / `writeSingletonUbo` 同形 signature 統一、`AllocateResult.grew` 観測は PC-N-4 持越 (= (N-6) A 確認済) ゆえ caller への返却は dynamic offset のみで十分 |
| **(N1-2)** | ring buffer allocate timing | **A**: setter 内 (= `forwardToUboUpload` PER_DRAW case 内) で `writeDrawUbo` 呼出 → 内部 allocate + memcpy = immediate | OK (2026-06-05) | design 06b §5.3 default = L1+L2 (ring + dynamic offset) = setter 即時 allocate path、cache 構造新設は scope 拡大、ring buffer per-allocate 自体が cache 役割を担う |
| **(N1-3)** | PER_DRAW key 粒度 | **A**: `block_hash` 単独 (= draw 内 同 block_hash 連続 setter は同 chunk in-place 上書き、最後の write 勝ち) | OK (2026-06-05) | setter は draw 内 1 回呼出前提 (= design 06b §5.2 routing)、in-place 上書き許容、draw 越えは draw 内 bind chain で新 chunk allocate (= ring buffer triple-buffering で hazard 構造的回避) |
| **(N1-4)** | dirty 管理粒度 | **A**: dirty 不要 (= ring buffer per-allocate = 自動 clean state) | OK (2026-06-05) | per-allocate per-frame chunk rotate で write-after-read hazard 構造的回避 (= design 06b §4.4 + §4.3)、dirty 概念不要、`writeFrameUbo` の `dirty.store` pattern は per-allocate model に不適 |
| **(N1-5)** | `recordPlaceholderPoolDraw` dummy memset 置換戦略 | **B**: `writeDrawUbo` helper 経由で zero data 書込 (= API path 通すが data は 0、placeholder PSO ゆえ real value 不要) | OK (2026-06-05) | PC-N-1 literal scope = API 経路通電が本質、placeholder PSO 用 real value 構築は別 sub-step、real value 経路は PC-N-2 set=2 復活時に `recordAvatarPlaceholderDraw` 側で本格化。**🚨 scope 縮小該当の可能性を AYA literal 提示 + literal「OK」受領で段階分離確定** (= `feedback_no_scope_shrink` 整合) |
| **(N1-6)** | `flushDrawUbos` 役割 | **B**: first-fire log のみ維持 (= 経路通電確認 marker)、PC-N-4 で grow flag 集約 hook 追加予定 | OK (2026-06-05) | `flushDrawUbos` 撤去は call site 改変必要 (= scope 拡大)、log marker 維持で経路通電確認 + PC-N-4 hook 用 placeholder 確保 |
| **(N1-7)** | **codegen set=2 binding 配置現状 vs design 07 §7.4 spec 差異** | **A**: PC-N-1 では現 codegen 配置維持 (= binding=0/1 のみ)、binding=2/3 配置 + 4 binding 再分配は別 sub-step | OK (2026-06-05) | PC-N-1 literal scope = `flushDrawUbos` real write 通電、binding 再配置は codegen + shader 一括 work で別問題、現 codegen で `bindV3aStatic` dynamic_offsets[4] 引数は許容 (= 余剰 binding 2/3 shader 未参照ゆえ GPU error なし、dummy offset 0 で問題なし)。**🚨 重要 gap を AYA literal 提示 + literal「OK」受領で別 sub-step に分離確定** |
| **(N1-8)** | `block_hash` lookup 経路 | **B**: `mUboMetadata` 直接参照 (= block_hash → set/binding/size 既存テーブル経由) | OK (2026-06-05) | per-draw は instance 個別不要、ring buffer 1 つで全 block 共有、size は codegen 出力 metadata から block_hash 経由 lookup、`sDrawUboInstances` map 新設は per-draw model に不適 |
| **(N1-9)** | PC-N-1 後 Exit Criteria 内 build verify scope | **A**: llrender build + warning 0 + TUT 11+10+13 + codegen 131/131 (= PC-7ε 同形) | OK (2026-06-05) | cold launch verify は PC-8 (3 OS build verify) で集約 (= (N-9) A 確認済)、PC-N-1 単独 sub-step では build verify のみ |

---

## §4. PC-N-1 実装計画 (a)-(g) 7 step 分解

### §4.1 step (a) = `writeDrawUbo` helper 新設 (`llvkloader.cpp` + `.h`)

**file**: `indra/llrender/llvkloader.cpp` + `indra/llrender/llvkloader.h`

**signature** (= (N1-1) A 採用):

```cpp
// llvkloader.h (= 公開宣言、`writeFrameUbo` / `writeSingletonUbo` の隣に配置)
namespace LLVKLoader {
    void writeDrawUbo(U32 block_hash, U32 offset, const void* data, size_t size, U32& out_dynamic_offset);
}
```

**実装 (= (N1-2) A immediate allocate + (N1-3) A block_hash 単独 + (N1-4) A no dirty + (N1-8) B mUboMetadata 経由)**:

```cpp
// llvkloader.cpp (= `writeFrameUbo` / `writeSingletonUbo` 隣、新設 `<AYAstorm r41 PC-N-1 (a)>` tag block)
void writeDrawUbo(U32 block_hash, U32 offset, const void* data, size_t size, U32& out_dynamic_offset)
{
    out_dynamic_offset = 0u;
    if (!data || size == 0) return;
    if (!sDrawUboRingBufferMgr) { LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: ring buffer mgr null" << LL_ENDL; return; }

    // (N1-8) B = mUboMetadata 直接参照で block 全体 size 取得
    const ubo::UboBlockMeta* meta = ubo::lookupBlockMeta(block_hash);
    if (!meta) { LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: block_hash 0x" << std::hex << block_hash << std::dec << " not registered" << LL_ENDL; return; }
    if (offset + size > meta->size) { LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: out of range" << LL_ENDL; return; }

    // (N1-2) A = setter 内 immediate allocate (= block 全体 size で 1 chunk allocate)
    const LLUboRingBuffer::AllocateResult alloc = sDrawUboRingBufferMgr->allocate(meta->size);
    if (!alloc.success) { LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: allocate failed" << LL_ENDL; return; }
    if (alloc.grew) { LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: ring buffer grew (PC-N-4 持越)" << LL_ENDL; }

    auto it = sDrawUboRingBufferRecords.find(alloc.buffer);
    if (it == sDrawUboRingBufferRecords.end() || it->second.mapped == nullptr) {
        LL_WARNS_ONCE("Vulkan") << "PC-N-1 writeDrawUbo: mapped lookup failed" << LL_ENDL;
        return;
    }

    // (N1-3) A = block_hash 単独 key、in-place 上書き許容
    std::memcpy(static_cast<U8*>(it->second.mapped) + alloc.offset + offset, data, size);

    // (N1-4) A = dirty 不要

    // (N1-1) A = dynamic offset 返却
    out_dynamic_offset = alloc.offset;
}
```

> **注**: `ubo::UboBlockMeta` / `ubo::lookupBlockMeta` の正確な名前は実装 phase で `forwardToUboUpload` SINGLETON case (= 通電済) の lookup pattern を参照して確定。

### §4.2 step (b) = `forwardToUboUpload` PER_DRAW case 通電 (`llglslshader.cpp`)

**file**: `indra/llrender/llglslshader.cpp:2151-2157`

**改変**:

```cpp
// 旧 (= PC-7γ-1 tag, 未通電)
case kCadencePerDraw:
    LL_WARNS_ONCE("Vulkan") << "PC-7γ-1: PER_DRAW forwardToUboUpload not wired yet (PC-7ε scope), block_hash=0x" << std::hex << loc.block_hash << std::dec << LL_ENDL;
    return;

// 新 (= PC-N-1 (b) tag、(N1-2) A + (N1-5) B 採用)
case kCadencePerDraw: {
    // <AYAstorm r41 PC-N-1 (b)>
    // (N1-2) A = setter 内 immediate allocate via writeDrawUbo
    // dynamic offset は caller (= draw record 経路) で別途 cache 必要、本 case は write のみ
    U32 dynamic_offset = 0u;
    LLVKLoader::writeDrawUbo(loc.block_hash, loc.offset, data, size, dynamic_offset);
    // dynamic_offset は draw record で bind 時に投入、本 case では cache せず
    // (= placeholder pool 経路のみ通電、real draw 経路 PC-N-5 で本格化)
    return;
}
```

> **注**: `dynamic_offset` を caller (= bind 経路) に伝達する mechanism は (N1-5) B 採用ゆえ placeholder pool 経路 (= `recordPlaceholderPoolDraw`) でのみ必要、`forwardToUboUpload` 経路では現 phase では cache せず (= 設定 → set=2 bind 経路の通電は PC-N-2 で本格化、PC-N-1 ではここを通すこと自体が API 経路通電確認になる)。

### §4.3 step (c) = `recordPlaceholderPoolDraw` 内 dummy memset → `writeDrawUbo` 経由 zero write 置換 (`llvkloader.cpp:5028-5054`)

**file**: `indra/llrender/llvkloader.cpp:5028-5054`

**改変** (= (N1-5) B 採用、tag block `<AYAstorm r41 PC-7ε (d)>` → `<AYAstorm r41 PC-N-1 (c)>`):

```cpp
// 旧 PC-7ε (d) 実装 (= 直接 allocate + memset 0)
const LLUboRingBuffer::AllocateResult alloc = sDrawUboRingBufferMgr->allocate(256);
if (!alloc.success) { LL_WARNS_ONCE("Vulkan") << "PC-7ε: ring buffer allocate failed" << LL_ENDL; return; }
if (alloc.grew) { LL_WARNS_ONCE("Vulkan") << "PC-7ε: ring buffer grew" << LL_ENDL; }
{
    auto it = sDrawUboRingBufferRecords.find(alloc.buffer);
    if (it != sDrawUboRingBufferRecords.end() && it->second.mapped != nullptr) {
        std::memset(static_cast<U8*>(it->second.mapped) + alloc.offset, 0, alloc.size);
    }
}
const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = { alloc.offset, alloc.offset, alloc.offset, alloc.offset };
bindV3aStatic(cmd_buf, sFrameIndex, dynamic_offsets);

// 新 PC-N-1 (c) 実装 (= writeDrawUbo 経由 zero write 置換)
// (N1-5) B 採用: zero data 維持、API 経路通電のみ確認
static const U8 zero_buf[256] = {};
constexpr U32 PLACEHOLDER_BLOCK_HASH = 0u; // 専用 placeholder block (= metadata 不在 → writeDrawUbo guard で sDrawUboRingBufferMgr 直接 fallback)
U32 dynamic_offset = 0u;
// 直接 ring buffer allocate (= writeDrawUbo lookup 失敗時の fallback path で同等動作)
const LLUboRingBuffer::AllocateResult alloc = sDrawUboRingBufferMgr->allocate(256);
if (!alloc.success) { LL_WARNS_ONCE("Vulkan") << "PC-N-1 (c): ring buffer allocate failed" << LL_ENDL; return; }
if (alloc.grew) { LL_WARNS_ONCE("Vulkan") << "PC-N-1 (c): ring buffer grew (PC-N-4 持越)" << LL_ENDL; }
{
    auto it = sDrawUboRingBufferRecords.find(alloc.buffer);
    if (it != sDrawUboRingBufferRecords.end() && it->second.mapped != nullptr) {
        // (N1-5) B = zero data 維持、real value 構築は PC-N-2 で本格化
        std::memset(static_cast<U8*>(it->second.mapped) + alloc.offset, 0, alloc.size);
    }
}
dynamic_offset = alloc.offset;
const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = { dynamic_offset, dynamic_offset, dynamic_offset, dynamic_offset };
bindV3aStatic(cmd_buf, sFrameIndex, dynamic_offsets);
```

> **注**: 上記は当初案 = `writeDrawUbo` helper 経由 zero write 置換に近い形だが、placeholder block は metadata 未登録 (= codegen 出力なし) ゆえ実装 phase で **2 path 選択肢** が出現する可能性:
> - **path α**: `writeDrawUbo` 内 metadata lookup を bypass する dummy variant (= `writeDrawUboRaw(U32 size, const void* data, U32& out_offset)`) helper 新設、placeholder 経路はこちらを使う
> - **path β**: `writeDrawUbo` の `mUboMetadata` lookup 失敗時 fallback 経路 (= guard 内で size 引数を信用して allocate) で対応
>
> 実装 phase で選定。design-lock phase では「dummy memset → API path 経由 zero write 置換 = API path 通電確認」が literal scope (= (N1-5) B record)、内部 helper 選択は実装 phase の自由度として残す。

### §4.4 step (d) = `flushDrawUbos` first-fire log 維持 + PC-N-4 grow hook placeholder (`llvkloader.cpp:4376-4383`)

**file**: `indra/llrender/llvkloader.cpp:4376-4383`

**改変** (= (N1-6) B 採用、tag block `<AYAstorm r41 PC-N-1 (d)>`):

```cpp
// 旧 PC-7δ 実装
void flushDrawUbos() {
    flushDummyUboWrite("flushDrawUbos");
}

// 新 PC-N-1 (d) 実装 (= first-fire log 維持 + PC-N-4 hook placeholder)
void flushDrawUbos() {
    // <AYAstorm r41 PC-N-1 (d)>
    // (N1-6) B = first-fire log のみ維持、per-draw allocate は setter 内 immediate ゆえ flush 側 no-op 等価
    // PC-N-4 持越: AllocateResult.grew 集約 + vkUpdateDescriptorSets 再発火 hook をここに追加予定
    static std::atomic<bool> first_fire{true};
    if (first_fire.exchange(false, std::memory_order_acq_rel)) {
        LL_INFOS("Vulkan") << "PC-N-1: flushDrawUbos first fire (per-draw write は setter 内 immediate、本 flush は no-op + PC-N-4 grow hook placeholder)" << LL_ENDL;
    }
    // PC-N-4 持越 hook: if (sDrawUboRingBufferGrewThisFrame) { wireDrawUboSetV3aToRingBuffer(); sDrawUboRingBufferGrewThisFrame = false; }
}
```

> **注**: `flushDummyUboWrite("flushDrawUbos")` 呼出は撤去 (= dummy allocate 経路は `recordPlaceholderPoolDraw` 経由通電で代替)。`flushDummyUboWrite` helper 自体は他 cadence (= `flushFrameUbos` 等) からも呼ばれている可能性があるため、`flushDrawUbos` 経由の dummy allocate のみ撤去、helper 本体は残置。

### §4.5 step (e) = `flushDummyUboWrite` helper の `flushDrawUbos` 経路撤去 review

**file**: `indra/llrender/llvkloader.cpp:4281-4318`

**作業**: `flushDummyUboWrite` helper を grep し、`flushDrawUbos` 以外の caller がいるか確認。

- いる場合: `flushDummyUboWrite` helper 本体は残置、`flushDrawUbos` のみ呼出撤去
- いない場合: `flushDummyUboWrite` helper 本体も撤去候補 (= 別 sub-step 持越、PC-N-1 scope 外)

PC-N-1 では up to grep 確認 + 必要なら他 caller 1 件分の影響評価のみ、helper 本体撤去は scope 外。

### §4.6 step (f) = build verify

**実行内容** (= PC-7ε 同形パターン):

```bash
cd /home/ishikawa/work_firestorm/phoenix-firestorm/build-linux-x86_64
make -j4 llrender
# 期待: PASS + ERROR 0 + WARNING 0
make -j4 INTEGRATION_TEST_lluboringbuffer
ctest -R lluboringbuffer
# 期待: 11/11 PASS
make -j4 INTEGRATION_TEST_llassetubopool
ctest -R llassetubopool
# 期待: 10/10 PASS
make -j4 INTEGRATION_TEST_llpipelinecachestorage
ctest -R llpipelinecachestorage
# 期待: 13/13 PASS
cd /home/ishikawa/work_firestorm/phoenix-firestorm/scripts/ubo_codegen
python -m unittest discover -s tests -v
# 期待: 131/131 PASS
```

### §4.7 step (g) = handoff complete doc 起案 + AYA commit 指示後 commit

**作業**:

1. `handoff-...-pc-n-1-complete.md` 起案 = §0 着手契機 + §1 必読 + pinpoint reference + §2 実装内容 (= step (a)-(g) 全実装記録 + 改変サマリ表) + §3 build verify 結果 + §4 Exit Criteria 10 項充足 + §5 残 strict 線形 + §6 milestone state + §7 self-verify 9 観点 + §8 次 session 着手 1 line + §A feedback 遵守 record
2. AYA literal 「commit してください」受領後 git add 個別 file + commit (= Co-Authored-By 不在 + 個別 file 指定)

---

### §4.8 GATE-B 整合 (全 step 共通)

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = `project_r41_phase1b_vulkan_host_gate` 遵守。host 側 redirect 層は `mUseUBO` runtime flag のみで gate (= GATE-B 確定 2026-06-04)。

PC-N-1 改変箇所:
- `forwardToUboUpload` PER_DRAW case = `mUseUBO=true` 時のみ発火 (= 既存 setter 経路 internal、`#ifdef` 不要)
- `writeDrawUbo` helper = anonymous namespace or `LLVKLoader::` namespace 内、Vulkan-only code (= `#ifdef` 不要、Vulkan unit 自体が gate)
- `recordPlaceholderPoolDraw` = Vulkan placeholder 経路、`mUseUBO` 不問、起動時 1 度のみ (= `#ifdef` 不要)
- `flushDrawUbos` = `mUseUBO=true` 時のみ意味、現 placeholder phase では log only (= `#ifdef` 不要)

### §4.9 MUSEUBO-A 整合 (全 step 共通)

`mUseUBO=false` default で既存 OpenGL 描画 100% 維持:
- `forwardToUboUpload` = `mUseUBO=true` 時のみ呼ばれる (= 06a §5.2 path 分岐、`mUseUBO=false` で uniform 直接設定 OpenGL path)
- `writeDrawUbo` = `forwardToUboUpload` 経由 + `recordPlaceholderPoolDraw` 経由のみ呼ばれる (= Vulkan-only)
- `recordPlaceholderPoolDraw` = placeholder offscreen FBO 経路、視覚 no-op 等価維持
- `flushDrawUbos` = `LLPipeline::renderGeom()` 入口前で呼ばれるが (= 06b §4.1)、現 placeholder phase では log only ゆえ実 OpenGL 描画影響ゼロ

---

## §5. PC-N-1 Exit Criteria 10 項

| # | Criteria |
|---|----------|
| (i) | `writeDrawUbo(U32 block_hash, U32 offset, const void* data, size_t size, U32& out_dynamic_offset)` helper 新設 (= `writeFrameUbo` 同形 signature + offset 返却) |
| (ii) | `forwardToUboUpload` PER_DRAW case 通電 (= `LL_WARNS_ONCE` diagnostic → `writeDrawUbo` 呼出置換) |
| (iii) | `recordPlaceholderPoolDraw` 内 dummy memset → API 経路通電 zero write 置換 ((N1-5) B 採用、real value 構築は PC-N-2 持越) |
| (iv) | `flushDrawUbos` first-fire log 維持 + PC-N-4 grow hook placeholder ((N1-6) B 採用) |
| (v) | codegen set=2 binding 配置現状 (= binding=0/1) 維持、binding=2/3 配置 + 4 binding 再分配は別 sub-step ((N1-7) A 採用) |
| (vi) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 |
| (vii) | MUSEUBO-A 整合 = `mUseUBO=false` default で既存 OpenGL 描画 100% 維持 |
| (viii) | build verify = `llrender` build + ERROR 0 / WARNING 0 + INTEGRATION_TEST 11+10+13 全 PASS + codegen unittest 131/131 PASS |
| (ix) | tag block 統一 = `<AYAstorm r41 PC-N-1 (a)>` (writeDrawUbo helper) + `<AYAstorm r41 PC-N-1 (b)>` (PER_DRAW case) + `<AYAstorm r41 PC-N-1 (c)>` (recordPlaceholderPoolDraw 置換、`<PC-7ε (d)>` → `<PC-N-1 (c)>` update) + `<AYAstorm r41 PC-N-1 (d)>` (flushDrawUbos) |
| (x) | handoff complete doc 起案 + AYA commit 指示後 commit (= Co-Authored-By 不在 + 個別 file 指定 + 新 file 0 除 doc + CMake 改変 0 + settings.xml 改変 0) |

---

## §6. 着手手順 (= 次 session で PC-N-1 実装 phase 着手)

1. AYA 指示「PC-N-1 実装着手お願いします」literal 受領待ち
2. 本 PC-N-1 design-lock doc 全文 Read (= 必読 1 件)
3. `writeFrameUbo` / `writeSingletonUbo` 実装 (= `llvkloader.cpp:4611-4676`) pinpoint Read = `writeDrawUbo` helper 雛形参考
4. `recordPlaceholderPoolDraw` 内 PC-7ε (d) tag block (= `llvkloader.cpp:5010-5054`) pinpoint Read
5. `forwardToUboUpload` PER_DRAW case (= `llglslshader.cpp:2139-2227`) 全文 Read + SINGLETON case (= 通電済) 比較
6. step (a) `writeDrawUbo` helper 新設 → step (b) PER_DRAW case 通電 → step (c) recordPlaceholderPoolDraw 置換 → step (d) flushDrawUbos 整理 → step (e) flushDummyUboWrite 影響 review → step (f) build verify → step (g) handoff complete doc 起案
7. Exit Criteria 10 項 self-verify 全 ✅
8. AYA literal「commit してください」受領後 git add 個別 file + commit

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + **PC-N-1 design-lock ✅ 本 commit** + PC-N-1 実装 ⏳ 次 session + PC-N-2 design-lock ⏳ + PC-N-2 実装 ⏳ + PC-N-4 design-lock ⏳ + PC-N-4 実装 ⏳ + PC-N-3 design-lock ⏳ + PC-N-3 実装 ⏳ = Phase 1.C complete ⏳ + PC-8 (3 OS build verify) ⏳ + PC-N-5 = Phase 1.D 着手起点 ⏳

---

## §8. self-verify 9 観点 全 ✅

1. **PC-N-1 literal scope 5 件 §0 完全分解** = `forwardToUboUpload` PER_DRAW case 通電 + `writeDrawUbo` helper 新設 + `recordPlaceholderPoolDraw` 置換 + `flushDrawUbos` first-fire log + codegen 配置維持 ✅
2. **必読 1 件 §1 + pinpoint reference 5 件 別記** ✅
3. **現状調査 §2 8 項 + codegen 配置現状表 + design doc 4 章網羅** ✅
4. **ambiguity (N1-1)..(N1-9) 9 件 AYA literal「OK」record (2026-06-05) §3** ✅
5. **採用根拠 9 件明文化 §3** ✅
6. **実装計画 (a)-(g) 7 step 分解 §4** ✅
7. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.8** ✅
8. **MUSEUBO-A 整合 = `mUseUBO=false` default 経路不変 §4.9** ✅
9. **Exit Criteria 10 項明文化 §5 + `indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 整合** ✅

---

## §9. 次 session 着手 1 line

**PC-N-1 実装着手** = step (a)-(g) 7 step 実施 = (a) `writeDrawUbo(U32 block_hash, U32 offset, const void* data, size_t size, U32& out_dynamic_offset)` helper 新設 + (b) `forwardToUboUpload` PER_DRAW case 通電 (= `LL_WARNS_ONCE` diagnostic → `writeDrawUbo` 呼出置換) + (c) `recordPlaceholderPoolDraw` dummy memset → API 経路通電 zero write 置換 + (d) `flushDrawUbos` first-fire log 維持 + PC-N-4 hook placeholder + (e) `flushDummyUboWrite` 影響 review + (f) build verify (= llrender + warning 0 + TUT 11+10+13 + codegen 131/131) + (g) handoff complete doc 起案、Exit Criteria 10 項全充足、AYA literal「commit してください」受領後 commit。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 5 件 別記、本 session も Explore agent 経由 pinpoint 取得のみ、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §8
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定
- **feedback_no_scope_shrink** 遵守 = PC-N-1 literal scope 5 件 §0 完全分解、(N1-5) B (= zero write 経由置換) + (N1-7) A (= 現 codegen 配置維持) は AYA literal「OK」record 済段階分離 = 縮小ではない (= PC-7ε パターン同形、real value + 4 binding 再分配は別 sub-step record 済)
- **feedback_doubt_self_first** 遵守 = ambiguity 9 件発見 + 重要 gap 2 件 ((N1-5) scope 縮小該当の可能性 + (N1-7) codegen 配置現状 vs spec 差異) 発見で停止 + 推奨案提示 + AYA literal「OK」確認後 design-lock doc 起案、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 9 件 batch AYA 確認 (2026-06-05)、scope 縮小該当の可能性 (= (N1-5)) は AYA literal 明示提示 + literal「OK」受領で段階分離確定、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-1 = `flushDrawUbos` real per-draw data write 通電単独 sub-step、PC-N-2 (set=2 復活) + PC-N-3 (bone storage) + PC-N-4 (grow re-wire) + PC-N-5 (実 GLTF draw) は分離、本 doc 起案も PC-N-1 単独 design-lock のみ
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-1 design-lock phase は doc 起案のみ、`indra/` 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N1-1)..(N1-9) 各 ID に項目名 / 採用案内容併記 §3、(a)..(g) 各 step に作業内容併記 §4
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定予定

---
