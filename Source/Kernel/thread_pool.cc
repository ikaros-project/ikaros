#include "thread_pool.h"




ThreadPool::ThreadPool(size_t numThreads) : 
    stop(false) 
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() 
{
    stop = true;
    condition.notify_all();
    for (std::thread &worker : workers) {
        worker.join();
    }
}

void ThreadPool::worker() 
{
    while (!stop) 
    {
        TaskSequence * task_sequence;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) return;
            task = std::move(tasks.front());
            tasks.pop();
        }
        task->execute();
    }
}

void ThreadPool::submit(TaskSequence* task_sequence) 
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.emplace(task);
    }
    condition.notify_one();
}

void ThreadPool::status()
{
    std::cout << "Tasks running: " << tasks.size() << std::endl;
}

