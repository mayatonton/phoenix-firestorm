# AYAstorm r31-2 SSS・フルブライト・グロー修正報告

作成日: 2026-05-26
対象ブランチ: `ayastorm-release`

## 目次

- [結論](#結論)
- [今回の対象範囲](#今回の対象範囲)
- [確認状況](#確認状況)
- [修正1: SSS と無関係な設置物グローの変化](#修正1-sss-と無関係な設置物グローの変化)
- [修正2: 太陽・月・星テクスチャの非表示](#修正2-太陽月星テクスチャの非表示)
- [今回採用しなかった天体ブルーム調整](#今回採用しなかった天体ブルーム調整)
- [既存修正との関係](#既存修正との関係)
- [関連コード](#関連コード)
- [回帰確認項目](#回帰確認項目)
- [残作業](#残作業)

## 結論

r31-2 では次の 2 点を修正対象とする。

1. Skin SSS を ON にすると、SSS whitelist UUID と無関係な FullBright + Glow 設置物の Glow が誤って変化する問題。
2. 太陽・月・星テクスチャが表示されない問題。

SSS / FullBright / Glow 問題は、SSS pass 後に render state が復元されず、後続の FullBright 系 pass が `mRT->screen.a` を書ける状態になっていたことが主因と判断した。修正は `doSkinSSS()` 後に post-deferred の通常 state を復元する範囲に限定する。

太陽・月・星の問題は、sky-domain の HAS_EMISSIVE 経路で visual alpha を 0 固定していたことが原因と判断した。修正は太陽・月・星・空本体の visual alpha を保持する範囲に限定する。

天体テクスチャへ専用の Glow / Bloom 量を付与する試作は、見た目の変化が十分に確認できなかったため r31-2 には含めない。

## 今回の対象範囲

対象に含めるもの:

- SSS 後の render state 復元。
- 太陽・月・星・空本体の visual alpha 復旧。
- 既存 SSS UUID 機能、PR112 の FullBright 越し SSS 透過防止、雲表示の回帰確認。

対象に含めないもの:

- SSS whitelist UUID 判定の変更。
- `mIsSSSTarget` / `aya_sss_skin_flag` の仕様変更。
- SSS shader の blur / strength / glow restore 計算変更。
- SSS pass の実行位置変更。
- 天体テクスチャ専用 Bloom 設定、UI、shader uniform の追加。

## 確認状況

Mac 版:

- in-world 視覚確認済み。
- SSS ON 時に、SSS whitelist UUID と無関係な FullBright + Glow 設置物の Glow が誤って変化する問題は修正されていることを確認した。
- 太陽・月・星テクスチャが表示されることを確認した。

未確認:

- Windows 版
- Linux 版

Windows 版と Linux 版は PR テスト時に追加確認する。

## 修正1: SSS と無関係な設置物グローの変化

### 症状

- Skin SSS を ON にすると、設置物の Glow が誤って強く見える。
- SSS の各パラメータを変更しても、設置物側の誤 Glow の見え方は変化しない。
- 条件は「Specular Map を含む material が Fullbright + Glow ON」の場合に確定。
- SSS whitelist UUID と関係ない object に Glow 変化が出ている。

このため、主原因は SSS shader の `glow restore` 値が設置物へ直接混入していることではなく、SSS ON によって変化する render state / pass order / Glow channel の扱いにあると判断した。

### 原因

PR112 / commit `e2ec57069f` では、FullBright object 越しに SSS 適用 body の pink shadow が透ける問題を直すため、SSS dispatch を FullBright pool 前へ移動した。

この順序は維持する必要がある。SSS pass を `POOL_ALPHA_POST_WATER` 側へ戻すと、FullBright 越しの pink bleed が再発する。

一方で、`LLPipeline::doSkinSSS()` は fullscreen composite のために alpha write を有効化する。

```cpp
LLGLDepthTest depth(GL_FALSE, GL_FALSE);
gGL.setColorMask(true, true);
...
gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
              LLRender::BF_ZERO, LLRender::BF_ONE);
...
gGL.setSceneBlendType(LLRender::BT_ALPHA);
```

しかし、関数終了時に `gGL.setColorMask(true, false)` へ戻していなかった。

`renderGeomPostDeferred()` は post-deferred 開始時に alpha write を止める前提で進む。

```cpp
gGL.setSceneBlendType(LLRender::BT_ALPHA);
gGL.setColorMask(true, false);
```

通常 Glow は `mRT->screen.a` を使う。そのため、FullBright や通常 color pass が alpha を不用意に書かないことが前提になっている。

PR112 前は SSS が FullBright 後に走っていたため、この state leak は FullBright へ影響しにくかった。PR112 後は SSS が FullBright 前に走るため、`doSkinSSS()` が alpha write enabled のまま戻ると、その後の FullBright 系 pass が `mRT->screen.a` を書けてしまう。

### 修正内容

対象ファイル:

- `indra/newview/pipeline.cpp`

`LLPipeline::renderGeomPostDeferred()` の `doSkinSSS()` 呼び出し直後に、post-deferred の通常 state を復元する。

```cpp
doSkinSSS();
// SSS enables alpha writes for its fullscreen composite. Restore
// the post-deferred default before FullBright/Glow pools so
// color passes do not leak into mRT->screen.a.
gGL.setSceneBlendType(LLRender::BT_ALPHA);
gGL.setColorMask(true, false);
```

この修正で行うこと:

- SSS fullscreen composite 後に blend state を `BT_ALPHA` へ戻す。
- alpha write を止め、color write のみに戻す。
- 後続の FullBright / FullBright Alpha Mask / FullBright Shiny が `mRT->screen.a` を誤って書くことを防ぐ。

### 影響範囲

既存の SSS UUID 機能へ問題が出る可能性は低い。

理由:

- whitelist UUID 判定を変更していない。
- SSS 対象 draw 判定を変更していない。
- GBuffer3 `.a` の skin mask 仕様を変更していない。
- SSS shader の計算を変更していない。
- SSS の実行位置 `POOL_FULLBRIGHT` 前を維持している。

正規の Glow 書き込みは `LLDrawPoolGlow::renderPostDeferred()` が自分で `setColorMask(false, true)` を設定してから行う。そのため、今回の state 復元によって通常 Glow の書き込みが壊れる可能性も低い。

ただし、SSS 後から Glow pool 前までのどこかに「alpha channel へ書けること」を暗黙に期待している pass があれば影響する可能性は残る。現時点では、そのような依存は確認していない。

## 修正2: 太陽・月・星テクスチャの非表示

### 症状

r31-2 対応中に、次の sky-domain texture が表示されない問題が報告された。

- 太陽テクスチャが表示されない。
- 月テクスチャが表示されない。
- 星が表示されない。

### 原因

雲消失と同じく、HAS_EMISSIVE 経路で visual alpha を 0 にしていることが原因と判断した。

現 `ayastorm-release` / r31-2 作業ブランチでは、雲だけは `6ae5f9bf1c` で `frag_data[3].a = alpha1` に戻っている。一方で、太陽・月・星・空本体はまだ `frag_data[3].a = 0.0` を書いていた。

該当 shader:

- `indra/newview/app_settings/shaders/class1/deferred/sunDiscF.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/moonF.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/starsF.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/skyF.glsl`

`softenLightF.glsl` は `GBUFFER_FLAG_SKIP_ATMOS` の sky-domain pixel では、HAS_EMISSIVE 時に `colorEmissive.rgb` を使って sky-domain color を合成する。この合成は skybox 側の alpha blending の影響を受けるため、太陽・月・星が `frag_data[3].rgb` に色を持っていても `frag_data[3].a = 0.0` だと表示に寄与しない。

### 修正内容

太陽・月・星・空本体の HAS_EMISSIVE 経路で、SSS 回避のために alpha を 0 固定しない。各 shader の visual alpha を維持する。

- `sunDiscF.glsl`: `frag_data[3] = vec4(c.rgb, c.a)`
- `moonF.glsl`: `frag_data[3] = vec4(c.rgb, c.a)`
- `starsF.glsl`: `frag_data[3] = vec4(col.rgb, col.a)`
- `skyF.glsl`: `frag_data[3] = vec4(color.rgb, 1.0)`

この方針は `fix/ayastorm-cloud-postprocess-chain` の `4188880321` と同じ方向であり、雲復旧 `6ae5f9bf1c` とも整合する。

### SSS 側の安全性

- `skinSSSF.glsl` には `d_raw >= 0.9999` の far-plane gate がある。
- sky / sun / moon / stars は `GBUFFER_FLAG_SKIP_ATMOS` の sky-domain draw であり、SSS 対象 object ではない。
- したがって、sky-domain の visual alpha を戻しても、SSS composite は far-plane sky pixel を早期 return で除外する。

通常描画側では、sky-domain の texture 表示に visual alpha が必要である。SSS 判定を shader 出力側で潰すのではなく、SSS 側の gate と render state 復元で閉じるため、太陽・月・星の表示と SSS 誤適用防止を両立できる。

## 今回採用しなかった天体ブルーム調整

太陽・月・星テクスチャへ専用の Bloom source を追加する案も検証したが、現時点では採用しない。

試した方向:

- `softenLightF.glsl` の sky-domain 合成後 alpha に、太陽・月・星の visual alpha または輝度を bloom source として渡す。
- `AYACelestialTextureGlowStrength` のような専用 cvar を追加し、AYAstorm Controls へ UI を置く。
- 通常の `RenderGlowStrength` / `RenderGlowWidth` と組み合わせて最終 Bloom を調整する。

見送り理由:

- 実機確認で、パラメータ変更に対する見た目の変化が十分に確認できなかった。
- 既存の Glow 抽出は `mRT->screen.a` と輝度抽出の組み合わせで動いており、sky-domain の emissive alpha を Bloom 用途へ再利用すると、SSS / sky visual alpha / postprocess alpha の責務が再び混ざる。
- 今回の不具合報告は「太陽・月・星テクスチャが表示されない」ことであり、Bloom 量の新規調整は別件として切り分けた方が回帰範囲を小さく保てる。

したがって、今回の r31-2 対応では `AYACelestialTextureGlowStrength` などの新規設定・UI・shader uniform は導入しない。天体 Bloom を扱う場合は、既存 Glow pipeline への接続点を再設計し、SSS mask / sky visual alpha / Glow alpha を明確に分離した別タスクとして扱う。

## 既存修正との関係

雲消失の既存修正は、今回と同じく `gbuffer3.a` / alpha channel の用途衝突が原因だった。

関連ブランチ:

- `fix/ayastorm-cloud-postprocess-chain`
- `fix/r30-13-cloud-recovery`

関連 commit:

- `6ae5f9bf1c` r30 #13 雲消失 fix: `cloudsF.glsl` HAS_EMISSIVE 経路の alpha を `alpha1` に戻す
- `4188880321` Fix sky emissive alpha blending

現在の `ayastorm-release` には `6ae5f9bf1c` が入っている。`fix/ayastorm-cloud-postprocess-chain` 側の `4188880321` は、sky / moon / stars / sunDisc も visual alpha を保持する広い修正だが、現 `HEAD` にはその commit 全体は入っていない。

雲消失の流れ:

1. r20 SSS の skin mask として `gbuffer3.a` を使った。
2. sky/cloud 側が SSS 対象にならないよう、HAS_EMISSIVE 経路で `frag_data[3].a = 0.0` にした。
3. しかし雲の描画では同じ alpha が合成にも使われるため、雲自体が消えた。
4. 修正では `cloudsF.glsl` の visual alpha を `alpha1` に戻し、SSS 側は `skinSSSF.glsl` の far-plane / sky-domain gate で除外する方針にした。

今回の修正でも同じ注意が必要である。非 SSS object の誤 Glow を避けるために FullBright / material / sky domain の alpha 意味を潰すと、雲消失と同種の回帰を起こす可能性がある。

そのため、object や shader の alpha 出力仕様は不要に変更しない。SSS 判定は SSS 側で閉じ、通常描画 pass の alpha 用途を SSS 対策で壊さない。

## 関連コード

### SSS 実行タイミング

`LLPipeline::renderGeomPostDeferred()` では、SSS pass は `POOL_FULLBRIGHT` 到達前に一度だけ実行される。

```cpp
U32 sss_pass = LLDrawPool::POOL_FULLBRIGHT;

if (cur_type >= sss_pass && !done_sss)
{
    if (dispatch_r20)
    {
        doSkinSSS();
    }
    done_sss = true;
}
```

この構造は PR112 の再発防止条件なので維持する。

### SSS シェーダ

`indra/newview/app_settings/shaders/class1/deferred/skinSSSF.glsl`

SSS shader は `emissiveRect.a` を skin mask として読み、skin bit が立っている pixel だけ alpha を持つ。

```glsl
float skin_mask = texture(emissiveRect, tc).a;
float skin_bit = (skin_mask >= 0.5) ? 1.0 : 0.0;
frag_color = vec4(sum, aya_strength * skin_bit);
```

`glow restore` 相当の加算は `sum` に含まれるが、最終合成の alpha は `aya_strength * skin_bit` で制限される。

### レガシーマテリアル経路

`indra/newview/lldrawpoolmaterials.cpp`

material pool は draw info ごとに `aya_sss_skin_flag` uniform を設定する。

```cpp
F32 skinFlag = params.mIsSSSTarget ? 1.f : 0.f;
glUniform1f(sssSkin, skinFlag);
```

`indra/newview/app_settings/shaders/class3/deferred/materialF.glsl`

Specular Map を含む legacy material は `POOL_MATERIALS` / `PASS_SPECMAP*` を通り、deferred shader で GBuffer へ書く。

```glsl
frag_data[0] = max(vec4(diffcol.rgb, emissive), vec4(0));
frag_data[1] = max(vec4(spec.rgb, glossiness), vec4(0));
frag_data[2] = encodeNormal(norm, env, flag);

#if defined(HAS_EMISSIVE)
    frag_data[3] = vec4(0, 0, 0, aya_sss_skin_flag);
#endif
```

今回の再現条件はこの path なので、単純な FullBright object と同じ扱いではない。

### 通常グロー経路

設置物の通常 Glow は SSS shader の `glow restore` ではなく、viewer の通常 Glow 経路で扱われる。

`LLDrawPoolGlow::renderPostDeferred()`:

```cpp
gGL.setSceneBlendType(LLRender::BT_ADD);
gGL.setColorMask(false, true);
pushBatches(LLRenderPass::PASS_GLOW, true, true);
gGL.setColorMask(true, false);
```

`emissiveF.glsl`:

```glsl
float a = diffuseLookup(vary_texcoord0.xy).a * vertex_color.a;
frag_color = max(vec4(0, 0, 0, a), vec4(0));
```

`glowExtractF.glsl`:

```glsl
vec4 col = texture(diffuseMap, vary_texcoord0.xy);
frag_color.a = max(col.a, mix(lum, warmth, warmthAmount) * maxExtractAlpha);
```

最終 Glow 抽出は `mRT->screen.a` を使う。したがって、FullBright color pass が誤って `screen.a` を書くと、Glow が変化して見える。

## 回帰確認項目

### PR112 回帰確認

条件:

- FullBright opaque object の背後に SSS whitelist 済み body/head がある。
- SSS ON。

期待:

- FullBright object 表面に body/head 形状の pink shadow が透けない。
- SSS pass は引き続き FullBright 描画前に走る。
- `sss_pass = LLDrawPool::POOL_FULLBRIGHT` は維持されている。

### 今回のグロー問題

条件:

- Specular Map を含む legacy material。
- Fullbright ON。
- Glow ON。
- SSS OFF/ON 比較。

期待:

- 設置物の Glow が SSS OFF/ON で変化しない。
- SSS strength / glow gain / blur radius / glow color を変更しても、非 SSS 設置物の Glow が変化しない。

### 既存 SSS UUID 機能

条件:

- SSS whitelist 済み body/head。
- SSS ON。

期待:

- 登録済み UUID の body/head には従来通り SSS がかかる。
- SSS parameter は登録済み skin target のみに効く。
- 未登録 object には SSS tint / blur / glow restore が乗らない。

### 基本フルブライト・グロー

条件:

- blank texture + Fullbright + Glow ON。
- FullBright Alpha Mask。
- FullBright Shiny。

期待:

- Glow channel へ入るのは `PASS_GLOW` / emissive path のみ。
- FullBright color pass の alpha が `mRT->screen.a` に入らない。

### スカイドメインテクスチャ

条件:

- HAS_EMISSIVE 有効。
- 太陽、月、星が見える時刻または EEP 設定。
- SSS ON/OFF 比較。

期待:

- 太陽テクスチャが表示される。
- 月テクスチャが表示される。
- 星が表示される。
- SSS ON にしても sky-domain に SSS tint / blur がかからない。
- 雲は `6ae5f9bf1c` 後と同じく表示される。

## 残作業

Windows 版と Linux 版で同じ in-world 回帰確認を行う。太陽・月・星の sky-domain texture も追加確認対象に含める。

もし追加確認で問題が残る場合は、次の順で切り分ける。

1. `skinSSSF.glsl` で `emissiveRect.a` を可視化し、設置物 pixel が skin mask に入っているか確認する。
2. `materialF.glsl` で `aya_sss_skin_flag` を canary 表示し、非 SSS object の `PASS_SPECMAP*` draw が 0 を書いているか確認する。
3. `LLDrawInfo` batching 条件へ `mIsSSSTarget` を一時的に追加し、per-draw uniform leak があるか確認する。
4. `LLDrawPoolGlow::renderPostDeferred()` 開始時の color mask / blend state をログまたは assert で確認する。

必要になった場合は、FullBright 系 pool の入口で `gGL.setColorMask(true, false)` を明示する追加の堅牢化も検討する。
