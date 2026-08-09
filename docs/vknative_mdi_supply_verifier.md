# VK-native MDI Per-Draw 供給検証器（恒久検出器・設計 doc v1）

> 目的 = **MDI 化した全 draw が `firstInstance` で間接参照する per-draw 供給（DrawData スロット＋コマンド直値）が、当該 draw の内容と一致し、参照後に別内容で上書きされていないこと**を、全 MDI サイト一律・恒久・**視覚に頼らず fail-closed で gate 可能**な形で機械検証する。
>
> 動機 = 自動 MDI 化により、per-draw 供給の stale/取り違えが **texture だけでなく glow / alpha / offset / color / spec / object_alpha を含むあらゆるフィールド**で発生し得る（例: emissive default 画像を使う particle は texture も色も同一で、差は glow/alpha だけ＝texture 照合では拾えない）。影に別影画像が混入したバグ（GitLog・既修正）も同型。**Rigged が壊れないのは「毎フレーム書き直す＝不変条件を常に満たす」から**であり、その他は不変条件を無意識に破っている。本器はこの不変条件の破れを直接検出する。

---

## 0. 現状 scaffold との関係（重複でなく一般化）

既存 `llvkcontract` に per-draw ID の弱い検査が在る（本器はこれを包含・置換しない・拡張する）:

| 既存 | 何を見るか | 限界 |
|---|---|---|
| `stashDrawDataID`/`checkDrawDataIDAtFire`（`llvkcontract.cpp:430-448`・`C_DRAWDATA_ID_MISMATCH`） | fire 時の `firstInstance` == 直前 stash した id か | **id 等価のみ**（内容は見ない）・**thread-local 単値**（全 slot を追えない）・**verbose 限定**（gate 不可） |
| `checkPerDrawIDFreshnessAtFire`（:455・`C_SKIN_DRAW_NO_COMMIT`） | skin draw が commit 済みか | skin set 限定・verbose 限定 |

本器の差分 = **(1) 内容指紋**（16-uint 全フィールド＋コマンド geometry）**(2) 全 slot の shadow 表**（取り違え・上書きを追える）**(3) 常時 ON・fail-closed**（gate 可能）。既存 `checkDrawDataIDAtFire` は「id 等価」層として残置し、本器が「内容・寿命」層を足す。

---

## 1. 核心不変条件（1文・gate 命題様式）

> **MDI コマンドが `firstInstance=id` で参照する per-draw スロットは、当該フレーム内に当該 draw の内容として author され、参照後に別内容で上書きされていないこと。**

対象 = `mVkUsesHeapSet || mVkUsesSkinSet` のシェーダ（＝DrawData を実読するもの）。非 bindless（conventional bind・INHERIT 正当）は対象外（既存 guard `llvertexbuffer.cpp:562` と同一条件で除外）。

---

## 2. 対象 = per-draw 供給の全分類（texture/geom に限定しない）

| 系統 | 実体 | stale/取り違えするか | 混入で見える不体裁 |
|---|---|---|---|
| **(A) 間接参照 = DrawData 16-uint**（`computeDrawDataSlots` `lldrawpool.cpp:517`） | slot[0..3]=tex heap slots / [4..7]=spec_color / [8]=emissive_brightness(fullbright) / [9]=env_intensity / [10]=minimum_alpha / [11]=sss / [12]=object_alpha | **する**（scratch 再利用・in-place 上書き・id 取り違え） | 別画像 / 質感 / **glow 有無** / 反射 / **alpha 欠け** / 肌 / **透明度** |
| **(B) コマンド直値 = `VkDrawIndexedIndirectCommand`** | indexCount / firstIndex / vertexOffset(=geometry offset) | build ミスのみ（stale せず＝コマンドに inline） | 別メッシュ / ずれ |
| **(C) run 不変量 = flush 単位共有** | model matrix / blend func / pipeline / bound set | run 割り忘れで混在 | 位置/合成モード違い |

指紋 `H = fnv1a( slot[0..12] )`（13 uint = GPU が実読する A の全バイト）。(B) は flush 時にコマンド↔source 照合。(C) は flush 時に単一性 assert。**∴ texture が同一でも [8][10][12] の差で必ず落ちる。**

### 2.1 2 つの真値レイヤ（α 供給 / β 実体）— 両方見ないと当初 particle バグを取り逃す

per-draw の「正しさ」は 2 段で、**別々に破れる**:

- **α（供給）= DrawData が指す slot 番号・glow・alpha… が当該 draw の値か**。上表の指紋 H が担当。stale/取り違え供給を捕捉（影の baked template stale＝この層）。
- **β（実体）= その slot 番号の heap ディスクリプタ `ayaTexHeap[slot]` が当該 draw の texture view を保持しているか**。**当初の particle 混入はこの層**（`computeDrawDataSlots` の slot[0] は `vkHeapSlotOrDefault(自分のtex)` ＝**番号は正しい**が、heap[番号] が residency churn/in-place 上書きで**別 tex の view**を保持＝別画像。α の指紋は「番号が正しい」ので**沈黙する**）。

∴ 本器は **β も一次検査に含める**（下 `C_MDI_HEAP_IDENTITY`）。これが「texture/geometry だけ見る検出器では取れない」の texture 版であり、当初バグを名指す本命。β は draw が持つ `params.mTexture`（多 tex は `mTextureList[i]`）の view と、`llvkloader` が保持する **slot→view の CPU shadow**（`bindlessWriteSlotInternal` が既に書く実体・residency refactor が所有）を照合する。

---

## 3. 失敗タクソノミー（新 cause・fail-closed）

`llvkcontract.h` `enum ECause` に追加（`CAUSE_COUNT` 直前・既存値は不変＝憲法4 の凍結を破らない追加）:

| cause | 発火 | 意味 |
|---|---|---|
| `C_MDI_STALE` | reference 時、`shadow[id].author_frame != cur_frame` | id が当該フレームに author されていない＝前フレーム残骸/INHERIT 盲参照/未 refresh（Rigged 以外の「書き直さない」病） |
| `C_MDI_OVERWRITE` | author 時、`shadow[id].ref_frame==cur_frame && shadow[id].hash != h` | 参照済みスロットが同フレーム内に別内容で上書き＝scratch 再利用/in-place/id 衝突（**GPU が実読する破れそのもの**） |
| `C_MDI_HEAP_IDENTITY` | reference/flush 時、`heapShadow[slot[i]] != params->mTextureList[i] の view`（slot が既定でない時のみ） | **β 層＝当初 particle 混入の本命**。番号は正しいが heap 実体が別 tex（residency churn/in-place 上書き）＝別画像混入 |
| `C_MDI_RUNINV` | flush 時、run 内でコマンド間の model matrix/blend 不一致 | MDI run 割り忘れ |
| `C_MDI_GEOM` | flush 時、コマンド geometry ≠ source draw の geometry | コマンド build 取り違え（副次） |

> `C_MDI_HEAP_IDENTITY` は **既定 slot（未 resident fallback = 白）を除外**して発火する（未 resident の白は別欠陥＝既存 `fb_heap_default` が計上）。∴ 「**実 slot を得たのに別 tex を指す**」真の混入のみを名指す＝当初バグと 1:1。

全て `LLVKContract::cause()` 経由＝既存 VKC-SUM `cause{…}` に載り、憲法2 の未承認 alarm として **gate ブロック**。

---

## 4. 状態機械（shadow 表）

llvkcontract.cpp に置く（llrender 層＝llvertexbuffer の fire からも触れる）:

```cpp
struct MdiSlot {
    U64 hash        = 0;            // 最後に author された slot[0..12] の指紋
    U32 author_frame = 0xFFFFFFFFu; // 最後に author したフレーム
    U32 ref_frame    = 0xFFFFFFFFu; // 最後に reference されたフレーム
};
static MdiSlot* sMdiShadow = nullptr;   // [DRAWDATA_TOTAL_SLOTS] = 1,048,576 要素
// 常時 ON = 16 B × 1,048,576 ≈ 16 MiB（§8 でコスト裁定）
```

- サイズ源 = `DRAWDATA_TOTAL_SLOTS`（`llvkloader.cpp:323`=1048576）。llvkloader が DrawData buffer を確保する初期化点（`llvkloader.cpp:3304` 近傍）から `LLVKContract::mdiInit(DRAWDATA_TOTAL_SLOTS)` を呼び確保。
- フレーム境界 = 既存 `LLVKContract::frameBegin()`（`llvkcontract.cpp` `frameBegin`）。`cur_frame` は既存 `sMonotonicFrameCount`（`llvkloader.cpp:715`）を llvkcontract に供給（`frameBegin(U32 frame)` 拡張 or getter）。**明示 clear 不要**＝frame 比較で判定（前フレームの author_frame は != cur なので自然に stale 扱い）。
- verbose 時のみの副表（名指し用）: `sMdiSrc[id] = {U64 author_src_key, U64 ref_src_key}`（`setResolvers` の key/describe で obj/tex UUID 解決）。always-on では確保しない。

---

## 5. WHERE = choke 2 点＋fire 検証（全 MDI サイト一律）

### 5.1 AUTHOR フック（内容を stamp・毎回 author_frame 更新）

**唯一の author 論理点 = `LLRenderPass::establishPerDrawId`（`lldrawpool.cpp:565`）**。id 確定直後（`return id` 前・`lldrawpool.cpp:599`）:

```cpp
if (cur->mVkUsesHeapSet || cur->mVkUsesSkinSet)   // 既存 guard と同条件
    LLVKContract::mdiAuthor(id, LLVKContract::mdiHash(slots), (const void*)params);
```
- **memo/cache hit でも毎回呼ぶ**（`ensureVkDrawDataSlot` が内容不変で slot 再利用しても author_frame を cur に更新）→ 正規 id は常に author_frame==cur ＝ §6 の V1 が cross-frame content-cache で誤発火しない。
- `mdiHash(slots)` = inline `fnv1a(slots, 13*4)`（slot[0..12]）。
- 直 MDI replay で `establishPerDrawId` を通らず `computeDrawDataSlots`+`firstInstance` を直書きする 2 site も同じ mdiAuthor を差す:
  - opaque bucket replay（`lldrawpool.cpp:1431` computeDrawDataSlots → :1435 `cmds[c].firstInstance`）
  - rigged indirect（`lldrawpool.cpp:1586` computeDrawDataSlots → :1591 draw_id）

### 5.2 REFERENCE フック（参照を mark＋V1 stale 判定）

**`firstInstance=id` を打つ全点を単一 helper に集約**（副産物 = MDI draw-id 供給の単一 choke）:

```cpp
inline void LLRenderPass::mdiSetFirstInstance(VkDrawIndexedIndirectCommand& c, U32 id, const void* src) {
    c.firstInstance = id;
    LLVKContract::mdiReference(id, src);   // 内部で heap/skin guard・非対象は no-op
}
```
差し替え対象（現状の直代入を helper へ）:
| site | file:line |
|---|---|
| appendAlphaRunCmd（alpha/particle/emissive） | `lldrawpoolalpha.cpp:975` |
| opaque bucket replay | `lldrawpool.cpp:1435` |
| rigged indirect | `lldrawpool.cpp:1580-1592` |
| bucket | `llvkbucket.cpp:362` 近傍 |
| GLTF scene | `gltfscenemanager.cpp:809 / 822` |
| 影 MDI（slice A/B） | 影 record→cmd 構築点（要 file:line 確定・§12 未決） |
| inline（非 MDI・fi 直渡し） | `llvertexbuffer.cpp:567/610/658`（既存 `checkDrawDataIDAtFire` 隣に `mdiReference(fi, currentDrawInfo())`） |

### 5.3 FLUSH/FIRE 検証（V2 保険・V3・V2 geom）

`flushAlphaRun`（`lldrawpoolalpha.cpp:983`）と bucket flush の `vkCmdDrawIndexedIndirect` 直前:
- 各 cmd: `LLVKContract::mdiVerifyAtFlush(cmd.firstInstance, cmd)` → shadow[id].hash が reference 後に変わっていないか（V2 の最終保険）＋ cmd geometry ↔ source（`C_MDI_GEOM`）。
- run 単位: `mdiRunInvariant(run.mModelMatrix, run.mShader, blend)` を span 頭で確認（`C_MDI_RUNINV`）。

---

## 6. 検証ロジック（擬似コード・確定仕様）

```cpp
// llvkcontract.cpp — cur = 現フレーム（sMonotonicFrameCount）

void mdiAuthor(U32 id, U64 h, const void* src) {
    if (sMdiShadow == nullptr || id >= DRAWDATA_TOTAL_SLOTS) return;   // INHERIT/scratch外は範囲外→skip
    MdiSlot& s = sMdiShadow[id];
    if (s.ref_frame == cur && s.hash != h)                            // V2: 参照済みを別内容で上書き
        causeNamed(C_MDI_OVERWRITE, mdiName(id, /*victim*/s, /*culprit*/src));
    s.hash = h;
    s.author_frame = cur;
    if (verbose) sMdiSrc[id].author_src_key = keyOf(src);
}

void mdiReference(U32 id, const void* src) {
    LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
    if (sh == nullptr || !(sh->mVkUsesHeapSet || sh->mVkUsesSkinSet)) return;  // 非対象=no-op
    const U32 slot = (id == PERDRAW_SLOT_INHERIT) ? 0u : id;
    if (sMdiShadow == nullptr || slot >= DRAWDATA_TOTAL_SLOTS) return;
    MdiSlot& s = sMdiShadow[slot];
    if (s.author_frame != cur)                                        // V1: 当該フレーム未 author
        causeNamed(C_MDI_STALE, mdiName(slot, s, src));
    s.ref_frame = cur;
    if (verbose) sMdiSrc[slot].ref_src_key = keyOf(src);
}
```

### 6.1 β 実体照合（当初 particle バグの本命・newview 層）

layering: `mdiAuthor/mdiReference` は llvkcontract（llrender）で LLDrawInfo を deref 不可。∴ β は **LLRenderPass 層**（newview・params と slots を持つ）で実行し cause だけ llvkcontract に上げる:

```cpp
// LLRenderPass::establishPerDrawId 末尾（slots 確定後・§5.1 の author 隣）
if ((cur->mVkUsesHeapSet) && params != nullptr) {
    const U32 n = (batch_textures && params->mTextureList.size() > 1)
                  ? llmin((U32)params->mTextureList.size(), 4u) : 1u;
    for (U32 i = 0; i < n; ++i) {
        const U32 slot = slots[i];
        if (slot == 0 || slot == defaultHeapSlot()) continue;      // 未 resident 白は除外(fb_heap_default 管轄)
        LLTexture* t = (n > 1) ? params->mTextureList[i].get() : params->mTexture.get();
        VkImageView intended = t ? t->getGLTexture()->getVkImageView() : VK_NULL_HANDLE;
        VkImageView actual   = LLVKLoader::bindlessSlotView(slot);   // slot→view CPU shadow(要 expose)
        if (intended != VK_NULL_HANDLE && actual != VK_NULL_HANDLE && actual != intended)
            LLVKContract::causeNamed(C_MDI_HEAP_IDENTITY,
                mdiHeapName(slot, /*intended tex*/t, /*actual*/actual));   // 実 slot が別 tex を保持
    }
}
```
- `LLVKLoader::bindlessSlotView(U32 slot)` = `bindlessWriteSlotInternal`（`llvkloader.cpp:3006`）が既に slot 毎に書く view を CPU shadow（`sBindlessSlotView[slot]`・1M×8B もしくは residency refactor の既存表）から返す薄い accessor。**新規 GPU readback 不要**。
- これが名指す = 「draw=SimRez particle・slot=S・intended=SimRez tex・actual=avatar tex」＝**当初バグの機械証明**。

**不変条件の充足証明（false-positive が無いこと）**:
- 正規 heap/skin draw: establishPerDrawId が id を返す度に mdiAuthor 実行 → author_frame==cur → reference で V1 沈黙。
- 同一内容 memo 共有（scratch dedup / persistent memcmp 一致）: 2 draw が同 slot 共有でも hash 同一 → V2 沈黙（同内容 = 無害＝視覚不体裁なし）。
- cross-frame persistent cache（内容不変で slot 跨フレーム再利用）: 毎フレーム mdiAuthor が author_frame を更新 → V1 沈黙。
- 非 bindless draw（INHERIT 正当）: guard で no-op → 誤発火なし。

**真陽性（捕捉）**:
- INHERIT を bindless shader が引く（slot0 盲参照）: author 無 → author_frame != cur → **V1**。
- scratch/persistent slot が参照後に別内容で上書き: **V2**（今回の particle・影混入の中核機序）。
- id が別 draw の slot を指し内容が違う: その slot は本 draw 用に author されていない → author_frame 経路 or V2 で捕捉。

---

## 7. HOW 報告（名指し内容）

`mdiName(id, victim, culprit)` = verbose 時に `describe()` resolver で:
`pass / shader / slot=id / victim=<obj·tex UUID or geom key> / culprit=<同> / diff=<16uint 人間可読差>`。
diff 例: `"glow 1→0" "obj_alpha 0.5→1.0" "texslot#0 0x1a→0x07" "min_alpha 0.5→0.0"`（author 側が保持する前 slot 値と新値の field 単位 diff）。always-on では cause カウントのみ（名指し不要＝gate は数で判定）。

---

## 8. 常時 ON 設計・コスト裁定

| 項 | clean path コスト |
|---|---|
| mdiAuthor | fnv1a(52B) 1 + int 比較 1 + store（~18k/frame） |
| mdiReference | int 比較 1 + store（~18k/frame） |
| flush 保険 | cmd 毎 int 比較（run 毎） |
| メモリ | 16 MiB 常駐（1M slot × 16B） |

合計 < 0.1 ms/frame 見込み（VKC 通常無音の設計を踏襲）。詳細 diff は**違反時のみ**。**∴ 常時 ON 可**。16 MiB が過大なら **代替**: DrawData slot の spare uint（[13][14][15]＝現 0・未 shader 参照）に `hash_lo/hash_hi/author_frame` を同居させ shadow 表を廃す（追加メモリ 0）。ただし ref_frame は SSBO に置けない（別 1M×4B bitset/stamp 要）＝§12 で裁定。**v1 は 16 MiB 別表で確定**（実装単純・SSBO レイアウト非改変＝憲法4 影響最小）。

---

## 9. gate 統合（憲法2・視覚に頼らない）

- `C_MDI_STALE / C_MDI_OVERWRITE / C_MDI_HEAP_IDENTITY / C_MDI_RUNINV / C_MDI_GEOM` は既定で **未承認 alarm**（allowlist 非登録）→ VKC-SUM 非ゼロ = **PASS 不可**。
- これにより「人間の目で混入を探す」を廃し、**静的 1 フレームでも走行中でも機械 gate**。Rigged が clean（沈黙）であることも同 gate が機械証明。

---

## 10. 実装 Brief（ファイル別・直列 1 実装単位）

1. **`llvkcontract.h`**: `ECause` に 5 cause 追加（`C_MDI_STALE/OVERWRITE/HEAP_IDENTITY/RUNINV/GEOM`・`CAUSE_COUNT` 前）。API 宣言 `mdiInit(U32)`, `mdiAuthor(U32,U64,const void*)`, `mdiReference(U32,const void*)`, `mdiVerifyAtFlush(U32,const VkDrawIndexedIndirectCommand&)`, `mdiRunInvariant(...)`, inline `mdiHash(const U32*)`。`CAUSE_NAMES[]` に 5 名追加（既存配列・要同期）。
2. **`llvkcontract.cpp`**: `sMdiShadow` 確保/解放、§6 ロジック、`frameBegin` に cur 供給、verbose 副表。
3. **`llvkloader.cpp/.h`**: DrawData buffer 確保点（:3304 近傍）で `LLVKContract::mdiInit(DRAWDATA_TOTAL_SLOTS)`。`frameBegin` へ `sMonotonicFrameCount` 供給。**β 用**: `bindlessWriteSlotInternal`（:3006）が書く view を `sBindlessSlotView[slot]` CPU shadow に保存（residency refactor が同型表を持つなら再利用）、accessor `VkImageView bindlessSlotView(U32)` と `U32 defaultHeapSlot()` を expose。
4. **`lldrawpool.cpp`**: `establishPerDrawId` 末尾に mdiAuthor（§5.1）＋ **β heap-identity 照合（§6.1）**。opaque(:1431)・rigged(:1586) replay に mdiAuthor（＋β）。`mdiSetFirstInstance` helper 定義。
5. **`lldrawpoolalpha.cpp`**: appendAlphaRunCmd(:975) を helper 化。flushAlphaRun(:983) に flush 検証。
6. **`llvertexbuffer.cpp`**: 3 draw 経路（:567/610/658）の `checkDrawDataIDAtFire` 隣に mdiReference。
7. **`llvkbucket.cpp` / `gltfscenemanager.cpp` / 影 MDI**: firstInstance 直代入を helper 化。
8. deploy = 3-path binary 同期（.h 改修含む＝フルビルド）。

**PoC 先行**（施主指示）: まず **particle 1 経路（appendAlphaRunCmd + establishPerDrawId author）** だけ配線し当初バグで発火実証 → 機構確定後に全 site 水平展開。

---

## 11. 自己監査（correctness / gap / hidden）

### 11.1 correctness
- **C1** V2 は「参照済み(cur) かつ hash 差」でのみ発火 ＝ GPU が実読する slot を同フレーム内で書き換える唯一の可視破れに一致。同内容共有は hash 一致で沈黙＝視覚無害を正しく除外。
- **C2** V1 は「author_frame != cur」＝正規 draw は毎フレーム mdiAuthor で refresh されるので、残るのは INHERIT 盲参照/未 author のみ＝真の stale。cross-frame cache 誤発火は mdiAuthor 毎回呼びで構造排除。
- **C3** guard（heap/skin set）で非 bindless を除外＝既存 `checkDrawDataIDAtFire` と同条件＝conventional bind の INHERIT を誤検出しない。
- **C4（β）** heap-identity は「実 slot（≠既定）を得たのに heap[slot] の view ≠ 当該 draw の tex view」でのみ発火＝当初 particle 混入（番号正・実体別）を直接名指す。未 resident 白（既定 slot）は除外＝`fb_heap_default` と非重複。**α が沈黙し β が鳴る**のが particle バグの想定署名（α=供給正・β=実体誤）。

### 11.2 gap
- **G1 GPU 側**: 本器は CPU 記録整合のみ。DrawData buffer を GPU/worker が直接壊す/torn read（UPDATE_AFTER_BIND）は対象外＝residency doc の (iii)/(iv) 層が担当。両器は相補（本器=どの slot を参照するかの整合、residency=slot descriptor の生存）。
- **G2 vertex buffer 内容**: 頂点色/頂点座標そのものの誤り（geometry inline）は cmd geometry 照合(C_MDI_GEOM)で offset/count は見るが、**buffer 内の bytes** は見ない（別課題）。
- **G3 flush 後〜submit 間の author**: 記録が flush 後も続き同 slot を別内容で author する経路があれば V2 は author 側で捕捉するが、flush 保険は flush 時点値のみ。submit 直前の最終 sweep は v1 では入れない（コスト）＝author フックで十分と判断（要監査確認）。
- **G4 32-bit ハッシュ衝突**: fnv1a 64bit 採用で pairwise 衝突 ~2^-64＝無視可。
- **G5（β の slot→view shadow 所有）**: β は `bindlessSlotView(slot)` の CPU shadow に依存。この表の書き手は residency refactor（working tree・未 commit）が既に持つ可能性大＝**二重管理を避け同一表を再利用**する（実装時に residency 側と突合・§12 未決）。shadow が in-place 上書き時に torn read され得る点は residency 層の責務で、β は「記録された最新 view」を読む（GPU 実体との最終一致は residency が保証）。
- **G6（β の解決タイミング）**: `t->getGLTexture()->getVkImageView()` を reference 時に読む＝draw 記録スレッド（main）。worker が同 tex を再 upload 中の view 遷移は latent race だが、mismatch は「別 tex の view」であって「同 tex の新旧 view」ではない＝別 UUID 判別なので churn 過渡の偽陽性は起きにくい（要 PoC 確認）。

### 11.3 hidden
- **H1 CAUSE_NAMES 同期**: `ECause` と `CAUSE_NAMES[]` の添字ずれ＝既存の隠れ規約。追加時に必ず同期（監査項目）。
- **H2 frame counter の供給スレッド**: `sMonotonicFrameCount` を llvkcontract が読むタイミング（main 記録前提）。off-main author（texture worker は DrawData を author しない＝establishPerDrawId は記録スレッド）＝前提成立、但し要確認。
- **H3 mVkUsesHeapSet の bind 前後**: mdiReference は `sCurBoundShaderPtr` を見る＝reference 時に正しいシェーダが bound 済み前提（appendAlphaRunCmd は bind 後）。inline 経路も drawRange 内＝bound 済み。
- **H4 shadow 未確保タイミング**: bindless 有効化前の draw は sMdiShadow==null で no-op（安全）。有効化後の初回フレームは author_frame 既定 0xFFFFFFFF ≠ cur → 初回のみ V1 誤発火し得る＝mdiInit で全 slot author_frame を「cur-1 でなく無効値」にし、reference 側で「author_frame==0xFFFFFFFF は未使用 slot＝warmup」を 1 フレーム猶予（要実装注記）。

### 11.4 監査結論
- **α（V1 stale / V2 overwrite）** は false-positive を構造排除しつつ、**影混入（baked template stale）を捕捉**する見込み。
- **β（heap-identity）** は **当初 particle 混入（番号正・実体別）を直接名指す**本命＝この検出器を作る主目的の 1 つを満たす。想定署名 = **α 沈黙・β 発火**。
- いずれも **正のトレースで主張**（走行での確認は gate であって可否根拠でない・憲法6）。
- 実装前に詰める点 = **H4（warmup 初回誤発火）/ G3（submit 直前 sweep）/ G5（β slot→view shadow の residency 側との一元化）/ 影 MDI の file:line**。**独立監査（実装後）必須**＝施主指示。

---

## 12. 申告欄（縮小・省略・未決）

- **未決①**: 影 MDI（slice A/B）の firstInstance 構築 file:line＝helper 化対象の確定（`impl_brief_shadow_rigged_mdi_sliceB` 系のコード実読で埋める）。
- **未決②**: 16 MiB 別表 vs SSBO spare 同居（§8 代替）＝v1 は別表確定、spare 同居は最適化として後続裁定。
- **未決③**: submit 直前の最終 sweep（G3）を入れるか＝author フックで足りるかを PoC で観測後に決定。
- **省略**: GPU 側 torn（residency 層）・vertex buffer 内容（G1/G2）は本器スコープ外＝別器。
- **やらないこと**: 既存 `checkDrawDataIDAtFire` の撤去（id 等価層として残置・本器と相補）。

---

## 14. 実装 v1 status + 実装自己監査（2026-08-10・全体設計は不変・改変なし）

### 14.1 実装済（全体設計 §5/§6 に忠実・全 MDI site 一律）
| 層 | 実配線 file:line |
|---|---|
| core | `mdiInit/mdiAuthor/mdiReference/mdiHash` + shadow(`MdiSlot[DRAWDATA_TOTAL_SLOTS]`)= `llvkcontract.cpp` / 5 cause + CAUSE_NAMES = `llvkcontract.h/.cpp` |
| β slot→view shadow | `bindlessWriteSlotInternal`(`llvkloader.cpp`)+ accessor `bindlessSlotView/bindlessFallbackView`・`mdiInit` at heap active |
| helper | `LLRenderPass::mdiAuthorAndCheck`(α author + β 多テクスチャ)/ `mdiSetFirstInstance`(reference)= `lldrawpool.cpp` |
| author | establishPerDrawId 末尾 / opaque・影 replay `pushIndirectBucket` / rigged `pushRiggedBatchesIndirect` |
| reference | appendAlphaRunCmd / opaque・影 replay / rigged / inline `drawRange`+`drawRangeFast`+`drawArrays`(llvertexbuffer・既存 guard 内) |
| β | 全 author site・`mVkUsesHeapSet` guard・`mTextureList` 多テクスチャ対応 |

### 14.2 未実装（**設計 §12③ が「未決」と明記した flush 検証**＝改変でなく設計の staging に忠実）
- **`C_MDI_GEOM`（cmd geometry 照合）/ V2 flush 保険 / `C_MDI_RUNINV`（run 不変量）= 未 fire**。5 cause は enum に定義済（将来配線用）だが **flush sweep は §12③ で「author フックで足りるか PoC 観測後に決定」= 未決**のため v1 では発火させない。**これは私の勝手な縮小でなく、凍結された設計の未決項目に従った結果**。PoC 観測後、施主判断で flush sweep を追加する。
- **要許可事項**: もし「v1 で flush sweep も実装せよ」なら設計 §12③ の未決を確定させる決定＝AYA 裁定。指示があれば実装する。

### 14.3 実装自己監査（correctness / gap / hidden）
- **C1** author は DrawData を書く全経路を被覆（establishPerDrawId=alpha/opaque-simple/gltf/全 buildAndOverride 呼び手、replay=bucket、rigged=rigged）。reference は firstInstance 全代入 + inline fire 全 3 経路。
- **C2** mdiAuthorAndCheck を id 返却毎に呼ぶ（memo/cache hit 含む）→ author_frame 毎フレーム更新 → V1 が cross-frame content-cache で誤発火しない（構造排除）。
- **C3/C4** β は heap guard・多テクスチャ・slot0/fallback 除外（fb_heap_default と非重複）。
- **G-i1** flush 検証未実装 = §12③ 未決（上記 14.2・申告済）。
- **G-i2** terrain MDI（`lldrawpoolterrain.cpp:288` firstInstance=0 固定）は**設計 §5 の site list に無い**ため非配線。追加は設計拡張＝AYA 裁定（勝手に足さない）。**申告**。
- **G-i3** `pushIndirectBucket` の `rec==null` 縁（テンプレ build 時 firstInstance を per-frame 更新しない稀経路）は非被覆。**申告**。
- **H-i1** 全 hook は記録スレッド（main）前提で `sMdiShadow` 無ロック。現直列モードでは安全。MT 再有効化時は要同期（[[project_serial_recording_is_temporary]]）。**申告**。
- **H-i2** author_frame は `sFrame`(llvkcontract・frameBegin で frame 頭に +1)。記録中は不変＝author→reference 同フレーム一貫。
- **H-i3** heap 有効化前 draw は shadow 空で no-op（安全）。author→reference 順序で warmup 誤発火なし（author が先行）。

## 14.4 独立監査 応答（2026-08-10・監査は本設計の外部 Fresh が実施）

独立監査が実コード全読で摘出した綻びを設計者が source 突合で裁定。**全体設計は不変・以下は実装を設計へ忠実化する修正**。

### 忠実化修正（適用済・設計が要求していたのに実装が欠いていた＝矮小化の是正）
- **F-1（β 既定 slot 除外・監査 3-1/2-1）**: β が `slot==0`+`actual!=fallback` のみ除外し、**既定 GL テクスチャ slot（heap 枯渇/未 resident の fb_heap_default 落ち先・非ゼロ）を除外していなかった** → heap 枯渇下で偽陽性。設計 §6.1 の `slot==defaultHeapSlot()` 除外を `LLImageGL::sDefaultGLTexture->getVkHeapSlot()` で実装（`lldrawpool.cpp` mdiAuthorAndCheck）。**§14.3 C3 の「fb_heap_default と非重複」は修正前は虚偽だった＝訂正**。
- **F-2（drawRangeFast 参照・監査 3-2/2-2）**: `replace_all` がインデント差（16 空白）で `drawRangeFast` を取り逃し、reference 未配線＝bindless fast path が無検査。手動で `mdiReference` 追加（`llvertexbuffer.cpp:609`）。**§14.1 の「drawRangeFast 配線済」は修正前は虚偽だった＝訂正・お詫び**。
- **F-3（mdiReference の shader guard・監査 3-3）**: 設計 §6 が要求する `!(mVkUsesHeapSet||mVkUsesSkinSet)→return` を欠いていた → 非 bindless/INHERIT が slot0 集約し偽 STALE/OVERWRITE。guard を実装（`llvkcontract.cpp` mdiReference・`llglslshader.h` include 追加）。
- **F-4（shadow 16 B 化・監査 2-3）**: `MdiSlot` に `const void* ref_src` を持たせ 24 B（≈24 MiB）だった。設計 §4/§8 どおり `ref_src` を **verbose 時のみの副表 `sMdiRefSrc`** に分離、always-on の `MdiSlot` を 16 B に戻した。

### 設計レベル（AYA 裁定待ち・凍結設計ゆえ勝手に改変しない）
- **D-1（監査 3-3B・要判断）**: 予約 slot 0（`draw_id=INVALID→0` の枯渇 fallback）を α author/reference が処理し、複数集約で偽 V1/V2。**推奨=予約 slot 0 を検出器の author/reference から no-op 除外**（slot 0 は codebase 全体で予約＝実 per-draw が住まない invariant を尊重）。ただし設計 §6 の INHERIT→0→check 挙動を変えるため **AYA 承認要**。未承認ゆえ未適用＝crowd/枯渇下で本 cause は偽陽性が残る。
- **D-2（監査 1-1）**: β は単一テクスチャ path で `slot[0]`（diffuse）のみ照合し **normal(`slot[1]`)/spec(`slot[2]`) の heap 実体取り違えを検出しない**。設計 §6.1 擬似コード自体の限界＝設計の穴。§2「あらゆるフィールド」看板との乖離。β を normal/spec へ拡張するか＝**AYA 裁定**。
- **D-3（監査 1-2）**: α V2 は content-dedup（ensureVkDrawDataSlot 内容一意・scratch 同内容 memo）ゆえ実効的にほぼ inert（scratch wrap か slot-0 集約でしか発火せず）。V2 の被覆は過大評価。設計の efficacy 観測＝要 AYA 認識。

### 監査が正トレースで「欠陥なし」と裁定した点（設計者確認・保持）
frame カウンタ整合 / rigged refreshed 分岐 / 同 tex 再upload torn view / 無ロック read = いずれも安全（監査 §「無し裁定」に同意）。

## 13. 段取り（施主合意事項）

1. 本 doc 確定 → **自己監査（本 §11 済）**。
2. **PoC**: particle 1 経路で当初バグ発火実証（機構の正しさを最小確定）。
3. 全 MDI site へ水平展開（§10）。
4. **実装後 = 設計＋実装を独立監査**（別 Fresh・施主指示）。
5. その後、**当初の particle 色混入バグ本体を本器で spot**（検出器が名指した slot/field/culprit から根へ）。
