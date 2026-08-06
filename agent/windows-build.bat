@echo off
setlocal EnableDelayedExpansion

echo ========================================
echo   Inferno Agent Build Script
echo ========================================
echo.

set "BUILD_TYPE=Release"
if /I "%~1"=="Debug" set "BUILD_TYPE=Debug"
if /I "%~1"=="Release" set "BUILD_TYPE=Release"

echo [INFO] Build type: %BUILD_TYPE%
echo.

REM ==================== PATH SETUP ====================
REM Navigate to script's directory (agent/)
pushd "%~dp0" 2>nul
if errorlevel 1 (
    set "SCRIPT_PATH=%~dp0"
    if defined SCRIPT_PATH (
        cd /d "%SCRIPT_PATH%" 2>nul
    )
)

REM Verify we're in the right place
if not exist "CMakeLists.txt" (
    REM Try going to agent/ subdirectory if we're in root
    if exist "agent\CMakeLists.txt" (
        cd agent
    )
)

echo [INFO] Working directory: %CD%
echo.

REM ==================== COMPILER DETECTION ====================

echo [INFO] Detecting C++ compiler...
echo.

set "COMPILER_TYPE="
set "COMPILER_PATH="
set "GENERATOR="

REM Try MinGW (common on Windows)
where /q g++ >nul 2>&1
if errorlevel 0 (
    set "COMPILER_TYPE=MinGW"
    set "GENERATOR=MinGW Makefiles"
    for /f "delims=" %%i in ('where g++') do set "COMPILER_PATH=%%i"
    echo [OK] Found MinGW G++: !COMPILER_PATH!
    goto :compiler_found
)

REM Try MSVC (Visual Studio)
where /q cl >nul 2>&1
if errorlevel 0 (
    set "COMPILER_TYPE=MSVC"
    set "GENERATOR=Visual Studio 17 2022"
    for /f "delims=" %%i in ('where cl') do set "COMPILER_PATH=%%i"
    echo [OK] Found MSVC cl.exe: !COMPILER_PATH!
    goto :compiler_found
)

REM Try Clang
where /q clang++ >nul 2>&1
if errorlevel 0 (
    set "COMPILER_TYPE=Clang"
    set "GENERATOR=Ninja"
    for /f "delims=" %%i in ('where clang++') do set "COMPILER_PATH=%%i"
    echo [OK] Found Clang: !COMPILER_PATH!
    goto :compiler_found
)

:compiler_not_found
echo [ERROR] No C++ compiler found in PATH!
echo.
echo Please install one of:
echo   - MinGW (g++^)
echo   - MSVC (Visual Studio)
echo   - Clang
echo.
echo Or add your compiler to PATH before running this script.
echo.
pause
exit /b 1

:compiler_found
echo [OK] Using: %COMPILER_TYPE% with %GENERATOR%
echo.

REM ==================== CMAKE CHECK ====================

where /q cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH!
    echo.
    echo Please install CMake from: https://cmake.org/download/
    echo.
    pause
    exit /b 1
)

echo [OK] CMake found
cmake --version
echo.

REM ==================== OPENSSL DETECTION ====================

echo [INFO] Detecting OpenSSL...
echo.

set "OPENSSL_PATH="
set "OPENSSL_CANDIDATES="

REM MSYS2 MinGW64
set "OPENSSL_CANDIDATES=!OPENSSL_CANDIDATES! C:\msys64\mingw64"

REM MSYS2 UCRT64
set "OPENSSL_CANDIDATES=!OPENSSL_CANDIDATES! C:\msys64\ucrt64"

REM vcpkg locations
set "OPENSSL_CANDIDATES=!OPENSSL_CANDIDATES! %USERPROFILE%\vcpkg\installed\x64-mingw-static"
set "OPENSSL_CANDIDATES=!OPENSSL_CANDIDATES! %USERPROFILE%\vcpkg\installed\x64-windows"

REM Common manual installs
set "OPENSSL_CANDIDATES=!OPENSSL_CANDIDATES! C:\tools\OpenSSL"
set "OPENSSL_CANDIDATES=!OPENSSL_CANDIDATES! C:\Program Files\OpenSSL-Win64"
set "OPENSSL_CANDIDATES=!OPENSSL_CANDIDATES! C:\Program Files\OpenSSL"

for %%O in (!OPENSSL_CANDIDATES!) do (
    if not defined OPENSSL_PATH (
        if exist "%%O\include\openssl\ssl.h" (
            if exist "%%O\lib\libssl.dll.a" (
                if exist "%%O\lib\libcrypto.dll.a" (
                    set "OPENSSL_PATH=%%O"
                )
            )
        )
    )
)

if defined OPENSSL_PATH (
    echo [OK] Found OpenSSL:
    echo      !OPENSSL_PATH!
) else (
    echo [WARNING] OpenSSL development files not found
    echo.
    echo The build may proceed, but TLS support will be disabled.
    echo.
)

echo.

REM ==================== VERIFY CMakeLists.txt ====================

if not exist "CMakeLists.txt" (
    echo [ERROR] CMakeLists.txt not found!
    echo This script must be in the agent/ directory.
    pause
    exit /b 1
)

echo [INFO] CMakeLists.txt verified
echo.

REM ==================== CLEAN BUILD DIRECTORY ====================

if exist "build" (
    echo [INFO] Cleaning old build directory...
    %SystemRoot%\System32\timeout.exe /t 1 /nobreak >nul
    rmdir /s /q build 2>nul
    if exist "build" (
        echo [WARNING] Could not delete build directory (files in use)
        %SystemRoot%\System32\timeout.exe /t 1 /nobreak >nul
        rmdir /s /q build 2>nul
        if exist "build" (
            echo [ERROR] Build directory is locked!
            echo Please close any programs using files in build/ and try again
            pause
            exit /b 1
        )
    )
)

mkdir build
cd build

echo ========================================
echo Step 1: Configuring with CMake
echo ========================================
echo.

REM Build CMake command
set "CMAKE_CMD=cmake .. -G "%GENERATOR%""
set "CMAKE_CMD=!CMAKE_CMD! -DCMAKE_BUILD_TYPE=%BUILD_TYPE%"

REM Add compiler if MinGW
if "%COMPILER_TYPE%"=="MinGW" (
    set "CMAKE_CMD=!CMAKE_CMD! -DCMAKE_CXX_COMPILER=g++"
    set "CMAKE_CMD=!CMAKE_CMD! -DCMAKE_C_COMPILER=gcc"
)

REM Add OpenSSL if found
if defined OPENSSL_PATH (
    set "CMAKE_CMD=!CMAKE_CMD! -DOPENSSL_ROOT_DIR="%OPENSSL_PATH%""
)

echo [INFO] CMake command:
echo !CMAKE_CMD!
echo.

!CMAKE_CMD!

if errorlevel 1 (
    echo.
    echo [ERROR] CMake configuration failed!
    echo.
    echo Troubleshooting:
    echo 1. Verify compiler is in PATH: g++ or cl
    echo 2. Verify CMake is installed and in PATH
    echo 3. Check CMakeLists.txt syntax
    echo.
    pause
    exit /b 1
)

echo.
echo [OK] Configuration successful
echo.

echo ========================================
echo Step 2: Building Agent
echo ========================================
echo.

cmake --build . --config %BUILD_TYPE% --parallel 4
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    echo Check the errors above
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Step 3: Running Tests (Optional)
echo ========================================
echo.

if exist "agent_tests.exe" (
    echo [INFO] Running agent tests...
    agent_tests.exe
    if errorlevel 1 (
        echo [WARNING] Some tests failed
        echo.
    ) else (
        echo [OK] All tests passed
    )
) else if exist "Release\agent_tests.exe" (
    echo [INFO] Running agent tests...
    Release\agent_tests.exe
    if errorlevel 1 (
        echo [WARNING] Some tests failed
        echo.
    ) else (
        echo [OK] All tests passed
    )
) else (
    echo [INFO] No test executable found (expected if tests not configured)
)

echo.
echo ========================================
echo   Build Successful!
echo ========================================
echo.

REM Find the executable
if exist "agent.exe" (
    echo Executable: %CD%\agent.exe
) else if exist "Release\agent.exe" (
    echo Executable: %CD%\Release\agent.exe
) else if exist "Debug\agent.exe" (
    echo Executable: %CD%\Debug\agent.exe
) else (
    echo Agent binary: built in build/ directory
    dir /b /s *.exe 2>nul | findstr agent
)

echo.
echo To run the agent:
echo   cd build
echo   agent.exe
echo.
echo.

pause