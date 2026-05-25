# AYAstorm r32 SSS / FullBright / Glow 修正報告

作成日: 2026-05-26
対象ブランチ: `ayastorm-release`

## 目次

- [概要](#概要)
- [対象不具合](#対象不具合)
- [修正内容](#修正内容)
- [確認状況](#確認状況)
- [影響範囲](#影響範囲)
- [原因調査](#原因調査)
- [関連コード](#関連コード)
- [回帰確認項目](#回帰確認項目)
- [今後の対応](#今後の対応)

## 概要

Skin SSS ON 時に、SSS whitelist UUID と無関係な FullBright + Glow 設置物の Glow が誤って変化する問題を修正した。

修正は `LLPipeline::renderGeomPostDeferred()` 内で `doSkinSSS()` 実行後に post-deferred の通常 render state を復元するもの。SSS の実行位置は PR112 / commit `e2ec57069f` の意図通り `POOL_FULLBRIGHT` 前のまま維持する。

Mac 版では in-world 視覚確認済み。Windows 版と Linux 版では未確認。

## 対象不具合

観測された症状:

- Skin SSS を ON にすると、設置物の Glow が誤って強く見える。
- SSS の各パラメータを変更しても、設置物側の誤 Glow の見え方は変化しない。
- 条件は「Specular Map を含む material が Fullbright + Glow ON」の場合に確定。
- SSS whitelist UUID と関係ない object に Glow 変化が出ている。

このため、主原因は SSS shader の `glow restore` 値が設置物へ直接混入していることではなく、SSS ON によって変化する render state / pass order / Glow channel の扱いにあると判断した。

## 修正内容

対象ファイル:

- `indra/newview/pipeline.cpp`

`LLPipeline::renderGeomPostDeferred()` の `doSkinSSS()` 呼び出し直後に、post-deferred の通常 state を復元する処理を追加した。

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

この修正で行わないこと:

- SSS whitelist UUID 判定は変更しない。
- `mIsSSSTarget` は変更しない。
- `aya_sss_skin_flag` は変更しない。
- SSS shader の blur / strength / glow restore 計算は変更しない。
- SSS pass の実行位置は変更しない。

## 確認状況

Mac 版:

- 2026-05-26 に in-world 視覚確認済み。
- SSS ON 時に、SSS whitelist UUID と無関係な FullBright + Glow 設置物の Glow が誤って変化する問題は修正されていることを確認した。

未確認:

- Windows 版
- Linux 版

Windows 版と Linux 版は PR テスト時の追加回帰確認として別途確認する必要がある。

## 影響範囲

既存の SSS UUID 機能へ問題が出る可能性は低い。

理由:

- whitelist UUID 判定を変更していない。
- SSS 対象 draw 判定を変更していない。
- GBuffer3 `.a` の skin mask 仕様を変更していない。
- SSS shader の計算を変更していない。
- SSS の実行位置 `POOL_FULLBRIGHT` 前を維持している。

正規の Glow 書き込みは `LLDrawPoolGlow::renderPostDeferred()` が自分で `setColorMask(false, true)` を設定してから行う。そのため、今回の state 復元によって通常 Glow の書き込みが壊れる可能性も低い。

ただし、SSS 後から Glow pool 前までのどこかに「alpha channel へ書けること」を暗黙に期待している pass があれば影響する可能性は残る。現時点では、そのような依存は確認していない。

## 原因調査

今回の Glow 問題は、PR112 の SSS 移動で露出した render state leak の可能性が高い。

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

`setColorMask(true, false)` は color だけを書き、alpha を書かない設定である。通常 Glow は `mRT->screen.a` を使うため、FullBright や通常 color pass が alpha を不用意に書かないことが前提になっている。

PR112 前は SSS が FullBright 後に走っていたため、この state leak は FullBright へ影響しにくかった。PR112 後は SSS が FullBright 前に走るため、`doSkinSSS()` が alpha write enabled のまま戻ると、その後の FullBright 系 pass が `mRT->screen.a` を書けてしまう。

これは「SSS 各パラメータを動かしても設置物の誤 Glow が変化しない」という観測とも一致する。

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

### Legacy material 経路

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

### 通常 Glow 経路

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

### 今回の Glow 問題の回帰確認

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

### 基本 FullBright / Glow

条件:

- blank texture + Fullbright + Glow ON。
- FullBright Alpha Mask。
- FullBright Shiny。

期待:

- Glow channel へ入るのは `PASS_GLOW` / emissive path のみ。
- FullBright color pass の alpha が `mRT->screen.a` に入らない。

## 今後の対応

Windows 版と Linux 版で同じ in-world 回帰確認を行う。

もし追加確認で問題が残る場合は、次の順で切り分ける。

1. `skinSSSF.glsl` で `emissiveRect.a` を可視化し、設置物 pixel が skin mask に入っているか確認する。
2. `materialF.glsl` で `aya_sss_skin_flag` を canary 表示し、非 SSS object の `PASS_SPECMAP*` draw が 0 を書いているか確認する。
3. `LLDrawInfo` batching 条件へ `mIsSSSTarget` を一時的に追加し、per-draw uniform leak があるか確認する。
4. `LLDrawPoolGlow::renderPostDeferred()` 開始時の color mask / blend state をログまたは assert で確認する。

必要になった場合は、FullBright 系 pool の入口で `gGL.setColorMask(true, false)` を明示する追加の堅牢化も検討する。
