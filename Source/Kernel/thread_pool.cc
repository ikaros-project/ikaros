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
    for (std::thread &worker : workers) 
    {
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
            condition.wait(lock, [this] { return stop || !task_sequences.empty(); });
            if (stop && task_sequences.empty()) return;
            task_sequence = std::move(task_sequences.front());
            task_sequences.pop();
        }
        task_sequence->execute();
    }
}

void ThreadPool::submit(TaskSequence* task_sequence) 
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        task_sequences.emplace(task_sequence);
    }
    condition.notify_one();
}

void ThreadPool::status()
{
    std::cout << "Tasks running: " << task_sequences.size() << std::endl;
}

