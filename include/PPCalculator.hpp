#pragma once

#include <map>
#include "custom-types/shared/delegate.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Networking/UnityWebRequestAsyncOperation.hpp"
#include "UnityEngine/Networking/DownloadHandler.hpp"
#include "UnityEngine/AsyncOperation.hpp"
#include "System/Action_1.hpp"

typedef System::Action_1<UnityEngine::AsyncOperation*>* DLFinish;
using callback_ptr = void(*)(std::string);

#define DLCompletedDeleg(Func) custom_types::MakeDelegate<DLFinish>(classof(DLFinish), static_cast<std::function<void(UnityEngine::AsyncOperation*)>>(Func)) \

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