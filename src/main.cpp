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

#include "custom-types/shared/delegate.hpp"

#include "System/DateTime.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Time.hpp"

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
#include "GlobalNamespace/PauseMenuManager.hpp"

#include "GlobalNamespace/BeatmapData.hpp"
#include "BeatmapSaveDataVersion3/BeatmapSaveData.hpp"
#include "BeatmapSaveDataVersion3/BeatmapSaveData_ColorNoteData.hpp"
#include "GlobalNamespace/BeatmapDataLoader.hpp"
#include "GlobalNamespace/BeatmapEnvironmentHelper.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmap.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/OverrideEnvironmentSettings.hpp"
#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/BeatmapDataCache.hpp"
#include "System/Action_2.hpp"
#include "System/Action.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "UnityEngine/UI/Button_ButtonClickedEvent.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "scoreutils/shared/ScoreUtils.hpp"

#include "HMUI/ViewController_DidActivateDelegate.hpp"
#include "HMUI/ViewController_DidDeactivateDelegate.hpp"

#include "Utils/EasyDelegate.hpp"

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
Il2CppString* cachedMaxRankText = nullptr;
TMPro::TextMeshProUGUI* scoreDiffText = nullptr;
TMPro::TextMeshProUGUI* rankDiffText = nullptr;
custom_types::Helpers::Coroutine FuckYouBeatSaviorData(LevelStatsView* self);
bool successfullySkipped = false;
bool leaderboardFirstActivation = false;
bool FUCKINGBEATSAVIORSUCKMYCOCK;
bool validResults;
bool finishedLoading;
int loadingStatus = 0;

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
    for (int i=0; i<3; i++) co_yield nullptr;

    auto* beatMySavior = QuestUI::ArrayUtil::First(self->get_transform()->get_parent()->GetComponentsInChildren<Button*>(), [](Button* x) { return x->get_name() == "BeatSaviorDataDetailsButton"; });
    if (beatMySavior && beatMySavior->get_gameObject()->get_active()){
        beatBeyondSaving = true;
        scoreDetailsUI->openButton->GetComponent<RectTransform*>()->set_anchoredPosition({-47.0f, 10.0f});
    }

    if (!beatBeyondSaving) scoreDetailsUI->openButton->GetComponent<RectTransform*>()->set_anchoredPosition({-47.0f, 0.0f});
    if (scoreDetailsUI != nullptr && GET_VALUE(alwaysOpen) && leaderboardFirstActivation){
        scoreDetailsUI->modal->Hide(false, nullptr);
        co_yield reinterpret_cast<System::Collections::IEnumerator*>(WaitForSeconds::New_ctor(0.4f));
    }
    if (self->get_isActiveAndEnabled() && scoreDetailsUI->hasValidScoreData && GET_VALUE(alwaysOpen) && GET_VALUE(enablePopup)) scoreDetailsUI->modal->Show(true, true, EasyDelegate::MakeDelegate<System::Action*>([](){
            scoreDetailsUI->updateInfo(finishedLoading ? "" : "loading...");
        }));
    leaderboardFirstActivation = false;
    co_return;
}

void toggleModalVisibility(bool value, LevelStatsView* self){
    scoreDetailsUI->openButton->get_gameObject()->SetActive(value);
    scoreDetailsUI->hasValidScoreData = value;
    if (!value) return scoreDetailsUI->modal->Hide(true, nullptr);
    if (FUCKINGBEATSAVIORSUCKMYCOCK) SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(FuckYouBeatSaviorData(self)));
    else if (self->get_isActiveAndEnabled() && GET_VALUE(alwaysOpen) && !scoreDetailsUI->modal->isShown) scoreDetailsUI->modal->Show(true, true, EasyDelegate::MakeDelegate<System::Action*>([](){
            scoreDetailsUI->updateInfo(finishedLoading ? "" : "loading...");
        }));
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
    finishedLoading = false;
    validResults = false;
    if (leaderboardFirstActivation) {
        using namespace std::chrono_literals;
        std::thread waitForLocalisation([self](){
            std::this_thread::sleep_for(400ms);
            auto rankTitle = self->get_transform()->Find("MaxRank/Title")->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
            cachedMaxRankText = rankTitle->get_text();
            rankTitle->SetText(GET_VALUE(showPercentageInMenu) ? "Percentage" : StringW(cachedMaxRankText));
            std::this_thread::sleep_for(200ms); // just to be sure that the beatsavior coroutine has passed if installed
            leaderboardFirstActivation = false;
        });
        waitForLocalisation.detach();
    }
    if (playerData != nullptr)
    {
        if (!leaderboardFirstActivation) self->get_transform()->Find("MaxRank/Title")->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->SetText(GET_VALUE(showPercentageInMenu) ? "Percentage" : StringW(cachedMaxRankText));
        if (scoreDetailsUI == nullptr || modalSettingsChanged) ScorePercentage::initModalPopup(&scoreDetailsUI, self->get_transform());
        scoreDetailsUI->currentMap = difficultyBeatmap;
        scoreDetailsUI->playerData = playerData;
        auto* playerLevelStatsData = playerData->GetPlayerLevelStatsData(difficultyBeatmap);
        ScorePercentage::MapUtils::updateBasicMapInfo(playerData, difficultyBeatmap);
        if (playerLevelStatsData->get_validScore()) ConfigHelper::LoadBeatMapInfo(mapData.mapID, mapData.idString);
        
        if (playerLevelStatsData->get_validScore()) if (GET_VALUE(enablePopup) && playerLevelStatsData->highScore > 0) toggleModalVisibility(true, self);
        
        if (!GET_VALUE(enablePopup) || !playerLevelStatsData->get_validScore() || playerLevelStatsData->highScore < 1) toggleModalVisibility(false, self);
        ScorePercentage::MapUtils::updateMapData(playerData, difficultyBeatmap);
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

    if (firstActivation) self->continueButton->get_onClick()->AddListener(EasyDelegate::MakeDelegate<Events::UnityAction*>([](){
        if (GET_VALUE(alwaysOpen) && scoreDetailsUI != nullptr && scoreDetailsUI->modal->isShown) scoreDetailsUI->updateInfo();
    }));

    // funny thing i wonder if anyone notices
    if (firstActivation) CreateText(self->get_transform(), "<size=150%>KNOBHEAD</size>", Vector2(20, 20));

    // Default Info Texts
    std::string rankText = self->rankText->get_text();
    std::string scoreText = self->scoreText->get_text();
    std::string missText = self->goodCutsPercentageText->get_text();

    bool isValidScore = !(self->get_practice() || mapData.currentScore == 0 || isParty);

    // only update stuff if level was cleared
    if (self->levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::Cleared)
    {
        if (firstActivation) createDifferenceTexts(self);

        resultScore = self->levelCompletionResults->modifiedScore;
        resultPercentage = CalculatePercentage(mapData.maxScore, resultScore);

        // probably stops stuff from breaking
        self->rankText->set_autoSizeTextContainer(false);
        self->rankText->set_enableWordWrapping(false);
        self->goodCutsPercentageText->set_enableWordWrapping(false);
        
        auto* rankTitleText = self->get_transform()->Find("Container/ClearedInfo/RankTitle")->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if (firstActivation) origRankText = rankTitleText->get_text();
        // rank text
        if (GET_VALUE(showPercentageOnResults))
        {
            rankTitleText->SetText("Percentage");
            rankText = "  " + Round(std::abs(resultPercentage), 2) + "<size=45%>%";
        }
        else rankTitleText->SetText(origRankText);

        // percentage difference text
        if (GET_VALUE(showPercentageDifference) && isValidScore){
            std::string rankDiff = valueDifferenceString(resultPercentage - mapData.currentPercentage);
            std::string sorry = GET_VALUE(showPercentageOnResults) ? "" : "  ";
            rankText = createScoreText(sorry + rankText);
            rankDiffText->get_gameObject()->SetActive(true);
            rankDiffText->SetText("<size=40%>" + rankDiff + "<size=30%>%");
        }
        else rankDiffText->get_gameObject()->SetActive(false);
        self->rankText->SetText(rankText);

        // score difference text
        if (GET_VALUE(showScoreDifference) && isValidScore)
        {
            self->newHighScoreText->SetActive(false);
            std::string formatting = "<size=40%>" + ((resultScore - mapData.currentScore >= 0) ? positiveColour + "+" : negativeColour); 
            scoreDiffText->get_gameObject()->SetActive(true);
            scoreDiffText->SetText(StringW(formatting)  + ScoreFormatter::Format(resultScore - mapData.currentScore));
            self->scoreText->SetText(createScoreText(" " + scoreText));
        }
        else {
            scoreDiffText->get_gameObject()->SetActive(false);
            self->scoreText->SetText(scoreText);        
        }

        // miss difference text
        if(GET_VALUE(showMissDifference) && scorePercentageConfig.missCount != -1 && isValidScore)
        {
            int currentMisses = scorePercentageConfig.badCutCount + scorePercentageConfig.missCount;
            int resultMisses = self->levelCompletionResults->missedCount + self->levelCompletionResults->badCutsCount;
            self->goodCutsPercentageText->SetText(createMissText(missText, currentMisses - resultMisses));
        }
        else self->goodCutsPercentageText->SetText(missText);

        // write new highscore to file
        if ((resultScore - mapData.currentScore) > 0 && !self->get_practice() && !isParty && bs_utils::Submission::getEnabled())
        {
            int misses = self->levelCompletionResults->missedCount;
            int badCut = self->levelCompletionResults->badCutsCount;
            std::string currentTime = System::DateTime::get_UtcNow().ToLocalTime().ToString("D");
            ConfigHelper::UpdateBeatMapInfo(mapData.mapID, mapData.idString, misses, badCut, pauseCount, currentTime);
        }
    }
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(GameplayCoreSceneSetupData_ctor, "", "GameplayCoreSceneSetupData", ".ctor", void, GameplayCoreSceneSetupData* self, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects, EnvironmentInfoSO* environmentInfo, ColorScheme* colorScheme, MainSettingsModelSO* mainSettingsModel, BeatmapDataCache* beatmapDataCache)
{
    GameplayCoreSceneSetupData_ctor(self, difficultyBeatmap, previewBeatmapLevel, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects, environmentInfo, colorScheme, mainSettingsModel, beatmapDataCache);
    auto * playerData = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<PlayerDataModel*>())->get_playerData();
    ScorePercentage::MapUtils::updateMapData(playerData, difficultyBeatmap, true);
    pauseCount = 0;
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
        // noodling = Modloader::requireMod("CustomJSONData");
        // therealnoods = Modloader::requireMod("NoodleExtensions");
        PPCalculator::PP::Initialize();
        leaderboardFirstActivation = true;

        using ActivateDeleg = HMUI::ViewController::DidActivateDelegate;
        using DeactivateDeleg = HMUI::ViewController::DidDeactivateDelegate;
        auto* lb = UnityEngine::Resources::FindObjectsOfTypeAll<PlatformLeaderboardViewController*>().FirstOrDefault();

        lb->add_didActivateEvent(EasyDelegate::MakeDelegate<ActivateDeleg*>([lb](bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
            if (FUCKINGBEATSAVIORSUCKMYCOCK) SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(FuckYouBeatSaviorData(lb->get_transform()->GetComponentInChildren<LevelStatsView*>())));
            else if (scoreDetailsUI != nullptr && scoreDetailsUI->hasValidScoreData && GET_VALUE(alwaysOpen) && GET_VALUE(enablePopup)){
                if (scoreDetailsUI->modal->isShown) scoreDetailsUI->modal->Hide(false, nullptr);
                scoreDetailsUI->modal->Show(false, true, EasyDelegate::MakeDelegate<System::Action*>([](){
                    scoreDetailsUI->updateInfo(finishedLoading ? "" : "loading...");
                }));
            }
        }));
    
        lb->add_didDeactivateEvent(EasyDelegate::MakeDelegate<DeactivateDeleg*>([](bool removedFromHierarchy, bool screenSystemDisabling){
            if (scoreDetailsUI != nullptr && scoreDetailsUI->modal->isShown) scoreDetailsUI->modal->Hide(false, nullptr);
        }));
    }
}

MAKE_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &MenuTransitionsHelper::RestartGame, void, MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback)
{
    scoreDetailsUI = nullptr;
    cachedMaxRankText = nullptr;
    MenuTransitionsHelper_RestartGame(self, finishCallback);
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    loadConfig();
    QuestUI::Init();
    custom_types::Register::AutoRegister();
    ScoreUtils::Init();
    getScorePercentageConfig().Init(modInfo);
    QuestUI::Register::RegisterModSettingsFlowCoordinator<ScoreDetailsUI::SettingsFlowCoordinator*>(modInfo);
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
    // PinkCore::API::GetFoundRequirementCallbackSafe() += CheckRequirements;
    ScoreUtils::MaxScoreRetriever::GetRetrievedMaxScoreCallback() += [](int maxScore){
        float currentPercentage = ScorePercentage::Utils::CalculatePercentage(maxScore, mapData.currentScore);
        mapData.currentPercentage = currentPercentage; 
        mapData.maxScore = maxScore;
        getLogger().info("Max Score from callback: %i", maxScore);
        if (finishedLoading) return;
        finishedLoading = true;
        QuestUI::MainThreadScheduler::Schedule([](){
            if (scoreDetailsUI != nullptr) {
                scoreDetailsUI->updateInfo();
                if (mapData.currentPercentage > 0 && GET_VALUE(showPercentageInMenu))scoreDetailsUI->modal->get_transform()->get_parent()->GetComponentInChildren<LevelStatsView*>()->maxRankText->SetText(Round(std::abs(mapData.currentPercentage), 2) + "%");
            }
        });
    };
}