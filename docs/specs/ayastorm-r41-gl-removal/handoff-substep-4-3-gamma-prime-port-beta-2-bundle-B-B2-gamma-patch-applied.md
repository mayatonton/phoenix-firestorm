# handoff: substep 4.3-γ'-port-β-2-bundle-B-B2-γ patch 適用済 (deploy/verify 待ち)

**作成日**: 2026-06-01
**状態**: cadence step 3 (patch) 完了、未 commit、step 5 (deploy + AYA cold cache launch verify) 未実施
**前 handoff**: handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B3-complete.md (§12 metric 訂正済、commit `d2d531fec4`)

---

## §1 cadence 位置

| step | 内容 | 状態 |
|------|------|------|
| 1 trace | utility shader / loadBasicShaders / attachShaderFeatures / generatePerProgramSPIRV trace | ✓ |
| 2 prep | 設計範式 3 件 (案 A 採用 = utility source cache + per-program tracking + concat hook) | ✓ AYA OK |
| 3 patch | 4 file 編集 (header 2 + source 2) | ✓ **本 handoff 時点完了、未 commit** |
| 4 self-verify | diff coherence cross-check | ✓ |
| 5 deploy + verify | configure → build → install → cache clear → AYA cold cache launch verify | ⬜ **次 session** |
| 6 metric | `.old`/`.log` pair-grep で link failed / No function definition / その他 cluster 計測 | ⬜ |
| 7 commit | AYA「OK」明示後 patch commit | ⬜ |
| 8 handoff (complete) | B2-γ-complete handoff doc 起草 + commit | ⬜ |
| 9 memory | MEMORY.md + project memory 更新 | ⬜ |

---

## §2 採用設計範式 (B2-γ 新規 3 件)

1. **Vulkan utility source cache** (LLShaderMgr 側)
   - `mVertexShaderSourceCache` / `mFragmentShaderSourceCache` = `std::map<std::string, std::vector<std::string>>`
   - key = filename (例 `"deferred/globalF.glsl"`)
   - value = preprocessed shader_code_text[] copy (各 entry の sources[0] は GL profile `#version XXX\n`)
   - 充填 = `loadShaderFile` 内 free 前、`out_sources == nullptr && LLVKLoader::isVulkanInitialized()` 条件下

2. **Per-program attached utility tracking** (LLGLSLShader 側)
   - `mVulkanAttachedVertexUtilities` / `mVulkanAttachedFragmentUtilities` = `std::vector<std::string>`
   - `attachVertexObject(file)` / `attachFragmentObject(file)` 内 `glAttachShader` 直後、Vulkan 時 push_back
   - attach 順 (= `attachShaderFeatures()` 仕様順) を保つ

3. **Utility source concat hook + createShader reorder**
   - `generatePerProgramSPIRV` 内、stage 先頭 prepend (`#version 460` + extension + `LL_VULKAN_GLSL` 直後) で utility cache から source 引いて prepend、その後 program-specific `mShaderFiles` source append
   - `createShader` で SPIR-V 生成を `attachShaderFeatures()` 完了 **後** に移動 (β-2-β までは前)、これで utility vector 充填済の状態で生成 hook 到達

---

## §3 修正 4 file 詳細

### `indra/llrender/llshadermgr.h` (+11 行)

`mVertexShaderObjects` / `mFragmentShaderObjects` map 宣言直下 (line 484 後) に:

```cpp
std::map<std::string, std::vector<std::string>> mVertexShaderSourceCache;
std::map<std::string, std::vector<std::string>> mFragmentShaderSourceCache;
```

### `indra/llrender/llshadermgr.cpp` (+44 行)

既存 `out_sources` 充填 block (line 1036-1051) の直後、free loop 前に utility cache populate block を追加。

```cpp
if (out_sources == nullptr && LLVKLoader::isVulkanInitialized())
{
    std::vector<std::string>* cache_entry = nullptr;
    if (type == GL_VERTEX_SHADER)        cache_entry = &mVertexShaderSourceCache[filename];
    else if (type == GL_FRAGMENT_SHADER) cache_entry = &mFragmentShaderSourceCache[filename];
    if (cache_entry)
    {
        cache_entry->clear();
        cache_entry->reserve(shader_code_count);
        for (GLuint i = 0; i < shader_code_count; ++i)
        {
            if (shader_code_text[i]) cache_entry->emplace_back(shader_code_text[i]);
            else                     cache_entry->emplace_back();
        }
    }
}
```

**配置位置の根拠**: free loop 前でないと shader_code_text[i] が dangling になる。out_sources block と同じ hook point。

### `indra/llrender/llglslshader.h` (+10 行)

`std::vector<StageSource> mStageSources;` 直下 (line 386 後) に:

```cpp
std::vector<std::string> mVulkanAttachedVertexUtilities;
std::vector<std::string> mVulkanAttachedFragmentUtilities;
```

### `indra/llrender/llglslshader.cpp` (~+60 行)

**(a)** `attachVertexObject` (line 924-) / `attachFragmentObject` (line 943-) の `glAttachShader` + `stop_glerror()` 直後、`return true;` 前に Vulkan 時 push_back:

```cpp
if (LLVKLoader::isVulkanInitialized())
{
    mVulkanAttachedVertexUtilities.push_back(object_path);  // または Fragment
}
```

**(b)** `createShader` (line 432-) 内、β-2-β までは `if (!mUsingBinaryProgram)` block 内末尾で SPIR-V 生成 + clear していたが、本 patch で:
- block 内末尾の `generatePerProgramSPIRV` call + `mStageSources.clear/shrink_to_fit` を削除 (comment 残置)
- `attachShaderFeatures(this)` call 直後に移動:
```cpp
if (!mUsingBinaryProgram && success && LLVKLoader::isVulkanInitialized() && !mStageSources.empty())
{
    generatePerProgramSPIRV(mStageSources);
}
mStageSources.clear(); mStageSources.shrink_to_fit();
mVulkanAttachedVertexUtilities.clear();   mVulkanAttachedVertexUtilities.shrink_to_fit();
mVulkanAttachedFragmentUtilities.clear(); mVulkanAttachedFragmentUtilities.shrink_to_fit();
```

**(c)** `generatePerProgramSPIRV` (line ~636-) 内、B3 が追加した stage 先頭 `#version 460` + extension + `#define LL_VULKAN_GLSL 1` 直後、program-specific `mShaderFiles` source append loop の **前** に utility prepend block を追加:

```cpp
const std::vector<std::string>* utility_files = nullptr;
const std::map<std::string, std::vector<std::string>>* source_cache = nullptr;
if (stage_type == GL_VERTEX_SHADER)
{
    utility_files = &mVulkanAttachedVertexUtilities;
    source_cache  = &mgr->mVertexShaderSourceCache;
}
else if (stage_type == GL_FRAGMENT_SHADER)
{
    utility_files = &mVulkanAttachedFragmentUtilities;
    source_cache  = &mgr->mFragmentShaderSourceCache;
}
if (utility_files && source_cache)
{
    for (const std::string& util_file : *utility_files)
    {
        auto it = source_cache->find(util_file);
        if (it == source_cache->end())
        {
            LL_WARNS("Vulkan") << "...cache miss for '" << util_file << "'..." << LL_ENDL;
            continue;
        }
        const std::vector<std::string>& util_sources = it->second;
        for (size_t i = 1; i < util_sources.size(); ++i)  // sources[0] = GL profile skip
            concatenated.append(util_sources[i]);
    }
}
```

---

## §4 self-verify 結果

| check | 結果 |
|-------|------|
| cache 充填 timing (free 前) | ✓ shader_code_text alive |
| cache key 一貫性 (filename) | ✓ loadBasicShaders と attachShaderFeatures が同一 path 文字列使用 (例 `"deferred/globalF.glsl"` / `"avatar/objectSkinV.glsl"`) |
| 型一致 (VS↔mVertexShaderSourceCache↔mVulkanAttachedVertexUtilities) | ✓ |
| GL path byte-for-byte | ✓ 全新規 code Vulkan-gated、glAttachShader call は push_back の前 |
| `mUsingBinaryProgram` path | ✓ SPIR-V 生成 guard 済、utility vector は populate されても直後 clear |
| 再帰 loadShaderFile (shader_level retry) | ✓ 再帰側で cache populate 実施、outer call の return 後 body は実行されない |
| std::map / std::vector include | ✓ llshadermgr.h で既に他 member が使用、transitive include 済 |
| ternary 一時 vector 回避 | ✓ pointer 化で copy 回避 |
| empty utility list (attachNothing) | ✓ iteration skip で B3 動作に degrade |
| GL_GEOMETRY_SHADER stage | ✓ utility_files=nullptr で prepend skip、B3 動作維持 |

---

## §5 charter §3 #1 (GL/Vulkan 並走) 担保

- `loadShaderFile`: GL `glCreateShader`/`glShaderSource`/`glCompileShader` 系の strdup/free path 完全不変、新規 cache populate は std::string copy のみ
- `attachVertex/FragmentObject`: `glAttachShader` 呼出は push_back の前、glAttachShader 失敗時 (count==0) は push_back せず `return false`
- `createShader`: reorder は SPIR-V hook 位置のみ、`attachShaderFeatures` call 位置不変、GL link 経路 (`mapAttributes`/`mapUniforms`) 順序不変
- Vulkan path 失敗 → WARN 出力のみ + GL fallback、GL 既 attach 済 program は untouched

---

## §6 残作業 (cadence step 5-9)

### step 5 deploy + verify

```bash
cd ~/work_firestorm/phoenix-firestorm
autobuild configure -A 64 -c ReleaseFS_open -- \
  -DLL_TESTS:BOOL=OFF -DPACKAGE:BOOL=ON \
  --fmodstudio --opensim -DLL_DULLAHAN_AUDIO_CALLBACK=ON
autobuild build -A 64 -c ReleaseFS_open
rm -rf ~/ayastorm
cd build-linux-x86_64
make install DESTDIR=$HOME/ayastorm
rm -rf ~/.ayastorm_x64/cache/shader_cache/
mv ~/.ayastorm_x64/logs/AYAstorm.log ~/.ayastorm_x64/logs/AYAstorm.old.b2-gamma-baseline
```

AYA に cold cache launch verify 依頼:
- 起動成立 + clean shutdown + `Goodbye!` 1 件 + Vulkan device/instance destroyed 各 1 + shader_cache 再生成
- FATAL/SIGSEGV/crash 0 件

### step 6 metric (期待値)

B3 baseline (`.old`) vs B2-γ verify (`.log`) で:

| 計測項目 | B3 baseline | 期待 B2-γ | delta |
|----------|-------------|-----------|-------|
| `link failed for program` | 55 | 0 | **-55 ✓ B2-γ 直接効果** |
| `No function definition` | 206 | 0 | **-206 ✓ B2-γ 直接効果** |
| `parse failed for stage` | 147 | ±0 | 構造的に B2-γ 無関係 |
| `0:N: 'location'` | 13 | ±0 | location 関連 (B? 残) |
| `binding` | 45 | ±0 | binding 関連 (B? 残) |
| `missing #endif` | 15 | ±0 | preprocessor (B? 残) |
| `non-opaque uniforms` | 74 | ±0 | uniform decl (B? 残) |
| `#version` cluster | 0 | ±0 | B3 既処理 |
| **net delta** | - | - | **-261 (link failed + No function definition)** |

**警戒**: pair-grep は case-insensitive `link` で false positive を出す (B3 §12 教訓)。`link failed for program` を literal 検索すること。

### step 7 commit (AYA「OK」明示後)

```bash
git add indra/llrender/llshadermgr.h indra/llrender/llshadermgr.cpp \
       indra/llrender/llglslshader.h indra/llrender/llglslshader.cpp
git commit  # commit message 例: §10 参照
```

### step 8 handoff (complete) doc 起草

`docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B2-gamma-complete.md`

範式継承: B3-complete (`44422cb14f`) / B2-β-complete (`64f3229109`)

§1-§13 構成 (§12 metric 訂正セクション枠は不要だが、本 patch metric 測定で false positive 発見した場合は §12 で訂正記録)

### step 9 memory 更新

- `MEMORY.md` 124 行目 `project_ayastorm_r41_vulkan_migration.md` 項の title + description 更新:
  - 達成位置: B2-γ 完遂 (utility source cache + per-program tracking + concat hook + reorder)
  - metric delta 記入
  - 次 sub-bundle = B? (残 location 13 + binding 45 + missing #endif 15 + non-opaque 74 + parse 147 mixed cleanup)
- `project_ayastorm_r41_vulkan_migration.md` 同等更新

---

## §7 不変式 (絶対 rule)

- A1-A7 / A8-recovery / B1 / B2-α / B2-β / B3 既処理 file の再 touch **禁止**
- shader file touch 0 (本 patch は C++ 4 file のみ)
- skip list 13 + A2 expansion 2 + 5 V skip 全 untouched
- charter §3 #1 (GL path byte-for-byte) 担保
- commit は AYA「OK」明示指示後のみ
- 検証用 LL_INFOS hook 等は commit 前に除去 (現 patch は除去対象なし)

---

## §8 次 session 着手用 message (AYA へ)

> 前 session で B2-γ patch 実装まで完了。未 commit。次 step = build + AYA cold cache launch verify。
>
> `docs/specs/ayastorm-r41-gl-removal/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B2-gamma-patch-applied.md` 参照。
>
> 進めて良ければ「OK」、設計再確認したい部分があれば指摘ください。

---

## §9 想定リスク

| risk | 想定影響 | mitigation |
|------|----------|------------|
| utility source cache miss (filename 不一致) | 該当 utility が prepend されず "No function definition" 残存 | WARN 出力 → log で検出可能、必要なら filename normalization |
| utility prepend で重複 #version 残存 | `must occur first` 復活 | 各 entry sources[0] skip で対応済、cache miss 時は安全側 skip |
| GLSL `layout(location=N)` 再宣言 (utility と program の location 番号衝突) | location error 増加 | 想定外時は location cluster +Δ で観測、別 sub-bundle で対処 |
| memory consumption (cache 全 utility 保持) | utility ~19 file × ~数 KB = 数 MB peak | 起動時 1 回のみ、createShader 完遂後 utility vector は shrink_to_fit、cache map は process lifetime 保持 (再 reload 時 overwrite) |
| `mUsingBinaryProgram` 時の utility vector populate | 直後 clear で無害 | 性能 negligible |

---

## §10 想定 commit message 雛形 (B2-γ patch commit 時)

```
feat(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ 完遂 (utility source cache + per-program attached utility tracking + concat hook + createShader reorder、4 file +XX/-XX、llshadermgr.{h,cpp} + llglslshader.{h,cpp} 編集 shader file touch 0、設計範式新規 3 件 = Vulkan utility source cache + per-program attached utility tracking + utility concat hook、generatePerProgramSPIRV で attachShaderFeatures() 経由 utility 集合を file_name key で引き stage 先頭 prepend、createShader reorder で SPIR-V hook を attachShaderFeatures() 後ろに移動 (β-2-β までは前 = utility vector 未充填状態 = "No function definition" 206 件の構造原因)、metric vs B3 baseline = link failed for program 55→0 (-55 ✓ B2-γ 直接効果) + No function definition 206→0 (-206 ✓ B2-γ 直接効果) + 他 cluster ±0、net delta -261、charter §3 #1 担保 (Vulkan path 内部改造のみ、GL path glCreateShader/glShaderSource/glCompileShader/glAttachShader 全不変、loadShaderFile strdup 不変、shader_cache 245 再生成 ±0)、shader file touch 0、AYA cold cache launch verify PASS 2026-MM-DD)
```

---

## §11 reference

- 前 handoff: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B3-complete.md` (commit `44422cb14f` + §12 訂正 commit `d2d531fec4`)
- 範式継承: B2-β-complete (`64f3229109`) / B2-α-complete (`c0e1b25310`)
- charter: `r41-vulkan-migration-charter.md` §3 #1 acceptance
- AYAstorm log path: `~/.ayastorm_x64/logs/AYAstorm.log` (現 verify) / `~/.ayastorm_x64/logs/AYAstorm.old.b2-gamma-baseline` (B2-γ 用 B3 baseline)
- shader_cache: `~/.ayastorm_x64/cache/shader_cache/` (verify 前 clear 必須)

---

**EOF**
