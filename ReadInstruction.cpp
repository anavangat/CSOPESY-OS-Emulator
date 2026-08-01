#include "ReadInstruction.h"
#include "Process.h"
#include "MemoryAllocator.h"

ReadInstruction::ReadInstruction(int pid, const std::string& variableName, int memoryAddress)
	: Instruction(pid, InstructionType::READ), variableName(variableName), memoryAddress(memoryAddress) {
}

void ReadInstruction::execute(Process& process, SymbolTable& symbolTable) {

	uint16_t value = 0;

	if (!process.getMemoryAllocator().read(process.getPid(), memoryAddress, value)) {
		process.reportMemoryViolation(memoryAddress);
		return;
	}

	symbolTable.setVariable(variableName, value);
}
