@echo off
setlocal EnableDelayedExpansion

echo ========================================
echo   Inferno Agent Windows Build Script
echo ========================================
echo.

set "SERVER_HOST=localhost"
set "BUILD_TYPE=Release"
if /I "%~1"=="Debug" set "BUILD_TYPE=Debug"

pushd "%~dp0" 2>nul

if exist "build-win" (
    echo [INFO] Cleaning old build directory...
    rmdir /s /q build-win 2>nul
)

mkdir build-win
cd build-win

echo ========================================
echo Step 1: Locating OpenSSL & Compilers
echo ========================================
echo.

REM Recherche d'OpenSSL (dans MSYS2, vcpkg, ou installation Windows standard)
set "OPENSSL_PATH="
set "OPENSSL_CANDIDATES=C:\msys64\mingw64 C:\msys64\ucrt64 %USERPROFILE%\vcpkg\installed\x64-mingw-static C:\tools\OpenSSL C:\Program Files\OpenSSL-Win64"

for %%O in (%OPENSSL_CANDIDATES%) do (
    if not defined OPENSSL_PATH (
        if exist "%%O\include\openssl\ssl.h" set "OPENSSL_PATH=%%O"
    )
)

if defined OPENSSL_PATH (
    echo [OK] Found OpenSSL at: !OPENSSL_PATH!
) else (
    echo [WARNING] OpenSSL path not detected automatically.
)

echo.
echo ========================================
echo Step 2: Configuring with CMake
echo ========================================
echo.

set "CMAKE_CMD=cmake .."
set "CMAKE_CMD=!CMAKE_CMD! -G "MinGW Makefiles""
set "CMAKE_CMD=!CMAKE_CMD! -DCMAKE_BUILD_TYPE=%BUILD_TYPE%"
set "CMAKE_CMD=!CMAKE_CMD! -DOPENSSL_USE_STATIC_LIBS=TRUE"
set "CMAKE_CMD=!CMAKE_CMD! -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc -static-libstdc++""
set "CMAKE_CMD=!CMAKE_CMD! -DBUILD_TESTING=OFF"

if defined OPENSSL_PATH (
    set "CMAKE_CMD=!CMAKE_CMD! -DOPENSSL_ROOT_DIR="%OPENSSL_PATH%""
)

!CMAKE_CMD!

if errorlevel 1 (
    echo.
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Step 3: Building Agent Executable
echo ========================================
echo.

cmake --build . --config %BUILD_TYPE% --parallel 4

if errorlevel 1 (
    echo.
    echo [FAILED] Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Build Successful!
echo ========================================
echo.
echo Executable generated at: %CD%\bin\agent.exe
echo.

pauseset /p RUN_NOW="Voulez-vous executer l'agent maintenant ? (O/N) : "

if /I "%RUN_NOW%"=="O" (
    echo.
    echo [INFO] Démarrage de l'agent avec SERVER_HOST=%SERVER_HOST%...
    echo ----------------------------------------------------
    
    :: Lancement direct : les std::cout / logs s'afficheront ici
    bin\agent.exe
)

pause