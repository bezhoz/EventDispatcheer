#include "HashBasedEventDispatcher4.h"

#include <iostream>
#include <type_traits>
#include <typeinfo>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("Hash based event dispatcher 4 simple test ") {
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
