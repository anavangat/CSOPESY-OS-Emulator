#pragma once
#include <cstdint>
#include "Instruction.h"

class SleepInstruction : public Instruction
{
public:
	SleepInstruction(int pid, uint8_t ticks);
	void execute() override;
	uint8_t getTicks() const;

private:
	uint8_t ticks;
};

