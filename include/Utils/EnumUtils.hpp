#pragma once

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include <algorithm>
#include <optional>

namespace ScorePercentage::EnumUtils {

    // Returns a map which maps the value of an enum to its name
    template<typename T>
    inline const auto& GetEnumValueNameMap() {
        static std::map<decltype(T::value__), std::string> values;

        if (!values.empty()) return values;

        Il2CppClass* klass = classof(T);

        for (int i=0; i<klass->field_count; i++) {

            // internal implementation of enums has extra shit haha lmao love it
            if (il2cpp_functions::class_from_type(klass->fields[i].type) != klass) continue;

            T value;
            il2cpp_functions::field_static_get_value(&klass->fields[i], &value); // retrieve the enum value
            
            values[value.value__] = std::string(klass->fields[i].name);
        }
        return values;
    }

    // Get the name of an enum from its value
    template<typename T>
    const std::string& GetEnumName(const T& enumValue) {
        return GetEnumValueNameMap<T>().find(enumValue.value__)->second;
    }

    // Try to parse the string name as an actual enum value
    template<typename T>
    const std::optional<T> ParseEnumString(const std::string& name) {
        const auto& map = GetEnumValueNameMap<T>();
        auto it = std::find_if(map.begin(), map.end(), [&](auto& val) { return val.second == name; });
        return it == map.end() ? std::optional<T>{std::nullopt} : T(it->first);
    }
}