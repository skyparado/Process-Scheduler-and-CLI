@echo off
REM ===========================================================================
REM build.bat  -  build the CSOPESY emulator + tests with MSVC (cl.exe)
REM Opens the VS Build Tools environment, then compiles into .\build\.
REM Usage:   build.bat          (build emulator + tests)
REM          build.bat test     (build then run the unit tests)
REM ===========================================================================
setlocal

set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
    echo [build] Could not find vcvars64.bat. Edit VCVARS in build.bat to point
    echo         at your Visual Studio install, or build with CMake instead.
    exit /b 1
)
call "%VCVARS%" >nul

if not exist build mkdir build

echo [build] Compiling emulator...
cl /nologo /EHsc /std:c++17 /W3 /Fe:build\csopesy.exe /Fo:build\ ^
   src\main.cpp src\Config.cpp src\CommandInterpreter.cpp src\Console.cpp src\Scheduler.cpp src\Process.cpp
if errorlevel 1 exit /b 1

echo [build] Compiling tests...
cl /nologo /EHsc /std:c++17 /W3 /Fe:build\csopesy_tests.exe /Fo:build\ ^
   tests\test_main.cpp src\Config.cpp src\CommandInterpreter.cpp src\Console.cpp src\Scheduler.cpp src\Process.cpp
if errorlevel 1 exit /b 1

echo [build] Done. Binaries in .\build\

if /I "%~1"=="test" (
    echo [build] Running tests...
    build\csopesy_tests.exe
)
endlocal
