//
// Created by zelin on 2021/8/6.
//
#include <chrono>
#include <thread>
#include <future>
#include "unistd.h"
#include "event_pool.h"
int main() {
    // handle test
    //  result: all passed
    handle<int(const char*)> h(printf, "1\n");

    handle<void()> h1([](){ printf("h void lambda test\n");});
    handle<void(int)> h2([](int a){ printf("h int lambda test %d\n",a);}, 1);
    handle<void(int,int)> h3([](int a, int b){ printf("h multi lambda test %d\n", a+b);}, 1,2);

    int a = 1;
    handle<void()> h4([a](){ printf("captured lambda test %d\n", a);});

    std::function<void()> f1 = [](){ printf("std::function test\n");};
    struct functor { void operator()(){ printf("functor void test\n");}
    void operator()(int a) { printf("functor arg test %d\n", a);}
    };
    handle<void()> h11(f1);
    functor ft;
    handle<void()> h21(ft);
    handle<void(int)> h22(ft, 1);


    h.run();
    h1.run();
    h2.run();
    h3.run();
    h4.run();
    h11.run();

    h21.run();
    h22.run();


    /// multithread test
    // std::thread(&handle_base::run, &h1).detach();
    // std::thread(&handle_base::run, &h1).detach();
    // std::thread(&handle_base::run, &h1).detach();
    // std::thread(&handle_base::run, &h2).detach();
    // std::thread(&handle_base::run, &h2).detach();
    // std::thread(&handle_base::run, &h2).detach();
    // std::thread(&handle_base::run, &h3).join();


    /// event pool \test
    event_pool ep;
    // std::thread p(&event_pool::run, &ep);

    handle_ptr_t hp1(new handle<void()>([](){ printf("ep| void lambda test\n");}));
    std::shared_ptr<handle<void(int)>> hp2(new handle<void(int)>([](int a){ printf("ep| 1 arg lambda test %d\n", a);}, 1));

    ep.register_callback("1", hp1);
    ep.register_callback("2", hp2);
    ep.register_callback("5", printf, "1\n");
//    ep.register_callback("6", printf, std::placeholders::_1); /// \todo placeholders support
    ep.register_callback("3", [](){ printf("direct call|lambda| void lambda test\n");});

    ep.register_callback("4", [](){sleep(3);printf("lambda terminate test\n");});

    ep.trigger_callback("1");
    ep.trigger_callback("2");
    ep.trigger_callback("2", 2);
    ep.trigger_callback("2", 3);
    ep.trigger_callback("2", 4);
    ep.trigger_callback("2", 5);
    ep.trigger_callback("2", 6);
    ep.trigger_and_set("2", 10);
    ep.trigger_callback("2");
    ep.trigger_callback("2");
    ep.trigger_callback("2");
    ep.trigger_callback("2");

    ep.trigger_callback("3");

    ep.trigger_callback("4");

    /// end the jobs
    ep.terminate();

    printf("all tests passed\n");


    printf("now for error tests\n");
    /// error tests:
    event_pool ep1;

    /// no match test
    ep1.register_callback("no match", [](int a){printf("test for not matched args %d\n", a);}, 0);
    // ep1.trigger_callback("no match", 1.1);  // error

    // implicit match test
    ep.register_callback("1", [](std::string s){}, "");
    // ep.trigger_callback("1", std::string("")); // error this would cause bad_cast



    return 0;
}