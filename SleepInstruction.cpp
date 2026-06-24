#include "SleepInstruction.h"

SleepInstruction::SleepInstruction(int pid, uint8_t ticks) 
	: Instruction(pid, SLEEP), ticks(ticks) {}

void SleepInstruction::execute() {
	Instruction::execute();
}

uint8_t SleepInstruction::getTicks() const {
	return ticks;
}