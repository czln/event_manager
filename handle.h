//
// Created by zelin on 2022/4/11.
//

#ifndef EVENT_MANAGER_HANDLE_H
#define EVENT_MANAGER_HANDLE_H

#include <future>
#include <type_traits>

template <typename T>
struct type_identity {
    using type = T;
};

template <typename T>
using type_identity_t =  typename type_identity<T>::type;


class handle_base {
private:
    std::launch strategy = std::launch::deferred;
public:
    virtual ~handle_base() = default;
    virtual void run() = 0;


};

using handle_ptr_t = std::shared_ptr<handle_base>;
#if __cplusplus < 201401
/// \todo
#elif __cplusplus < 201703  // c++ 14
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
#else  // c++ 17
template<typename Function, typename Tuple>
auto call(Function f, Tuple t) {
    return std::apply(f, t);
}
#endif


template <typename ...T>
class handle : public handle_base { };

///< handle for no arg function
template <typename Ret>
class handle<Ret()> : public handle_base {
private:
    std::function<Ret()> function_;
public:
    template<typename Functor>
    handle(Functor func):
        function_(func) {
    }
    Ret operator()() {
        function_();
    }
    void run() {
        function_();
    }
    
    /// copy
    handle(const handle<Ret()>& lhs) :
        function_(lhs.function_) {}

//    handle& operator= (const handle<Ret(Args...)>& lhs) {
//        function_ = lhs.function_;
//        args_ = lhs.args_;
//        return *this;
//    }

    template<typename Func>
    handle& operator=(Func function)  {
        function_(function);
    }
};

template <typename Ret, typename ...Args>
class handle<Ret (Args...)> : public handle_base {
private:
    std::function<Ret(Args...)> function_;
    std::tuple<Args...> args_;
public:
    typedef Ret func_ptr_t (Args...);
    // handle(func_ptr_t func_ptr) :function_(func_ptr) { }

    // template<typename Func>
    // handle(Func function, Args... args) :
    //         function_(function),
    //         args_(std::make_tuple(args...)) { }

   template<typename Functor>
   handle(Functor func, Args... args) :
           function_(func),
           args_(std::make_tuple(args...)) {}
    // template<typename Class>
    // handle(Class functor, Args... args) :
    //         function_(functor) {
    //             set(args...);
    //         }
           
   template<typename Functor>
   handle(Functor func):
           function_(func) {}

//    template<typename Class, typename Method>
//    handle(Ret Class::*Method)

    /// copy
    handle(const handle<Ret(Args...)>& lhs) :
        function_(lhs.function_),
        args_(lhs.args_) {
    }

//    handle& operator= (const handle<Ret(Args...)>& lhs) {
//        function_ = lhs.function_;
//        args_ = lhs.args_;
//        return *this;
//    }

    template<typename Func>
    handle& operator=(Func function)  {
        function_(function);
    }



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

    Ret operator()(Args... args) {
        return call(function_,std::make_tuple(args...));
    }

    // /// enabled only when \tparam T is void
    // template <typename T, std::enable_if_t<std::is_void_v<T>, int> = 0>
    // Ret operator()(T t) {
    //     return call(function_, args_);
    // }

};

// template <typename Func, typename ...Args>
// inline handle_ptr_t create_handle_ptr(Func func, Args... args) {
//     using result_type = std::result_of_t<Func>;
//     return std::make_shared<handle<result_type(Args...)>>(std::forward<Func>(func), args...);
// }

#endif //EVENT_MANAGER_HANDLE_H
