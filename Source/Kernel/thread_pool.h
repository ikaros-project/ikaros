#pragma once

#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

class Task {
public:
    enum class Kind
    {
        generic,
        component,
        connection,
    };

    explicit Task(Kind kind = Kind::generic): kind_(kind) {}
    virtual ~Task() = default;

    Kind kind() const { return kind_; }
    virtual void Tick() = 0;
    virtual std::string Info() const = 0;
    virtual bool ShouldTick() const { return true; }
    virtual bool Priority() { return false; }
    virtual bool TryProfilingBegin() { return false; }
    virtual void ProfilingBegin() {}
    virtual void ProfilingEnd() {}

private:
    Kind kind_;
};



class TaskSequence 
{
public:
    TaskSequence(const std::vector<Task *> & tasks): tasks_(tasks)
    {
        for(Task * task : tasks_)
            if(task == nullptr)
                throw std::invalid_argument("TaskSequence cannot contain a null Task.");
    }

    virtual ~TaskSequence() = default;

    void execute()
    {
        executeImpl(false);
    }

    bool hasError() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return error_;
    }

    void rethrowIfError() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(error_ && exception_)
            std::rethrow_exception(exception_);
    }

    bool isRunning() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_ == State::running;
    }

    bool isCompleted() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_ == State::completed;
    }

    bool waitForCompletion(double seconds = -1.0)
    {
        if(!std::isfinite(seconds))
            throw std::invalid_argument("TaskSequence timeout must be finite.");

        std::unique_lock<std::mutex> lock(mutex_);
        auto completed = [this]() { return state_ == State::completed; };
        if(seconds < 0.0)
        {
            condition_.wait(lock, completed);
            return true;
        }

        return condition_.wait_for(lock, std::chrono::duration<double>(seconds), completed);
    }

protected:
    virtual void Tick()
    {
        for(auto & task : tasks_)
        {
            if(!task->ShouldTick())
                continue;
            const bool profiling_started = task->TryProfilingBegin();
            try
            {
                task->Tick();
            }
            catch(const std::exception & e)
            {
                if(profiling_started)
                    task->ProfilingEnd();
                throw std::runtime_error("Error in task \"" + task->Info() + "\": " + e.what());
            }
            catch(...)
            {
                if(profiling_started)
                    task->ProfilingEnd();
                throw std::runtime_error("Error in task \"" + task->Info() + "\": Unknown error.");
            }
            if(profiling_started)
                task->ProfilingEnd();
        }
    }

    std::vector<Task *> tasks_;

private:
    friend class ThreadPool;

    enum class State
    {
        idle,
        queued,
        running,
        completed,
    };

    void prepareForSubmission()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(state_ == State::queued || state_ == State::running)
            throw std::logic_error("TaskSequence is already queued or running.");

        state_ = State::queued;
        error_ = false;
        exception_ = nullptr;
    }

    void cancelSubmission()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(state_ != State::queued)
                return;
            state_ = State::completed;
        }
        condition_.notify_all();
    }

    void executeSubmitted()
    {
        executeImpl(true);
    }

    void executeImpl(bool submitted)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(submitted)
            {
                if(state_ != State::queued)
                    throw std::logic_error("TaskSequence was not queued for execution.");
            }
            else if(state_ == State::queued || state_ == State::running)
                throw std::logic_error("TaskSequence is already queued or running.");

            state_ = State::running;
            error_ = false;
            exception_ = nullptr;
        }

        std::exception_ptr eptr;
        try
        {
            Tick();
        }
        catch(...)
        {
            eptr = std::current_exception();
            std::lock_guard<std::mutex> lock(mutex_);
            error_ = true;
            exception_ = eptr;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_ = State::completed;
        }
        condition_.notify_all();
        if(eptr)
            std::rethrow_exception(eptr);
    }
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    State state_ = State::idle;
    bool error_ = false;
    std::exception_ptr exception_ = nullptr;
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
