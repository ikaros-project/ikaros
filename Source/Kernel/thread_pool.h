#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <chrono>

class Task {
public:
    virtual void Tick() = 0;
    virtual std::string Info() = 0;
    virtual bool Priority() { return false; }
};

class TaskSequence 
{
public:
    TaskSequence(std::vector<Task *> &tasks)
        : tasks_(tasks), running(false), completed(false) {}

    virtual ~TaskSequence()  = default;

    void execute() 
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running = true;
        }
        Tick();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            completed = true;
        }
        condition_.notify_all();
    }

    bool isRunning() const 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return running;
    }

    bool isCompleted() const 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return completed;
    }

    void waitForCompletion() 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() { return completed; });
    }

protected:
    virtual void Tick() 
    {
        for (auto &task : tasks_)
            task->Tick();
    }

private:
    std::vector<Task *> &tasks_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool running;
    bool completed;
};

class ThreadPool 
{
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    void submit(std::shared_ptr<TaskSequence> task_sequence);
    bool working();

private:
    void worker();

    std::vector<std::thread> workers;
    std::queue<std::shared_ptr<TaskSequence>> task_sequences;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};