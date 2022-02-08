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
#include "ScoreDetailsConfig.hpp"
#include "PPCalculator.hpp"
#include "main.hpp"

using namespace QuestUI::BeatSaberUI;
using namespace UnityEngine::UI;
using namespace UnityEngine;
using namespace GlobalNamespace;
using namespace ScorePercentage::Utils;

ScoreDetailsConfig ScoreDetails::config;

ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

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

void ScoreDetails::loadConfig() {
    getConfig().Load();
    ConfigHelper::LoadConfig(config, getConfig().config);
}

int currentScore = 0;
int multiCurrentScore = 0;
double currentPercentage = 0;
ScorePercentage::ModalPopup* scoreDetailsUI = nullptr;
std::string mapID = "";
std::string mapType = "";
int pauseCount = 0;
bool ScoreDetails::modalSettingsChanged = false;

struct MultiplayerMapData{
    IDifficultyBeatmap* beatmap;
    PlayerLevelStatsData* statsdata;
} multiplayerMapData;

MAKE_HOOK_MATCH(Menu, &LevelStatsView::ShowStats, void, LevelStatsView* self, IDifficultyBeatmap* difficultyBeatmap, PlayerData* playerData) {
    Menu(self, difficultyBeatmap, playerData);
    if (playerData != nullptr)
    {
        if (scoreDetailsUI == nullptr || ScoreDetails::modalSettingsChanged) ScorePercentage::initModalPopup(&scoreDetailsUI, self->get_transform()->get_parent());
        
        auto* playerLevelStatsData = playerData->GetPlayerLevelStatsData(difficultyBeatmap);
        currentScore = playerLevelStatsData->highScore;
        mapID = to_utf8(csstrtostr(playerLevelStatsData->get_levelID()));
        mapType = to_utf8(csstrtostr(playerLevelStatsData->get_beatmapCharacteristic()->get_serializedName()));
        if (playerLevelStatsData->validScore)
        {
            currentPercentage = calculatePercentage(calculateMaxScore(difficultyBeatmap->get_beatmapData()->cuttableNotesCount), currentScore);
            std::string idString;
            if (mapType.compare("Standard") != 0) idString = mapType + std::to_string(difficultyBeatmap->get_difficultyRank());
            else idString = std::to_string(difficultyBeatmap->get_difficultyRank());
            ConfigHelper::LoadBeatMapInfo(mapID, idString);

            if (ScoreDetails::config.MenuHighScore)
            {
                self->get_gameObject()->set_active(false);
                scoreDetailsUI->openButton->get_gameObject()->SetActive(true);
                scoreDetailsUI->updateInfo(playerLevelStatsData, difficultyBeatmap);
            }
        }
        if (!ScoreDetails::config.MenuHighScore || !playerLevelStatsData->validScore)
        {
            self->get_gameObject()->set_active(true);
            scoreDetailsUI->modal->Hide(true, nullptr);
            scoreDetailsUI->openButton->get_gameObject()->SetActive(false);
        }
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
    if (firstActivation) CreateText(self->get_transform(), "<size=150%>KNOBHEAD</size>", UnityEngine::Vector2(20, 20));

    // Default Info Texts
    std::string rankText = to_utf8(csstrtostr(self->rankText->get_text()));
    std::string scoreText = to_utf8(csstrtostr(self->scoreText->get_text()));
    std::string missText = to_utf8(csstrtostr(self->goodCutsPercentageText->get_text()));

    bool isValidScore = !(self->get_practice() || currentScore == 0);

    // only update stuff if level was cleared
    if (self->levelCompletionResults->levelEndStateType == GlobalNamespace::LevelCompletionResults::LevelEndStateType::Cleared)
    {
        resultScore = self->levelCompletionResults->modifiedScore;
        resultPercentage = calculatePercentage(calculateMaxScore(self->difficultyBeatmap->get_beatmapData()->cuttableNotesCount), resultScore);

        // probably stops stuff from breaking
        self->rankText->set_autoSizeTextContainer(false);
        self->rankText->set_enableWordWrapping(false);
        self->goodCutsPercentageText->set_enableWordWrapping(false);
        
        auto* rankTitleText = self->get_transform()->Find(il2cpp_utils::newcsstr("Container/ClearedInfo/RankTitle"))->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        
        // rank text
        if (ScoreDetails::config.LevelEndRank)
        {
            rankTitleText->SetText(il2cpp_utils::newcsstr("PERCENTAGE"));
            rankText = Round(resultPercentage, 2) + "<size=45%>%";    
        }
        else rankTitleText->SetText(il2cpp_utils::newcsstr("RANK"));

        // percentage difference text
        if (ScoreDetails::config.ScorePercentageDifference && isValidScore) rankText = createRankText(rankText, resultPercentage - currentPercentage);
        self->rankText->SetText(il2cpp_utils::newcsstr(rankText));

        // score difference text
        if (ScoreDetails::config.ScoreDifference && isValidScore)
        {
            self->newHighScoreText->SetActive(false);
            self->scoreText->SetText(il2cpp_utils::newcsstr(createScoreText(scoreText, resultScore - currentScore)));
        }

        // miss difference text
        if(ScoreDetails::config.missDifference && ScoreDetails::config.missCount != -1 && isValidScore)
        {
            int currentMisses = ScoreDetails::config.badCutCount + ScoreDetails::config.missCount;
            int resultMisses = self->levelCompletionResults->missedCount + self->levelCompletionResults->badCutsCount;
            self->goodCutsPercentageText->SetText(il2cpp_utils::newcsstr(createMissText(missText, currentMisses - resultMisses)));
        }
        else self->goodCutsPercentageText->SetText(il2cpp_utils::newcsstr(missText));

        // write new highscore to file
        if ((resultScore - currentScore) > 0 && !self->get_practice())
        {
            int misses = self->levelCompletionResults->missedCount;
            int badCut = self->levelCompletionResults->badCutsCount;
            int diff = self->difficultyBeatmap->get_difficultyRank();
            std::string idString = mapType.compare("Standard") != 0 ? mapType + std::to_string(diff) : std::to_string(diff);
            std::string currentTime = to_utf8(csstrtostr(System::DateTime::get_UtcNow().ToLocalTime().ToString(il2cpp_utils::newcsstr("D"))));
            ConfigHelper::UpdateBeatMapInfo(mapID, idString, misses, badCut, pauseCount, currentTime);
        }
    }
}

MAKE_HOOK_MATCH(MultiplayerResults, &ResultsTableCell::SetData, void, ResultsTableCell* self, int order, IConnectedPlayer* connectedPlayer, LevelCompletionResults* levelCompletionResults){
    MultiplayerResults(self, order, connectedPlayer, levelCompletionResults);

    if (ScoreDetails::config.LevelEndRank){
        if (!self->rankText->get_richText()){
                self->rankText->set_enableWordWrapping(false);
                self->rankText->set_richText(true);
                self->scoreText->set_richText(true);
                self->scoreText->set_enableWordWrapping(false);
                self->scoreText->get_transform()->set_localPosition(self->scoreText->get_transform()->get_localPosition() + UnityEngine::Vector3(-5, 0, 0));
                self->rankText->get_transform()->set_localPosition(self->rankText->get_transform()->get_localPosition() + UnityEngine::Vector3(-2, 0, 0));
            }
        std::string percentageText = Round(calculatePercentage(calculateMaxScore(multiplayerMapData.beatmap->get_beatmapData()->get_cuttableNotesCount()), levelCompletionResults->modifiedScore), 2);
        self->rankText->SetText(il2cpp_utils::newcsstr(percentageText + "<size=75%>%</size>"));
        std::string score = to_utf8(csstrtostr(self->scoreText->get_text()));
        std::string missText = levelCompletionResults->fullCombo ? "FC" : "<color=red>X</color><size=65%> </size>" + std::to_string(levelCompletionResults->missedCount + levelCompletionResults->badCutsCount);
        self->scoreText->SetText(il2cpp_utils::newcsstr(missText + "    " + score));
    }
    else if (self->rankText->get_richText() && !ScoreDetails::config.LevelEndRank){
        self->rankText->set_enableWordWrapping(true);
        self->rankText->set_richText(false);
        self->scoreText->set_richText(false);
        self->scoreText->set_enableWordWrapping(true);
        self->scoreText->get_transform()->set_localPosition(self->scoreText->get_transform()->get_localPosition() + UnityEngine::Vector3(5, 0, 0));
        self->rankText->get_transform()->set_localPosition(self->rankText->get_transform()->get_localPosition() + UnityEngine::Vector3(2, 0, 0));
    }
    if (!ScoreDetails::config.LevelEndRank){
        if (levelCompletionResults->levelEndStateType == GlobalNamespace::LevelCompletionResults::LevelEndStateType::Failed) self->rankText->SetText(il2cpp_utils::newcsstr("F"));
        else self->rankText->SetText(GlobalNamespace::RankModel::GetRankName(levelCompletionResults->rank));
    }
    if (connectedPlayer->get_isMe() && (levelCompletionResults->modifiedScore - multiCurrentScore > 0) && levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::Cleared){
            mapType = to_utf8(csstrtostr(multiplayerMapData.statsdata->get_beatmapCharacteristic()->get_serializedName()));
            mapID = to_utf8(csstrtostr(multiplayerMapData.statsdata->get_levelID()));
            int misses = levelCompletionResults->missedCount;
            int badCut = levelCompletionResults->badCutsCount;
            int diff = multiplayerMapData.beatmap->get_difficultyRank();
            std::string idString = mapType.compare("Standard") != 0 ? mapType + std::to_string(diff) : std::to_string(diff);
            std::string currentTime = to_utf8(csstrtostr(System::DateTime::get_UtcNow().ToLocalTime().ToString(il2cpp_utils::newcsstr("D"))));
            ConfigHelper::UpdateBeatMapInfo(mapID, idString, misses, badCut, pauseCount, currentTime);
        }
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(GameplayCoreSceneSetupData_ctor, "", "GameplayCoreSceneSetupData", ".ctor", void, GameplayCoreSceneSetupData* self, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects, EnvironmentInfoSO* environmentInfo, ColorScheme* colorScheme)
{
    GameplayCoreSceneSetupData_ctor(self, difficultyBeatmap, previewBeatmapLevel, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects, environmentInfo, colorScheme);
    multiplayerMapData.beatmap = difficultyBeatmap;
    multiplayerMapData.statsdata = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<PlayerDataModel*>())->playerData->GetPlayerLevelStatsData(difficultyBeatmap);
    multiCurrentScore = multiplayerMapData.statsdata->get_highScore();
    pauseCount = 0;
    if (scoreDetailsUI != nullptr) scoreDetailsUI->modal->Hide(true, nullptr);
    getLogger().info("current score: %s", std::to_string(multiCurrentScore).c_str());
}

MAKE_HOOK_MATCH(PPTime, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    PPTime(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    PPCalculator::PP::Initialize();
}

MAKE_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &GlobalNamespace::MenuTransitionsHelper::RestartGame, void, GlobalNamespace::MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback)
{
    delete scoreDetailsUI;
    scoreDetailsUI = nullptr;
    MenuTransitionsHelper_RestartGame(self, finishCallback);
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    ScoreDetails::loadConfig();
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