#pragma once

#include <array>

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
  ArrayView2(const std::array<T, S>& i_array): d_array(i_array.data()),
                                              d_size(i_array.size())
  {}

  std::size_t size() const
  {
    return d_size;
  }

  const T& operator[](std::size_t i_pos) const
  {
    return d_array[i_pos];
  }
private:
  const std::size_t d_size;
  const T* d_array;
};
