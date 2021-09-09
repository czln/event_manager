//
// Created by zelin on 2021/8/6.
//

#ifndef EVENT_MANAGER_EDA_H
#define EVENT_MANAGER_EDA_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <semaphore.h>

template <typename T>
struct type_identity {using type = T;};
template <typename T>
using type_identity_t = typename type_identity<T>::type;

/// millisocond timer
class timer {
    enum type_t {
        every,
        once
    };
private:
    sem_t &trigger;
    std::chrono::milliseconds interv;
    std::atomic_bool quit;
    type_t type;
    std::thread running;
public:
    timer(sem_t &trig, size_t time_ms, type_t t = every);
    ~timer() { terminate(); }
    void run();
    void terminate() { quit = true; if (running.joinable()) running.join(); }

private:
    void run_once();
    void run_every();
};
timer::timer(sem_t &trig, size_t time_ms, type_t t) :
    trigger(trig),
    interv(time_ms),
    quit(false),
    type(t) {}

void timer::run() {
    if (type == once) {
        run_once();
    } else if (type == every) 
        run_every();
}

/// \internal
void timer::run_once() {
    running = std::thread([this]() {
        std::this_thread::sleep_for(interv);
        sem_post(&trigger);
    });
}
void timer::run_every() {
    running = std::thread([this]() {
        while (!quit.load()) {
            std::this_thread::sleep_for(interv);
            sem_post(&trigger);
        }
    });
}

class handle_base {
private:
    std::launch strategy = std::launch::deferred;
public:
    virtual ~handle_base() = default;
    virtual void run() = 0;


};

using handle_ptr_t = std::shared_ptr<handle_base>;

// helper functions to expand std::tuple
template<typename Function, typename Tuple, size_t ... I>
auto call(Function f, Tuple t, std::index_sequence<I ...>) {
    return f(std::get<I>(t) ...);
}
template<typename Function, typename Tuple>
auto call(Function f, Tuple t) {
    static constexpr auto size = std::tuple_size<Tuple>::value;
    return call(f, t, std::make_index_sequence<size>{});
}

template <typename ...T>
class handle : public handle_base { };

template <typename Ret, typename ...Args>
class handle<Ret (Args...)> : public handle_base {
private:
    std::function<Ret(Args...)> function_;
    std::tuple<Args...> args_;
    char* flag = new char();
public:


    handle(std::function<Ret(Args...)> function, Args... args) :
    function_(function),
    args_(std::make_tuple(args...)) { }

    /// not thread safe, may need to add lock job while using it,
    /// depending on the args and function
    void run() override {
        call(function_, args_);
    }
    /// function with return
    // Ret run() {
    //     return call(function_, args_);
    // }

    void set(Args... args) {
        args_ = std::make_tuple(args...);
    }

    std::function<Ret(Args...)> get_func() {
        return function_;
    }
};

// template <typename ...Args>
// class timer_handle : public handle_base {};

// template <typename Ret, typename ...Args>
// class timer_handle<Ret(Args...)> {
// private:
//     std::function<Ret(Args...)> function_;
//     std::tuple<Args...> args_;
//     char* flag = new char();
// public:


//     handle(timer::type_t type, std::function<Ret(Args...)> function, Args... args) :
//         args_(std::make_tuple(args...)) {
//         if (type == timer::type_t::once) {
//             function_ = std::bind([function, args...](){

//             });
//         }
//     }

//     /// not thread safe, may need to add lock job while using it,
//     /// depending on the args and function
//     void run() override {
//         call(function_, args_);
//     }
//     void set(Args... args) {
//         args_ = std::make_tuple(args...);
//     }

//     std::function<Ret(Args...)> get_func() {
//         return function_;
//     }

// };

static const int THREAD_NUM = 6;
static const int TIMEOUT_MILLISECOND = 1000;

class event_pool {
private:
    std::atomic<bool> quit;
    std::unordered_map<std::string, handle_ptr_t> events;

    std::vector<std::future<void>> threads;
    sem_t   thread_ok[THREAD_NUM];
    std::vector<std::queue<handle_ptr_t>> active_event_queue;
    std::mutex  lk_events;

public:
    event_pool();
    ~event_pool() { 
        terminate();
        for (int i=0; i<THREAD_NUM; ++i) {
            sem_destroy(thread_ok + i);
        }
    }

    /// \WARNING: 
    /// this won't end until all tasks finished, so don't add any unfinishable 
    /// task
    /// there is no way to rerun the event pool after calling terminate().
    /// if you do need to do that, you have to create a new instance of event_pool
    /// and start over
    void terminate() {
        quit = true;
        std::chrono::milliseconds timeout(TIMEOUT_MILLISECOND);
        for (int i=0; i<THREAD_NUM; ++i) {
            sem_post(thread_ok + i);    // continue all the blocked threads

            if (std::future_status::timeout == threads[i].wait_for(timeout)) {
                printf("timeout %d\n", i);
            }
        }
    }
    /// this function won't block the thread, so you have to make sure the resources
    /// won't be destroyed yourself;
    void try_terminate() {quit = true;}

    //  this function is unable to get the function signature during compile time
    void register_event(const std::string &id_, const handle_ptr_t &event);

    template <typename Func, typename ...Args>
    void register_event(const std::string &id_,
                        Func function_,
                        Args... args);

    void unregister_event(const std::string &id_);


    /// \NOTE: you're not allowed to trigger events afted the pool terminated
    void trigger_event(const std::string &id_);

    template <typename ...Args>
    void trigger_event(const std::string &id_, Args... args);

    /// fixme: this change the args of event directly, which means
    ///        if the event triggered but weren't done by then,
    ///         the result would be affected by this
    /// \brief set the args of the event and save, then trigger the event
    template <typename ...Args>
    void trigger_and_set(const std::string &id_, Args... args);

private:
/// \internal add a task to the active event queue (the one with the least tasks)
    void add_task(handle_ptr_t tmp_handle);

/// \internal poll all the active events, and add them to the correct task flow
    void poll_events();
};

inline event_pool::event_pool() :
    quit(false),
    active_event_queue(THREAD_NUM) {
    // sem_init(&at_least_one_active, 0, 0);
    for (int i=0; i<THREAD_NUM; ++i) {
        sem_init(thread_ok+i, 0, 0);
    }

    // start to poll events in thread pool
    poll_events();
}

inline void event_pool::register_event(const std::string &id_, 
                                       const handle_ptr_t &event) {
    auto e = events.find(id_);
    if (e == events.end()) {
        events.emplace(id_, event);
        return;
    }
    throw std::runtime_error("fail to register: id already occupied");
}

/// !\warning
/// if this \tparam Args different from the args type in \tparam Func,
/// and it can implicit convert to the args type in \tparam Func, it
/// would pass the compile exam, but the type of handle registered is
/// \tparam Args instead of the args type in \tparam Func, which may cause
/// a bad_cast, if you still give the args type when try triggerring this:
/// \code
/// bad example:
/// event_pool ep;
/// ep.register("bad", [](std::string str) {}, ""); // note the actual handle type 
///                                                 // registered is 
///                                                 // handle<void (const char*)>
/// ep.trigger("bad", std::string(""));             // this would raise a bad_cast
/// \endcode
template <typename Func, typename ...Args>
inline void event_pool::register_event(const std::string &id_,
                                       Func function_,
                                       Args... args_) {
    using result_type = std::result_of_t<Func(Args...)>;

    auto e = events.find(id_);
    if (e == events.end()) {
        std::shared_ptr<handle_base> hp(new handle<result_type (Args...)>(function_,
                                                                   args_...));
        events.template emplace(id_, hp);
        return;
    }
    throw std::runtime_error("fail to register: id already occupied");

}

inline void event_pool::unregister_event(const std::string &id_) {
    auto event = events.find(id_);
    if (event != events.end()) {
        std::lock_guard<std::mutex> lg(lk_events);
        events.erase(event);
    }
}

// this just call the function in pool
inline void event_pool::trigger_event(const std::string &id_) {
    if (quit.load()) 
        throw std::runtime_error("the event pool has been terminated");
    auto event = events.find(id_);
    if (event != events.end()) {
        add_task(event->second);

        return;
    }
    std::string err("fail to trigger: no registered events matched");
    err = err + ": " + id_;
    throw std::runtime_error(err.data());
}

//  this creates a temporary event and adds it into the active pool.
//  but it's not hold by the event pool. which means after execution,
//  it will expire
template <typename ...Args>
inline void event_pool::trigger_event(const std::string &id_, Args... args) {
    if (quit.load()) 
        throw std::runtime_error("the event pool has been terminated");
    auto event = events.find(id_);
    if (event != events.end()) {
        /// \NOTE:
        /// this would throw a bad_cast if the (Args...) in this function does
        /// not match with the (Args...) in the register_event() related 
        auto tmp_func = dynamic_cast<handle<void (Args...)>&>(*(event->second))
                            .get_func();

        add_task(std::make_shared<handle<void(Args...)>>(tmp_func, args...));
        return;

    }
    std::string err("fail to trigger: no registered events matched");
    err = err + ": " + id_;
    throw std::runtime_error(err.data());
}

template <typename ...Args>
inline void event_pool::trigger_and_set(const std::string &id_, Args ...args) {
    if (quit.load()) 
        throw std::runtime_error("the event pool has been terminated");
    auto event = events.find(id_);
    if (event != events.end()) {
        dynamic_cast<handle<void (Args...)>*> (event->second.get())
                                                ->set(args...);
                                                
        add_task(event->second);

        return;
    }
    std::string err("fail to trigger: no registered events matched");
    err = err + ": " + id_;
    throw std::runtime_error(err.data());

}

/// \internal functions below

inline void event_pool::add_task(handle_ptr_t tmp_handle) {
                                                
    int min_tasks = INT32_MAX;  // the minimum tasks in queue of all threads
    int min_i     = -1;
    for (int i=0; i<THREAD_NUM; ++i) {
        if (active_event_queue[i].empty()) {
            active_event_queue[i].emplace(tmp_handle);
            sem_post(thread_ok + i);
            min_i = -1; // clear this to original value
            break;
        } else {
            if (active_event_queue[i].size() < min_tasks) {
                min_tasks = active_event_queue[i].size();
                min_i = i;
            }
        }
    }
    // no empty task queue, add to the queue with the least tasks
    if (min_i != -1) {
        active_event_queue[min_i].emplace(tmp_handle);
        sem_post(thread_ok + min_i);
    }
}

inline void event_pool::poll_events() {
    for (int i=0; i<THREAD_NUM; ++i) {
        threads.emplace_back(std::async([i, this](){
            while (!quit.load()) {
                sem_wait(&thread_ok[i]);
                while (active_event_queue[i].size()) {
                    auto task = active_event_queue[i].front();
                    task->run();
                    active_event_queue[i].pop();
                }
            }
        }));
    }
}

#endif //EVENT_MANAGER_EDA_H
