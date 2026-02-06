#pragma once
#include "CollectBaseHashes.h"
#include "FunctionTraits.h"

#include <unordered_map>
#include <vector>
#include <memory>

namespace HB2
{
  using Hash = const size_t*;

  template<typename T>
  struct HashHolder
  {
    static constexpr size_t i{};
  };

  template<typename T>
  static constexpr auto TypeHash = &HashHolder<T>::i;

  template<typename Event>
  struct HandlerItem
  {
    using F = void (*)(void*, const Event&);

    void invoke(const Event& i_event)
    {
      d_func(d_object, i_event);
    }

    template<auto Method>
    static HandlerItem<Event> create(Class<Method>& i_object)
    {
      HandlerItem<Event> result;
      result.d_object = static_cast<void*>(&i_object);
      result.d_func = [](void* object, const Event& event)
      {
        (static_cast<Class<Method>*>(object)->*Method)(event);
      };
      return result;
    }

    F d_func;
    void* d_object;
  };

  struct IInvoker
  {
    virtual ~IInvoker() = 0;
  };

  IInvoker::~IInvoker()
  {}

  struct HandlersInfo
  {
    void setInvoked(const void* object)
    {
      handlersInfo[object] = true;
    }

    bool isInvoked(const void* object) const
    {
      auto it = handlersInfo.find(object);
      return it != handlersInfo.end() && it->second;
    }

    void clearAll()
    {
      for (auto& [object, invoked] : handlersInfo)
      {
        invoked = false;
      }
    }

    std::unordered_map<const void*, bool> handlersInfo;
  };

  template<typename T>
  struct Invoker : IInvoker
  {
    explicit Invoker(HandlersInfo& i_handlersInfo) : handlersInfo(i_handlersInfo)
    {}

    void invoke(const T& event)
    {
      for (auto& handler : eventHandlers)
      {
        if (!handlersInfo.isInvoked(handler.d_object))
        {
          handler.invoke(event);
          handlersInfo.setInvoked(handler.d_object);
        }
      }
    }

    template<auto Method>
    void connect(Class<Method>& i_object)
    {
      auto handlerItem = HandlerItem<T>::template create<Method>(i_object);
      eventHandlers.push_back(handlerItem);
    }
    std::vector<HandlerItem<T>> eventHandlers;
    HandlersInfo& handlersInfo;
  };

  struct InvokerContainer
  {
    template<typename Event>
    Invoker<Event>* findInvoker()
    {
      static constexpr auto event_hash = TypeHash<Event>;
      if (auto it = invokers.find(event_hash); it != invokers.end())
      {
        return static_cast<Invoker<Event>*>(it->second.get());
      }
      return nullptr;
    }

    template<typename T>
    void invoke(const T& event)
    {
      if (auto* invoker = findInvoker<T>())
      {
        invoker->invoke(event);
      }
      if constexpr (has_base_member<T>::value)
      {
        invoke<typename T::Base>(event);
      }
      handlersInfo.clearAll();
    }

    template<typename Event>
    Invoker<Event>& getOrCreateInvoker()
    {
      static constexpr auto event_hash = TypeHash<Event>;
      if (invokers.find(event_hash) == invokers.end())
      {
        invokers[event_hash] = std::make_unique<Invoker<Event>>(handlersInfo);
      }
      return *static_cast<Invoker<Event>*>(invokers[event_hash].get());
    }

    template<auto Method>
    void connect(Class<Method>& i_object)
    {
      getOrCreateInvoker<Argument<Method>>().template connect<Method>(i_object);
    }

    std::unordered_map<Hash, std::unique_ptr<IInvoker>> invokers;
    HandlersInfo handlersInfo;
  };
}

