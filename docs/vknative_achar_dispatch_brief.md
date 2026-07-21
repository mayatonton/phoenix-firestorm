# aChar dispatch 実装 brief(Phase 2 本工事・Fresh 実装用)

**status**: 設計(designer brief)。実装は Fresh セッション想定([[feedback_designer_directs_fresh_crosscheck]])。**入場ゲート = 並列安全性検出器 D1-D4 完(commit `81b7b0aef4`)= 通過必須の守り。**
前提監査 = [[project_achar_shared_state_audit]] / 検出器 = `docs/vknative_phase2_safety_detector_design.md` / handoff = [[handoff_phase2_safety_detector]]。
**⚠️ HEAD で file:line 再確認必須**(本 brief の行番号は 2026-07-21 時点)。

## 0. 目的
aChar = `LLVOAvatar::updateCharacter`(llvoavatar.cpp:5344・2.05ms/f・avatar の 66%)を **per-avatar 並列**化。管を 1 本(main)から増やす = Phase 2 の本命。

## 1. 現構造(トレース済)
- 中央 = `LLViewerObjectList::update` の object idle loop(llviewerobjectlist.cpp:1047-1061)= 全 object に `idleUpdate` を**直列**呼び。
- avatar の `idleUpdate`(llvoavatar.cpp:2849)→ 2997 で `updateCharacter(agent)`(2.05ms)。
- updateCharacter(5344-5444)の中身と owner = 監査表参照。要点:
  - **純 per-avatar(worker 安全)= updateMotions(5410)+ updateWorldMatrixChildren(5435)**(2.05ms 本体)。前段 updateRootPositionAndRotation(5394)は非 self では per-avatar(self は gAgent 書込)。
  - **main-only 副作用 = updateDebugText(5344)・updateFootstepSounds(5432)・updateHeadOffset(5429)・sit(5375/5383)・self の root(5394)**。← WorkerForbiddenGuard は debug/footstep に設置済。
- 既存 worker 基盤 = `LL::ThreadPool("General",3)`(llappviewer.cpp:2613)+ `LL::WorkQueue::getInstance("General")`。

## 2. 設計 = compute/publish 2 相分割 + pre-pass 並列

### 2.1 updateCharacter を 3 相に分割(prologue / 純 compute / 依存 publish)
- **prologue(main・パス1)** = debug/needs_update/overallApp/sit/velocity/**root 位置確定**(updateRootPositionAndRotation)。root は main 直列のまま(最小 race 面)。
- **`updateCharacterCompute()`(worker・パス2a)** = **updateMotions + updateWorldMatrixChildren のみ**(純 per-avatar 数学・副作用ゼロ)。root は含めない(prologue で確定済)。
- **依存 publish(main・パス2b)** = head/footstep(compute 出力依存)+ mNeedsSkin 等。
- **kill switch OFF 時 = 従来 updateCharacter そのまま**(prologue→compute→publish を 1 avatar 内で直列 = 等価フォールバック)。

### 2.2 ★2 パス post-pass(pre-pass は不可 = ordering 依存・2026-07-21 実装者トレースで訂正)
**⚠️ pre-pass(idle loop 前)は不可**: compute(updateWorldMatrixChildren)は mRoot world 位置に依存し、それは updateRootPositionAndRotation の入力 velocity = **LLViewerObject::idleUpdate のネット補間(直列 idle loop 内)**に依存。loop 前に compute すると骨格が 1 フレーム遅れる = 視覚同一 gate 違反 = 品質トレード = 却下。

**正しい構造 = prologue(main・既存直列)→ 純 compute(worker 並列 post-pass)→ 依存 publish(main・join 後)**:
- **パス1(既存 object idle loop・直列)**: 各 avatar の `LLViewerObject::idleUpdate`(補間)+ updateCharacter prologue(debug/needs_update/overallApp/sit/velocity/**root 位置確定**)。重い純 compute は per-avatar deferred フラグ(`mCharComputeFrame` 等)で積む。self/非visible/MT無効はここで inline 完遂(積まない)。
- **パス2a(post-pass・object idle loop の直後・worker 並列 fan-out → join)**:
```
LLVKLoader::parEpochBegin();   // arming wrapper(§申告)
// 積まれた eligible avatar を LLPointer pin(UAF)→ 各 updateMotions + updateWorldMatrixChildren
//   のみを worker へ scatter(worker 冒頭 markWorkerThread(true)+ DeadObjectGuard)
// atomic counter + condvar で gather(join・WorkQueue に join primitive 無し = 自前)
LLVKLoader::parEpochEnd();
// pin 解放
```
- **パス2b(main・join 後・直列)**: **compute 出力に依存する副作用を publish**。
  - ⚠️ 現コードは updateMotions(5401)→ head(5431)→ footstep(5434)→ skeleton(5437)順で **head/footstep が motion と skeleton の間に挟まる**(head=motion 出力依存・footstep=skeleton world 依存の公算)。∴ **head/footstep はパス2b(join 後 main)へ回す**。純 compute 2 つ(motion+skeleton)だけ worker・残り副作用は main。
- **self avatar は並列除外**(gAgent 依存)= パス1 で従来どおり compute+publish 全部 inline。

### 2.3 compute 済フラグ
`mCharComputeFrame`(U32・per-avatar)= compute した frame。updateCharacter は `mCharComputeFrame==現frame` なら compute skip して publish のみ。pre-pass 未処理(self/非visible/MT無効)は従来どおり両方 main。

## 3. 検出器の武装(守り)
- pre-pass worker 冒頭 = `LLVKContract::markWorkerThread(true)`(job 終了で false)。
- epoch = `parallelEpochBegin/End` で pre-pass を囲む。
- avatar = **`LLPointer` で pin**(dispatch..join 中に解体されない)+ compute 冒頭に `DeadObjectGuard(avatar->isDead())`。
- updateCharacterCompute が触る main-only(万一の副作用混入)= 既設 WorkerForbiddenGuard(debug/footstep)+ **必要なら updateRootPositionAndRotation の self 経路・sit 経路にも MainOnlyGuard 追加**(実装時に compute が触る範囲を再監査して設置)。
- **LLJoint::sNumUpdates/sNumTouches は atomic 化済**(D2)。

## 4. kill switch(工事足場・gate 後即削除)
- `AYASTORM_MT_THREADS=1` = 全 MT 直列化(既存)= pre-pass を立てず従来直列(epoch も立たない)。これを工事中の帰属切り分けに使う。
- 追加の一時 switch は最小限(pre-pass 有無)。gate PASS 後削除。

## 5. gate(fail-closed・憲法)
1. **視覚同一**(AYA 目視・アニメ/骨格が並列前と一致)。
2. **検出器沈黙**: 通常走行 + crowd stress で `C_PAR_*` 発火ゼロ(Layer B)。
3. **TSan(Layer A)**: USE_TSAN build で crowd stress → race/UAF ゼロ(prebuilt lib は suppression)。
4. VVL 0 + stack_trace 新規ゼロ。
5. **性能**: aChar 2.05ms/f が worker へ移り main の idl が減る実測(VkPerf idl 欄 slot 28)。
- PASS は自称不可。検出器沈黙 + 正のオラクル(性能移動 + 視覚同一)で。

## 6. 罠(実装時に必ず対処)
- **updateRootPositionAndRotation の非 self 純度**: camera 相対計算で gAgentCamera を読む可能性 = compute に入れる前に実トレース(read-only なら worker 可・write なら publish へ)。
- **updateMotions の leaf**(LLMotion::onUpdate 各種)= keyframe/等の onUpdate が static/shared を書かないか = TSan で炙る(手動監査の限界)。
- **updateWorldMatrixChildren の親子**: 各 avatar は独立 skeleton だが、attachment/animesh の joint 共有がないか確認(通常は per-avatar)。
- **idle_list の avatar 収集**: isDead/isSelf/isVisible/needs_update の判定は **main で pre-pass 前に**行い、worker には確定リストを渡す。
- **WorkQueue の join 待ち**: main が pre-pass の join でブロック = その間 main は他仕事をしない(または後続の非 avatar idle と重ねる設計は v2)。v1 は素直に join。
- **General pool は他用途と共有**(texture/geo worker と競合)= avatar 専用 lane にするか General 相乗りか実測で判断。

## 7. 増分計画(小さく始める)
- **v1(2 パス post-pass)**: パス1(既存直列 loop で prologue+root+補間・compute を defer)→ パス2a(post-loop で updateMotions+updateWorldMatrixChildren を worker fan-out→join)→ パス2b(head/footstep publish)。root は main 直列(最小 race 面)。self/非visible/MT無効はパス1 で inline。
- **v2**(任意): 配当と TSan 結果を見て compute 相を広げる or pool を専用 lane 化。
- 各 v = gate(§5)通過を確認してから次。**arming = `LLVKLoader::parEpochBegin/End` + `markWorkerThread` wrapper**(newview は llvkcontract.h 非 include・parWorkerForbiddenCheck と同型・§8 申告)。

## 8. 申告(designer)
- **縮小**: self avatar は並列対象外(gAgent 依存)= 恒久受容(1 体だけ main)。
- **未決(実装時トレース)**: updateRootPositionAndRotation 非 self 純度 / updateMotions leaf の shared / General pool 相乗り可否。→ **手動監査でなく TSan(Layer A)で炙るのが設計の眼目**(だから検出器が入場ゲート)。
- product 分岐なし(内部並列)。視覚同一が最終 gate。
