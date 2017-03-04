#include "memcpy_adv.h"
#include "os.hpp"

#include <array>
#include <vector>
#include <queue>
#include <iostream>

size_t WORK_BLOCK_SIZE = 1024 * 1024;

struct memcpy_task {
	void* from = nullptr;
	void* to = nullptr;
	size_t size = 0;
	os::Semaphore* semaphore = nullptr;
	size_t index = 0;
};

struct memcpy_env {
	os::Semaphore memcpy_semaphore;
	std::vector<std::thread> memcpy_threads;
	std::mutex memcpy_queue_mutex;
	std::queue<memcpy_task> memcpy_queue;
	bool memcpy_threads_exit = false;
};
// ToDo: Make thread-local
memcpy_env* memcpy_active_env = nullptr;

#define min(v,low) ((v < low) ? v : low)
#define max(v,high) ((v > high) ? v : high)
static void memcpy_thread_main(memcpy_env* env) {
	//std::cout << "Thread initialized" << std::endl;
	os::Semaphore* semaphore;
	void
		*from,
		*to;
	size_t size;
	do {
		//std::cout << "Thread waiting" << std::endl;
		env->memcpy_semaphore.wait();
		//std::cout << "Thread woken" << std::endl;
		{
			std::unique_lock<std::mutex> lock(env->memcpy_queue_mutex);
			if (env->memcpy_queue.empty())
				continue;

			// Dequeue
			memcpy_task& task = env->memcpy_queue.front();
			from = task.from;
			to = task.to;
			size = min(task.size, WORK_BLOCK_SIZE);
			semaphore = task.semaphore;
			//std::cout << task.index << ": " << reinterpret_cast<intptr_t>(from) << ", " << reinterpret_cast<intptr_t>(to) << ", " << size << std::endl;

			// Update
			if (task.size > WORK_BLOCK_SIZE) {
				task.from = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(task.from)+WORK_BLOCK_SIZE);
				task.to = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(task.to)+WORK_BLOCK_SIZE);
				task.size -= WORK_BLOCK_SIZE;
				task.index++;
				//env->memcpy_semaphore.notify(); // We're doing this when queueing, for speed reasons.
			} else {
				env->memcpy_queue.pop();
			}
		}
		std::memcpy(to, from, size);
		semaphore->notify();
	} while (!env->memcpy_threads_exit);
}

void* memcpy_thread_initialize(size_t threads) {
	memcpy_env *env = new memcpy_env();
	env->memcpy_threads.resize(threads);
	for (std::thread& thread : env->memcpy_threads) {
		thread = std::thread(memcpy_thread_main, env);
	}
	return env;
}

void memcpy_thread_env(void* env) {
	memcpy_env* renv = (memcpy_env*)env;
	memcpy_active_env = renv;
}

void* memcpy_thread(void* to, void* from, size_t size) {
	size_t blocks_complete = size / WORK_BLOCK_SIZE;
	size_t blocks_incomplete = size % WORK_BLOCK_SIZE;
	size_t blocks = blocks_complete + (blocks_incomplete > 0 ? 1 : 0);
	
	os::Semaphore semaphore;

	memcpy_task task;
	task.from = from;
	task.to = to;
	task.size = size;
	task.semaphore = &semaphore;

	{
		std::unique_lock<std::mutex> lock(memcpy_active_env->memcpy_queue_mutex);
		memcpy_active_env->memcpy_queue.push(task);
		memcpy_active_env->memcpy_semaphore.notify(blocks);
	}

	semaphore.wait(blocks);
	
	return to;
}

void memcpy_thread_finalize(void* env) {
	if (env == nullptr)
		return;

	memcpy_env* renv = (memcpy_env*)env;
	renv->memcpy_threads_exit = true;
	renv->memcpy_semaphore.notify(INT_MAX);
	for (std::thread& thread : renv->memcpy_threads) {
		thread.join();
	}
	renv->memcpy_threads.clear();
	
	// ToDo: Release any waiting memcpy_thread calls.
	delete renv;
}


