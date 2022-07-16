//
// Created by zelin on 2022/4/20.
//

#ifndef EVENT_MANAGER_SEMA_H
#define EVENT_MANAGER_SEMA_H
#include "noncopyable.h"
#ifdef __linux__
#include <semaphore.h>
#elif defined(_WIN32)
#endif
#include <algorithm>

class Semaphore : public noncopyable {
private:
#ifdef __linux__
    sem_t sem_{};
#endif
public:
    explicit Semaphore(const size_t cnt = 0) {
#ifdef __linux__
        sem_init(&sem_, 0, cnt);
#endif
    }
    ~Semaphore() {
#ifdef __linux__
        sem_destroy(&sem_);
#endif
    }

    Semaphore(Semaphore&& other):
#ifdef __linux__
    sem_(std::move(other.sem_)) {
#endif
    }

    Semaphore& operator= (Semaphore&& other) {
        if (this == &other) {
            return *this;
        }
        sem_ = std::move(other.sem_);
        return *this;
    }

    
    int acquire() {
#ifdef __linux__
        return sem_wait(&sem_);
#endif
    }

    int release() {
#ifdef __linux__
        return sem_post(&sem_);
#endif
    }
};

template <typename SEM>
class SemaGuard : public noncopyable {
private:
    SEM& sem_;
public:
    explicit SemaGuard(const SEM& sem):
        sem_(sem) {
        sem_.acquire();
    }
    ~SemaGuard() {
        sem_.release();
    }
};


#endif //EVENT_MANAGER_SEMA_H
