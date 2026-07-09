#pragma once
#include <thread>
#include <atomic>
#include "AScheduler.h"

class FCFS_Scheduler : public AScheduler
{
public:
	FCFS_Scheduler(int numCpu, int batchProcessFreq, int minIns, int maxIns, int delaysPerExec, std::atomic<int>& cpuTick, int maxOverallMem, int memPerFrame, int memPerProc)
		: AScheduler(numCpu, batchProcessFreq, minIns, maxIns, delaysPerExec, cpuTick,  maxOverallMem,  memPerFrame,  memPerProc) { }

private:
	void schedulerLoop() override;
};

