@echo off
echo ============================================
echo   MAKOTO EXECUTOR - Build Injector
echo ============================================
echo.

:: Find csc.exe
set CSC_PATH=
for /f "delims=" %%i in ('dir /s /b "%ProgramFiles(x86)%\MSBuild\*\Bin\amd64\csc.exe" 2^>nul') do set CSC_PATH=%%i
if not defined CSC_PATH (
    for /f "delims=" %%i in ('dir /s /b "%ProgramFiles%\Microsoft Visual Studio\*\*\MSBuild\*\Bin\amd64\csc.exe" 2^>nul') do set CSC_PATH=%%i
)
if not defined CSC_PATH (
    for /f "delims=" %%i in ('dir /s /b "%Windash\Microsoft.NET\Framework64\v*\csc.exe" 2^>nul') do set CSC_PATH=%%i
)

if not defined CSC_PATH (
    echo [ERROR] C# compiler not found. Trying dotnet...
    where dotnet >nul 2>nul
    if %errorlevel% equ 0 (
        echo Found dotnet, creating project...
        mkdir InjectorApp 2>nul
        cd InjectorApp
        dotnet new winforms -n MakotoInjector --force
        cd ..
        echo Please build manually with: cd InjectorApp; dotnet build -c Release
        pause
        exit /b 0
    ) else (
        echo [ERROR] No C# compiler found. Please install .NET SDK.
        pause
        exit /b 1
    )
)

echo Found compiler: %CSC_PATH%
echo.

:: Compile
echo Compiling injector...
"%CSC_PATH%" /nologo /target:winexe /out:MakotoInjector.exe /reference:System.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll /reference:System.Core.dll injector.cs

if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed.
    pause
    exit /b 1
)

echo.
echo ============================================
echo   BUILD SUCCESSFUL
echo ============================================
echo.
echo Output: MakotoInjector.exe
echo.
echo Usage:
echo   1. Copy MakotoExecutor.dll and MakotoInjector.exe to same folder
echo   2. Run MakotoInjector.exe
echo   3. Open Roblox
echo   4. Click "Attach to Roblox"
echo.
pause
