#pragma once
#include "Instruction.h"
#include <string>

class DeclareInstruction : public Instruction
{
public:
	DeclareInstruction(int pid, const std::string& variableName, uint16_t initialValue = 0);

	void execute(Process& process, SymbolTable& symbolTable) override;

private:
	std::string variableName;
	uint16_t initialValue;

};

