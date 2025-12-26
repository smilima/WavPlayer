@echo off
REM Simple script to build and run tests using CMake

echo ============================================
echo Building WavPlayer Tests
echo ============================================
echo.

REM Create build directory if it doesn't exist
if not exist build mkdir build
cd build

REM Configure CMake (detect Visual Studio automatically)
echo Configuring CMake...
cmake .. -A x64
if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Make sure CMake is installed and in your PATH.
    pause
    exit /b 1
)

echo.
echo ============================================
echo Building tests...
echo ============================================
cmake --build . --config Debug --target WavPlayerTests
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ============================================
echo Running tests...
echo ============================================
echo.
Debug\WavPlayerTests.exe

echo.
echo ============================================
echo Tests completed!
echo ============================================
pause
