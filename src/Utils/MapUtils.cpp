#include "Utils/MapUtils.hpp"
#include "Utils/ScoreUtils.hpp"

#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/SliderData.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "GlobalNamespace/SceneInfo.hpp"
#include "GlobalNamespace/OverrideEnvironmentSettings.hpp"
#include "GlobalNamespace/ColorSchemeSO.hpp"
#include "GlobalNamespace/BeatmapDataTransformHelper.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"

#include "scoreutils/shared/ScoreUtils.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine;

namespace ScorePercentage::MapUtils{

    void updateMapData(PlayerData* playerData, IDifficultyBeatmap* difficultyBeatmap, bool forced){
        if (scoreDetailsUI != nullptr) scoreDetailsUI->updateInfo("loading...");
        updateBasicMapInfo(playerData, difficultyBeatmap);
        int maxScore = ScoreUtils::MaxScoreRetriever::RetrieveMaxScoreDataFromCache();
        if (maxScore != -1 && scoreDetailsUI != nullptr){
            finishedLoading = true;
            float currentPercentage = ScorePercentage::Utils::CalculatePercentage(maxScore, mapData.currentScore);
            mapData.currentPercentage = currentPercentage; mapData.maxScore = maxScore;
            if (scoreDetailsUI != nullptr) scoreDetailsUI->updateInfo();
        }
    }

    void updateBasicMapInfo(PlayerData* playerData, IDifficultyBeatmap* difficultyBeatmap){
        auto* playerLevelStatsData = playerData->GetPlayerLevelStatsData(difficultyBeatmap);
        int currentScore = playerLevelStatsData->highScore;
        std::string mapID = playerLevelStatsData->levelID;
        std::string mapType = playerLevelStatsData->get_beatmapCharacteristic()->get_serializedName();
        mapData.mapID = mapID;
        int diff = difficultyBeatmap->get_difficultyRank();
        mapData.currentScore = currentScore;
        mapData.diff = difficultyBeatmap->get_difficulty();
        mapData.mapType = mapType;
        mapData.isFC = playerLevelStatsData->fullCombo;
        mapData.playCount = playerLevelStatsData->playCount;
        mapData.maxCombo = playerLevelStatsData->maxCombo;
        mapData.idString = mapType.compare("Standard") != 0 ? mapType + std::to_string(diff) : std::to_string(diff);
    }
}