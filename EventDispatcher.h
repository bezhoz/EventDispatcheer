#pragma once

struct IHandler;

struct IEvent
{
  virtual ~IEvent() = default;
  virtual void sendTo(IHandler& i_handler) const = 0;
};

struct IHandler
{
  virtual ~IHandler() = default;
  virtual void handle(const IEvent& i_event) = 0;
};

template<typename T>
struct ISingleEventHandler
{
  virtual void handle(const T& i_event) = 0;
};

template<typename T>
struct EventBase : IEvent
{
  using ISEHandler = ISingleEventHandler<T>;

  void sendTo(IHandler& i_handler) const override
  {
    if (auto* eventHandler = dynamic_cast<ISEHandler*>(&i_handler);
            eventHandler != nullptr)
    {
      eventHandler->handle(*static_cast<const T*>(this));
    }
  }
};

template<typename... E>
class HandlerBase : public IHandler, public E::ISEHandler...
{
  void handle(const IEvent& i_event) override
  {
    i_event.sendTo(*this);
  }
};
