//
//  AsyncTaskQueue.h
//  Heatray
//
//  Handles processing queues of tasks on a separate thread.
//
//

#pragma once

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace util {
 
template<class T>
class AsyncTaskQueue
{
public:
	AsyncTaskQueue() {}
	~AsyncTaskQueue() { deinit(); }

	using AsyncTaskFunction = std::function<bool(T& task)>;

	void init(AsyncTaskFunction  function)
	{
		m_async_task_function = function;

		m_thread_launched = true;
		m_stop = false;
		m_thread_state = State::kIdle;
		m_thread = std::thread(&AsyncTaskQueue::threadFunc, this);
	}

	void deinit()
	{
		if (m_thread.joinable()) {
			finish(); // Wait for the thread to go idle.

			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_stop = true;
			}
			m_condition_variable.notify_one();
			m_thread.join();
		}

		m_thread_launched = false;
	}

	void addTask(const T&& task)
	{
		if (m_thread_launched == false) {
			// Nothing to do.
			return;
		}

		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_queue.push(task);
		}

		m_condition_variable.notify_one();
	}

	void finish()
	{
		if (m_thread_launched == false) {
			return; // Nothig to do.
		}

		// Wait for the internal thread to be idle before returning.
		bool can_finish = false;
		while (!can_finish) {
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				can_finish = m_queue.empty();
			}

			if (can_finish) {
				// Wait for the thread's state to become idle.
				bool is_idle = false;
				do {
					{
						std::unique_lock<std::mutex> lock(m_mutex);
						is_idle = (m_thread_state == State::kIdle);
					}
					if (is_idle == false) {
						std::this_thread::yield();
					}
				} while (is_idle == false);
			}
		}
	}

private:
	std::mutex              m_mutex;
	std::thread             m_thread;
	std::condition_variable m_condition_variable;
	std::queue<T>           m_queue;
	bool                    m_stop;
	AsyncTaskFunction       m_async_task_function;
	bool				    m_thread_launched = false;

	enum class State {
		kIdle,
		kProcessing
	} m_thread_state;

	void threadFunc()
	{
		bool stop = m_stop;
		std::queue<T> tasks;
		while (!stop) {
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				if (m_queue.empty()) {
					m_thread_state = State::kIdle;
					m_condition_variable.wait(lock, [this]() {
						return ((m_stop != false) || (m_queue.empty() == false));
					});
				}

				tasks.swap(m_queue);
				stop = m_stop;
				m_thread_state = State::kProcessing;
			}

			while (!tasks.empty()) {
				auto item = std::move(tasks.front());
				tasks.pop();

				// Invoke the custom task function.
				if (m_async_task_function(item)) {
					// Callback has requested that the thread be shutdown.
					{
						std::unique_lock<std::mutex> lock(m_mutex);
						m_thread_state = State::kIdle;
						m_stop = true;
					}
					return;
				}
			}
		}
	}
};

}  // namespace util.

