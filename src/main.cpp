#include <iomanip>
#include <cmath>
#include "questui/shared/QuestUI.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "System/Action.hpp"
#include "System/DateTime.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "GlobalNamespace/ScoreFormatter.hpp"
#include "GlobalNamespace/ResultsViewController.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "GlobalNamespace/GamePause.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "GlobalNamespace/PracticeSettings.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/LevelStatsView.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/MultiplayerResultsViewController.hpp"
#include "GlobalNamespace/ResultsTableView.hpp"
#include "GlobalNamespace/ResultsTableCell.hpp"
#include "GlobalNamespace/IConnectedPlayer.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/RankModel.hpp"
#include "ScoreDetailsModal.hpp"
#include "ScoreUtils.hpp"
#include "SettingsFlowCoordinator.hpp"
#include "PPCalculator.hpp"
#include "main.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/MultiplayerScoreRingItem.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/MultiplayerScoreRingManager.hpp"
#include "GlobalNamespace/RelativeScoreAndImmediateRankCounter.hpp"
#include "GlobalNamespace/IScoreController.hpp"
#include "GlobalNamespace/MultiplayerLocalActiveClient.hpp"
#include "GlobalNamespace/MultiplayerScoreProvider.hpp"
#include "GlobalNamespace/MultiplayerScoreProvider_RankedPlayer.hpp"
#include "GlobalNamespace/MultiplayerConnectedPlayerScoreDiffText.hpp"
#include "TMPro/TextMeshPro.hpp"
#include "UnityEngine/SpriteRenderer.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "GlobalNamespace/DifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapLevelData.hpp"
#include "GlobalNamespace/HMCache_2.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Linq/Enumerable.hpp"
#include "System/Collections/Generic/IEnumerable_1.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Func_2.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "System/Collections/Generic/Queue_1.hpp"
#include "GlobalNamespace/IBeatmapDataAssetFileModel.hpp"

using namespace QuestUI::BeatSaberUI;
using namespace UnityEngine::UI;
using namespace UnityEngine;
using namespace GlobalNamespace;
using namespace ScorePercentage::Utils;

ScorePercentageConfig scorePercentageConfig;
ModInfo modInfo;
ScorePercentage::ModalPopup* scoreDetailsUI = nullptr;
int pauseCount = 0;
bool modalSettingsChanged = false;
BeatMapData mapData;
Il2CppString* origRankText = nullptr;
TMPro::TextMeshProUGUI* scoreDiffText = nullptr;
TMPro::TextMeshProUGUI* rankDiffText = nullptr;
IScoreController* scoreController = nullptr;
std::string nitoID = "QXzRo+cwkPArBeq+0i4yxw";
std::string carlosID = "EjTRmgT3gfbSZZ8tz6As90";
std::string myID = "JWSt5qMrClC7flwEEKi8hl";
std::string playerDataPath = "/sdcard/Android/data/com.beatgames.beatsaber/files/PlayerData.dat";
bool hasBeenNitod;
custom_types::Helpers::Coroutine FuckYouBeatSaviorData(LevelStatsView* self);

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load(); // Load the config file
    getLogger().info("Completed setup!");
}

void loadConfig() {
    getConfig().Load();
    ConfigHelper::LoadConfig(scorePercentageConfig, getConfig().config);
}

custom_types::Helpers::Coroutine FuckYouBeatSaviorData(LevelStatsView* self)
    {
        bool beatBeyondSaving = false;
        for (int i=0; i<3; i++){
            if (i == 2){
                auto beatMySavior = QuestUI::ArrayUtil::First(self->get_transform()->get_parent()->GetComponentsInChildren<Button*>(), [](Button* x) { return to_utf8(csstrtostr(x->get_name())) == "QuestUIButton"; });
                if (beatMySavior && beatMySavior->get_gameObject()->get_active()){
                    beatBeyondSaving = true;
                    scoreDetailsUI->openButton->GetComponent<RectTransform*>()->set_anchoredPosition({-47.0f, 10.0f});
                }
            }
            else co_yield nullptr;
        }
        if (!beatBeyondSaving) scoreDetailsUI->openButton->GetComponent<RectTransform*>()->set_anchoredPosition({-47.0f, 0.0f});
        co_return;
    }

void updateMapData(PlayerLevelStatsData* playerLevelStatsData, IDifficultyBeatmap* difficultyBeatmap){
    int currentScore = playerLevelStatsData->highScore;
    std::string mapID = to_utf8(csstrtostr(playerLevelStatsData->get_levelID()));
    std::string mapType = to_utf8(csstrtostr(playerLevelStatsData->get_beatmapCharacteristic()->get_serializedName()));
    int diff = difficultyBeatmap->get_difficultyRank();
    int maxScore = calculateMaxScore(difficultyBeatmap->get_beatmapData()->cuttableNotesCount);
    float currentPercentage = calculatePercentage(maxScore, currentScore);
    mapData.currentScore = currentScore;
    mapData.currentPercentage = currentPercentage;
    mapData.maxScore = maxScore;
    mapData.mapID = mapID;
    mapData.diff = difficultyBeatmap->get_difficulty();
    mapData.mapType = mapType;
    mapData.isFC = playerLevelStatsData->fullCombo;
    mapData.playCount = playerLevelStatsData->playCount;
    mapData.maxCombo = playerLevelStatsData->maxCombo;
    mapData.idString = mapType.compare("Standard") != 0 ? mapType + std::to_string(diff) : std::to_string(diff);
}

void toggleMultiResultsTableFormat(bool value, ResultsTableCell* cell){
    cell->rankText->set_enableWordWrapping(!value);
    cell->rankText->set_richText(value);
    cell->scoreText->set_richText(value);
    cell->scoreText->set_enableWordWrapping(!value);
    int multiplier = value ? 1 : -1;
    cell->scoreText->get_transform()->set_localPosition(cell->scoreText->get_transform()->get_localPosition() + UnityEngine::Vector3(multiplier * -5, 0, 0));
    cell->rankText->get_transform()->set_localPosition(cell->rankText->get_transform()->get_localPosition() + UnityEngine::Vector3(multiplier * -2, 0, 0));
}

void toggleModalVisibility(bool value, LevelStatsView* self){
    // self->get_gameObject()->set_active(!value);
    scoreDetailsUI->openButton->get_gameObject()->SetActive(value);
    if (value) scoreDetailsUI->updateInfo();
    else scoreDetailsUI->modal->Hide(true, nullptr);
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(reinterpret_cast<custom_types::Helpers::enumeratorT*>(custom_types::Helpers::CoroutineHelper::New(FuckYouBeatSaviorData(self))));
}

void createDifferenceTexts(ResultsViewController* self){
    scoreDiffText = Object::Instantiate(self->rankText, self->rankText->get_transform()->get_parent());
    rankDiffText = Object::Instantiate(self->rankText, self->rankText->get_transform()->get_parent());
    auto scoreTextPos = self->scoreText->get_transform()->get_localPosition();
    auto rankTextPos = self->rankText->get_transform()->get_localPosition();
    scoreDiffText->get_transform()->set_localPosition(Vector3(scoreTextPos.x - 1.0f, rankTextPos.y - 4.5f, scoreTextPos.z));
    rankDiffText->get_transform()->set_localPosition(Vector3(rankTextPos.x + 1.0f, rankTextPos.y - 4.5f, rankTextPos.z));
    scoreDiffText->set_enableWordWrapping(false);
}

MAKE_HOOK_MATCH(Menu, &LevelStatsView::ShowStats, void, LevelStatsView* self, IDifficultyBeatmap* difficultyBeatmap, PlayerData* playerData) {
    Menu(self, difficultyBeatmap, playerData);
    if (playerData != nullptr)
    {
        if (scoreDetailsUI == nullptr || modalSettingsChanged) ScorePercentage::initModalPopup(&scoreDetailsUI, self->get_transform());
        auto* playerLevelStatsData = playerData->GetPlayerLevelStatsData(difficultyBeatmap);
        updateMapData(playerLevelStatsData, difficultyBeatmap);
        if (playerLevelStatsData->validScore)
        {
            ConfigHelper::LoadBeatMapInfo(mapData.mapID, mapData.idString);
            if (scorePercentageConfig.MenuHighScore) toggleModalVisibility(true, self);
        }
        if (!scorePercentageConfig.MenuHighScore || !playerLevelStatsData->validScore) toggleModalVisibility(false, self);
    }
}

MAKE_HOOK_MATCH(Pause, &GamePause::Pause, void, GamePause* self) {
    Pause(self);
    pauseCount++;
}

MAKE_HOOK_MATCH(Results, &ResultsViewController::DidActivate, void, ResultsViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    Results(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    int resultScore;
    double resultPercentage;

    // disable score differences in party mode cuz idk how that's supposed to work (still shows rank as percentage tho) 
    auto* flowthingy = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<PartyFreePlayFlowCoordinator*>());
    bool isParty = flowthingy != nullptr ? flowthingy->isActivated ? true : false : false;

    // funny thing i wonder if anyone notices
    if (firstActivation) CreateText(self->get_transform(), "<size=150%>KNOBHEAD</size>", UnityEngine::Vector2(20, 20));

    // Default Info Texts
    std::string rankText = to_utf8(csstrtostr(self->rankText->get_text()));
    std::string scoreText = to_utf8(csstrtostr(self->scoreText->get_text()));
    std::string missText = to_utf8(csstrtostr(self->goodCutsPercentageText->get_text()));

    bool isValidScore = !(self->get_practice() || mapData.currentScore == 0 || isParty);

    // only update stuff if level was cleared
    if (self->levelCompletionResults->levelEndStateType == GlobalNamespace::LevelCompletionResults::LevelEndStateType::Cleared)
    {
        if (scoreDiffText == nullptr) createDifferenceTexts(self);

        resultScore = self->levelCompletionResults->modifiedScore;
        resultPercentage = calculatePercentage(mapData.maxScore, resultScore);

        // probably stops stuff from breaking
        self->rankText->set_autoSizeTextContainer(false);
        self->rankText->set_enableWordWrapping(false);
        self->goodCutsPercentageText->set_enableWordWrapping(false);
        
        auto* rankTitleText = self->get_transform()->Find(il2cpp_utils::newcsstr("Container/ClearedInfo/RankTitle"))->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if (origRankText == nullptr) origRankText = rankTitleText->get_text();
        // rank text
        if (scorePercentageConfig.LevelEndRank)
        {
            rankTitleText->SetText(il2cpp_utils::newcsstr("Percentage"));
            rankText = "  " + Round(std::abs(resultPercentage), 2) + "<size=45%>%";
        }
        else rankTitleText->SetText(origRankText);

        // percentage difference text
        if (scorePercentageConfig.ScorePercentageDifference && isValidScore){
            std::string rankDiff = valueDifferenceString(resultPercentage - mapData.currentPercentage);
            std::string sorry = scorePercentageConfig.LevelEndRank ? "" : "  ";
            rankText = createScoreText(sorry + rankText);
            rankDiffText->get_gameObject()->SetActive(true);
            rankDiffText->SetText(il2cpp_utils::newcsstr("<size=40%>" + rankDiff + "<size=30%>%"));
        }
        else rankDiffText->get_gameObject()->SetActive(false);
        self->rankText->SetText(il2cpp_utils::newcsstr(rankText));

        // score difference text
        if (scorePercentageConfig.ScoreDifference && isValidScore)
        {
            self->newHighScoreText->SetActive(false);
            std::string formatting = "<size=40%>" + ((resultScore - mapData.currentScore >= 0) ? positiveColour + "+" : negativeColour); 
            scoreDiffText->get_gameObject()->SetActive(true);
            scoreDiffText->SetText(il2cpp_utils::newcsstr(formatting  + to_utf8(csstrtostr(ScoreFormatter::Format(resultScore - mapData.currentScore)))));
            self->scoreText->SetText(il2cpp_utils::newcsstr(createScoreText(" " + scoreText)));
        }
        else scoreDiffText->get_gameObject()->SetActive(false);

        // miss difference text
        if(scorePercentageConfig.missDifference && scorePercentageConfig.missCount != -1 && isValidScore)
        {
            int currentMisses = scorePercentageConfig.badCutCount + scorePercentageConfig.missCount;
            int resultMisses = self->levelCompletionResults->missedCount + self->levelCompletionResults->badCutsCount;
            self->goodCutsPercentageText->SetText(il2cpp_utils::newcsstr(createMissText(missText, currentMisses - resultMisses)));
        }
        else self->goodCutsPercentageText->SetText(il2cpp_utils::newcsstr(missText));

        // write new highscore to file
        if ((resultScore - mapData.currentScore) > 0 && !self->get_practice() && !isParty)
        {
            int misses = self->levelCompletionResults->missedCount;
            int badCut = self->levelCompletionResults->badCutsCount;
            std::string currentTime = to_utf8(csstrtostr(System::DateTime::get_UtcNow().ToLocalTime().ToString(il2cpp_utils::newcsstr("D"))));
            ConfigHelper::UpdateBeatMapInfo(mapData.mapID, mapData.idString, misses, badCut, pauseCount, currentTime);
        }
    }
}

MAKE_HOOK_MATCH(MultiplayerResults, &ResultsTableCell::SetData, void, ResultsTableCell* self, int order, IConnectedPlayer* connectedPlayer, LevelCompletionResults* levelCompletionResults){
    MultiplayerResults(self, order, connectedPlayer, levelCompletionResults);
    bool passedLevel = levelCompletionResults->levelEndStateType == 1 ? true : false;
    if (scorePercentageConfig.LevelEndRank){
        if (!self->rankText->get_richText()) toggleMultiResultsTableFormat(true, self);
        bool isNoFail = levelCompletionResults->gameplayModifiers->get_noFailOn0Energy() && levelCompletionResults->energy == 0;
        std::string percentageText = Round(calculatePercentage(mapData.maxScore, levelCompletionResults->modifiedScore), 2);
        self->rankText->SetText(il2cpp_utils::newcsstr(percentageText + "<size=75%>%</size>"));
        std::string score = to_utf8(csstrtostr(self->scoreText->get_text()));
        std::string preText = isNoFail ? "NF" + tab : !passedLevel ? "F" + tab : "";
        int totalMisses = levelCompletionResults->missedCount + levelCompletionResults->badCutsCount;
        std::string missText = levelCompletionResults->fullCombo ? "FC" : preText + "<color=red>X</color><size=65%> </size>" + std::to_string(totalMisses);
        self->scoreText->SetText(il2cpp_utils::newcsstr(missText + tab + score));
    }
    else{
        if (self->rankText->get_richText()) toggleMultiResultsTableFormat(false, self);
        if (!passedLevel) self->rankText->SetText(il2cpp_utils::newcsstr("F"));
        else self->rankText->SetText(GlobalNamespace::RankModel::GetRankName(levelCompletionResults->rank));
    }
    if (connectedPlayer->get_isMe() && (levelCompletionResults->modifiedScore - mapData.currentScore > 0) && passedLevel){
        int misses = levelCompletionResults->missedCount;
        int badCut = levelCompletionResults->badCutsCount;
        std::string currentTime = to_utf8(csstrtostr(System::DateTime::get_UtcNow().ToLocalTime().ToString(il2cpp_utils::newcsstr("D"))));
        ConfigHelper::UpdateBeatMapInfo(mapData.mapID, mapData.idString, misses, badCut, pauseCount, currentTime);
    }
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(GameplayCoreSceneSetupData_ctor, "", "GameplayCoreSceneSetupData", ".ctor", void, GameplayCoreSceneSetupData* self, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects, EnvironmentInfoSO* environmentInfo, ColorScheme* colorScheme)
{
    GameplayCoreSceneSetupData_ctor(self, difficultyBeatmap, previewBeatmapLevel, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects, environmentInfo, colorScheme);
    auto* playerLevelStatsData = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<PlayerDataModel*>())->playerData->GetPlayerLevelStatsData(difficultyBeatmap);
    updateMapData(playerLevelStatsData, difficultyBeatmap);
    pauseCount = 0;
    if (scoreDetailsUI != nullptr) scoreDetailsUI->modal->Hide(true, nullptr);
    scoreController = nullptr;
    hasBeenNitod = false;
}

MAKE_HOOK_MATCH(PPTime, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    PPTime(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    if (firstActivation){
        PPCalculator::PP::Initialize();

        std::function<void()> onClick = [](){
            auto* mapModel = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<BeatmapLevelsModel*>());
            auto loadedMaps = mapModel->loadedBeatmapLevels;
            
            int highScoreREAL = 0;
            std::string levelIdREAL = "";
            int diffREAL = 0;
            ConfigDocument d;
            if(!parsejsonfile(d, playerDataPath)) {
                getLogger().info("how tf do you not have player data??");
                return;
            }
            auto array = d.FindMember("localPlayers")->value.GetArray()[0].FindMember("levelsStatsData")->value.GetArray();
            for (int i = 0; i < array.Size(); i++) {
                std::string levelID = array[i].FindMember("levelId")->value.GetString();
                int highScore = array[i].FindMember("highScore")->value.GetInt();
                int diff = array[i].FindMember("difficulty")->value.GetInt();
                if (highScore > highScoreREAL){
                    highScoreREAL = highScore;
                    levelIdREAL = levelID;
                    diffREAL = diff;
                }
                if (levelID.starts_with("custom_level_")){
                    auto* previewInfo = mapModel->GetLevelPreviewForLevelId(il2cpp_utils::newcsstr(levelID));
                    auto* map = mapModel->GetBeatmapLevelIfLoaded(il2cpp_utils::newcsstr(levelID));
                    if (map != nullptr && previewInfo != nullptr){
                        getLogger().info("the following map is loaded");
                        std::string songName = to_utf8(csstrtostr(previewInfo->get_songName()));
                        getLogger().info("song name that is about to fuck me over: %s", songName.c_str());
                        mapModel->beatmapDataAssetFileModel->TryDeleteAssetBundleFileForPreviewLevelAsync(previewInfo, System::Threading::CancellationToken::get_None());
                        reinterpret_cast<System::Object*>(map)->Finalize();
                    }
                }
            }
            loadedMaps->addedElements->Clear();
            loadedMaps->cache->Clear();
            loadedMaps->Clear();
            mapModel->ClearLoadedBeatmapLevelsCaches();
            mapModel->UpdateLoadedPreviewLevels();

            auto* previewInfo = mapModel->GetLevelPreviewForLevelId(il2cpp_utils::newcsstr(levelIdREAL));
            std::string songName = to_utf8(csstrtostr(previewInfo->get_songName()));
            std::string mappeName = to_utf8(csstrtostr(previewInfo->get_levelAuthorName()));
            std::string difficultyName;
            switch (diffREAL){
                case 0:
                    difficultyName = "Easy"; break;
                case 1:
                    difficultyName = "Normal"; break;
                case 2:
                    difficultyName = "Hard"; break;
                case 3:
                    difficultyName = "Expert"; break;
                case 4:
                    difficultyName = "Expert+"; break;
            }
            getLogger().info("HIGHEST SCORE: %s, Mapper: %s, Difficulty: %s, Score: %i", songName.c_str(), mappeName.c_str(), difficultyName.c_str(), highScoreREAL);
        };
        self->howToPlayButton->get_onClick()->AddListener(il2cpp_utils::MakeDelegate<UnityEngine::Events::UnityAction*>(classof(UnityEngine::Events::UnityAction*), onClick));
    }
}

MAKE_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &GlobalNamespace::MenuTransitionsHelper::RestartGame, void, GlobalNamespace::MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback)
{
    delete scoreDetailsUI;
    scoreDetailsUI = nullptr;
    origRankText = nullptr;
    scoreDiffText = nullptr;
    rankDiffText = nullptr;
    MenuTransitionsHelper_RestartGame(self, finishCallback);
}

MAKE_HOOK_MATCH(ScoreRingManager_UpdateScoreText, &MultiplayerScoreRingManager::UpdateScore, void, MultiplayerScoreRingManager* self, IConnectedPlayer* playerToUpdate){
    MultiplayerScoreProvider::RankedPlayer* player;
    
    auto* scoreRingItem = self->GetScoreRingItem(playerToUpdate->get_userId());
    if (!hasBeenNitod && to_utf8(csstrtostr(playerToUpdate->get_userId())).compare(nitoID) == 0){
        hasBeenNitod = true;
        scoreRingItem->SetName(il2cpp_utils::newcsstr("MUNCHKIN"));
    }
    bool flag = self->scoreProvider->TryGetScore(playerToUpdate->get_userId(), player);
    if (!flag || player->get_isFailed()){
        scoreRingItem->SetScore(il2cpp_utils::newcsstr("X"));
        return;
	}
    if (scoreController != nullptr){
        int userScore = player->get_score();
        int maxPossibleScore = scoreController->get_immediateMaxPossibleRawScore();
        std::string userPercentage = Round(calculatePercentage(maxPossibleScore, userScore), 2);
        scoreRingItem->SetScore(il2cpp_utils::newcsstr(std::to_string(userScore) + " (" + userPercentage + "%)"));
    }
}

MAKE_HOOK_MATCH(ScoreDiffText, &MultiplayerConnectedPlayerScoreDiffText::AnimateScoreDiff, void, MultiplayerConnectedPlayerScoreDiffText* self, int scoreDiff){
    ScoreDiffText(self, scoreDiff);
    if (scoreController != nullptr){
        if(self->onPlatformText->get_enableWordWrapping()){
            self->onPlatformText->set_richText(true);
            self->onPlatformText->set_enableWordWrapping(false);
            auto* transform = (UnityEngine::RectTransform*)(self->backgroundSpriteRenderer->get_transform());
            transform->set_localScale({transform->get_localScale().x *1.7f, transform->get_localScale().y});
        }
        std::string baseText = to_utf8(csstrtostr(self->onPlatformText->get_text()));
        int maxPossibleScore = scoreController->get_immediateMaxPossibleRawScore();
        std::string posneg = (scoreDiff >= 0) ? "+" : "";
        std::string percentageText = " (" + posneg + Round(calculatePercentage(maxPossibleScore, scoreDiff), 2) + "%)";
        self->onPlatformText->SetText(il2cpp_utils::newcsstr(baseText + percentageText));
    }
}

MAKE_HOOK_MATCH(ScoreStuff, &MultiplayerLocalActiveClient::ScoreControllerHandleImmediateMaxPossibleScoreDidChange, void, MultiplayerLocalActiveClient* self, int immediateMaxPossibleRawScore, int immediateMaxPossibleModifiedScore){
    ScoreStuff(self, immediateMaxPossibleRawScore, immediateMaxPossibleModifiedScore);
    if (scoreController == nullptr) scoreController = self->scoreController;
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    loadConfig();
    QuestUI::Init();
    custom_types::Register::AutoRegister();
    QuestUI::Register::RegisterModSettingsFlowCoordinator<ScoreDetailsUI::SettingsFlowCoordinator*>(modInfo);
    // QuestUI::Register::RegisterModSettingsViewController<ScoreDetailsUI::UIController*>(modInfo);
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), Menu);
    INSTALL_HOOK(getLogger(), Results);
    INSTALL_HOOK(getLogger(), MultiplayerResults);
    INSTALL_HOOK(getLogger(), Pause);
    INSTALL_HOOK(getLogger(), PPTime);
    INSTALL_HOOK(getLogger(), GameplayCoreSceneSetupData_ctor)
    INSTALL_HOOK(getLogger(), MenuTransitionsHelper_RestartGame);
    INSTALL_HOOK(getLogger(), ScoreRingManager_UpdateScoreText);
    INSTALL_HOOK(getLogger(), ScoreStuff);
    INSTALL_HOOK(getLogger(), ScoreDiffText);
    getLogger().info("Installed all hooks!");
}