#pragma once

#include <type_traits>

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

template<auto Method>
using Class = typename FunctionTraits<decltype(Method)>::Class;
