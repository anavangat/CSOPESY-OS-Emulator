#pragma once
#include <string>
#include "Instruction.h"
#include "SymbolTable.h"
class PrintInstruction : public Instruction
{
public:
	PrintInstruction(int pid, const std::string& toPrint, const std::string& variableName = "");
	void execute(Process& process, SymbolTable& symbolTable) override;
private:
	std::string toPrint;
	std::string variableName; // Optional variable name to print its value
};

