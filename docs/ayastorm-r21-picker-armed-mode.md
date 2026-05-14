# AYAstorm r21 self rigged picker armed mode

**作成日**: 2026-05-15
**対象ブランチ**: `ayastorm-r21-picker-armed-mode`
**対象機能**: r21 self rigged attachment GPU picker

---

## 目次

- [概要](#概要)
- [目的](#目的)
- [負荷見積もり](#負荷見積もり)
- [実装方針](#実装方針)
- [設定](#設定)
- [GPU picker が readback できる条件](#gpu-picker-が-readback-できる条件)
- [実測ログ](#実測ログ)
- [カーソルを外した場合](#カーソルを外した場合)
- [追加ログ追跡](#追加ログ追跡)
- [マウスルック時の追加確認](#マウスルック時の追加確認)
- [現時点の評価](#現時点の評価)
- [今後の改善候補](#今後の改善候補)

---

## 概要

armed mode は、self rigged picker 用の GPU ID pass を常時描画せず、必要になりそうな短い期間だけ描画するための試作である。

この変更は、GPU メモリリソースや描画余力が限られている PC 向けの発展的な負荷低減候補として扱う。

---

## 目的

`FSSelfRiggedPickerGPU` が有効な場合、self rigged picker 用の GPU ID pass は描画パイプライン側で定期的に走る。

実測ログでは ID buffer は次のサイズだった。

```text
buffer=2560x1368
```

この buffer は一時的な render target / buffer であり、フレームごとにメモリへ蓄積されるものではない。

ただし、描画 pass としては self rigged attachment 候補を追加で描くため、GPU 描画負荷は発生する。

armed mode の目的は、常時 GPU ID pass を走らせず、右クリック前に必要になりそうな期間だけ ID buffer を用意すること。

---

## 負荷見積もり

現時点では GPU time / CPU time を計測する profiling は行っていない。そのため、この節では実測ログから確認できる負荷要素と、armed mode によって削減できる範囲を整理する。

常時描画の場合、`FSSelfRiggedPickerGPU` が有効であれば、self rigged picker 用の GPU ID pass は deferred lighting のタイミングで継続的に走る。

実測ログでは、1 回の GPU ID pass で次の規模の描画が発生していた。

```text
buffer=2560x1368
candidates=124 draw_calls=124 triangles=539611

buffer=2560x1368
candidates=129 draw_calls=129 triangles=619980
```

この pass は通常表示用の描画とは別に、picker 用 ID buffer へ self rigged attachment 候補を描く。したがって、1 回あたりの主な負荷は次の通り。

- 実行解像度相当の ID buffer を render target として使う
- self rigged attachment 候補ぶんの draw call を追加で発行する
- 上記ログでは約 12 万から 13 万候補ではなく、約 124 から 129 draw calls
- triangle 数は約 54 万から 62 万
- 右クリック時には該当 mouse pixel の readback が走る

buffer はフレームごとに蓄積されるものではないため、メモリ使用量が毎フレーム増え続ける種類の負荷ではない。一方で、描画 pass としては毎回追加描画になるため、GPU 描画時間と bandwidth には影響する。

armed mode で削減できるのは、「GPU ID pass を実行する時間帯」である。

armed でない時間は `renderSelfRiggedObjectIDBuffer()` を呼ばないため、上記の追加 draw call / triangle 描画は発生しない。

一方で、armed 中の 1 回あたりの負荷は常時描画時と同じ。armed mode は ID buffer を軽くするものでも、描画解像度を下げるものでも、draw call を削減するものでもない。

したがって、負荷削減量はおおむね次の比率で決まる。

```text
削減される GPU ID pass 負荷 ~= 1 - (armed になっている時間 / viewer 稼働時間)
```

例:

- viewer 稼働中の 5% だけ self avatar / attachment に hover する使い方なら、GPU ID pass の追加描画時間はおおむね 5% まで減る
- ほとんど自分のアバター上にカーソルを置かない使い方では、常時描画と比べて大きく減る
- カーソルを自分のアバター / 装着物上に置き続ける使い方では、armed が更新され続けるため常時描画に近くなる

実測では、カーソルをアバターから外した状態で約 20 秒追跡した範囲では `GPU ID pass ran` は出なかった。この状態では、常時描画時に発生していた GPU ID pass の追加描画は抑制できている。

ただし、hover 中は `GPU ID pass ran` が断続的に継続した。hover 中の描画頻度をさらに抑えるには、armed mode とは別に throttling や低解像度 buffer 化が必要になる。

---

## 実装方針

カーソルが自分のアバター、または自分の装着物に hover したら picker を armed 状態にする。

armed 状態の間だけ `renderSelfRiggedObjectIDBuffer()` の実行を許可する。

重要な点として、armed mode は GPU ID pass の描画間隔を間引くものではない。

従来は deferred lighting のタイミングで `renderSelfRiggedObjectIDBuffer()` を呼んでいた。armed mode でも、armed 中は同じ deferred lighting のタイミングで呼ぶ。

違いは「呼ぶ間隔」ではなく、「armed でない時に呼ばない」という条件だけ。

したがって、カーソルを自分のアバター / 装着物上に置き続けて armed が更新され続ける場合、armed 中の描画頻度は従来の定期描画と同じになる。

---

## 設定

現在の試作値:

```text
FSSelfRiggedPickerArmedMode = true
FSSelfRiggedPickerArmSeconds = 3.0
```

`FSSelfRiggedPickerArmSeconds` は、最後に hover してから GPU ID pass を許可する秒数である。

挙動:

1. 自分のアバター、または自分の装着物にカーソルを乗せる
2. picker が armed 状態になる
3. 次の描画フレームで GPU ID pass が走る
4. hover し続けると armed が更新される
5. カーソルを外すと、armed window が切れた後に GPU ID pass が止まる

---

## GPU picker が readback できる条件

GPU picker として readback できる条件は、単に設定が ON であることだけではない。

必要条件:

- `FSSelfRiggedPickerGPU` が true
- `mObjectIDBuffer` が complete
- armed mode 有効時は、hover 後に GPU ID pass が少なくとも 1 回走っている
- 右クリック時点で buffer が ready 扱いになっている

---

## 実測ログ

カーソルを自分のアバター / 装着物に乗せた状態では、GPU ID pass と readback が出た。

```text
INFO #FSSelfRiggedPicker# ... renderSelfRiggedObjectIDBuffer : GPU ID pass ran buffer=2560x1368 candidates=124 draw_calls=124 triangles=539611
INFO #FSSelfRiggedPicker# ... findClosestAttachment : readback mouse=1202,735 buffer_xy=1202,735 rgba=(179,10,205,40) local_id=684526259
```

別の時点では、candidate 数が変化していた。

```text
INFO #FSSelfRiggedPicker# ... GPU ID pass ran buffer=2560x1368 candidates=129 draw_calls=129 triangles=619980
```

`rgba` が非ゼロで `local_id` が復元されている場合、GPU ID buffer の readback は実際に機能している。

一方で、readback しても対象 ID が入っていない座標では次のようになる。

```text
INFO #FSSelfRiggedPicker# ... findClosestAttachment : readback mouse=1148,829 buffer_xy=1148,829 rgba=(0,0,0,0) local_id=0
```

これは GPU picker が走っていないという意味ではなく、その座標に self rigged attachment ID が描かれていないという意味。

---

## カーソルを外した場合

カーソルをアバターから外した状態で約 20 秒ログ追跡した。

その間、picker 関連ログは出なかった。

確認できなかったログ:

```text
GPU ID pass ran
findClosestAttachment : readback
readback skipped armed_mode=1
GL Error happens before reading back texture. Error code: 1282
```

この結果から、試作 armed mode は少なくとも「カーソルをアバターから外すと GPU ID pass が止まる」挙動になっている。

---

## 追加ログ追跡

2026-05-15 に、armed mode 試作 app で追加のログ追跡を行った。

追跡対象:

```text
FSSelfRiggedPicker
GPU ID pass
readback
armed_mode
GL Error
```

### カーソルをアバターから外した状態

カーソルをアバターから外した状態で約 20 秒追跡した。

結果:

- `GPU ID pass ran` は出なかった
- `findClosestAttachment : readback` は出なかった
- `GL Error happens before reading back texture. Error code: 1282` は出なかった

この状態では、armed mode により GPU ID pass は止まっている。

### カーソルを乗せて操作した状態

カーソルを自分のアバター / 装着物付近に戻して操作した状態で追跡した。

観測例:

```text
INFO #FSSelfRiggedPicker# ... findClosestAttachment : readback mouse=1241,612 buffer_xy=1241,612 rgba=(79,8,205,40) local_id=684525647
INFO #FSSelfRiggedPicker# ... renderSelfRiggedObjectIDBuffer : GPU ID pass ran buffer=2560x1368 candidates=129 draw_calls=129 triangles=619980
INFO #FSSelfRiggedPicker# ... findClosestAttachment : readback mouse=1260,571 buffer_xy=1260,571 rgba=(214,10,205,40) local_id=684526294
INFO #FSSelfRiggedPicker# ... findClosestAttachment : readback mouse=1311,714 buffer_xy=1311,714 rgba=(0,0,0,0) local_id=0
```

結果:

- `GPU ID pass ran` は出た
- `readback` も出た
- `rgba` 非ゼロの local id 復元と、`local_id=0` の両方を確認した
- `GL Error ... 1282` は出なかった
- `readback skipped armed_mode=1` は出なかった

右クリック / 対象確認中は readback が連続して出る。その後、readback が止まり、GPU ID pass のみが断続的に出る状態になった。

これは selection 処理そのものではなく、hover により armed 状態が更新され、ID buffer の描画だけが継続している状態と見てよい。

---

## マウスルック時の追加確認

2026-05-15 に、マウスルック状態で armed mode が走るかを追加確認した。

追跡対象:

```text
FSSelfRiggedPicker
GPU ID pass
readback
armed_mode
GL Error
MouseLook
mouselook
```

確認手順:

1. ログ追跡を開始
2. マウスルックに入る
3. 途中でマウスルックを抜ける
4. その後の picker 関連ログを確認

結果:

- マウスルック開始後、最初の約 25 秒は picker 関連ログなし
  - `GPU ID pass ran` なし
  - `findClosestAttachment : readback` なし
- マウスルック解除後と思われるタイミングで `GPU ID pass ran` が 3 回出た
- `findClosestAttachment : readback` は出なかった
- その後、picker 関連ログは止まった

観測ログ:

```text
2026-05-14T20:31:56Z INFO #FSSelfRiggedPicker# ... GPU ID pass ran buffer=2560x1368 candidates=129 draw_calls=129 triangles=619980
2026-05-14T20:31:58Z INFO #FSSelfRiggedPicker# ... GPU ID pass ran buffer=2560x1387 candidates=82 draw_calls=82 triangles=513153
2026-05-14T20:32:00Z INFO #FSSelfRiggedPicker# ... GPU ID pass ran buffer=2560x1368 candidates=0 draw_calls=0 triangles=0
```

判断:

マウスルック中に armed mode が継続的に走っている形跡は確認できなかった。

最後に出た 3 回の GPU ID pass は、マウスルック解除時または解除直後の hover / 描画状態変化で armed が一時的に入った可能性が高い。

`candidates` が `129 -> 82 -> 0` と落ちているため、解除遷移中の一時的な描画更新として扱う。

---

## 現時点の評価

armed mode の挙動としては、カーソルをアバターから外すと GPU ID pass は止まる。

一方で、カーソルが自分のアバター / 装着物上にある間は hover によって armed が更新されるため、GPU ID pass は断続的に継続する。

したがって、現試作は「非 hover 時の常時描画を止める」目的には効いている。

ただし、「hover 中の描画頻度をさらに抑える」目的では追加調整が必要。

---

## 今後の改善候補

- `ArmSeconds` を短くする
- hover のたびに arm を延長せず、一定間隔でだけ更新する
- mouse down 時に arm し、1 frame 待ってから picker / selection を行う
- ID buffer を実行解像度そのままではなく縮小 buffer で試す

mouse down 後に 1 frame 待つ案は menu / selection の応答タイミングに影響するため、慎重に扱う必要がある。
