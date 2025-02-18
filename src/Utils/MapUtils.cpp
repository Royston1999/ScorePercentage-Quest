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
#include "main.hpp"
#include "Utils/HttpUtils.hpp"

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
        int maxScore = RetrieveMaxScoreDataFromCache();
        if (maxScore != -1) return gotMaxScoreResult(maxScore, mapData.key);
        maxScore = ScoreModel::ComputeMaxMultipliedScoreForBeatmap(beatmapData);
        if (maxScore == -1) return;
        cacheMaxScoreData(mapData.key, maxScore);
        gotMaxScoreResult(maxScore, mapData.key);
    }

    task_coroutine<IReadonlyBeatmapData*> getBeatmapDataAsync(BeatmapKey key) {
        auto helper = getTransitionsHelper();
        auto playerData = getPlayerData();
        BeatmapLevel* level = helper->_beatmapLevelsModel->GetBeatmapLevel(key.levelId);
        bool isCustom = key.levelId->StartsWith("custom_level_");
        LoadBeatmapLevelDataResult levelDataResult = co_await helper->_beatmapLevelsModel->LoadBeatmapLevelDataAsync(key.levelId, nullptr);
        if (levelDataResult.isError) co_return nullptr;

        IBeatmapLevelData* levelData = levelDataResult.beatmapLevelData;
        auto env = playerData->get_overrideEnvironmentSettings()->GetOverrideEnvironmentInfoForType(EnvironmentType::Normal).cast<IEnvironmentInfo>();
        auto mod = playerData->get_gameplayModifiers();
        auto set = playerData->get_playerSpecificSettings();
        
        if (!isCustom) co_await YieldMainThread();
        IReadonlyBeatmapData* beatmapData = co_await helper->_beatmapDataLoader->LoadBeatmapDataAsync(levelData, key, level->beatsPerMinute, false, env, mod, set, false);
        co_return beatmapData;
    }

    task_coroutine<void> getMaxScoreCoro(BeatmapKey key) {
        static std::atomic<uint32_t> guid = 0; // the user doesn't have over 4 billion songs right :surely:
        uint32_t localGuid = ++guid;

        IReadonlyBeatmapData* beatmapData = co_await getBeatmapDataAsync(key);
        co_await YieldMainThread();

        if (!beatmapData) {
            if (localGuid != guid) co_return;
            co_return gotMaxScoreResult(-1, key);
        }
        int maxScoreOmg = ScoreModel::ComputeMaxMultipliedScoreForBeatmap(beatmapData);
        if (maxScoreOmg != -1) cacheMaxScoreData(key, maxScoreOmg);

        if (localGuid != guid) co_return;
        co_return gotMaxScoreResult(maxScoreOmg, key);
    }

    DECLARE_JSON_CLASS(person, 
        NAMED_VALUE(std::string, username, "username")
        NAMED_VALUE(int, typingSpeed, "typing_speed")

        person() = default;
        person(const std::string& u, int t) : username(u), typingSpeed(t) {}
    )

    DECLARE_JSON_CLASS(request_content, 
        NAMED_VALUE(bool, return_error, "return_error")
        NAMED_VALUE(person, user, "user")

        request_content(bool error, const person& person) : return_error(error), user(person) {}
    )

    DECLARE_JSON_CLASS(error_content,
        NAMED_VALUE_DEFAULT(std::string, error, "", "error")
        NAMED_VALUE_DEFAULT(std::string, details, "", "details")
    )

    task_coroutine<void> testhttp() {
        
        const std::string test_url = "http://192.168.1.35:8080/test-post-endpoint-error";
        const HttpService::HeaderMap headers = {{"User-Agent", MOD_ID " " VERSION}, {"Content-Type", "application/json"}};
       
        request_content successReuqest{false, {"bob bumder", 506843}};
        HttpResponse<> sucessResponse = co_await HttpService::PostAsync(test_url, successReuqest, headers);
        if (sucessResponse.success) {
            getLogger().info("code: {}, content: {}", sucessResponse.responseCode, sucessResponse.content);
        }
        
        request_content errorReuqest{true, {"jenna talia", 13}};
        HttpResponse<std::vector<uint8_t>, error_content> errorResponse = co_await HttpService::PostAsync<std::vector<uint8_t>, error_content>(test_url, errorReuqest, headers);
        if (!errorResponse.success) {
            getLogger().info("code: {}, error: {}, reason: {}", errorResponse.responseCode, errorResponse.errorContent.error, errorResponse.errorContent.details);
        }

        auto imageBytes = co_await HttpService::GetAsync<std::vector<uint8_t>>("https://cdnjs.cloudflare.com/ajax/libs/twemoji/14.0.2/72x72/1f1ec-1f1e7.png");

        co_await HttpService::PostAsync("http://192.168.1.35:8080/test-post-bytes", imageBytes.content);
    }

    void getMaxScoreForBeatmapAsync(BeatmapKey key) {
        getLogger().info("hi this is the main thread");
        getMaxScoreCoro(key);
        // testhttp();
    }

    void updateMapData(BeatmapKey* beatmapKey, bool forced){
        if (scoreDetailsUI != nullptr) scoreDetailsUI->updateInfo("loading...");
        updateBasicMapInfo(beatmapKey);
        mapData.maxScore = RetrieveMaxScoreDataFromCache();
        if (mapData.maxScore == -1 && forced) return getMaxScoreForBeatmapAsync(*beatmapKey);
        else if (mapData.maxScore != -1) gotMaxScoreResult(mapData.maxScore, *beatmapKey);
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