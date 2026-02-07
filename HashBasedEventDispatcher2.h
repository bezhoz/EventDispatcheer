#pragma once

#include "CollectBaseHashes.h"
#include "FunctionTraits.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace HB2
{
  using Hash = const size_t*;

  template<typename T>
  struct TypeHashHolder
  {
    static constexpr size_t i{};
  };

  template<typename T> static constexpr auto TypeHash = &TypeHashHolder<T>::i;

  template<auto T>
  struct ValueHashHolder
  {
    static constexpr size_t i{};
  };

  template<auto T> static constexpr auto ValueHash = &ValueHashHolder<T>::i;

  template<auto Value>
  struct TemplateParameter
  {
  };

  template<typename Event>
  struct HandlerItem
  {
    using F = void (*)(void*, const Event&);

    void invoke(const Event& i_event) const
    {
      func(object, i_event);
    }

    template<auto Method>
    HandlerItem(Class<Method>& i_object, TemplateParameter<Method>):
            func([](void* object, const Event& event)
                 {
                   (static_cast<Class<Method>*>(object)->*Method)(event);
                 }), object(static_cast<void*>(&i_object)),
            hash(ValueHash<Method>)
    {
    }

    F func;
    void* object;
    Hash hash;
  };

  struct IInvoker
  {
    virtual bool isEmpty() const = 0;
    virtual void disconnectAll(const void* i_object) = 0;
    virtual ~IInvoker() = 0;
  };

  IInvoker::~IInvoker()
  {
  }

  struct HandlersInfo
  {
    void setInvoked(const void* object)
    {
      handlersInfo[object] = true;
    }

    bool isInvoked(const void* object) const
    {
      const auto it = handlersInfo.find(object);
      return it != end(handlersInfo) && it->second;
    }

    void clearAll()
    {
      for (auto&[_, invoked]: handlersInfo)
      {
        invoked = false;
      }
    }

    std::unordered_map<const void*, bool> handlersInfo;
  };

  template<typename T>
  struct Invoker : IInvoker
  {
    explicit Invoker(HandlersInfo& i_handlersInfo) : handlersInfo(
            i_handlersInfo)
    {
    }

    void invoke(const T& event)
    {
      const auto firstLevel = !isInInvokeProcess;
      isInInvokeProcess = true;
      for (auto& handler: handlers)
      {
        if (handler.has_value() && !handlersInfo.isInvoked(handler->object))
        {
          handler->invoke(event);
          handlersInfo.setInvoked(handler->object);
        }
      }
      if (firstLevel)
      {
        isInInvokeProcess = false;
        removeEmpty();
      }
    }

    bool isEmpty() const override
    {
      return handlers.empty();
    }

    void disconnectAll(const void* object) override
    {
      for (auto& handler: handlers)
      {
        if (handler.has_value() && handler->object == object)
        {
          handler.reset();
          dirty = true;
        }
      }
      removeEmpty();
    }

    template<typename MethodType>
    void disconnect(const void* object, MethodType method)
    {
      for (auto& handler: handlers)
      {
        if (handler.has_value() &&
            handler->object == object &&
            handler->hash == ValueHash<method>)
        {
          handler.reset();
          dirty = true;
        }
      }
      removeEmpty();
    }

    void removeEmpty()
    {
      if (!dirty || isInInvokeProcess > 0)
      {
        return;
      }
      handlers.erase(
              std::remove_if(begin(handlers), end(handlers), [](auto& handler)
              {
                return !handler.has_value();
              }), end(handlers));
      dirty = false;
    }

    template<auto Method>
    void connect(Class<Method>& i_object)
    {
      handlers.push_back(HandlerItem<T>(i_object, TemplateParameter<Method>()));
    }

    std::vector<std::optional<HandlerItem<T>>> handlers;
    HandlersInfo& handlersInfo;
    bool isInInvokeProcess = false;
    bool dirty = false;
  };

  struct InvokerContainer
  {
    template<typename Event>
    Invoker<Event>* findInvoker() const
    {
      static constexpr auto event_hash = TypeHash<Event>;
      const auto it = invokers.find(event_hash);
      return it == end(invokers)
             ? nullptr
             : static_cast<Invoker<Event>*>(it->second.get());
    }

    template<typename T>
    void invoke(const T& event)
    {
      const auto firstLevel = !isInInvokeProcess;
      isInInvokeProcess = true;
      if (auto* invoker = findInvoker<T>())
      {
        invoker->invoke(event);
      }
      if constexpr (has_base_member<T>::value)
      {
        invoke<typename T::Base>(event);
      }
      if (firstLevel)
      {
        isInInvokeProcess = false;
        handlersInfo.clearAll();
        removeEmpty();
      }
    }

    template<typename Event>
    Invoker<Event>& getOrCreateInvoker()
    {
      static constexpr auto event_hash = TypeHash<Event>;
      const auto findedIt = invokers.find(event_hash);
      const auto it = findedIt != end(invokers) ? findedIt : invokers.emplace(
              event_hash, std::make_unique<Invoker<Event>>(handlersInfo)).first;
      return static_cast<Invoker<Event>&>(*(it->second));
    }

    template<auto... Methods>
    void connect(Class<Methods...>& i_object)
    {
      (getOrCreateInvoker<Argument<Methods>>().template connect<Methods>(
              i_object), ...);
    }

    template<auto... Methods>
    void disconnect(Class<Methods...>& i_object)
    {
      if constexpr(sizeof...(Methods) == 0)
      {
        for (auto&[_, invoker]: invokers)
        {
          invoker->disconnectAll(i_object);
        }
      }
      else
      {
        (getOrCreateInvoker<Argument<Methods>>().disconnect(i_object, Methods), ...);
      }
      dirty = true;
      removeEmpty();
    }

    void removeEmpty()
    {
      if (dirty && !isInInvokeProcess)
      {
        for (auto it = begin(invokers); it != end(invokers);)
        {
          it = it->second->isEmpty() ? invokers.erase(it) : next(it);
        }
      }
      dirty = false;
    }

    std::unordered_map<Hash, std::unique_ptr<IInvoker>> invokers;
    HandlersInfo handlersInfo;
    bool isInInvokeProcess = false;
    bool dirty = false;
  };
}

