#include <stdio.h>
#include <string>
#include <vector>
#include "async-promise/promise.hpp"
using namespace promise;
#define output_func_name() do{ printf("in function %s, line %d\n", __func__, __LINE__); } while(0)
void test1() {
    output_func_name();
}
int test2() {
    output_func_name();
    return 5;
}
void test3(int n) {
    output_func_name();
    printf("n = %d\n", n);
}
Promise run(Promise &next){
    return newPromise([](Defer d) {
        output_func_name();
        d.resolve(3, 5, 6);
    }).then([](pm_any &any){
        output_func_name();
        return resolve(3, 5, 16);
    }).then([](const int &a, int b, int c) {
        printf("%d %d %d\n", a, b, c);
        output_func_name();
    }).then([](){
        output_func_name();
    }).then([&next](){
        output_func_name();
        next = newPromise([](Defer d) {
            output_func_name();
        });
        return next;
    }).finally([](int n) {
        printf("finally n == %d\n", n);
    }).then([](int n, char c) {
        output_func_name();
        printf("n = %d, c = %c\n", (int)n, c);
    }).then([](char n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](char n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](short n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](int &n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](const std::string &str) {
        output_func_name();
        printf("str = %s\n", str.c_str());
    }).fail([](uint64_t n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).then(test1)
    .then(test2)
    .then(&test3);
}
void test_promise_all() {
    std::vector<Promise> ds = {
        newPromise([](Defer d) {
            output_func_name();
            d.resolve(1);
        }),
        newPromise([](Defer d) {
            output_func_name();
            d.resolve(2);
        })
    };
    all(ds).then([]() {
        output_func_name();
    }, []() {
        output_func_name();
    });
}
int main(int argc, char **argv) {
    handleUncaughtException([](Promise &d) {
        d.fail([](long n, int m) {
            printf("UncaughtException parameters = %d %d\n", (int)n, m);
        }).fail([]() {
            printf("UncaughtException\n");
        });
    });
    test_promise_all();
    Promise next;
    run(next);
    printf("======  after call run ======\n");
    if(next) {
        next.reject((long)123, (int)345);
    }
    return 0;
}