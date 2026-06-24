#include "Process.h"

int Process::getPid() const {
	return pid;
}

std::string Process::getName() const {
	return name;
}

Process::ProcessState Process::getState() const {
	return state.load();
}

void Process::setState(ProcessState newState) {
	state.store(newState);
}

int Process::getCoreID() const {
	return coreID;
}

std::time_t Process::getArrivalTime() const {
	return arrivalTime;
}

int Process::getRemainingInstructions() const {
	return totalInstructions - programCounter;
}

int Process::getTotalInstructions() const {
	return totalInstructions;
}

SymbolTable& Process::getSymbolTable() {
	return symbolTable;
}

void Process::addInstruction(const std::shared_ptr<Instruction>& instruction) {
	if (!instruction) {
		return;
	}

	instructions.push_back(instruction);
	++totalInstructions;
}

void Process::executeCurrentInstruction() {
	if (programCounter < 0 || programCounter >= static_cast<int>(instructions.size())) {
		return;
	}

	auto instruction = instructions[programCounter];
	if (instruction) {
		instruction->execute();
	}
}

void Process::moveToNextInstruction() {
	if (programCounter < static_cast<int>(instructions.size())) {
		++programCounter;
	}
}

std::shared_ptr<Instruction> Process::getCurrentInstruction() const {
	if (programCounter < 0 || programCounter >= static_cast<int>(instructions.size())) {
		return nullptr;
	}
	return instructions[programCounter];
}

bool Process::isFinished() const {
	return programCounter >= static_cast<int>(instructions.size());
}

void Process::addInMemoryLog(const std::string &logline){
	inMemoryLogs.push_back(logline);
}

void Process::setCoreID(int core) {
    this->coreID = core;
}

void Process::setWakeUpTick(int tick) {
	this->wakeUpTick = tick;
}

int Process::getWakeUpTick() const {
	return wakeUpTick;
}
