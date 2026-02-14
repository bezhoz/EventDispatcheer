#pragma once

#include <array>
#include <vector>

template<typename T>
struct ArrayView
{
  using Fsize = std::size_t (*)(const void*);
  using Fitem = const T& (*)(const void*, std::size_t);

  template<std::size_t S>
  ArrayView(const std::array<T, S>& i_array): d_array(&i_array)
  {
    d_size = [](const void* p_array)
    {
      return static_cast<const std::array<T, S>*>(p_array)->size();
    };
    d_item = [](const void* p_array, std::size_t index) -> const T&
    {
      return (*static_cast<const std::array<T, S>*>(p_array))[index];
    };
  }

  std::size_t size() const
  {
    return d_size(d_array);
  }

  const T& operator[](std::size_t i_pos) const
  {
    return d_item(d_array, i_pos);
  }
private:
  Fsize d_size;
  Fitem d_item;
  const void* d_array;
};

template<typename T>
struct ArrayView2
{
  template<std::size_t S>
  constexpr ArrayView2(const std::array<T, S>& i_array): d_array(i_array.data()),
                                              d_size(i_array.size())
  {}

  constexpr ArrayView2(const std::vector<T>& i_array): d_array(i_array.data()),
                                              d_size(i_array.size())
  {}

  constexpr ArrayView2(): d_array(nullptr), d_size(0)
  {}

  constexpr std::size_t size() const
  {
    return d_size;
  }

  constexpr bool empty() const
  {
    return d_size == 0;
  }

  constexpr const T& back() const
  {
    return d_array[size() - 1];
  }

  constexpr const T* begin() const
  {
    return d_array;
  }

  constexpr const T* end() const
  {
    return d_array + size();
  }

  constexpr const T& operator[](std::size_t i_pos) const
  {
    return d_array[i_pos];
  }

  constexpr ArrayView2 decrease_size_by1()
  {
    ArrayView2 result;
    result.d_array = d_array;
    result.d_size = d_size > 0 ? d_size - 1 : 0;
    return result;
  }

  ArrayView2& operator--()
  {
    if (d_size > 0)
    {
      --d_size;
    }
    return *this;
  }

private:
  std::size_t d_size;
  const T* d_array;
};

