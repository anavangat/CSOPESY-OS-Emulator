#pragma once
#include "Instruction.h"
#include <string>

class SubtractInstruction : public Instruction 
{
public:
	SubtractInstruction(int pid, const std::string& destVar, const std::string& srcVar1, const std::string& srcVar2);

	void execute(Process& process, SymbolTable& symbolTable) override;
private:
	std::string destVar;
	std::string srcVar1;
	std::string srcVar2;
};

