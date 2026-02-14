#include "HashBasedEventDispatcher3.h"

#include <numeric>

namespace HB3
{

  constexpr bool isBaseOrEqual(const ShortTypeInfo i_base,
                               const ArrayView2<Hash> i_derived)
  {
    return !i_base.empty() &&
           i_derived.size() >= i_base.depthOfInheritance &&
           i_base.typeId == i_derived[i_base.depthOfInheritance - 1];
  }

  constexpr bool isBaseOrEqual(const ArrayView2<ArrayView2<Hash>> i_bases,
                               const ArrayView2<Hash> i_derived)
  {
    return std::any_of(i_bases.begin(), i_bases.end(),
                       [i_derived](const auto base)
                       {
                         return isBaseOrEqual(base, i_derived);
                       });
  }

  void Invoker::invoke(const void* event)
  {
    invokeIf(event, [](const auto&)
    {
      return true;
    });
  }

  void Invoker::invoke(const void* event, ArrayView2<TypeId> i_eventTypes)
  {
    invokeIf(event, [i_eventTypes](const auto& handler)
    {
      return !isBaseOrEqual(handler.notProcessesEvents, i_eventTypes);
    });
  }

  size_t Invoker::disconnect(const void* object)
  {
    return removeIf(object, [](const auto&)
    {
      return true;
    });
  }

  size_t Invoker::disconnect(const void* object, const MethodId methodId)
  {
    return removeIf(object, [methodId](const auto& handler)
    {
      return handler.methodId == methodId;
    });
  }

  void Invoker::removeEmpty()
  {
    if (!dirty || isInInvokeProcess > 0)
    {
      return;
    }
    handlers.erase(
            std::remove_if(begin(handlers), end(handlers), [](const auto& handler)
            {
              return !handler.has_value();
            }), end(handlers));
    dirty = false;
  }

  void InvokerContainerImpl::invoke(const void* i_event, const ArrayView2<TypeId> i_eventType)
  {
    const auto firstLevel = !isInInvokeProcess;
    isInInvokeProcess = true;
    if (auto* invoker = findInvoker(i_eventType.back()))
    {
      invoker->invoke(i_event);
    }
    auto baseEventType = i_eventType;
    while (!(--baseEventType).empty())
    {
      if (auto* invoker = findInvoker(baseEventType.back()))
      {
        invoker->invoke(i_event, i_eventType);
      }
    }
    if (firstLevel)
    {
      isInInvokeProcess = false;
      removeEmpty();
    }
  }

  void InvokerContainerImpl::removeEmpty()
  {
    if (dirty && !isInInvokeProcess)
    {
      for (auto it = begin(invokers); it != end(invokers);)
      {
        it = it->second.isEmpty() ? invokers.erase(it) : next(it);
      }
    }
    dirty = false;
  }

  size_t InvokerContainerImpl::disconnect(const void* i_object)
  {
    size_t disconnected = 0;
    for (auto&[_, invoker]: invokers)
    {
      disconnected += invoker.disconnect(i_object);
    }
    dirty = disconnected > 0;
    removeEmpty();
    return disconnected;
  }

  size_t InvokerContainerImpl::disconnect(const void* i_object, const ArrayView2<EventMethodType> i_eventMethodTypes)
  {
    const auto disconnected =
            std::accumulate(i_eventMethodTypes.begin(), i_eventMethodTypes.end(), 0,
                            [i_object, this](size_t disconnected,
                                             const auto eventMethodType)
                            {
                              return disconnected + disconnect1(i_object, eventMethodType);
                            });
    dirty = disconnected > 0;
    removeEmpty();
    return disconnected;
  }
}

