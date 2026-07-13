# Ayastorm Viewer ビルド手順書

Linux版 / Windows版
2026年4月

---

## Linux版ビルド手順

### 1. 必要な環境

- Ubuntu 22.04 LTS (x86_64)
- RAM 16GB以上、ストレージ 64GB以上
- GCC 11（Ubuntu 22.04のデフォルト）
- Python 3（venv使用推奨）

### 2. 必要パッケージのインストール（一度だけ）

```bash
sudo apt install libgl1-mesa-dev libglu1-mesa-dev libpulse-dev build-essential \
  python3-pip git libssl-dev libxinerama-dev libxrandr-dev \
  libfontconfig-dev libfreetype6-dev gcc-11 cmake
```

### 3. ディレクトリ作成とリポジトリのclone

```bash
mkdir ~/work_ayastorm && cd ~/work_ayastorm
git clone https://github.com/mayatonton/phoenix-firestorm.git
cd phoenix-firestorm
git checkout ayastorm-release

# ビルド変数リポジトリ
cd ~/work_ayastorm
git clone https://github.com/FirestormViewer/fs-build-variables.git
```

### 3.1 Release tag を埋め込む場合

AYAstorm r32 以降の update notification は、build に埋め込まれた `AYASTORM_RELEASE_TAG` と GitHub Releases の tag を比較します。Release 配布物を作る場合は、必ず release tag を埋め込んでください。

tag が付いた commit から configure する場合は、CMake が自動検出します。

```bash
git checkout REPLACE_WITH_RELEASE_TAG
```

tag checkout ではない CI / source archive から build する場合は、configure 時に明示します。

```bash
export AYA_RELEASE_TAG="REPLACE_WITH_RELEASE_TAG"
-DAYASTORM_RELEASE_TAG=REPLACE_WITH_RELEASE_TAG
```

`-bugfix-N` 形式も対応しています。例えば `v7.2.4-ayastorm-r32-bugfix-2` は viewer 内では `AYAstorm r32.2` として扱われます。

`autobuild.xml` には release tag を固定値として直書きしません。`autobuild.xml` は共通の build configuration 定義なので、release ごとに変わる値は `autobuild configure` の `--` 後に CMake 引数として渡すか、CI / build script の変数から渡してください。

build tree を再利用する場合、既存の CMake cache が `dev` のままだと、コンパイル / リンク / パッケージングだけを再実行しても release tag は更新されません。tag を変更した場合、または `-DAYASTORM_RELEASE_TAG=...` を変更した場合は configure を再実行してください。

### 4. Python仮想環境とautobuildのセットアップ（一度だけ）

```bash
cd ~/work_ayastorm/phoenix-firestorm
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### 5. 環境変数の設定

毎回ビルド前に実行するか `~/.bashrc` に追記しておく：

```bash
source ~/work_ayastorm/phoenix-firestorm/.venv/bin/activate
export AUTOBUILD_VARIABLES_FILE=$HOME/work_ayastorm/fs-build-variables/variables

# Release build only. Dev build では空のままでよい。
export AYA_RELEASE_TAG=""
```

### 6. FMODのセットアップ（一度だけ）

https://www.fmod.com で無料アカウントを作成してLinux版 **FMOD Studio API** をダウンロード（バージョン2.03.07）。

```bash
cd ~/work_ayastorm
# AYAstorm は SDK 同梱の libopus を staging する fork を使う (Opus 5.1 surround 対応のため)
git clone https://github.com/mayatonton/3p-fmodstudio.git
cp ~/ダウンロード/fmodstudioapi20307linux.tar.gz ~/work_ayastorm/3p-fmodstudio/
cd ~/work_ayastorm/3p-fmodstudio
autobuild build -A 64 --all
autobuild package -A 64 --results-file result.txt
cat result.txt  # md5値を確認
```

result.txt の md5 値を確認してFirestormに登録：

```bash
cd ~/work_ayastorm/phoenix-firestorm
autobuild installables edit fmodstudio platform=linux64 \
  hash=<md5値> \
  url=file:///home/{user name}/work_ayastorm/3p-fmodstudio/fmodstudio-2.03.07-linux64-*.tar.bz2
```

### 7. configure（初回または --clean のとき）

```bash
cd ~/work_ayastorm/phoenix-firestorm
autobuild configure -A 64 -c ReleaseFS_open -- \
  --fmodstudio \
  -DLL_TESTS:BOOL=FALSE \
  ${AYA_RELEASE_TAG:+-DAYASTORM_RELEASE_TAG="$AYA_RELEASE_TAG"} \
  --package \
  --chan AYAstorm-release
```

開発ビルドでは `-DAYASTORM_RELEASE_TAG=...` を省略できます。その場合は `dev` として扱われます。

### 8. ビルド

```bash
autobuild build -A 64 -c ReleaseFS_open --no-configure
```

### 9. キャッシュの削除 & インストール

```bash
autobuild configure -A 64 -c ReleaseFS_open -- \
  --fmodstudio \
  -DLL_TESTS:BOOL=FALSE \
  ${AYA_RELEASE_TAG:+-DAYASTORM_RELEASE_TAG="$AYA_RELEASE_TAG"} \
  --package \
  --chan AYAstorm-release
autobuild build -A 64 -c ReleaseFS_open

cd ~/work_ayastorm/phoenix-firestorm/build-linux-x86_64/newview/packaged
rm -rf ~/ayastorm/
rm -rf ~/.local/share/applications/ayastorm-viewer.desktop
./install.sh
rm -rf ~/.ayastorm_x64/cache/
```

### 10. 実行
```bash
~/ayastorm/ayastorm 
```




---

## Windows版ビルド手順

### 1. 必要ツールのインストール（一度だけ）

> **重要：** すべての作業はPowerShellではなく **cmd.exe（コマンドプロンプト）管理者モード** で行う。

#### Visual Studio 2022 Community（無料）

- 管理者として実行
- 「Desktop development with C++」にチェック

#### Git for Windows

- 「Checkout as-is, commit as-is」を選択（**重要！**）

#### CMake 4.1.2以上

- 「Add CMake to the system PATH for all users」を選択

#### Cygwin 64

- 管理者として実行
- 追加パッケージ：`Devel/patch` を選択

#### Python 3

- 管理者として実行
- 「Add Python to PATH」にチェック
- インストール先：`C:\Python3`

#### NSIS（インストーラー作成用）

- https://nsis.sourceforge.io からダウンロード

### 2. リポジトリのclone

```cmd
c:
mkdir work_ayastorm
cd work_ayastorm
git clone https://github.com/mayatonton/phoenix-firestorm.git
cd phoenix-firestorm
git checkout ayastorm-release

cd c:\work_ayastorm
git clone https://github.com/FirestormViewer/fs-build-variables.git
```

### 2.1 Release tag を埋め込む場合

AYAstorm r32 以降の update notification は、build に埋め込まれた `AYASTORM_RELEASE_TAG` と GitHub Releases の tag を比較します。Release 配布物を作る場合は、必ず release tag を埋め込んでください。

tag が付いた commit から configure する場合は、CMake が自動検出します。

```cmd
git checkout REPLACE_WITH_RELEASE_TAG
```

tag checkout ではない CI / source archive から build する場合は、configure 時に明示します。

```cmd
set AYA_RELEASE_TAG=REPLACE_WITH_RELEASE_TAG
-DAYASTORM_RELEASE_TAG=REPLACE_WITH_RELEASE_TAG
```

`-bugfix-N` 形式も対応しています。例えば `v7.2.4-ayastorm-r32-bugfix-2` は viewer 内では `AYAstorm r32.2` として扱われます。

`autobuild.xml` には release tag を固定値として直書きしません。`autobuild.xml` は共通の build configuration 定義なので、release ごとに変わる値は `autobuild configure` の `--` 後に CMake 引数として渡すか、CI / build script の変数から渡してください。

build tree を再利用する場合、既存の CMake cache が `dev` のままだと、コンパイル / リンク / パッケージングだけを再実行しても release tag は更新されません。tag を変更した場合、または `-DAYASTORM_RELEASE_TAG=...` を変更した場合は configure を再実行してください。

### 3. autobuildのセットアップ（一度だけ）

```cmd
cd c:\work_ayastorm\phoenix-firestorm
pip install -r requirements.txt
```

### 4. 環境変数の設定

ビルドのたびに **管理者cmd** で以下を実行：

```cmd
set PYTHONUTF8=1
set AUTOBUILD_VSVER=170
set AUTOBUILD_VARIABLES_FILE=c:\work_ayastorm\fs-build-variables\variables
set PATH=C:\cygwin64\bin;%PATH%
set AUTOBUILD_CONFIG_FILE=my_autobuild.xml
rem Release build only. Dev build では未設定のままでよい。
rem Example: set "AYA_RELEASE_TAG=v7.2.4-ayastorm-r32-bugfix-2"
set "AYA_RELEASE_TAG_ARG="
if defined AYA_RELEASE_TAG set "AYA_RELEASE_TAG_ARG=-DAYASTORM_RELEASE_TAG=%AYA_RELEASE_TAG%"
```

> `my_autobuild.xml` はFMODセットアップ後に作成されます。

### 5. FMODのセットアップ（一度だけ）

https://www.fmod.com で無料アカウントを作成してWindows版 **FMOD Studio API** をダウンロード（バージョン2.03.07）。

```cmd
cd c:\work_ayastorm
:: AYAstorm は SDK 同梱の libopus を staging する fork を使う (Opus 5.1 surround 対応のため)
git clone https://github.com/mayatonton/3p-fmodstudio.git
copy fmodstudioapi20307win-installer.exe c:\work_ayastorm\3p-fmodstudio\
cd c:\work_ayastorm\3p-fmodstudio
autobuild build -A 64 --all
autobuild package -A 64 --results-file result.txt
type result.txt
```

result.txt の md5 値を確認してFirestormに登録：

```cmd
cd c:\work_ayastorm\phoenix-firestorm
copy autobuild.xml my_autobuild.xml
set AUTOBUILD_CONFIG_FILE=my_autobuild.xml
autobuild installables edit fmodstudio platform=windows64 ^
  hash=<md5値> ^
  url=file:///c:/work_ayastorm/3p-fmodstudio/fmodstudio-2.03.07-windows64-*.tar.bz2
```

### 6. configure (Legacy)

管理者cmdで環境変数を設定した後に実行：

```cmd
cd c:\work_ayastorm\phoenix-firestorm
rmdir /s /q build-vc170-64
autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio -DLL_TESTS:BOOL=FALSE %AYA_RELEASE_TAG_ARG% --package --chan AYAstorm-release
```

開発ビルドでは `-DAYASTORM_RELEASE_TAG=...` を省略できます。その場合は `dev` として扱われます。

### 7. ビルド (Legacy)

```cmd
autobuild build -A 64 -c ReleaseFS_open --no-configure
```

### 8. インストーラーの場所 (Legacy)

```
c:\work_ayastorm\phoenix-firestorm\build-vc170-64\newview\Release\
Phoenix-FirestormOS-Ayastorm-release_LEGACY-7-2-4-80621_Setup.exe
```

### 9. configure (AVX2)

管理者cmdで環境変数を設定した後に実行：

```cmd
cd c:\work_ayastorm\phoenix-firestorm
rmdir /s /q build-vc170-64
autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio --avx2 -DLL_TESTS:BOOL=FALSE %AYA_RELEASE_TAG_ARG% --package --chan AYAstorm-release
```

### 10. ビルド (AVX2)

```cmd
autobuild build -A 64 -c ReleaseFS_AVX2 --no-configure
```

### 11. インストーラーの場所 (AVX2)

```
c:\work_ayastorm\phoenix-firestorm\build-vc170-64\newview\Release\
Phoenix-FirestormOS-AYAstorm-release_AVX2-7-2-4-80621_Setup.exe
```
