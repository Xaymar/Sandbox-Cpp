#include "stdafx.h"
#include "os.hpp"
#include <iostream>

#define THREAD_START_TIME 10
#define THREAD_STOP_TIME 100

os::Semaphore::Semaphore(size_t count) {
	m_Count = count;
}

os::Semaphore::~Semaphore() {

}

void os::Semaphore::notify(size_t count) {
	std::unique_lock<std::mutex> lock(m_Mutex);
	m_Count += count;
	if (count == 1)
		m_CondVar.notify_one();
	else
		m_CondVar.notify_all();
}

void os::Semaphore::wait() {
	std::unique_lock<std::mutex> lock(m_Mutex);
	do {
		m_CondVar.wait(lock, [this] {
			return (m_Count > 0);
		});
		if (m_Count > 0) { // Another thread may have been faster.
			m_Count--;
			return; // Jumps straight back to the old code
		}
	} while (true); // This should produce a cond-less jmp or reloc operation
}

bool os::Semaphore::try_wait() {
	std::unique_lock<std::mutex> lock(m_Mutex);
	if (m_Count > 0) {
		m_Count--;
		return true;
	}
	return false;
}

bool os::ThreadTask::work() {
	return true;
}

os::ThreadPool::ThreadPool(size_t threads) {
	m_Threads.resize(threads);
	for (size_t n = 0; n < threads; n++) {
		ThreadPoolWorker* worker = new ThreadPoolWorker();
		worker->isRunning = false;
		worker->doStop = false;
		worker->currentTask = nullptr;
		worker->thread = std::thread(ThreadMain, worker, this);
		// Timed Wait
		for (size_t w = 0; w <= THREAD_START_TIME; w++) {
			if (worker->isRunning)
				break;

			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
		/// We failed to start the thread, do error stuff.
		if (!worker->isRunning)
			throw std::runtime_error("Failed to start ThreadPool worker thread.");

		m_Threads[n] = worker;
	}
}

os::ThreadPool::~ThreadPool() {
	for (ThreadPoolWorker* worker : m_Threads) {
		worker->doStop = true;
		PostTask(nullptr);
	}

	for (ThreadPoolWorker* worker : m_Threads) {
		if (worker->isRunning) {
			// Timed Wait for thread stop.
			for (size_t w = 0; w <= THREAD_STOP_TIME; w++) {
				if (!worker->isRunning)
					break;

				std::this_thread::sleep_for(std::chrono::microseconds(1));
			}
		}
		delete worker;
	}
}

void os::ThreadPool::PostTask(ThreadTask *task) {
	std::unique_lock<std::mutex> lock(m_Tasks.mutex);
	m_Tasks.queue.push(task);
	m_Tasks.semaphore.notify();
}

void os::ThreadPool::PostTasks(ThreadTask *task[], size_t count) {
	std::unique_lock<std::mutex> lock(m_Tasks.mutex);
	for (size_t n = 0; n < count; n++)
		m_Tasks.queue.push(task[n]);
	m_Tasks.semaphore.notify(count);
}

os::ThreadTask* os::ThreadPool::WaitTask() {
	m_Tasks.semaphore.wait();
	std::unique_lock<std::mutex> lock(m_Tasks.mutex);
	os::ThreadTask* task = m_Tasks.queue.front();
	m_Tasks.queue.pop();
	return task;
}

int os::ThreadPool::ThreadMain(ThreadPoolWorker* worker, ThreadPool* pool) {
	worker->isRunning = true;
	while (!worker->doStop) {
		if (worker->currentTask != nullptr) {
			worker->currentTask->failed = !worker->currentTask->work();
			worker->currentTask->completed = true;
			worker->currentTask = nullptr;
		}
		worker->currentTask = pool->WaitTask();
	}
	worker->isRunning = false;
	return 0;
}

os::ThreadPoolWorker::ThreadPoolWorker() {
	isRunning = false;
	doStop = false;
	currentTask = nullptr;
}
