#include "DeclareInstruction.h"
#include <stdexcept>
#include "Process.h"

DeclareInstruction::DeclareInstruction(int pid, const std::string& variableName, uint16_t initialValue)
	: Instruction(pid, DECLARE), variableName(variableName), initialValue(initialValue) {
}

void DeclareInstruction::execute(Process& process, SymbolTable& symbolTable) {
	
	if (!symbolTable.hasVariable(variableName)) {
		bool declared = symbolTable.setVariable(variableName, initialValue);
		if (!declared) {
			std::string errorLine = "DECLARE failed: symbol table full, could not declare '" + variableName + "'";
			process.appendOutput(errorLine);
			return; // skip the success log below
		}
	}

	std::string logLine = "Assignment: " + variableName + "=" + std::to_string(initialValue);
	process.appendOutput(logLine);
}