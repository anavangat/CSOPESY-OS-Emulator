#include "ReadyQueue.h"

void ReadyQueue::push(std::shared_ptr<Process> process) {
	{
		std::lock_guard<std::mutex> lock(mutex); //mutex is locked for the duration of this block, ensuring thread safety when modifying the queue // basically it allows one thread to access the queue at a time //lock_guard will lock the whole block and automatically release the lock when the block is exited, even if an exception occurs, preventing deadlocks and ensuring proper synchronization between threads
		queue.push(process); // add the process to the back of the queue
	}

	condition.notify_one(); // notify one waiting thread that a new process is available in the queue, allowing it to wake up and pop the process for execution
}

std::shared_ptr<Process> ReadyQueue::pop() {
	std::unique_lock<std::mutex> lock(mutex); // unique_lock is used here instead of lock_guard because we need to use condition_variable's wait function, which requires a unique_lock to manage the mutex while waiting. // unique_lock allows us to lock and unlock the mutex manually, which is necessary for the condition_variable to work correctly. // it will automatically release the lock when the block is exited, even if an exception occurs, preventing deadlocks and ensuring proper synchronization between threads
	condition.wait(lock, [this]() { return !queue.empty(); }); //thread will sleep and release lock of the queue with mutex until condition.wait() is notified with notify.one(). Thread will open the lock and check the condition (queue is not empty) before proceeding. If the condition is not met, the thread will go to sleep and release the lock, allowing other threads to push processes into the queue. Once a process is pushed and notify_one() is called, one waiting thread will wake up, reacquire the lock, and check the condition again. If the condition is met (queue is not empty), it will proceed to pop a process from the queue for execution.

	auto process = queue.front();
	queue.pop();
	return process;
}
