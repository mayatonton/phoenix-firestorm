# AYAstorm r28 other rigged picker 実装・検証メモ

作成日: 2026-05-19 JST
対象ブランチ: `feature/ayastorm-r28-other-rigged-picker`
対象機能: 他人 avatar の rigged attachment 右クリック picker

関連資料:

- `docs/specs/ayastorm-r21-self-rigged-picker.md`
- `docs/specs/ayastorm-r21-picker-armed-mode.md`
- `docs/specs/ayastorm-r21-selection-handoff-investigation.md`

## 目次

1. [要約](#1-要約)
2. [目的と非目標](#2-目的と非目標)
3. [現在の設定値](#3-現在の設定値)
4. [実装方針](#4-実装方針)
5. [実装内容](#5-実装内容)
6. [負荷対策](#6-負荷対策)
7. [ログ設計](#7-ログ設計)
8. [検証結果](#8-検証結果)
9. [残リスクと運用注意](#9-残リスクと運用注意)
10. [Mac 版実機確認](#10-mac-版実機確認)

## 1. 要約

r21 の self rigged picker は、自分の rigged attachment を右クリックしたときに、画面上で見えている attachment を GPU ID buffer から選び直す機能である。

r28 では、この仕組みを他人 avatar の rigged attachment へ限定的に拡張した。視界内の全 avatar を常時描く設計ではなく、hover または right-click で得た対象 avatar を 1 人だけ armed し、その avatar の rigged attachment だけを GPU ID pass に描く。

2026-05-19 時点で確認済み:

- Release arm64 test build 成功。
- `AYAstorm.app` の codesign verify 成功。
- 他人 avatar 用 GPU ID pass が `armed -> render ready -> readback -> redirected` まで動作することをログで確認。
- デフォルトカメラ gate が効き、ESC 後の標準視点では他人 avatar hover で GPU ID pass を起動しないことを確認。
- self avatar は other picker の対象外として skip される。
- 自分 avatar の顔右クリックは self avatar として選択できる状態に修正済み。

## 2. 目的と非目標

目的:

- 他人 avatar の rigged attachment を右クリックしたとき、既存 worldray pick が外した場合でも GPU ID pass により画面上の attachment を選択しやすくする。
- 対象 avatar を 1 人に限定し、混雑地での負荷増大を抑える。
- self picker の既存挙動を維持する。

非目標:

- 視界内の全 avatar / 全 attachment を常時 ID buffer に描かない。
- 他人 avatar 全員の attachment ID buffer をフレームごとに維持しない。
- HUD attachment は対象外のままにする。
- 非 rigged attachment は原則として既存 worldray picker に委ねる。
- 編集権限、所有者権限、コンテキストメニュー権限の仕様は変更しない。

## 3. 現在の設定値

現在の r28 関連 hidden setting:

```text
FSOtherRiggedPickerEnable = true
FSOtherRiggedPickerGPU = true
FSOtherRiggedPickerArmSeconds = 1.0
```

補足:

- `FSOtherRiggedPickerEnable` は default on、`FSOtherRiggedPickerGPU` は kill-switch、`FSOtherRiggedPickerArmSeconds` は hover arm window。
- `RequireNonDefaultCamera` 動作は内部 hardcoded true。ESC 後のデフォルトカメラ位置では other picker を arm しない (cvar として外には出さない)。
- draw call / triangle 予算は内部 hardcoded (512 / 1,200,000)。tuning は将来 release で再調整するまで動かさない。
- 検証ログは出荷物から strip 済 (r21 doctrine: `feedback_remove_verification_logs.md`)。再現確認は build local で `LL_DEBUGS` 系を一時投入する運用。
- other picker は常に armed window 方式で動く。`FSOtherRiggedPickerArmedMode` は持たせない。

## 4. 実装方針

### 4.1 self picker は維持する

r28 は r21 self picker を置き換えない。

- self avatar: 既存 self picker を維持する。
- self avatar: デフォルトカメラ位置でも picker は動作する。
- other avatar: デフォルトカメラ位置では hover しても GPU ID pass を起動しない。
- other avatar: mouselook / avatar customize 中は hover、render、right-click readback のすべてで GPU picker を使わない。

### 4.2 対象 avatar は 1 人だけ

対象 avatar は次のいずれかで決める。

1. hover pick で得た avatar。
2. right-click の既存 worldray pick が当てた avatar または attachment の root avatar。

どちらも取得できない場合、other picker は起動しない。

対象 avatar が変わった場合、古い buffer は ready として扱わない。ready 判定は avatar ID と generation の一致で行う。

### 4.3 ID pass は短時間だけ走らせる

hover で target avatar を arm し、`FSOtherRiggedPickerArmSeconds` の間だけ GPU ID pass を許可する。

right-click 時に buffer が ready であれば readback する。ready でなければ readback せず既存 worldray pick に fallback する。

### 4.4 self armed と other armed の優先順位

`mObjectIDBuffer` は self picker と other picker で共有する。

`renderDeferredLighting()` では other armed を先に評価する。理由は、self picker の armed window が残っている間でも、カーソルが他人 avatar に移って other picker が armed された場合は、その frame で必要なのは other avatar 用の ID buffer だからである。

以前の順序では、`self armed` が先に true になると `else if` の other 側へ入らず、other 用の `renderOtherRiggedObjectIDBuffer()` が呼ばれない可能性があった。これは buffer が上書きされる問題ではなく、分岐順により other pass が実行されない問題である。

現在の順序:

```cpp
if (!gCubeSnapshot && isOtherRiggedObjectIDBufferArmed())
{
    renderOtherRiggedObjectIDBuffer();
}
else if (!gCubeSnapshot && (!armed_mode || isSelfRiggedObjectIDBufferArmed()))
{
    renderSelfRiggedObjectIDBuffer();
}
```

## 5. 実装内容

### 5.1 変更ファイル

主な変更ファイル:

- `indra/newview/app_settings/settings.xml`
- `indra/newview/lltoolpie.cpp`
- `indra/newview/fsselfriggedpicker.cpp`
- `indra/newview/pipeline.cpp`
- `indra/newview/pipeline.h`

### 5.2 pipeline 状態

`pipeline.cpp` に other picker 用の状態を追加した。

```cpp
LLFrameTimer sFSOtherRiggedPickerArmTimer;
F32 sFSOtherRiggedPickerArmSeconds;
U32 sFSOtherRiggedPickerArmGeneration;
U32 sFSOtherRiggedPickerRenderGeneration;
LLPointer<LLVOAvatar> sFSOtherRiggedPickerAvatar;
LLUUID sFSOtherRiggedPickerAvatarID;
```

ready 条件:

- armed window 内である。
- target avatar が alive である。
- target avatar ID が一致している。
- render generation と arm generation が一致している。

### 5.3 pipeline API

other picker 用に以下を追加した。

```cpp
void LLPipeline::armOtherRiggedObjectIDBuffer(LLVOAvatar* avatar, F32 seconds);
bool LLPipeline::isOtherRiggedObjectIDBufferArmed() const;
bool LLPipeline::isOtherRiggedObjectIDBufferReady(const LLUUID& avatar_id) const;
void LLPipeline::clearOtherRiggedObjectIDBuffer();
void LLPipeline::renderOtherRiggedObjectIDBuffer();
```

`renderRiggedObjectIDBufferForAvatar()` は `LLPipeline` 内部 helper とし、外部公開 API にはしない。

`armOtherRiggedObjectIDBuffer()` は次を拒否する。

- null avatar
- dead avatar
- self avatar
- imposter
- seconds <= 0

### 5.4 hover arm

`lltoolpie.cpp` の hover 経路で pick object から avatar を解決する。

流れ:

```text
hover pick object あり
  -> object が attachment なら owner avatar を取得
  -> object が avatar 本体ならその avatar を取得
  -> avatar が self なら other picker は使わない
  -> mouselook / avatar customize なら clear して使わない
  -> default camera gate 中なら clear して使わない
  -> 他人 avatar なら other picker を arm
```

### 5.5 デフォルトカメラ gate

other picker の hover arm は、ESC 後の標準視点では許可しない。

default camera とみなす条件:

- 三人称 camera
- free camera ではない
- `focusOnAvatar` が true
- current zoom fraction が 1.0 付近

実カメラ位置と初期 offset の差分は判定に使わない。collision correction、座り、頭位置補正、カメラスムージングで標準視点でも位置がずれるためである。

### 5.6 ID pass 描画

`renderRiggedObjectIDBufferForAvatar(target_avatar, ...)` が、指定 avatar の rigged draw info だけを `mObjectIDBuffer` に描く。

対象 pass は `PASS_*_RIGGED` のみ。

描画時に行うこと:

- `gGL.flush()` で直前の通常描画状態を確定する。
- `mObjectIDBuffer` を bind する。
- color mask / clear color / cull face mode を保存し、終了時に復元する。
- depth は deferred screen と共有し、depth test は使うが depth write はしない。
- target avatar と一致しない `LLDrawInfo` は skip する。
- `mFSPickerLocalID` を RGBA8 に pack して shader uniform へ渡す。
- draw call / triangle budget を超えた場合は pass を失敗扱いにし、buffer を clear して not ready にする。

### 5.7 readback と解決

`FSSelfRiggedPicker::findClosestAttachmentForAvatar()` を追加した。

right-click 時の流れ:

```text
1. 既存 worldray pick を取得
2. self avatar / self attachment は既存 self picker で処理
3. other avatar / other attachment の場合だけ other picker を試す
4. other ID buffer ready を確認
5. ready なら glReadPixels(1x1) で LocalID を読む
6. LocalID != 0 なら target avatar の attachment tree だけを探索
7. 見つかれば mPick.mObjectID を picked object に補正
8. 見つからなければ既存 worldray pick に fallback
```

LocalID 解決は target avatar の attachment tree だけで行う。これにより、別 avatar の attachment と誤対応しない。

### 5.8 self avatar 顔選択修正

自分 avatar の顔を右クリックしたとき、worldray が self avatar object を返していても `mPick.mObjectID` が `gAgent.getID()` に正規化されず、self avatar として menu 判定されない経路があった。

対応:

- upstream が self avatar object を返した場合、`mPick.mObjectID` を `gAgent.getID()` に正規化する。
- GPU self picker が attachment hit なしを返した場合でも、avatar body 選択へ戻す。

この修正後、自分 avatar の顔は選択できることを確認済み。

## 6. 負荷対策

### 6.1 常時全員描画を禁止

初期実装では以下をしない。

- 全 avatar を毎フレーム ID buffer に描く。
- カメラ内 avatar 全員を候補にする。
- nearby list 全員を事前描画する。

### 6.2 draw call / triangle budget

現在の budget (`indra/newview/pipeline.cpp` 内 hardcoded):

```text
kMaxDrawCalls = 512
kMaxTriangles = 1200000
```

経緯:

- 初期値 `MaxDrawCalls=160` では重い avatar で over budget になり、ID pass が ready まで進まなかった → 512。
- triangle `800000` では、実用域の重い avatar で境界に当たった → 1,200,000。

上限超過時は `renderRiggedObjectIDBufferForAvatar` が false を返し、`sFSOtherRiggedPickerRenderGeneration` を 0 に倒して `isReady()` を false に戻す。right-click 側は readback に入らず、既存 worldray pick にフォールスルーする。

budget 再調整が必要になった場合は次の release で hardcoded 値を動かす方針 (cvar 化しない)。

### 6.3 readback は右クリック時のみ

hover 中に毎フレーム readback しない。hover 中は ID buffer を準備するだけにする。

`glReadPixels(1x1)` は right-click 時だけ実行する。buffer が ready でない場合は readback helper に入らない。

## 7. ログ設計

出荷物には永続的な検証ログ hook を含めない (r21 doctrine: `feedback_remove_verification_logs.md`)。再現確認が必要になった場合は、build local で `LL_DEBUGS("FSOtherRiggedPicker")` などを一時投入する運用とする。

主な確認ポイント (オペレータが心の中で追う項目):

- `armed`: hover で対象 avatar が決まった。
- `render ready`: other avatar 用 GPU ID pass が実際に描かれ、readback 可能になった。
- `readback local_id`: pixel から attachment LocalID を読めた。
- `right-click redirected`: GPU ID pass の結果で selection target を補正した。
- `readback empty`: その pixel には rigged attachment ID がなかった。
- `right-click fallback`: GPU が補正対象を返さなかったため、既存 worldray pick に戻した。
- `target_avatar=<self id> skipped`: other picker は self avatar には介入しない。

## 8. 検証結果

### 8.1 build / codesign

実行コマンド:

```text
git diff --check
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild -project build-darwin-universal/Firestorm.xcodeproj -scheme ayastorm-bin -configuration Release -derivedDataPath build-darwin-universal/DerivedData ARCHS=arm64 ONLY_ACTIVE_ARCH=YES build
codesign --verify --deep --strict --verbose=2 build-darwin-universal/newview/Release/AYAstorm.app
```

結果:

- `git diff --check`: pass
- Release arm64 `ayastorm-bin`: `BUILD SUCCEEDED`
- `AYAstorm.app` codesign verify: pass

### 8.2 runtime log

2026-05-19 の runtime log で、他人用 GPU ID pass の一連の流れを確認した。

デフォルトカメラ gate:

```text
hover ignored by default-camera gate
```

other avatar の arm と render ready:

```text
armed avatar=12ccf354-2cc5-4a44-8ca2-2937cf2c18a1 seconds=1 generation=10 target_changed=1
render ready avatar=12ccf354-2cc5-4a44-8ca2-2937cf2c18a1 generation=10 draw_calls=98 triangles=441767
```

GPU ID readback と redirect:

```text
readback local_id=633088194 avatar=12ccf354-2cc5-4a44-8ca2-2937cf2c18a1 hit=e482ae8d-23c2-072b-c52f-dc5ee86c48b0
right-click redirected avatar=12ccf354-2cc5-4a44-8ca2-2937cf2c18a1 upstream=9ad4bf05-1c38-089c-dcaa-ff6febf67bde picked=e482ae8d-23c2-072b-c52f-dc5ee86c48b0 local_id=633088194
```

別 avatar でも render ready / redirect を確認:

```text
render ready avatar=9c83faed-0483-47a4-898e-a837d3e5f2cf generation=7 draw_calls=321 triangles=365903
right-click redirected avatar=9c83faed-0483-47a4-898e-a837d3e5f2cf upstream=56b48a8a-50e3-5c49-b57d-497a4e637d86 picked=44585339-b66f-f095-8f1d-355f93437b8f local_id=633095806
```

readback empty の fallback:

```text
readback empty avatar=9c83faed-0483-47a4-898e-a837d3e5f2cf local_id=0
right-click fallback avatar=9c83faed-0483-47a4-898e-a837d3e5f2cf upstream=de4fae01-92ca-9798-cafa-61a16b7b396b gpu_authoritative=1
```

self avatar skip:

```text
right-click skipped target_avatar=60764175-8400-427a-af9c-87d891897281 upstream_hud=0
```

このため、現時点では「他人用 GPU ID pass がコード上存在する」だけではなく、実行時に `armed -> render ready -> readback -> redirected` まで到達していることを確認済み。

## 9. 残リスクと運用注意

残リスク:

- 混雑地で hover 対象を頻繁に変えると、ID pass の対象切替が増える。
- 他人 avatar の outfit が非常に重い場合、1 avatar 限定でも ID pass の draw call / triangle が増える。
- alpha-blend で depth を書かない髪は、self picker と同じく完全には解決できない。
- hover pick が不安定な場合、right-click 時に buffer が ready でないことがある。その場合は既存 worldray pick に fallback する。

運用注意:

- 問題が出た場合は `FSOtherRiggedPickerEnable=false` で other picker 全体を止められる。
- 通常の背後視点で他人 avatar に cursor が触れただけでは ID pass を走らせない (RequireNonDefaultCamera は内部 hardcoded、cvar として外には出さない)。

## 10. Mac 版実機確認

2026-05-19 時点で、Mac 版の test build を実機起動し、以下を確認した。

- `FSOtherRiggedPickerEnable=true` で other picker が有効化される。
- other picker は対象 avatar を 1 人に限定する。
- デフォルトカメラ位置では other picker の hover arm が抑止される。
- mouselook / avatar customize 中は other picker を使わない。
- `renderOtherRiggedObjectIDBuffer()` が他人 avatar 用 GPU ID pass を描く。
- runtime で `armed -> render ready -> readback -> redirected` を確認済み (検証 build に一時投入したログでチェック、出荷物には残していない)。
- self avatar は other picker の対象外として skip される。
- self picker の既存挙動は維持され、自分 avatar の顔選択も修正済み。
- draw call / triangle budget を持ち、上限超過時は fallback する。
- readback 失敗時または `local_id=0` 時は既存 worldray pick に戻る。
- selection handoff は補正後の `mPick` で行う。
- Release arm64 build と `AYAstorm.app` codesign verify が成功した。
