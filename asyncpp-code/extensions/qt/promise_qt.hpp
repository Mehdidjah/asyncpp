#pragma once
#ifndef INC_PROMISE_QT_HPP_
#define INC_PROMISE_QT_HPP_
#include "async-promise/promise.hpp"
#include <chrono>
#include <set>
#include <atomic>
#include <QObject>
#include <QTimerEvent>
#include <QApplication>
#ifdef PROMISE_HEADONLY
#define PROMISE_QT_API inline
#elif defined PROMISE_BUILD_SHARED
#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(promise_qt_EXPORTS)
#    ifdef __GNUC__
#      define  PROMISE_QT_API __attribute__(dllexport)
#    else
#      define  PROMISE_QT_API __declspec(dllexport)
#    endif
#  else
#    ifdef __GNUC__
#      define  PROMISE_QT_API __attribute__(dllimport)
#    else
#      define  PROMISE_QT_API __declspec(dllimport)
#    endif
#  endif
#elif defined __GNUC__
#  if __GNUC__ >= 4
#    define PROMISE_QT_API __attribute__ ((visibility ("default")))
#  else
#    define PROMISE_QT_API
#  endif
#elif defined __clang__
#  define PROMISE_QT_API __attribute__ ((visibility ("default")))
#else
#   error "Do not know how to export classes for this platform"
#endif
#else
#define PROMISE_QT_API
#endif
namespace promise {
class PromiseEventListener;
class PromiseEventPrivate;
class PromiseEventFilter : public QObject {
private:
    PROMISE_QT_API PromiseEventFilter();
public:
    PROMISE_QT_API std::weak_ptr<PromiseEventListener> addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func);
    PROMISE_QT_API void removeEventListener(std::weak_ptr<PromiseEventListener> listener);
    PROMISE_QT_API static PromiseEventFilter &getSingleInstance();
protected:
    PROMISE_QT_API bool eventFilter(QObject *object, QEvent *event) override;
    std::shared_ptr<PromiseEventPrivate> private_;
};
PROMISE_QT_API Promise waitEvent(QObject      *object,
                                 QEvent::Type  eventType,
                                 bool          callSysHandler = false);
PROMISE_QT_API std::weak_ptr<PromiseEventListener> addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func);
PROMISE_QT_API void removeEventListener(std::weak_ptr<PromiseEventListener> listener);
struct QtTimerHolder: QObject {
    PROMISE_QT_API ~QtTimerHolder();
private:
    PROMISE_QT_API QtTimerHolder();
public:
    PROMISE_QT_API static Promise delay(int time_ms);
    PROMISE_QT_API static Promise yield();
    PROMISE_QT_API static Promise setTimeout(const std::function<void(bool)> &func,
                                             int time_ms);
protected:
    PROMISE_QT_API void timerEvent(QTimerEvent *event);
private:
    std::map<int, promise::Defer>  defers_;
    PROMISE_QT_API static QtTimerHolder &getInstance();
};
PROMISE_QT_API Promise delay(int time_ms);
PROMISE_QT_API Promise yield();
PROMISE_QT_API Promise setTimeout(const std::function<void(bool)> &func,
                                  int time_ms);
PROMISE_QT_API void cancelDelay(Promise promise);
PROMISE_QT_API void clearTimeout(Promise promise);
struct PROMISE_QT_API QtPromiseTimerHandler {
    void wait();
    QTimer *timer_;
};
PROMISE_QT_API std::shared_ptr<QtPromiseTimerHandler> qtPromiseSetTimeout(const std::function<void()> &cb, int ms);
}
#ifdef PROMISE_HEADONLY
#include "promise_qt_inl.hpp"
#endif
#endif