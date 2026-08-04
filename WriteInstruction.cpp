#include "WriteInstruction.h"
#include "Process.h"
#include "MemoryAllocator.h"
#include <sstream>

WriteInstruction::WriteInstruction(int pid, int memoryAddress, uint16_t value)
	: Instruction(pid, InstructionType::WRITE), memoryAddress(memoryAddress), value(value), sourceVariable("") {
}

WriteInstruction::WriteInstruction(int pid, int memoryAddress, const std::string& sourceVariable)
	: Instruction(pid, InstructionType::WRITE), memoryAddress(memoryAddress), value(0), sourceVariable(sourceVariable) {
}

void WriteInstruction::execute(Process& process, SymbolTable& symbolTable) {

	uint16_t valueToWrite = value;

	if (!sourceVariable.empty()) {
		if (!symbolTable.hasVariable(sourceVariable)) {
			symbolTable.setVariable(sourceVariable, 0); // same auto-declare fallback ADD/SUBTRACT use
		}
		valueToWrite = symbolTable.getVariable(sourceVariable);
	}

	if (!process.getMemoryAllocator().write(process.getPid(), memoryAddress, valueToWrite))
	{
		process.reportMemoryViolation(memoryAddress);
		return;
	}

	std::stringstream logLine;
	logLine << "WRITE " << valueToWrite << " to 0x" << std::hex << memoryAddress << std::dec << ".";
	process.appendOutput(logLine.str());
}