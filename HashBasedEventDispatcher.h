#pragma once

namespace HB
{
  struct IEvent
  {};

  template<typename BaseClass>
  struct Inherit : BaseClass
  {
    using Base = BaseClass;
  };

  template<typename T>
  static bool isBaseOf(ArrayView2<uint64_t> i_hashes)
  {
    static constexpr auto hashes = collect_base_hashes<T>();
    return hashes.size() <= i_hashes.size() && hashes.back() == i_hashes[hashes.size() - 1];
  }

  struct IHandler
  {
    virtual ~IHandler() = default;
    virtual void handle(const IEvent& i_event, ArrayView2<uint64_t> i_hashes) = 0;

  protected:
    template<auto Method>
    bool handleEvent(const IEvent& i_event, ArrayView2<uint64_t> i_hashes)
    {
      using EventType = Argument<Method>;
      if (isBaseOf<EventType>(i_hashes))
      {
        (static_cast<Class<Method>*>(this)->*Method)(static_cast<const EventType&>(i_event));
        return true;
      }
      return false;
    }

    template<auto... Methods>
    bool handleEventAll(const IEvent& i_event, ArrayView2<uint64_t> i_hashes)
    {
      return  (handleEvent<Methods>(i_event, i_hashes) || ...);
    }
  };

  template<typename T>
  void invoke(IHandler& i_handler, const T& i_event)
  {
    static constexpr auto hashes = collect_base_hashes<T>();
    i_handler.handle(i_event, hashes);
  }}
