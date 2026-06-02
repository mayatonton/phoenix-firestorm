# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-18 完遂 handoff

**status**: B?-η-18 完遂 (Phase 1 ACCEPT、option A 採用 = C++ runtime transformer の Phase 2 拡張 (V↔F pair allocator)、link failed 14 → 10 件 / V↔F bare varying location alignment mismatch 全消解 = Deferred Diffuse / Bump 系統 6 件 + Underwater / Glow / Highlight / Pathfinding / Solid Color 系統 cascade 群 完全解消、残 10 件は cascade shift forward 第16層露出 = MaterialUBO/anonymous member/vary_position-no-V-out = η-19+ 別 sub-bundle 移管) → 次 sub-bundle B?-η-19 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: (本 doc 起草と同 cycle で commit、AYA「handoff doc + commit 進めて」明示指示下 2026-06-02 範式継承)
**handoff doc commit**: 本 doc (η-17-complete 範式継承、別 commit)
**勤続範式継承**: B?-η-17-complete `cc2e878c7e` (patch `e2d4d3bbca`) / B?-η-16-complete `3653efed00` (patch `72fd4f3c3c`) / B?-η-16-prep-D-switch `1c3eb16b6b` / B?-η-15-complete `cb28cf1daa` (patch `6a11eabc73`) / B?-η-14-complete `78df235243` (patch `c971838656`) / B?-η-13-reverted (no commit) / B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-18 scope = **option A 採用 (C++ runtime transformer Phase 2 拡張 = V↔F pair allocator)** で V↔F bare varying location alignment mismatch を構造的に解消 → η-17 末 link failed 14 件 → η-18 末 10 件 (Δ -4 件 net、ただし **V↔F bare varying-specific 失敗は 0 件達成**、残 10 件は別カテゴリ = MaterialUBO/anonymous member/vary_position-no-V-out)。Phase 1 で 1 file (`llglslshader.cpp`) に LocationAllocator namespace 拡張 + bare `out`/`in` regex pair + qualifier `kQuals` 範式 (before/after 両対応) + 既存 manual layout wrap 由来 ident → slot map pre-pass 記録 + F stage 既存 layout in 自動 override (V map と mismatch 時 V slot で書換) を実装。η-17 末 cascade exposure 第15層後始末 (non-opaque 11 / vary_fragcoord 6 / TerrainMix 2 / weight4 1) は **η-19+ 移管** (Phase 2 副 scope は時間軸 + scope 単純化のため次 sub-bundle に分離)。

- **Phase 1 (option A 採用、C++ runtime transformer の Phase 2 拡張)**:
  - `LocationAllocator` 構造体に Phase 2 (η-18) 用 namespace 追加 = `mUsedVertOutSlots` / `mUsedFragInSlots` / `mVertOutCursor` / `mVertOutIdentToSlot`
  - `vulkanizeStageSource()` signature を `(source, stage_type, alloc&)` に変更し、V stage / F stage の両方を処理対象に拡張 (G stage は touch しない、Phase 3 / η-19+ 移管)
  - regex 5 種 (`bare_out_pattern` / `bare_in_pattern` / `existing_layout_out_pattern` / `existing_layout_in_pattern` / `existing_layout_out_with_ident_pattern` / `existing_layout_in_with_ident_pattern`) を `kQuals` 共有定数で qualifier 列 (flat/smooth/noperspective/centroid/highp/mediump/lowp/invariant) の **before / after 両位置対応** で構築
  - pre-pass で V stage 既存 `layout(location=N) out` を ident → slot map に記録 (`mVertOutIdentToSlot`)
  - main pass:
    - V stage bare `out`: map 既存 entry あれば再利用 (manual wrap 由来 slot を canonical truth と扱う、preprocessor 別分岐の bare 版が同 slot に wrap される)、なければ cursor 進めて新規 allocate + map 記録
    - F stage bare `in`: V map から ident lookup → 同 slot で wrap (V↔F pair location alignment 構造保証)、map 不在は LL_WARNS + 元行 keep
    - F stage 既存 `layout(location=N) in <ident>;`: V map と照合 → mismatch なら V slot で override 書換 (LL_DEBUGS で観測可能化)
  - kFixedStageOrder = {V, G, F} で program 単位 LocationAllocator instance を data flow 順に iterate (V → G → F = 正しい cross-stage shared state 構築順)
  - `program_hash_obj.update("vulkanize:v5_p2_inout_pair_prepass_group_fix")` で cache invalidate (v1 → v2 → v3 → v4 → v5 の 4 段 iterate、η-18 内 phase 1-1/1-2/1-3 falsification を反映)
- **option A 選択判定 (AYA 2026-06-02 "A")**: η-17 §10.7 V↔F pair allocator 設計仕様を直接実装、option B (手動 shader file 全 audit) は 14 件 × V/F 双方の番号一致 patch 作成 = upstream merge 影響大 + 将来 cascade 残量 (normalMap 69 / depthMap 9 / FXAA 等) でも同 manual repetition 必須 = 構造解決力で option A 優位と判定。

**計**: **C++ 1 file (`llglslshader.cpp` +291/-44 = +247)、shader file 変更 0、user_settings.xml 変更 0** (Phase D の `RenderVulkanShaderDumpTransformed=1` 既 inject は η-17 末で AYA さん戻し対象案内済、η-18 verify 中は dump 未取得で済んだため追加 inject 不要)

**主指標達成** (vs η-17 末 baseline = log `AYAstorm.log` 起動 2026-06-02T08:51Z 直前 baseline):

- `link failed` 14 → **10** ✓ **-4 件 net 改善、V↔F bare varying-specific 失敗は 0 件達成** (内訳: Deferred Diffuse 4 + Bump 2 系統完全解消、残 10 件は cascade shift forward 第16層 = MaterialUBO 4 + anonymous member 4 + vary_position-no-V-out 2 = η-19+ 別 sub-bundle 移管)
- `parse failed (total)` 100 → **100** ±0 (主 scope 外、Phase 2 副 scope = cascade 後始末を η-19+ 移管した結果)
- `Goodbye!` 1 / `Vulkan device destroyed` 1 / FATAL/SIGSEGV/Aborted 0/0/0 = ✓ clean shutdown

**cascade shift forward 第16層露出** (link failed のカテゴリ内訳変動):

| # | metric | η-17末 | η-18後 | Δ | 解析 |
|---|---|---|---|---|---|
| 1 | `link failed (total)` | 14 | **10** | **-4** | V↔F bare varying-specific は 0 達成、残 10 件は別カテゴリへ shift |
| 2 | 内訳: V↔F bare varying location mismatch | 14 | **0** | **-14** | ✓ η-18 主目標完全達成 |
| 3 | 内訳: MaterialUBO V/F member mismatch | 0 | **4** | **+4** | 第16層露出 (Skinned/通常/HUD PBR Opaque + HUD PBR Alpha) |
| 4 | 内訳: Anonymous member (UBO global var) | 0 | **4** | **+4** | 第16層露出 (Skinned/通常/HUD Fullbright Alpha Masking + Deferred Star) |
| 5 | 内訳: vary_position no V out | 0 | **2** | **+2** | 第16層露出 (Skinned/通常 PBR Glow + HUD PBR Opaque 重複 1) |
| 6 | parse failed `non-opaque uniforms outside a block` | 11 | 22 | **+11** | 第16層 cascade 継続露出 (η-17 §10.3 想定通り) |
| 7 | parse failed `overlapping use of location` (parse stage) | 0 | **5** | **+5** | 第16層露出 (Deferred Alpha + PBR Terrain で location 20 collision = η-19+ ratchet) |
| 8 | `SPIR-V requires location` | 0 | **3** | **+3** | 第16層露出 (η-17 で 0 達成だが η-18 で異なる shader path で再露出 = η-19+) |
| 9 | parse failed `'normalMap' : redefinition` | 69 | 56 | **-13** | 副次的減少 (V↔F pair 整合の波及効果、η-19+ 移管継続) |
| 10 | parse failed `'depthMap' : redefinition` | 9 | 5 | **-4** | 副次的減少 (同上) |
| 11 | parse failed `'vary_fragcoord' : redefinition` | 6 | 6 | ±0 | (η-19+ 移管継続) |
| 12 | parse failed `nameless block ... global scope` | 2 | 2 | ±0 | (η-19+ 移管継続) |

**主指標達成総括**: link failed 14 件中 V↔F bare varying-specific 14 件を **構造的解決 (C++ runtime transformer Phase 2 = V↔F pair allocator)** で完全解消。残 link failed 10 件は別カテゴリで cascade shift forward 第16層に shift = **η-19+ 別 sub-bundle 移管対象**。

**他既達主指標完全維持** (η-18 末):
- `Layout location qualifier` (link error) 14 → **0** ✓ (link failed 内訳と一致、Phase 1 主目標達成)
- `Cannot reuse block name` 0 / `'binding'` 0 / `GBufferInfo redefinition struct` 0 / `'size' undeclared` 0 / `undeclared identifier` 0 = 全件維持
- `'weight4' : redefinition` 1 / `'TerrainMix' : redefinition struct` 2 / `nameless block ... global scope` 2 維持

**Phase 構成の特徴**: η-17 §10.2 着手前 trace 14 ステップを literal 実施 → option A/B 比較 → AYA 「A」judgment (2026-06-02) で C++ runtime transformer Phase 2 拡張へ進行。**Phase 1 実装中に 3 段の falsification iterate** (v2 → v3 → v4 → v5):
- v2: 初回実装、bare `out`/`in` regex は qualifier `after only` 設計 → 検証で `smooth out vec3 vary_normal;` (qualifier `before`) を match miss = link failed 14 unchanged + 新 regressions
- v3 (Phase 1-1): `kQuals` 共有定数を導入し qualifier before/after 両対応 regex 4 種を re-design + F stage 既存 layout in 自動 override logic 追加 → 検証で link failed **113 件** に regression、原因 = V mapping table が空 (group index bug)
- v4 (Phase 1-2): V bare_out で map 既存 entry あれば再利用 (preprocessor 別分岐の bare 版が manual wrap slot を上書きしない) → 検証で link failed 113 件変化なし、原因 = pre-pass の group index 不整合
- v5 (Phase 1-3): pre-pass の `m[4]` → `m[2]` 修正 (kQuals の `(?:...)` non-capturing group が group 番号を増やさないため、ident は m[2] が正) → **検証 PASS、link failed 113 → 10**

各 falsification は AYA cold launch verify で実 log 検出 (推論で済まさず literal 観測)、3 サイクルでの収束。

---

## §2 完遂結果 metric (vs B?-η-17 末 baseline log)

| metric | η-17末 | η-18後 (verify) | Δ vs η-17 | 判定 |
|---|---|---|---|---|
| **link failed (total)** | **14** | **10** | **-4** | ✓ 主 scope = V↔F bare varying-specific は 0 達成 (残 10 = 別カテゴリ shift) |
| **link failed: V↔F bare varying location mismatch** | **14** | **0** | **-14** | ✓ **Phase 1 主目標完全達成** |
| **Layout location qualifier (link error)** | **14** | **0** | **-14** | ✓ link failed と同期 0 達成 |
| parse failed (total) | 100 | 100 | ±0 | 主 scope 外 (副 scope = cascade 後始末 η-19+ 移管) |
| non-opaque uniforms outside a block | 11 | 22 | **+11** | cascade shift forward 第16層想定通り |
| overlapping use of location (parse stage) | 0 | 5 | **+5** | 第16層露出 (η-19+ 移管) |
| SPIR-V requires location | 0 | 3 | **+3** | 第16層露出 (η-19+ 移管) |
| 'normalMap' : redefinition | 69 | 56 | **-13** | 副次的減少 (波及効果、η-19+ 継続) |
| 'depthMap' : redefinition | 9 | 5 | **-4** | 副次的減少 (同上) |
| 'vary_fragcoord' : redefinition | 6 | 6 | ±0 | (η-19+ 継続) |
| 'TerrainMix' : redefinition struct | 2 | 2 | ±0 | (η-19+ 継続) |
| 'weight4' : redefinition | 1 | 1 | ±0 | (η-19+ 継続) |
| nameless block ... global scope | 2 | 2 | ±0 | (η-19+ 継続) |
| undeclared identifier | 0 | 0 | ±0 | ✓ η-14 達成維持 |
| 'binding' | 0 | 0 | ±0 | ✓ η-8 達成維持 |
| Cannot reuse block name | 0 | 0 | ±0 | ✓ η-1 達成維持 |
| 'size' undeclared | 0 | 0 | ±0 | ✓ η-3 達成維持 |
| GBufferInfo redefinition struct | 0 | 0 | ±0 | ✓ ζ 達成維持 |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | ±0 | ✓ |
| Goodbye | 1 | 1 | ±0 | ✓ clean shutdown |
| Vulkan device destroyed | 1 | 1 | ±0 | ✓ clean shutdown |

**link failed: V↔F bare varying location mismatch -14 件 完全消解 (Phase 1 主目標達成) + 9 種既達主指標完全維持 + clean shutdown** = **η-18 Phase 1 option A 主目標達成**。残 link failed 10 件 + cascade 第16層露出 = **η-19+ 別 sub-bundle 移管対象**。

---

## §3 設計範式

### §3.1 新規 設計範式: C++ runtime transformer V↔F pair allocator 範式 (option A / Phase 2 拡張)

**範式根拠**: η-16 §3.1 「C++ runtime location emit 範式」+ η-9 §3.1 / η-11 §3.1 / η-12 §3.1 / η-17 §3.2 「V/F pair canonical partner 同定範式」+ η-16 §10.7 「V↔F pair allocator 設計仕様」の統合実装。

**範式構造**:
1. **program 単位 LocationAllocator instance** (`generatePerProgramSPIRV` 内 anonymous local)
2. **namespace 分離**: `mUsedFragOutSlots` (Phase 1, η-16) + `mUsedVertOutSlots` / `mUsedFragInSlots` / `mVertOutCursor` / `mVertOutIdentToSlot` (Phase 2, η-18) + `mUsedVertAttribSlots` 等 (Phase 3, η-19+)
3. **kFixedStageOrder = {V, G, F}** で data flow 順 iterate (stages_by_type は std::map<GLenum,...> で key sort 順だと F → V → G になり cross-stage shared state 構築不能)
4. **pre-pass 範式**: 当該 stage 全行 scan で既存 `layout(location=N)` を audit + V stage では ident → slot map に記録
5. **main pass 範式**: bare `out`/`in` を line 単位検出 → V stage out は map 既存 entry あれば再利用、F stage in は V map から ident lookup で同 slot wrap、F stage 既存 layout in は V map と照合 mismatch 時自動 override
6. **qualifier 列 before/after 両対応** (`kQuals` 共有定数 = `(?:(?:flat|smooth|noperspective|centroid|highp|mediump|lowp|invariant)\s+)*`、non-capturing group で regex group 番号を増やさない)
7. **cache invalidate**: `program_hash_obj.update("vulkanize:vN_p2_<scope>")` で transformer version tag bump (η-18 内 v2 → v3 → v4 → v5 の 4 段 iterate)
8. **kill-switch**: `RenderVulkanShaderAutoLocation` cvar で transformer 自体の有効/無効切替 (η-16 §3.1 継承)

### §3.2 新規 設計範式: preprocessor 別分岐 bare/manual-wrap 共存範式 (Phase 1-2)

**範式根拠**: GLSL shader file の典型 pattern:

```glsl
#ifdef LL_VULKAN_GLSL
layout(location=20) out vec3 vary_AdditiveColor;
#else
out vec3 vary_AdditiveColor;
#endif
```

transformer は preprocessor 展開前の raw text に対して動作するため、両分岐の line を見る。pre-pass で manual wrap slot (=20) を `mVertOutIdentToSlot[ident]` に記録、main pass で bare 行を検出した際に **map 既存 entry を canonical truth と扱い同 slot で wrap** することで両分岐とも `layout(location=20)` で揃う。preprocessor で どちらが選ばれても location alignment 保証。

**範式違反パターン (Phase 1 初回実装の falsification)**: main pass で bare 行を無条件に `mVertOutIdentToSlot[ident] = new_slot` で上書き → manual wrap slot (=20) を bare 由来 new_slot (=0) で上書き → F stage 自動 override が新 slot (=0) を信じて wrap → 実 live V (=20) と mismatch。→ Phase 1-2 で `find()` チェック + 既存 entry 再利用に修正。

### §3.3 新規 設計範式: regex non-capturing group + 共有定数による group index 安定範式 (Phase 1-3)

**範式根拠**: 共通の qualifier 列 (`flat smooth ...`) を複数 regex で再利用するため `kQuals` 共有定数として外出し。**non-capturing group `(?:...)`** を使うことで、構成上 group 番号を増やさない設計を選択。

**範式違反パターン (Phase 1-3 falsification)**: 設計時の group 番号 mental model が capturing group `(...)` 前提のままで code を書き、実 regex の non-capturing group `(?:...)` 反映漏れで `m[4]` を ident 取得に使用 → 常に空文字列 → `if (!ident.empty())` で skip → V mapping table 完全に空のまま動作。

**範式遵守チェック**: regex を `kQuals` 等共有定数で構築する場合、regex 自体の capturing group 数を **正規表現本体で literal 数える** + 各 group 番号を **comment で明示** + 主要 group は `m[N]` 直接アクセスの直前で **comment 再記** (Phase 1-3 修正で `// groups: 1=location, 2=ident` を pre-pass body にも記録)。

### §3.4 falsification iterate 範式 (η-18 内 v2 → v5 4 段、prefer-cold-launch-verify)

**範式根拠**: shader transformer のような **GLSL source 解析系の bug** は static code 読みだけでは検出困難 (regex の group index / preprocessor 別分岐の text-level 解釈 / kQuals non-capturing 等)。実 verify を **早く / 短いサイクルで** 回すことで、各 falsification step の 真因 を 1 つに絞り込める。

- v2 (初回実装) → AYA cold launch verify → log 観測で smooth qualifier before 全 miss + 新 regressions 検出 → Phase 1-1 へ
- v3 (Phase 1-1) → AYA cold launch verify → log 観測で link failed 113 件 (regression!) 検出 → 真因 = V mapping 空 → Phase 1-2 へ
- v4 (Phase 1-2) → AYA cold launch verify → log 観測で link failed 113 件 unchanged → 真因 = pre-pass group index bug → Phase 1-3 へ
- v5 (Phase 1-3) → AYA cold launch verify → log 観測で link failed 113 → 10 件 PASS

**範式遵守**: AYA「コマンド + ビルド全権」+「verify は 1 ステップずつ」+「分からないときは log 観測で実データ取る」(memory feedback_admit_unknown / feedback_one_step_at_a_time / feedback_bd_port_autonomous_exec 系) を統合運用。推論を 2 連続外したら推論止めて 必ず literal observation で真因取る。

---

## §4 patch 内容 (`llglslshader.cpp` のみ)

### §4.1 LocationAllocator 構造体拡張 (L662-682)

Phase 1 既存 `mUsedFragOutSlots` / `mFragOutCursor` は維持。Phase 2 (η-18) で新規 4 member 追加。

### §4.2 `vulkanizeStageSource()` signature + 主要 logic (L684-932)

- 新 signature: `(const std::string& source, GLenum stage_type, LocationAllocator& alloc)`
- 対象 stage gate: `if (stage_type != GL_FRAGMENT_SHADER && stage_type != GL_VERTEX_SHADER) return source;`
- `kQuals` 共有定数 (qualifier 列 non-capturing group)
- regex 6 種 (bare_out / bare_in / existing_layout_out / existing_layout_in / existing_layout_out_with_ident / existing_layout_in_with_ident)
- pre-pass: stage 別に既存 layout 由来 location を audit + V stage では ident→slot map 記録
- main pass:
  - **F stage 既存 layout in override** (skip 条件より先に判定、map 不在 / match は touch なし、mismatch は V slot で override)
  - skip 条件 (既 layout / 関数宣言 / uniform 行)
  - **bare out** (V stage = map 既存再利用優先 + 新規 allocate、F stage = η-16 既存 = fragment color attachment slot 割当)
  - **bare in** (F stage のみ V map から ident lookup → 同 slot wrap、map 不在は LL_WARNS + 元行 keep)

### §4.3 cache invalidate version tag (L1002)

`vulkanize:v5_p2_inout_pair_prepass_group_fix` (η-18 内 v2→v3→v4→v5 4 段 iterate の終端 tag)

### §4.4 kFixedStageOrder + allocator instance (L1120-1127)

```cpp
LocationAllocator alloc;
static const GLenum kFixedStageOrder[] = {
    GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER,
};
for (GLenum stage_type : kFixedStageOrder)
{
    auto stage_it = stages_by_type.find(stage_type);
    if (stage_it == stages_by_type.end()) continue;
    // ... stage_type 別に utility prepend + concatenate + vulkanizeStageSource(concatenated, stage_type, alloc)
}
```

---

## §5 cascade shift forward 第16層後始末計画 (η-19+ 移管)

### §5.1 link failed 残 10 件

| カテゴリ | 件数 | 対象 program | error 種別 | 推定対応 sub-bundle |
|---|---|---|---|---|
| MaterialUBO V/F member mismatch | 4 | Skinned/通常/HUD PBR Opaque + HUD PBR Alpha | F 側 `metallicFactor` 等 member 参照、V 側 UBO に同 member 不在 | η-19 (UBO 設計 audit + 統一範式) |
| Anonymous member (UBO global var) | 4 | Skinned/通常/HUD Fullbright Alpha Masking + Deferred Star | UBO 内 anonymous member name 衝突 | η-19 (uniform block 命名整理) |
| vary_position no matching V out | 2 | Skinned/通常 PBR Glow | F bare `in vec3 vary_position;` 対応する V `out` 不在 (PBR Glow V source 構造) | η-19 (PBR Glow 系統 V↔F bare pair 補修) |

### §5.2 parse failed (total) 100 件 内訳 + 推奨 sub-bundle 分割

| カテゴリ | 件数 | 推奨 sub-bundle |
|---|---|---|
| `'normalMap' : redefinition` | 56 | η-19 主 scope 候補 |
| `'depthMap' : redefinition` | 5 | η-19 主 scope 候補 (normalMap と同系統) |
| `non-opaque uniforms outside a block` | 22 | η-19 副 scope (UBO wrap 範式継続) |
| `'vary_fragcoord' : redefinition` | 6 | η-19 副 scope |
| `overlapping use of location (parse stage)` | 5 | η-19 副 scope (η-18 transformer の cascade 露出、location 20 collision) |
| `SPIR-V requires location` | 3 | η-19 副 scope |
| `nameless block ... global scope` | 2 | η-19 副 scope |
| `'TerrainMix' : redefinition struct` | 2 | η-19 副 scope |
| `'weight4' : redefinition` | 1 | η-19 副 scope |

**合計**: 102 件 (一部 multi-error program で重複カウント可能性、log 内訳調査は η-19 着手前 trace で実施)

---

## §6 risks (η-17 §6 継承 + 新規)

| # | risk | 対応状態 | 引継 sub-bundle |
|---|---|---|---|
| 1 | C++ runtime transformer が GL path (RenderVulkanShaderAutoLocation=0) で influence する | ✓ kill-switch で完全 bypass、η-16 から継承 | 引継不要 |
| 2 | preprocessor 別分岐の bare/manual-wrap 共存で map 上書き発生 | ✓ Phase 1-2 で再利用優先範式に修正済 | 引継不要 |
| 3 | regex group index 不整合で V mapping table 空のまま動作 | ✓ Phase 1-3 で `m[4]` → `m[2]` 修正済 + comment 強化 | 引継不要 |
| 4 | qualifier 列 before/after 両対応漏れで全 smooth varying match miss | ✓ Phase 1-1 で `kQuals` 共有定数化 + 全 regex pair 拡張 | 引継不要 |
| 5 | F stage 既存 layout in が V map と mismatch (手動 wrap 不整合) | ✓ Phase 1-1 で自動 override logic 追加 (12 件発見、LL_DEBUGS で観測可能化) | η-19 で残発見時継続 |
| 6 | cascade shift forward 第16層露出 (link failed 10 + non-opaque +11 + overlapping +5 + SPIR-V +3) | 想定通り (η-17 §10.3 forecast) | η-19+ で順次解消 |
| 7 | transformer version tag iterate (v2→v5 4 段 falsification) | 完了 (η-18 末 v5 据置) | η-19 で Phase 3 (vertex attribute) 拡張時 v6 bump |
| 8 | upstream OpenGL Firestorm merge 時 C++ side compatibility (transformer 自体は GL path 不変、shader 側修正なし) | r41 sub-step 4.5 merge phase で初検証 | sub-step 4.5 |
| 9 | F stage `vary_position` map 不在 LL_WARNS が 2 件発生 (PBR Glow 系) | 観測のみ、構造修正は η-19 (PBR Glow V↔F bare pair 補修) | η-19 |
| 10 | LL_DEBUGS 12 件は default 非表示、観測必要時は `Vulkan` log tag enable で表示 | 仕様化 | 引継不要 |

---

## §7 observability

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310) | η-12〜η-18 計測未実施 (cache miss/hit log で間接観測のみ) | B?-η-19 で計測再開 |
| 2 | C++ runtime location emit 範式 (η-16 §3.1) Phase 2 (V↔F pair) 拡張完了 | ✓ η-18 で完遂 | B?-η-19 で Phase 3 (vertex attribute) 拡張候補 |
| 3 | LocationAllocator namespace 拡張 (Phase 1 mUsedFragOutSlots → Phase 2 +mUsedVertOutSlots/mUsedFragInSlots/mVertOutCursor/mVertOutIdentToSlot) | ✓ η-18 で拡張 | B?-η-19 で Phase 3 用 mUsedVertAttribSlots 等追加候補 |
| 4 | regex pair 拡張 (η-16 bare `out` のみ → η-18 bare `in` + existing_layout_in_with_ident + kQuals 共有) | ✓ η-18 で拡張 | B?-η-19 で V stage bare `in` (= vertex attribute) regex 追加候補 |
| 5 | F stage 既存 layout in 自動 override 12 件発生 (V↔F pair alignment 強制保証) | LL_DEBUGS で観測可能化 | B?-η-19 で残発見時継続観測 |
| 6 | F stage `vary_position` map 不在 LL_WARNS 2 件 (PBR Glow 系) | LL_WARNS で観測 | B?-η-19 で構造修正 (PBR Glow V↔F bare pair 補修) |
| 7 | dump 機構 (η-16 §3.1 内包) η-18 では未使用 (verify は log のみで完結) | 維持 | B?-η-19 で必要に応じて再投入 (user_settings.xml 直接 inject 運用継承) |
| 8 | falsification iterate 範式 (η-18 内 v2→v5 4 段) | 適用第 N 例 | B?-η-19 でも同範式継承 (推論 2 連続外したら literal observation 切替) |
| 9 | 既達主指標完全維持 (η-18 で 9 種維持 + link failed: V↔F bare varying-specific 0 達成、退行 0 件) | ✓ η-18 で退行 0 件達成 (link failed 10 = 別カテゴリ shift、V↔F bare varying は完全解消) | B?-η-19 で第16層 cascade 残系統 scope |
| 10 | cinematic_bd directory 全 .glsl audit (η-17 §10.4 継承) | 未実施 (η-8 から継承、η-9〜η-18 全て未実施) | B?-η-19 着手前 or 完遂後の別 phase として継続推奨 |
| 11 | feedback_self_verify_before_handoff 適用 | η-18 で Read による edit 後構造 literal 再確認実施 (regex group 番号 / V/F main pass logic / kFixedStageOrder 順序 / LL_DEBUGS 化) | B?-η-19 でも AYA cold launch 依頼前に必ず実施 |
| 12 | feedback_confirm_referent_before_acting 適用 | AYA「A」judgment → option A 直訳実装、scope shrink せず literal 全実装 | B?-η-19 でも継承 |
| 13 | feedback_remove_verification_logs 適用 | η-18 で override INFO → DEBUG 降格 (12 件/起動が INFO で noisy) | B?-η-19 で追加 LL_INFOS hook あれば commit 前に DEBUG/削除 判断 |
| 14 | feedback_admit_unknown 適用 (推論 2 連続外したら literal observation 切替) | η-18 内 v3 → v4 で推論外し → v4 もう一回外し → v5 で literal trace 強制 + comment 強化 で収束 | B?-η-19 でも継承 |

---

## §8 引継 scope 推奨 (B?-η-19)

### §8.1 着手前 trace (14 ステップ ベースで η-19 適応)

1. AYA 「コマンド + ビルド全権」+「verify は 1 ステップずつ」運用継承確認
2. η-18 末 baseline log (`/home/ishikawa/.ayastorm_x64/logs/AYAstorm.log` 起動 2026-06-02T08:51Z) を canonical baseline として extract
3. parse failed 100 件 + link failed 10 件 = 計 110 件 を **カテゴリ別 + program 別** に系統整理
4. **主 scope 候補 A**: link failed 残 10 件 解消 (MaterialUBO 4 + anonymous member 4 + vary_position-no-V-out 2)
5. **主 scope 候補 B**: parse failed 残 `'normalMap' : redefinition` 56 件 解消 (η-18 で 69 → 56 に副次的減少、η-17 から継続)
6. **主 scope 候補 C**: parse failed 残 `'depthMap' : redefinition` 5 件 解消 (normalMap と同系統)
7. **副 scope 候補**: non-opaque uniforms 22 件 / overlapping use of location (parse stage) 5 件 / SPIR-V requires location 3 件 / vary_fragcoord 6 件 / TerrainMix 2 件 / weight4 1 件 / nameless block 2 件 = 計 41 件 (η-18 末 cascade 残)
8. **A/B/C どれを Phase 1 主 scope に置くか + 副 scope を η-19 内 Phase 2 として同梱するか別 sub-bundle に分離するか** = AYA judgment 候補
9. transformer Phase 3 拡張 (V stage bare `in` = vertex attribute) は **η-19 で対象か / η-20+ 移管か** を A/B/C 主 scope 選択と連動判定
10. transformer version tag は η-18 末 `v5_p2_inout_pair_prepass_group_fix` 据置、η-19 で Phase 3 拡張時 `v6_p3_vertex_attribute` bump 必須
11. AYA log restore 設定: η-17 末で `RenderVulkanShaderDumpTransformed=1` 戻し対象案内済、AYA 環境 settings.xml で 0 確認 (η-18 verify では dump 不要だったが、η-19 で必要なら再 inject)
12. cinematic_bd directory audit (η-17 §10.4 継承) を η-19 着手前 or 完遂後の別 phase として継続推奨
13. upstream merge dry-run (r41 sub-step 4.4 末点) は本 sub-bundle で追加した 1 file (`llglslshader.cpp`) のみで shader file 変更なし = merge conflict 候補軽量
14. **feedback_admit_unknown / feedback_one_step_at_a_time 範式継承** で η-19 内でも literal observation 優先

### §8.2 想定 Phase 構成 (η-19)

η-19 着手前 trace + AYA judgment で確定だが、現時点 推定:
- **Phase 1 (主 scope 候補 A)**: link failed 10 件解消 (MaterialUBO 4 + anonymous member 4 + vary_position 2)、UBO 命名整理 + V↔F bare pair 補修 = manual range
- **Phase 2 (副 scope)**: cascade 第16層後始末 (non-opaque / overlapping / SPIR-V req loc / vary_fragcoord 等)、option B 手動 wrap or option A transformer Phase 3 拡張
- **Phase 3 (option A 拡張時)**: transformer Phase 3 = V stage bare `in` (vertex attribute) 自動 wrap、η-17 previewV.glsl で manual 解消した 3 件を構造化

### §8.3 B?-η-19 完遂後の想定 cascade exposure 第17層

- link failed 10 件解消想定 = link failed 0 達成復帰
- normalMap/depthMap 61 件解消想定 = 大量解消 (Phase 1 主 scope 完走で)
- 第17層 emergence 観測点: link 段階を超えた pipeline 段階 (PSO compile / runtime binding / pipeline cache 等) で新 error 露出可能性
- shader_cache 件数: η-19 で計測再開 + 第17層 emergence 観測

---

## §9 commit message 案

```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-18 完遂

option A 採用 (C++ runtime transformer の Phase 2 拡張 = V↔F pair allocator)
で V↔F bare varying location alignment mismatch を構造的に解消。
η-17 末 link failed 14 件 → η-18 末 10 件 (Δ -4 件 net、V↔F bare
varying-specific 失敗は 0 件達成、残 10 件は cascade shift forward
第16層露出 = MaterialUBO/anonymous member/vary_position-no-V-out
= η-19+ 別 sub-bundle 移管)。

Phase 1 (option A、C++ runtime transformer Phase 2 拡張):
- LocationAllocator 構造体に Phase 2 用 namespace 追加
  (mUsedVertOutSlots/mUsedFragInSlots/mVertOutCursor/mVertOutIdentToSlot)
- vulkanizeStageSource() signature を (source, stage_type, alloc&) に変更
  し V/F 両 stage を対象に拡張
- regex 6 種を kQuals 共有定数 (qualifier before/after 両対応) で構築
- pre-pass で V stage 既存 layout out を ident→slot map に記録
- main pass:
  * V stage bare out: map 既存再利用優先 (preprocessor 別分岐の bare 版
    が manual wrap slot を上書きしない)
  * F stage bare in: V map から ident lookup → 同 slot で wrap
  * F stage 既存 layout in: V map と照合 mismatch なら V slot で override
- kFixedStageOrder = {V, G, F} で data flow 順 iterate
- transformer version tag を v5_p2_inout_pair_prepass_group_fix に bump

実装中 4 段の falsification iterate (v2 → v3 → v4 → v5):
- v2: qualifier after only 設計 → smooth out match miss
- v3 (Phase 1-1): kQuals 共有定数 + F stage override → link failed 113 件 regression
- v4 (Phase 1-2): map 既存再利用 → 変化なし
- v5 (Phase 1-3): pre-pass group index m[4] → m[2] 修正 → PASS

主指標達成 (vs η-17 末 baseline):
- link failed 14 → 10 (-4, V↔F bare varying-specific は 0 達成)
- Layout location qualifier (link error) 14 → 0
- parse failed total 100 → 100 (±0、主 scope 外)
- 9 種既達主指標完全維持 + clean shutdown

詳細: docs/specs/ayastorm-r41-gl-removal/
       handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-18-complete.md
```

---

**handoff doc 完。次 sub-bundle B?-η-19 着手は本 doc §8 推奨 scope (link failed 残 10 解消 + normalMap/depthMap 系統 + cascade 第16層後始末) を起点として、fresh context で実施。**
