# AYAstorm r30 — リリース告知

**r30 は撮影描画章 (r30+) の第 1 弾**で、AYAstorm に **Cinematic mode** を新設し、BlackDragon Viewer (BD) 995a1354d8 の描画 pipeline を **1:1 完全移植** して並走できるようにします。AYAstorm View (r14-r20 拡張) と BlackDragon の写真寄り視覚表現を、**1 個の cvar (`AYAVisualRealismEnabled`) で再起動切替** できます。

> **配信形態**: r30 は単独タグで発行予定 (β release から開始)。
>
> **再起動切替モデル**: mode 切替は再起動必須 (Phase 4 G4 で round-trip 動作は推奨確認、安定性確証は β feedback 後)。

実装・移植戦略・shader mount 表・受入基準は永続資料 (docs/specs/ 配下) に常駐し、本ノートはそこへの誘導と差分ハイライトに徹します。

| 資料 | 内容 |
|---|---|
| [`ayastorm-r30-bd-full-port-inventory.md`](../specs/ayastorm-r30-bd-full-port-inventory.md) | Phase 0 inventory (BD と AY の差分棚卸し、shader 49 + cpp 92 + cvar 拡張) |
| [`ayastorm-r30-bd-full-port-phase1-audit.md`](../specs/ayastorm-r30-bd-full-port-phase1-audit.md) | Phase 1 audit (REDO/REUSE 分類 + r14-r20 gating 棚卸し) |
| [`ayastorm-r30-bd-full-port-phase2-spec.md`](../specs/ayastorm-r30-bd-full-port-phase2-spec.md) | Phase 2 architectural (D1-D4 dispatch 構造) |
| [`ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md) | Phase 3.2 C++ Cinematic dispatch (53 file) |
| [`ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md) | Phase 3.5 AY-only Render* cvar 26 件 dispatch |
| [`ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md) | Phase 3.8 shader 49 file A/B/C/D mount |
| [`ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md) | Phase 3.9 BD UI floater + bdsidebar mount |
| [`ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md) | Phase 4 3 mode 受入検証 (G1-G5) |
| [`ayastorm-r30-bd-full-port-phase5-cleanup-spec.md`](../specs/ayastorm-r30-bd-full-port-phase5-cleanup-spec.md) | Phase 5 cleanup / release prep |

---

## AYAstorm r30 — BlackDragon 完全移植 + 3 mode 統合

### r30 の柱: 1 viewer / 3 mode で「Firestorm 互換」「AYAstorm 視覚拡張」「BlackDragon 写真撮影」を切替

AYAstorm はこれまで r14-r20 で独自の視覚的リアリティ拡張 (atmospheric perspective / cloud volumetric / sun Kelvin modulator / SSS skin marker / translucency / chromatic aberration / SMAA T2x / volumetric godrays / depth-of-field 強化 等) を積み上げてきました。一方で BlackDragon Viewer は写真撮影向けに別の方向で進化を続けており、AYAstorm 利用者からも「BlackDragon の撮影体験そのものを AYAstorm から呼べないか」という需要がありました。

r30 は **BD の描画 pipeline を完全に 1:1 移植** し、AYAstorm 内に **3 モード** を統合します:

| `AYAVisualRealismEnabled` | mode 名 | 描画 pipeline | 用途 |
|---|---|---|---|
| `0` | Firestorm View | Linden / Firestorm baseline (AY 拡張 OFF) | 配信視聴 / 業務 / リソース節約 |
| `1` | AYAstorm View (既定) | AY r14-r20 視覚的リアリティ拡張 | 通常の AYAstorm 体験 |
| `2` | Cinematic | BlackDragon 995a1354d8 pipeline 1:1 移植 | 写真撮影 / machinima |

### 仕組み: 3 モードを並走させる 4 層 dispatch

1. **C++ dispatch (Phase 3.7)**: pipeline.cpp / lldrawpool* / llviewershadermgr 等 53 file が `AYAVisualRealismEnabled` を読み、mode 別に shader bind / state setting / render order を切替
2. **shader permutation (Phase 3.8)**: 49 REDO shader を 4 strategy で mount
   - **A**: uniform 給餌で吸収 (5 file、shader 触らず neutral 値で collapse)
   - **B**: overwrite (9 file、BD baseline で上書き、AY 拡張なし)
   - **C**: `#if AYASTORM_CINEMATIC` permutation (26 file、AY と BD を同 file 共存)
   - **D**: dual-file mount (2 file、`cinematic_bd/` 配下に BD 別配置 + shadermgr 探索切替)
3. **cvar 拡張 (Phase 3.3-3.6)**: BD-only cvar 7 件追加、BD-only sky/water/day preset 7 件同梱、AY-only Render* cvar 26 件は Cinematic で BD-noop 値固定
4. **UI mount (Phase 3.9)**: Cinematic mode (`MachinimaSidebar=1`) のとき画面右に BD の Machinima Sidebar (panel_machinima 1021 行) を出現させる、`gSideBar->refreshGraphicControls()` 経由で双方向 binding

### 設定: 通常運用での操作

**通常運用は既定値のまま (mode 1 = AYAstorm View)**。Cinematic に切替えたい場合のみ:

| Cvar | 既定値 | 用途 |
|---|---|---|
| `AYAVisualRealismEnabled` | `1` | `0`=Firestorm View / `1`=AYAstorm View / `2`=Cinematic、**再起動必須** |
| `MachinimaSidebar` | `1` | Cinematic mode 時の BD Machinima Sidebar 表示。`0` で sidebar 非表示 (描画 pipeline は BD のまま) |
| `RenderShadowAutomaticDistance` | `1` | BD-only 自動 shadow distance 計算 (mode 2 で活性) |

### 移行ノート

- **AYAstorm View (mode 1) は r29 までの描画と同等**、何もせず r30 にアップグレードしても見た目は変わりません。
- Cinematic mode (mode 2) の動作確認は Phase 4 受入 spec ([`docs/specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md)) の G1-G5 に従ってください。
- mode 切替は **再起動を挟む** ことを推奨。round-trip 切替 (cvar だけ変えて再起動なし) は β feedback で安定性を確認するまで使用注意。
- Cinematic mode 内の Machinima Sidebar slider 操作は AY 拡張 cvar と一部 control 名が衝突する可能性があり、AY 拡張の精密調整は mode 1 に戻して行ってください。
- BD `panel_preferences_render_settings` / `panel_preferences_ui_colors` は **配置のみ orphan** とし、AY preferences タブ並びは変更しません ([Phase 3.9 §3.3](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md) 判断、AY 17 タブ既存 UX 優先)。

### 既知の留意点

- mode 2 Cinematic + AY 拡張 floater (`floater_aya_cinematic.xml`) は共存可能ですが、画面右の bdsidebar と UI 競合する場合があります — どちらかに寄せて運用してください。
- BD `llfloatereditsky` / `llfloatereditwater` は BD 上流自身が register していない orphan ファイルで、AY 側でも register せず orphan のまま移植しました (1:1 完全移植原則)。
- cinematic_bd/ 配下 shader は GPU class 別 fallback (class3→class2→class1) を維持。下位 class GPU でも自動的に下位 fallback を辿ります。

### 謝辞

BlackDragon Viewer (NiranV Dean) の描画 pipeline / panel_machinima UI / shader 群を移植元として全面参照しました。bdfunctions / bdsidebar の `Copyright (C) 2018, NiranV Dean` header はそのまま保持しています。

---
