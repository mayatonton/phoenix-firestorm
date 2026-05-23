> **Language / 言語 / 语言**: [English](./rigged-mesh-picker-gpu-buffer.md) · **日本語** · [中文](./rigged-mesh-picker-gpu-buffer.zh.md)

# Rigged Mesh Picker — 全 SL viewer 共通の構造的不具合を解決した GPU object-ID buffer 改修

**ステータス**: AYAstorm 実装済 (r21.1 — self pick / r28 — 他アバター pick)。LL viewer 派生 fork からの自由取込を歓迎します — PR 不要、必要なところだけ持っていってください。

**Reference commits** (`mayatonton/phoenix-firestorm` の `ayastorm-release` branch):

| commit | author | 役割 |
|---|---|---|
| `940b989ca5` (2026-05-14) | mayatonton (AYA) | r21.1 初期実装 — 上流 rigged ray-mesh を独立 fallback で差し替え |
| `f3c0829ea8` (2026-05-14) | mayatonton (AYA) | r21.1 M4.17 — `LLDrawInfo` 単位の `mFSPickerLocalID` で BoM hash collision 解消 |
| `d4fa807f00` (2026-05-14) | mayatonton (AYA) | r21.1 cleanup — CPU stage 廃止、GPU 一本化 (~746 行削減) |
| `34acea572f` (2026-05-15) | mayatonton (AYA) | r21 M5 — `FSSelfRiggedPickerGPU` default ON 切替 |
| `cd35ef4fd8` (2026-05-15) | **t-noami** | M6 — selection handoff 修正、AYA block 後の `handleObjectSelection()` 一回呼び出しに集約 |
| `556607465f` (2026-05-15) | **t-noami** | M7 — armed-window mode (hover 中のみ GPU pass) |
| `7aa18dde2e` (2026-05-19) | **t-noami** | r28 — 他アバター対応への拡張 |
| `c97c14a19d` / `3792ecf857` (2026-05-19/21) | **t-noami** | r28 — buffer-owner tracking、古いフレームの reject |
| `8e68f83ba9` (2026-05-20) | mayatonton (AYA) | r28 P0 fixup — cvar 統合、検証ログ除去 |

**詳細トレース**:
- [`docs/specs/ayastorm-r21-self-rigged-picker.md`](./ayastorm-r21-self-rigged-picker.md) (self picker 設計 + canary protocol)
- [`docs/specs/ayastorm-r21-picker-armed-mode.md`](./ayastorm-r21-picker-armed-mode.md) (armed-window perf gate)
- [`docs/specs/ayastorm-r28-other-rigged-picker.md`](./ayastorm-r28-other-rigged-picker.md) (他アバター拡張)
- [`docs/specs/ayastorm-r21-selection-handoff-investigation.md`](./ayastorm-r21-selection-handoff-investigation.md) (M6 handoff 修正)

---

## 1. TL;DR

ユーザーが **rigged mesh の装着物** (Mesh body / Mesh head に付ける髪・服・アクセサリ) を右クリックしたとき、LL 派生 viewer はすべて **CPU 側の bind-pose vertex データ** に対して当たり判定を試みます。結果、構造的に 3 つの破綻が起きます:

1. **スキニングずれ** — CPU vertex は rest pose、画面に見える GPU vertex は idle animation のスキニング後。差は典型的に数 cm、ポートレート距離では「見えてるメッシュにカーソルが当たらない」状態に。
2. **アルファ discard の貫通** — GPU の fragment shader で `discard` される髪・薄い布地の三角形は CPU 視点では「実体」のままなので、画面上は透過している場所をクリックしても透過していないものとして扱われ、見えているメッシュを「通り抜けて」奥のものを掴んでしまう。
3. **クローズアップでの miss** — 顔ポートレート級のズームでは、bind-pose の誤差が画面上のターゲットサイズより大きくなる。

AYAstorm は **GPU object-ID buffer** を可視シーンと同じ解像度・**同じスキニング行列**・**同じ alpha-discard path** で描画し、対象の `LocalID` (32-bit) を RGBA8 に pack、右クリックされた pixel を `glReadPixels(1, 1)` 一発で解決します。**画面上の pixel → 物体の identity** が構造的に pixel-perfect 一致。

結果として「**Add to SSS whitelist**」の右クリックフローが、ユーザーが見ている実際のメッシュを確実に掴めます — 髪・薄手の生地・Bento head パーツ・BoM body、どのズーム距離でも。CPU ray path の上に workaround を重ねるのではなく、レンダリングパイプライン側で構造的に解決した形になります。

## 2. 影響を受ける viewer

このバグは **AYAstorm 固有ではありません**。CPU ray-mesh intersection は Linden Lab の upstream viewer に元から存在するため、LL upstream 派生 viewer 全般が同じ code path を継承しています。

再現は viewer 種別に依存しません。Bento head + 任意のメジャーな rigged hair を装備したアバターをポートレート距離までズーム、見えている髪の毛をピンポイントで右クリック — 開くメニューはほぼ確実に「下の body」「head」「何もない」のいずれかになります。

## 3. 現象

CPU ray viewer すべてで見られる具体的なユーザー視点の破綻:

- **「髪が見えているのに、髪をクリックしているのに、メニューは服を選んだと言う」**
- 顔ズームで Bento head を右クリック → system avatar の bone が選ばれる
- 透過したシアー袖を右クリック → 下の body が掴まれる、穴あきキャップを右クリック → 後ろの髪が掴まれる
- 複数 prim 構成の BoM body (head + torso + hands が別 Mesh asset で同じ rig を共有) で個別を識別できない — picker は常に同じ prim を返すか、狙ったものを絶対に返さないか

ほとんどの viewer は「アバター丸ごと選択」という粒度の粗い選択で誤魔化していますが、(AYAstorm の Skin SSS whitelist フローのように) **mesh asset UUID そのもの** が必要な場合は使い物になりません。

## 4. 根本原因

### 4.1 上流 path

上流 `LLPipeline::lineSegmentIntersectInWorld()` は可視 object を walk して `LLViewerObject::lineSegmentIntersect()` を呼びます。rigged mesh の場合、最終的に world-space ray を **CPU 側の vertex buffer** に対してテストする実装になっています。その buffer に入っているのは **bind-pose (rest pose) 位置** — フレーム毎に skinning が再適用されることはありません。skinning は GPU 専用です。

```
[ user click ]
      ↓
LLPipeline::lineSegmentIntersectInWorld()
      ↓
LLViewerObject::lineSegmentIntersect()        ← CPU
      ↓
bind-pose vertex buffer に対する ray test       ← ずれ発生源
      ↓
最大 cm スケール誤差を伴う "hit" / "miss"
```

### 4.2 3 つの構造的破綻

| 破綻 | 原因 | 顕在化条件 |
|---|---|---|
| **スキニングずれ** | CPU bind-pose と GPU skinned-pose の不一致 | 常時非ゼロ、ポートレート以上のズームで顕在化 |
| **alpha-discard 貫通** | CPU はすべての三角形を opaque として扱う、GPU は fragment shader で `discard` | 髪、レース、メッシュタイツ、シアー生地、穴あきキャップ |
| **クローズアップ miss** | 画面上のターゲットサイズに対して誤差が相対的に大きくなる | 顔・手・細かいアクセサリの近接撮影 |

### 4.3 identity 問題 (BoM mesh hash collision)

別問題ですが picker が壊れる要因として加算: AYAstorm 初期実装で picked rig の識別に `LLMeshSkinInfo::mHash` (rig の skinning hash) を使ったところ、BoM body では複数の異なる Mesh asset (head / torso / hands) が **同じ rig hash を共有** することが判明。hash 単一 key の map では全部が単一 identity に collapse、picker でそれらを区別できませんでした。

## 5. 修正

### 5.1 アーキテクチャ

```
[ user hover ]
      ↓
armed window 開始 (~150ms)
      ↓
armed 中の各可視フレームで:
  FBO bind → ID buffer (WorldViewRectRaw 解像度、RGBA8)
  対象 avatar の各 rigged DrawInfo について:
    fsObjectIDV.glsl / fsObjectIDF.glsl を bind
    skinning matrix palette を upload  ← 可視シーンと同一 matrix
    object_id_packed = pack32(LocalID) を upload  ← uniform vec4 ([0,1] byte)
    同じ VBO・同じ depth test・同じ alpha-discard で draw
      ↓
[ user right-click ]
      ↓
fsselfriggedpicker.cpp::readObjectIDBufferLocalID(x, y)
  scaled → raw pixel (HiDPI 補正)
  window → buffer-local (WorldViewRect offset)
  glReadPixels(1, 1, RGBA, UNSIGNED_BYTE)
  unpack: id = b0 | b1<<8 | b2<<16 | b3<<24
      ↓
findAttachmentOnAvatarByLocalID(target_avatar, id)  ← avatar scope
      ↓
LLViewerObject* → メニュー、SSS.Add、…
```

### 5.2 Encoding (pipeline.cpp:10796–10800)

```cpp
F32 r = ((id >>  0) & 0xff) / 255.f;
F32 g = ((id >>  8) & 0xff) / 255.f;
F32 b = ((id >> 16) & 0xff) / 255.f;
F32 a = ((id >> 24) & 0xff) / 255.f;
gFSObjectIDShader.uniform4f(sObjectIDPacked, r, g, b, a);
```

U32 LocalID を CPU 側で 4 byte に分割し `vec4` uniform で upload。fragment shader はその vec4 をそのまま書き込み。RGBA8 + filter 無しで byte が損失なく保持、`glReadPixels` で同じ順で取り戻せます。

### 5.3 DrawInfo 単位の identity (llvovolume.cpp:5840–5843, 5783)

BoM hash collision を解決するため、`LLDrawInfo` に prim 毎の `mFSPickerLocalID` を持たせ、DrawInfo 構築時に `LLViewerObject::getLocalID()` から stamp。batch merge ロジックに `mFSPickerLocalID` 一致条件を追加し、たまたま skinning hash が同じだけの 2 つの Mesh asset が **同一 batch に merge されない** よう保証します。

### 5.4 「画面と同じものを描く」保証

ID-buffer pass は以下を使用します:

- 可視シーンの rigged shader と同じ `getObjectSkinnedTransform()` GLSL helper
- 同じ `LLRenderPass::uploadMatrixPalette()` 経由の skinning matrix palette
- 同じ VBO、同じ depth test、同じ alpha-discard 分岐

画面に出ているものが、そのまま ID buffer に記録されます。second source of truth は存在しません。

### 5.5 マウス座標変換 (fsselfriggedpicker.cpp:80–116)

2 つの補正が必須かつどちらも忘れやすい:

1. **HiDPI**: logical (LLCoordGL) → raw pixel への変換、`DisplayScale = WindowWidthRaw / WindowWidthScaled` 使用
2. **UI chrome offset**: ID buffer は `WorldViewRectRaw` を覆う (window 全体ではない) ので `mLeft` / `mBottom` を引く

どちらかを忘れると `id == 0` (clear color) が返り、picker は無言で上流 worldray に fallback、§4 の全理由で失敗します。開発中の「GPU picker 効きません」報告のほとんどは、このどちらかの追跡でした。

## 6. なぜこれで解けるか

§4 の構造的破綻は ID buffer が可視シーンと同じ skinning・同じ depth・同じ alpha-discard で描かれるため消失します。

| 破綻 (§4) | ID buffer がどう解決するか |
|---|---|
| スキニングずれ | shader が可視シーンと同じ matrix palette で `getObjectSkinnedTransform()` を再実行 → ID pixel と color pixel が一致 |
| alpha-discard 貫通 | fragment `discard` が ID shader でも同じく走る → discard されたフラグメントは ID buffer に現れない、ユーザーは alpha hole を「通り抜けて」見えるものを画面通りに掴める |
| クローズアップ miss | pixel 解像度 = 可視 pixel 解像度、1 pixel 単位の精度 |

picker が renderer から幾何学的に独立する余地が一切無くなる — これが本質です。

## 7. パフォーマンス

「常に ID buffer を描く」素朴実装だと picker だけで ~130 rigged draw call/frame かかります。**armed window** (`FSSelfRiggedPickerArmedMode`、default ON / `FSSelfRiggedPickerArmSeconds`、default 短時間) は pass を **カーソルが avatar 上にあるフレームのみ** に制限:

- armed window 外: 追加 draw call ゼロ、picker は休眠
- armed window 内 (hover 中のみ): 対象 avatar 1 体分の ID-buffer pass を 1 フレーム = 通常 rigged pass 1 回と同等の draw count

r28 の他アバター拡張では buffer-owner field を持たせ、avatar A 用に作った buffer を avatar B のクリックで誤消費しないよう、古いフレームを reject。

## 8. 検証

最も視覚的に明快な証拠は ID buffer を disk に dump すること。アバターの各 mesh は固有色 (pack された `LocalID`) で表示され、clear された背景は黒です。

![Picker buffer](./images/picker/picker-dump.png)

生成した picker buffer の画像です。

再現手順:

1. Bento head + Mesh body + rigged hair + rigged dress を装備
2. ポートレート距離までズーム
3. 見える髪の毛を直接右クリック → **Add to SSS whitelist** (or 任意の rigged 対応右クリック動作)

   - **GPU pixel-accurate picker (AYAstorm)**: 髪が選ばれる、mesh asset UUID が追加される
   - **CPU ray-mesh-intersection picker (LL upstream 派生)**: body / head / "no object" のメニューが開く

4. シアー生地テスト: 穴あきキャップやレーストップの alpha 穴部分を右クリック

   - **GPU pixel-accurate picker**: 奥のオブジェクトが picked (ID buffer も `discard` されているため)
   - **CPU ray-mesh-intersection picker**: 透過しているにも関わらず cap / top が picked

## 9. この方式でしか実現できないこと — 髪の隙間から顔を掴む

| 髪 mesh を狙う | **髪越しに head mesh を狙う** |
|:---:|:---:|
| ![hair picked](./images/picker/pick-hair-front.png) | ![head picked through hair](./images/picker/pick-head-behind.png) |
| 髪の毛そのものをクリック → 髪 mesh が選択される (青 wireframe = 髪)。 | **髪の隙間から見えている顔の側面をクリック → click が髪を貫通して head mesh が選択される** (青 wireframe = head)。ray-mesh intersect 方式の picker では髪 mesh の三角形で click が止まるため、従来は頭を掴むのに髪を一度外す手間が必要だった領域。 |

picker buffer を可視シーンとまったく同じ alpha-discard で描画している副産物として、**透過のあるメッシュは picker からも透過します**:

- 髪のフリンジ越しに見えている顔の横 → そのまま顔が掴める
- レース / シアー生地越しに見えている肌 → 肌が掴める
- フープピアスやリングの穴を通して見えている耳 → 耳が掴める

**ray-mesh intersect 方式 picker との挙動差** (透過 mesh の奥):

| picker 方式 | 透過 mesh の奥 |
|---|---|
| ray-mesh intersect | ray が最前面 mesh の **三角形** に当たって停止。命中点の **テクスチャ alpha** は picker から参照できないため、髪の隙間に見えている顔は「画面上は見えているが picker からは届かない」。 |
| GPU pixel-accurate (本実装) | picker buffer 自体が alpha-discard で **穴が開いた状態**で描かれている。可視シーンで顔が見えている pixel は picker でも顔の色になっており、click は素直に顔へ到達する。 |

ray-mesh intersect 型は原理上「ray が mesh の三角形に当たったか」しか答えられず、当たった点の **テクスチャ alpha** までは参照しません。BoM body の肌 mesh は耳から肩・腰まで一枚の三角面シートとして張られているため、髪・服が同じ画面領域を占めているところでは「奥」に到達する経路が存在しない。

GPU で **可視シーンと完全に同じ shader を使って 1 枚 buffer を描く** という構造を取ったからこそ、副産物として得られている解像度です。

## 10. この修正が解決しないこと

- **non-rigged 装着物** (rigid prim accessory、単 prim earring) は上流 `lineSegmentIntersectInWorld` を引き続き使用。元々 rigged drift / discard 問題が無いため、GPU 化は churn にしかなりません。GPU buffer が click に対し `id == 0` を返した場合、picker は明示的に上流 worldray へ **fallback** します。
- **HUD attachment** は対象外。HUD は独自 RT に描画され、独自 pick path を持ちます。
- **`LLToolPie` による terrain / water / static prim の粗粒度選択** は不変。

## 11. 取り込み (Adoption)

最小取り込みセット:

| file | 役割 |
|---|---|
| `indra/newview/fsselfriggedpicker.{h,cpp}` | マウス座標変換、`glReadPixels`、scoped avatar walk |
| `indra/newview/pipeline.cpp` — `renderRiggedObjectIDBufferForAvatar()`、`renderSelfRiggedObjectIDBuffer()`、`renderOtherRiggedObjectIDBuffer()` | per-frame ID-buffer pass + armed-window gate |
| `indra/newview/llvovolume.cpp` — DrawInfo への `mFSPickerLocalID` stamp + merge guard | BoM body のための prim 単位 identity |
| `indra/newview/app_settings/shaders/class1/deferred/fsObjectIDV.glsl` | rigged vertex shader (`getObjectSkinnedTransform()` 再利用) |
| `indra/newview/app_settings/shaders/class1/deferred/fsObjectIDF.glsl` | fragment shader (`frag_color = object_id_packed` 一行) |
| `indra/newview/lltoolpie.cpp` — picker へのハンドラチェーン | 右クリック解決 |
| `indra/newview/llviewercontrol.cpp` + `settings.xml` | `FSSelfRiggedPickerGPU` / `FSSelfRiggedPickerArmedMode` / `FSSelfRiggedPickerArmSeconds` |

```sh
git remote add ayastorm https://github.com/mayatonton/phoenix-firestorm.git
git fetch ayastorm ayastorm-release

# GPU path + cleanup + identity fix を含む cherry-pick range
git log --oneline 940b989ca5^..8e68f83ba9 -- indra/newview/fsselfriggedpicker.cpp indra/newview/pipeline.cpp indra/newview/llvovolume.cpp indra/newview/app_settings/shaders/class1/deferred/fsObjectID*.glsl
```

upstream への PR 予定なし — 自分の都合で取り込んでください。

## 12. Attribution

- **mayatonton (AYA)**: self-picker 初期設計、GPU 置き換えアーキテクチャ、CPU stage 廃止、BoM identity fix (M4.17)、default ON 切替、r28 P0 cleanup
- **t-noami**: M6 selection handoff 修正、M7 armed-mode perf gate、r28 他アバター拡張、buffer-owner tracking

---

## License

本ドキュメントおよび `ayastorm-release` 内のリファレンス実装は Phoenix-Firestorm / Linden Lab viewer と同じライセンス (LGPL v2.1) で公開しています。
