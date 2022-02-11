#include <iomanip>
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
    self->get_gameObject()->set_active(!value);
    scoreDetailsUI->openButton->get_gameObject()->SetActive(value);
    if (value) scoreDetailsUI->updateInfo();
    else scoreDetailsUI->modal->Hide(true, nullptr);
}

MAKE_HOOK_MATCH(Menu, &LevelStatsView::ShowStats, void, LevelStatsView* self, IDifficultyBeatmap* difficultyBeatmap, PlayerData* playerData) {
    Menu(self, difficultyBeatmap, playerData);
    if (playerData != nullptr)
    {
        if (scoreDetailsUI == nullptr || modalSettingsChanged) ScorePercentage::initModalPopup(&scoreDetailsUI, self->get_transform()->get_parent());
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

    // funny thing i wonder if anyone notices
    // if (firstActivation) CreateText(self->get_transform(), "<size=150%>KNOBHEAD</size>", UnityEngine::Vector2(20, 20));

    // Default Info Texts
    std::string rankText = to_utf8(csstrtostr(self->rankText->get_text()));
    std::string scoreText = to_utf8(csstrtostr(self->scoreText->get_text()));
    std::string missText = to_utf8(csstrtostr(self->goodCutsPercentageText->get_text()));

    bool isValidScore = !(self->get_practice() || mapData.currentScore == 0);

    // only update stuff if level was cleared
    if (self->levelCompletionResults->levelEndStateType == GlobalNamespace::LevelCompletionResults::LevelEndStateType::Cleared)
    {
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
            rankText = Round(resultPercentage, 2) + "<size=45%>%";    
        }
        else rankTitleText->SetText(origRankText);

        // percentage difference text
        if (scorePercentageConfig.ScorePercentageDifference && isValidScore) rankText = createRankText(rankText, resultPercentage - mapData.currentPercentage);
        self->rankText->SetText(il2cpp_utils::newcsstr(rankText));

        // score difference text
        if (scorePercentageConfig.ScoreDifference && isValidScore)
        {
            self->newHighScoreText->SetActive(false);
            self->scoreText->SetText(il2cpp_utils::newcsstr(createScoreText(scoreText, resultScore - mapData.currentScore)));
        }

        // miss difference text
        if(scorePercentageConfig.missDifference && scorePercentageConfig.missCount != -1 && isValidScore)
        {
            int currentMisses = scorePercentageConfig.badCutCount + scorePercentageConfig.missCount;
            int resultMisses = self->levelCompletionResults->missedCount + self->levelCompletionResults->badCutsCount;
            self->goodCutsPercentageText->SetText(il2cpp_utils::newcsstr(createMissText(missText, currentMisses - resultMisses)));
        }
        else self->goodCutsPercentageText->SetText(il2cpp_utils::newcsstr(missText));

        // write new highscore to file
        if ((resultScore - mapData.currentScore) > 0 && !self->get_practice())
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
}

MAKE_HOOK_MATCH(PPTime, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    PPTime(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    if (firstActivation) PPCalculator::PP::Initialize();
}

MAKE_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &GlobalNamespace::MenuTransitionsHelper::RestartGame, void, GlobalNamespace::MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback)
{
    delete scoreDetailsUI;
    scoreDetailsUI = nullptr;
    origRankText = nullptr;
    MenuTransitionsHelper_RestartGame(self, finishCallback);
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
    getLogger().info("Installed all hooks!");
}