#include "Process.h"

int Process::getPid() const {
	return pid;
}

std::string Process::getName() const {
	return name;
}

Process::ProcessState Process::getState() const {
	return state;
}

void Process::setState(ProcessState newState) {
	state = newState;
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

bool Process::isFinished() const {
	return programCounter >= static_cast<int>(instructions.size());
}
