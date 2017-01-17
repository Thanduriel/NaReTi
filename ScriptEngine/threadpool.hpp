#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <memory>
#include <queue>

namespace utils{
	template <typename T>
	class LockFreeQueue {
	public:
		void push(const T& value) 
		{
			std::unique_lock<std::mutex> lock(this->mutex);
			this->q.push(value);
		}
		// deletes the retrieved element, do not use for non integral types
		bool pop(T& _ret) 
		{
			std::unique_lock<std::mutex> lock(this->mutex);
			if (this->q.empty())
				return false;
			_ret = std::move(this->q.front());
			this->q.pop();
			return true;
		}

		bool empty() 
		{
			std::unique_lock<std::mutex> lock(this->mutex);
			return this->q.empty();
		}
	private:
		std::queue<T> q;
		std::mutex mutex;
	};

	class ThreadPool
	{
	public:
		class Task : public std::function<void()>
		{
		public:
			template <typename... Args>
			Task(Args&&... _args) :
				std::function<void()>(std::forward<Args>(_args)...),
				isDone(false)
			{}

			std::atomic<bool> isDone;
		};

		typedef std::shared_ptr<Task> TaskHandle;

		ThreadPool(int _numThreads):
			m_numWorking(0)
		{
			m_threads.reserve(_numThreads);
			m_flags.resize(_numThreads);

			for (int i = 0; i < _numThreads; ++i)
			{
				auto runner = [&]()
				{
		//			std::atomic<bool>& flag = *m_flags[i].get();
					//wait for task
					while (true)
					{
						std::shared_ptr<Task> f;
						if (m_queue.pop(f))
						{
					//		flag = true;
							(*f)();
							f->isDone = true;
					//		flag = false;
						}
					}
				};
				m_threads.emplace_back(runner);
			}
		}

		template<typename... _Args>
		std::shared_ptr<Task> push(_Args&&... _args)
		{
			auto ptr = std::make_shared<Task>(std::forward<_Args>(_args)...);
			m_queue.push(ptr);

			return std::move(ptr);
		}
	private:

		std::vector< std::thread > m_threads;
		std::vector < std::unique_ptr<std::atomic<bool>> > m_flags;
		LockFreeQueue< std::shared_ptr<Task> > m_queue;

		int m_numWorking;
		std::mutex m_mutex;
	};
}