#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <memory>
#include "Instruction.h"
#include "SymbolTable.h"

class Process
{
public:
	enum ProcessState {
		READY,
		RUNNING,
		WAITING,
		FINISHED
	};

	Process(int pid, const std::string& name, std::time_t arrivalTime) : 
		pid(pid), name(name), state(READY), programCounter(0), coreID(-1), arrivalTime(arrivalTime) {}

	int getPid() const;
	std::string getName() const;
	ProcessState getState() const;
	void setState(ProcessState newState);
	int getCoreID() const;
	std::time_t getArrivalTime() const;
	int getRemainingInstructions() const;
	int getTotalInstructions() const;
	SymbolTable& getSymbolTable();

	void addInstruction(const std::shared_ptr<Instruction>& instruction);
	void executeCurrentInstruction();
	void moveToNextInstruction();
	bool isFinished() const;

private:
	int pid;
	std::string name;
	ProcessState state;
	int coreID; // Assigned core ID, -1 if not assigned
	std::time_t arrivalTime;


	int programCounter;
	std::vector<std::shared_ptr<Instruction>> instructions;
	int totalInstructions;
	int remainingInstructions;

	SymbolTable symbolTable;
};

