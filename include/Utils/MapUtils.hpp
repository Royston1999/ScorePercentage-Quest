#pragma once
#include "main.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/BeatmapKey.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "System/Threading/Tasks/Task_1.hpp"

namespace ScorePercentage::MapUtils{
    void updateMapData(GlobalNamespace::BeatmapKey* beatmayKey, bool forced = false);
    void updateBasicMapInfo(GlobalNamespace::BeatmapKey* beatmayKey);
    void getMaxScoreForBeatmapAsync(GlobalNamespace::BeatmapKey* key);
    void updateMaxScoreFromIReadonlyBeatmapData(GlobalNamespace::IReadonlyBeatmapData* beatmapData);
    template<class T>
    void ClearVector(std::vector<T>* vector){
        vector->clear(); std::vector<T>().swap(*vector);
    };
    
}