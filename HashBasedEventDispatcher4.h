#pragma once

#include "FunctionTraits.h"
#include "ArrayView.h"

#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

namespace HB4
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

  using TypeId = Hash;
  using MethodId = Hash;

  struct ShortTypeInfo
  {
    constexpr ShortTypeInfo(ArrayView2<TypeId> i_TypeInfo) : typeId(
            i_TypeInfo.back()), depthOfInheritance(i_TypeInfo.size())
    {
    }

    constexpr inline bool empty() const
    {
      return depthOfInheritance == 0;
    }

    TypeId typeId;
    size_t depthOfInheritance;
  };

  struct FunctionView
  {
    template<auto Method>
    FunctionView(Class<Method>& i_object, TemplateParameter<Method>):
            object(static_cast<void*>(&i_object)),
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

    inline const void* getObject() const
    {
      return object;
    }

  private:
    using F = void (*)(void*, const void*);
    void* object;
    F func;
  };

  template<typename T>
  struct Numbered
  {
    Numbered(const size_t i_pos, const T& i_value): pos(i_pos), value(i_value)
    {};
    
    size_t pos;
    std::optional<T> value;
  };

  using NumberedFunctionView = Numbered<FunctionView>;

  struct Handler
  {
    template<auto Method>
    Handler(Class<Method>& i_object, TemplateParameter<Method> method):
            fv(i_object, method), methodId(ValueHash<Method>)
    {
    }

    inline const void* getObject() const
    {
      return fv.getObject();
    }

    MethodId methodId;
    std::vector<ArrayView2<TypeId>> notProcessesEvents;

    FunctionView fv;
    size_t pos;
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

  struct Invoker
  {
    explicit Invoker()
    {
    }

    inline void append(const Handler& i_handlerItem)
    {
      handlers.push_back(i_handlerItem);
    }

    inline bool isObjectExists(const void* i_object) const
    {
      return std::find_if(begin(handlers), end(handlers),
                          [i_object](const auto& i_handler)
                          {
                            return i_handler.has_value() &&
                                   i_handler->getObject() == i_object;
                          }) != end(handlers);
    }

    void setNotProcessedEvents(const void* i_object,
                               std::vector<ArrayView2<Hash>>&& notProcessedEvents)
    {
      for (auto& handler: handlers)
      {
        if (handler.has_value() && handler->getObject() == i_object)
        {
          handler->notProcessesEvents = std::move(notProcessedEvents);
        }
      }
    }

    template<typename F>
    inline size_t removeIf(const void* object, F shouldRemove)
    {
      size_t disconnected = 0;
      for (auto& handler: handlers)
      {
        if (handler.has_value() &&
            handler->getObject() == object &&
            shouldRemove(*handler))
        {
          handler.reset();
          ++disconnected;
        }
      }
      dirty = disconnected > 0;
      removeEmpty();
      return disconnected;
    }

    size_t disconnect(const void* object);
    size_t disconnect(const void* object, const MethodId methodHash);

    inline bool isEmpty() const
    {
      return handlers.empty();
    }

    std::vector<std::optional<Handler>> handlers;

  private:
    void removeEmpty();

    bool isInInvokeProcess = false;
    bool dirty = false;
  };

  struct SimpleInvoker
  {
    explicit SimpleInvoker()
    {
    }

    inline void append(const size_t i_pos, const FunctionView& i_function)
    {
      functions.emplace_back(i_pos, i_function);
    }

    void invoke(const void* event)
    {
      const auto firstLevel = !isInInvokeProcess;
      isInInvokeProcess = true;
      // нельзя использовать range for
      // при увеличении длины массива handlers он может быть перенесен в другое
      // место в памяти и все итераторы станут невалидными
      for (size_t i = 0; i < functions.size(); ++i)
      {
        const auto& function = functions[i];
        if (function.value.has_value())
        {
          function.value->invoke(event);
        }
      }
      if (firstLevel)
      {
        isInInvokeProcess = false;
        removeEmpty();
      }
    }

    inline size_t disconnect(const void* object)
    {
      size_t disconnected = 0;
      for (auto& [pos, function]: functions)
      {
        if (function.has_value() && function->getObject() == object)
        {
          function.reset();
          ++disconnected;
        }
      }
      dirty = disconnected > 0;
      removeEmpty();
      return disconnected;
    }

    inline bool isEmpty() const
    {
      return functions.empty();
    }

    inline void sort()
    {
      std::sort(begin(functions), end(functions),
                [](const auto& left, const auto& right)
                {
                  return left.pos < right.pos;
                });
    }

    std::vector<NumberedFunctionView> functions;

  private:
    void removeEmpty();

    bool isInInvokeProcess = false;
    bool dirty = false;
  };

  struct EventMethodType
  {
    TypeId event;
    MethodId method;
  };

  inline bool isBaseOf(const ShortTypeInfo i_base,
                       const ArrayView2<TypeId> i_derived)
  {
    return !i_base.empty() &&
           i_derived.size() > i_base.depthOfInheritance &&
           i_base.typeId == i_derived[i_base.depthOfInheritance - 1];
  };

  struct InvokerContainerImpl
  {
    inline void registerType(const ArrayView2<TypeId> typeInfo)
    {
      eventTypes.try_emplace(typeInfo.back(),
                             std::vector<TypeId>{typeInfo.begin(),
                                                 typeInfo.end()});
    }

    inline void connect(const ArrayView2<TypeId> eventType, Handler handler)
    {
      handler.pos = pos++;
      invokers[eventType.back()].append(handler);
      registerType(eventType);
      updateDependencies(handler.getObject());
      simpleInvokersUpdated = false;
    }

    inline ArrayView2<TypeId> getTypeInfo(const TypeId i_typeId)
    {
      const auto it = eventTypes.find(i_typeId);
      return it == end(eventTypes) ? ArrayView2<TypeId>() : it->second;
    }

    template<typename T>
    struct TreeNode
    {
      T value;
      std::vector<TreeNode<T>> subTree;
    };

    struct EventHandlers
    {
      ArrayView2<TypeId> eventTypeInfo;
      std::vector<Handler*> handlers;
    };

    using EventHandlersTreeNode = TreeNode<EventHandlers>;

    inline EventHandlersTreeNode* updateTreeFrom(
            std::vector<EventHandlersTreeNode>& result,
            const ArrayView2<TypeId> newTypeInfo, Handler& handler)
    {
      for (auto& treeNode: result)
      {
        if (isBaseOf(treeNode.value.eventTypeInfo, newTypeInfo))
        {
          return updateTreeFrom(treeNode.subTree, newTypeInfo, handler);
        }
        if (isBaseOf(newTypeInfo, treeNode.value.eventTypeInfo))
        {
          auto extractedTreeNode = std::move(treeNode);
          treeNode.value.eventTypeInfo = newTypeInfo;
          treeNode.value.handlers = decltype(treeNode.value.handlers){&handler};
          treeNode.subTree =
                  decltype(treeNode.subTree){std::move(extractedTreeNode)};
          return &treeNode;
        }
        if (newTypeInfo.back() == treeNode.value.eventTypeInfo.back())
        {
          treeNode.value.handlers.push_back(&handler);
          return &treeNode;
        }
      }
      result.push_back({{newTypeInfo, {&handler}}});
      return &result.back();
    }

    inline void updateTreeFrom(std::vector<EventHandlersTreeNode>& result,
                               Invoker& invoker, const TypeId eventTypeId,
                               const void* i_object)
    {
      EventHandlersTreeNode* eventHandlersTreeNode = nullptr;
      for (auto& handler: invoker.handlers)
      {
        if (handler.has_value() && handler->getObject() == i_object)
        {
          if (eventHandlersTreeNode)
          {
            eventHandlersTreeNode->value.handlers.push_back(&*handler);
          }
          else
          {
            const auto eventTypeInfo = getTypeInfo(eventTypeId);
            eventHandlersTreeNode =
                    updateTreeFrom(result, eventTypeInfo, *handler);
          }
        }
      }
    }

    inline std::vector<EventHandlersTreeNode> makeInvokersTree(
            const void* i_object)
    {
      std::vector<EventHandlersTreeNode> result;
      for (auto&[eventTypeId, invoker]: invokers)
      {
        updateTreeFrom(result, invoker, eventTypeId, i_object);
      }
      return result;
    }

    inline static std::vector<ArrayView2<TypeId>> getDirectСhildren(
            const std::vector<EventHandlersTreeNode>& i_tree,
            const ArrayView2<TypeId> eventTypeInfo)
    {
      std::vector<ArrayView2<TypeId>> result;
      for (const auto& node: i_tree)
      {
        if (node.value.eventTypeInfo.back() == eventTypeInfo.back())
        {
          std::transform(begin(node.subTree), end(node.subTree),
                         std::back_inserter(result), [](const auto& node)
                         {
                           return node.value.eventTypeInfo;
                         });
          return result;
        }
        if (isBaseOf(node.value.eventTypeInfo, eventTypeInfo))
        {
          return getDirectСhildren(node.subTree, eventTypeInfo);
        }
      }
      return result;
    }

    inline void updateDependencies(std::vector<EventHandlersTreeNode>& nodes)
    {
      for (auto& node: nodes)
      {
        const auto directChildren =
                getDirectСhildren(nodes, node.value.eventTypeInfo);
        for (auto* handler: node.value.handlers)
        {
          handler->notProcessesEvents = directChildren;
        }
        updateDependencies(node.subTree);
      }
    }

    inline void updateDependencies(const void* i_object)
    {
      auto invokersTree = makeInvokersTree(i_object);
      updateDependencies(invokersTree);
    }

    void updateSimpleInvokers();

    void invoke(const void* i_event, const ArrayView2<TypeId> i_eventType);
    size_t disconnect(const void* i_object);
    size_t disconnect(const void* i_object,
                      const ArrayView2<EventMethodType> i_eventMethodTypes);

  private:
    inline Invoker* findInvoker(const TypeId eventTypeId)
    {
      const auto it = invokers.find(eventTypeId);
      return it == end(invokers) ? nullptr : &it->second;
    }

    inline SimpleInvoker* findSimpleInvoker(const TypeId eventTypeId)
    {
      const auto it = simpleInvokers.find(eventTypeId);
      return it == end(simpleInvokers) ? nullptr : &it->second;
    }

    void removeEmpty();

    inline size_t disconnect1(const void* i_object, const EventMethodType hash)
    {
      if (auto* invoker = findInvoker(hash.event))
      {
        if (const auto disconnected = invoker->disconnect(i_object, hash.method); disconnected > 0)
        {
          updateDependencies(i_object);
          return disconnected;
        }
      }
      return 0;
    }

    std::unordered_map<TypeId, Invoker> invokers;
    std::unordered_map<TypeId, std::vector<TypeId>> eventTypes;
    std::unordered_map<TypeId, SimpleInvoker> simpleInvokers;
    bool isInInvokeProcess = false;
    bool dirty = false;
    bool simpleInvokersUpdated = false;
    size_t pos = 0;
  };

  struct InvokerContainer
  {
    template<typename Event>
    void invoke(const Event& event)
    {
      static constexpr auto eventTypeInfo = collectBaseHashes<Event>();
      invokerContainerImpl.invoke(&event, eventTypeInfo);
    }

    template<auto ...Methods>
    void connect(Class<Methods...>& i_object, Register<Methods...>)
    {
      connect<Methods...>(i_object);
    }

    template<auto... Methods>
    void connect(Class<Methods...>& i_object)
    {
      // создается временный array на который потом ссылаютя через arrayview
      // нужно переделать
      (invokerContainerImpl.connect(collectBaseHashes<Argument<Methods>>(),
                                    Handler(i_object,
                                            TemplateParameter<Methods>())), ...);
    }

    template<auto... Methods>
    size_t disconnect(const Class<Methods...>& i_object)
    {
      if constexpr(sizeof...(Methods) == 0)
      {
        return invokerContainerImpl.disconnect(&i_object);
      }
      else
      {
        return invokerContainerImpl.disconnect(&i_object, {EventMethodType{
                TypeHash<Argument<Methods>>, ValueHash<Methods>}...});
      }
    }

  private:
    InvokerContainerImpl invokerContainerImpl;
  };
}

