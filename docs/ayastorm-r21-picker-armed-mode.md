# AYAstorm r21 self rigged picker armed mode

**作成日**: 2026-05-15
**対象ブランチ**: `ayastorm-r21-picker-armed-mode`
**対象機能**: r21 self rigged attachment GPU picker

---

## 目次

- [概要](#概要)
- [目的](#目的)
- [実装方針](#実装方針)
- [設定](#設定)
- [GPU picker が readback できる条件](#gpu-picker-が-readback-できる条件)
- [実測ログ](#実測ログ)
- [カーソルを外した場合](#カーソルを外した場合)
- [追加ログ追跡](#追加ログ追跡)
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
