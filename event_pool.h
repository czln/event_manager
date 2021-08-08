//
// Created by zelin on 2021/8/6.
//

#ifndef EVENT_MANAGER_EDA_H
#define EVENT_MANAGER_EDA_H

#include <functional>
#include <atomic>
#include <exception>
#include <unordered_map>
#include <queue>
#include <tuple>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <condition_variable>

#include <semaphore.h>

template <typename T>
struct type_identity {using type = T;};
template <typename T>
using type_identity_t = typename type_identity<T>::type;

class handle_base {
public:
    virtual ~handle_base() = default;
    virtual void run() = 0;


};

using handle_ptr_t = std::shared_ptr<handle_base>;

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

template <typename ...Args>
class handle<void (Args...)> : public handle_base {
private:
    std::function<void(Args...)> function_;
    std::tuple<Args...> args_;
public:

    handle(std::function<void(Args...)> function, Args... args) :
    function_(function),
    args_(std::make_tuple(args...)) { }

    /// not thread safe, may need to add lock job while using it,
    /// depending on the args and function
    void run() override {
        call(function_, args_);
    }

    void set(Args... args) {
        args_ = std::make_tuple(args...);
    }

    std::function<void(Args...)> get_func() {
        return function_;
    }
};

class event_pool {
private:
    std::atomic<bool> quit;
    /// TODO: semaphore have better performance, but its value
    ///       would be unnecessarily large during execution
    ///       ( each trigger add one, but while minus one, it may
    ///       deal with multiple events, so there would be some
    ///       "save up" ). but as each active event emplaced first,
    ///       it works fine by now. if any bug, change to condition
    ///       variable
    sem_t at_least_one_active;
//    std::mutex lock_helper;
//    std::condition_variable at_least_one_active;
    std::queue<handle_ptr_t> active_events;
    std::unordered_map<std::string, handle_ptr_t> events;

public:
    event_pool() :
    quit(false) { sem_init(&at_least_one_active, 0, 0); }
    ~event_pool() { sem_destroy(&at_least_one_active); }

    /// \WARNING: 
    /// 1. DO NOT RUN THIS IN PARALLEL, if using
    /// condition variable it would cause DEAD LOCK, if
    /// semaphore would cause RACE CONDITION
    /// 2. after calling terminate, this won't end until all
    /// active_events handled, so don't dealloc the thread resources
    /// until this function actually ends
    void run();
    void terminate() {quit = true;}

    void register_event(const std::string &id_, const handle_ptr_t &event);

    template <typename ...Args>
    void register_event(const std::string &id_,
                        type_identity_t<std::function<void(Args...)>> function_,
                        Args... args);

    void unregister_event(const std::string &id_);

    void trigger_event(const std::string &id_);

    template <typename ...Args>
    void trigger_event(const std::string &id_, Args... args);

    /// fixme: this change the args of event directly, which means
    ///        if the event triggered but weren't done by then,
    ///         the result would be affected by this
    /// \brief set the args of the event and save, then trigger the event
    template <typename ...Args>
    void trigger_and_set(const std::string &id_, Args... args);
};

void event_pool::run() {
//    std::unique_lock<std::mutex> lk(lock_helper);
    while (!quit.load()) {
        sem_wait(&at_least_one_active);
//        at_least_one_active.wait(lk);
        while (active_events.size()) {
            auto event = active_events.front();
            /// TODO: thread pool
            /// if running, and unregister_event called, UB!
            std::thread(&handle_base::run, event).detach();
            active_events.pop();
        }
    }
}

void event_pool::register_event(const std::string &id_, const handle_ptr_t &event) {
    auto e = events.find(id_);
    if (e == events.end()) {
        events.emplace(id_, event);
        return;
    }
    throw std::runtime_error("fail to register: id already occupied");
}

template <typename ...Args>
void event_pool::register_event(const std::string &id_,
                                type_identity_t<std::function<void(Args...)>> function_,
                                Args... args_) {
    auto e = events.find(id_);
    if (e == events.end()) {
        std::shared_ptr<handle_base> hp(new handle<void (Args...)>(function_,
                                                                   args_...));
        events.template emplace(id_, hp);
        return;
    }
    throw std::runtime_error("fail to register: id already occupied");

}

void event_pool::unregister_event(const std::string &id_) {
    auto event = events.find(id_);
    if (event != events.end()) {
        std::mutex lk;
        lk.lock();
        events.erase(event);
        lk.unlock();
    }
}

// this just call the function in pool
void event_pool::trigger_event(const std::string &id_) {
    auto event = events.find(id_);
    if (event != events.end()) {
        active_events.emplace(event->second);
        sem_post(&at_least_one_active);
//        at_least_one_active.notify_all();
        return;
    }
    throw std::runtime_error("fail to trigger: no registered events matched");
}

//  this creates a temporary event and adds it into the active pool.
//  but it's not hold by the event pool. which means after execution,
//  it will expire
template <typename ...Args>
void event_pool::trigger_event(const std::string &id_, Args... args) {
    auto event = events.find(id_);
    if (event != events.end()) {
        auto tmp_func = dynamic_cast<handle<void (Args...)>*>(event
                ->second.get())
                        ->get_func();
        handle_ptr_t tmp_handle(new handle<void (Args...)>(tmp_func, args...));
        active_events.emplace(tmp_handle);
        sem_post(&at_least_one_active);
//        at_least_one_active.notify_all();
        return;
    }
    throw std::runtime_error("fail to trigger: no registered events matched");
}

template <typename ...Args>
void event_pool::trigger_and_set(const std::string &id_, Args ...args) {
    auto event = events.find(id_);
    if (event != events.end()) {
        dynamic_cast<handle<void (Args...)>*> (event->second.get())
                                                ->set(args...);
        active_events.emplace(event->second);
        sem_post(&at_least_one_active);
//        at_least_one_active.notify_all();
        return;
    }
    throw std::runtime_error("fail to trigger: no registered events matched");

}

#endif //EVENT_MANAGER_EDA_H
