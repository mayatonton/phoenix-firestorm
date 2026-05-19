# AYAstorm r28 other rigged picker 実装・検証メモ

作成日: 2026-05-19 JST
対象機能: `feature/ayastorm-r28-other-rigged-picker` / 他人 avatar の rigged attachment 右クリック picker
関連資料:

- `docs/specs/ayastorm-r21-self-rigged-picker.md`
- `docs/specs/ayastorm-r21-picker-armed-mode.md`
- `docs/specs/ayastorm-r21-selection-handoff-investigation.md`

実装状況:

- `feature/ayastorm-r28-other-rigged-picker` で初期実装を開始。
- `FSOtherRiggedPickerEnable` は release default on。
- self picker は既存経路を維持。
- other picker は non-default camera gate、1 avatar 限定、draw call / triangle budget つき。
- 2026-05-19 時点で Release arm64 test build と codesign verify は完了。

## 1. 目的

r21 の GPU self-rigged picker は、自分の rigged attachment を右クリックしたときに「画面で見えている attachment」を正しく選ぶための機能である。

本計画では、その考え方を **他人 avatar の rigged attachment** に限定的に拡張する。ただし、視界内の全 avatar を常時 GPU ID pass に描く設計は採らない。負荷増大を避けるため、対象 avatar は **hover pick または既存 worldray pick で得られた 1 人だけ** に限定する。

## 2. 非目標

- 視界内の全 avatar / 全 attachment を常時 ID buffer に描かない。
- 他人 avatar 全員の attachment ID buffer をフレームごとに維持しない。
- HUD attachment は対象外のままにする。
- 非 rigged attachment の挙動は、原則として既存 worldray picker に委ねる。
- 編集権限、所有者権限、コンテキストメニュー権限の仕様は変更しない。

## 3. 基本方針

### 3.0 self picker は既存挙動を維持する

この計画は、r21 の「自分の rigged attachment を右クリックしやすくする」機能を置き換えない。既存 self picker は、デフォルトカメラ位置でも従来どおり動作させる。

追加するのは **他人 avatar の rigged attachment 用 picker** であり、この追加 picker だけに負荷抑制のためのカメラ状態 gate を掛ける。

採用する方針:

- self avatar: 既存 self picker を維持する。
- other avatar: デフォルトカメラ位置では hover しても GPU ID pass を起動しない。
- other avatar: mouselook / avatar customize 中は hover、render、right-click readback のすべてで GPU picker を使わない。
- other avatar: カメラ位置がデフォルト位置から外れているときだけ、hover による armed を許可する。

これにより、通常の視点で他人 avatar にカーソルが触れただけでは追加 pass が発生しない。一方、自分の服を右クリックしやすくする既存目的は維持できる。

### 3.1 対象 avatar を 1 人に絞る

対象 avatar の候補は次のどちらかから決める。

1. hover pick で得た avatar
2. 右クリック時の既存 worldray pick が当てた avatar または attachment の root avatar

どちらも取得できない場合、他人 avatar GPU picker は起動しない。

ただし hover pick を起点にする場合は、さらにカメラ状態 gate を通す。デフォルトカメラ位置では、他人 avatar を hover しても対象 avatar として arm しない。

### 3.2 GPU ID pass は必要な短時間だけ走らせる

self picker の armed mode と同じ考え方を使う。

- 他人 avatar / その attachment に hover したら armed 状態にする
- armed window 中だけ other-avatar ID pass を許可する
- armed window が切れたら pass を止める
- 右クリック時に buffer が ready でなければ既存 picker に戻す

現在の初期値:

```text
FSOtherRiggedPickerEnable = true
FSOtherRiggedPickerGPU = true
FSOtherRiggedPickerRequireNonDefaultCamera = true
FSOtherRiggedPickerArmSeconds = 1.0
FSOtherRiggedPickerDebugLog = false
FSOtherRiggedPickerMaxDrawCalls = 512
FSOtherRiggedPickerMaxTriangles = 800000
```

初期リリースでは master switch を `true` にする。ただし、default camera gate、armed-window 方式、draw call / triangle budget により、通常視点で他人 avatar に cursor が触れただけでは追加 ID pass が走らない構造を維持する。

other picker は常に armed-window 方式とする。self picker の `FSSelfRiggedPickerArmedMode=false` は「常時 ID pass 描画に戻す」意味を持つが、other picker は対象 avatar を hover で 1 人に決める設計なので、同じ意味の `FSOtherRiggedPickerArmedMode` は持たせない。

`FSOtherRiggedPickerRequireNonDefaultCamera` は初期値 `true` とする。これを `false` にするのは、負荷測定で十分に安全と判断できた場合だけにする。

## 4. 実装内容

### 4.0 現在の実装反映状況

2026-05-19 時点の `feature/ayastorm-r28-other-rigged-picker` では、以下を実装済み。

- `settings.xml` に other picker 用の hidden/default-on 設定を追加。
- `FSOtherRiggedPickerArmedMode` は追加しない。other picker は対象 avatar を持たない常時描画モードを定義せず、hover による armed window のみで動作する。
- `FSOtherRiggedPickerDebugLog` を追加し、test build で `arm / clear / render ready / render failed / readback / redirect / fallback / gate skip / not-ready skip` を `AYAstorm.log` へ追えるようにした。
- `lltoolpie.cpp` の hover 経路で、pick 結果から avatar / attachment owner avatar を解決し、other picker を arm する処理を追加。
- other picker の hover arm は、self avatar ではなく他人 avatar のみを対象にする。
- other picker は mouselook / avatar customize では hover arm、ID pass render、right-click readback のすべてを抑止する。
- other picker の hover arm は、デフォルトカメラ位置では抑止する。デフォルトカメラは `focusOnAvatar` かつ三人称かつ初期 zoom として判定し、実カメラ位置の補正差では解除しない。
- default-camera gate の debug log は hover ごとの連続 spam にならないよう、状態遷移時だけに抑制する。
- `LLPipeline` に other picker の arm / ready / clear / render API を追加。
- `renderRiggedObjectIDBufferForAvatar()` は `LLPipeline` 内部 helper とし、外部公開 API にはしない。
- `clearOtherRiggedObjectIDBuffer()` はすでに clear 済みの場合に generation を進めず、そのまま return する。
- rigged object ID pass は target avatar を 1 人に限定して描画する。
- ID pass は通常描画へ干渉しないよう、実行前に `gGL` を flush し、color mask / clear color / cull face mode を復元する。
- self picker と other picker は同一 object ID buffer を共有し、同一フレームでは self picker を優先する。
- `FSSelfRiggedPicker::findClosestAttachmentForAvatar()` を追加し、対象 avatar の attachment tree だけから local ID を解決する。
- 右クリック時は self picker の既存経路を先に処理し、other avatar / other attachment の場合だけ other picker 解決を試みる。
- 右クリック時は other ID buffer ready を先に確認し、not ready の場合は readback helper に入らず既存 worldray pick に戻す。
- other picker が ready でない、readback が 0、対象 avatar が不一致、budget 超過などの場合は既存 worldray pick に fallback する。

現時点で UI の preference 項目は追加していない。設定は hidden setting として扱う。

### 4.1 新規状態

`pipeline.cpp` 側に other-avatar picker 用の状態を追加した。

現在の状態:

```cpp
LLFrameTimer sFSOtherRiggedPickerArmTimer;
F32 sFSOtherRiggedPickerArmSeconds;
U32 sFSOtherRiggedPickerArmGeneration;
U32 sFSOtherRiggedPickerRenderGeneration;
LLPointer<LLVOAvatar> sFSOtherRiggedPickerAvatar;
LLUUID sFSOtherRiggedPickerAvatarID;
```

ID buffer は既存の `mObjectIDBuffer` を self と共有する。self と other を同じフレームで同時に描かず、pass 実行時に対象モードを 1 つに限定する。

ready 判定は avatar ID と generation の一致で行う。対象 avatar が変わる、clear される、render に失敗する、armed window が切れる、といった場合は古い buffer を ready として扱わない。

### 4.2 arm API

`pipeline` に対象 avatar を指定して arm する API を追加した。

```cpp
void LLPipeline::armOtherRiggedObjectIDBuffer(LLVOAvatar* avatar, F32 seconds);
bool LLPipeline::isOtherRiggedObjectIDBufferReady(const LLUUID& avatar_id) const;
void LLPipeline::clearOtherRiggedObjectIDBuffer();
```

`armOtherRiggedObjectIDBuffer()` は null、self avatar、dead object、imposter、HUD camera の対象を拒否する。

self avatar を拒否するのは、self picker の既存経路と other picker の追加経路を混ぜないためである。user-facing な意味では「avatar の装着物を拾いやすくする」機能でも、実装上は self と other を分けておく方が既存挙動を壊しにくい。

ID pass の共通描画処理は `renderRiggedObjectIDBufferForAvatar()` に集約したが、この helper は `LLPipeline` の private 実装に留める。外側からは self / other それぞれの専用 API だけを使う。

### 4.3 デフォルトカメラ gate

other picker の hover armed は、カメラが ESC 後の標準的な背後視点にある間は許可しない。

実装上の判定:

```cpp
bool FSOtherRiggedPicker::shouldArmFromHover()
{
    if (!gSavedSettings.getBOOL("FSOtherRiggedPickerEnable")) return false;
    if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK) return false;
    if (gAgentCamera.cameraCustomizeAvatar()) return false;
    if (isDefaultAvatarCameraView()) return false;
    return true;
}
```

`isDefaultAvatarCameraView()` は次の状態を default camera とみなす。

- 三人称 camera
- free camera ではない
- `focusOnAvatar` が true
- current zoom fraction が 1.0 付近

実カメラ位置と初期 offset の差分は判定に使わない。ビューア側の補正差で ESC 後の標準視点が「非デフォルト」と誤判定されるためである。

zoom / orbit / pan / free camera などで default camera から外れた状態では、other picker の hover armed を許可する。

mouselook / avatar customize は、`FSOtherRiggedPickerRequireNonDefaultCamera=false` でも常に拒否する。直前に armed されていた buffer が残るケースを避けるため、render 経路と right-click 経路でも同じ gate で clear / fallback する。

### 4.4 hover で対象 avatar を更新

`lltoolpie.cpp` の hover 経路で、pick 結果から root avatar を解決する。

判定の流れ:

```text
hover pick object あり
  -> object が attachment なら root avatar を取得
  -> object が avatar 本体ならその avatar を取得
  -> avatar が self なら self picker に委譲
  -> mouselook / avatar customize なら other picker を clear して何もしない
  -> avatar が他人でもデフォルトカメラ位置なら何もしない
  -> avatar が他人なら other picker を arm
```

対象 avatar が変わった場合、前の other ID buffer ready 状態は破棄する。

### 4.5 ID pass の描画対象を 1 avatar に限定

現在の self picker は次の条件で `gAgentAvatarp` 以外を skip する。

```cpp
if (info->mAvatar.get() != agent_avatar) skip;
```

other picker では、ここを指定 avatar に差し替えられるようにする。

```text
target_avatar = self mode ? gAgentAvatarp : mOtherRiggedPickerAvatar
if (info->mAvatar.get() != target_avatar) skip
```

描画対象は引き続き `PASS_*_RIGGED` に限定する。

### 4.6 ID encoding

self picker は LocalID だけで attachment tree を解決できる。

他人 avatar でも、対象 avatar を 1 人に限定するなら buffer 内の ID は LocalID のままでよい。ただし readback 後の lookup は **armed 時に記録した対象 avatar の attachment tree だけ** を探索する。

対象 avatar が変わった場合、古い buffer の readback は無効にする。

### 4.7 右クリック時の解決

`FSSelfRiggedPicker::findClosestAttachment()` をそのまま拡張するより、責務を分ける。

現在の分離:

```text
FSSelfRiggedPicker::findClosestSelfAttachment(...)
FSSelfRiggedPicker::findClosestAttachmentForAvatar(target_avatar, ...)
```

右クリック時の流れ:

```text
1. 既存 worldray pick を取得
2. self rigged なら既存 self picker
3. other avatar / other attachment なら other picker 条件判定
4. other ID buffer ready を先に確認し、not ready なら readback せず fallback
5. ready の場合だけ `glReadPixels(1x1)` で readback
6. local_id != 0 なら target avatar の attachment tree から object 解決
7. mPick を補正
8. LLToolSelect::handleObjectSelection() は補正後に 1 回だけ呼ぶ
9. readback 不成立なら既存 worldray pick をそのまま使う
```

selection handoff は r21 と同じく、stale selection を先に送らないことを必須条件にする。

デフォルトカメラ位置で other picker が not ready の場合は、通常どおり既存 worldray pick に fallback する。これは失敗扱いではなく、負荷抑制のための仕様とする。

## 5. 負荷対策

### 5.1 常時全員描画を禁止

他人 avatar を対象にすると、混雑地では draw call と triangle が avatar 数に比例して増える。したがって初期実装では次を禁止する。

- 全 avatar を毎フレーム ID buffer に描く
- カメラ内 avatar 全員を候補にする
- nearby list 全員を事前描画する

### 5.2 draw call / triangle 上限

対象 avatar が 1 人でも、重い outfit では self picker 実測値を超える可能性がある。

初期実装では、pass ごとに draw call と triangle を数え、上限超過時は pass を中断して buffer を not ready にする。

現在の設定:

```text
FSOtherRiggedPickerMaxDrawCalls = 512
FSOtherRiggedPickerMaxTriangles = 800000
```

上限超過時は通常の worldray picker に fallback する。

実地確認では `160` だと重い avatar で `over_budget=1` になり、ID pass が ready まで進まなかったため、現在のテストビルドでは `512` に引き上げている。

### 5.3 armed window を短くする

他人 avatar picker は hover 中だけ必要になるため、self より短い armed window で始める。

現在の設定:

```text
FSOtherRiggedPickerArmSeconds = 1.0
```

連続 hover 中は更新されるが、カーソルを外したら速やかに止まる。

### 5.4 readback は右クリック時のみ

hover 中に毎フレーム readback しない。ID pass は buffer を準備するだけにし、`glReadPixels(1x1)` は右クリック時だけ実行する。

右クリック時でも、other ID buffer が ready でない場合は readback helper に入らない。デフォルトカメラ gate 中や armed window 外では、GPU readback を発生させず既存 worldray pick に戻す。

## 6. 検証項目

### 6.1 基本機能

- デフォルトカメラ位置でも、自分の rigged 服は既存 self picker で右クリック補正される。
- 他人 avatar の rigged 服を右クリックし、画面上でクリックした attachment の menu が出る。
- 他人 avatar の rigged 髪を右クリックし、髪 attachment が選択される。
- 他人 avatar の mesh body を右クリックし、body attachment が選択される。
- 他人 avatar の非 rigged jewelry は既存 worldray 挙動に戻る。
- land / object / HUD / self avatar の既存挙動が壊れない。

### 6.2 デフォルトカメラ gate

- ESC 後のデフォルトカメラ位置では、他人 avatar に hover しても other ID pass が走らない。
- ESC 後のデフォルトカメラ位置では、他人 avatar を右クリックしても other picker readback は使わず、既存 worldray pick に fallback する。
- `FSOtherRiggedPickerDebugLog=true` の場合、default camera では `hover ignored by default-camera gate` は状態遷移時に抑制出力され、`render ready` は出ない。
- カメラを orbit / zoom / pan してデフォルト位置から外した状態では、他人 avatar hover で other picker が armed される。
- カメラをデフォルト位置に戻すと、other picker の armed 状態が解除される。
- self avatar の右クリック補正は、デフォルトカメラ位置でも非デフォルトカメラ位置でも既存どおり動く。
- mouselook 中は、直前に other picker が armed されていても other ID pass は描画されず、右クリック readback も行われない。

### 6.3 対象 avatar 限定

- A さんに hover している間、B さんの attachment は ID buffer 解決されない。
- hover 対象を A さんから B さんへ移した場合、古い A さんの buffer readback は無効になる。
- 右クリック時に対象 avatar が消えていた場合、fallback する。
- avatar が imposter 化している場合、fallback する。

### 6.4 selection handoff

- 右クリック 1 回につき `LLToolSelect::handleObjectSelection()` が補正後に 1 回だけ呼ばれる。
- `Couldn't find object ... selected.` 警告が増えない。
- ObjectProperties 応答が selection と食い違わない。

### 6.5 負荷

混雑していない場所:

- other picker OFF
- other picker ON だが hover なし
- other picker ON / デフォルトカメラ位置で 1 avatar hover
- other picker ON / 非デフォルトカメラ位置で 1 avatar hover

混雑地:

- 5 人程度の avatar が視界内
- 20 人程度の avatar が視界内
- 重い mesh body / 髪 / 服の avatar に hover

計測する値:

- FPS
- main `AYAstorm` CPU
- GPU ID pass の draw calls
- GPU ID pass の triangles
- ID pass 実行頻度
- readback 成功/失敗
- fallback 回数

期待値:

- hover なしでは other ID pass が走らない。
- デフォルトカメラ位置では、他人 avatar hover 中でも other ID pass が走らない。
- hover 中でも対象 avatar は 1 人だけ。
- draw call / triangle 上限を超えた場合は fallback し、継続的な高負荷にならない。

## 7. ログ設計

検証ログは hidden setting の `FSOtherRiggedPickerDebugLog` で制御する。default は `false`。

```text
FSOtherRiggedPickerDebugLog = false
```

主なログ:

```text
armed avatar=<uuid> seconds=<n> generation=<n> target_changed=<0|1>
hover ignored by default-camera gate
render ready avatar=<uuid> generation=<n> draw_calls=<n> triangles=<n>
render failed avatar=<uuid> generation=<n> draw_calls=<n> triangles=<n> over_budget=<0|1> budget_draw_calls=<n> budget_triangles=<n>
right-click skipped not-ready avatar=<uuid> upstream=<uuid>
readback local_id=<id> avatar=<uuid> hit=<uuid>
readback empty avatar=<uuid> local_id=0
right-click redirected upstream=<uuid> resolved=<uuid> avatar=<uuid>
right-click fallback reason=<reason> avatar=<uuid> upstream=<uuid>
```

hover は発生頻度が高いため、default-camera gate の skip log は連続出力しない。詳細ログは hidden debug setting として残す場合も default off を維持する。

## 8. 検証結果

2026-05-19 の `feature/ayastorm-r28-other-rigged-picker` で、mouselook hard gate 追加後に以下を確認した。

```text
git diff --check
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild -project build-darwin-universal/Firestorm.xcodeproj -scheme ayastorm-bin -configuration Release -derivedDataPath build-darwin-universal/DerivedData ARCHS=arm64 ONLY_ACTIVE_ARCH=YES build
codesign --verify --deep --strict --verbose=2 build-darwin-universal/newview/Release/AYAstorm.app
```

結果:

- `git diff --check`: pass
- Release arm64 `ayastorm-bin`: `BUILD SUCCEEDED`
- `AYAstorm.app` codesign verify: pass

ビルド時に CoreSimulator / Xcode の warning と、一部 third-party static object の macOS version warning は出たが、r28 変更箇所の compile / link / bundle / codesign は完了している。

## 9. リスク

- 混雑地で hover 対象を頻繁に変えると、ID pass の対象切替が増える。
- 他人 avatar の outfit が非常に重い場合、1 avatar 限定でも負荷が大きい。
- LocalID 解決を target avatar に限定しないと、別 avatar の attachment と誤対応する可能性がある。
- alpha-blend で depth を書かない髪は、self picker と同じく完全には解決できない。
- hover pick が不安定なケースでは、right-click 時に buffer が ready でない可能性がある。
- debug log を有効にしたままだと hover 操作でログ量が増えるため、通常運用では default off のままにする。

## 10. 初期採用条件

初期実装を有効化する条件:

- master switch default は on。ただし hidden setting として残し、問題があれば即時 off に戻せるようにする。
- armed window は必須。other picker では `ArmedMode=false` の常時描画 fallback を持たない。
- self picker の既存挙動を変更しない。
- デフォルトカメラ位置では other picker を hover arm しない。
- 対象 avatar は常に 1 人。
- hover なしでは pass が走らない。
- draw call / triangle 上限を持つ。
- readback 失敗時は既存 worldray に戻る。
- selection handoff の二重送信を発生させない。

この条件を満たせない場合、他人 avatar rigged picker は常用機能としては入れない。
