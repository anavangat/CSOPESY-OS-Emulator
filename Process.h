#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <memory>
#include <atomic>
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
		pid(pid), name(name), state(READY), programCounter(0), coreID(-1), arrivalTime(arrivalTime),
		totalInstructions(0) {}

	int getPid() const;
	std::string getName() const;
	ProcessState getState() const;
	void setState(ProcessState newState);
	int getCoreID() const;
	std::time_t getArrivalTime() const;
	int getTotalInstructions() const;
	int getRemainingInstructions() const;
	SymbolTable& getSymbolTable();

	void addInstruction(const std::shared_ptr<Instruction>& instruction);
	void executeCurrentInstruction();
	void moveToNextInstruction();
	bool isFinished() const;
	int coreID; // Assigned core ID, -1 if not assigned
	void setCoreID(int core);

private:
	int pid;
	std::string name;

	ProcessState state;
	std::atomic<ProcessState> state;
	int coreID; // Assigned core ID, -1 if not assigned
	std::time_t arrivalTime;


	std::vector<std::shared_ptr<Instruction>> instructions;
	int programCounter; // current instruction number being executed
	int totalInstructions;



	SymbolTable symbolTable;
};
