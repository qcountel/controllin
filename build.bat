@echo off
cd /d "%~dp0"
echo Building Controllin Injector...

REM Find Visual Studio installation
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set VS_PATH=%%i
)

if "%VS_PATH%"=="" (
    echo Visual Studio not found!
    exit /b 1
)

set CMAKE_PATH="%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set MSBUILD_PATH="%VS_PATH%\MSBuild\Current\Bin\MSBuild.exe"

if exist build (
    echo Cleaning old build directory...
    rd /s /q build
)

echo Configuring project with CMake...
%CMAKE_PATH% -B build -S . -A x64 "-DCMAKE_POLICY_VERSION_MINIMUM=4.0"

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    exit /b %errorlevel%
)

echo Building project (Release)...
%MSBUILD_PATH% "build\ControllinInjector.slnx" /p:Configuration=Release /p:Platform=x64

if %errorlevel% neq 0 (
    echo Build failed!
    exit /b %errorlevel%
)

echo Build successful! Executable is located at:
echo build\Release\
