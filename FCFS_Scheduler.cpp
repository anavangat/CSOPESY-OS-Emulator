#include "FCFS_Scheduler.h"
#include <chrono>
#include <thread>

void FCFS_Scheduler::schedulerLoop() {
	// TODO: FCFS algorithm goes here

	while (!readyQueue.isEmpty()) {
		for (int i = 0; i < numCpu; ++i) {
			
		}
	}

}
