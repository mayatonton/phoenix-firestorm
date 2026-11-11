@echo off
set PYTHONUTF8=1
set AUTOBUILD_VSVER=170
set AUTOBUILD_VARIABLES_FILE=c:\firestorm\fs-build-variables\variables
set AUTOBUILD_CONFIG_FILE=my_autobuild.xml
rem Release build only: set this to the actual GitHub release tag before configure.
rem Example: set "AYASTORM_RELEASE_TAG=v7.2.4-ayastorm-r32-bugfix-2"
set "AYASTORM_RELEASE_TAG_ARG="
if defined AYASTORM_RELEASE_TAG set "AYASTORM_RELEASE_TAG_ARG=-DAYASTORM_RELEASE_TAG=%AYASTORM_RELEASE_TAG%"
set PATH=C:\cygwin64\bin;%PATH%

cd c:\firestorm\phoenix-firestorm

echo [1] 特殊文字を修正中...
python fixall2.py

echo [2] Configure中...
autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio -DLL_TESTS:BOOL=FALSE %AYASTORM_RELEASE_TAG_ARG% --package --chan AYAstorm-release
if errorlevel 1 (
    echo Configure失敗
    pause
    exit /b 1
)

echo [3] ビルド中...
autobuild build -A 64 -c ReleaseFS_open --no-configure
if errorlevel 1 (
    echo ビルド失敗
    pause
    exit /b 1
)

echo 完了
pause
