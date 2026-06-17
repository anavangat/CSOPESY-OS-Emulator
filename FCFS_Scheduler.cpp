#include "FCFS_Scheduler.h"
#include <chrono>
#include <thread>

void FCFS_Scheduler::schedulerLoop() {
	size_t nextToEnqueue = 0; // index of the next not-yet-queued process in allProcesses

	while (running) {
		// Push any freshly created processes into the ready queue,
		// in the same order they were created (creation order == FCFS order)
		while (nextToEnqueue < allProcesses.size()) {
			readyQueue.push(allProcesses[nextToEnqueue]);
			++nextToEnqueue;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // avoid busy-spinning
	}

}
