#include "memcpy_adv.h"
#include "os.hpp"

#include <array>
#include <iostream>
#include <queue>
#include <vector>

#ifdef _WIN32
#include "windows.h"
#endif

#include "intrin.h"

//#define BLOCK_BASED
#define BLOCK_SIZE 256 * 1024

#undef min
#undef max
#define min(v, low) ((v < low) ? v : low)
#define max(v, high) ((v > high) ? v : high)

void* memcpy_movsb(unsigned char* to, unsigned char* from, size_t size)
{
	__movsb(to, from, size);
	return to;
}

struct memcpy_task {
	void*          from       = nullptr;
	void*          to         = nullptr;
	size_t         size       = 0;
	os::Semaphore* semaphore  = nullptr;
	size_t         index      = 0;
	size_t         block_size = 0;
#ifndef BLOCK_BASED
	size_t block_size_rem = 0;
#endif
};

struct memcpy_env {
	void* (*copyfnc)(void*, void*, size_t);
	os::Semaphore            semaphore;
	std::mutex               queue_mutex;
	std::queue<memcpy_task>  task_queue;
	size_t                   block_size   = BLOCK_SIZE;
	bool                     exit_threads = false;
	std::vector<std::thread> threads;
};
// ToDo: Make thread-local
memcpy_env* memcpy_active_env                          = nullptr;
static void* (*memcpy_fnc)(void*, const void*, size_t) = &memcpy;

static void memcpy_thread_main(memcpy_env* env)
{
	os::Semaphore* semaphore;
	void *         from, *to;
	size_t         size;
	do {
		env->semaphore.wait();
		{
			std::unique_lock<std::mutex> lock(env->queue_mutex);
			if (env->task_queue.empty())
				continue;

			// Dequeue
			memcpy_task& task = env->task_queue.front();
			from              = task.from;
			to                = task.to;
#ifdef BLOCK_BASED
			size = (task.size > task.block_size ? task.block_size : task.size);
#else
			size = task.block_size + task.block_size_rem;
#endif
			semaphore = task.semaphore;

			// Update
			if (task.size > size) {
				task.from = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(task.from) + size);
				task.to   = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(task.to) + size);
				task.size -= size;
				task.index++;
			} else {
				env->task_queue.pop();
			}
		}
		memcpy_fnc((unsigned char*)to, (unsigned char*)from, size);
		semaphore->notify();
	} while (!env->exit_threads);
}

void* memcpy_thread_initialize(size_t threads)
{
	memcpy_env* env = new memcpy_env();
	env->threads.resize(threads);
	DWORD mask = 1;
	for (std::thread& threads : env->threads) {
		threads = std::thread(memcpy_thread_main, env);

#ifdef _WIN32
		//SetThreadAffinityMask(threads.native_handle(), mask);
#else
// Linux/MacOSX?
#endif

		mask <<= 1;
	}
	return env;
}

void memcpy_thread_set_memcpy(void* (*memcpy)(void*, const void*, size_t))
{
	memcpy_fnc = memcpy;
}

void memcpy_thread_env(void* env)
{
	memcpy_env* renv  = (memcpy_env*)env;
	memcpy_active_env = renv;
}

void* memcpy_thread(void* to, void* from, size_t size)
{
	os::Semaphore semaphore;

	memcpy_task task;
	task.from      = from;
	task.to        = to;
	task.size      = size;
	task.semaphore = &semaphore;

#ifdef BLOCK_BASED
	size_t blocks_complete   = size / memcpy_active_env->block_size;
	size_t blocks_incomplete = size % memcpy_active_env->block_size;
	size_t blocks            = blocks_complete + (blocks_incomplete > 0 ? 1 : 0);
	task.block_size          = memcpy_active_env->block_size;
#else
	size_t blocks = min(max(size, memcpy_active_env->block_size) / memcpy_active_env->block_size,
	                    memcpy_active_env->threads.size());
	task.block_size = size / blocks;
	task.block_size_rem = size - (task.block_size * blocks);
#endif

	{
		std::unique_lock<std::mutex> lock(memcpy_active_env->queue_mutex);
		memcpy_active_env->task_queue.push(task);
		memcpy_active_env->semaphore.notify(blocks);
	}

	semaphore.wait(blocks);

	return to;
}

void memcpy_thread_finalize(void* env)
{
	if (env == nullptr)
		return;

	memcpy_env* renv   = (memcpy_env*)env;
	renv->exit_threads = true;
	renv->semaphore.notify(INT_MAX);
	{
		std::unique_lock<std::mutex> lock(renv->queue_mutex);
		if (renv->task_queue.size() > 0) {
			for (size_t n = 0; n < renv->task_queue.size(); n++) {
				memcpy_task task = renv->task_queue.front();
				task.semaphore->notify(INT_MAX);
			}
		}
	}
	for (std::thread& thd : renv->threads) {
		thd.join();
	}
	renv->threads.clear();

	// ToDo: Release any waiting memcpy_thread calls.
	delete renv;
}
