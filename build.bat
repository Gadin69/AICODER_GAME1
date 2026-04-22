@echo off
echo Building Game Engine...

REM Create build directory
if not exist build mkdir build
cd build

REM Run CMake
cmake .. -G "Visual Studio 16 2019" -A x64
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!
echo Run the executable from: build\bin\Release\GameEngine.exe
pause
