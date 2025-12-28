#pragma once
#ifndef INC_PM_ANY_HPP_
#define INC_PM_ANY_HPP_
#include <vector>
#include <exception>
#include <utility>
#include <type_traits>
#include <tuple>
#include "extensions.hpp"
#include "call_traits.hpp"
namespace promise {
class any;
template<typename ValueType>
inline ValueType any_cast(const any &operand);
class any {
public:
    any()
        : content(0) {
    }
    template<typename ValueType>
    any(const ValueType &value)
        : content(new holder<typename std::remove_cvref<ValueType>::type>(value)) {
    }
    template<typename RET, typename ...ARG>
    any(RET value(ARG...))
        : content(new holder<RET (*)(ARG...)>(value)) {
    }
    any(const any &other)
        : content(other.content ? other.content->clone() : 0) {
    }
    any(any &&other)
        : content(other.content) {
        other.content = 0;
    }
    template<typename ValueType>
    any(ValueType &&value
        , typename std::enable_if<!std::is_same<any &, ValueType>::value>::type* = nullptr
        , typename std::enable_if<!std::is_const<ValueType>::value>::type* = nullptr)
        : content(new holder<typename std::remove_cvref<ValueType>::type>(static_cast<ValueType &&>(value))) {
    }
    ~any() {
        if (content != nullptr) {
            delete(content);
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
        virtual ~placeholder() {
        }
    public:
        virtual type_index type() const = 0;
        virtual placeholder *clone() const = 0;
        virtual any call(const any &arg) const = 0;
    };
    template<typename ValueType>
    class holder : public placeholder {
    public:
        holder(const ValueType & value)
            : held(value) {
        }
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
template<typename RET, typename NOCVR_ARGS, typename FUNC>
struct any_call_t;
template<typename RET, typename ...NOCVR_ARGS, typename FUNC>
struct any_call_t<RET, std::tuple<NOCVR_ARGS...>, FUNC> {
    static inline RET call(const typename FUNC::fun_type &func, const any &arg) {
        return call(func, arg, std::make_index_sequence<sizeof...(NOCVR_ARGS)>());
    }
    template<size_t ...I>
    static inline RET call(const typename FUNC::fun_type &func, const any &arg, const std::index_sequence<I...> &) {
        using nocvr_argument_type = std::tuple<NOCVR_ARGS...>;
        using any_arguemnt_type = std::vector<any>;
        const any_arguemnt_type &args = (arg.type() != type_id<any_arguemnt_type>()
            ? any_arguemnt_type{ arg }
            : any_cast<any_arguemnt_type &>(arg));
        if(args.size() < sizeof...(NOCVR_ARGS))
            throw bad_any_cast(arg.type(), type_id<nocvr_argument_type>());
        return func(any_cast<typename std::tuple_element<I, nocvr_argument_type>::type &>(args[I])...);
    }
};
template<typename RET, typename NOCVR_ARG, typename FUNC>
struct any_call_t<RET, std::tuple<NOCVR_ARG>, FUNC> {
    static inline RET call(const typename FUNC::fun_type &func, const any &arg) {
        using nocvr_argument_type = std::tuple<NOCVR_ARG>;
        using any_arguemnt_type = std::vector<any>;
        if (arg.type() == type_id<std::exception_ptr>()) {
            try {
                std::rethrow_exception(any_cast<std::exception_ptr>(arg));
            }
            catch (const NOCVR_ARG &ex_arg) {
                return func(const_cast<NOCVR_ARG &>(ex_arg));
            }
        }
        if (type_id<NOCVR_ARG>() == type_id<any_arguemnt_type>()) {
            return func(any_cast<NOCVR_ARG &>(arg));
        }
        const any_arguemnt_type &args = (arg.type() != type_id<any_arguemnt_type>()
            ? any_arguemnt_type{ arg }
            : any_cast<any_arguemnt_type &>(arg));
        if(args.size() < 1)
            throw bad_any_cast(arg.type(), type_id<nocvr_argument_type>());
        return func(any_cast<NOCVR_ARG &>(args.front()));
    }
};
template<typename RET, typename FUNC>
struct any_call_t<RET, std::tuple<any>, FUNC> {
    static inline RET call(const typename FUNC::fun_type &func, const any &arg) {
        using any_arguemnt_type = std::vector<any>;
        if (arg.type() != type_id<any_arguemnt_type>())
            return (func(const_cast<any &>(arg)));
        any_arguemnt_type &args = any_cast<any_arguemnt_type &>(arg);
        if (args.size() == 0) {
            any empty;
            return (func(empty));
        }
        else if(args.size() == 1)
            return (func(args.front()));
        else
            return (func(const_cast<any &>(arg)));
    }
};
template<typename RET, typename NOCVR_ARGS, typename FUNC>
struct any_call_with_ret_t {
    static inline any call(const typename FUNC::fun_type &func, const any &arg) {
        return any_call_t<RET, NOCVR_ARGS, FUNC>::call(func, arg);
    }
};
template<typename NOCVR_ARGS, typename FUNC>
struct any_call_with_ret_t<void, NOCVR_ARGS, FUNC> {
    static inline any call(const typename FUNC::fun_type &func, const any &arg) {
        any_call_t<void, NOCVR_ARGS, FUNC>::call(func, arg);
        return any();
    }
};
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