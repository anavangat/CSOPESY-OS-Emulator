#include "FCFS_Scheduler.h"
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <memory>
#include "Process.h"

void FCFS_Scheduler::schedulerLoop() {
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
