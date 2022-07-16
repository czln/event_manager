//
// Created by zelin on 2022/4/20.
//

#ifndef EVENT_MANAGER_THREADPOOL_H
#define EVENT_MANAGER_THREADPOOL_H

#include "handle.h"
#include "sema.h"

#include <atomic>
#include <memory>
#include <vector>
#include <thread>
#include <queue>

class ThreadPool {
private:
    std::atomic_bool quit_{false};
    const size_t n_threads_;
    std::vector<Semaphore> sems_;
    std::vector<std::thread> threads_;
    /// \todo max task number for each queue
    std::vector<std::queue<handle_ptr_t>> tasks_;
public:
    explicit ThreadPool(const size_t n_threads = 6) :
        n_threads_(n_threads),
        sems_(n_threads),
        threads_(n_threads),
        tasks_(n_threads){
        poll_events();
    }

    ~ThreadPool() {
        terminate();
        for (auto& thread : threads_) {
            if (thread.joinable())
                thread.join();
        }
        printf("ended\n");
    }

    /// \note if already terminated, it does nothing
    template<typename ...Args>
    void add_task(type_identity_t<std::function<void(Args...)>> func, Args... args) {
        if (quit_) {
            return;
        }
        auto hp = std::make_shared<handle<void(Args...)> >(func, args...);
        add_task(hp);
    }

    /// \note if already terminated, it does nothing
    void add_task(const handle_ptr_t& handle) {
        if (quit_)
            return;
        int min_tasks = INT32_MAX;  // the minimum tasks in queue of all threads
        int min_i     = -1;
        for (int i=0; i<n_threads_; ++i) {
            if (tasks_[i].empty()) {
                tasks_[i].emplace(handle);
                sems_[i].release();
                min_i = -1; // clear this to original value
                break;
            } else {
                if (tasks_[i].size() < min_tasks) {
                    min_tasks = tasks_[i].size();
                    min_i = i;
                }
            }
        }

        // no empty task queue, add to the queue with the least tasks
        if (min_i != -1) {
            tasks_[min_i].emplace(handle);
            sems_[min_i].release();
        }
    }

    void terminate() {
        quit_ = true;
        for (auto& sem : sems_) {
            sem.release();   // continue all the blocked threads
        }
    }
private:
    void poll_events() {
        for (size_t i=0; i< n_threads_; ++i) {
            threads_.emplace_back([i, this]() {
                while (!quit_) {
                    sems_[i].acquire();
                    while (!tasks_[i].empty()) {
                        auto task = tasks_[i].front();
                        task->run();
                        tasks_[i].pop();
                    }
                }
            });
        }
    }
};

class TaskFlow {
private:
	std::atomic_bool quit_{false};
	Semaphore sem_;
	std::thread flow_;
	std::queue<handle_ptr_t> tasks_;
public:
	TaskFlow():
		sem_(1),
        flow_(){
	}
public:
};
#endif //EVENT_MANAGER_THREADPOOL_H
