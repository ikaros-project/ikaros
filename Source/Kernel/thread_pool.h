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

#include "profiler.h"

class Task {
public:
    virtual void Tick() = 0;
    virtual std::string Info() = 0;
    virtual bool Priority() { return false; }
    virtual void ProfilingBegin() {};
    virtual void ProfilingEnd()  {};

    Profiler profiler_;
};



class TaskSequence 
{
public:
    TaskSequence(const std::vector<Task *> &tasks)
        : tasks_(tasks), running(false), completed(false), error_(false) {}

    virtual ~TaskSequence() = default;

    void execute() 
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running = true;
            error_ = false;
        }
        try {
            Tick();
        }
        catch(...) {
            std::lock_guard<std::mutex> lock(mutex_);
            error_ = true;
            throw;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            completed = true;
        }
        condition_.notify_all();
    }

    bool hasError() const 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return error_;
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

    bool waitForCompletion(double seconds = -1.0) 
    {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (seconds < 0.0) {
            // Infinite wait
            condition_.wait(lock, [this]() { return completed; });
            return true;
        } else {
            // Wait with timeout
            auto timeout = std::chrono::milliseconds(
                static_cast<long>(seconds * 1000)
            );
            return condition_.wait_for(lock, timeout, [this]() { return completed; });
        }
    }

protected:
    virtual void Tick() 
    {
        for (auto &task : tasks_)
        {
            if(task == nullptr)
                continue; // FIXME: Should not happen - task list may not have been correctky deleted ******
            task->ProfilingBegin();
            task->Tick();
            task->ProfilingEnd();
        }
    }

private:
    std::vector<Task *> tasks_;  // Changed from reference to value
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool running;
    bool completed;
    bool error_;
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
    std::atomic<size_t> active_tasks{0};
};