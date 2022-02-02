#include "main.hpp"
#include "SettingsFlowCoordinator.hpp"
#include "ScoreDetailsConfig.hpp"
#include "PPCalculator.hpp"
#include "HMUI/ModalView.hpp"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "questui/shared/QuestUI.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/ResultsViewController.hpp"
#include "GlobalNamespace/PracticeViewController.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "GlobalNamespace/OverrideEnvironmentSettings.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/PracticeSettings.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/GamePause.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "System/Action.hpp"
#include "System/Action_2.hpp"
#include "System/DateTime.hpp"

#include "System/Math.hpp"
#include "HMUI/ImageView.hpp"
#include "GlobalNamespace/ScoreFormatter.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/TextAnchor.hpp"
#include "UnityEngine/Transform.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/LevelStatsView.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/MultiplayerResultsViewController.hpp"
#include "GlobalNamespace/ResultsTableView.hpp"
#include "GlobalNamespace/ResultsTableCell.hpp"
#include "GlobalNamespace/IConnectedPlayer.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "ScoreDetailsModal.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/RankModel.hpp"
#include "ScoreUtils.hpp"

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
std::string idString = "";
std::string statsButtonText = "VIEW SCORE DETAILS";
int pauseCount = 0;
int totalUIText = 0;
bool ScoreDetails::modalSettingsChanged = false;
int multiLevelMaxScore = 0;

struct MultiplayerMapData{
    IDifficultyBeatmap* beatmap;
    PlayerLevelStatsData* statsdata;
} multiplayerMapData;

MAKE_HOOK_MATCH(Menu, &LevelStatsView::ShowStats, void, LevelStatsView* self, IDifficultyBeatmap* difficultyBeatmap, PlayerData* playerData) {
    Menu(self, difficultyBeatmap, playerData);
    pauseCount = 0;
    if (playerData != nullptr)
    {
        if (scoreDetailsUI == nullptr || ScoreDetails::modalSettingsChanged){
            ScorePercentage::initModalPopup(&scoreDetailsUI, self->get_transform());
        }

        auto* levelStatsComponents = self->get_transform()->GetComponentsInChildren<TMPro::TextMeshProUGUI*>();
        GlobalNamespace::PlayerLevelStatsData* playerLevelStatsData = playerData->GetPlayerLevelStatsData(difficultyBeatmap);
        currentScore = playerLevelStatsData->highScore;
        mapID = to_utf8(csstrtostr(playerLevelStatsData->get_levelID()));
        mapType = to_utf8(csstrtostr(playerLevelStatsData->get_beatmapCharacteristic()->get_serializedName()));
        if (playerLevelStatsData->validScore)
        {
            currentPercentage = calculatePercentage(calculateMaxScore(difficultyBeatmap->get_beatmapData()->cuttableNotesCount), playerLevelStatsData->highScore);

            if (mapType.compare("Standard") != 0) idString = mapType + std::to_string(difficultyBeatmap->get_difficultyRank());
            else idString = std::to_string(difficultyBeatmap->get_difficultyRank());
            ConfigHelper::LoadBeatMapInfo(mapID, idString);

            if (ScoreDetails::config.MenuHighScore)
            { 
                for(int i=0; i<levelStatsComponents->get_Length(); i++) if (!(to_utf8(csstrtostr((*levelStatsComponents)[i]->get_text())).compare(statsButtonText)==0))(*levelStatsComponents)[i]->set_enabled(false);
                scoreDetailsUI->openButton->get_gameObject()->SetActive(true);
                scoreDetailsUI->updateInfo(playerLevelStatsData, difficultyBeatmap);
            }
        }
        if (!ScoreDetails::config.MenuHighScore || !playerLevelStatsData->validScore){
            scoreDetailsUI->modal->Hide(true, nullptr);
            scoreDetailsUI->openButton->get_gameObject()->SetActive(false);
            for(int i=0; i<levelStatsComponents->get_Length(); i++) (*levelStatsComponents)[i]->set_enabled(true);
        }
    }
}

MAKE_HOOK_MATCH(Pause, &GamePause::Pause, void, GamePause* self) {
    Pause(self);
    pauseCount++;
}

MAKE_HOOK_MATCH(Results, &ResultsViewController::DidActivate, void, ResultsViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    Results(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    int maxScore, resultScore, modifiedScore;
    double resultPercentage;

    // Default Rank Text
    std::string rankTextLine1 = to_utf8(csstrtostr(self->rankText->get_text()));
    std::string rankTextLine2 = "";

    // funny thing i wonder if anyone notices
    if (firstActivation) CreateText(self->get_transform(), "<size=150%>KNOBHEAD</size>", UnityEngine::Vector2(20, 20));

    //Only calculate percentage, if map was successfully cleared
    if (self->levelCompletionResults->levelEndStateType == GlobalNamespace::LevelCompletionResults::LevelEndStateType::Cleared)
    {
        std::string currentTime = to_utf8(csstrtostr(System::DateTime::get_UtcNow().ToLocalTime().ToString(il2cpp_utils::newcsstr("D"))));
        modifiedScore = self->levelCompletionResults->modifiedScore;
        maxScore = calculateMaxScore(self->difficultyBeatmap->get_beatmapData()->cuttableNotesCount);
        
        //use modifiedScore with negative multipliers
        GlobalNamespace::GameplayModifiers* mods = self->levelCompletionResults->gameplayModifiers;
        if (mods->get_noFailOn0Energy() || mods->get_enabledObstacleType().NoObstacles || mods->noArrows || mods->noBombs)
            resultScore = modifiedScore;

        //use rawScore without and with positive modifiers to avoid going over 100% without recalculating maxScore
        else resultScore = self->levelCompletionResults->rawScore;
        
        resultPercentage = calculatePercentage(maxScore, resultScore);

        //disable wrapping and autosize (unneccessary?)
        //might still be unnecessary not bothered to check
        self->rankText->set_autoSizeTextContainer(false);
        self->rankText->set_enableWordWrapping(false);
        
        if ((modifiedScore - currentScore) > 0 && !self->get_practice() && self->difficultyBeatmap->get_beatmapData()->cuttableNotesCount != 0){
            int diff = self->difficultyBeatmap->get_difficultyRank();
            std::string idString = "";
            if (mapType.compare("Standard") != 0) idString = mapType + std::to_string(diff);
            else idString =  std::to_string(diff);
            int misses = self->levelCompletionResults->missedCount;
            int badCut = self->levelCompletionResults->badCutsCount;
            ConfigHelper::UpdateBeatMapInfo(mapID, idString, misses, badCut, pauseCount, currentTime);
        }
        TMPro::TextMeshProUGUI* rankTitleText = self->get_transform()->Find(il2cpp_utils::newcsstr("Container/ClearedInfo/RankTitle"))->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        //Rank Text Changes
        if (ScoreDetails::config.LevelEndRank)
        {
            rankTitleText->SetText(il2cpp_utils::newcsstr("PERCENTAGE"));
            //Set Percentage to first line
            rankTextLine1 = Round(resultPercentage, 2) + "<size=45%>%";
            if (currentScore !=0 && ScoreDetails::config.ScorePercentageDifference && !self->get_practice()) rankTextLine2 = valueDifferenceString(resultPercentage - currentPercentage) + "<size=30%>%";
            self->rankText->SetText(il2cpp_utils::newcsstr(" <line-height=30%><size=60%>" + rankTextLine1 + "\n" + rankTextLine2));
        }

        if (!ScoreDetails::config.LevelEndRank)
        {
            rankTitleText->SetText(il2cpp_utils::newcsstr("RANK"));
            std::string rankTextLine1 = to_utf8(csstrtostr(self->rankText->get_text()));
            if (ScoreDetails::config.ScorePercentageDifference && currentScore != 0 && !self->get_practice())
            {
                rankTextLine2 = valueDifferenceString(resultPercentage - currentPercentage) + "<size=30%>%";
                self->rankText->SetText(il2cpp_utils::newcsstr("  <line-height=30%><size=60%>" + rankTextLine1 + "\n<line-height=30%><size=40%>" + rankTextLine2));
            }
            else self->rankText->SetText(il2cpp_utils::newcsstr(rankTextLine1));
                
        } 

        //Add ScoreDifference Calculation if enabled
        if (ScoreDetails::config.ScoreDifference && !self->get_practice() && currentScore != 0)
        {
            std::string scoreDifferenceText;
            self->newHighScoreText->SetActive(false);
            scoreDifferenceText = valueDifferenceString((modifiedScore - currentScore));
            self->scoreText->SetText(il2cpp_utils::newcsstr("<line-height=30%><size=60%> " + to_utf8(csstrtostr(ScoreFormatter::Format(modifiedScore))) + "\n<size=40%>" + scoreDifferenceText));
        }

        std::string goodCutsText = to_utf8(csstrtostr(self->goodCutsPercentageText->get_text()));
        if(ScoreDetails::config.missDifference && ScoreDetails::config.badCutCount != -1 && ScoreDetails::config.missCount != -1 && !self->get_practice() && currentScore != 0)
        {
            std::string missDifferenceText;
            int missDifference = (ScoreDetails::config.badCutCount + ScoreDetails::config.missCount) - (self->levelCompletionResults->missedCount + self->levelCompletionResults->badCutsCount);
            missDifferenceText = valueDifferenceString(missDifference);
            self->goodCutsPercentageText->set_enableWordWrapping(false);
            self->goodCutsPercentageText->SetText(il2cpp_utils::newcsstr(goodCutsText + " <size=50%>(" + missDifferenceText + "</color>)"));
        }
        else self->goodCutsPercentageText->SetText(il2cpp_utils::newcsstr(goodCutsText));
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
            getLogger().info("Old Score: %s", std::to_string(multiCurrentScore).c_str());
            getLogger().info("New Score: %s", std::to_string(levelCompletionResults->modifiedScore).c_str());
            getLogger().info("Player Data : %s", std::to_string(multiplayerMapData.statsdata->get_highScore()).c_str());
            std::string currentTime = to_utf8(csstrtostr(System::DateTime::get_UtcNow().ToLocalTime().ToString(il2cpp_utils::newcsstr("D"))));
            int diff = multiplayerMapData.beatmap->get_difficultyRank();
            std::string idString = "";
            mapType = to_utf8(csstrtostr(multiplayerMapData.statsdata->get_beatmapCharacteristic()->get_serializedName()));
            mapID = to_utf8(csstrtostr(multiplayerMapData.statsdata->get_levelID()));
            if (mapType.compare("Standard") != 0) idString = mapType + std::to_string(diff);
            else idString = std::to_string(diff);
            int misses = levelCompletionResults->missedCount;
            int badCut = levelCompletionResults->badCutsCount;
            ConfigHelper::UpdateBeatMapInfo(mapID, idString, misses, badCut, pauseCount, currentTime);
        }
        else getLogger().info("New Score was not better. data not submitted");
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(GameplayCoreSceneSetupData_ctor, "", "GameplayCoreSceneSetupData", ".ctor", void, GameplayCoreSceneSetupData* self, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects, EnvironmentInfoSO* environmentInfo, ColorScheme* colorScheme)
{
    GameplayCoreSceneSetupData_ctor(self, difficultyBeatmap, previewBeatmapLevel, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects, environmentInfo, colorScheme);
    multiplayerMapData.beatmap = difficultyBeatmap;
    multiplayerMapData.statsdata = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<PlayerDataModel*>())->playerData->GetPlayerLevelStatsData(difficultyBeatmap);
    multiCurrentScore = multiplayerMapData.statsdata->get_highScore();
    pauseCount = 0;
    scoreDetailsUI->modal->Hide(true, nullptr);
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