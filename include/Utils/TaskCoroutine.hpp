#pragma once

#include "System/Threading/Tasks/Task.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "custom-types/shared/delegate.hpp"
#include <coroutine>
#include <bsml/shared/BSML/MainThreadScheduler.hpp>
#include <exception>
#include <type_traits>
#include <stack>

template<typename T>
concept CSharpTask = std::is_base_of_v<System::Threading::Tasks::Task, T>;

template<typename T, typename... Args>
concept is_void_task = std::is_same_v<T, void> && sizeof...(Args) == 0;

template<typename T, typename... Args>
concept is_value_task = !std::is_same_v<T, void> && !(std::is_same_v<void, Args> || ...);

template<typename T, typename... Args>
concept is_single_value_task = sizeof...(Args) == 0 && !std::is_same_v<T, void>;

template<typename T, typename... Args>
concept is_multi_value_task = sizeof...(Args) > 0 && is_value_task<T, Args...>;

template <CSharpTask T> class CSharpTaskAwaiter;
template <typename T, typename... Args> class TaskCoroutineAwaiter;

template<typename T, typename... Args> struct task_promise;
template<> struct task_promise<void>;

struct resumer_coroutine {
    struct resumer_promise {
        void unhandled_exception() noexcept {}
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        resumer_coroutine get_return_object() noexcept { return { std::coroutine_handle<resumer_promise>::from_promise(*this) }; }
        void return_void() {}
    };
    using promise_type = resumer_promise;
    std::coroutine_handle<resumer_promise> handle;
};

template<typename S, typename... Types>
requires (is_value_task<S, Types...> || is_void_task<S, Types...>)
class task_coroutine_internal {
    template <typename T, typename... Args> friend class promise_internal;

    public:
    using promise_type = task_promise<S, Types...>;

    ~task_coroutine_internal() {
        if (!handle) return;
        handle.promise().ref_count--;
        if (!(handle.promise().ref_count) && handle.done()) { handle.destroy(); }
    }

    task_coroutine_internal(const task_coroutine_internal& copy) : handle(copy.handle) { handle.promise().ref_count++; };
    task_coroutine_internal(task_coroutine_internal&& other) noexcept : handle(other.handle) { other.handle = nullptr; }

    protected:
    task_coroutine_internal(std::coroutine_handle<promise_type> handle) : handle(handle) { handle.promise().ref_count++; }
    std::coroutine_handle<promise_type> handle;

    void internal_wait() const { while (!handle.done()) continue; }
};

template<typename S, typename... Types>
struct task_coroutine : public task_coroutine_internal<S, Types...> {
    auto await_result() {
        this->internal_wait();
        if constexpr(is_single_value_task<S, Types...>) return std::get<S>(this->handle.promise().get_future().get());
        else if constexpr(is_multi_value_task<S, Types...>) return this->handle.promise().get_future().get();
    }
};

template<>
struct task_coroutine<void> : public task_coroutine_internal<void> {
    void wait() { this->internal_wait(); }
};

struct coro_stack {
    std::stack<std::coroutine_handle<>> handles;
    void add_handle(std::coroutine_handle<> handle) {
        handles.push(handle);
    }
    std::coroutine_handle<> pop_handle() {
        if (handles.empty()) return nullptr;
        std::coroutine_handle<> handle = handles.top();
        handles.pop();
        return handle;
    }
};

inline resumer_coroutine resume_coroutines(coro_stack& routines) {
    while (auto handle = routines.pop_handle()) handle.resume();
    co_return;
}

struct suspend_conditional {
    bool ready;
    coro_stack& coros;
    constexpr bool await_ready() const noexcept { return ready; }
    constexpr std::coroutine_handle<> await_suspend(std::coroutine_handle<> complete) const noexcept {
        switch (coros.handles.size()) {
            case 0: return std::noop_coroutine(); // if nothing to resume then do nothing
            case 1: return coros.pop_handle(); // if only 1 coro to resume then pass that directly
            default: return resume_coroutines(coros).handle; // if multiple, create resumer routine to resume them one by one 
        }
    }
    constexpr void await_resume() const noexcept {}
};

template<typename S, typename... Types>
struct promise_internal {
    using ret_type = std::conditional_t<std::is_same_v<S, void>, int, std::tuple<S, Types...>>;
    
    int ref_count = 0;
    coro_stack coros;

    void unhandled_exception() noexcept { exception = std::current_exception(); }
    void rethrow_if_exception() {
        if (exception) {
            std::rethrow_exception(exception);
        }
    }

    task_coroutine<S, Types...> get_return_object() { return { std::coroutine_handle<task_promise<S, Types...>>::from_promise(*static_cast<task_promise<S, Types...>*>(this))}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    suspend_conditional final_suspend() noexcept {
        IL2CPP_CATCH_HANDLER(rethrow_if_exception();)
        return {!ref_count && coros.handles.empty(), coros}; 
    }
    promise_internal() : future(prom.get_future()) {}
    std::shared_future<ret_type> get_future() { return future; }

    template<typename T> auto await_transform(T&& t) { return std::forward<T>(t); }
    template<typename T, typename... Args> auto await_transform(task_coroutine<T, Args...>& t) { return TaskCoroutineAwaiter<T, Args...>{t.handle}; }
    template<typename T, typename... Args> auto await_transform(task_coroutine<T, Args...>&& t) { return TaskCoroutineAwaiter<T, Args...>{t.handle}; }

    template<CSharpTask T> auto await_transform(T* t) { return CSharpTaskAwaiter(t); }
    
    protected:
    std::promise<ret_type> prom;
    std::shared_future<ret_type> future;
    std::exception_ptr exception;
};

template<typename S, typename... Types>
struct task_promise : public promise_internal<S, Types...> {
    void return_value(const std::tuple<S, Types...>& val) { this->prom.set_value(val); }
};

template<>
struct task_promise<void> : public promise_internal<void> {
    void return_void() { this->prom.set_value(0); }
};

template <typename T, typename... Args>
struct TaskCoroutineAwaiter {

    std::coroutine_handle<task_promise<T, Args...>> caller_handle;

    bool await_ready() { return caller_handle.done(); }

    void await_suspend(std::coroutine_handle<> handle) {
        caller_handle.promise().coros.add_handle(handle);
    }
    
    auto await_resume() {
        if constexpr(is_multi_value_task<T, Args...>) return caller_handle.promise().get_future().get();
        else if constexpr(is_single_value_task<T, Args...>) return std::get<T>(caller_handle.promise().get_future().get());
    }
};

template <CSharpTask T>
struct CSharpTaskAwaiter {

    template<CSharpTask U> struct TaskReturnType { using TRet = void; };
    template<typename U> struct TaskReturnType<System::Threading::Tasks::Task_1<U>> { using TRet = U; };
    
    using TRet = TaskReturnType<T>::TRet;

    CSharpTaskAwaiter(T* task) : _task(task) {}

    bool await_ready() { return _task->get_IsCompleted(); }
    
    void await_suspend(std::coroutine_handle<> handle) {
        static_cast<System::Threading::Tasks::Task*>(_task)->ContinueWith(
            custom_types::MakeDelegate<System::Action_1<System::Threading::Tasks::Task*>*>(
                std::function([handle](T* t) {
                    handle.resume();
                })
            )
        );
    }

    TRet await_resume() { 
        if constexpr(!std::is_same_v<TRet, void>) return _task->get_Result();
    }

    private:
    T* _task;
};

struct YieldMainThread {
    bool await_ready() { return BSML::MainThreadScheduler::CurrentThreadIsMainThread(); }
    void await_suspend(std::coroutine_handle<> handle) {
        BSML::MainThreadScheduler::Schedule([handle]() {
            handle.resume();
        });
    }
    void await_resume() { }
};

struct YieldNewCSharpThread {
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        il2cpp_utils::il2cpp_aware_thread([handle]() {
            handle.resume();
        }).detach();
    }
    void await_resume() { }
};