#pragma once
#include <thread>
#include <atomic>
#include "AScheduler.h"

class FCFS_Scheduler : public AScheduler
{
public:
	FCFS_Scheduler(int numCpu, int batchProcessFreq, int minIns, int maxIns, int delaysPerExec) 
		: AScheduler(numCpu, batchProcessFreq, minIns, maxIns, delaysPerExec) { }

private:
	void schedulerLoop() override;
};

