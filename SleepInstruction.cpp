#include "SleepInstruction.h"

SleepInstruction::SleepInstruction(int pid, uint8_t ticks) 
	: Instruction(pid, SLEEP), ticks(ticks) {}

void SleepInstruction::execute(Process& process, SymbolTable& symbolTable) {
	Instruction::execute(process, symbolTable);
}

uint8_t SleepInstruction::getTicks() const {
	return ticks;
}