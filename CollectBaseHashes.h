#pragma once

#include "TypeHash.h"

#include <array>
#include <type_traits>

// primary template handles types that have no nested ::Base member:
template<class, class = void>
struct has_base_member : std::false_type {};

// specialization recognizes types that do have a nested ::Base member:
template<class T>
struct has_base_member<T, std::void_t<typename T::Base>> : std::true_type {};

template<typename T>
constexpr auto count_base_hashes(const size_t i = 0)
{
  if constexpr (has_base_member<T>::value)
  {
    return count_base_hashes<typename T::Base>(i + 1);
  }
  return i + 1;
}

template<typename T, typename A>
constexpr auto collect_base_hashes_int(A& hashes, size_t i = 0)
{
  hashes[i++] = type_hash<T>(); // корень
  if constexpr (has_base_member<T>::value)
  {
    collect_base_hashes_int<typename T::Base>(hashes, i);
  }
}

template<typename T>
constexpr auto collect_base_hashes()
{
  std::array<uint64_t, count_base_hashes<T>()> hashes{};
  collect_base_hashes_int<T>(hashes);
  return hashes;
}

