# AYAstorm r42 — Windows 開発者向け handoff

Date: 2026-07-14 / 起点 branch: `dev/ayastorm-vk-3os`(= `e39389fe7d`)
書き手: AYAstorm 設計チーム(Linux 側)。この文書は人間と開発支援 AI の両方が読む前提で書かれている。

---

## 0. 最重要原則(この文書自体にも適用される)

1. **コードが唯一の真実**。この文書を含むあらゆる文書・コメント・過去の記録は「書かれた日の snapshot」であり、HEAD のソースと食い違ったらソースが正しい。判断は必ず実コードの file:line トレースで行うこと。
2. 報告は 3 分類で行う: **VERIFIED**(自分で実行/実読して確認した・file:line や実行ログを添える)/ **CLAIMED**(伝聞・文書由来・未検証)/ **OPEN**(未着手・不明)。「できているはず」を VERIFIED と書かない。
3. **機械的一括置換の禁止**。複数ファイルを script/regex で一括編集しない。編集前に対象ファイルの該当箇所を必ず読む(同名の識別子が別の意味を持つ箇所が実在する)。
4. 完了・done の宣言をしない。上記 3 分類で事実だけ報告する。

## 1. AYAstorm r42 とは(30 秒)

- Firestorm viewer の fork。**r42 で OpenGL を完全に削除し、Vulkan 専用機になった**。Linux では実 GL 呼びゼロ・`ldd` から libGL/libOpenGL 消滅・実機動作検証済み(VERIFIED on Linux)。
- 以後 OpenGL をメンテする計画はない。upstream(Firestorm)からの機能取り込みは継続する。
- 版体裁: 版番号 = **42.0.0**(r 連番がメジャー。実体 = `indra/newview/VIEWER_VERSION_AYA.txt`)/ channel = **`AYAstorm-VK-release`** / based on Firestorm 7.2.4(`VIEWER_VERSION_FS.txt`)・SL 26.1.1(`VIEWER_VERSION.txt`)。版の表示を新設するときは XML への直書き禁止・`LLVersionInfo` 経由(直書きが stale 化した前科あり)。

## 2. あなた(Windows 担当)の任務

**Windows 向けの編集はこれまで一度もコンパイルされていない = 全て CLAIMED。** あなたの仕事はそれを VERIFIED に変えること。

1. **コンパイルを通す**。GL 削除(99 file, -3679 行規模の掃除を含む)の余波で Windows 固有コードにビルドエラーが出る前提で臨むこと。
2. **起動 gate**: 起動 → ログイン画面が出る → ログイン → 左下チャットウィンドウの文字が正常描画される(これが Linux 側で使っている合格基準)。
3. **Vulkan validation layer をクリーンに保つ**(Linux 側は sync validation エラー 0 を基礎ラインにしている。症状追いの前にまず validation エラーを全滅させる、が鉄則)。

## 3. Windows 側の現状(2026-07-14 時点の実装状態)

| 項目 | 状態 | 根拠(file:line は書時点) |
|---|---|---|
| Vulkan WSI(Win32 surface) | 実装済・**CLAIMED** | `vkCreateWin32SurfaceKHR` = `indra/llrender/llvkloader.cpp:7328-7332`、`VK_USE_PLATFORM_WIN32_KHR` 定義 = `indra/cmake/00-Common.cmake:96` |
| GL context 生成の退役 | 実施済・**CLAIMED** | `indra/llwindow/llwindowwin32.cpp`(r42 Phase1 B-②、commit `bb4bc6177e` 系列) |
| GL 能力値(gGLManager)の VK 供給化 | 実施済(共有コード)・Linux VERIFIED | commit `709b8387d8` 系列 |
| Windows でのビルド | **OPEN** | 未実施 |
| 実機動作 | **OPEN** | 未実施 |

- Vulkan loader は **volk による runtime load**(リンク時に vulkan-1.lib を要求しない設計)。`indra/llrender/volk.h/.c` 参照。
- ビルド手順の一般論は `docs/build/building_ayastorm.md`(Linux/Windows 併記・2026-04)を参照。autobuild フロー自体は有効だが、**GL 時代の記述が残っている可能性がある**(食い違ったらコードと CMake が正)。

## 4. 既知の罠(Linux 側で実際に踏んだもの)

- **識別子に `Status` を使わない**。Linux の Xlib が `#define Status int` を漏らすため、共有コードに `Status` という型/変数名を入れると Linux ビルドが壊れる。3 OS 共有コードを編集するときは必ず順守。
- **新しい cvar(設定変数)を追加したら** `settings.xml` に登録し、persistent なものは `Comment` 必須(欠けると "Missing Files" crash)。
- **shader(`indra/newview/app_settings/shaders/`)は GLSL ソースが実行時コンパイルされる**。`#ifdef LL_VULKAN_GLSL` 分岐が Vulkan 用。shader を変更したら実行環境への配布(packaged への copy)を忘れると反映されない。
- **「GL だと問題ないが VK で発火する」族が主要な不具合パターン**。既知 3 族と規約:
  1. UBO 残留 — 共有/ring UBO は生成時ゼロ初期化されない。**write-before-read が規約**(毎フレーム無条件書込み or 消費 draw 直前書込みのどちらかを必ず満たす)。
  2. push constant — offset 重複帯(64-128)があり、**毎 pass/毎 draw の再 push が規約**。set-once は禁止。
  3. sampler 状態 — texture の filter/address は **必ず正規 `LLTexUnit::bind()` 経由**で反映させる。`mCurrImageGL` の手動差し替えや `bindManual` 単独は sampler 残留バグを作る(2026-07-14 に実例を修正済)。
- レンダリングの不具合調査では、runtime 設定(cvar)の切替を証拠にしない。**消費直前にコードで値を固定(ハードコード)した diff でビルドして** A/B すること(cvar は誰がいつ書き換えるか分からない)。

## 5. 開発の進め方(推奨)

1. まず `docs/build/building_ayastorm.md` の Windows 節に従い、**現状のままビルドを試みて全エラーを採取**する(直しながら進めない。全体像が先)。
2. エラーを「Windows 固有ファイル / 共有コード / ビルド系」に分類して報告。共有コードの修正は Linux ビルドを壊し得るので、変更内容を明示すること(こちらで Linux gate をかける)。
3. コンパイルが通ったら起動 gate(§2)→ validation 確認 → 結果を VERIFIED/CLAIMED/OPEN で報告。
4. 不明点・設計判断が必要な点は AYA(mayatonton)へ。この文書と HEAD が食い違ったら、**コードを正としてその旨も報告**してほしい(文書を直す)。
