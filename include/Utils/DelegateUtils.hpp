#pragma once
#include "custom-types/shared/delegate.hpp"

namespace DelegateUtils {
    template<typename T, typename O>
    struct delegate_func;

    template<typename TRet, typename T, typename... TArgs>
    struct delegate_func<TRet(T::*)(TArgs...), void> {
        using FuncPtr = TRet(*)(TArgs...);
    };

    template<typename O, typename TRet, typename T, typename... TArgs>
    struct delegate_func<TRet(T::*)(TArgs...), O> {
        using FuncPtr = TRet(O::*)(TArgs...);
    };

    template<typename T, typename O>
    using delegate_func_t = typename delegate_func<decltype(&T::Invoke), O>::FuncPtr;

    template<typename T = void*, typename O = void>
    struct DelegateW {
        T funcPtr;
        O* objPtr;
        DelegateW(T f, O* o = nullptr) : funcPtr(f), objPtr(o) {}
    };

    template<typename T>
    concept Delegate = std::is_convertible_v<T*, System::MulticastDelegate*>;

    template <typename T, typename O, typename TRet, typename... TArgs>
    inline T* make_delegate(const DelegateW<TRet(O::*)(TArgs...), O>& func) {
        return custom_types::MakeDelegate<T*>(func.objPtr, std::function<TRet(O*, TArgs...)>(func.funcPtr));
    }

    template<typename T, typename TRet, typename... TArgs>
    inline T* make_delegate(const DelegateW<TRet(*)(TArgs...), void>& func) {
        return custom_types::MakeDelegate<T*>(std::function<TRet(TArgs...)>(func.funcPtr));
    }

    template<typename T, typename U>
    inline bool compare_function(const std::function<T>& function, U fptr) {
        auto val = function.template target<decltype(fptr)>();
        return val && *val == fptr;
    }

    template<typename O, typename TRet, typename... TArgs>
    inline bool compare_delegate(System::Object* inst, const DelegateW<TRet(O::*)(TArgs...), O>& func) {
        auto opt = il2cpp_utils::try_cast<custom_types::DelegateWrapperInstance<TRet, O*, TArgs...>>(inst);
        if (!opt.has_value()) return false;
        return opt.value()->obj == func.objPtr && compare_function(opt.value()->wrappedFunc, func.funcPtr);
    }

    template<typename TRet, typename... TArgs>
    inline bool compare_delegate(System::Object* inst, const DelegateW<TRet(*)(TArgs...), void>& func) {
        auto opt = il2cpp_utils::try_cast<custom_types::DelegateWrapperStatic<TRet, TArgs...>>(inst);
        if (!opt.has_value()) return false;
        return compare_function(opt.value()->wrappedFunc, func.funcPtr);
    }

    template<Delegate T, typename O>
    inline T* extract_delegate(T*& csDelegate, const DelegateW<delegate_func_t<T, O>, O>& func) {
        std::vector<System::Delegate*> delegates;
        delegates.push_back(csDelegate);
        if (csDelegate->delegates) delegates.insert(delegates.end(), csDelegate->delegates.begin(), csDelegate->delegates.end());
        
        for (System::Delegate* currentDeleg : delegates | std::views::reverse) {
            if (!currentDeleg || !currentDeleg->m_target) continue;
            if (!compare_delegate(currentDeleg->m_target, func)) continue;
            return static_cast<T*>(currentDeleg);
        }
        return nullptr;
    }

    template<Delegate T, typename O>
    void operator+=(T*& csDelegate, const DelegateW<delegate_func_t<T, O>, O>& func) {
        T* delegate = make_delegate<T>(func);
        csDelegate = static_cast<T*>(System::Delegate::Combine(csDelegate, delegate));
    }

    template<Delegate T, typename O>
    void operator-=(T*& csDelegate, const DelegateW<delegate_func_t<T, O>, O>& func) {
        if (!csDelegate) return;

        T* delegate = extract_delegate(csDelegate, func);
        if (!delegate) return;
        csDelegate = static_cast<T*>(System::Delegate::Remove(csDelegate, delegate));
    }
}