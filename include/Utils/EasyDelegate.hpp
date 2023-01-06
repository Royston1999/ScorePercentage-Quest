#pragma once
#include "custom-types/shared/delegate.hpp"

namespace EasyDelegate{
    template <typename T>
    struct function_traits : public function_traits<decltype(&T::operator())>
    {};

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const>
    // we specialize for pointers to member function
    {
        using result_type = ReturnType;
        using arg_tuple = std::tuple<Args...>;
        static constexpr auto arity = sizeof...(Args);
    };

    template <class F, std::size_t ... Is, class T>
    auto lambda_to_func_impl(F f, std::index_sequence<Is...>, T) {
        return std::function<typename T::result_type(std::tuple_element_t<Is, typename T::arg_tuple>...)>(f);
    }

    template <class F>
    auto lambda_to_func(F f) {
        using traits = function_traits<F>;
        return lambda_to_func_impl(f, std::make_index_sequence<traits::arity>{}, traits{});
    }
}

// Takes the delegate type and the lambda callback and then constructs the delegate  
#define MAKEDELEG(delegType, func) custom_types::MakeDelegate<delegType>(EasyDelegate::lambda_to_func(func))

namespace EasyDelegate{
    /**
     * @param F the lambda callback to be used with the delegate.
     * @return delegate of type T
     */
    template <typename T, class F>
    T MakeDelegate(F f){ return MAKEDELEG(T, f); }
}