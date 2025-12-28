#pragma once
#ifndef INC_ASIO_TIMER_HPP_
#define INC_ASIO_TIMER_HPP_
#include "async-promise/promise.hpp"
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
namespace promise {
inline Promise yield(boost::asio::io_service &io){
    auto promise = newPromise([&io](Defer &defer) {
#if BOOST_VERSION >= 106600
        boost::asio::defer(io, [defer]() {
            defer.resolve();
        });
#else
        io.post([defer]() {
            defer.resolve();
        });
#endif
    });
    return promise;
}
inline Promise delay(boost::asio::io_service &io, uint64_t time_ms) {
    auto timer = std::make_shared<boost::asio::steady_timer>(io, std::chrono::milliseconds(time_ms));
    return newPromise([timer, &io](Defer &defer) {
        timer->async_wait([defer, timer](const boost::system::error_code& error_code) {
            if (timer) {
                defer.resolve();
            }
        });
    }).finally([timer]() {
        timer->cancel();
    });
}
inline void cancelDelay(Promise promise) {
    promise.reject();
}
inline Promise setTimeout(boost::asio::io_service &io,
                          const std::function<void(bool)> &func,
                          uint64_t time_ms) {
    return delay(io, time_ms).then([func]() {
        func(false);
    }, [func]() {
        func(true);
    });
}
inline void clearTimeout(Promise promise) {
    cancelDelay(promise);
}
#if 0
inline Promise wait(boost::asio::io_service &io, Defer d, uint64_t time_ms) {
    return newPromise([&io, d, time_ms](Defer &dTimer) {
        boost::asio::steady_timer *timer =
            pm_new<boost::asio::steady_timer>(io, std::chrono::milliseconds(time_ms));
        dTimer->any_ = timer;
        d.finally([=](){
            if (!dTimer->any_.empty()) {
                boost::asio::steady_timer *timer = any_cast<boost::asio::steady_timer *>(dTimer->any_);
                dTimer->any_.clear();
                timer->cancel();
                pm_delete(timer);
            }
        }).then(dTimer);
        timer->async_wait([=](const boost::system::error_code& error_code) {
            if (!dTimer->any_.empty()) {
                boost::asio::steady_timer *timer = any_cast<boost::asio::steady_timer *>(dTimer->any_);
                dTimer->any_.clear();
                pm_delete(timer);
                d.reject(boost::system::errc::make_error_code(boost::system::errc::timed_out));
            }
        });
    });
}
#endif
}
#endif