# r40 sub-phase 3: Vulkan 化選択 + 工程プラン策定 (active 2026-05-28〜)

**status**: active (work item (a) 全完了 2026-05-28 / work item (b) Vulkan API 設計 **draft 全 10 section 完成** (group B + C 追加で finalize、AYA review 待ち))
**親 charter**: `00-charter.md`
**位置付け**: r40 章の sub-phase 3 (Vulkan 化選択を正式採用、工程プラン策定中)
**前章**: sub-phase 2 (`02-sub-phase-2-extended-falsify.md`)
**達成条件**: 本 doc 内の work item (a)-(e) 全完了 = r40 達成 → r41 着手

---

## 1. sub-phase thesis

「**sub-phase 1 (CPU perf 全 REJECT) + sub-phase 2 (鉱脈ゼロで延命前提崩壊) の falsification を受けて、Vulkan 化を正式に選択。完遂 goal (parity 完遂) + time horizon (無期限) + 本線同居 → r41.5 で VK repo 分離 という基本設計のもと、Vulkan 化作業の工程プラン (portage 棚卸し + API 設計 + 工程算定 + r42+ 区切り確定) を策定する。**」

策定完了 = r40 達成、r41 (GL 除去 + Vulkan 空転) 着手へ。

## 2. work item 一覧

charter §9 で定義した (a)-(e) work item の status tracker:

| ID | work item | 出力 doc | 順序依存 | status |
|---|---|---|---|---|
| (a) | Vulkan portage 棚卸し phase | `04-portage-inventory.md` | (なし、最初) | **完了 (2026-05-28)** |
| (b) | Vulkan API 設計 | `05-vulkan-api-design.md` | (a) 完了後 | **draft 全 10 section 完成** (foundation + group A + group B + C 全部、AYA review 待ち) |
| (c) | 工程算定 | `06-effort-estimation.md` | (a)(b) 完了後 | 未着手 |
| (d) | r42+ 区切り確定 | `00-charter.md` §6 更新 | (a)(b)(c) 完了後 | 未着手 |
| (e) | charter 完成 → r40 達成 | 全 doc final review | (a)(b)(c)(d) 完了後 | 未着手 |

work item は **逐次進行** (並列不可、(a) → (b) → (c) → (d) → (e) 順序依存)。

## 3. work item (a): Vulkan portage 棚卸し phase

### 目的

現 OpenGL コード base (phoenix-firestorm 系統 + AYAstorm 追加分) の Vulkan port 影響範囲を分類し、要 port / 要再設計 / 不要 port の判断と LOC 算定を出力する。

### 棚卸し対象

| 範囲 | 想定 LOC | 棚卸し方針 |
|---|---|---|
| `indra/llrender/` | 28.2K LOC / 16 files | 全 file の GL call を Vulkan 等価実装 (vkCreate* / vkCmd* / vk* 等) に置換、interface 単位で分類 |
| GL header include | 212 files | include 文書き換え範囲、Vulkan header 移行 |
| GLSL shader | 248 file | SPIR-V 移行範囲、descriptor set 設計影響 |
| pipeline.cpp | (要計測) | 3 大グローバル + cull/stateSort 内 GL 呼出 + geometry mutation の再設計範囲 |
| その他 | (要計測) | LLDrawPool* / LLViewerShaderMgr / LLViewerTexture / VBO 関連 / FBO 関連 / 等 |

### 分類軸

各 file / function を以下に分類:

- **要 port** (Vulkan 等価実装で置換): redundant な GL call で済むもの
- **要再設計** (構造変更必須): pipeline.cpp 3 大グローバル / cull/stateSort GL 呼出 / geometry mutation 等
- **不要 port** (削除 or skip): OpenGL 固有 API で Vulkan に対応物のない / 使われていない
- **AYAstorm 固有機能の影響範囲**: r1-r30 機能の各 file が描画 stage に依存するか / 並行 port 可能か

### 出力 doc 構成 (`04-portage-inventory.md`)

1. 棚卸し対象 file list + LOC
2. 各 file の分類 verdict (4 軸)
3. 要 port file の Vulkan 等価実装方針
4. 要再設計 file の構造変更案
5. AYAstorm 固有機能の描画 stage 依存度マップ
6. 棚卸し総括 (要 port LOC 合計 / 要再設計 LOC 合計 / 不要 port LOC 合計)

### 進め方

- Claude が file 棚卸し → 分類 verdict draft 作成
- AYA さん review → 修正指示
- 確定 verdict を 04-portage-inventory.md に記録
- 棚卸し完了で (b) Vulkan API 設計へ

## 4. work item (b): Vulkan API 設計

### 目的

Vulkan 化に必要な API 選定 / 設計を行う。棚卸し (a) 出力の要 port file が「どの Vulkan API でどう実装されるか」の方針を決める。

### 設計項目

- **Vulkan version**: vulkan-1.3 を default 想定 (Linux/Win は 1.3 widely available、Mac は MoltenVK 経由で 1.2 相当)
- **loader**: volk 推奨 (header-only / extension load 自動 / 既存 viewer 系で実績)
- **SDK**: Vulkan SDK 1.3.x (Linux: distro package / Win: LunarG / Mac: MoltenVK SDK)
- **shader cross compile chain**: glslang (GLSL → SPIR-V) + spirv-cross (SPIR-V → MSL/HLSL for Mac/Win 補助)
- **descriptor set / pipeline layout**: descriptor set 2-3 個 (per-frame / per-material / per-draw) で運用、Vulkan 1.3 push descriptor 採用可能性
- **render pass / framebuffer**: deferred g-buffer の Vulkan 表現 (subpass で attachment 共有 or 明示 image transition)、Vulkan 1.3 dynamic rendering (render pass less) 採用可能性
- **sync 戦略**: frame in flight = 2-3 個、fence + semaphore + image memory barrier の使い分け
- **memory allocator**: VMA (Vulkan Memory Allocator, AMD) 採用推奨 (実績豊富 / open-source / header-only style)
- **swapchain / present mode**: FIFO default、mailbox optional (VSync OFF user 向け)、immediate は採用しない
- **3 OS 対応**:
  - Linux: Vulkan native (Mesa or proprietary driver)
  - Win: Vulkan native (NV / AMD / Intel driver)
  - Mac: MoltenVK 経由 (Vulkan → Metal 変換、MoltenVK は Vulkan 1.2 + 一部 1.3 extension 対応)

### 出力 doc 構成 (`05-vulkan-api-design.md`)

1. Vulkan version + loader + SDK 選定根拠
2. shader cross compile chain (例コード含む)
3. descriptor set / pipeline layout 設計図
4. render pass / framebuffer (deferred g-buffer の Vulkan 表現)
5. sync 戦略 (state diagram / barrier table)
6. memory allocator 方針
7. swapchain / present mode
8. 3 OS 対応詳細 (Mac MoltenVK 制約含む)
9. extension 採用 list (vulkan-1.3 base + 必須 extension)
10. abstraction interface 設計 (r41.5 で本線 ↔ VK repo 分離するときの interface skeleton、ただし詳細は r41.5 charter で詰める)

### 進め方

- Claude が設計 draft 作成 (棚卸し (a) 出力に基づく)
- AYA さん review → 採否判定
- 確定方針を 05-vulkan-api-design.md に記録
- 設計完了で (c) 工程算定へ

## 5. work item (c): 工程算定

### 目的

棚卸し (a) + API 設計 (b) から、Vulkan 化全工程の人年算定を行う。Doom 2016 / Blender Vulkan の参照点を反映し、AYAstorm 体制 (1 人 本職並走) で具体的な月数 / 年数を出す。

### 算定軸

- **per-file 工数**: 棚卸し (a) の要 port / 要再設計 file ごとに工数 (h / file)
- **per-shader 工数**: 248 GLSL shader → SPIR-V 移行の per-shader 工数 (descriptor set 再設計含む)
- **per-milestone 工数**: r41 / r41.5 / r42 / r43 / r44 / r45+ の milestone 単位積算
- **3 OS 工数**: Linux 先行 + Win 追加 + Mac (MoltenVK) 追加の per-OS 増分
- **review / test / bug fix 余裕**: 各 milestone 内 work 工数 + 30-50% 余裕

### 参照点 (charter §4 (3) 再掲)

- Doom 2016 (id Tech 6): 3 名 6-12 か月 (clean abstraction あり)
- Blender Vulkan: 2019 着手 → 2026 現在 7 年未完 (近似 scale)
- AYAstorm 規模: 6-15 人年 (abstraction 不在 + 3 大グローバル + 248 shader)
- 1 人 full-time 換算で物理 6-15 年、本職並走なら 15-30 年

### 出力 doc 構成 (`06-effort-estimation.md`)

1. per-file 工数算定 (棚卸し (a) 出力に基づく)
2. per-shader 工数算定
3. per-milestone 工数積算 (r41 / r41.5 / r42 / r43 / r44 / r45+)
4. 3 OS per-OS 増分
5. 各 milestone の所要月数 / 年数 (本職並走前提)
6. 算定の uncertainty band (上方 / 下方)
7. Doom / Blender 参照点との比較
8. plan B trigger 条件 (charter §8 (B) 工程プラン破綻判定の閾値設定)

### 進め方

- Claude が算定 draft 作成
- AYA さん review → 修正指示 (体感の本職並走可能性 / 余裕の取り方 等)
- 確定算定を 06-effort-estimation.md に記録
- 算定完了で (d) r42+ 区切り確定へ

## 6. work item (d): r42+ 区切り確定

### 目的

棚卸し (a) + API 設計 (b) + 工程算定 (c) から、charter §6 仮 line up を正式 line up に確定する。

### 確定項目

- r42 / r43 / r44 / r45+ の正式区切り
- 各 milestone の描画 stage 範囲 (vk-β / vk-γ / vk-δ / vk-RC との対応)
- 各 milestone の AYAstorm 機能 pull-in 対象 (r1-r30 のどれをどの milestone で port するか)
- 各 milestone の 3 OS 対応 timing (Linux first / Win 追加 / Mac 追加)

### 出力

charter §6 仮 line up 表の **本 line up 更新**。本 doc (03-sub-phase-3-vulkan-plan.md) §6 に確定 line up を記録、charter §6 表は本 line up に置換。

### 進め方

- Claude が仮 line up を本 line up に格上げ draft 作成 (棚卸し + API + 算定の出力統合)
- AYA さん review → 修正指示
- 確定 line up を charter §6 に反映
- 確定で (e) charter 完成へ

## 7. work item (e): charter 完成 → r40 達成

### 目的

(a)-(d) work item の全出力を統合し、r40 章 charter (00-charter.md) を final 化、AYA さん承認で r40 達成宣言。

### final 化対象

- 00-charter.md (全 section の整合確認 + 各 section 内の TBD / 仮 値を本 値に置換)
- 03-sub-phase-3-vulkan-plan.md (本 doc) の work item status を全 (完了) に更新
- 04-portage-inventory.md / 05-vulkan-api-design.md / 06-effort-estimation.md の final 化
- memory `project_ayastorm_r40_cpu_parallel.md` を r40 達成 status に更新

### r40 達成宣言の条件

- 上記 doc 群が AYA さん review で全 PASS
- AYA さんが明示的に「r40 達成」と宣言
- → r41 charter (`docs/specs/ayastorm-r41-gl-removal/00-charter.md`) 起草へ移行

## 8. 進行 workflow (AYA-Claude 共同)

### 基本サイクル

各 work item の進め方:

1. **Claude が draft 作成** — 既存 memory + repo 棚卸し + 設計検討から draft doc 作成
2. **AYA review 待ち** — Claude は AYA review を待つ、別 work item には進まない (順序依存)
3. **AYA review** — AYA さんが draft 確認、修正指示
4. **修正反映** — Claude が修正、確定版に
5. **確定 commit** — 確定版を git commit、次 work item へ

### context 圧迫時の handoff

各 work item 完了時 (= AYA 確定後 commit 後) は handoff 区切りとして適切。context 圧迫が見えてきたら work item 境界で handoff-*.md 作成、次 session へ託す (memory `feedback_proactive_handoff.md` 参照)。

handoff doc 構成:
- 完了済 work item と出力 doc
- 次 work item の status + 着手前提
- 中間状態の特記事項

### 並列 work item の禁止

(a)-(d) は順序依存があるので **並列進行禁止**。Claude が (a) draft 作成中に (b) を始めない (前提 input が確定していない状態で work すると drift)。

## 9. AYA review 待ちの tracking

各 work item の status は本 doc §2 の表で tracking:
- 未着手 / draft 作成中 / AYA review 待ち / 修正中 / 確定

Claude は AYA review 待ち中は別 work item に進まない、AYA review 完了 + 修正反映 + 確定後に次へ進む。

## 10. 関連 doc / memory

### r40 章内部 doc

- `00-charter.md` — r40 章 charter (本 sub-phase 3 で final 化)
- `01-sub-phase-1-cpu-perf.md` — sub-phase 1 詳細
- `02-sub-phase-2-extended-falsify.md` — sub-phase 2 詳細
- `04-portage-inventory.md` — work item (a) 出力 (起草予定)
- `05-vulkan-api-design.md` — work item (b) 出力 (起草予定)
- `06-effort-estimation.md` — work item (c) 出力 (起草予定)

### r40 章外部 doc

- `docs/specs/ayastorm-render-perf-survey.md` — sub-phase 1 で参照した perf 棚卸し doc

### memory

- `project_ayastorm_r40_cpu_parallel.md` — r40 章 active memory
- `project_ayastorm_r40_extended.md` — sub-phase 2 詳細 memory
- `project_ayastorm_r41_vulkan_migration.md` — r41 milestone (GL 除去 + Vulkan 空転)、r40 達成後に着手
- `project_ayastorm_three_platforms.md` — 3 OS 大前提 (charter §4 (2) で Linux 先行 = 明示指示例外、3 OS 完遂は維持)
- `feedback_proactive_handoff.md` — work item 境界での handoff 方針
- `feedback_falsification_as_progress.md` — sub-phase 1, 2 の falsification が sub-phase 3 への絞り込み成果
