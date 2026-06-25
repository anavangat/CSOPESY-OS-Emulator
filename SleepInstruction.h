#pragma once
#include <cstdint>
#include "Instruction.h"
#include "SymbolTable.h"

class SleepInstruction : public Instruction
{
public:
	SleepInstruction(int pid, uint8_t ticks);
	void execute(Process& process, SymbolTable& symbolTable) override;
	uint8_t getTicks() const;

private:
	uint8_t ticks;
};

