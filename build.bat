@echo off
setlocal EnableExtensions

set "configuration=%~1"
if not defined configuration set "configuration=Debug"
if /i not "%configuration%"=="Debug" if /i not "%configuration%"=="Release" (
    echo Usage: %~nx0 [Debug^|Release]
    exit /b 2
)

where msbuild.exe >nul 2>&1
if not errorlevel 1 (
    set "msbuild_path=msbuild.exe"
    goto build
)

set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%vswhere%" goto missing_msbuild
for /f "usebackq delims=" %%i in (`"%vswhere%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    set "msbuild_path=%%i"
    goto build
)

:missing_msbuild
echo MSBuild was not found. Install Visual Studio with Desktop development with C++.
exit /b 1

:build
echo Building Multishoot %configuration% x64...
"%msbuild_path%" "%~dp0Multishoot.sln" /m /t:Build /p:Configuration=%configuration% /p:Platform=x64
exit /b %errorlevel%
