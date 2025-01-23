#include "System/Threading/Tasks/Task.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "custom-types/shared/delegate.hpp"
#include <coroutine>
#include <bsml/shared/BSML/MainThreadScheduler.hpp>

template <typename T> 
class TaskW_1;

class TaskCoroutine {
    public:
    struct Promise {
        TaskCoroutine get_return_object() { return TaskCoroutine {}; }

        void unhandled_exception() noexcept { }
        void return_void() noexcept { }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }

        template<typename T>
        auto await_transform(T&& t) { return std::forward<T>(t); }

        template<typename T>
        auto await_transform(System::Threading::Tasks::Task_1<T>*&& t) { return TaskW_1(t); }
    };
    using promise_type = Promise;
};

template <typename T> 
class TaskW_1 {
    public:
    TaskW_1(System::Threading::Tasks::Task_1<T> *task) : _task(task) {}

    bool await_ready() { return _task->get_IsCompleted(); }
    bool await_suspend(std::coroutine_handle<> handle) {
        if (_task->get_IsCompleted()) return false;
        reinterpret_cast<System::Threading::Tasks::Task*>(_task)->ContinueWith(
            custom_types::MakeDelegate<System::Action_1<System::Threading::Tasks::Task*>*>(
                std::function([handle](System::Threading::Tasks::Task_1<T>* t) {
                    handle.resume();
                })));
        return true;
    }
    T await_resume() { return _task->get_Result(); }

    private:
    System::Threading::Tasks::Task_1<T>* _task;
};

class YieldMainThread {
    public:
    bool await_ready() { return BSML::MainThreadScheduler::CurrentThreadIsMainThread(); }
    bool await_suspend(std::coroutine_handle<> handle) {
        if (BSML::MainThreadScheduler::CurrentThreadIsMainThread()) return false;
        BSML::MainThreadScheduler::Schedule([handle]() {
            handle.resume();
        });
        return true;
    }
    void await_resume() { }
};