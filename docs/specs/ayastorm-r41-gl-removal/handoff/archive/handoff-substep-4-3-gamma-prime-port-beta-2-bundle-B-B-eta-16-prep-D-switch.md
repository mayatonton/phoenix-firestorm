# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-16 prep handoff (D 切替境界)

**status**: η-15 完遂境界後、η-16 着手前 trace + D 切替判断 完了 → **Phase 1 (LLGLSLShader source transformer 実装) 着手** を次 session fresh context で実施
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: 本 sub-bundle (η-16) では **patch 未投入** (D 切替判断のみ、設計プラン段階で context 切替)
**handoff doc commit**: 本 doc は AYA 明示 commit 指示後に commit (feedback_no_auto_commit)
**勤続範式継承**: η-15-complete `cb28cf1daa` (patch `6a11eabc73`) を canonical baseline として継承、η-1〜η-15 全範式継承

---

## §1 サマリー

本 doc は η-16 sub-bundle の **着手前 trace + 設計判断段階の引継 handoff**。η-15 末で確定した B (shader-only mechanical wrap) 範式を、AYA 判断で **D = C++ runtime location emit 化** に切替する境界を記録。

### §1.1 判断の経緯 (2026-06-02 session)
1. η-15 handoff doc §10.1 で η-16 候補 scope = **SPIR-V missing 41 件** が確定済
2. 着手前 trace で 41 件全件の program/stage/line 把握、root cause = bare `in`/`out` 宣言の `layout(location=N)` 修飾子欠落
3. shader file 単位 audit で `fsObjectIDF.glsl` L27 等の bare-out 数件は確定、ただし FRAGMENT stage error の多くは shared util 経由で post-preprocess line 番号と原文 line が乖離、static trace 限界
4. **AYA 問題提起**: 「Upstream 側が OpenGL のままアップデートしてくることを考えるとどれが受け入れやすいのか」+「AYAstorm として Vulkan 化したい本来の目的はプロセス分離による CPU core 処理分離」
5. 両面再評価 → **D (C++ runtime location emit 化)** が唯一の両面 better
   - upstream merge conflict 0 (.glsl は upstream と同型)
   - Vulkan 化本丸 (process 分離) と整合 (LLGLSLShader 集約で pipeline build process 分離設計が clean)
6. AYA「D に切替で進めて」judgment 確定 (2026-06-02)
7. Plan agent で D preliminary design 完了
8. context 容量自己評価 → Phase 1 実装着手は次 session に handoff (本 doc)

### §1.2 D 切替の意義
- η-1〜η-15 で確立した shader-only 範式 (約 70+ file wrap 済) を **公式 retire**
- 新範式名: **「C++ runtime location emit 範式」** (η-8 §3.3 `mIndexedTextureChannels` Vulkan-aware C++ touch 範式の自然延長)
- charter §3 #1 改訂方針は §10 で詳述

---

## §2 η-15 末状態 (baseline、canonical pointer)

**canonical handoff doc**: `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-15-complete.md`

**main metric (η-15 末)**:
| metric | count |
|---|---|
| **SPIR-V requires location** | **41** ← η-16 D 主指標 |
| 'normalMap' redefinition | 69 (η-17+ 移管) |
| 'depthMap' redefinition | 9 (η-17+ 移管) |
| non-opaque uniforms outside a block | 0 (η-15 達成維持) |
| overlapping use of location | 0 (η-12 達成維持) |
| Layout location qualifier | 0 (η-10 達成維持) |
| 'binding' | 0 (η-8 達成維持) |
| Cannot reuse block name | 0 (η-1 達成維持) |
| 'weight4'/'weight' | 0 (η-5/η-7 達成維持) |
| Link failed | 0 (η-3 達成維持) |
| 'size' | 0 (η-3 達成維持) |
| GBufferInfo struct redefinition | 0 (ζ 達成維持) |
| nameless block global scope | 0 (η-14 達成維持) |
| undeclared identifier | 0 (η-14 達成維持) |
| parse failed (count) | 121 |
| FATAL/SIGSEGV/Aborted | 0/0/0 |
| Goodbye / Vulkan device destroyed | 1 / 1 |

**既達主指標**: 14 種 (non-opaque 含む)、η-16 で全件 0 維持必須

**set/binding 使用済**: 14-62 + 100-103 (η-15 §10.6)、新規 UBO 割当は **binding=63 から**

---

## §3 SPIR-V missing 41 件 stage 別分類 (着手前 trace 結果)

### §3.1 stage 0x8dd9 (GEOMETRY) 4 件
- Skinned Normal Debug Shader (L47, L48)
- Normal Debug Shader (L46, L47)

候補 file: `class1/interface/normaldebugG.glsl` L32 `out vec4 vertex_color;` / L34 `in vec4 normal_g[];` / L36 `in vec4 tangent_g[];` (HAS_ATTRIBUTE_TANGENT)、bare 確定

### §3.2 stage 0x8b31 (VERTEX) 2 件
- Skinned Object Preview Shader (L481)
- Object Preview Shader (L331)

候補 file: 要 verify (preview 系 V shader)、η-16 Phase 1 着手時に grep + log dump で確定

### §3.3 stage 0x8b30 (FRAGMENT) 35 件
- Deferred Diffuse Shader (L342) / Skinned (L343)
- Deferred Diffuse Alpha Mask Shader (L189) / Skinned (L190)
- Deferred Blur Light Shader (L1674)
- Deferred Fullbright Shader (L1199) / Skinned (L1200) / HUD (L1200)
- Deferred Fullbright Alpha Masking Shader (L1200) / Skinned (L1201) / HUD (L1201)
- Deferred Fullbright Alpha Masking Alpha Shader (L1977) / Skinned (L1978) / HUD (L1978)
- Deferred FullbrightShiny Shader (L3081) / Skinned (L3082) / HUD (L3082)
- Deferred Emissive Shader (L1199) / Skinned (L1200)
- FS Object ID Shader (L173) ← `fsObjectIDF.glsl` L27 `out vec4 frag_color;` bare 確定
- Deferred Shadow Fullbright Alpha Mask Shader (L191) / Skinned (L192)
- Deferred Shadow Alpha Mask Shader (L188) / Skinned (L189)
- Deferred Gamma Correction Post Process (L1673) / Legacy (L1674)
- Deferred Tonemap Post Process (L2106) / No Post (L2107) / Gamma (L2107) / No Post Gamma (L2108) / Legacy (L760) / No Post Legacy (L2109)
- Deferred Buffer Visualization Shader (L173)
- AYAstorm Velocity Alpha Shader (L193) / Skinned (L194)

**注**: 主要 fragment file (`diffuseAlphaMaskF.glsl`, `emissiveF.glsl`, `shadowAlphaMaskF.glsl`, `fullbrightF.glsl`, `avatarVelocityF.glsl`) は in/out 既 wrap 済確認済。post-preprocess line 188-194 / 1199-1201 / 1673-1674 / 1977-1978 / 2106-2109 / 3081-3082 等は **shared util file 経由の bare in/out** が真因 (静的 trace 限界、Phase 1 で runtime preprocessed dump で確定)。

---

## §4 D 設計確定要素 (Plan agent 出力エッセンス)

### §4.1 Hook 点
- **`LLGLSLShader::generatePerProgramSPIRV()`** (llglslshader.cpp:648) が Vulkan compile path の **唯一の entry**
- concatenated buffer 完成直後の **cpp:849 直前** (`concat_buffers.push_back(std::move(concatenated))` の手前) に `vulkanizeStageSource(concatenated, stage_type, location_allocator)` を inject
- 既存 `LL_VULKAN_GLSL` macro injection 箇所: cpp:777 + llshadermgr.cpp:543
- preprocessor pass は **存在せず** glslang 自身が `#version` / `#ifdef` を処理する → D は raw text transformer として小さく実装可能

### §4.2 detection regex
```regex
^[ \t]*(in|out)[ \t]+((flat|smooth|noperspective|centroid|highp|mediump|lowp)[ \t]+)*([a-zA-Z_][a-zA-Z0-9_]*)[ \t]+([a-zA-Z_][a-zA-Z0-9_]*)(\[[^;]*\])?[ \t]*;
```
- group 4: type (`vec3`, `float`, `vec4`, `int`, `mat4` etc.)
- group 5: identifier
- group 6: array suffix (`[]`, `[3]` — geometry shader `in vec4 normal_g[];` 対応)

**false positive 除去 (順序付き)**:
1. line 内 `//` で trim (block comment `/* ... */` は state machine で skip)
2. 既に `layout(` を含む行は skip
3. `(` が同 line にある → 関数宣言 (`void foo(out vec3 x)`) skip
4. `uniform` 行は regex で自動除外
5. `layout(triangles) in;` 等 primitive layout は type 欠落で自然 skip

### §4.3 LocationAllocator (program 単位 local stack)
```cpp
struct LocationAllocator {
    std::map<std::string, int> mVaryingLocations;   // name → location (V/F pair 整合)
    std::set<int>              mUsedVaryingSlots;   // 既使用 location 番号
    std::set<int>              mUsedAttrSlots;      // vertex attribute slot
    int                        mNextVaryingSlot = 30; // 既存 20-29 帯 reserve 後の auto-allocate cursor
    int                        mFragOutCursor   = 0;  // fragment output 連続割当
};
```

- **V→G→F 順処理**: `std::map<GLenum>` の sorted 順では fragment (0x8B30) が vertex (0x8B31) より先になる罠 → 内側 stage 処理順を `[VERTEX, GEOMETRY, FRAGMENT]` に **明示固定** する内 loop 必須
- V の `out X` 検出 → `mVaryingLocations[X] = mNextVaryingSlot++` 登録、書換: `layout(location=N) out X;`
- F の `in X` 検出 → `mVaryingLocations[X]` lookup → 同 N emit、lookup miss は safe fallback で新規 alloc + WARN log
- G→F: G の `out X` (scalar) と F の `in X` を name match (V→F と同 map)
- **既存 layout(location=N) との衝突回避**: pre-pass で `mUsedVaryingSlots` に登録、auto-allocate cursor は skip loop で衝突 avoid
- **fragment out 特殊扱い**: `out vec4 frag_color;` / `out vec4 frag_data[N];` は **常に location=0 起点連続割当** (MRT 対応)
- **vertex attribute** (vertex `in`): η-16 では touch しない方針、`LLShaderMgr::mReservedAttribs` との整合は Phase 3 で必要時に拡張

### §4.4 既存 wrap honor (移行期間互換)
- 既存 70+ file の `#ifdef LL_VULKAN_GLSL\nlayout(location=N) in X;\n#else\nin X;\n#endif` block は Vulkan path で `#ifdef` 側が active
- pre-pass で `layout(location=N)` 行を `mUsedVaryingSlots` に登録、bare 検出 (`#else` 側) は **同名既登録ガード**で skip
- **既存 wrap touch 0** が charter §3 #1 byte-for-byte 担保

### §4.5 kill-switch / fallback
- **debug setting**: `LLCachedControl<bool> sVulkanShaderAutoLocation("RenderVulkanShaderAutoLocation", true)` を `generatePerProgramSPIRV()` 先頭で読む、false なら transformer skip
- **環境変数**: `getenv("LL_VULKAN_SHADER_AUTO_LOCATION") == "0"` で強制 disable

### §4.6 diagnostic dump (η-8 §3.2 範式継承)
- `LLCachedControl<bool> sVulkanShaderDumpTransformed("RenderVulkanShaderDumpTransformed", false)` true で、transformed concatenated source を `<cache_dir>/transformed/<program_hash>_<stage>.glsl` に書き出し
- 既存 cache_dir (cpp:670-674) 再利用、追加 IO cost のみ
- sub-bundle 完遂時に default disable に戻す (η-15 §3.2 範式)

---

## §5 Phase 構成 (5-7 cycle 想定、charter 中央値 +1 上振れ)

| Phase | 内容 | 推定 cycle |
|---|---|---|
| Phase 1 | detection regex + fragment out only 簡易 transformer + dump 機構 + kill-switch | 1-2 cycle |
| Phase 2 | V/F pair 整合 (V→F varying) + 既存 layout pre-pass | 2 cycle |
| Phase 3 | 3-stage (V/G/F) + array suffix 対応 + interpolation qualifier 対応 | 1-2 cycle |
| Phase 4 | 既存 wrap honor verify + cache key 統合 (transformed source も HBXXH128 hash 入力) + smoke run | 1 cycle |
| **合計** | | **5-7 cycle** |

### §5.1 sub-bundle 分割可否 (AYA 未決定 judgment)
- **option A**: 全 Phase 1-4 を η-16 内で消化 (charter 中央値超過、4-6 cycle 想定外れ可能性)
- **option B**: Phase 1-2 を η-16、Phase 3-4 を η-17 に分割 (進捗計測しやすい、handoff doc が 2 周回必要)
- AYA judgment 必要、Phase 1 cycle 1 終了境界で再判断する選択肢もあり

---

## §6 残 risk

| # | risk | mitigation |
|---|---|---|
| 1 | **cache key 整合性**: cpp:657-666 の `HBXXH128` は `stage.sources` を入力にする、D で transform した結果は cache key に乗らない → transform on/off 切替時に **stale cache hit** | (a) cache key 計算式に `vulkanizeStageSource()` の result string を入れる (cpp:667 を移動) (b) kill-switch on/off 時に cache invalidate を強制 — Phase 4 で詳細設計、AYA judgment 必要 (cache invalidate 許容可否) |
| 2 | **vertex attribute binding** (`LLShaderMgr::mReservedAttribs`) と auto-allocate の整合 | η-16 Phase 1-3 では vertex `in` touch しない、Phase 3 で bare vertex in 実存 grep audit 後判断 |
| 3 | **`#ifdef LL_VULKAN_GLSL` の `#else` bare 行を D が誤って書き換える** | 同名 layout 既登録ガード (§4.4) で skip、Phase 1 末 smoke で literal verify |
| 4 | **cascade pair shift**: 41 件解消で +N regression 露出可能性 (handoff §11 #13 cascade 第14層) | η-11 §3.4 / η-12 §3.2 / η-14 §3.4 範式 (順方向 / 逆方向 / Y=X 境界条件) で ACCEPT 判定 |
| 5 | **既存 wrap 70+ file location 番号体系との衝突** | pre-pass で構造的解消、auto-allocate cursor 30 起点 → 既存 20-29 帯と衝突 0 (論理保証) |
| 6 | **normalMap/depthMap 78 件**: D とは独立 root cause (uniform redefinition、bare → bare 重複 declaration) | η-16 では touch 0、η-17+ 専用 phase で対処 (handoff §10.1 で sub-bundle 分割推奨) |
| 7 | **interpolation qualifier** (`flat`/`noperspective`) 出現密度 unknown | Phase 1 着手前 grep audit で実存数確定、regex で対応済 (§4.2) |
| 8 | **`std::map<GLenum>` の sorted 順罠** (0x8B30 FRAGMENT が 0x8B31 VERTEX より先) | 内側 stage 処理順を `[V, G, F]` 明示固定 (§4.3) |
| 9 | **post-preprocess line 番号から file 同定困難**: log error は post-preprocess line で原文と乖離 | Phase 1 着手時に diagnostic dump (§4.6) で transformed source を取得、確定的に root cause 同定 |
| 10 | **既存 charter §3 #1 shader-only 範式と矛盾** | §10 charter 改訂方針で公式 retire 宣言、handoff §3.X で新範式宣言 |

---

## §7 着手前 trace 範式 12 ステップ (η-15 §10.2 継承 + D 切替追加)

η-15 §10.2 範式を D 切替で **拡張** (新 step 11-12 追加、step 9 D 化、step 10 D 化):

1. **literal grep**: 各既達主指標 (15 種、non-opaque 含) の literal grep + SPIR-V 41 件 + normalMap 69 + depthMap 9
2. **handoff doc 記録漏れ補完範式** (η-12 §3.4): 前 sub-bundle (η-15) handoff §2 metric 表 + §10.1 推奨 scope の literal 再検証
3. **handoff doc misanalysis 検出救済範式** (η-14 §3.5): 前 sub-bundle handoff doc の前提に literal 確認 step を必ず組込
4. **GL stage hex 解釈確認範式** (η-14 §3.1): log error の 0x8B30=FRAGMENT / 0x8B31=VERTEX / 0x8DD9=GEOMETRY を着手前 trace で再確認
5. **log context 抽出**: 各 program の error LINE × 件数 + stage type 番号 + 真因 file 候補
6. **V/F pair / auto-attach helper 同定** (η-9〜η-14 範式): location 系は V/F pair、UBO 系は auto-attach helper 経路同定
7. **nameless block 重複衝突 検出** (η-14 §3.2)
8. **同 UBO 別 stage 複製判定** (η-14 §3.3)
9. **binding 60+ 連続割当範式** (η-15 §3.1): 新規 UBO 割当は **binding=63 から** (D で新規 UBO 追加なし想定)
10. **3 重 nest 条件 UBO wrap 範式** (η-15 §3.2): D で feature flag 内 bare in/out 出現時に適用
11. **【新規 D 切替 step】preprocessed dump 取得範式 (η-8 §3.2 範式再投入)**: post-preprocess line 番号から原文 file 同定不能な fragment shader (~30+ 件) の root cause file 確定、Phase 1 着手時必須
12. **【新規 D 切替 step】LLGLSLShader concatenation 順序 audit**: cpp:751-839 の utility shader prepend 順序を literal trace (どの shared util が何行目に挿入されるか) → post-preprocess line 番号と原文 file の mapping table 構築

---

## §8 次 session Phase 1 着手手順

### §8.1 fresh context 着手前 trace (§7 全 12 step 適用)
1. 本 handoff doc §2-§7 を canonical baseline として読む
2. η-15-complete handoff doc を canonical pointer 経由で再確認 (§2)
3. log `~/.ayastorm_x64/logs/AYAstorm.log` の 41 件 SPIR-V error literal grep で baseline 確認
4. 14 種既達主指標 0 維持 confirm
5. Phase 1 着手 scope = **detection regex + fragment out only transformer + dump 機構 + kill-switch** を AYA に提示
6. AYA「OK」judgment 後 patch 設計に進む

### §8.2 Phase 1 実装手順
1. **read**: `indra/llrender/llglslshader.cpp` の `generatePerProgramSPIRV()` 周辺 (cpp:648-870)
2. **read**: `llglslshader.h` の `StageSource` struct 周辺
3. **read**: `indra/llrender/llshadermgr.cpp` の `createSPIRVFromGLSL()` (廃 path、参考)
4. **read**: `indra/llrender/llshadermgr.h` の `mReservedAttribs` (vertex attribute mapping、Phase 3 用 reference)
5. **read**: `indra/newview/app_settings/settings.xml` で `RenderVulkan*` 系既存 debug setting pattern 確認
6. **edit**: `llglslshader.cpp` に `vulkanizeStageSource()` 静的関数追加
   - detection regex 実装
   - `LocationAllocator` struct を関数内 local stack
   - fragment out のみ transform (Phase 1 限定)
   - kill-switch (debug setting + env var)
   - dump 機構 (debug setting で transformed source を file に書き出し)
7. **edit**: `settings.xml` に `RenderVulkanShaderAutoLocation` (default true) + `RenderVulkanShaderDumpTransformed` (default false) 追加
8. **build**: `make -C build-linux-x86_64 -j$(nproc)` (1 回目は 20-40 分想定)
9. **install**: `cd build-linux-x86_64/newview && make install` → `~/ayastorm/` へ deploy
10. **shader cache clear**: `rm -rf ~/.ayastorm_x64/cache/shader_cache/*`
11. **dump 有効化**: `RenderVulkanShaderDumpTransformed=true` を debug setting に
12. **AYA cold cache launch** 依頼 → log + transformed source dump 取得
13. **verify**:
    - SPIR-V missing 41 件のうち **fragment out 由来分** が解消したか count
    - non-opaque + 14 種既達主指標 0 維持 confirm
    - cascade pair shift (新規 regression) 検出
    - clean shutdown
14. **dump 無効化**: `RenderVulkanShaderDumpTransformed=false` に戻す (η-15 §3.2 範式)
15. **handoff doc 起草**: Phase 1 完遂 doc を起草

### §8.3 Phase 2 以降の準備
- Phase 1 完遂後、V/F pair 整合実装に進む
- V→F symbol table 構築、stage 処理順 `[V, G, F]` 明示固定の内 loop 実装
- 既存 layout pre-pass で `mUsedVaryingSlots` 構築

---

## §9 未決 AYA judgment 2 件

### §9.1 sub-bundle 分割可否
- option A: 全 Phase 1-4 を η-16 内で消化
- option B: Phase 1-2 を η-16、Phase 3-4 を η-17 に分割
- option C: Phase 1 cycle 1 終了境界で再判断 (動的決定)

### §9.2 cache key 変更の妥当性
- `HBXXH128` 計算式に transformer 結果を含める仕様変更で、既存 cache invalidate (cold cache から再 build) が初回 1 回必要
- 初回 launch 時間が伸びる (~2-5 分追加想定)
- AYA judgment 必要

---

## §10 charter / 範式更新方針

### §10.1 charter §3 #1 改訂
- **現行**: shader-only 範式維持
- **改訂後**: shader-only 範式は η-8 §3.3 (`mIndexedTextureChannels` Vulkan-aware) と η-16 D (`vulkanizeStageSource()`) の **2 例の C++ touch 例外を持つ**

### §10.2 §11 観測点 #9 改訂
- **現行**: 「C++ touch 0 維持」
- **改訂後**: 「C++ touch は (a) η-8 §3.3 mIndexedTextureChannels Vulkan-aware (b) η-16 D = `generatePerProgramSPIRV()` 内 source transformer の **2 例に限定**、それ以外の C++ touch は AYA judgment 要」

### §10.3 新範式宣言
- 名称: **「C++ runtime location emit 範式」** (η-8 §3.3 範式の自然延長)
- η-16-complete handoff §3.X で公式宣言
- shader-only 範式は **retire**、η-17+ で既存 wrap 70+ file の段階 revert pathway を開く

### §10.4 charter §3 #4 acceptance refine
- 現行 §3 #4 last 段 (charter 215-219 行 acceptance refine 運用) に追記:
  > "base 228 file の SPIR-V 化を、shader-only wrap でなく C++ runtime location emit で達成しても acceptance を満たす"
- 趣旨 (#1-#9) 自体は変更しないので AYA 確認不要範囲

---

## §11 観測点 (η-15 §11 継承 + 新規)

| # | 観測点 | 状態 | 次 sub-bundle 引継 |
|---|---|---|---|
| 1 | shader_cache 件数 (η-7 305 baseline、η-11 310) | η-12/η-14/η-15/η-16-prep 計測未実施 | η-16 Phase 1 で計測再開 + 第15層 emergence 観測 |
| 2 | binding 60+ 連続割当範式 (η-15 §3.1) 次安全番号 | **63** から開始 | D で新規 UBO 追加なし想定、η-17+ 別 sub-bundle で適用 |
| 3 | 3 重 nest 条件 UBO wrap 範式 (η-15 §3.2) | 適用済 (SMAABlendWeightsF) | D で feature flag 内 bare in/out 出現時に併用 |
| 4 | 1-shot ACCEPT vs 2-phase 構成判定範式 | η-15 第 5 例 | η-16 Phase 1 はおそらく 1-2 cycle 必要 (1-shot ACCEPT 想定外) |
| 5 | C++ touch 0 維持 | **η-16 で公式 retire** (D 投入で例外 2 例化) | §10 charter 改訂方針適用 |
| 6 | cinematic_bd directory 全 .glsl audit | 未実施 (η-8 から継承) | η-16 完遂後の別 phase として推奨 |
| 7 | runtime preprocessed dump 取得範式 (η-8 §3.2) | **η-16 Phase 1 で再投入** (D 設計 §4.6 で恒久化) | dump 機構は default disable で残置、sub-bundle 完遂毎に確認 |
| 8 | set/binding allocation | 14-62 + 100-103 占有 | D で追加割当なし想定 |
| 9 | 既達主指標 14 種 完全維持 | η-15 達成 | η-16 で 15 種 (SPIR-V 加わる) 維持 |
| 10 | cascade exposure 第14層 残 | SPIR-V 41 + normalMap 69 + depthMap 9 = 119 件 | η-16 で SPIR-V 41 → 0 想定、normalMap/depthMap は η-17+ |
| 11 | std140 alignment 範式 | η-15 で確認済 | D で UBO 追加なし、適用機会なし |
| 12 | feedback_self_verify_before_handoff | η-15 で適用、η-16-prep でも適用 (本 doc) | Phase 1 完遂時も適用必須 |
| 13 | **【新規】shader-only 範式 retire 境界** | η-15 末で最終達成、η-16 で D 切替 | η-17+ で既存 wrap 70+ file 段階 revert pathway |
| 14 | **【新規】C++ runtime location emit 範式 第 1 例** | η-16 で投入予定 | 次 sub-bundle で第 2 例適用可能性 |
| 15 | **【新規】upstream merge conflict 防御範式** | η-16 D で構造的解消 | 将来 upstream merge 時に .glsl 同型保持 verify |
| 16 | **【新規】Vulkan 化本丸 = process 分離との整合** | D 投入で LLGLSLShader 集約 = process 分離設計が clean | pipeline build process 分離は r41 後半 or r42 で別 phase |

---

## §12 feedback memory rules 全件継承確認

- **feedback_no_auto_commit**: 本 handoff doc は AYA 明示 commit 指示後に commit
- **feedback_no_claude_coauthor**: commit message に Co-Authored-By を一切含めない
- **feedback_shader_only_fast_iterate**: η-16 D 切替で **shader edit が大半消える**、C++ build 必須化 → autobuild 必要
- **feedback_proactive_handoff**: 本 doc が **自発的** handoff (context 容量自己評価)、AYA 指示なしで起草
- **feedback_one_step_at_a_time**: 次 session でも 1 step ずつ
- **feedback_self_verify_before_handoff**: 本 doc 起草で Plan agent 出力 + 自分の trace 整合確認済
- **feedback_explanation_lead_with_conclusion**: 本 doc §1.1 / §1.2 で結論ファースト
- **feedback_no_scope_shrink**: 41 件全件を D で解消する scope 維持、削減しない
- **feedback_admit_unknown**: post-preprocess line 番号から原文 file 同定不能 = §6 risk #9 で admit
- **feedback_falsification_as_progress**: B → D 切替は B の falsification ではなく upstream merge / Vulkan 化本丸 観点での **両面 better 選択**
- **feedback_build_only_verified**: D 設計は preliminary、Phase 1 実装で実機検証必須
- **feedback_doubt_self_first**: D 設計の location auto-allocate / V/F pair 整合の正しさは Phase 1 smoke で実機 verify
- **feedback_root_cause_not_dump**: 41 件 SPIR-V missing の root cause = shader-only 範式の構造限界、D で根本解消
- **feedback_warn_aya_off_bd_line**: BD 移植期間外 (2026-05-24 完全終了)、適用外
- **feedback_bd_port_autonomous_exec**: BD 期間外、適用外
- **feedback_use_agents_proactively**: 着手前 trace で Explore agent + Plan agent 投入済
- **feedback_no_dual_doc_split**: handoff doc を docs/specs/ に一本化、内部/公開分けない

---

## §13 次 session 着手 prompt 案

```
# AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-16 D Phase 1 着手

## 現状
- η-15 完遂・commit 済 (6a11eabc73 patch / cb28cf1daa handoff doc)
- η-16-prep-D-switch handoff doc 起草済 (本 doc commit pending)
- branch: feature/ayastorm-r41-gl-removal
- D (C++ runtime location emit 化) に切替確定 (AYA judgment 2026-06-02)
- Phase 1 = detection regex + fragment out only transformer + dump 機構 + kill-switch (1-2 cycle 想定)

## 必須参照
- handoff doc (canonical): docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-16-prep-D-switch.md
  - §4 D 設計確定要素 (Hook 点 cpp:849 直前 / regex / LocationAllocator / kill-switch / dump)
  - §5 Phase 構成 5-7 cycle
  - §6 risks 10 件
  - §7 着手前 trace 12 step
  - §8 Phase 1 実装手順 15 step
  - §9 未決 AYA judgment 2 件 (sub-bundle 分割 / cache invalidate 許容)
  - §10 charter 改訂方針

## 遵守 memory rules
- feedback_no_auto_commit
- feedback_no_claude_coauthor
- feedback_proactive_handoff (context 残量自己監視)
- feedback_one_step_at_a_time
- feedback_self_verify_before_handoff
- feedback_explanation_lead_with_conclusion
- feedback_use_agents_proactively

## 最初の発話 (推奨)
「η-16 Phase 1 着手します。§8.1 着手前 trace 12 step を実施します。最初に §9 未決 AYA judgment 2 件 (sub-bundle 分割 / cache invalidate 許容) ご判断ください。」
```

---

**handoff doc 完。次 session で本 doc §8.1 着手前 trace + §9 AYA judgment 取得 + §8.2 Phase 1 実装、を順次実施。**
