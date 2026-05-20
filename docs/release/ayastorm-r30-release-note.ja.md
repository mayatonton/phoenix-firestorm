# AYAstorm r30 — リリース告知

**r30 は撮影描画章 (r30+) の第 1 弾**で、AYAstorm に **Cinematic mode** を新設し、新しい描画エンジン 995a1354d8 を **1:1 完全移植** して並走できるようにします。AYAstorm View (r14-r20 拡張) と新しい描画エンジンの写真寄り視覚表現を、**1 個の cvar (`AYAVisualRealismEnabled`) で再起動切替** できます。

> **配信形態**: r30 は単独タグで発行予定 (β release から開始)。
>
> **再起動切替モデル**: mode 切替は再起動必須 (Phase 4 G4 で round-trip 動作は推奨確認、安定性確証は β feedback 後)。

実装・移植戦略・shader mount 表・受入基準は永続資料 (docs/specs/ 配下) に常駐し、本ノートはそこへの誘導と差分ハイライトに徹します。

| 資料 | 内容 |
|---|---|
| [`ayastorm-r30-bd-full-port-inventory.md`](../specs/ayastorm-r30-bd-full-port-inventory.md) | Phase 0 inventory (新しい描画エンジンと AY の差分棚卸し、shader 49 + cpp 92 + cvar 拡張) |
| [`ayastorm-r30-bd-full-port-phase1-audit.md`](../specs/ayastorm-r30-bd-full-port-phase1-audit.md) | Phase 1 audit (REDO/REUSE 分類 + r14-r20 gating 棚卸し) |
| [`ayastorm-r30-bd-full-port-phase2-spec.md`](../specs/ayastorm-r30-bd-full-port-phase2-spec.md) | Phase 2 architectural (D1-D4 dispatch 構造) |
| [`ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.2-cpp-dispatch-spec.md) | Phase 3.2 C++ Cinematic dispatch (53 file) |
| [`ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.5-ay-only-render-cvar-spec.md) | Phase 3.5 AY-only Render* cvar 26 件 dispatch |
| [`ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.8-shader-cinematic-mount-spec.md) | Phase 3.8 shader 49 file A/B/C/D mount |
| [`ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md`](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md) | Phase 3.9 UI mount (新しい描画エンジン env floater + Cinematic Controls 統合、sidebar 路線は §0 で撤去記録) |
| [`ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md) | Phase 4 3 mode 受入検証 (G1-G5) |
| [`ayastorm-r30-bd-full-port-phase5-cleanup-spec.md`](../specs/ayastorm-r30-bd-full-port-phase5-cleanup-spec.md) | Phase 5 cleanup / release prep |

---

## AYAstorm r30 — 新しい描画エンジン完全移植 + 3 mode 統合

### r30 の柱: 1 viewer / 3 mode で「Firestorm 互換」「AYAstorm 視覚拡張」「新しい描画エンジン 写真撮影」を切替

AYAstorm はこれまで r14-r20 で独自の視覚的リアリティ拡張 (atmospheric perspective / cloud volumetric / sun Kelvin modulator / SSS skin marker / translucency / chromatic aberration / SMAA T2x / volumetric godrays / depth-of-field 強化 等) を積み上げてきました。一方で写真撮影に特化した描画エンジンが別系統で進化を続けており、AYAstorm 利用者からも「その撮影体験そのものを AYAstorm から呼べないか」という需要がありました。

r30 は **新しい描画エンジンを完全に 1:1 移植** し、AYAstorm 内に **3 モード** を統合します:

| `AYAVisualRealismEnabled` | mode 名 | 描画 pipeline | 用途 |
|---|---|---|---|
| `0` | Firestorm View | Linden / Firestorm baseline (AY 拡張 OFF) | 配信視聴 / 業務 / リソース節約 |
| `1` | AYAstorm View | AY r14-r20 視覚的リアリティ拡張 | r29 以前の AYAstorm 体験 |
| `2` | Cinematic (既定) | 新しい描画エンジン 995a1354d8 1:1 移植 | 写真撮影 / machinima |

### 仕組み: 3 モードを並走させる 4 層 dispatch

1. **C++ dispatch (Phase 3.7)**: pipeline.cpp / lldrawpool* / llviewershadermgr 等 53 file が `AYAVisualRealismEnabled` を読み、mode 別に shader bind / state setting / render order を切替
2. **shader permutation (Phase 3.8)**: 49 REDO shader を 4 strategy で mount
   - **A**: uniform 給餌で吸収 (5 file、shader 触らず neutral 値で collapse)
   - **B**: overwrite (9 file、新しい描画エンジン baseline で上書き、AY 拡張なし)
   - **C**: `#if AYASTORM_CINEMATIC` permutation (26 file、AY と新しい描画エンジンを同 file 共存)
   - **D**: dual-file mount (2 file、`cinematic_bd/` 配下に新しい描画エンジン側別配置 + shadermgr 探索切替)
3. **cvar 拡張 (Phase 3.3-3.6)**: 新しい描画エンジン専用 cvar 7 件追加、専用 sky/water/day preset 7 件同梱、AY-only Render* cvar 26 件は Cinematic で新しい描画エンジン-noop 値固定
4. **UI mount (Phase 3.9)**: Cinematic mode 用の操作は **`Avatar → Cinematic Controls...` (`Alt+C`) の `floater_aya_cinematic.xml` に統合**。当初は新しい描画エンジン側の `panel_machinima.xml` (1021 行) + 専用 sidebar を mount する設計でしたが、AYAstorm の floater 体系と整合させるため Cinematic Controls floater に統合し、sidebar 路線は撤去しました ([Phase 3.9 spec §0](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md))

### 設定: 通常運用での操作

**通常運用は既定値のまま (mode 2 = Cinematic)**。AYAstorm View / Firestorm View に戻したい場合のみ:

| Cvar | 既定値 | 用途 |
|---|---|---|
| `AYAVisualRealismEnabled` | `2` | `0`=Firestorm View / `1`=AYAstorm View / `2`=Cinematic、**再起動必須** |
| `RenderShadowAutomaticDistance` | `1` | 新しい描画エンジン専用 自動 shadow distance 計算 (mode 2 で活性) |

Cinematic mode の各種パラメータ調整は `Avatar → Cinematic Controls...` (`Alt+C`) で開く Cinematic Controls floater から行います。

### 移行ノート

- **r30 既定は Cinematic (mode 2) に変更**。r29 までの描画 (= AYAstorm View) を維持したい場合は Debug Settings で `AYAVisualRealismEnabled` を `1` に戻して再起動してください。
- Cinematic mode (mode 2) の動作確認は Phase 4 受入 spec ([`docs/specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md`](../specs/ayastorm-r30-bd-full-port-phase4-verify-spec.md)) の G1-G5 に従ってください。
- mode 切替は **再起動を挟む** ことを推奨。round-trip 切替 (cvar だけ変えて再起動なし) は β feedback で安定性を確認するまで使用注意。
- Cinematic mode 内の Machinima Sidebar slider 操作は AY 拡張 cvar と一部 control 名が衝突する可能性があり、AY 拡張の精密調整は mode 1 に戻して行ってください。
- 新しい描画エンジン側の `panel_preferences_render_settings` / `panel_preferences_ui_colors` は **配置のみ orphan** とし、AY preferences タブ並びは変更しません ([Phase 3.9 §3.3](../specs/ayastorm-r30-bd-full-port-phase3.9-ui-cinematic-mount-spec.md) 判断、AY 17 タブ既存 UX 優先)。

### 既知の留意点

- `llfloatereditsky` / `llfloatereditwater` は新しい描画エンジン上流自身が register していない orphan ファイルで、AY 側でも register せず orphan のまま移植しました (1:1 完全移植原則)。
- cinematic_bd/ 配下 shader は GPU class 別 fallback (class3→class2→class1) を維持。下位 class GPU でも自動的に下位 fallback を辿ります。
- Cinematic mode では classic / system avatar body (素体・Ruth/Roth・古い system 服の素体部分) は **motion blur 対象外** です。新しい描画エンジン baseline (995a1354d8 で `LLDrawPoolAvatar::renderMotionBlur` 全体が `/* ... */` で commented out) と整合させた挙動で、現代の rigged mesh アバター (手・髪・服を含む大半の attachments) は他 pool 経由で blur 対象として残ります。

### 謝辞

外部の写真撮影向け描画エンジン実装 (NiranV Dean 氏) を移植元として全面参照しました。`bdfunctions` の `Copyright (C) 2018, NiranV Dean` header はそのまま保持しています。

---
