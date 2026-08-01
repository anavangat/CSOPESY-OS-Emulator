#include "RR_Scheduler.h"
#include "Process.h"
#include "LogUtils.h"
#include "SleepInstruction.h"
#include "Instruction.h"
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <memory>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>

void RR_Scheduler::workerLoop(int coreID) {
	int idleTickStart = cpuTick.load();

	while (running) {
		auto process = readyQueue.pop();
		if (process == nullptr) {
			idleCpuTicks += (cpuTick.load() - idleTickStart);
			break; // stop signal
		}

		if (!memoryAllocator.isAllocated(process->getPid())) {
			if (!memoryAllocator.allocate(process->getPid(), process->getMemoryRequired())) {
				readyQueue.push(process);
				continue; // skip this process and move to the next one
			}
		}
		process->setMemoryAllocator(&memoryAllocator);

		idleCpuTicks += (cpuTick.load() - idleTickStart);
		int activeTickStart = cpuTick.load();
		
		process->setCoreID(coreID);
		process->setState(Process::RUNNING);
		int executedThisQuantum = 0;
		bool wentToSleep = false;
		bool wasMemoryViolated = false;

		while (running && !process->isFinished() && executedThisQuantum < quantum) {
			auto instruction = process->getCurrentInstruction();
			process->executeCurrentInstruction();

			//Check for memory violation after executing the instruction
			if (process->hasMemoryViolation()) {
				memoryAllocator.deallocate(process->getPid());
				process->setState(Process::FINISHED);
				process->setCoreID(-1);
				wasMemoryViolated = true;
				break;
			}

			LogUtils::print_command(cpuTick.load(), *process, coreID);

			executedThisQuantum++;
			process->moveToNextInstruction();

			if (putToSleepIfNeeded(process, instruction)) {
				wentToSleep = true;
				break; // exit the quantum loop if the process goes to sleep
			}

			waitForExecDelay();
		}

		if (!wasMemoryViolated) {
			if (process->isFinished()) {
				memoryAllocator.deallocate(process->getPid());
				process->setState(Process::FINISHED);
			}
			else if (!wentToSleep) {
				process->setState(Process::READY);
				process->setCoreID(-1);
				readyQueue.push(process); // re-enqueue the process for the next round
			}
		}

		activeCpuTicks += (cpuTick.load() - activeTickStart);
		idleTickStart = cpuTick.load();
	}
}
