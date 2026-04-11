#pragma once
#include <entt/entt.hpp>
#include <functional>
#include <memory>

// expose a handle type for game layer to declare member variables
template<typename TEvent>
class MessageHandle {
    using Fn = std::function<void(const TEvent&)>;
public:
    MessageHandle() = default;

    MessageHandle(entt::dispatcher& dispatcher, Fn fn)
        : _dispatcher(&dispatcher)
        , _fn(std::make_shared<Fn>(std::move(fn)))
    {
        _wrapper = std::make_shared<Fn>([f = _fn](const TEvent& e) {
            (*f)(e);
            });
        _dispatcher->sink<TEvent>().connect(*_wrapper);
    }

    MessageHandle(const MessageHandle&) = delete;
    MessageHandle& operator=(const MessageHandle&) = delete;
    MessageHandle(MessageHandle&&) = default;
    MessageHandle& operator=(MessageHandle&&) = default;

    ~MessageHandle() {
        if (_dispatcher && _wrapper)
            _dispatcher->sink<TEvent>().disconnect(*_wrapper);
    }

private:
    entt::dispatcher* _dispatcher = nullptr;
    std::shared_ptr<Fn> _fn;
    std::shared_ptr<Fn> _wrapper;

	// messagecenter needs access to the constructor
    friend class MessageCenter;
};

//for ui and game audio
class MessageCenter {
public:
    static MessageCenter& Get() {
        static MessageCenter instance;
        return instance;
    }

	// suscribe a free function or static member function
    template<typename TEvent, auto Fn>
    static void Subscribe() {
        Get()._dispatcher.sink<TEvent>().connect<Fn>();
    }

	// suscribe a member function
    template<typename TEvent, auto MemberFn, typename T>
    static void Subscribe(T* instance) {
        Get()._dispatcher.sink<TEvent>().connect<MemberFn>(instance);
    }

	// suscribe a lambda or any callable object, return a handle that automatically unsubscribes when destroyed
    // todo: complete
    template<typename TEvent, typename Callable>
    [[nodiscard]] static MessageHandle<TEvent> Subscribe(Callable&& fn) {
        return MessageHandle<TEvent>(
            Get()._dispatcher,
            std::forward<Callable>(fn)
        );
    }

    template<typename TEvent, auto Fn>
    static void Unsubscribe() {
        Get()._dispatcher.sink<TEvent>().disconnect<Fn>();
    }

    template<typename TEvent, auto MemberFn, typename T>
    static void Unsubscribe(T* instance) {
        Get()._dispatcher.sink<TEvent>().disconnect<MemberFn>(instance);
    }

    template<typename TEvent>
    static void Publish(TEvent&& message) {
        Get()._dispatcher.trigger(std::forward<TEvent>(message));
    }

    template<typename TEvent>
    static void PublishDeferred(TEvent&& message) {
        Get()._dispatcher.enqueue(std::forward<TEvent>(message));
    }

	// deal with all the pending events in the queues
    static void Flush() {
        Get()._dispatcher.update();
    }

	// deal with the pending events of a specific type
    template<typename TEvent>
    static void Flush() {
        Get()._dispatcher.update<TEvent>();
    }

private:
    MessageCenter() = default;
    entt::dispatcher _dispatcher;
};