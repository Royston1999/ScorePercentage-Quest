#include <iomanip>
#include <cmath>

#include "ScoreDetailsModal.hpp"
#include "Utils/ScoreUtils.hpp"
#include "UI/SettingsFlowCoordinator.hpp"
#include "PPCalculator.hpp"
#include "Utils/MapUtils.hpp"
#include "MultiplayerHooks.hpp"
#include "main.hpp"

#include "questui/shared/QuestUI.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "bs-utils/shared/utils.hpp"

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
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/MainSettingsModelSO.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/GameplayModifiersModelSO.hpp"
#include "GlobalNamespace/GameplayModifierParamsSO.hpp"
#include "UnityEngine/WaitForSeconds.hpp"
#include "GlobalNamespace/LevelCompletionResultsHelper.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"

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
custom_types::Helpers::Coroutine FuckYouBeatSaviorData(LevelStatsView* self);
bool noException = false;
bool successfullySkipped = false;
bool leaderboardFirstActivation = false;
bool FUCKINGBEATSAVIORSUCKMYCOCK;
bool validResults;

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
            auto* beatMySavior = QuestUI::ArrayUtil::First(self->get_transform()->get_parent()->GetComponentsInChildren<Button*>(), [](Button* x) { return x->get_name() == "BeatSaviorDataDetailsButton"; });
            if (beatMySavior && beatMySavior->get_gameObject()->get_active()){
                beatBeyondSaving = true;
                scoreDetailsUI->openButton->GetComponent<RectTransform*>()->set_anchoredPosition({-47.0f, 10.0f});
            }
        }
        else co_yield nullptr;
    }
    if (!beatBeyondSaving) scoreDetailsUI->openButton->GetComponent<RectTransform*>()->set_anchoredPosition({-47.0f, 0.0f});
    if (scoreDetailsUI != nullptr && scorePercentageConfig.alwaysOpen && leaderboardFirstActivation){
        noException = true;
        scoreDetailsUI->modal->Hide(false, nullptr);
        co_yield reinterpret_cast<System::Collections::IEnumerator*>(UnityEngine::WaitForSeconds::New_ctor(0.4f));
    }
    if (self->get_isActiveAndEnabled() && scoreDetailsUI->hasValidScoreData && scorePercentageConfig.alwaysOpen && scorePercentageConfig.MenuHighScore) scoreDetailsUI->modal->Show(true, true, nullptr);
    leaderboardFirstActivation = false;
    co_return;
}

void toggleModalVisibility(bool value, LevelStatsView* self){
    scoreDetailsUI->openButton->get_gameObject()->SetActive(value);
    scoreDetailsUI->hasValidScoreData = value;
    if (!value){
        noException = true;
        scoreDetailsUI->modal->Hide(true, nullptr);
    }
    if (FUCKINGBEATSAVIORSUCKMYCOCK) SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(FuckYouBeatSaviorData(self)));
    else if (self->get_isActiveAndEnabled() && scoreDetailsUI->hasValidScoreData && scorePercentageConfig.alwaysOpen && scorePercentageConfig.MenuHighScore && !scoreDetailsUI->modal->isShown) scoreDetailsUI->modal->Show(true, true, nullptr);
}

void createDifferenceTexts(ResultsViewController* self){
    scoreDiffText = Object::Instantiate(self->dyn__rankText(), self->dyn__rankText()->get_transform()->get_parent());
    rankDiffText = Object::Instantiate(self->dyn__rankText(), self->dyn__rankText()->get_transform()->get_parent());
    auto scoreTextPos = self->dyn__scoreText()->get_transform()->get_localPosition();
    auto rankTextPos = self->dyn__rankText()->get_transform()->get_localPosition();
    scoreDiffText->get_transform()->set_localPosition(Vector3(scoreTextPos.x - 1.0f, rankTextPos.y - 4.5f, scoreTextPos.z));
    rankDiffText->get_transform()->set_localPosition(Vector3(rankTextPos.x + 1.0f, rankTextPos.y - 4.5f, rankTextPos.z));
    scoreDiffText->set_enableWordWrapping(false);
}

MAKE_HOOK_MATCH(Menu, &LevelStatsView::ShowStats, void, LevelStatsView* self, IDifficultyBeatmap* difficultyBeatmap, PlayerData* playerData) {
    Menu(self, difficultyBeatmap, playerData);
    auto* test = reinterpret_cast<IPreviewBeatmapLevel*>(difficultyBeatmap->get_level());
    getLogger().info("ID: %s", static_cast<std::string>(test->get_levelID()).c_str());
    validResults = false;
    if (noException) noException = false;
    if (playerData != nullptr)
    {
        if (scoreDetailsUI == nullptr || modalSettingsChanged) ScorePercentage::initModalPopup(&scoreDetailsUI, self->get_transform());
        auto* playerLevelStatsData = playerData->GetPlayerLevelStatsData(difficultyBeatmap);
        ScorePercentage::MapUtils::updateMapData(playerData, difficultyBeatmap);
        if (playerLevelStatsData->get_validScore())
        {
            ConfigHelper::LoadBeatMapInfo(mapData.mapID, mapData.idString);
            if (scorePercentageConfig.MenuHighScore && playerLevelStatsData->highScore > 0) toggleModalVisibility(true, self);
        }
        if (!scorePercentageConfig.MenuHighScore || !playerLevelStatsData->get_validScore() || playerLevelStatsData->highScore < 1) toggleModalVisibility(false, self);
    }
}

MAKE_HOOK_MATCH(Pause, &GamePause::Pause, void, GamePause* self) {
    Pause(self);
    pauseCount++;
}

MAKE_HOOK_MATCH(TakeMeToResults_Fix, &LevelCompletionResultsHelper::ProcessScore, void, PlayerData* playerData, PlayerLevelStatsData* playerLevelStats, LevelCompletionResults* levelCompletionResults, IReadonlyBeatmapData* transformedBeatmapData, IDifficultyBeatmap* difficultyBeatmap, PlatformLeaderboardsModel* platformLeaderboardsModel){
    TakeMeToResults_Fix(playerData, playerLevelStats, levelCompletionResults, transformedBeatmapData, difficultyBeatmap, platformLeaderboardsModel);
    if (levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::Cleared) validResults = true;
}

MAKE_HOOK_MATCH(Results, &ResultsViewController::DidActivate, void, ResultsViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    Results(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    int resultScore;
    double resultPercentage;

    if (!validResults && !self->get_practice()) return;
    validResults = false;
    // disable score differences in party mode cuz idk how that's supposed to work (still shows rank as percentage tho) 
    auto* flowthingy = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<PartyFreePlayFlowCoordinator*>());
    bool isParty = flowthingy != nullptr ? flowthingy->get_isActivated() ? true : false : false;

    // funny thing i wonder if anyone notices
    if (firstActivation) CreateText(self->get_transform(), "<size=150%>KNOBHEAD</size>", Vector2(20, 20));

    // Default Info Texts
    std::string rankText = self->dyn__rankText()->get_text();
    std::string scoreText = self->dyn__scoreText()->get_text();
    std::string missText = self->dyn__goodCutsPercentageText()->get_text();

    bool isValidScore = !(self->get_practice() || mapData.currentScore == 0 || isParty);

    // only update stuff if level was cleared
    if (self->dyn__levelCompletionResults()->dyn_levelEndStateType() == LevelCompletionResults::LevelEndStateType::Cleared)
    {
        if (scoreDiffText == nullptr) createDifferenceTexts(self);

        resultScore = self->dyn__levelCompletionResults()->dyn_modifiedScore();
        resultPercentage = calculatePercentage(mapData.maxScore, resultScore);

        // probably stops stuff from breaking
        self->dyn__rankText()->set_autoSizeTextContainer(false);
        self->dyn__rankText()->set_enableWordWrapping(false);
        self->dyn__goodCutsPercentageText()->set_enableWordWrapping(false);
        
        auto* rankTitleText = self->get_transform()->Find("Container/ClearedInfo/RankTitle")->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if (origRankText == nullptr) origRankText = rankTitleText->get_text();
        // rank text
        if (scorePercentageConfig.LevelEndRank)
        {
            rankTitleText->SetText("Percentage");
            rankText = "  " + Round(std::abs(resultPercentage), 2) + "<size=45%>%";
        }
        else rankTitleText->SetText(origRankText);

        // percentage difference text
        if (scorePercentageConfig.ScorePercentageDifference && isValidScore){
            std::string rankDiff = valueDifferenceString(resultPercentage - mapData.currentPercentage);
            std::string sorry = scorePercentageConfig.LevelEndRank ? "" : "  ";
            rankText = createScoreText(sorry + rankText);
            rankDiffText->get_gameObject()->SetActive(true);
            rankDiffText->SetText("<size=40%>" + rankDiff + "<size=30%>%");
        }
        else rankDiffText->get_gameObject()->SetActive(false);
        self->dyn__rankText()->SetText(rankText);

        // score difference text
        if (scorePercentageConfig.ScoreDifference && isValidScore)
        {
            self->dyn__newHighScoreText()->SetActive(false);
            std::string formatting = "<size=40%>" + ((resultScore - mapData.currentScore >= 0) ? positiveColour + "+" : negativeColour); 
            scoreDiffText->get_gameObject()->SetActive(true);
            scoreDiffText->SetText(StringW(formatting)  + ScoreFormatter::Format(resultScore - mapData.currentScore));
            self->dyn__scoreText()->SetText(createScoreText(" " + scoreText));
        }
        else {
            scoreDiffText->get_gameObject()->SetActive(false);
            self->dyn__scoreText()->SetText(scoreText);        
        }

        // miss difference text
        if(scorePercentageConfig.missDifference && scorePercentageConfig.missCount != -1 && isValidScore)
        {
            int currentMisses = scorePercentageConfig.badCutCount + scorePercentageConfig.missCount;
            int resultMisses = self->dyn__levelCompletionResults()->dyn_missedCount() + self->dyn__levelCompletionResults()->dyn_badCutsCount();
            self->dyn__goodCutsPercentageText()->SetText(createMissText(missText, currentMisses - resultMisses));
        }
        else self->dyn__goodCutsPercentageText()->SetText(missText);

        // write new highscore to file
        if ((resultScore - mapData.currentScore) > 0 && !self->get_practice() && !isParty && bs_utils::Submission::getEnabled())
        {
            int misses = self->dyn__levelCompletionResults()->dyn_missedCount();
            int badCut = self->dyn__levelCompletionResults()->dyn_badCutsCount();
            std::string currentTime = System::DateTime::get_UtcNow().ToLocalTime().ToString("D");
            ConfigHelper::UpdateBeatMapInfo(mapData.mapID, mapData.idString, misses, badCut, pauseCount, currentTime);
        }
    }
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(GameplayCoreSceneSetupData_ctor, "", "GameplayCoreSceneSetupData", ".ctor", void, GameplayCoreSceneSetupData* self, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects, EnvironmentInfoSO* environmentInfo, ColorScheme* colorScheme, MainSettingsModelSO* mainSettingsModel)
{
    GameplayCoreSceneSetupData_ctor(self, difficultyBeatmap, previewBeatmapLevel, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects, environmentInfo, colorScheme, mainSettingsModel);
    auto * playerData = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<PlayerDataModel*>())->get_playerData();
    auto* playerLevelStatsData = playerData->GetPlayerLevelStatsData(difficultyBeatmap);
    if (mapData.mapID != playerLevelStatsData->levelID || mapData.diff != difficultyBeatmap->get_difficulty() || mapData.mapType != playerLevelStatsData->beatmapCharacteristic->serializedName) ScorePercentage::MapUtils::updateMapData(playerData, difficultyBeatmap);
    pauseCount = 0;
    noException = true;
    if (scoreDetailsUI != nullptr) scoreDetailsUI->modal->Hide(true, nullptr);
    
    // multiplayer tomfoolery
    hasBeenNitod = false;
    playerInfos.clear();
    auto* modifierModel = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<GameplayModifiersModelSO*>());
    modifierMultiplier = modifierModel->GetTotalMultiplier(modifierModel->CreateModifierParamsList(gameplayModifiers), 10);
}

MAKE_HOOK_MATCH(PPTime, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    PPTime(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    if (firstActivation){
        FUCKINGBEATSAVIORSUCKMYCOCK = Modloader::requireMod("BeatSaviorData");
        PPCalculator::PP::Initialize();
        leaderboardFirstActivation = true;
    }
}

MAKE_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &MenuTransitionsHelper::RestartGame, void, MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback)
{
    scoreDetailsUI = nullptr;
    origRankText = nullptr;
    scoreDiffText = nullptr;
    rankDiffText = nullptr;
    MenuTransitionsHelper_RestartGame(self, finishCallback);
}

//silly hooks for modal stuff
MAKE_HOOK_MATCH(Modal_Hide, &HMUI::ModalView::Hide, void, HMUI::ModalView* self, bool animated, System::Action* finishedCallback){
    bool isMyModal = self->get_name() == "ScoreDetailsModal";
    if (scoreDetailsUI != nullptr && isMyModal && !noException && scorePercentageConfig.alwaysOpen) return;
    else Modal_Hide(self, animated, finishedCallback);
    noException = false;
}

MAKE_HOOK_MATCH(LeaderBoard_Activate, &PlatformLeaderboardViewController::DidActivate, void, PlatformLeaderboardViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    LeaderBoard_Activate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    if (FUCKINGBEATSAVIORSUCKMYCOCK) SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(FuckYouBeatSaviorData(self->get_transform()->GetComponentInChildren<LevelStatsView*>())));
    else if (scoreDetailsUI != nullptr && scoreDetailsUI->hasValidScoreData && scorePercentageConfig.alwaysOpen && scorePercentageConfig.MenuHighScore){
        if (scoreDetailsUI->modal->isShown){
            noException = true;
            scoreDetailsUI->modal->Hide(false, nullptr);
            scoreDetailsUI->modal->Show(false, true, nullptr);
        }
        else scoreDetailsUI->modal->Show(true, true, nullptr);
    }
}

MAKE_HOOK_MATCH(LeaderBoard_DeActivate, &PlatformLeaderboardViewController::DidDeactivate, void, PlatformLeaderboardViewController* self, bool removedFromHierarchy, bool screenSystemDisabling){
    LeaderBoard_DeActivate(self, removedFromHierarchy, screenSystemDisabling);
    if (!FUCKINGBEATSAVIORSUCKMYCOCK) return;
    if (scoreDetailsUI != nullptr && scoreDetailsUI->modal->dyn__isShown()){
        noException = true;
        scoreDetailsUI->modal->Hide(false, nullptr);
    }
}

MAKE_HOOK_MATCH(LeaderBoard_Deactivate, &SoloFreePlayFlowCoordinator::BackButtonWasPressed, void, SinglePlayerLevelSelectionFlowCoordinator* self, HMUI::ViewController* topViewController){
    noException = topViewController != reinterpret_cast<HMUI::ViewController*>(self->dyn__practiceViewController());
    LeaderBoard_Deactivate(self, topViewController);
    if (scoreDetailsUI != nullptr && scoreDetailsUI->modal->dyn__isShown()) scoreDetailsUI->modal->Hide(false, nullptr);
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    loadConfig();
    QuestUI::Init();
    custom_types::Register::AutoRegister();
    QuestUI::Register::RegisterModSettingsFlowCoordinator<ScoreDetailsUI::SettingsFlowCoordinator*>(modInfo);
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), Menu);
    INSTALL_HOOK(getLogger(), Results);
    INSTALL_HOOK(getLogger(), Pause);
    INSTALL_HOOK(getLogger(), PPTime);
    INSTALL_HOOK(getLogger(), GameplayCoreSceneSetupData_ctor)
    INSTALL_HOOK(getLogger(), MenuTransitionsHelper_RestartGame);
    INSTALL_HOOK(getLogger(), Modal_Hide);
    INSTALL_HOOK(getLogger(), LeaderBoard_Activate);
    INSTALL_HOOK(getLogger(), LeaderBoard_Deactivate);
    INSTALL_HOOK(getLogger(), LeaderBoard_DeActivate);
    INSTALL_HOOK(getLogger(), TakeMeToResults_Fix);
    ScorePercentage::MultiplayerHooks::InstallHooks();
    getLogger().info("Installed all hooks!");
}