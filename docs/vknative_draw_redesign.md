# VK-native 描画 worker 記録 再設計（段階 II）

> **位置づけ**: AYA 方法論（survey §4）の**段階 II = 再設計**。段階 I の全体地図 `docs/vknative_draw_structure_map.md`
> が確定した欠陥（S6/S7/S8-b/S13/S14/S15）と資産（A1-A6）を出発点に、**「規約でなく機構」で worker 記録を安全化する設計**を定める。
> **grandfather しない**（Phase A/B の現実装も本設計に置き換える対象）。実装は**段階 III（大工事一括再実装）**・計測は**段階 IV（最終 gate のみ）**。
> 要件源 = 地図 §6.4（再設計要件）/ §7.4（snapshot spec）/ §8.3-8.4（alpha・on-demand seed）。
> **AYA 決定（2026-07-28）**: scheduling は**安全側**（record 窓中に可視化した geometry の 1 frame 遅延を受容・消失/化けより優先）。

---

## §0 設計の公理（不変条件ファースト）

本設計が確立する**単一の不変条件**（地図 §6.4 の命題を実装可能形に）:

> **公理 F（Frozen-Snapshot）**: すべての worker record job は、**dispatch 前に原子的に確定した不変 snapshot**（= DrawPlan + 凍結された VB/texture + snapshot された uniform + seed）の上でのみ動く。
> snapshot 確定から join まで、**main は snapshot が参照する state を一切変異させない**。そして
> **「snapshot 外の共有 state を record が読んだ／snapshot 対象が変異した」ことを機構（fail-closed）が検出する**。

地図の全欠陥は「公理 F が規約でしか守られていない」ことの現れ。段階 II は公理 F を**機構**で立てる。

---

## §1 中核アーキテクチャ = DrawPlan snapshot + freeze-by-contract（背骨）

### 1.1 二分原則（read-set の性質で手法を分ける）
地図 §7.1-7.3 の read-set を性質で 2 分し、それぞれに最安の凍結手法を当てる:

| read-set 種別（地図 §7.1-7.3） | 凍結手法 | 根拠 |
|-------------------------------|---------|------|
| 小さい可変スカラ・参照（draw range / mModelMatrix / texture 参照 / heap slot / skinInfo 参照 / program 識別） | **DrawPlan に copy**（materialize） | 小さい・型で網羅すれば S7 転写漏れが構造的に不可能 |
| 大きい共有 buffer（VB 内容） | **freeze-by-contract**（copy せず「窓中不変」を frame schedule で保証） | VB は巨大 = copy 非現実的。schedule 制約が最安 |
| uniform（delta MV / GlobalF / per-program UBO） | **A6 arena / ctx snapshot**（既存機構の横展開） | per-program UBO は解決済（A6）。残 S14/S15 も同機構で |
| seed（program→descriptor） | **on-demand thread-safe 生成 + fail-closed**（§2 で詳述） | 静的リストは網羅不能（S6・§8） |

### 1.2 DrawPlan snapshot 構造（スカラ・参照の materialize）
worker が読む per-draw state を**型で括った不変レコード**にし、plan-build phase（main・全 mutation 後）で copy する。
これにより「worker は live な LLDrawInfo を読まず、copy した DrawPlan だけを読む」= S7（転写漏れ）と S13（集合の凍結）が構造的に閉じる。

```
struct DrawPlanItem {            // 1 draw = 1 item（地図 §7.1 の read フィールドを漏れなく）
    VkBuffer      vb;            // freeze-by-ref（内容は §1.3 契約で不変）
    U32 start, end, count, offset;   // draw range（copy）
    glm::mat4     model_matrix;  // copy（mModelMatrix）
    LLPointer<LLTexture> textures[MAX];  // copy = ref 保持で生存（mTexture/mTextureList）
    U32           heap_slots[MAX];       // snapshot 時に解決して固定（texture evict 耐性）
    const LLMatrix4* tex_matrix; // ref（mTextureMatrix）
    LLVOAvatar*   avatar;        // rigged: ref（palette は frame-stamp 凍結）
    const LLMeshSkinInfo* skin;  //   〃
    U32           program_id;    // seed 参照キー（§2）
};
struct DrawPlan {                // 1 worker job = 1 program の連続 span（静的 pool）/ order 付き span（alpha §4）
    U32           program_id;    // 1 job 1 program（seed 生命線・地図 S6）
    span<DrawPlanItem> items;    // A6 型 arena（frame 別・使い捨て）から確保
};
```
- **materialize 元** = 現 `LLVKBucket::forEachSource(type, …)`（地図 §7.2）を plan-build phase で 1 回舐めて copy。以降 worker はこの forEachSource を**呼ばない**（live 集合を触らない）。
- **格納** = A6 の per-frame arena 相当（`sPerDrawUBOArena` の DrawPlan 版・frame 別・毎 frame reset）= 割当が cross-thread 安全・GC 不要。
- **型で網羅** = 地図 §7.1 の read フィールドが全て DrawPlanItem に載る = **転写漏れ（S7）はコンパイル時に露見**（新フィールドを worker が読むなら DrawPlanItem に足さねば読めない）。

### 1.3 freeze-by-contract（VB 内容の凍結 = eager-mutation scheduling）
VB は copy しない。代わりに **record 窓中 main が geometry を一切変異させない**ことを frame schedule で保証する。
現状 S8-b の元凶 = `stateSort`（`pipeline.cpp:4355`）が record 窓中に **lazy `rebuildMesh`**（`:8218`・VB 同期書き）を呼ぶこと。

**frame schedule 改（安全側・AYA 決定・深掘り §1.3.1 で feasibility 確定）**:
1. **mutation phase（main・record 窓の前に全完了・全カメラ分）**: geo apply（drainGeoPublishQueue）→ 全カメラ（world + shadow 4 cascade）の cull → **eager rebuildMesh 一括**（下記 §1.3.1・既存 `mMeshDirtyGroup` 機構流用）→ カメラ毎の stateSort。ここで **VB/draw map の変異が全て完了**。
2. **plan-build phase**: カメラ毎に **stateSort 直後（その cull result が「現在」の間）に DrawPlan を materialize**（§1.3.1 の draw source 制約）。全 DrawPlan 確定後 mutation は起きない。
3. **record 窓（worker）**: worker は DrawPlan のみ読む。**窓中 main は rebuildMesh/geo apply/stateSort を一切呼ばない**（不変条件）。phase 依存（shadow→gbuffer→lighting→alpha）は record/submit 順で保つ。
4. **join**: 全 worker 完了後に窓を閉じる。窓中に生じた dirty は次 frame の mutation phase へ（1 frame 遅延・AYA 受容）。

#### §1.3.1 feasibility verdict（深掘り 2026-07-28・機械的事実）
「全 mutation を record 窓前に完了できるか」を実トレースで確定 = **feasible YES**（3 つの機械的事実に基づく）:
- **F1: draw source = 現在 grab 済み cull result**。`LLVKBucket::forEachSource`（`llvkbucket.h:114`）は非 emit 時 `gPipeline.beginRenderMap(pass)` = stateSort の `grabReferences`（`pipeline.cpp:4241`）が設定する**単一「現 frame cull」**を読む。∴ **plan-build は各 stateSort の直後**（その result が現在の間）に行う必要 = **deferred 不可・interleave 必須**（次の grabReferences で上書きされる）。cull result は shadow=`static result[4]`・world=別 static で分離・同時保持は `sNoDelete`（`:960`→`:1221`）が render まで張るので可。
- **F2: rebuildMesh は既に一括機構あり**。`mMeshDirtyGroup` に「delay した group」を集め `:5160-5163` で batch rebuild。**流用して eager 化**: 現 stateSort 内 lazy rebuild（`:4266`/`:4355`）を撤去し、全可視 dirty group を mMeshDirtyGroup へ集約 → **全カメラ cull 後・plan-build 前に 1 回 batch rebuild**。VB は camera 非依存ゆえ group 毎 1 回で足る（現 per-cascade 呼びは MESH_DIRTY guard で no-op = 冗長を除去）。
- **F3: world stateSort（ph9）を shadow-record（ph6）より前へ hoist 可能**。shadow の CPU cull/stateSort は自己完結（自前 shadow_cam・`result[j]`）で world stateSort に非依存。world stateSort も shadow に非依存（shadow map は GPU 側 depth・CPU sort は無関係）。∴ 両者を mutation phase に集約可。
- **∴ 精密化**: §1.3 は「cascade interleave 廃止」でなく **「全カメラの cull/rebuildMesh/stateSort を単一 up-front mutation phase に集約 → カメラ毎 plan-build（stateSort 直後）→ 全 record」**。cull/stateSort 自体は main 直列ゆえ interleave のままで安全（worker 並走が無い）。**変異と record の時間分離**が本質。
- **段階 III 最大工事の実体**: (a) rebuildMesh の lazy 撤去 + mMeshDirtyGroup 一括化（既存機構流用で軽い）(b) world stateSort の phase 前倒し（frame 構造改変・中）(c) plan-build を stateSort 直後に挿入（新規）。**「frame 全面再構成」ではなく「mutation の前倒し集約 + plan-build 挿入」= 侵襲は想定より局所**。
- **帰結**: 窓中に可視化した geometry・dirty 化した mesh は**次 frame の mutation phase で処理**（1 frame 遅延・AYA 受容済）。消失/化け（S8-b）は構造的に不可能に。
- **isMapped skip の廃止**: 地図 §7 で見た `pushUntexturedBatch:1835` の isMapped skip は「窓中 VB が変わり得る」前提の緩和策。本契約で窓中不変が保証されるので **skip 分岐は不要**（= textured/untextured の非対称も消える = Phase B gbuffer の corruption 根治）。

### 1.4 欠陥 → 機構 対応表（背骨がどの S を閉じるか）
| 地図欠陥 | 背骨の機構 | 閉じ方 |
|---------|-----------|--------|
| **S7**（ctx 転写漏れ） | DrawPlanItem 型網羅（§1.2） | worker が読むフィールドは型に載る = 漏れはコンパイル時露見 |
| **S8-b**（VB read-during-write） | freeze-by-contract + eager rebuildMesh（§1.3） | 窓中 mutation ゼロ = race 消滅・isMapped skip 廃止 |
| **S13**（可視集合の直列依存） | DrawPlan materialize（§1.2） | 集合を copy 凍結 = 直列前提を機構化 |
| **S14/S15**（delta MV / GlobalF） | uniform snapshot（§3・A6 横展開） | ring/ctx 化で単一共有 mutable を窓から排除 |
| **S6**（seed 網羅） | on-demand thread-safe seed + fail-closed（§2） | 静的リスト全廃・unseeded は消さず検出 |
| **公理 F 全般** | fail-closed 検出装置（§5） | 「snapshot 外 read / 対象変異」を機構で捕捉 |

### 1.5 申告欄（背骨）
- **省略**: DrawPlanItem の MAX（テクスチャ配列上限）・arena サイズ等の定数は段階 III 実装時に確定（本設計は構造と不変条件が対象）。
- **解釈**: 「窓中 mutation ゼロ」= geometry/draw 集合の変異を指す。texture GPU upload（T1 worker・S17）は PE FIFO 順で別管理ゆえ窓と独立（heap slot は plan-build 時に解決固定するので窓中の再 upload も DrawPlan には影響しない）。
- **やらない（背骨では）**: seed 機構の内部（§2）・uniform ring 詳細（§3）・alpha 順序保存（§4）・検出装置（§5）は後続節。VB を copy する案（Approach A）は VB 巨大ゆえ棄却済み（§1.1 根拠）。lock で freeze する案（Approach B の lock 版）は窓中 main を止める＝並列の意味を減じるため棄却（schedule 制約を採用）。

---

## §2 on-demand thread-safe seed 機構（S6 根治）

### 2.1 現機構と欠陥（地図 S6）
- record job 中 `populateAndBindUniversalDescriptorSet`（`llglslshader.cpp:3616-3648`）は **thread_local `sRecordSeedMap` を find**（`:3621`）。事前 build（`ensureShadowWorkerSeeds` の手書き 5 program / `ensureCameraWorkerSeeds` の per-pass）にある program だけ hit。
- **miss = `C_RECORD_JOB_PULL`（`:3649`）→ descriptor 未 bind で return → サイレント draw 消失**（警告は 2 冪 throttle）。= Phase B ボディ消失の真犯人。
- 静的リストは **原理的に網羅不能**（地図 §8: alpha の per-GLTF-material program は draw 内容依存で動的）。

### 2.2 鍵 = per-lane descriptor 基盤は既に thread-safe（新規機構は不要・既存を統一するだけ）
段階 I トレースで確定した既存事実:
- **main 経路**（`populateAndBindUniversalDescriptorSet` 非 record・`:3703-3722`）は **per-lane cache `cur->mVkPerDrawLane[lane]`（`set[frame]`・per-lane per-frame）** で on-demand build（hit → 再利用 / miss → 割当+write）。
- 割当元 = **`sPerDrawDescLanes[tRecordLaneIndex]`（`llvkloader.cpp:493/6700`）= lane 毎に独立の `pools`/`cache`/`lru`（struct `:484`）**。
- worker は lane 一意（`tRecordLaneIndex` = worker i → lane i+1・段階 I 確定）→ **`sPerDrawDescLanes[lane]` からの割当・cache 更新は lane 隔離で無競合 = 既に thread-safe**。
- ∴ **record 経路が seed map に短絡する必然性は無い**（seed map は per-lane 基盤が整う前の legacy 短絡）。

### 2.3 設計 = record 経路を main 経路と統一（seed map 全廃・on-demand per-lane build）
- **record job 中も、bind された program の descriptor を `mVkPerDrawLane[lane]` 経由で on-demand build する**（main 経路 `:3703` 以降と同一ロジック）。lane 隔離ゆえ worker 並列で無競合。
- **撤去対象**: `sRecordSeedMap` の find 短絡（`:3616-3648`）/ `ensureShadowWorkerSeeds`（`pipeline.cpp:14404`）/ `ensureCameraWorkerSeeds`（camera 版）/ `vkCaptureSeedDynamicBuffers` / `RecordSeed`・`record_seed_map_t` / ctx の `seeds` 項目。
- **結果**: record 中に**どの program が bind されても、その lane で descriptor が生成される**（事前列挙ゼロ）→ **S6 根治**（unseeded という状態が存在しなくなる）。dynamic pool（alpha/GLTF material）も bind 時に自動生成 = **§8 の動的 pool 網羅問題が消える**。
- **per-program UBO / dynamic offset** = A6 arena（`allocPerDrawUBOSlice`・cross-thread 安全）で解決済み（地図 深掘り 3）。seed 撤去後も worker はこの arena 経路で offset を得る。

### 2.4 fail-closed（build 失敗をサイレント消失にしない・憲法 2）
on-demand build は per-lane pool が伸びる（`pools` は vector・成長可）ため通常成功。真の失敗（OOM/device-lost/pool 成長失敗）時:
- **サイレント return 禁止**（現 `:3649` の悪弊を継承しない）。
- **処置 = ①VKC 非 throttle alarm（fail-closed signal・憲法 2 の default-deny に載せる）+ ②当該 draw を「未記録」として記録し、join 後に main で inline 再記録（fallback）**。→ **消失でなく劣化縮退 + 検出**。frame は「build 失敗 N 件」を持ち、gate は非ゼロを PASS 不可とする。
- ※ inline fallback の副作用（main で再記録 = その draw だけ直列）は稀失敗時のみ・正しさ優先（AYA 安全側方針と整合）。

### 2.5 「1 program 1 job」制約の解除（§4 alpha の前提）
- 現設計の生命線「1 job = 1 program 排他」は **seed 事前 build の都合**（地図 S6）だった。on-demand seed では **1 job が複数 program を bind してよい**（各 bind で lane 上に生成）。
- ∴ **job 粒度が program 制約から自由になる** = alpha の「per-draw で shader 切替える job」が成立可能に（§4 の前提）。crowd への道が seed 面で開く。

### 2.6 seed（= per-lane descriptor cache）の生存期間
- `mVkPerDrawLane[lane].set[frame]` は **per-lane per-frame**。既存の cache/LRU/deferred_free 機構（`PerDrawDescLane`）をそのまま使用。frame 別ゆえ cross-frame 上書きなし（A6 と同じ frame 隔離思想）。
- DrawPlan（§1.2）の `program_id` は、この on-demand build のキー（`ScenePerDrawCacheKey`）に対応させる。

### 2.7 欠陥対応・申告・段階 III 検証点
- **閉じる欠陥**: S6（seed 網羅）= 完全根治（on-demand で unseeded 消滅）。§8 動的 pool 問題も解消。
- **段階 III 検証点（深掘り 2026-07-28 で完了・下記に確定所見）**:

#### §2.7.1 descriptor write 経路の lane 隔離 verdict（深掘り・機械的事実）
build 経路（`populateAndBindUniversalDescriptorSet:3724+`）の共有性を実トレースし確定:
- **✅ 安全（lane 隔離 or thread_local or read-only）**: per-lane pool 割当（`sPerDrawDescLanes[lane]`）/ cache・LRU・deferred_free（`PerDrawDescLane` per-lane）/ shader layout metadata（`mVkLayoutBindings`/`mVkBindingToChannel`/`mTexture` = link 時確定 read-only）/ `gGL` texunit・`live_view`（`thread_local LLRender`）/ **`drawDataWriteScratch`+`tCurrentDrawDataID`（thread_local `:387/12424` = Phase A drawdata_race fix 済）** / `sBindlessHeapPool`（read-only universal set 参照）。
- **❌ 唯一の shared-mutable ハザード = `cur->mVkEnumBoundView`（shader インスタンスの共有 `std::vector`・非 thread_local・`llglslshader.h:360`）**。`shader->bindTexture()`（`:2481/2524` + body）が `gGL bindFast`（thread_local）の**後に `vkCaptureEnumBoundView`（`:2215`）で `mVkEnumBoundView[enum]` を書く**（gate なし）。alpha 等 per-draw で `shader->bindTexture`（`lldrawpoolalpha.cpp:640/645`）を呼ぶ pool では、**同一 shader を使う 2 worker が per-draw で共有 write = データ競合**。
- **なぜ現設計は無事か** = 現 record 経路は seed の descriptor set を**再利用**し build 経路（`:3724+`）を**通らない**（seed は main 単一スレッドで pre-build）。∴ 現 worker は `mVkEnumBoundView` を書かない。**私の §2 on-demand は build を worker で走らせる = この race を新たに導入する**。§2.7.2 で対処必須。

#### §2.7.2 §2 設計の補正（on-demand build を mVkEnumBoundView から切り離す）
- **原則**: `mVkEnumBoundView` は L3 最適化（bound view の memo で redundant 解決を省く）。build 経路には **memo 不使用の直接解決 fallback（`channel>=0` → `live_view((U32)channel)` `:3824` = thread_local gGL から直接）** が既にあり、**得られる `view` は memo 経由と同一**（どちらも現 bound texture を指す）。
- **設計（推奨）= worker build 経路は L3 memo を bypass**: record job 中は `vkCaptureEnumBoundView`（write）を**呼ばず**、view は常に直接解決（`live_view`）する。共有 shader write ゼロ = lane 隔離完成。`vkWarnL3Fallback` の noise は record job で抑制（fallback が worker の正常経路）。
- **代替**: `mVkEnumBoundView` を per-lane 化（`[MAX_RECORD_LANES]`）。memory 小増だが shader struct 改変が広い。**bypass を推奨**（build 経路局所・struct 不変）。
- **効果**: この 1 点補正で **§2 の「per-lane build は thread-safe」が真に成立**（pool 隔離 + view 直接解決 + drawData thread_local + layout read-only）→ S6 on-demand seed が worker 並列で安全に成立。
- **段階 III audit 項目**: 他 pool で `shader->bindTexture` per-draw 呼びの全数（bump/gltf 等）を列挙し、bypass 適用の網羅を確認（材料 materials は per-draw texture を `gGL bindFast` 直で `mVkEnumBoundView` を書かない可能性が高いが要確認）。
- **申告（省略/解釈）**: on-demand build の**初回コスト**（seed 事前 build が消える代わり record 中に build）は「1 program 初回のみ・以降 cache hit」ゆえ定常負荷増は軽微の見込み（段階 IV で実測・設計判断には使わない=憲法 6）。VKC alarm の具体シグナル名は §5 検出装置で定義。
- **やらない（本節）**: 検出装置本体（§5）・alpha 順序保存（§4）は後続。

## §3 uniform snapshot（S14/S15・A6 横展開）

### 3.1 対象と性質
地図で「未隔離な cross-cutting frame state は 3 つのみ = S14 `gGLDeltaModelView` / S15 GlobalF UBO / per-program UBO（A6 で解決済）」（§7.3）。残る S14/S15 を閉じる。両者とも **frame/phase 定数**（per-draw 変化なし・window 内不変）である点が鍵。

### 3.2 S14 = `gGLDeltaModelView`/`gGLInverseDeltaModelView` → thread_local + ctx snapshot
- 現状 = plain global（非 thread_local・`llrender.h:483-484`）で sibling `gGLModelView`（thread_local `:478`）と非対称。worker が暗黙に共有 read。
- **設計 = sibling と対称化**: `thread_local` 化 + worker の ctx 転写に含める（`gGLModelView` を ctx memcpy するのと同じ経路・§1.2 DrawPlan header or record ctx）。frame 定数ゆえ per-draw 不要。
- **効果** = A1（描画現在状態の網羅 thread_local 化）の穴を 1 個塞ぐ = S14 が A1 に吸収され機構化。最小の改修。

### 3.3 S15 = GlobalF UBO → per-lane/frame arena slice（A6 横展開）
- 現状 = 単一共有 UBO を phase 毎に main が上書き（`writeCurrentGlobalFUBO` `:6101`/`:12662`）。worker の draw が descriptor 経由で read。
- **設計 = plan-build 時に GlobalF 値を snapshot し、worker window で A6 arena（`allocPerDrawUBOSlice` 相当）に per-lane/frame slice として書き、worker の descriptor はその slice を参照**。
  - GlobalF は per-worker-window 定数ゆえ、window 開始時に lane 毎 1 slice 確保→値 write で足る（per-draw 確保不要）。
  - A6 は frame 別・atomic・cross-thread 安全（深掘り 3）= 単一共有 mutable が worker window から排除される = S15 機構化。
- **代替（§1.3 との相互作用）**: §1.3 の freeze schedule では phase が直列化され worker window 中 GlobalF 上書きは起きない。ゆえ「単一共有のまま」でも window 内は安全になる。**しかし「規約でなく機構」原則（公理 F）に照らし、arena slice snapshot を採る**（window 延伸/将来の phase 並列化でも壊れない）。§5.2 window guard は writeCurrentGlobalFUBO も mutator として監視対象にでき、二重の網。

### 3.4 深掘り 3 の残懸念（非 rotated 単一 UBO post program）の扱い
- 深掘り 3 = post/lighting の非 rotated 単一 buffer program（sun/luminance 等）は **serial main のみ・worker 非対象**（地図確定）。∴ **本再設計（worker 記録の安全化）の対象外**。
- ただし cross-frame overlap の潜在（値がほぼ frame 安定で benign・`C_PP_FALLBACK_LOSSY` alarm ガード付き）は**別件の serial-path 潜在バグ**として残す。§6 の段階 III で「余裕があれば A6 化 or rotate 追加」で潰せるが**本線ではない**（申告）。

### 3.5 欠陥対応・申告
- **閉じる**: S14（thread_local 対称化）・S15（A6 arena slice snapshot）= cross-cutting frame state の未隔離ゼロに。
- **申告**: S15 は §1.3 schedule だけでも window 内安全になるが、機構化のため arena slice を採用（設計判断・二重防御）。非 rotated post program の serial-path 潜在は本線外（§3.4）。
- **やらない**: 実装（thread_local 宣言変更 = フルビルド・ctx 項目追加）は段階 III。

## §4 alpha 順序保存 worker 化（§8.3・crowd blocker）

### 4.1 alpha の 2 障害（地図 §8.3）と本節の対象
1. **per-draw shader 切替 + per-GLTF-material 動的 bind** → 静的 seed 不能。**§2 の on-demand seed で解決済み**（bind された program はその lane で生成・「1 program 1 job」制約も §2.5 で解除）。
2. **depth sort 順序制約**（back-to-front の描画順が blend の正しさの要件）→ 並列化が順序を壊す。**本節の対象**。

### 4.2 核心洞察 = 「順序が要るのは GPU 実行順・CPU 記録は並列化できる」
- alpha blend は**本質的に直列**（framebuffer への読み書きが draw 順依存）= GPU 実行順は back-to-front を保たねばならない。
- しかし**律速は「single main thread の CPU 記録」**（本プロジェクトの前提 = serialization-bound）。**CPU の per-draw 記録を並列化**すれば、GPU blend が直列のままでも勝てる。
- ∴ 設計目標 = **記録は並列・GPU 実行は back-to-front 順**。

### 4.3 設計 = ordered span + secondary CB + vkCmdExecuteCommands 順序実行
- alpha 可視 draw（sort 済み back-to-front）を **順序連続 span** に分割（span 0 = 最遠…span k = 最近）。各 span = §1 の DrawPlan に **order index** を付けたもの。
- **各 span を worker が secondary command buffer に並列記録**（alpha render pass を inherit）。span 内は back-to-front（sort の slice）で記録。
- **main CB が alpha phase で**: `beginRendering(alpha target)` → `vkCmdExecuteCommands(main, N, secondaries[span 順])` → `endRendering`。
- **順序保証** = 同一 render pass instance 内で secondary は **配列順に実行**され、同一 attachment への draw は **API 順で blend**（Vulkan 保証）。∴ **span 順 = back-to-front が GPU 実行順で保たれる**。**手動 barrier 不要**（render pass が順序を保証）。
- **per-draw shader 切替** = span 内で §2 の on-demand seed が bind 毎に descriptor を lane 生成 → 動的 material も安全。

### 4.4 なぜ secondary CB か（pre_cmds 不可・secondary が唯一適合）
- alpha は **mid-frame（PostDeferred・lighting の後、scene color へ blend）** かつ **順序依存**。
- 現 worker モデルの `sPendingPreFrameCmds`（primary CB を frame CB の**前**に submit）は **alpha に使えない**（alpha は gbuffer+lighting の後に実行される必要がある）。primary CB を alpha phase 位置に差し込むには main CB を分割 submit（semaphore 連結）= 煩雑。
- **secondary CB なら main CB は 1 submission のまま**、alpha phase で `vkCmdExecuteCommands` で順序実行するだけ。= **mid-frame 順序付き並列記録の唯一きれいな解**。gbuffer（順序非依存・opaque depth-test）が pre_cmds primary で足りたのと対照的に、**alpha は secondary が本質的に必要**。

### 4.5 freeze 契約との整合（§1.3）
- alpha の DrawPlan も **mutation phase で materialize**（全 mutation 後）。alpha window（記録）中 main は変異しない。
- frame schedule: mutation phase（全 draw の plan-build 含む）→ gbuffer worker window → join → lighting（serial main）→ **alpha worker window（secondary 記録）→ join** → main CB が execute。
- alpha は depth を **条件付き書き込み**（`write_depth`・`lldrawpoolalpha.cpp:389/448`）だが、span 順序実行ゆえ depth 書きも順序化 = 追加ハザードなし。scene color は read-modify-write（blend）だが同 render pass 内 API 順で保証。

### 4.6 span 数の設計
- span 数 = CPU 記録並列度と secondary/execute オーバーヘッドのトレード。**推奨 = worker lane 数に一致**（各 lane 1 span で均等並列）。draw 数が少ない frame は span 数を draw 数で clamp（1 span = 直列 fallback）。
- 分割は **sort 順の等分**（draw 数均等 or 記録コスト推定均等）。span 境界は sort list の index 区切りのみ = 順序は自明に保たれる。

### 4.7 欠陥対応・申告・段階 III
- **閉じる / 開ける**: alpha worker 化が可能に = **crowd の最大 blocker（半透明衣装/髪/エフェクト）の CPU 記録が並列化**。北極星（crowd で固まらない）への本丸。
- **段階 III 追加項目**: ①record lane に **secondary CB level**（現 `rwAcquireLaneCmd:1374` は PRIMARY 固定）+ `VkCommandBufferInheritanceRenderingInfo`（dynamic rendering 継承）②main CB の alpha phase で `vkCmdExecuteCommands`（順序配列）③DrawPlan の order index + sort slice 分割。
- **申告（省略/解釈）**: span 内の per-draw shader bind コストは worker 並列で吸収（§2）。GPU blend の直列性は**不可避で本設計の対象外**（記録の並列化が目的）。alpha の sort 自体（back-to-front 決定）は mutation phase の既存 stateSort が担う（変更なし）。secondary CB の pool/生存期間は lane per-frame（§2.6 と同様）。
- **やらない（本節）**: fail-closed 検出装置（§5）は次段。alpha 以外の順序依存 pass（あれば）は同型で後述。

### 4.8 secondary CB 実現性 verdict（深掘り 2026-07-28）
X =「secondary CB が dynamic rendering scope 内で動くか（本 codebase の VK 版/設定 + MoltenVK）」を実トレース:
- **✅ API サポート = VK 1.3 core**。`app_info.apiVersion = VK_API_VERSION_1_3`（`llvkloader.cpp:1785`）+ device ≥1.3 必須（`:1874`）+ `dynamicRendering` feature 有効化（`:2052-2065`）。∴ **`VkCommandBufferInheritanceRenderingInfo` + `VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT` は 1.3 core で利用可能**（desktop/Linux path）。
- **🆕 基盤は完全新規**: `LEVEL_SECONDARY`/`InheritanceRenderingInfo`/`vkCmdExecuteCommands` の痕跡ゼロ = 段階 III で新規実装（record lane の SECONDARY level CB + secondary pool、`beginDynamicRendering` wrapper に `VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT` flag 追加、main CB での `vkCmdExecuteCommands`）。
- **🧱 壁 = MoltenVK（macOS Apple Silicon target）の secondary-CB-in-dynamic-rendering サポート**。これは**ソースから決定不能**（MoltenVK バージョン依存・Metal の parallel render encoder へのマッピング可否）。→ **段階 III で macOS 実機/MoltenVK ドキュメントによる外部検証が要る 1 点**（釣りでなく named wall）。
- **🛟 fallback 設計（MoltenVK が壁でも alpha worker 化を諦めない）= ordered primary CB spans + inter-span color barrier**:
  - 各 alpha span = **primary CB**（`BeginRendering(LOAD scene color)`/draws/`EndRendering(STORE)`）を span 順に PE へ（既存 pre_cmds/PE FIFO モデル流用）。
  - **inter-span color-attachment barrier**（WRITE→WRITE）で GPU 実行順（back-to-front）を強制。
  - コスト = span 毎に scene color を LOAD/STORE（帯域）= secondary CB（単一 render pass instance・中間 LOAD/STORE なし）より非効率だが**動作は保証**（VK core・MoltenVK 非依存）。
  - ∴ **secondary CB を第一候補（効率）+ primary-span を fallback（MoltenVK 保険）** の 2 段構え = **crowd blocker 解消は MoltenVK リスクに関わらず達成可能**。
- **verdict = feasible YES（desktop 確実 / macOS は secondary=要検証・fallback で保証）**。段階 III III-6 で secondary を実装し macOS 検証、不可なら primary-span へ切替（同じ ordered-span/on-demand-seed 設計の上物だけ差し替え）。

## §5 fail-closed 検出装置（公理 F の enforcer）

> **この装置が「規約でなく機構」を完成させる**。§1-§4 は公理 F を*立てる*設計、§5 は公理 F の*違反を機構が捕捉する*設計。
> 憲法 2（default-deny・未承認 alarm 1 つで PASS 不可）+ 憲法 5/6（正のオラクル・幸運ログ禁止）準拠。**現 seed-lost の「2 冪 throttle = 実質サイレント」を継承しない**。

### 5.1 検出対象 = 公理 F 違反の全分類
| 違反 | 現れ（地図） | 検出手段 |
|------|-------------|---------|
| V1: snapshot 対象が窓中に変異 | S8-b（rebuildMesh 窓中 lazy） | window-mutation guard（§5.2） |
| V2: worker が DrawPlan 外の live state を read | S7 残余（新フィールド未転写） | worker-read seal（§5.3） |
| V3: on-demand seed build 失敗 | S6 残余（消失） | seed-fail alarm（§5.4） |
| V4: DrawPlan が参照する group/VB が世代変化 | 越境の stale | generation validation（§5.5） |

### 5.2 検出 1 = window-mutation guard（V1・S8-b/S13 の機構化）
- **frame-scoped フラグ `sRecordWindowActive`**（最初の dispatch で set・最終 join で clear）。
- **mutation entry point に guard を挿す**: `LLSpatialGroup::rebuildMesh`（`pipeline.cpp:8218`）/ `applyGeoStaged`（`llvovolume.cpp:6194`）/ `stateSort` の変異部 / VB の map/unmap。**record 窓 active 中に呼ばれたら fail-closed alarm**（非 throttle）。
- これは **freeze 契約（§1.3）の正のオラクル**: 契約破り（窓中に mutation が滑り込む）を即捕捉 = サイレント corruption 不可能に。窓が健全なら guard はゼロコスト（フラグ check 1 個）。
- **契約が正しく実装されていれば guard は永久に沈黙する** = これが「実装済み機構の合否 gate」（憲法 6）。

### 5.3 検出 2 = worker-read seal（V2・S7 残余）
- **第一防御は型（§1.2）**: worker が読むフィールドは DrawPlanItem に載る = 未転写フィールドはコンパイル時に「読めない」（構造的封鎖）。
- **runtime 残余防御**: 旧 read entry point（`LLVKBucket::forEachSource` / live `LLDrawInfo` accessor）に **「record-lane thread から呼ばれたら alarm」** の thread-check を挿す。worker は DrawPlan のみ読む契約ゆえ、旧経路への read は違反。
- 効果 = 「worker が snapshot 外を読む」を機構が捕捉（規約でなく）。VKC verbose 時のみ重 check、通常は軽 thread-id 比較。

### 5.4 検出 3 = seed-fail alarm（V3・§2.4 の形式化）
- on-demand build 失敗（§2.4）= **非 throttle VKC alarm**（新シグナル名 例 `seed_build_fail{program,lane}`）+ **join 後 inline fallback**（消失させない）。
- frame は「seed_build_fail 件数」を保持。**gate = 非ゼロで PASS 不可**（憲法 2）。現 `C_RECORD_JOB_PULL` の throttle warning を**この非 throttle 会計に置換**。

### 5.5 検出 4 = DrawPlan generation validation（V4・A3 の record 窓版）
- plan-build 時、各 DrawPlanItem に参照 group の **世代 `mVkGeoGen`（A3 資産・`llspatialpartition.h:450`）を snapshot**。
- record（or execute 前）で worker が **現 `mVkGeoGen` と snapshot 値を照合**。不一致 = 窓中に geometry が変わった = freeze 契約破り → **alarm + 当該 draw skip-with-fallback**（applyGeoStaged の stale_gen 原子対 = A3 と同型の扱い）。
- V1 guard（§5.2）が正しく効いていれば V4 は起き得ない = **二重の網**（guard が破れても gen check が捕まえる）。

### 5.6 fail-closed 統合（憲法 2/4・既存 VKC 装置との統合）
- **全 alarm は非 throttle・default-deny**: V1-V4 の 1 件でも未承認なら PASS 不可（憲法 2）。「たまたまエラーが出なかった走行」を安全の根拠にしない（憲法 6）。
- **既存 VKC 装置（地図 冒頭 §🕸️ L1-L3）に統合**、並行装置を作らない。V1 = L1 発火契約の拡張 / V2 = L2 連続性 / V4 = A3 原子対の窓版。
- **⚠️ 憲法 4**: 検出器 file（`llvkcontract.*`）への diff は **AYA 承認必須**。§5 の検出器実装（段階 III）は AYA gate を通す。装置を盲目化する変更は禁止。
- **新シグナルは docs/alarm_allowlist.md に無断追記しない**（憲法 4）= 新 alarm は既定でブロック側。

### 5.7 移行 A/B オラクル（段階 III gate・正のオラクル・憲法 5/6）
- 地図 冒頭方針「経路移行は **L3 型 A/B オラクル**（worker 出力 vs 旧経路 byte 照合・`verdict=src|kernel`）を伴わない限り受け入れない」を段階 III の受け入れ gate にする。
- **A/B = 同一シーンを ①新 worker 経路 ②旧 serial 経路 で記録し、生成 CB / draw 列 / 描画結果を byte 照合**。差分ゼロ = 非退行の**正の証明**（error 不在でなく機能同一の証明 = 憲法 5）。
- kill switch（`AYASTORM_MT_THREADS=1` 系）で新旧を同一走行内で切替え A/B（起動またぎ A/B 禁止 = 地図/memory の教訓）。

### 5.8 申告・段階 III
- **閉じる**: 公理 F 全違反（V1-V4）を機構で捕捉 = **「規約でなく機構」完成**。これで §1-§4 の設計が「たまたま」に戻らない歯止めを得る。
- **段階 III 追加項目**: `sRecordWindowActive` guard + mutator への挿入 / worker-read seal の thread-check / seed_build_fail 非 throttle 会計 / DrawPlanItem への gen snapshot + 照合 / L3 型 A/B ハーネス。**全て憲法 4 の AYA 承認対象（検出器改変）**。
- **申告（省略/解釈）**: V2 の完全 runtime seal は重いので verbose 限定 + 型封鎖を主防御にする（型が第一・runtime は保険）。guard の粒度（どの mutator まで挿すか）は段階 III で mutation entry の実 audit で確定（VB map/unmap まで含めるか）。A/B の「byte 照合」の対象粒度（CB byte / draw 列 / pixel）は段階 III で決定（geometry A/B は CB/draw 列・視覚は最終 gate のみ=地図厳命）。
- **やらない（本節）**: §3 uniform snapshot（S14/S15）と §6 段階 III 移行計画は残 skeleton。

## §6 段階 III 移行計画（実装順・非退行 gate）

> **grandfather しない**: Phase A（shadow worker）・Phase B（materials worker）の現実装も本設計へ置換する。
> 各段は **①L3 型 A/B（新 vs 旧 serial の byte 照合・§5.7）②validation 0 ③診断起動で検出装置沈黙（§5）** の 3 点 gate。**PASS は AYA のみ**（憲法 1）。

### 6.1 実装順（依存順・foundation first）
| 段 | 内容 | なぜこの順 | 主 gate |
|----|------|-----------|---------|
| **III-0 検出装置 先行** | §5.2 window-mutation guard + §5.5 gen validation を**現行コードに先に入れる** | 移行前に「今どこで freeze 契約が破れているか」を可視化（S8-b の現発生を捕捉）。移行の安全網を先に張る | guard がシーンで発火する箇所 = 移行前に潰すべき mutation の実地図 |
| **III-1 背骨（DrawPlan + freeze schedule）** | §1.2 DrawPlanItem/DrawPlan + arena / §1.3 eager rebuildMesh + 窓中 mutation ゼロ + shadow cascade interleave 廃止 | 全経路の土台。snapshot が無いと seed/uniform/alpha が乗らない | window guard 沈黙（freeze 契約成立の正のオラクル）/ 視覚同一 |
| **III-2 seed 統一（S6 根治）** | §2 record 経路を main 経路の per-lane on-demand build に統一・seed map/ensureXxxSeeds/vkCaptureSeedDynamicBuffers 撤去 + §2.7 descriptor write 経路の lane 隔離 audit | 背骨の上で seed を on-demand 化。動的 pool の前提 | seed_build_fail=0 / shadow・materials A/B byte 一致 |
| **III-3 uniform（S14/S15）** | §3 gGLDeltaModelView thread_local 対称化 + GlobalF arena slice | 小・独立。背骨の ctx に相乗り | A/B byte 一致 |
| **III-4 既存 worker 経路の移行** | Phase A shadow + Phase B materials を DrawPlan + on-demand seed へ載せ替え・**isMapped skip 撤去**（§1.3）・「1 program 1 job」前提コード撤去（§2.5） | 新機構で既存を再実装＝非退行を先に確定してから拡張 | shadow/materials の L3 A/B・稀症状（体消失/関節崩れ）消失の実地確認 |
| **III-5 静的 pool 拡張** | simple/fullbright を worker 化（§8: 1 pass 1 shader ゆえ低リスク） | 背骨+seed が効くので安全に横展開 | 各 pool A/B・shad/gbuffer wall 短縮 |
| **III-6 alpha（crowd 本丸）** | §4 secondary CB lane + inheritance + vkCmdExecuteCommands 順序実行 + DrawPlan order span | 最難関を最後に。secondary CB 基盤が新規 | alpha A/B（blend 結果 pixel 一致）・crowd 会場で固まらない（北極星 gate） |

### 6.2 各段の gate 詳細（憲法 5/6・正のオラクル）
- **L3 型 A/B**（§5.7）= 同一走行内で kill switch 切替（起動またぎ禁止・地図/memory 教訓）。照合粒度 = geometry/CB は draw 列・byte / 視覚は最終 gate のみ（地図厳命「視覚を判定オラクルにするな」）。
- **検出装置沈黙** = 診断起動（`AYASTORM_VKC=1`）で §5 の V1-V4 全て 0。**沈黙は「実装済み機構の合否」にのみ使い実装可否には使わない**（憲法 6）。
- **幸運ログ禁止**（憲法 6）= 「N 走行クリーン」を安全の根拠にしない。防御（fail-closed 経路）は「失敗したら何が起こるか」で設計・実装する。

### 6.3 kill switch 運用（CLAUDE.md 準拠）
- 各段に切り分け switch（`AYASTORM_MT_THREADS=1` 系の粒度）。**gate PASS 後に即削除**（恒久 fallback を残さない）。III-6 の secondary alpha は専用 switch で A/B 後撤去。
- 工事中の描画異常はまず当該段 switch で新旧帰属を確定してからコードを追う。

### 6.4 憲法遵守チェックリスト（着手前・全段）
- **憲法 4**: §5 検出器・`llvkcontract.*` への diff は **AYA 承認必須**。装置盲目化・allowlist 無断追記 禁止。
- **憲法 2**: 新 alarm（seed_build_fail 等）は default-deny 側。未承認 1 件で PASS 不可。
- **隠蔽禁止**（憲法 5）: isMapped skip 撤去は「skip で誤魔化す」の除去 = 正しい方向。try-catch/早期 return で信号を消さない。
- **品質トレード禁止**: 記録並列化は品質不変（GPU 出力同一が A/B gate）。速度のための品質低下は product 決裁。
- **read before edit / 機械一括置換禁止 / 新規コメント禁止 / commit トレーラ禁止**。

### 6.5 リスク・申告（段階 III 全体）
- **最大リスク = III-1 freeze schedule の frame 再構成**（shadow cascade interleave 廃止 + eager rebuildMesh）。現 frame 構造への侵襲大 → III-0 の guard で現状の mutation 分布を先に把握してから着手（探索を安い手＝guard に寄せる）。
- **secondary CB（III-6）は新規基盤** = record lane の PRIMARY 固定（`:1374`）を拡張。dynamic rendering 継承の検証が要る。
- **申告（省略/解釈）**: DrawPlan の arena サイズ・span 数・MAX 定数は実装時に実測で確定（設計判断には使わない=憲法 6）。GLTF PBR opaque pool（地図 §8.5 未精査）は III-5 で 1 pass 1 shader を確認してから編入。avatar pool（mode 切替）は本移行では静的 pool 群の後・§2.5 で seed 制約は解けているので mode 別 program union でなく on-demand で自然に載る見込み（III-5/6 で確認）。
- **やらない**: 実装そのもの（段階 III の作業）。本 §6 は順序と gate の設計まで。

---

## §7 設計文書 完了サマリ（段階 II 全体）

- **公理 F**（§0）= worker は不変 snapshot 上でのみ動き・窓中 main は不変・違反を機構が検出。地図の全欠陥は公理 F が規約依存だったこと。
- **§1 背骨** = DrawPlan（スカラ copy）+ freeze-by-contract（VB は eager-mutation schedule で窓中不変）→ **S7/S8-b/S13 を閉じる**。
- **§2 seed** = 既存 per-lane 基盤を統一し on-demand 化・seed map 撤去 → **S6 根治 + 「1 program 1 job」解除 + 動的 pool 網羅**。
- **§3 uniform** = S14 thread_local 対称化・S15 A6 arena slice → cross-cutting 未隔離ゼロ。
- **§4 alpha** = ordered span + secondary CB 順序実行 → **crowd blocker を CPU 記録並列化で崩す**（GPU blend は直列のまま）。
- **§5 検出** = window guard / worker-read seal / seed-fail / gen validation + L3 A/B → **「規約でなく機構」完成**（fail-closed・憲法 2/5/6）。
- **§6 移行** = 検出先行 → 背骨 → seed → uniform → 既存移行 → 静的拡張 → alpha の順・各段 A/B gate。

> **段階 II（再設計）ここまで**。実装は段階 III・計測は段階 IV（最終 gate のみ）。**PASS は宣言しない**（憲法 1）= 本設計の検収・段階 III 着手は AYA gate。
