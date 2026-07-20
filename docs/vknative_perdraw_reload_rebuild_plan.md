# per-draw set 検証 & reload lifecycle 作り直し計画(欠陥 1/2/3・設計者 5 代目)

> 位置づけ = **現行の top plan**。AYA 指示(2026-07-21)=「現状の処理は汚く壊れている。消して作り直す。掃除しながら実装。1/2/3 完了まで build 禁止・ソースレベルで正しくきれいに。完了後に起動チェック → エラー全改修 → commit」。
> 白化(未解決 #1)は 1/2/3 完了後に戻る。本計画は白化を直さない(defect 3 の resize-skip 意味論だけ白化フェーズへ明示引き渡し)。
> コードの真実は HEAD。本書の file:line は着手時に再確認する。

## 0. 現行構造(トレース済・file:line)

### per-draw descriptor set = 3 本の並行 resolver(欠陥 1)
1. `LLRenderPass::buildAndOverrideScenePerDrawSet`(lldrawpool.cpp:566-1349・~780 行)= scene draw(LLDrawInfo backed)。内訳:
   - bindless DrawData slot 書込(599-639)
   - memo fast-path(641-679): guard{shader==,shape==,set[f]!=null,texSig==,accBuilt,topoGen==}+`vkValidatePerCallCache`
   - bindless-lane fast-path(681-714): guard{gltf==0,accBuilt,set[f]!=null,topoGen==}+validate — **texSig 無し・viewDestroyGen 無し**
   - build(720-1348): sampler 解決 loop(759-1044)+ UBO writes(1046-1157)+ SIG_AUDIT(1067-1185)+ dynamic UBO(1205)+ ensure(1213)+ memo_fill pin(1232-1268)+ bindless_fill pin(被覆規則 1269-1340)
2. `LLGLSLShader::populateAndBindUniversalDescriptorSet`(llglslshader.cpp:3473-~3990・~500 行)= immediate/UI/font。内訳:
   - immediate fast-path(3541-3566): guard{set!=null,sig==(viewDestroyGen+attachmentSig+perProgUBO 込),topoGen==}+validate
   - build(3568-~3980): **build loop の複製** + immediate fill pin
3. 共有検証 `vkValidatePerCallCache`(llglslshader.cpp:2331)= dead-set 照合 + ring_sig 再計算 + l3 view 再解決比較。

### cache = 3 型(全て set[3]/tok[3]/ringSig[3] per-frame + **共有 1 本の l3Views/l3Enums/l3Count/topoGen**)
- memo: LLDrawInfo(`mVkSetMemo*`・llspatialpartition.h)
- bindless-lane: `VkBindlessSet1LaneState`(llglslshader.h:409)
- immediate: `VkImmediateSet1LaneState`(llglslshader.h:423・追加 `sig[3]`)

### reload lifecycle = 散在(欠陥 3)
- request: `requestSetShaders`(7 site)+`requestGLBufferRebuild`(4 site)+ resize flag(`gWindowResized`/`gResizeScreenTexture`/`gResizeShadowTexture`)
- **leak**: `setGraphicsLevel`(llfeaturemanager.cpp:662)= request 化から漏れた唯一の直呼び `setShaders()`
- drain: doFrame:1756 の手組み cluster
- self-guard: `setShaders()`:856(`isInFrame()` なら再延期)

## 1. 欠陥の一文特定(破れている不変条件)

- **欠陥 1**: 「cache set が有効か」の判定機構が一つでなく、resolver ごとに別実装 → 実装間の隙間にバグが棲む(hang 真因 = bindless だけ 2 gate 欠落)。
- **欠陥 2**: **payload(set[3] per-frame)と証拠(l3* 共有 1 本)の基数・寿命不一致**。1 slot の store が共有証拠を書換え stale slot を合格させる。かつ証拠容量 MAX_PERCALL_L3=16 < 参照 MAX_SAMPLERS=35 → 被覆規則が live unit-backed 参照('R')を pin 拒否 → spot OFF で bld 50x。正しさ穴と性能退行が**同根**。
- **欠陥 3**: reload(横断関心)に単一の quiesce→teardown→rebuild chokepoint が無い。12+ site + 直呼び 1 + 手組み drain + self-guard = 1 site 漏れ = 1 バグ。

## 2. 目標構造

### A. per-draw set = resolver 1 本・cache slot 1 型・validate 1 本

**A1 共有 builder** `vkBuildPerDrawBindings(cur, ctx, bindings&, evidence&)`
sampler 解決 + UBO writes を **1 関数に抽出**(lldrawpool build と immediate build の重複を削除)。`ctx` が 2 caller の差分(scene: params/textureList/batch/gltf UBO / immediate: gGL unit)を吸収。

**A2 証拠を payload と同基数・同寿命に**(欠陥 2 の構造解)
```
static constexpr U32 PDC_MAX_REFS = 16;   // live 再検証可能 ref のみ(overflow => not pinnable)
struct PerDrawEvidence {
    const void* shader;                    // 同 frame 別 shader 誤用を弾く
    U64  reloadEpoch, topoGen, attachmentSig, ringSig;
    U32  shape;
    U8   refCount;
    bool pinnable;
    S16  refSource[PDC_MAX_REFS];          // ≥0: enum / <0: -(unit+2)。fallback は保持しない
    void* refView[PDC_MAX_REFS];           // 捕捉 view handle
};
struct PerDrawCacheLane {                  // bindless XOR immediate = 1 型に統合
    VkDescriptorSet set[3]; void* tok[3]; U64 pinEpoch[3]; PerDrawEvidence ev[3];
};
```
**各 frame slot が自分の証拠 `ev[frame]` を持つ** → 共有証拠・all-slot-clear パッチ・evidence_same/header_same を全削除。1 slot store が他 slot に影響しない = 欠陥 2 の機構消滅。LLDrawInfo(memo)も同 `PerDrawCacheLane` 1 個を持つ。

**A3 validate 1 本** `vkValidatePerDrawSlot(cur, ev) -> bool`
`ev.shader==cur` → reloadEpoch== → topoGen== → attachmentSig== → dead-set(refView,refCount)→ ringSig 再計算== → 各 ref を source から再解決(enum→`vkResolveEnumBoundView` / unit→`getLiveVkImageView`)して捕捉 view と比較 → shape==。memo_hdr_ok 連鎖・bindless else-if 連鎖・immediate if 連鎖・`vkValidatePerCallCache` を**全置換**。
> **texSig と viewDestroyGen は削除**: per-ref 再解決 + dead-set が「参照が変わった/死んだ」をより正確に捕捉するため(グローバル viewDestroyGen は非参照 view の破棄でも全 cache を潰す大鎚 = 不要)。reloadEpoch が reload 事象を、per-ref+dead-set が texture liveness を、それぞれ精密に担う。

**A4 cacheability(欠陥 2 性能解・設計者裁定 = D1)= 全 live 参照を一様に再検証可能化**
現行の 'R' 拒否 + L3 被覆規則は「unit-backed live view は再検証不能」を理由に pin を拒み 50x を生んだ。**裁定**: 証拠に live ref の (source, 捕捉 view) を持ち、validate 時に source を再解決して比較。同一 draw の fast-path 直前は当該 draw の texture が unit に bind 済 = 再解決は当該 view を返す → 変化時のみ miss。dead-set が handle 再利用を被覆。**pinnable の条件** = 全 ref が再解決可能(enum/unit)かつ refCount ≤ PDC_MAX_REFS。**成立しない族(immediate の indexed `enum==-2`・ref overflow)は pinnable=false → 毎 frame build(安全側・AYA 合意)**。これで L/E/C/T/R 分類・被覆規則・'R' 拒否・pin_refuse_* を全廃し、spot OFF の pin を回復しつつ正しさ維持。
> 申告(設計者裁定): unit-backed の一様検証可能化(D1-b)を採用。速度は「コードで安全と確定できる範囲」に留め、pinnable=false 族の高速化は別途設計(AYA 合意)。

### B. reload lifecycle = 単一 owner

**B1** `LLReloadQueue`(新規・小 / static)
```
enum ReloadKind : U32 { RK_Shaders=1, RK_GLBuffers=2, RK_ScreenResize=4, RK_ShadowResize=8 };
static void request(U32 kinds);  // pending mask に OR
static void drain();             // frame 先頭・固定順で消化・mask clear
```
- 全 request site → `request(RK_*)`。drain = doFrame 先頭で 1 回、固定依存順(現 cluster から導出)。
- **leak 閉塞**: setGraphicsLevel:662 → `request(RK_Shaders)`(register C 消化)。
- 削除: `sPendingSetShaders`/`sGLBufferRebuildPending` と手組み cluster → `drain()` 1 本。self-guard(isInFrame 再延期)→ drain は frame 先頭のみ = 不要 → `llassert(!isInFrame())` の不変条件に置換。

**B2** resize-skip 意味論(撤去済 `if(gWindowResized) return;`)= **白化フェーズへ明示引き渡し**。本計画では現行 drain 位置(beginFrame 前)を維持し render-skip を追加変更しない。

## 3. 削除リスト(掃除しながら)
immediate build 複製 / 3 fast-path 連鎖 / 3 fill 三重化 / 共有 l3* フィールド(3 struct)/ 被覆規則・'R' 拒否・L/E/C/T/R 分類・pin_refuse_* / SIG_AUDIT + C_SIG_DIET_MISMATCH + audit_* / AUXFB2 + s_aux_fb_dump2 / 分岐用 VKPERF_SETCZ_* 細分(HIT/BUILD 最小へ)/ reload bool flag + cluster + self-guard。

## 4. フェーズ(直列・P5 まで build 禁止)
- **P1** reload lifecycle(欠陥 3)= 自己完結・最小リスク・leak 閉塞。D1 と独立。
- **P2** 共有 builder + PerDrawEvidence + PerDrawSlot 型(欠陥 2 構造)。
- **P3** 3 caller を builder+validate+pin に集約・重複削除(欠陥 1)。
- **P4** scaffolding + dead code 一掃(削除リスト完遂)。
- **P5** 起動チェック → エラー全改修 → commit。

## 5. gate(命題様式・完了時)
検証命題列挙 + register 未証明項併記 + 実効設定確認欄(SSR/spot preset 実値)。PASS は自称不可。build 健全性 + 起動 + validation(未承認 alarm=0)+ 視覚同一で判定。白化は本計画の射程外(併記)。

## 6. 申告(縮小・省略・解釈)
- 白化・MultiLight skip は射程外(1/2/3 後)。
- D1 は設計者裁定(内部機構)。resize-skip は白化フェーズ引き渡し。
- ubo audit(sig_audit)は診断 scaffold として削除するが、byte 等価の機械証明を失う → P5 で validation 0 + 視覚同一で代替検収。
