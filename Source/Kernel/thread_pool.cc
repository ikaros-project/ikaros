#include "thread_pool.h"


ThreadPool::ThreadPool(size_t numThreads): 
    stop(false) 
{
    for (size_t i = 0; i < numThreads; ++i) 
        workers.emplace_back(&ThreadPool::worker, this);
}

ThreadPool::~ThreadPool()
{
    std::cout << "Shutting down ThreadPool..." << std::endl;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all(); // Notify all threads to wake up and exit
    for (std::thread &worker : workers) 
    {
        if (worker.joinable()) 
            worker.join(); // Wait for all threads to finish
    }

    // Ensure all tasks are completed
    if (!task_sequences.empty()) 
    {
        std::cerr << "Warning: Tasks remaining in the queue during shutdown." << std::endl;
    }
}


void ThreadPool::worker() 
{
    while (true) 
    {
        std::shared_ptr<TaskSequence> task_sequence;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return stop || !task_sequences.empty(); });
            if (stop && task_sequences.empty()) return;
            task_sequence = task_sequences.front();
            task_sequences.pop();
        }
        
        if (task_sequence) {
            try {
                task_sequence->execute();
            } catch (const std::exception &e) {
                std::cerr << "Task execution error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error during task execution." << std::endl;
            }
        }
    }
}


void ThreadPool::submit(std::shared_ptr<TaskSequence> task_sequence) 
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        task_sequences.emplace(task_sequence);
    }
    condition.notify_one();
}

bool ThreadPool::working() 
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return !task_sequences.empty();
}
