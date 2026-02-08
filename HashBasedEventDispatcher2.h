#pragma once

#include "FunctionTraits.h"
#include "ArrayView.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace HB2
{
  // primary template handles types that have no nested ::Base member:
  template<class, class = void>
  struct HasBaseMember : std::false_type
  {
  };

// specialization recognizes types that do have a nested ::Base member:
  template<class T>
  struct HasBaseMember<T, std::void_t<typename T::Base>> : std::true_type
  {
  };

  template<typename T>
  struct TypeHashHolder
  {
    static constexpr size_t i{};
  };

  template<typename T> static constexpr auto TypeHash = &TypeHashHolder<T>::i;

  using Hash = const size_t*;

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
    void invoke(const Event& i_event) const
    {
      func(object, i_event);
    }

    template<auto Method>
    HandlerItem(Class<Method>& i_object, TemplateParameter<Method>):
            object(static_cast<void*>(&i_object)),
            hash(ValueHash<Method>),
            func([](void* object, const Event& event)
                 {
                   (static_cast<Class<Method>*>(object)->*Method)(event);
                 })
    {
    }

    void* object;
    Hash hash;
  private:
    using F = void (*)(void*, const Event& i_event);

    F func;
  };

  struct IInvoker
  {
    virtual bool isEmpty() const = 0;
    virtual void disconnectAll(const void* i_object) = 0;
    virtual ~IInvoker() = 0;
  };

  struct HandlersInfo
  {
    inline void setInvoked(const void* object)
    {
      handlersInfo[object] = true;
    }

    inline bool isInvoked(const void* object) const
    {
      const auto it = handlersInfo.find(object);
      return it != end(handlersInfo) && it->second;
    }

    inline void clearAll()
    {
      for (auto&[_, invoked]: handlersInfo)
      {
        invoked = false;
      }
    }
  private:
    std::unordered_map<const void*, bool> handlersInfo;
  };

  template<typename T>
  struct Invoker : IInvoker
  {
    explicit Invoker(HandlersInfo& i_handlersInfo) : handlersInfo(
            i_handlersInfo)
    {
    }

    template<auto Method>
    void connect(Class<Method>& i_object)
    {
      handlers.push_back(HandlerItem<Argument<Method>>(i_object, TemplateParameter<Method>()));
    }
    
    void invoke(const T& event)
    {
      const auto firstLevel = !isInInvokeProcess;
      isInInvokeProcess = true;
      for (size_t i = 0; i < handlers.size(); ++i)
      {
        const auto& handler = handlers[i];
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

    template<auto Method>
    void disconnect(Class<Method>* object)
    {
      disconnect(object, ValueHash<Method>);
    }

  private:
    void disconnect(const void* object, Hash hash)
    {
      for (auto& handler: handlers)
      {
        if (handler.has_value() &&
            handler->object == object &&
            handler->hash == hash)
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

    std::vector<std::optional<HandlerItem<T>>> handlers;
    HandlersInfo& handlersInfo;
    bool isInInvokeProcess = false;
    bool dirty = false;
  };

  struct InvokerContainer
  {
    template<typename T>
    void invoke(const T& event)
    {
      const auto firstLevel = !isInInvokeProcess;
      isInInvokeProcess = true;
      if (auto* invoker = findInvoker<T>())
      {
        invoker->invoke(event);
      }
      if constexpr (HasBaseMember<T>::value)
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

    template<auto... Methods>
    void connect(Class<Methods...>& i_object)
    {
      (getOrCreateInvoker<Argument<Methods>>().template connect<Methods>(
              i_object), ...);
    }

    template<auto Method>
    void disconnect1(const Class<Method>& i_object)
    {
      if (auto* invoker = findInvoker<Argument<Method>>())
      {
        invoker->disconnect<Method>(&i_object);
      }
    }

    template<auto... Methods>
    void disconnect(const Class<Methods...>& i_object)
    {
      if constexpr(sizeof...(Methods) == 0)
      {
        for (auto&[_, invoker]: invokers)
        {
          invoker->disconnectAll(&i_object);
        }
      }
      else
      {
        (disconnect1<Methods>(i_object), ...);
      }
      dirty = true;
      removeEmpty();
    }

  private:
    inline IInvoker* findInvoker(Hash eventHash) const
    {
      const auto it = invokers.find(eventHash);
      return it == end(invokers) ? nullptr : it->second.get();
    }

    template<typename Event>
    Invoker<Event>* findInvoker() const
    {
      return static_cast<Invoker<Event>*>(findInvoker(TypeHash<Event>));
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

    void removeEmpty();

    std::unordered_map<Hash, std::unique_ptr<IInvoker>> invokers;
    HandlersInfo handlersInfo;
    bool isInInvokeProcess = false;
    bool dirty = false;
  };

}

