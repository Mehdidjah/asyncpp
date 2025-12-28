#pragma once
#ifndef INC_PROMISE_WIN32_HPP_
#define INC_PROMISE_WIN32_HPP_
#include "async-promise/promise.hpp"
#include <chrono>
#include <map>
#include <windows.h>
namespace promise {
inline void cancelDelay(Defer d);
inline void clearTimeout(Defer d);
struct WindowsTimerHolder {
    WindowsTimerHolder() {};
public:
    static Promise delay(int time_ms) {
        UINT_PTR timerId = ::SetTimer(NULL, 0, (UINT)time_ms, &WindowsTimerHolder::timerEvent);
        return newPromise([timerId](Defer &Defer) {
            WindowsTimerHolder::getDefers().insert({ timerId, Defer });
        }).then([timerId]() {
            WindowsTimerHolder::getDefers().erase(timerId);
            return promise::resolve();
        }, [timerId]() {
            ::KillTimer(NULL, timerId);
            WindowsTimerHolder::getDefers().erase(timerId);
            return promise::reject();
        });
    }
    static Promise yield() {
        return delay(0);
    }
    static Promise setTimeout(const std::function<void(bool)> &func,
                              int time_ms) {
        return delay(time_ms).then([func]() {
            func(false);
        }, [func]() {
            func(true);
        });
    }
protected:
    static void CALLBACK timerEvent(HWND unnamedParam1,
                                    UINT unnamedParam2,
                                    UINT_PTR timerId,
                                    DWORD unnamedParam4) {
        (void)unnamedParam1;
        (void)unnamedParam2;
        (void)unnamedParam4;
        auto found = getDefers().find(timerId);
        if (found != getDefers().end()) {
            Defer d = found->second;
            d.resolve();
        }
    }
private:
    friend void cancelDelay(Defer d);
    inline static std::map<UINT_PTR, promise::Defer> &getDefers() {
        static std::map<UINT_PTR, promise::Defer>  s_defers_;
        return s_defers_;
    }
};
inline Promise delay(int time_ms) {
    return WindowsTimerHolder::delay(time_ms);
}
inline Promise yield() {
    return WindowsTimerHolder::yield();
}
inline Promise setTimeout(const std::function<void(bool)> &func,
    int time_ms) {
    return WindowsTimerHolder::setTimeout(func, time_ms);
}
inline void cancelDelay(Defer d) {
    d.reject();
}
inline void clearTimeout(Defer d) {
    cancelDelay(d);
}
}
#endif