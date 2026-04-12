#include "Utils/MapUtils.hpp"
#include "Utils/ScoreUtils.hpp"

#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "GlobalNamespace/OverrideEnvironmentSettings.hpp"
#include "GlobalNamespace/BeatmapDataTransformHelper.hpp"
#include "GlobalNamespace/LevelStatsView.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/MainFlowCoordinator.hpp"
#include "UnityEngine/Resources.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/BeatmapDataLoader.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/IEnvironmentInfo.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include <cstdint>
#include <unordered_map>
#include "Utils/TaskCoroutine.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"

using DiffMap = std::unordered_map<int, int>;
using CharacMap = std::unordered_map<std::string, DiffMap>;
using ScoreValuesMap = std::unordered_map<std::string, CharacMap>;

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace System::Threading;
using namespace System::Threading::Tasks;
namespace ScorePercentage::MapUtils{
    static ScoreValuesMap maxScoreValues;
        
    void cacheMaxScoreData(BeatmapKey beatmapKey, int maxScore) {
        auto levelID = beatmapKey.levelId;
        auto characteristic = beatmapKey.beatmapCharacteristic->get_serializedName();
        auto difficulty = (int)beatmapKey.difficulty;

        maxScoreValues[levelID][characteristic][difficulty] = maxScore;
    }

    int RetrieveMaxScoreDataFromCache(BeatmapKey beatmapKey) {
        auto foundLevel = maxScoreValues.find(beatmapKey.levelId);
        if (foundLevel == maxScoreValues.end()) return -1;
        auto foundCharac = foundLevel->second.find(beatmapKey.beatmapCharacteristic->get_serializedName());
        if (foundCharac == foundLevel->second.end()) return -1;
        auto foundDiff = foundCharac->second.find((int)beatmapKey.difficulty);
        if (foundDiff == foundCharac->second.end()) return -1;
        int maxScore = foundDiff->second;
        return maxScore;
    }

    MenuTransitionsHelper* getTransitionsHelper() {
        static SafePtrUnity<MenuTransitionsHelper> transitionsHelper;
        if (transitionsHelper) return transitionsHelper.ptr();
        transitionsHelper = UnityEngine::Resources::FindObjectsOfTypeAll<MenuTransitionsHelper*>().front_or_default();
        return transitionsHelper.ptr();
    }

    PlayerData* getPlayerData() {
        static SafePtrUnity<PlayerDataModel> playerDataModel;
        if (playerDataModel) return playerDataModel->get_playerData();
        playerDataModel = UnityEngine::Resources::FindObjectsOfTypeAll<PlayerDataModel*>().front_or_default();
        return playerDataModel->get_playerData();
    }

    task_coroutine<IReadonlyBeatmapData*> getBeatmapDataAsync(BeatmapKey key) {
        auto helper = getTransitionsHelper();
        auto playerData = getPlayerData();
        BeatmapLevel* level = helper->_beatmapLevelsModel->GetBeatmapLevel(key.levelId, false);
        bool isCustom = key.levelId->StartsWith("custom_level_");
        BeatmapLevelDataVersion ver = BeatmapLevelDataVersion::Original;
        LoadBeatmapLevelDataResult levelDataResult = co_await helper->_beatmapLevelsModel->LoadBeatmapLevelDataAsync(key.levelId, ver, nullptr);
        if (levelDataResult.isError) co_return nullptr;

        IBeatmapLevelData* levelData = levelDataResult.beatmapLevelData;
        auto env = playerData->get_overrideEnvironmentSettings()->GetOverrideEnvironmentInfoForType(EnvironmentType::Normal).cast<IEnvironmentInfo>();
        auto mod = playerData->get_gameplayModifiers();
        auto set = playerData->get_playerSpecificSettings();

        if (!isCustom) co_await YieldMainThread();
        IReadonlyBeatmapData* beatmapData = co_await helper->_beatmapDataLoader->LoadBeatmapDataAsync(levelData, key, level->beatsPerMinute, false, env, env, ver, mod, set, false);
        co_return beatmapData;
    }

    task_coroutine<int> getMaxScoreAsync(BeatmapKey key) {
        int cached = RetrieveMaxScoreDataFromCache(key);
        if (cached != -1) co_return cached;
        static std::atomic<uint32_t> guid = 0; // the user doesn't have over 4 billion songs right :surely:
        uint32_t localGuid = ++guid;

        IReadonlyBeatmapData* beatmapData = co_await getBeatmapDataAsync(key);
        co_await YieldMainThread();

        if (!beatmapData) co_return -1;

        int maxScoreOmg = ScoreModel::ComputeMaxMultipliedScoreForBeatmap(beatmapData);
        if (maxScoreOmg != -1) cacheMaxScoreData(key, maxScoreOmg);
        co_return maxScoreOmg;
    }
}