#pragma once
#include "AScheduler.h"

class RR_Scheduler : public AScheduler
{
public:
	RR_Scheduler(int numCpu, int batchProcessFreq, int minIns, int maxIns, int delaysPerExec, std::atomic<int>& cpuTick, int quantum, int maxOverallMem, int memPerFrame, int minMemPerProc, int maxMemPerProc)
		: AScheduler(numCpu, batchProcessFreq, minIns, maxIns, delaysPerExec, cpuTick,  maxOverallMem,  memPerFrame, minMemPerProc, maxMemPerProc), quantum(quantum) {
	}

private:
	int quantum; // time quantum for round-robin scheduling
	void workerLoop(int coreID) override; // implement worker loop for round-robin scheduling
};

