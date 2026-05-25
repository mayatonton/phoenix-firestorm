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
