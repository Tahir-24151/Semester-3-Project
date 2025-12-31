@echo off
REM ============================================
REM Mini Google Maps Server/Client Build Script
REM ============================================

echo.
echo ============================================
echo    Building Mini Google Maps Server/Client
echo ============================================
echo.

REM Check if g++ is available
where g++ >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: g++ not found. Please install MinGW or MSYS2.
    pause
    exit /b 1
)

echo Compiling server...
g++ -std=c++17 -o server.exe ^
    src\server.cpp ^
    src\BTreeNode.cpp ^
    src\BTree.cpp ^
    src\Graph.cpp ^
    src\Navigation.cpp ^
    src\DatabaseManager.cpp ^
    -lws2_32

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Server compilation failed!
    pause
    exit /b 1
)
echo Server compiled successfully: server.exe

echo.
echo Compiling client...
g++ -std=c++17 -o client.exe src\client.cpp -lws2_32

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Client compilation failed!
    pause
    exit /b 1
)
echo Client compiled successfully: client.exe

echo.
echo ============================================
echo    Build Successful!
echo ============================================
echo.
echo To run:
echo   1. Start server: server.exe
echo   2. Start client: client.exe (in another terminal)
echo.
echo Multiple clients can connect simultaneously.
echo.

pause
