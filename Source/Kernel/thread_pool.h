#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <chrono>



class Task         // Component or Connection
{
public:
    virtual void Tick() = 0;
    virtual std::string Info() = 0;
    virtual bool Priority() { return false; }
};



class TaskSequence // Holds a list of tasks to be run sequentially within a single thread
{
public:
    TaskSequence(std::vector<Task *> & tasks): 
        tasks_(tasks),
        running(false), 
        completed(false)
    {}

    virtual ~TaskSequence() = default;

    void execute() {
        running = true;
        Tick();
        completed = true;
        running = false;
    }

    bool isRunning() const {
        return running;
    }

    bool isCompleted() const {
        return completed;
    }

protected:
    virtual void Tick()
    {
        for(auto & task : tasks_)
            task->Tick();
    }

private:
    std::atomic<bool> running;
    std::atomic<bool> completed;

    std::vector<Task *> & tasks_;
};



class ThreadPool 
{
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    void submit(TaskSequence * task_sequence); // Submit a task to be executed by the thread pool
    bool working(); /// True if at least one worker is running

private:
    void worker(); // Worker threads function

    std::vector<std::thread> workers; // Vector of worker threads
    std::queue<TaskSequence *> task_sequences;   // TaskSequence queue
    std::mutex queueMutex; // Synchronization
    std::condition_variable condition;
    std::atomic<bool> stop;
};



