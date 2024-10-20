#pragma once

#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"

using callback_ptr = void(*)(std::string);

struct RawPPData {
    float _Easy_SoloStandard = 0.0f;
    float _Normal_SoloStandard = 0.0f;
    float _Hard_SoloStandard = 0.0f;
    float _Expert_SoloStandard = 0.0f;
    float _ExpertPlus_SoloStandard = 0.0f;
};

namespace PPCalculator {
    namespace PP {
        static std::unordered_map<std::string, RawPPData> index;

        void Initialize();
        void HandlePPWebRequestCompleted(std::string text);
        float CalculatePP(float maxPP, float accuracy);
        float BeatmapMaxPP(std::string songID, GlobalNamespace::BeatmapDifficulty difficulty);
    }
}

namespace ScorePercentage::WebUtils{
    void SendWebRequest(std::string URL, callback_ptr callback);
}