//
// Created by zelin on 2022/5/19.
//
#include "handle.h"
#include <cstdio>
#include <functional>
#include <iostream>

struct Foo {
    Foo(int num) : num_(num) {}
    void print_add(int i) const { std::cout << num_+i << '\n'; }
    int num_;
};

void print_num(int i)
{
    std::cout << i << '\n';
}

struct PrintNum {
    void operator()(int i) const
    {
        std::cout << i << '\n';
    }
};

void print(){
    printf("basic print\n");
}

int main()
{
    //  basic function pointer
    handle<void()> fp(print);
    fp();
    // store a free function
    handle<void(int)> f_display(print_num);
    handle<void(int)> f_display1 = print_num;
    f_display(-9);
    f_display1(-0.9);

    // store a lambda
    handle<void()> f_display_42 = []() { print_num(42); };
    handle<void()> f_display_421([]() { print_num(42); });
    f_display_42();
    f_display_421();

    // store the result of a call to std::bind
    handle<void()> f_display_31337 = std::bind(print_num, 31337);
    f_display_31337();

    // todo store a call to a member function
//    handle<void(const Foo&, int)> f_add_display = &Foo::print_add;
//    handle<void(const Foo&, int)> f_add_display(&Foo::print_add);
   const Foo foo(314159);
//    f_add_display(foo, 1);
//    f_add_display(314159, 1);

    // todo store a call to a data member accessor
//    handle<int(Foo const&)> f_num = &Foo::num_;
//    std::cout << "num_: " << f_num(foo) << '\n';

    // store a call to a member function and object
    using std::placeholders::_1;
    handle<void(int)> f_add_display2 = std::bind( &Foo::print_add, foo, _1 );
    f_add_display2(2);
    // store a call to a member function and object ptr
    handle<void(int)> f_add_display3 = std::bind( &Foo::print_add, &foo, _1 );
    f_add_display3(3);

    // store a call to a function object
    PrintNum p;
    handle<void(int)> f_display_obj = PrintNum();
    handle<void(int)> f_display_obj1 (p);
    f_display_obj(18);
    f_display_obj1(19);

    auto factorial = [](int n) {
        // store a lambda object to emulate "recursive lambda"; aware of extra overhead
        handle<int(int)> fac = [&](int n){ return (n < 2) ? 1 : n*fac(n-1); };
        // note that "auto fac = [&](int n){...};" does not work in recursive calls
        return fac(n);
    };
    for (int i{5}; i != 8; ++i) { std::cout << i << "! = " << factorial(i) << ";  "; }
}
