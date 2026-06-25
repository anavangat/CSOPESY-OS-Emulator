#include "PrintInstruction.h"
#include <iostream>
#include <string>
#include "SymbolTable.h"
#include "Process.h"

PrintInstruction::PrintInstruction(int pid, const std::string& toPrint, const std::string& variableName)
	: Instruction(pid, PRINT), toPrint(toPrint), variableName(variableName)
{
}

void PrintInstruction::execute(Process& process, SymbolTable& symbolTable) //TODO: make it only print in the process not the console
{
	std::string output = toPrint;

	if (!variableName.empty()) { //check if variable name is provided
		if (!symbolTable.hasVariable(variableName))
		{
			symbolTable.setVariable(variableName, 0); // Initialize to 0 if not declared
		}

		output += std::to_string(symbolTable.getVariable(variableName)); // Append the variable's value to the output
	}
	//for testing
	//std::cout << "output << std::endl;

	process.appendOutput(output); // Append the output to the process's output log

}