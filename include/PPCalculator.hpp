#pragma once

#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "Utils/TaskCoroutine.hpp"
#include "rapidjson-macros/shared/macros.hpp"

DECLARE_JSON_STRUCT(RawPPData) {
    NAMED_VALUE_DEFAULT(float, _Easy_SoloStandard, -1, "_Easy_SoloStandard");
    NAMED_VALUE_DEFAULT(float, _Normal_SoloStandard, -1, "_Normal_SoloStandard");
    NAMED_VALUE_DEFAULT(float, _Hard_SoloStandard, -1, "_Hard_SoloStandard");
    NAMED_VALUE_DEFAULT(float, _Expert_SoloStandard, -1, "_Expert_SoloStandard");
    NAMED_VALUE_DEFAULT(float, _ExpertPlus_SoloStandard, -1, "_ExpertPlus_SoloStandard");
};

namespace PPCalculator {
    namespace PP {
        static StringKeyedMap<RawPPData> index;

        task_coroutine<void> Initialize();
        float CalculatePP(float maxPP, float accuracy);
        float BeatmapMaxPP(std::string songID, GlobalNamespace::BeatmapDifficulty difficulty);
    }
}