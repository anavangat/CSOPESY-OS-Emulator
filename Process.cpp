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

	std::vector<std::shared_ptr<Instruction>> flattenedInstructions;
	ForInstruction::flatten(instruction, flattenedInstructions);

	for (const auto& instr : flattenedInstructions) {
		instructions.push_back(instr);
		++totalInstructions;
	}
}

void Process::executeCurrentInstruction() {
	if (programCounter < 0 || programCounter >= static_cast<int>(instructions.size())) {
		return;
	}

	auto instruction = instructions[programCounter];
	if (instruction) {
		instruction->execute(*this, symbolTable);
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

void Process::addInMemoryLog(int tick, int coreID, const std::string& event){
	std::lock_guard<std::mutex> lock(logMutex);

	inMemoryLogs.emplace_back( tick, coreID, pid, event );
}

 std::vector<LogEntry> Process::getInMemoryLogs() const {
	std::lock_guard<std::mutex> lock(logMutex);
	return inMemoryLogs;
}

void Process::appendOutput(const std::string& output) {
	std::lock_guard<std::mutex> lock(outputMutex);
	outputBuffer.push_back(output);
}

std::vector<std::string> Process::getOutput()const {
	std::lock_guard<std::mutex> lock(outputMutex);
	return outputBuffer;
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

int Process::getMemoryRequired() const
{
	return memoryRequired;
}