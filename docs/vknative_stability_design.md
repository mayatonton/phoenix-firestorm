# VK-native レンダラ 安定性 基盤設計（Stability by Design）

> **位置づけ**: これは**安定性だけ**の設計。並列化（worker 記録・§9 等）は本設計に**含めない**別トラックで、安定基盤が機構で立った後にその上へ乗る（AYA 2026-07-28）。
> **原理**: 不安定なアプリを安定にするのは**設計**（クラッシュのクラスを不変条件の機構で構造的に不能化）であって、症状を 1 件ずつ潰すバグつぶしではない（[[feedback_stability_is_design_not_bugfixing]]）。落ちる = 未完成。設計で落ちないように作る。
> **真実源**: HEAD ソース file:line。推論禁止（憲法 line 78）。

---

## §0 単一の設計原理（すべての不変条件の親）

> **すべての資源（CPU オブジェクト / GPU オブジェクト / 共有 state）は、その生存期間とアクセス規律が「構造（型・fence・隔離・依存・上限・縮退経路）」で強制される。instability = どこかで構造の代わりに規約・偶然が使われている状態。**

現コードは大半を**規約・偶然**で成立させている（＝「動くが落ちる」）。安定化とは、各資源の規律を**規約→機構**に格上げし、破れを構造的に不能にすること。落ちる条件が多数あっても、その根は下記 6 クラスの欠けた不変条件に集約される。**個々の症状を潰すのでなく、不変条件を機構で立てればクラス全体が同時に消える。**

この原理は 6 つの facet（不変条件 I〜VI）に分解される。各 facet は「クラッシュのクラス」と 1:1。**6 つ全てが機構で成立して初めて安定**。

---

## §I. CPU オブジェクト生存（heap 破損 / UAF / double-free クラス）

**不変条件**: すべての managed オブジェクト（refcountable=LLRefCount / poolable）は、その生存期間が**全スレッド・全フレームの全使用を厳密に包含**する。借用中解放・二重解放・解放後使用・再割当後の旧参照使用が構造的に不能。

**機構（設計）= 境界越え参照の所有型強制**:
> **スレッド境界 or フレーム境界を越える managed オブジェクトへの参照は、所有参照（`LLPointer<T>`）か値コピーのいずれかでなければならない。managed オブジェクトへの raw pointer / index は境界を越えてはならない。**

- 境界を越える構造体（job / staged / snapshot / built）は **`LLPointer<T>` か値のみ**を保持できる型に制約する。managed への raw を持てない＝転写漏れ・生存漏れがコンパイル時に露見（型で網羅）。
- 「owner が pin されてるから raw でも安全」を**明示化**する：raw の referent を実際に pin する（`LLPointer` 化）か、必要な値をコピーする。**owner-pin への暗黙依存（＝owner が窓中に sub-object を再割当/解放しないという規約）を残さない**。

**現状（トレース済 file:line）**:
- geo worker（`llvovolume.cpp`）= **大半機構**: `LLGeoRebuildJob`(:5855) が `LLPointer<LLSpatialGroup> mGroup` / `std::vector<LLPointer<LLVolume>> mPinned`（source pin）/ `LLGeoFaceApply`(:5797) が `LLPointer<LLDrawable> mDrawable` + `LLPointer<LLVertexBuffer> mBuffer`。
- **HEAD 再トレース（2026-07-28・訂正）= flagged raw は「規約」でなく機構保護だった**: `VbSlice::mVb`(:5824)/`BuiltDraw::mVb`/`mTexRaw`/`mTexListRaw` は refcountable で、**隣接 LLPointer による連続 pin**（`mVb`←`mBuffer`(:5802 LLPointer)/`mTexRaw`←snapshot `mTexture`(:5786 LLPointer)→ fold で `di->mTexture` LLPointer(:7469)へ・gap なし）。`mFace`/`mLeaderFace`(LLFace\*=非 refcount, llface.h:165) は worker では **deref せず raw 転送のみ**（build fn 7293-7457 は snapshot 値のみ読む）で、実 deref（`setDrawInfo` :7481）は **main の `foldBuiltDrawInfo` が applyGeoStaged の 6224 `getFace(TEOffset)==e.mFace` identity guard 通過後にのみ**行う＝validate-then-apply（A3）機構。geo fill worker（runVkGeoFill llface.cpp:2916）は worker 所有 deep-copy snapshot を読み job-private VB（`mBuffer` LLPointer pin + in-flight isolation）へ書く。∴ geo は公理 F/§I を**既に満たす**。旧「規約に寄りかかる＝機構でない」記述は HEAD 未トレースの assumption（憲法：doc claim は根拠にならない）。残るは refcountable raw の LLPointer **明示化**（暗黙の連続 pin 依存を自明化・UAF 修正ではない・低優先）のみ。

**設計適用**: 上記型強制を境界越え構造に適用 → raw referent を pin or 値化。geo は VKGeo fix で相当固いが残渣を機構化。**他境界（texture upload / avatar-skin / PE job / pool-arena cross-frame）も同一機構で判定・conform**（実装時に各構造の raw を型強制で洗う。**「安全な raw」を型が許さない**＝発生源が構造的に消える）。

---

## §II. GPU 資源生存（device-lost / in-use destroy / VUID / teardown crash クラス）

**不変条件**: GPU 資源（VkBuffer/Image/View/DescriptorSet/CommandBuffer/Fence/Pipeline）の**破棄・reset・上書き**は、それを参照する GPU 作業が **fence 完了した後にのみ**許される（資源生存 ⊇ GPU 使用 span）。teardown は GPU idle 確認後にのみ破棄。

**機構（設計）= fence-gated ライフサイクル**:
> **GPU 資源はそれを最後に使った frame-in-flight に紐づき、その FIF の fence が signal した後にのみ reclaim（破棄/reset/上書き）される。破棄は全て fence-gated な単一 reclamation 経路を通る。teardown は `vkDeviceWaitIdle` の戻り値を検査し、成功時のみ破棄する。**

- 即時 `vkDestroy*` を禁じ、**deferred-free**（frame fence に紐づく遅延解放）へ一本化。per-frame reset（command pool / arena）も FIF の fence 完了が前提。
- 単一 submitter（PE thread が唯一 `vkQueueSubmit/Present`＝資産 A2）を死守：異スレッド同時 submit のクラスを構造排除。
- teardown = **quiesce（producer 停止）→ WaitIdle（戻り値検査）→ destroy** の順を全経路で守る。

**現状（トレース済）**:
- 機構: 単一 PE submitter（`llvkloader.cpp:1012`＝資産 A2/S16）/ frames-in-flight + fence poll（S1）/ texture readiness = PE FIFO 順（S17）/ shadow depth = submit 順+barrier（S9）/ teardown design C（quiesce 最前 + reapAllDeferred）。
- **残渣**: teardown `vkDeviceWaitIdle`(`llvkloader.cpp:4910)`の**戻り値未検査** → GPU hang 中終了で DEVICE_LOST でも idle 扱い → pending CB 保持 pool 破棄 → driver UAF → jemalloc SEGV（＝旧 P4/G3）。device-lost 早期 return の待ち手解放（G2）は§VI へ。

**設計適用**: WaitIdle 戻り値を検査し非成功なら lost 経路（破棄を deferred/skip）。全 `vkDestroy*` を deferred-free 単一経路へ。

---

## §III. 並行性 / 共有 state（race → corruption クラス）

**不変条件**: >1 スレッドが触る**すべての** state は、(a) 共有中は不変（immutable snapshot）/ (b) 一時点で単一スレッド所有（thread_local or per-lane 隔離）/ (c) 明示同期下（mutex/atomic）のいずれか。**同時アクセスされる共有 mutable が存在しない**。

**機構（設計）= 隔離 or 不変 or 同期を構造で強制**:
> **並行経路（worker が走る窓）で触れる state は、thread_local か per-lane か immutable snapshot でなければならない。共有 mutable への write が並行経路に存在しない（構造的除去）。残る共有は明示 lock 下のみ。**

- 描画現在状態は網羅 thread_local（資産 A1）/ per-draw 資源は per-lane（`sPerDrawDescLanes`）/ per-frame 資源は frame-index arena（A6）。**これらの隔離を「全数・機構」で保証**（1 個でも共有 mutable が並行経路に残れば race）。
- cross-thread handoff は mutex 越し所有権移動（geo publish / PE enqueue＝A3/S10）。

**現状**:
- 機構: A1（gGL 含む描画状態 thread_local）/ per-lane descriptor / A6 arena / mutex handoff。
- **残渣**: 並行経路が触る共有 program-instance 可変 field（per-program UBO slot/content・push-constant shadow `mVkFragPC` 等）。**※これは並列化トラック（§9）の worker↔worker 軸で扱う対象**。安定性トラックとしては「共有 mutable を並行経路に置かない」という不変条件を親規律として確定し、具体箇所は並列化設計が conform する。

**設計適用**: 並行経路の read-set は snapshot/隔離、write は経路から構造排除。安定性設計は不変条件を立てるところまで（並列化が守る）。

---

## §IV. 順序 / 依存（out-of-order → garbage / hang クラス）

**不変条件**: すべての操作は、その data 依存が**生成・同期された後にのみ**実行される。順序は明示同期（CPU-GPU=fence / GPU-GPU cross-submit=semaphore / intra-CB=barrier / same-queue=submit 順 / CPU-CPU=mutex handoff）で保証し、偶然のスケジューリングに依存しない。

**機構（設計）= 依存エッジの明示 owner**:
> **すべての cross-CB / cross-thread / cross-frame の data 依存に、それを保証する同期プリミティブを file:line で名指せる。名指せない依存＝偶然依存＝IV 違反。**

**現状**:
- 機構: shadow depth（submit 順+barrier・S9）/ present fence gate（S1）/ texture PE FIFO（S17）/ layout 遷移 barrier。
- **残渣**: async cadence 分離下の submit 列順序（producer/consumer/apjob の pre_cmds 経路＝旧 P1/S1）。**※並列化トラックの順序破壊として §9 が扱う**が、安定性としては「消費前に生成物が破壊されない」不変条件を親規律に置く。RT layout CPU tracker の記録順=実行順前提（S10）も IV。

**設計適用**: 各依存に同期 owner を割り当て、偶然依存を消す。

---

## §V. 資源枯渇（OOM / 無限膨張 → hang クラス）

**不変条件**: すべての生産（alloc / inflight 提出 / queue 投入）は消費で**有界**。無限成長する queue/pool/inflight-set が存在しない。

**機構（設計）= 全生産に hard cap + back-pressure**:
> **すべての producer は hard cap を持ち、cap 到達時は block（back-pressure）or drop-with-signal（fail-closed）する。1 job が cap を超える場合の分割・inline 予算外・最低 1 job も cap 経路に載せる（穴を残さない）。**

**現状**:
- 機構: geo inflight byte cap（`GEO_INFLIGHT_BYTE_CAP` `llvovolume.cpp:5882`=128MB）/ frame pacing backstop（V1）/ one-shot submit byte cap（V2）。
- **残渣**: safety_valve V2a の穴（最低 1 job 分割不能 / inline 予算外）＝旧 G4。PE queue / geo publish queue の上限。

**設計適用**: 全 queue/pool/inflight に cap を機構化し、穴（分割不能・inline）を塞ぐ。

---

## §VI. 失敗処理（error path → crash クラス）

**不変条件**: すべての失敗（device-lost / OOM / 資源欠落 / fence timeout / 検証失敗）に **clean-degrade 経路**があり、失敗が半端・corrupt 状態を残さない。**失敗は必ず起きる前提（憲法 6b）で、観測でなく「失敗したら何が起こるか」で設計する。**

**機構（設計）= 単一解放点 + validate-then-apply + fail-closed**:
> **(a) 非同期 job のあらゆる exit path（成功/失敗/skip/device-lost）は、その job に紐づく全待ち手（slot/fence/sync/pin）を「単一解放点」で必ず release（個別ケース列挙でなく機構）。**
> **(b) 越境入力は使用前に全検証し、不整合なら原子的に abort（部分適用ゼロ）＋再予約（喪失↔再予約の原子対）＝資産 A3。**
> **(c) 検出は fail-closed（黙って落とさない・憲法 2）。silent return 禁止。**

**現状**:
- 機構: validate-all-then-apply + 喪失↔再予約原子対（applyGeoStaged＝A3/S11）/ device-lost → clean 終了（恒久受容）。
- **残渣**: peExecute device-lost gate の待ち手解放が**ケース列挙**（is_frame/is_oneshot のみ救い sync 漏れ＝旧 G2・freeze の根）＝**列挙 vs 不変条件ファースト違反**。G2 の最小 mirror は現状十分だが accidental completeness ＝「単一解放点」で機構化すべき。

**設計適用**: 待ち手を「単一解放点」で機構化（全 exit path がそこを通る＝列挙漏れ構造的に不能）。全 error path を clean-degrade 化。

---

## §7 並列化との関係（別トラック・本設計には含めない）

並列化（worker 記録）は独立目標でなく、**上記安定基盤の consumer**:
- worker が触る read-set は §III（隔離/snapshot）+ §I（境界越え所有型）を守る。
- worker CB の submit 順は §IV（依存 owner）を守る。
- worker 増設は §II（単一 submitter）+ §V（inflight cap）を守る。

∴ **安定基盤（I〜VI）が機構で緑になるまで、並列化は乗せない**（空中の家を建てない）。並列化設計（§9 等）は本設計の上に conform する後段トラック。

---

## §8 完了定義（この設計の DoD）

**安定 = I〜VI の全不変条件が「規約・偶然」でなく「機構」で成立**。各不変条件について、現アーキが〈機構保証 / 規約・偶然 / 未保証〉のどれかを file:line で判定し、〈規約・偶然 / 未保証〉を機構化 → その結果、対応するクラッシュのクラスが構造的に不能になる。**PASS は AYA gate のみ**（憲法 1）。

## §9 申告（縮小・省略・解釈）
- **設計の完全性**: I〜VI の**不変条件と機構（親規律）は本 doc で確定**。各不変条件の現アーキ全境界での〈機構/規約/未保証〉判定は、geo（§I）/ teardown（§II）/ A1・per-lane（§III）/ S9・S1（§IV）/ cap（§V）/ A3・G2（§VI）まで file:line で確定済。~~未判定境界 = texture upload / avatar-skin-palette / PE job 生存 / pool-arena cross-frame reset~~ → **✅ 全て HEAD トレース完了（2026-07-28）**: texture(全 source LLPointer)/ avatar-skin(LLDrawInfo 全 LLPointer)/ palette(source=global static・target=fence-gated ring)/ PE job(handles=値・sync=caller-stack)/ arena-reset(fence-gated `:5562`)= **全て §I/§II 機構保護**。§I raw-UAF residue は現アーキに不在。trace= scratch `trace_stability_sec1_*` / `sec2_arena_reset`。
- **taxonomy の解釈**: I〜VI は「構造で機構化できる安定性の facet」。per-operation の論理正しさ（誤 index 計算等）は architectural 機構でなく **validate-then-apply（A3）+ fail-closed 検出**（§VI）で捕捉。設計は「不正入力が corrupt を生まず degrade する」まで保証し、「論理が常に正しい」は保証しない（それは test/検証の領域）。
- **並列化の分離**: §III/§IV の worker↔worker 具体は並列化トラック（§9）へ委譲。本設計は親不変条件の確定まで。
- **PASS 宣言なし**（憲法 1）: 本設計の検収・実装は AYA gate。

---

## §10 現アーキの〈機構 / 規約〉判定 = 全境界（file:line・＝機構の適用結果）

> 各境界を確定済み機構（§I〜VI）で判定。〈規約・偶然〉の行＝不安定の発生源＝機構化対象。**別台帳でなく設計の適用結果**（機構は §I〜VI で確定・ここは適用先の列挙）。

| 境界 | 不変条件 | 機構で満たす部分 | 〈規約・偶然〉残渣（発生源） |
|---|---|---|---|
| geo worker（`llvovolume.cpp`） | §I | source/drawable/buffer/drawinfo = **LLPointer** | ~~raw = owner-pin 規約~~ → **✅ 機構保護（HEAD 訂正 2026-07-28）**: refcountable(mVb/mTex) = 隣接 LLPointer 連続 pin / mFace = worker 転送のみ + main 6224 A3 下 deref / fill = 所有 snapshot + private pinned VB。残＝LLPointer 明示化のみ（UAF でない） |
| texture upload（`llimagegl.cpp:1602`） | §I | 値 + 自作 VK output（`LLVkTexUploadJob`） | ~~raw data/mPickMask = 規約~~ → **✅ 機構保護（HEAD 訂正）**: `TexCreateJobEntry`(llviewertexturelist.h:173) の source = 全 LLPointer(`mRawImage`)・worker は pinned から getData()(cpp:1410)・`mPickMask` = job 所有(dtor delete)・`data` フィールド無し |
| avatar-skin（LLDrawInfo mAvatar/mSkinInfo） | §I | `LLPointer<LLDrawInfo>` pin | ~~内部 mAvatar/mSkinInfo raw~~ → **✅ 機構保護（HEAD 訂正）**: `LLDrawInfo`(llspatialpartition.h:134-143) の `mAvatar`/`mSkinInfo`/`mAttachedToAvatar` = 全て LLPointer/LLConstPointer pin（:135 に「be defensive about UAF」コメント）。未列挙 raw = `LLSpatialGroup::mAvatarp`(:440) は low-priority |
| PE job（`llvkloader.cpp:888`） | §II | 単一 submitter(A2) + handle 値 + FIF fence + `sync*`=blocking stack 安全 | **残渣なし**（arena/pool reset の fence-gate のみ要確認 = 下記） |
| arena/lane reset（lane pool `:1414` / arena `:7618` / main CB `:5626`） | §II | frame-index[sFrameIndex] + `reset_frame`/`a.frame`==`sMonotonicFrameCount` | ~~要 1-hop~~ → **✅ 機構保護（HEAD 検証 2026-07-28）**: 全 reset は beginFrame の FIF fence wait（`:5562` `vkWaitForFences(sInFlightFences[sFrameIndex])`）**後**（lane=recording / arena=reap churn `:5652` / CB=`:5626`・straight-line single-main）。加えて `destroyBufferVk`(:8711)=deferred-free。trace= scratch `trace_stability_sec2_arena_reset.md` |
| teardown（`:4921`・stale ref 4910 訂正） | §II | quiesce + design C（reapAllDeferred） | ~~`vkDeviceWaitIdle` 戻り値未検査（G3）~~ → **✅ 実装済（2026-07-28）**: 戻り値検査し非成功で `device_lost=true` 昇格 → `reapAllDeferred(REAP_LOST)` 既存 lost 経路へ routing（jemalloc SEGV クラス機構化） |
| inflight cap（`:5882` 他） | §V | geo byte cap / V1 backstop / V2 | ~~V2a 穴（G4）= 上流 gate 7557 のみ~~ → **✅ 実装済（2026-07-28）**: dispatch の単一 choke（`llvovolume.cpp:8162` inline 判定）に inflight admission（over-cap∧inflight 非空→inline・bounded/progress・min-1-job は dispatch）を機構化＝上流 gate バイパスに依存しない |
| waiter release（`:948` peExecute） | §VI | A3 validate-then-apply（applyGeoStaged） | device-lost gate が**ケース列挙**（is_frame/is_oneshot・sync 漏れ・G2）= 場当たり → **単一解放点**で機構化 |
| worker↔worker 共有 program state | §III/IV | A1 thread_local / per-lane / A6 arena | 共有 program 可変 field = **並列化トラック（§9）へ委譲**・本設計は親不変条件まで |

### 結論 = 安定性の発生源（＝機構化すべき規約残渣・4 クラス）
1. ~~**§I raw sub-ref**（最広範）~~ → **✅ 訂正（HEAD トレース 2026-07-28）**: flagged 3 境界（geo/texture/avatar-skin）の raw は全て機構保護済（refcountable=連続 LLPointer pin / 非 refcountable=A3・所有 snapshot・in-flight isolation）と判明。§I は「最大工事」ではない。残る §I 実作業 = ①refcountable raw の LLPointer 明示化（低優先・UAF でない）②未列挙 raw（`mAvatarp` 等）と arena reset fence-gate の 1-hop 精査。
2. **§II teardown WaitIdle 未検査**（G3）→ ✅ 戻り値検査 + lost 昇格実装済（2026-07-28）。
3. **§V inflight cap 穴**（G4）→ ✅ dispatch 単一 choke に admission 実装済（2026-07-28）。
4. **§VI 待ち手ケース列挙**（G2）→ **単一解放点（← 現在の実作業標的）**。

**今日の jemalloc crash の §I raw-UAF 説は flagged 3 境界では不成立**（全て機構保護）。残候補 = §II G3（実装済）/ §VI G2（freeze の根・実装中）/ doc 未列挙 raw。attribution 追跡は不要（AYA 明言）＝機構を立てることで当該クラスを構造的に不能化する方針。

### §10 申告
- avatar-skin の `mAvatar`/`mSkinInfo` の正確な型は header grep 不一致で**未確定**（geo/texture と同一 §I パターンの強い推定）＝実装時 1-hop で確定。arena reset の fence-gate も要 1-hop。**いずれも確定済み機構（型強制 / fence-gate）の適用対象で、新しい設計判断は不要**。
- §III/§IV の worker↔worker 具体は並列化トラックへ委譲（安定性設計は親不変条件の確定まで＝分離厳守）。
