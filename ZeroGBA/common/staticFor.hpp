#pragma once
#include <utility>

template<typename T, T Begin, class Func, T ...Is>
constexpr void staticFor(Func&& f, std::integer_sequence<T, Is...>) {
    (f(std::integral_constant<T, Begin + Is>{}), ...);
}

template<typename T, T Begin, T End, class Func>
constexpr void staticFor(Func&& f) {
    staticFor<T, Begin>(static_cast<Func&&>(f), std::make_integer_sequence<T, End - Begin>{});
}