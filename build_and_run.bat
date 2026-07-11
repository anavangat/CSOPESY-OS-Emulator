@echo off
cls
echo ===================================================
echo  Compiling CSOPESY OS Emulator via MSVC (cl)...
echo ===================================================


cl /EHsc /std:c++17 main.cpp LogEntry.cpp LogUtils.cpp Process.cpp FCFS_Scheduler.cpp RR_Scheduler.cpp Instruction.cpp PrintInstruction.cpp DeclareInstruction.cpp AddInstruction.cpp SubtractInstruction.cpp SleepInstruction.cpp ForInstruction.cpp ReadyQueue.cpp SymbolTable.cpp MemoryAllocator.cpp /Fe:emulator.exe
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