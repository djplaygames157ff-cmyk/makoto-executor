@echo off
echo ============================================
echo   MAKOTO EXECUTOR v2.0 - Build Script
echo ============================================
echo.

:: Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found. Please install CMake.
    pause
    exit /b 1
)

:: Create build directory
if not exist build mkdir build
cd build

:: Configure
echo [1/3] Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

:: Build
echo [2/3] Building MAKOTO EXECUTOR...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

:: Copy output
echo [3/3] Copying files...
if not exist ..\dist mkdir ..\dist
copy Release\MakotoExecutor.dll ..\dist\
copy Release\MakotoHost.exe ..\dist\ 2>nul

echo.
echo ============================================
echo   BUILD SUCCESSFUL
echo ============================================
echo.
echo Output: dist\MakotoExecutor.dll
echo.
echo Next steps:
echo   1. Run MakotoHost.exe to inject
echo   2. Open Roblox
echo   3. Use the UI to execute scripts
echo.
pause
