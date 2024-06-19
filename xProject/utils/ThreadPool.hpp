#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <future>
#include <utility>

#include "collections/QeueuLockfree.hpp"
#include "Utils.hpp"

namespace Pool {

	class ThreadTaskBase 
	{
	public:
		virtual ~ThreadTaskBase() {}
		virtual void Execute() = 0;
	};

	/// ThreadTask
	/// ------------------------------------------------------------
	template<typename Result>
	class ThreadTask : public ThreadTaskBase 
	{
	private:
		std::packaged_task<Result()> task;
	public:
		template<typename Func, typename... Args>
		ThreadTask(Func&& _func, Args&&... _args) :
			task(
				[f = std::forward<Func>(_func),
				 args = std::tuple(std::forward<Args>(_args)...)]()
				{
					return std::apply(f, args);
				}
			)
		{}
		
		std::future<Result> GetFuture()
		{
			return task.get_future();
		}

		void Execute() { task(); }
		
		ThreadTask(ThreadTask&&) = default;
		ThreadTask& operator=(ThreadTask&&) = default;
		ThreadTask(const ThreadTask&) = delete;
		ThreadTask& operator=(const ThreadTask&) = delete;
	};

	template<typename Func, typename... Args>
	auto MakeTask(Func&& _func, Args&&... _args)
	{
		using Result = std::invoke_result_t<Func, Args...>;
		return std::make_unique<ThreadTask<Result>>(std::forward<Func>(_func), 
												    std::forward<Args>(_args)...);
	}

	/// ThreadPool
	/// ------------------------------------------------------------
	class ThreadPool 
	{
	private:
		using TaskPtrBase = std::unique_ptr<ThreadTaskBase>;

		std::atomic_bool stopThreads = false;
		std::vector<std::thread> threads;
		Utils::QueueLF<TaskPtrBase> tasks;

	public:
		explicit ThreadPool(std::uint8_t _numThread)
		{
			threads.reserve(_numThread);
			for (std::uint8_t i = 0; i < _numThread; i++)
			{
				threads.emplace_back(&ThreadPool::WaitForWork, this);
			}
		}

		~ThreadPool()
		{
			tasks.stop_wait();
			Destroy();
		}

		template<typename Func, typename... Args>
		auto Submit(Func&& _func, Args&&... _args)
		{
			auto task = Pool::MakeTask(std::forward<Func>(_func), std::forward<Args>(_args)...);
			auto taskFuture = task->GetFuture();
			
			tasks.push_back(std::move(task));
			
			return taskFuture;
		}

	private:
		void WaitForWork()
		{
			while (!stopThreads.load(std::memory_order_acquire))
			{
				tasks.wait();

				TaskPtrBase task = std::move(tasks.pop_front());
				if (task.get() != nullptr)
				{
					task->Execute();
				}

			}
		}

		void Destroy()
		{
			stopThreads.store(true, std::memory_order_release);
			for (auto& thread : threads)
			{
				if (thread.joinable())
				{
					thread.join();
				}
			}
		}
	};

}