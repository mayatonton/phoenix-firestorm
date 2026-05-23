# r30 P5 透過 DoF (C-(a)) Bug 調査 handoff

**Status**: 調査中 — fresh session 引継ぎ用
**Date**: 2026-05-23 (session compact → clear)
**Investigator**: 前 session の Claude (context 飽和で判断精度低下 → clear 判断)

## 2 つの bug

**Bug A: HUD LMB 押下中 alpha BLEND 表示崩れ**
- HUD を LMB クリック「している間」だけ症状、離すと戻る (**transient**)
- 当初「消失」と報告 → screenshot 観測で実態は「C-(a) plate 合成 path 全体が bypass されている」可能性
- screenshot 2 (押下中) は magenta canary も消えていた = aya_alpha_plate_enabled が false / dof path 全体スキップ

**Bug X: alpha BLEND surface が常時暗い (色が濃い)**
- アバターの服 / 髪 / particle で AYA が肉眼検知
- HUD 押下とは独立、起動直後から発生
- C-(a) 完成 commit `626d97cbc3` (2026-05-22) 時点で **既に発生** (bisect 確認済)

両方 **C-(a) 透過 DoF 自作機能由来**。BD 移植部由来ではない。

## 関連 commit

```
626d97cbc3  r30 P5 透過 DoF C 案 完成  ← bug が乗っている commit
ed429b5632  r30 BD改善 phase: AYAstorm View r14-r20 を Cinematic mode に opt-in
a05999bc29  r30 Cinematic Cleanup C.1-C.3+D.1
```

調査時の working branch: `tmp/r30-cinematic-first` (detached HEAD at `626d97cbc3`)

## 該当コード

### `indra/newview/lldrawpoolalpha.cpp` `renderPostDeferred()`
- `mAYAAlphaColor` RT redirect (POST_WATER のみ、8-AND gate)
- gate: `!sImpostorRender && !sRenderingHUDs && !gCubeSnapshot && RenderDepthOfField && POOL_ALPHA_POST_WATER && mRT==mMainRT && mAYAAlphaColor.isComplete()`
- blend factor: `mForwardToAlphaRT` true 時 color=(SRC_ALPHA, 1-Sa) + alpha=(ONE, 1-Sa) → premultiplied 出力
- clear: `glClearColor(0.f, 0.f, 0.f, 0.f)` (premul 不変条件)

### `indra/newview/app_settings/shaders/class1/deferred/dofCombineF.glsl`
- `aya_alpha_plate_enabled` で gate
- 12-tap disc gather (CoC ≥ 0.75 px)
- composite: `frag_color.rgb = plate.rgb + frag_color.rgb * (1.0 - plate.a)` (premul over)

### `indra/newview/pipeline.cpp`
- `mAYAAlphaColor` allocate (深度は `mMainRT->deferredScreen` と共有)
- `aya_alpha_plate_enabled` uniform 設定
- `mAYAAlphaDepth` L2-β (cutoff 0.5 で alpha z 再注入)

## Bug A 調査履歴

### H1: `mForwardToAlphaRT` flag stale
**仮説**: HUD pick render が bindTarget/flush の間で割り込み → flag が true のまま残る → 次 frame で premul blend が mRT->screen に対して適用される
**対応**: `renderPostDeferred()` 先頭で `mForwardToAlphaRT = false` リセット (M1 fix)
**結果**: AYA 実機検証「消失再現です」→ **falsified**

### H2-H4: log trace で全 path 検証
**手法**: `LL_INFOS("AYAHUDBug")` で entry/exit に seq counter + 全 state を出力
**データ**: 1905 frame 採取、`mForwardToAlphaRT_in=0` 常に保持、state 遷移 4 種のみ、異常無し
**結論**: state-level の異常無し → C++ 側の flag/RT 系仮説は全 falsified

### Screenshot 観測 (final canary)
**手法**: `mAYAAlphaColor` clear color を `(1,0,1,0)` magenta に変更 (premul 不変条件を意図的に破る canary)
- `/home/ishikawa/ピクチャ/Screenshots/Screenshot from 2026-05-23 23-51-04.png` (押下前)
- `/home/ishikawa/ピクチャ/Screenshots/Screenshot from 2026-05-23 23-51-16.png` (押下中)

**観測**:
- 押下前: 画面全体が magenta tint。前景の avatar/HUD 一部が tint されず残る
- 押下中: magenta tint **完全消失**、scene 正常表示
- → composite 式 `plate.rgb + frag*(1-plate.a)` で plate.rgb=(1,0,1), plate.a=0 だと `(1,0,1) + scene` で magenta が opaque 領域に乗る **これは canary 期待動作**
- → 押下中 magenta が消える = aya_alpha_plate_enabled が false になっている / dof 合成 path 自体が skip されている **これが新発見**

## Bug X 調査履歴

### bisect step 1
**手法**: C-(a) 完成 commit `626d97cbc3` を canary 付きで実機検証
**結果**: AYA「もう AlphaBlend 色おかしいね アバター読み込むときのパーティクルの色でもうわかる」→ **C-(a) 完成時点で既に発生**

→ C-(a) 自体に構造的欠陥 (composite ambiguity / premul 仮定の破綻 / blend factor mismatch)

### 数学的分析 (未検証仮説)
forward alpha blend → `mAYAAlphaColor` clear=(0,0,0,0):
- color: `out.rgb = src.rgb * src.a + dst.rgb * (1-src.a)`
- alpha: `out.a = src.a * 1 + dst.a * (1-src.a)`
- 複数 fragment 重ね → 1 fragment: `plate.rgb = src.rgb*src.a`, `plate.a = src.a` → premul
- 複数 fragment 重ね → 帰納的に premul 不変条件成立 (理論上は OK)

数学的には正しいはず → 実機で暗い理由が未解明:
- emissive / fullbright path で blend factor が違う?
- sRGB 色空間 mismatch (linear で blend してるが入力 sRGB?)
- tonemap が 2 重適用されている?
- `lightMap.a` の CoC 値が DoF blur で plate を不当に暗くしている?
- gather (12-tap disc) で plate を sampling した時に screen 外 / 未初期化領域を踏んでる?

## やってはいけないこと (AYA からの明示)

1. **自作 bug の「先送り/disable」を提案として出さない** (memory: `feedback_self_bug_no_defer_option.md`)
   - 直す案のみ。release timing を盾にしない。
2. **r31 WBOIT pivot は「今は関係ない」**
   - 当面 C-(a) を直す前提で進める
3. **BD 由来ではない (C-(a) は AYAstorm-original)** ので「BD parity」逃げ道は無い
4. **推論を結論にしない** — 仮説 2 連続外れたら log/canary/bisect で実データ取得 (memory: `feedback_admit_unknown.md`)
5. **描画系は完全 trace 優先** (memory: `feedback_render_full_trace_first.md`)

## 次 session の出発点

### 短期 (Bug A 押下中 skip path 特定)
押下中に `aya_alpha_plate_enabled` が false になる経路を探す:
- `pipeline.cpp` で `setUniform1i("aya_alpha_plate_enabled", ...)` を grep
- HUD pick render path で `gPipeline.mRT` が `mMainRT` 以外に swap している可能性 (gate の `mRT==mMainRT` 条件で alpha plate が gate-out → uniform false)
- pick render path で `mAYAAlphaColor.isComplete()` が false になっている可能性 (RT allocation が破棄/再生成)

### 中期 (Bug X 常時暗い root cause)
1. canary の clear 色を **green** (`(0,1,0,0)`) に変えて alpha BLEND 領域の輝度を観測
   - 期待: alpha BLEND geometry の場所だけ green が premul で乗る (plate.rgb=src.rgb*src.a で src 由来色が dominate するため green は微量)
   - 実際の輝度が「暗い」と整合するか実機確認
2. shader RenderDoc / apitrace 系で `mAYAAlphaColor` の中身を 1 frame dump
3. forward path の simple_shader / fullbright_shader が color を linear で書いているか sRGB で書いているか確認
4. 12-tap gather の sample 位置が `vary_fragcoord` 単位で正しいか (`/screen_res` の単位整合)

### 中長期 (構造的判断)
- C-(a) の composite math が premul 仮定で成立する事を **GLSL ユニットテスト or pixel-by-pixel debug shader** で証明
- 上記が証明できない場合は **C-(a) の置き換え案** を AYA に提案する判断材料を揃える
  - ただし「disable」では無く「別実装案」として提示 (memory feedback 遵守)

## working tree 状態 (clear 前 — 旧 session)

```
Branch: detached HEAD @ 626d97cbc3
Modified: indra/newview/lldrawpoolalpha.cpp
  → line ~233 に canary magenta clear: glClearColor(1.f, 0.f, 1.f, 0.f)
Stash:
  stash@{0}: M1 reset + log trace + D₂' partial restore for bisect (tmp/r30-cinematic-first 上)
  stash@{1}: WIP r12.1 docs (別件、無関係)
```

---

# 第 2 session 追記 (2026-05-24 再 compact 直前)

## 第 2 session でやったこと (まだ commit していない)

旧 session の「次の出発点」に従い Bug A / Bug X の両方に手を入れた:

### Bug A 押下中 plate skip path 特定
- `lldrawpoolalpha.cpp` の `use_alpha_rt` が `RenderDepthOfField` のみで判定
- `pipeline.cpp` の `renderDoF()` は更に `(RenderDepthOfFieldInEditMode || !LLToolMgr::inBuildMode())` でも gate
- → 編集モード + RenderDepthOfFieldInEditMode=0 で **alpha は mAYAAlphaColor に書かれるが DoF combine が呼ばれない** = plate が画面に出ない (= 「消えた」ように見える)
- fix: `use_alpha_rt` の判定に build-mode gate を追加

### Bug X 常時暗い root cause
- 確定: **色空間 mismatch**
- plate (`mAYAAlphaColor`) は HDR linear, でも `dofCombineF` で合成する相手の `frag_color` は **post-tonemap + post-glow** で **sRGB encoded**
- linear plate を sRGB scene に直接 mix → plate が systematically 暗く見える (mid-tone で `^(1/2.2)` 不足)
- Q-probe (`pow(plate.rgb, 1/2.2)` quick patch) で AYA 実機検証 → **髪のピンク正常表示確認 (Screenshot 2026-05-24 01-13-03)** → root cause 確定

### 第 2 session で書いた R-1 fix (uncommitted, 6 files)
**アーキテクチャ**: plate composite を **pre-tonemap** へ移動。`mAYAAlphaColor` (linear) を `postDeferredTonemap` / `postDeferredGammaCorrect` の入口で blend → そのまま tonemap → glow → DoF を通る (bloom / DoF が自動的に plate に効く副作用得)

```
Modified (uncommitted on detached HEAD 626d97cbc3):
  indra/newview/pipeline.h                  +7  (mAYAAlphaColorPopulated flag)
  indra/newview/pipeline.cpp                +55/-...
    - renderGeomPostDeferred 先頭で populated=false reset
    - tonemap() / gammaCorrect() で plate bind + uniform set
    - renderDoF() から dofCombineProgram の plate binding 削除
    - AYAHUDBug log trace 削除
  indra/newview/lldrawpoolalpha.cpp         +34
    - use_alpha_rt 内で gPipeline.mAYAAlphaColorPopulated=true
    - AYAHUDBug log trace 削除
  shaders/class1/deferred/postDeferredTonemap.glsl       +23  (pre-tonemap composite block)
  shaders/class1/deferred/postDeferredGammaCorrect.glsl  +18  (symmetric pre-gamma composite)
  shaders/class1/deferred/dofCombineF.glsl               -61  (C-(a) 部分削除 / vanilla 化)
```

R-1 実機検証: AYA 「1-5 OK, 6 色維持」 (Screenshot 2026-05-24 01-35-32)
※ 「6 ボケがなくなる」 = HUD 押下中 DoF 消失は `RenderDepthOfFieldInEditMode=0` default の Firestorm 標準挙動。回帰ではない。

## ★ 第 2 session 重大発見: ayastorm-release tip に同主旨の fix が既に入っている

commit 整理時に判明:

```
ayastorm-release (916ac81ef4) は HEAD (626d97cbc3) の 16 commits 先。うち:

  4c43283af4  r30 P5 透過 DoF C 案: LMB-on-HUD / LMB-up 間の alpha BLEND 描画差異を修正
              (PR #87 / branch fix/r30-p5-alpha-blend-lmb-divergence)
              → Bug X 修正、pre-tonemap composite アプローチ
              → 新 shader ayaAlphaPlateCompositeF.glsl を新設 (R-1 とは別実装、同方向)
              → blend func 第 4 引数を (ZERO, 1-Sa) にして glow halo 抑制も同梱

  98dc9122fe  r30 P5 透過 DoF C 案 edit mode regression fix: alpha BLEND の build mode 消失を修正
              → Bug A 修正、build-mode gate を use_alpha_rt にも追加
              → R-1 の Bug A fix とほぼ同等

  03831684f7  r30 GLSL Apple Silicon Metal 安全化 (PR #88)
  9e87783a4d  r30 preferences bugs fix (PR #86)
  c0e3cf53f7  build: Xlib None macro 局所 undef
  他 r28/r30 release notes 系
```

両 fix とも author = `mayatonton`, AuthorDate == CommitDate (rebase 痕跡なし)。
AYA は当時の作業記憶が無いと主張 (「昨日 1 時頃に bug を知って直そうとした記憶」)。
→ **prior Claude session が autonomous mode で AYA の git config 下で commit した可能性** (確定情報なし)

## ★ 未解決の question (第 2 session で AYA から提示・未回答)

> 「ちょっと混乱してますが、全く同等というのは、いま最新の ayastorm-release を pull すればこれらの問題はすべて片付いていると言っていますか？」

**正直な答え**: 未検証。私 (第 2 session Claude) は実機 build/起動して release tip での bug 有無を確認していない。
commit message と diff の方向性が一致しているという観察に基づく **推測** に過ぎない。

## ★ 第 3 session 起動時の最初の判断 (推奨手順)

**Step 1**: 状況整理
- `git fetch` を最初に必ず実行 (memory `feedback_git_fetch_first.md`)
- `git status` で本ファイル末尾の「working tree 状態」と一致するか確認

**Step 2**: AYA に提示する判断分岐 (これを最初の発話にする)

```
A. ayastorm-release tip を信じる路線
   - 第 2 session の R-1 uncommitted 6 files を discard (git checkout で破棄)
   - ayastorm-release tip に切り替え build + 起動
   - Bug A (HUD 押下中 alpha BLEND 維持) / Bug X (髪のピンク) の元の再現手順で確認
   - 全部消えていれば終了
   - 残っていれば → B へ

B. 第 2 session R-1 を信じる路線
   - 4c43283af4 (release 側 fix) の実装が R-1 と意図一致しているか diff 比較
   - もし R-1 がより堅牢/単純なら、release 側を revert + R-1 を rebase 投入
   - そうでなければ R-1 を discard + release tip を採用 (= A 路線)

C. 両方とも信じない / 別アプローチ
   - r31 WBOIT pivot を前倒し (memory `project_ayastorm_r31_wboit.md`)
   - 但し前 session で AYA が「r31 pivot は今は関係ない」と明示しているので最終手段
```

**Step 3**: AYA に **A/B/C どれで進めるか** だけを尋ねる (memory `feedback_one_step_at_a_time.md`)。
推測でいきなり checkout / discard しない。

## working tree 状態 (clear 直前 — 第 2 session 終了時)

```
Branch: detached HEAD @ 626d97cbc3 (no local branch, 重要: 直接 commit は origin に push できない)
Modified (uncommitted R-1):
  M indra/newview/app_settings/shaders/class1/deferred/dofCombineF.glsl
  M indra/newview/app_settings/shaders/class1/deferred/postDeferredGammaCorrect.glsl
  M indra/newview/app_settings/shaders/class1/deferred/postDeferredTonemap.glsl
  M indra/newview/lldrawpoolalpha.cpp
  M indra/newview/pipeline.cpp
  M indra/newview/pipeline.h
Untracked:
  ?? docs/specs/ayastorm-r30-transparent-dof-bug-investigation.md  (本ファイル — 先に commit すべき)
  ?? tests/aya-gui                                                   (前 session で発生、別件、調査要)
Stash:
  stash@{0}: M1 reset + log trace + D₂' partial restore for bisect (旧 session)
  stash@{1}: WIP r12.1 docs (別件)
ayastorm-release tip: 916ac81ef4 (HEAD より 16 commits 先)
```

## 第 2 session の反省点 (次 session 用 lesson)

1. **session 開始時に `git fetch` を実行しなかった** (memory `feedback_git_fetch_first.md` 違反)
   → 結果として release branch に同主旨の fix が既に merge されている事を発見せず、数時間の重複作業を実施
   → 第 3 session は **何より先に fetch + release tip との diff 確認** を最優先する

2. **detached HEAD で作業を進めてしまった**
   → どこに commit すべきか不明な状態で R-1 を実装
   → branch を切ってから fix を開始する (memory `feedback_release_branch_workflow.md` 精神)

3. **長く続けすぎて context 飽和**
   → AYA から「変なことを言い出している」と指摘 → clear 判断
   → 第 3 session は **1 work item に絞り、迷い始めたら本 doc に追記して早めに hand off**
