#pragma once
#include "main.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/BeatmapKey.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "Utils/TaskCoroutine.hpp"

namespace ScorePercentage::MapUtils {
    task_coroutine<int> getMaxScoreAsync(GlobalNamespace::BeatmapKey key);
}