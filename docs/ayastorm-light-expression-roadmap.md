# AYAstorm 光表現 長期ロードマップ (r14 →)

**作成日**: 2026-05-12
**対象**: AYAstorm の光/視覚表現拡張シリーズ
**位置づけ**: r13 出荷で完結した 3D Stream / 音響表現章 (`docs/ayastorm-stream3d-roadmap.md`) の後継。光・大気・露出といった視覚表現面で「SL のヘッドホン視聴+目視体験」を一段引き上げるための中長期計画

---

## 1. ねらい

SL viewer の「太陽を見ても眩しくない」「暗所から屋外に出ても目が眩まない」「強い光源が画として弱い」といった、AAA タイトルでは当たり前に存在する **光の体感表現** が SL ではほぼ未実装、という現状を段階的に埋める。

音響表現章 (r7→r13) と同じ思想 — **各リリースは独立に完結し、個別にユーザ価値を届けられる構成**、**配布負債ゼロ (viewer 内 shader / post-process 完結、新 binary 不要)**、**設定は最小限・無難なデフォルト優先** — を継続する。

---

## 2. アーキテクチャの 3 層モデル

設計の核心は次の **直交した 3 層構造**:

| Layer | 何を解く問題か | 担当リリース候補 |
|---|---|---|
| **Layer A: 光源** | どれだけ強い光を出すか? (sun disc / projector / emissive prim) | r14 (sun disc overbright) |
| **Layer B: 大気** | 光が空間でどう散る・届くか? (volumetric scattering / godrays / 霧) | r17+ (volumetric godrays、未確定) |
| **Layer C: カメラ/目** | カメラ/目に届いてからどう感じさせるか? (bloom / lens flare / 露出適応 / tonemap) | r14 (bloom 強化) / r15 (lens flare) / r16 (auto-exposure) |

各層は **競合せず、掛け算で効果**が出る。実写映像でも Layer A (太陽輝度) × Layer C (カメラ露出/ゴースト/フレア) が組合さって「太陽眩しい」が成立する。Layer B (大気散乱) は更にその上に乗る "高級" 表現。

```
┌──────── Layer A: SOURCE (光源) ────────┐
│ ・sun disc / sky                        │
│ ・projector / point light               │ ← r14 (sun disc overbright)
│ ・emissive prim                         │
└─────────────────────────────────────────┘
                  ▼
┌──────── Layer B: MEDIUM (大気) ─────────┐
│ ・atmospheric scattering (既存 SL)      │
│ ・volumetric godrays / raymarch         │ ← r17+ 候補 (重い)
│ ・霧 / 雲 / 体積散乱                    │
└─────────────────────────────────────────┘
                  ▼
┌──────── Layer C: CAMERA/EYE (受け側) ───┐
│ ・bloom / glow buffer                   │ ← r14
│ ・lens flare (ghost chain)              │ ← r15
│ ・auto-exposure (eye adaptation)        │ ← r16
│ ・tonemapping (既存 PBR/ACES 系)        │
└─────────────────────────────────────────┘
```

---

## 3. 各リリースの概要

### r14: 太陽眩しさ (sun disc overbright + bloom 強化)

- ブランチ: 未起票 (spec 起票済、実装着手時に `feature/aya-r14-sun-dazzle` 相当を切る予定)
- スコープ: **Layer A の sun disc 輝度** と **Layer C の glow / bloom kernel** を組合せ、視線を太陽に向けた瞬間に「白く滲んで眩しい」体感を成立させる
- 主要変更: 太陽 billboard の emissive を HDR 域 (10x〜) に boost、glow buffer kernel パラメータ調整 (半径 / 強度)
- 仕様詳細: `docs/ayastorm-r14-sun-dazzle.md` (canonical、フェーズ P0〜P3 / 受入条件 / リスク L1〜L5 を保持)
- 工数感: **0.5〜1 日 (3 OS 込み)**。既存 glow buffer / sun rendering の流用が中心、新 pass 追加なし
- ROI: 単発で「太陽を見ると眩しい」体験の体感 6 割を獲得 (lens flare / 露出適応の前段としても活きる)

### r15: lens flare (候補、未確定)

- スコープ: 太陽スクリーン位置を起点に sprite チェーン (ghost / halo) を加算合成。視線を太陽に向けた時の「差し」を演出
- 主要変更: post-process 段に lens flare pass 追加、sun の depth visibility (occlusion query) で flare 強度をモジュレート
- 参考実装: Black Dragon 等
- 工数感: **1〜2 日 (3 OS 込み)**
- ROI: r14 単独より明確に "華やか"。配信者の屋外撮影需要に効く

### r16: auto-exposure (eye adaptation、候補、未確定)

- スコープ: HDR フレーム平均輝度を時間平滑して tonemap exposure に反映、暗所→屋外で「一瞬白飛び」する眼の適応反応を再現
- 主要変更: HDR 輝度 reduction (mipmap chain or compute)、時間平滑 (eye adaptation speed)、tonemap shader 接続
- 工数感: **4〜6 日 (3 OS 込み)**。コンテンツ側の見え方が変わるため tuning が沼になりやすい
- ROI: 化けるが工数大。r14/r15 で土台を作ってから

### r17+: 未確定枠

- volumetric godrays (raymarch、Layer B 本格化)
- 月夜 / 夜景表現の底上げ (Layer A 暗側)
- emissive prim の HDR ハンドリング統一
- 水面 / 反射の改善
- 影品質
- ...

---

## 4. 工数感

`project_release_workload_norms.md` の物差し (r8 実績 2 日、r9 見積 3〜4 日) で。

| リリース | 工数感 (3 OS 込み) | 状態 |
|---|---|---|
| r14 sun dazzle | 0.5〜1 日 | spec 起票済 (`docs/ayastorm-r14-sun-dazzle.md`)、実装着手前 |
| r15 lens flare | 1〜2 日 | 候補 |
| r16 auto-exposure | 4〜6 日 | 候補 |
| r14+r15+r16 フル盛り | 6〜9 日 | 参考値 (r9 級) |

ROI が最も高いのは **r14 単発 (0.5〜1 日で太陽眩しい体感 6 割)** で、ここから始めて感触で続きを判断する方針。

---

## 5. 検討中で本ロードマップに入っていない事項

- **HDR / wide color gamut パイプライン全体見直し**: 個別機能の積み上げで足りなくなったら検討
- **dynamic exposure UI**: auto-exposure 速度 / 範囲のユーザ設定。`feedback_prefer_defaults_over_config.md` に従い極力 default で固める
- **動画 / 写真撮影向け露出ロック**: 撮影 viewer ニーズが顕在化したら別 release
- **ML 系 denoise / upscale**: 配布負債大、検討対象外

---

## 6. 更新履歴

- 2026-05-12: 初版作成。r13 で完結した 3D Stream / 音響表現章 (`docs/ayastorm-stream3d-roadmap.md`) の後継として光/視覚表現章を開始。3 層モデル (Layer A 光源 / Layer B 大気 / Layer C カメラ/目) を策定、r14 (sun dazzle) を最初の弾として、r15 (lens flare) / r16 (auto-exposure) を候補に置く。軸転換の背景は memory `project_ayastorm_r14_pivot_to_light.md` 参照
