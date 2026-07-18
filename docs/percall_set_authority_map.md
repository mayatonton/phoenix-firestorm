# ambient per-call descriptor set — 棚卸し + 所有権設計(invalidator 蒸発)

- 目的: 散在する ~20 の `sCurPerCallVkDescriptorSet = VK_NULL_HANDLE`(invalidator poke)を、設計判断で**蒸発**させる。per-symptom の散在無効化は設計失敗の signal(不変条件ファースト)。T3 が scene 側だけ検証キャッシュ化した残り = immediate 側 + scene consume-clear + resource/RT 安全 poke を一掃する。
- 位置づけ: per-draw 所有権改修の §2.1(RecordContext)+ §2.2(二層一本化)の **immediate 側適用**。T3 = scene per-draw の memo/per-shader cache 化。本設計 = immediate per-call authority の検証キャッシュ化 + scene/immediate routing の明示化。
- 判定原則: コードが唯一の真実。file:line は HEAD(`7f78c1dc72` + 未 commit の set_reuse 掃除)実読。
- **本 doc は設計基盤(棚卸し)**。実装は本 doc から分割 brief を切って直列で解消する(飛びつき禁止・E2 の轍を踏まない)。

---

## 0. 破れている不変条件(一文)

> 記録/inline draw が bind する per-call descriptor set(set-1 universal)は、その draw の束縛状態(shader・texunit views/samplers・UBO ring・RT)を**単一 authority が検証して生成**したものでなければならない。ambient の「最後に誰かが NULL したか」に依存してはならない。

現状: `sCurPerCallVkDescriptorSet`(thread_local)は scene と immediate が**共有する跨ぎキャッシュ**。cross-draw の正しさを ~20 箇所の散在 NULL poke(状態変化点で無効化)で維持している。T3 は scene authority に自己検証キャッシュ(memo/per-shader)を与えたが、**immediate authority(`populateAndBindUniversalDescriptorSet`)は検証キャッシュを持たず散在 poke に依存したまま**。

---

## 1. 機構の全体像(高所)

### 1.1 状態(thread_local・LLGLSLShader static)
- `sCurPerCallVkDescriptorSet`(set-1 の handle・共有 handoff + de-facto cross-draw cache)
- `sCurPerCallVkDynamicOffsets[]` / `sCurPerCallVkOffsetsDirty`(dynamic offset・per-draw refresh flag)
- `sCurPerCallVkSetShape`(shape 種別)

### 1.2 二つの authority
| authority | 起動 | 入力 | 検証キャッシュ |
|---|---|---|---|
| **scene** `buildAndOverrideScenePerDrawSet(params, batch)`(lldrawpool.cpp:466〜) | pool が draw 前に明示 call | `LLDrawInfo* params`(texture list 等) | **有**(T3: memo `mVkSetMemo*` / per-shader `mVkBindlessSet1Lanes` / build dedup) |
| **immediate** `populateAndBindUniversalDescriptorSet()`(llglslshader.cpp:3230〜) | reader が `sCurPerCall==NULL` 時に PULL | ambient(`gGL.getTexUnit(i)->mCurrImageGL` + bound shader + UBO ring) | **無**(毎回 build → ensureScenePerDrawDescriptorSet の content dedup のみ) |

### 1.3 reader(全 5 群・同一 pattern)
`llvertexbuffer.cpp:557/684/779`(drawRange 3 変種)・`lldrawpool.cpp:1076`(pushIndirectSpans = MDI)・`lldrawpoolalpha.cpp:947`(alpha run):
```
set = sCurPerCall
if (set && offsetsDirty) { vkRefreshDynamicOffsetsForDraw(); set = sCurPerCall }   // offset のみ per-draw 更新
if (set == NULL)        { populateAndBindUniversalDescriptorSet(); set = sCurPerCall } // immediate 導出(PULL)
if (set == NULL) skip
bind(set)
```
**`sCurPerCall==NULL ⟺ 「scene authority が走っていない=immediate として導出せよ」の信号。**非 NULL ⟺ scene が push 済 or immediate が前に導出した set を再利用。

### 1.4 現状の帰結(mess の本質)
- scene→scene: 次 draw が buildAndOverride を再 call(memo/cache で検証) = poke 不要。
- **scene→immediate**: scene の leftover set を immediate が誤再利用しないよう、scene draw 後に NULL poke(カテゴリ A)。
- **immediate→immediate(state 変化)**: immediate は検証キャッシュが無いので、texunit/sampler/UBO/RT が変わったら NULL poke で無効化しないと stale set を再利用(カテゴリ B/E)。
- **resource 破棄**: cached set が指す view が破棄されたら NULL poke(カテゴリ D)。
- = **immediate authority に検証キャッシュが無いことの代償を、全 state mutator に散在 poke を撒いて払っている**。

---

## 2. poke 完全 inventory(全 site・カテゴリ別)

### A. scene consume-clear(scene draw 後に leftover を消す)
- `lldrawpool.cpp:1717` / `:1772`(buildAndOverride→drawRange→NULL)
- `gltfscenemanager.cpp:832`(drawRangeFast→NULL)
- `lldrawpooltree.cpp:121`(drawRange loop 後→NULL)

### B. immediate cache 無効化(ambient state 変化)
- `llrender.cpp:134`(`vkNotifyShaderChannelBound` = texunit へ channel bind)
- `llrender.cpp:466`(texunit disable/reset = mCurrImageGL/RT/CubeMap クリア)
- `llrender.cpp:516`(`setTextureAddressModeFast` = sampler addressMode)
- `llrender.cpp:531`(`setTextureFilteringOptionFast` = filter)
- `llglslshader.cpp:3020`(per-program UBO ring advance)
- `llvkloader.cpp:6097`(shared Deferred UBO ring update マクロ)

### C. shader bind/unbind(per-call set は per-shader)
- `llglslshader.cpp:2134`(`bind()` = shader bind 時 reset)
- `llglslshader.cpp:3209`(shader unbind/frame reset = sCurBoundShader=null 同時)

### D. resource 破棄/再生成の安全(cached set が指す view の消滅)
- `llrender.cpp:1449`(texunit victim = image 破棄で mCurrImageGL クリア)
- `llrender.cpp:1479`(cubemap victim)
- `llvkloader.cpp:7375`(VkImage free)
- `llvkloader.cpp:7577` / `:8247`(VkImage 生成)
- `llimagegl.cpp:1338`(setExternalVkBacking)
- `llimagegl.cpp:1668`(upload 完了で新 image/view 差替)

### E. frame/RT 境界
- `llvkloader.cpp:4094`(frame 境界・sInFrame guard 後)
- `llrendertarget.cpp:470`(RT bind)
- `llrendertarget.cpp:804`(RT flush)

### F. refresh-path 内部(境界 poke でない=温存)
- `llglslshader.cpp:3095`(cur==null 安全)
- `llglslshader.cpp:3113` / `:3128`(vkRefreshDynamicOffsetsForDraw の UBO 解決失敗 return)

### G. authority write(poke でない=温存)
- `lldrawpool.cpp:543`(fallback scratch id 経路)・`:560`(per-shader cache hit)・`:940`(build 成功)
- `llglslshader.cpp:3462`(populateAndBind の set 確定)

**蒸発対象 = A/B/C/D/E(~18 site)。F/G は温存。**

---

## 3. 目標設計(単一 authority + 検証キャッシュ = poke 蒸発)

原則(§1 の破れた不変条件を機構化):
1. **scene/immediate を reader が明示 routing**(ambient NULL 信号を廃止)。
2. **immediate authority に自己検証キャッシュ**を与える(T3 が scene にやったのと同型)。

### 3.1 scene/immediate routing(カテゴリ A 蒸発)
- 新 flag `sCurPerCallAuthored`(thread_local bool)。scene authority(buildAndOverride 成功時)が `true`、reader が消費して `false`。
- reader:
  ```
  if (sCurPerCallAuthored) { set = sCurPerCall; if (offsetsDirty) refresh; sCurPerCallAuthored = false; }  // scene
  else                     { populateAndBindUniversalDescriptorSet(); set = sCurPerCall; }                 // immediate(検証キャッシュ内蔵)
  bind(set)
  ```
- ⟹ scene の leftover を immediate が再利用する経路が消滅 → **A 全廃**。scene→scene は buildAndOverride 再 call(不変)。

### 3.2 immediate authority の検証キャッシュ(カテゴリ B/D/E 蒸発)
`populateAndBind` を「毎回 build」から「署名一致なら再利用」に(memo/per-shader と同型):
- 署名 = bound shader + 各 binding の resolved live view/sampler(texunit `getLiveVkImageView`/`getLiveVkSampler`)+ per-program/shared UBO ring buffer + RT attachment gen。
- 一致 → cached set 再利用(build/dedup 省略)。不一致 → 再 build。
- texunit の image/addressMode/filter 変化(B)・view 破棄/再生成(D)・RT 変化(E)は**署名不一致で自動捕捉** → **poke 不要**。
- **未決 1**: 常時「毎 immediate draw 再導出(cache 無し)」で足りるか(UI draw 数 × build cost の実測)。足りるなら §3.2 の署名キャッシュは不要で更に単純。**着手前に immediate draw 数と populateAndBind cost を計測して決める**(標的自己実測の原則)。

### 3.3 shader bind/unbind(カテゴリ C)
- C は §3.1 の authored routing に自然吸収: shader bind で authored=false(新 shader は未 authored)→ 次 draw は immediate 導出 or 次の buildAndOverride が push。明示 poke 不要。
- ただし `bind()`(2134)/reset(3209)は sCurBoundShaderPtr 等も触る = **poke 行だけ外し authored=false 化に置換**(要 per-site 精査)。

### 3.4 温存
- F(refresh 内部)= vkRefreshDynamicOffsetsForDraw の失敗 return は正当。
- G(authority write)= buildAndOverride / populateAndBind の set 確定。
- offsetsDirty 機構 = per-draw の dynamic offset 更新(別軸・温存)。

---

## 4. 分割(task list・直列・各 brief 化)

| 段 | 内容 | 触る主 file | 前提 | 検収 |
|---|---|---|---|---|
| **P0** | immediate draw 数 + populateAndBind cost 自己実測(§3.2 未決 1 を潰す) | 計器 | — | 数値で cache 要否確定 |
| **P1** | authored routing 導入(A 蒸発)= flag + 全 5 reader + buildAndOverride | llglslshader / llvertexbuffer / lldrawpool / alpha | P0 | scene 視覚同一・A poke 削除で回帰 0 |
| **P2** | immediate 検証キャッシュ(B/D/E 蒸発)※P0 で要と出た場合 | llglslshader `populateAndBind` | P1 | immediate/UI 視覚同一・texunit/RT/破棄で追従 |
| **P3** | C poke の authored=false 置換 + 残 poke 物理削除 + 消費者カタログ全消化確認 | 横断 | P2 | grep で A-E poke 0・validation 0 |
| **P4** | AYA cold-launch gate(scene + UI + immediate + RT + streaming 全部) | — | P3 | 視覚同一・validation 0・crash 0 |

- 各段: 直前に JIT brief(触る file・データ構造・罠・gate・申告)→ 実装 → 突合 → 次段。context を焼かないよう brief 単位で完結。

## 5. 罠 / 未決 / リスク

- **未決 1(P0 で潰す)**: immediate 検証キャッシュが要るか(常時再導出で足りるか)。UI draw 数次第。
- **リスク(hot path)**: reader は inline 全 draw が通る hot path。authored flag の set/clear タイミングを 1 個間違えると「scene が immediate 導出に落ちる(遅い/誤 texture)」or「immediate が scene set 再利用(誤描画)」。P1 は最小差分 + 突合厳格に。
- **1:1 前提の検証**: scene draw が必ず buildAndOverride を直前に通るか(inline 経路)を P1 前に全 caller で確認(lldrawpool 1402/1713/1768・alpha 1452/1474・gltf 808/821)。MDI 経路(pushIndirectSpans:1076)は run 頭で 1 回 bind = authored の寿命が run をまたぐ扱いを P1 で明示。
- **D の safety との整合**: view 破棄時、cached set が deferred-free 前に再利用されない保証。immediate 検証キャッシュ(署名に live view)なら破棄=署名不一致で捕捉。scene は T3 の L3 検証が捕捉(要確認: RT attachment 変化を cache hit path が見るか = E の scene 分)。
- **並列**: authored flag も per-lane 化が要る(T5 と同型)。P1 で per-lane 前提に(single lane では lane0)。

## 6. 閉鎖条件
- A-E poke(~18 site)= grep で 0。
- immediate/scene の per-call set が単一 authority + 検証で解決(ambient NULL 信号の廃止)。
- 全 draw 種(inline scene / MDI / immediate UI / RT)で視覚同一 + validation 0。
