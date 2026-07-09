#pragma once
#include "AScheduler.h"
class RR_Scheduler : public AScheduler
{
public:
	RR_Scheduler(int numCpu, int batchProcessFreq, int minIns, int maxIns, int delaysPerExec, std::atomic<int>& cpuTick, int quantum, int maxOverallMem, int memPerFrame, int memPerProc)
		: AScheduler(numCpu, batchProcessFreq, minIns, maxIns, delaysPerExec, cpuTick,  maxOverallMem,  memPerFrame, memPerProc), quantum(quantum) {
	}

private:
	int quantum; // time quantum for round-robin scheduling
	void schedulerLoop() override; // implement round-robin scheduling algorithm
	void workerLoop(int coreID) override; // implement worker loop for round-robin scheduling
};

