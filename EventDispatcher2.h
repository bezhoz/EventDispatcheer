#pragma once

#include "EventDispatcher.h"
#include "FunctionTraits.h"

template<auto...Method>
class HandlerBase2;

template<auto Method>
class HandlerBase2<Method>
        : protected Class<Method>, public ISingleEventHandler<Argument<Method>>,
          public HandlerBase<>
{
  void handle(const Argument<Method>& i_event) override
  {
    (static_cast<Class<Method>*>(this)->*Method)(i_event);
  }
};

template<auto Method1, auto Method2, auto... Methods>
class HandlerBase2<Method1, Method2, Methods...>:
        public ISingleEventHandler<Argument<Method1>>,
        public HandlerBase2<Method2, Methods...>
{
  void handle(const Argument<Method1>& i_event) override
  {
    (static_cast<Class<Method1>*>(this)->*Method1)(i_event);
  }
};
