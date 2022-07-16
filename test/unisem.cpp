//
// Created by zelin on 2022/5/11.
//
#include "sema.h"
#include <algorithm>

int main() {
    Semaphore s;
    Semaphore s1 = std::move(s);
    s.release();
    s.acquire();
    //  not allowed to copy
    // auto a = s;
    // Semaphore b(s);
    // auto c = std::copy(s);
    return 0;
}