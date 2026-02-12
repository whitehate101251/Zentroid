@echo off

echo=
echo=
echo ---------------------------------------------------------------
echo check ENV
echo ---------------------------------------------------------------

:: Get required parameters from environment variables
:: example: D:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat
:: set vcvarsall="%ENV_VCVARSALL%"

:: example: D:\Qt\Qt5.12.5\5.12.5
:: echo ENV_VCVARSALL %ENV_VCVARSALL%
echo ENV_QT_PATH %ENV_QT_PATH%

:: Get script absolute path
set script_path=%~dp0
:: Enter script directory; this affects the working directory of programs executed next
set old_cd=%cd%
cd /d %~dp0

:: Startup parameter declarations
set cpu_mode=x86
set build_mode=RelWithDebInfo
set errno=1

echo=
echo=
echo ---------------------------------------------------------------
echo check build param[Debug/Release/MinSizeRel/RelWithDebInfo]
echo ---------------------------------------------------------------

:: Build parameter validation
if "%1"=="Debug" (    
    goto build_mode_ok
)
if "%1"=="Release" (    
    goto build_mode_ok
)
if "%1"=="MinSizeRel" (    
    goto build_mode_ok
)
if "%1"=="RelWithDebInfo" (
    goto build_mode_ok
)
echo error: unknown build mode -- %1
goto return
:build_mode_ok

set build_mode=%1
set cmake_vs_build_mode=Win32
set qt_cmake_path=%ENV_QT_PATH%\msvc2019\lib\cmake\Qt5

if /i "%2"=="x86" (
    set cpu_mode=x86
    set cmake_vs_build_mode=Win32
    set qt_cmake_path=%ENV_QT_PATH%\msvc2019\lib\cmake\Qt5
)
if /i "%2"=="x64" (
    set cpu_mode=x64
    set cmake_vs_build_mode=x64
    set qt_cmake_path=%ENV_QT_PATH%\msvc2019_64\lib\cmake\Qt5
)

:: Display info
echo current build mode: %build_mode% %cpu_mode%
echo qt cmake path: %qt_cmake_path%

echo=
echo=
echo ---------------------------------------------------------------
echo begin cmake build
echo ---------------------------------------------------------------

:: Delete output directory
set output_path=%script_path%..\..\output
if exist %output_path% (          
    rmdir /q /s %output_path%
)
:: Delete temporary directory
set temp_path=%script_path%..\build_temp
if exist %temp_path% (          
    rmdir /q /s %temp_path%
)
md %temp_path%
cd %temp_path%

set cmake_params=-DCMAKE_PREFIX_PATH=%qt_cmake_path% -DCMAKE_BUILD_TYPE=%build_mode% -G "Visual Studio 17 2022" -A %cmake_vs_build_mode%
echo cmake params: %cmake_params%

cmake %cmake_params% ../..
if not %errorlevel%==0 (
    echo "cmake failed"
    goto return
)

cmake --build . --config %build_mode% -j8
if not %errorlevel%==0 (
    echo "cmake build failed"
    goto return
)

echo=
echo=
echo ---------------------------------------------------------------
echo finish!!!
echo ---------------------------------------------------------------

set errno=0

:return
cd %old_cd%
exit /B %errno%
