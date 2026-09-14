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

set STAGE=%TEMP%\jam-vulkan-lib
set SRC=%STAGE%\src
set BUILD=%STAGE%\build

if not exist "%SRC%" mkdir "%SRC%"
if not exist "%BUILD%" mkdir "%BUILD%"

if not exist "%SRC%\SPIRV-Cross" (
    git clone --branch vulkan-sdk-1.4.350.0 --depth 1 https://github.com/KhronosGroup/SPIRV-Cross.git "%SRC%\SPIRV-Cross"
    if errorlevel 1 exit /b 1
)

:: Fetches pinned glslang/SPIRV-Tools/SPIRV-Headers flat into third_party/.
if not exist "%SRC%\shaderc" (
    git clone https://github.com/google/shaderc.git "%SRC%\shaderc"
    if errorlevel 1 exit /b 1
    git -C "%SRC%\shaderc" checkout d5f08ae5c5a9a45165578445cbd0f9adf0223448
    if errorlevel 1 exit /b 1
    pushd "%SRC%\shaderc"
    python utils\git-sync-deps
    if errorlevel 1 ( popd & exit /b 1 )
    popd
)

echo Configuring SPIRV-Cross (Release)...
:: SPIRV-Cross's cmake_minimum_required(3.10) otherwise silently ignores
:: CMAKE_MSVC_RUNTIME_LIBRARY and builds /MD.
cmake -S "%SRC%\SPIRV-Cross" -B "%BUILD%\spirv-cross-release" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%BUILD%\install" -DCMAKE_POLICY_DEFAULT_CMP0091=NEW "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>" -DSPIRV_CROSS_STATIC=ON -DSPIRV_CROSS_SHARED=OFF -DSPIRV_CROSS_CLI=OFF -DSPIRV_CROSS_ENABLE_TESTS=OFF -DSPIRV_CROSS_ENABLE_GLSL=OFF -DSPIRV_CROSS_ENABLE_HLSL=OFF -DSPIRV_CROSS_ENABLE_MSL=OFF -DSPIRV_CROSS_ENABLE_CPP=OFF -DSPIRV_CROSS_ENABLE_REFLECT=OFF -DSPIRV_CROSS_ENABLE_C_API=OFF -DSPIRV_CROSS_ENABLE_UTIL=OFF
if errorlevel 1 exit /b 1
cmake --build "%BUILD%\spirv-cross-release"
if errorlevel 1 exit /b 1
cmake --install "%BUILD%\spirv-cross-release"
if errorlevel 1 exit /b 1

echo Configuring SPIRV-Cross (Debug)...
:: Debug config kept for /MTd + postfix-d, but optimized (/O2) and stripped of /Zi: third-party code is never stepped into.
cmake -S "%SRC%\SPIRV-Cross" -B "%BUILD%\spirv-cross-debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="%BUILD%\install" -DCMAKE_POLICY_DEFAULT_CMP0091=NEW "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>" -DSPIRV_CROSS_STATIC=ON -DSPIRV_CROSS_SHARED=OFF -DSPIRV_CROSS_CLI=OFF -DSPIRV_CROSS_ENABLE_TESTS=OFF -DSPIRV_CROSS_ENABLE_GLSL=OFF -DSPIRV_CROSS_ENABLE_HLSL=OFF -DSPIRV_CROSS_ENABLE_MSL=OFF -DSPIRV_CROSS_ENABLE_CPP=OFF -DSPIRV_CROSS_ENABLE_REFLECT=OFF -DSPIRV_CROSS_ENABLE_C_API=OFF -DSPIRV_CROSS_ENABLE_UTIL=OFF -DCMAKE_C_FLAGS_DEBUG="/O2 /Ob2 /DNDEBUG" -DCMAKE_CXX_FLAGS_DEBUG="/O2 /Ob2 /DNDEBUG"
if errorlevel 1 exit /b 1
cmake --build "%BUILD%\spirv-cross-debug"
if errorlevel 1 exit /b 1
cmake --install "%BUILD%\spirv-cross-debug"
if errorlevel 1 exit /b 1

echo Configuring shaderc (Release)...
:: SHADERC_ENABLE_SHARED_CRT=OFF is shaderc's static-CRT path (/MT, /MTd via its
:: own MultiThreaded generator expression, propagated through glslang/SPIRV-Tools);
:: CMAKE_DEBUG_POSTFIX=d makes the Debug archive shaderc_combinedd.lib so both
:: configs coexist in lib/.
cmake -S "%SRC%\shaderc" -B "%BUILD%\shaderc-release" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%BUILD%\install" -DSHADERC_SKIP_TESTS=ON -DSHADERC_SKIP_EXAMPLES=ON -DSHADERC_SKIP_EXECUTABLES=ON -DSHADERC_SKIP_COPYRIGHT_CHECK=ON -DSHADERC_ENABLE_SHARED_CRT=OFF -DCMAKE_DEBUG_POSTFIX=d -DENABLE_GLSLANG_BINARIES=OFF -DSPIRV_SKIP_EXECUTABLES=ON
if errorlevel 1 exit /b 1
cmake --build "%BUILD%\shaderc-release"
if errorlevel 1 exit /b 1
cmake --install "%BUILD%\shaderc-release"
if errorlevel 1 exit /b 1

echo Configuring shaderc (Debug)...
:: Debug config kept for /MTd + postfix-d, but optimized (/O2) and stripped of /Zi: third-party code is never stepped into.
cmake -S "%SRC%\shaderc" -B "%BUILD%\shaderc-debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="%BUILD%\install" -DSHADERC_SKIP_TESTS=ON -DSHADERC_SKIP_EXAMPLES=ON -DSHADERC_SKIP_EXECUTABLES=ON -DSHADERC_SKIP_COPYRIGHT_CHECK=ON -DSHADERC_ENABLE_SHARED_CRT=OFF -DCMAKE_DEBUG_POSTFIX=d -DENABLE_GLSLANG_BINARIES=OFF -DSPIRV_SKIP_EXECUTABLES=ON -DCMAKE_C_FLAGS_DEBUG="/O2 /Ob2 /DNDEBUG" -DCMAKE_CXX_FLAGS_DEBUG="/O2 /Ob2 /DNDEBUG"
if errorlevel 1 exit /b 1
cmake --build "%BUILD%\shaderc-debug"
if errorlevel 1 exit /b 1
cmake --install "%BUILD%\shaderc-debug"
if errorlevel 1 exit /b 1

echo Copying artifacts to %ROOT%...
copy /Y "%BUILD%\install\lib\shaderc_combined.lib" "%ROOT%\"
if errorlevel 1 exit /b 1
copy /Y "%BUILD%\install\lib\shaderc_combinedd.lib" "%ROOT%\"
if errorlevel 1 exit /b 1
copy /Y "%BUILD%\install\lib\spirv-cross-core.lib" "%ROOT%\"
if errorlevel 1 exit /b 1
copy /Y "%BUILD%\install\lib\spirv-cross-cored.lib" "%ROOT%\"
if errorlevel 1 exit /b 1

echo ==========================================
echo jam_vulkan/lib build complete:
echo   shaderc_combined.lib
echo   shaderc_combinedd.lib
echo   spirv-cross-core.lib
echo   spirv-cross-cored.lib
echo ==========================================
endlocal
