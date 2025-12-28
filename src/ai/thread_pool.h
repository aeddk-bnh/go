#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t n) { start(n); }
    ~ThreadPool() { stop(); }

    void enqueue(std::function<void()> job) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            jobs_.push(std::move(job));
        }
        cv_.notify_one();
    }

    void start(size_t n) {
        running_ = true;
        for (size_t i = 0; i < n; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> job;
                    {
                        std::unique_lock<std::mutex> lock(this->mutex_);
                        this->cv_.wait(lock, [this] { return !this->jobs_.empty() || !running_; });
                        if (!running_ && this->jobs_.empty())
                            return;
                        job = std::move(this->jobs_.front());
                        this->jobs_.pop();
                    }
                    job();
                }
            });
        }
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            running_ = false;
        }
        cv_.notify_all();
        for (auto &t : workers_)
            if (t.joinable()) t.join();
        workers_.clear();
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> jobs_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_ = false;
};
