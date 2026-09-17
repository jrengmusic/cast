@echo off
setlocal

set ROOT=%~dp0
if %ROOT:~-1%==\ set ROOT=%ROOT:~0,-1%

:: Find vcvarsall.bat via vswhere
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found. Is Visual Studio installed?
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set VS_PATH=%%i

set VCVARSALL=%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat
if not exist "%VCVARSALL%" (
    echo ERROR: vcvarsall.bat not found at %VCVARSALL%
    exit /b 1
)

echo Setting up MSVC x64 environment...
call "%VCVARSALL%" x64

:: Use VS-bundled ninja (avoids MSYS2 ld.exe conflict)
set PATH=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%

echo Configuring cast (Release, no-sign)...
cmake -S "%ROOT%" -B "%ROOT%\Builds\Release" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=OFF
if errorlevel 1 exit /b 1

echo Building cast...
ninja -C "%ROOT%\Builds\Release"
if errorlevel 1 exit /b 1

echo ==========================================
echo cast built: %ROOT%\Builds\Release\cast.exe
echo Installed to: %USERPROFILE%\.local\bin\cast.exe
echo ==========================================
endlocal
