#include "HashBasedEventDispatcher4.h"

#include "struct_util.h"

#include <functional>
#include <tuple>
#include <utility>
#include <numeric>

#include <cxxabi.h>

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

template<typename, typename = void> constexpr bool
        is_to_string_convertible = false;

template<typename T> constexpr bool is_to_string_convertible
        <T, std::void_t<decltype(std::to_string(std::declval<T>()))>> = true;

template<typename, typename = void> constexpr bool
        is_string_initializer = false;

template<typename T> constexpr bool is_string_initializer
        <T, std::void_t<decltype(std::string(std::declval<T>()))>> = true;

template<typename, typename = void> constexpr bool
        is_stringstream_suitable = false;

template<typename T> constexpr bool is_stringstream_suitable
        <T, std::void_t<decltype(std::declval<std::ostringstream>() << std::declval<T>())>> = true;

std::string demangle(std::string_view typeName)
{
  int status;
  char* realname = abi::__cxa_demangle(typeName.data(), NULL, NULL, &status);
  std::string result(realname);
  std::free(realname);
  return result;
}

namespace doctest
{
  template<typename... Ts>
  struct StringMaker<std::tuple<Ts...>>
  {
    static String convert(const std::tuple<Ts...>& value)
    {
      return std::apply([](auto&& ...args)
                        {
                          const auto to_string = [](auto&& value) -> std::string
                          {
                            using ValueType = decltype(value);
                            if constexpr (is_to_string_convertible<ValueType>)
                            {
                              return std::to_string(value);
                            }
                            else if constexpr (is_string_initializer<ValueType>)
                            {
                              return value;
                            }
                            else if constexpr (is_stringstream_suitable<ValueType>)
                            {
                              std::ostringstream ss;
                              ss << value;
                              return ss.str();
                            }
                            else
                            {
                              return ".";
                            }
                          };
                          auto prefix = "";
                          return "{" +
                                 ((std::exchange(prefix, ", ") +
                                   to_string(args)) + ...) +
                                 "}";
                        }, value).c_str();
    }
  };

  template<typename T>
  struct StringMaker<std::vector<T>>
  {
    static String convert(const std::vector<T>& vector)
    {
      String result = "[";
      String coma;
      for (const auto& value: vector)
      {
        result += std::exchange(coma, ", ") + StringMaker<T>::convert(value);
      }
      return result + "]";
    }
  };
}

template<typename T>
size_t getTypeHashCode(const T& object)
{
  return typeid(object).hash_code();
}

using EventProcessingInfo = std::tuple<HB4::Hash, int, size_t>;
using EventsProcessingInfo = std::vector<EventProcessingInfo>;

struct EventProcessingLogger
{
  template<typename E>
  void log(HB4::Hash handlerId, const E& i_event)
  {
    eventsLog.emplace_back(handlerId, i_event.value, getTypeHashCode(i_event));
  };

  template<auto Method, typename E>
  void log(const E& i_event)
  {
    eventsLog.emplace_back(HB4::ValueHash<Method>, i_event.value, getTypeHashCode(i_event));
  };
  EventsProcessingInfo eventsLog;
};

TEST_CASE("Hash based event dispatcher 4 complex test ")
{
  struct EventBase
  {
    int value = 0;
    EventBase(int i_value): value(i_value){}
    virtual ~EventBase(){};
  };

  struct Event1 : EventBase
  {
    using Base = EventBase;
    Event1(int value): Base(value){}
  };

  struct Event2 : EventBase
  {
    using Base = EventBase;
    Event2(int value): Base(value){}
  };

  struct Event1_1 : Event1
  {
    using Base = Event1;
    Event1_1(int value): Base(value){}
  };

  struct Event2_1 : Event2
  {
    using Base = Event2;
    Event2_1(int value): Base(value){}
  };
  
  struct Handler1
  {
    EventProcessingLogger& logger;

    Handler1(EventProcessingLogger& i_logger) : logger(i_logger)
    {
    }

    void handle1(const Event1& event)
    {
      logger.log<&Handler1::handle1>(event);
    }

    void handle3(const Event1& event)
    {
      logger.log<&Handler1::handle3>(event);
    }

    void handle2(const Event2& event)
    {
      logger.log<&Handler1::handle2>(event);
    }
  };

  struct Handler2
  {
    EventProcessingLogger& logger;

    Handler2(EventProcessingLogger& i_logger) : logger(i_logger)
    {
    }

    void onEvent1(const Event1& event)
    {
      logger.log<&Handler2::onEvent1>(event);
//      std::cout << "Имя функции: " << __FUNCTION__ << std::endl;
//      std::cout << "Полная сигнатура: " << __PRETTY_FUNCTION__ << std::endl;
//      std::cout << "Имя функции: " << __func__ << std::endl;
//      std::cout << "Имя типа: " << demangle(typeid(*this).name()) << std::endl;
    }

    void onEvent2(const Event2& event)
    {
      logger.log<&Handler2::onEvent2>(event);
    }

    void onEvent1_1(const Event1_1& event)
    {
      logger.log<&Handler2::onEvent1_1>(event);
    }

    void onEvent2_1(const Event2_1& event)
    {
      logger.log<&Handler2::onEvent2_1>(event);
    }
  };

  EventProcessingLogger logger;
  Handler1 h1{logger};
  Handler2 h2{logger};
  HB4::InvokerContainer ic;

  HB4::Register<&Handler1::handle1, &Handler1::handle2> r1;
  HB4::Register<&Handler2::onEvent1, &Handler2::onEvent2, &Handler2::onEvent1_1, &Handler2::onEvent2_1>
          r2;

  ic.connect(h1, r1);
  ic.connect(h2, r2);

  SUBCASE("Event1:1")
  {
    const Event1 e{1};
    ic.invoke(e);

    EventProcessingLogger expected;
    expected.log<&Handler1::handle1>(e);
    expected.log<&Handler2::onEvent1>(e);

    CHECK_EQ(logger.eventsLog, expected.eventsLog);
  }
  SUBCASE("Event2:2")
  {
    const Event2 e{2};
    ic.invoke(e);

    EventProcessingLogger expected;
    expected.log<&Handler1::handle2>(e);
    expected.log<&Handler2::onEvent2>(e);
    CHECK_EQ(logger.eventsLog, expected.eventsLog);
  }
  SUBCASE("Event1_1:3")
  {
    const Event1_1 e{3};
    ic.invoke(e);

    EventProcessingLogger expected;
    expected.log<&Handler1::handle1>(e);
    expected.log<&Handler2::onEvent1_1>(e);
    CHECK_EQ(logger.eventsLog, expected.eventsLog);
  }
  SUBCASE("Event2_1:4")
  {
    const Event2_1 e{4};
    ic.invoke(e);

    EventProcessingLogger expected;
    expected.log<&Handler1::handle2>(e);
    expected.log<&Handler2::onEvent2_1>(e);
    CHECK_EQ(logger.eventsLog, expected.eventsLog);
  }
}