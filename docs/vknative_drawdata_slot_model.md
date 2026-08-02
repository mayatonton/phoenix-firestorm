# DrawData slot モデル(層2 設計正本)

> 制定 2026-08-02(設計者)。位置づけ = Material D 段階計画の **PP2 層**: per-draw ID(G★ = `docs/vknative_bind_redesign.md` §8)で index される **DrawData slot の内容・寿命・書込規約**を契約化する。層1 = `docs/vknative_ubo_supply_model.md`。行使段 = Material D(slot 予約欄への material params 格納が本契約の証明)。全 file:line は 2026-08-02 HEAD(`d181c6357b8`)接地。

## 0. 一文モデル
**DrawData slot は「draw の自己記述データ」(self-describing per-draw data・direction.md §0)の物理実体である。1 slot = 12 uint(48B)・per-draw ID がそのまま slot index・GPU からは set2 b0 の SSBO として全 shader に一様に見える。内容の意味と書込規約が暗黙だったのを本書が確定する。**

## 1. 物理実体
- **単一 persistent mapped buffer**(llvkloader.cpp:3044 生成・`DRAWDATA_TOTAL_SLOTS × DRAWDATA_SLOT_UINTS(=12) × 4B`・llvkloader.h:156)。slot 0 は生成時ゼロ化(:3050)。
- **領域 2 分**: ①persistent 域 = domain allocator(`drawDataAcquireSlot` :11943-11978 = free-list + slab grow・枯渇 = `C_DRAWDATA_EXHAUSTED` fail-closed・`VkcRaceProbe` = `C_DRAWDATA_RACE`)②scratch 域 = frame ring(`drawDataWriteScratch` :12079-12108 = `DRAWDATA_PERSISTENT_SLOTS + frame_region × DRAWDATA_SCRATCH_PER_FRAME`・巻き戻り = `C_DRAWDATA_SCRATCH_WRAP`・thread-local memo で同値再利用)。
- **GPU 視点**: set2 binding0 std430 readonly `AyaDrawDataBlock { AyaDrawData aya_dd[]; }`・`struct AyaDrawData { uvec4 tex_slots; vec4 spec_color; vec4 misc; }`(materialF.glsl:306-307 ほか)。index = `aya_draw_id`(= `gl_InstanceIndex` ← `firstInstance` ← per-draw ID・§8 鎖)。

## 2. slot layout 登記簿(12 uint の意味・正)
| uint | GLSL view | 内容 | 書き手の値源(LLDrawInfo) |
|---|---|---|---|
| [0..3] | `tex_slots` | bindless texture heap slot(single: diffuse/normal/specular/0・batch: list 先頭 4)| mTexture/mNormalMap/mSpecularMap or mTextureList |
| [4..7] | `spec_color` | **(D で確定)** specular_color rgba | mSpecColor |
| [8] | `misc.x` | **(D)** emissive_brightness | mFullbright ? 1 : 0 |
| [9] | `misc.y` | **(D)** env_intensity | mEnvIntensity |
| [10] | `misc.z` | **(D)** minimum_alpha | mAlphaMaskCutoff |
| [11] | `misc.w` | **(D)** aya_sss_skin_flag | mIsSSSTarget ? 1 : 0 |

D 以前の [4..11] は全 writer 常ゼロ(予約)。**欄の追加・意味変更は本表の改訂 = 設計者+AYA 承認事項**(shader と C++ の暗黙結合点のため)。

改訂 2026-08-02(AYA 承認・**予約のみ・未実装**): 影 alpha 節の MDI 段で slot を 16 uint(64B・uvec4×4)へ拡幅し、**[12] = object_alpha(mObjectAlpha)**・[13..15] = 予約とする。現行実装は 12 uint のまま。実装トリガー = 台帳「影 alpha 節の MDI/bucketize 段」の採択(indirect span 内の per-draw 値は slot 経由のみ可 = object_alpha が前提)。

## 3. 不変条件(契約)
- **INV-1(純関数)**: slot 内容は **LLDrawInfo(+ batch_textures フラグ)の純関数**。shader・pass・pool・呼び手に依存してはならない。material でない draw にも [4..11] は無条件に充填する(読まない shader には無害・byte 安定が目的)。
- **INV-2(単一正準 compute)**: slot 内容を組む関数は**単一**(`computeDrawDataSlots`・Material D Brief §D2-a で実装)。writer 全列挙(現 3 箇所)= ①establishPerDrawId(lldrawpool.cpp:540-568)②rigged-MDI(lldrawpool.cpp:1456-1477)③llvkbucket record template(llvkbucket.cpp:300-336)。**writer 追加 = 正準関数の呼び出しのみ可**(独自組みは INV-3 違反経路)。
- **INV-3(copy-on-write)**: 内容変化 ⇒ **新 slot 取得 + 旧 slot 遅延解放**(`ensureVkDrawDataSlot` llspatialpartition.cpp:4249-4264)= in-flight GPU read と衝突しない。系: **writer 間 byte 不一致 ⇒ 毎フレーム slot churn ⇒ INV-2 で構造的に禁止**(INV-2 の存在理由)。
- **INV-4(寿命)**: persistent slot の所有者 = LLDrawInfo(dtor で遅延解放 llspatialpartition.cpp:4224-4231・変化時解放 :4258)。解放は deferred(:11980-11993 = frame 猶予)。slot 0 と `BINDLESS_INVALID_SLOT` は解放対象外(:11982)。
- **INV-5(scratch)**: scratch slot は当該 frame 限り有効・frame 跨ぎ参照禁止(ring 上書き)。
- **INV-6(既定)**: slot 0 = 全ゼロの universal default(INHERIT/無効時の安全な着地)。
- **INV-7(CPU 書込唯一性)**: mapped への書込は `drawDataAcquireSlot`/`drawDataWriteScratch` 内部のみ(直接 `sDrawDataMapped` に触る新規コード禁止)。

## 3.1 fallback 規約と既知の残差(監査 F2/F4 反映 2026-08-02)
- **textureless 規約(F2)**: `params != nullptr` かつ texture 無しの draw の `tex_slots[0]` = **正準 default slot**(`vkHeapSlotOrDefault(nullptr)`)。旧 establish の「その時 GL に bind されていた texture の slot」(ambient 依存 = 非純関数)は INV-1 是正で廃止。**heap fetch する draw が ambient bind に依存するのは caller 側の契約違反**(texture は DrawInfo が携行する)。gate 視覚チェック項目: textureless-DrawInfo 描画の白化有無を全 pool 一巡。`params == nullptr`(scratch 専用・persistent slot 非関与)のみ gGL fallback を留置。
- **batch_textures の cross-path 残差(F4・既知/低)**: batch_textures は INV-1 の純関数入力の一部であり、同一 DrawInfo を異なる batch 値の writer が書く経路が併存すると(bucket = true 固定 / establish = caller 値)、mTextureList>1 の draw で tex_slots byte が交替し INV-3 churn になり得る。現状 material は inert([4..11] は batch 非依存)・bucket 型と establish batch 値の併存も従来構造のまま。**恒久解候補(次期)= batch 判定の DrawInfo 属性化**(caller フラグを排し純関数入力から除去)。

## 4. 計器(fail-closed)
`C_DRAWDATA_RACE`(domain 所有権プローブ)/ `C_DRAWDATA_EXHAUSTED`(persistent 枯渇)/ `C_DRAWDATA_SCRATCH_WRAP`(ring 溢れ)/ G★ 系 `drawdata_id_mismatch` ほか fire 点オラクル(bind_redesign §7 G★)= id→slot 整合の検証器。

## 5. 実装の畳み込み(段階規律)
本層の実装物(正準 compute 関数 + 3 writer 統一 + [4..11] 充填)は**単独では inert に近い**(読む shader が D 以前に存在しない)ため、儀式的な単独 gate は張らず **Material D の実装 step 1 として同 build に畳む**(D Brief §D8・`feedback_no_gate_ceremony_on_inert_code` 準拠)。設計層としての検収は D の gate(視覚同一 + 機械オラクル)が兼ねる = 「D が層の正しさの証拠」。
