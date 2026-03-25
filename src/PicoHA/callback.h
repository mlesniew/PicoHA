#pragma once

#ifndef PICOCALLBACK_USE_STD_FUNCTION

#include <cstddef>
#include <type_traits>
#include <utility>

template <typename ReturnType, typename... Args>
class PicoCallback final {
public:
    using FuncPtr = ReturnType (*)(const void *, Args...);
    using NoContextFuncPtr = ReturnType (*)(Args...);

    // Build from raw callback parts.
    // Example: PicoCallback<void> cb(myFuncPtr, myContext);
    PicoCallback(FuncPtr func = nullptr, const void * ctx = nullptr)
        : func(func), ctx(ctx) {}

    // Build from a free function with no context.
    // Example: PicoCallback<int, int> cb(myFreeFunction);
    PicoCallback(ReturnType (*f)(Args...))
        : func(trampoline), ctx(reinterpret_cast<const void *>(f)) {}

    // Build from a free function with mutable context.
    // Example: PicoCallback<void, int> cb(setValue, &state);
    template <typename T>
    PicoCallback(ReturnType (*f)(T *, Args...), T * context)
        : func(reinterpret_cast<FuncPtr>(f)), ctx(context) {}

    // Build from a free function with read-only context.
    // Example: PicoCallback<int> cb(readValue, &state);
    template <typename T>
    PicoCallback(ReturnType (*f)(const T *, Args...), const T * context)
        : func(reinterpret_cast<FuncPtr>(f)), ctx(context) {}

    // Build from a non-capturing lambda with mutable context.
    // Example: PicoCallback<void, int> cb([](Foo* f, int x){ f->set(x); },
    // &foo);
    template <typename F, typename T,
              typename std::enable_if<
                  std::is_convertible<F, ReturnType (*)(T *, Args...)>::value,
                  int>::type = 0>
    PicoCallback(F f, T * context)
        : func(reinterpret_cast<FuncPtr>(+f)), ctx(context) {}

    // Build from a non-capturing lambda with read-only context.
    // Example: PicoCallback<int> cb([](const Foo* f){ return f->value(); },
    // &foo);
    template <
        typename F, typename T,
        typename std::enable_if<
            std::is_convertible<F, ReturnType (*)(const T *, Args...)>::value,
            int>::type = 0>
    PicoCallback(F f, const T * context)
        : func(reinterpret_cast<FuncPtr>(+f)), ctx(context) {}

    PicoCallback & operator=(ReturnType (*f)(Args...)) {
        func = trampoline;
        ctx = reinterpret_cast<const void *>(f);
        return *this;
    }

    PicoCallback & operator=(std::nullptr_t) {
        reset();
        return *this;
    }

    template <typename F, typename Ptr = decltype(+std::declval<F>()),
              typename std::enable_if<
                  std::is_pointer<Ptr>::value &&
                      std::is_convertible<Ptr, NoContextFuncPtr>::value,
                  int>::type = 0>
    PicoCallback & operator=(F f) {
        return (*this = static_cast<NoContextFuncPtr>(+f));
    }

    void reset() {
        func = nullptr;
        ctx = nullptr;
    }

    bool isSet() const { return func != nullptr; }

    explicit operator bool() const { return isSet(); }

    ReturnType operator()(Args... args) const {
        return func(ctx, std::forward<Args>(args)...);
    }

private:
    // Trampoline for no-context free functions
    static ReturnType trampoline(const void * p, Args... args) {
        auto fp = reinterpret_cast<ReturnType (*)(Args...)>(p);
        return fp(std::forward<Args>(args)...);
    }

    FuncPtr func;
    const void * ctx;
};

#else

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

// When PICOCALLBACK_USE_STD_FUNCTION is defined, PicoCallback wraps
// std::function with the same constructor API as the zero-heap version.
template <typename ReturnType, typename... Args>
class PicoCallback final {
public:
    using FuncType = std::function<ReturnType(Args...)>;

    // Build empty or from std::function directly.
    // Example: PicoCallback<void> cb(myFunction);
    PicoCallback(FuncType func = nullptr) : func(func) {}

    // Build from any callable std::function can store.
    // Example: PicoCallback<void> cb([x]{ use(x); });
    template <typename F, typename std::enable_if<
                              std::is_constructible<FuncType, F>::value &&
                                  !std::is_same<typename std::decay<F>::type,
                                                PicoCallback>::value &&
                                  !std::is_same<typename std::decay<F>::type,
                                                std::nullptr_t>::value,
                              int>::type = 0>
    PicoCallback(F f) : func(std::forward<F>(f)) {}

    // Build from callable + context pointer.
    // Example: PicoCallback<void, int> cb(setValue, &state);
    template <typename F, typename T>
    PicoCallback(F f, T * context)
        : func([f, context](Args... args) {
              return f(context, std::forward<Args>(args)...);
          }) {}

    // Assign nullptr to clear.
    PicoCallback & operator=(std::nullptr_t) {
        reset();
        return *this;
    }

    // Assign from any callable std::function can store.
    // Example: cb = [x]{ use(x); };
    template <typename F, typename std::enable_if<
                              std::is_assignable<FuncType &, F>::value &&
                                  !std::is_same<typename std::decay<F>::type,
                                                PicoCallback>::value &&
                                  !std::is_same<typename std::decay<F>::type,
                                                std::nullptr_t>::value,
                              int>::type = 0>
    PicoCallback & operator=(F f) {
        func = std::forward<F>(f);
        return *this;
    }

    void reset() { func = nullptr; }

    bool isSet() const { return static_cast<bool>(func); }

    explicit operator bool() const { return isSet(); }

    ReturnType operator()(Args... args) const {
        return func(std::forward<Args>(args)...);
    }

private:
    FuncType func;
};

#endif