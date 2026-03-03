#include "HashBasedEventDispatcher4.h"

#include <functional>
#include <tuple>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

TEST_CASE("Hash based event dispatcher 4 simple test ")
{
  struct TestEvent
  {
    int value = 0;
  };

  struct TestHandler
  {
    void onEvent1(const TestEvent& event)
    {
      value += event.value;
    }
    int value = 0;
  };
  TestHandler testHandler;

  HB4::InvokerContainer ic;

  ic.connect<&TestHandler::onEvent1>(testHandler);
  ic.invoke(TestEvent{5});
  CHECK(testHandler.value == 5);
}

template<typename T, typename R, typename F, size_t I = 0>
R accumulateTuple(const T& tuple, R initial, F op)
{
  if constexpr(I < std::tuple_size_v<T>)
  {
    return accumulateTuple<T, R, F, I + 1>(tuple, op(std::move(initial), std::get<I>(tuple)), op);
  }
  return initial;
}

namespace doctest
{
  template<typename... Ts>
  struct StringMaker<std::tuple<Ts...>>
  {
    static String convert(const std::tuple<Ts...>& value)
    {
      const auto result = accumulateTuple(value, std::string("{"),[prefix = ""](std::string&& initial , auto value) mutable
      {
        initial += prefix + std::to_string(value);
        prefix = ", ";
        return initial;
      });
      return (result + "}").c_str();
    }
  };

  template<typename T>
  struct StringMaker<std::vector<T>>
  {
    static String convert(const std::vector<T>& vector)
    {
      String result;
      String coma;
      for (const auto& value: vector)
      {
        result += coma;
        result += StringMaker<T>::convert(value);
        coma = ", ";
      }
      result = "[" + result + "]";
      return result;
    }
  };
}

template<typename T>
size_t getTypeHashCode(const T& object)
{
  return typeid(object).hash_code();
}

TEST_CASE("Hash based event dispatcher 4 complex test ")
{
  struct EventBase
  {
    int value = 0;
    EventBase(int i_value): value(i_value){}
    
    virtual int getType() const
    {
      return 0;
    }
  };

  struct Event1 : EventBase
  {
    using Base = EventBase;
    Event1(int value): Base(value){}
    int getType() const override
    {
      return 1;
    }
  };

  struct Event2 : EventBase
  {
    using Base = EventBase;
    Event2(int value): Base(value){}
    int getType() const override
    {
      return 2;
    }
  };

  struct Event1_1 : Event1
  {
    using Base = Event1;
    Event1_1(int value): Base(value){}
    int getType() const override
    {
      return 3;
    }
  };

  struct Event2_1 : Event2
  {
    using Base = Event2;
    Event2_1(int value): Base(value){}
    int getType() const override
    {
      return 4;
    }
  };

  using LogFunction = std::function<void(int, const EventBase&)>;

  struct Handler1
  {
    LogFunction logf;

    Handler1(LogFunction i_logFunction = [](int, const EventBase&)
    {
    }) : logf(i_logFunction)
    {
    }

    void handle1(const Event1& event)
    {
      logf(1, event);
    }

    void handle2(const Event2& event)
    {
      logf(2, event);
    }
  };

  struct Handler2
  {
    LogFunction logf;

    Handler2(LogFunction i_logFunction = [](int, const EventBase&)
    {
    }) : logf(i_logFunction)
    {
    }

    void onEvent1(const Event1& event)
    {
      logf(3, event);
    }

    void onEvent2(const Event2& event)
    {
      logf(4, event);
    }

    void onEvent1_1(const Event1_1& event)
    {
      logf(5, event);
    }

    void onEvent2_1(const Event2_1& event)
    {
      logf(6, event);
    }
  };

  std::vector<std::tuple<int, int, size_t>> invoker_log;
  const auto invoker_log_push_back = [&invoker_log](int handlerId,  const EventBase& i_event)
  {
//    invoker_log.emplace_back(handlerId, i_event.value, i_event.getType());
    invoker_log.emplace_back(handlerId, i_event.value, getTypeHashCode(i_event));
  };
  Handler1 h1{invoker_log_push_back};
  Handler2 h2{invoker_log_push_back};
  HB4::InvokerContainer ic;

  HB4::Register<&Handler1::handle1, &Handler1::handle2> r1;
  HB4::Register<&Handler2::onEvent1, &Handler2::onEvent2, &Handler2::onEvent1_1, &Handler2::onEvent2_1>
          r2;

  ic.connect(h1, r1);
  ic.connect(h2, r2);

  SUBCASE("Event1:1")
  {
    const Event1 e{1};
    const auto typeHashCode = getTypeHashCode(e);
    ic.invoke(e);
    REQUIRE_EQ(invoker_log.size(), 2);
    CHECK_EQ(invoker_log[0], std::tuple{1, 1, typeHashCode});
    CHECK_EQ(invoker_log[1], std::tuple{3, 1, typeHashCode});
  }
  SUBCASE("Event2:2")
  {
    const Event2 e{2};
    const auto typeHashCode = getTypeHashCode(e);
    ic.invoke(e);
    REQUIRE_EQ(invoker_log.size(), 2);
    CHECK_EQ(invoker_log[0], std::tuple{2, 2, typeHashCode});
    CHECK_EQ(invoker_log[1], std::tuple{4, 2, typeHashCode});
  }
  SUBCASE("Event1_1:3")
  {
    const Event1_1 e{3};
    const auto typeHashCode = getTypeHashCode(e);
    ic.invoke(e);
    REQUIRE_EQ(invoker_log.size(), 2);
    CHECK_EQ(invoker_log[0], std::tuple{1, 3, typeHashCode});
    CHECK_EQ(invoker_log[1], std::tuple{5, 3, typeHashCode});
  }
  SUBCASE("Event2_1:4")
  {
    const Event2_1 e{4};
    const auto typeHashCode = getTypeHashCode(e);
    ic.invoke(e);
    REQUIRE_EQ(invoker_log.size(), 2);
    CHECK_EQ(invoker_log[0], std::tuple{2, 4, typeHashCode});
    CHECK_EQ(invoker_log[1], std::tuple{6, 4, typeHashCode});
    CHECK_EQ(invoker_log, std::vector{std::tuple{2, 4, typeHashCode}, std::tuple{6, 4, size_t(7)}});
  }
}