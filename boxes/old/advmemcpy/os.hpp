#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>

#include <vector>
#include <queue>

namespace os {
	class Semaphore {
		public:
		Semaphore(size_t count = 0);
		~Semaphore();

		void notify(size_t count = 1);
		void wait(size_t count = 1);
		bool try_wait(size_t count = 1);

		private:
		size_t m_Count;
		std::mutex m_Mutex;
		std::condition_variable m_CondVar;
	};

	struct ThreadTask {
		bool completed;
		bool failed;

		ThreadTask() { completed = false; failed = false; }
		virtual ~ThreadTask() {};

		virtual bool work();
	};

	struct ThreadPoolWorker {
		ThreadPoolWorker();
		ThreadPoolWorker(const ThreadPoolWorker &) = delete;

		std::thread thread;

		bool isRunning;
		bool doStop;
		ThreadTask* currentTask;
	};

	class ThreadPool {
		public:
		ThreadPool(size_t threads);
		~ThreadPool();

		void PostTask(ThreadTask *task);
		void PostTasks(ThreadTask *task[], size_t count);
		ThreadTask* WaitTask();

		static int ThreadMain(ThreadPoolWorker* worker, ThreadPool* pool);

		private:
		std::vector<ThreadPoolWorker*> m_Threads;
		struct {
			Semaphore semaphore;
			std::queue<ThreadTask*> queue;
			std::mutex mutex;
		} m_Tasks;
	};
}