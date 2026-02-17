#include "HashBasedEventDispatcher4.h"

#include <numeric>

namespace HB4
{

  constexpr bool isBaseOrEqual(const ShortTypeInfo i_base,
                               const ArrayView2<TypeId> i_derived)
  {
    return !i_base.empty() &&
           i_derived.size() >= i_base.depthOfInheritance &&
           i_base.typeId == i_derived[i_base.depthOfInheritance - 1];
  }

  constexpr bool isBaseOrEqual(const ArrayView2<ArrayView2<TypeId>> i_bases,
                               const ArrayView2<TypeId> i_derived)
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
    handlers.erase(std::remove_if(begin(handlers), end(handlers),
                                  [](const auto& handler)
                                  {
                                    return !handler.has_value();
                                  }), end(handlers));
    dirty = false;
  }

  void SimpleInvoker::removeEmpty()
  {
    if (!dirty || isInInvokeProcess > 0)
    {
      return;
    }
    functions.erase(std::remove_if(begin(functions), end(functions),
                                   [](const auto& function)
                                   {
                                     return !function.has_value();
                                   }), end(functions));
    dirty = false;
  }

  void InvokerContainerImpl::updateSimpleInvokers()
  {
    if (simpleInvokersUpdated)
    {
      return;
    }
    simpleInvokers.clear();
    for (const auto&[eventTypeId, invoker]: invokers)
    {
      std::vector<std::pair<size_t, FunctionView>> sortedFunctions;
      for (const auto& handler: invoker.handlers)
      {
        if (handler.has_value())
        {
          sortedFunctions.emplace_back(handler->pos, handler->fv);
//            simpleInvoker.append(handler->fv);
        }
      }
      const auto eventTypeInfo = getTypeInfo(eventTypeId);
      auto baseEventType = eventTypeInfo;
      while (!(--baseEventType).empty())
      {
        if (auto* invoker = findInvoker(baseEventType.back()))
        {
          for (const auto& handler: invoker->handlers)
          {
            if (handler.has_value() &&
                !isBaseOrEqual(handler->notProcessesEvents, eventTypeInfo))
            {
              sortedFunctions.emplace_back(handler->pos, handler->fv);
//              simpleInvoker.append(handler->fv);
            }
          }
        }
      }
      std::sort(begin(sortedFunctions), end(sortedFunctions),
                [](const auto& left, const auto& right)
                {
                  return left.first < right.first;
                });
      auto& simpleInvoker = simpleInvokers[eventTypeId];
      std::transform(begin(sortedFunctions), end(sortedFunctions),
                     std::back_inserter(simpleInvoker.functions),
                     [](const auto& value)
                     {
                       return value.second;
                     });
    }
    simpleInvokersUpdated = true;
  }

  void InvokerContainerImpl::invoke(const void* i_event,
                                    const ArrayView2<TypeId> i_eventType)
  {
    const auto firstLevel = !isInInvokeProcess;
    isInInvokeProcess = true;
    if (firstLevel)
    {
      updateSimpleInvokers();
    }
    if (auto* invoker = findSimpleInvoker(i_eventType.back()))
    {
      invoker->invoke(i_event);
    }
    if (firstLevel)
    {
      isInInvokeProcess = false;
      removeEmpty();
    }
  }

  void InvokerContainerImpl::removeEmpty()
  {
    if (!dirty || isInInvokeProcess)
    {
      return;
    }
    for (auto it = begin(invokers); it != end(invokers);)
    {
      if (it->second.isEmpty())
      {
        it = invokers.erase(it);
        eventTypes.erase(it->first);
      }
      else
      {
        ++it;
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

  size_t InvokerContainerImpl::disconnect(const void* i_object,
                                          const ArrayView2<EventMethodType> i_eventMethodTypes)
  {
    const auto disconnected = std::accumulate(i_eventMethodTypes.begin(),
                                              i_eventMethodTypes.end(), 0,
                                              [i_object, this](
                                                      size_t disconnected,
                                                      const auto eventMethodType)
                                              {
                                                return disconnected +
                                                       disconnect1(i_object,
                                                                   eventMethodType);
                                              });
    dirty = disconnected > 0;
    removeEmpty();
    return disconnected;
  }
}

