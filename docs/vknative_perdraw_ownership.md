# VK-native per-draw 所有権モデル(Phase 2 前提)

- 位置づけ: **分散化(並列 record)が要求する前提**。「per-draw の GPU 束縛状態を誰が所有し、draw とどう運ぶか」を確定する目標設計。
- 適用先 branch: `ref/phase2-perdraw-ownership`。上位 = `docs/vknative_recovery_plan.md`(工程)/ `docs/vknative_architecture.md` §1(資源モデル)。
- 本資料は **目標状態のみ**を記す。現状の ambient 機構・症状・回避策は本資料の対象外(棚卸しログが持つ)。
- **判定原則(不変)**: コードが唯一の真実。本資料は方向を定めるが、実装の可否は HEAD の file:line で判定する。

---

## 0. 破れている不変条件(治療対象・一文)

> **記録済み `vkCmdDraw` が実行時に読む GPU 束縛状態(descriptor set・DrawData id・dynamic offsets)は、その draw の `LLDrawInfo` を知る単一の権威が生成したものでなければならない。**

現状はこれが **ambient thread-local 変数 + 散在無効化 + lossy fallback** で維持され、「権威が最後に走り、間に誰も乱さなかった」規約でしか正しくない。単一 record スレッド前提であり、並列化で全て競合/stale 化する。

---

## 1. 原則(3 つ)

1. **PULL でなく PUSH**: record 呼び出しは ambient な「今の状態」を引かない。権威が draw の完全束縛を**明示的に引数で渡す**。
2. **単一 authority**: 1 draw の束縛は 1 箇所が 1 回計算する。fallback は状態を**発明しない**(権威が走らなければ assert して落とす)。
3. **draw に束縛して運ぶ**: 束縛は draw(`LLDrawInfo`)が所有し、frame-slot 単位で自己検証キャッシュする。並列 recorder は draw を触るだけで衝突しない。

---

## 2. 所有権モデル(層別・目標)

### 2.1 RecordContext(T1 ambient の受け皿)— 本丸

現在 global thread-local に散っている「今の draw」状態を **RecordContext** オブジェクトに集約する。

- 収容: `curBoundShader` / `perCallDescriptorSet` / `dynamicOffsets`+`shape` / DrawData `currentId` / Once-memo(`lastPipeline`/`lastDescSet0-2`/`lastViewport`)。
- 個数: **並列 recorder(record lane / in-flight command buffer)ごとに 1**。単一 record なら 1 個 = 現状と挙動同値。
- Once-memo は「その command buffer に何が bound か」= 本来 **RecordContext 固有**。global thread-local から context field へ移すだけで意味論は不変(監査 T1memo = 既に単一 choke)。

### 2.2 二層キャッシュの一本化(22 invalidator の蒸発)

現状は **2 段のキャッシュ**が重複している:
- 粗い global 層 = `sCurPerCallVkDescriptorSet`(22 箇所が「変えたら NULL」)。
- 精密な per-draw 層 = `LLDrawInfo::mVkSetMemo*`(**live view/sampler/image ポインタ + topo-gen + ring-sig で自己検証**)。

**per-draw 層は既に自己検証で正しい**。ゆえに:
- 粗い global 層と **22 の invalidate-poke を全廃**。
- record は「権威が返した validated 束縛」だけを使う。cross-draw 正しさは per-draw memo の署名が担保する。
- → 22 個を個別修正するのではなく、**設計判断 1 個で一括蒸発**する。

### 2.3 DrawData id / texture slots(T2)

- authority を **単一化**: 現状 2 系(`buildAndOverride` = {diff,norm,spec} / `llvkbucket ensureRecordDrawDataSlot` = diffuse-only)を 1 つに統合。record 系は per-program 数だけの薄い adapter に。
- id 生成は authority のみ。**fallback の id 生成(`populateAndBindUniversalDescriptorSet` heap 分岐)は削除**。
- slot 内容 = 4 channel を full に扱う(diffuse/normal/spec/…)。「diffuse-only を 0 埋め」を機構から除く。

### 2.4 texture の GPU 同一性(T3)

- resolver `heap_slot_for` の **3 重複(lldrawpool / llvkbucket / llglslshader)を 1 関数に統合**。
- 同一性の安定化: heap slot を「view 変化で mutate + 不完全購読」から、**取得後不変 handle**(texture 生存中 slot 固定・view 差替は descriptor 更新のみ)へ。これで購読 patch 自体が不要化する(可能なら)。判断は 3.2。

---

### 2.5 gate 反映(2026-07-18 feasibility = GO・`docs/gate_perdraw_ownership_result.md`)

feasibility gate の結果、以下 3 点を上記モデルに折り込む(GO を覆さない有界事項):

- **§2.2 の訂正(重要)**: per-draw memo(`mVkSetMemo*`)は **non-bindless indexed 限定**(`is_indexed = mIndexedTextureChannels>0 && !mVkUsesBindlessHeap`)。**bindless(= materials/E2 の目的地)・非 indexed 単テクスチャ・gltf・immediate は memo 対象外**。よって「global 層を memo で一括蒸発」は **memo 対象 subset にのみ真**。bindless の per-call set は事実上 per-shader 準定数(per-program UBO + dynamic offset)なので、global 層除去後は ①bindless 専用 memo(key = DrawData-id + ring-sig)を足す か ②rebuild-every-draw を受容する かを **T3 直前に決裁**。invalidator 22 は全て COVERED/N-A(署名拡張不要)なので削除自体は安全。
- **§2.3 の訂正**: fallback `populateAndBindUniversalDescriptorSet` は **全廃不可**。immediate-mode `LLRender::flush`(`llrender.cpp:1664`)= `LLDrawInfo` を持たない正当な paramless draw で権威代替不可。→「fallback 削除」でなく **immediate-mode を RecordContext 所有の paramless authority に切り出す**(§3-③ の assert はこの切り分けの発見に使う)。
- **T5 の束ね**: G2 の per-lane memo 化(`mVkSetMemo*` を `[NUM_LANES]` struct 化)と ScenePerDrawCache の pin/release thread-safe 化は **同時 landing**(memo partition だけ先行させると pin refcount 破壊)。texture-lifecycle 大改修は不要。

## 3. 未決の設計分岐(実装前に確定)

1. **RecordContext の受け渡し**: 明示引数 vs thread-bound context(thread_local だが「globals でなく context object」)。並列度と侵襲性のトレードオフ。**推奨 = thread-bound context**(呼び出し面の改修を最小化しつつ per-lane 所有を得る)。
2. **T3 の同一性安定化**: 「取得後不変 handle」に全面移行 vs 現行 mutate + 購読を authority 単一化だけで延命。前者は購読機構ごと消せるが texture 側改修が要る。**要判断**。
3. **fallback 全廃の安全性**: 「権威を経ない heap draw」が実在するか(監査で未特定)。全廃前に assert 版を一度通し、発火経路を炙り出してから削除。

---

## 4. 閉鎖条件(B が「追加」でなく「置換」であること)

新オーナーを足して旧経路を残すと病は温存される。閉鎖 = 棚卸しの **消費者カタログを全消化**:
- T1a invalidator 22 + fallback id 生成 = 削除。
- T1 memo global thread-local = RecordContext field へ移設。
- T2 dual-writer = 単一 authority へ統合。
- T3 resolver 3 コピー = 1 関数へ。

各項の file:line は棚卸しログ(A)を参照。

---

## 5. 実装 TaskList(工程・直列・各段 gate)

各段: **直前に JIT 詳細設計を AYA へ提示 → 承認 → 実装 → gate(視覚同一 + validation 0)→ 完了報告**。直列(1 段ずつ)。順序 = 掃除で表面積を減らす → RecordContext で並列安全化 → 閉鎖。

| # | 段 | 型 | 前提 gate | 触る主 file |
|---|---|---|---|---|
| **T0** | **feasibility gate**(読取・Fresh) | 判定 | — | `docs/gate_perdraw_ownership_feasibility_brief.md` |
| T1 | heap resolver 3→1 統合 | prep・低 | T0=GO | lldrawpool / llvkbucket / llglslshader の `heap_slot_for` |
| T2 | DrawData id authority 単一化(dual-writer 解消) | prep・中 | T1 | lldrawpool `buildAndOverride` + llvkbucket `ensureRecordDrawDataSlot` |
| T3 | 二層キャッシュ一本化 = global `sCurPerCall` + 22 invalidator 廃止 → per-draw memo へ | core・**本丸** | T0.G1=COVERED | 22 サイト(棚卸しカタログ)+ llvertexbuffer/lldrawpool の reader |
| T4 | fallback の id 生成廃止 → assert → 経路 routing → fallback 削除 | core・中 | T0.G1-fb | llglslshader `populateAndBindUniversalDescriptorSet` |
| T5 | **RecordContext 集約**(残 ambient + Once-memo を per-context 化)= 並列 record 安全化 | core・大 | T3,T4 | llglslshader(thread_local 群)+ llvkloader(Once 族・reset) |
| T6 | T3 同一性の不変 handle 化(purchase 機構削減) | opt・条件付き | T0.G2 の判断 | llimagegl `updateVkHeapSlot` + llvkbucket 購読 |
| T7 | 閉鎖確認 = 消費者カタログ全消化 + 診断/kill switch 整理 + E2 WIP 再判断 + white 消滅の視覚確認 | closure | T3-T5 | 横断 |

- **gate 反映(§2.5)**: T3 は「invalidator 22 削除(安全確定)」+「bindless の per-call set 所有(memo 追加 or rebuild 受容)を **T3 直前に決裁**」の 2 部構成。T4 は fallback「削除」でなく「immediate-mode を paramless authority に切出」。T5 は per-lane memo 化と ScenePerDrawCache thread-safe 化を**同時 landing**。
- **閉鎖条件**: §4 のカタログが全消化されていること(旧 ambient を読む/書く/前提にする残存ゼロ)。T7 で機械確認。
- **kill switch**: 各 core 段は工事中のみ切替 switch を置き、gate PASS 後に即削除(恒久 fallback を残さない = 本改修の趣旨そのもの)。
- **E2 白**: T3〜T5 の副産物として消える見込み。T7 で `AYASTORM_E2_MAT` を戻して視覚確認 = 本改修の end-to-end 検収の一部。

## 6. 非目標 / 保留

- E2(materials bindless)の white バグ単体の修正は本モデルの副産物として自然に消える見込みだが、**本モデルの目的ではない**。E2 WIP は stash 退避済(要否は別議論)。
- 実際の並列 record スケジューリング(どのパス/バケットをどの lane に割るか)は本モデルの**外**。本モデルは「並列にしても壊れない所有権」までを担保する。
