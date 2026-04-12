#pragma once

#define SCORE_PERCENTAGE_EXPORT __attribute__((visibility("default")))
#ifdef __cplusplus
#define SCORE_PERCENTAGE_EXPORT_FUNC extern "C" SCORE_PERCENTAGE_EXPORT
#else
#define SCORE_PERCENTAGE_EXPORT_FUNC SCORE_PERCENTAGE_EXPORT
#endif

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include "scotland2/shared/loader.hpp"
// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "paper2_scotland2/shared/logger.hpp"

inline const auto& getLogger() {
    static constexpr auto Logger = Paper::ConstLoggerContext(MOD_ID);
    return Logger;
}