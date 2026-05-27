# AYAstorm r34 mouselook FPS investigation and fix plan

対象ブランチ: `feature/ayastorm-r34-mouselook-fps`

ベース: `origin/ayastorm-release` `be42559fde90e8d0ff474df3abfa56545042496e`

起票日: 2026-05-26

## 1. 問題

mouselook 中、特に視点操作中に FPS が低下する問題を r34 として扱う。

この問題は r21 self rigged picker 実装以前から認知されているため、r21/r28 の GPU picker は主因候補から外す。r34 では Firestorm 既存の mouselook / input / attachment update / first-person avatar render 経路を対象にする。

## 2. 現時点の静的確認

### 2.1 self rigged picker は主因候補から外す

`FSSelfRiggedPickerGPU` / `FSOtherRiggedPicker*` は r21/r28 以降の機能であり、既存問題の時系列と合わない。

ただし、r34 修正後の回帰確認では以下を確認する。

- 三人称で self rigged picker が壊れていない
- 他人 avatar picker が mouselook / customize 中に skip される
- picker の追加 pass が mouselook FPS 問題の説明になっていない

### 2.2 `lldrawable.cpp` の mouselook undamped 分岐は表示制御ではない

該当箇所:

- `indra/newview/lldrawable.cpp`
- `LLDrawable::updateMove()`
- 条件: `!isRoot() && gAgentCamera.cameraMouselook() && !mVObjp->isRiggedMesh() && mVObjp->getAvatar() && mVObjp->getAvatar()->isSelf()`

この分岐は mouselook 中の self 非 rigged attachment child drawable を `updateMoveUndamped()` へ送るだけで、attachment の表示/非表示を決めない。

表示制御は以下で行われる。

- `LLVOAvatarSelf::updateAttachmentVisibility()`
- `LLViewerJointAttachment::setAttachmentVisibility()`
- `avatar_lad.xml` の `visible_in_first_person`

したがって「まつげアイテムが mouselook 中に見える」現象は、この undamped 分岐そのものでは説明しない。

ただし、見えていない self 非 rigged child attachment でも moved list に載れば transform update 負荷が発生し得るため、FPS 低下候補としては残す。

### 2.3 eye / face eye attachment point は first-person 非表示

`avatar_lad.xml` 上では以下が `visible_in_first_person="false"`:

- `ATTACH_LEYE`
- `ATTACH_REYE`
- `ATTACH_FACE_LEYE`
- `ATTACH_FACE_REYE`

まつげが見える場合は、次のどれかを疑う。

- 別の first-person visible attachment point に装着されている
- HUD attachment
- rigged mesh として avatar render 側に乗っている
- visibility refresh / shadow / mirror pass など別経路

## 3. 実測メモ

測定環境:

- Viewer: AYAstorm release r31.1 相当 (`Firestorm-AYAstorm-release 7.2.4.81209`)
- 場所: `SKYHIGH!! Biomesia Isle - The Blazer Club`
- 条件: 同一 outfit / 同一車両 / shadows OFF
- 計測: Statistics floater の FPS を画面読み取り

### 3.1 mouselook 一般ではなく、車両 seated mouselook が問題

| Case | FPS | Notes |
| --- | ---: | --- |
| 車外 third-person idle | 約 20.4 | 降車後 |
| 車外 mouselook idle | 約 27.5 | 降車後。mouselook でむしろ上昇 |
| 車外 mouselook + mouse move | 約 26.0 | input / cursor warp 主因ではなさそう |
| 車内 third-person idle | 約 20.4-21.6 | 乗車中 |
| 車内 mouselook idle | 約 17.1-17.9 | 乗車中に低下 |
| 車内 mouselook + mouse move | 約 14.8-15.4 | 視点移動でさらに低下 |

この測定では、mouselook そのものや mouse input / cursor warp だけでは低下を説明できない。車両に座った状態の first-person 視界、特に近距離に入る車内描画が主因候補。

### 3.2 FirstPersonAvatarVisible は主因ではない

| Case | FirstPersonAvatarVisible | FPS |
| --- | --- | ---: |
| 車内 mouselook idle | ON | 約 17.1 |
| 車内 mouselook + mouse move | ON | 約 14.8 |
| 車内 mouselook idle | OFF | 約 17.9 |
| 車内 mouselook + mouse move | OFF | 約 15.4 |

差は約 0.6-0.8 FPS 程度。self avatar / self attachment 表示は補助要因の可能性はあるが、主因ではない。

### 3.3 Alpha render type は主因ではない

`Ctrl+Alt+Shift+2` で Alpha rendering type を OFF。

| Case | FPS |
| --- | ---: |
| 車内 mouselook idle 通常 | 約 17.5 |
| 車内 mouselook idle, Alpha OFF | 約 16.0-17.3 |

Alpha OFF で改善しないため、透明描画 pass 単体は主因から外す。

### 3.4 Volume render type が支配的

`Ctrl+Alt+Shift+9` で Volume rendering type を OFF。

| Case | FPS |
| --- | ---: |
| 車内 mouselook idle 通常 | 約 17.5 |
| 車内 mouselook idle, Volume OFF | 約 32.3-36.5 |
| Volume ON に戻した直後 | 約 16.0 |

Volume OFF で車内モデルが消え、FPS がほぼ倍まで戻る。現時点の本命は、車内 mouselook で近距離に入る車両 interior / attachment volume geometry の描画負荷。

## 4. 主な調査候補

### A. mouse input / cursor warp 経路

優先度: 低

車外 mouselook + mouse move で FPS が大きく落ちないため、主因候補からは下げる。車内 mouselook + mouse move で追加低下はあるので、二次要因として残す。

対象:

- `LLViewerWindow::moveCursorToCenter()`
- `LLViewerWindow::updateMouseDelta()`
- macOS `LLWindowMacOSX::setCursorPosition()`
- macOS `CGWarpMouseCursorPosition()`
- macOS cursor decouple / delta event

確認すること:

- 1 frame あたりの `handleMouseMove()` 回数
- 1 frame あたりの cursor warp 回数
- warp が synthetic mouse move を増幅していないか
- `MouseWarpMode` の違いで FPS が変わるか
- `MouseSmooth` の ON/OFF で FPS が変わるか

### B. self non-rigged attachment child drawable の undamped update

優先度: 低から中

対象:

- `LLDrawable::updateMove()`
- `LLDrawable::updateMoveUndamped()`
- `LLPipeline::updateMovedList()`
- `LLPipeline::markMoved()`

確認すること:

- mouselook 中、この分岐が何回 hit するか
- root / child の内訳
- rigged / non-rigged の内訳
- visible-in-first-person attachment point か
- hidden attachment point でも更新されているか
- linkset child prim が多い outfit で frame time が増えるか

修正候補:

- mouselook 中でも非表示 attachment point の child drawable は undamped 強制しない
- first-person visible attachment point のみに限定
- `LLDrawable::INVISIBLE` / spatial bridge type / attachment visibility を見て早期 return できるか検討

注意:

この分岐は「animated parts の attachment が mouselook で壊れる」ことを防ぐための古い修正なので、単純削除は避ける。

### C. seated first-person の近距離 Volume 描画

優先度: 最優先

対象:

- `LLPipeline::renderGeomDeferred()`
- `LLPipeline::renderGeomPostDeferred()`
- `LLRenderPass::pushBatches()`
- `LLRenderPass::pushRiggedBatches()`
- `LLRenderPass::PASS_SIMPLE`
- `LLRenderPass::PASS_FULLBRIGHT`
- `LLRenderPass::PASS_SHINY`
- `LLRenderPass::PASS_BUMP`
- `LLRenderPass::PASS_FULLBRIGHT_SHINY`
- rigged variants

確認すること:

- 車内 mouselook で visible group / drawinfo / triangle count が増える pass
- `Ctrl+Alt+Shift+9` の Volume OFF で消える pass の内訳
- 車両 root object / child prim / seated attachment / self attachment のどれが draw call を支配しているか
- mouselook camera near plane / frustum / occlusion で車内モデルが過剰に可視化されていないか
- close-up interior の shiny / bump / fullbright / reflection 相当 pass が膨らんでいないか

修正候補:

- seated mouselook 中、視点に極近い車両 interior volume を安くする条件を作れるか検討
- vehicle / attachment の owner・seat・camera relation を見て、first-person で不要な interior faces を抑制できるか検討
- 既存 object rendering semantics を壊さないため、まずは pass 別計測を入れて原因 object/pass を確定する

### D. FirstPersonAvatarVisible + shadows

優先度: 低

対象:

- `FirstPersonAvatarVisible`
- `LLPipeline::generateSunShadow()`
- `LLVOAvatarSelf::updateAttachmentVisibility(CAMERA_MODE_THIRD_PERSON)`

確認すること:

- `FirstPersonAvatarVisible=0/1` で FPS 差があるか
- shadows OFF/ON で FPS 差があるか
- shadow pass 中に first-person では本来非表示の attachments が一時復活していないか

修正候補:

- shadow pass で全 attachment を third-person 相当に戻す範囲を狭める
- first-person visible attachment のみに限定できるか検討

### E. mouselook UI / overlay / combat features

優先度: 低から中

対象:

- `FSShowInterfaceInMouselook`
- `FSShowStatsBarInMouselook`
- `FSMouselookCombatFeatures`
- `ExodusMouselookIFF`
- `ShowCrosshairs`

確認すること:

- すべて OFF にした場合の FPS
- stats floater 表示時だけ frame time が増えるか
- combat target marker が avatar 多数環境で増えるか

## 5. 計測方針

まず修正を入れず、軽い instrumentation で実態を見る。

候補:

- `LL_PROFILE_ZONE_NAMED` を hot path に追加
- debug setting で一時 trace を gate
- 1 秒集計のカウンタを `LL_DEBUGS` に出す

最小カウンタ:

- `MouselookVolumeVisibleGroupsPerSec`
- `MouselookVolumeDrawInfoCountPerSec`
- `MouselookVolumeTriangleCountPerSec`
- `MouselookVolumePassSimpleCountPerSec`
- `MouselookVolumePassFullbrightCountPerSec`
- `MouselookVolumePassShinyCountPerSec`
- `MouselookVolumePassBumpCountPerSec`
- `MouselookVolumePassFullbrightShinyCountPerSec`
- `MouselookMouseMoveEventsPerSec`
- `MouselookCursorWarpsPerSec`

出荷前には検証用ログ / cvar を削除する。

## 6. 再現マトリクス

同一場所、同一 outfit、同一 graphics preset で比較する。

| Case | Camera | Mouse move | FirstPersonAvatarVisible | Shadows | Notes |
| --- | --- | --- | --- | --- | --- |
| A0 | third person | no | off | current | baseline |
| A1 | mouselook | no | off | current | mode entry cost |
| A2 | mouselook | continuous | off | current | input / warp cost |
| B1 | mouselook | continuous | on | current | first-person avatar cost |
| B2 | mouselook | continuous | on | off | shadow interaction |
| C1 | mouselook | continuous | off | current | `MouseSmooth=0` |
| C2 | mouselook | continuous | off | current | `MouseWarpMode` variants |
| D1 | mouselook | continuous | off | current | minimal attachments |
| D2 | mouselook | continuous | off | current | heavy non-rigged linkset attachments |
| E1 | seated vehicle mouselook | no | off | off | vehicle interior baseline |
| E2 | seated vehicle mouselook | no | off | off | Alpha rendering type OFF |
| E3 | seated vehicle mouselook | no | off | off | Volume rendering type OFF |

## 7. 受け入れ条件

- r34 branch 上で原因候補を少なくとも 1 つ実測で確定または除外する
- mouselook 中の FPS 低下が改善する、または主要因が明確に特定される
- self attachment の mouselook 表示挙動を不要に壊さない
- animated non-rigged attachment の mouselook 破綻を再発させない
- `FirstPersonAvatarVisible` OFF の既定挙動を維持する
- r21/r28 picker の既存挙動を壊さない

## 8. 次の初手

1. `LLPipeline::renderGeomDeferred()` / `renderGeomPostDeferred()` に pass 別・render type 別の一時カウンタを入れる。
2. seated mouselook 中だけ 1 秒単位で visible groups / drawinfos / triangles / pass counts をログ出力する。
3. `Ctrl+Alt+Shift+9` で消える pass の内訳を r31.1 実測と突き合わせる。
4. 必要なら drawinfo から source object / avatar / attachment / vehicle owner を辿れる範囲で owner 分類を追加する。
5. 修正は、原因 pass/object が確定してから最小範囲で行う。

## 9. r34 計測ビルド変更

追加した一時 Debug setting:

- `AYAR34MouselookVolumeTraceEnabled`
- Type: Boolean
- Persist: 1
- Default: true in the local r34 test build
- `AYAR34MouselookTopSourceTraceEnabled`
- Type: Boolean
- Persist: 1
- Default: false
- `AYAR34MouselookSuppressChildLocalID`
- Type: U32
- Persist: 1
- Default: 0

ON の間だけ、`LLRenderPass` の batch push 経路で mouselook 中の drawinfo / triangle / pass 分類を 1 秒ごとに `AYAR34MouselookFPS` ログへ出す。通常時は無効。

`AYAR34MouselookTopSourceTraceEnabled` は `world_top_sources` / `world_top_children` の詳細集計を追加する。これは drawinfo ごとの source object 分類と 1 秒ごとの sort / 詳細ログ生成を伴うため、suppression-only FPS 検証では false のままにする。

`AYAR34MouselookSuppressChildLocalID` は検証用の描画抑制スイッチ。非 0 の場合、mouselook 中だけ該当 source child local ID の world volume drawinfo を skip する。対象は world volume のみで、avatar / attachment drawinfo は対象外。skip 量は `suppressed_draw_infos` / `suppressed_triangles` に出す。

r34 suppression は live 3D Stream binding に参加している root / speaker / media source prim を対象外にする。`source:media` は media 面と plugin audio ring の更新に依存するため、draw pass 抑制の検証で巻き込まない。

計測対象:

- `LLRenderPass::pushBatches()`
- `LLRenderPass::pushUntexturedBatches()`
- `LLRenderPass::pushRiggedBatches()`
- `LLRenderPass::pushUntexturedRiggedBatches()`
- `LLRenderPass::pushMaskBatches()`
- `LLRenderPass::pushRiggedMaskBatches()`

ログ項目:

- `draw_infos`
- `triangles`
- `rigged_draw_infos`
- `world_draw_infos`
- `self_rigged_draw_infos`
- `self_attachment_draw_infos`
- `other_avatar_draw_infos`
- `suppressed_draw_infos`
- `suppressed_triangles`
- pass group 別 `draw_infos/triangles`
- `world_top_sources`: `AYAR34MouselookTopSourceTraceEnabled=TRUE` の時だけ、world volume drawinfo を root object 単位で triangle 降順 top 5
- `world_top_children`: `AYAR34MouselookTopSourceTraceEnabled=TRUE` の時だけ、world volume drawinfo を source child prim 単位で triangle 降順 top 8

pass group:

- `simple`
- `fullbright`
- `shiny`
- `bump`
- `fb_shiny`
- `alpha_mask`
- `material`
- `pbr`
- `glow`
- `alpha`

`world_top_sources` / `world_top_children` の項目:

- `root_local` / `root_id`: 集約対象の root object
- `owner`: root object owner
- `root_dist_m`: camera から root までの概算距離
- `root_forward_m`: camera forward axis 上の root 位置。負なら camera 後方
- `root_dot`: camera forward と root 方向の dot。1 に近いほど正面、0 付近は横、負は後方
- `root_angle_deg`: camera 正面から root 方向までの角度
- `root_children`: root の child 数
- `root_mesh`: root が mesh object か
- `sample_local` / `sample_id`: その root 配下で最初に観測した source prim
- `sample_dist_m`: camera から sample prim までの概算距離
- `sample_forward_m`: camera forward axis 上の sample prim 位置。負なら camera 後方
- `sample_dot`: camera forward と sample prim 方向の dot
- `sample_angle_deg`: camera 正面から sample prim 方向までの角度
- `sample_mesh` / `sample_rigged`: sample prim の mesh / rigged mesh 判定
- `draw_infos` / `triangles`: 1 秒窓の集計
- `passes`: pass group 別 `draw_infos/triangles`

想定手順:

1. r34 計測ビルドを起動する。
2. Debug Settings で `AYAR34MouselookVolumeTraceEnabled=TRUE`。
3. suppression-only FPS 検証では `AYAR34MouselookTopSourceTraceEnabled=FALSE` のままにする。原因 object の再特定が必要な時だけ TRUE にする。
4. 車内 mouselook idle を 5 秒以上維持してログを採る。
5. 同じ状態で `Ctrl+Alt+Shift+9` により Volume OFF、ログを採る。
6. Volume ON に戻し、ログが通常値へ戻るか確認する。
7. `AYAR34MouselookVolumeTraceEnabled=FALSE` に戻す。

child 抑制実験:

1. `world_top_children` で対象 `sample_local` を決める。
2. Debug Settings で `AYAR34MouselookSuppressChildLocalID=<sample_local>`。
3. 同じ mouselook idle / rotate を採る。
4. `suppressed_triangles`、FPS、`world_top_children` の変化、車内表示破綻を確認する。
5. 実験後は `AYAR34MouselookSuppressChildLocalID=0` に戻す。

root pass 抑制実験:

1. `AYAR34MouselookSuppressChildLocalID=0` にする。
2. `world_top_sources` で対象 root を決める。
3. Debug Settings で `AYAR34MouselookSuppressRootLocalID=<root_local>`。
4. `AYAR34MouselookSuppressRootPassMode=1` にする。mode 1 は alpha-mask 系 pass のみを抑制する。
5. 同じ mouselook idle / rotate を採る。
6. `suppressed_triangles`、FPS、`world_top_sources` / `world_top_children` の変化、車内表示破綻を確認する。
7. 実験後は `AYAR34MouselookSuppressRootPassMode=0` に戻す。

root outer-cone 抑制実験:

1. `AYAR34MouselookSuppressChildLocalID=0` にする。
2. 固定 root で試す場合は `AYAR34MouselookSuppressRootLocalID=<root_local>` を対象車両 root にする。
3. `AYAR34MouselookSuppressRootPassMode=2` にする。
4. `AYAR34MouselookSuppressRootOuterDot=<dot>` にする。初期値 `0.80` は約 37 度より外側の source object を対象にする aggressive test。
5. mode 2 は対象 root 配下で、source object が outer cone にある drawinfo の simple / fullbright / glow / alpha-mask 系 pass を skip する。
6. 同じ mouselook idle / rotate を採り、FPS と車内表示破綻を確認する。
7. 破綻が強い場合は `AYAR34MouselookSuppressRootOuterDot` を下げる。削減が少ない場合は上げる。

auto root outer-cone 抑制実験:

1. `AYAR34MouselookSuppressRootLocalID=0` にする。
2. `AYAR34MouselookSuppressAutoRootEnabled=TRUE` にする。
3. `AYAR34MouselookSuppressAutoRootMaxDistance=12.0`、`AYAR34MouselookSuppressAutoRootMinChildren=50` を初期値にする。
4. `AYAR34MouselookSuppressRootPassMode=2` と組み合わせる。
5. mouselook camera から近い large linkset を自動対象にするため、車両 root local ID が変わる場合や周辺の近距離 large root が top に出る場合を同時に検証できる。

判定:

- `world_draw_infos` と特定 pass group の triangles が車内 mouselook で支配的なら、車両 interior/world volume が本命。
- `world_top_sources` の上位 root が車両 root なら、車両 linkset 内の描画負荷として扱う。車内を壊さないため、root 丸ごとの非表示は避ける。
- `world_top_children` で同一 root 配下の重い `sample_local` を特定し、child 単位の LOD bias / pass 簡略化候補にする。
- `sample_angle_deg` が大きい、または `sample_forward_m` が負の source が支配的なら、視線外/後方寄り geometry の LOD bias や描画簡略化候補にする。
- `self_attachment_draw_infos` が支配的なら、self attachment 表示/更新側へ戻って調査する。
- `pbr` / `material` / `bump` / `shiny` のどれが膨らむかで、次の修正候補を決める。

## 10. r34 計測結果メモ

2026-05-26 の r34 計測ビルド実測では、車両 seated mouselook 中に Volume ON/OFF で大きな差が出た。

| Case | FPS | Notes |
| --- | ---: | --- |
| 車両 seated third-person idle | 約 31.5-36.8 | baseline |
| 車両 seated mouselook idle, Volume ON | 約 15.3-15.8 | `world_draw_infos` 約 5.5k-7.6k、triangles 約 5.8M-8.1M |
| 車両 seated mouselook rotate, Volume ON | 約 16.4 | triangles 最大約 9.2M |
| 車両 seated mouselook idle, Volume OFF | 約 31.5-32.2 | `world_draw_infos=0`、rigged drawinfo も 0 |

この結果から、Alpha/self 表示単体ではなく、車両 seated mouselook で視界に入る world volume geometry が主因候補。次の計測では `world_top_sources` で車両 root / child / pass の内訳を確定する。

追加実測:

- `AYAR34MouselookSuppressChildLocalID=685612198`: 後方 alpha child は抑制できたが、削減は約 0.08M triangles/sec で FPS 改善は限定的。
- `AYAR34MouselookSuppressChildLocalID=685612193`: 最大 simple child は抑制でき、削減は約 0.36M-0.42M triangles/sec。ただし FPS は約 24-25 で頭打ち。
- 単一 child 抑制では根本改善しないため、次は root `685612251` 配下の pass 単位簡略化を検証する。
- `AYAR34MouselookSuppressRootPassMode=1`: root `685612251` 配下の alpha-mask 系抑制は発火したが、削減は約 0.13M-0.17M triangles/sec。FPS は約 24-25 で、支配的な simple/fullbright/glow 側には届かない。
- 次の検証として `AYAR34MouselookSuppressRootPassMode=2` を追加。車内を丸ごと消さずに削れるか確認するため、root 配下かつ mouselook camera の outer cone にある source object の simple/fullbright/glow/alpha-mask pass を skip する。
- 固定 root `685612251` の mode 2 は約 0.48M-0.64M triangles/sec まで削減量が増えたが、FPS は約 24 前後で改善が薄い。別 root `685641002` が top に出たため、固定 root 方式では負荷の逃げを拾い切れない。
- 次の検証として auto root mode を追加。mouselook camera から `12m` 以内、かつ child 数 `50` 以上の nearby large linkset を mode 2 対象にする。

## 11. 追加提案検証: self head / head attachment CPU fast path

### 11.1 結論

提案の方向性は「局所的な CPU 削減候補」としては妥当。ただし、r34 の実測で見えている車両 seated mouselook の主因対策としては優先度を下げる。

理由:

- 既存コードは mouselook 中の self avatar/head 表示を既にかなり落としている。
- `FirstPersonAvatarVisible` の切替差は実測で約 0.6-0.8 FPS 程度に留まった。
- 車両 seated mouselook では `Volume` OFF で FPS が大きく戻り、r34 trace でも `world_draw_infos` / world triangles が支配的だった。
- shadow は今回の検証条件では既に OFF なので、`AYASkipSelfAvatarShadowInMouselook` 系は主因にはならない。
- ただし、hidden self head attachment が drawable rebuild / rigged skinning / probe / mirror 経路で残っている可能性はあるため、計測付きの段階実装対象にはできる。

したがって、r34 ではこの提案を Candidate F として扱う。最初から skeleton / animation / attachment transform を止める実装は不可。実装する場合は、既定 OFF の計測付き fast path として、まず attachment render/shadow/probe queue の除外から始める。

### 11.2 関連関数と呼び出し経路

camera mode から attachment visibility への経路:

- `LLAgentCamera::setCameraMode()` 周辺から `gAgentAvatarp->updateAttachmentVisibility(mCameraMode)` が呼ばれる。
- `LLVOAvatarSelf::updateAttachmentVisibility(U32 camera_mode)` は `mAttachmentPoints` を走査し、HUD は常に表示、それ以外は mouselook で `FirstPersonAvatarVisible && attachment->getVisibleInFirstPerson()` または mirror pass の時だけ表示する。
- `LLViewerJointAttachment::setAttachmentVisibility(bool visible)` は attached object の spatial bridge `mDrawableType` を、表示時は `RENDER_TYPE_VOLUME` / `RENDER_TYPE_HUD`、非表示時は `0` にする。これは local render queue から外すための既存フックで、inventory state や server state は変えない。

self avatar/head render への経路:

- `LLAgent::needsRenderAvatar()` は `cameraMouselook() && !LLVOAvatar::sVisibleInFirstPerson` で self avatar render を不要にする。
- `LLAgent::needsRenderHead()` は mouselook 中は基本 false。reflection render かつ `FirstPersonAvatarVisible` の場合などだけ true になる。
- `LLVOAvatar::renderSkinned()` / `renderTransparent()` / `renderRigid()` は `isSelf()` と `needsRenderAvatar()` / `needsRenderHead()` / `LLPipeline::sShadowRender` を見て、head / eyelash / hair / eyeball の描画を抑制している。
- `LLVOAvatar::updateOrientation()` には self mouselook 用の forward / pelvis follow 補正があるため、この周辺は最適化対象にしない。

drawable update / rebuild / skinning への経路:

- `LLDrawable::updateMove()` には「mouselook 中の self 非 rigged attachment child は undamped update にする」古い分岐がある。これは可視制御ではなく、attached linkset の動きの補正であり、まつげや face item が見える/見えない判定そのものには効かない。
- `LLVOVolume::updateGeometry()` は `REBUILD_RIGGED` や LOD / sculpt / material 変更から `updateRiggedVolume()`、`dirtySpatialGroup()`、`LLViewerObject::updateGeometry()` へ進む。
- `LLVOVolume::updateRiggedVolume()` / `LLRiggedVolume::update()` は skin matrix palette を作り、face vertex を skinning して bounding box も更新する。ここを止める場合は render 用 skinning だけに限定し、avatar skeleton / animation / attachment transform は止めない。
- `LLVolumeGeometryManager::rebuildGeom()` は spatial group の face list から `LLDrawInfo` を作る。rigged face は `facep->mAvatar` / `mSkinInfo` を持つ。
- `LLVolumeGeometryManager::addGeometryCount()` 周辺で作られる `LLDrawInfo` には r34 で `mFSPickerLocalID` と `mAttachedToAvatar` が入っている。これにより draw pass 側で self attachment と world volume を区別できる。
- `LLRenderPass::pushBatches()` / `pushRiggedBatches()` / `pushMaskBatches()` は `LLDrawInfo` を実際に push する。r34 の world volume suppression はここに入っているが、現在は `params.mAvatar.isNull() && params.mAttachedToAvatar.isNull()` の world volume だけを対象にしている。

shadow / reflection / probe への経路:

- `LLPipeline::generateSunShadow()` は mouselook かつ `FirstPersonAvatarVisible` の場合に、一時的に `updateAttachmentVisibility(CAMERA_MODE_THIRD_PERSON)` を呼び、shadow pass 用に attachment visibility を戻す。その後、元の camera mode の visibility に復帰する。
- `LLPipeline::renderGeomShadow()` は pool の shadow pass を描画する。
- `LLVOAvatarSelf::updateAttachmentVisibility()` は `gPipeline.mHeroProbeManager.isMirrorPass()` の場合、mouselook でも attachment を表示側にする。mirror/probe を変える場合は、通常視点・鏡・snapshot の差分確認が必要。

### 11.3 self / mouselook 判定箇所

安全な基本条件:

- `avatar->isSelf()`
- `gAgentCamera.cameraMouselook()`
- 新規 master setting が ON
- HUD attachment ではない
- `gRlvHandler.hasBehaviour(RLV_BHVR_SHOWSELF)` / `RLV_BHVR_SHOWSELFHEAD` を壊さない

既存判定:

- `LLVOAvatar::isSelf()` は self avatar 限定に使える。
- `LLViewerObject::getAvatar()` は attachment wearer を返す。r34 の `LLDrawInfo::mAttachedToAvatar` はここから入る。
- `LLViewerObject::isHUDAttachment()` / `LLVOVolume::isHUDAttachment()` は HUD 除外に使える。
- `LLViewerObject::getAttachmentState()` から `ATTACHMENT_ID_FROM_STATE(state)` で attachment point id を取得できる。

### 11.4 head attachment 判定方法

`avatar_lad.xml` では head/face 系 attachment point の多くが `visible_in_first_person="false"` になっている。既存データ上の明確な対象:

- id 2 `Skull`, joint `mHead`, group 2, first-person false
- id 11 `Mouth`, joint `mHead`, group 2, first-person false
- id 12 `Chin`, joint `mHead`, group 2, first-person false
- id 13 `Left Ear`, joint `mHead`, group 2, first-person false
- id 14 `Right Ear`, joint `mHead`, group 2, first-person false
- id 15 `Left Eyeball`, joint `mEyeLeft`, group 2, first-person false
- id 16 `Right Eyeball`, joint `mEyeRight`, group 2, first-person false
- id 17 `Nose`, joint `mHead`, group 2, first-person false
- id 47 `Jaw`, joint `mFaceJaw`, first-person false
- id 48 `Alt Left Ear`, joint `mFaceEar1Left`, first-person false
- id 49 `Alt Right Ear`, joint `mFaceEar1Right`, first-person false
- id 50 `Alt Left Eye`, joint `mFaceEyeAltLeft`, first-person false
- id 51 `Alt Right Eye`, joint `mFaceEyeAltRight`, first-person false
- id 52 `Tongue`, joint `mFaceTongueTip`, first-person false

注意点:

- id 39 `Neck` は `visible_in_first_person="true"` なので、head 関連として雑に除外しない。
- chest / hand / pelvis / root / avatar center は武器、カメラ、ライト、車両操作系 attachment があり得るので対象外。
- rigged mesh は attachment point だけでは分類が不足する。髪やまつげが別 attachment point に付いていて head joint に weight されている場合、attachment point classifier では拾えない。Phase 2 で扱うなら skin joint influence または face bounds を見る追加分類が必要。

### 11.5 render 除外可能箇所

安全度が高い順:

1. `LLVOAvatarSelf::updateAttachmentVisibility()` の既存 visibility gate を拡張し、mouselook fast path 中は hidden head attachment を shadow/probe 用にも表示復帰させない。ただし mirror/snapshot では別設定か例外が必要。
2. `LLVolumeGeometryManager::rebuildGeom()` / `rebuildMesh()` の face list 構築前に、self hidden head attachment の drawable を drawinfo 生成対象から外す。既存の bridge `mDrawableType=0` と整合する位置。
3. `LLRenderPass::pushBatches()` / `pushRiggedBatches()` で `mAttachedToAvatar->isSelf()` と source local id / attachment classifier を見て skip する。r34 の計測フックに近く実験しやすいが、drawinfo 生成後なので CPU 削減は render push 以降に限られる。

避けるべき箇所:

- avatar skeleton update 全体
- animation update 全体
- attachment transform update 全体
- `LLVOAvatar::updateOrientation()` の self mouselook 分岐
- head / neck / root / pelvis joint transform

### 11.6 CPU update 除外可能箇所

候補:

- `LLDrawable::updateMove()` の self mouselook undamped 分岐で、hidden head attachment かつ非 rigged / 非 selected / 非 HUD なら pipeline move update を抑える検証。
- `LLVOVolume::updateGeometry()` で hidden head attachment の geometry rebuild を defer する検証。ただし texture/material/LOD 変更が復帰時に破綻しないよう、visibility 復帰時に rebuild を必ず発火させる必要がある。
- `LLVOVolume::updateRiggedVolume()` で render 用 rigged volume 更新を抑える検証。これは bounding box / pick / visual extents / reflection への影響が大きいので Phase 2 以降。

現時点の判断:

- Phase 1 では update/skinning には踏み込まない。
- Phase 2 でやる場合も、まず counter-only を入れて「hidden self head attachment の rebuild/skinning が実際に何件・何 ms あるか」を確認してから gate する。

### 11.7 破壊リスク

- shadow path は意図的に mouselook 中の attachment visibility を third-person 相当に戻しているため、ここを変えると self shadow / mirror / probe / snapshot の見え方が変わる。
- `LLViewerJointAttachment::setAttachmentVisibility(false)` は bridge type を 0 にするだけで、既に dirty になった drawable rebuild / rigged update を必ず止めるわけではない。
- functional attachment は head/face に付く場合がある。local render を止めるのはよいが、transform / script state / attachment state を止めると壊れる。
- RLV の `showself` / `showselfhead` は既存の表示制御に入っているため、fast path が RLV を上書きしてはいけない。
- rigged hair / lashes は attachment point と skinning joint が一致しないことがある。attachment point id だけで「頭部負荷」を完全には拾えない。
- draw pass 末端で skip すると見た目の検証はしやすいが、CPU の rebuild/skinning 負荷は残る。

### 11.8 最小パッチ案

設定:

- `AYAOptimizeSelfAvatarInMouselook`: bool, default false。master switch。
- `AYASkipSelfHeadAttachmentsInMouselook`: bool, default true。master switch が true の時だけ有効。
- `AYASkipSelfAvatarShadowInMouselook`: bool, default true。master switch が true の時だけ有効。
- `AYAMouselookSelfAvatarCPUFastPathDebug`: bool, default false。counter/log only。

helper:

- `bool ayaUseMouselookSelfAvatarFastPath()`
- `bool ayaIsHiddenSelfHeadAttachment(const LLViewerObject* objectp)`
- `bool ayaIsHeadAttachmentPointId(S32 attachment_id)`

`ayaIsHiddenSelfHeadAttachment()` は最低限、次を満たす場合だけ true:

- object が attachment
- wearer avatar が self
- mouselook
- HUD ではない
- attachment point id が head/face allow-list
- master setting ON

Phase 1:

- 既定 OFF で設定だけ追加。
- `LLRenderPass` 側に self head attachment drawinfo counter を追加し、`self_attachment_draw_infos` の内訳を head/face とそれ以外に分ける。
- master ON の時だけ `LLRenderPass` で hidden self head attachment drawinfo skip を検証可能にする。これは実験用で、主目的は見た目差分と skip 量の確認。
- shadow/probe は `AYASkipSelfAvatarShadowInMouselook` が ON の時だけ、mouselook 中に hidden head attachment を third-person 復帰させない経路を検討する。ただし mirror pass は初期実装では除外しない。

Phase 2:

- `LLVolumeGeometryManager::rebuildGeom()` / `rebuildMesh()` の前段で hidden self head attachment を drawinfo 生成から外す。
- 復帰時に dirty rebuild を保証する。
- `LLVOVolume::updateRiggedVolume()` の抑制は counter で効果が見えてから、非 selected / 非 HUD / non-interactive / strong classifier の場合に限定する。

Phase 3:

- self avatar mouselook 専用 update throttle は最後に検討する。skeleton / animation / transform を止めない条件を守る限り、期待効果は rebuild/skinning の実測次第。

### 11.9 r34 での優先順位

この提案は実装可能だが、現在の主戦場ではない。r34 の次の優先順は以下。

1. world volume trace の auto root / top source を継続し、車両 seated mouselook の支配 root / child / pass を確定する。
2. self attachment trace に head/face 内訳 counter を足し、この提案の効果上限を数値化する。
3. 数値が出た場合だけ Phase 1 の render skip を入れる。
4. rebuild/skinning は、counter で hidden self head attachment が有意に重いと確認できた場合だけ Phase 2 として扱う。

## 12. 他ゲーム/エンジンの LOD・culling 事例から転用できる手法

### 12.1 参照した事例

- Unreal Engine: Static Mesh LOD は screen space size で LOD transition を制御する。自動 LOD 生成では quadratic mesh simplification と LOD Group preset を使い、screen size を自動計算できる。
  - https://dev.epicgames.com/documentation/unreal-engine/optimizing-lod-screen-size-per-platform-in-unreal-engine?lang=en-US
  - https://dev.epicgames.com/documentation/unreal-engine/static-mesh-automatic-lod-generation-in-unreal-engine?application_version=5.7
- Unreal Engine: Cull Distance Volume は actor bounds の longest dimension と camera distance の pair で culling する。large outdoor level + detailed interior のような場面が想定例に入っている。
  - https://dev.epicgames.com/documentation/unreal-engine/cull-distance-volumes-in-unreal-engine?lang=en-US
- Unreal Engine: Precomputed Visibility Volume は camera/player position ごとに actor visibility を cell として持つ。静的・制限された移動範囲には強いが、dynamic object には弱い。
  - https://dev.epicgames.com/documentation/unreal-engine/precomputed-visibility-volumes-in-unreal-engine
- Unity: LODGroup は object の screen space height ratio で LOD を切り替え、cross fade / fade transition width を持つ。
  - https://docs.unity3d.com/jp/current/Manual/class-LODGroup.html
- Unity: CullingGroup API は bounding sphere の visibility と distance band を非同期に返し、band 変更時に CPU 負荷の低い挙動へ切り替える用途を想定している。
  - https://docs.unity.cn/Manual/CullingGroupAPI.html
- Godot: Visibility ranges / HLOD は per-node の visible range、hysteresis margin、fade mode を持つ。HLOD は複数の小オブジェクトを大きな proxy に置き換えて draw call を減らす考え方。
  - https://docs.godotengine.org/en/4.0/tutorials/3d/visibility_ranges.html
- CryEngine: Vis Area / Portal は indoor area を定義し、area 内外の相互描画を切る。portal は door のような入口として扱う。
  - https://www.cryengine.com/docs/static/engines/cryengine-5/categories/23756816/pages/26215443
- Unity: Occlusion Portal は door のような open/closed state を持つ遮蔽物に対応する。
  - https://docs.unity.cn/Documentation/Manual/class-OcclusionPortal.html

### 12.2 r34 に転用できる考え方

重要: ここからの実装計画では「車両に乗っているかどうか」は主条件にしない。車両 seated mouselook は最初に観測できた代表ケースとして扱う。最適化対象は、mouselook 中に camera 近傍へ dense world volume linkset が入る状況全般。

汎用対象例:

- 乗り物の車内・船内・機体内部
- 家具や装飾が多い建物内部
- ステージ、店舗、展示ブースなどの dense linkset
- camera 近傍に大型 linkset の多数 child が入る mouselook 状態

除外対象:

- avatar / self avatar / attachment / HUD
- selected object / edit 対象
- MOAP / 3D Stream protected source
- media 面、speaker、stream source と判定できる prim
- small linkset や通常の屋外 world geometry

#### A. screen-space LOD bias

距離ではなく、対象 child prim / drawinfo の画面占有率で LOD を落とす。Unreal / Unity の LOD は距離そのものより screen size を重視している。mouselook では camera 近傍の dense linkset が距離だけだとほぼ全て high priority になりやすいが、画面占有率なら視界端・小物・床下・背後・外装・装飾の優先度を下げられる。

r34 への適用:

- `LLVolumeGeometryManager` または LOD selection 周辺で、mouselook かつ nearby dense root 配下の child にだけ追加 LOD bias を掛ける。
- 入力は `source object bounds`, `camera distance`, `projected screen size`, `camera dot`, `root child count`。
- render pass で丸ごと skip するより、既存 mesh LOD の lower level を選ばせる方が表示破綻を抑えやすい。

優先度: 高。

#### B. bounds-size + distance band

Unreal の Cull Distance Volume は bounds size と distance の pair で cull する。SL viewer では region 全体に volume を置けないが、r34 の auto root 検出に「child bounds size と camera distance の band」を入れる形なら転用できる。

r34 への適用:

- root linkset が camera から近く、child 数が多い場合だけ対象にする。
- 小さい child は近距離でも早めに lower LOD / pass reduction へ送る。
- 大きい child、camera forward cone 内、MOAP/stream3D protected child は維持する。

優先度: 高。

#### C. hysteresis / band cache

Godot visibility range や Unity CullingGroup は band 切替に margin / 非同期性を持つ。これを入れないと mouselook で少し動いた時に LOD が毎フレーム揺れて、見た目も CPU も悪化する。

r34 への適用:

- child local ID ごとに `last_lod_band` / `last_decision_frame` を持つ軽量 cache を追加する。
- threshold を上げる時と下げる時で別値にする。
- LOD decision を毎 frame 再計算せず、N frame または camera delta 閾値で更新する。

優先度: 高。

#### D. root-aware HLOD / proxy

Unreal / Godot の HLOD は複数 mesh を proxy にまとめて draw call と triangle を減らす。ただし Firestorm/SL viewer 側で任意ユーザー生成 linkset の proxy mesh を安全に生成・永続化するのは重い。

r34 への適用:

- 本格 HLOD proxy 生成は r34 の範囲外。
- 代替として、nearby large root 配下の child を band 分類し、複数 child に同じ LOD bias / pass policy を適用する root-aware LOD に留める。
- 将来案として、viewer cache 内に transient simplified proxy を作る余地はあるが、mesh upload / material / alpha / picking / ownership のリスクが大きい。

優先度: 中から低。

#### E. portal / room visibility

CryEngine の Vis Area / Portal や Unity Occlusion Portal は、屋内と屋外の境界が明示される場合に強い。SL の linkset には portal metadata がないため、車両・建物・展示物のどれでも直接の portal culling はできない。

r34 への適用:

- 汎用 portal は不可。
- mouselook camera が dense root linkset 近傍または内部にある時だけ、camera forward cone と source child position で「近傍 linkset 視界 band」を近似する。
- door/window/transparent surface を理解できないため、完全な occlusion culling ではなく LOD bias に留める。

優先度: 中。

#### F. material simplification

Godot docs は distant LOD mesh で normal / clearcoat / anisotropy / height / SSS / refraction など高コスト material feature を落とす例を挙げている。r34 の計測でも pass group 別に material / pbr / bump / shiny を見ているため、CPU 側だけではなく draw preparation と shader/pass 数の削減候補になる。

r34 への適用:

- Metal/OpenGL 固有ではなく Viewer logic として、mouselook dense root 配下の低優先 child に material/pass policy を掛ける。
- 初期実装は shader feature を直接変えず、pass group 単位の LOD policy として `bump/shiny/material` を lower priority に落とせるか調査する。
- 見た目破綻が大きいため、いきなり default ON にはしない。

優先度: 中。

### 12.3 r34 で採用すべき最小方針

次の実装候補は、他エンジンの事例と r34 実測の両方に合う。

1. `AYAR34MouselookDenseRootLODBiasEnabled` を追加する。
2. mouselook 中、camera から近い dense root linkset だけを対象にする。
3. child ごとに `screen size`, `camera dot`, `forward distance`, `bounds size`, `pass group`, `stream3D protected` を評価する。
4. `skip` ではなく、まず `LOD bias` を掛ける。
5. band 判定には hysteresis と frame cache を入れる。
6. MOAP / 3D Stream / selected / HUD / avatar attachment / self avatar は対象外にする。
7. counter と top source ログに `lod_band`, `lod_bias`, `band_reason` を出す。

推奨 band:

- Band 0: forward cone / large visible / protected。既存 LOD。
- Band 1: side cone または small child。LOD を 1 段落とす。
- Band 2: rear cone または very small child。LOD を 2 段落とす、または expensive pass を抑制候補にする。
- Band 3: camera 後方かつ small child。実験時のみ skip 可能。

初期値案:

- forward cone: `dot >= 0.75`
- side cone: `0.0 <= dot < 0.75`
- rear cone: `dot < 0.0`
- small projected height: viewport height の `1.5%` 未満
- very small projected height: viewport height の `0.5%` 未満
- cache interval: 4 frames
- hysteresis: band を重くする方向は即時、軽く戻す方向は 8 frames 維持

### 12.4 やらない方がよい手法

- 本格的な offline precomputed visibility: SL の dynamic user content / movable object / arbitrary linkset には合わない。
- portal metadata 前提の culling: viewer 側から linkset の室内/入口情報を安定して得られない。
- root 丸ごと cull: 車内・室内・展示物など必要な見た目が消える。
- pass 末端の aggressive skip だけで解決すること: 既に r34 実測で FPS 改善が薄い。LOD selection / rebuild cost / draw preparation へ寄せる必要がある。
- self head optimization を主修正にすること: 実測と既存コードから主因ではない。

### 12.5 次の実装優先度

1. `LLVOVolume` の LOD selection 経路を特定し、mouselook dense-root child に追加 LOD bias を掛けられる最小フックを探す。
2. r34 trace の source cache に projected screen size と bounds size を追加する。
3. suppression ではなく LOD bias 実験の debug setting を追加する。
4. idle / rotate で FPS、world triangles、drawinfos、見た目破綻を比較する。

### 12.6 汎用 mouselook dense-root LOD 実装計画

目的:

- 車両乗車に依存せず、mouselook 中の camera 近傍 dense world volume linkset を軽くする。
- world volume だけを対象にし、avatar / attachment / HUD / selected / MOAP / 3D Stream は壊さない。
- 初期実装は LOD bias と計測に限定し、root 丸ごと cull や aggressive skip は行わない。

対象判定:

- `gAgentCamera.cameraMouselook()` が true。
- `LLDrawInfo` / source object が world volume。`mAvatar == NULL` かつ `mAttachedToAvatar == NULL`。
- source root が camera から近い。初期値は `12m`。
- source root の child 数が多い。初期値は `50`。
- source object が stream3D protected ではない。
- source object / root が selected ではない。
- source object が HUD / attachment / avatar ではない。

dense root 判定:

- root child count >= `AYAR34MouselookDenseRootMinChildren`
- root distance <= `AYAR34MouselookDenseRootMaxDistance`
- root または child の drawinfo が 1 秒窓で一定以上出ている場合は dense とみなす追加条件を検討する。

band 判定:

- Band 0: 保持。forward cone、large projected size、protected source。
- Band 1: mild bias。side cone または small projected size。
- Band 2: stronger bias。rear cone、very small projected size、または root 内で低寄与 pass。
- Band 3: 実験用。camera 後方かつ small projected size の child。初期実装では skip せず、counter-only か最大 bias に留める。

設定案:

- `AYAR34MouselookDenseRootLODBiasEnabled`: bool, default false
- `AYAR34MouselookDenseRootTraceEnabled`: bool, default false
- `AYAR34MouselookDenseRootMaxDistance`: F32, default `12.0`
- `AYAR34MouselookDenseRootMinChildren`: S32, default `50`
- `AYAR34MouselookHeavyMeshMaxDistance`: F32, default `24.0`
- `AYAR34MouselookHeavyMeshMinTriangles`: S32, default `50000`
- `AYAR34MouselookDenseRootForwardDot`: F32, default `0.75`
- `AYAR34MouselookDenseRootSmallScreenPct`: F32, default `1.5`
- `AYAR34MouselookDenseRootVerySmallScreenPct`: F32, default `0.5`
- `AYAR34MouselookDenseRootDecisionIntervalFrames`: S32, default `4`
- `AYAR34MouselookDenseRootHysteresisFrames`: S32, default `8`

実装 Phase:

Phase 0: 調査

- `LLVOVolume` の LOD 選択経路、`LLDrawable::updateDistance()`、`LLSpatialGroup::changeLOD()`、mesh LOD selection 周辺を確認する。
- 既存 LOD に bias を足せる最小地点を特定する。

Phase 1: 計測のみ

- r34 source cache に projected screen size / bounds size / band candidate を追加する。
- `AYAR34MouselookDenseRootTraceEnabled` で root / child / band / pass / triangles をログに出す。
- 既存描画は変えない。

Phase 2: LOD bias 実験

- `AYAR34MouselookDenseRootLODBiasEnabled` ON の時だけ band に応じて LOD bias を掛ける。
- Band 1 は 1 段、Band 2 は 2 段を目安にする。
- Band 3 は当面 skip しない。
- hysteresis と decision cache を入れて、mouselook 回転中の LOD 揺れを防ぐ。

Phase 3: pass policy 実験

- Band 2/3 の child だけ expensive pass の簡略化を検討する。
- `bump/shiny/material/pbr/alpha-mask` のどれが支配的か r34 trace で確認してから行う。

Phase 4: 汎用化判定

- 車両、建物内部、家具密集、ステージ装置で比較する。
- 破綻が少なく FPS / frame time が改善する場合だけ default candidate に昇格する。

検証ケース:

- 車両 seated mouselook idle / rotate。代表ケース。
- 車両に乗らず、車両や大型 linkset の近くで mouselook idle / rotate。
- 家具密集室内で mouselook idle / rotate。
- ステージ/店舗/展示ブースなど dense root 近傍で mouselook idle / rotate。
- 通常の屋外 world で false positive が出ないこと。
- MOAP / 3D Stream 面がある scene で media / audio が壊れないこと。

成功条件:

- `world_draw_infos` / `world triangles` / frame time が下がる。
- camera forward cone 内の主要形状が維持される。
- mouselook rotate で LOD band がちらつかない。
- 乗車状態に依存せず、dense world volume 近傍で再現性がある。
- 通常 scene では発火しない、または発火しても見た目差が許容範囲。

### 12.7 実装メモ

2026-05-27 に Phase 1/2 の最小実装を追加した。

変更点:

- `LLVOVolume::calcLOD()` で通常 LOD detail 算出後、mouselook dense-root 条件に合う world volume child だけ追加 LOD bias を掛ける。
- 対象外条件は `HUD`, `avatar`, `attachment`, `selected`, `MOAP/3D Stream protected`。
- dense root 条件は `root distance <= AYAR34MouselookDenseRootMaxDistance` かつ `root child count >= AYAR34MouselookDenseRootMinChildren`。
- 追加検証で、root child count が少なくても近距離 high triangle mesh を拾う heavy-mesh 条件を追加した。条件は `root distance <= AYAR34MouselookHeavyMeshMaxDistance` かつ `max(current triangles, estimated max triangles) >= AYAR34MouselookHeavyMeshMinTriangles`。
- band 判定は `camera dot` と projected screen height percentage で行う。
- band cache / hysteresis を child local ID 単位で持ち、mouselook rotate 中の LOD 揺れを抑える。
- `AYAR34MouselookDenseRootTraceEnabled` で `dense_lod` の 1 秒 summary を `AYAR34MouselookFPS` ログへ出す。summary には `dense_root` と `heavy_mesh` の候補数を含め、どちらの経路で対象化されたかを確認できる。
- 既存の aggressive suppression 実験 default は OFF に戻した。新しい LOD bias も default OFF。

2026-05-27 追加実測メモ:

- `AYAR34MouselookDenseRootLODBiasEnabled=TRUE` で `dense_lod candidates/biased` は出たが、主負荷には `root_children=12` や `root_children=45` の high triangle root が残った。
- `root child count >= 50` だけでは、少数 child の大型 mesh / high triangle source を拾い切れない。
- Phase 2b として heavy-mesh 条件を追加し、車両搭乗状態に依存せず「近距離 + high triangle + 視界外/小画面」の world volume child に LOD bias を掛ける。

2026-05-27 追加修正:

- heavy mesh の検証距離を `24m` から `96m` に広げた。直近ログでは top source が `26m`, `37m`, `80m+`, `90m+` に出ており、`24m` では支配的な high-triangle source を候補化できなかったため。
- `AYAR34MouselookHeavyMeshScreenForceEnabled` を追加した。heavy mesh については、画面占有率が小さい場合に限り forward cone 内でも LOD bias を許可する。これにより、正面方向にあるが小さく映る高負荷 mesh が `object_dot >= AYAR34MouselookDenseRootForwardDot` で常に `band=0` になる問題を避ける。
- `dense_lod` summary に `heavy_seen`, `heavy_distance_rejected`, `heavy_forward_protected`, `screen_forced` を追加した。候補化前に距離で漏れているのか、候補化後に forward cone 保護で落ちていないのか、画面占有率で強制 bias されたのかをログから判定する。

2026-05-27 追加修正 2:

- 実測では top source に出ている high-triangle world mesh が `LLVOVolume::calcLOD()` 側に十分流れていなかった。render-by-group の visible spatial group は group distance だけ更新され、個別 drawable の LOD 再評価が毎秒十分に走らないケースがあるため。
- `LLSpatialGroup::updateDistance()` に r34 mouselook 限定の per-object LOD refresh を追加した。対象は existing LOD bias と同じく、world volume かつ dense root または heavy mesh candidate のみ。HUD / avatar / attachment / selected / 3D Stream protected は除外する。
- `AYAR34MouselookForceLODRefreshIntervalFrames` を追加し、同一 local id の forced refresh を既定 4 frame 間隔に抑える。
- `dense_lod` summary に `forced_refresh` を追加した。ここが増え、同時に `heavy_mesh` / `screen_forced` が top source に追随するか確認する。

2026-05-28 追加修正:

- 実測ログでは `forced_refresh` が増えて LOD refresh path は動作していたが、`world_top_sources` には 100 万 triangle 超の source が残った。つまり主問題は「LOD 再評価されていない」から「LOD bias だけでは描画投入 triangle が十分落ちない」に移った。
- r34 検証用に `AYAR34MouselookSuppressAutoHeavyEnabled` を追加した。既定 ON とし、mouselook 中だけ per-frame の root / child source triangle を積算して、`AYAR34MouselookSuppressAutoHeavyMinSourceTriangles` 以上の heavy world mesh source を selected outer-cone passes から除外する。
- 対象は world mesh のみ。avatar / attachment / selected object / 3D Stream protected prim は除外する。これにより MOAP/3D Stream、装着物、編集対象を壊さない。
- suppression 判定は `AYAR34MouselookSuppressAutoHeavyMaxDistance`、`AYAR34MouselookSuppressAutoHeavyOuterDot`、`AYAR34MouselookSuppressAutoHeavySmallScreenPct` で制御する。正面の大きい形状は残し、視界外または小さく映る high-triangle source を先に落とす。
- `AYAR34MouselookVolumeTraceEnabled` の summary に `auto_heavy_suppressed_draw_infos` / `auto_heavy_suppressed_triangles` を追加した。ここが増え、同時に `world triangles` と FPS が改善するか確認する。

ビルド:

- Mac Release app build 成功。
- `codesign --verify --deep --strict --verbose=2 build-darwin-universal/newview/Release/AYAstorm.app` 成功。

初回検証手順:

1. `AYAR34MouselookVolumeTraceEnabled=TRUE`
2. `AYAR34MouselookTopSourceTraceEnabled=TRUE`
3. `AYAR34MouselookDenseRootTraceEnabled=TRUE`
4. `AYAR34MouselookDenseRootLODBiasEnabled=FALSE` で baseline を取る。
5. `AYAR34MouselookDenseRootLODBiasEnabled=TRUE` にして同じ mouselook idle / rotate を取る。
6. `dense_lod candidates/biased/bands` と FPS、`world_draw_infos`、triangles、見た目破綻を比較する。
7. `AYAR34MouselookSuppressAutoHeavyEnabled=TRUE` で同じ mouselook idle / rotate を取り、`auto_heavy_suppressed_triangles` と `world_top_sources` の残り方を見る。
8. 検証後は `AYAR34MouselookDenseRootLODBiasEnabled=FALSE`、`AYAR34MouselookSuppressAutoHeavyEnabled=FALSE` に戻す。
