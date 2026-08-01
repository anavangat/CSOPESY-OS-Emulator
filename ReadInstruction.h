#pragma once
#include "Instruction.h"
#include <string>

class ReadInstruction : public Instruction
{
public:
	ReadInstruction(int pid, const std::string& variableName, int memoryAddress);

	void execute(Process& process, SymbolTable& symbolTable) override;

private:
	std::string variableName;
	int memoryAddress;
};

