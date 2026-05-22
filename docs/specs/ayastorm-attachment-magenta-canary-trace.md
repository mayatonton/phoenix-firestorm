# AYAstorm Attachment Magenta Canary — trace + plan

## §1 目的

AYAstorm 描画系 fix で 2 度 (Plan D / Plan E1) 推論ベース実装が空振りした。AYA さん指示:

> 装着物をすべてマゼンダ色にしてみてください。まずそれができないと無理でしょうから。

= 「色シフト fix の前に、Claude が shader 上で **attachment という概念を touch できる** ことを最小コードで証明せよ」。本書はそのための canary 実装の trace と plan。

memory: `feedback_doubt_self_first.md` / `feedback_render_full_trace_first.md` / `feedback_diag_canary_design.md`。

## §2 attachment 識別の C++ 根拠

### §2.1 `LLDrawInfo::mAttachedToAvatar` (r30 P2 既存)

- 宣言: `indra/newview/llspatialpartition.h:130`
- 初期化: `indra/newview/llvovolume.cpp:5838` → `draw_info->mAttachedToAvatar = vobj->getAvatar();`
- **意味**: vobj が attachment object なら親 avatar の `LLPointer<LLVOAvatar>` が入る。**rigged / non-rigged 不問**、attachment であれば notNull。
- HUD attachment / particle / 通常 prim / avatar body 本体は notNull にならない (vobj->getAvatar() が返す条件)。
- 既使用例: velocity pass (`indra/newview/lldrawpool.cpp:834`) で `params.mAttachedToAvatar.notNull()` 判定。本 canary は同じ判定を alpha pool draw 直前に流用する。

### §2.2 alpha pool draw loop の hook 点

`indra/newview/lldrawpoolalpha.cpp` の `renderAlphaHighlight()` / `renderAlpha()` 系で:
- L706 ~ L887: draw batch loop (rigged + non-rigged 共通)
- L843: 実 draw call `params.mVertexBuffer->drawRange(...)`
- L791: `gPipeline.bindDeferredShaderFast(*target_shader);` (非 PBR alpha)
- L734: 同 (PBR alpha BLEND)

両 bind path とも L843 の drawRange へ収束する。**L843 直前** が「全 alpha pool draw call を 1 箇所で捕まえる」最小 hook。

## §3 shader 編集対象の選定

### §3.1 alpha pool の shader file 一覧 (`llviewershadermgr.cpp` から特定)

| program | shader file (fragment) | 行 |
|---------|----------------------|---|
| `gDeferredAlphaProgram` | `class2/deferred/alphaF.glsl` (class fallback で class1/class2 自動選択) | 1903, 1963 |
| `gDeferredAvatarAlphaProgram` | `class2/deferred/alphaF.glsl` | 2581 |
| `gDeferredFullbrightAlphaMaskAlphaProgram` | `class1/deferred/fullbrightF.glsl` | 2105+ |
| `gDeferredMaterialProgram[i]` (16 種) | `class1/deferred/materialF.glsl` | 1404 |
| `gDeferredPBRAlphaProgram` | `class1/deferred/pbralphaF.glsl` | 1571-72 |

### §3.2 canary 初手スコープ: `alphaF.glsl` 1 file のみ

「段階を踏む」AYA さん指示に従い:
- まず `class2/deferred/alphaF.glsl` だけ patch
- alpha mesh 髪 (AYA 観察対象) が **完全に magenta** → alpha 系プログラムでは識別 OK と確定、次段 (material/fullbright/PBR) 拡張
- **部分的に magenta** → 髪が alphaF 以外の shader (materialF / pbralphaF など) を踏んでいる証拠、その shader 拡張
- **全く magenta にならない** → C++ hook が走っていない / `mAttachedToAvatar` の前提崩壊、根本見直し

これは canary そのもの。「全部いっぺんに magenta」を狙うと **失敗時に diagnostic 価値が落ちる** (どの path が悪いか切り分けられない)。

## §4 実装内容

### §4.1 shader 編集: `class2/deferred/alphaF.glsl`

- file 末尾 main 関数の `frag_color = max(color, vec4(0));` (L317) **直前** に:
  ```glsl
  // <AYAstorm canary: attachment magenta override>
  // C++ (lldrawpoolalpha.cpp) が drawRange 直前に
  // mAttachedToAvatar.notNull() のとき 1 をセット。本 uniform を立てた batch は
  // 全 pixel が magenta になる。診断目的、shader uniform 未使用なら GL が剥がす。
  if (aya_attachment_canary != 0)
  {
      frag_color = vec4(1.0, 0.0, 1.0, color.a);
      return;
  }
  // </AYAstorm>
  ```
- file 冒頭 uniform 宣言部 (L34 `out vec4 frag_color;` の付近) に:
  ```glsl
  // <AYAstorm canary>
  uniform int aya_attachment_canary;
  // </AYAstorm>
  ```

### §4.2 C++ 編集: `lldrawpoolalpha.cpp`

L843 `params.mVertexBuffer->drawRange(...)` の **直前** に:
```cpp
// <AYAstorm canary: attachment magenta override>
{
    static LLStaticHashedString s_aya_canary("aya_attachment_canary");
    if (current_shader)
    {
        current_shader->uniform1i(
            s_aya_canary,
            params.mAttachedToAvatar.notNull() ? 1 : 0);
    }
}
// </AYAstorm>
```

`LLStaticHashedString` 経由 (`llglslshader.h:225` の overload)。shader 側で uniform 未参照なら GL が optimize-out、`getUniformLocation` が -1 を返して silent no-op。

### §4.3 触らないもの (canary 範囲外)

- `materialF.glsl` / `pbralphaF.glsl` / `pbropaqueF.glsl` / `fullbrightF.glsl` — 次段で拡張判定
- `lldrawpoolmaterials.cpp` / `lldrawpoolpbropaque.cpp` — 同上
- LLShaderMgr enum 追加 — `LLStaticHashedString` 経由なら不要
- pipeline.cpp / gbuffer / FBO — 不要

## §5 期待結果 & 診断分岐

### §5.1 検証手順 (AYA さん依頼)

1. 起動後、自分のアバター (任意) を視野に入れる
2. 髪・服・靴・アクセサリ等が **完全に magenta 一色** で塗りつぶされるか確認
3. avatar 本体 (BoM 肌部) は magenta にならないこと確認
4. 周囲の prim / 地形 / sky は magenta にならないこと確認

### §5.2 結果から得る診断情報

| 観察結果 | 結論 | 次の手 |
|---------|-----|------|
| 髪が完全 magenta、肌は normal | 髪は `alphaF.glsl` path 上、識別成功 | 色シフト fix 設計を alphaF.glsl で開始 |
| 髪が **部分的に** magenta (毛束の一部だけ) | 髪が複数 shader path に跨っている (materialF / pbralphaF など) | 次 canary で materialF.glsl 拡張 |
| 髪が全く magenta にならない | C++ hook が走っていない / `mAttachedToAvatar` の前提が崩壊 | C++ hook 配置点見直し、`LL_INFOS` で `params.mAttachedToAvatar` の値を log |
| 服 / 靴も magenta になる | alpha mesh 系の opaque-blend attachment も alphaF 上、想定通り | 次段で opaque pool 拡張するか判断 |
| 肌が magenta になってしまう | `mAttachedToAvatar` が body 本体にも notNull を返している、AYA spec の前提崩壊 | trace 全面見直し |

## §6 想定 risk

- **uniform default 値**: GL spec 上、未 set な int uniform は 0。C++ で attachment でないとき毎回明示 0 を set するので、program 切替後も leakage なし。
- **shader program 数**: `alphaF.glsl` を fragment shader に持つ program は複数 (§3.1 表)。1 file 編集で全 program に伝播するため、編集コスト 1。
- **shader class**: AYAstorm Cinematic mode が class1/2/3 のどれを使うかで `alphaF.glsl` の実体が変わる可能性。`class2` が存在し、class3 は無い (Glob 結果)。class1 は無い (= class fallback)。Cinematic でも class2 が使われる前提。検証で magenta 出なければ class fallback を疑う。
- **canary cleanup**: AYA OK 出たら次の本実装 commit 前に本 canary コード除去 (memory `feedback_remove_verification_logs.md`)。または `aya_attachment_canary` を keep して将来の diagnostic 用に残す判定を AYA に仰ぐ。

## §7 file:line index

- `indra/newview/llspatialpartition.h:130` — `LLDrawInfo::mAttachedToAvatar`
- `indra/newview/llvovolume.cpp:5838` — `mAttachedToAvatar` 初期化
- `indra/newview/lldrawpool.cpp:834` — 既存使用例 (velocity pass)
- `indra/newview/lldrawpoolalpha.cpp:706-887` — draw batch loop
- `indra/newview/lldrawpoolalpha.cpp:843` — drawRange (本 canary hook 直前)
- `indra/newview/app_settings/shaders/class2/deferred/alphaF.glsl:317` — `frag_color = max(color, vec4(0));`
- `indra/llrender/llglslshader.h:225` — `uniform1i(LLStaticHashedString, GLint)` overload

## 更新履歴

- 2026-05-21: 初版 (Step 1 trace + plan、AYA さん「装着物をすべてマゼンダ色」指示を受けて起稿)
