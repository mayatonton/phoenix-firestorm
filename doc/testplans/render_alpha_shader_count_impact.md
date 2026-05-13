# Render Alpha Material Shader Count Impact

## 結論

`LLDrawPoolAlpha::renderPostDeferred()` の material shader prepare loop を `LLMaterial::SHADER_COUNT * 2` から `LLMaterial::SHADER_COUNT` に縮めても、現行コード前提では描画対象や shader variant は減らない。

減るのは、alpha pass 前に rigged/skinned material shader をもう一度 prepare していた重複処理である。

## 前提

`gDeferredMaterialProgram` は `LLMaterial::SHADER_COUNT * 2` 個の shader を持つ。

- `0..15`: 通常 material shader
- `16..31`: 対応する rigged/skinned material shader

shader 初期化では、通常 material shader に対応する rigged/skinned shader が `mRiggedVariant` として接続される。

```cpp
gDeferredMaterialProgram[i].mRiggedVariant = &gDeferredMaterialProgram[i + 0x10];
```

draw 時は、通常 mesh では `gDeferredMaterialProgram[mask]` を使い、avatar/rigged mesh ではその `mRiggedVariant` を使う。

## 変更前後の動き

変更前:

```text
base 0 prepare -> rigged 16 prepare
...
base 15 prepare -> rigged 31 prepare
rigged 16 prepare
...
rigged 31 prepare
```

変更後:

```text
base 0 prepare -> rigged 16 prepare
...
base 15 prepare -> rigged 31 prepare
```

`prepare_alpha_shader()` は、渡された shader 自身を prepare した後、`mRiggedVariant` があれば再帰的に rigged/skinned shader も prepare する。そのため loop を `LLMaterial::SHADER_COUNT` までにしても、rigged/skinned shader は prepare される。

この変更で省くのは、loop 後半で同じ rigged/skinned shader を直接もう一度 prepare する処理だけである。

## 減る処理

rigged/skinned material shader 16 個分について、以下の重複が減る。

- `shader->bind()`
- `DISPLAY_GAMMA` uniform 設定
- `waterSign` uniform 設定
- `WATER_WATERPLANE` uniform 設定
- `setMinimumAlpha()`
- `mCanBindFast = false`

Frame Profile 上では、shader bind/state setup 由来の `binds` や shader category cost の低下として現れる可能性がある。

## 描画結果への影響

現行コード前提では、描画結果への直接影響は想定しない。

- shader の生成数は変えない。
- `gDeferredMaterialProgram[16..31]` は残る。
- avatar/rigged mesh の描画 path は変えない。
- draw 時の shader 選択は変えない。
- rigged/skinned shader も `mRiggedVariant` 再帰で prepare される。

したがって、この変更は描画対象や shader 種類を減らす変更ではなく、alpha pass 前の状態準備の重複を減らす変更である。

## リスク

この修正は以下の前提に依存している。

- `prepare_alpha_shader()` が `mRiggedVariant` も再帰的に prepare する。
- 通常 material shader の `mRiggedVariant` が対応する rigged/skinned shader に正しく接続されている。
- 通常 material shader と rigged/skinned material shader が、alpha pass 用の共通 uniform/state を同じ値で使える。

将来 `prepare_alpha_shader()` の再帰を消す、`mRiggedVariant` の接続方式を変える、または rigged/skinned 専用の alpha prepare 値を追加する場合は、この loop 範囲を再確認する必要がある。

## 履歴調査

`LLMaterial::SHADER_COUNT * 2` の配列自体は、2013 年の rigged attachments 対応で導入された。

- commit: `14f02e48b877569539c96ef5f261ac8b3943579f`
- date: `2013-03-29`
- author: `Dave Parks`
- message: `NORSPEC-66 Hook up material parameters to rigged attachments.`

この commit では `gDeferredMaterialProgram` が `LLMaterial::SHADER_COUNT` から `LLMaterial::SHADER_COUNT * 2` に拡張され、`HAS_SKIN` permutation と `hasObjectSkinning` が追加された。意図は通常 material shader に加えて rigged/skinned material shader を持つことである。

`renderPostDeferred()` の alpha prepare loop に `LLMaterial::SHADER_COUNT * 2` が入ったのは、2022 年の water alpha 修正である。

- commit: `07bca31e06e4219401f82ae04539418c65e22ea8`
- date: `2022-10-10`
- author: `Dave Parks`
- message: `SL-18190 Fix alpha not playing nice with water surface by split LLDrawPoolAlpha into two passes, one above water, one below water, and clip against water plane. Currently brute forces two complete alpha passes, still need to cull against water plane and add support for fullbright shaders.`

この commit は water plane / waterSign を alpha shader に渡すための変更であり、material shader 全体を prepare する loop を追加している。ただし、なぜ loop 範囲が `LLMaterial::SHADER_COUNT * 2` である必要があるのかを明示するコメントは見つからなかった。

2022 年 6 月の `616f2b639b` では、material alpha prepare loop は `LLMaterial::SHADER_COUNT` までで、`prepare_alpha_shader()` の `mRiggedVariant` 再帰もすでに存在していた。このため今回の変更は、履歴上は以前の前提に近い形へ戻すものでもある。

## 参考コード

- `indra/newview/lldrawpoolalpha.cpp`: `prepare_alpha_shader()` と `renderPostDeferred()`
- `indra/newview/llviewershadermgr.cpp`: `gDeferredMaterialProgram` の生成、`HAS_SKIN`、`mRiggedVariant` 接続
- `indra/llrender/llglslshader.h`: `mRiggedVariant`
- `indra/llrender/llglslshader.cpp`: shader bind/profile counter

