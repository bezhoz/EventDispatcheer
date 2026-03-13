#pragma once

#include <type_traits>

#include <tuple>

template<typename T>
struct FunctionTraits;

template <typename C, typename R, typename A>
struct FunctionTraits<R(C::*)(A)>
{
  using Class = C;
  using Argument = typename std::remove_cv_t<std::remove_reference_t<A>>;
};

template<auto Method>
using Argument = typename FunctionTraits<decltype(Method)>::Argument;

template<typename T,  typename ... TS>
struct First
{
  using Type = T;
};

template<auto... Methods>
using Class = typename FunctionTraits<typename First<decltype(Methods)...>::Type>::Class;

template<typename TupleT, typename Fn>
void for_each_tuple2(TupleT&& tp, Fn&& fn)
{
  std::apply([&fn](auto&& ...args)
             {
               (fn(std::forward<decltype(args)>(args)), ...);
             }, std::forward<TupleT>(tp));
}

template<typename T, typename R, typename F>
auto accumulateT(const T& tuple, R initial, F op)
{
  auto result = std::move(initial);
  const auto mapOp = [&result, &op](auto&& value)
  {
    result = op(result, value);
  };
  for_each_tuple2(tuple, mapOp);
  return result;
}

