# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-3 完了

**作成日**: 2026-06-04
**前 session commit** (= η-30 Phase 1.B 系列、新しい順):
- `8ad78b42b0` (= PB-2 complete handoff doc 起案)
- `e43d93dd25` (= PB-2 `mapUniforms()` Vulkan path integer index 経路 cache 構築 block 追加)
- `7032a0b1a8` (= PB-1 complete handoff doc 起案)
- `0ba743463c` (= PB-1 LLGLSLShader cache 構造 3 member 追加)
- `7fe58b7428` (= 副次 (a) `_PREFIX_TO_CADENCE` 拡張)

**本 session 物理出力** (= 全て **未 commit**、AYA さん明示指示後 batch commit):
- `indra/llrender/llglslshader.cpp` modified (= +47 line、(a) anonymous namespace 末尾 `g_static_hashed_uniform_names[]` 80 名配列追加 + (b) `mapUniforms()` 末尾 PB-2 block 直後 = `unbind()` 直前 に LLStaticHashedString 経路補助 cache 構築 iterate block 追加)
- 本 handoff doc 新規 (= PB-3 complete = 30 setter redirect 層 第 3 sub-step 終端 marker)

**次 session 着手**: **AYA 判断不要 自走可** (= 2026-06-04 AYA 判断 4 件 (entry handoff §3.6) + GATE-B (PB-1 complete handoff §3.1) + PB-2 設計判断 1 件 (= 挿入位置 = `unbind()` 直前) + 本 session 確定 設計判断 5 件 (= §3.1〜§3.5) + 順序組替え 1 件 (= §3.6 PB-6 前倒し) 全採用済) → **PB-6 着手** = `forwardToUboUpload()` shell 実装 (= FWD-1 採用、空 stub 関数追加。PB-4 着手前に technical dependency として前倒し)。

---

## §0 state 一行 summary

η-30 **Phase 1.B PB-3 complete state**:
- **PB-3 LLStaticHashedString 経路補助 cache 構築 完了** (= `indra/llrender/llglslshader.cpp` modified、未 commit) =
  - (a) anonymous namespace 末尾 (`llglslshader.cpp:993-1017`) に `g_static_hashed_uniform_names[]` 80 名 `const char* const` 配列追加 (= 2026-06-04 `indra/newview` + `indra/llrender` 全 grep 抽出結果)
  - (b) `mapUniforms()` 末尾 PB-2 block 直後 = `unbind()` 直前 (`llglslshader.cpp:1925-1944`) に `if (mUseUBO) { for (name in g_static_hashed_uniform_names) { lookup_runtime → hash → mUniformUBOLocByHash 登録 } }` block 追加、+47 line。
- **GATE-B 整合** = `#ifdef LL_VULKAN_GLSL` 不使用、`if (mUseUBO)` runtime gate 単独。
- **mUseUBO=false default 維持** (= MUSEUBO-A) ゆえ本 block 走らず、既存 OpenGL 挙動 100% 維持。
- **本 session 設計判断 5 件** (= §3.1〜§3.5) =
  - §3.1 spec literal `getStringHash()` → 実装 API `Hash()` 修正 (= `LLStaticHashedString` class に `getStringHash()` method 不在、`Hash()` のみ存在、戻り型 `size_t` を `static_cast<U64>` で hash key 化)
  - §3.2 grep 抽出 80 名 (推定 67 名より +13、+19%) で S1-C 自走継続 (= AYA 「A で」承認)
  - §3.3 配置場所 = anonymous namespace 末尾 (= helper 関数群の後、まとめて宣言)
  - §3.4 iterate block 位置 = `unbind()` 直前 = PB-2 block 直後 (= PB-2 設計判断と整合、対称構造内)
  - §3.5 hash 計算 API = `LLStaticHashedString(name).Hash()` + `static_cast<U64>` cast (= 32bit/64bit platform 両対応 + PB-5 setter 側で同 API 経由で hash 一致保証)
- **本 session 順序組替え 1 件** (= §3.6) = **PB-6 を PB-4 直前に前倒し** (= `forwardToUboUpload()` が PB-4 setter から call されるため、stub 先行宣言が link error 回避の技術的必然)。
- **llrender 単体 build PASS** + **full viewer build PASS** = `[100%] Built target llpackage` + tar.xz package `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz` 生成。
- **残 sub-step**: PB-6 (= 前倒し) → PB-4.1〜.17 → PB-5.1〜.13 → PB-7 → PB-N strict 線形。

---

## §1 pre-requisite 最小読み (= `feedback_handoff_minimal_pre_req_read` 準拠)

**全件読み禁止**。次 session 着手時は **3 件のみ** 読む。残りは作業中に必要箇所のみ pinpoint Read。

### §1.1 必読 3 件

| # | file | 読む箇所 | 目的 |
|---|---|---|---|
| 1 | 本 handoff doc (= `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-3-complete.md`) | 全文 | PB-3 完了 state + 設計判断 5 件 + 順序組替え (PB-6 前倒し) + 残 sub-step 順序 |
| 2 | `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md` §5.2 (= `forwardToUboUpload()` 宣言場所 + signature) | 当該節のみ | PB-6 = `forwardToUboUpload()` shell 実装 = signature 確定 + 空 stub 実装 |
| 3 | `indra/llrender/llglslshader.h:427-429` (= PB-1 で追加 3 member) + `indra/llrender/llglslshader.cpp:993-1017` (= PB-3 追加 g_static_hashed_uniform_names[]) + `indra/llrender/llglslshader.cpp:1925-1944` (= PB-3 追加 iterate block) | 当該箇所のみ | PB-3 完成形参照、PB-6 追加位置選定 |

### §1.2 pinpoint Read 用 reference

| file | 必要時の参照箇所 |
|---|---|
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-2-complete.md` | PB-2 完了 state + 挿入位置設計判断 (§3.1) + 残 sub-step 表 |
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-PB-1-complete.md` | PB-1 完了 state + GATE-B 確定根拠 |
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-b-entry.md` | entry handoff §3.6 AYA 判断 4 件確定表 (= S1-C / FWD-1 / MUSEUBO-A / §2.3 default) |
| `indra/llcommon/llstaticstringtable.h:35-66` | `LLStaticHashedString` class 定義 (= `Hash()` method 戻り型 `size_t`、`String()` method 戻り型 `const std::string&`、`makehash()` = djb2 variant) |
| `indra/llrender/llglslshader.h:427-429` | PB-1 で追加した 3 member (`mUniformUBOLoc` / `mUniformUBOLocByHash` / `mUseUBO`) |
| `indra/llrender/llglslshader.cpp:993-1017` | PB-3 で追加した anonymous namespace 内 `g_static_hashed_uniform_names[]` 80 名配列 |
| `indra/llrender/llglslshader.cpp:1925-1944` | PB-3 で追加した `mapUniforms()` 内 iterate block |
| `indra/llrender/llglslshader.cpp:1899-1923` | PB-2 で追加した `mapUniforms()` 内 integer index 経路 cache 構築 block |
| `build-linux-x86_64/codegen/ubo/ubo_perfect_hash.inl:887-908` | `lookup_runtime(const char* name)` API + `fnv1a_32()` (= PB-3 で利用済、setter 側でも同 API) |
| `docs/specs/ayastorm-r41-gl-removal/design/06a-cache-structure-and-setter-redirect.md:§5.5` | LLStaticHashedString 13 method 一覧 (= PB-5 着手時) |
| `indra/llrender/llglslshader.cpp:2643-2870` | LLStaticHashedString 経路 13 method の現状 (= PB-5 着手時、PB-3 は cache 構築のみで setter 改変なし) |
| `indra/llrender/llglslshader.cpp:2141-2538` | integer index 経路 17 method の現状 (= PB-4 着手時) |

---

## §2 残 sub-step (PB-6 → PB-4 → PB-5 → PB-7 → PB-N) strict 線形構成 (= 本 session 順序組替え反映、§3.6 参照)

| PB-X | 内容 | 該当 chapter | 物理改変 file | 出力契約 | 状態 |
|---|---|---|---|---|---|
| ~~PB-1~~ | ~~`mUniformUBOLoc` cache 構造実装~~ | 06a §3 | `indra/llrender/llglslshader.h` | header + full build PASS | **✅ 完了** (commit `0ba743463c`) |
| ~~PB-2~~ | ~~`mapUniforms()` Vulkan path integer index 経路 cache 構築~~ | 06a §4.1 / §4.2 | `indra/llrender/llglslshader.cpp:1899-1923` | +26 line `if (mUseUBO) { resize + lookup_runtime loop }` block | **✅ 完了** (commit `e43d93dd25`) |
| ~~PB-3~~ | ~~LLStaticHashedString 経路 cache 構築 (= S1-C 採用)~~ | 06a §4.3 / §4.3.1 | `indra/llrender/llglslshader.cpp:993-1017` + `1925-1944` | +47 line `g_static_hashed_uniform_names[]` 80 名 + iterate block | **✅ 完了** (本 session、未 commit) |
| **PB-6** (= 前倒し) | **`forwardToUboUpload()` shell 実装** (= FWD-1 採用) = (a) `llglslshader.h` 内に method 宣言追加 = `void forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size);` + (b) `llglslshader.cpp` 末尾 (= `mapUniforms()` の後 / 他適切位置) に空 stub 関数本体 `{}` 実装 | 06a §5.2 (= 「(= 06b で実装)」literal) | `indra/llrender/llglslshader.h` (= method 宣言) + `indra/llrender/llglslshader.cpp` (= 空 stub) | shell 関数 link PASS、後続 PB-4/PB-5 で call 解決 | **未着手 = 次 session 着手** |
| **PB-4** | 17 method (integer index 経路) Vulkan path 分岐追加 = 各 method 末尾 `glUniform*` 直前に `if (mUseUBO) { auto& loc = mUniformUBOLoc[index]; if (loc.cadence_tag == 0xFFFFFFFFu) return; if (loc.cadence_tag == 5 /* sampler */) return; forwardToUboUpload(loc, data, size); return; }` 分岐挿入。1 method 1 PB-4.X sub-step (= `feedback_ubo_migration_one_at_a_time` 準拠、17 setter 一括禁止) | 06a §5.2 / §5.3 / §5.6 | `indra/llrender/llglslshader.cpp` 17 箇所 (= line 2141-2538) | 17 method 全 compile PASS + mUseUBO=false 経路 unchanged |
| **PB-5** | 13 method (LLStaticHashedString 経路) Vulkan path 分岐追加 = 各 method の `const LLStaticHashedString&` overload 版に `mUniformUBOLocByHash.find(static_cast<U64>(uniform.Hash()))` lookup → `forwardToUboUpload()` 分岐追加。1 method 1 PB-5.X sub-step | 06a §5.5 | `indra/llrender/llglslshader.cpp` 該当 LLStaticHashedString overload 13 箇所 (= line 2643-2870) | 13 method 全 compile PASS + mUseUBO=false 経路 unchanged |
| **PB-7** | 整合 check 仕込み = `mapUniforms()` 末尾 PB-2/PB-3 拡張部直後に debug build 用 `llassert(mUniformUBOLoc.size() == mUniform.size())` + cadence_tag check | 06a §4.4 | `mapUniforms()` 末尾 | debug build PASS + assert 動作確認 |
| **PB-N (= PB-8)** | Phase 1.B Exit Criteria 検証 = full build + install + cache clear + viewer launch + bind 不変動作確認 + log で Phase 1.B 起因 fail 0 件確認 + AYA さん起動目視「変化していないと思う」確認 + Phase 1.B 全終了 handoff doc 起案 | 09 §4.2 | (検証 phase = 物理 code 改変なし) | viewer 起動 + login + region entry PASS + AYA 確認後 commit |

---

## §3 本 session で新規確定した設計判断 (5 件) + 順序組替え (1 件)

### §3.1 spec literal `getStringHash()` → 実装 API `Hash()` 修正 (= bug 解消)

**設計問**: handoff §3.3 + PB-2 complete handoff §3.3 で literal `LLStaticHashedString("name").getStringHash()` と書かれていたが、`LLStaticHashedString` class 実装 (= `indra/llcommon/llstaticstringtable.h:35-66`) に `getStringHash()` method 不在。

**実装確認**:
- L46: `size_t Hash() const { return string_hash; }` (= 大文字 H、戻り型 `size_t`)
- L45: `const std::string& String() const { return string; }`
- `getStringHash()` という名前は class 内に存在しない

**修正**: PB-3 物理実装で `LLStaticHashedString(name).Hash()` (= 大文字 H) を採用、`static_cast<U64>` で U64 key 化 (= `mUniformUBOLocByHash` は `std::unordered_map<U64, ubo::UniformLocation>` key 型と一致)。

**根拠**:
- (i) `LLStaticHashedString` 既存 caller (= `llglslshader.cpp:1624 LLStaticHashedString hashedName(name); mUniformMap[hashedName] = location;`) も `Hash()` 経由 (= `LLStaticStringHasher` operator() 内 `key_value.Hash()` 呼出し、`indra/llcommon/llstaticstringtable.h:71`)
- (ii) hash アルゴリズム = djb2 variant `((hashval<<5) + hashval) + *c++` (= `indra/llcommon/llstaticstringtable.h:52-62`)、PB-3 cache 構築 / PB-5 setter 側で同 API 経由が hash 一致保証
- (iii) `feedback_doubt_self_first` 適用 = spec literal を疑い、実装 grep で確認 → 正しい API 採用

**spec 06a 更新は不要** (= PB-2 complete handoff §3.3 + entry handoff の literal 修正は handoff doc chain 内の参照価値が低い、本 PB-3 handoff §3.1 で記録すれば足りる)。Phase 1.B 全終了時の handoff doc 統合時に spec 06a §4.3 code shape (= 仮 code shape) の `getStringHash()` literal は実装に合わせて `Hash()` で例示更新する余地あり (= AYA 判断項に格上げしない、Phase 1.B 後の spec 整合作業 scope)。

### §3.2 grep 抽出 80 名 (推定 67 より +13、+19%) で S1-C 自走継続

**設計問**: handoff §3.2 で literal「67 個前後収束 → S1-C 自走 / 大幅乖離 (= 200 個超) → AYA に S1-B 切替提案」。grep 抽出結果 = **80 unique** uniform 名 (= 93 定義 - 13 重複)。

**判断**: 67 → 80 は +19%、handoff literal「200 個超」乖離閾値には届かず「67 個前後」許容内と解釈。**S1-C 自走継続**。AYA に 1 line ACK 仰ぎ (= 「A で」承認) → 自走。

**80 名 list** (= `g_static_hashed_uniform_names[]` 入力、`indra/llrender/llglslshader.cpp:1001-1017` 物理確認):
```
SMAA_RT_METRICS, NoiseTexture, RenderTexture, above_water, alpha_scale,
ambiance, aya_blur_dir, aya_blur_radius, aya_glow_color, aya_glow_gain,
aya_strength, aya_translucency_params, aya_translucency_tint, bloomStrength,
blurDirection, blurWidth, brightMult, brightness, bump_code, camPosLocal,
cas_param_0, cas_param_1, clip_plane, contrast, contrastBase, custom_alpha,
delta, diffuse_luminance_scale, direction, dist_factor, dither_scale,
dither_scale_s, dither_scale_t, dither_tex, dt, dynamic_exposure_enabled,
dynamic_exposure_params, dynamic_exposure_params2, exposure, extractHigh,
extractLow, glowMap, hdri_split_screen, kern, kern_scale, lumWeights,
maxRoughness, maxZDepth, mipLevel, noiseStrength, noiseVec, norm_mat,
norm_scale, object_id_packed, offset, out_screen_res, probe_strength,
resScale, roughness, saturation, screenMap, screenRes, sourceIdx,
ssao_irradiance_max, ssao_irradiance_scale, stepX, stepY, tex0, tex1,
texelSize, texture0, texture1, tint, tolerance, tonemap_mix,
tonemap_type, u_width, waterSign, zfar, znear
```

**filter 除外 2 件** (= LLStaticHashedString として定義されているが uniform setter として実際使用なし):
- `color_in` (= `llface.cpp:74` 定義のみ)
- `texture_index_in` (= `llface.cpp:73` 定義のみ)

**出処分布** (= validity 高、Agent 報告):
- `pipeline.cpp` 37 定義
- `llpostprocess.cpp` 15 定義
- `llheroprobemanager.cpp` 9 定義
- 残 19 = LL 由来 (= bloom/SMAA/glow/lighting/postproc/水/atmos 系)

### §3.3 配置場所 = anonymous namespace **末尾** (= helper 関数群の後)

**設計問**: handoff §3.3 default literal「`llglslshader.cpp` anonymous namespace」だが、namespace 内のどこ (= 頭 / 末) かは未指定。

**3 案検討**:

| 案 | 内容 | 採否 |
|---|---|---|
| (a) namespace 頭 (= L632 直後) | 80 名表が読み手に最初に見える、ただし helper 関数群より前に巨大 data | 退け |
| **(b) namespace 末尾 (= L991 直前)** | helper 関数群の後に data、まとめて宣言 | **採用** |
| (c) namespace 外 (= 新規 file `static_hashed_uniform_names.inl`) | 80 名 table の独立性が高い、ただし PB-3 scope overshoot | 退け |

**採用根拠**:
- (i) anonymous namespace 内 helper 関数群 (= L632-991、glslang init / vulkanize transformer 等) は long & complex、頭に巨大 data 入れると helper 関数 reach までスクロール負担増
- (ii) 末尾配置 = helper 関数群 → data の自然な順序 (= helper 関数が data を使うわけではないので、依存方向問題なし)
- (iii) 80 名表は単純な `const char* const` 配列 1 件のみ = 新規 file 化のコストに見合うほどの大きさではない、Codegen 化は PB-3 scope overshoot (= 後 Phase で必要なら refactor)

**memory 保存不要** = PB-3 内の個別 sub-step 配置判断、Phase 1.B 全 sub-step の一般原則ではない。

### §3.4 iterate block 位置 = `unbind()` 直前 = PB-2 block 直後 (= PB-2 設計判断と整合)

**設計問**: spec 06a §4.3 literal「`mapUniforms()` Vulkan path 内の追加 step、§4.1 拡張部の後」の物理位置 = PB-2 block 末尾 (= `if (mUseUBO) { ... }` closing brace 直後) か `unbind()` 直後 (= 関数末尾) か。

**判断**: **PB-2 block 直後 = `unbind()` 直前** (= bind/unbind 対称構造内、PB-2 block と PB-3 block が連続配置)。

**根拠**:
- (i) PB-2 complete handoff §3.1 設計判断 (= 「`unbind()` 直前 = bind/unbind 対称構造内」) と整合、Phase 1.B sub-step 内一貫性
- (ii) PB-2 (= integer index cache 構築) と PB-3 (= LLStaticHashedString cache 構築) は対称的な処理、連続配置で意図的並列性が表現できる
- (iii) `lookup_runtime` は GL state を一切触らない (= 純粋 CPU side hash table lookup) ため bind 状態に依存しない、`unbind()` 後でも機能は同じ
- (iv) PB-7 整合 check (= debug `llassert`) も同位置に追加すれば、PB-2/PB-3/PB-7 が 1 block 内に集中 = 後 maintenance 時の find/edit 局所化

**memory 保存不要** = PB-2 設計判断 §3.1 と同一 logic、本 PB-3 handoff §3.4 で参照足りる。

### §3.5 hash 計算 API = `LLStaticHashedString(name).Hash()` + `static_cast<U64>` cast

**設計問**: §3.1 で `getStringHash()` → `Hash()` 修正確定、`Hash()` 戻り型は `size_t`、`mUniformUBOLocByHash` key 型は `U64` (= `std::unordered_map<U64, ubo::UniformLocation>`)。`size_t` → `U64` の cast は必要か?

**判断**: `static_cast<U64>(LLStaticHashedString(name).Hash())` で明示 cast 採用。

**根拠**:
- (i) Linux x86_64 環境では `size_t = unsigned long = 64bit = U64` 同型 = cast は no-op、warning も出ない
- (ii) 32-bit platform (= Win32 等、Phoenix Firestorm は 64-bit only build だが念のため) では `size_t = unsigned int = 32bit`、U64 cast で zero-extend、衝突無し
- (iii) PB-5 setter 側でも `static_cast<U64>(uniform.Hash())` で同 cast 経由 → 同 hash 値で lookup 成功 (= hash 一致保証は LLStaticHashedString::Hash() 共通使用、cast pattern も共通)
- (iv) 明示 cast は implicit narrowing warning / future-proofing 観点で安全 (= 将来 `size_t` ≠ `U64` 環境への適応コスト最小)

**memory 保存不要** = PB-5 setter 側着手時に本 §3.5 参照すれば足りる。

### §3.6 順序組替え = PB-6 を PB-4 直前に前倒し (= technical dependency)

**設計問**: handoff §2 残 sub-step strict 線形は元々 PB-3 → PB-4.1〜.17 → PB-5.1〜.13 → PB-6 → PB-7 → PB-N の順 (= entry handoff §3.6 Q-PB-ORDER = §2.3 default 採用)。だが PB-4 が `forwardToUboUpload()` を call する → PB-6 (= `forwardToUboUpload()` 実装) を PB-4 より後に置くと PB-4 build で **undefined reference linker error**。

**判断**: **PB-6 を PB-4 直前に前倒し**、新順序: PB-3 (= 本 commit) → **PB-6** → PB-4.1〜.17 → PB-5.1〜.13 → PB-7 → PB-N。

**根拠**:
- (i) PB-6 は FWD-1 採用確定 (= entry handoff §3.6) = **空 stub 関数 `{}` 一括実装**、内容変化なし、順序組替えは形式的
- (ii) PB-4 が `forwardToUboUpload()` を call する設計 (= 06a §5.2 literal `forwardToUboUpload(loc, data, size); return;`) ゆえ、call site (PB-4) より先に declaration + stub (PB-6) が必須 = technical compile dependency
- (iii) Q-PB-ORDER §2.3 default 「概念的順序」は記述順 = 「cache 構築 → setter 改変 → stub 実装 → 検証」、physical compile order は別軸
- (iv) AYA 判断不要 (= 順序の技術的必然、scope/方針変化なし)、本 handoff doc §3.6 で記録すれば足りる

**memory 保存不要** = 順序組替えは PB-6 → PB-4 → PB-5 → PB-7 で固定、本 PB-3 handoff §2 残 sub-step 表で反映済。

---

## §4 self-verify (= 9 観点、本 handoff 起案時点)

| 観点 | 確認 | 結果 |
|---|---|---|
| (1) PB-3 物理 diff | `git diff --stat indra/llrender/llglslshader.cpp` で 1 file changed + 47 insertions、handoff doc 未起案時点 | ✅ |
| (2) PB-3 配列追加位置 | `llglslshader.cpp:993-1017` = anonymous namespace (L632-1018) 末尾、helper 関数群の後、`const char* const g_static_hashed_uniform_names[]` 80 名物理確認 | ✅ |
| (3) PB-3 iterate block 位置 | `llglslshader.cpp:1925-1944` = `mapUniforms()` 末尾 PB-2 block 直後 (`if (mUseUBO) { integer index cache } / / [PB-3 block] / / unbind();` 順)、bind/unbind 対称構造内 | ✅ |
| (4) GATE-B 順守 | `#ifdef LL_VULKAN_GLSL` 不使用、`if (mUseUBO)` runtime gate のみ、grep で `LL_VULKAN_GLSL` 文字列出現 0 件 (本 block 内) | ✅ |
| (5) mUseUBO=false default 維持 | `mUseUBO` initial value = `false` (= llglslshader.h:429)、本 block 内で書き換えなし、既存 OpenGL build / 既存挙動 100% 維持 | ✅ |
| (6) llrender 単体 build PASS | `make -j$(nproc) llrender` = `[ 33%] Built target codegen_ubo` → `Building llglslshader.cpp.o` → `Linking libllrender.a` → `[100%] Built target llrender` | ✅ |
| (7) full viewer build PASS | `make -j$(nproc)` = `[100%] Built target llpackage` + tar.xz package `Phoenix-FirestormOS-AYAstorm-release_LEGACY-7-2-4-81586.tar.xz` 生成 | ✅ |
| (8) spec 06a §4.3 / §4.3.1 S1-C literal 準拠 | build-time list 化 + iterate で `lookup_runtime → hash 計算 → mUniformUBOLocByHash[hash] = *loc` (= S1-C 採用、§3.1 で literal `getStringHash()` → `Hash()` 修正) | ✅ |
| (9) git working tree | `git status --short` で `M indra/llrender/llglslshader.cpp` のみ + 本 handoff doc 新規、scripts/ 改変なし + handoff doc 以外 doc 改変なし + Co-Authored-By: Claude 行不在 | ✅ |

---

## §5 引き継ぎ済 memory (= 次 session で active、PB-2 complete handoff から継承、追加なし)

- `project_r41_phase1b_vulkan_host_gate.md` — Phase 1.B 全 sub-step (PB-4〜PB-N) で C++ 側 `#ifdef LL_VULKAN_GLSL` 不使用、runtime `mUseUBO` flag 単独 gate (= PB-1 / PB-2 / PB-3 で適用済)
- `feedback_handoff_minimal_pre_req_read` — §1.1 3 件のみ厳守
- `feedback_no_scope_shrink` — Phase 1.B Exit Criteria literal「30 setter 全部」厳守
- `feedback_self_verify_before_handoff` — PB-N 検証時に AYA 起動目視前に Claude 「3 経路非到達 verify」を Phase 1.B 後 state で再走
- `feedback_no_claude_coauthor` — 本 handoff doc 含め全 commit 共著行不在
- `feedback_one_step_at_a_time` — PB-6 → PB-4.1 → … → PB-N strict 線形、各 PB-X 単独 commit
- `feedback_ubo_migration_one_at_a_time` — PB-4 内部 17 method / PB-5 内部 13 method 一括禁止、1 method 1 sub-step
- `feedback_doubt_self_first` — PB-3 で literal `getStringHash()` → `Hash()` 修正の根拠、PB-X 中の不可解な build error / link fail は AYA に投げる前に Claude が root cause 特定
- `feedback_admit_unknown` — Vulkan path 動作確認等の判断は推測で進めず source-of-truth (= 06a chapter) literal 再確認
- `feedback_proactive_handoff` — Phase 1.B 全終了時に Phase 1.C entry handoff doc 起案
- `feedback_no_auto_commit` — 本 handoff doc + PB-3 commit + 各 PB-X commit は AYA 明示指示後 commit
- `feedback_remove_verification_logs` — PB-X 実装中の `LL_INFOS` hook は commit 前必ず除去、debug `llassert` は仕様内ゆえ残す
- `feedback_build_only_verified` — Phase 1.B Exit Criteria literal 充足 (= viewer launch + 経路非到達 verify) 後 commit
- `feedback_tests_dir_never_commit` — `scripts/ubo_codegen/tests/` は commit 不可
- `feedback_build` — Phase 1.B PB-N full build フローは Claude 全権実行
- `project_build_procedure` — PB-N で参照
- `project_ayastorm_r41_vulkan_migration` — Phase 1.B PB-3 完了 = 次 PB-6 着手状態
- `project_ayastorm_r41_design_principles` — 原則 1「upstream 取込やすさ維持」 = GATE-B 採用根拠
- `project_ayastorm_three_platforms` — Phase 1.B Exit Criteria literal 充足は Linux first-class baseline で十分
- `feedback_use_agents_proactively` — PB-3 grep 抽出 で Agent 投入済 (= §3.2 80 名 list 確定)
- `feedback_explanation_lead_with_conclusion` — AYA 判断項提示時に結論先 (= §3.2 「A で」承認 1 line ACK pattern)

---

## §6 次 session 着手 1 line

**「前 session で Phase 1.B PB-3 LLStaticHashedString 経路補助 cache 構築 完了 (= `indra/llrender/llglslshader.cpp` modified、+47 line = (a) anonymous namespace 末尾 (`L993-1017`) に `g_static_hashed_uniform_names[]` 80 名 `const char* const` 配列追加 + (b) `mapUniforms()` 末尾 PB-2 block 直後 (`L1925-1944`) に `if (mUseUBO) { for (name in g_static_hashed_uniform_names) { lookup_runtime → hash 計算 → mUniformUBOLocByHash 登録 } }` block 追加、GATE-B 整合で `#ifdef LL_VULKAN_GLSL` 不使用 + `if (mUseUBO)` runtime gate 単独、llrender 単体 build PASS + full viewer build PASS + tar.xz package 生成)、PB-3 完了 handoff doc 起案 + AYA 明示指示後 batch commit。本 session = **AYA 判断不要 自走** = **PB-6 着手** (= 順序組替え PB-6 を PB-4 直前に前倒し、本 handoff §3.6) = `forwardToUboUpload()` shell 実装 (= FWD-1 採用、空 stub 一括) = (a) `indra/llrender/llglslshader.h` に method 宣言 `void forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size);` 追加 + (b) `indra/llrender/llglslshader.cpp` 末尾に空 stub `{}` 実装。PB-6 → llrender 単体 build PASS → full viewer build PASS → 独立 commit (= AYA 明示指示後) → 以降 PB-4.1〜.17 (= 17 method integer index、1 method 1 sub-step) → PB-5.1〜.13 (= 13 method hashed string、1 method 1 sub-step) → PB-7 (= 整合 check 仕込み) → PB-N (= MUSEUBO-A `mUseUBO=false` default で full build + viewer launch + bind 不変 verify) と strict 線形進行。」**
