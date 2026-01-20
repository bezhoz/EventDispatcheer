#include <iostream>
#include <type_traits>
#include <typeinfo>

class IHandler;

class IEvent
{
public:
    virtual ~IEvent() = default;
    virtual void sendTo(IHandler* i_handler) = 0;
};

class IHandler
{
public:
    virtual ~IHandler() = default;
    virtual void handle(IEvent* i_event) = 0;
};

template<typename T>
struct ISingleEventHandler
{
    virtual void handle(T i_event) = 0;
};

template<typename T>
class EventBase : public IEvent
{
public:
    using ISEHandler = ISingleEventHandler<T*>;
    void sendTo(IHandler* i_handler) override
    {
        if (auto* eventHandler = dynamic_cast<ISEHandler*>(i_handler); eventHandler !=
                                                                       nullptr)
        {
            eventHandler->handle(static_cast<T*>(this));
        }
    }
};

template<typename... E>
class HandlerBase : public IHandler, public E::ISEHandler...
{
public:
    void handle(IEvent* i_event) override
    {
        i_event->sendTo(this);
    }
};

template<typename T>
struct FunctionTraits;

template <typename C, typename R, typename A>
struct FunctionTraits<R(C::*)(A)>
{
    using ClassType = C;
    using ArgumentType = A;
    using ReaultType = R;
};

template<auto...Method>
class HandlerBase3;

template<auto Method>
class HandlerBase3<Method>: public FunctionTraits<decltype(Method)>::ClassType,
                            public ISingleEventHandler<typename FunctionTraits<decltype(Method)>::ArgumentType>,
                            public HandlerBase<>
{
    using MethodType = decltype(Method);
    using MethodTraits = FunctionTraits<MethodType>;
    using ClassType = typename MethodTraits::ClassType;
    using ArgumentType = typename MethodTraits::ArgumentType;

    void handle(ArgumentType i_event) override
    {
        (static_cast<ClassType*>(this)->*Method)(i_event);
    }
};

template<auto Method1, auto Method2, auto... Methods>
class HandlerBase3<Method1, Method2, Methods...>:
        public ISingleEventHandler<typename FunctionTraits<decltype(Method1)>::ArgumentType>,
        public HandlerBase3<Method2, Methods...>
{
    using MethodType = decltype(Method1);
    using MethodTraits = FunctionTraits<MethodType>;
    using ClassType = typename MethodTraits::ClassType;
    using ArgumentType = typename MethodTraits::ArgumentType;

    void handle(ArgumentType i_event) override
    {
        (static_cast<ClassType*>(this)->*Method1)(i_event);
    }
};

class Event1 : public EventBase<Event1>{};
class Event2 : public EventBase<Event2>{};
class Event3 : public EventBase<Event3>{};

class Handler2Impl
{
public:
    void call1(Event1* i_event)
    {
        std::cout << "Handler2Impl::call1(Event1)" << std::endl;
    }

    void call2(Event2* i_event)
    {
        std::cout << "Handler2Impl::call2(Event2)" << std::endl;
    }
    void call3(Event3* i_event)
    {
        std::cout << "Handler2Impl::call3(Event3)" << std::endl;
    }
};

using Invoker3 = HandlerBase3<&Handler2Impl::call1, &Handler2Impl::call2, &Handler2Impl::call3>;

class Invoker : public HandlerBase<Event1, Event2>
{
public:
    void handle(Event1* i_event) override
    {
        std::cout << "Invoker::handle(Event1)" << std::endl;
    }
    void handle(Event2* i_event) override
    {
        std::cout << "Invoker::handle(Event2)" << std::endl;
    }
};

int main()
{
    Event1 e1;
    Event2 e2;
    Event3 e3;

    Invoker i1;
    Invoker3 i3;

    IHandler* pi1 = &i1;
    IHandler* pi3 = &i3;

    IEvent* pe1 = &e1;
    IEvent* pe2 = &e2;
    IEvent* pe3 = &e3;

    for (auto* handler : {pi1, pi3})
    {
        std::cout << "----Handler=" << handler << std::endl;
        for (auto* event : {pe1, pe2, pe3})
        {
            std::cout << "Event=" << event << " handle: ";
            handler->handle(event);
            std::cout << std::endl;
        }
    }
    return 0;
}
