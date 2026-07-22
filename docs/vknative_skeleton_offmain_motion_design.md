# self avatar skeleton off-main — 本質設計(引き継ぎ用・ソース導出)

> **✅ 実装完遂(2026-07-22・HEAD `42d5fb9531`)= M1 `a3021d8135` / M2 `9c15d8c2e4` / M3+構造fix `5bba495a06`。** self motion compute(updateMotions)を avatar domain worker へ async 1-frame dispatch。§5「motion controller lifecycle race」は予言的中 = gate で jemalloc heap-corruption crash 発火 → **§7 の deferred(M1/M3b の INV-M)だけでは不足**(start/stop しか守れず flushAllMotions 等が無保護 + worker の LLJointState 非atomic refcount off-main)→ **構造 fix = controller は compute window 中 single-owner**(`std::recursive_mutex mComputeMutex` を worker compute + main container mutator が排他)で根治。⚠️ **§7 が gate に据えた TSan(設計 §6 Layer A)は boost.fiber 非互換で実行不能と判明**(memory `finding_tsan_layerA_blocked_boost_fiber`)→ jemalloc heap-corruption crash を正の race オラクルに使用(probabilistic)。gate 実績 = appearance 662 件で新規 crash 0・race 検出器 0・視覚/アニメ同一(AYA)。本 doc は設計理解の資料として保持。次 = palette off-main(memory `handoff_avatar_palette_offmain`)。

- 状態: ✅ 実装完遂(上記バナー)。本質設計 2026-07-22 深夜。真実源 = HEAD の実行コード。**引き継ぐ実装者向け**に、分解可能性の**ソース導出**・境界・等価保証・**迷走防止(やってはいけない道)**を残す。
- 位置づけ: `docs/vknative_avatar_relocate_design.md` §5 Phase 1 の skeleton 相。入口 `docs/vknative_avatar_skeleton_offmain_design.md` の §2 仮説(出力=joint を snapshot し readers を redirect)を **全面 supersede**。
- 実測資産: aChar/updateCharacter ≈ 2.05ms/f(idl28)が self skeleton の main 費目(measurements doc)。

---

## 0. ⛔ 迷走防止 = やってはいけない道(このセッションで実際に払った授業料・削るな)

引き継ぐ人へ。**以下の 5 つは全て一度やって壁に当たり revert した。同じ轍を踏むな。**

1. **⛔「joint(出力)を snapshot して readers を redirect する」= 間違った層。** これをやると skinning/camera/attachment/HUD/nametag の**全 reader を追う一覧**が生まれ、部分 redirect で内部 desync し、しかも**計算は main に残ったまま = 何も off-main にならない**。**off-main にするのは出力(joint)でなく入力(motion 計算)。readers は live joint を読むだけ = redirect 不要**(§1）。
2. **⛔「reader 一覧」を作った時点でカメラが混入する。** カメラは reader だが、**skeleton content でなく配置(A)の消費者**。一覧を作ると機械的に遅延側へ引きずられ矛盾する。**一覧を作らない設計(入力を off-main)にすればカメラは最初から scope 外**（§6）。
3. **⛔ カメラを off-main に持っていかない。** 視点は入力→表示のリアルタイム系。**入力→視点の遅延だけは払えない通貨**。カメラは Main の聖域（§6）。
4. **⛔ skeleton を「安全のため」main に残さない。** crowd 30-100 体分の skeleton が main に残ったら目的(main を空ける)が死ぬ。**skeleton は全員 off-main**（§1・§5）。
5. **⛔ 1-frame latency の縁を過剰分析しない。** 1-frame は thread 化の受容済み代償。**pay' 済み**。唯一 live 必須なのは「自分の移動→視点」= 配置(A)で、それは構造的に live に残る（§1）。「決定的な小計測で設計を潰せるか」に逃げない。

---

## 1. 設計の核(ソース導出・一行)

**現コードは「源が独立なのに 1 本の main pass で融合し共有 live joint に書く」。分離は源に在り計算に無い。境界を計算に作るだけ。作り直し不要。**

- **(A) 配置 = 視点 anchor**: drawable/agent/velocity 由来。**pose 非依存・軽い・live/main 残置**。
- **(B) articulation = skeleton content**: motion 由来・root 相対 pose。**重い・off-main**。
- **融合 = re-root**: `updateWorldMatrixChildren` が mRoot(A・live)× child 局所(B・off-main)→ world。**main 残置**。
- **カメラ = A の消費者**(自前状態を持ち A を読むだけ・joint に融合していない)= **live A を読んで無傷**。

全アバター skeleton(B)off-main。カメラが anchor するのは A(配置)で、self の A だけが「自分の移動」ゆえ live 必須(他体の A は観察=1-frame 可)。

## 2. ソース導出(file:line・分解可能性の根拠)

### (A) 配置は pose 非依存 — `updateRootPositionAndRotation`(llvoavatar.cpp:5187)
- mRoot world 位置 = `gAgent.getPosGlobalFromAgent(getRenderPosition())`(:5283/5296)。`getRenderPosition()` = `mDrawable->getPositionAgent()` + pelvis-fixup(skin 由来・静的)(llvoavatar.cpp:1274）+ hover/bodysize/ground 補正。**object/network 駆動。motion 出力を読まない。**
- mRoot 回転 = `updateOrientation`(:5304)= `getVelocity()` + `agent.getAtAxis()`（movement/agent 駆動・updateOrientation:20/34）。
- sit 時 = mRoot は mDrawable に slaved(:5310-5314）。
- ⟹ **A は B なしで計算可能**。読むのは drawable/agent/velocity + 静的 body 寸法のみ。

### (B) articulation は root 相対 — `updateMotions`(:5412)
- `LLMotionController::updateMotions`(llmotioncontroller.cpp:821) → `updateMotionsByType`(:562) → 各 motion `onUpdate` → `LLPoseBlender`/`LLJointStateBlender::blendJointStates`（llpose.cpp:238) → `target_joint->setPosition/setScale/setRotation`(:388-390) = **child joint の局所(親相対)pos/rot**。
- 出力が**局所**である事実が核心 = main が live root で re-root 可能。

### 融合点 — `updateWorldMatrixChildren`(:5437)
- mRoot（A）を根に child 局所（B）を world へ伝播。child 局所が main 産か off-main 産かに**依らず同一結果**。

### カメラは A の消費者・自前状態あり
- `LLViewerCamera`（origin/axes）+ `LLAgentCamera`（mCameraMode 等）= **自前の視点状態**。毎フレーム A（`getAvatarRootPosition()` = `mRoot->getWorldPosition()` llagentcamera.cpp:2374/1961）を**入力として読み**、自状態に格納。joint に埋まっていない。
- **RLV `@setcam` 族**（rlvhandler.cpp:896・setcam_eyeoffset/focusoffset/avdistmin）= 視点位置/焦点を self と独立にパラメタ駆動する既存機構 = **「カメラを self と切る」足場が実在**。

## 3. 等価性保証(reference = 現コード)

off-main が **同じ入力 snapshot から現 updateMotions と同じ child 局所変換**を出し、main が同じ root で `updateWorldMatrixChildren` すれば、world 出力は **byte 等価（1-frame ずれのみ）**。融合点は child 局所の産地に依らず同一 ⟹ 分割は挙動等価。正のオラクル = GEOAB（幾何 A/B）kernel 0 + 視覚同一 + アニメ挙動同一。

## 4. 作るべき境界 = updateMotions(B)を main pass から切り出す 1 点

源が独立ゆえ、計算の融合を **「A(+融合)= main / B = off-main」** に割るだけ。**新しい境界の実体はここ 1 本**:
- **入力 handoff(main→off-main)** = 稼働 motion 集合 + mAnimTime 基点 + delta + pixel area + mPaused/mTimeFactor（immutable snapshot）。
- **出力 handoff(off-main→main)** = per-joint 局所 pos/rot（back-buffer）+ deferred lifecycle 事象（§5）。
- **main sync** = deferred 適用 + live root（updateRootPositionAndRotation）+ back-buffer を live joint へ適用 + updateWorldMatrixChildren。

## 5. B 内部の壁 = motion controller の thread-safety(実装の本丸)

境界は clean だが、**B（updateMotions）内部は純関数でない**。ここが B.2 の move に相当する貫通対象。

**壁の全数(llmotioncontroller.cpp)**:
- `updateMotionsByType`(:562) が `onUpdate`（純計算）と状態変更を interleave:
  - lifecycle mutate: `deactivateMotionInstance`（:1000・deprecated なら **motion を delete** = removeMotionInstance）/ `stopMotionInstance`（:452）。
  - character callback: `mCharacter->requestStopMotion`（:632/713/759）。
  - loop 前 lifecycle: `purgeExcessMotions`/`updateLoadingMotions`（:837/862・init+activate+delete）。
- **main も並行 mutate**: `startMotion`（:402）→ deprecate/create/`activateMotionInstance`（:935）。`stopMotionLocally`→`stopMotionInstance`。
- ⟹ off-main で回すと mActiveMotions/mAllMotions/mLoadingMotions の並行 mutate + motion delete で **UAF/race**。

**解 = 状態変更を全て main に寄せる（B.2 と同型）**:
> off-main は pose 計算だけ。lifecycle 変更（deactivate/stop/delete/load）と callback（requestStopMotion）は**その場で実行せず deferred list に record**。main が sync 点で適用。⟹ **全状態変更が main = single-thread = race 不能 by construction**。off-main は back-buffer に書くだけ・live joint と共有 list に触れない。
- 入力 snapshot（稼働 motion 集合）中、main は当該 avatar の motion lifecycle を mutate しない（INV-M）= 既存 volume-pin（sGeoVolumePins llvovolume.cpp:5841）の avatar 版拡張で保証。
- motion instance の playback 前進（onUpdate）は窓中 off-main 単独所有 = single-writer。

## 6. カメラの扱い(Main 聖域・A の消費者)

- カメラは Main に残す。A（配置）を live で読む → **通常操作（follow-cam・3rd person）で無傷**（follow-cam は mRoot=A のみ読む・毎フレーム）。
- **pose 依存でカメラが skeleton を読む経路**（1-frame 古い pose を見るが latency-tolerant なもの）: mouselook 一人称の眼位置（:1636）/ focus・lookAt の対象頭（他体含む・lookAtLastChat llagentcamera.cpp:3203）/ RLV 距離クランプ（:2232）。観察系ゆえ 1-frame 許容（AYA 受容）。
- **唯一の真の縁 = mouselook 一人称ボディ表示**: 視点が頭に座る → 眼位置が pose ぶん 1-frame 泳ぐ。頭の frame 間 pose 移動は小 ⟹ 恐らく不可視だが**実機で確認 or この mode だけ pose live**。
- **カメラ→skeleton 逆流 = mouselook pelvis 書き戻し**（llagentcamera.cpp:1665-1667・updateCharacter の後に camera が pelvis を書き updateWorldMatrixChildren 再実行）= RLV-eye 型の面倒な縁。**本線を汚すなら別設計に切り出してよい**（AYA 承認）。

## 7. 実装計画(直列・各 gate・B.2 の 2 段階安全化)

- **M1（挙動不変・main 上）= 相分割**: updateMotions を「lifecycle 前処理 / 純計算 / deferred 適用」に分割。deactivate/stop/requestStopMotion を deferred list へ積み loop 後 main で適用。joint は従来直書き。gate = 視覚同一 + アニメ挙動同一（歩行/表情/停止/フェード）+ validation 0。**deferred 化の挙動不変を off-main 前に証明**。
- **M2（挙動不変）= back-buffer 経路**: blendAndApply の出力先を live joint 直書き → back-buffer 局所 pos/rot。main sync で live joint へ適用。まだ main 連続。gate = 視覚同一（back-buffer 経由で同一 pose）+ GEOAB kernel 0。
- **M3 = 入力 snapshot + off-main dispatch**: 窓開始で MotionInput snapshot。avatar domain thread（`geoWorkerMain` パターン llvovolume.cpp:5785 が雛形 / B.2 の sAvatarJobQueue 機構再利用）で純計算を off-main。INV-M pin。self 1 体。
- **M4 = 1-frame 化 + gate**: production(N)→ apply(N+1)。gate = self+attachment 視覚同一（手足 1-frame・不可視）+ **camera 位置追従が遅れない（live A）** + アニメ同一 + validation 0 + 装置全層沈黙 + C_*_RACE 0 + **main の self idl28 費目が退避（直接計測・捏造不能）** + kill-switch `AYASTORM_MT_THREADS=1` で inline 退化 A/B。

各 gate: cold launch = AYA・**PASS は宣言しない**（憲法1）。commit = AYA gate 後・trailer なし。

## 8. 設計不変条件
- **INV-boundary**: off-main = B（motion 計算 → child 局所変換）のみ。A（配置）と融合（updateWorldMatrixChildren）は main。
- **INV-camera（聖域）**: カメラ・配置（自分の移動→視点）は live/main。off-main は content（pose）に限る。
- **INV-M（production 窓排他）**: 窓中、main は当該 avatar の motion lifecycle（start/stop/add/remove）を mutate しない。network/UI 由来は sync-in 境界で drain。per-domain race probe で検出。
- **INV-defer（状態変更は main）**: off-main は lifecycle/delete/callback を実行しない（record のみ）。全 mutate が main = race 不能。
- **INV-1frame**: pose/skinning は 1-frame 可。**着脱・topology 変化は例外**（即時 = B.2 の membership 二層）。
- **INV-equiv**: 現コードが reference。GEOAB kernel 0 + 視覚同一 + アニメ同一で等価を証明。

## 9. 複雑点 / OPEN(境界の外側で個別に潰す)
1. **updateOrientation が pelvis(child)も書く疑い**（movement-facing 腰回転・pelvis_rot_threshold updateOrientation:72）= A 相に居る「配置駆動の pose 断片」。A に残す（配置の一部）か B へ移すか要トレース確定。**M1 前に updateOrientation 全読**。
2. **mouselook pelvis 書き戻し**（llagentcamera.cpp:1665）= §6 の縁。別設計に逃がし可。
3. **motion controller lifecycle**（§5）= B 内部の壁。deferred-mutation で解く。境界とは別レイヤ。
4. **INV-M の pin 対象** = `startMotion` 全呼び元の実トレース（grep でなく）。M3 前。
5. **quantum 経路（mTimeStep!=0）は現状 disabled**（llmotioncontroller.cpp:824-827）ゆえ非対象。有効化時は別途。
6. **body 寸法（mBodySize/mPelvisToFoot）** = A が読むが appearance-change 時のみ更新（per-frame pose でない）= 静的入力として snapshot 可。
7. **着脱/topology 変化の 1-frame 例外契約**（B.2 の membership 二層を skeleton へ）。

## 10. 申告(縮小・省略・解釈)
- **motion load/init/delete（updateLoadingMotions/purgeExcessMotions）は main 残置** = 縮小でない（lifecycle は本来 main・重い decode を off-main pose 計算と混ぜない）。off-main に出るのは「確定 active 集合の pose 計算」。
- **back-buffer 化で LLPose/LLPoseBlender 改修**（apply 先の抽象化）= llcharacter 層改修（検出器でない = 憲法4 対象外）。
- **カメラの pose 依存読み（mouselook/focus/RLV/lookAt）は 1-frame content 扱い**（AYA 受容）= 縮小でなく doctrine 適用。唯一の縁（mouselook 一人称ボディ）は §6/§9-2。
- **本 doc は設計地図**。動作証明は §7 各 gate の実測。PASS は宣言しない。
- crowd（非 self）への一般化は Phase 2（本 doc は self 1 体 = Phase 1）。
