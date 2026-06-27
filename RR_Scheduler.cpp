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

void RR_Scheduler::schedulerLoop() {
	size_t nextToEnqueue = 0; // index of the next not-yet-queued process in allProcesses

	while (running) {
		std::vector<std::shared_ptr<Process>> newProcesses;
		{
			std::lock_guard<std::mutex> lock(allProcessesMutex);
			while (nextToEnqueue < allProcesses.size()) {
				newProcesses.push_back(allProcesses[nextToEnqueue]);
				++nextToEnqueue;
			}
		}
		for (auto& p : newProcesses) {
			readyQueue.push(p);
		}
		//std::this_thread::sleep_for(std::chrono::milliseconds(10)); // avoid busy-spinning
	}
}

void RR_Scheduler::workerLoop(int coreID) {
	while (running) {
		auto process = readyQueue.pop();
		if (process == nullptr) break; // stop signal
		
		process->setCoreID(coreID);
		process->setState(Process::RUNNING);
		int executedThisQuantum = 0;
		bool wentToSleep = false;

		while (running && !process->isFinished() && executedThisQuantum < quantum) {
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
			executedThisQuantum++;

			if (isSleep) {
				process->setCoreID(-1);
				process->setState(Process::WAITING);
				process->setWakeUpTick(cpuTick.load() + sleepTicks);
				{
					std::lock_guard<std::mutex> lock(sleepingProcessesMutex);
					sleepingProcesses.push_back(process);
				}
				wentToSleep = true;
				break; // exit the quantum loop if the process goes to sleep
			}

			int waitStartTick = cpuTick.load();
			while (cpuTick.load() - waitStartTick < delaysPerExec) {
				// wait for delaysPerExec ticks
			}
		}
		if (process->isFinished()) {
			process->setState(Process::FINISHED);
		}
		else if (!wentToSleep) {
			process->setCoreID(-1);
			process->setState(Process::READY);
			readyQueue.push(process); // re-enqueue the process for the next round
		}
	}
}