# AYAstorm r21 selection handoff investigation

**作成日**: 2026-05-15
**対象ブランチ**: `fix/ayastorm-r21-self-rigged-picker-selection`
**対象機能**: r21 self rigged attachment GPU picker

---

## 概要

r21 self rigged attachment GPU picker の検証中、右クリック後に次の警告が大量に出るケースを確認した。

```text
Couldn't find object ... selected.
```

この警告は `LLSelectMgr::processObjectProperties()` で出ている。意味は、sim から `ObjectProperties` 応答が返ってきたが、その UUID の object が現在の selection node に存在しない、というもの。

調査の結果、GPU picker の readback 自体ではなく、右クリック時に temporary selection を渡す順序が原因である可能性が高い。

---

## 障害内容

修正前の `LLToolPie::handleRightClickPick()` では、`LLToolSelect::handleObjectSelection(mPick, false, true)` が AYA GPU picker 補正前に一度実行されていた。

さらに、GPU picker が self rigged attachment を拾った場合、補正後の `mPick` に対しても `handleObjectSelection()` が再度呼ばれる構造だった。

このため、右クリック 1 回につき次の 2 種類の temporary selection が連続して送られる可能性があった。

1. 上流 worldray の stale な selection
2. AYA GPU picker 補正後の selection

その結果、先に送られた selection に対する `ObjectProperties` 応答が返った時点では、すでに現在の selection が別 object に置き換わっており、`processObjectProperties()` が対象 object を見つけられず警告を出していたと考えられる。

---

## 修正案

`LLToolSelect::handleObjectSelection(mPick, false, true)` の呼び出しを、AYA GPU picker の補正後に 1 回だけ行う。

AYA GPU picker block 内では、次の補正だけを行う。

- `object` の差し替え
- `mPick.mObjectID` の差し替え

selection は block の後で一度だけ渡す。

```cpp
// Can't ignore children here. Select only after the optional AYA GPU
// redirect has finalized mPick, otherwise right-click can send a stale
// temporary selection before the corrected attachment selection. Keep the
// call unconditional so land/no-object picks still follow upstream
// deselection behavior.
LLToolSelect::handleObjectSelection(mPick, false, true);
```

この呼び出しは unconditional のまま維持する。land / no-object pick における upstream の deselection 挙動を変えないため。

---

## 調査用 trace

実機確認のため、一時的に `FSSelfRiggedPickerTrace` を追加して以下を `FSSelfRiggedPicker` channel に出していた。

- `renderSelfRiggedObjectIDBuffer()` が実際に走ったか
- `mObjectIDBuffer` のサイズ
- self rigged DrawInfo の candidate / draw call / triangle 数
- 右クリック時の `glReadPixels` 結果 RGBA と復元 LocalID
- GPU readback を skip した場合の理由

調査用機能のため、本 investigation での PASS 判定後に cvar と LL_INFOS hook 一式を出荷物から除去した (memory `feedback_remove_verification_logs.md` 方針)。本書中で `FSSelfRiggedPickerTrace` を有効にする手順は、当時の調査状況の記録として残している。

---

## 比較検証

2026-05-15 に、同じ trace を入れた状態で以下の 2 パターンを比較した。

### 修正後 app

- `LLToolSelect::handleObjectSelection()` を AYA GPU picker 補正後の 1 回に集約した状態
- `FSSelfRiggedPicker` の GPU pass / readback は継続して動作
- `Couldn't find object ... selected.` はログ検索で再発なし

### 修正前 trace-only app

- `LLToolSelect::handleObjectSelection()` を AYA GPU picker 補正前にも呼ぶ旧構造
- 比較用 app:

```text
build-darwin-universal/newview/Release/AYAstorm-trace-only-before-selection-fix.app
```

- GPU pass / readback は動作
- `Couldn't find object ... selected.` が大量に再発

観測例:

```text
2026-05-14T19:22:07Z INFO #FSSelfRiggedPicker# ... readback ... local_id=49768396
2026-05-14T19:22:07Z WARNING # newview/llselectmgr.cpp(6275) processObjectProperties : Couldn't find object ... selected.

2026-05-14T19:22:10Z INFO #FSSelfRiggedPicker# ... readback ... local_id=49768397
2026-05-14T19:22:10Z WARNING # newview/llselectmgr.cpp(6275) processObjectProperties : Couldn't find object ... selected.
```

同ログ内で `Couldn't find object ... selected.` は 887 件確認された。

---

## 他人アバター右クリック時の確認

2026-05-15 に、他人アバターの装着物を右クリックして、self rigged picker が介入していないか確認した。

確認時のログ開始位置:

```text
AYAstorm.log 3297 行
2026-05-14T19:33:25Z
```

3297 行以降では、次の定期 GPU ID pass は確認された。

```text
2026-05-14T19:33:28Z INFO #FSSelfRiggedPicker# ... GPU ID pass ran buffer=2560x1368 candidates=87 draw_calls=87 triangles=456559
2026-05-14T19:33:30Z INFO #FSSelfRiggedPicker# ... GPU ID pass ran buffer=2560x1368 candidates=36 draw_calls=36 triangles=301016
```

ただし、右クリック時に実際の picker readback を示す次のログは出ていない。

```text
findClosestAttachment : readback
```

したがって、他人アバターの装着物右クリックでは、`FSSelfRiggedPicker::findClosestAttachment()` の readback 経路には入っていない。

`GPU ID pass ran` は描画パイプライン側で self rigged picker 用の ID buffer を定期更新しているログであり、右クリック判定そのものではない。この pass は自分の `gAgentAvatarp` にぶら下がる rigged attachment だけを描く。`renderSelfRiggedObjectIDBuffer()` 側でも `info->mAvatar.get() != agent_avatar` を skip するため、他人アバターの装着物を ID buffer に描くものではない。

今回の selection handoff 修正で変更したのは `LLToolSelect::handleObjectSelection()` を呼ぶタイミングであり、`renderSelfRiggedObjectIDBuffer()` が定期実行される構造は変更していない。したがって、この定期 GPU ID pass が出る挙動は修正前後で同じ。

---

## 判断

比較結果から、警告は GPU picker の readback そのものではなく、selection を渡すタイミングによって誘発されている可能性が高い。

修正前 trace-only app では、GPU readback が動作しているにもかかわらず警告が大量に出る。修正後 app では、同じ GPU pass / readback が動作していても警告は再発していない。

したがって、`LLToolSelect::handleObjectSelection()` を AYA GPU picker 補正後の 1 回に集約する修正は妥当と判断する。
