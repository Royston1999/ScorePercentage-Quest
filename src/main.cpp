#include <cmath>

#include "main.hpp"

#include "ScoreDetailsModal.hpp"
#include "MultiplayerHooks.hpp"
#include "PPCalculator.hpp"

#include "UI/SettingsFlowCoordinator.hpp"

#include "Utils/ScoreUtils.hpp"
#include "Utils/MapUtils.hpp"
#include "Utils/EasyDelegate.hpp"

#include "bs-utils/shared/utils.hpp"

#include "System/DateTime.hpp"
#include "System/Nullable_1.hpp"

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
#include "GlobalNamespace/LevelStatsView.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/GameplayModifiersModelSO.hpp"
#include "GlobalNamespace/LevelCompletionResultsHelper.hpp"
#include "GlobalNamespace/PauseMenuManager.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"

#include "BeatSaber/PerformancePresets/PerformancePreset.hpp"

#include "bsml/shared/BSML.hpp"
#include "bsml/shared/Helpers/creation.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"

using namespace UnityEngine::UI;
using namespace UnityEngine;
using namespace GlobalNamespace;
using namespace ScorePercentage::Utils;
using namespace BeatSaber::PerformancePresets;

ScorePercentageConfig scorePercentageConfig;
modloader::ModInfo modInfo{MOD_ID, VERSION, 0};
ScorePercentage::ModalPopup* scoreDetailsUI = nullptr;
int pauseCount = 0;
BeatMapData mapData;
Il2CppString* cachedResultsRankText = nullptr;
Il2CppString* cachedMaxRankText = nullptr;
TMPro::TextMeshProUGUI* scoreDiffText = nullptr;
TMPro::TextMeshProUGUI* rankDiffText = nullptr;
bool successfullySkipped = false;
bool leaderboardFirstActivation = false;
bool validResults;
bool finishedLoading;
int loadingStatus = 0;
extern bool isMapDataValid;

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
const Paper::ConstLoggerContext<16UL>& getLogger() {
    static constexpr auto Logger = Paper::ConstLoggerContext(MOD_ID);
    return Logger;
}

// Called at the early stages of game loading
SCORE_PERCENTAGE_EXPORT_FUNC void setup(CModInfo& info) {
    info = modInfo.to_c();

    getConfig().Load();
    getLogger().info("Completed setup!");
}

void loadConfig() {
    getConfig().Load();
    ConfigHelper::LoadConfig(scorePercentageConfig, getConfig().config);
}

void toggleModalVisibility(bool value, LevelStatsView* self){
    scoreDetailsUI->openButton->get_gameObject()->SetActive(value);
    scoreDetailsUI->hasValidScoreData = value;
    if (!value) return scoreDetailsUI->modal->Hide(true, nullptr);
    else if (self->get_isActiveAndEnabled() && GET_VALUE(alwaysOpen) && !scoreDetailsUI->modal->_isShown) {
        scoreDetailsUI->updateInfo("lodaing...");
        scoreDetailsUI->modal->Show(true, true, EasyDelegate::MakeDelegate<System::Action*>([](){
            scoreDetailsUI->updateInfo(finishedLoading ? "" : "loading...");
        }));
    }
}

void createDifferenceTexts(ResultsViewController* self){
    scoreDiffText = Object::Instantiate(self->_rankText, self->_rankText->get_transform()->get_parent());
    rankDiffText = Object::Instantiate(self->_rankText, self->_rankText->get_transform()->get_parent());
    auto scoreTextPos = self->_scoreText->get_transform()->get_localPosition();
    auto rankTextPos = self->_rankText->get_transform()->get_localPosition();
    scoreDiffText->get_transform()->set_localPosition(Vector3(scoreTextPos.x - 1.0f, rankTextPos.y - 4.5f, scoreTextPos.z));
    rankDiffText->get_transform()->set_localPosition(Vector3(rankTextPos.x + 1.0f, rankTextPos.y - 4.5f, rankTextPos.z));
    scoreDiffText->set_enableWordWrapping(false);
}

void cacheTextAfterLocalisation(TMPro::TextMeshProUGUI* rankTitle) {
    cachedMaxRankText = rankTitle->get_text();
    if (GET_VALUE(showPercentageInMenu)) rankTitle->set_text("Percentage");
    leaderboardFirstActivation = false;
}

MAKE_HOOK_MATCH(Menu, &LevelStatsView::ShowStats, void, LevelStatsView* self, ByRef<BeatmapKey> beatmapKey, PlayerData* playerData) {
    Menu(self, beatmapKey, playerData);
    finishedLoading = false;
    validResults = false;
    
    if (playerData != nullptr)
    {
        auto rankTitle = self->get_transform()->Find("MaxRank/Title")->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if (leaderboardFirstActivation) BSML::MainThreadScheduler::ScheduleAfterTime(0.4f, [=](){ cacheTextAfterLocalisation(rankTitle); });
        else rankTitle->set_text(GET_VALUE(showPercentageInMenu) ? "Percentage" : StringW(cachedMaxRankText));
        if (!scoreDetailsUI || scoreDetailsUI->modalSettingsChanged) ScorePercentage::initModalPopup(&scoreDetailsUI, self->get_transform());

        BeatmapKey* key = reinterpret_cast<BeatmapKey*>(beatmapKey.convert());
        scoreDetailsUI->currentMap = key;
        scoreDetailsUI->playerData = playerData;

        auto* playerLevelStatsData = playerData->GetOrCreatePlayerLevelStatsData(beatmapKey);

        ScorePercentage::MapUtils::updateMapData(key, true);
        ConfigHelper::LoadBeatMapInfo(mapData.mapID, mapData.idString);
        
        bool enablePopup = GET_VALUE(enablePopup) && playerLevelStatsData->get_validScore() && playerLevelStatsData->_highScore > 0;
        toggleModalVisibility(enablePopup, self);
    }
}

MAKE_HOOK_MATCH(Pause, &GamePause::Pause, void, GamePause* self) {
    Pause(self);
    pauseCount++;
}

MAKE_HOOK_MATCH(Restart, &PauseMenuManager::RestartButtonPressed, void, PauseMenuManager* self) {
    Restart(self);
    pauseCount = 0;
}

MAKE_HOOK_MATCH(TakeMeToResults_Fix, &LevelCompletionResultsHelper::ProcessScore, void, ByRef<BeatmapKey> beatmapKey, PlayerData* playerData, PlayerLevelStatsData* playerLevelStats, LevelCompletionResults* levelCompletionResults, IReadonlyBeatmapData* transformedBeatmapData, PlatformLeaderboardsModel* platformLeaderboardsModel){
    TakeMeToResults_Fix(beatmapKey, playerData, playerLevelStats, levelCompletionResults, transformedBeatmapData, platformLeaderboardsModel);
    if (levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::Cleared) validResults = true;
}

MAKE_HOOK_MATCH(Results, &ResultsViewController::DidActivate, void, ResultsViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    Results(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    int resultScore;
    double resultPercentage;

    if (!validResults && !self->get_practice()) return;
    validResults = false;
    // disable score differences in party mode cuz idk how that's supposed to work (still shows rank as percentage tho) 
    auto* flowthingy = Resources::FindObjectsOfTypeAll<PartyFreePlayFlowCoordinator*>().front_or_default();
    bool isParty = flowthingy != nullptr ? flowthingy->get_isActivated() ? true : false : false;

    if (firstActivation) self->_continueButton->get_onClick()->AddListener(EasyDelegate::MakeDelegate<Events::UnityAction*>([](){
        if (GET_VALUE(alwaysOpen) && scoreDetailsUI != nullptr && scoreDetailsUI->modal->_isShown) scoreDetailsUI->updateInfo();
    }));

#if !NO_GIMMICK
    // funny thing i wonder if anyone notices
    if (firstActivation) BSML::Helpers::CreateText(self->get_transform(), "<size=150%>KNOBHEAD</size>", Vector2(20, 20));
#endif

    // Default Info Texts
    std::string rankText = self->_rankText->get_text();
    std::string scoreText = self->_scoreText->get_text();
    std::string missText = self->_goodCutsPercentageText->get_text();

    bool isValidScore = !(self->get_practice() || mapData.currentScore == 0 || isParty);

    // only update stuff if level was cleared
    if (self->_levelCompletionResults->levelEndStateType != LevelCompletionResults::LevelEndStateType::Cleared) return;
    
    if (!scoreDiffText || !scoreDiffText->m_CachedPtr.m_value || !rankDiffText || !rankDiffText->m_CachedPtr.m_value) createDifferenceTexts(self);

    resultScore = self->_levelCompletionResults->modifiedScore;
    resultPercentage = CalculatePercentage(mapData.maxScore, resultScore);

    // probably stops stuff from breaking
    self->_rankText->set_autoSizeTextContainer(false);
    self->_rankText->set_enableWordWrapping(false);
    self->_goodCutsPercentageText->set_enableWordWrapping(false);
    
    auto* rankTitleText = self->get_transform()->Find("Container/ClearedInfo/RankTitle")->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
    if (firstActivation) cachedResultsRankText = rankTitleText->get_text();
    // rank text
    if (GET_VALUE(showPercentageOnResults))
    {
        rankTitleText->set_text("Percentage");
        // FONT SPACING FIX 1
        // rankText = "  " + Round(std::abs(resultPercentage), 2) + "<size=45%>%";
        rankText = "  " + createFixedNumberText(std::abs(resultPercentage), false) + "<size=45%>%";
    }
    else rankTitleText->SetText(cachedResultsRankText);

    // percentage difference text
    if (GET_VALUE(showPercentageDifference) && isValidScore){
        // FONT SPACING FIX 2
        // std::string rankDiff = valueDifferenceString(resultPercentage - mapData.currentPercentage);
        std::string rankDiff = createFixedNumberText(resultPercentage - mapData.currentPercentage, true);
        std::string sorry = GET_VALUE(showPercentageOnResults) ? "" : "  ";
        rankText = createScoreText(sorry + rankText);
        rankDiffText->get_gameObject()->SetActive(true);
        rankDiffText->SetText("<size=40%>" + rankDiff + "<size=30%>%", true);
    }
    else rankDiffText->get_gameObject()->SetActive(false);
    self->_rankText->set_text(rankText);

    // score difference text
    if (GET_VALUE(showScoreDifference) && isValidScore)
    {
        self->_newHighScoreText->SetActive(false);
        std::string formatting = "<size=40%>" + ((resultScore - mapData.currentScore >= 0) ? positiveColour + "+" : negativeColour);
        scoreDiffText->get_gameObject()->SetActive(true);
        // FONT SPACING FIX 3
        // scoreDiffText->set_text(StringW(formatting) + ScoreFormatter::Format(resultScore - mapData.currentScore));
        scoreDiffText->set_text(StringW(formatting) +  correctText(ScoreFormatter::Format(resultScore - mapData.currentScore)));
        self->_scoreText->set_text(createScoreText(" " + scoreText));
    }
    else {
        scoreDiffText->get_gameObject()->SetActive(false);
        self->_scoreText->set_text(scoreText);        
    }

    // miss difference text
    if(GET_VALUE(showMissDifference) && scorePercentageConfig.missCount != -1 && isValidScore)
    {
        int currentMisses = scorePercentageConfig.badCutCount + scorePercentageConfig.missCount;
        int resultMisses = self->_levelCompletionResults->missedCount + self->_levelCompletionResults->badCutsCount;
        self->_goodCutsPercentageText->set_text(createMissText(missText, currentMisses - resultMisses));
    }
    else self->_goodCutsPercentageText->set_text(missText);

    // write new highscore to file
    if ((resultScore - mapData.currentScore) > 0 && !self->get_practice() && !isParty && bs_utils::Submission::getEnabled())
    {
        int misses = self->_levelCompletionResults->missedCount;
        int badCut = self->_levelCompletionResults->badCutsCount;
        std::string currentTime = System::DateTime::get_UtcNow().ToLocalTime().ToString("D");
        ConfigHelper::UpdateBeatMapInfo(mapData.mapID, mapData.idString, misses, badCut, pauseCount, currentTime);
    }
}

MAKE_HOOK_MATCH(GameplayCoreSceneSetupData_ctor, static_cast<void(GameplayCoreSceneSetupData::*)(ByRef<BeatmapKey>, BeatmapLevel*, GameplayModifiers*, PlayerSpecificSettings*, PracticeSettings*, bool, EnvironmentInfoSO*, ColorScheme*, PerformancePreset*, AudioClipAsyncLoader*, BeatmapDataLoader*, bool, bool, System::Nullable_1<RecordingToolManager::SetupData>)>(&GameplayCoreSceneSetupData::_ctor), void, GameplayCoreSceneSetupData* self, ByRef<BeatmapKey> beatmapKey, BeatmapLevel* beatmapLevel, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects, EnvironmentInfoSO* environmentInfo, ColorScheme* colorScheme, PerformancePreset* performancePreset, AudioClipAsyncLoader* audioClipAsyncLoader, BeatmapDataLoader* beatmapDataLoader, bool enableBeatmapDataCaching, bool allowNullBeatmapLevelData, System::Nullable_1<RecordingToolManager::SetupData> recordingToolData)
{
    GameplayCoreSceneSetupData_ctor(self, beatmapKey, beatmapLevel, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects, environmentInfo, colorScheme, performancePreset, audioClipAsyncLoader, beatmapDataLoader, enableBeatmapDataCaching, allowNullBeatmapLevelData, recordingToolData);
    ScorePercentage::MapUtils::updateBasicMapInfo(reinterpret_cast<BeatmapKey*>(beatmapKey.convert()));
    isMapDataValid = true;
    pauseCount = 0;
    if (scoreDetailsUI != nullptr) scoreDetailsUI->modal->Hide(true, nullptr);
    // multiplayer tomfoolery
    auto* modifierModel = Resources::FindObjectsOfTypeAll<GameplayModifiersModelSO*>().front_or_default();
    modifierMultiplier = modifierModel->GetTotalMultiplier(modifierModel->CreateModifierParamsList(gameplayModifiers), 10);
}

MAKE_HOOK_MATCH(PPTime, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    PPTime(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    if (!firstActivation) return;
    PPCalculator::PP::Initialize();
    leaderboardFirstActivation = true;

    using ActivateDeleg = HMUI::ViewController::DidActivateDelegate;
    using DeactivateDeleg = HMUI::ViewController::DidDeactivateDelegate;
    auto* lb = UnityEngine::Resources::FindObjectsOfTypeAll<PlatformLeaderboardViewController*>().front_or_default();

    lb->add_didActivateEvent(EasyDelegate::MakeDelegate<ActivateDeleg*>([lb](bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
        if (scoreDetailsUI != nullptr && scoreDetailsUI->hasValidScoreData && GET_VALUE(alwaysOpen) && GET_VALUE(enablePopup)){
            if (scoreDetailsUI->modal->_isShown) scoreDetailsUI->modal->Hide(false, nullptr);
            scoreDetailsUI->updateInfo("lodaing...");
            scoreDetailsUI->modal->Show(false, true, EasyDelegate::MakeDelegate<System::Action*>([](){
                scoreDetailsUI->updateInfo(finishedLoading ? "" : "loading...");
            }));
        }
    }));

    lb->add_didDeactivateEvent(EasyDelegate::MakeDelegate<DeactivateDeleg*>([](bool removedFromHierarchy, bool screenSystemDisabling){
        if (scoreDetailsUI != nullptr && scoreDetailsUI->modal->_isShown) scoreDetailsUI->modal->Hide(false, nullptr);
    }));
}

MAKE_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &MenuTransitionsHelper::RestartGame, void, MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback)
{
    scoreDetailsUI = nullptr;
    cachedMaxRankText = nullptr;
    MenuTransitionsHelper_RestartGame(self, finishCallback);
}

// Called later on in the game loading - a good time to install function hooks
SCORE_PERCENTAGE_EXPORT_FUNC void late_load() {
    il2cpp_functions::Init();
    loadConfig();
    BSML::Init();
    custom_types::Register::AutoRegister();
    getScorePercentageConfig().Init(modInfo);
    BSML::Register::RegisterMainMenuFlowCoordinator("Score Percentage", "idk what to put here", csTypeOf(ScoreDetailsUI::SettingsFlowCoordinator*));
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), Menu);
    INSTALL_HOOK(getLogger(), Results);
    INSTALL_HOOK(getLogger(), Pause);
    INSTALL_HOOK(getLogger(), PPTime);
    INSTALL_HOOK(getLogger(), GameplayCoreSceneSetupData_ctor)
    INSTALL_HOOK(getLogger(), MenuTransitionsHelper_RestartGame);
    INSTALL_HOOK(getLogger(), TakeMeToResults_Fix);
    INSTALL_HOOK(getLogger(), Restart);
    
    ScorePercentage::MultiplayerHooks::InstallHooks();
    getLogger().info("Installed all hooks!");
}