#include "SubtractInstruction.h"
#include "Process.h"


SubtractInstruction::SubtractInstruction(int pid, const std::string& destVar, const std::string& srcVar1, const std::string& srcVar2)
	: Instruction(pid, SUBTRACT), destVar(destVar), srcVar1(srcVar1), srcVar2(srcVar2) {
}

void SubtractInstruction::execute(Process& process, SymbolTable& symbolTable) {
	if (!symbolTable.hasVariable(srcVar1)) {
		symbolTable.setVariable(srcVar1, 0); // Initialize to 0 if not declared
	}
	if (!symbolTable.hasVariable(srcVar2)) {
		symbolTable.setVariable(srcVar2, 0); // Initialize to 0 if not declared
	}
	if (!symbolTable.hasVariable(destVar)) {
		symbolTable.setVariable(destVar, 0); // Initialize to 0 if not declared
	}
	uint16_t value1 = symbolTable.getVariable(srcVar1);
	uint16_t value2 = symbolTable.getVariable(srcVar2);
	int32_t result = static_cast<int32_t>(value1) - static_cast<int32_t>(value2); // Use int32_t to handle potential negative results before clamping to 0

	if (result < 0) {
		result = 0; // Ensure result does not go below 0
	}
	else if (result > 65535) {
		result = 65535; // Ensure result does not exceed 16-bit unsigned integer max value
	}

	symbolTable.setVariable(destVar, static_cast<uint16_t>(result));

	std::string logLine = destVar + " = " + srcVar1 + " (" + std::to_string(value1) + ") - " 
                        + srcVar2 + " (" + std::to_string(value2) + ") is performed. " 
                        + destVar + " is now " + std::to_string(result) + ".";
    process.appendOutput(logLine);
}