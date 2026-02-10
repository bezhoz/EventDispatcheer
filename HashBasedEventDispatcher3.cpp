#include "HashBasedEventDispatcher3.h"

namespace HB3
{
  void Invoker::invoke(const void* event, std::size_t initialLevelOfInheritanceDepth)
  {
    const auto firstLevel = !isInInvokeProcess;
    isInInvokeProcess = true;
    // нельзя использовать range for
    // при увеличении длины массива handlers он может быть перенесен в другое
    // место в памяти и все итераторы станут невалидными
    for (size_t i = 0; i < handlers.size(); ++i)
    {
      const auto& handler = handlers[i];
      if (handler.has_value() && !handlersInfo.isInvoked(handler->object) &&
          handler->maxAllowedLevelOfInheritanceDepth > initialLevelOfInheritanceDepth)
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

  size_t Invoker::disconnectAll(const void* object)
  {
    size_t disconnected = 0;
    for (auto& handler: handlers)
    {
      if (handler.has_value() && handler->object == object)
      {
        handler.reset();
        ++disconnected;
      }
    }
    dirty = disconnected > 0;
    removeEmpty();
    return disconnected;
  }

  size_t Invoker::disconnect(const void* object, const Hash methodHash)
  {
    size_t disconnected = 0;
    for (auto& handler: handlers)
    {
      if (handler.has_value() &&
          handler->object == object &&
          handler->hash == methodHash)
      {
        handler.reset();
        ++disconnected;
      }
    }
    dirty = disconnected > 0;
    removeEmpty();
    return disconnected;
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

  void InvokerContainerImpl::invoke(const void* event, const ArrayView2<Hash> eventHashes)
  {
    const auto firstLevel = !isInInvokeProcess;
    isInInvokeProcess = true;
    const auto initialLevelOfInherinaceDepth = eventHashes.size();
    for (auto i = eventHashes.size(); i > 0; --i)
    {
      if (auto* invoker = findInvoker(eventHashes[i - 1]))
      {
        invoker->invoke(event, initialLevelOfInherinaceDepth);
      }
    }
    if (firstLevel)
    {
      isInInvokeProcess = false;
      handlersInfo.clearAll();
      removeEmpty();
    }
  }

  Invoker& InvokerContainerImpl::getOrCreateInvoker(const Hash eventHash)
  {
    const auto findedIt = invokers.find(eventHash);
    const auto it =
            findedIt != end(invokers) ? findedIt : invokers.emplace(eventHash,
                                                                    Invoker(handlersInfo))
                                                           .first;
    return it->second;
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

  size_t InvokerContainerImpl::disconnectAll(const void* i_object)
  {
    size_t disconnected = 0;
    for (auto&[_, invoker]: invokers)
    {
      disconnected += invoker.disconnectAll(i_object);
    }
    dirty = disconnected > 0;
    removeEmpty();
    return disconnected;
  }

  size_t InvokerContainerImpl::disconnect(const void* i_object, const ArrayView2<EventMethodHashes> i_hashes)
  {
    size_t disconnected = 0;
    for (int i = 0; i < i_hashes.size(); ++i)
    {
      disconnected += disconnect1(i_object, i_hashes[i]);
    }
    dirty = disconnected > 0;
    removeEmpty();
    return disconnected;
  }
}

