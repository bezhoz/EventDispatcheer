#include "ArrayView.h"
#include "CollectBaseHashes.h"
#include "EventDispatcher.h"
#include "EventDispatcher2.h"
#include "FunctionTraits.h"
#include "HashBasedEventDispatcher.h"
#include "HashBasedEventDispatcher2.h"
#include "HashBasedEventDispatcher3.h"

#include <iostream>
#include <type_traits>
#include <typeinfo>

class Event1 : public EventBase<Event1>{};
class Event2 : public EventBase<Event2>{};
class Event3 : public EventBase<Event3>{};
class Event4 : public Event3{};

class Handler : public HandlerBase<Event1, Event2>
{
    void handle(const Event1& i_event) override
    {
        std::cout << "Handler::handle(Event1)" << std::endl;
    }
    void handle(const Event2& i_event) override
    {
        std::cout << "Handler::handle(Event2)" << std::endl;
    }
};

struct Handler2Impl
{
  void call1(const Event1& i_event)
  {
      std::cout << "Handler2Impl::call1(Event1)" << std::endl;
  }

  void call2(const Event2& i_event)
  {
      std::cout << "Handler2Impl::call2(Event2)" << std::endl;
  }

  void call3(const Event3& i_event)
  {
      std::cout << "Handler2Impl::call3(Event3)" << std::endl;
  }
};

using Handler2 = HandlerBase2<&Handler2Impl::call1, &Handler2Impl::call2, &Handler2Impl::call3>;

struct Handler3Impl
{
  void doSomething(const Event1& i_event)
  {
      std::cout << "Handler3Impl::doSomething(Event1)" << std::endl;
  }

  void handleEvent2(const Event2& i_event)
  {
      std::cout << "Handler3Impl::handleEvent2(Event2)" << std::endl;
  }

  void asdfrf(const Event3& i_event)
  {
      std::cout << "Handler3Impl::asdfrf(Event3)" << std::endl;
  }
};

using Handler3 = HandlerBase2<&Handler3Impl::doSomething, &Handler3Impl::handleEvent2, &Handler3Impl::asdfrf>;

void printHashes(const ArrayView2<uint64_t>& i_hashes)
{
  std::cout << "count = " << i_hashes.size() << std::endl;
  for (int i = 0; i < i_hashes.size(); ++i)
  {
    std::cout << "hash[" << i << "]=" << i_hashes[i] << std::endl;
  }
}

template<typename T>
void printHashes()
{
  printHashes(collect_base_hashes<T>());
}

namespace HB
{
  struct Event1 : public IEvent
  {
    using Base = IEvent;
  };

  struct Event2 : public IEvent
  {
    using Base = IEvent;
  };

  struct Event3 : public IEvent
  {
    using Base = IEvent;
  };

  struct Event4 : public Event3
  {
    using Base = Event3;
  };

  struct Event1_1 : Inherit<IEvent>{};
  struct Event2_1 : Inherit<Event1_1>{};
  struct Event3_1 : Inherit<Event2_1>{};
  struct Event4_1 : Inherit<Event3_1>{};

  struct HandlerBase : IHandler
  {
    void handle(const IEvent& i_event, ArrayView2<uint64_t> i_hashes) override
    {
      handleEventAll<&HandlerBase::onEvent2_1, &HandlerBase::handle1>(i_event, i_hashes);
    }
    
    void handle1(const Event1_1& i_event)
    {
      std::cout << "HB::HandlerBase::handle(Event1_1)" << std::endl;
    }

    void onEvent2_1(const Event2_1& i_event)
    {
      std::cout << "HB::HandlerBase::onEvent2_1(Event2_1)" << std::endl;
    }
  };
};

namespace HB2
{
  struct EventBase{};
  struct Event1: EventBase
  {
    using Base = EventBase;
  };

  struct Event2: EventBase
  {
    using Base = EventBase;
  };

  struct Event1_1: Event1
  {
    using Base = Event1;
  };

  struct Event2_1: Event2
  {
    using Base = Event2;
  };

  struct Handler1
  {
    void handle1(const Event1& event)
    {
      std::cout << "Handler1::handle1::event = " << &event << std::endl;
    }
    void handle2(const Event2& event)
    {
      std::cout << "Handler1::handle2::event = " << &event << std::endl;
    }
  };

  struct Handler2
  {
    void onEvent1(const Event1& event)
    {
      std::cout << "Handler2::onEvent1::event = " << &event << std::endl;
    }
    void onEvent2(const Event2& event)
    {
      std::cout << "Handler2::onEvent2::event = " << &event << std::endl;
    }
    void onEvent1_1(const Event1_1& event)
    {
      std::cout << "Handler2::onEvent1_1::event = " << &event << std::endl;
    }
    void onEvent2_1(const Event2_1& event)
    {
      std::cout << "Handler2::onEvent2_1::event = " << &event << std::endl;
    }
  };
};

int main()
{
  std::cout << "1 and 2 event system \n";
    Event1 e1;
    Event2 e2;
    Event3 e3;
    Event4 e4;

    Handler i1;
    Handler2 i2;
    Handler3 i3;

    IHandler* pi1 = &i1;
    IHandler* pi2 = &i2;
    IHandler* pi3 = &i3;

    IEvent* pe1 = &e1;
    IEvent* pe2 = &e2;
    IEvent* pe3 = &e3;
    IEvent* pe4 = &e4;

    for (auto* handler: {pi1, pi2, pi3})
    {
        std::cout << "----Handler=" << handler << std::endl;
        for (const auto* event: {pe1, pe2, pe3, pe4})
        {
            std::cout << "Event=" << event << " handle: ";
            handler->handle(*event);
            std::cout << std::endl;
        }
    }

  std::cout << "\nhashes based 1 event system \n";
  printHashes<HB::Event4>();
  printHashes<HB::Event4_1>();

  HB::HandlerBase handlerBase;
  HB::IHandler& hb = handlerBase;
  HB::Event1_1 e1_1;
  HB::Event2_1 e2_1;
  HB::Event3_1 e3_1;
  HB::Event4_1 e4_1;

  std::cout << "e1_1\n";
  HB::invoke(hb, e1_1);
  printHashes<HB::Event1_1>();

  std::cout << "e2_1\n";
  HB::invoke(hb, e2_1);
  printHashes<HB::Event2_1>();

  std::cout << "e3_1\n";
  HB::invoke(hb, e3_1);
  printHashes<HB::Event3_1>();

  std::cout << "e4_1\n";
  HB::invoke(hb, e4_1);
  printHashes<HB::Event4_1>();

  std::cout << "\nhashes based 2 event system \n";
  {
    HB2::Handler1 h1;
    HB2::Handler2 h2;
    HB2::InvokerContainer ic;
    ic.connect<&HB2::Handler1::handle1,
               &HB2::Handler1::handle2>(h1);
    ic.connect<&HB2::Handler2::onEvent1,
               &HB2::Handler2::onEvent2,
               &HB2::Handler2::onEvent1_1,
               &HB2::Handler2::onEvent2_1>(h2);
    std::cout << "Event1----------\n";
    ic.invoke(HB2::Event1{});
    std::cout << "Event2----------\n";
    ic.invoke(HB2::Event2{});
    std::cout << "Event1_1----------\n";
    ic.invoke(HB2::Event1_1{});
    std::cout << "Event2_1----------\n";
    ic.invoke(HB2::Event2_1{});
  }
  std::cout << "\nHashes based 3 event system \n";
  {
    HB2::Handler1 h1;
    HB2::Handler2 h2;
    HB3::InvokerContainer ic;
    ic.connect<&HB2::Handler1::handle1,
            &HB2::Handler1::handle2>(h1);
    ic.connect<&HB2::Handler2::onEvent1,
            &HB2::Handler2::onEvent2,
            &HB2::Handler2::onEvent1_1,
            &HB2::Handler2::onEvent2_1>(h2);
    std::cout << "Event1----------\n";
    ic.invoke(HB2::Event1{});
    std::cout << "Event2----------\n";
    ic.invoke(HB2::Event2{});
    std::cout << "Event1_1----------\n";
    ic.invoke(HB2::Event1_1{});
    std::cout << "Event2_1----------\n";
    ic.invoke(HB2::Event2_1{});
  }
}
