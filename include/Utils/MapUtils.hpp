#pragma once
#include "main.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "custom-types/shared/coroutine.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "System/Threading/Tasks/Task_1.hpp"

using DiffMap = std::map<int, int>;
using CharacMap = std::map<std::string, DiffMap>;
using ScoreValuesMap = std::map<std::string, CharacMap>;

namespace ScorePercentage::MapUtils{
    void updateMapData(GlobalNamespace::PlayerData* playerData, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap, bool forced = false);
    void updateBasicMapInfo(GlobalNamespace::PlayerData* playerData, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap);
    template<class T>
    void ClearVector(std::vector<T>* vector){
        vector->clear(); std::vector<T>().swap(*vector);
    };
    
}