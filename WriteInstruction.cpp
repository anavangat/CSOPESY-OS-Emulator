#include "WriteInstruction.h"
#include "Process.h"
#include "MemoryAllocator.h"

WriteInstruction::WriteInstruction(int pid, int memoryAddress, uint16_t value)
	: Instruction(pid, InstructionType::WRITE), memoryAddress(memoryAddress), value(value) {
}

void WriteInstruction::execute(Process& process, SymbolTable& symbolTable) {

    if (!process.getMemoryAllocator().write(process.getPid(), memoryAddress,value))
    {
        process.reportMemoryViolation(memoryAddress);
        return;
    }
}
