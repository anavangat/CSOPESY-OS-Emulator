#pragma once
#include <thread>
#include "SymbolTable.h"
class Process; // Forward declaration of Process class to avoid circular dependency while including Process.h functionalities


class Instruction
{
public:
	enum InstructionType {
		PRINT,
		DECLARE,
		ADD,
		SUBTRACT,
		SLEEP,
		FOR
	};
	
	Instruction(int pid, InstructionType type);
	InstructionType getInstructionType() const;
	virtual void execute(Process& process, SymbolTable& symbolTable);

	
	
protected:
	int pid;
	InstructionType type;
};

inline Instruction::InstructionType Instruction::getInstructionType() const
{
	return type;
}

inline Instruction::Instruction(int pid, InstructionType type) : pid(pid), type(type) { }

inline void Instruction::execute(Process& process, SymbolTable& symbolTable) //Executing instruction needs the process's symbol table to access variables and their values
{
	// Base implementation can be empty or contain common logic for all instructions
}
