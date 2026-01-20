#include <iostream>
#include <type_traits>
#include <typeinfo>

class IHandler;

class IEvent
{
public:
    virtual ~IEvent() = default;
    virtual void sendTo(IHandler& i_handler) const = 0;
};

class IHandler
{
public:
    virtual ~IHandler() = default;
    virtual void handle(const IEvent& i_event) = 0;
};

template<typename T>
struct ISingleEventHandler
{
    virtual void handle(const T& i_event) = 0;
};

template<typename T>
class EventBase : public IEvent
{
public:
    using ISEHandler = ISingleEventHandler<T>;
    void sendTo(IHandler& i_handler) const override
    {
        if (auto* eventHandler = dynamic_cast<ISEHandler*>(&i_handler); eventHandler !=
                                                                       nullptr)
        {
            eventHandler->handle(*static_cast<const T*>(this));
        }
    }
};

template<typename... E>
class HandlerBase : public IHandler, public E::ISEHandler...
{
public:
    void handle(const IEvent& i_event) override
    {
        i_event.sendTo(*this);
    }
};

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

template<auto...Method>
class HandlerBase2;

template<auto Method>
class HandlerBase2<Method>: public Class<Method>,
                            public ISingleEventHandler<Argument<Method>>,
                            public HandlerBase<>
{
    void handle(const Argument<Method>& i_event) override
    {
        (static_cast<Class<Method>*>(this)->*Method)(i_event);
    }
};

template<auto Method1, auto Method2, auto... Methods>
class HandlerBase2<Method1, Method2, Methods...>:
        public ISingleEventHandler<Argument<Method1>>,
        public HandlerBase2<Method2, Methods...>
{
    void handle(const Argument<Method1>& i_event) override
    {
        (static_cast<Class<Method1>*>(this)->*Method1)(i_event);
    }
};

class Event1 : public EventBase<Event1>{};
class Event2 : public EventBase<Event2>{};
class Event3 : public EventBase<Event3>{};

class Handler : public HandlerBase<Event1, Event2>
{
public:
    void handle(const Event1& i_event) override
    {
        std::cout << "Handler::handle(Event1)" << std::endl;
    }
    void handle(const Event2& i_event) override
    {
        std::cout << "Handler::handle(Event2)" << std::endl;
    }
};

class Handler2Impl
{
public:
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

int main()
{
    Event1 e1;
    Event2 e2;
    Event3 e3;

    Handler i1;
    Handler2 i2;

    IHandler* pi1 = &i1;
    IHandler* pi3 = &i2;

    IEvent* pe1 = &e1;
    IEvent* pe2 = &e2;
    IEvent* pe3 = &e3;

    for (auto* handler : {pi1, pi3})
    {
        std::cout << "----Handler=" << handler << std::endl;
        for (auto* event : {pe1, pe2, pe3})
        {
            std::cout << "Event=" << event << " handle: ";
            handler->handle(*event);
            std::cout << std::endl;
        }
    }
    return 0;
}
