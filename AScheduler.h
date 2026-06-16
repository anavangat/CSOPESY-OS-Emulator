#pragma once
#include <atomic>
#include <vector>
#include <thread>
#include <string>
#include <memory>
#include "Process.h"
#include "ReadyQueue.h"
#include "PrintInstruction.h"

class AScheduler
{
public:
	AScheduler(int numCpu, int batchProcessFreq, int minIns, int maxIns, int delaysPerExec)
		: numCpu(numCpu), batchProcessFreq(batchProcessFreq), minIns(minIns), maxIns(maxIns), delaysPerExec(delaysPerExec) {
	}
	virtual ~AScheduler() = default;

	void start() {
		generationEnabled = false; //SHould this be set to true?

		// create thread for scheduler
		schedulerThread = std::thread(&AScheduler::schedulerLoop, this);

		// create worker threads
		for (int i = 0; i < numCpu; i++) {
			workerThreads.emplace_back(&AScheduler::workerLoop, this, i);
		}
	}

	void stop() {
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
	}

	/* Sent to scheduler loop of FCFS
	void startDummyProcessGeneration() {
		int pid = 1;
		while (generationEnabled) {
			auto process = createProcess(pid++);
			allProcesses.push_back(process);
			readyQueue.push(process);
		}
	}

	*/

	virtual void schedulerLoop() = 0; // inheriting classes implement scheduling algorithm here

	void stopDummyProcessGeneration() {
		generationEnabled = false;
	}

protected:
	// config vars
	int numCpu;
	int batchProcessFreq;
	int minIns;
	int maxIns;
	int delaysPerExec;

	std::thread schedulerThread; // thread for scheduler
	ReadyQueue readyQueue; // one ready queue for the scheduler

	//  flag for generating processes
	std::atomic<bool> generationEnabled{ false };

	std::vector<std::thread> workerThreads; // create numCpu threads

	// list of all processes for screen -ls and report-util
	std::vector<std::shared_ptr<Process>> allProcesses;

	std::shared_ptr<Process> createProcess(int pid) {
		std::string n = std::to_string(pid);
		std::string name = "Process" + n;

		auto process = std::make_shared<Process>(pid, name, std::time(nullptr));

		// TODO: generate instructions and add to process

		int instructionCount = rand() % (maxIns - minIns + 1) + minIns; // random number of instructions between minIns and maxIns

		for (int i = 0; i < instructionCount; i++) {

			std::string text = "Instruction " + std::to_string(i + 1) + " of " + name;

			process->addInstruction(std::make_shared<PrintInstruction>(pid, text));
		}


		return process;
	}

	
	
	void workerLoop(int coreID) {
		while (true) {
			auto process = readyQueue.pop();

			if (process == nullptr) break; // stop signal

			process->setState(Process::RUNNING);
			
			while (!process->isFinished()) {
				process->executeCurrentInstruction();
				process->moveToNextInstruction();
				std::this_thread::sleep_for(std::chrono::milliseconds(delaysPerExec)); // apply delay per instruction execution
			}

			process->setState(Process::FINISHED);
		}
	}
};