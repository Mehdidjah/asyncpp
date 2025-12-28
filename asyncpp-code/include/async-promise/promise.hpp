#pragma once
#ifndef INC_PROMISE_HPP_
#define INC_PROMISE_HPP_
#if defined PROMISE_HEADONLY
#define PROMISE_API inline
#elif defined PROMISE_BUILD_SHARED
#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(promise_EXPORTS)
#    ifdef __GNUC__
#      define  PROMISE_API __attribute__(dllexport)
#    else
#      define  PROMISE_API __declspec(dllexport)
#    endif
#  else
#    ifdef __GNUC__
#      define  PROMISE_API __attribute__(dllimport)
#    else
#      define  PROMISE_API __declspec(dllimport)
#    endif
#  endif
#elif defined __GNUC__
#  if __GNUC__ >= 4
#    define PROMISE_API __attribute__ ((visibility ("default")))
#  else
#    define PROMISE_API
#  endif
#elif defined __clang__
#  define PROMISE_API __attribute__ ((visibility ("default")))
#else
#   error "Do not know how to export classes for this platform"
#endif
#else
#define PROMISE_API
#endif
#include <list>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include "any_type.hpp"
namespace promise {
enum class TaskState {
    kPending,
    kResolved,
    kRejected
};
struct PromiseHolder;
struct SharedPromise;
class Promise;
struct Task {
    TaskState state_;
    std::weak_ptr<PromiseHolder> promiseHolder_;
    any                          onResolved_;
    any                          onRejected_;
};
struct PromiseHolder {
    PROMISE_API PromiseHolder();
    PROMISE_API ~PromiseHolder();
    std::list<std::weak_ptr<SharedPromise>> owners_;
    std::list<std::shared_ptr<Task>>        pendingTasks_;
    TaskState                               state_;
    any                                     value_;
    mutable std::recursive_mutex mutex_;
    std::condition_variable_any cond_;

    PROMISE_API void dump() const;
    PROMISE_API static any *getUncaughtExceptionHandler();
    PROMISE_API static any *getDefaultUncaughtExceptionHandler();
    PROMISE_API static void onUncaughtException(const any &arg);
    PROMISE_API static void handleUncaughtException(const any &onUncaughtException);
};
template<typename ...ARGS>
struct is_one_any : public std::is_same<typename tuple_remove_cvref<std::tuple<ARGS...>>::type, std::tuple<any>> {
};
struct SharedPromise {
    std::shared_ptr<PromiseHolder> promiseHolder_;
    PROMISE_API void dump() const;
#if PROMISE_MULTITHREAD
#endif
};
class Defer {
public:
    template<typename ...ARGS>
    inline std::enable_if_t<!is_one_any<ARGS...>::value> resolve(ARGS &&...args) const {
        resolve(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }
    template<typename ...ARGS>
    inline std::enable_if_t<!is_one_any<ARGS...>::value> reject(ARGS &&...args) const {
        reject(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }
    PROMISE_API void resolve(const any &arg) const;
    PROMISE_API void reject(const any &arg) const;
    PROMISE_API Promise getPromise() const;
private:
    friend class Promise;
    friend PROMISE_API Promise newPromise(const std::function<void(Defer &defer)> &run);
    PROMISE_API Defer(const std::shared_ptr<Task> &task);
    std::shared_ptr<Task>          task_;
    std::shared_ptr<SharedPromise> sharedPromise_;
};
class DeferLoop {
public:
    template<typename ...ARGS>
    inline std::enable_if_t<!is_one_any<ARGS...>::value> doBreak(ARGS &&...args) const {
        doBreak(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }
    template<typename ...ARGS>
    inline std::enable_if_t<!is_one_any<ARGS...>::value> reject(ARGS &&...args) const {
        reject(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }
    PROMISE_API void doContinue() const;
    PROMISE_API void doBreak(const any &arg) const;
    PROMISE_API void reject(const any &arg) const;
    PROMISE_API Promise getPromise() const;
private:
    friend PROMISE_API Promise doWhile(const std::function<void(DeferLoop &loop)> &run);
    PROMISE_API DeferLoop(const Defer &cb);
    Defer defer_;
};
class Promise {
public:
    PROMISE_API Promise &then(const any &deferOrPromiseOrOnResolved);
    PROMISE_API Promise &then(const any &onResolved, const any &onRejected);
    PROMISE_API Promise &fail(const any &onRejected);
    PROMISE_API Promise &always(const any &onAlways);
    PROMISE_API Promise &finally(const any &onFinally);
    template<typename ...ARGS>
    inline std::enable_if_t<!is_one_any<ARGS...>::value> resolve(ARGS &&...args) const {
        resolve(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }
    template<typename ...ARGS>
    inline std::enable_if_t<!is_one_any<ARGS...>::value> reject(ARGS &&...args) const {
        reject(any{ std::vector<any>{std::forward<ARGS>(args)...} });
    }
    PROMISE_API void resolve(const any &arg) const;
    PROMISE_API void reject(const any &arg) const;
    PROMISE_API void clear();
    PROMISE_API operator bool() const;
    PROMISE_API void dump() const;
    std::shared_ptr<SharedPromise> sharedPromise_;
};
PROMISE_API Promise newPromise(const std::function<void(Defer &defer)> &run);
PROMISE_API Promise newPromise();
PROMISE_API Promise doWhile(const std::function<void(DeferLoop &loop)> &run);
template<typename ...ARGS>
inline Promise resolve(ARGS &&...args) {
    return newPromise([&args...](Defer &defer) { defer.resolve(std::forward<ARGS>(args)...); });
}
template<typename ...ARGS>
inline Promise reject(ARGS &&...args) {
    return newPromise([&args...](Defer &defer) { defer.reject(std::forward<ARGS>(args)...); });
}
PROMISE_API Promise all(const std::list<Promise> &promise_list);
template<typename PROMISE_LIST>
inline auto all(const PROMISE_LIST &promise_list) -> std::enable_if_t<is_iterable<PROMISE_LIST>::value && !std::is_same_v<PROMISE_LIST, std::list<Promise>>, Promise> {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return all(copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST>
inline auto all(PROMISE0 defer0, PROMISE_LIST ...promise_list) -> std::enable_if_t<!is_iterable<PROMISE0>::value, Promise> {
    return all(std::list<Promise>{ defer0, promise_list ... });
}
PROMISE_API Promise race(const std::list<Promise> &promise_list);
template<typename PROMISE_LIST>
inline auto race(const PROMISE_LIST &promise_list) -> std::enable_if_t<is_iterable<PROMISE_LIST>::value && !std::is_same_v<PROMISE_LIST, std::list<Promise>>, Promise> {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return race(copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST>
inline auto race(PROMISE0 defer0, PROMISE_LIST ...promise_list) -> std::enable_if_t<!is_iterable<PROMISE0>::value, Promise> {
    return race(std::list<Promise>{ defer0, promise_list ... });
}
PROMISE_API Promise raceAndReject(const std::list<Promise> &promise_list);
template<typename PROMISE_LIST>
inline auto raceAndReject(const PROMISE_LIST &promise_list) -> std::enable_if_t<is_iterable<PROMISE_LIST>::value && !std::is_same_v<PROMISE_LIST, std::list<Promise>>, Promise> {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return raceAndReject(copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST>
inline auto raceAndReject(PROMISE0 defer0, PROMISE_LIST ...promise_list) -> std::enable_if_t<!is_iterable<PROMISE0>::value, Promise> {
    return raceAndReject(std::list<Promise>{ defer0, promise_list ... });
}
PROMISE_API Promise raceAndResolve(const std::list<Promise> &promise_list);
template<typename PROMISE_LIST>
inline auto raceAndResolve(const PROMISE_LIST &promise_list) -> std::enable_if_t<is_iterable<PROMISE_LIST>::value && !std::is_same_v<PROMISE_LIST, std::list<Promise>>, Promise> {
    std::list<Promise> copy_list = { std::begin(promise_list), std::end(promise_list) };
    return raceAndResolve(copy_list);
}
template <typename PROMISE0, typename ... PROMISE_LIST>
inline auto raceAndResolve(PROMISE0 defer0, PROMISE_LIST ...promise_list) -> std::enable_if_t<!is_iterable<PROMISE0>::value, Promise> {
    return raceAndResolve(std::list<Promise>{ defer0, promise_list ... });
}
inline void handleUncaughtException(const any &onUncaughtException) {
    PromiseHolder::handleUncaughtException(onUncaughtException);
}
}
#ifdef PROMISE_HEADONLY
#include "promise_implementation.hpp"
#endif
#endif