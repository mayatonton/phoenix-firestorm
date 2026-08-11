# #29: pool object 相中 state の全列挙と局所化(defect sweep 第 2 弾)

制定 2026-08-11。台帳 = `docs/vknative_parallel_record_feasibility.md` §6.6 系統②(#29)。
gate = **直列で挙動恒等の純リファクタ**(lane=1 恒等)。

## L0 不変条件
**破れている不変条件**: 「record 相中に pool 系の単一実体 state(class static / file static)を書く」。fork 時に worker 間で共有・衝突する。
**根治形**: ①読者ゼロの state は削除 ②instance member の鏡写しは削除して原本直読み ③生存する cross-function state は thread_local 化(= lane 形・直列では従来と同一 storage 意味論)。

## L1 全列挙(実読トレース済・file:line)
### 対象 3 pool の shadow 相
| pool | begin/end/renderShadow の pool state 書き |
|---|---|
| terrain(lldrawpoolterrain.cpp:176-199) | **ゼロ**(LLFacePool::beginRenderPass = 空 base lldrawpool.cpp:255-257・shader bind/gGL のみ = #3/#2 管轄) |
| tree(lldrawpooltree.cpp:134-226) | **ゼロ**(polygon offset = gGL・setMinimumAlpha = shader #3。renderDeferred の addTextureStats は既知の需要系) |
| avatar(lldrawpoolavatar.cpp) | 下表の static 族(camera 側 pass とも共有) |

### avatar pool の static 族(全数)
| symbol | 種別 | 書き | 読み | 処置 |
|---|---|---|---|---|
| sShadowPass | class static(h:132) | :351/:393 | **ゼロ** | **削除** |
| is_deferred_render | file static(:76) | :175/:201 | **ゼロ** | **削除** |
| is_post_deferred_render | file static(:77) | :266/:275 | **ゼロ** | **削除** |
| sRenderingSkinned | file static(:106) | 13 site | **ゼロ** | **削除** |
| LLVOAvatar::shouldAlphaMask() | method(llvoavatar.h:588) | — | **呼び手ゼロ** | **削除**(sSkipTransparent の唯一の renderSkinned 外読者だった) |
| sShaderLevel | file static(:67) | 全 write が `= mShaderLevel`(prerender :149 ほか) | begin/end 系 8 箇所 | **削除・読みは mShaderLevel 直読み**(全読者 = instance method・全 instance 同値 = prerender で SHADER_AVATAR level を一律取得) |
| sSkipOpaque / sSkipTransparent | class static | pool の begin/render 系 | llvoavatar renderSkinned :5855/:5900 のみ(shouldAlphaMask 削除後) | **thread_local 化** |
| sVertexProgram | class static | 各 begin/end | render() 中間読み(:932 同一性比較/:949 SSS PC)+ end 系 | **thread_local 化** |
| sDiffuseChannel | class static | 各 begin/end | in-file + llviewerjointmesh.cpp:193 | **thread_local 化** |
| normal_channel / specular_channel | **裸グローバル**(:107-108・static すら無し) | :557-558 | :876-886(impostor bind) | **static thread_local 化** |
| sMinimumAlpha | class static | **pipeline.cpp:15943-15979 の save/restore**(impostor 時 0) | setMinimumAlpha 5 site | **不触・#23/#24(PassContext)へ送り**(pass 文脈通信 = sUseOcclusion save/restore と同族) |

## L2 処置の根拠
- 削除 6 件: 読者/呼び手ゼロは grep(単語境界・repo 全域 *.h/*.cpp/*.inl)で機械確認。GL 物理削除の残骸(旧 GL 経路が読者だった)。
- thread_local: §6.6 の「pass ローカル/lane へ移す」の lane 形。直列(main 1 thread)では static と storage 意味論同一 = 挙動恒等が構造的に自明。引数化(renderSkinned への flag threading)は render(2) が deferred-skinned と post-deferred で同番という pass 番号の縮退があり、renderAvatars(公開 API・preview floater 2 呼び手)まで signature 変更が波及するため不採用 → 真の引数化は #23/#24 RecordPassContext が pass 文脈ごと運ぶ時に統合。
- sShaderLevel 等価性: 全 write が mShaderLevel の鏡(grep 全数)+ 全読者が LLDrawPoolAvatar instance method + mShaderLevel は prerender で全 instance 一律 `getShaderLevel(SHADER_AVATAR)`。

## L3 gate
1. フルビルド(.h 変更)+ 3-path deploy。shader 変更なし。
2. 通常走行: 警報全欄ゼロ・validation 0・mdi 系 0・視覚同一(avatar 描画・影・impostor・preview floater)。
3. 恒等の根拠は L1/L2 の構造証明(削除 = 読者ゼロ・TL = 直列同一・mirror = 同値)。走行は反証標本(憲法 6)。

## L4 縮小・省略・解釈申告
1. sMinimumAlpha は不触(#23/#24 送り・上表)。
2. renderSkinned の flag 引数化は不採用(理由 L2)= PassContext 工事で再訪。
3. renderSkinned 配下(llvoavatar/joint 木)の非 pool state(gGL・LLViewerJoint 内部等)は #29 範囲外(#2 gGL lifecycle / #27 palette 済)。
4. terrain drawLoop / tree renderDeferred 内の per-draw 系(establish scratch 等)は既知 PART-safe(#5)= 不触。
5. sShaderLevel 削除で「shader level 変更が prerender を経ず反映される」timing 差は生じない(旧も新も読み時点の値は prerender 後同値・変更は 次 prerender で両者同時反映)。
