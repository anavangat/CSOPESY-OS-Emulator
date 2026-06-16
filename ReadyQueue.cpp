#include "ReadyQueue.h"

void ReadyQueue::push(std::shared_ptr<Process> process) {
	{
		std::lock_guard<std::mutex> lock(mutex);
		queue.push(process);
	}

	condition.notify_one();
}

std::shared_ptr<Process> ReadyQueue::pop() {
	std::unique_lock<std::mutex> lock(mutex);
	condition.wait(lock, [this]() { return !queue.empty(); });

	auto process = queue.front();
	queue.pop();
	return process;
}
