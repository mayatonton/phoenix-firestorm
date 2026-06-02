# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-13 退行 revert handoff

**status**: B?-η-13 patch 退行 verdict 確定 → 全 revert → η-12 baseline 再現完了 → 次 sub-bundle B?-η-14 (Path C 設計) 着手境界 fresh context 引継
**branch**: feature/ayastorm-r41-gl-removal
**patch commit**: **無し** (η-13 patch は全 revert、commit せず、η-12 baseline `bf9164ce34` に retreat)
**handoff doc commit**: 本 doc (η-12-complete `7c1762d214` 範式継承、別 commit 予定)
**勤続範式継承**: B?-η-12-complete `7c1762d214` / B?-η-11-complete `74705c35bb` / B?-η-10-complete `9246142639` / B?-η-9-complete `6fee4a818b` / B?-η-8-complete `6994eba271` / B?-η-7-complete `b1e8689634` / B?-η-6-complete `fe757ea624` / B?-η-5-complete `d6afcfaee3` / B?-η-4-complete `a9bfd37c29` / B?-η-3-complete `5b1aa7001f` / B?-η-2 (a)-complete `6924d4b827` / B?-η-1-complete `92e3550dca`

---

## §1 サマリー

η-13 scope = **η-12 末で X 系既達退行 4 件 (nameless block 2 + cloud_scale 1 + non-opaque 増分 1) のうち、cascade root cause を共有する 3 件 (nameless block 2 + cloud_scale 1) まとめ解消** を 1-shot 目標。Path B (FrameAtmosphere_Skybox → FrameAtmosphere_Lighting 統合) で着手 → **退行 verdict 確定** → 全 revert で η-12 baseline retreat。

- **Phase 単一 (退行 detect)**: 10 file 同期 patch (Group A 5 file の Lighting UBO へ haze_horizon + gamma 追加、Group B 5 file の Skybox → Lighting rename)
  - Group A (helper UBO 拡張) = atmosphericsFuncs.glsl + atmosphericsHelpersV/F.glsl + atmosphericsF.glsl + skyF.glsl (5 file)
  - Group B (Skybox → Lighting rename) = skyV.glsl + cloudsV.glsl + postDeferredTonemap.glsl + postDeferredGammaCorrect.glsl + CASF.glsl (5 file)
  - 設計仮説: 同 binding=2 で異なる block 名 (Skybox vs Lighting) + 異なる member 数 (22 vs 20) が cross-stage validation で衝突 → 統一すれば skyF 2 件 + cloud_scale 1 件 = 3 件 cascade 解消
  - 実装は **shader-only patch** で C++ touch 0 (charter §3 #1 担保)、UBO buffer は Skybox layout (22-member) で既存稼働中

**実機 verify 結果** (η-13 patch state, 2026-06-02 08:22):

| metric | η-12 baseline | η-13 result | Δ | 判定 |
|---|---|---|---|---|
| `overlapping use of location` | 0 | 0 | ±0 | ✓ 維持 |
| `nameless block ... global scope` | 2 (`FrameAtmosphere_Skybox`) | **30 (`AtmoExtraUBO_Legacy`)** | **+28** | ✗ **退行 (Y=30 ≫ X=2)** |
| `'cloud_scale' undeclared identifier` | 1 | 0 | -1 | ✓ 解消 |
| `non-opaque uniforms outside a block` | 6 | 4 | -2 | ✓ 改善 |
| ERROR: 0: total | (η-12 比較未計測) | 251 | - | (退行支配) |

**η-11 §3.4 cascade pair shift forward ACCEPT 範式判定**:
- X (主指標解消 + 改善) = 2 + 1 + 2 = **5**
- Y (新規退行) = **30**
- Y ≫ X → **ACCEPT 不可、退行 verdict**

**全 revert 完了** (2026-06-02 08:25):
- 10 file 全て `git diff --stat HEAD` clean (η-12 HEAD `bf9164ce34` と byte-for-byte 一致)
- install dir (`~/ayastorm/app_settings/shaders/`) も η-12 baseline に restore
- shader cache (`~/.ayastorm_x64/cache/shader_cache/`) clear 済
- **次起動で η-12 baseline (nameless block 2 + cloud_scale 1 + non-opaque 6) 完全再現可能**

**新規範式確立**: §3.5 退行 verdict + 全 revert + retreat 範式 (新規)、§3.6 同 binding nameless block member 名衝突診断範式 (新規)、§3.7 4 Path 比較 → AYA judgment 仰ぎ Path A revert + Path C 次案確定範式 (新規)

---

## §2 完遂結果 metric (vs B?-η-12 baseline commit `bf9164ce34`)

| metric | η-12 baseline | η-13 patch | η-13 revert後 (= η-12 baseline 再現) | Δ vs η-12 |
|---|---|---|---|---|
| **overlapping use of location** | **0** | **0** | **0 (再現)** | **±0** ✓ |
| **nameless block ... global scope** | **2 (Skybox)** | **30 (AtmoExtraUBO_Legacy)** ✗ | **2 (Skybox、再現)** | **±0** ✓ |
| **'cloud_scale' undeclared identifier** | **1** | **0** | **1 (再現)** | **±0** ✓ |
| **non-opaque uniforms outside a block** | **6** | **4** | **6 (再現)** | **±0** ✓ |
| Layout location qualifier must match | 0 | 0 | 0 | ±0 ✓ |
| 'binding' | 0 | 0 | 0 | ±0 ✓ |
| Cannot reuse block name | 0 | 0 | 0 | ±0 ✓ |
| 'weight4'/'weight' redefinition | 0/0 | 0/0 | 0/0 | ±0 ✓ |
| Link failed | 0 | 0 | 0 | ±0 ✓ |
| 'size' undeclared | 0 | 0 | 0 | ±0 ✓ |
| GBufferInfo redefinition struct | 0 | 0 | 0 | ±0 ✓ |
| '#' preprocessor | 0 | 0 | 0 | ±0 ✓ |
| SPIR-V requires location | 41 | 41 | 41 | ±0 ✓ |
| 'normalMap' redefinition | 69 | 69 | 69 | ±0 ✓ |
| 'depthMap' redefinition | 9 | 9 | 9 | ±0 ✓ |
| FATAL/SIGSEGV/Aborted | 0/0/0 | 0/0/0 | 0/0/0 | ±0 ✓ |

**η-13 patch state は退行確定**、**revert 後 η-12 baseline 完全再現**、次 sub-bundle η-14 着手 base は η-12 baseline (`bf9164ce34`) で継承。

---

## §3 設計範式

### §3.1 設計範式継承: η-9 §3.1 V/F pair canonical partner 同定範式

本 sub-bundle では V/F pair canonical 同定は **直接適用なし** (本 scope の対象は cross-stage UBO 統合であり、V/F pair の vary 整合ではない)。但し η-9 範式の精神 (canonical 側を変更せず patch 側を整合) は §3.7 Path 比較で参照。

### §3.2 設計範式継承: η-3 §3.2 per-group rename → §3.6 同 binding 多 block member 名衝突診断範式 へ繋ぐ

η-3 で確立した per-group rename 範式 (`FrameAtmosphere_Skybox` / `FrameAtmosphere_Lighting` 分離) は本 η-13 が初めて **逆方向統合 (Skybox → Lighting unification)** を試行した。結果は退行で、**「per-group 分離は維持されるべきだった」** という falsification 結論。η-3 範式自体は維持される。

### §3.3 設計範式継承: η-10 §3.1 handoff doc canonical 記載 着手前再検証範式

η-12 handoff §3 設計範式 + §4.3 root cause 推定 + §10 推奨 scope を着手前に再検証 → 「nameless block 2 件 + cloud_scale 1 件 cascade 共有 root cause = Skybox UBO declaration parse failure chain」と判断。本判断は **shallow analysis の典型例** で、AtmoExtraUBO_Legacy との member 名衝突 (haze_horizon) を見落とした。**§3.6 で新範式として補完**。

### §3.4 設計範式継承: η-11 §3.4 cascade pair shift forward ACCEPT 範式

本 η-13 で **第 3 例の判定に使用**。

| Detection step | 結果 | 判定 |
|---|---|---|
| 1. Y count (既達退行) | **30** (AtmoExtraUBO_Legacy nameless block 衝突) | - |
| 2. X count (主指標解消 + 改善) | **5** (cloud_scale 1 + non-opaque 2 + Skybox nameless 2 = 5) | - |
| 3. 退行 program = patch program | △ **部分一致** (退行 30 program は patch 直接対象外、helper 経由間接対象) | - |
| 4. 退行 root cause = cascade | ✗ **新 root cause (member 名衝突)** = 単純 cascade ではなく **patch member 追加起因の global scope 直接衝突** | - |
| **総合判定** | **Y=30 ≫ X=5 + 退行 root cause が新規 (cascade ではない)** | **退行 verdict、ACCEPT 不可、revert** |

### §3.5 新規範式: 退行 verdict + 全 revert + retreat 範式

**範式定義**: cascade pair shift forward ACCEPT 範式判定が「ACCEPT 不可」となった場合の処理。

**条件 (退行 verdict 確定)**:
1. Y > X (新規退行件数 > 主指標解消件数)、または
2. Y = X だが退行 root cause が新規 (cascade ではない member/block 直接衝突)、または
3. AYA judgment で「ACCEPT 不可、retreat」確定

**処理**:
1. **全 revert** = patch 10 file (Group A + Group B) を η-12 HEAD と byte-for-byte 一致まで戻す
2. **install dir restore** = `~/ayastorm/app_settings/shaders/` も η-12 baseline に restore
3. **cache clear** = `~/.ayastorm_x64/cache/shader_cache/*` 削除
4. **patch commit せず** = retreat 状態を commit ログに残さない、handoff doc のみで記録
5. **handoff doc は `-reverted` suffix** で起草、`-complete` suffix は付けない (η-12 範式逸脱)

**handoff doc 必須項目**:
- patch state 実機 verify metric (退行 evidence)
- revert 完了 evidence (git diff clean、install dir restore、cache clear)
- root cause 詳細 (なぜ patch 設計が誤りだったか)
- 4 Path 比較 (A=revert / B=C++ touch / C=alternative design / D=micro fix)
- AYA judgment 仰ぎ確定 Path
- 次 sub-bundle (η-14) 設計骨子

**適用第 1 例**: 本 η-13。η-12 末で 4 件既達退行を cascade と推定し Path B (UBO 統合) を試行 → AtmoExtraUBO_Legacy 衝突で 30 件爆発 → revert。

### §3.6 新規範式: 同 binding 多 block member 名衝突診断範式

**範式定義**: 同一 binding (例 binding=2) または同一 TU 内に複数の nameless interface block が attach されるとき、各 block 間で **member 名が global scope で衝突しないか** を着手前に精査する範式。

**GLSL 仕様 (glslang 実装)**:
- nameless interface block の member name は **global scope に展開される** (block 名を qualifier として使えない)
- 同 TU 内 2 つ以上の nameless block で同名 member を持つと parse failure: `'X' : nameless block contains a member that already has a name at global scope`

**診断手順** (着手前):
1. patch 対象 UBO の member list を全列挙
2. **同 TU に attach される他 UBO** の member list を全列挙 (auto-attach helper 経由を漏らさない、特に `atmosphericsFuncs.glsl` / `atmosphericsVarsV/F.glsl` 経由の AtmoExtraUBO_Legacy / atmosphericsVars 等)
3. **member 名 cross-check** = patch UBO に追加予定の member が他 UBO に既存しないか確認
4. 衝突発見 → rename or 削除 (Lighting/Skybox 等の意味的設計と整合)

**η-13 適用 (反例)**: AtmoExtraUBO_Legacy (set=3, binding=0) に `haze_horizon` が L93 で既存 → Lighting (set=0, binding=2) に `haze_horizon` 追加 → 同 TU 内 nameless block 2 つで同名 member → 衝突 → 30 program で発火。

**次 η-14 適用**: §7 Path C 設計で本範式を着手前に適用、Lighting UBO に haze_horizon/gamma 追加せず別経路で取得する design を確定。

### §3.7 新規範式: 4 Path 比較 → AYA judgment 仰ぎ確定範式

**範式定義**: 退行 verdict 後の retreat 設計で、複数 Path を AYA に提示し judgment を仰ぐ範式。

**Path 構成 (η-13 例)**:

| Path | 概要 | shader-only | 設計負債 | 推奨度 |
|---|---|---|---|---|
| **A (revert + 再設計)** | η-13 全 revert + η-14 で別 approach (例: Path C) | ✓ | 無し (η-12 baseline 維持) | **★★★** (本 η-13 採用) |
| B (η-13 base + 別 UBO 縮小) | AtmoExtraUBO_Legacy から haze_horizon 削除 + helper 内 usage 切替 | ✗ (C++ buffer size 変更) | charter §3 #1 逸脱 (GL #else path 連動) | ★ |
| C (η-13 base + rename) | Lighting.haze_horizon を sky_haze_horizon に rename + 5 file usage 追従 | ✓ | 意味的冗長 (同値 2 名前) | ★★ |
| D (Lighting helper から haze_horizon のみ削除) | Group A 5 file の haze_horizon 削除、gamma は残存 | ✓ | Group B 5 file の haze_horizon usage を AtmoExtraUBO_Legacy 経由に切替必要 | ★★ |

**判定基準**:
1. shader-only (charter §3 #1 担保)
2. 設計負債最小
3. η-14 で次の問題を誘発しない backwards-clean

**η-13 適用**: AYA judgment 仰ぎ Path A (revert + 再設計) 確定 2026-06-02 → 本 doc 起草。

---

## §4 root cause 詳細分析

### §4.1 patch 設計仮説 (η-12 末時点)

η-12 末で 4 件既達退行を観測:
- `FrameAtmosphere_Skybox nameless block` × 2 (EnvMap fragment + WLSky fragment)
- `'cloud_scale' undeclared identifier` × 1 (WLCloud vertex)
- `non-opaque uniforms outside a block` × 1 (Underwater L1099)

η-13 着手前推定: 上記 3 件 (Skybox 2 + cloud_scale 1) は **同 root cause = Skybox UBO declaration parse failure chain**。

仮説機序:
1. skyV/cloudsV の Skybox UBO (binding=2, 22-member) と helper auto-attach の Lighting UBO (binding=2, 20-member) が同 TU 内で binding 衝突
2. glslang parser が後発の Lighting block を skip し、Skybox の member (sunlight_color 等) を global scope で 重複認識 → nameless block 衝突
3. cloudsV の cloud_scale も parse corruption の cascade で undeclared
4. 解 = 統一 → Skybox → Lighting に rename + member 追加 (haze_horizon + gamma) で 22-member layout を Lighting 名で統一

### §4.2 実装 (Path B)

| File | 変更内容 |
|---|---|
| atmosphericsFuncs.glsl | FrameAtmosphere_Lighting に `float haze_horizon; float gamma;` 追加 (`float max_cof;` と `float _pad_atm0;` の間) |
| atmosphericsHelpersV.glsl | 同上 |
| atmosphericsHelpersF.glsl | 同上 |
| atmosphericsF.glsl | 同上 |
| skyF.glsl | 同上 |
| skyV.glsl | `FrameAtmosphere_Skybox` → `FrameAtmosphere_Lighting` rename + guard `FRAME_ATMOSPHERE_SKYBOX_DEFINED` → `FRAME_ATMOSPHERE_LIGHTING_DEFINED` |
| cloudsV.glsl | 同上 |
| postDeferredTonemap.glsl | 同上 |
| postDeferredGammaCorrect.glsl | 同上 |
| CASF.glsl | 同上 |

### §4.3 退行 root cause (新規発見)

**`atmosphericsFuncs.glsl` L91-102** で `AtmoExtraUBO_Legacy` (set=3, binding=0) が既に **member `haze_horizon` (L93)** を持つ。

```glsl
layout(set=3, binding=0, std140) uniform AtmoExtraUBO_Legacy {
    vec3  lightnorm;
    float haze_horizon;    // ← ここに既存
    float cloud_shadow;
    ...
};
```

η-13 で FrameAtmosphere_Lighting (set=0, binding=2) にも `haze_horizon` を追加した結果:

- atmosphericsFuncs.glsl が auto-attach される全 calculatesAtmospherics program (~30 個) の TU 内に **nameless block 2 つで同名 member `haze_horizon`** が共存
- GLSL 仕様: nameless interface block member は global scope に展開 → 同名は禁止
- glslang error: `'AtmoExtraUBO_Legacy' : nameless block contains a member that already has a name at global scope` × 30 program

**なぜ η-12 では衝突しなかったか**:
- η-12 では Lighting UBO に haze_horizon 無し (20-member)
- AtmoExtraUBO_Legacy.haze_horizon だけが TU 内に存在 → global scope 唯一 → 衝突無し

**なぜ skyV/cloudsV の Skybox.haze_horizon (η-12 で既存) は AtmoExtraUBO_Legacy.haze_horizon と衝突しなかったか**:
- skyV/cloudsV (gEnvironmentMapProgram / gDeferredWLSkyProgram / gDeferredWLCloudProgram) は **calculatesAtmospherics=false の可能性** = atmosphericsFuncs.glsl が auto-attach されない = AtmoExtraUBO_Legacy が TU に存在しない
- 一方 η-13 で helper UBO (Lighting) に haze_horizon 追加 → atmosphericsFuncs/atmosphericsHelpers が auto-attach される ~30 program に haze_horizon が露出 → AtmoExtraUBO_Legacy.haze_horizon と衝突

**η-14 で確認すべき**:
- llviewershadermgr.cpp の各 program 定義で `mFeatures.calculatesAtmospherics` の設定値 (skyV/cloudsV 系の真偽)
- llshadermgr.cpp の atmosphericsFuncs auto-attach 条件 (vertex/fragment 両方)

### §4.4 Path B falsification: per-group 分離は正しい設計だった

η-3 で確立した per-group rename (`FrameAtmosphere_Skybox` / `FrameAtmosphere_Lighting`) は **同 binding=2 で異なる member layout を持つ 2 用途を分離する設計** だった。η-13 で逆方向統合を試みたが、Skybox member (haze_horizon/gamma) を Lighting に取り込むと AtmoExtraUBO_Legacy との衝突が新規発生する → **per-group 分離は維持されるべき**。

η-12 末の Skybox 2 件衝突は別 root cause (skyV → skyF cross-stage validation での member 漏出) であり、η-14 では Skybox UBO declaration を **fragment 側にも declare する** or **Skybox 自体を named block 化する** etc の別 path を検討する。

---

## §5 Path C (η-14 候補設計骨子)

**目的**: η-12 末の 3 件 (nameless block 2 + cloud_scale 1) を解消、non-opaque 1 件は η-14 以降に移管。

**設計概要** (確定前、着手時に §3.6 範式で再検証):

1. **per-group 分離は維持** (Skybox / Lighting 名前空間分離)
2. **Skybox UBO の cross-stage 衝突を別経路で解消**:
   - 案 C-1: **skyF.glsl の Lighting (helper) を `binding=2` から別 binding に移動** = binding 衝突回避、但し C++ side layout 変更要 (shader-only 逸脱)
   - 案 C-2: **skyF/EnvMap fragment に Skybox UBO declaration を追加** = skyV と skyF で同 Skybox UBO を declare、cross-stage で同 layout になり衝突回避。Skybox UBO は guard で de-dup される。**shader-only で完結**
   - 案 C-3: **Skybox UBO を named block 化** = `layout(...) uniform FrameAtmosphere_Skybox { ... } u_sky;` で member 名を qualifier で参照 → global scope 衝突回避、但し全 5 file の haze_horizon usage を `u_sky.haze_horizon` に rename 要 (大規模 patch)
3. **cloud_scale undeclared 解消**:
   - cloudsV.glsl L106-111 の CloudsVParamUBO_Legacy (set=3, binding=3) が cloud_scale を提供。η-12 末で undeclared になる root cause は Skybox UBO parse 失敗の cascade と推定 → 案 C-2 で skyV/cloudsV の Skybox cross-stage 衝突が消えれば cloudsV vertex parse 自体が成功し cloud_scale も解決される可能性
   - 案 C-2 で改善しなければ別途 patch (CloudsVParamUBO_Legacy の declaration 位置・guard を見直し)
4. **non-opaque 1 件 (Underwater L1099)**:
   - 別 root cause 想定 (Underwater 専有 uniform)、η-14 scope 外、η-15 以降に移管

**推奨候補**: **案 C-2 (skyF/EnvMap に Skybox UBO declaration 追加)**
- shader-only 完結 (charter §3 #1 担保)
- 設計負債小 (Skybox UBO の同 layout を fragment 側にも declare、guard で de-dup)
- AtmoExtraUBO_Legacy との衝突無し (Skybox は haze_horizon を持つが Lighting には無いので、helper 経由でも同名衝突しない…ただし要再検証 = atmosphericsFuncs/atmosphericsVarsF の auto-attach 対象に skyF/EnvMap が含まれるか確認要)

**η-14 着手前必須 step**:
1. **§3.6 範式適用** = patch 対象 UBO と attach 他 UBO の member 名 cross-check
2. llshadermgr.cpp の auto-attach 条件確認 (skyV/cloudsV/postDeferredTonemap 等の `calculatesAtmospherics` flag 値)
3. llviewershadermgr.cpp gEnvironmentMapProgram / gDeferredWLSkyProgram / gDeferredWLCloudProgram の feature flag 値確認
4. 案 C-2 適用後の TU 内 UBO list を予想 → 衝突予測 → 着手前 paper review

---

## §6 ファイル変更一覧 (η-13 patch state、revert で全て元に戻り)

| File | η-13 patch 内容 | revert 後 state |
|---|---|---|
| `indra/newview/app_settings/shaders/class1/windlight/atmosphericsFuncs.glsl` | Lighting UBO に haze_horizon + gamma 追加 | η-12 HEAD と byte 一致 (`bf9164ce34`) |
| `indra/newview/app_settings/shaders/class1/windlight/atmosphericsHelpersV.glsl` | 同上 | 同上 |
| `indra/newview/app_settings/shaders/class1/windlight/atmosphericsHelpersF.glsl` | 同上 | 同上 |
| `indra/newview/app_settings/shaders/class1/windlight/atmosphericsF.glsl` | 同上 | 同上 |
| `indra/newview/app_settings/shaders/class1/deferred/skyF.glsl` | 同上 | 同上 |
| `indra/newview/app_settings/shaders/class1/deferred/skyV.glsl` | Skybox → Lighting rename + guard 変更 | 同上 |
| `indra/newview/app_settings/shaders/class1/deferred/cloudsV.glsl` | 同上 | 同上 |
| `indra/newview/app_settings/shaders/class1/deferred/postDeferredTonemap.glsl` | 同上 | 同上 |
| `indra/newview/app_settings/shaders/class1/deferred/postDeferredGammaCorrect.glsl` | 同上 | 同上 |
| `indra/newview/app_settings/shaders/class1/deferred/CASF.glsl` | 同上 | 同上 |

**revert evidence**:
```bash
$ git diff --stat HEAD -- <10 file paths>
# (出力空 = 10 file 全て HEAD と一致)
```

`~/ayastorm/app_settings/shaders/` も η-12 baseline (`bf9164ce34`) と同一に restore 済、shader cache clear 済。

---

## §7 η-14 着手者向け引継チェックリスト

### §7.1 baseline 確認

- [ ] `git log --oneline | head -5` で HEAD = η-12 patch commit `bf9164ce34` を確認 (η-13 commit は無い)
- [ ] `git diff --stat HEAD -- <10 file>` で η-13 revert 完全性確認 (出力空)
- [ ] `~/ayastorm/app_settings/shaders/` が η-12 baseline と一致 (md5sum 等で必要に応じて検査)
- [ ] cold cache 再起動で η-12 baseline metric (nameless block 2 + cloud_scale 1 + non-opaque 6) 再現確認

### §7.2 η-14 設計

- [ ] §5 案 C-2 (skyF/EnvMap に Skybox UBO declaration 追加) を **§3.6 範式で着手前精査**
- [ ] llshadermgr.cpp の atmosphericsFuncs / atmosphericsVarsF / atmosphericsHelpersF auto-attach 条件確認
- [ ] llviewershadermgr.cpp の sky/cloud 3 program の feature flag 確認
- [ ] 案 C-2 で TU 内 UBO list を予想、AtmoExtraUBO_Legacy.haze_horizon と Skybox.haze_horizon が同 TU 内に並ぶか確認 → 並ぶなら案 C-2 も §4.3 と同種 30 件衝突を再発するため、案 C-3 (named block) or 他案を検討
- [ ] charter §3 #1 (shader-only、GL #else 不変) 担保
- [ ] η-3 §3.2 per-group rename 範式維持
- [ ] η-9 §3.1 V/F pair canonical partner 範式 (cross-stage UBO 整合 = V/F pair で同 declaration を持つ点で同精神)

### §7.3 observation point (次 session で先行確認)

**重要**: §5 案 C-2 で **AtmoExtraUBO_Legacy.haze_horizon と Skybox.haze_horizon の TU 共存** が起きる場合、η-13 と同種衝突が再発する。**着手前に必ず確認**:

1. skyV.glsl の現在の Skybox UBO (η-12 HEAD) に haze_horizon が存在 (L132、22-member)
2. もし skyV TU に AtmoExtraUBO_Legacy が attach されているなら、**η-12 baseline でも衝突しているはず** → でも実機 log では skyV の haze_horizon 衝突は無い → skyV の TU には AtmoExtraUBO_Legacy が attach されていない (calculatesAtmospherics=false 強い推定)
3. なら案 C-2 で skyF/EnvMap に Skybox UBO を declare しても、skyF/EnvMap も atmosphericsFuncs 不在 (calculatesAtmospherics=false) なら衝突しない → 安全
4. **但し** skyF/EnvMap の他の helper attach (atmosphericsHelpersF/atmosphericsVarsF) で別 UBO が attach される可能性 → 個別確認

**確認手順**:
```bash
# llshadermgr.cpp の auto-attach 全条件と関連 helper file 一覧を確認
grep -n "atmospherics\|Sky\|Cloud" indra/llrender/llshadermgr.cpp | head -50

# llviewershadermgr.cpp の sky 3 program の feature flag 設定を確認
grep -B2 -A20 "gEnvironmentMapProgram\|gDeferredWLSkyProgram\|gDeferredWLCloudProgram" indra/newview/llviewershadermgr.cpp | head -100
```

---

## §8 commit / push 計画

**η-13 patch commit**: **無し** (退行 verdict のため retreat、commit ログに痕跡を残さない)

**η-13 handoff doc commit** (AYA 「commit して」明示指示後、本 session で実施):
```
docs(r41): sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-13 退行 revert handoff 起草

η-13 Path B (Skybox → Lighting unification) 退行 verdict 確定、全 revert で
η-12 baseline (bf9164ce34) 再現完了。

- 退行 evidence: nameless block 2 → 30 (Y ≫ X、ACCEPT 不可)
- root cause: Lighting UBO に追加した haze_horizon が AtmoExtraUBO_Legacy
  (atmosphericsFuncs.glsl L93) の haze_horizon と同 TU 内 nameless block
  global scope 名前衝突 (~30 calculatesAtmospherics program で発火)
- revert state: git diff clean、install dir restore、cache clear 済
- 新規範式 3 件 (§3.5 退行 verdict + revert + retreat、§3.6 同 binding 多
  block member 名衝突診断、§3.7 4 Path 比較 → AYA judgment 仰ぎ確定)
- η-14 着手は Path C (案 C-2 skyF/EnvMap への Skybox UBO declaration 追加)
  を §3.6 範式で着手前精査して確定
```

**push**: AYA 側で実施 (memory feedback_release_flow)

---

## §9 終了条件 / 次 sub-bundle 移行

**本 sub-bundle 終了条件**:
- [x] η-13 patch 退行 verdict 確定 (Y=30 ≫ X=5、§3.4 範式)
- [x] 全 revert 完了 (10 file git diff clean、install dir restore、cache clear)
- [x] handoff doc 起草 (本 doc)
- [ ] handoff doc commit (AYA「commit して」指示後)

**次 sub-bundle (η-14) 着手境界**:
- baseline: η-12 patch commit `bf9164ce34` (η-13 commit 無し)
- scope: §5 案 C-2 (skyF/EnvMap への Skybox UBO declaration 追加) を §3.6 範式で着手前精査 → 確定後 patch
- 目標: η-12 末既達退行 4 件のうち 3 件 (nameless block 2 + cloud_scale 1) 解消、non-opaque 1 件は η-15+ 移管

---

**本 handoff doc は §3.5 新規範式 (退行 verdict + 全 revert + retreat 範式) の第 1 適用例**。η-13 で初めて patch 後の退行 verdict + revert が発生したため、handoff doc 命名・記録項目・commit 方針を新規確立した。今後の η-* で退行 verdict が発生した場合、本 doc を範式テンプレートとする。
