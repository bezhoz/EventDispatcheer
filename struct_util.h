#include <iostream>
#include <typeinfo>
#include <string>

struct A
{
  int i;
  double x;
  std::string text;
};

struct AC
{
  int v1;
  double v2;
  std::string v3;
};

struct A0
{};

struct A1
{
  int v;
};

struct A2
{
  int x;
  double y;
};


// constexpr A2 a2_3{{}, {}, {}};
//constexpr A2 a2_2{{}, {}};
//constexpr A2 a2_1{{}};
//constexpr A2 a2_0{};
//
//template<typename S, typename... Ts>
//auto makeS(Ts... values) -> decltype(S{values...})
//{
//  return S{values...};
//}
//
//const A aaa = makeS<A>(1, 4.0, std::string());

struct any_type
{
  template<typename T>
  operator T()
  {
    return T{};
  }
};

//int x{any_type()};

struct any_type_2 : any_type
{
  any_type_2(int)
  {}
};

//template<typename S, int... I>
//auto makeSSS() -> decltype(S{any_type_2(I)...})
//{
//  return S{};
//}

//auto aaa0 = makeSSS<A0>();
//auto aaa1 = makeSSS<A1, 1>();
//auto aaa2 = makeSSS<A2, 1, 2>();

template<typename S, size_t... I>
constexpr auto makeSSSI(std::index_sequence<I...>) -> decltype(S{any_type_2(I)...})
{
  return S{};
}

//auto aaai0 = makeSSSI<A0>(std::make_index_sequence<0>());
//auto aaai1 = makeSSSI<A1>(std::make_index_sequence<1>());
//auto aaai2 = makeSSSI<A2>(std::make_index_sequence<2>());

template<typename T, size_t I, typename = void>
struct can_created_with_n_parameters : std::false_type {};

template<typename T, size_t I>
struct can_created_with_n_parameters<T, I, std::void_t<decltype(makeSSSI<T>(std::make_index_sequence<I>()))>>
        : std::true_type {};

//const auto can_a0_0 = can_created_with_n_parameters<A0, 1>::value;
//const auto can_a0_1 = can_created_with_n_parameters<A1, 1>::value;
//const auto can_a0_2 = can_created_with_n_parameters<A2, 1>::value;

template<typename T, size_t I = 0>
constexpr size_t ssize()
{
  if constexpr (can_created_with_n_parameters<T, I>::value)
  {
    return ssize<T, I + 1>();
  }
  else
  {
    return I - 1;
  }
};

template<typename T>
constexpr size_t get_size(const T&)
{
  return ssize<T>();
}

template<size_t I, typename T, typename... Ts>
auto& select(T& value, Ts&... values)
{
  static_assert(I <= sizeof... (Ts));
  if constexpr (I == 0)
  {
    return value;
  }
  else
  {
    return select<I - 1>(values...);
  }
}

template<typename T>
auto to_tuple(T& t)
{
  return 0;
}

template<size_t I, typename T>
auto& get(T& t)
{
  constexpr auto structSize = ssize<T>();
  if constexpr (structSize == 1 && I == 0)
  {
    auto& [x] = t;
    return x;
  }
  else if constexpr (structSize == 2)
  {
    auto& [x1, x2] = t;
    return select<I>(x1, x2);
  }
  else if constexpr (structSize == 3)
  {
    auto& [x1, x2, x3] = t;
    return select<I>(x1, x2, x3);
  }
  else
  {
    static_assert(false);
  }
}

//int main()
//{
//  A a{1, 2.0, "asdfsfd"};
//  A1 a1{4};
//  const auto& [x, y, z] = a;
//  std::cout << ssize<A0>() << std::endl;
//  std::cout << ssize<A1>() << std::endl;
//  std::cout << ssize<A2>() << std::endl;
//  std::cout << get_size(a) << std::endl;
//  std::cout << get<2>(a) << std::endl;
//  get<2>(a) = "asdfasdfasdfsdfasdfsadf";
//  std::cout << get<2>(a) << std::endl;
//  std::cout << reinterpret_cast<AC*>(&a)->v1 << std::endl;
//  std::cout << reinterpret_cast<AC*>(&a)->v2 << std::endl;
//  std::cout << reinterpret_cast<AC*>(&a)->v3 << std::endl;
//}