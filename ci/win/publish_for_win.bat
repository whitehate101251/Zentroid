@echo off

echo=
echo=
echo ---------------------------------------------------------------
echo check ENV
echo ---------------------------------------------------------------

:: Get required parameters from environment variables
:: example: D:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat
set vcvarsall="%ENV_VCVARSALL%"
:: e.g. d:\a\Zentroid\Qt\5.12.7
set qt_msvc_path="%ENV_QT_PATH%"
:: Setting VCINSTALLDIR causes windeployqt to auto-copy vc_redist.x**.exe (vcruntime dll installer)
:: set VCINSTALLDIR="%ENV_VCINSTALL%"

echo ENV_VCVARSALL %ENV_VCVARSALL%
echo ENV_QT_PATH %ENV_QT_PATH%

:: Get script absolute path
set script_path=%~dp0
:: Enter script directory; this affects the working directory of programs executed next
set old_cd=%cd%
cd /d %~dp0

:: Startup parameter declarations
set cpu_mode=x86
set publish_dir=%2
set errno=1

if /i "%1"=="x86" (
    set cpu_mode=x86
)
if /i "%1"=="x64" (
    set cpu_mode=x64
)

:: Display info
echo current build mode: %cpu_mode%
echo current publish dir: %publish_dir%

:: Environment variable setup
set adb_path=%script_path%..\..\Zentroid\ZentroidCore\src\third_party\adb\win\*.*
set jar_path=%script_path%..\..\Zentroid\ZentroidCore\src\third_party\scrcpy-server
set keymap_path=%script_path%..\..\keymap
set config_path=%script_path%..\..\config

if /i %cpu_mode% == x86 (
    set publish_path=%script_path%%publish_dir%\
    set release_path=%script_path%..\..\output\x86\RelWithDebInfo
    set qt_msvc_path=%qt_msvc_path%\msvc2019\bin
) else (
    set publish_path=%script_path%%publish_dir%\
    set release_path=%script_path%..\..\output\x64\RelWithDebInfo
    set qt_msvc_path=%qt_msvc_path%\msvc2019_64\bin
)
set PATH=%qt_msvc_path%;%PATH%

:: Register VC environment (after registration, windeployqt will copy vc_redist (vcruntime installer))
if /i %cpu_mode% == x86 (
    call %vcvarsall% %cpu_mode%
) else (
    call %vcvarsall% %cpu_mode%
)

if exist %publish_path% (
    rmdir /s/q %publish_path%
)

:: Copy packages to publish
xcopy %release_path% %publish_path% /E /Y
xcopy %adb_path% %publish_path% /Y
xcopy %jar_path% %publish_path% /Y
xcopy %keymap_path% %publish_path%keymap\ /E /Y
xcopy %config_path% %publish_path%config\ /E /Y

:: Add Qt dependency packages
windeployqt %publish_path%\Zentroid.exe

:: Remove unnecessary Qt dependency packages
rmdir /s/q %publish_path%\iconengines
rmdir /s/q %publish_path%\translations

:: Screenshot feature requires qjpeg.dll
del %publish_path%\imageformats\qgif.dll
del %publish_path%\imageformats\qicns.dll
del %publish_path%\imageformats\qico.dll
::del %publish_path%\imageformats\qjpeg.dll
del %publish_path%\imageformats\qsvg.dll
del %publish_path%\imageformats\qtga.dll
del %publish_path%\imageformats\qtiff.dll
del %publish_path%\imageformats\qwbmp.dll
del %publish_path%\imageformats\qwebp.dll

:: Delete vc_redist, manually copy vcruntime dlls instead
if /i %cpu_mode% == x86 (
    del %publish_path%\vc_redist.x86.exe
) else (
    del %publish_path%\vc_redist.x64.exe
)

:: Copy vcruntime dlls
if /i %cpu_mode% == x64 (
    cp "C:\Windows\System32\msvcp140_1.dll" %publish_path%\msvcp140_1.dll
    cp "C:\Windows\System32\msvcp140.dll" %publish_path%\msvcp140.dll
    cp "C:\Windows\System32\vcruntime140.dll" %publish_path%\vcruntime140.dll
    :: Only x64 needs this
    cp "C:\Windows\System32\vcruntime140_1.dll" %publish_path%\vcruntime140_1.dll
) else (
    cp "C:\Windows\SysWOW64\msvcp140_1.dll" %publish_path%\msvcp140_1.dll
    cp "C:\Windows\SysWOW64\msvcp140.dll" %publish_path%\msvcp140.dll
    cp "C:\Windows\SysWOW64\vcruntime140.dll" %publish_path%\vcruntime140.dll
    
)

::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\VCRUNTIME140.dll" %publish_path%\VCRUNTIME140.dll
::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\api-ms-win-crt-runtime-l1-1-0.dll" %publish_path%\api-ms-win-crt-runtime-l1-1-0.dll
::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\api-ms-win-crt-heap-l1-1-0.dll" %publish_path%\api-ms-win-crt-heap-l1-1-0.dll
::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\api-ms-win-crt-math-l1-1-0.dll" %publish_path%\api-ms-win-crt-math-l1-1-0.dll
::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\api-ms-win-crt-stdio-l1-1-0.dll" %publish_path%\api-ms-win-crt-stdio-l1-1-0.dll
::cp "C:\Program Files (x86)\Microsoft Visual Studio\Installer\api-ms-win-crt-locale-l1-1-0.dll" %publish_path%\api-ms-win-crt-locale-l1-1-0.dll

echo=
echo=
echo ---------------------------------------------------------------
echo finish!!!
echo ---------------------------------------------------------------

set errno=0

:return
cd %old_cd%
exit /B %errno%