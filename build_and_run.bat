@echo off
cls
echo ===================================================
echo  Compiling CSOPESY OS Emulator via MSVC (cl)...
echo ===================================================

:: /EHsc enables standard exception handling
:: /std:c++17 specifies the language standard
:: /Fe: sets the output executable name
cl /EHsc /std:c++17 main.cpp LogUtils.cpp Process.cpp FCFS_Scheduler.cpp Instruction.cpp PrintInstruction.cpp ReadyQueue.cpp SymbolTable.cpp /Fe:emulator.exe

:: Check if compilation succeeded
if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] Compilation complete. Launching emulator.exe...
    echo ---------------------------------------------------
    emulator.exe
) else (
    echo.
    echo [ERROR] Compilation failed. Please check the errors above.
)

echo.
echo ---------------------------------------------------
pause