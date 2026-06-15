#include "PrintInstruction.h"
#include <iostream>
#include <string>

PrintInstruction::PrintInstruction(int pid, const std::string& toPrint)
	: Instruction(pid, Instruction::PRINT), toPrint(toPrint)
{
}

void PrintInstruction::execute()
{
	Instruction::execute(); // Call base class execute if needed (not implemented here, but can be used for logging or other purposes)

	std::cout << "Process " << pid << ": " << toPrint << std::endl;
}