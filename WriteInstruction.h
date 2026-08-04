#pragma once
#include "Instruction.h"
#include <string>

class WriteInstruction : public Instruction
{
public:
	// Literal form:  WRITE 0x500 42
	WriteInstruction(int pid, int memoryAddress, uint16_t value);
	// Variable form: WRITE 0x500 varA  -- used by the spec's own screen -c sample program.
	WriteInstruction(int pid, int memoryAddress, const std::string& sourceVariable);

	void execute(Process& process, SymbolTable& symbolTable) override;

private:
	int memoryAddress;
	uint16_t value;
	std::string sourceVariable; // empty => literal form
};