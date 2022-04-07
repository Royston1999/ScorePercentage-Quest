#pragma once
#include "main.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "custom-types/shared/coroutine.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"

namespace ScorePercentage::MapUtils{
    void updateMapData(GlobalNamespace::PlayerData* playerData, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap);

    template<class T>
    void ClearVector(std::vector<T>* vector){
        vector->clear(); std::vector<T>().swap(*vector);
    };
    
}