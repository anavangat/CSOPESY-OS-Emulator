#pragma once
#include "Instruction.h"

class WriteInstruction : public Instruction
{
public:
    WriteInstruction(int pid, int memoryAddress, uint16_t value);

	void execute(Process& process, SymbolTable& symbolTable) override;

private:
	int memoryAddress;
	uint16_t value;
};

