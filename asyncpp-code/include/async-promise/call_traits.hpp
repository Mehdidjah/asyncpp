#pragma once
#ifndef INC_CALL_TRAITS_HPP_
#define INC_CALL_TRAITS_HPP_
#include <type_traits>
#include <functional>
#include <tuple>
namespace promise {
template<typename T,
         bool is_basic_type = (std::is_void<T>::value
                            || std::is_fundamental<T>::value
                            || (std::is_pointer<T>::value && !std::is_function<typename std::remove_pointer<T>::type>::value)
                            || std::is_union<T>::value
                            || std::is_enum<T>::value
                            || std::is_array<T>::value) && !std::is_function<T>::value>
struct call_traits_impl;
template<typename T>
struct has_operator_parentheses {
private:
    struct Fallback { void operator()(); };
    struct Derived : T, Fallback { };
    template<typename U, U> struct Check;
    template<typename>
    static std::true_type test(...);
    template<typename C>
    static std::false_type test(Check<void (Fallback::*)(), &C::operator()>*);
public:
    typedef decltype(test<Derived>(nullptr)) type;
};
template<typename T, bool has_operator_parentheses>
struct operator_parentheses_traits {
    typedef std::function<void(void)> fun_type;
    typedef void result_type;
    typedef std::tuple<> argument_type;
    static fun_type to_std_function(const T &t) {
        (void)t;
        return nullptr;
    }
};
template<typename FUNCTOR>
struct operator_parentheses_traits<FUNCTOR, true> {
private:
    typedef decltype(&FUNCTOR::operator()) callable_type;
    typedef call_traits_impl<callable_type> the_type;
public:
    typedef typename the_type::fun_type fun_type;
    typedef typename the_type::result_type result_type;
    typedef typename the_type::argument_type argument_type;
    static fun_type to_std_function(const FUNCTOR &functor) {
        if (is_std_function<FUNCTOR>::value) {
            return functor;
        }
        else {
            return call_traits_impl<callable_type>::to_std_function(const_cast<FUNCTOR &>(functor), &FUNCTOR::operator());
        }
    }
};
template<typename T>
struct call_traits_impl<T, true> {
    static const bool is_callable = false;
    typedef std::function<void(void)> fun_type;
    typedef void result_type;
    typedef std::tuple<> argument_type;
    static fun_type to_std_function(const T &t) {
        (void)t;
        return nullptr;
    }
};
template<typename RET, class T, typename ...ARG>
struct call_traits_impl<RET(T::*)(ARG...), false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;
    static fun_type to_std_function(T &obj, RET(T::*func)(ARG...)) {
        return [obj, func](ARG...arg) -> RET {
            return (const_cast<typename std::remove_const<T>::type &>(obj).*func)(arg...);
        };
    }
    static fun_type to_std_function(RET(T::*)(ARG...)) {
        return nullptr;
    }
};
template<typename RET, class T, typename ...ARG>
struct call_traits_impl<RET(T::*)(ARG...) const, false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;
    static fun_type to_std_function(T &obj, RET(T:: *func)(ARG...) const) {
        return [obj, func](ARG...arg) -> RET {
            return (obj.*func)(arg...);
        };
    }
    static fun_type to_std_function(RET(T:: *)(ARG...)) {
        return nullptr;
    }
};
template<typename RET, typename ...ARG>
struct call_traits_impl<RET(*)(ARG...), false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;
    static fun_type to_std_function(RET(*func)(ARG...)) {
        return func;
    }
};
template<typename RET, typename ...ARG>
struct call_traits_impl<RET(ARG...), false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;
    static fun_type to_std_function(RET(*func)(ARG...)) {
        return func;
    }
};
template<typename T>
struct call_traits_impl<T, false> {
    static const bool is_callable = has_operator_parentheses<T>::type::value;
private:
    typedef operator_parentheses_traits<T, is_callable> the_type;
public:
    typedef typename the_type::fun_type fun_type;
    typedef typename the_type::result_type result_type;
    typedef typename the_type::argument_type argument_type;
    static fun_type to_std_function(const T &t) {
        return the_type::to_std_function(t);
    }
};
template<typename T>
struct call_traits {
private:
    using RawT = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
    typedef call_traits_impl<RawT> the_type;
public:
    static const bool is_callable = the_type::is_callable;
    typedef typename the_type::fun_type fun_type;
    typedef typename the_type::result_type result_type;
    typedef typename the_type::argument_type argument_type;
    static fun_type to_std_function(const T &t) {
        return the_type::to_std_function(t);
    }
};
}
#endif