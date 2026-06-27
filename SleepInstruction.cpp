#include "SleepInstruction.h"
#include "Process.h"

SleepInstruction::SleepInstruction(int pid, uint8_t ticks) 
	: Instruction(pid, SLEEP), ticks(ticks) {}

void SleepInstruction::execute(Process& process, SymbolTable& symbolTable) {
	Instruction::execute(process, symbolTable);

	//Comment this one if not needed or not in specs :>
	std::string logLine = "Instruction: SLEEP for " + std::to_string(ticks) + " ticks.";
    process.appendOutput(logLine);
}

uint8_t SleepInstruction::getTicks() const {
	return ticks;
}