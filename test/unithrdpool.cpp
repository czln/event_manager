//
// Created by zelin on 2022/5/17.
//

#include "threadpool.h"
#include <unistd.h>

int main() {
    ThreadPool tp(6);
//    handle_ptr_t handle(printf, "1\n");
    auto now = std::chrono::system_clock::now();
    std::atomic_uint i = 0;
    auto cb = [&i, now]() {
        auto comp = std::chrono::system_clock::now();
        printf("%u time cost: %lf \n", ++i, (comp-now).count() / 1e6);
    };
    for (size_t j=0; j<100; ++j) {
//        now = std::chrono::system_clock::now();
        tp.add_task(cb);
    }
    sleep(1);
    return 0;
}