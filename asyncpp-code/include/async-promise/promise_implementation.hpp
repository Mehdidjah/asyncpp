#pragma once
#ifndef INC_PROMISE_IMPLEMENTATION_HPP_
#define INC_PROMISE_IMPLEMENTATION_HPP_
#include <cassert>
#include <stdexcept>
#include <vector>
#include <atomic>
#include "promise.hpp"

namespace promise {
static inline void healthyCheck(int line, PromiseHolder *promiseHolder) {
    (void)line;
    (void)promiseHolder;
#ifndef NDEBUG
    if (!promiseHolder) {
        fprintf(stderr, "line = %d, %d, promiseHolder is null\n", line, __LINE__);
        throw std::runtime_error("");
    }
    for (const auto &owner_ : promiseHolder->owners_) {
        auto owner = owner_.lock();
        if (owner && owner->promiseHolder_.get() != promiseHolder) {
            fprintf(stderr, "line = %d, %d, owner->promiseHolder_ = %p, promiseHolder = %p\n",
                line, __LINE__,
                owner->promiseHolder_.get(),
                promiseHolder);
            throw std::runtime_error("");
        }
    }
    for (const std::shared_ptr<Task> &task : promiseHolder->pendingTasks_) {
        if (!task) {
            fprintf(stderr, "line = %d, %d, promiseHolder = %p, task is null\n", line, __LINE__, promiseHolder);
            throw std::runtime_error("");
        }
        if (task->state_ != TaskState::kPending) {
            fprintf(stderr, "line = %d, %d, promiseHolder = %p, task = %p, task->state_ = %d\n", line, __LINE__,
                promiseHolder, task.get(), (int)task->state_);
            throw std::runtime_error("");
        }
        if (task->promiseHolder_.lock().get() != promiseHolder) {
            fprintf(stderr, "line = %d, %d, promiseHolder = %p, task = %p, task->promiseHolder_ = %p\n", line, __LINE__,
                promiseHolder, task.get(), task->promiseHolder_.lock().get());
            throw std::runtime_error("");
        }
    }
#endif
}
void Promise::dump() const {
#ifndef NDEBUG
    printf("Promise = %p, SharedPromise = %p\n", this, this->sharedPromise_.get());
    if (this->sharedPromise_)
        this->sharedPromise_->dump();
#endif
}
void SharedPromise::dump() const {
#ifndef NDEBUG
    printf("SharedPromise = %p, PromiseHolder = %p\n", this, this->promiseHolder_.get());
    if (this->promiseHolder_)
        this->promiseHolder_->dump();
#endif
}
void PromiseHolder::dump() const {
#ifndef NDEBUG
    printf("PromiseHolder = %p, owners = %d, pendingTasks = %d\n", this, (int)this->owners_.size(), (int)this->pendingTasks_.size());
    for (const auto &owner_ : owners_) {
        auto owner = owner_.lock();
        printf("  owner = %p\n", owner.get());
    }
    for (const auto &task : pendingTasks_) {
        if (task) {
            auto promiseHolder = task->promiseHolder_.lock();
            printf("  task = %p, PromiseHolder = %p\n", task.get(), promiseHolder.get());
        }
        else {
            printf("  task = %p\n", task.get());
        }
    }
#endif
}
static inline void join(const std::shared_ptr<PromiseHolder> &left, const std::shared_ptr<PromiseHolder> &right) {
    healthyCheck(__LINE__, left.get());
    healthyCheck(__LINE__, right.get());
    for (const std::shared_ptr<Task> &task : right->pendingTasks_) {
        task->promiseHolder_ = left;
    }
    left->pendingTasks_.splice(left->pendingTasks_.end(), right->pendingTasks_);
    std::list<std::weak_ptr<SharedPromise>> owners;
    owners.splice(owners.end(), right->owners_);
    right->state_ = TaskState::kResolved;
    if(owners.size() > 100) {
        fprintf(stderr, "Warning: Possible memory leak, too many promise owners: %zu\n", owners.size());
    }
    for (const std::weak_ptr<SharedPromise> &owner_ : owners) {
        std::shared_ptr<SharedPromise> owner = owner_.lock();
        if (owner) {
            owner->promiseHolder_ = left;
            left->owners_.push_back(owner);
        }
    }
    healthyCheck(__LINE__, left.get());
    healthyCheck(__LINE__, right.get());
}
static inline void call(std::shared_ptr<Task> task) {
    std::shared_ptr<PromiseHolder> promiseHolder;
    while (true) {
        promiseHolder = task->promiseHolder_.lock();
        if (!promiseHolder) return;
        std::unique_lock<std::recursive_mutex> lock(promiseHolder->mutex_);
            if (task->state_ != TaskState::kPending) return;
            if (promiseHolder->state_ == TaskState::kPending) return;
            std::list<std::shared_ptr<Task>> &pendingTasks = promiseHolder->pendingTasks_;
            while (pendingTasks.front() != task) {
                promiseHolder->cond_.wait(lock);
            }
            assert(pendingTasks.front() == task);
            pendingTasks.pop_front();
            task->state_ = promiseHolder->state_;
            try {
                if (promiseHolder->state_ == TaskState::kResolved) {
                    if (task->onResolved_.empty()
                        || task->onResolved_.type() == type_id<std::nullptr_t>()) {
                    }
                    else {
                        promiseHolder->state_ = TaskState::kPending;
                        const any &value = task->onResolved_.call(promiseHolder->value_);
                        if (value.type() != type_id<Promise>()) {
                            promiseHolder->value_ = value;
                            promiseHolder->state_ = TaskState::kResolved;
                        }
                        else {
                            Promise &promise = value.cast<Promise &>();
                            join(promise.sharedPromise_->promiseHolder_, promiseHolder);
                            promiseHolder = promise.sharedPromise_->promiseHolder_;
                        }
                    }
                }
                else if (promiseHolder->state_ == TaskState::kRejected) {
                    if (task->onRejected_.empty()
                        || task->onRejected_.type() == type_id<std::nullptr_t>()) {
                    }
                    else {
                        try {
                            promiseHolder->state_ = TaskState::kPending;
                            const any &value = task->onRejected_.call(promiseHolder->value_);
                            if (value.type() != type_id<Promise>()) {
                                promiseHolder->value_ = value;
                                promiseHolder->state_ = TaskState::kResolved;
                            }
                            else {
                                Promise &promise = value.cast<Promise &>();
                                join(promise.sharedPromise_->promiseHolder_, promiseHolder);
                                promiseHolder = promise.sharedPromise_->promiseHolder_;
                            }
                        }
                        catch (const bad_any_cast &) {
                            promiseHolder->state_ = TaskState::kRejected;
                        }
                    }
                }
            }
            catch (const promise::bad_any_cast &ex) {
                fprintf(stderr, "promise::bad_any_cast: %s -> %s", ex.from_.name(), ex.to_.name());
                promiseHolder->value_ = std::current_exception();
                promiseHolder->state_ = TaskState::kRejected;
            }
            catch (...) {
                promiseHolder->value_ = std::current_exception();
                promiseHolder->state_ = TaskState::kRejected;
            }
            task->onResolved_.clear();
            task->onRejected_.clear();
        }
        std::list<std::shared_ptr<Task>> &pendingTasks2 = promiseHolder->pendingTasks_;
        if (pendingTasks2.size() == 0) {
            return;
        }
        task = pendingTasks2.front();
    }
}
promise::Defer::Defer(const std::shared_ptr<Task> &task) {
    std::shared_ptr<SharedPromise> sharedPromise(new SharedPromise{ task->promiseHolder_.lock() });
    task_ = task;
    sharedPromise_ = sharedPromise;
}
void promise::Defer::resolve(const any &arg) const {
    std::unique_lock<std::recursive_mutex> lock(sharedPromise_->promiseHolder_->mutex_);
    if (task_->state_ != TaskState::kPending) return;
    std::shared_ptr<PromiseHolder> &promiseHolder = sharedPromise_->promiseHolder_;
    promiseHolder->state_ = TaskState::kResolved;
    promiseHolder->value_ = arg;
    call(task_);
}
void promise::Defer::reject(const any &arg) const {
    std::unique_lock<std::recursive_mutex> lock(sharedPromise_->promiseHolder_->mutex_);
    if (task_->state_ != TaskState::kPending) return;
    std::shared_ptr<PromiseHolder> &promiseHolder = sharedPromise_->promiseHolder_;
    promiseHolder->state_ = TaskState::kRejected;
    promiseHolder->value_ = arg;
    call(task_);
}
promise::Promise promise::Defer::getPromise() const {
    return Promise{ sharedPromise_ };
}
struct DoBreakTag {};
promise::DeferLoop::DeferLoop(const promise::Defer &defer)
    : defer_(defer) {
}
void promise::DeferLoop::doContinue() const {
    defer_.resolve();
}
void promise::DeferLoop::doBreak(const any &arg) const {
    defer_.reject(DoBreakTag(), arg);
}
void promise::DeferLoop::reject(const any &arg) const {
    defer_.reject(arg);
}
promise::Promise promise::DeferLoop::getPromise() const {
    return defer_.getPromise();
}
promise::PromiseHolder::PromiseHolder()
    : owners_()
    , pendingTasks_()
    , state_(TaskState::kPending)
    , value_()
    , mutex_()
    , cond_()
{
}
promise::PromiseHolder::~PromiseHolder() {
    if (this->state_ == TaskState::kRejected) {
        static thread_local std::atomic<bool> s_inUncaughtExceptionHandler{false};
        if(s_inUncaughtExceptionHandler) return;
        s_inUncaughtExceptionHandler = true;
        struct Releaser {
            Releaser(std::atomic<bool> *inUncaughtExceptionHandler)
                : inUncaughtExceptionHandler_(inUncaughtExceptionHandler) {}
            ~Releaser() {
                *inUncaughtExceptionHandler_ = false;
            }
            std::atomic<bool> *inUncaughtExceptionHandler_;
        } releaser(&s_inUncaughtExceptionHandler);
        PromiseHolder::onUncaughtException(this->value_);
    }
}
promise::any *promise::PromiseHolder::getUncaughtExceptionHandler() {
    static any onUncaughtException;
    return &onUncaughtException;
}
promise::any *promise::PromiseHolder::getDefaultUncaughtExceptionHandler() {
    static any defaultUncaughtExceptionHandler = [](Promise &d) {
        d.fail([](const std::runtime_error &err) {
            fprintf(stderr, "onUncaughtException in line %d, %s\n", __LINE__, err.what());
        }).fail([]() {
            fprintf(stderr, "onUncaughtException in line %d\n", __LINE__);
        });
    };
    return &defaultUncaughtExceptionHandler;
}
void promise::PromiseHolder::onUncaughtException(const promise::any &arg) {
    any *onUncaughtException = getUncaughtExceptionHandler();
    if (onUncaughtException == nullptr || onUncaughtException->empty()) {
        onUncaughtException = getDefaultUncaughtExceptionHandler();
    }
    try {
        onUncaughtException->call(reject(arg));
    }
    catch (...) {
        fprintf(stderr, "onUncaughtException in line %d\n", __LINE__);
    }
}
void promise::PromiseHolder::handleUncaughtException(const promise::any &onUncaughtException) {
    (*getUncaughtExceptionHandler()) = onUncaughtException;
}
promise::Promise &promise::Promise::then(const promise::any &deferOrPromiseOrOnResolved) {
    if (deferOrPromiseOrOnResolved.type() == type_id<Defer>()) {
        Defer &defer = deferOrPromiseOrOnResolved.cast<Defer &>();
        Promise promise = defer.getPromise();
        Promise &ret = then([defer](const any &arg) -> any {
            defer.resolve(arg);
            return nullptr;
        }, [defer](const any &arg) ->any {
            defer.reject(arg);
            return nullptr;
        });
        promise.finally([=]() {
            ret.reject();
        });
        return ret;
    }
    else if (deferOrPromiseOrOnResolved.type() == type_id<DeferLoop>()) {
        DeferLoop &loop = deferOrPromiseOrOnResolved.cast<DeferLoop &>();
        Promise promise = loop.getPromise();
        Promise &ret = then([loop](const any &arg) -> any {
            (void)arg;
            loop.doContinue();
            return nullptr;
        }, [loop](const any &arg) ->any {
            loop.reject(arg);
            return nullptr;
        });
        promise.finally([=]() {
            ret.reject();
        });
        return ret;
    }
    else if (deferOrPromiseOrOnResolved.type() == type_id<Promise>()) {
        Promise &promise = deferOrPromiseOrOnResolved.cast<Promise &>();
        std::shared_ptr<Task> task;
        if (promise.sharedPromise_ && promise.sharedPromise_->promiseHolder_) {
            join(this->sharedPromise_->promiseHolder_, promise.sharedPromise_->promiseHolder_);
            if (this->sharedPromise_->promiseHolder_->pendingTasks_.size() > 0) {
                task = this->sharedPromise_->promiseHolder_->pendingTasks_.front();
            }
        }
        if(task)
            call(task);
        return *this;
    }
    else {
        return then(deferOrPromiseOrOnResolved, any());
    }
}
promise::Promise &promise::Promise::then(const promise::any &onResolved, const promise::any &onRejected) {
    std::shared_ptr<Task> task;
    task = std::make_shared<Task>(Task {
        TaskState::kPending,
        sharedPromise_->promiseHolder_,
        onResolved,
        onRejected
    });
    sharedPromise_->promiseHolder_->pendingTasks_.push_back(task);
    call(task);
    return *this;
}
promise::Promise &promise::Promise::fail(const promise::any &onRejected) {
    return then(any(), onRejected);
}
promise::Promise &promise::Promise::always(const promise::any &onAlways) {
    return then(onAlways, onAlways);
}
promise::Promise &promise::Promise::finally(const promise::any &onFinally) {
    return then([onFinally](const any &arg)->any {
        return newPromise([onFinally, arg](Defer &defer) {
            try {
                onFinally.call(arg);
            }
            catch (bad_any_cast &) {}
            defer.resolve(arg);
        });
    }, [onFinally](const any &arg)->any {
        return newPromise([onFinally, arg](Defer &defer) {
            try {
                onFinally.call(arg);
            }
            catch (bad_any_cast &) {}
            defer.reject(arg);
        });
    });
}
void promise::Promise::resolve(const promise::any &arg) const {
    if (!this->sharedPromise_) return;
    sharedPromise_->promiseHolder_->state_ = TaskState::kResolved;
    sharedPromise_->promiseHolder_->value_ = arg;
    std::shared_ptr<Task> task;
    std::list<std::shared_ptr<Task>> &pendingTasks_ = sharedPromise_->promiseHolder_->pendingTasks_;
    if (pendingTasks_.size() > 0) {
        task = pendingTasks_.front();
    }
    if (task) {
        call(task);
    }
}
void promise::Promise::reject(const promise::any &arg) const {
    if (!this->sharedPromise_) return;
    std::shared_ptr<Task> task;
    std::list<std::shared_ptr<Task>> &pendingTasks_ = this->sharedPromise_->promiseHolder_->pendingTasks_;
    if (pendingTasks_.size() > 0) {
        task = pendingTasks_.front();
    }
    if (task) {
        Defer defer(task);
        defer.reject(arg);
    }
}
void promise::Promise::clear() {
    sharedPromise_.reset();
}
promise::Promise::operator bool() const {
    return sharedPromise_.operator bool();
}
promise::Promise promise::newPromise(const std::function<void(promise::Defer &defer)> &run) {
    Promise promise;
    promise.sharedPromise_ = std::make_shared<SharedPromise>();
    promise.sharedPromise_->promiseHolder_ = std::make_shared<PromiseHolder>();
    promise.sharedPromise_->promiseHolder_->owners_.push_back(promise.sharedPromise_);
    promise.then(any(), any());
    std::shared_ptr<Task> &task = promise.sharedPromise_->promiseHolder_->pendingTasks_.front();
    Defer defer(task);
    try {
        run(defer);
    }
    catch (...) {
        defer.reject(std::current_exception());
    }
    return promise;
}
promise::Promise promise::newPromise() {
    Promise promise;
    promise.sharedPromise_ = std::make_shared<SharedPromise>();
    promise.sharedPromise_->promiseHolder_ = std::make_shared<PromiseHolder>();
    promise.sharedPromise_->promiseHolder_->owners_.push_back(promise.sharedPromise_);
    promise.then(any(), any());
    return promise;
}
promise::Promise promise::doWhile(const std::function<void(promise::DeferLoop &loop)> &run) {
    return promise::newPromise([run](promise::Defer &defer) {
        promise::DeferLoop loop(defer);
        run(loop);
    }).then([run](const promise::any &arg) -> promise::any {
        (void)arg;
        return promise::doWhile(run);
    }, [](const promise::any &arg) -> promise::any {
        return promise::newPromise([arg](promise::Defer &defer) {
            bool isBreak = false;
            if (arg.type() == promise::type_id<std::vector<promise::any>>()) {
                std::vector<promise::any> &args = promise::any_cast<std::vector<promise::any> &>(arg);
                if (args.size() == 2
                    && args.front().type() == promise::type_id<DoBreakTag>()
                    && args.back().type() == promise::type_id<std::vector<promise::any>>()) {
                    isBreak = true;
                    defer.resolve(args.back());
                }
            }
            if(!isBreak) {
                defer.reject(arg);
            }
        });
    });
}
promise::Promise promise::all(const std::list<promise::Promise> &promise_list) {
    if (promise_list.empty()) {
        return promise::resolve();
    }
    auto finished = std::make_shared<size_t>(0);
    auto size = promise_list.size();
    auto retArr = std::make_shared<std::vector<promise::any>>();
    retArr->resize(size);
    return promise::newPromise([=](promise::Defer &defer) {
        size_t index = 0;
        for (auto promise : promise_list) {
            const size_t current_index = index;
            promise.then([retArr, finished, size, defer, current_index](const promise::any &arg) {
                (*retArr)[current_index] = arg;
                if (++(*finished) >= size) {
                    defer.resolve(*retArr);
                }
            }, [defer](const promise::any &arg) {
                defer.reject(arg);
            });
            ++index;
        }
    });
}
static promise::Promise race(const std::list<promise::Promise> &promise_list, std::shared_ptr<int> winner) {
    return promise::newPromise([=](promise::Defer &defer) {
        int index = 0;
        for (auto promise : promise_list) {
            const int current_index = index;
            promise.then([winner, defer, current_index](const promise::any &arg) {
                *winner = current_index;
                defer.resolve(arg);
                return arg;
            }, [winner, defer, current_index](const promise::any &arg) {
                *winner = current_index;
                defer.reject(arg);
                return arg;
            });
            ++index;
        }
    });
}
promise::Promise promise::race(const std::list<promise::Promise> &promise_list) {
    std::shared_ptr<int> winner = std::make_shared<int>(-1);
    return ::race(promise_list, winner);
}
promise::Promise promise::raceAndReject(const std::list<promise::Promise> &promise_list) {
    std::shared_ptr<int> winner = std::make_shared<int>(-1);
    return ::race(promise_list, winner).finally([promise_list, winner] {
        int index = 0;
        for (auto promise : promise_list) {
            if (index != *winner) {
                promise.reject();
            }
            ++index;
        }
    });
}
promise::Promise promise::raceAndResolve(const std::list<promise::Promise> &promise_list) {
    std::shared_ptr<int> winner = std::make_shared<int>(-1);
    return ::race(promise_list, winner).finally([promise_list, winner] {
        int index = 0;
        for (auto promise : promise_list) {
            if (index != *winner) {
                promise.resolve();
            }
            ++index;
        }
    });
}
#endif