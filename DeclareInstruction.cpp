#include "DeclareInstruction.h"
#include <stdexcept>
#include "Process.h"

DeclareInstruction::DeclareInstruction(int pid, const std::string& variableName, uint16_t initialValue)
	: Instruction(pid, DECLARE), variableName(variableName), initialValue(initialValue) {
}

void DeclareInstruction::execute(Process& process, SymbolTable& symbolTable) {
	if (!symbolTable.hasVariable(variableName)) {
		symbolTable.setVariable(variableName, initialValue);
	}
	else {
		//throw std::runtime_error("Variable '" + variableName + "' already declared."); //throw error if declared a variable that already exists
	}

	std::string logLine = "Assignment: " + variableName + "=" + std::to_string(initialValue);
	process.appendOutput(logLine);
}