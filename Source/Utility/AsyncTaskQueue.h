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

    //-------------------------------------------------------------------------
    // Initialize the async task queue and launch its internal thread. The
    // supplied callback will be invoked on the new thread. If the callback
    // returns true, that signals this task queue to shutdown its internal thread.
    using AsyncTaskFunction = std::function<bool(T& task)>;
    void init(AsyncTaskFunction function)
    {
        m_asyncTaskFunction = function;

        m_threadLaunched = true;
        m_stop = false;
        m_threadState = State::kIdle;
        m_thread = std::thread(&AsyncTaskQueue::threadFunc, this);
    }

    //-------------------------------------------------------------------------
    // Tear down the async task queue and its internal thread. Any queued jobs
    // will still execute and this function will stall until they have finished
    // processing.
    void deinit()
    {
        if (m_thread.joinable()) {
            finish(); // Wait for the thread to go idle.

            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_stop = true;
            }
            m_conditionVariable.notify_one();
            m_thread.join();
        }

        m_threadLaunched = false;
    }

    //-------------------------------------------------------------------------
    // Add a new task to the async queue. Tasks are processed in a FIFO ordering.
    void addTask(const T&& task)
    {
        if (m_threadLaunched == false) {
            // Nothing to do.
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(task);
        }

        m_conditionVariable.notify_one();
    }

    //-------------------------------------------------------------------------
    // Stall the calling thread until all tasks on the queue have finished
    // processing.
    void finish()
    {
        if (m_threadLaunched == false) {
            return; // Nothig to do.
        }

        // Wait for the internal thread to be idle before returning.
        bool canFinish = false;
        while (!canFinish) {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                canFinish = m_queue.empty();
            }

            if (canFinish) {
                // Wait for the thread's state to become idle.
                bool isIdle = false;
                do {
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        isIdle = (m_threadState == State::kIdle);
                    }
                    if (isIdle == false) {
                        std::this_thread::yield();
                    }
                } while (isIdle == false);
            }
        }
    }

private:
    std::mutex              m_mutex;
    std::thread             m_thread;
    std::condition_variable m_conditionVariable;
    std::queue<T>           m_queue;
    bool                    m_stop = false;
    AsyncTaskFunction       m_asyncTaskFunction;
    bool				    m_threadLaunched = false;

    //-------------------------------------------------------------------------
    // Possible states for the task thread.
    enum class State {
        kIdle, // The thread is waiting.
        kProcessing // The thread is currently processing a task.
    } m_threadState = State::kIdle;

    //-------------------------------------------------------------------------
    // Internal thread function that performs the actual tasks.
    void threadFunc()
    {
        bool stop = m_stop;
        std::queue<T> tasks;
        while (!stop) {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (m_queue.empty()) {
                    m_threadState = State::kIdle;
                    m_conditionVariable.wait(lock, [this]() {
                        return ((m_stop != false) || (m_queue.empty() == false));
                    });
                }

                tasks.swap(m_queue);
                stop = m_stop;
                m_threadState = State::kProcessing;
            }

            while (!tasks.empty()) {
                auto item = std::move(tasks.front());
                tasks.pop();

                // Invoke the custom task function.
                if (m_asyncTaskFunction(item)) {
                    // Callback has requested that the thread be shutdown.
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_threadState = State::kIdle;
                        m_stop = true;
                    }
                    return;
                }
            }
        }
    }
};

}  // namespace util.

