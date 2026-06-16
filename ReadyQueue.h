#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include "Process.h"
class ReadyQueue
{
public:
	void push(std::shared_ptr<Process> process);
	std::shared_ptr<Process> pop();

private:
	std::queue<std::shared_ptr<Process>> queue;
	std::mutex mutex;
	std::condition_variable condition;
};

