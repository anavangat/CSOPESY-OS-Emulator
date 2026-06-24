#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <memory>
#include <atomic>
#include "Instruction.h"
#include "ForInstruction.h"
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
	std::shared_ptr<Instruction> getCurrentInstruction() const;
	bool isFinished() const;
	void setCoreID(int core);

	void setWakeUpTick(int tick);
	int getWakeUpTick() const;

	void addInMemoryLog(const std::string& logline);
	const std::vector<std::string>& getInMemoryLogs() const;

private:
	int pid;
	int coreID; //Assigned core ID, -1 if not assigned
	std::string name;

	std::atomic<ProcessState> state;
	std::time_t arrivalTime;

	std::vector<std::shared_ptr<Instruction>> instructions;
	int programCounter; // current instruction number being executed
	int totalInstructions;

	int wakeUpTick = 0; // The tick at which the process should wake up from waiting state

	std::vector<std::string> inMemoryLogs;

	SymbolTable symbolTable;
};
