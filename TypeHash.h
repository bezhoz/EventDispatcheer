#pragma once

#include <string_view>
// Compile-time hash of a type using the compiler-provided function signature
// string. This is constexpr and unique per type within a given
// compiler/standard library combination.
constexpr std::size_t fnv1a_64(std::string_view sv) noexcept
{
  std::size_t hash = 14695981039346656037ull;
  for (unsigned char c: sv)
  {
    hash ^= static_cast<std::size_t>(c);
    hash *= 1099511628211ull;
  }
  return hash;
}

// Returns a constexpr hash for the type T.
// Note: the exact value is implementation-defined (depends on compiler's
// signature spelling) but stable within a toolchain and TU set.
template<typename T>
constexpr std::size_t type_hash() noexcept
{
#if defined(__clang__) || defined(__GNUC__)
  return fnv1a_64({__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 1});
#elif defined(_MSC_VER)
  return fnv1a_64({__FUNCSIG__, sizeof(__FUNCSIG__) - 1});
#else
      return fnv1a_64({__func__, sizeof(__func__) - 1});
#endif
}
