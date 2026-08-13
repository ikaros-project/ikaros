#include "thread_pool.h"


ThreadPool::ThreadPool(size_t numThreads): 
    stop(false) 
{
    if(numThreads == 0)
        throw std::invalid_argument("ThreadPool requires at least one worker.");

    try
    {
        workers.reserve(numThreads);
        for(size_t i = 0; i < numThreads; ++i)
            workers.emplace_back(&ThreadPool::worker, this);
    }
    catch(...)
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread & worker : workers)
            if(worker.joinable())
                worker.join();
        throw;
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread & worker : workers)
        if(worker.joinable())
            worker.join();
}


void
ThreadPool::worker()
{
    while(true)
    {
        std::shared_ptr<TaskSequence> task_sequence;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return stop || !task_sequences.empty(); });
            if(stop && task_sequences.empty())
                return;
            task_sequence = task_sequences.front();
            task_sequences.pop();
            ++active_tasks;
        }

        try
        {
            task_sequence->executeSubmitted();
        }
        catch(...)
        {
            // TaskSequence retains the exception for its submitter.
        }
        --active_tasks;
    }
}


void
ThreadPool::submit(std::shared_ptr<TaskSequence> task_sequence)
{
    if(task_sequence == nullptr)
        throw std::invalid_argument("ThreadPool cannot submit a null TaskSequence.");

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if(stop)
            throw std::runtime_error("ThreadPool is stopping and cannot accept work.");

        task_sequence->prepareForSubmission();
        try
        {
            task_sequences.emplace(task_sequence);
        }
        catch(...)
        {
            task_sequence->cancelSubmission();
            throw;
        }
    }
    condition.notify_one();
}

bool
ThreadPool::working()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return !task_sequences.empty() || active_tasks.load() > 0;
}
