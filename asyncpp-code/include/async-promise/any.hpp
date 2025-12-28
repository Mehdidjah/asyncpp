#pragma once
#ifndef INC_PM_ANY_HPP_
#define INC_PM_ANY_HPP_
#include <vector>
#include <exception>
#include <utility>
#include <type_traits>
#include <memory>
#include <tuple>
#include "add_ons.hpp"
#include "call_traits.hpp"
namespace promise {
class any;
template<typename ValueType>
inline ValueType any_cast(const any &operand);
class any {
public:
    any() : content(0) {}
    template<typename ValueType>
    any(const ValueType &value)
        : content(new holder<typename std::remove_cvref<ValueType>::type>(value)) {
    }
    any(const any &other)
        : content(other.content ? other.content->clone() : 0) {
    }
    any(any &&other)
        : content(other.content) {
        other.content = 0;
    }
    ~any() {
        if (content != nullptr) {
            delete content;
        }
    }
    any call(const any &arg) const {
        return content ? content->call(arg) : any();
    }
    template<typename ValueType,
        typename std::enable_if<!std::is_pointer<ValueType>::value>::type *dummy = nullptr>
    inline ValueType cast() const {
        return any_cast<ValueType>(*this);
    }
    template<typename ValueType,
        typename std::enable_if<std::is_pointer<ValueType>::value>::type *dummy = nullptr>
    inline ValueType cast() const {
        if (this->empty())
            return nullptr;
        else
            return any_cast<ValueType>(*this);
    }
public:
    any & swap(any & rhs) {
        std::swap(content, rhs.content);
        return *this;
    }
    template<typename ValueType>
    any & operator=(const ValueType & rhs) {
        any(rhs).swap(*this);
        return *this;
    }
    any & operator=(const any & rhs) {
        any(rhs).swap(*this);
        return *this;
    }
public:
    bool empty() const {
        return !content;
    }
    void clear() {
        any().swap(*this);
    }
    type_index type() const {
        return content ? content->type() : type_id<void>();
    }
public:
    class placeholder {
    public:
        virtual ~placeholder() {}
    public:
        virtual type_index type() const = 0;
        virtual placeholder * clone() const = 0;
        virtual any call(const any &arg) const = 0;
    };
    template<typename ValueType>
    class holder : public placeholder {
    public:
        holder(const ValueType &value) : held(value) {}
    public:
        virtual type_index type() const {
            return type_id<ValueType>();
        }
        virtual placeholder * clone() const {
            return new holder(held);
        }
        virtual any call(const any &arg) const {
            return any_call(held, arg);
        }
    public:
        ValueType held;
    private:
        holder & operator=(const holder &);
    };
public:
    placeholder * content;
};
class bad_any_cast : public std::bad_cast {
public:
    type_index from_;
    type_index to_;
    bad_any_cast(const type_index &from, const type_index &to)
        : from_(from)
        , to_(to) {
    }
    virtual const char * what() const throw() {
        return "bad_any_cast";
    }
};
template<typename ValueType>
ValueType * any_cast(any *operand) {
    typedef typename any::template holder<ValueType> holder_t;
    return operand &&
        operand->type() == type_id<ValueType>()
        ? &static_cast<holder_t *>(operand->content)->held
        : 0;
}
template<typename ValueType>
inline const ValueType * any_cast(const any *operand) {
    return any_cast<ValueType>(const_cast<any *>(operand));
}
template<typename ValueType>
ValueType any_cast(any & operand) {
    typedef typename std::remove_cvref<ValueType>::type nonref;
    nonref *result = any_cast<nonref>(&operand);
    if (!result)
        throw bad_any_cast(operand.type(), type_id<ValueType>());
    return *result;
}
template<typename ValueType>
inline ValueType any_cast(const any &operand) {
    typedef typename std::remove_cvref<ValueType>::type nonref;
    return any_cast<nonref &>(const_cast<any &>(operand));
}
template<typename FUNC>
inline any any_call(const FUNC &func, const any &arg) {
    using func_t = call_traits<FUNC>;
    using nocvr_argument_type = typename tuple_remove_cvref<typename func_t::argument_type>::type;
    const auto &stdFunc = func_t::to_std_function(func);
    if (!stdFunc)
        return any();
    if (arg.type() == type_id<std::exception_ptr>()) {
        try {
            std::rethrow_exception(any_cast<std::exception_ptr>(arg));
        }
        catch (const any &ex_arg) {
            return any_call_with_ret_t<typename call_traits<FUNC>::result_type, nocvr_argument_type, func_t>::call(stdFunc, ex_arg);
        }
        catch (...) {
        }
    }
    return any_call_with_ret_t<typename call_traits<FUNC>::result_type, nocvr_argument_type, func_t>::call(stdFunc, arg);
}
using pm_any = any;
}
#endif
