#pragma once
#include <atomic>
#include <vector>
#include <thread>
#include <string>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <algorithm>
#include "Process.h"
#include "ReadyQueue.h"
#include "LogUtils.h"
#include "PrintInstruction.h"
#include "DeclareInstruction.h"
#include "AddInstruction.h"
#include "SubtractInstruction.h"
#include "SleepInstruction.h"
#include "ForInstruction.h"

class AScheduler
{
public:
	AScheduler(int numCpu, int batchProcessFreq, int minIns, int maxIns, int delaysPerExec, std::atomic<int>& cpuTick)
		: numCpu(numCpu), batchProcessFreq(batchProcessFreq), minIns(minIns), maxIns(maxIns), delaysPerExec(delaysPerExec), cpuTick(cpuTick) {
	}
	virtual ~AScheduler() = default;

	void start() {
		// create thread for scheduler
		schedulerThread = std::thread(&AScheduler::schedulerLoop, this);

		// create worker threads
		for (int i = 0; i < numCpu; i++) {
			workerThreads.emplace_back(&AScheduler::workerLoop, this, i);
		}

		dummyProcessGeneratorThread = std::thread(&AScheduler::dummyProcessGenerationLoop, this);
		sleepWakeThread = std::thread(&AScheduler::sleepWakeLoop, this);
	}

	void stop() {
		if (generationEnabled) {
			stopDummyProcessGeneration();
		}

		running = false;
		generationEnabled = false;

		for (int i = 0; i < numCpu; i++) {
			readyQueue.push(nullptr); // push nullptr to unblock worker threads
		}

		for (auto& t : workerThreads) {
			if (t.joinable()) t.join();
		}
		workerThreads.clear();

		if (schedulerThread.joinable()) {
			schedulerThread.join();
		}

		if (dummyProcessGeneratorThread.joinable()) {
			dummyProcessGeneratorThread.join();
		}

		if (sleepWakeThread.joinable()) {
			sleepWakeThread.join();
		}
	}

	void startDummyProcessGeneration() {
		std::cout << "started dummy process generation:" << std::endl;

		generationEnabled = true;
	}

	void stopDummyProcessGeneration() {
		std::cout << "stopped dummy process generation:" << std::endl;
		generationEnabled = false;
	}

	std::vector<std::shared_ptr<Process>> getProcessesByState(Process::ProcessState state) {
		std::vector<std::shared_ptr<Process>> result;
		std::lock_guard<std::mutex> lock(allProcessesMutex);
		for (const auto& process : allProcesses) {
			if (process->getState() == state) {
				result.push_back(process);
			}
		}
		return result;
	}

	// Resolves a process by name for "screen -r". Returns nullptr if no such name exists.
	std::shared_ptr<Process> getProcessByName(const std::string& name) {
		std::lock_guard<std::mutex> lock(allProcessesMutex);
		for (const auto& process : allProcesses) {
			if (process->getName() == name) {
				return process;
			}
		}
		return nullptr;
	}

	int getNumCpu() const {
		return numCpu;
	}

	std::shared_ptr<Process> createUserProcess(const std::string& name) {
		int newPid = pid++;
		auto process = std::make_shared<Process>(newPid, name, std::time(nullptr));

		int instructionCount = rand() % (maxIns - minIns + 1) + minIns;
		std::vector<std::string> variables;
		while (process->getTotalInstructions() < instructionCount) {
			int remaining = instructionCount - process->getTotalInstructions();
			process->addInstruction(createRandomInstruction(newPid, name, variables, remaining));
		}

		std::lock_guard<std::mutex> lock(allProcessesMutex);
		for (const auto& existing : allProcesses) {
			if (existing->getName() == name) {
				return nullptr; // duplicate name; do not register
			}
		}
		allProcesses.push_back(process);
		return process;
	}

protected:
	std::atomic<int>& cpuTick;

	// config vars
	int numCpu;
	int batchProcessFreq;
	int minIns;
	int maxIns;
	int delaysPerExec;

	std::thread schedulerThread; // thread for scheduler
	ReadyQueue readyQueue; // one ready queue for the scheduler

	// flag for running the scheduler and worker threads
	std::atomic<bool> running{ true };

	//  flag for generating processes
	std::atomic<bool> generationEnabled{ false };
	//int pid = 1; // PID counter for generating unique PIDs for dummy processes
	std::atomic<int> pid{ 1 }; // PID counter (atomic: shared by generator thread and screen -s)

	std::thread dummyProcessGeneratorThread; // thread for generating dummy processes

	std::vector<std::thread> workerThreads; // create numCpu threads

	// list of all processes for screen -ls and report-util
	std::vector<std::shared_ptr<Process>> allProcesses;
	std::mutex allProcessesMutex; // mutex to protect access to allProcesses

	std::vector<std::shared_ptr<Process>> sleepingProcesses; // list of sleeping processes
	std::mutex sleepingProcessesMutex; // mutex to protect access to sleepingProcesses
	std::thread sleepWakeThread; // thread for checking sleeping processes

	virtual void schedulerLoop() = 0; // inheriting classes implement scheduling algorithm here
	
	virtual void workerLoop(int coreID) {
		while (running) {
			auto process = readyQueue.pop();
			if (process == nullptr) break; // stop signal

			process->setState(Process::RUNNING);
			process->setCoreID(coreID);
			while (running && !process->isFinished()) {
				auto instruction = process->getCurrentInstruction();
				process->executeCurrentInstruction();
				LogUtils::print_command(cpuTick.load(), *process, coreID);

				bool isSleep = instruction && instruction->getInstructionType() == Instruction::SLEEP;
				int sleepTicks = 0;
				if (isSleep) {
					auto sleepInstruction = std::dynamic_pointer_cast<SleepInstruction>(instruction);
					if (sleepInstruction) {
						sleepTicks = sleepInstruction->getTicks();
					}
				}

				process->moveToNextInstruction();

				if (isSleep) {
					process->setCoreID(-1);
					process->setState(Process::WAITING);
					process->setWakeUpTick(cpuTick.load() + sleepTicks);
					{
						std::lock_guard<std::mutex> lock(sleepingProcessesMutex);
						sleepingProcesses.push_back(process);
					}
					break; // exit the inner loop to allow other processes to run
				}

				int waitStartTick = cpuTick.load();
				while (cpuTick.load() - waitStartTick < delaysPerExec) {
					 // wait for delaysPerExec ticks
				}
			}
			if (process->isFinished()) {
				process->setState(Process::FINISHED);
			}
		}
	}

	void dummyProcessGenerationLoop() {
		int lastTick = cpuTick.load();

		while (running) {
			if (generationEnabled && 
				cpuTick.load() - lastTick >= batchProcessFreq) {
				auto process = createProcess(pid++);
				{
					std::lock_guard<std::mutex> lock(allProcessesMutex);
					allProcesses.push_back(process);
				}
				lastTick = cpuTick.load();
			}
		}
	}

	void sleepWakeLoop() {
		while (running) {
			std::vector<std::shared_ptr<Process>> processesToWake;
			{
				std::lock_guard<std::mutex> lock(sleepingProcessesMutex);
				int currentTick = cpuTick.load();

				auto it = sleepingProcesses.begin();
				while (it != sleepingProcesses.end()) {
					if (currentTick >= (*it)->getWakeUpTick()) {
						processesToWake.push_back(*it);
						it = sleepingProcesses.erase(it);
					}
					else {
						it++;
					}
				}
			}
			
			for (auto& p : processesToWake) {
				p->setState(Process::READY);
				readyQueue.push(p);
			}
		}
	}

	std::shared_ptr<Process> createProcess(int pid) { //TODO: INCLUDE OTHER INSTRUCTIONS
		std::string nameStr = std::to_string(pid);
		if (nameStr.length() == 1){
			nameStr = "0" + nameStr;
		}

		std::string paddedName = "p" + nameStr;
		/**std::string n = std::to_string(pid);
		std::string name = "Process" + n;**/

		auto process = std::make_shared<Process>(pid, paddedName, std::time(nullptr)); 

		// generate instructions and add to process
		int instructionCount = rand() % (maxIns - minIns + 1) + minIns; // random number of instructions between minIns and maxIns

		std::vector<std::string> variables;

		while (process->getTotalInstructions() < instructionCount) {

			//std::string text = "Instruction " + std::to_string(i + 1) + " of " + paddedName;

			//process->addInstruction(std::make_shared<PrintInstruction>(pid, text));

			int remainingInstructions = instructionCount - process->getTotalInstructions();
			auto instruction = createRandomInstruction(pid, paddedName, variables, remainingInstructions);
			process->addInstruction(instruction);
		}

		return process;
	}


	std::shared_ptr<Instruction> createRandomInstruction(int pid, const std::string& paddedName, std::vector<std::string>& variables, int remainingInstructions) {
		int instructionType = rand() % 6; // random instruction type (0-5) - PRINT, SLEEP, ADD, SUBTRACT, FOR, DECLARE

		switch (instructionType) {
			case Instruction::PRINT:
			{
				if (variables.empty() || rand() % 2 == 0) { //if no variables or 50% chance, print a message of hello world
					return std::make_shared<PrintInstruction>(pid, "Hello world from " + paddedName);
				}
				else {
					std::string varToPrint = variables[rand() % variables.size()];
					return std::make_shared<PrintInstruction>(pid, "Value of variable " + varToPrint + " is: ", varToPrint);
				}
				break;
			}

			case Instruction::DECLARE:
			{
				std::string var = "var" + std::to_string(variables.size()); // create a new variable name sequentially
				uint16_t value = rand() % 100; // random value between 0 and 99;

				variables.push_back(var); // add to list of variables

				return std::make_shared<DeclareInstruction>(pid, var, value);
				break;
			}

			case Instruction::ADD:
			{
				if (variables.size() >= 2) {
					std::string dest = variables[rand() % variables.size()];
					std::string var1 = variables[rand() % variables.size()]; // select a random variable from the list
					std::string var2 = variables[rand() % variables.size()];
					return std::make_shared<AddInstruction>(pid, dest, var1, var2);
				}
				else if (variables.size() < 2) {
					std::string var = "var" + std::to_string(variables.size()); // create a new variable name sequentially
					uint16_t value = rand() % 100; // random value between 0 and 99;

					variables.push_back(var); // add to list of variables

					return std::make_shared<DeclareInstruction>(pid, var, value);
				}
				break;
			}

			case Instruction::SUBTRACT:
			{
				if (variables.size() >= 2) {
					std::string dest = variables[rand() % variables.size()];
					std::string var1 = variables[rand() % variables.size()]; // select a random variable from the list
					std::string var2 = variables[rand() % variables.size()];
					return std::make_shared<SubtractInstruction>(pid, dest, var1, var2);
				}
				else if (variables.size() < 2) {
					std::string var = "var" + std::to_string(variables.size()); // create a new variable name sequentially
					uint16_t value = rand() % 100; // random value between 0 and 99;

					variables.push_back(var); // add to list of variables

					return std::make_shared<DeclareInstruction>(pid, var, value);
				}
				break;
			}

			case Instruction::SLEEP:
			{
				int sleepTicks = rand() % 5 + 1; // random sleep ticks between 1 and 5
				return std::make_shared<SleepInstruction>(pid, sleepTicks);
				break;
			}

			case Instruction::FOR:
			{
				std::vector<std::shared_ptr<Instruction>> body;

				int bodySize = rand() % 3 + 1; // random body size between 1 and 3
				bodySize = std::min(bodySize, remainingInstructions); // body can't be bigger than remaining instructions

				for (int j = 0; j < bodySize; j++) {
					body.push_back(createRandomInstruction(pid, paddedName, variables, remainingInstructions));
				}

				int maxRepeats = std::max(1, remainingInstructions / static_cast<int>(body.size()));
				int repeats = rand() % 3 + 1; // random number of repeats between 1 and 3
				repeats = std::min(repeats, maxRepeats); // make sure that repeated body can fit in the remaining number of instructions

				return std::make_shared<ForInstruction>(pid, body, repeats);

				break;

			}

		}

		return nullptr;
	}


};