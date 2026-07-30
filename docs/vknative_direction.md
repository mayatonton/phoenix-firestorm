# AYAstorm VK-native — 方向(governance・2026-07-30 全面改訂)

これは統治文書。旧 `vknative_recovery_plan.md` と worker/並列化前提の設計 Doc 群(draw_redesign / serialization_phase_b / *_offmain_* / ui_scene_decouple / rigged_skin_palette_redesign 等)は **2026-07-30 に全削除**した(方向が誤りで山積・行の無駄)。必要なら git 履歴から復元し作り直す。

## 1. 確定した事実(実測・この会話で確定)
- **ガン = CPU の per-draw 記録コスト**。VK なのに GL 比 約 6 倍遅い。lockstep でも draw 数でも GPU 待ちでもない。
- 実測(Safe Hub・vsync OFF/IMMEDIATE・RTX5090):fps≈23.7 / frame≈42ms / **cpu main=95%** / draws/f≈18,304。
  - main の GPU 待ち = **fence 0.4ms・acquire 1.0ms・PE slot 0.0ms** = ほぼゼロ → **CPU⊥GPU の ロックステップは起きていない**。
  - 記録パス `mlp beg≈15.8ms/f` が最大単一項 = **main が 1.8 万 draw を直列記録している事そのもの**が律速。
  - MDI/dedup は効いている(mdi call≈20万・bind の大半 skip)。static は畳み済。
- **残る動的 draw(alphaPost/mat 等 ≈13k scene draw)は per-draw 依存(material/skin/alpha 順序)で畳めない**。

## 2. 並列化 = category error(撤退確定)
- フレームは因果順序が強固な不可分の直列鎖:`cull消費 → 幾何構築 → fill → transform/skeleton resolve → 記録 → submit`。
- off-main worker 群(`recordWorkerCount()>0` gate = geo-fill / avatar-domain build+motion / record pool の camera 使用 / texture off-main)は **この鎖を内部で割った** = 共有・進化中の状態(draw map / VB / transform / spatial group)を worker と main が順序を破って並行に触る。
- 症状 = **avatar + SimRez オブジェクト + 影 が全て「rig がついたように宙を泳ぐ」大崩壊**(特定 skin/palette でなく広域 geometry/transform 破損)。bisect: `AYASTORM_MT_GEO=0`(=off-main 装置無効)→ **完全 clean**。`MT_THREADS=1` → clean。
- **正当な並列は「順序独立を証明できた仕事」のみ = shadow の複数カスケード(独立視錐台)**。それ以外は撤退。
- 例外的に残す off-main = **PE submit/present スレッド**(VK の単一 submitter・鎖の分割ではない・load-bearing)/ **present-wait sleep**(vsync 空回り防止)/ **aux window present**(multiwindow・最小限)。

## 3. 方針(順序)
1. **分散処理を main に戻す**(直列土台を正しい設計として据える)。worker を段階的に物理削除(残すのは §2 の例外のみ)。kill switch で誤魔化さず既定 = 直列。
2. **per-draw 記録を VK 本来の安さにする**(直列のまま)。「1 draw 記録で何をやっているか」を追い、GL 時代の重い per-draw 処理を bindless/indirect で削る。参照 = `percall_set_authority_map.md` / `vknative_draw_structure_map.md`。
3. per-draw 依存を崩せる族だけ畳む(rigged=bindless index に可能性 / alpha=物理的に順序必須で頭打ち)。

## 4. doctrine(不変)
- **並列化の前に分解可能性を検証**(不可分な鎖を割るな)。
- **静的トレースで safety を証明するな・観測可能にせよ**(この会話でも静的判断が 2 度外れ、実測が方向を正した)。
- **品質を下げて速くするのは最適化ではない**(GL 同品質 6 倍差が基準)。
- 憲法(PASS は AYA gate のみ・default-deny・幸運ログ禁止)は不変。
- 実装の有無・正しさは **HEAD のコードを file:line** で判定(Doc/memory の claim は根拠にならない)。

## 4.5 並列化の試みは無駄ではない(残す資産)
worker 化を目指した過程で、**GL 旧来の per-draw 処理が大きく削られ VK-native 構造に置き換わった**。撤退するのは off-main **スレッド化**だけで、以下の VK-native 資産は**残して直列で使う**:
- draw-info snapshot(`LLDrawInfoSnapshot`= 登録の deep-copy・applyGeoStaged で直列使用)/ DrawData / MDI bucket(`llvkbucket`)/ bindless scaffolding(skin palette ring・skinBindlessStorePalette・writeDrawSkinBase)/ mega-buffer / dedup(pipe/desc/push の skip)。
  - ⚠️ **geo-fill snapshot(`LLGeoFaceSnapshot`/`buildVkGeoFill`/`runVkGeoFill`= live face の deep-copy)は worker 入力専用**で直列経路は使わない(直列は旧 `getGeometryVolume` で inline fill)= トレースで到達不能を確認し撤去済(S3)。ここで残すのは draw-info snapshot の方。
- ②per-draw 記録の軽量化は、この既存 bindless/indirect 基盤の上に乗る(ゼロからではない)。

## 5. 北極星
50-100 体 crowd で viewer 自身が固まらない。達成手段 = 並列化でなく **per-draw 記録の軽量化**(§3)。
