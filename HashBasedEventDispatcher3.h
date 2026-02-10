#pragma once

#include "FunctionTraits.h"
#include "ArrayView.h"

#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

namespace HB3
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
  constexpr auto countBaseClasses(const size_t i = 0)
  {
    if constexpr (HasBaseMember<T>::value)
    {
      return countBaseClasses<typename T::Base>(i + 1);
    }
    return i + 1;
  }

  template<typename T>
  struct TypeHashHolder
  {
    static constexpr size_t i{};
  };

  template<typename T> static constexpr auto TypeHash = &TypeHashHolder<T>::i;

  using Hash = const size_t*;

  template<typename T, typename A>
  constexpr auto collectBaseHashesInt(A& hashes, size_t i)
  {
    hashes[--i] = TypeHash<T>; // корень
    if constexpr (HasBaseMember<T>::value)
    {
      collectBaseHashesInt<typename T::Base>(hashes, i);
    }
  }

  template<typename T>
  constexpr auto collectBaseHashes()
  {
    std::array<Hash, countBaseClasses<T>()> hashes{};
    collectBaseHashesInt<T>(hashes, hashes.size());
    return hashes;
  }

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

  struct HandlerItem
  {
    template<auto Method>
    HandlerItem(Class<Method>& i_object, TemplateParameter<Method>):
            object(static_cast<void*>(&i_object)),
            hash(ValueHash<Method>),
            func([](void* object, const void* event)
                 {
                   (static_cast<Class<Method>*>(object)->*Method)(
                           *static_cast<const Argument<Method>*>(event));
                 })
    {
    }

    inline void invoke(const void* i_event) const
    {
      func(object, i_event);
    }

    void* object;
    Hash hash;
    size_t maxAllowedLevelOfInheritanceDepth = std::numeric_limits<size_t>::max();

  private:
    using F = void (*)(void*, const void*);
    F func;
  };

  template<auto ... Methods>
  constexpr bool isSameObjectType()
  {
    return (std::is_same_v<Class<Methods...>, Class<Methods>> && ...);
  }

  template<auto ... Methods>
  struct Register
  {
    static_assert(isSameObjectType<Methods...>());
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

  struct Invoker
  {
    explicit Invoker(HandlersInfo& i_handlersInfo) : handlersInfo(i_handlersInfo)
    {
    }

    inline void connect(const HandlerItem& i_handlerItem)
    {
      handlers.push_back(i_handlerItem);
    }

    void invoke(const void* event, std::size_t initialLevelOfInheritanceDepth = 0);
    size_t disconnectAll(const void* object);
    size_t disconnect(const void* object, const Hash methodHash);

    inline bool isEmpty() const
    {
      return handlers.empty();
    }

  private:
    void removeEmpty();

    std::vector<std::optional<HandlerItem>> handlers;
    HandlersInfo& handlersInfo;
    bool isInInvokeProcess = false;
    bool dirty = false;
  };

  struct EventMethodHashes
  {
    Hash event;
    Hash method;
  };

  struct InvokerContainerImpl
  {
    inline void connect(Hash eventHash, const HandlerItem& handlerItem)
    {
      getOrCreateInvoker(eventHash).connect(handlerItem);
    }
    void invoke(const void* event, const ArrayView2<Hash> eventHashes);
    size_t disconnectAll(const void* i_object);
    size_t disconnect(const void* i_object, const ArrayView2<EventMethodHashes> i_hashes);

  private:
    inline Invoker* findInvoker(const Hash eventHash)
    {
      const auto it = invokers.find(eventHash);
      return it == end(invokers) ? nullptr : &it->second;
    }

    Invoker& getOrCreateInvoker(const Hash eventHash);
    void removeEmpty();

    inline size_t disconnect1(const void* i_object, const EventMethodHashes& hash)
    {
      auto* invoker = findInvoker(hash.event);
      return invoker != nullptr ? invoker->disconnect(i_object, hash.method) : 0;
    }
    
    std::unordered_map<Hash, Invoker> invokers;
    HandlersInfo handlersInfo;
    bool isInInvokeProcess = false;
    bool dirty = false;
  };
  
  struct InvokerContainer
  {
    template<typename Event>
    void invoke(const Event& event)
    {
      static constexpr auto eventHashes = collectBaseHashes<Event>();
      invokerContainerImpl.invoke(&event, eventHashes);
    }

    template<auto ...Methods>
    void connect(Class<Methods...>& i_object, Register<Methods...>)
    {
      connect<Methods...>(i_object);
    }

    template<auto... Methods>
    void connect(Class<Methods...>& i_object)
    {
      (invokerContainerImpl.connect(TypeHash<Argument<Methods>>,
                                    HandlerItem(i_object,
                                                TemplateParameter<Methods>())), ...);
    }

    template<auto... Methods>
    size_t disconnect(const Class<Methods...>& i_object)
    {
      if constexpr(sizeof...(Methods) == 0)
      {
        return invokerContainerImpl.disconnectAll(&i_object);
      }
      else
      {
        return invokerContainerImpl.disconnect(&i_object, {EventMethodHashes{
                TypeHash<Argument<Methods>>, ValueHash<Methods>}...});
      }
    }

  private:
    InvokerContainerImpl invokerContainerImpl;
  };
}

