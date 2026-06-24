#pragma once
#include "AScheduler.h"
class RR_Scheduler : public AScheduler
{
public:
	RR_Scheduler(int numCpu, int batchProcessFreq, int minIns, int maxIns, int delaysPerExec, std::atomic<int>& cpuTick, int quantum)
		: AScheduler(numCpu, batchProcessFreq, minIns, maxIns, delaysPerExec, cpuTick), quantum(quantum) {
	}

private:
	int quantum; // time quantum for round-robin scheduling
	void schedulerLoop() override; // implement round-robin scheduling algorithm
	void workerLoop(int coreID) override; // implement worker loop for round-robin scheduling
};

