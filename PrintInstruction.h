#pragma once
#include <string>
#include "Instruction.h"
class PrintInstruction : public Instruction
{
public:
	PrintInstruction(int pid, const std::string& toPrint);
	void execute() override;
private:
	std::string toPrint;
};

