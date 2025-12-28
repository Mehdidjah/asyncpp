#include <stdio.h>
#include <string>
#include <map>
#include <chrono>
#include <thread>
#include <utility>
#include "async-promise/promise.hpp"
#include "extensions/task_scheduler/simple_task.hpp"
using namespace promise;
Promise testTimer(Service &service) {
    return service.delay(3000).then([&] {
        printf("timer after 3000 ms!\n");
        return service.delay(1000);
    }).then([&] {
        printf("timer after 1000 ms!\n");
        return service.delay(2000);
    }).then([] {
        printf("timer after 2000 ms!\n");
    }).fail([] {
        printf("timer cancelled!\n");
    });
}
int main() {
    Service service;
    testTimer(service);
    testTimer(service);
    testTimer(service);
    service.run();
    return 0;
}