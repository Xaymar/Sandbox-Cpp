#include "stdafx.h"
#include "memcpy_adv.h"
#include "os.hpp"

#include <array>
#include <vector>
#include <queue>

struct MemCpyTask : public os::ThreadTask {
	void* from;
	void* to;
	size_t size;

	MemCpyTask() { from = nullptr; to = nullptr; size = 0; }

	virtual bool work() override {
		std::memcpy(to, from, size);
		return true;
	}
};

os::ThreadPool* memcpy_threadpool = nullptr;

void* memcpy_thread_initialize(size_t threads) {
	try {
		return reinterpret_cast<void*>(new os::ThreadPool(threads));
	} catch (...) {
		return nullptr;
	}
}

void memcpy_thread_env(void* env) {
	memcpy_threadpool = reinterpret_cast<os::ThreadPool*>(env);
}

size_t WORK_BLOCK_SIZE = 128 * 1024;
void* memcpy_thread(void* to, void* from, size_t size) {
	if (size > WORK_BLOCK_SIZE) {
		size_t blocks = size / WORK_BLOCK_SIZE;
		size_t lastBlockSize = size % WORK_BLOCK_SIZE;
		size_t blocksUsed = blocks + (lastBlockSize > 0 ? 1 : 0);
		
		// Allocate Task Memory
		MemCpyTask **tasks = new MemCpyTask*[blocksUsed];

		// Task building
		for (size_t n = 0; n < blocksUsed; n++) { // Can optimize here.
			size_t offset = n * WORK_BLOCK_SIZE;

			tasks[n] = new MemCpyTask();
			tasks[n]->from = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(from) + offset);
			tasks[n]->to = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(to) + offset);
			tasks[n]->size = WORK_BLOCK_SIZE;
		}
		if (lastBlockSize != 0)
			tasks[blocks]->size = lastBlockSize;

		// Post to ThreadPool
		memcpy_threadpool->PostTasks((os::ThreadTask**)tasks, blocksUsed);

		// Wait for completion
		bool allCompleted = false;
		do {
			allCompleted = true;
			for (size_t n = 0; n < blocksUsed; n++) {
				if (tasks[n]->completed == false)
					allCompleted = false;
			}
		} while (!allCompleted);

		// Clear Memory
		for (size_t n = 0; n < blocksUsed; n++) {
			delete tasks[n];
		}
		delete[] tasks;

		return to;
	} else {
		return std::memcpy(to, from, size);
	}
}

void memcpy_thread_finalize(void* env) {
	if (env) {
		delete env;
		env = nullptr;
	}
}


