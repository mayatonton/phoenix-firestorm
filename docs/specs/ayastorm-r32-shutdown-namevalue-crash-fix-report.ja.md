# AYAstorm r32 shutdown NameValue crash 修正報告書

## 目次

- [要約](#要約)
- [対象](#対象)
- [発生していた問題](#発生していた問題)
- [原因](#原因)
- [修正内容](#修正内容)
- [GPU picker への影響](#gpu-picker-への影響)
- [検証結果](#検証結果)
- [採用しなかった対応](#採用しなかった対応)
- [補足: 調査根拠](#補足-調査根拠)

## 要約

AYAstorm 7.2.4.81349 の macOS release app 終了時に、C++ static object の破棄中に `LLStringTable::removeString()` でクラッシュしていた。

原因は、r28 で追加された other-avatar rigged GPU picker が、最後に hover した他人アバターを static `LLPointer<LLVOAvatar>` として保持し続ける経路にある。これにより avatar/object/NameValue の破棄がプロセス終了時まで遅れ、NameValue 用 string table が先に破棄された後で `LLNameValue` が `removeString()` を呼ぶ可能性があった。

修正は `indra/newview/pipeline.cpp` のみ。other picker の target avatar 参照を、期限切れ時と viewer cleanup 時に明示的に clear するようにした。

Release ビルドは成功済み。調査用ログは最終コードから削除済み。

## 対象

| 項目 | 内容 |
| --- | --- |
| 対象アプリ | AYAstorm 7.2.4.81349 macOS release app |
| ブランチ | `fix/ayastorm-r32-shutdown-namevalue-crash` |
| 変更ファイル | `indra/newview/pipeline.cpp` |
| 主な対象機能 | other-avatar rigged GPU picker の終了時クリーンアップ |

## 発生していた問題

アプリ終了後、macOS のクラッシュレポートが表示された。

クラッシュは通常操作中の描画や UI 処理ではなく、終了処理で `exit()` に入り、C++ static object が破棄されるタイミングで起きていた。

クラッシュレポート上の主要経路:

```text
exit()
__cxa_finalize_ranges
LLPointer<LLVOAvatar>::~LLPointer()
LLVOAvatar::~LLVOAvatar()
LLViewerJointAttachment::~LLViewerJointAttachment()
LLPointer<LLViewerObject>::~LLPointer()
LLViewerObject::~LLViewerObject()
LLNameValue::~LLNameValue()
LLStringTable::removeString()
```

`LLNameValue` はアバターや装着物の属性情報に使われる。今回の問題は NameValue の通常利用ではなく、終了時に NameValue string table と avatar/object の破棄順が逆転したことによるクラッシュ。

## 原因

other-avatar rigged GPU picker は、最後に hover した他人アバターを次の static 変数で保持する。

```cpp
LLPointer<LLVOAvatar> sFSOtherRiggedPickerAvatar;
LLUUID sFSOtherRiggedPickerAvatarID;
```

修正前は、GPU picker の arm window が期限切れしても `sFSOtherRiggedPickerAvatar` は clear されなかった。

そのため、GPU ID pass は止まっていても、最後に hover した target avatar への `LLPointer` 参照だけが残り続ける状態になっていた。

この参照が viewer cleanup 後、C++ static destruction まで残ると、次の順序でクラッシュ条件が成立する。

1. NameValue 用の global static string table が破棄される。
2. その後で `sFSOtherRiggedPickerAvatar` が破棄される。
3. `LLVOAvatar` の destructor から attachment object が破棄される。
4. attachment object の `LLNameValue` が `LLStringTable::removeString()` を呼ぶ。
5. すでに string table 内部配列が NULL 化されているため、NULL ベース参照で落ちる。

クラッシュアドレス `0x0000000000000180` は、`LLStringTable` の内部配列 `mStringList` が NULL の状態で bucket 参照した場合の低アドレスアクセスと整合する。

## 修正内容

修正は、原因である stale target avatar 参照を解放する最小変更にした。

1. `LLPipeline::cleanup()` の先頭で `clearOtherRiggedObjectIDBuffer()` を呼ぶ。
2. `isOtherRiggedObjectIDBufferArmed()` で arm window の期限切れを検出したとき、`sFSOtherRiggedPickerAvatar` と `sFSOtherRiggedPickerAvatarID` を clear する。

これにより、hover が外れて `FSOtherRiggedPickerArmSeconds` を過ぎた後は、最後の target avatar 参照が残らない。

また、他人アバターを hover している最中、または hover 直後の arm window 内に終了した場合でも、viewer cleanup の段階で static avatar 参照を落とせる。

調査中は一時的に `FSOtherRiggedPicker` ログを追加して挙動を確認したが、最終コードからは削除済み。`LL_INFOS("FSOtherRiggedPicker")`、`LL_DEBUGS("FSOtherRiggedPicker")`、reason 付き `clearOtherRiggedObjectIDBuffer()` API は残していない。

## GPU picker への影響

通常の GPU picker 挙動は変更しない。

| 状態 | 修正前 | 修正後 |
| --- | --- | --- |
| other avatar hover 中 | arm timer が更新され GPU ID pass が走る | 同じ |
| hover を外した直後 | default 1 秒以内は GPU ID pass が走り得る | 同じ |
| arm window 期限切れ後 | GPU ID pass は止まるが target avatar 参照は残る | GPU ID pass が止まり target avatar 参照も clear |
| hover 中または hover 直後に終了 | static avatar 参照が終了時まで残り得る | viewer cleanup で clear |

GPU 負荷が上がり続ける問題ではない。arm window を過ぎると GPU ID pass は走らない。

メモリも増え続ける問題ではない。保持される static `LLPointer` は 1 本なので、残るのは最後に hover した target avatar 参照だけ。ただし、その 1 本が avatar/object/NameValue の破棄を遅らせ、今回の終了時クラッシュ条件を作っていた。

self picker は今回の修正対象ではない。self picker は `gAgentAvatarp.get()` をその場で使っており、追加の static `LLPointer<LLVOAvatar>` で self avatar を延命する構造ではない。

## 検証結果

| 検証 | 結果 |
| --- | --- |
| other GPU picker を拾いながら終了 | cleanup 中に other picker avatar 参照が clear されることを一時ログで確認 |
| 終了到達 | `cleanup : Goodbye!` と `status: stopped` まで到達 |
| macOS crash report | 同じ検証終了で新規 crash report なし |
| 実装ログ | 調査用の `FSOtherRiggedPicker` ログは最終コードから削除済み |

## 採用しなかった対応

`LLNameValue::~LLNameValue()` 側で string table の破棄済み状態を判定する案は採用しなかった。クラッシュ点に近い防御ではあるが、今回の直接原因である other picker の avatar retention を解消しないため。

`LLStringTable::removeString()` に `mStringList == NULL` の guard を入れる案も採用しなかった。同様に、破棄済み string table に触るほど avatar/object destruction が遅れていること自体を解消しないため。

`sFSOtherRiggedPickerAvatar` を `LLPointer<LLVOAvatar>` から `LLUUID` + weak lookup に変える案も今回は採用しなかった。より根本的ではあるが、picker 実行中に対象 avatar が消えた場合の扱いを関連 call site 全体で整理する必要があり、今回の終了時 crash 修正としては影響範囲が広い。

## 補足: 調査根拠

### other picker が static avatar 参照を持つ根拠

`armOtherRiggedObjectIDBuffer()` は hover で取得した avatar を static `LLPointer` に保存する。

```cpp
sFSOtherRiggedPickerAvatar = avatar;
sFSOtherRiggedPickerAvatarID = avatar->getID();
sFSOtherRiggedPickerArmSeconds = seconds;
sFSOtherRiggedPickerArmTimer.reset();
```

呼び出し元は `lltoolpie.cpp` の hover path。

```cpp
gPipeline.armOtherRiggedObjectIDBuffer(avatar, (F32)arm_seconds);
```

設定は default ON。

```xml
<key>FSOtherRiggedPickerEnable</key>
<integer>1</integer>
<key>FSOtherRiggedPickerGPU</key>
<integer>1</integer>
<key>FSOtherRiggedPickerArmSeconds</key>
<real>1.0</real>
```

### 修正前に期限切れしても参照が残る根拠

修正前の `isOtherRiggedObjectIDBufferArmed()` は、期限切れ時に false を返すだけだった。

```cpp
if (sFSOtherRiggedPickerArmSeconds <= 0.f ||
    sFSOtherRiggedPickerArmTimer.getElapsedTimeF32() > sFSOtherRiggedPickerArmSeconds)
{
    return false;
}
```

この path では `sFSOtherRiggedPickerAvatar = nullptr` が実行されない。

### hover を外した後の GPU pass

描画側は `renderDeferredLighting()` で armed 状態を見て other pass を走らせる。

```cpp
if (!gCubeSnapshot && isOtherRiggedObjectIDBufferArmed())
{
    renderOtherRiggedObjectIDBuffer();
}
```

そのため、hover を外した直後でも最後の arm から `FSOtherRiggedPickerArmSeconds` の間は GPU ID pass が走り得る。default 1 秒経過後は `isOtherRiggedObjectIDBufferArmed()` が false になり、GPU ID pass は走らない。

### self picker との差分

self picker state は timer/seconds/generation のみで、`static LLPointer<LLVOAvatar>` を持たない。

```cpp
LLFrameTimer sFSSelfRiggedPickerArmTimer;
F32 sFSSelfRiggedPickerArmSeconds = 0.f;
U32 sFSSelfRiggedPickerArmGeneration = 0;
U32 sFSSelfRiggedPickerRenderGeneration = 0;
```

self picker の描画と readback は、その場の `gAgentAvatarp.get()` を使う。

```cpp
renderRiggedObjectIDBufferForAvatar(gAgentAvatarp.get(), 0, 0)
findAttachmentOnAvatarByLocalID(gAgentAvatarp.get(), local_id)
```

このため、self picker は今回の static avatar retention 経路ではない。

### 一時検証ログ

avatar UUID は伏せる。ログ出力は最終コードから削除済み。

```text
FSOtherRiggedPicker arm avatar_id=<masked> seconds=1 generation=1
FSOtherRiggedPicker expire_clear avatar_id=<masked> elapsed=1.02353 arm_seconds=1
FSOtherRiggedPicker arm avatar_id=<masked> seconds=1 generation=2
FSOtherRiggedPicker expire_clear avatar_id=<masked> elapsed=1.00311 arm_seconds=1
FSOtherRiggedPicker arm avatar_id=<masked> seconds=1 generation=5
User requested quit
Setting app state to QUITTING
shutdownGL : Cleaning up pipeline
FSOtherRiggedPicker clear reason=pipeline_cleanup was_armed=false avatar_id=<masked> had_avatar=true elapsed=0.990032 arm_seconds=1
cleanup : Goodbye!
status: stopped
```

このログにより、終了直前に other picker が arm され、`sFSOtherRiggedPickerAvatar` が残った状態で `LLPipeline::cleanup()` に入り、`clearOtherRiggedObjectIDBuffer()` で参照が解放されたことを確認した。
