@echo off
echo Building Market Making Optimizer...

REM Try to find Visual Studio
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    echo Found Visual Studio 2022 Community
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    echo Found Visual Studio 2019 Community
) else (
    echo Visual Studio not found! Please install Visual Studio with C++ support.
    pause
    exit /b 1
)

REM Navigate to build directory
cd build

REM Try cmake
where cmake >nul 2>&1
if %errorlevel% equ 0 (
    echo Building with CMake...
    cmake --build . --config Release --target database_test
    if %errorlevel% equ 0 (
        echo Build successful!
        echo Testing database connection...
        .\Release\database_test.exe
    ) else (
        echo Build failed!
    )
) else (
    echo CMake not found in PATH
    echo Please add CMake to your PATH or use Visual Studio GUI
)

pause

