#include <stdio.h>
#include <iostream>
#include <string>
#include <chrono>
#include "async-promise/promise.hpp"
#include "extensions/task_scheduler/simple_task.hpp"
using namespace promise;
namespace chrono       = std::chrono;
using     steady_clock = std::chrono::steady_clock;
static const int N = 1000000;
void dump(std::string name, int n,
    steady_clock::time_point start,
    steady_clock::time_point end)
{
    auto ns = chrono::duration_cast<chrono::nanoseconds>(end - start);
    std::cout << name << "    " << n << "      " <<
        ns.count() / n <<
        "ns/op" << std::endl;
}
void task(Service &io, int task_id, int count, int *pcoro, Defer defer) {
    if (count == 0) {
        -- *pcoro;
        if (*pcoro == 0)
            defer.resolve();
        return;
    }
    io.yield().then([=, &io]() {
        task(io, task_id, count - 1, pcoro, defer);
    });
};
Promise test_switch(Service &io, int coro) {
    steady_clock::time_point start = steady_clock::now();
    int *pcoro = new int(coro);
    return newPromise([=, &io](Defer &defer){
        for (int task_id = 0; task_id < coro; ++task_id) {
            task(io, task_id, N / coro, pcoro, defer);
        }
    }).then([=]() {
        delete pcoro;
        steady_clock::time_point end = steady_clock::now();
        dump("BenchmarkSwitch_" + std::to_string(coro), N, start, end);
    });
}
int main() {
    Service io;
    int i = 0;
    doWhile([&](DeferLoop &loop) {
#ifdef PM_DEBUG
        printf("In while ..., alloc_size = %d\n", (int)(*dbg_alloc_size()));
#else
        printf("In while ...\n");
#endif
        test_switch(io, 1).then([&]() {
            return test_switch(io, 1000);
        }).then([&]() {
            return test_switch(io, 10000);
        }).then([&]() {
            return test_switch(io, 100000);
        }).then([&, loop]() {
            if (i++ > 3) loop.doBreak();
            return test_switch(io, 1000000);
        }).then(loop);
    });
    io.run();
    return 0;
}