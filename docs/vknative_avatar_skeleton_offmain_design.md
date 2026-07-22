# avatar skeleton/joint 機構の off-main 化 — 設計の入口(Phase 1 self 完遂の本丸)

> **🔀 SUPERSEDED(2026-07-22 深夜)= 本 doc の §2 技術仮説(joint world matrix=出力を snapshot し readers を redirect / double-buffer)は誤りと判明・全面破棄。実装して壁に当たり revert 済み。正しい本質設計 = `docs/vknative_skeleton_offmain_motion_design.md`(ソース導出・A/B 分離・出力でなく入力=motion 計算を off-main・カメラは配置(A)の消費者として Main 聖域)。本 doc は §0 の「意識(実装先行・退却禁止・矮小化禁止)」as 教訓 + 経緯としてのみ保持。技術方針は新 doc が唯一。**
>
> **誤りの要点(後進が繰り返さぬよう)**: 「skeleton の出力(joint)を snapshot して reader を redirect」は (a) 全 reader(skinning/camera/attachment/HUD/nametag)を追う一覧を生み (b) カメラを機械的に遅延側へ巻き込み矛盾 (c) 計算は main に残り何も off-main にならない。正しくは**入力(updateMotions)を off-main・main は live root で re-root・readers は live joint を読むだけ(redirect 不要)**。詳細 = 新 doc。

- 状態: **入口設計(entrance)= 技術仮説は SUPERSEDED**。§0 意識のみ有効。真実源 = HEAD の実行コード。
- 位置づけ: `docs/vknative_avatar_relocate_design.md` §5 Phase 1 P1-c の skeleton 相。**本質設計は `docs/vknative_skeleton_offmain_motion_design.md`**。**Phase 1 = self avatar 1 体の skeleton を off-main にできるかの証明**(速度でなく「位置」)。B.2(geometry apply)は完了・これは残る最難関。

---

## 0. 後進へ最初に伝える意識(これが本 doc の核・削るな)

**この機構は「詳細設計を先に完璧にする」ことが原理的に不可能。実装して壁にぶつかって初めて設計ミスに気づく類の代物。** 前任も B.2 で同じ轍を踏んだ:

- B.2 で「apply を off-main」の素朴設計(snapshot の LLPointer を off-main で copy)を実装 → 監査で **非 atomic refcount の data race = UB** と判明(壁)。
- そこで前任は一度 **「materialize は main 固定」= 分散化の矮小化**に退避しかけた → AYA に「設計退行・矮小化」と却下された。
- 正解は退避でなく **move(refcount no-op で off-main)= 壁を貫通する再設計**だった。

**⛔ 後進への厳命(解雇判例 [[handoff_designer_dismissed_step0_belittlement]] の教訓)**:
1. **まず実装を入れろ**。詳細設計から始めるな(荷が重い・かつ机上で壁は見えない)。小さく off-main を通し、壁(race/desync/読み手の front 参照漏れ)を実機と監査で炙り出せ。
2. **壁にぶつかったら、ひるまず再設計しろ**。壁は設計が間違っていた証拠であって、撤退の理由ではない。
3. **⛔ 壁を「難しい部分を main に残す」で回避するな = それが矮小化 = このプロジェクトで最も嫌われる敗北**。「共有だから main」「重いから main」は B.2 で却下された退避と同じ。共有・非 atomic・pervasive な壁こそ、B.2 の move のような **構造的貫通解**を探せ。off-main の目的は塊を丸ごと外へ出すこと。一部を main に残したら目的の放棄。
4. あなたの入口設計(下記)が間違っていても構わない。**間違いに気づいたら入口ごと再設計しろ**。前任の入口を「stale doc は読み手が刈る」で正すのは歓迎(ただし監査でなく実装で正せ)。

**doctrine**: off-main では parallelize 解禁([[project_vk_doctrine_eliminate_not_parallelize]])。通貨 = 1-frame latency(pose 1 コマ遅れ = 不可視)。

## 1. なぜ最難関か(readiness = HEAD 実トレース済・updateCharacter :5344)

geometry apply(B.2)には **clean な snapshot 境界**があった(main capture → off-main build → main fold)。skeleton には**それがない** = 出力(joint world matrix)を main の多経路が pervasive に読む。

`LLVOAvatar::updateCharacter(LLAgent&)`(llvoavatar.cpp:5344)が触る main-所有 state:

| 処理(file:line) | shared state | 壁 |
|---|---|---|
| `updateRootPositionAndRotation(agent…)`(:5396) | **gAgent**(self 位置/回転の権威・main) | self の root は agent 依存 |
| `updateMotions(NORMAL_UPDATE)`(:5412) | **mMotionController**(main が anim register/deregister・ANIM_* trigger) | 稼働 motion 集合が main と共有・self は controller 処理を中断できない(:5401 コメント) |
| `mRoot->updateWorldMatrixChildren()`(:5437) | **joint 階層 world matrix = 骨格出力** | ★核心。下記「読み手」参照 |
| sitOnObject/getOffObject(:5375-5387) | 着座 state・parent object | main mutate |
| updateOverallAppearance(:5370) | visual params・texture・bake | main と共有 |
| updateHeadOffset(:5431)/footstep(:5434)/debug(:5346) | eye joint / audio / UI | 副作用(main 前提) |
| `mNeedsSkin = true`(:5442) | render への skin 要求 flag | seam |

### ★ 核心の壁 = joint world matrix の「読み手」(実装前に全数列挙せよ)
skeleton 出力(mRoot 階層の world matrix)を main が読む経路 = off-main 化で **全て front-buffer 参照に変える必要がある**。実装者の最初の仕事 = **この読み手の全数列挙**(grep でなく実トレース = [[handoff_vk_deadcode_trace_method]] の教訓):
- avatar mesh の skinning(palette build = initSkinningMatrixPalette が joint を読む・render 経路)
- **attachment の追従**(attachment は joint に親子付け = joint world matrix を読んで位置決め)
- self camera(視点が head/eye joint 依存)
- HUD attachment・name tag・LOD 判定 等
- ⚠️ 「戻った者の速さでなく戻らなかった者を数えろ」(装置 doctrine)= 読み手を1つ漏らすと desync(attachment がズレる/消える)。

## 2. 方向(入口の仮説・間違っていたら再設計せよ)

B.2 の pattern を skeleton へ拡張する仮説:
- **入力 snapshot(production 窓開始・main)**: gAgent 位置/回転 + 稼働 motion 集合 + 時間 step を immutable 化(INV-5 = 窓中 main は self の該当 state を mutate しない・既存 sGeoVolumePins 機構の拡張)。
- **off-main compute(avatar domain thread)**: updateMotions + updateWorldMatrixChildren を snapshot 入力から回し、joint world matrix を **back buffer** に書く。palette build も同 thread で back joint → back mGLMp。
- **出力 double-buffer(最大の新規実装)**: joint 階層の world matrix を front/back 化。main の全読み手(§1 核心の列挙)を front 参照に。swap は Update-Geom 点(既存 drainAvatarPublished の隣・render 前の安全窓)。
- **1-frame 契約**: pose/skinning は遅延可。**着脱・topology 変化は例外**(即時 = detach ghost 封じ・B.2 と同じ INV-3)。

### 想定される壁(前任の予言・実装で確かめよ)
1. **joint double-buffer の規模**: mRoot 階層は数十〜百 joint。全 world matrix の double-buffer + 全読み手の front 化 = attachment/skinning/camera の広範改修。ここで「大きすぎるから main 残し」に退避したくなる = ⛔ §0-3。
2. **gAgent 依存**: self の root は agent 権威。snapshot で 1-frame 遅らせると self 視点が 1 コマ遅れる = 自分のカメラの遅延は「不可視」でない可能性(要 AYA product 判断)。← ここは真の product 分岐かもしれない(相談事項)。
3. **motion controller の thread 安全性**: mMotionController は main 前提。off-main で updateMotions を回すと motion register(main)と race。snapshot で稼働集合を固めても、controller 内部状態(motion の再生位置等)の所有が問題。
4. **B.2 と同じ refcount 罠**: joint/motion が LLPointer で共有オブジェクトを持つなら、off-main copy は同じ非 atomic race。move / snapshot で貫通(B.2 = `docs/vknative_apply_offmain_snapshot_design.md` の move-materialize が template)。

## 3. 後進の最初の一歩(詳細設計でなく実装から)

**Step A(実装・小さく壁を見る)= joint 読み手の front/back 分離の足場を1つ作る。** 推奨着手 = 最も孤立した読み手 1 経路(例: palette build = initSkinningMatrixPalette が joint を読む点)を「back joint を読む」に変え、self の joint を試験的に double-buffer 化(1 joint でなく mRoot 階層まるごとの shadow copy を production 窓で作る)。self 1 体で回し、**desync(attachment ズレ・skinning 化け)を実機と VKC L2 連続性オラクルで炙り出す**。壁が見えたら §2 の仮説を捨てて再設計。

**着手前 readiness(必須・CLAUDE.md 着手プロトコル)**: ①本 doc + relocate 設計書 + B.2 設計書(move template)+ 解雇判例を読了列挙 ②updateCharacter/updateWorldMatrixChildren/attachment 追従/palette の実読 file:line ③§1 核心「joint 読み手の全数列挙」を先に出す(これが無いと desync 必至)。

## 4. gate(命題様式・B.2 と同型)
- self avatar + attachment が視覚同一(desync ゼロ = attachment 追従・skinning・camera)+ validation 0 + 装置全層沈黙(VKC L2 連続性 = 消える attachment を数える)+ C_*_RACE 0 + main の self skeleton/palette 費目(idl28 + palette)が退避(直接計測・捏造不能)。
- **PASS は宣言しない**(憲法1)。正のオラクル = 上記実測。

## 5. 申告(前任の限界・正直に)
- 本 doc は **入口**であって設計解ではない。§2 の方向は仮説・§3 で壊れる想定。
- joint double-buffer の実装コスト・gAgent 1-frame 遅延の product 可否は未確定(§2 壁2 = 相談事項)。
- 前任は skeleton の実装に一歩も入っていない(B.2 で context を使い切った)= **この doc は机上の地図・実装者が実機で描き直せ**。
- 上位 = `docs/vknative_avatar_relocate_design.md` / B.2 template = `docs/vknative_apply_offmain_snapshot_design.md` / 意識 = 本 doc §0。
