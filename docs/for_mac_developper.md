# AYAstorm r42 — macOS 開発者向け handoff

Date: 2026-07-15 / 対象 branch: `dev/ayastorm-vk-3os` / 静的監査 base: `4c0fa4a827`
原文: AYAstorm 設計チーム(Linux 側)。2026-07-15 の macOS 静的監査では source/config を実読し、build/run は **OPEN** のまま更新した。この文書は人間と開発支援 AI の両方が読む前提で書かれている。

---

## 0. 最重要原則(この文書自体にも適用される)

1. **コードが唯一の真実**。この文書を含むあらゆる文書・コメント・過去の記録は「書かれた日の snapshot」であり、HEAD のソースと食い違ったらソースが正しい。判断は必ず実コードの file:line トレースで行うこと。
2. 報告は 3 分類で行う: **VERIFIED**(自分で実行/実読して確認した・file:line や実行ログを添える)/ **CLAIMED**(伝聞・文書由来・未検証)/ **OPEN**(未着手・不明)。「できているはず」を VERIFIED と書かない。
3. **機械的一括置換の禁止**。複数ファイルを script/regex で一括編集しない。編集前に対象ファイルの該当箇所を必ず読む。
4. 完了・done の宣言をしない。上記 3 分類で事実だけ報告する。

## 1. AYAstorm r42 とは(30 秒)

- Firestorm viewer の fork。**r42 の描画 backend は Vulkan 専用**。Linux では実 GL 呼びゼロ・実機動作検証済み(VERIFIED on Linux)。macOS では **MoltenVK**(Vulkan→Metal 変換層)経由で動かす設計だが、実機 gate 前の移行 bootstrap として NSOpenGLView/CGL を温存している。したがって「macOS でも OpenGL を完全削除済み」はまだ **OPEN**。
- 以後 OpenGL をメンテする計画はない。upstream(Firestorm)からの機能取り込みは継続する。
- 版体裁: 版番号 = **42.0.0**(実体 = `indra/newview/VIEWER_VERSION_AYA.txt`)/ channel = **`AYAstorm-VK-release`** / based on Firestorm 7.2.4・SL 26.1.1。版の表示を新設するときは XML 直書き禁止・`LLVersionInfo` 経由。

## 2. あなた(macOS 担当)の任務

**mac 向けの MoltenVK groundwork は実装済みだが、mac 実機では一度もビルド・実行されていない = 全て CLAIMED。** あなたの仕事は以下の 3 段:

1. **Vulkan SDK / MoltenVK の導入と packaging 設計**: build 時は Vulkan SDK の header が必要。runtime は volk による dlopen(リンク時依存なし)。まず Vulkan Loader + MoltenVK ICD 方式を基準とし、`libvulkan.dylib`・MoltenVK・ICD JSON を app bundle 内で発見できる配置、runpath、署名を設計する。Loader 経由で MoltenVK を列挙するための portability 対応もこの段階に含める。
2. **実機で surface/swapchain 生成を検証**: 起動 → ログイン画面 → ログイン → 左下チャットウィンドウの文字が正常描画(Linux 側の合格基準と同じ)。
3. **検証後に NSOpenGLView/CGL を退役**: 現状 CAMetalLayer は contentView の **sublayer 方式で NSOpenGLView と共存**している(意図的な移行設計)。VK 描画が実機で安定したら、Linux で実施済みの「GL context 生成退役」(r42 Phase1 B-② パターン、`indra/llwindow/llwindowsdl2.cpp` の履歴が参考例)を mac にも適用する。

## 3. macOS 側の現状(2026-07-15 静的監査時点)

| 項目 | 状態 | 根拠(file:line は書時点) |
|---|---|---|
| Metal surface 生成 | 実装済・**CLAIMED** | `vkCreateMetalSurfaceEXT` = `indra/llrender/llvkloader.cpp:7343-7348`、`VK_USE_PLATFORM_METAL_EXT` 定義 = `indra/cmake/00-Common.cmake:238` |
| CAMetalLayer 生成・resize 追従 | 実装済・**CLAIMED** | `indra/llwindow/llwindowmacosx-objc.mm:235-270`、`indra/llwindow/llwindowmacosx.cpp:414-423` |
| native handles 配線(window→VK) | 実装済・**CLAIMED** | `indra/llwindow/llwindowmacosx.cpp:2524-2536` |
| portability enumeration/subset | **OPEN(未実装)** | instance extensions/flags = `indra/llrender/llvkloader.cpp:455-539`、device extensions = `同:801-885` |
| Loader / MoltenVK packaging | **OPEN(未実装)** | mac manifest に Vulkan/MoltenVK の同梱処理なし |
| NSOpenGLView/CGL | **温存中(意図的)** | VK 実機検証まで退役しない設計 |
| mac でのビルド | **OPEN** | 未実施 |
| 実機動作 | **OPEN** | 未実施 |

- `docs/build/building_ayastorm_macos.md`(2026-05)は **GL 時代の DMG 作成手順**。autobuild フロー・環境構築の参考にはなるが、Vulkan/MoltenVK の記述はなく、configure 例の channel も旧 `AYAstorm-release` のまま。現 branch では `AYAstorm-VK-release` を使い、食い違ったらコードと CMake を正とする。
- この viewer は `VkApplicationInfo::apiVersion` と physical-device gate の両方で **Vulkan 1.3 以上を要求**する(`indra/llrender/llvkloader.cpp:525-531,596-619`)。MoltenVK は「Vulkan 1.2 相当」と決め打ちせず、Vulkan 1.3 以上を公開する release を固定する。viewer が使う機能で未対応のものが出た場合は、対処方針を独断で決めず報告すること(Linux 側の実装を変える判断があり得る)。
- Vulkan Loader + ICD 方式では、instance 作成時に `VK_KHR_portability_enumeration` と `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR`、device が公開する場合は `VK_KHR_portability_subset` の有効化が必要。現 HEAD にはないため **OPEN** とする。
- portability の一次資料: [MoltenVK README](https://github.com/KhronosGroup/MoltenVK/blob/main/README.md)、[VK_KHR_portability_enumeration](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_portability_enumeration.html)。

## 4. 既知の罠(Linux 側で実際に踏んだもの)

- **識別子に `Status` を使わない**。Linux の Xlib が `#define Status int` を漏らすため、共有コードに `Status` という型/変数名を入れると Linux ビルドが壊れる。3 OS 共有コードを編集するときは必ず順守。
- **新しい cvar(設定変数)を追加したら** `settings.xml` に登録し、persistent なものは `Comment` 必須(欠けると "Missing Files" crash)。
- **shader(`indra/newview/app_settings/shaders/`)**: `#ifdef LL_VULKAN_GLSL` 分岐が Vulkan 用。変更したら実行環境への配布を忘れない。
- **「GL だと問題ないが VK で発火する」族が主要な不具合パターン**。既知 3 族と規約:
  1. UBO 残留 — 共有/ring UBO は生成時ゼロ初期化されない。**write-before-read が規約**。
  2. push constant — offset 重複帯(64-128)があり、**毎 pass/毎 draw の再 push が規約**。set-once 禁止。
  3. sampler 状態 — texture の filter/address は**必ず正規 `LLTexUnit::bind()` 経由**。手動の状態差し替えは残留バグを作る(2026-07-14 に実例修正済)。
- レンダリングの不具合調査では cvar 切替を証拠にしない。**消費直前にコードで値を固定した diff でビルドして** A/B すること。
- MoltenVK 固有: 検証時は `AYASTORM_VK_VALIDATION=1` で viewer 側の validation 要求を有効化し、`MVK_CONFIG_LOG_LEVEL` 等の MoltenVK ログと Vulkan validation layer の両方を見ること(変換層起因か viewer 起因かの切り分けが最初の分岐)。使用する変数値は固定した MoltenVK release の文書に合わせ、実行ログに残す。

## 5. 開発の進め方(推奨)

1. `docs/build/building_ayastorm_macos.md` は Xcode・autobuild・DMG 作成の基礎として参照するが、そのまま実行しない。configure 前に full Xcode、Vulkan SDK(header / loader / validation layer)、Vulkan 1.3 以上を公開する MoltenVK release を準備し、version と入手元を記録する。configure の channel は `AYAstorm-VK-release` を使う。
2. source を修正する前に baseline build を一度実行し、完全な configure/build log を保存する。「全エラー」を保証するのではなく、最初に現れる根因群を「環境・configure / compile・link / Loader・ICD / mac 固有(llwindowmacosx 系) / 共有コード / packaging・signing」に分類して報告する。共有コードの修正は Linux build を壊し得るので変更内容を明示(こちらで Linux gate をかける)。
3. 起動検証前に portability enumeration/subset、Loader + MoltenVK ICD の bundle 配置、runpath、universal architecture、license 同梱、code signing を実装・確認する。
4. 起動 gate は一括判定せず、`volkInitialize` → `vkCreateInstance` → MoltenVK physical-device 列挙 → `vkCreateDevice` → Metal surface → swapchain → first frame → ログイン画面 → ログイン → 左下チャット文字描画の順に、各段階の log と VERIFIED/CLAIMED/OPEN を報告する。validation/MVK log も同時に保存する。
5. package gate では、Vulkan SDK を system install していない別環境でも署名済み app/DMG が起動できることを確認する。`file` / `lipo -archs` / `otool -L` / `codesign --verify --deep --strict --verbose=2` の結果を添付する。
6. NSOpenGLView/CGL 退役は**上記の実機 gate 通過後**に別変更として着手し、同じ gate を再実行する(先にやらない)。
7. 不明点・設計判断は AYA(mayatonton)へ。この文書と HEAD が食い違ったら、**コードを正としてその旨も報告**してほしい(文書を直す)。
