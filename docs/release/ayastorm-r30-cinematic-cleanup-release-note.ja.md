# AYAstorm r30 Cinematic Cleanup — リリース告知

**r30 Cinematic Cleanup は Phase 6 (live cvar 移植) 後の Cinematic Controls floater を磨き直すリリース** — 全 35 cvar / 9 タブを端から端まで監査し、コード上で dispatch されていなかった項目を生かし、見える効果が出ない項目を撤去します。

完全 audit と cvar 別 trace は永続資料 (`docs/specs/ayastorm-r30-cinematic-controls-cleanup.md`) に置いてあります。本 note は入口と移行サマリです。

---

## AYAstorm r30 Cinematic Cleanup

### Headline: floater の全スライダーが本当に効くようになる

Phase 6 で Cinematic Controls floater に 13 件の live cvar を載せた後、コード trace audit を行ったところ、**floater から触れる 5 項目が実際には何も起きていない** ことが判明しました。cvar の値域が `pipeline.cpp` 側 dispatch を超えていたり、他の経路で完全に bypass されていたりが原因です。Cleanup はこの 5 件を片付けます。

### 変更点

| ID | コントロール | 処置 | 効果 |
|---|---|---|---|
| A.1 | Shadow Detail スライダー最大値 | **0–3 → 0–2 に縮小** | level 3 は何もしない (LL/BD ではスポット光とプロジェクター光が同じ `mSpotShadow[]` 配列を共有しており、level 2 で既に両方が有効)。tooltip を訂正 |
| A.2 | `RenderFSAAType = 3` | **SMAA + T2x として dispatch するようになった** | 以前は何もしない値だった (1=FXAA / 2=SMAA のみ実装)。FSAAType が AA の単一セレクターに: 0=オフ, 1=FXAA, 2=SMAA, 3=SMAA+T2x |
| A.3 | Cinematic 時の `RenderShadowResolutionScale` | **per-cascade Vector4 に乗算されるようになった** | Cinematic per-channel shadow 経路で完全に無視されていた。Scale を `RenderShadowResolution` と `RenderProjectorShadowResolution` の両方に乗算、0 サイズ allocation 防止のため 64px の下限あり |
| A.4 | General タブの「Deferred Rendering」チェックボックス | **撤去** | Cinematic mode は設計上 deferred 専用 (forward path は維持していない)。Header テキストに deferred 必須要件を明記。cvar 自体は変更なし、Cinematic 以外での利用は環境設定→グラフィック→Advanced Lighting Model から従来通りアクセス可 |
| A.5 | `RenderSMAAT2x` チェックボックス + cvar | **撤去** | 機能は A.2 (`FSAAType = 3`) に吸収。並列 2 トグルより単一 enum のほうが UX 明快 |

### 移行注意

- **`RenderSMAAT2x` は消えます。** debug settings で以前 `1` に設定していた場合は、同等の効果のために `RenderFSAAType = 3` に切替を。古い cvar 値は警告なしに無視されます — `pipeline.cpp` 側 dispatch は BD parity の都合で Cinematic mode では元々空回りしていたため、ユーザー視点での実害はゼロです
- **`RenderDeferred` チェックボックスは移動。** 環境設定→グラフィック→Advanced Lighting Model を使ってください。Cinematic mode は ON 必須です
- **`RenderShadowDetail = 3` は floater から選べなくなります。** debug settings からは引き続き設定可能ですが、dispatch 上は `= 2` と同等です (挙動変化なし)

### ドキュメント

- 完全 cleanup spec + cvar 別 audit: [`docs/specs/ayastorm-r30-cinematic-controls-cleanup.md`](../specs/ayastorm-r30-cinematic-controls-cleanup.md)
- 親 spec (r30 章): [`docs/specs/ayastorm-r30-cinematic-chapter.md`](../specs/ayastorm-r30-cinematic-chapter.md)
- Phase 6 (本 cleanup が片付ける live cvar 移植): [`docs/specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md`](../specs/ayastorm-r30-bd-full-port-phase6-live-cvar-port-spec.md)
