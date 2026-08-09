# VK Texture Residency Model — bindless slot 所有と状態機械（設計 doc・草案 v0）

> 目的 = texture の VK backing（image/view/alloc）と bindless heap slot の寿命を **明示的な状態を持つ 1 個の所有者**に集約し、現状の「各サイトが `destroy → mVkImageView=NULL → updateVkHeapSlot → create → updateVkHeapSlot` を手続きで書き写す」if 散在を撤廃する。副産物として **パーティクル白フラッシュ**（再upload 過渡に slot descriptor が 1×1 白 fallback へ張り替わる）が構造的に発生不能になる。

---

## 0. 現状の病理（なぜ if が増え続け破綻するか）

`LLImageGL` は VK backing の**状態変数を持たない**。状態を毎回ハンドルの null 組合せから推論する:

- `mVkImage == VK_NULL_HANDLE` / `mVkImageView == VK_NULL_HANDLE` / `hasVkImage()` / `mVkHeapSlot == INVALID`

これは状態ではなく**状態の残骸**で、各サイトが自前解釈する。結果 `updateVkHeapSlot()` の呼び出しが **11 サイト**に散り、うち 6 サイトが同一の手続き（destroy→null→update）を複製している。

| # | file:line | 関数 | 現在の行為 | 分類 |
|---|---|---|---|---|
| 1 | llimagegl.cpp:1019 | `setImage`（D6 追加） | upload 失敗 → destroy→null→update | **POISON** |
| 2 | :1127 | `syncVulkanMip0Image` | 再生成(can_reuse=false) → destroy→null→update | **POISON** |
| 3 | :1213 | `syncVulkanMip0Image` | convert 失敗 → destroy→null→update | **POISON** |
| 4 | :1257 | `syncVulkanMip0Image` | upload 失敗 → destroy→null→update | **POISON** |
| 5 | :1564 | `setSubImage` | サブ更新失敗 → destroy→null→update | **POISON** |
| 6 | :1993 | `destroyGLTexture` | 破棄 → destroy→null→update | **POISON**(本来 release) |
| 7 | :1150 | `syncVulkanMip0Image` | create 成功 → update(正 view) | PUBLISH |
| 8 | :1393 | `setExternalVkBacking` | 外部 backing → update(正 view) | PUBLISH |
| 9 | :2028 | `setAddressMode` | sampler 変更 → update | RE-PUBLISH(view 無で poison) |
| 10 | :2044 | `setFilteringOption` | sampler 変更 → update | RE-PUBLISH(view 無で poison) |
| 11 | :1438 | `vkHeapSlotOrDefault` | **draw 時**に slot 未取得なら lazy acquire | ACQUIRE(描画中副作用) |

**白 poison の構造原因**: `updateVkHeapSlot()` → `bindlessUpdateSlot()` → `bindlessWriteSlotInternal()` は `view==VK_NULL_HANDLE` を **`sDefaultFallbackImageView`（1×1 白・llvkloader.cpp:2495）** に置換する。POISON サイトは mVkImageView を NULL にした直後に呼ぶので、slot descriptor が**白に張り替わる**。`destroyImageVk` は defer 破棄（old view は frames-in-flight 生存）なので、この白張替は**不要かつ有害**。

**排他が無い**: `createGLTexture`/`setImage` は off-main で走り得る（llimagegl.cpp:1793 `main_thread` 分岐）。worker が slot を書く間に GPU は bound bindless descriptor（UPDATE_AFTER_BIND）を読む。torn descriptor を GPU が読むのが症状の物理。

**観測される 2 症状（同根）**:
- **白フラッシュ**（パーティクル等）= `mVkImageView==NULL` 経由の **白 poison**（destroy/upload 失敗時・#1-6・D6 が顕在化）。torn 内容 = 1×1 白。
- **ちらちら**（通常オブジェクト）= 失敗でない**通常の再streaming**（discard 変化）が create 成功で `bindlessUpdateSlot(slot, new_view)` を **in-place 上書き**（llimagegl.cpp:1426・#7 相当）→ 同 slot をサンプル中の in-flight frame と torn read = 内容が一瞬ズレる。
- 両者とも `gDeferredDiffuseProgram`/`gDeferredFullbrightAlphaMaskAlphaProgram` 等 **path A（heap slot・mIndexedTextureChannels=4）** の全 texture が対象。∴ 「白 poison を消す」だけでは**ちらちらは残る** — fresh-slot（in-flight slot を上書きしない）で初めて両方閉じる（実機で通常オブジェクトのちらつきが観測される事実が本判断を支持）。

---

## 1. モデル = 2 つの型

### 1.1 `VkBacking`（値型・完成品のみ）

texture 1 枚の GPU 実体。**valid() な view を持つ完成品としてのみ生成**され、以後 image/view は不変（＝作りかけを外から観測させない）。

```cpp
struct VkBacking
{
    VkImage       image = VK_NULL_HANDLE;
    VkImageView   view  = VK_NULL_HANDLE;
    VmaAllocation alloc = nullptr;
    U32           width = 0, height = 0, mips = 1;
    VkFormat      format = VK_FORMAT_UNDEFINED;
    bool          owned = true;   // false = external backing(setExternalVkBacking)= 退役時 destroy しない(§8-H5)

    bool valid() const { return view != VK_NULL_HANDLE; }
};
```
> retire/commit で旧 backing を退役する際、`owned==false` なら `destroyImageVk` を呼ばない（外部所有 view の誤破棄防止・自己監査 H5 で捕捉した欠陥の修正）。

### 1.2 `VkTexResidency`（状態機械・slot の唯一の所有者）

`LLImageGL` は VK 関連メンバ（mVkImage/View/Alloc/HeapSlot/…）を**この 1 個に置換**する。bindless slot を書く権限はこの class だけが持つ（＝現 `updateVkHeapSlot` の唯一 choke 性を型で固定）。

```cpp
class VkTexResidency
{
public:
    // 新しい完成品を publish。slot を next へ atomic 張替 → 旧 backing を defer 退役。
    // slot は「常に生きた view」を指す。next.valid() 必須。
    void  commit(VkBacking next);        // Empty|Live -> Live

    // 構築失敗。現状維持（旧 backing 保持・slot 不変）。白張替は起きない。
    void  abandon();                     // 状態不変

    // texture 消滅。slot 解放 + backing 退役（defer）。
    void  retire();                      // Live -> Empty

    // sampler だけ変更（address/filter）。Live なら同 view で再 publish・Empty なら no-op。
    void  resample(VkSampler s);         // Live -> Live / Empty -> no-op

    // 描画時解決（副作用なし・const）。Live=slot 番号 / Empty=DEFAULT_SLOT。
    U32   slotForDraw() const;

    bool  isLive() const { return mCur.valid(); }

private:
    U32       mSlot = BINDLESS_INVALID_SLOT;  // heap index。**commit ごとに付け替わる**（後述・spec 制約）
    VkBacking mCur;                            // 現在 publish 中の完成品
    VkSampler mSampler = VK_NULL_HANDLE;
    // 不変: (mSlot 有効) <=> (mCur.valid())。状態は mCur.valid() で一意に決まる。
};
```

> ⚠️ **slot は recreate をまたいで不変にできない（spec 制約）**。bindless heap set は `VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT`（llvkloader.cpp:3155）で作られる ＝ **in-flight コマンドが使用中の slot 要素を更新するのは UB**。∴ 既存 view を持つ slot に新 view を in-place 上書きする（現 `updateVkHeapSlot` line 1424-1428 の挙動）と、その slot をサンプル中の in-flight frame と競合し torn read になる。よって `commit` は **fresh slot を取得**し旧 slot は **defer 解放**（GPU 完了 gate）する。in-flight frame は旧 slot/旧 view を最後まで読み、次 frame は DrawData 再計算で新 slot を拾う。

**状態は 2 つだけ**: `Empty`（backing 無し・slot 無し）/ `Live`（完成品を publish 中・slot 有効）。**「view 無しで slot が live」は型として表現できない** → poison 分岐が消える。

---

## 2. 遷移と実装

### 2.0 `publish(view, sampler, next?)` = 全遷移の内部 primitive

commit/resample はこの 1 つに落ちる。**「新しい生 view を持つ fresh slot を立ててから、旧 slot を退役」**が唯一の書式。

```
前提: view != VK_NULL_HANDLE
1. U32 s_new = bindlessAcquireSlot(view, sampler);   // fresh slot（in-flight 未参照 = UPDATE_UNUSED_WHILE_PENDING 安全）
   if (s_new == INVALID) return false;               // 取得失敗 = publish せず false（呼び手が abandon）
2. U32 s_old = mSlot; mSlot = s_new; mSampler = sampler;
3. if (next != nullptr) { VkBacking prev = mCur; mCur = *next;
                          if (prev.valid()) destroyImageVk(prev...); }   // backing 差替時のみ旧 backing 退役(defer)
4. if (s_old != INVALID) bindlessReleaseSlotDeferred(s_old);            // 旧 slot 退役(defer・GPU 完了 gate)
return true;
```

### 2.1 `commit(next)` = create-then-swap（白窓の除去点）

```
前提: next.valid()
sampler = getSamplerForState(mAddressMode, mFilterOption, mHasMipMaps, false);
if (!publish(next.view, sampler, &next)) { destroyImageVk(next...); /* 呼び手が abandon 相当 */ }
```

要点:
- **slot 番号は commit ごとに付け替わる**（fresh 取得）。in-flight frame は**旧 slot/旧 view を最後まで**読む（上書きしない）→ torn(白)も torn(旧内容)も発生しない。
- DrawData は毎 frame `computeDrawDataSlots`→`vkHeapSlotOrDefault(mTexture)` で texture の**現 slot 番号**を読み直す（`ensureVkDrawDataSlot` の memcmp が差分検出）→ 次 frame の draw が自然に新 slot を拾う。
- 旧 slot / 旧 backing は共に **defer 退役**（`reapReady` = GPU 完了 monotonic gate）。dangling も白も無い。
- コスト: recreate ごとに slot 1 個の acquire/deferred-release + DrawData slot 1 個の再取得。頻度 = discard 変化時のみ（毎 frame でない）＝ 有界（§6 でプール規模を確認）。

### 2.2 `abandon()` = 失敗時

何もしない。旧 `mCur`（last-good）を保持。**現行の「失敗→白張替」を廃止**。作りかけ image は commit されないので `VkBacking` の生成側（後述 builder）が破棄責任を持つ。

### 2.3 `retire()` = 破棄

```
if (mSlot != INVALID) bindlessReleaseSlotDeferred(mSlot); mSlot = INVALID;
if (mCur.valid())     destroyImageVk(mCur.image, mCur.view, mCur.alloc);
mCur = {};
```
slot を「白へ張替」ではなく**解放**する（現 destroyGLTexture #6 の誤り＝白残置を是正）。以後 `slotForDraw()` は Empty → DEFAULT_SLOT。

### 2.4 `resample(s)`

```
mAddressMode/mFilterOption は既に更新済み前提。
if (mCur.valid()) publish(mCur.view, s, nullptr);   // Live のみ・fresh slot に同 view+新 sampler
// Empty は no-op（次 commit で新 sampler が乗る）
```
backing は差し替えないので `next=nullptr`（旧 view は mCur が保持し続け破棄しない）。#9/#10 の「view 無しで sampler 更新 → poison」も、Empty で no-op になるので構造的に消える。sampler 変更も in-place 上書きを避け fresh slot 経由（in-flight との torn 回避）。

### 2.5 build（作りかけの隔離）

作りかけ image/view は `VkTexResidency` の外の**ローカル builder**で組む。`createTextureImageVk`/`uploadImageData*Vk` は temp ハンドルに書き、成功したら `VkBacking` に束ねて `commit`、失敗したら temp を破棄して `abandon`。＝ **半壊 backing は residency に一切入らない**。

---

## 3. 排他（concurrency / 何が torn を防ぐか）

| 境界 | 防御 |
|---|---|
| worker が commit 中に GPU が slot を読む | commit は **生 view → 生 view** の単一 `vkUpdateDescriptorSets`。GPU はどの瞬間も旧 or 新の**完全な生 view**を読む（白/NULL を publish しない）。 |
| slot descriptor 書込みの競合 | 既存 `sBindlessSlotMutex` で直列化（bindlessAcquire/Update 内）。本モデルはそれを前提に、**publish 内容を常に生 view に限定**することで意味的 torn も消す。 |
| 旧 view の破棄 vs in-flight GPU | `destroyImageVk` の defer（`reapReady` = GPU 完了 monotonic gate）。swap 後に退役するので slot は旧を参照しない。 |
| draw 時の副作用 | `slotForDraw()` は const・取得しない（#11 の draw 中 lazy acquire を廃止）。取得は commit（upload 時）に前倒し。 |

**核心不変条件（1 文）**:
> bindless slot は「生きた完成 view」だけを publish する。旧 backing は新 backing が slot に張り替わった後にのみ退役する。

これが型（`commit` が `valid()` な `VkBacking` しか受けない）と遷移で強制されるので、**「この場合は…この場合は…」の if を足す余地が無い**。

---

## 4. 11 サイトの畳み込み

| 現サイト | 新 API |
|---|---|
| #1 setImage 失敗 / #3 convert 失敗 / #4 upload 失敗 / #5 setSubImage 失敗 | builder が temp 破棄 → `abandon()` |
| #2 syncVulkanMip0Image 再生成 / #7 create 成功 | builder が新 `VkBacking` 構築 → `commit(next)` |
| #8 setExternalVkBacking | `commit(VkBacking{external...})` |
| #6 destroyGLTexture | `retire()` |
| #9 setAddressMode / #10 setFilteringOption | `resample(sampler)` |
| #11 vkHeapSlotOrDefault の draw 時 acquire | 廃止 → `slotForDraw()`（const）。取得は commit 時 |

`destroy→mVkImageView=NULL→updateVkHeapSlot` という文字列がソースから**消える**。

---

## 5. 移行手順（案）

1. `VkBacking` / `VkTexResidency` を新設（llimagegl.h または新 header）。
2. `LLImageGL` の mVk* を `VkTexResidency mVkRes` に置換。`hasVkImage()`＝`mVkRes.isLive()`・`getVkImageView()`＝`mVkRes` 経由。
3. syncVulkanMip0Image / setImage / setSubImage を **builder→commit/abandon** に書換（temp 隔離）。
4. destroyGLTexture / ~LLImageGL → `retire()`。setAddressMode/Filter → `resample()`。
5. `computeDrawDataSlots` / `vkHeapSlotOrDefault` の draw 時解決を `slotForDraw()` へ（副作用除去）。
6. gate = 視覚（白フラッシュ消滅・AYA）+ validation 0 + VKC 沈黙。

---

## 7. 詳細設計（実装可能レベル）

### 7.1 型の配置

- `VkBacking` / `VkTexResidency` を `indra/llrender/llvktexresidency.h`（新規・.cpp 併設）に定義。VK 型依存のみで LL 非依存。
- `LLImageGL`（llimagegl.h:243-253）の VK メンバ 8 本（mVkImage/View/Allocation/Width/Height/MipLevels/Format/HeapSlot+HeapSlotView/Sampler）を **`VkTexResidency mVkRes` 1 本**に置換。
- accessor 委譲: `getVkImage()`→`mVkRes.image()`・`getVkImageView()`→`mVkRes.view()`・`hasVkImage()`/`getHasGLTexture()`→`mVkRes.isLive()`・`getVkImageFormat()`/`getVkImageMipLevels()`→`mVkRes.format()/mips()`・`getVkHeapSlot()`→`mVkRes.slotForDraw()`。
  - `width/height/format/mips` は `mCur`（VkBacking）が保持。upload 中の寸法比較（syncVulkanMip0Image の can_reuse 判定）は `mVkRes.cur().format==...` 等で読む。

### 7.2 `VkTexResidency` メンバ関数（§2 の primitive を確定）

| 関数 | 実装 | 呼び手 |
|---|---|---|
| `bool commit(VkBacking&& next, VkSampler)` | §2.0 publish(next.view, sampler, &next)。false = acquire 失敗 → 呼び手が next を destroy | mip0 upload 成功後 / external |
| `void resample(VkSampler)` | isLive() 時 publish(mCur.view, s, nullptr) | setAddressMode/Filter |
| `void retire()` | slot 有れば releaseSlotDeferred・backing 有れば destroyImageVk・{}化 | destroyGLTexture / dtor |
| `U32 slotForDraw() const` | isLive()? mSlot : DEFAULT_SLOT。**副作用なし** | vkHeapSlotOrDefault / computeDrawDataSlots |
| `VkImageView view() const` | mCur.view | getLiveVkImageView 等 |
| `bool isLive() const` | mCur.valid() | hasVkImage |
| `VkImage image() const`, `format()/mips()/w()/h()` | mCur の各値 | upload/mip 判定 |

**thread 契約（H1 解決）**: cross-thread 読取は「hot = `slot()`」「稀 = `view()`/`image()`/`format()`/`mips()`（bump/terrain mipgen が main 描画時に読む・llterrainpaintmap/lldrawpoolbump）」。
- `mSlot` = `std::atomic<U32>`。`mView` = `std::atomic<VkImageView>`（isLive/view の ready フラグ兼用）。
- **publish 順序規律**: commit は detail フィールド（image/alloc/w/h/mips/format/owned）を先に書き、**最後に `mView`→`mSlot` を store**（＝ ready の公開）。reader は必ず `isLive()`（=`mView!=NULL`）を先に見てから detail を読む契約 → torn 構造体読みを回避（RCU 不要・現行の単値 latent race と同等以上）。
- 書き手（commit/retire/resample）は当該 texture の upload を回すスレッド（LLVKLoader の per-object 直列前提を踏襲）。同一 texture の commit が複数スレッド同時に走らない前提は現行と同じ（createGLTexture の checkActiveThread/mActiveThread）。

### 7.3 各サイトの書換（11 → 5 API）

**A. `syncVulkanMip0Image`（#2/#3/#4/#7 = 白 poison 3 + publish 1 を一掃）**

mip0 経路を **build-into-temp → upload → commit** に再構成:
```
if (mip_level == 0) {
    if (can_reuse(mVkRes)) { /* 既存 image に再 upload = mip>0 と同経路 */ }
    else {
        VkImage img; VkImageView view; VmaAllocation al;
        if (!createTextureImageVk(w,h,fmt,img,view,al,mips))  { recordVkSupplyFail; return false; }   // temp
        if (!uploadImageDataVk(img, w,h, data, size, 0))      { destroyImageVk(img,view,al); return false; }  // temp・失敗=abandon
        mVkRes.commit(VkBacking{img,view,al,w,h,mips,fmt}, samplerFor());   // 内容確定後に publish
    }
} else {  // mip>0 = 既存 backing への in-place mip upload
    if (!mVkRes.isLive() || mVkRes.format()!=fmt || mip_level>=mVkRes.mips()) { recordVkSupplyFail("mip_no_base"); return false; }
    if (!uploadImageDataVk(mVkRes.image(), w,h, data, size, mip_level)) { recordVkSupplyFail("upload"); return false; }  // 失敗=現状維持(destroy しない)
}
```
→ `destroy→mVkImageView=NULL→updateVkHeapSlot` が **1 つも残らない**。失敗は全て「temp 破棄」または「no-op（現状維持）」。

**B. `setImage`（#1 = D6 の白 poison）**
`vk_ok` 失敗時の `destroyImageVk+updateVkHeapSlot` ブロックを削除。syncVulkanMip0Image が既に temp 隔離するので、setImage は成否を伝播するだけ（`mGLTextureCreated = vk_ok; return vk_ok;` は残す＝D6 の fail-closed 意図は保持）。

**C. `setSubImage`（#5）** 失敗時の destroy+poison ブロック削除 → `return false`（現状 backing 維持）。

**D. `setExternalVkBacking`（#8）** → `mVkRes.commit(VkBacking{external...}, samplerFor())`。

**E. `destroyGLTexture`（#6）/ `~LLImageGL`** → `mVkRes.retire()`（白張替でなく slot 解放）。

**F. `setAddressMode`/`setFilteringOption`（#9/#10）** → mode 更新後 `mVkRes.resample(samplerFor())`。

**G. `vkHeapSlotOrDefault`（#11 draw 時 lazy acquire）** → `gl->mVkRes.slotForDraw()`（const・取得しない）。取得は commit 時に前倒し済。

### 7.4 スレッド所有

- `commit`/`resample`/`retire`（書き手）= texture upload を回すスレッド（main or texture worker・createGLTexture の `main_thread` 分岐）。slot descriptor 書込みは `bindlessAcquire/ReleaseSlotDeferred` 内の `sBindlessSlotMutex` で直列。
- `slotForDraw`/`view`/`isLive`（読み手）= main（記録）。`mSlot` atomic 読取で old/new いずれか一貫値を得る（両 slot とも生 view = torn 無害）。
- fresh-slot 方式ゆえ、書き手が読み手/GPU の in-flight slot を**上書きしない** = UPDATE_UNUSED_WHILE_PENDING を満たす。

---

## 8. 自己監査（correctness / gap / hidden）

### 8.1 correctness
- **C1 白窓の閉鎖 = 正**: 全 poison サイトが temp 隔離/abandon になり、`bindlessWriteSlotInternal` に NULL が渡る per-texture 経路が消える。fresh-slot ゆえ in-flight slot 不変更 = torn(白/旧)とも構造消滅。**ただし §8.2-G2 の DEFAULT_SLOT 自体が白い問題は本モデルの外に残る**（別記）。
- **C2 slot spec 適合 = 正**: fresh slot は free-list/末尾から取得 = in-flight 未参照 → UPDATE_UNUSED_WHILE_PENDING 合法。旧 slot は reapReady(GPU 完了)後に free 化。
- **C3 dangling 無し = 正**: 旧 backing/旧 slot とも swap 後に defer 退役。commit 失敗（acquire 失敗）時は mCur 不変（last-good 保持）。

### 8.2 gap（設計が触れていない/要追対応）
- **G1 publish-before-upload の是正は mip0 のみ**: 現状 mip0 は「create→updateVkHeapSlot(publish)→**その後** upload」で空 image を publish する窓があった（line 1150 vs 1244）。本設計は upload 後 commit で閉じる。**mip>0 は依然 in-place upload** = 既存 backing の image を GPU 読取中に書く content hazard（RAW/WAR）。これは slot モデルの外＝ uploadImageDataVk 内の barrier 責務。**本設計は変えない**が、mip streaming 中の上位 mip 未定義窓は残る（既存・OPEN）。
- **G2 DEFAULT_SLOT が 1×1 白**: Empty texture の `slotForDraw()` は既定 slot を返し、その実体は今も `sDefaultGLTexture`（既定画像）。**「streaming 未完で backing がまだ無い texture」は依然この既定に落ちる**＝ 純粋な streaming 遅延の白は本モデルでは消えない（消えるのは「resident texture の再upload/失敗の過渡白」）。要 product 判断: 既定を透明/前 discard 保持にするか（別課題）。
- **G3 slot/DrawData churn（解決）**: DrawData persistent = `DRAWDATA_PERSISTENT_SLOTS`≈950k（llvkloader.cpp:323-325）。bindless heap = device 依存の大容量。commit ごとの in-flight 滞留 = recreate 数 × `FRAMES_IN_FLIGHT`(3)。重 streaming でも数百〜千/frame 程度 ≪ 950k = 無視可能。`bindlessAcquireSlot` 枯渇時（`heap exhausted`）は INVALID 返し → commit false → **abandon（旧 backing 保持・crash なし）** で graceful degrade。∴ worst-case でも安全側に倒れる。
- **G4 resample の slot churn**: address/filter 変更ごとに fresh slot。頻度は低い想定だが未実測。過大なら「resample は in-place 許容（sampler 差は torn しても両者正 sampler = 無害）」に緩める分岐余地（設計判断・要 AYA）。

### 8.3 hidden（隠れた前提・見落としやすい所）
- **H1 `mSlot`/`mCur` cross-thread（解決）**: 読み手は全て main 描画記録（getVkImageView/getLiveVkImageView・§9 でトレース済）＋ bump/terrain の mipgen（getVkImage/format/mips）。書き手は worker/main。`mSlot`+`mView` を atomic 化し **publish 順序規律**（§7.2）で torn 構造体読みを回避 = 現行の単値 latent race と同等以上（RCU は未観測の過剰防御＝憲法5 で先送り・観測されたら昇格）。同一 texture の commit 直列は現行 checkActiveThread と同前提。
- **H2 commit 中の可視性順序**: `mSlot=s_new` を publish する前に s_new の descriptor 書込み（vkUpdateDescriptorSets）が完了している必要。§2.0 手順 1（acquire=descriptor 書込み）が手順 2（mSlot 差替）より前 = 満たす。ただし main が `mSlot` を読んで DrawData に載せるのは次 frame なので余裕あり。
- **H3 retire と in-flight**: retire で slot を releaseSlotDeferred するが、その frame に既に記録済みの draw がその slot を参照している可能性。releaseSlotDeferred は reapReady まで free しない＝ in-flight 中は生存 → 安全。ただし retire 後 `slotForDraw`=DEFAULT に変わるので、**同 frame 内で retire→再 draw 記録**が起きると別 slot になる（稀・texture 破棄は通常 frame 境界）。
- **H4 「作りかけの他 mip」**: mip0 commit 後、mip1..N が未 upload の窓。G1 と同根の既存挙動。本モデル対象外。
- **H5 external backing の所有**: setExternalVkBacking(#8) の image/view は外部所有（mExternalTexture）。retire/commit で destroyImageVk してはならない → VkBacking に `owned` フラグ要（外部は defer 退役対象外）。**設計追加が必要（見落とし）**。

### 8.4 監査結論
- 白フラッシュ（resident texture 再upload/失敗の過渡）は本モデルで**構造的に閉じる**（C1/C2/C3）。
- ただし **(a) G2 = streaming 未 backing の既定白**、**(b) H5 = external 所有フラグ**、**(c) G3 プール worst-case** の 3 点は本設計に**追補が要る**。特に H5 は correctness 欠陥（外部 view の誤破棄）＝ 実装前に必須修正。
- G1/H4（mip 未定義窓）・mip>0 content hazard は既存・別課題として切り出し（本モデルは slot/view 寿命に限定）。

---

## 10. 独立監査 findings 突合（設計者裁定）

- **F1(HIGH・採択・修正済)** scaleDown リーク: downscaleImageVk 内部確保画像を setExternalVkBacking(owned=false) に流し永久リーク（回帰）。→ scaleDown を `commitVkBacking(VkBacking{owned=true})` に切替(llimagegl.cpp scaleDown)。setExternalVkBacking は真の外部呼び手(llrendertarget/llcubemaparray)専用に純化。
- **F2(MED・採択・修正済)** data==nullptr 経路が commit 戻り値無視 → heap 枯渇時 leak+偽成功。→ `if (created_new && !commitVkBacking) { destroyImageVk; return false; }`。
- **F3(部分採択・撤去被覆の申告)** draw 時 lazy-acquire 撤去 = 「commit 前に bindless 有効化 → 未 re-commit の 2D texture」等の復旧網喪失。**draw=read-only 原則を優先し再導入しない**が、これは撤去被覆損失として申告する。実到達性 = 実運用では sBindlessActive が texture streaming より前に true 化するため窓は実質空。heap recreate 後の stale slot は旧 lazy(INVALID 時のみ発火)でも復旧しない=既存 gap(本改修由来でない)。**§7.3-G の「取得は commit 時に前倒し済」は復旧経路喪失を申告せず過大主張だった → 本節で是正**。恒久復旧網の要否は AYA 裁定。
- **N1(framing 反証・doc 明確化)** 「mCur 多フィールド torn = 旧より劣化」は refute: 旧 readback も mVkImage/Format/Width/Height の別メンバ 4 個を非アトミック読み(readBackRaw 等)= 同じ cross-field torn 曝露。∴ mCur 構造体化は**等価**(§8.3-H1「同等以上」は hot 単値 slot/view で優位・多フィールド管理読みで等価 = 主張維持)。多フィールド管理読み(image+format+mips)は旧新とも per-texture 直列(checkActiveThread)前提に依存する点を明記。

## 9. closure 検証（「これだけで閉じるか」の正トレース）

path A draw の GPU 読取は 3 段: **(i) firstInstance → (ii) DrawData[番号].slot0 → (iii) gTextureHeap[slot0]**。各段の torn 源を列挙し、本モデル外が安全か確認する（assert でなく file:line）。

| 段 | 経路 | 現状の安全性 | 判定 |
|---|---|---|---|
| (i) firstInstance | opaque=`pushIndirectBucket` replay | 毎frame `computeDrawDataSlots`+`ensureVkDrawDataSlot`+`firstInstance=mVkDrawDataSlot`(lldrawpool.cpp:1431-1454・54e8559) | 安全 |
| (i) | alpha/particle | 毎frame `buildAndOverride`→`dc.firstInstance=id`(lldrawpoolalpha.cpp:1546/1585) | 安全 |
| (i) | rigged 主視界=`pushRiggedBatchesIndirect` | 毎frame `s_items`再構築+draw_id fresh(lldrawpool.cpp:1538/1585-1592) | 安全 |
| (ii) DrawData content | `ensureVkDrawDataSlot` | new-slot + `drawDataReleaseSlotDeferred`(llspatialpartition.cpp:4182)=**in-place 上書き無し** | 安全 |
| (ii) | scratch | per-frame region rotation(sFrameIndex)・reap=`reapReady` GPU-gate | 安全 |
| (iii) **heap slot descriptor** | `updateVkHeapSlot` | **in-place 上書き(llimagegl.cpp:1426)+NULL→白poison** | ✗ **バグ＝本モデル対象** |
| (iii) reap 白書込み | slot 解放 | `reapReady` GPU-gate(llvkloader.cpp:9996) | 安全 |

**結論**: (i)(ii) は既に「毎frame refresh / new-slot-defer-old」で安全。**唯一 in-place 上書きが残るのが (iii)** で、discard 変化（LOD 切替）ごとに全 path-A texture で発火＝高頻度＝「たまにちらちら」と一致。本モデルは (iii) を (i)(ii) と同じ new-slot 規律に揃える → **stated 症状（path A 主視界の色ちらつき/白フラッシュ）は閉じる**（列挙証明）。

### 9.1 本モデルで閉じない隣接経路（別課題・要 AYA スコープ判断）
- **(iv) in-place content 書込み**: mip>0 追い upload（`uploadImageDataVk(mVkRes.image(),…,mip_level)`）/ `setSubImage` 部分更新 = view/slot でなく **image 内容**を GPU 読取中に書く（RAW hazard）。full re-stream は本モデルの create-then-swap で消えるが、部分更新（media/動的 texture）は残る。要 barrier or content double-buffer（別設計）。
- **path B（cube/3D/RT）**: `getLiveVkImageView` の live deref（反射/影/depth）。heap slot 非経由の別機構＝本モデル対象外。同型の「作りかけ隔離」を別途要検討。
- **rigged shadow MDI（slice B）**: 影 pass の baked template stale（54e8559 が「同病理」と明記・memory `impl_brief_shadow_rigged_mdi_sliceB`）。影のちらつき（主視界の色でない）＝別工程。

∴ **「これだけで閉じる」= path A 主視界の色ちらつき/白は閉じる。ただし (iv)/path B/rigged-shadow は別課題**として残る（それらの症状が観測されるなら別途）。

---

## 6. 申告欄（未決・要 AYA/追トレース）

- **縮小/解釈**: 本 doc は 2D texture（bindless heap 対象・`TT_TEXTURE`）に限定。cubemap/3D/RT は heap slot 非対象（mCurrVkHeapSlot=INVALID 経路）＝本モデル対象外だが、同型の「作りかけ隔離」は別途要点検（未トレース＝OPEN）。
- **未決**: `mSampler` の初期値と、sampler が view より先に確定するケース（createShader 時？）の順序＝ 要 file:line（OPEN）。
- **未決**: off-main commit と main の descriptor set bind タイミングの厳密順序（sBindlessSlotMutex の被覆範囲が vkUpdateDescriptorSets 全域か）＝ llvkloader の bind 経路 file:line 要確認（OPEN）。
- **未検証**: 本モデルで白フラッシュが消えることは「機構上発生不能」の正トレースで主張しており、視覚 gate は未実施（AYA 領分）。
- **やらないこと**: `bindlessWriteSlotInternal` の NULL→白置換自体は触らない（slot 解放時の default 供給など他用途があり層が違う）。本モデルは「per-texture slot に NULL を渡さない」ことで上流封鎖する。
