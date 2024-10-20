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
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include <unordered_map>

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

    int RetrieveMaxScoreDataFromCache() {
        auto foundLevel = maxScoreValues.find(mapData.mapID);
        if (foundLevel == maxScoreValues.end()) return -1;
        auto foundCharac = foundLevel->second.find(mapData.mapType);
        if (foundCharac == foundLevel->second.end()) return -1;
        auto foundDiff = foundCharac->second.find((int)mapData.diff);
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

    void gotMaxScoreResult(int maxScore, BeatmapKey beatmapKey) {
        float currentPercentage = ScorePercentage::Utils::CalculatePercentage(maxScore, mapData.currentScore);
        mapData.currentPercentage = currentPercentage; mapData.maxScore = maxScore;
        if (!scoreDetailsUI) return;
        finishedLoading = true;
        scoreDetailsUI->updateInfo();
        if (mapData.currentPercentage <= 0) return;
        scoreDetailsUI->modal->get_transform()->get_parent()->GetComponentInChildren<LevelStatsView*>()->_maxRankText->set_text(GET_VALUE(showPercentageInMenu) ? StringW(ScorePercentage::Utils::Round(std::abs(mapData.currentPercentage), 2) + "%") : RankModel::GetRankName(getPlayerData()->GetOrCreatePlayerLevelStatsData(ByRef<BeatmapKey>(beatmapKey))->_maxRank));                
    }

    void updateMaxScoreFromIReadonlyBeatmapData(IReadonlyBeatmapData* beatmapData) { // this is kinda dodgy
        int maxScore = ScoreModel::ComputeMaxMultipliedScoreForBeatmap(beatmapData);
        if (maxScore == -1) return;
        cacheMaxScoreData(mapData.key, maxScore);
        gotMaxScoreResult(maxScore, mapData.key);
    }

    void getMaxScoreForBeatmapAsync(BeatmapKey key) {
        static uint32_t guid = 0; // the user doesn't have over 4 billion songs right :surely:
        static std::mutex mtx;

        mtx.lock(); guid++; uint32_t localGuid = guid; mtx.unlock();

        auto helper = getTransitionsHelper();
        auto playerData = getPlayerData();
        BeatmapLevel* level = helper->_beatmapLevelsModel->GetBeatmapLevel(key.levelId);
        bool isCustom = key.levelId->StartsWith("custom_level_");
        Task_1<GlobalNamespace::LoadBeatmapLevelDataResult>* task = helper->_beatmapLevelsModel->LoadBeatmapLevelDataAsync(key.levelId, nullptr);

        il2cpp_utils::il2cpp_aware_thread([=]() {
            while(!task->get_IsCompleted()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));           
                std::lock_guard<std::mutex> lock(mtx);
                if (localGuid != guid) return;
            }
            LoadBeatmapLevelDataResult  levelDataResult = task->get_ResultOnSuccess();
            if (levelDataResult.isError) return;
            IBeatmapLevelData* data = levelDataResult.beatmapLevelData;
            Task_1<IReadonlyBeatmapData*>* mapDataTask = nullptr;
            auto env = playerData->get_overrideEnvironmentSettings()->GetOverrideEnvironmentInfoForType(EnvironmentType::Normal).cast<IEnvironmentInfo>();
            auto mod = playerData->get_gameplayModifiers();
            auto set = playerData->get_playerSpecificSettings();
            if (isCustom) { // stops lag spikes on choky levels with custom data
                mapDataTask = helper->_beatmapDataLoader->LoadBeatmapDataAsync(data, key, level->beatsPerMinute, false, env, mod, set, false);
            }
            else BSML::MainThreadScheduler::Schedule([&]() { // ost needs to run on main thread cuz they do unity work in an async method :aaaaaaaaaaaa:
                mapDataTask = helper->_beatmapDataLoader->LoadBeatmapDataAsync(data, key, level->beatsPerMinute, false, env, mod, set, false);
            });
            while(!mapDataTask || !mapDataTask->get_IsCompleted()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            IReadonlyBeatmapData* beatmapData = mapDataTask->get_ResultOnSuccess();
            if (!beatmapData) return;
            BSML::MainThreadScheduler::Schedule([=]() { // RUNNING THIS OFF THE MAIN THREAD CAUSES AN UNRELATED CRASH SOMEWHERE ELSE WHAT!!!!?!?!?!?!?!
                int maxScoreOmg = ScoreModel::ComputeMaxMultipliedScoreForBeatmap(beatmapData);
                if (maxScoreOmg != -1) cacheMaxScoreData(key, maxScoreOmg);
                std::lock_guard<std::mutex> lock(mtx);
                if (localGuid != guid) return;
                gotMaxScoreResult(maxScoreOmg, key);
            });
        }).detach();
    }

    void updateMapData(BeatmapKey* beatmapKey, bool forced){
        if (scoreDetailsUI != nullptr) scoreDetailsUI->updateInfo("loading...");
        updateBasicMapInfo(beatmapKey);
        mapData.maxScore = RetrieveMaxScoreDataFromCache();
        if (mapData.maxScore == -1 && forced) return getMaxScoreForBeatmapAsync(*beatmapKey);
        gotMaxScoreResult(mapData.maxScore, *beatmapKey);
    }

    void updateBasicMapInfo(BeatmapKey* beatmapKey){
        auto* playerLevelStatsData = getPlayerData()->GetOrCreatePlayerLevelStatsData(ByRef<BeatmapKey>(beatmapKey));
        int currentScore = playerLevelStatsData->_highScore;
        std::string mapID = playerLevelStatsData->_levelID;
        std::string mapType = playerLevelStatsData->get_beatmapCharacteristic()->get_serializedName();
        mapData.mapID = mapID;
        int diff = (beatmapKey->difficulty.value__ + 1) * 2 - 1;
        mapData.currentScore = currentScore;
        mapData.diff = beatmapKey->difficulty.value__;
        mapData.mapType = mapType;
        mapData.isFC = playerLevelStatsData->_fullCombo;
        mapData.playCount = playerLevelStatsData->_playCount;
        mapData.maxCombo = playerLevelStatsData->_maxCombo;
        mapData.idString = mapType.compare("Standard") != 0 ? mapType + std::to_string(diff) : std::to_string(diff);
        mapData.key = *beatmapKey;
    }
}