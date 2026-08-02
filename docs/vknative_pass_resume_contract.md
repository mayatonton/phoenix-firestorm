# AYAstorm VK-native — pass interrupt/resume 契約(F2 foundation・確定 2026-08-02)

> dynamic-rendering pass の中断/再開を支配する **load-bearing 契約**。resume 経路・layered RT・aux window・cmd buffer 切替を触る前に必読。この契約は「GL 移植の後付け resume が未設計のまま 3 つの状態源に割れていた」欠陥(影 multiview 工事の R4 F-1 監査で噴出)を根治した確定設計。

## 契約(正本)

**「pass 記述は begin の副作用として単一 canonical mirror(`sSaved*`)に原子的に記録される。suspend(end)は記述を保存し、resume は記述の replay である。いかなる経路も静的構成からの逆算をしない。cmd buffer 切替は記述の完全 save/restore を伴う。replay する pass は resume する RT に属さねばならない(membership)。」**

## 構造(なぜこれが成立するか・規範的根拠)

- **begin の扉は 2 つだけ**(grep 全数・第 3 経路なし):
  1. `beginDynamicRendering`(llvkloader.cpp)— vkCmdBeginRendering の直前に `sSaved*` を書く = **記述と実 begin が同一関数内 = 乖離不能(原子性)**。既に pass 中なら暗黙 end(1 cmd に active pass は常に高々 1・nest なし)。
  2. `resumeSavedPass`(llvkloader.cpp)— `sSaved*` を loadOp=LOAD で replay。
- **`sSaved*`(thread_local)= 現 active pass の唯一の記述** = {ColorInfos[4], ColorCount, DepthInfo, HasDepth, RenderWidth/Height, **ViewMask, LayerCount**}。読者 = transition 内 auto-resume(path①)/ consumer resume(path②)/ aux save-restore / per-draw memo pass 署名 / active-attachment guard / multiview accessor(currentRenderViewMask/currentRenderDepthView)。
- **per-RT の第 2 記述は作らない**: writer が 2 つになると pass 属性(viewMask の次は shading rate 等)が増える度に二重更新 = 将来荷重に比例して乖離リスクが増える。RT メンバは「静的構成」であって「今どの sub-shape で bound か」を持たない — そこから resume を**逆算するのが旧欠陥**(layered RT で単層 bound を array/0xF に誤再開 = F-1)。

## resume の 2 経路(両方 sSaved\* replay に統合済)

| 経路 | 契機 | 実装 |
|---|---|---|
| path① | `transitionImageLayoutVk` が pass 中の transition を検出 → end → barrier → auto-resume | `resumeSavedPass()` 呼出(llvkloader.cpp transition 末尾) |
| path② | consumer の明示 resume(texture upload / mip gen / texlayer / bump / terrain / probe / draw 復旧) | `LLRenderTarget::resumeVkDynamicRendering` = **membership 検査 → `resumeSavedPass()`**(RT 逆算は削除済) |

- **membership 検査(`ownsSavedPass()`・llrendertarget.cpp)**: replay する sSaved\* の view 群が this の **{mVkTexView[], mVkDepthView, mVkDepthArrayView, mVkDepthLayerViews[]}** に属するか。不一致 = WARNS + **skip(begin しない)= fail-closed**。RT の静的 view 集合を「検証」に読むだけで pass state を「再構成」しない = 単一 source を破らない。Release build は llassert が no-op のため、この検査が degenerate 経路(begin 失敗後の bound RT に対する誤 replay)の唯一の歯止め。
- **draw 復旧 site**(beginShaderDrawOrSkip)は resume 後に `isInRenderPassScope()` を再確認し、begin されなかった draw は `C_CMD_NULL` で skip(pass 外 record を構造的に排除)。

## aux window(cmd buffer 切替)

aux UI frame は別 cmd への切替時に `sSaved*` を**完全に**(ViewMask/LayerCount 込み)save し、復帰 2 経路(begin 失敗巻戻し / end)双方で完全 restore する。multiview pass を aux が中断しても viewMask は保存される。

## 契約の適用範囲外(既知・意図的)

- **flush の再 bind**(`mPreviousRT->bindTarget()`)は resume でなく**新 begin**。bind stack は RT ポインタのみ保持し bind 時形状(単層/array)を持たない → **layered RT を nested bind(mPreviousRT 化)する設計は現状存在しない**(shadow render call-tree に bindTarget/end 出現ゼロ)。将来 layered RT を nest する設計が現れたら bind stack に形状を積む(その時の設計事項)。membership 検査が違反を observable 化する。
- swapchain は RT でなく begin 専用経路(path② 非経由)。

## 教訓(この契約の来歴)

- 前任は候補 A(global 一元化)/ 候補 B(per-RT 自己記述)の間で往復した。原因はトレース不足でなく、**契約未宣言の codebase では双方に「半分そうなっている」証拠が実在し、トレースが裁定しない**こと。**未設計のものは発掘できない** — 将来の荷重(MDI・multiview・layered)に耐える契約を先に宣言し、コードをそれに従わせた。A はその帰結(begin 原子性)であって多数決ではない。
