#include "RR_Scheduler.h"
#include "Process.h"
#include "LogUtils.h"
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
		
		process->setState(Process::RUNNING);
		process->setCoreID(coreID);
		int executedThisQuantum = 0;
		while (running && !process->isFinished() && executedThisQuantum < quantum) {
			process->executeCurrentInstruction();
			LogUtils::print_command(*process, coreID);
			process->moveToNextInstruction();
			executedThisQuantum++;
			int waitStartTick = cpuTick.load();
			while (cpuTick.load() - waitStartTick < delaysPerExec) {
				// wait for delaysPerExec ticks
			}
		}
		if (process->isFinished()) {
			process->setState(Process::FINISHED);
		}
		else {
			process->setState(Process::READY);
			readyQueue.push(process); // re-enqueue the process for the next round
		}
	}
}