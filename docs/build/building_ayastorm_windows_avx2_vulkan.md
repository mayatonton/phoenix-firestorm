# AYAstorm Windows x64 AVX2 / Vulkan ビルド

最終点検日: 2026-08-31 / build baseline checkout: `feature/ayastorm-r42-phase2` / `ffe5f5cf62`

この文書は、AYAstorm の Windows x64 AVX2 版をローカルでビルドし、最終的に
インストーラー `.exe` を作るための準備手順と、この PC の点検結果をまとめたものです。
2026-08-31 に **環境点検、前提ツール導入、FMOD + Opus の Windows package 作成、
viewer 本体の configure、AVX2 build、NSIS package 生成まで完了**した build baseline を記録しています。
この baseline は `ffe5f5cf62` と当時の未コミット Vulkan/MSVC 差分に対するもので、その後の
Windows 開発版プロファイル分離修正（`5a7132e57b`）は含みません。

## 1. 結論

この PC は CPU、GPU、MSVC、Windows SDK、メモリ、空き容量に加え、CMake、Python/autobuild、
Cygwin、NSIS、Vulkan SDK/glslang まで準備できました。

AYAstorm 用 FMOD + Opus の Windows autobuild package と `my_autobuild.xml` も準備済みです。
記録上の判定は **Windows x64 AVX2 build / package 成功**です。生成バイナリの静的監査まで完了して
いますが、viewer の起動、ログイン、Vulkan 描画、音声、CEF の実行試験はまだ行っていません。

## 2. 旧 Windows 手順との差分

既存の `docs/build/building_ayastorm.md` の Windows 節は 2026-04-29 頃の内容を基礎にしており、
Vulkan 移行後に必要になった SDK と package discovery が書かれていません。

現行ソースでは次が無条件に組み込まれます。

- `indra/llrender/CMakeLists.txt` が `Vulkan.cmake` と `Glslang.cmake` を include
- Windows では CMake の `FindVulkan` により Vulkan headers を検索
- `find_package(glslang CONFIG REQUIRED)` により glslang を検索
- `VK_USE_PLATFORM_WIN32_KHR` を定義して Win32 Vulkan surface を有効化
- `--avx2` により MSVC の `/arch:AVX2` を全体へ追加

Vulkan 専用の configure switch はありません。Vulkan 対応は現行バイナリに組み込まれ、
`RenderBackend` の既定値は `1`（Vulkan + fallback）です。旧 OpenGL 用の設定、コメント、
fallback コードは一部残っていますが、旧手順のまま Vulkan SDK なしで configure できる構成では
ありません。

`indra/cmake/SpirvCross.cmake` も存在しますが、現時点ではどの CMakeLists からも include されて
いません。このため `spirv_cross_c_shared` は現在の viewer configure の直接必須項目ではありません。
将来 UBO codegen を配線すると必要になるため、Vulkan SDK 導入後の存在確認だけ行います。

### 2.1 Linux / macOS との依存差分再点検

`autobuild.xml`、3 OS の build 文書、CMake の include、runtime copy/manifest を突き合わせました。
Linux/macOS build にある共通機能が、Windows package の未登録だけを理由に欠ける状態はありません。

| 機能・依存 | Linux / macOS | Windows r42 の扱い | 判定 |
| --- | --- | --- | --- |
| Vulkan headers / loader | Linux は system、macOS は `vulkan_sdk_macos` | LunarG Vulkan SDK 1.4.350.0 を local 導入 | 導入済み |
| glslang / SPIR-V tools | platform SDK / package | LunarG Vulkan SDK の CMake package を使用 | 導入済み |
| FMOD + Opus | OS 別の AYAstorm package | 2.03.07 Windows local package を作成し `my_autobuild.xml` に登録 | 準備済み |
| t-noami/dullahan audio callback | 3 OS package 登録済み | `dullahan_aya_audio` の Windows package を使用 | 登録済み、明示フラグ必須 |
| OpenAL + ALUT | macOS 手順は `--openal` を明示 | Windows package と DLL copy が実装済み | 登録済み、今回明示化 |
| Vulkan GLTF data | `common` package | 同じ `common` package を使用 | 登録済み |
| CEF / WebRTC / SLVoice / OpenXR | OS 別 package | Windows64 package を CMake が取得 | 登録済み |
| Discord | macOS/Windows package metadata あり | CMake 統合は全 OS で現在コメントアウト | Windows 固有の欠落ではない |

Linux の GLib、GStreamer、Mesa、SDL、jemalloc、libuuid などは Linux 固有依存です。macOS の
MoltenVK、framework、codesign/notarization、universal/arm64 設定も macOS 固有です。これらを
Windows build に追加する必要はありません。逆に Windows は Win32 Vulkan surface、Windows SDK、
MSVC runtime、NSIS を使います。

既存 Linux 手順は Vulkan/glslang と Dullahan callback の記載がなく、現行 r42 の完全な比較元には
できません。今回の確認ではソースと `autobuild.xml` を優先し、より新しい macOS 手順も照合しました。
GitHub Actions の Windows job も Vulkan SDK 導入と Dullahan callback の明示がないため、現行 r42
Windows Vulkan build の成立証明には使いません。

## 3. この PC の点検結果

| 項目 | 観測結果 | 判定 |
| --- | --- | --- |
| OS | Windows x64、version `10.0.26200.0` | OK |
| CPU | Intel Core i9-10900K、20 logical processors | OK |
| AVX / AVX2 | Windows `IsProcessorFeaturePresent(39/40)` が両方 `True` | OK |
| RAM | 31.9 GiB、点検時 18.1 GiB available | OK |
| E: 空き容量 | 330.8 GiB | OK |
| GPU | NVIDIA GeForce RTX 3070、driver 591.86 | OK |
| Vulkan runtime | loader 1.4.321、GPU API 1.4.325、`vulkaninfo --summary` 成功 | OK |
| Git | 2.54.0.windows.1 | OK |
| Git LFS | 3.7.1 | OK（この repo に submodule/LFS 必須物はなし） |
| Visual Studio | Build Tools 2022 17.14.31 | OK |
| MSVC | x64 compiler 19.44.35226 | OK |
| MSBuild | 17.14.40.60911 | OK |
| Windows SDK | 10.0.26100.0 / component 10.0.26100.7705 | OK |
| CMake | standalone 4.4.2、`C:\Program Files\CMake\bin` は system PATH 登録済み | OK |
| Python / pip | Python 3.13.15 x64、venv 内 pip 26.2.1 | OK |
| Cygwin / patch | bash 5.2.21、findutils 4.11.0、patch 2.8 | OK |
| NSIS | `C:\Program Files (x86)\NSIS\makensis.exe` v3.12 | OK |
| Vulkan SDK | `E:\Ayastorm_build-deps\VulkanSDK\1.4.350.0`、user 環境変数/PATH 登録済み | OK |
| glslang development package | glslang 16.2.0、SPIRV-Tools v2026.2、CMake metadata と libraries を確認 | OK |
| autobuild | repo `.venv` 内 autobuild 3.10.2 | OK |
| fs-build-variables | `E:\Ayastorm_build-deps\fs-build-variables`、commit `c17312d38377c003977cf5b6dcc83c6042eba194` | OK |
| FMOD package | `fmodstudio-2.03.07-windows64-262400856.tar.bz2`、`my_autobuild.xml` 登録済み | OK |
| t-noami/dullahan | Windows `audio-callback.5` のURL/hashは `autobuild.xml` に登録済み | staging 同梱確認済み |
| OpenAL / ALUT | Windows 1.24.2-r1 package のURL/hashは `autobuild.xml` に登録済み | staging 同梱確認済み |
| build tree | `build-vc170-64`、Release viewer、AVX2 Setup を生成 | OK |

補足:

- Visual Studio Community 2022 もありますが、C++ toolchain は Build Tools 2022 側が完全です。
  `AUTOBUILD_VSVER=170` と Build Tools 側の `vcvars64.bat` を使います。
- `PYTHONUTF8=1` を設定した `autobuild source_environment` が Build Tools 2022、MSVC 14.44.35207、
  Windows SDK 10.0.26100.0 を選ぶことを 2026-08-28 に実行確認済みです。日本語 Windows ではこの設定が
  ないと `vswhere` の JSON を cp932 として decode して失敗するため、省略できません。
- VS 同梱 CMake 3.31.6 ではなく、upstream の `doc/building_windows.md` が要求する 4.1.2 以上を満たす
  standalone CMake 4.4.2 を使います。
- `vulkaninfo` は OBS overlay layer の API version 警告を 1 件出します。driver/ICD の列挙自体は
  成功しており build blocker ではありません。実行時問題が出る場合は overlay を無効化して再確認します。
- copy-only SDK の `vulkaninfoSDK` は SDK layer を machine 登録していないため layer manifest の registry
  lookup 警告も出しますが、Vulkan instance と RTX 3070 の列挙は成功します。build blocker ではありません。
  validation layer を実行時検証に使う場合は、別途 SDK の layer path を明示します。
- Git の `core.autocrlf` は `true` ですが、この repo の `.gitattributes` は原則 `eol=lf` を強制し、
  点検開始時の worktree は clean でした。将来の clone 用には `core.autocrlf=false`、Windows long path 用には
  `core.longpaths=true` を推奨します。

### 3.1 依存導入の試行記録

2026-08-15 に Terra agent で依存導入を開始し、`fs-build-variables` の取得まで完了しました。
その時点で実行中だった VS Code user updater が Windows Installer を使用しており、`winget` による
CMake 導入が installer mutex 待ちになりました。既存 updater、`msiexec`、`winget` は強制終了せず、
CMake、Python、Cygwin、NSIS、Vulkan SDK の導入をいったん保留しました。

VS Code 更新完了後に Terra agent を再実行し、CMake、Python/venv/autobuild、Cygwin/patch、NSIS を
導入しました。Vulkan SDK の通常 installer は完全な管理者 token を得られず空の `C:\VulkanSDK` だけを
残したため、署名と SHA-256 を確認した同じ公式 installer の `copy_only=1` で
`E:\Ayastorm_build-deps\VulkanSDK\1.4.350.0` へ導入しました。空の `C:\VulkanSDK` は確認後に削除済みです。

## 4. 必要物の導入

インストーラーを使う作業だけ管理者権限で行います。実際の configure/build は通常権限の
**x64 Native Tools Command Prompt for VS 2022** で行います。PowerShell は build shell に使いません。

### 4.1 既に揃っているもの

Visual Studio Installer の Build Tools 2022 には、次の component が導入済みです。

- Desktop development with C++
- MSVC v143 x64/x86 build tools
- C++ CMake tools for Windows
- Windows 11 SDK 10.0.26100

再インストールは不要です。公式参照:
[Microsoft C++ command-line tools](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170)

### 4.2 CMake

[CMake Downloads](https://cmake.org/download/) から x64 版 4.1.2 以上を導入し、
`C:\Program Files\CMake\bin` を system PATH に追加します。

この PC では standalone CMake 4.4.2 を導入済みです。

```cmd
where cmake
cmake --version
```

### 4.3 Python 3 と autobuild

[Python for Windows](https://www.python.org/downloads/windows/) から 64-bit Python 3 を導入し、
`pip` と PATH 追加を有効にします。Microsoft Store の app execution alias は無効にします。

この PC では Python 3.13.15 x64 と repo 内 `.venv` を導入済みで、venv 内の pip は 26.2.1、
autobuild は 3.10.2 です。

repo 内の venv を使います。`.venv/` は gitignore 済みです。

```cmd
cd /d E:\Ayastorm_build
python -m venv .venv
call .venv\Scripts\activate.bat
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
autobuild --version
```

### 4.4 Cygwin 64

[Cygwin](https://www.cygwin.com/install.html) を `C:\cygwin64` へ導入し、追加 package
`Devel/patch` を選びます。`C:\cygwin64\bin` を PATH に追加します。

この repo の `autobuild.xml` は configure/build command として `bash` を呼び、Windows 分岐では
`/usr/bin/find` も使います。Git Bash があっても、既存の Firestorm Windows baseline に合わせて
Cygwin を使います。

この PC では `C:\cygwin64` に導入済みで、bash 5.2.21、findutils 4.11.0、patch 2.8 を
sandbox 外の通常 Windows process で実行確認済みです。

```cmd
where bash
where patch
bash --version
patch --version
```

### 4.5 NSIS

[NSIS](https://nsis.sourceforge.io/Download) を導入します。viewer 本体だけなら後回しにできますが、
今回の最終目的である Setup `.exe` の作成には必要です。

この PC では NSIS v3.12 を導入済みです。

```cmd
where makensis
makensis /VERSION
```

### 4.6 Vulkan SDK

[LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home) の Windows x64 SDK を導入します。
この branch は Windows SDK version を repo 内で pin していません。最初の候補は、点検日時点の
公式版 `1.4.350.0` とします。
SDK更新を configure と build の途中で行ってはいけません。

点検日時点の公式 download listing にある Windows x64 installer の SHA-256 は
`855b27ba05d2d8119c5114c5d4ff870ca38f2c632b11e1bb9923b9b7e6ecfe7b` です。

```cmd
certutil -hashfile vulkansdk-windows-X64-1.4.350.0.exe SHA256
```

この PC では公式 installer の `copy_only=1` を使用し、SDK を
`E:\Ayastorm_build-deps\VulkanSDK\1.4.350.0` へ配置しました。user scope の `VULKAN_SDK` と
`VK_SDK_PATH` はこの path、user PATH はその `Bin` を指します。新しい command prompt を開いてから
確認します。SDK と GPU driver は別物であり、この PC の Vulkan runtime/driver は既に動作しています。

```cmd
echo %VULKAN_SDK%
vulkaninfoSDK --summary
dir "%VULKAN_SDK%\Include\vulkan\vulkan.h"
dir "%VULKAN_SDK%\Lib\cmake\glslang\glslang-config.cmake"
dir "%VULKAN_SDK%\Lib\glslang.lib"
dir "%VULKAN_SDK%\Lib\SPIRV.lib"
dir "%VULKAN_SDK%\Lib\glslang-default-resource-limits.lib"
```

1.4.350.0 の copy-only 展開物では glslang/SPIRV-Tools の CMake metadata が package directory 内の
一段深い場所にあり、そのままでは `CMAKE_PREFIX_PATH=%VULKAN_SDK%` から標準 discovery できません。
metadata を同じ SDK 内の `Lib\cmake\glslang` と `Lib\cmake\SPIRV-Tools*` の期待位置へ複製済みです。
`glslang-targets.cmake` の `_IMPORT_PREFIX` が SDK root となり、`Lib\glslang.lib`、`Lib\SPIRV.lib`、
`Lib\glslang-default-resource-limits.lib` を指すことも確認済みです。

参考として SPIRV-Cross も確認します。なければ現行 viewer configure の blocker にはしません。

```cmd
dir "%VULKAN_SDK%\Lib\cmake\spirv_cross_c_shared\spirv_cross_c_shared-config.cmake"
```

Vulkan SDK の導入仕様:
[Getting Started with the Windows Vulkan SDK](https://vulkan.lunarg.com/doc/view/latest/windows/getting_started.html)

## 5. AYAstorm 固有依存の準備

### 5.1 fs-build-variables

source tree の外に依存 checkout を置きます。

```cmd
mkdir E:\Ayastorm_build-deps
cd /d E:\Ayastorm_build-deps
git clone https://github.com/FirestormViewer/fs-build-variables.git
```

この PC では checkout 済みです。使用 commit は
`c17312d38377c003977cf5b6dcc83c6042eba194` です。

### 5.2 FMOD + Opus

AYAstorm は標準 Firestorm と異なり、FMOD package に `opus.dll`、`opus.lib`、Opus headers を含む
`mayatonton/3p-fmodstudio` fork を必要とします。

この build では **FMOD 2.03.07** に固定します。根拠は次のとおりです。

- 3p fork の HEAD `477cf16d75e53e4e534ae5926b595b04f06db8fb` が `FMOD_VERSION=20307` を明示
- viewer の `autobuild.xml` metadata と Linux local package も 2.03.07
- Windows local URL の 2.03.12 は、別 PC 固有の path を含む過去の local package 記録

したがって 2.03.12 の Windows URL は再利用せず、この PC で 2.03.07 package を作り直します。

3p fork は `E:\Ayastorm_build-deps\3p-fmodstudio` に上記 commit で clone 済みです。Windows 上の
`vswhere -latest` が C++ component のない Visual Studio Community を選ぶ問題を避けるため、C++ x64
tools を条件に Build Tools を検索する patch を適用済みです。また、展開先 directory と同名の `.exe` を
Cygwin の `cygpath` が誤選択する問題と、Windows ACL 上で `cp --preserve=mode` が失敗する問題を避ける
patch も適用済みです。再現用 patch は
[`patches/3p-fmodstudio-windows-vswhere.patch.txt`](patches/3p-fmodstudio-windows-vswhere.patch.txt) と
[`patches/3p-fmodstudio-windows-installer-path.patch.txt`](patches/3p-fmodstudio-windows-installer-path.patch.txt)、
[`patches/3p-fmodstudio-windows-copyflags.patch.txt`](patches/3p-fmodstudio-windows-copyflags.patch.txt)
に保存しています。patched script は Bash 構文確認済みで、`dumpbin.exe` と `lib.exe` を含む
MSVC 14.44.35207 を選びます。

3p recipe は Bash script の LF 改行を前提とします。この clone では repo-local
`core.autocrlf=false` を設定済みで、意図した未 commit 差分は `build-cmd.sh` の上記 patch だけです。
新しく作り直す場合は checkout 前から LF を維持します。

```cmd
cd /d E:\Ayastorm_build-deps
git clone -c core.autocrlf=false https://github.com/mayatonton/3p-fmodstudio.git
cd 3p-fmodstudio
git checkout 477cf16d75e53e4e534ae5926b595b04f06db8fb
git apply E:\Ayastorm_build\docs\build\patches\3p-fmodstudio-windows-vswhere.patch.txt
git apply E:\Ayastorm_build\docs\build\patches\3p-fmodstudio-windows-installer-path.patch.txt
git apply E:\Ayastorm_build\docs\build\patches\3p-fmodstudio-windows-copyflags.patch.txt
bash -n build-cmd.sh
```

Windows 版 FMOD Studio API 2.03.07 は [FMOD Downloads](https://www.fmod.com/download) から取得し、
次へ配置済みです。installer と認証情報は repo に commit しません。

```text
E:\Ayastorm_build-deps\3p-fmodstudio\fmodstudioapi20307win-installer.exe
```

この installer の Authenticode signer は `Firelight Technologies Pty Ltd` ですが、証明書は
2025-08-03 失効、timestamp なしのため、2026-08-28 現在の Windows 検証結果は `0x800B0101` です。
公式 download から取得したことを確認し、利用者の明示承認後に実行しました。記録した SHA-256 は
`0556939aeeb62352679a44ae11ffaf0b18a5b72a924bc6e2005a65d3cf268c27` です。

package 作成は次の手順で完了済みです。

```cmd
cd /d E:\Ayastorm_build
call .venv\Scripts\activate.bat
set PYTHONUTF8=1
set AUTOBUILD_VSVER=170
set AUTOBUILD_VARIABLES_FILE=E:\Ayastorm_build-deps\fs-build-variables\variables
cd /d E:\Ayastorm_build-deps\3p-fmodstudio
dir fmodstudioapi20307win-installer.exe
autobuild build -A 64 --all
autobuild package -A 64 --results-file result.txt
type result.txt
```

作成結果:

```text
file: E:\Ayastorm_build-deps\3p-fmodstudio\fmodstudio-2.03.07-windows64-262400856.tar.bz2
size: 2551887 bytes
md5: 29be522b78820157e28e09654d2f3bcd
sha256: 0dfbfd3e6f8ccfa437033cb77c9030c7e837bcc349a5df67c19608137aa4f40a
```

archive は 27 entries で、release/debug の `fmod.dll` / `fmodL.dll`、FMOD import libraries、
`opus.dll`、生成した `opus.lib`、FMOD/Opus headers、licenses を確認済みです。`opus.dll` には
`opus_decode`、`opus_multistream_decode`、`opus_multistream_decoder_create` が export され、生成した
import library にも対応 symbol があることを `dumpbin` で確認しました。

source tree の tracked `autobuild.xml` は変更せず、gitignore 済みの `my_autobuild.xml` に登録済みです。

```cmd
cd /d E:\Ayastorm_build
copy /Y autobuild.xml my_autobuild.xml
set AUTOBUILD_CONFIG_FILE=E:\Ayastorm_build\my_autobuild.xml
autobuild installables edit fmodstudio platform=windows64 hash=29be522b78820157e28e09654d2f3bcd hash_algorithm=md5 url=file:///E:/Ayastorm_build-deps/3p-fmodstudio/fmodstudio-2.03.07-windows64-262400856.tar.bz2
```

`autobuild installables print fmodstudio` で version、Windows64 URL、MD5 を読み返し確認済みです。

### 5.3 t-noami/dullahan audio callback

`autobuild.xml` には upstream `secondlife/dullahan` と、CEF/MOAP audio callback 対応の
`t-noami/dullahan` fork が別 installable として登録済みです。Windows build で使うのは次の
tracked package です。

```text
installable: dullahan_aya_audio
release: v1.26.0-CEF_139.0.40-ayastorm-audio-callback.5
platform: windows64
sha1: d77d2b871524b3a542554719b65dd8dff7b09df1
```

この package は公開 GitHub release URL から autobuild が取得するため、FMOD のようなローカル
package 化は不要です。ただし `LL_DULLAHAN_AUDIO_CALLBACK` の CMake default は `OFF` です。
configure で必ず `-DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE` を指定します。

`indra/cmake/CEFPlugin.cmake` はこの値が `TRUE` のときだけ `dullahan_aya_audio` を選び、upstream
`dullahan` を uninstall して両 package の file 衝突を避けます。フラグがない場合は upstream
`secondlife/dullahan` が選ばれ、CEF/MOAP audio callback 経路は compile out されます。

### 5.4 OpenAL / ALUT

macOS の現行手順は `--openal` を明示しています。Windows 側にも同じ OpenAL Soft package があり、
`indra/cmake/OPENAL.cmake` の `USE_OPENAL` default も `ON` ですが、OS 間の意図を揃えるため Windows
configure にも `--openal` を明示します。

```text
installable: openal
release: 1.24.2-r1
platform: windows64
sha1: 8ad24fba1191c9cb0d2ab36e64b04b4648a99f43
runtime: OpenAL32.dll, alut.dll
```

この package は公開 GitHub release URL から autobuild が取得します。FMOD と違い、ローカル package
化は不要です。Windows では FMOD が compile されている場合に audio engine として先に選択されるため、
OpenAL は FMOD 初期化失敗後の自動 fallback ではなく、別 engine を使う構成と package parity のための
依存として扱います。

## 6. build shell の環境

新しい **x64 Native Tools Command Prompt for VS 2022** を開き、毎回次を設定します。
Vulkan SDK 内の CMake packages を `find_package(glslang CONFIG REQUIRED)` から見つけるため、
`CMAKE_PREFIX_PATH` も明示します。

```cmd
cd /d E:\Ayastorm_build
call .venv\Scripts\activate.bat

set PYTHONUTF8=1
set AUTOBUILD_VSVER=170
set AUTOBUILD_VARIABLES_FILE=E:\Ayastorm_build-deps\fs-build-variables\variables
set AUTOBUILD_CONFIG_FILE=E:\Ayastorm_build\my_autobuild.xml
set CMAKE_PREFIX_PATH=%VULKAN_SDK%
set PATH=C:\Program Files\CMake\bin;C:\Users\Tia Rungray\AppData\Local\Programs\Python\Python313;C:\Users\Tia Rungray\AppData\Local\Programs\Python\Python313\Scripts;C:\cygwin64\bin;%VULKAN_SDK%\Bin;%PATH%
```

`PYTHONUTF8=1` は日本語 locale での `vswhere` JSON decode error を防ぐため必須です。2026-08-28 に
viewer repo と 3p-fmodstudio repo の両方で `autobuild source_environment` が exit code 0 となり、
Visual Studio 2022 Build Tools を選ぶことを確認済みです。

### 6.1 preflight

```cmd
where git
where cmake
where python
where autobuild
where bash
where patch
where find
where cl
where msbuild
where makensis

git --version
cmake --version
python --version
python -m pip --version
autobuild --version
autobuild source_environment > NUL
cl
msbuild -version -nologo
vulkaninfo --summary

echo %AUTOBUILD_VSVER%
echo %AUTOBUILD_VARIABLES_FILE%
echo %AUTOBUILD_CONFIG_FILE%
echo %VULKAN_SDK%
echo %CMAKE_PREFIX_PATH%
```

すべて path/version を返し、`AUTOBUILD_VARIABLES_FILE` と `AUTOBUILD_CONFIG_FILE` が実在して
初めて configure へ進みます。

Git の推奨設定は、通常権限の command prompt で一度だけ行います。現在の clone には
repo-local の `core.longpaths=false` があるため、この repo では local 値を直接更新します。

```cmd
git config --global core.autocrlf false
git config --local core.longpaths true
```

## 7. configure / build / package コマンド

2026-08-31 の build baseline（`ffe5f5cf62` + 当時の未コミット Vulkan/MSVC 差分）で使用した基本コマンドです。
後続の Windows 開発版プロファイル分離修正（`5a7132e57b`）を含む現行 checkout の成果物を示すものではありません。

`ReleaseFS_open` を configure の入口にするのは、入手できない proprietary KDU package を要求せず、
AYAstorm に必要な FMOD、OpenAL、Dullahan audio callback、AVX2 を明示するためです。

```cmd
cd /d E:\Ayastorm_build

autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio --openal --avx2 --package --chan AYAstorm-release -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE
autobuild build -A 64 -c ReleaseFS_AVX2 --no-configure
```

configure では system Python を拾わせず、venv 内の `llsd` / `llbase` を manifest 生成に使うため、
次も明示しました。

```cmd
set VIRTUAL_ENV=E:\Ayastorm_build\.venv
set PYTHON=E:\Ayastorm_build\.venv\Scripts\python.exe

autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio --openal --avx2 --package --chan AYAstorm-release -DLL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE -DPYTHON_EXECUTABLE:FILEPATH=E:/Ayastorm_build/.venv/Scripts/python.exe -DPython3_EXECUTABLE:FILEPATH=E:/Ayastorm_build/.venv/Scripts/python.exe
```

`scripts/configure_firestorm.sh` 自体が `LL_TESTS=OFF` を CMake に渡すため、旧手順の
`-DLL_TESTS:BOOL=FALSE` は重複しており省略します。

既存 `build-vc170-64` を作り直す必要が出た場合でも、削除前に path を確認し、必要なログを退避します。
この文書では削除 command を定型化しません。

### 7.1 Windows Vulkan build で必要だった修正

現行 branch はそのままでは Windows build を完走しなかったため、次の最小修正を作業ツリーへ
適用しています。commit 前には Linux/macOS build への影響も CI で確認してください。

- 11 個の `indra/llrender/llvk*.cpp` で `<pthread.h>` を `LL_LINUX` の内側へ移動
- `llvkimages.cpp` の return address 取得を Windows では MSVC の `_ReturnAddress()` に切り替え
- `ll::vulkan` imported target を `GLOBAL` にし、media plugin から参照可能に変更
- `ll::pluginlibraries` へ `ll::vulkan` を伝播し、CEF/libVLC plugin に Vulkan include path を追加

LunarG SDK 1.4.350.0 の copy-only 配置では、同梱 CMake metadata が参照する layout に合わせ、
SDK 内の `Lib\lib` と `Lib\bin` / `Lib\include` を補完しました。これは source tree 外のローカル
SDK layout 調整です。

## 8. configure 後の成立確認

build 前に CMake cache を確認します。

```cmd
findstr /C:"USE_AVX2_OPTIMIZATION:BOOL=ON" build-vc170-64\CMakeCache.txt
findstr /C:"USE_FMODSTUDIO:BOOL=ON" build-vc170-64\CMakeCache.txt
findstr /C:"USE_OPENAL:BOOL=ON" build-vc170-64\CMakeCache.txt
findstr /C:"PACKAGE:BOOL=ON" build-vc170-64\CMakeCache.txt
findstr /C:"LL_DULLAHAN_AUDIO_CALLBACK:BOOL=TRUE" build-vc170-64\CMakeCache.txt
findstr /C:"Vulkan_INCLUDE_DIR" build-vc170-64\CMakeCache.txt
findstr /C:"glslang_DIR" build-vc170-64\CMakeCache.txt
findstr /C:"VIEWER_CHANNEL:STRING=AYAstorm-release" build-vc170-64\CMakeCache.txt
```

追加確認:

- configure log に `Compiling with AVX2 optimizations` がある
- generated `.vcxproj` に `/arch:AVX2` がある
- `Vulkan_INCLUDE_DIR` が `%VULKAN_SDK%\Include` を指す
- `glslang_DIR` が `%VULKAN_SDK%\Lib\cmake\glslang` を指す
- configure log に `Dullahan audio callback path: ENABLED` がある
- autobuild staging が `dullahan_aya_audio` の Windows package を使用している
- FMOD package staging に `fmod.dll`、`opus.dll`、`fmod_vc.lib`、`opus.lib` がある
- OpenAL package staging に `OpenAL32.dll`、`alut.dll` と link libraries がある
- configure が system OpenGL SDK の有無だけで成立していないこと

## 9. build baseline の成果物と監査結果

以下は開発版プロファイル分離修正前の build baseline の記録です。現行 checkout で再ビルドするまで、
この成果物の hash や実行結果を現行 HEAD の検証結果として扱わないでください。

想定 directory:

```text
E:\Ayastorm_build\build-vc170-64\newview\Release\
```

今回の成果物:

```text
viewer:   E:\Ayastorm_build\build-vc170-64\newview\Release\FirestormOSAYAstorm-release.exe
size:     59,564,544 bytes
sha256:   51469F2079B0AA8FA5366BC7339C9A0D7B5F122B3E89C87F9C2F426884587100

installer:E:\Ayastorm_build\build-vc170-64\newview\Release\Phoenix-FirestormOSAYAstorm-release_AVX2-42-1-0-262400938_Setup.exe
size:     229,270,414 bytes
sha256:   DE36A54B1D88C57802BC83D203A1AB8F850F5B56CAA9B0DCFCDC974907EE0053
```

監査結果:

- build command は exit code `0`、MSBuild は警告 `0` / エラー `0`
- viewer は PE32+ x64 (`8664 machine (x64)`)
- generated project と実コンパイル command に `/arch:AVX2` を確認
- `build_data.json` は version `42.1.0.262400938`、platform `win`、address size `64`
- staging に `fmod.dll`、`opus.dll`、`OpenAL32.dll`、`alut.dll` が存在
- `llplugin` に CEF/libVLC plugin、`dullahan_host.exe`、`libcef.dll`、CEF 用
  `vulkan-1.dll` / SwiftShader が存在
- NSIS Setup 自体は標準の PE32 x86 stub だが、収録 viewer は x64
- signing configuration がないため viewer と installer は未署名
- installer は実行していない

最終的な配布可否を判断する前に、installer を隔離した試験環境で実行し、viewer 起動、ログイン、
Vulkan instance/device・Win32 surface・swapchain、通常 world 描画、FMOD/Opus、OpenAL、CEF media を
確認します。Vulkan validation error、device lost、shader compile failure がないこともログで確認します。

## 10. 残作業

1. 未署名成果物を隔離した試験環境へコピーする。
2. installer を実行して install/uninstall を確認する。
3. viewer の Vulkan 描画、FMOD/Opus、OpenAL、Dullahan/CEF を実機確認する。
4. 配布する場合は正式なコード署名を行い、署名後の SHA-256 を別途記録する。

FMOD version は 2.03.07、Vulkan SDK は 1.4.350.0 をこの PC の最初の build baseline として固定しました。
configure、compile、package の blocker は解消済みです。残る不確定要素は実行時の Vulkan と
media/audio 経路です。
