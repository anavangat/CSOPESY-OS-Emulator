#pragma once
#include <atomic>
#include <vector>
#include <thread>
#include <string>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include "Process.h"
#include "ReadyQueue.h"
#include "PrintInstruction.h"
#include "SleepInstruction.h"
#include "LogUtils.h"

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
		//sleepWakeThread = std::thread(&AScheduler::, this);
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
	int pid = 1; // PID counter for generating unique PIDs for dummy processes

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
				LogUtils::print_command(*process, coreID);

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

	std::shared_ptr<Process> createProcess(int pid) {
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

		for (int i = 0; i < instructionCount; i++) {

			std::string text = "Instruction " + std::to_string(i + 1) + " of " + paddedName;

			process->addInstruction(std::make_shared<PrintInstruction>(pid, text));
		}


		return process;
	}


};