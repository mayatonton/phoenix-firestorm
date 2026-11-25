# AYAstorm: 読み込み中 FPS 低下と他メタバース比較調査

作成日: 2026-05-31  
対象ブランチ: `perf/ayastorm-loading-fps-investigation`

## 2026-06-07 現時点の実装・検証状態

2026-06-07 時点では、`FPS安定化` と `テクスチャ読み込み・メモリ管理` を分ける設計に沿って実装を進めている。直近の arm64 app 差分ビルドは `ayastorm-bin` scheme / `Release` / `ARCHS=arm64` / `ONLY_ACTIVE_ARCH=YES` で成功し、生成物は `build-darwin-universal/newview/Release/AYAstorm.app` である。DMG は作っていない。

### 2026-06-08 実機確認結果

2026-06-08 の実機確認で、avatar 読み込み停止回帰は修正済みとして扱える状態になった。特に、テクスチャメモリ整理導入後に発生していた「近距離/可視 avatar が名前だけ残り、本体・簡易表示・usable impostor のいずれにも進まない」症状は、直近の app で解消確認済みである。

この確認により、以下の実装方針は維持する。

- avatar loading 判定は Firestorm 既存の avatar loaded/cloud/rez status 経路を所有させ、AYA texture QoS から直接書き換えない。
- remote avatar baked texture の fetch/callback は、`FTT_SERVER_BAKE` / `FTT_HOST_BAKE` と callback 状態を根拠に保護する。boost だけを avatar bake 判定に使わない。
- callback 待ちなのに fetch が止まった avatar baked texture には、最低 fetch area を再付与して fetch 再始動を促す。
- texture cap / sweep / downscale は avatar の loaded 判定を完了扱いにしない。低解像度到達は texture QoS の品質観測であり、avatar の存在表示条件ではない。
- `LLDrawPoolAvatar` 側では、未完成 impostor が body fallback を消して「非表示」に落ちる経路を避ける。合格条件は cloud、gray/simple body、または usable impostor のいずれかで avatar の存在が見えること。

残る注意点:

- 他人 avatar baked texture を読み込み完了後に距離/視野外 cap へ戻す設計は、今回の回帰解消後の別段階で扱う。今回の完了条件には含めない。
- `AYA avatar_loading_wait` と `AYA avatar_draw_state` は、将来同種の回帰が出た時の確認ログとして維持する。

### 2026-06-08 GPU Picker Self 回帰調査

実機で `GPU Picker` のうち Self 経路が壊れて見える可能性が報告された。コード調査では、`fsselfriggedpicker.cpp::readObjectIDBufferLocalID()` が `mObjectIDBuffer` を `WorldViewRectRaw` と同サイズと仮定していたことが原因候補として確定した。

現在の render target 経路では、`LLPipeline::resizeScreenTexture()` / `LLPipeline::allocateScreenBufferInternal()` が `RenderResolutionMultiplier` または AYA render relief の effective multiplier を適用し、`mRT->screen` / `mRT->deferredScreen` / `mObjectIDBuffer` を world view raw より小さいサイズで確保する。`LLRenderTarget::bindTarget()` は bind した target の `mResX` / `mResY` を viewport にするため、ID buffer pass も低解像度 buffer 上へ描かれる。

一方、修正前の picker readback は以下の変換で止まっていた。

```text
mouse scaled -> window raw -> world view raw local
```

このため、低解像度 render target 時には `world view raw local` をそのまま `glReadPixels()` 座標に使い、クリック位置と ID buffer 上の pixel が一致しなかった。左上/中央付近では別位置の ID を読み、右側/上側では buffer 範囲外として `id=0` になりやすい。Self picker は `id=0` を authoritative fallback として扱う場面があるため、自分の rigged attachment 選択が body 側へ落ちる、または反応しないように見える。

修正内容:

- `readObjectIDBufferLocalID()` で、world view raw local 座標を `mObjectIDBuffer.getWidth()/WorldViewRectRaw.width` と `mObjectIDBuffer.getHeight()/WorldViewRectRaw.height` の比率で active buffer 座標へ変換してから `glReadPixels()` する。
- `RenderResolutionMultiplier=1.0` の時は scale が 1.0 なので従来挙動と同じ。
- `RenderResolutionMultiplier<1.0` または AYA render relief が active の時だけ、低解像度 ID buffer に正しく対応する。
- これは Self/Other rigged picker の readback 共通 helper に入るが、低解像度化時の座標補正であり、selection policy や object ID の意味は変更しない。

検証方針:

- `FSSelfRiggedPickerGPU=1`, `FSSelfRiggedPickerArmedMode=1` で、自分の rigged hair / body / clothing を右クリックし、正しい attachment が選ばれるか確認する。
- `RenderResolutionMultiplier=1.0` と `0.65`、AYA `AYALoadingFPSGuardEnabled=true` + resolution relief strength active の両方で確認する。
- HUD attachment は world-space `mObjectIDBuffer` の対象外なので、従来通り upstream worldray を使う。HUD をこの検証の成功条件に含めない。
- buffer が未 armed / 未 ready の場合は picker が fallback するため、hover 後 1 frame 以上置いて右クリックする条件も確認する。

### 2026-06-07 avatar 未ロード停止回帰の一次対策

実機ログでは、テクスチャメモリ整理 ON 後に一部 non-self avatar が `rez_status=1` / `fully_textured=false` のまま止まり、名前だけまたは簡易表示に落ちない状態が観測された。これは単なる HTTP 失敗やサーバ側問題として扱わない。テクスチャメモリ整理実装前には観測されていなかったため、AYA 側の texture cap / sweep / create queue が avatar loading の既存経路へ干渉した回帰として扱う。

チームレビューで確認した原因:

- remote avatar baked texture は `LLVOAvatar::getBakedTextureImage()` で `FTT_SERVER_BAKE` / `FTT_HOST_BAKE` として作られるが、初期 boost は `BOOST_NONE` である。
- boost だけで avatar bake を保護すると、`LLViewerLODTexture::processTextureStats()` の `RenderMaxTextureResolution` clamp、AYA distance/offscreen cap、`scaleDown()`、freeze-stop 経路から漏れる。
- `allBakedTexturesCompletelyDownloaded()` は full-res `discard == 0` を要求する。低解像度到達を avatar loading 完了条件の代替にすると、cloud/rez status と texture QoS が混ざって破綻する。
- `LLViewerTextureList::updateImagesCreateTextures()` の GL create 後即時 downscale と downscale queue にも protected FTType / protected boost 除外が必要である。
- incomplete impostor の場合、`aya_render_unloaded_fallback=true` でも body pass が suppress されると、簡易表示ではなく非表示に見える経路が残る。

実装修正:

- `LLViewerLODTexture::processTextureStats()` で `FTT_SERVER_BAKE` / `FTT_HOST_BAKE` / `BOOST_AVATAR_BAKED*` を avatar baked texture として明示判定する。
- avatar baked texture は、AYA texture management ON 中でも `mDesiredDiscardLevel = 0` を維持し、AYA cap / immediate `scaleDown()` / freeze-stop から外す。
- `LLViewerTextureList::updateImagesCreateTextures()` の post-create downscale と `mDownScaleQueue` 実行でも protected FTType / protected boost を downscale しない。
- `LLDrawPoolAvatar` では、`AOA_NORMAL` かつ明示非表示でない未ロード avatar について、impostor が未完成なら body fallback を suppress しない。
- `AYA avatar_loading_wait` ログに baked slot ごとの `discard`, `desired`, `FTType`, `boost`, `missing`, `fetching`, `callbacks`, `has_gl`, `gl_created`, `mesh_gl`, `mesh_comp` を追加し、`all_baked_downloaded=true` でも `fully_textured=false` / `rez_status<2` なら mesh 状態を出す。

検証:

- チームコードレビュー 3 系統で、既知の P1/P2 を潰す一次対策は入っている。ただし後述の追跡ログにより、avatar baked texture の fetch/callback 停止条件がまだ残っているため、avatar 未ロード停止回帰は完了扱いにしない。
- `git diff --check` 成功。
- `DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild -project build-darwin-universal/Firestorm.xcodeproj -scheme ayastorm-bin -configuration Release -derivedDataPath build-darwin-universal/DerivedData CLANG_MODULE_CACHE_PATH=build-darwin-universal/ModuleCache ARCHS=arm64 ONLY_ACTIVE_ARCH=YES build` 成功。
- `lipo -archs build-darwin-universal/newview/Release/AYAstorm.app/Contents/MacOS/AYAstorm` は `arm64`。
- `codesign --verify --deep --strict --verbose=2 build-darwin-universal/newview/Release/AYAstorm.app` 成功。

現時点の制約:

- 安全優先として avatar baked texture は full-res loading 完了経路を保護する。したがって、他人 avatar baked texture を読み込み完了後に距離/視野外で cap/downscale へ戻す設計は、この回帰が実機で解消された後に別段階で再設計する。
- 低解像度化は world / scene texture を主対象に戻し、avatar loading 判定を所有しない。
- 次回実機確認では、詰まり avatar が残る場合に `AYA avatar_loading_wait` の `baked_fetch_state` と `AYA avatar_draw_state` を同時に見る。

### 2026-06-07 追跡ログで判明した未解決原因

arm64 app 実機追跡ログでは、一次対策後も一部 avatar が `rez_status=1` / `fully_textured=false` / `all_baked_downloaded=false` のまま残った。代表例は以下である。

- `chapilo`: TE20 の baked texture が `d=-1 desired=0 ft=1 boost=2 missing=0 fetching=0 callbacks=1 has_gl=0`。
- `Suika Manamiko`: TE11 / TE20 の baked texture が `d=-1 desired=0 ft=1 boost=2 missing=0 fetching=0 callbacks=1 has_gl=0`。
- `c1b93b1e-bb90-4f59-87f2-29e71dad0080`: 20 秒後に `left after ruth seconds as cloud`。

このログの重要点は、`desired=0` でも fetch が進むとは限らないことである。`LLViewerFetchedTexture::updateFetch()` は `mMaxVirtualSize` を `decode_priority` として使い、`decode_priority <= 0` の場合は新しい fetch request を作らない。したがって、`desired=0` は「最高品質を欲しい」という状態であって、「現在 fetch 中」または「fetch が再始動する」ことの保証ではない。

`callbacks=1 fetching=0 has_gl=0` は、callback が残っているが、raw/GL texture がなく、fetch も走っていない停止状態として扱う。`LLViewerFetchedTexture::setLoadedCallback()` は callback list と desired discard を設定するが、それ自体では fetch を開始しない。`doLoadedCallbacks()` も raw/gl がなければ完了できず、`mForceCallbackFetch` が立っていない限り、callback 待ちだけで再取得へ戻る保証はない。

チーム調査で揃った原因仮説:

- テクスチャメモリ整理導入前は、avatar baked texture の `mMaxVirtualSize` が通常の描画統計や avatar 側の `addTextureStats()` によって再び正値へ戻りやすく、callback が残っていても fetch/create が自然に進む余地があった。
- 導入後は distance/offscreen cap、memory sweep、teleport sweep、cap candidate 処理、create/backpressure などの干渉点が増えた。ただし現行コードでは `FTT_SERVER_BAKE` / `FTT_HOST_BAKE` / callback 持ち texture は sweep/backpressure の直接破棄から概ね保護されているため、未解決の本命は `mMaxVirtualSize=0` / `decode_priority=0` による fetch 再始動不能である。
- avatar baked texture の discard を上げる direct cap は一次対策で塞いだが、`mMaxVirtualSize` を 0 にしたままにする indirect stall はまだ塞ぎ切れていない。
- remote avatar baked texture は作成時 boost が `BOOST_NONE` で、後から `BOOST_AVATAR_BAKED` へ寄るため、boost だけを保護条件にすると初期段階で漏れる。FTType (`FTT_SERVER_BAKE` / `FTT_HOST_BAKE`) と callback 状態を保護条件に含める必要がある。
- callback 優先判定が face owner 依存の場合、baked texture がまだ mesh へ貼られていない段階では avatar owner を見つけられず、avatar loading 用 callback なのに priority/create guard の優先枠から漏れる可能性がある。

現時点で確定している切り分け:

- `desired=0` なので「AYA cap が baked texture を低解像度へ落としている」状態ではない。
- `fetching=0` なので「ネットワークが遅いだけで現在 fetch 中」とも言えない。
- `missing=0` なので「missing asset 確定」でもない。
- `callbacks=1` なので avatar loading 側の完了待ちは残っている。
- `has_gl=0` なので表示に使える GL texture はまだない。
- `allow_body_fallback suppress_body=false` は body 描画完了ログではない。これは drawpool の pass 0 で「body fallback を許可した」ログであり、後続 pass が mesh/skin draw まで進まなければユーザーには簡易表示として成立しない。

したがって、次の修正は「baked texture を cap/downscale から外す」だけでは不十分である。avatar loading 中の baked texture について、callback 完了または avatar `rez_status>=2` まで以下のどちらかを保証する必要がある。

- `mMaxVirtualSize` に最低 fetch priority を与え、`decode_priority > 0` を維持する。
- callback timeout 後だけでなく、callback 待ちが一定時間続き `fetching=0 && has_gl=0 && !missing` なら、明示的に refetch / create priority を再始動する。

この修正は texture memory 整理側の QoS で行い、avatar loaded/cloud 判定を低解像度到達で代替してはいけない。avatar のロード完了条件は Firestorm 既存の full-res baked texture 条件に戻したまま、fetch/create/callback の停止だけを回復する。

### 確実に avatar を読み込むための修正計画

この計画の目的は、`Texture management ON` 中でも visible / near avatar の baked texture fetch、GL create、callback 完了を止めないことである。表示側の一時 fallback と、avatar loading 完了条件は分離する。

非目標:

- `isFullyLoaded()` や `allBakedTexturesCompletelyDownloaded()` を偽装して、低解像度到達を loaded 扱いにしない。
- missing / HTTP 403 / explicit failure を無限 refetch しない。
- AYA 側で friends-only、visual mute、explicit do-not-render、`AOA_INVISIBLE` を描画へ戻さない。
- texture memory 整理全体を止めて解決扱いにしない。

#### P0: avatar baked fetch priority を保証する

主介入点:

- `LLViewerLODTexture::processTextureStats()`
- `LLViewerFetchedTexture::doLoadedCallbacks()`

確認/最終防衛点:

- `LLViewerFetchedTexture::updateFetch()`

条件:

- texture が `FTT_SERVER_BAKE` / `FTT_HOST_BAKE` / `BOOST_AVATAR_BAKED*` の avatar baked texture である。
- callback が残っている。
- missing asset ではない。
- `getDiscardLevel() < 0`、`!hasGLTexture()`、または callback が要求する discard に達していない。
- `mMaxVirtualSize <= 10.f` または `decode_priority <= 0` へ落ちている。

処理:

- `mDesiredDiscardLevel = 0` は維持する。
- `mMaxVirtualSize` に avatar baked 用の最低 fetch priority area を与え、`decode_priority > 0` を保証する。
- 通常介入は `processTextureStats()` / `doLoadedCallbacks()` 側で `mMaxVirtualSize` を正値へ戻す。`updateFetch()` の `decode_priority <= 0` gate を広域特例で突破する実装にはしない。
- `desired=0` だけで合格にしない。`desired=0 && maxvs>0 && fetching/create/callback が進む` ことを合格条件にする。

理由:

`desired=0` は「full-res が欲しい」という要求であって、`updateFetch()` が request を作る保証ではない。`mMaxVirtualSize` が 0 のままなら `decode_priority <= 0` で止まる。

2026-06-07 実装:

- `LLViewerLODTexture::processTextureStats()` で `FTT_SERVER_BAKE` / `FTT_HOST_BAKE` / `BOOST_AVATAR_BAKED*` を avatar baked texture として扱い、texture management ON 中でも `mDesiredDiscardLevel=0` を維持する。
- callback 待ちの avatar baked texture が `mMaxVirtualSize<=10` かつ未取得/未GL状態なら、最低 fetch priority area (`256*256`) を追加する。`desired=0` だけで止まらず、`decode_priority>0` へ戻すための床である。
- `RenderMaxTextureResolution` clamp、AYA texture cap、即時 `scaleDown()`、freeze-stop は avatar baked texture へ適用しない。
- `updateFetch()` の gate 自体は広域特例で壊さない。fetch request が作られる条件を、手前の stats/callback 経路で満たす。

#### P0: callback 停止 recovery を入れる

主介入点:

- `LLViewerFetchedTexture::doLoadedCallbacks()`
- `LLVOAvatar::updateIsFullyLoaded()` の diagnostic / escalation log

recovery 条件:

- avatar baked texture。
- `hasCallbacks() == true`。
- `fetching=0`。
- `has_gl=0`。
- raw / saved raw がない。
- missing asset ではない。
- 一定時間 callback 待ちが続いている。timeout 後に限定しない。

処理:

- rate limit 付きで最低 fetch priority を再付与する。
- 必要なら `forceToRefetchTexture()` 相当を使う。ただし priority 0 のまま refetch だけを呼ばない。
- `mNeedsCreateTexture=true` または raw がある場合は refetch ではなく create priority 側へ送る。
- avatar loaded/cloud 判定は変更しない。

2026-06-07 実装:

- `LLViewerFetchedTexture::doLoadedCallbacks()` で `avatar baked + callback pending + fetching=0 + rawなし + saved rawなし + has_gl=0 + maxvs<=10` が 2 秒を超えた場合だけ recovery を発動する。
- recovery は最低 fetch priority area を再付与し、`forceToRefetchTexture(0, 5.f)` を一度走らせる。`mLastCallBackActiveTime` を更新して、毎フレーム refetch にならないようにする。
- `mNeedsCreateTexture=true` の場合は recovery refetch しない。create queue 側の priority に任せる。
- ログは `avatar_bake_callback_recovery` として `id`, `ft_type`, `callbacks`, `discard`, `desired`, `max_virtual_size`, `idle_seconds` を出す。

禁止:

- 毎フレーム refetch しない。
- missing / 403 確定済み texture を無制限に再取得しない。
- callback を削除して loaded 扱いにしない。

#### P0: callback/create priority は face owner 非依存で守る

主介入点:

- `aya_texture_is_avatar_callback_priority()`
- `aya_texture_create_priority_score()`
- `LLViewerTextureList::updateImagesCallbacks()`
- `LLViewerTextureList::updateImagesCreateTextures()`

現状リスク:

avatar baked texture が mesh / face へ貼られる前は、face owner 走査だけでは avatar loading 用 texture と判定できない場合がある。そのため、未完了 baked callback が priority pre-pass から漏れ、通常 callback budget に埋もれる可能性がある。

処理:

- `FTT_SERVER_BAKE || FTT_HOST_BAKE` かつ `hasCallbacks()` は face owner 検出に依存せず avatar callback priority 対象にする。ただし bounded priority / quota 内で優先し、全 callback 枠を avatar baked だけで占有しない。
- `FTT_SERVER_BAKE || FTT_HOST_BAKE || BOOST_AVATAR_BAKED*` は create priority で protected bucket に入れる。
- `mNeedsCreateTexture=true` の baked callback texture は refetch ではなく create queue を先に進める。
- protected/avatar baked は backpressure drop、post-create downscale、downscale queue 実行から除外する。

2026-06-07 実装:

- `aya_texture_is_avatar_callback_priority()` で `FTT_SERVER_BAKE` / `FTT_HOST_BAKE` / `BOOST_AVATAR_BAKED` かつ `hasCallbacks()` の texture は、face owner 走査に依存せず priority callback pre-pass に載せる。
- 優先処理は既存の `priority_callback_limit = effective_max_callbacks` と time budget 内に収める。avatar baked callback があるからといって callback budget を無制限化しない。

#### P1: 表示側は「何かが見える」ことを保証する

主介入点:

- `LLDrawPoolAvatar::renderAvatars()`
- `LLVOAvatar::updateLOD()`
- `LLVOAvatar::renderSkinned()`
- `LLVOAvatar::renderRigid()`

方針:

- `allow_body_fallback suppress_body=false` は合格条件ではない。これは pass 0 で body fallback を許可しただけで、実際の mesh / skin draw 成立を示さない。
- visible / near で friends-only、mute、explicit do-not-render、`AOA_INVISIBLE` などの既存非表示対象ではない avatar は、5 秒以内に `complete_impostor`、`body/simple fallback`、`cloud` のいずれかがユーザーに見える必要がある。
- body/simple fallback が `num_indices==0` で成立しない場合は、cloud particle を残す。cloud OFF 設定でも代替表示がない場合は failure とする。

追加したい outcome ログ:

- `avatar_render_outcome mode=hidden|complete_impostor|body|simple_fallback|cloud|failed_no_draw`
- `reason=explicit_do_not_render|friends_only|aoa_invisible|impostor_complete|fallback_body|mesh_not_built|no_visible_parts|texture_not_visible|callback_wait`
- `num_indices_rigid`
- `num_indices_skinned`
- `impostor_complete`
- `is_impostor`
- `overall`
- `rez_status`
- `fully_loaded`

触るべきでないもの:

- `isFullyLoaded()` を fallback 成功で true にしない。
- `allBakedTexturesCompletelyDownloaded()` を low-res 到達で true 扱いしない。
- `isImpostor()` / `shouldImpostor()` を広域に false へ倒さない。
- friends-only、mute、explicit do-not-render を fallback で復活させない。

必要なら触る候補:

- fallback 対象かつ incomplete impostor の時だけ、`updateLOD()` の impostor early return を避け、mesh update が一度は走るようにする。
- `renderSkinned()` / `renderRigid()` で `num_indices==0` の場合に `failed_no_draw` を出し、cloud fallback へ戻す。

#### P1: texture memory 整理との両立条件

memory sweep / backpressure / downscale は残す。ただし avatar loading 中 baked texture と callback pending texture には次の invariant を置く。

- callback pending baked texture は raw / GL destructive sweep の対象外。
- callback pending baked texture は backpressure pending create drop の対象外。
- callback pending baked texture は post-create downscale / downscale queue の対象外。
- `mMaxVirtualSize=0` のまま放置しない。
- `RenderTextureGuardMaxCreatesPerFrame` / callback guard が有効でも、avatar baked callback/create の最低処理枠を確保する。
- world texture starvation を避けるため、avatar 専用に全枠を使い切らない。scene/world reserved create 枠は維持する。

評価は `destroyed_gl` や `scaled_down` の数だけで行わない。以下を同時に見る。

- `fetching=0 callbacks>0 has_gl=0 maxvs=0` の継続秒数。
- `avatar_bake_recovery event=start|refetch|create|complete|give_up`。
- `protected_avatar_baked_count`。
- `avatar_baked_refetch_count`。
- `avatar_baked_create_promoted_count`。
- `backpressure_dropped_avatar_baked` は常に 0。
- `downscaled_avatar_baked` は loading 中 0。
- `created_scene_reserved` が 0 に張り付かないこと。

#### 合格条件

avatar loading 側:

- visible / near で friends-only、mute、explicit do-not-render、`AOA_INVISIBLE` などの既存非表示対象ではない avatar で、`d=-1 desired=0 fetching=0 callbacks=1 has_gl=0 maxvs=0` が一定時間以上継続しない。
- recovery 後、同一 texture id で `maxvs>0`、次に `fetching=1` または create priority 昇格、最終的に `has_gl=1` / callback 減少へ進む。
- avatar 単位で `callback_loaded == callback_total`、`all_baked_downloaded=true`、`fully_textured=true`、`rez_status>=2` へ到達する。

UX 側:

- visible / near で friends-only、mute、explicit do-not-render、`AOA_INVISIBLE` などの既存非表示対象ではない avatar は、5 秒以内に `complete_impostor`、`body/simple fallback`、`cloud` のいずれかで存在が見える。
- `allow_body_fallback` だけでは合格にしない。`num_indices>0` または complete impostor または cloud 表示を outcome として確認する。
- `rez_status=1` が残っても、ユーザーに cloud/body/simple/impostor が見えていれば表示 fallback は合格。ただし loading 完了は別途 fetch/callback 側で追う。

不採用条件:

- 既存非表示対象ではない近距離/可視 avatar の name-only / invisible が 5 秒を超えて続く。
- callback pending baked texture が backpressure drop / downscale / destructive sweep される。
- missing / HTTP 403 を無限 refetch してログや queue を汚す。
- avatar 優先で world texture が starvation し、床/壁/HUD/近距離 object が読み込まない。

### テクスチャメモリ整理導入前後で問題になるもの

導入前:

- world / avatar / attachment texture は Firestorm 既存の texture stats、fetch priority、create queue、callback 処理に任される。
- 遠距離・視野外・旧リージョン由来 texture の GL/raw は残りやすく、テレポート後や混雑地で RSS が下がりにくい。
- 一度高解像度で作成された GL texture は残りやすいが、その分、再表示時の recreate/refetch は起きにくい。
- avatar baked callback は既存経路に乗るため、メモリは重いが、今回の `callbacks=1 fetching=0 has_gl=0` 停止は起きにくかったと推定する。

導入後:

- distance/offscreen cap は要求解像度と `mMaxVirtualSize` を下げ、数値上の desired discard を上げる方向へ寄せることで、world / scene texture を低解像度へ寄せる。
- memory sweep は saved raw image、raw image、GL texture の破棄と、downscale queue への投入を行う。これは disk texture cache / asset cache の purge ではなく、viewer 内の in-memory texture QoS である。
- stale/unreferenced/far/offscreen texture を捨てることでメモリ圧を下げられる一方、再表示時に GL recreate、raw decode、場合によっては cache/network refetch を増やす。
- queue guard は frame hitch を抑える一方、create/callback 処理枠を絞るため、avatar baked callback のような完了条件を持つ texture が優先枠から漏れると、ユーザーには cloud/gray/name-only として見える。
- AYA が HTTP 403 を作るわけではない。ただし raw/GL を破棄して再取得機会を増やすと、server/cache 側の 403/missing が実機で露出しやすくなる可能性はある。これは texture management の直接原因とは別列で記録する。

このため、テクスチャメモリ整理は「捨てれば軽くなる」ではなく、`破棄利益 - 再取得/再作成コスト - callback停止リスク` で評価する。単に `scaled_down` や `destroyed_gl` が増えたことは合格条件ではない。`oldest callback wait`、`fetching=0 callbacks>0 has_gl=0`、`recreated_after_destroy`、`refetched_after_drop`、`create_queue_now`、`downscale_queue_now`、`protected_avatar_baked_count` を同時に見る必要がある。

### 2026-06-07 チーム再議論による設計修正

前回までの議論には、「FPS が低い時に発動する」「teleport 直後や texture backlog がある時だけ発動する」という古い発想が残っていた。これはユーザーの期待と一致しない。チェックボックスを ON にしているなら、in-world では選択した profile が効いているべきである。

今後の確定方針:

- `FPS安定化 ON` は、in-world で AYA の固定軽量 profile を常時適用することを意味する。
- FPS、teleport、texture backlog、callback backlog は発動条件にしない。これらは telemetry / A/B 評価 / ログ分類にのみ使う。
- `AYALoadingResolutionReliefStrength` は FPS pressure ではなく固定 profile selector として扱う。
- 3D render target の倍率は、ON/OFF、profile 変更、in-world 出入り、安全除外への出入りでだけ変わる。FPS 値で上下させない。
- `Texture management ON` も、texture policy が常時 active であることを意味する。FPS 低下時の緊急処置ではない。
- Improve Graphics Speed / AutoTune は、AYA fixed profile が敷かれた後段で avatar / draw distance / ART / max non-impostors を調整する。AutoTune が active でも AYA fixed profile は止めない。

#### FPS安定化の新しい意味

`FPS安定化` は「FPS が悪くなったら介入する自動救済」ではなく、「指定した軽量 profile を in-world で敷く機能」である。FPS は結果であり、制御入力ではない。

固定 profile 案:

| profile | 3D render target 倍率 | World LOD |
| --- | ---: | --- |
| `0` | AYA 側は無効 | AYA 側は無効 |
| `1` | `0.85` | なし、または最小 |
| `2` | `0.75` | mild |
| `3` | `0.66` | standard |
| `4` | `0.50` | strong |
| `5` | `0.33` または `0.25` | 検証用最大。通常 UI から隠す候補 |

撤回対象:

- `AYALoadingResolutionReliefStrength` から `pressure_fps` を作る設計。
- `gFPSClamped < pressure_fps` を dynamic resolution の発動条件にする設計。
- `sqrt(current_fps / pressure_fps)` で倍率を再計算する adaptive multiplier。
- `adjust_down` / `adjust_up` / `adaptive_bucket` による FPS 追従。
- `teleport_window` / `startup_window` / `texture_backlog_window` を発動条件にする設計。
- `RestoreFPS` を復帰条件にする設計。

残すべきもの:

- 既存 `RenderResolutionMultiplier` 系の render target 縮小経路。
- ユーザー保存値と AYA session-only profile multiplier の分離。
- `effective_multiplier = min(user_saved_multiplier, aya_profile_multiplier)` の考え方。
- ログイン画面、XUI 初期化、snapshot / high quality render などの安全除外。
- 設定変更時だけ `gResizeScreenTexture` を立てる挙動。

#### Texture management の新しい意味

`テクスチャ読み込み・メモリ管理` は、FPS 低下時の緊急処置ではなく、常時の texture QoS policy として扱う。内部責務は分離する。

| policy | 責務 | 主な対象外 |
| --- | --- | --- |
| `World Texture Policy` | world / scene texture の distance cap、offscreen cap、far/offscreen/unreferenced の downscale | self、HUD、avatar bake、近距離/可視 avatar、selected/protected object |
| `Avatar Loading Protection` | 未ロード avatar の baked callback 維持、近距離/可視 avatar の保護、avatar loading 中 texture を cap/sweep から外す | world texture cap 値の決定、memory sweep の強度決定 |
| `Texture Memory Policy` | raw cleanup、stale GL cleanup、unreferenced / old-region / far-offscreen の整理 | loading 中 avatar、HUD、callback pending、create/downscale pending、raw 保存要求 |
| `Texture Queue Policy` | GL create / callback / backpressure の処理枠を整理し、avatar と world の starvation を防ぐ | avatar visibility policy、AutoTune 設定 |

avatar texture の扱い:

- self avatar と HUD は常時保護。
- 近距離/可視/未ロード remote avatar は保護。
- remote avatar baked texture は、今回の未ロード停止回帰対策として一旦 cap/downscale 対象から外す。読み込み完了後のみ対象へ戻す案は、実機で回帰解消を確認してから別段階で再設計する。
- attachment は一律保護しない。avatar 表示成立に必要な近距離/可視 attachment は保護し、遠距離/視野外の装飾 attachment は整理対象へ戻せる。

FPS を条件にしない代わりに使う条件:

- memory pressure。
- create / callback / downscale queue の soft/hard limit。
- unreferenced duration。
- teleport generation / old region 判定。
- far / offscreen stale duration。
- protected owner / HUD / avatar loading state。

#### Improve Graphics Speed との関係

AYA は Improve Graphics Speed / AutoTune の代替ではない。AYA は fixed profile の前段 baseline であり、AutoTune はその後段である。

| レイヤー | 所有するもの |
| --- | --- |
| AYA 3D fixed profile | 3D render target の固定低下、non-avatar world LOD |
| AYA texture QoS policy | world texture cap、texture queue / memory policy、avatar loading texture protection |
| Firestorm AutoTune / Improve Graphics Speed | `TargetFPS`, `AutoTuneFPS`, `AutoTuneLock`, `TuningFPSStrategy`, `RenderFarClip`, `RenderAvatarMaxART`, `RenderAvatarMaxNonImpostors`, avatar shadow / impostor policy |

AutoTune active 時も AYA fixed profile は維持する。AYA を止めると AutoTune の観測対象が変わり、AutoTune が `RenderFarClip` や avatar 設定を過剰に削る可能性がある。衝突を避ける方法は、AYA を止めることではなく、所有する設定を交差させないことである。

AYA が触らないもの:

- `TargetFPS`
- `AutoTuneFPS`
- `AutoTuneLock`
- `TuningFPSStrategy`
- `RenderFarClip`
- `RenderAvatarMaxART`
- `RenderAvatarMaxNonImpostors`
- `isImpostor()` / `shouldImpostor()` の AYA 都合での上書き

### 2026-06-07 UX 基準による計画立て直し

直近の旧実装の実機確認では、AYA loading 系実装は「最大アバター数を下げた時」よりもユーザー体験が悪く見える状態を作っていた。最大アバター数を下げた場合、表示される avatar と省略される avatar の境界は比較的一貫しており、ユーザーは「制限によって見えない」と理解しやすい。一方、旧 AYA 実装では、近距離 avatar が名前だけ残る、complete impostor 扱いなのに body/simple fallback が出ない、scene texture が後回しに見える、低解像度化と memory sweep の効果が体感しにくい、といった状態が混在していた。現行実装では avatar visibility / impostor policy を Firestorm 標準へ戻し、texture QoS は texture priority/cap/sweep に閉じる。

この状態は採用基準を満たさない。今後の計画は、平均 FPS 改善よりも先に以下を UX の下限として置く。

- 近距離/可視 avatar は、明示非表示、friends-only 非 friend、visual mute、AOA invisible などの既存非表示理由を除き、名前だけで長時間残してはいけない。
- avatar は cloud、body/simple fallback、usable impostor のいずれかで存在が分かる必要がある。cloud particle を OFF にする場合は、代替表示が成立していることをログで確認してから消す。
- `AYALoadingTextureManagementEnabled` は texture priority / cap / sweep の機能であり、avatar 本体の表示可否、mesh 構築、impostor 化そのものを直接所有しない。
- texture 低解像度化は「見えるものを粗くする」機能であり、「avatar や object を消す」機能ではない。
- `FPS安定化` は `RenderResolutionMultiplier` 系統と dense-world LOD を中心に扱い、avatar visibility policy や texture callback 完了条件を直接変えない。
- 既存 Firestorm の `RenderAvatarMaxNonImpostors` / AutoTune / impostor 系と競合する独自分岐は、最大アバター数を下げた時より悪い UX を作るなら不採用にする。

直近ログから分かった事実:

- `aya_simple_fallback=true` は現行ログで 0 件だった。したがって「簡易表示に落ちている」とは言えない。
- `not_loaded_no_fallback`, `aoa_invisible`, `explicit_do_not_render`, `friends_only_hidden`, `avatar_render_skip` 系は現行ログで 0 件だった。ログ上は明示非表示や mesh/drawable 未構築 early return ではない。
- 多くの avatar は `render_impostor=true`, `impostor_complete=true`, `suppress_body=true` へ流れている。つまり「非表示ログ」ではなく、complete impostor 扱いにより body pass が止まり、fallback がユーザーの期待通りに表面化していない。
- `allow_body_fallback` は出ているが一時的であり、多くは `render_impostor` / `suppress_body_pass` に戻る。`allow_body_fallback` が出たことだけでは合格にしない。

このため、次の実装方針は以下へ戻す。

1. Avatar visibility safety を texture memory 管理から切り離す。
   - texture 管理 ON だけで avatar を独自に impostor/body/simple へ振り分けない。
   - 近距離/可視 avatar の「存在表示」は Firestorm 既存の avatar render policy を基準にし、AYA 側は texture の低解像度到達ログと queue 優先度だけを補助する。
   - cloud particle OFF は、body/simple/usable impostor のいずれかが成立した後だけ許可する。代替表示が成立しないなら、cloud の方が名前だけ/不可視より UX がよい。
2. `complete impostor` を無条件に信じない。
   - texture callback 未完了、baked texture 低解像度到達も確認できない状態、または impostor が視覚的に usable か確認できない状態では、`suppress_body_pass` を合格扱いにしない。
   - ただし body/simple fallback を無理に出すことで既存 impostor policy を壊す場合は、独自 fallback を撤回し、Firestorm 既存の max avatar / impostor 挙動を優先する。
3. Texture management は texture に限定する。
   - fetch/decode/create priority、distance/offscreen cap、raw/GL sweep、teleport sweep を扱う。
   - avatar mesh、attachment mesh、object visibility、impostor eligibility を直接変更しない。
   - remote avatar baked texture の低解像度到達は texture QoS の品質観測・優先度評価に限定し、avatar loading/cloud 完了条件の代替にはしない。表示形態と loaded 判定は Firestorm 標準 policy に従う。
4. FPS安定化は描画負荷低減に限定する。
   - dynamic resolution は 3D render target のみを下げる。
   - dense-world LOD は non-avatar world object を主対象にし、avatar visibility と attachment visibility には触れない。
   - shadow throttle / impostor refresh throttle は既定 OFF または別 kill switch とし、UX 検証に通るまで通常 UI の強度に混ぜない。
5. 採用比較に「最大アバター数低下 baseline」を追加する。
   - `B0`: AYA 全 OFF。
   - `B1`: AYA 全 OFF + Firestorm 標準の最大アバター数/AutoTune/Improve Graphics Speed で軽量化。
   - `A1`: AYA FPS安定化のみ。
   - `A2`: AYA texture management のうち priority/cap/sweep の各単独。
   - AYA の候補は `B1` より avatar visibility、scene readability、操作感が悪いなら、FPS が上がっていても不採用にする。

新しい合格条件:

- 近距離/可視 avatar の `name_only_duration` は 5 秒以内。10 秒を超えたら failure。
- `aya_simple_fallback=true` または Firestorm 既存 simple/impostor 表示のどちらかで、簡易表示が成立していることを実機とログで確認する。
- `render_impostor=true` の場合は、`impostor_complete=true` だけでなく、body pass suppress 後もユーザーに存在が見えていることを確認する。
- texture management ON で scene texture が starvation しない。avatar だけを読み続け、床/壁/看板/HUD がいつまでも出ない状態は failure。
- memory sweep は RSS だけでなく、`destroyed_gl`, `destroyed_raw`, `scaled_down`, `queue oldest wait`, `refetch/recreate count` を見て、再確保ループを増やしていないことを確認する。

この基準に照らすと、旧案の「avatar fallback と texture memory 管理を強く結合した案」は過剰である。現行実装では、avatar visibility へ干渉していた AYA 独自分岐を撤回し、texture 管理を texture の priority/cap/sweep へ閉じ込めた状態で再検証する。

直近で確定した実装事実:

- `FPS安定化` は `AYALoadingFPSGuardEnabled` 配下の描画負荷低減として扱い、texture callback/create/downscale の guard budget を直接所有しない。
- `テクスチャ読み込み・メモリ管理` は `AYALoadingTextureManagementEnabled` を master とし、texture priority / backpressure / distance-offscreen cap / memory sweep を扱う。未ロード avatar の見せ方や impostor policy は Firestorm 標準側へ戻し、この master の責務に含めない。
- `AYALoadingTextureManagementEnabled=false` の時は、下位の priority/backpressure/cap/sweep 系 kill switch が ON でも、現行のテクスチャ管理経路は実動しない。A/B ではまず master の状態を見る。
- `AYALoadingTextureDownscaleStrength=0` の時は、distance/offscreen cap 系 kill switch が ON でも、低解像度化 cap / sweep の実効は限定される。cap を検証する run では `AYALoadingTextureManagementEnabled=true` と strength `1..5` を明示する。
- 未ロード avatar を描くかどうかは avatar visibility safety であり、texture cap/sweep の責務ではない。ただし、テクスチャメモリ整理が原因で近距離/可視 avatar が `rez_status=1` のまま名前だけになる回帰を避けるため、`LLDrawPoolAvatar` では限定的な unloaded body fallback を残す。
- `AOA_INVISIBLE` と明示的な do-not-render は fallback 対象外にする。限定 fallback は `AOA_NORMAL`、非 visual mute、近距離または可視、未ロード avatar に限り、impostor が未完成の時に body pass suppress を避けるための safety である。
- AYA 側で `isImpostor()` / `shouldImpostor()` を false 側へ倒す分岐は撤回済みである。現行は impostor policy 自体を書き換えず、未完成 impostor が body fallback を消してしまう箇所だけを避ける。
- 他人 avatar baked texture の avatar loading 完了条件は Firestorm 既存基準へ戻す。texture management は avatar loaded/cloud 判定を低解像度到達で代替しない。低解像度 cap は texture priority/cap/sweep の問題として扱い、avatar の loaded 状態とは分離する。
- callback や saved raw image を必要とする texture は、cap 候補であっても即時 `scaleDown()` / post-create downscale の対象から外す。avatar bake、preview、sculpt、保存要求のある raw image を潰さないためである。
- `scaleDown()` 自体が callback-safe なのではなく、現行実装は呼び出し側で `!hasCallbacks()` / `!needsToSaveRawImage()` を条件にして保護している。将来の call site 追加時もこの guard を外してはいけない。
- baked texture の取得は、default/invisible UUID なら既存 image を使い、URL が空なら host bake (`FTT_HOST_BAKE`)、URL があるなら server bake (`FTT_SERVER_BAKE`) へ分岐する。host bake fallback が URL fetch 経路で上書きされないことを検証する。
- texture create queue は avatar 関連が枠を食い切らないよう、`AYATextureCreateSceneMinCreatesPerFrame` / `AYATextureCreateSceneScanAhead` で non-avatar scene/world texture の最低 create 枠を持つ。
- 遠距離かつ視野外の loading remote avatar は、近距離/可視 avatar と同じ callback 維持・texture 保護には入らない場合がある。これは近距離/可視を優先する設計上のトレードオフであり、遠方 avatar の復帰遅延として観測され得る。
- AYA strength `1..5` でも、現行 safety では remote avatar baked texture は cap/downscale 対象から外す。avatar loading 完了条件は Firestorm 既存の full-res `discard == 0` 基準を維持し、world / scene texture の cap/downscale と分離する。

直近の実機観測として、ログイン画面フォームの起動直後崩れは一旦改善したように見える。一方で、まだ以下は「直った」とは扱わない。

- 一部 avatar の texture がいつまでも粗い、または一部だけ未読込のまま残る。
- 簡易表示へ落ちるべき avatar が、条件によって非表示に見える可能性がある。
- テレポート後や混雑地で、アバターを優先しすぎて scene texture が遅れる可能性がある。
- texture memory sweep が実際に queue 詰まりを解消しているか、単に downscale/destroy 候補を処理しているだけかは、ログで継続確認が必要である。
- teleport sweep は既定では raw 破棄と downscale queue への誘導が中心であり、未使用 pre-teleport GL texture の強制 destroy は `AYATeleportTextureSweepDestroyGLEnabled=true` の検証用動作である。メモリ使用量がすぐ下がらない場合でも、まず `teleport_texture_sweep` と `texture_work_stats` の処理数を見る。

直近 QA の扱い:

- ログインフォーム崩れは `startup_login_ui` による dynamic resolution 復帰と `FSPanelLogin` の holder rect 使用で一旦改善したように見える。ただし完了扱いではなく、default / ja / starlight / starlightcui、1000x700 付近、1024x768、起動直後数フレームで継続確認する。
- avatar texture は `all_baked_downloaded=true` または低解像度到達だけで「完全表示」と判定しない。full quality 到達、低解像度到達、一部 texture 未読込残留を分けて見る。
- avatar visibility は `allow_body_fallback` が出たことだけでは合格にしない。`skinned_not_built_initial`, `skinned_not_built_after_update`, `rigid_not_built`, `not_loaded_no_fallback`, `suppress_body_pass`, `aoa_invisible`, `render_impostor` を分類して、実際に body/simple/impostor/cloud のどれかで存在が見えるかを確認する。
- 合格条件は、近距離/可視の non-self avatar が mute / friends-only / 明示非表示を除き、cloud、gray/simple body、または usable impostor のいずれかで存在確認できること。名前だけが長時間続く状態は failure とする。

## 要約

「他のメタバースは読み込み中でも FPS が高く感じるのに、Second Life だけ極端に落ちる」という体感は、完全な錯覚ではない可能性が高い。

ただし、比較対象によっては「実世界を同じ密度で描画しながら高 FPS を維持している」のではなく、以下を組み合わせて高 FPS に見せている場合がある。

- ロード画面、ロビー、初期低 LOD、プレースホルダー、フォールバックアバターを先に描画する。
- 近傍だけを分割ロードし、遠方や重い要素をまだ描画しない。
- ダウンロード、デコード、依存関係解決を非同期化し、最終的な生成やインスタンス化だけを少しずつ行う。
- 重すぎるユーザー生成コンテンツを性能ランク、サイズ上限、モバイル制限、アバター非表示で制限する。
- FPS 表示が「ロード画面の FPS」であり、「完全に内容が出揃った世界の FPS」ではない。

Second Life/AYAstorm は、読み込み中でも実際のリージョン、アバター、オブジェクト、テクスチャ、シャドウ、LOD、UI、ネットワーク処理を同じメインループ上で進めるため、1 フレーム内にロード由来の処理が集中しやすい。特に macOS では OpenGL on Metal 経由のテクスチャ作成、FBO/PBO 系処理、シャドウ、状態ソート、ライティングがフレーム時間に乗りやすい。

## 調査した一次情報

### Unreal Engine

Epic の Level Streaming は、レベルを分割し、プレイ中に非同期でロード/アンロードし、表示状態を切り替える仕組みである。目的は巨大な世界を小さなチャンクに分け、必要な部分だけをメモリに置き描画すること。

出典: [Unreal Engine Level Streaming](https://dev.epicgames.com/documentation/en-us/unreal-engine/level-streaming-in-unreal-engine)

示唆:

- 「世界全体を一気にロードして描く」のではなく、必要領域だけに絞る設計。
- プレイヤーから見える範囲とロード範囲を設計側が管理できる。
- 正しく構成された場合、読み込みは体感上シームレスになりやすい。

### Unity / Addressables / AssetBundle

Unity Addressables は、キー解決、依存関係収集、リモート AssetBundle ダウンロード、メモリロードを非同期操作として扱う。Unity Support も、AssetBundle の同期ロードはメインスレッドをブロックしてヒッチを起こすため、`LoadAssetAsync` を使うよう説明している。ただし、非同期ロードしても GameObject の大量インスタンス化はヒッチになり得る。

出典:

- [Unity Addressables: Loading Addressable assets](https://docs.unity.cn/Packages/com.unity.addressables%401.19/manual/LoadingAddressableAssets.html)
- [Unity Support: Load assets asynchronously from AssetBundles](https://support.unity.com/hc/en-us/articles/207349003-How-can-I-load-Assets-asynchronously-from-AssetBundles-stored-in-disk)
- [Unity Addressables: Synchronous loading](https://docs.unity.cn/Packages/com.unity.addressables%402.2/manual/SynchronousAddressables.html)

示唆:

- 他アプリが高 FPS に見える場合、ロードの多くを非同期操作として処理している可能性が高い。
- ただし Unity でも最終的なシーン統合、生成、`WaitForCompletion`、大量インスタンス化はメインスレッドを止め得る。
- つまり「Unityだから絶対にロード中も滑らか」ではない。実装が悪いと普通に落ちる。

### Roblox

Roblox の Instance Streaming は、Workspace 内の 3D コンテンツやインスタンスを動的にロード/アンロードする仕組み。近傍や必要箇所を優先して、クライアント上の存在量を減らす。

出典: [Roblox Creator Hub: Instance streaming](https://create.roblox.com/docs/workspace/streaming)

示唆:

- 見えていない、または優先度が低いインスタンスをクライアントから外せる。
- 読み込み中に全部を保持、更新、描画しないため、体感 FPS を維持しやすい。

### VRChat

VRChat はユーザー生成アバターを許容する一方で、Avatar Performance Rank、Minimum Displayed Performance Rank、Fallback avatar、モバイル向け制限を持つ。一定条件ではアバター全体をフォールバックに置き換えたり、ライト、パーティクル、PhysBone、Constraint などを削除して表示する。

出典: [VRChat Creator Docs: Avatar Performance Ranking System](https://creators.vrchat.com/avatars/avatar-performance-ranking-system/)

示唆:

- 「重いものをそのまま全部出す」のではなく、重いコンテンツを非表示、置換、部分削除できる。
- 高 FPS に見える場合、実際には重いアバターやコンポーネントが表示されていない可能性がある。
- PC では既定が緩いが、ユーザー設定やモバイルではより強い制限が入る。

### Godot

Godot の Background loading は、ロード処理を `poll()` で段階的に進め、1 フレーム内でブロックする時間を制御する設計例を示している。例ではロード画面を表示しながら、一定時間だけ処理を進める。

出典: [Godot Docs: Background loading](https://docs.godotengine.org/en/3.0/tutorials/io/background_loading.html)

示唆:

- 単純な同期ロードではなく、ロード処理をフレーム間に分割するのが一般的な対策。
- 「ロード画面が滑らか」なのは、ロード対象の世界を完全描画しているからではなく、表示するものを軽くし、ロード処理に時間予算を与えているからでもある。

### Second Life Viewer

Second Life Wiki の Image System は、レンダーパイプラインが必要テクスチャ、discard level、優先度を決め、Texture Fetcher がキャッシュ、ネットワーク、受信、キャッシュ保存、デコードを行い、最終的にデコード済みテクスチャを OpenGL に渡す流れを説明している。Viewer Architecture では、メインスレッドが入出力メイン処理とレンダリングを担うと説明されている。

出典:

- [Second Life Wiki: Image System](https://wiki.secondlife.com/wiki/Image_System)
- [Second Life Wiki: Viewer Architecture](https://wiki.secondlife.com/wiki/Viewer_Architecture)

ローカルコード上でも、現在の AYAstorm では以下がメインループ内にある。

- `LLAppViewer::idle()` から `gObjectList.update(gAgent)` と `LL::GLTFSceneManager::instance().update()` を毎フレーム実行する。
  - `indra/newview/llappviewer.cpp`
- `LLViewerObjectList::update()` が active object の `idleUpdate()` を走らせる。
  - `indra/newview/llviewerobjectlist.cpp`
- `display()` 内で `gPipeline.generateSunShadow()` と `LLVOAvatar::updateImpostors()` が走る。
  - `indra/newview/llviewerdisplay.cpp`
- `LLViewerTextureList::updateImagesCreateTextures()` がデコード済み画像から GL texture を作る。
  - `indra/newview/llviewertexturelist.cpp`

## なぜ他メタバースは高 FPS に見えやすいのか

### 1. そもそも「ロード中に描いているもの」が違う

多くの現代エンジンでは、ロード画面、ロビー、プレースホルダー、低 LOD、フォールバックを先に出す。これは高 FPS に見えるが、実際には重い本体コンテンツをまだ描画していない。

Second Life はログイン直後から実リージョンに入る。灰色や未ロード表示であっても、オブジェクト更新、アバター更新、テクスチャ優先度計算、LOD、ネットワーク、描画パスが同時に走る。したがって「見た目は読み込み中」でも内部処理はかなり重い。

### 2. チャンク単位のストリーミング設計が違う

Unreal の Level Streaming や Roblox の Instance Streaming は、世界を分割し、必要箇所だけをロード/表示する前提が強い。

Second Life も可視性、距離、テクスチャ discard、object interest などは持つが、ユーザー生成リージョンに後付けで到着し、動的アバターや動的オブジェクト、スクリプト、テクスチャを一斉に受ける構造である。ゲームエンジンのように事前ビルドされたチャンクと依存関係を完全に制御できない。

### 3. 重いユーザー生成コンテンツを隠す仕組みの差

VRChat は Performance Rank や Fallback avatar で、重いアバターの表示を制限できる。Roblox も体験側、エンジン側でストリーミング範囲や存在量を制御しやすい。

Second Life は歴史的に「そこにあるものをできるだけ表示する」方向が強い。Avatar Complexity や impostor はあるが、VRChat のフォールバック置換ほど強く統制された体験にはなりにくい。

### 4. 非同期ロードでも最後の適用はメインスレッドに戻る

Unity の資料でも、非同期ロードはメインスレッドブロックを避けるための手段だが、大量インスタンス化や同期完了待ちはヒッチになるとされている。

SL/AYAstorm でも、ネットワーク受信や画像デコードに worker thread があっても、最終的な GL texture 作成、描画リソース更新、object idle、shadow/impostor 更新はメイン/レンダー側に戻る。ここが積み上がると FPS は落ちる。

### 5. macOS OpenGL on Metal の影響

AYAstorm の実測では、AppleMetalOpenGLRenderer 経由のパスがサンプルに出ていた。OpenGL API 自体は旧来の即時/状態機械的な設計で、Metal 上ではドライバ層が変換する。テクスチャ作成、readback、FBO/PBO、状態変更、同期点があると、フレーム時間に出やすい。

これは「SL だけが設計ミス」というより、古い OpenGL レンダラーを現代 macOS 上で動かす時の構造的コストも含む。

## 勘違いの可能性

以下の場合、「他メタバースは読み込み中でも高 FPS」と感じても、同条件比較ではない可能性がある。

### ロード画面の FPS を見ている

ロード画面は軽い UI や簡単な 3D 背景だけを描く。高 FPS でも、実際のワールド、全アバター、全テクスチャを描いているわけではない。

### 未ロードを非表示にしている

Roblox や VRChat では、近傍だけ、許可されたアバターだけ、性能制限を通った要素だけが表示されている可能性がある。Second Life のように「読み込み中の不完全な世界」をそのまま見せているわけではない。

### FPS 表示の計測対象が違う

メニュー、ロビー、ロード画面、ワールド内、混雑イベント、初回訪問、キャッシュ済み再訪問では意味が違う。比較するなら以下を揃える必要がある。

- 初回訪問かキャッシュ済みか
- 視界内アバター数
- テクスチャ総量
- GPU/CPU/メモリ状況
- ロード画面か実ワールド描画か
- FPS の取得元と平均化方法

### 他アプリも条件次第では落ちる

Unity の資料が示す通り、非同期ロードでも最終的な生成や同期待ちはヒッチを起こす。VRChat も公式ヘルプで、ロード中コンテンツやアバターが重いとパフォーマンス問題になることを説明している。つまり「他は常に高 FPS」は正しくない。

## AYAstorm 実測から見えたこと

今回の暫定計測では、読み込み中 FPS が 8 から 10 前後まで落ちた区間で、以下が主にフレーム時間を消費していた。

- `textures_avg_ms`: GL texture 作成/ダウンスケール由来のスパイク。
- `objectlist_avg_ms`: active object の idle update。
- `statesort_avg_ms`: 描画状態ソート。
- `lighting_avg_ms`: deferred lighting。
- `shadows_impostors_avg_ms`: sun shadow と avatar impostor 更新。
- `stream3d_avg_ms`: 0.02 から 0.07ms 程度で、今回の主因ではなかった。

テクスチャ処理にフレーム予算を入れた後、`textures_avg_ms` は大きく下がった。一方、FPS がまだ落ちる区間では `objectlist`、`lighting`、`statesort`、`shadows_impostors` が残った。これは「単一犯」ではなく、ロード中に複数のメインループ処理が同時に増える問題と見るべき。

## 仮説

### 仮説 A: SL はロードと描画の分離が弱い

SL は worker thread を使っているが、最終的に描画可能な状態へ持っていく処理がメイン/レンダー側に多く戻る。これがロード中 FPS 低下の直接原因になっている可能性が高い。

検証方法:

- `textures`, `objectlist`, `statesort`, `lighting`, `shadows_impostors` の各平均/最大 ms をログ化する。
- それぞれを個別に時間予算化し、FPS とロード完了時間のトレードオフを見る。

### 仮説 B: 他メタバースは「表示しない」ことで滑らかに見せている

他メタバースは、未ロードのコンテンツ、遠方、重いアバターを一時的に表示しない、または簡略表示する。SL はそれらを早く実表示しようとしてフレームを失っている可能性がある。

検証方法:

- SL 側で、固定 profile の 3D描画軽量化、texture create priority、distance/offscreen cap、memory sweep を個別に有効化し、何がフレーム時間を削るかを測る。
- FPS が保たれるか、ただし見た目の完成、近距離 avatar の存在表示、scene texture starvation がどれだけ悪化するかを測る。

### 仮説 C: ユーザーの体感は一部バイアスを含む

ロード画面やロビーが滑らかな体験と、SL の実リージョン読み込みを比較している可能性がある。

検証方法:

- 比較対象を「ワールド内に入った瞬間から、周囲が完成するまで」に固定する。
- キャッシュを消した初回訪問と、キャッシュあり再訪問を分ける。
- 1% low FPS、最低 FPS、ロード完了までの秒数を同時に取る。

## 修正方針

行き当たりばったりに複数箇所を同時に変えない。以下の順で、1 仮説につき 1 変更だけを検証する。

### 2026-06-02 計画レビューで見つかった穴

現時点の実装方針は方向性として妥当だが、以下を明示しないまま進めると、FPS は一時的に上がってもロード完了遅延、見た目の残留、別機能の劣化を見逃す危険がある。

2026-06-07 のチーム再議論で、この節の 1-3 は旧設計として撤回する。`FPS安定化` は「低 FPS になった後に入る guard」ではなく、「チェックボックス ON 中は in-world で固定 profile が効く lightweight policy」とする。ログイン直後、テレポート直後、カメラ大移動、視界内 avatar 急増は発動条件ではなく、評価ログの分類軸として扱う。

1. FPS を発動条件にすると、ユーザーが強度を設定したつもりでも実際には FPS 追従になり、render target resize と見た目の揺れを招く。
2. ログイン直後、テレポート直後、カメラ大移動、視界内 avatar 急増は「なぜ重いか」を分類する telemetry として必要だが、ON/OFF の gate にはしない。
3. FPS安定化の状態遷移は ON/OFF、profile 変更、in-world 出入り、安全除外出入りに限定する。ヒステリシスは FPS 復帰判定ではなく、profile 変更や safety transition の連続 resize を避ける目的に限る。
4. texture, object idle, shadow, impostor, callback を同時に絞ると、どれが効いたか分からなくなる。
5. visual な印象だけでは texture cap の効果判定ができない。ログで discard/downscale/queue/fps を見る必要がある。
6. 「メモリが下がらない」だけで失敗判定しない。GL/Metal driver cache のため、RSS は遅れて変わるか、変わらないことがある。
7. AutoTune との相互作用を分けて測る必要がある。AutoTune on での改善を AYA の FPS安定化の効果と誤認しない。
8. 起動が `initStrings()` で止まる問題は、FPS安定化の効果検証とは別の blocker として扱う。

### 追加すべき設計条件

実装を続ける場合、以下を前提条件にする。

- FPS安定化は、チェックボックス ON 中の in-world 固定 profile として扱う。`低 FPS` と `ロード直後時間窓` は発動条件にしない。
- FPS安定化の解除は FPS 回復ではなく、チェック OFF、profile 0、world 退出、login/XUI/snapshot などの安全除外で行う。
- 各 Phase と各機能系統は debug setting で単独 on/off できるようにし、A/B 比較できる状態を維持する。
- FPS安定化中でも P0/P1 は止めない。入力、カメラ、UI、近傍衝突、近傍 avatar の存在把握を優先する。
- queue が増え続ける場合は成功ではない。平均 FPS が上がっても、texture create/downscale/callback queue が発散するなら設定を戻す。
- ログは最低 120 秒取り、最初の 30 秒だけで結論を出さない。

### 2026-06-04: 機能分割方針

これまで `AYALoadingFPSGuardEnabled` を中心に、描画解像度低下、dense-world LOD、texture cap、create queue priority、backpressure、memory sweep をまとめて扱っていた。しかし実機検証と UI 検討の結果、これらを 1 つの「FPSを指定する」機能として見せると、ユーザーも検証者も効果と副作用を切り分けにくい。

分割する理由:

- `RenderResolutionMultiplier` と dense-world LOD は描画負荷を下げる機能であり、主な効果指標は `geom`, `lighting`, `statesort`, GPU/driver stall, FPS である。
- texture create priority / backpressure は、近距離 avatar/object/HUD を先に成立させるための読み込み順制御であり、主な効果指標は `created_near`, `queued_near`, `name_only avatar`, 近距離 object の表示完了である。
- distance/offscreen cap は queue の順序そのものではなく、遠距離/視野外の品質上限を下げる補助機能である。読み込み優先制御と併用できるが、効果判定では priority/backpressure と分けて見る。
- memory sweep / teleport sweep は、使わない GL texture / raw image を段階的に捨てるメモリ圧力対策であり、主な効果指標は RSS、downscale/destroy 件数、再読み込みの副作用である。
- これらを同時に ON/OFF すると、FPS が上がらない時に「機能が効いていない」のか、「読み込み優先は効いているが描画負荷が残っている」のか、「sweep が逆に再確保負荷を増やしている」のか判定できない。

2026-06-05 の再整理で、当初の `FPS安定化` / `読み込み優先制御` / `テクスチャメモリ整理` の 3 系統案は通常 UI から外し、`FPS安定化` と `テクスチャ読み込み・メモリ管理` の 2 系統にする。これは後続要件で「テクスチャプライオリティとテクスチャメモリ整理を一つの設定可能な項目として作り直す」と決めたためである。texture create priority と texture memory sweep は実機上セットで評価した方が「近いものを先に成立させ、不要なものを整理する」という利用者の意図に近い。通常 UI では内部処理名を出しすぎず、説明は体感ベースにする。

2026-06-06 の追加整理で、`FPS安定化` の数値入力を `目標 FPS` ではなく強度制へ変更する。ユーザーは「目標 FPS」と表示されると、その FPS まで上がる保証として受け取りやすい。しかし実際には CPU/GPU/サーバー応答/scene complexity の上限があり、viewer 側は負荷低減しかできない。このため通常 UI では `3D 解像度低下の強度` と `World LOD 低下の強度` を別スライダーにする。

2026-06-07 のチーム再議論で、`FPS安定化` は「低 FPS 時にだけ発動する adaptive guard」ではなく、「ON 中は in-world で固定 profile を敷く軽量 policy layer」へ再定義する。FPS、teleport、texture backlog は発動条件ではなく telemetry として扱う。`テクスチャ読み込み・メモリ管理` も FPS 低下時の緊急処置ではなく、ON 中に常時効く texture QoS policy として扱う。

| 表示名 | 通常 UI の説明 | OFF 時の意味 |
| --- | --- | --- |
| `3D描画を軽くする` | in-world で、選択した固定 profile に従って 3D world の内部描画解像度と non-avatar world LOD を軽くする。FPS 値では揺らさない | AYA 側では 3D render target / world LOD を下げない |
| `テクスチャ読み込みを整理する` | 近くの avatar、HUD、近くの object を保護し、遠距離/視野外/world texture と未参照 texture を常時 policy に従って整理する | AYA 側の texture priority / cap / sweep / queue QoS を使わない |

`FPS安定化` の通常 UI:

| 表示名 | 内部設定 | 意味 |
| --- | --- | --- |
| `3D解像度の下げ幅` | `AYALoadingResolutionReliefStrength` | 固定 profile の selector。例: `1=0.85`, `2=0.75`, `3=0.66`, `4=0.50`, `5=検証用最大`。FPS 値で倍率を再計算しない |
| `World LOD 低下の強度` | `AYALoadingLODReliefStrength` | non-avatar world object の LOD bias / forced refresh profile。avatar / attachment / HUD / selected object は対象外。FPS pressure ではなく固定 profile として扱う |

`テクスチャ読み込み・メモリ管理` の通常 UI:

| 表示名 | 内部設定 | 意味 |
| --- | --- | --- |
| `遠くのテクスチャを軽くする強さ` | `AYALoadingTextureDownscaleStrength` | world / far / offscreen / unreferenced texture の cap/downscale profile。未ロード avatar、自分、HUD、callback pending、protected texture は対象外。loaded remote avatar は遠距離/視野外時のみ対象へ戻せる |

debug/advanced 側の内部対応:

| 系統 | 内部処理候補 |
| --- | --- |
| `3D描画を軽くする` | fixed dynamic resolution profile, fixed non-avatar world LOD profile。shadow/impostor throttle は通常 UI から分離 |
| `テクスチャ読み込みを整理する` | `World Texture Policy`, `Avatar Loading Protection`, `Texture Memory Policy`, `Texture Queue Policy` |

設定名の扱い:

- 短期実装では既存の `AYALoadingFPSGuardEnabled` を互換のため残す。ただし文書上の意味は「すべての AYA loading 対策の master」ではなく、`FPS安定化` 側の互換 master へ縮小する。
- 後続実装で設定名を整理できるなら、`AYALoadingRenderReliefEnabled` を `FPS安定化` の明示 master とし、`AYALoadingFPSGuardEnabled` は移行 alias として扱う。
- `AYALoadingResolutionReliefStrength` と `AYALoadingLODReliefStrength` を追加する。`AYALoadingFPSGuardEnabled` が OFF の場合は両方とも動作しない。`0..5` の段階式だが、意味は pressure ではなく fixed profile selector である。`5` は検証用最大強度であり、通常 UI から隠す候補にする。
- `AYALoadingTextureManagementEnabled` を通常 UI の上位 master とする。`AYALoadingPriorityControlEnabled` / `AYALoadingTextureMemoryReliefEnabled` は互換用の旧設定として残すが、新規処理の gate には使わない。
- `AYATextureCreatePriorityEnabled`, `AYATextureCreateBackpressureEnabled`, `AYATextureDistanceCapEnabled`, `AYATextureOffscreenCapEnabled`, `AYATextureMemorySweepEnabled`, `AYATeleportTextureSweepEnabled` は内部 kill switch として残す。
- `AYALoadingTextureDownscaleStrength` を追加し、world / far / offscreen / unreferenced texture cap と sweep の低解像度化 profile を通常 UI から制御する。`0..5` の段階式とし、`0` は通常表示、`1..4` は cap 対象の上限を段階的に下げる。`5` は検証用最大強度であり、通常 UI から隠す候補にする。未ロード avatar、self、HUD、callback/save-raw/create/downscale pending、protected owner は cap/downscale/sweep から外す。loaded remote avatar は遠距離または offscreen の場合のみ対象へ戻せる。
- FPS 到達保証に見える UI 表現は避ける。通常 UI から `目標 FPS` 入力は外す。`pressure_fps` や `target_fps` は再設計後の制御入力として使わない。互換ログに残る場合も、過去実装由来の診断値として扱う。

検証上のルール:

- `3D描画を軽くする` OFF / `テクスチャ読み込みを整理する` ON の組み合わせを許可する。これは texture QoS 単独の効果を見るために必要である。
- `3D描画を軽くする` ON / `テクスチャ読み込みを整理する` OFF の組み合わせを許可する。これは fixed render profile 単独の効果を見るために必要である。
- memory sweep の副作用を見る場合は `AYATextureMemorySweepEnabled` を内部 kill switch として A/B する。通常 UI では texture priority と memory cleanup を1項目として扱う。
- ログは系統別に読む。再設計後は `profile_active`, `profile_id`, `profile_multiplier`, `user_render_resolution_multiplier`, `effective_render_resolution_multiplier`, `texture_policy_active`, `world_texture_policy_active`, `avatar_loading_protection_active`, `memory_policy_active`, `queue_policy_active` を出す。FPS は効果測定値であり、active/inactive や倍率変更の理由にしてはいけない。

検証マトリクス:

| 記号 | FPS安定化 | テクスチャ読み込み・メモリ管理 | cap/強度 | memory sweep | 主評価 | 見てはいけない誤解 |
| --- | --- | --- | --- | --- | --- | --- |
| `000` | OFF | OFF | OFF | OFF | baseline。既存挙動、最低 FPS、近距離表示完了、メモリ推移 | これ単体で改善判断しない |
| `100` | ON | OFF | OFF | OFF | `geom`, `lighting`, `statesort`, GPU/driver stall, FPS | 近距離 rez 改善を期待しない |
| `010` | OFF | ON | 強度 0 | 必要なら OFF | `created_near`, `queued_near`, `name_only_duration`, 近距離 avatar/object 表示完了 | FPS が大きく上がらないだけで失敗扱いしない |
| `011` | OFF | ON | 強度 1..5 | ON | RSS、`destroyed_*`, `scaled_down_*`, `cap_candidates`, 再読み込み副作用 | FPS安定化の効果として読まない |
| `110` | ON | ON | 強度 0 | 必要なら OFF | 描画負荷低減と近接優先が同時に成立するか | どちらが効いたかの単独効果として読まない |
| `111` | ON | ON | 強度 1..5 | ON | smoke test。相互作用と副作用の確認 | 採用判断の主比較に使わない |

cap 補助のサブマトリクス:

| 記号 | priority | backpressure | distance/offscreen cap | 主評価 |
| --- | --- | --- | --- | --- |
| `P00` | ON | OFF | OFF | 近距離 texture create 順序だけを見る |
| `P10` | ON | ON | OFF | queue 過多時の drop/defer が近距離保護を壊さないか見る |
| `C01` | OFF | OFF | ON | `AYALoadingTextureManagementEnabled=true` かつ `AYALoadingTextureDownscaleStrength>0` の条件下で、cap 単独の可読性劣化、カメラ回転 pop-in、downscale 反映を見る |
| `P11` | ON | ON | ON | priority/backpressure と cap 補助を併用した最終候補を見る |

比較条件:

- 同一リージョン、同一カメラ向き、同一グラフィック設定、AutoTune OFF、キャッシュ状態固定で 120 秒以上採取する。
- 各 run に `run_id`, `region`, `cache_state`, `camera_pos/orientation bucket`, `settings snapshot hash`, `AutoTune state`, `feature active states` を残す。
- sweep 系は `011` で単独確認するまで `000/100/010/110` では `AYATextureMemorySweepEnabled=false` にする。`AYALoadingTextureManagementEnabled=true` かつ `AYALoadingTextureDownscaleStrength>0` でも、再設計後の sweep / cap-enforcement の active 条件は memory pressure、texture-management enable/downscale-strength increase による bounded strength sweep、または cap enforcement に限定する。stale/unreferenced/offscreen/far は active sweep 内の bucket 分類であり、teleport generation は teleport texture sweep 側で扱う。FPS pressure は制御理由にしない。enable/strength-change sweep は distance/offscreen cap kill switch の少なくとも一方が有効な時だけ cap candidate を作り、起動時点の `mUUIDMap` を一周、または bounded retry 上限に到達するまで継続し、一時ブロック候補と downscale budget 超過候補は bounded retry する。
- distance/offscreen cap は cap サブマトリクス以外では明示的に OFF にし、可読性劣化を priority/backpressure の副作用と混同しない。

### 現状実装の位置付けと撤回対象

現時点のコードは、初期の「低 FPS を検知して一部処理を絞る low-FPS throttle」から複数系統へ分離済みである。ただし 2026-06-07 のチーム再議論により、以下の一部は次の実装で撤回または意味変更する。

- `FPS安定化`: 再設計後に通常 UI へ残す責務は、fixed dynamic resolution profile と non-avatar dense-world LOD profile である。現行コードに残る object idle throttle / avatar impostor refresh throttle との結合は撤回または debug/advanced の独立 kill switch へ隔離する。
- `FPS安定化` は texture callback/create/downscale の per-frame/time budget を直接絞らない。texture callback/create/downscale の guard budget、texture queue の priority / backpressure / cap / sweep は `AYALoadingTextureManagementEnabled` 側に限定する。
- dynamic resolution の旧 gate、つまり startup window、teleport window、texture backlog window、`loading_window && fps < pressure_fps` は撤回対象である。再設計後は FPS、teleport、backlog を発動条件にしない。
- dynamic resolution の旧復帰条件、つまり `AYALoadingDynamicResolutionMinHoldSeconds`、`AYALoadingDynamicResolutionRestoreFPS`、loading window 終了は通常 UI の制御から外す。再設計後は ON/OFF、profile 変更、in-world 出入り、安全除外出入りだけで状態遷移する。
- dense-world LOD の旧 gate、つまり `mouselook || fps_latch_active` は撤回または別 profile として明示する。再設計後は `FPS安定化 ON && in_world && LOD profile>0 && !safety_excluded` を基本条件にする。
- sun shadow throttle は FPS安定化から既定で切り離す。`AYALoadingSunShadowThrottleEnabled=false` のままなら、FPS安定化 ON だけで `RenderSunShadowGuardFrameInterval` は使われない。影が遅れてついてくる副作用を避けるためである。
- `テクスチャ読み込み・メモリ管理`: 現行コードは `AYALoadingTextureManagementEnabled` を master とし、priority reorder / backpressure / distance-offscreen cap / memory sweep を制御する。再設計後は `World Texture Policy`, `Avatar Loading Protection`, `Texture Memory Policy`, `Texture Queue Policy` に分離する。
- `AYALoadingFPSGuardMinFPS` は render relief のユーザー向け target ではない。再設計後は texture cap / memory sweep の制御入力からも外し、互換診断値に落とす。

未解決の重要リスク:

- 2026-06-06 の実機報告では、テレポート後に process memory が約 15GB まで増えているにもかかわらず、周囲の texture が一向に表示されない状態が観測されている。これは単純な LOD 低下ではなく、旧リージョン由来 texture / saved raw / GL texture の破棄不足、texture fetch/create/callback queue の詰まり、または sweep が cap 候補を処理しても queue 側の詰まりを解消できていない可能性として扱う。
- テレポート後の検証では、`memory_sweep_stats` の `destroyed_gl`, `destroyed_raw`, `scaled_down`, `cap_candidates`, `image_count` と、`texture_work_stats` / `create_backpressure` の queue 数、near/far/offscreen 処理数を同時に見る。メモリ使用量だけで「破棄できている」と判断しない。
- 2026-06-06 21:16 の crash report では、`LLViewerTextureList::updateImagesCreateTextures()` の AYA backpressure/reorder 経路で `mCreateTextureList.front()` を raw pointer に落としてから `pop()` し、その後で `LLPointer` 化していたため、queue が最後の参照だった texture で use-after-free になった。backpressure、priority reorder、scene reserve の pop 経路は、`LLPointer<LLViewerFetchedTexture>` で参照を保持してから `pop()` し、lambda 戻り値も `LLPointer` にする。
- 追加レビューで同種の lifetime risk が `mAYATeleportSweepQueue` と `mDownScaleQueue` にも確認されたため、queue front は `LLPointer<LLViewerFetchedTexture>` で保持してから処理する。`shutdown()` では `mCreateTextureList`, `mDownScaleQueue`, `mAYATeleportSweepQueue` を明示的に flush し、pending flag と active flag を落としてから image map/list を clear する。memory sweep の cap candidate は `updateImageDecodePriority(..., false, false)` とし、flush による delete 後の `scaleDown()` を避ける。
- ログイン画面のフォームが起動直後に一瞬崩れる症状は、texture priority/backpressure/cap/sweep だけでは説明しきれない。現行修正では、`LLStartUp::getStartupState() < STATE_STARTED` に加えて、login holder に child が残っている間も `aya_update_dynamic_resolution_guard()` を早期 return し、dynamic resolution が active なら `event=exit reason=startup_login_ui` で復帰させる。これにより、保存済みまたは直前状態の AYA dynamic resolution がログイン XUI/media の初期 layout 中に render target resize を起こす経路を止める。
- ログイン画面の geometry 側は、`FSPanelLogin` が full viewer rect ではなく `login_holder` の local rect を使って初期化/reshape する。NUI login XUI は default/starlight/starlightcui で 1000x700 を基準にし、HTML 領域 1000x500、bottom stack 1000、左右 elastic pad 15、login/password container `min_width=206` とする。日本語 XUI は localized override として login/password container の `min_width=206` を持つ。これは低解像度化の workaround ではなく、1000x700 の最小想定で XUI が 1024x768 前提のまま生まれてから潰れるのを避けるための layout 修正である。
- `STATE_STARTED` gate は texture list / individual texture、avatar callback 維持、dense-world LOD など、world/avatar 向けの処理全般に適用する。ログイン前や startup/precache 中に 3D描画軽量化や texture QoS が動くと、ログイン UI や progress UI の初期 layout と競合する可能性がある。ただし、ログイン画面の崩れ対策としては `STATE_STARTED` だけでは足りず、ログイン panel がまだ表示されている `STATE_STARTED` 後の数フレームも守る必要がある。
- 近距離 avatar が cloud のまま残る症状は、mesh の LOD ではなく baked texture callback、attachment pending、texture create/downscale/fetch の詰まりでも起きる。したがって、avatar mesh を描く/描かないの問題と texture memory 管理を混同しない。現行 safety では remote avatar baked texture の `desired_discard` は 0 を維持し、低解像度 cap で avatar loading 完了条件を代替しない。
- 現行実装では、non-self avatar でも baked texture 完了条件は Firestorm 既存の full-res `discard == 0` 基準へ戻している。低解像度到達は texture QoS の優先度・ログ・品質観測だけで扱い、avatar の `loaded/cloud/rez status` 判定を所有しない。検証では full quality 到達、低解像度到達、missing image / callback pending を分けて読む。
- 2026-06-06 の追加実装では、`AYALoadingTextureManagementEnabled=true` かつ GL texture create guard が効いている時でも、avatar 関連 texture が create 枠を食い切らないように、`LLViewerTextureList::updateImagesCreateTextures()` で最低 `AYATextureCreateSceneMinCreatesPerFrame` 件の non-avatar scene/world create を予約する。予約枠を満たせない時は `AYATextureCreateSceneScanAhead` 件まで先読みし、scene/world 候補を promote する。検証ログは `texture_work_stats created_scene_reserved`, `created_avatar_related`, `scene_promoted` を見る。`created_avatar_related` だけ増えて `created_scene_reserved=0` が続くなら、scene texture starvation が残っている。
- 「テクスチャを読み込む前は impostor でよい」が、「簡易表示可能な avatar まで render cost の too-slow 判定だけで impostor に落ち、ユーザーから名前だけ/不可視に見える」のは今回の UX 要件に反する。旧実装では `AYALoadingTextureManagementEnabled=true` の間に AYA 側で `isImpostor()` / `shouldImpostor()` を false 側へ倒す案を入れたが、実機ログでは `aya_simple_fallback=true` が 0 件で、`render_impostor=true`, `impostor_complete=true`, `suppress_body=true` が支配的だった。現行実装では、impostor policy 自体は書き換えず、未完成 impostor が body fallback を消す場合だけ suppress しない。
- `AYADisableAvatarLoadingCloudParticles=true` は「読み込み cloud particle を描かない」ための独立設定であり、avatar のロード完了条件そのものではない。再設計後は texture QoS と自動連動させない。drawpool 側の `aya_render_unloaded_fallback` は、texture management が壊した未ロード停止の安全策として限定的に残し、`AOA_INVISIBLE`、明示 do-not-render、friends-only、visual mute などの既存非表示理由は尊重する。
- ただし fallback は万能ではない。`overall_appearance == AOA_INVISIBLE`、明示的 do-not-render、friends-only 非 friend、visual mute、または mesh/drawable が未構築の avatar は描けない。これらは `AYA avatar_draw_state` と `AYA avatar_loading_wait` のログで区別する。実機で「簡易表示に落ちない」と見えた場合は、`aoa_invisible`、`explicit_do_not_render`、`not_loaded_no_fallback`、`render_impostor`、`allow_body_fallback` のどれを踏んでいるかをまず確認する。
- texture cap/downscale の安全条件として、`hasCallbacks()` または `needsToSaveRawImage()` が true の texture は、`LLViewerLODTexture::processTextureStats()` の即時 `scaleDown()` と、`LLViewerTextureList::updateImagesCreateTextures()` の post-create downscale から外す。これは「cap 候補では raw/GL を破棄しない」という意味ではなく、callback/raw 保存を必要とする texture をその場で潰さないという意味である。`scaleDown()` 自体が callback-safe なのではなく、call site の guard によって安全を作っている。安全条件を外して強制破棄すると、bake、preview、sculpt、特殊 texture callback の完了通知を壊す可能性がある。
- `AYA avatar_gray_decloud` のように texture QoS 側から avatar の cloud / fully-loaded 状態を直接外す補助は撤回する。低解像度 baked texture の到達確認は texture callback / priority / discard ログで行い、avatar の loaded/cloud 状態は Firestorm 既存の avatar loading 条件に戻す。

### 採用/不採用の共通基準

採用条件:

- 最低 FPS が悪化せず、1% low FPS が baseline 比で少なくとも +20% 改善する。ただしテクスチャ読み込み・メモリ管理単独 (`010`) では FPS 改善を必須にしない。
- `textures`, `objectlist`, `shadows_impostors`, `statesort`, `lighting` のどれが改善したかログで説明できる。
- ロード完了時間の悪化が baseline 比 +15% 以内である。
- queue が 120 秒時点で継続的な増加傾向を示さない。
- AutoTune off の状態でも効果が確認できる。
- callback 最古待ち秒数は 10 秒以内、近距離 avatar の `name_only_duration` は 5 秒以内を暫定目安にする。
- object idle 一巡秒数は暫定 10 秒以内、近距離 shadow/impostor stale は暫定 2 秒以内、遠距離 shadow/impostor stale は暫定 10 秒以内に収まる。
- 近距離看板、装着物、HUD 以外の UI 的な面、mouselook 時の自分の装備が、指定秒数内に読める。

不採用または再調整条件:

- 平均 FPS は上がるが、ロード完了が極端に遅くなる。
- texture callback が詰まり、灰色表示や未完成 avatar が長時間残る。
- 非 avatar object idle の間引きで animated texture、media surface、script-driven visual の破綻が出る。
- shadow/impostor が古いまま残り、ユーザーが移動判断を誤るレベルになる。
- 距離 cap により近距離看板、UI 的な面、ショップ商品などの可読性が落ちる。

### 追加したい必須ログ項目

現在の `AYAPerf`, `AYATextureCap`, `AYALoadingFPSGuard`, `AYATextureQueue` だけでは、guard の副作用を十分に判定できない。以下は現行ログではなく、次の実装/検証前に追加したい項目である。

- run metadata: `run_id`, `sample_time_sec`, `region`, `cache_state`, `camera_pos/orientation bucket`, `settings_snapshot_hash`, `AutoTune state`
- feature state: `render_relief_active`, `dynamic_resolution_active`, `dense_world_lod_active`, `texture_management_active`, `memory_sweep_active`, `teleport_sweep_active`, `texture_downscale_strength_level`, `profile_id`, `profile_multiplier`, `effective_render_resolution_multiplier`, `in_world`, `safety_excluded`, `active_reason`, `strength_change`, `memory_pressure`, `queue_backlog_metrics`, `sweep_bucket_counts`, `teleport_sweep_generation`
- texture create/downscale: queue 現在値、queue 最大値、可能なら最古 item 待ち秒数
- texture callback: callback queue 現在値、processed、skipped、time_limit_hit、count_limit_hit、最古 callback 待ち秒数
- distance cap: `faces`, `clamped_faces`, `aux_faces`, face 数上限で cap 対象外にした件数、大型 face/中心距離による cap 件数
- object idle: active non-avatar 数、updated、visited、skipped、cursor 一巡推定秒数
- shadow: updated/skipped、使用 interval、guard による skip か low FPS throttle による skip か
- impostor: candidates、updated、skipped、最古更新待ち秒数、近距離/遠距離別件数
- avatar loading: visible avatar 数、gray/cloud/loading avatar 数、fully baked 到達数、attachment 数。現行実装では avatar loading 完了条件は Firestorm 既存の full-res `discard == 0` 基準を維持する。low-res 到達は texture QoS の品質観測ログとして別に扱い、過去ログや texture management OFF の結果と比較する時は、full quality 到達時間と low-res 到達時間を分けて読む。
- AutoTune: on/off、strategy、変更された `RenderFarClip`, `RenderAvatarMaxART`, `RenderAvatarMaxNonImpostors`

検証ビルドではログ既定 ON でよいが、通常配布へ入れる場合は `AYAPerf*Log` と `AYATextureDistanceCapLog` の既定値を OFF に戻し、検証時だけ有効化する。

### ロールバック設定

実機で副作用が強い場合は、コードを戻す前に以下の setting で機能単位に切り分ける。

| 対象 | 無効化 setting |
| --- | --- |
| FPS安定化全体 | 短期互換では `AYALoadingFPSGuardEnabled = false`。後続整理後は `AYALoadingRenderReliefEnabled = false` |
| 3D 内部解像度の自動低下 | `AYALoadingDynamicResolutionEnabled = false` |
| dense-world LOD | `AYALoadingDenseWorldLODEnabled = false` |
| 旧 adaptive 3D 内部解像度低下 | `AYALoadingDynamicResolutionLowFPSEnabled = false`。再設計後は通常 UI と接続しない |
| 未ロード avatar 演出の抑制 | `AYADisableAvatarLoadingCloudParticles = false`。texture QoS / avatar visibility policy の責務には含めない |
| テクスチャ読み込み・メモリ管理全体 | `AYALoadingTextureManagementEnabled = false` |
| texture create priority | `AYATextureCreatePriorityEnabled = false` |
| texture create backpressure | `AYATextureCreateBackpressureEnabled = false` |
| 距離 texture cap | `AYATextureDistanceCapEnabled = false` |
| 視野外 texture cap | `AYATextureOffscreenCapEnabled = false` |
| 補助マップ cap | `AYATextureDistanceCapAuxEnabled = false` |
| texture downscale cap | `AYALoadingTextureDownscaleStrength = 0` |
| texture memory sweep | `AYATextureMemorySweepEnabled = false` |
| teleport texture sweep | `AYATeleportTextureSweepEnabled = false` |
| texture create 制限 | `RenderTextureMaxCreatesPerFrame` / `RenderTextureCreateTimeSliceMs` と、テクスチャ管理 ON 時の `RenderTextureGuardMaxCreatesPerFrame` / `RenderTextureGuardCreateTimeSliceMs` を確認する |
| texture downscale | `RenderTextureMaxDownscalesPerFrame = 0` で通常 downscale を止められる。テクスチャ管理 ON 時は `RenderTextureGuardMaxDownscalesPerFrame` / `RenderTextureGuardDownscaleTimeSliceMs` を使う |
| texture callback 制限 | `RenderTextureMaxCallbacksPerFrame` / `RenderTextureCallbackTimeSliceMs` と、テクスチャ管理 ON 時の `RenderTextureGuardMaxCallbacksPerFrame` / `RenderTextureGuardCallbackTimeSliceMs` を確認する |
| object idle throttle | `RenderObjectIdleMinFPS = 0` |
| shadow throttle | `RenderSunShadowMinUpdateFPS = 0` |
| impostor throttle | `RenderAvatarImpostorMinUpdateFPS = 0` |
| impostor per-frame 上限 | `RenderAvatarMaxImpostorUpdatesPerFrame = 0` |

注意:

- `RenderObjectIdleMinFPS = 0`, `RenderSunShadowMinUpdateFPS = 0`, `RenderAvatarImpostorMinUpdateFPS = 0` は旧 low-FPS throttle を止めるだけで、fixed 3D描画軽量化 profile とは別である。
- 個別 throttle を完全に止めて A/B 比較する場合は、短期互換では `AYALoadingFPSGuardEnabled = false`、後続整理後は `AYALoadingRenderReliefEnabled = false` も併用する。ただし avatar impostor policy は Firestorm 標準側へ戻すため、AYA 通常 UI の強度 profile に混ぜない。
- `RenderAvatarMaxImpostorUpdatesPerFrame` は FPS しきい値とは別に通常時も効く。impostor 更新数制限を完全に戻す場合は `0` にする。
- `RenderTextureMaxDownscalesPerFrame` と `RenderTextureGuardMaxDownscalesPerFrame` は設定値をそのまま使う。`0` は downscale 停止、`1` は `1/frame`、`4` は `4/frame` である。Guard 側は FPS安定化単独では使わず、テクスチャ読み込み・メモリ管理 ON 時に使う。
- downscale time slice も設定値をそのまま使い、内部の最小値はタイマー比較のための `0.001ms` だけにする。
- texture create/callback は個数上限と time slice の両方で止まるため、片方だけ上げても切り戻しとして不十分な場合がある。

## 読み込み中の表示優先順位

ロード中にすべてを同じ優先度で表示しようとすると、フレーム時間が破綻する。AYAstorm では「ユーザーが操作を続けるために必要な情報」と「見た目の完成度」を分け、以下の順で表示/更新するのが妥当である。

### P0: 操作と安全に必要な情報

最優先。これが遅れると、FPS が出ていても操作不能に感じる。

- 自分のアバター位置、向き、カメラ、入力反応。
- 地形、床、壁、階段、衝突判定に関わる近傍オブジェクト。
- 移動先の足元と進行方向の低 LOD 形状。
- UI、チャット、通知、ログイン/テレポート状態。
- 最低限の近傍 avatar 位置と名前表示。

方針:

- 低 LOD、単色、未テクスチャでもよいので、位置と形状を先に出す。
- P0 はフレーム予算を削りすぎない。ここを間引くと体感が悪化する。

### P1: 近傍の存在理解に必要な情報

次点。世界の構造や人の存在を把握するために必要。

- 近距離の建物、家具、ドア、椅子、イベント会場の主要構造物。
- 近距離 avatar の基本姿勢、シルエット、impostor/簡易表示。
- 看板や案内など、行動判断に影響する近距離テキスト/面。
- 近距離オブジェクトの中解像度テクスチャ。

方針:

- まず低解像度テクスチャで表示し、詳細テクスチャは後回しにする。
- avatar は完全表示より、低コストのシルエット/簡易 impostor を優先する。

### P2: 見た目の品質を上げる情報

FPS が安定してから進める。ロード中に急いで入れるとヒッチの主因になる。

- 高解像度テクスチャ。
- マテリアル、normal/specular/PBR 補助情報。
- 遠景オブジェクトの詳細 LOD。
- avatar の詳細 attachment、複雑な alpha、髪、装飾。
- animated texture、particle、media surface。

方針:

- FPS が閾値を下回る間は、作成数と処理時間を強く制限する。
- 近距離でも「操作に不要な装飾」は P2 に落とす。

### P3: 後回しにしてよい演出

ロード中は積極的に遅延または停止する。完成後に段階的に戻す。

- sun shadow map の頻繁な更新。
- reflection probe / mirror 系の更新。
- 遠距離 avatar impostor の高頻度再生成。
- 高品質 lighting の追加パス。
- 遠距離オブジェクトの texture downscale / upscale 調整。

方針:

- FPS が低い間は、既存結果を使い回す。
- 数フレームから数十フレーム単位で間引く。
- 見た目の古さより、入力応答と最低 FPS を優先する。

### P4: 完全表示のための仕上げ

ロード完了後、または FPS が十分に戻った後で実行する。

- 遠景の高解像度化。
- 全 avatar attachment の完全表示。
- shadow/reflection/impostor の通常頻度復帰。
- texture discard の最終調整。
- render complexity 表示や詳細統計の更新。

方針:

- FPS が安定してから段階解除する。
- 一気に解除せず、数秒単位で戻す。

### 実装上の優先ルール

ロード中のスケジューラは、単純な FIFO ではなく以下の重みを使うべきである。

1. 距離: 近いものを先に出す。
2. 画面占有率: 大きく見えているものを先に出す。
3. 操作影響: 衝突、移動、クリック対象を先に出す。
4. 人の存在: 近距離 avatar の位置とシルエットを先に出す。
5. 視線方向: カメラ正面を優先し、背後は落とす。
6. コスト: 高コスト要素は分割し、1 フレームに集中させない。
7. 完成度: 高解像度、影、反射、装飾は最後に回す。

重要な訂正:

- 距離別 texture cap は「近いものを先に読み込む」実装ではない。これは既に face/texture stats に乗った texture の要求解像度を下げる処理であり、fetch/decode/create/callback/render queue の順序を直接変えない。
- 近距離 avatar や近距離 object が名前だけ、または低 LOD のまま長時間残る場合、見るべき問題は texture cap ではなく queue scheduling である。
- AYAstorm のロード対策は、cap/downscale だけでなく、fetch、decode、GL create、callback、material、object update の各 queue で P0/P1 を先に処理する必要がある。
- loading cloud particle を OFF にしても、avatar の実ロード優先度は上がらない。OFF は演出の削減であり、近距離 avatar を先に rez させる仕組みではない。

### Queue の扱い

ロード中の queue は「処理順」と「破棄/縮小候補」を分けて扱う。

先に処理する対象:

- 近距離 avatar の存在、名前、姿勢、baked texture、最低限の表示。
- 近距離 object の形状、衝突、クリック対象、看板/案内など行動判断に必要な面。
- カメラ正面、足元、進行方向の object。
- 既に画面内で大きく見えている diffuse/basecolor texture。

後回しにする対象:

- 遠距離 object の高解像度 texture。
- normal/specular/PBR 補助マップ。
- avatar の装飾 attachment、髪、alpha が重い装備など、存在把握に不要な詳細。
- shadow、reflection、遠距離 impostor の更新。

破棄または縮小候補:

- 遠距離 texture。
- 視野外 texture。
- テレポート前リージョン由来で、到着先で再利用されていない texture。
- face/volume 参照がなく、callback/raw保存/fetch/fast cache 中でもない texture。
- GL create queue に残っているが、現在 scene から参照されていない通常 LOD texture。

破棄してはいけない対象:

- 近距離 P0/P1 の avatar / object を成立させるために必要な texture、mesh、callback。
- baked avatar texture、avatar appearance 完了に必要な callback。
- HUD、preview、sculpt、media、local texture、保存要求のある raw image。
- 現在 face/volume 参照があり、近距離または画面内で使われている texture。

この分類により、queue が増えた時に「何でも止める」のではなく、P0/P1 を前に出し、遠距離・視野外・旧リージョン由来・参照なし候補を破棄/縮小してフレーム時間とメモリを取り戻す。

### AYAstorm での暫定ロードポリシー

| 優先度 | 表示対象 | 許容する仮表示 | policy active 時の扱い |
| --- | --- | --- | --- |
| P0 | 自分、カメラ、UI、足元、衝突形状 | 低 LOD/単色可 | 間引かない |
| P1 | 近距離構造物、近距離 avatar 位置 | 低解像度 texture / Firestorm 標準 impostor | 優先 bucket に上げる |
| P2 | 高解像度 texture、material、attachment | 低解像度/未適用で可 | 強度 profile に従って品質上限を下げる |
| P3 | shadow、reflection、遠距離 impostor | 前回結果の使い回し | 明示 throttle が ON の場合だけ更新頻度を落とす |
| P4 | 遠景仕上げ、最終 LOD 調整 | 未完成で可 | 遠距離・視野外・旧リージョン・参照なし候補から縮小/破棄する |

この順序にすると、ロード中の見た目は一時的に粗くなるが、操作可能な状態を先に作れる。Second Life の従来挙動は P2/P3 も早期に同時進行しがちで、ここが低 FPS の原因になっている可能性が高い。

2026-06-02 の実機観察メモ:

- 近くに少数 avatar しかいないのに名前だけ見えて本体が出ない状態は、近接優先が成立していない兆候として扱う。
- この状態で avatar loading cloud particle が OFF だと、未ロード中の演出が消えるため症状が目立つ。ただし、particle を戻すことは根本対策ではない。
- 根本対策は、近距離 avatar / 近距離 object を fetch/decode/create/callback/render queue で先に処理し、遠距離・視野外・旧リージョン由来・参照なし候補を破棄/縮小対象にすることである。
- 現行実装では、near / in-frustum avatar 関連 face は texture create priority や memory sweep 側で保護・優先される。さらに、near / in-frustum の未ロード remote avatar owner は `protected_owner` に入り、distance/offscreen/aux cap を回避する。ただし、全 remote avatar / all attachments を常時保護する設計ではない。近距離 avatar の本体/装着物が frustum 判定の揺れや auxiliary cap で低解像度扱いに落ち、名前だけ表示される状態が残る場合は、owner 保護判定と実際の face 所属をログで突き合わせる。

## 既存の Improve Graphics Speed / AutoTune の評価

Firestorm/AYAstorm には `Improve Graphics Speed` floater があり、現在 FPS と自動調整 UI を表示する。これは有用な機能だが、実装を見る限り「ロード中の各処理をフレーム予算内に収めるスケジューラ」ではない。そのため、今回のような読み込み中の 10fps 級低下に対しては、効きが薄く見える可能性が高い。

### 実際に調整している主な項目

実装上、AutoTune が主に触るのは以下である。

- `RenderAvatarMaxART`: avatar の描画時間しきい値。
- `RenderAvatarMaxNonImpostors`: 非 impostor avatar 数。
- `RenderFarClip`: 描画距離。
- `AutoTuneRenderFarClipMin` / `AutoTuneRenderFarClipTarget`: 描画距離の下限と復帰目標。
- `AutoTuneImpostorFarAwayDistance` / `AutoTuneImpostorByDistEnabled`: 距離ベースの avatar impostor 関連。
- `TargetFPS`, `AutoTuneFPS`, `AutoTuneLock`, `TuningFPSStrategy`: AutoTune の基準 FPS と自動調整モード。

一方で、`RenderReflectionDetail` を調整する経路は残っているが、現在の調整処理では `#if 0` で無効化されている。つまり、現状の主戦場は avatar と描画距離である。

### 効く可能性が高いケース

- 混雑地で avatar 描画が支配的な場合。
- 遠景の描画負荷が高く、`RenderFarClip` を下げるだけで負荷が減る場合。
- AutoTune の基準 FPS を十分高く設定し、continuous tuning を有効にしている場合。

この条件では、AutoTune が avatar を簡略化し、描画距離を落とすことで FPS が改善する可能性がある。

### 効きが薄い可能性が高いケース

今回のロード中 FPS 低下で疑っている主因は、AutoTune が直接制御していない領域に多い。

- texture の GL 作成、decode 後の upload、downscale。
- object list の `idleUpdate()` 集中。
- shadow map 更新。
- avatar impostor の生成頻度そのもの。
- state sort / lighting pass の一時的な集中。
- GLTF scene manager や network callback 後のメインスレッド処理。

これらは P2/P3 のロード処理であり、AutoTune の avatar/draw-distance 調整だけでは直接止まらない。したがって、FPS が 10 以下まで落ちる瞬間の内訳が `textures`, `objectlist`, `statesort`, `lighting`, `shadows_impostors` に寄っているなら、`Improve Graphics Speed` の効果が薄いという体感は妥当である。

### 注意点

- `TargetFPS` のデフォルトは低めで、標準状態では 40fps や 60fps を基準値として使う設定とは限らない。
- floater に表示される現在 FPS は、機能がその FPS を維持できることを意味しない。
- AutoTune は後追いの調整であり、ログイン直後やテレポート直後の突発的な読み込みスパイクを事前に分散する仕組みではない。
- UI には `Avatars Only` と `Avatars and Scene` が見えるが、実装上の調整範囲は広い意味の scene 全体ではなく、主に draw distance である。
- 2026-06-02 時点で起動中の別ビルドの `AYAstorm.log` には、14-15fps 付近への低下が複数回記録されているが、AutoTune による設定変更を示す明確なログは確認できなかった。
- 同ログには `floater_fs_performance.xml` の `flaoter.string` typo と、`panel_fs_performance_nearby.xml` の `FSRenderAvatarMaxART` に対して C++ 側が `RenderAvatarMaxART` を探す子コントロール名不一致も出ている。これは FPS 低下の主因ではないが、floater の品質問題として別途直すべきである。

### 検証手順

同じリージョン、同じカメラ方向、同じキャッシュ状態で以下を比較する。

1. AutoTune off の基準値を取る。
2. AutoTune on、continuous on、`AutoTuneFPS=30/45` を基準値として、strategy `Avatars and Scene` で取る。
3. `RenderFarClip`, `RenderAvatarMaxART`, `RenderAvatarMaxNonImpostors`, FPS, AYAPerf 内訳を同時に記録する。
4. FPS が改善した瞬間に、avatar 簡略化か draw distance 低下が起きているか確認する。
5. 改善しない場合、同じ時間帯の `textures`, `objectlist`, `statesort`, `lighting`, `shadows_impostors` を見る。

判定:

- avatar/render-distance が支配的なら、既存 AutoTune は効く。
- texture/object/shadow/loading callback が支配的なら、既存 AutoTune では不足。

### 結論

`Improve Graphics Speed` は残す価値がある。ただし、これは「混雑時の描画負荷を落とす機能」であって、「ロード中に P0 から P4 へ優先順位を付けてフレーム予算を守る機能」ではない。AYAstorm で必要なのは、AutoTune を置き換えることではなく、AutoTune の外側に読み込み中の描画負荷制御とテクスチャ読み込み・メモリ管理を追加し、texture/object/shadow/impostor の処理量を測定値に応じて制御することである。

### Phase 0: 計測だけ

目的:

- 挙動変更を入れず、現在の AYAstorm でロード中のフレーム時間内訳を取る。

採取する値:

- `FPS`
- `display total_avg_ms/max_ms`
- `textures_avg_ms/max_ms`
- `objectlist_avg_ms/max_ms`
- `statesort_avg_ms/max_ms`
- `lighting_avg_ms/max_ms`
- `shadows_impostors_avg_ms/max_ms`
- `stream3d_avg_ms/max_ms`
- ロード完了までの時間
- キャッシュ状態
- リージョン名、視界内 avatar 数

### Phase 1: Texture budget

仮説:

- GL texture 作成/ダウンスケールがメインスレッドを止めている。

変更:

- `RenderTextureCreateTimeSliceMs`
- `RenderTextureDownscaleTimeSliceMs`
- `RenderTextureMaxCreatesPerFrame`
- `RenderTextureMaxDownscalesPerFrame`

採用条件:

- `textures_avg_ms` と `textures_max_ms` が下がる。
- 最低 FPS が上がる。
- 近距離 avatar / 近距離 object が遠距離・視野外・旧リージョン由来 texture より先に表示される。
- テクスチャ完成時間の悪化が許容範囲内。
- create/downscale/callback queue と最古待ち秒数が発散しない。
- callback queue ログを追加してから判定する。

実装メモ:

- `AYALoadingFPSGuardEnabled` と `AYALoadingFPSGuardMinFPS` を追加した。
- 現行実装では `FPS安定化` 発動中でも `RenderTextureGuardCreateTimeSliceMs`, `RenderTextureGuardMaxCreatesPerFrame`, `RenderTextureGuardDownscaleTimeSliceMs`, `RenderTextureGuardMaxDownscalesPerFrame`, `RenderTextureGuardCallbackTimeSliceMs`, `RenderTextureGuardMaxCallbacksPerFrame` は使わない。これらは `AYALoadingTextureManagementEnabled` ON 時の texture create/callback/downscale 予算として使い、`FPS安定化` 単独 A/B に混ぜない。
- texture queue の近接優先は `AYALoadingTextureManagementEnabled` 配下の priority reorder / backpressure / distance-offscreen cap / memory sweep で扱う。
- queue 内の順序が FIFO のままなら、遠距離・視野外・旧リージョン由来の作業が近距離 avatar/object より先に処理され続ける可能性があるため、priority/backpressure のログと avatar 表示完了時間を必ず併読する。

取りこぼしやすいリスク:

- texture 管理側で create/backpressure を強くしすぎると FPS は上がっても灰色表示が長く残る。
- queue の優先順位を変えずに create/backpressure だけ絞ると、近距離 avatar / object の表示も遅くなり、名前だけ見えて本体が出ない状態が悪化する。
- callback を絞りすぎると、ロード完了通知、bake/preview/特殊 texture の後処理が遅延する可能性がある。現行コードでは FPS安定化単独では callback 予算を絞らず、テクスチャ読み込み・メモリ管理 ON 時だけ guard budget を使う。
- downscale を絞りすぎると、一度作られた高解像度 GL texture が残り続ける。
- downscale を増やしすぎると、逆に GL/Metal 側の再確保コストでヒッチが戻る。
- `addTextureStats(0.f)` の reset 挙動変更は距離 cap には必要だが、既存の texture priority reset 経路にも影響するため、過剰な再 fetch や priority 低下が出ないか見る。

追加で見るログ:

- `AYATextureCap texture_work_stats created/downscaled/create_queue_now/downscale_queue_now`
- `AYAPerf display textures_avg_ms/textures_max_ms`
- 灰色表示が残る時間
- texture 完成までの秒数

#### Phase 1.1: Camera-distance texture cap と破棄

目的:

- 混雑地で一度近距離/大画面占有として読み込まれた texture が、カメラを引いた後も高解像度のまま残り続ける問題を抑える。
- 「読み込みに時間がかかる」のは許容しつつ、読み込み完了後に FPS が 20 未満へ残留する状態を減らす。
- 近距離の視認性は維持し、チャット範囲外や遠景は積極的に低解像度化する。
- これは近接優先 scheduler の代替ではない。距離 cap は遠距離/視野外の品質を下げる部品であり、近距離 avatar/object を queue の先頭へ送る機能ではない。

暫定設定:

| カメラ距離 | 最大要求解像度 | 設定 |
| --- | ---: | --- |
| `<= 10m` | `2048px` | `AYATextureDistanceCapNearMeters`, `AYATextureDistanceCapNearResolution` |
| `<= 20m` | `1024px` | `AYATextureDistanceCapMidMeters`, `AYATextureDistanceCapMidResolution` |
| `<= 50m` | `512px` | `AYATextureDistanceCapFarMeters`, `AYATextureDistanceCapFarResolution` |
| `> 50m` | `256px` | `AYATextureDistanceCapDistantResolution` |

実装メモ:

- `LLViewerTextureList::updateImageDecodePriority()` で face ごとのカメラ距離を見て、HUD attachment 以外の texture の `vsize` を距離別 cap に丸める。
- HUD attachment は対象外にする。HUD は距離で品質を落とすと UI として破綻しやすい。
- `BOOST_HIGH` 以上の boosted texture は既存ロジック通り対象外にする。preview、baked、特殊用途の texture を不用意に落とさないため。
- 同一 texture が複数 face で使われる場合、最終的な要求は各 face の最大 `vsize` が勝つ。近距離 face と遠距離 face が同じ texture を共有している場合、近距離側の要求が残る。

実装上の重要な注意:

- 現実装は `objp->isHUDAttachment()` だけを除外しているため、厳密には「world texture だけ」ではない。
- avatar attachment、装着看板、mouselook 時の自分の装備、HUD ではない UI 的オブジェクトにも距離 cap が効く可能性がある。
- face 中心とカメラ距離で cap を決めているため、大きな床、壁、ステージ背景、看板では「近くに見えているが中心が遠い」面を低解像度化する可能性がある。
- face 数が多すぎる texture は既存の `max_faces_to_check` 回避により cap 判定から外れ、高解像度要求が残る可能性がある。共有 atlas や繰り返し texture ほど対象外になりやすい。

追加の回帰確認:

- 近距離大型看板、床、壁、イベントステージ背景。
- avatar attachment、髪、装着ライト、装着 UI、クリック装備。
- mouselook で見える自分の装備。
- HUD ではないが UI 的に使われる面、ショップ商品、案内表示。
- preview、sculpt、media surface、bake、inventory texture picker、local texture。

破棄の流れ:

1. 距離 cap により `mMaxVirtualSize` が小さくなる。
2. `LLViewerLODTexture::processTextureStats()` が `mMaxVirtualSize` から `mDesiredDiscardLevel` を上げる。
3. 現在の `discard` が desired より高品質なら `scaleDown()` を呼ぶ。
4. `LLViewerTextureList::updateImagesCreateTextures()` の downscale queue で `LLImageGL::scaleDown()` が実行される。
5. GL texture は小さいサイズへ再確保され、元の大きい GL texture 領域は解放対象になる。

今回見つかった問題:

- `LLViewerTexture::addTextureStats(0.f)` は reset 意図のコメントがあったが、実装上は `virtual_size > mMaxVirtualSize` の時だけ更新していたため、0 では `mMaxVirtualSize` が下がらなかった。
- `LLViewerFetchedTexture::setKnownDrawSize()` はコメント上 `0,0` で無効化できるはずだが、実装上は大きい方向にしか更新せず、既存値を消せなかった。
- `LLViewerLODTexture::processTextureStats()` は `mKnownDrawWidth/Height` がある場合、`mMaxVirtualSize` ではなく known draw size を優先して discard を計算していた。そのため、一度近距離で 2048 相当に見た texture は、カメラを引いて距離 cap が 512/256 を要求しても downscale されにくかった。

修正:

- `addTextureStats(0.f)` を実際に `mMaxVirtualSize = 0.f` の reset として扱う。
- `setKnownDrawSize(0,0)` で `mKnownDrawWidth/Height` を clear できるようにする。
- `AYALoadingTextureManagementEnabled && AYALoadingTextureDownscaleStrength>0 && AYATextureDistanceCapEnabled` が有効で `mMaxVirtualSize` が有効値を持つ場合、LOD texture の discard 計算では `known draw size` と `mMaxVirtualSize` の小さい方を使う。

これにより、カメラを引いた後に距離 cap が `mDesiredDiscardLevel` へ反映され、既に高解像度で作成済みの GL texture も downscale queue に入りやすくなる。

注意点:

- これは「距離が離れた瞬間に全 texture を即時破棄する」強制 purge ではない。downscale はフレーム予算内で順に処理する。
- `RenderTextureMaxDownscalesPerFrame` を 0 にすると、破棄要求が出ても実際の GL downscale が進まない。テクスチャ管理 ON 時は `RenderTextureGuardMaxDownscalesPerFrame` も実効予算になる。
- 通常の raw image は GL texture 作成後に破棄される。`mSavedRawImage` は callback、preview、bake、sculpt 等の用途があるため、無条件破棄しない。Phase 1.7 では安全判定を通った候補だけ条件付きで破棄する。
- Apple の Activity Monitor 上のメモリ値は、GL/Metal driver cache や purgeable memory の影響を受けるため、downscale 直後に単純に RSS が下がるとは限らない。効果確認は FPS、AYAPerf の `textures_*`、GL texture discard 状態、長めのメモリ推移で見る。

#### Phase 1.2: 実機で距離 cap が見えにくい理由と追加対応

2026-06-02 の実機観察では、カメラ距離に応じて texture 品質が明確に変わっているようには見えなかった。この観察は実装上も不自然ではない。距離 cap は描画済み texture をその場で強制破棄する機能ではなく、face ごとの `vsize` と texture の desired discard を下げ、downscale queue 経由で GL texture を小さくしていく機能である。

見た目に反映されにくい主な理由:

- 同じ texture が近距離 face と遠距離 face で共有されている場合、近距離側の高い要求が勝つ。
- HUD、preview、baked、callback、raw 保存が必要な texture は不用意に落とせない。
- `known draw size` や min discard の影響で、距離 cap が desired discard へすぐ反映されない経路があった。
- downscale queue の処理数が `1/frame` 相当だと、FPS 10 前後では数百から数千 texture の破棄反映が非常に遅い。
- GL/Metal driver cache の影響で、downscale 後も Activity Monitor のメモリ値や見た目がすぐ変わらないことがある。

追加対応:

- `LLViewerTextureList::updateImageDecodePriority()` に `AYATextureCap` ログを追加し、`faces`, `clamped_faces`, `aux_faces`, 距離帯別件数、`downscale_queue` を 10 秒間隔で出す。
- `LLViewerTextureList::updateImagesCreateTextures()` に `texture_work_stats` ログを追加し、`created`, `redundant_creates`, `downscaled`, queue 最大/現在値、guard 状態、downscale 上限、FPS を出す。
- `RenderTextureMaxDownscalesPerFrame` の既定値を `4` に上げる。`RenderTextureGuardMaxDownscalesPerFrame` はテクスチャ管理 ON 時の guard 予算として残す。
- ユーザー設定は実効 downscale 数へそのまま反映する。`0` は明示的な downscale 停止、`1` は `1/frame`、`4` は `4/frame` として扱う。
- downscale time slice も設定値をそのまま反映する。極端に低い値は検証用であり、距離 cap の反映速度と再確保負荷を A/B で見る。

不足している追加対応:

- face 数上限により cap 対象外になった texture 件数をログ化する。
- face 中心距離によって cap された大型面の件数、または大きな `vsize` が cap された件数をログ化する。
- callback queue の詰まりを別ログに分ける。texture create/downscale が改善しても callback が詰まると見た目の完成は遅れる。

#### Phase 1.3: Normal / specular / PBR 補助マップの扱い

material の補助マップは、diffuse/basecolor と同じ見た目優先度で扱うべきではない。近距離の識別には diffuse/basecolor が最重要であり、normal/specular/metallic-roughness/emissive はロード中 FPS が低い間は P2 として後回しにできる。

実装上、normal/specular/PBR material map は `LLFace` の texture channel を通って `LLViewerTextureList::updateImageDecodePriority()` の同じループに入る。今回の追加では、`LLRender::DIFFUSE_MAP` と `LLRender::BASECOLOR_MAP` 以外の channel を auxiliary material map とみなし、通常 texture より低い距離 cap を適用する。

暫定 auxiliary cap:

| カメラ距離 | 補助マップ最大要求解像度 | 設定 |
| --- | ---: | --- |
| `<= 10m` | `1024px` | `AYATextureDistanceCapAuxNearResolution` |
| `<= 20m` | `512px` | `AYATextureDistanceCapAuxMidResolution` |
| `<= 50m` | `256px` | `AYATextureDistanceCapAuxFarResolution` |
| `> 50m` | `128px` | `AYATextureDistanceCapAuxDistantResolution` |

注意:

- channel `1` は normal map と alternate diffuse map が重なるため、現時点の実装では alternate diffuse 用途も auxiliary 扱いになる可能性がある。
- これはロード中 FPS とメモリ圧を下げるための暫定策であり、見た目の副作用が出る場合は channel の意味を material type ごとに分けて判定する必要がある。
- 補助マップの cap が効いているかは、見た目だけでは判断しにくい。`AYATextureCap` の `aux_faces`, `clamped_faces`, `downscaled`, `downscale_queue_now` を見る。

追加の穴:

- `LLFace` の texture channel だけで auxiliary 判定すると、legacy material、PBR、alternate diffuse の意味を完全には区別できない。
- normal/specular を落としても diffuse/basecolor が高解像度のままなら、見た目の差は小さく、メモリ削減だけが主効果になる可能性がある。
- emissive は視認性に関わる看板や案内表示で使われる場合があるため、常に P2 扱いするとユーザーの行動判断を悪化させる可能性がある。

次の実装判断:

- 現状の broad auxiliary cap は検証用とし、正式採用は保留する。
- emissive/alternate diffuse/装着 UI/看板の回帰確認が終わるまでは、通常配布では `AYATextureDistanceCapAuxEnabled=false` も候補に残す。
- `aux_faces` が多く、かつ可読性劣化が出る場合は、material 種別を見て normal/specular/ORM と emissive/alternate diffuse を分ける。
- 看板/案内の emissive 劣化が確認されたら、emissive は diffuse/basecolor と同じ cap に戻す。

#### Phase 1.4: 視野外 texture cap

距離だけで texture 解像度を決めると、カメラの裏や画面外の近距離 object が高解像度要求を維持する。これは「動ける FPS を守る」という目的に反するため、frustum 外の face は距離帯 cap よりさらに低い上限を重ねる。

暫定設定:

| 対象 | 最大要求解像度 | 設定 |
| --- | ---: | --- |
| 視野外 diffuse/basecolor | `256px` | `AYATextureOffscreenCapResolution` |
| 視野外 normal/specular/PBR 補助マップ | `128px` | `AYATextureOffscreenCapAuxResolution` |

実装メモ:

- `LLViewerTextureList::updateImageDecodePriority()` で `LLFace::mInFrustum == false` の face に offscreen cap を適用する。
- HUD attachment は距離 cap と同様に対象外にする。
- 視野外 cap は距離 cap と併用する。例えばカメラの裏にある 5m の object は near `2048px` ではなく offscreen `256px` が勝つ。
- ログは `AYATextureCap distance_cap_stats` に `offscreen_faces` を追加する。

注意:

- 視野外 cap は「見えていないものを先に粗くする」ための処理であり、カメラを振り向いた瞬間に再 fetch/downscale 復帰が起きる可能性がある。
- 視野外でも同一 texture が視野内 face と共有されている場合、視野内側の高い要求が勝つ。
- カメラ端の object は frustum 判定の揺れで cap が切り替わる可能性があるため、ログと実機でポップインを確認する。

#### Phase 1.5: テレポート後の pre-teleport texture sweep

現在の viewer はテレポート時に `LLViewerTextureList::clearFetchingRequests()` で fetch request を消すが、すでに GL texture として作られた UUID texture は global texture list に残る。テレポート先でも同じ UUID が表示されるなら保持すべきだが、前リージョンでだけ使われていた texture まで高解像度のまま残す必要はない。

実装方針:

- テレポート表示開始時に、既存の通常 LOD texture を snapshot queue に入れ、`mAYATeleportSeenGeneration` を付ける。
- 到着後の decode/cache 処理が始まった texture は、`updateImageDecodePriority()` で face 使用が確認された時に `mAYATeleportUsedGeneration` を付ける。
- `AYATeleportTextureSweepGraceSeconds` 経過後、snapshot queue を `AYATeleportTextureSweepMaxTexturesPerFrame` の範囲で走査する。
- テレポート先で再利用された texture、まだ face/volume 参照がある texture、boost 付き、callback/raw 保存/fetch/fast cache 中の texture は downscale しない。
- 安全に未使用と判断できた通常 LOD texture は、まず saved raw image を破棄し、`AYATeleportTextureSweepDestroyGLEnabled=true` なら GL texture も `destroyGLTexture()` で破棄する。既定ではこの強制破棄は OFF とし、通常は `mMaxVirtualSize=0` として `processTextureStats()` に通し、既存の downscale queue に送る。

設定:

| 設定 | 既定値 | 意味 |
| --- | ---: | --- |
| `AYATeleportTextureSweepEnabled` | `true` | テレポート後の pre-teleport texture sweep を有効化する。分離検証では `011` 以外で明示的に OFF にする |
| `AYATeleportTextureSweepGraceSeconds` | `20.0` | 到着側の再利用判定を待つ猶予秒数 |
| `AYATeleportTextureSweepMaxTexturesPerFrame` | `128` | 1 frame に検査する snapshot texture 数 |
| `AYATeleportTextureSweepMaxQueuedTextures` | `2048` | snapshot queue に保持する最大候補数 |
| `AYATeleportTextureSweepDestroyGLEnabled` | `false` | テレポート先で再利用されなかった pre-teleport GL texture を実破棄する。強い検証用スイッチ |
| `AYATeleportTextureSweepLog` | `true` | start/progress/complete をログ出力する |

見るログ:

- `AYATextureCap teleport_texture_sweep event=start queued=...`
- `AYATextureCap teleport_texture_sweep event=progress scheduled=... destroyed_gl=... destroyed_raw=... kept_used=... kept_active=... kept_special=...`
- `AYATextureCap teleport_texture_sweep event=complete downscale_queue=...`

注意:

- `AYATeleportTextureSweepDestroyGLEnabled=true` の場合は強制 purge に近い動きになるため、既定 OFF とする。無効時の GL texture 縮小は既存の `mDownScaleQueue` と、通常時は `RenderTextureMaxDownscalesPerFrame`、テクスチャ管理 ON 時は `RenderTextureGuardMaxDownscalesPerFrame` に従って進む。
- face/volume 参照が残っている texture は保守的に保持するため、前リージョン由来の texture が一部残る可能性はある。
- テレポート先で同じ UUID が再利用されている場合は `kept_used` として保持される。
- メモリ表示は driver cache の影響を受けるため、効果判定は FPS、`downscaled`, `downscale_queue_now`, `teleport_texture_sweep scheduled` を合わせて見る。

#### Phase 1.6: 近接優先 texture / object queue scheduler

2026-06-02 の実機観察で、近距離に少数 avatar しかいないのに名前だけ見えて本体が出ない状態が確認された。これは「距離 cap が見た目に効いていない」というだけではなく、近接 avatar/object を queue の先頭へ送る仕組みが不足している可能性を示す。

目的:

- 近距離 avatar / 近距離 object を、遠距離・視野外・旧リージョン由来・参照なし候補より先に表示する。
- texture management ON 中は、P0/P1 の存在把握を先に成立させる。
- queue が発散する場合、遠距離・視野外・旧リージョン由来・参照なし候補を破棄/縮小して、近接表示の処理枠を空ける。

重要な前提:

- loading cloud particle の表示/非表示は未ロード状態の見せ方であり、texture QoS の責務ではない。`AYADisableAvatarLoadingCloudParticles` は別系統の avatar visibility / UX safety 設定として扱い、テクスチャ読み込み・メモリ管理 ON と自動連動させない。
- 未ロード avatar の drawpool fallback は avatar visibility policy であり、texture QoS の責務ではない。ただし、texture management 回帰で近距離または可視の未ロード avatar が `LLDrawPoolAvatar::render()` の `!isFullyLoaded()` early return で名前だけになる場合に限り、限定 safety として body fallback を許可する。
- avatar attachment を一律で保護対象にするのは誤り。近距離 avatar の存在把握に必要なものと、遠距離/視野外の装飾 attachment は分ける。
- 近距離かつ視野内の avatar 本体・avatar baked texture・avatar attachment は、queue priority では protected bucket に上げ、memory sweep でも近距離 avatar face として保持する。near / in-frustum の未ロード remote avatar owner は `protected_owner` に入り、distance/offscreen cap を回避する。ただし全 remote avatar / all attachments 常時保護ではないため、遠距離・視野外の avatar attachment は保護対象から外れ得る。
- 遠距離・視野外の avatar attachment は、active sweep 内では通常の遠距離・視野外 bucket 候補として扱う。sweep の active 条件そのものは memory pressure、bounded strength sweep、cap enforcement に限定し、teleport generation は teleport texture sweep 側で扱う。全 attachment 保護に戻すと混雑地でメモリと create queue を占有し、近距離表示を逆に遅らせるため。
- texture cap は「品質上限」であり、queue の「処理順」ではない。近接優先は fetch/decode/create/callback/object update 側で実装する。

実装方針:

- texture create queue に priority score を導入する。FIFO のままではなく、近距離・画面内・大画面占有・avatar bake/基本表示に必要なものを先に処理する。
- score は少なくとも `distance`, `in_frustum`, `pixel_area`, `boost level`, `face/volume ref`, `avatar/basic object relevance`, `teleport generation` を見る。
- GL create queue が soft limit を超えた場合は、遠距離・視野外・旧リージョン由来・参照なし候補を先に drop/downscale 候補にする。
- callback queue の P0/P1 優先は必要条件として残す。現行実装では callback queue 全体の priority sort ではなく、texture management 中に avatar baked / avatar boost / unloaded avatar face owner の callback を先に処理する優先パスを追加している。FPS stabilization 単独では callback priority/budget を所有しない。ただしこの優先パスは通常 callback と同じ per-frame/time budget を消費する。したがって保護 callback が多すぎると、通常 callback の進行を圧迫し得る。完全な queue sort と詳細ログは今後の追加対象である。
- object update は、近距離 avatar と近距離 object の transform/visibility/LOD を優先する。遠距離 active object、media、particle、decorative update は、独立した object update policy が有効な場合だけ間引く。texture management 単独では object visibility を直接変えない。

実装済み:

- `LLViewerTextureList::updateImagesCreateTextures()` で pending GL texture create queue を priority score 順に並べ替える。
- score は `FTT_SERVER_BAKE/FTT_HOST_BAKE`, protected boost, raw保存/sculpt, callback, face 距離、frustum、pixel area、avatar 関連 object を見る。
- 全 queue を毎回 sort せず、`AYATextureCreatePriorityScanLimit` 件を上限に先頭・中央・末尾 window を評価して window reorder する。2万件級 queue で reorder 自体が FPS を潰さないようにしつつ、新しく queue 末尾に入った近距離 texture と、camera 移動などで queue 中央に残ったまま近距離化した texture を救出できるようにするため。
- 中央 window は reorder ごとに probe 位置を進める。巨大 queue の middle にいる候補を全件採点せず、複数 frame で順番に拾う。
- 中央・末尾 window から前方へ救出するのは near / mid / protected bucket のみ。far / offscreen / unreferenced を前方へ送って近距離候補を圧迫しない。
- backpressure は先頭候補を pop/push で末尾へ回さない。scan window 内で安全に cancel できたものだけ破棄し、残りは元の順序で queue 先頭へ戻す。
- callback は drop 保護対象だが、score 上は avatar bake/raw保存/sculpt/protected boost より弱く扱う。遠距離 callback が近距離 visible texture を一律に追い越すのを避ける。
- priority score の距離しきい値は `AYATextureDistanceCapNear/Mid/FarMeters` と同期する。
- `AYATextureCreatePriorityEnabled` で有効化する。
- `AYATextureCreatePriorityMinQueue` 件以上の pending create がある場合にだけ reorder する。
- `AYATextureCreatePriorityIntervalSeconds` で reorder 間隔を制限し、毎フレーム全 queue sort にならないようにする。
- `AYATextureCreateBackpressure*` は、queue が soft limit を超えた時に遠距離・視野外・参照なし通常 LOD の pending raw image を cancel してメモリを空ける。hard limit 超過時は1 frame あたりの最大 drop 数を増やす。
- backpressure は avatar bake、callback/raw保存、sculpt、local/map tile、boosted/protected texture を破棄しない。
- `FTT_DEFAULT` 経由の avatar bake fallback を誤って落とすリスクを下げるため、参照なし候補でも `AYATextureCreateBackpressureMinUnreferencedSeconds` 経過前は drop しない。
- 2026-06-03 の実機ログでは、旧実装の backpressure は `create_queue_now` が約 22,709 から 35,788 へ増え、FPS が約 10 から 6 台へ落ちる間に `backpressure_scanned` は 100 万件以上増えたが、`backpressure_dropped=0` のままだった。原因は release 条件が「face/volume 参照なし、fetch/cache/callback/raw保存なし、一定秒数経過」に寄りすぎ、遠距離・視野外の pending create が scene から参照されている限り cancel されなかったこと。
- 2026-06-03 の再検証ログでは `create_queue_now` が約 4,100〜4,500、`queued_far` が約 6万〜7万件まで増え、`backpressure_scanned` が増えても `backpressure_dropped=0` のままだった。原因は distance defer が `hard limit=8192` 超過時だけ有効で、soft limit 直上の実運用 queue では far/offscreen の cancel が発動していなかったこと。さらに priority reorder が near/mid/protected を queue 前方へ寄せるため、backpressure が先頭 window だけを見ると後方に退避された far/offscreen を見に行けなかった。
- `AYATextureCreateBackpressureDeferDistanceEnabled` は soft limit 超過時から有効化し、backpressure は queue 先頭だけでなく末尾 window も走査する。near / mid / protected は引き続き cancel 対象外にする。
- teleport sweep は active 中の再入を拒否し、候補数上限 `AYATeleportTextureSweepMaxQueuedTextures` を持つ。
- 旧リージョン由来の直接識別は、現時点では teleport sweep に依存する。create queue priority/backpressure には teleport generation 由来の明示的な old-region score はまだ入っていない。

設定:

| 設定 | 既定値 | 意味 |
| --- | ---: | --- |
| `AYALoadingFPSGuardEnabled` | `false` | FPS安定化の上位 master。OFF では解像度低下強度 / LOD低下強度の両方をスルーする |
| `AYALoadingResolutionReliefStrength` | `3` | 3D 内部解像度低下の強度。`0=無効`, `1=軽度`, `5=品質崩壊も許容して下限倍率へ寄せる` |
| `AYALoadingLODReliefStrength` | `3` | dense-world LOD 低下の強度。`0=無効`, `1=軽度`, `5=world object の強い LOD 劣化を許容`。avatar / attachment / HUD / selected object は world-object LOD 低下の対象外。non-avatar object idle throttle や avatar impostor refresh throttle は通常 UI の責務から外し、残す場合は debug/advanced の独立 kill switch とする |
| `AYALoadingTextureManagementEnabled` | `false` | テクスチャ読み込み・メモリ管理の上位 master。通常 UI では master checkbox と `AYALoadingTextureDownscaleStrength` の強度操作を表示する |
| `AYALoadingTextureDownscaleStrength` | `0` | texture cap / memory sweep scaleDown の低解像度化強度。`0=通常`, `1=cap/2`, `2=cap/4`, `3=cap/8`, `4=cap/16`, `5=cap 対象を最大 discard`。実効解像度は `MAX_DISCARD_LEVEL` と各 texture の `getMaxDiscardLevel()` に従う |
| `AYALoadingPriorityControlEnabled` | `false` | 旧互換設定。新規 gate には使わない |
| `AYATextureCreatePriorityEnabled` | `true` | pending GL texture create queue を近接/可視/保護優先で並べ替える |
| `AYATextureCreatePriorityMinQueue` | `32` | reorder を始める最小 queue 件数 |
| `AYATextureCreatePriorityScanLimit` | `2048` | 1回の reorder で score 計算する最大件数。巨大 queue では先頭・中央・末尾に分けて評価する |
| `AYATextureCreatePriorityIntervalSeconds` | `0.25` | reorder の最小間隔 |
| `AYATextureCreatePriorityCameraMotionBoostEnabled` | `true` | カメラ移動/回転時に interval を待たず priority reorder を発火する |
| `AYATextureCreatePriorityCameraMoveMeters` | `2.0` | priority reorder を即時発火するカメラ移動量 |
| `AYATextureCreatePriorityCameraTurnDot` | `0.985` | priority reorder を即時発火するカメラ向きの dot 閾値 |
| `AYATextureCreatePriorityCameraMotionScanMultiplier` | `3` | カメラ移動/回転時に priority scan 件数へ掛ける倍率 |
| `AYATextureCreatePriorityLog` | `false` | reorder summary をログ出力する |
| `AYATextureCreateBackpressureEnabled` | `true` | queue 過多時に安全な pending create を cancel する |
| `AYATextureCreateBackpressureQueueLimit` | `4096` | backpressure の soft limit |
| `AYATextureCreateBackpressureHardLimit` | `8192` | soft limit 超過時より drop 数を増やす hard limit |
| `AYATextureCreateBackpressureMaxDropsPerFrame` | `512` | soft limit 時の最大 drop 数 |
| `AYATextureCreateBackpressureMinUnreferencedSeconds` | `20.0` | 参照なし pending create を drop 可能にするまでの最小未参照秒数 |
| `AYATextureCreateBackpressureDeferDistanceEnabled` | `true` | soft limit 超過時に遠距離・視野外・参照なし pending create を cancel/defer できるようにする |

発動条件:

- priority reorder は `AYALoadingTextureManagementEnabled && AYATextureCreatePriorityEnabled` でだけ有効化する。
- カメラ位置・向きが変わった場合は、近距離/視野内判定も変わるため、`AYATextureCreatePriorityCameraMotionBoostEnabled` により次の interval を待たずに scan window を広げて再評価する。
- ただし起動中の UI image preload では `LLViewerCamera` がまだ未初期化の場合があるため、camera-motion 判定は priority reorder の実行候補が成立し、かつ `LLViewerCamera` が取得できる場合だけ行う。`AYALoadingTextureManagementEnabled=false` のときはカメラ・queue reorder・backpressure・distance/offscreen cap・memory sweep を完全にスルーする。
- backpressure は `AYALoadingTextureManagementEnabled && AYATextureCreateBackpressureEnabled` でだけ有効化する。
- distance/offscreen cap は `AYALoadingTextureManagementEnabled && AYALoadingTextureDownscaleStrength > 0` を上位条件とし、`AYATextureDistanceCapEnabled` / `AYATextureOffscreenCapEnabled` を内部 kill switch として併用する。これは queue 順序そのものではないが、遠距離・視野外の解像度上限を下げて近距離/可視 texture の成立を優先するため、通常 UI ではテクスチャ読み込み・メモリ管理に含める。
- 実装上、cap は `mMaxVirtualSize` を下げ、`processTextureStats()` / `updateImageDecodePriority()` が `mDesiredDiscardLevel` を上げることで fetch/cache/decode 側の要求 discard level を粗くする。つまり新規/再要求分は低解像度 discard を要求できる。一方、一度 GL texture / raw image / saved raw image として高解像度が成立したものは、cap 変更だけでは即時に消えない。このためテクスチャ管理を ON にした直後、strength を上げた直後、または memory pressure がある時に、bounded strength sweep / cap enforcement として既存の cap 候補を `scaleDown()` 予約へ進める。stale/unreferenced/offscreen/far は active sweep 内の分類であり、teleport generation は teleport texture sweep 側で扱う。FPS は cap-enforcement の制御理由にしない。cap 候補の raw/GL texture 破棄は通常行わず、破棄は未参照・遠距離・視野外の destructive sweep 側に限定する。
- 後続の検証ビルドでは `AYATextureCreateBackpressureDryRun` / `AYATextureMemorySweepDryRun` 相当の dry-run ログを追加し、`would_drop`, `actually_dropped`, `would_destroy`, `actually_destroyed` を分けてログ化する。現時点では未実装のため、採用判定では actual counters と実機副作用を必ず併読する。

低解像度要求と後処理縮小の違い:

- 通常の SL texture は J2C の progressive / discard level を前提に扱われる。ビューワーは「512px 版」のような別 asset をシミュレーターへ要求するのではなく、`mDesiredDiscardLevel` を `LLTextureFetch::createRequest()` に渡し、J2C の必要 byte 数を `LLImageJ2C::calcDataSizeJ2C(width, height, components, desired_discard)` で決める。
- HTTP texture fetch では `mDesiredSize` が `mRequestedSize` になり、通常は HTTP Range request で必要分だけ取得する。したがって J2C については、低解像度 cap が効けばネットワーク取得量と decode 負荷の両方が下がる余地がある。
- decode でも J2C は要求 discard より高解像度には decode しない。つまり「常に full raw を落としてから後で縮小している」わけではない。
- 例外として、非 J2C texture、`desired_discard == 0`、または `HttpRangeRequestsDisable=true` の場合は full request になりやすい。この場合は要求時点の転送量削減は期待しにくく、GL/raw 側の後段整理が中心になる。
- 一度高解像度の GL texture / raw image / saved raw image として成立済みのものは、後から要求 cap を下げても自動では消えない。このためテクスチャ管理を ON にした直後、strength を上げた直後、または memory pressure 時に sweep 予算を使う。stale/unreferenced/offscreen/far は active sweep 内の分類であり、teleport generation は teleport texture sweep 側で扱う。通常時は `updateImageDecodePriority()` / `processTextureStats()` の自然な優先度更新に任せる。cap candidate は raw/GL を破棄せず、要求 discard 更新と `scaleDown()` 予約だけを行う。raw/GL 破棄は未参照・遠距離・視野外など、表示や avatar 完了条件を壊しにくい bucket に限定する。
- cap の実効解像度は任意 px ではなく discard level に丸める。`MAX_DISCARD_LEVEL=5` のため、代表例では `2048px -> 64px`, `1024px -> 32px`, `512px -> 16px` が最大低解像度になる。`AYALoadingTextureDownscaleStrength=5` は「12px固定」ではなく「その texture が安全に持てる最大 discard まで落とす」という意味で扱う。
- cap の要求 floor は `16px` 未満にしない。`1px` まで落とすと `mMaxVirtualSize <= 10` の「不要 texture」sentinel に入り、`LLTextureFetch::createRequest()` まで到達せず、cache read / network fetch / decode / GL create が始まらないためである。強度 `5` でも「最大圧縮を要求する」のであって、「要求しない」状態にしてはいけない。

破棄/縮小候補:

- 遠距離 texture。
- 視野外 texture。
- 旧リージョン由来で、テレポート先で再利用されていない texture。
- face/volume 参照なし texture。
- callback/raw保存/fetch/fast cache 中ではない通常 LOD texture。
- GL create queue に残っているが、現在の scene から参照されていない pending raw image。

破棄しない対象:

- avatar appearance / baked texture / near avatar の基本表示に必要な texture と callback。
- 近距離の床、壁、階段、衝突、クリック対象、看板/案内面。
- HUD、preview、sculpt、media、local texture、保存要求のある raw image。
- 現在 face/volume 参照があり、近距離または画面内で使われている texture。

追加したいログ:

- `AYATextureQueue priority_stats near=... mid=... far=... offscreen=... old_region=... unreferenced=...`
- `AYATextureCap texture_work_stats ... oldest_wait_near=... oldest_wait_far=... oldest_wait_offscreen=...`
- `AYATextureCap create_backpressure event=drop_pending ... cause=backpressure would_drop=... actually_dropped=... dropped_far=... dropped_offscreen=... dropped_old_region=... dropped_unreferenced=...`
- `AYAAvatarLoad nearby pending=... fully_baked=... fully_loaded=... name_only=...`
- `AYAObjectQueue near_updated=... far_skipped=... offscreen_skipped=...`

現行ログ:

- `AYATextureQueue priority_stats event=reorder queue=... near=... mid=... far=... offscreen=... unreferenced=... protected=...`
- `AYATextureCap create_backpressure event=drop_pending queue_before=... queue_after=... scanned=... scanned_front=... scanned_tail=... dropped=... dropped_inactive=... dropped_distance=... dropped_far=... dropped_offscreen=... dropped_unreferenced=... allow_distance_defer=... kept=...`
- `AYATextureCap texture_work_stats ... created_near=... created_mid=... created_far=... created_offscreen=... created_unreferenced=... created_protected=... created_scene_reserved=... created_avatar_related=... scene_promoted=... queued_near=... downscale_queue_now=... downscale_queue_max=... downscale_limit=... downscale_time_ms=...`
- `AYA avatar_draw_state ...` は texture management 有効、in-world 開始後、pass 0/1、300 frame sampling で出る。drawpool 側の表示形態/early return を見るログであり、`not_loaded_no_fallback`, `suppress_body_pass`, `aoa_invisible`, `explicit_do_not_render`, `friends_only_hidden`, `render_impostor`, `allow_body_fallback` などを分類する。
- `AYA avatar_render_skip ...` も 300 frame sampling の現行ログであり、mesh/drawable 未構築による skip を見る。`skinned_not_built_initial`, `skinned_drawable_null`, `skinned_not_built_after_update`, `rigid_not_built` などを分類する。

採用条件:

- 近距離に avatar が少数しかいない状況で、名前だけ表示され本体が長時間出ない状態が改善する。
- `name_only` または未 fully loaded の近距離 avatar が減る。
- `texture_work_stats` で near が far/offscreen より先に処理されている。
- `create_backpressure` で破棄対象が遠距離・視野外・参照なし候補に偏っている。旧リージョン由来の確認は、現行では create backpressure ではなく teleport sweep の `teleport_texture_sweep` ログで見る。
- FPS が 10 未満へ落ちても、入力、カメラ、UI、近距離存在把握が維持される。
- `time_to_first_avatar_silhouette`, `time_to_fully_baked`, `name_only_duration`, `HUD texture clamped count`, `near sign readable seconds`, `click response latency` を採取し、主観だけで合否を決めない。

失敗条件:

- FPS は上がるが、近距離 avatar/object が未表示のまま長時間残る。
- avatar bake/callback を drop してしまい、appearance が完了しない。
- 近距離 object より遠距離/offscreen texture が先に GL create され続ける。
- 破棄候補に近距離 P0/P1 が混ざる。

実機で重点確認する未解決症状:

- テレポート後、process memory が約 15GB まで増えているにもかかわらず、新しい場所の texture が一向に表示されない場合がある。
- この症状では「破棄したか」だけでなく、`mCreateTextureList` / `mDownScaleQueue` / `mCallbackList` が旧リージョン由来または遠距離/offscreen の候補で詰まっていないかを見る。
- `destroyed_gl` / `destroyed_raw` が増えず、`image_count` が高止まりし、かつ `queued_far` / `queued_offscreen` が増えているなら、teleport sweep または backpressure が実効候補に届いていない可能性が高い。
- `scaled_down` / `cap_candidates` だけが増えて `created_near` が進まない場合、低解像度化は走っていても近距離 texture create/callback の unblock にはなっていない。これは `scaleDown()` の効果不足ではなく、queue priority/backpressure/callback 側の未解決問題として扱う。
- avatar texture 未読込残留は、`all_baked_downloaded` だけで合否を決めない。`AYA avatar_loading_wait`, `AYA avatar_draw_state`, `AYA avatar_render_skip`, `texture_work_stats created_scene_reserved/created_avatar_related/scene_promoted` を同時に見る。`AYA avatar_gray_decloud` は撤回済みなので、これが出る状態は実装矛盾として扱う。
- 近距離/可視 avatar が名前だけ表示される場合、まず `not_loaded_no_fallback`, `allow_body_fallback`, `render_impostor`, `suppress_body_pass`, `aoa_invisible`, `explicit_do_not_render`, `friends_only_hidden` のどれかを確認する。`allow_body_fallback` が出ていても、`skinned_not_built_initial`, `skinned_not_built_after_update`, `rigid_not_built` が続くなら、fallback 許可ではなく mesh/drawable 構築の未成立が問題である。
- 簡易表示へ落ちるべき avatar の合格条件は、mute/friends-only/明示非表示を除き、cloud、gray/simple body、または usable impostor のどれかで存在が見えること。`AOA_INVISIBLE` と explicit do-not-render は現ブランチでは fallback で復帰させない。
- avatar 関連 texture と scene texture の競合を見る時は、`created_avatar_related` だけでなく `created_scene_reserved` と `scene_promoted` を必ず見る。avatar 優先で scene texture が止まる場合、avatar の修正が scene starvation を起こしている。

固定回帰シナリオ:

- 近距離 avatar 1 / 5 / 20 体。
- HUD 装着、mouselook 時の自分の装備、alpha hair、装着ライト、クリック装備。
- 近距離大型看板、ショップ商品、media surface、animated texture、moving platform。
- 180 度カメラ回転、mouselook 開始/解除、カメラズーム、背後の UI 的オブジェクト再表示。
- `HUD texture clamped count = 0`, `turnaround_rez_delay <= 3s`, `popin_count <= 3/回転`, `collision object visible before movement = true` を暫定合格条件にする。

#### Phase 1.7: texture memory sweep

2026-06-03 の実機観察では、距離 cap と create queue backpressure だけではメモリ増加が残る可能性があった。特に一度 GL texture 化されたもの、または `mSavedRawImage` として保持されているものは、`scaleDown()` だけでは RSS/driver memory の圧力を十分に下げられないことがある。

目的:

- Texture memory sweep の active 条件は `AYALoadingTextureManagementEnabled && AYATextureMemorySweepEnabled` の上で、memory pressure、texture-management enable/downscale-strength increase による bounded strength sweep、またはその cap enforcement に限定する。stale/unreferenced/offscreen/far は active sweep 内の分類であり、teleport generation は teleport texture sweep 側で扱う。FPS は制御入力にしない。
- 近距離または視野内で未ロードの avatar、HUD、自分の表示、callback/sculpt/fetch/create/downscale 中の texture は破壊しない。avatar baked texture は `FTT_SERVER_BAKE` / `FTT_HOST_BAKE` の時点から full-res loading 経路を保護し、読み込み完了済みの他人 avatar bake を cap 対象へ戻す設計は今回の回帰解消後に再検討する。
- `scaleDown()` だけでなく、必要な場合は `destroySavedRawImage()` と `destroyGLTexture()` まで進める。無参照候補は優先破棄し、遠距離/視野外候補も memory pressure 中、または一定時間 stale の場合は GL texture まで破棄できるようにする。

実装済み:

- `LLViewerTextureList::updateImagesFetchTextures()` の後段に incremental sweep を追加した。
- `aya_texture_memory_sweep_bucket()` で候補を `keep`, `unreferenced`, `offscreen`, `far` に分類する。
- self avatar/HUD/protected boost/callback/fetch/create/downscale/sculpt/fast cache 等は destructive sweep 対象外にする。現行 safety では、他人 avatar の BOM/baked texture も `FTT_SERVER_BAKE` / `FTT_HOST_BAKE` または `BOOST_AVATAR_BAKED*` として AYA cap/downscale/sweep から外す。cap candidate の raw は downscale 実処理前には破棄しない。未ロードかつ近距離または視野内の avatar owner は保護し、遠距離・視野外の未ロード avatar を全員優先する挙動にはしない。
- `unreferenced` は `destroySavedRawImage()` と、budget があれば `destroyGLTexture()` を実行する。
- `offscreen` / `far` は `destroySavedRawImage()` と `scaleDown()` 予約に留める。`AYATextureMemorySweepDestroyFarOffscreenGLEnabled=true` の明示検証時だけ、memory pressure 中、または `AYATextureMemorySweepDestroyFarOffscreenMinSeconds` 以上 stale の場合に `destroyGLTexture()` まで進める。
- scan は `mUUIDMap` 全体を毎 frame 全走査せず、前回位置から `AYATextureMemorySweepScanPerFrame` 件ずつ進める。
- 旧実装には active 条件として FPS pressure が残っていたが、現行実装では撤回済みである。sweep は `AYALoadingTextureManagementEnabled && AYATextureMemorySweepEnabled` の間、memory pressure、texture-management enable/downscale-strength increase による bounded strength sweep、または cap enforcement の範囲で動く。stale/unreferenced/offscreen/far は active sweep 内の bucket 分類であり、teleport generation は teleport texture sweep 側で扱う。FPS は telemetry であり sweep の制御入力ではない。
- enable/strength-change pressure はテクスチャ管理を ON にした直後、または `AYALoadingTextureDownscaleStrength` を上げた直後に成立し、distance/offscreen cap kill switch の少なくとも一方が有効な場合だけ、開始時点の `mUUIDMap` 件数を一周、または bounded retry 上限に到達するまで継続する。fetch/create/callback/downscale 中で一時的に対象外だった候補と、downscale budget 超過で処理できなかった cap candidate は texture key を保持し、bounded retry で再試行する。cap candidate の raw/GL 破棄失敗を retry 理由にはしない。
- cap candidate は face 距離/視野外 cap の計算を通すため `updateImageDecodePriority()` を使うが、この経路では saved raw flush を無効化し、`scaleDown()` 予約だけを行う。`scaleDown()` は downscale queue への enqueue であり、実 downscale 前に raw を破棄すると create/downscale 待ちと衝突し得るため、cap candidate の `destroySavedRawImage()` / `destroyRawImage()` はこの sweep では行わない。GL texture の破棄は無参照、または明示的な遠距離/視野外 GL 破棄条件を満たしたものに限る。
- sweep は `AYALoadingFPSGuardEnabled` に依存させない。`render_relief_active=false`, `texture_management_active=true`, `memory_sweep_active=true` の単独検証モードを成立させる。
- 2026-06-05 の実機確認で、強度変更直後に `Trying to create a texture from invalid image data` が連続し、可視 avatar に loading cloud particle が残る症状が出た。これは単なる低解像度化ではなく、GL texture 作成待ちまたは downscale 待ちの texture raw を strength-change sweep が破棄し、後段の `createTexture()` が無効データを掴む可能性があるためである。修正後の strength-change / cap-enforcement sweep は `mCreatePending` / `mDownScalePending` / fetching / fast cache / save-raw / sculpt / callback / avatar bake / 近距離または視野内の未ロード avatar owner / self-HUD owner 等を除外する。完全に参照切れの pending create は queue を詰まらせないよう backpressure で解放可能にする。
- Firestorm の loading cloud は「メッシュが全く出ていない時」だけではない。`LLVOAvatar::updateIsFullyLoaded()` は baked texture callback、pending attachment、pending mesh、attachment count 待ちでも loading 扱いを継続するため、体や attachment の一部が見えていても `mFullyLoaded=false` なら cloud particle は残る。このため、texture management は avatar mesh / attachment mesh / visibility を直接所有せず、近距離/可視/未ロード avatar に関係する texture callback・fetch・create を sweep/backpressure で壊さない範囲に限定する。

設定:

| 設定 | 既定値 | 意味 |
| --- | ---: | --- |
| `AYALoadingTextureManagementEnabled` | `false` | テクスチャ読み込み・メモリ管理の上位 master |
| `AYALoadingTextureMemoryReliefEnabled` | `false` | 旧互換設定。新規 gate には使わない |
| `AYATextureMemorySweepEnabled` | `true` | texture memory sweep policy を有効化する。再設計後は FPS pressure ではなく、memory pressure、bounded strength sweep、cap enforcement を active 条件にする。stale/unreferenced/offscreen/far は分類、teleport generation は teleport sweep 側で扱う |
| `AYATextureMemorySweepScanPerFrame` | `256` | 1 frame に検査する texture 数 |
| `AYATextureMemorySweepDestroyGLPerFrame` | `16` | 1 frame に `destroyGLTexture()` できる最大数 |
| `AYATextureMemorySweepDestroyRawPerFrame` | `64` | 1 frame に `destroySavedRawImage()` できる最大数 |
| `AYATextureMemorySweepScaleDownPerFrame` | `32` | 1 frame に `scaleDown()` 予約できる最大数 |
| `AYATextureMemorySweepDestroyFarOffscreenGLEnabled` | `false` | 遠距離/視野外 GL texture も memory pressure または stale 時に破棄対象にする。強い検証用スイッチ |
| `AYATextureMemorySweepDestroyFarOffscreenMinSeconds` | `12.0` | memory pressure 外で遠距離/視野外 GL texture を破棄可能にするまでの未参照秒数 |
| `AYATextureMemorySweepMinUnreferencedSeconds` | `8.0` | 無参照候補として扱うまでの猶予秒数 |
| `AYATextureMemorySweepFPSRatio` | `0.75` | 旧 adaptive 実装の互換設定。再設計後は sweep の発動条件に使わない |
| `AYATextureMemorySweepMinPhysicalMB` | `7168` | process RSS がこの値以上なら memory pressure とする。`0` で無効 |
| `AYATextureMemorySweepLog` | `false` | `memory_sweep_stats` をログ出力する。通常は OFF、queue/drop/sweep の検証時だけ ON にする |

実装後ログ:

- `AYATextureCap memory_sweep_stats` では `fps_pressure` / `target_fps` を制御理由として出さない。現行ログでは `memory_pressure`, `strength_change_pressure`, `cap_enforcement_pressure`, `fps`, `downscale_strength_level`, `unreferenced`, `offscreen`, `far`, `cap_candidates` を分けて読み、`unreferenced/offscreen/far` は active sweep 内の分類として扱う。

追加したいログ:

- `refetched_after_drop cause=backpressure`, `recreated_after_destroy cause=memory_sweep|teleport_sweep`, `destroy_to_recreate_seconds`, `distance_cap_downscale` を追加し、backpressure、memory sweep、teleport sweep、distance cap downscale の副作用を分離する。
- `destroyed_raw_bytes_est`, `destroyed_gl_bytes_est`, `scaled_down_bytes_est`, `refetch_count`, `gl_recreate_count`, `post_sweep_texture_spike_ms`, `near_protected_count`, `hud_protected_count` は現行 `memory_sweep_stats` には出ていない。採用判定に必要なら実装する。

通常検証時のログ設定:

- `AYATextureCreateBackpressureLog`, `AYATextureMemorySweepLog`, `AYATextureDistanceCapLog`, `AYALoadingDynamicResolutionLog` は既定 OFF にする。
- 2026-06-03 の実機ログでは `create_backpressure` が 1 秒に 50 行以上出る区間があり、ログ出力自体が FPS 測定のノイズになり得る。
- 再度 queue/drop/sweep の内訳を検証する時だけ、対象ログを短時間 ON にして採取する。特に `AYATextureCreateBackpressureLog` は frame loop 内で高頻度になりやすいため、常用しない。

### Phase 1.8: RenderResolutionMultiplier adaptive control 検算と修正方針

2026-06-03 のサブエージェント検算では、RenderResolutionMultiplier の負荷式は一次近似として以下で妥当と判断した。

```text
OFF:
  T_off = C + G

ON:
  T_on = C + G * m^2 + lambda * R / F
```

- `T`: 1 frame あたりの時間。
- `C`: 解像度を下げても減らない CPU / scene update / UI / network / texture fetch-decode 等の固定負荷。
- `G`: full resolution 時の pixel dependent GPU 負荷。
- `m`: effective `RenderResolutionMultiplier`。
- `lambda`: render target 再確保回数 / 秒。
- `R`: render target 再確保 1 回の停止コスト。
- `F`: FPS。

ON が OFF より軽い条件:

```text
T_on < T_off
C + G * m^2 + lambda * R / F < C + G
G * (1 - m^2) > lambda * R / F
```

つまり、低解像度化で浮く pixel dependent GPU 負荷が、render target 再確保の償却コストを上回る必要がある。

コード調査と再評価:

- 旧 `llviewerdisplay.cpp` 実装は `sqrt(current_fps / pressure_fps)` 相当で倍率を推定していた。ここでの `pressure_fps` は UI 目標ではなく、`AYALoadingResolutionReliefStrength` から `{0, 30, 60, 90, 144, 288}` に写像される内部圧力値だった。
- 2026-06-07 の再議論では、この adaptive multiplier を撤回対象とする。ユーザーは slider を固定強度として操作しており、FPS に応じて倍率が変わると「ON にした profile が揺れる」体験になる。
- `pipeline.cpp` は `RenderResolutionMultiplier` を `resX` / `resY` の両方に掛けるため、画面依存 RT 面積は概ね `m^2` に比例する。
- 倍率変更は `resizeScreenTexture()` から `releaseScreenBuffers()`, shadow target release, `allocateScreenBufferInternal()` へ到達する。
- 再確保対象は `deferredScreen`, `screen`, `deferredLight`, `mObjectIDBuffer`, `mVelocityMap`, `mSMAAHistory`, `mAYAAlphaDepth`, `mAYAAlphaColor`, FXAA/SMAA buffers, post buffers, water buffers 等であり、単なる scale 変更ではなく GPU texture/FBO lifecycle を多数走らせる。

ログ照合:

- `allocated mVelocityMap` は 2026-06-03 の対象ログ全体で 706 回。
- in-world 相当の 13:22:10 以降では 705 回 / 1614 秒、`lambda = 0.437 回/秒`。
- 直近 5 分では 165 回 / 292 秒、`lambda = 0.565 回/秒`。約 1.8 秒に 1 回 RT 再確保していた。
- 再確保あり区間は平均 FPS が高く、低解像度化の平均効果は出ている。一方で `total_max_ms` の最大値は再確保あり区間の方が大きく、瞬間停止/カクつきの原因候補になっている。

旧修正方針と撤回:

- 旧案では `m^2` の利益を残しつつ `lambda` を下げるために、連続調整を bucket + cooldown + hysteresis へ変える方針だった。
- しかしこの案でも FPS が倍率変更の制御入力として残るため、`ON = fixed profile` という再定義と矛盾する。
- よって `sqrt(fps / pressure_fps)`、`adaptive_bucket`、`adjust_down`、`adjust_up`、`RestoreFPS` は撤回対象とする。

新修正方針:

- `m^2` の利益を残しつつ `lambda` をほぼゼロに近づけるため、倍率を固定 profile にする。
- `lambda` は ON/OFF、profile 変更、in-world 出入り、安全除外出入りの時だけ発生する。
- 同じ profile 値なら同じ multiplier を使い、FPS が 20 でも 80 でも倍率を変えない。
- `profile_multiplier` とユーザー保存済み `RenderResolutionMultiplier` は分離する。effective は `min(user_saved_multiplier, profile_multiplier)` とする。
- 保存済み `RenderResolutionMultiplier` を AYA が恒久変更しない。AYA profile は session-only state として扱う。
- 正確な `R` を測る必要がある場合は、`resizeScreenTexture()` または `allocateScreenBufferInternal()` の所要時間ログを短時間だけ追加して検証する。

確認ポイント:

- `profile_active=1` の間、FPS、teleport、texture backlog、camera movement を理由に `RenderResolutionMultiplier` 相当の effective multiplier が変わらない。
- `realloc_count_per_sec` は ON/OFF、profile 変更、安全除外出入り以外では 0 に近い。
- `geom`, `lighting`, `statesort`, GPU/driver stall が下がる一方、`textures`, callback queue, 近距離表示完了が悪化していない。
- fixed render profile と world LOD の両方が有効な run では、どちらの効果かを単独比較として読まない。

2026-06-03 ビルド検証:

- `cmake --build build-darwin-universal --config Release --target ayastorm-bin -j 8 -- ARCHS=arm64 ONLY_ACTIVE_ARCH=YES` は成功した。
- 生成物: `build-darwin-universal/newview/Release/AYAstorm.app`
- `file` では `Mach-O 64-bit executable arm64`。
- `lipo -info` では non-fat `arm64`。
- `codesign --verify --deep --strict` は成功。

#### 2026-06-02 ビルド/起動検証

ビルド:

- `xcodebuild -project build-darwin-universal/Firestorm.xcodeproj -scheme ayastorm-bin -configuration Release ... build` は成功した。
- 生成物: `build-darwin-universal/newview/Release/AYAstorm.app`
- `lipo -info` では `AYAstorm` 本体は non-fat `arm64`。
- `codesign --verify --deep --strict --verbose=2` は valid。
- app 内 `Contents/Resources/app_settings/settings.xml` に `AYATextureDistanceCapAux*`, `AYATextureDistanceCapLog`, downscale 既定値変更が反映されている。

起動検証:

- app は起動し、`AYAstorm.log` は新しい app 内の `settings.xml` を読み込んだ。
- ただし描画ループ到達前にログが止まり、`AYATextureCap` はまだ出ていない。
- `sample` では main thread が `LLAppViewer::initStrings()` -> `LLTransUtil::parseStrings()` -> `LLXMLNode::parseFile()` -> `LLFile::getContents()` -> `fopen/open` にいた。
- これは texture 距離 cap の検証段階より前の起動/ファイル読み込み問題であり、今回の FPS安定化効果検証とは分けて扱う。

次の確認:

1. 起動が `initStrings()` を抜けてログイン画面または world 描画まで進む状態にする。
2. ログイン/テレポート後 120 秒の `AYATextureCap` と AYAPerf を採取する。
3. `clamped_faces > 0`, `aux_faces > 0`, `downscaled > 0`, `downscale_queue_now` が減るかを見る。
4. 見た目ではなく、FPS、`textures_avg_ms/max_ms`、`texture_work_stats`、メモリ推移で効果判定する。

### Phase 2: Active object idle budget

仮説:

- ロード中に active object の `idleUpdate()` が一気に増え、`objectlist` が FPS を潰している。

変更:

- 非 avatar active object の更新を、明示的な object idle profile が有効な場合だけ時間予算化する。
- avatar は操作感への影響が強いため、最初の検証では別扱いにする。

採用条件:

- `objectlist_avg_ms` が下がる。
- avatar 表示、物理、スクリプト、animated texture の破綻がない。
- ロード完了や object 表示の遅延が許容範囲。
- active non-avatar 数、updated/visited/skipped、cursor 一巡秒数のログを追加してから判定する。

実装メモ:

- 旧案では `AYALoadingFPSGuardEnabled` が ON かつ `AYALoadingLODReliefStrength > 0` で pressure FPS を下回る場合に、非 avatar active object の `idleUpdate()` を `RenderObjectIdleGuardTimeSliceMs` / `RenderObjectIdleGuardMinUpdatesPerFrame` でさらに絞る想定だった。この pressure FPS gate は撤回対象である。
- 再設計後に残す場合は、`AYALoadingLODReliefStrength` を fixed object/LOD profile selector として扱い、FPS ではなく profile と safety exclusion で制御する。通常 UI の 3D描画軽量化から avatar impostor policy へ波及させない。
- avatar は P0/P1 の存在把握と操作感に近いため、この guard では毎フレーム更新を維持する。
- ただし、この「avatar 維持」は avatar 本体の `idleUpdate()` を維持するという意味であり、attachment まで毎フレーム維持する保証ではない。attachment は非 avatar object として間引かれる可能性がある。

取りこぼしやすいリスク:

- 非 avatar object でも、animated texture、particle emitter、media surface、click target、moving platform は体感に影響する。
- cursor 方式で公平に回しても、object 数が極端に多い場所では一巡に数秒かかる可能性がある。
- object idle を絞ったことで `objectlist_avg_ms` は下がっても、`statesort` や `lighting` が残る場合は FPS が上がらない。

検証で見る値:

- `AYAPerf idle objectlist_avg_ms/objectlist_max_ms`
- active object 数と、guard 中に 1 巡するまでの推定秒数
- animated texture / media / moving object の見た目破綻
- 近距離クリック対象や衝突対象の反応遅延
- AO、髪、装着ライト、装着 UI、クリック装備の遅延
- non-avatar active object cursor の一巡秒数

### Phase 3: Shadow / impostor throttle

仮説:

- shadow map と avatar impostor 更新が不要にフレームを使っている場面がある。

変更:

- 既定では FPS安定化から shadow throttle を切り離す。
- avatar impostor 更新制御は Firestorm 標準の avatar visibility policy と衝突しやすいため、通常 UI の FPS安定化から切り離す。
- 必要な場合だけ debug/advanced の明示 kill switch で、遠距離 impostor 更新頻度を抑える実験として扱う。

採用条件:

- `shadows_impostors_avg_ms` と `max_ms` が下がる。
- 影のカクつき、古い impostor の残留が許容範囲。
- FPS が上がる。
- shadow updated/skipped と impostor candidates/updated/skipped/stale 秒数の別ログを追加してから判定する。

実装メモ:

- FPS安定化発動中でも、既定では sun shadow 更新間隔を `RenderSunShadowGuardFrameInterval` に切り替えない。明示的に `AYALoadingSunShadowThrottleEnabled=true` にした場合だけ、FPS安定化側の shadow throttle を許可する。
- avatar impostor 更新は、既存の `RenderAvatarImpostorMinUpdateFPS` など Firestorm 標準の設定を優先する。AYA 側で simple/body fallback 対象を `isImpostor()` / `shouldImpostor()` false に倒す旧案は、texture management と avatar visibility の責務衝突を起こしているため撤回または別 kill switch 化する。

取りこぼしやすいリスク:

- `shadows_impostors` は shadow と impostor が同じ計測枠に入っているため、どちらが効いたか分かりにくい。
- impostor 更新を完全停止すると、古い姿勢や位置が長く残る。
- sun shadow の間引きは昼夜/環境変更時の見た目遅延を起こす。
- snapshot や high quality render では throttle を適用しない方がよい。

追加方針:

- 可能なら shadow と impostor を別ログに分ける。
- guard 中の impostor は完全停止だけでなく、近距離 avatar を優先し遠距離だけ停止する案を残す。
- `RenderAvatarMaxImpostorUpdatesPerFrame = 0` は実装上、per-frame 上限なしを意味する。完全な切り戻しではこの値も 0 にする。
- shadow は updated/skipped と interval をログへ出す。
- impostor は candidates/updated/skipped と最古更新待ち秒数をログへ出す。

### Phase 4: Rendering load policy

仮説:

- SL でも「完成度よりフレーム維持」を優先する policy が必要。ただし 2026-06-07 再設計では、これは FPS/teleport/backlog を gate にする一時モードではなく、ユーザーが ON にした fixed profile / texture QoS policy として扱う。

変更候補:

- ユーザー設定で 3D描画軽量化と texture QoS policy を個別に ON/OFF できるようにする。
- 3D描画軽量化は fixed profile とし、FPS 回復で自動解除しない。
- texture QoS policy は queue pressure、memory pressure、stale age、teleport generation、unreferenced state を見て遠距離・視野外・旧リージョン・参照なし候補を処理する。FPS は発動条件にしない。

採用条件:

- 体感 FPS が改善する。
- ロード完了が極端に遅くならない。
- 見た目の劣化が一時的で予測可能。

未実装の重要要素:

- rendering load policy 全体の統一 state。dynamic resolution の固定 profile、texture priority/backpressure/memory sweep/dense-world LOD を、同じログ分類で追える状態がまだ不十分である。
- profile OFF、world exit、safety exclusion 時に、dynamic resolution、dense-world LOD、texture cap、sweep を順番に戻す段階解除。
- guard 状態の統一ログ出力。現状は `AYALoadingFPSGuard`, `AYAPerf`, `AYATextureCap`, `AYATextureQueue` に分散している。
- AutoTune と AYA fixed profile の所有権境界。
- ユーザーが UI から現在 guard 中か確認できる表示。
- texture priority/backpressure/callback queue まで含む完全な P0/P1 優先 scheduler。現状、texture create queue は priority reorder 済みで、avatar/self/HUD/near unloaded avatar の callback priority pre-pass も実装済みである。ただし callback queue 全体の priority sort と詳細ログは未実装であり、pre-pass は通常 callback と同じ per-frame/time budget を消費する。

旧案のような「ロード専用モード」は撤回する。現在の実装では render relief はチェック ON 中の in-world fixed profile、texture management は texture QoS policy として扱い、loading window / FPS pressure / teleport / backlog は activation gate にしない。ロード開始前から P0/P1 を全経路で守る完全な scheduler はまだ未完成であり、texture create priority、callback priority、memory sweep、avatar visibility safety を分けて検証する必要がある。

#### 別案: 読み込み中だけ内部 3D 解像度を下げる

AYAstorm/Firestorm には、ウィンドウサイズを変えずに 3D scene の内部 render target 解像度だけを下げる既存設定がある。

- `RenderResolutionDivisor`
  - `1` が等倍。
  - `2` なら 3D scene を 1/2 解像度で描画する。
- `RenderResolutionMultiplier`
  - `1.0` が等倍。
  - `0.5` なら 3D scene を半解像度で描画する。
  - `RenderResolutionDivisor > 1` が有効な場合は divisor 側が優先され、multiplier は使われない。

実装上は `LLPipeline::resizeScreenTexture()` / `LLPipeline::allocateScreenBufferInternal()` で world view サイズから縮小した `mRT->screen` / `mRT->deferredScreen` を確保する。UI は別経路で描画されるため、狙いとしては「ウィンドウと UI はそのまま、3D 描画だけ低解像度」が可能である。

描画負荷が下がる根拠:

- `LLPipeline::resizeScreenTexture()` は `getWorldViewWidthRaw()` / `getWorldViewHeightRaw()` を元に、`RenderResolutionDivisor` または `RenderResolutionMultiplier` を適用した `scaledResX` / `scaledResY` を計算する。
- `LLPipeline::allocateScreenBufferInternal()` は、その縮小後の `resX` / `resY` で `mRT->deferredScreen` と `mRT->screen` を確保する。deferred path の color/depth render target 自体が小さくなるため、G-buffer 書き込み、HDR scene buffer 書き込み、depth buffer、post-process 入力のピクセル数が減る。
- `llviewerdisplay.cpp` の world rendering は `mRT->deferredScreen.bindTarget()` へ描画し、`renderGeomDeferred()`、`renderDeferredLighting()`、その後の `renderFinalize()` へ進む。つまり UI ではなく world 3D 側が縮小 render target に乗る。
- `renderFinalize()` では `mRT->screen.getWidth()/getHeight()` や source buffer の width/height を使って DoF、tonemap、glow、FXAA/SMAA、最終 fullscreen pass の shader uniform / viewport を組む。内部解像度を `0.5` にすると、対象ピクセル数は概ね `0.5 * 0.5 = 25%` になる。`0.67` なら約 `45%` になる。
- deferred lighting や fullscreen post process が支配的な区間では、fragment shader 実行回数、render target bandwidth、MSAA/resolve/post-process の読み書きが減るため、FPS 改善が期待できる。

一方で、これは万能ではない:

- mesh/object update、texture fetch/decode、GL texture create、callback、statesort の CPU 側 queue そのものは、内部 3D 解像度を下げても直接は減らない。
- geometry pass の vertex transform、skinning、draw call 数、state change 数も、ピクセル数ほどは減らない。
- したがって `AYAPerf display` で `lighting`, `geom`, `ui_swap`, GPU/driver stall が支配的なら効果が出やすい。`textures`, `objectlist`, callback queue が支配的なら効果は限定的で、近接優先 scheduler / backpressure / object idle throttle 側を優先する。
- 効果判定は `dynamic_resolution event=enter` 前後で、`lighting_avg/max`, `geom_avg/max`, `ui_swap_avg/max`, `textures_avg/max`, FPS を同時に比較する。FPS だけが上がらず `textures` や `objectlist` が残る場合、「内部解像度低下でロード中fpsを救う」という仮説はその場面では外れであり、実装計画を queue 制御中心に戻す。

調査メモ:

- `RenderResolutionDivisor` は古い Debug Setting であり、Second Life Wiki の Debug Settings にも「3D scene を reduced resolution で描画する divisor」として載っている。
- Firestorm 履歴では `RenderResolutionDivisor` は `FIRE-7066`, `FIRE-11830`, `RenderResolutionDivisor is applied twice` など、通常 UI 機能ではなく debug/render pipeline 設定として修正され続けている。
- `RenderResolutionMultiplier` は 2018-03-27 の Catznip/RLVa 由来コミット `d56b519bed` で「`RenderResolutionDivisor` のより細かい代替」として追加された。
- その追加コミットは `settings.xml`, `llviewercontrol.cpp`, `pipeline.cpp`, `pipeline.h` だけを触っており、Preferences XUI は触っていない。
- その後の変更も safety guard と二重適用バグ修正であり、表側 UI 化の履歴は確認できなかった。
- `RenderResolutionDivisor` は RLVa から触れる Debug Setting としても許可されている。従来は「視界をぼかす」用途にも使われ、Firestorm サポート文脈では画面がぼやけた時に Debug Settings で default に戻す案内がある。

なぜ表側 UI にないと見られるか:

- 画質が露骨にぼやけ、一般ユーザーが常用設定として触ると誤解や事故が起きやすい。
- 起源が Debug/RLVa/特殊効果寄りであり、Firestorm の通常 Graphics 品質 UI として設計された機能ではない。
- divisor は整数で粗く、multiplier は後付けの Catznip パッチで、UX として段階・説明・復帰条件が整備されていない。

AYAstorm での扱い案:

- 常時ユーザー設定として `RenderResolutionMultiplier` そのものを表へ出すのではなく、`FPS安定化` の fixed profile として扱う。
- チェックボックス ON 中は、login/XUI/snapshot などの safety exclusion を除く in-world で選択 profile が効く。FPS、teleport、startup window、texture backlog、callback backlog は発動条件にしない。
- `AYALoadingResolutionReliefStrength` は profile selector であり、FPS pressure から倍率を再計算する入力ではない。例: 1=0.85, 2=0.75, 3=0.66, 4=0.50, 5=0.33/0.25 debug 候補。
- 状態遷移は ON/OFF、profile 変更、world enter/exit、safety exclusion enter/exit に限定する。同じ profile のまま FPS が上下しても render target resize しない。
- 発動/復帰時は `gResizeScreenTexture = true` により render target 再確保が起きるため、切り替え頻度は抑える。ただし抑える理由は FPS hysteresis ではなく、profile/safety transition の連続発火を避けるためである。
- 効果判定は FPS だけでなく、`lighting`, `geom`, `ui_swap`, `statesort`, `textures`, GPU/driver stall の変化を見る。CPU bound 区間では効果が薄い可能性がある。
- UI や HUD の可読性は維持される想定だが、3D world 内の看板、名前表示、mouselook 時の視認性は劣化するため、P0/P1 の可読性確認が必要。

2026-06-02 実装メモ:

- `RenderResolutionMultiplier` をユーザー設定値として runtime 書き換えする経路は撤回する。ただし独自の effective multiplier を別経路で render target 計算へ合成するのも撤回する。既存 `RenderResolutionMultiplier` と違う合成/viewport 経路になると、3D render 部だけが不自然に拡大されたり、カメラ位置がずれたように見える UX 悪化を起こし得る。
- AYA loading 用倍率は保存設定ではなく `LLPipeline` の内部一時状態として持つ。FPS安定化有効中だけ、保存済み `RenderResolutionMultiplier` と AYA loading 用倍率の小さい方を `LLPipeline::RenderResolutionMultiplier` の cached 値そのものへ反映する。これにより既存の `resizeScreenTexture()` / `allocateScreenBufferInternal()` 分岐だけを通し、window/world view/UI サイズは変えず、3D render target だけを縮小する。
- 既存仕様どおり `RenderResolutionDivisor > 1` が有効な場合は divisor 側が優先される。発動/復帰時は `gResizeScreenTexture = true` で screen/deferred render target の再確保だけを要求する。
- 既定では通常 UI の master は OFF。ON にした場合だけ、`AYALoadingDynamicResolutionEnabled=TRUE` かつ profile strength > 0 のとき in-world 固定 profile を適用する。旧互換設定 `AYALoadingDynamicResolutionMultiplier` は settings 上に残っても、通常 UI profile の制御入力としては使わない。
- `AYALoadingDynamicResolutionLowFPSEnabled`、`AYALoadingDynamicResolutionRestoreFPS`、`pressure_fps`、`adaptive_floor`、bucket による FPS 追従は撤回対象である。残す場合も legacy diagnostic / debug experiment に限定し、通常 UI の `FPS安定化` とはつながない。
- 低解像度化は user-facing な `RenderResolutionMultiplier` setting を保存値としては上書きしないが、FPS安定化有効中は `LLPipeline::RenderResolutionMultiplier` の cached 値そのものへ一時反映する。独自の effective multiplier を別経路で解釈すると、既存 `RenderResolutionMultiplier` 使用時より合成/viewport 体験が悪化し得るため、不採用とする。つまり world view/window/UI のサイズは変えず、既存の `LLPipeline::resizeScreenTexture()` / `LLPipeline::allocateScreenBufferInternal()` で 3D render target だけを縮小し、最終合成で raw world view に拡大するのが正しい挙動である。
- `AYALoadingResolutionReliefStrength` は通常 UI の操作値なので、値が変わった場合は次の display update で profile multiplier を反映する。ただし FPS による `adjust_down|adjust_up` は出さない。
- 復帰条件はチェック OFF、profile 0、world exit、startup/login UI safety、snapshot safety である。FPS が回復したかどうかは復帰条件にしない。
- ログは `AYALoadingFPSGuard` タグで `dynamic_resolution event=enter|exit|active|profile_change|safety_excluded|safety_restored` を出す。`reason`, `active`, `fps`, `profile_id`, `profile_multiplier`, `user_render_resolution_multiplier`, `effective_render_resolution_multiplier`, `cached_render_resolution_multiplier`, `startup_login_ui`, `snapshot_active`, `in_world`, `autotune_active`, `render_far_clip`, `render_avatar_max_art`, `enter_count`, `exit_count` を確認する。teleport/backlog/queue 値は同じ行または adjacent 行に telemetry として出してよいが、control reason にはしない。
- 実機ログで `fps=170.89` かつ `startup_window=true` のみを理由に `event=enter reason=loading_window` が出た。これは「チェック ON 中の固定 profile」とも「低 FPS 時だけ使う」という旧計画とも違う中途半端な状態だったため、startup window / teleport window / backlog を control reason にしない修正対象とする。
- build 検証では arm64 Release の `AYAstorm.app` 生成まで通した。実機検証では、login UI が消えた後の in-world で `event=enter reason=profile_active` 相当が出ること、profile 変更時だけ倍率が変わること、FPS/teleport/backlog だけでは enter/exit しないこと、チェック OFF / profile 0 / world exit / safety exclusion で `event=exit` が出ることを確認する。

#### ログイン画面/XUI 初期描画への防御策

2026-06-05 の確認では、`aya_update_dynamic_resolution_guard()` は `display()` の早い段階で呼ばれ、`STATE_STARTED` 未満ではその後 `display_startup()` へ戻る構造である。つまり、ログイン画面表示中でも guard 関数自体は評価され得る。一方で、ログイン画面は XUI、HTML/media、フォント、画像ロード、初期 layout が数フレームかけて安定するため、この段階で `RenderResolutionMultiplier` 系の render target resize を起こすと、ログイン画面が一瞬崩れてから正しく見える症状を増幅する可能性がある。

実装方針:

- `LLStartUp::getStartupState() < STATE_STARTED` の間は dynamic resolution guard を完全に早期 return する。
- ただし `STATE_STARTED` だけでは不足する。NUI login panel は `STATE_STARTED` 到達後も login holder 配下に残るため、`login_holder` が visible かつ child を持つ間も startup/login UI active とみなし、dynamic resolution guard を早期 return する。
- 何らかの残留で dynamic resolution が active の場合は、早期 return 前に倍率を restore し、`event=exit reason=startup_login_ui` をログへ出す。現行コードでは `startup_not_started` ではなく、ログイン UI 表示中も含む `startup_login_ui` を使う。
- `STATE_STARTED` 到達時点では dynamic resolution profile をまだ再適用しない。login UI が消えた後に初めて in-world fixed profile の適用候補にする。これにより、ログイン画面/XUI 初期描画は保護しつつ、ログイン後の in-world 3D描画軽量化は維持する。
- `FSPanelLogin` は初期生成時と `reshapePanel()` で `getLoginPanelHolder()->getLocalRect()` の非ゼロサイズを優先する。渡された viewer 全体 rect をそのまま使うと、login holder の実サイズと panel 内 layout が一瞬ずれ、入力フォームや bottom stack が潰れる可能性がある。
- NUI login XUI は default/starlight/starlightcui の en を 1000x700 基準へ揃える。root panel は 1000x700、`login_html` は 1000x500、`ui_stack` は width 1000、左右 elastic pad は 15、`login` / `password_container` は `min_width=206` とする。ja overlay は localized override として `login` / `password_container` の `min_width=206` を追従する。
- dense-world LOD は avatar/world volume の LOD bias であり、ログイン画面 XUI へ直接作用しない。XUI への防御対象は dynamic resolution の render target resize 経路と、login panel 自身の初期 geometry/layout に限定する。

#### ResolutionMultiplier と FXAA の viewport 不整合

2026-06-03 の実機切り分けで、`RenderResolutionMultiplier` による低解像度化時の「画面が拡大され、カメラ位置がずれたように見える」現象は、アンチエイリアスが FXAA の場合に発生し、AA 無効または SMAA では発生しないことが確認された。これは設定ファイル混入ではなく、FXAA pass 固有の viewport 不整合で説明できる。

原因:

- `LLPipeline::resizeScreenTexture()` / `LLPipeline::allocateScreenBufferInternal()` は `RenderResolutionMultiplier` 適用後の低解像度 render target を確保する。
- `LLRenderTarget::bindTarget()` は bind した FBO に合わせて `glViewport(0, 0, mResX, mResY)` を設定する。
- しかし FXAA 経路の `LLPipeline::applyFXAA()` は `dst->bindTarget()` 後に、`gViewerWindow->getWorldViewRectRaw()` 由来の raw world viewport を `gGLViewport` と `glViewport()` に再設定していた。
- その結果、低解像度 FBO に対して raw world viewport で fullscreen triangle を描くことになり、FXAA 出力だけが拡大/位置ズレしたように見える。
- SMAA 経路は各 pass で `bindTarget()` の viewport をそのまま使うため、同じ `RenderResolutionMultiplier` 条件でもこのズレが出にくい。

修正方針:

- FXAA pass では `dst->bindTarget()` が設定した低解像度 FBO の viewport を維持する。
- `gGLViewport` は final backbuffer 復帰時に使われる raw world viewport として残す。FXAA pass 内で低解像度値へ書き換えない。
- FXAA の `width` / `height` と `mFXAAMap` の比率から作る sampling uniform は render target サイズ基準のまま使う。`dst` と `mFXAAMap` が同じ縮小率なら `scale_x/scale_y` は `1.0` になり、低解像度画像を同じ構図で処理できる。

#### AYAstorm と Firestorm のユーザー設定混在メモ

2026-06-03 の実機確認で、Firestorm Viewer 側の Debug Settings に AYAstorm の設定名が見える状態は確認された。これは `~/Library/Application Support/Firestorm/user_settings/settings.xml` を共有している可能性を示すが、上記の FXAA 固有ズレの主因ではない。

扱い:

- `RenderResolutionMultiplier` の画面ズレは「FXAA でのみ発生し、AA 無効/SMAA では発生しない」ため、設定ファイル混在より FXAA viewport 不整合を優先して修正する。
- macOS の保存先初期化 `indra/llfilesystem/lldir_mac.cpp` を `APP_NAME` に変更する案は、Firestorm/AYAstorm の設定分離としては検討余地があるが、今回の FXAA 修正とは分離する。
- 現時点では保存先参照を `Firestorm` 固定の既存挙動に戻し、RenderResolution の視覚不具合修正へ影響させない。
- 既に混在した AYA 追加キーが Firestorm 側に見える問題は、別タスクとして扱う。RenderResolution の検証では `RenderFSAAType=1` かどうか、`RenderResolutionMultiplier` の値、`RenderResolutionDivisor=0` であることを先に確認する。

#### Texture cache の open 失敗後ログ嵐

2026-06-03 の実機ログで、`/Volumes/AdobeCache/Firestorm/texturecache/texture.entries` に対する `Too many open files` の直後、`llapr.cpp write : apr mFile is removed by somebody else. Can not write.` が大量発生した。

原因整理:

- `LLTextureCache::openHeaderEntriesFile()` は `LLAPRFile` の作成後、実際の `apr_file_open()` が失敗して `mFile == NULL` でも `LLAPRFile*` を返していた。
- 呼び出し側の `writeEntryToHeaderImmediately()`, `readEntryFromHeaderImmediately()`, `writeEntriesAndClose()` などは、その返り値を有効な file として扱い、NULL file に対して read/write を呼んでいた。
- `LLTextureCache::openFastCache()` も同様に、fast cache の open 失敗後に `mFastCachep` を保持し得るため、後続の fast cache read/write が NULL file に進む余地があった。

修正方針:

- header entries / fast cache の open 失敗時は `LLAPRFile` を即 delete し、ポインタを `NULL` に戻す。
- 呼び出し側は open 失敗を cache I/O 失敗として扱い、read/write/seek に進まない。
- これにより file descriptor 枯渇そのものを直接解決するというより、枯渇発生後に cache I/O が無限に壊れた file handle を叩き続け、ログとフレーム時間をさらに悪化させる経路を止める。

### Phase 1.9: mouselook-fps ブランチの dense-world LOD 取り込み計画

2026-06-03 時点の検証では、`RenderResolutionMultiplier` は pixel cost を下げる対策として有効だが、混雑地点や重いワールドで FPS が上がり切らない場面がある。これは負荷が `G_pixel * m^2` だけではなく、scene 側の drawinfo、triangle、state sort、geometry、lighting にも残っているためである。

そのため、別ブランチ `feature/ayastorm-r34-mouselook-fps` の LOD 制御を「mouselook 専用機能」としてではなく、「FPS安定化の scene-bound 補助」として取り込めるか検証する。

取り込む候補:

- `LLVOVolume::calcLOD()` への LOD bias hook。近距離保護距離の外側にある、視野中心外、小画面占有、大量 child を持つ dense root、重い mesh に対して detail を強度に応じて 1-4 段落とす。
- camera distance、camera forward dot、projected screen height、root child count、triangle count を使った candidate 判定。
- decision interval と hysteresis。毎フレーム LOD を揺らさず、realloc や state churn を増やさない。
- `LLSpatialGroup::updateDistance()` からの forced LOD refresh。距離や視野が変わったのに古い LOD が残り続ける状態を避ける。
- 集計ログ。現行の詳細ログは `AYALoadingFPSGuard dense_world_lod ...` として `biased`, `bias_total`, `dense_root`, `heavy_mesh`, `heavy_seen`, `heavy_distance_rejected`, `heavy_forward_protected`, `screen_forced`, `forced_refresh`, `bands`, `refresh_per_sec`, `refresh_per_frame`, `force_interval_frames`, `force_max_per_frame`, `fps` を低頻度で出し、FPS安定化の効果と副作用を確認する。

取り込まない候補:

- `lldrawpool.cpp` の draw pass suppression / drawinfo skip。これは描画欠落や近距離オブジェクト未 rez のリスクが高く、今回の「近いものから必ず表示する」方針と衝突する。
- localID 指定の suppression 実験設定。
- trace default ON。ログ出力自体が FPS と I/O を悪化させる可能性があるため、検証時だけ ON にする。
- Stream3D protection を外す実験。
- `AYAR34Mouselook...` の設定名をそのまま持ち込むこと。旧実装では `gAgentCamera.cameraMouselook()` による pressure bypass が残っていたが、現行実装では mouselook は dense-world LOD の activation gate ではなく telemetry として扱う。

LOD bias / forced refresh の除外対象:

- 自分の avatar、他人の avatar 本体、avatar baked texture、avatar attachment、HUD attachment は今回取り込む LOD bias / forced refresh の対象にしない。
- object 側だけでなく root 側が avatar / attachment / HUD に属する場合も除外する。
- selected object、編集中 object は対象にしない。
- 近距離保護距離内の object は dense root / heavy mesh でも対象にしない。まず近距離 avatar / attachment / HUD / 近距離 world object が rez することを優先する。
- texture cap / sweep / backpressure 側の attachment 保護範囲は別途扱う。自分、HUD、近距離 avatar 成立に必要な attachment は保護するが、遠距離・視野外の他人装飾 attachment を永久保護する設計にはしない。

設定名案:

- `AYALoadingDenseWorldLODEnabled` default `TRUE`
- `AYALoadingDenseWorldLODTraceEnabled` default `FALSE`
- `AYALoadingDenseWorldLODMaxDistance`
- `AYALoadingDenseWorldNearProtectDistance`
- `AYALoadingDenseWorldLODMinChildren`
- `AYALoadingDenseWorldHeavyMeshMaxDistance`
- `AYALoadingDenseWorldHeavyMeshMinTriangles`
- `AYALoadingDenseWorldForwardDot`
- `AYALoadingDenseWorldSmallScreenPct`
- `AYALoadingDenseWorldVerySmallScreenPct`
- `AYALoadingDenseWorldDecisionIntervalFrames`
- `AYALoadingDenseWorldHysteresisFrames`
- `AYALoadingDenseWorldForceLODRefreshIntervalFrames`
- `AYALoadingDenseWorldForceLODRefreshMaxPerFrame`

発動方針:

- ユーザー向けの master は `FPS安定化` とする。短期互換では `AYALoadingFPSGuardEnabled` を使うが、後続整理後は `AYALoadingRenderReliefEnabled` に寄せる。
- `AYALoadingDenseWorldLODEnabled` は FPS安定化内の内部 kill switch として default ON 候補にする。ここを default OFF にすると、ユーザーが FPS安定化を ON にしても scene-bound 対策が発動せず、通常の実機検証で機能が届かない。
- UI からの FPS安定化有効化と連動させる場合も、保存済みユーザー設定を勝手に変更せず、FPS安定化 master と `AYALoadingDenseWorldLODEnabled` の AND として扱う。
- 2026-06-07 再設計後の基本条件は `AYALoadingDenseWorldLODEnabled && AYALoadingFPSGuardEnabled && AYALoadingLODReliefStrength>0 && in_world && !safety_excluded` とする。FPS latch、loading window、teleport window、backlog は gate にしない。
- mouselook 中に常時 dense-world LOD を効かせる要件は、読み込み中 FPS安定化とは別の明示設定として扱う。短期互換で同じ master に残す場合も、ログには `reason=mouselook_profile` と出し、FPS pressure bypass と混同しない。
- 旧コードに残っていた `current_fps < pressure_fps` で ON、`current_fps >= pressure_fps * 1.05` で OFF の latch hysteresis は撤回済みである。`AYALoadingLODReliefStrength` は pressure FPS ではなく LOD profile selector として扱う。
- 検証用に dense-world LOD の active/inactive 状態遷移を `AYAPerf` ログへ出す。再設計後のログは `lod_enabled`, `fps_guard_enabled`, `lod_strength`, `lod_profile`, `active_reason`, `mouselook`, `fps`, `in_world`, `safety_excluded` を含む。候補数、bias 数、forced refresh 数などの詳細集計は `AYALoadingDenseWorldLODTraceEnabled` が ON の時だけ出し、通常検証でログ負荷を増やさない。
- dense-world LOD は破棄/縮小ではなく LOD bias / forced refresh のみを担当する。視野外、遠距離、旧リージョン由来、参照なし候補の破棄/縮小は distance/offscreen cap、teleport sweep、memory sweep 側で扱う。

負荷式上の位置づけ:

```text
T_off = C_cpu + C_scene + G_pixel + U_swap
T_on  = C_cpu + C_scene' + G_pixel * m^2 + U_swap(m) + lambda * R + S_recovery
```

`RenderResolutionMultiplier` は `G_pixel * m^2` を下げる。一方 dense-world LOD は `C_scene' <= C_scene` を狙う。つまり、画面解像度を下げても `geom`, `statesort`, `lighting` が残る場面では、LOD bias によって triangle / drawinfo / material cost を下げる必要がある。ただし LOD 更新頻度や texture 再確保が増えると `lambda * R + S_recovery` が増え、逆に重くなるため、decision interval と hysteresis は必須である。

検証項目:

- OFF/ON で `fps`, `total`, `geom`, `lighting`, `statesort`, `textures`, `ui_swap`, `realloc_count_per_sec` を比較する。
- `RenderResolutionMultiplier` の倍率変化と LOD bias の発動が同時に暴れないこと。
- forced LOD refresh が per-frame cap 内に収まり、`statesort` / `geom` を逆に悪化させないこと。
- 近距離 avatar、attachment、HUD、近距離 world object が未 rez のまま残らないこと。
- `near protect distance` 内の `bias=0`、selected/edit object の `bias=0`、HUD/avatar attachment の `bias=0` をログまたは実機で確認する。
- OFF 後 N 秒以内に通常 LOD 経路へ戻り、低 LOD が固定化されないこと。
- `forced_refresh_per_frame <= AYALoadingDenseWorldForceLODRefreshMaxPerFrame` を満たすこと。

forced LOD refresh の検証式:

```text
F = fps * dt
R_observed = forced_refresh
R_per_sec = R_observed / dt
R_per_frame = R_observed / F
R_cap_frame = MaxPerFrame * F
R_cap_object = N_refresh_candidates * ceil(F / IntervalFrames)
R_upper <= min(R_cap_frame, R_cap_object)
```

2026-06-04 の実機ログでは、旧設定 `MaxPerFrame=64`, `IntervalFrames=4` で以下の burst が出た。

```text
dt=5.02 fps=29.2 candidates=1818 forced_refresh=1184
F ~= 146 frames
R_per_frame ~= 8.1 refresh/frame
R_cap_frame = 64 * 146 = 9344
```

この場合、`MaxPerFrame=64` は上限としてほぼ効いていない。`MaxPerFrame=1` へ落とすと同じ 5 秒窓の理論上限は `1 * 146 = 146` になり、burst refresh は最大でも約 1/8 まで下がる。一方、安定区間では `forced_refresh ~= 64-69 / 5s` で `R_per_frame ~= 1` に張り付いていたため、`MaxPerFrame=1` だけでは安定区間の削減は限定的である。`IntervalFrames=30` へ上げ、同一 object への再 refresh を抑える追加検証が必要。

比較ビルドでは `AYALoadingDenseWorldForceLODRefreshMaxPerFrame=1`, `AYALoadingDenseWorldForceLODRefreshIntervalFrames=30` を初期値にし、`dense_world_lod` ログへ `refresh_per_sec`, `refresh_per_frame`, `force_interval_frames`, `force_max_per_frame` を出す。FPS 改善を見る時は、`biased` が残り、`forced_refresh` と `refresh_per_frame` が下がり、`fps` が上がるかを同じ地点・同じカメラ条件で比較する。

## 比較計測計画

他メタバースとの比較をするなら、体感ではなく以下の条件で測る。

### 共通条件

- キャッシュ削除済み初回ロード。
- 同じネットワーク環境。
- 同じ表示解像度。
- 同じ FPS 計測方法。
- 1% low FPS、最低 FPS、平均 FPS、ロード完了秒数を取る。

### AYAstorm / Second Life

- ログイン直後またはテレポート直後から 120 秒計測。
- リージョン名、視界内 avatar 数、グラフィック設定、キャッシュ状態を記録。
- `AYAPerf` ログでセクション別 ms を記録。

### VRChat

- ワールド入室直後から 120 秒計測。
- avatar 表示設定、Performance Rank filtering、avatar download size 制限を記録。
- 何体が fallback/blocked になったか確認。

### Roblox

- StreamingEnabled の有無が分かる体験を選ぶ。
- 入室直後から 120 秒計測。
- ストリーミング中に未ロード領域が非表示か、プレースホルダーかを観察。

### Unity/Unreal 系

- ロード画面中と、実ワールド表示中を分けて計測。
- 可能ならデバッグ表示で async loading / streaming status を確認する。

## 結論

現時点の結論は以下。

1. 他メタバースが読み込み中に高 FPS に見えるのは、非同期ロード、チャンクストリーミング、フォールバック、重いコンテンツの非表示、ロード画面の利用による可能性が高い。
2. その一方で、ユーザーが見ている FPS が「実ワールド描画中の FPS」ではなく「軽いロード画面の FPS」である可能性もある。
3. Second Life/AYAstorm は、読み込み中でも実リージョンの object/texture/avatar/render 更新をメインループ上で進めるため、ロードと描画が同じフレーム予算を奪い合いやすい。
4. AYAstorm の修正は、単に速くするより先に「ロード中に何を遅らせるか」「何を仮表示するか」「何を描かないか」を設計する必要がある。
5. 次の実装は、計測ログを基準に、Texture、Object idle、Shadow/Impostor を個別に検証し、有効なものだけ採用する。
