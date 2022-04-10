#include "Utils/MapUtils.hpp"
#include "Utils/ScoreUtils.hpp"

#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/SliderData.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "GlobalNamespace/BeatmapEnvironmentHelper.hpp"

#include "UnityEngine/Resources.hpp"

#include "System/Threading/Tasks/Task_1.hpp"

using namespace GlobalNamespace;

std::vector<int> routines;

namespace ScorePercentage::MapUtils{

    custom_types::Helpers::Coroutine DoNewPercentageStuff(PlayerSpecificSettings* playerSpecificSettings, IDifficultyBeatmap* difficultyBeatmap)
    {
        int crIndex = routines.size() + 1; routines.push_back(crIndex);
        if (scoreDetailsUI != nullptr) scoreDetailsUI->loadingInfo();
        auto* envInfo = BeatmapEnvironmentHelper::GetEnvironmentInfo(difficultyBeatmap);
        auto* result = difficultyBeatmap->GetBeatmapDataAsync(envInfo, playerSpecificSettings);
        while (!result->get_IsCompleted()) co_yield nullptr;
        auto* data = result->get_ResultOnSuccess();
        if (routines.empty() || routines.size() != crIndex) co_return;
        ClearVector<int>(&routines);
        int maxScore = data != nullptr ? ScoreModel::ComputeMaxMultipliedScoreForBeatmap(data) : 1;
        float currentPercentage = ScorePercentage::Utils::calculatePercentage(maxScore, mapData.currentScore);
        mapData.currentPercentage = currentPercentage; mapData.maxScore = maxScore;
        if (scoreDetailsUI != nullptr) data != nullptr ? scoreDetailsUI->updateInfo() : scoreDetailsUI->loadingFailed();
        co_return;
    }

    void updateMapData(PlayerData* playerData, IDifficultyBeatmap* difficultyBeatmap){
        auto* playerLevelStatsData = playerData->GetPlayerLevelStatsData(difficultyBeatmap);
        int currentScore = playerLevelStatsData->highScore;
        std::string mapID = playerLevelStatsData->levelID;
        std::string mapType = playerLevelStatsData->get_beatmapCharacteristic()->get_serializedName();
        mapData.mapID = mapID;
        int diff = difficultyBeatmap->get_difficultyRank();
        mapData.currentScore = currentScore;
        SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(DoNewPercentageStuff(playerData->playerSpecificSettings, difficultyBeatmap)));
        mapData.diff = difficultyBeatmap->get_difficulty();
        mapData.mapType = mapType;
        mapData.isFC = playerLevelStatsData->fullCombo;
        mapData.playCount = playerLevelStatsData->playCount;
        mapData.maxCombo = playerLevelStatsData->maxCombo;
        mapData.idString = mapType.compare("Standard") != 0 ? mapType + std::to_string(diff) : std::to_string(diff);
    }
}