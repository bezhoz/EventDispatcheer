#include "HashBasedEventDispatcher2.h"

namespace HB2
{
  IInvoker::~IInvoker()
  {
  }

  void InvokerContainer::removeEmpty()
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

}

