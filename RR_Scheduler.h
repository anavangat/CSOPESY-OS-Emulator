#pragma once
#include "AScheduler.h"

class RR_Scheduler : public AScheduler
{
public:
	RR_Scheduler(int numCpu, int batchProcessFreq, int minIns, int maxIns, int delaysPerExec, std::atomic<int>& cpuTick, int quantum, int maxOverallMem, int memPerFrame, int memPerProc)
		: AScheduler(numCpu, batchProcessFreq, minIns, maxIns, delaysPerExec, cpuTick,  maxOverallMem,  memPerFrame, memPerProc), quantum(quantum) {
	}

	void start() override;
	void stop() override;

private:
	int quantum; // time quantum for round-robin scheduling
	void schedulerLoop() override; // implement round-robin scheduling algorithm
	void workerLoop(int coreID) override; // implement worker loop for round-robin scheduling

	// FOR FIRST-FIT MA ASSIGNMENT
	std::thread memorySnapshotThread; // for writing txt files
	std::atomic<int> snapshotCounter{ 0 }; // qq
	void memorySnapshotLoop();
};

