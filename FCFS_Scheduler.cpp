#include "FCFS_Scheduler.h"
#include <chrono>+
#include <thread>

void FCFS_Scheduler::schedulerLoop() {
	// TODO: FCFS algorithm goes here

	int pid = 1;

	generationEnabled = true;

	while (generationEnabled)
	{
		auto process = createProcess(pid++); // create a new process with unique PID

		allProcesses.push_back(process); // keep track of all processes

		readyQueue.push(process); // add the new process to the ready queue for execution

		std::this_thread::sleep_for(std::chrono::seconds(batchProcessFreq)); // wait for the specified batch process frequency before creating the next process
	
	}

}
