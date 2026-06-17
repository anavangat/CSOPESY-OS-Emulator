#pragma once
#include <thread>
class Instruction
{
public:
	enum InstructionType {
		PRINT
	};

	Instruction(int pid, InstructionType type);
	InstructionType getInstructionType() const;
	virtual void execute() = 0;
	
protected:
	int pid;
	InstructionType type;
};

inline Instruction::InstructionType Instruction::getInstructionType() const
{
	return type;
}

inline Instruction::Instruction(int pid, InstructionType type) : pid(pid), type(type) { }

inline void Instruction::execute()
{
	// Base implementation can be empty or contain common logic for all instructions
}
