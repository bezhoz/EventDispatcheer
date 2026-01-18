#include <iostream>
#include <type_traits>
#include <typeinfo>

class IInvoker;

class IEvent
{
public:
    virtual ~IEvent() = default;
    virtual void invoke(IInvoker* i_invoker) = 0;
};

class IInvoker
{
public:
    virtual ~IInvoker() = default;
    virtual void invoke(IEvent* i_event) = 0;
};

template<typename T>
struct IEventInvoker
{
    virtual void call(T i_event) = 0;
};

template<typename T>
class IEventBase : public IEvent
{
public:
    using IEInvoker = IEventInvoker<T*>;
    void invoke(IInvoker* i_invoker) override
    {
        if (auto* eventInvoker = dynamic_cast<IEInvoker*>(i_invoker); eventInvoker !=
                                                                          nullptr)
        {
            eventInvoker->call(static_cast<T*>(this));
        }
    }
};

template<typename... E>
class InvokerBase : public IInvoker, public E::IEInvoker...
{
public:
    void invoke(IEvent* i_event) override
    {
        i_event->invoke(this);
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

template<class T>
struct remove_cvpref
{
    using t1 = std::remove_reference_t<T>;
    using t2 = std::remove_pointer_t<t1>;
    using type = std::remove_cv_t<t2>;
};

template<auto Method>
class EventInvokerBase2: public FunctionTraits<decltype(Method)>::ClassType, public IEventInvoker<typename FunctionTraits<decltype(Method)>::ArgumentType>
{
    using MethodType = decltype(Method);
    using MethodTraits = FunctionTraits<MethodType>;
    using ClassType = typename MethodTraits::ClassType;
    using ArgumentType = typename MethodTraits::ArgumentType;

    void call(ArgumentType i_event) override
    {
        (static_cast<ClassType*>(this)->*Method)(i_event);
    }
};

class A{
public:
    class AA
    {
    public:
        void print()
        {
            std::cout << "A::AA" << std::endl;
        }
    };
};
class B{
public:
    class AA
    {
    public:
        void print()
        {
            std::cout << "B::AA" << std::endl;
        }
    };
};
class C{
public:
    class AA
    {
    public:
        void print()
        {
            std::cout << "C::AA" << std::endl;
        }

    };
};

template<typename... Ts>
class X;

template<>
class X<>{};

template<typename T>
class X<T> : public T, public T::AA
{};

template<typename T1, typename T2, typename... Ts>
class X<T1, T2, Ts...> : public T1, public X<T2, Ts...>
{};

[[maybe_unused]] X<A, B, C> x;

template<auto...Method>
class EventInvokerBase3;

template<auto Method>
class EventInvokerBase3<Method>: public FunctionTraits<decltype(Method)>::ClassType,
        public IEventInvoker<typename FunctionTraits<decltype(Method)>::ArgumentType>,
                public InvokerBase<>
{
    using MethodType = decltype(Method);
    using MethodTraits = FunctionTraits<MethodType>;
    using ClassType = typename MethodTraits::ClassType;
    using ArgumentType = typename MethodTraits::ArgumentType;

    void call(ArgumentType i_event) override
    {
        (static_cast<ClassType*>(this)->*Method)(i_event);
    }
};

template<auto Method1, auto Method2, auto... Methods>
class EventInvokerBase3<Method1, Method2, Methods...>: public IEventInvoker<typename FunctionTraits<decltype(Method1)>::ArgumentType>, public EventInvokerBase3<Method2, Methods...>
{
    using MethodType = decltype(Method1);
    using MethodTraits = FunctionTraits<MethodType>;
    using ClassType = typename MethodTraits::ClassType;
    using ArgumentType = typename MethodTraits::ArgumentType;

    void call(ArgumentType i_event) override
    {
        (static_cast<ClassType*>(this)->*Method1)(i_event);
    }
};

class Event1 : public IEventBase<Event1>{};
class Event2 : public IEventBase<Event2>{};
class Event3 : public IEventBase<Event3>{};

class Invoker2Impl
{
public:
    void call1(Event1* i_event)
    {
        std::cout << "Invoker2Impl::call1(Event1)" << std::endl;
    }

    void call2(Event2* i_event)
    {
        std::cout << "Invoker2Impl::call2(Event2)" << std::endl;
    }
    void call3(Event3* i_event)
    {
        std::cout << "Invoker2Impl::call3(Event3)" << std::endl;
    }
};

using Invoker3 = EventInvokerBase3<&Invoker2Impl::call1, &Invoker2Impl::call2, &Invoker2Impl::call3>;

class Invoker2 : public InvokerBase<>, public EventInvokerBase2<&Invoker2Impl::call1>
{};


class Invoker : public InvokerBase<Event1, Event2>
{
public:
    void call(Event1* i_event) override
    {
        std::cout << "Invoker::call(Event1)" << std::endl;
    }
    void call(Event2* i_event) override
    {
        std::cout << "Invoker::call(Event2)" << std::endl;
    }
};

int main()
{
    const auto name = typeid(decltype(x)).name();
    std::cout << name << std::endl;
    x.print();

    Event1 e1;
    Event2 e2;
    Event3 e3;

    Invoker i1;
    Invoker2 i2;
    Invoker3 i3;

    IInvoker* pi1 = &i1;
    IInvoker* pi2 = &i2;
    IInvoker* pi3 = &i3;

    IEvent* pe1 = &e1;
    IEvent* pe2 = &e2;
    IEvent* pe3 = &e3;

    for (auto* invoker : {pi1, pi2, pi3})
    {
        std::cout << "----Invoker=" << invoker << std::endl;
        for (auto* event : {pe1, pe2, pe3})
        {
            std::cout << "Event=" << event << " call: ";
            invoker->invoke(event);
            std::cout << std::endl;
        }
    }
    return 0;
}
