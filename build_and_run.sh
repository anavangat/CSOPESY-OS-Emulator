#!/bin/bash

clear
echo "==================================================="
echo " Compiling CSOPESY OS Emulator via Clang/G++..."
echo "==================================================="

# Compile all source files using C++17 standard
clang++ -std=c++17 \
  main.cpp \
  LogEntry.cpp \
  LogUtils.cpp \
  Process.cpp \
  FCFS_Scheduler.cpp \
  RR_Scheduler.cpp \
  Instruction.cpp \
  PrintInstruction.cpp \
  DeclareInstruction.cpp \
  AddInstruction.cpp \
  SubtractInstruction.cpp \
  SleepInstruction.cpp \
  ForInstruction.cpp \
  ReadyQueue.cpp \
  SymbolTable.cpp \
  MemoryAllocator.cpp \
  -o emulator

# $? stores the exit status of the compile command (equivalent to %errorlevel%)
if [ $? -eq 0 ]; then
    echo ""
    echo "[SUCCESS] Compilation complete. Launching emulator..."
    echo "---------------------------------------------------"
    ./emulator
else
    echo ""
    echo "[ERROR] Compilation failed. Please check the errors above."
fi

echo ""
echo "---------------------------------------------------"