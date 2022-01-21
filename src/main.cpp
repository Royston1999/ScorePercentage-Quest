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

using namespace QuestUI::BeatSaberUI;
using namespace UnityEngine::UI;
using namespace UnityEngine;
using namespace GlobalNamespace;

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

static std::string Round (float val, int precision)
{
	std::stringstream stream;
    stream << std::fixed << std::setprecision(precision) << val;
    std::string Out = stream.str();
	return Out;
}

int currentScore = 0;
int multiCurrentScore = 0;
double currentPercentage = 0;
HMUI::ModalView* screen = nullptr;
UnityEngine::UI::GridLayoutGroup* buttonHolder = nullptr;
std::string mapID = "";
std::string mapType = "";
std::string idString = "";
TMPro::TextMeshProUGUI* score;
TMPro::TextMeshProUGUI* maxCombo;
TMPro::TextMeshProUGUI* playCount;
TMPro::TextMeshProUGUI* missCount;
TMPro::TextMeshProUGUI* badCutCount;
TMPro::TextMeshProUGUI* pauseCountGUI;
TMPro::TextMeshProUGUI* datePlayed;
std::string scoreText;
std::string maxComboText;
std::string playCountText;
std::string missCountText;
std::string badCutCountText;
std::string pauseCountText;
std::string datePlayedText;
std::string statsButtonText = "VIEW SCORE DETAILS";
std::string testThingy = "";
UnityEngine::UI::Button* statsButton;
Array<TMPro::TextMeshProUGUI*>* levelStatsComponents;
int pauseCount = 0;
std::string colorPositive = "<color=#00B300>";
std::string colorNegative = "<color=#FF0000>";
std::string colorNoMiss = "<color=#05BCFF>";
std::string ppColour = "<color=#5968BB>";
int totalUIText = 0;
bool ScoreDetails::modalSettingsChanged = false;
int multiLevelMaxScore = 0;

static int calculateMaxScore(int blockCount)
{
    int maxScore;
    if(blockCount < 14)
    {
        if (blockCount == 1)
            maxScore = 115;
        else if (blockCount < 5)
            maxScore = (blockCount - 1) * 230 + 115;
        else
            maxScore = (blockCount - 5) * 460 + 1035;
    }
    else
        maxScore = (blockCount - 13) * 920 + 4715;
    return maxScore;
}

static std::string valueDifferenceString(float valueDifference){
    std::string differenceColor, positiveIndicator;
    //Better or same Score
    if (valueDifference >= 0)
    {
        differenceColor = colorPositive;
        positiveIndicator = "+";
    }
    //Worse Score
    else
    {
        differenceColor = colorNegative;
        positiveIndicator = "";
    }
    return differenceColor + positiveIndicator + (ceilf(valueDifference) != valueDifference ? Round(valueDifference, 2) : Round(valueDifference, 0));
}

static double calculatePercentage(int maxScore, int resultScore)
{
    double resultPercentage = (double)(100 / (double)maxScore * (double)resultScore);
    return resultPercentage;
}

struct MultiplayerMapData{
    IDifficultyBeatmap* beatmap;
    PlayerLevelStatsData* statsdata;
} multiplayerMapData;

void initScoreDetailsUI(LevelStatsView* self){
    totalUIText = ScoreDetails::config.uiPlayCount + ScoreDetails::config.uiMissCount + ScoreDetails::config.uiBadCutCount + ScoreDetails::config.uiPauseCount + ScoreDetails::config.uiDatePlayed;
    if (screen!=nullptr) GameObject::Destroy(screen->get_gameObject());
    if (buttonHolder!=nullptr) GameObject::Destroy(buttonHolder->get_gameObject());
    // 55
    int x = 25 + (6 * totalUIText);
    screen = CreateModal(self->get_transform(), UnityEngine::Vector2(60, x), [](HMUI::ModalView *modal) {}, true);
    VerticalLayoutGroup* list = CreateVerticalLayoutGroup(screen->get_transform());
    list->set_spacing(-1.0f);
    list->set_padding(UnityEngine::RectOffset::New_ctor(7, 0, 10, 1));
    list->set_childForceExpandWidth(true);
    list->set_childControlWidth(false);
    // 21
    int y = 6 + (3 * totalUIText);
    auto* details = CreateText(screen->get_transform(), "<size=150%>SCORE DETAILS</size>", UnityEngine::Vector2(13, y));
    score = CreateText(list->get_transform(), "");
    maxCombo = CreateText(list->get_transform(), "");
    if (ScoreDetails::config.uiPlayCount) playCount = CreateText(list->get_transform(), "");
    if (ScoreDetails::config.uiMissCount) missCount = CreateText(list->get_transform(), "");
    if (ScoreDetails::config.uiBadCutCount) badCutCount = CreateText(list->get_transform(), "");
    if (ScoreDetails::config.uiPauseCount) pauseCountGUI = CreateText(list->get_transform(), "");
    if (ScoreDetails::config.uiDatePlayed) datePlayed = CreateText(list->get_transform(), "");
    buttonHolder = CreateGridLayoutGroup(self->get_transform());
    buttonHolder->set_cellSize({70.0f, 10.0f});
    statsButton = CreateUIButton(buttonHolder->get_transform(), statsButtonText, "PracticeButton", {0, 0}, {70.0f, 10.0f}, [](){
        screen->Show(true, true, nullptr);
    });
}

MAKE_HOOK_MATCH(MenuTransitionsHelper_StartStandardLevel, static_cast<void (MenuTransitionsHelper::*)(Il2CppString* gameMode,
    IDifficultyBeatmap* difficultyBeatmap,
    IPreviewBeatmapLevel* previewBeatmapLevel,
    OverrideEnvironmentSettings* overrideEnvironmentSettings,
    ColorScheme* overrideColorScheme,
    GameplayModifiers* gameplayModifiers,
    PlayerSpecificSettings* playerSpecificSettings,
    PracticeSettings* practiceSettings,
    Il2CppString* backButtonText,
    bool useTestNoteCutCountEffects,
    System::Action* beforeSceneSwitchCallback,
    System::Action_2<StandardLevelScenesTransitionSetupDataSO*, LevelCompletionResults*>* levelFinishedCallback)>(&MenuTransitionsHelper::StartStandardLevel),
    void,
    MenuTransitionsHelper* self,
    Il2CppString* gameMode,
    IDifficultyBeatmap* difficultyBeatmap,
    IPreviewBeatmapLevel* previewBeatmapLevel,
    OverrideEnvironmentSettings* overrideEnvironmentSettings,
    ColorScheme* overrideColorScheme,
    GameplayModifiers* gameplayModifiers,
    PlayerSpecificSettings* playerSpecificSettings,
    PracticeSettings* practiceSettings,
    Il2CppString* backButtonText,
    bool useTestNoteCutCountEffects,
    System::Action* beforeSceneSwitchCallback,
    System::Action_2<StandardLevelScenesTransitionSetupDataSO*, LevelCompletionResults*>* levelFinishedCallback) {
    MenuTransitionsHelper_StartStandardLevel(
        self,
        gameMode,
        difficultyBeatmap,
        previewBeatmapLevel,
        overrideEnvironmentSettings,
        overrideColorScheme,
        gameplayModifiers,
        playerSpecificSettings,
        practiceSettings,
        backButtonText,
        useTestNoteCutCountEffects,
        beforeSceneSwitchCallback,
        levelFinishedCallback);

    if (screen != nullptr) if (screen->get_gameObject() != nullptr && screen->isShown) screen->Hide(true, nullptr);
}

MAKE_HOOK_MATCH(Menu, &LevelStatsView::ShowStats, void, LevelStatsView* self, IDifficultyBeatmap* difficultyBeatmap, PlayerData* playerData) {
    Menu(self, difficultyBeatmap, playerData);
    pauseCount = 0;
    if (playerData != nullptr)
    {
        if((screen == nullptr && buttonHolder == nullptr) || ScoreDetails::modalSettingsChanged){
            initScoreDetailsUI(self);
            ScoreDetails::modalSettingsChanged = false;
        }
        levelStatsComponents = self->get_transform()->GetComponentsInChildren<TMPro::TextMeshProUGUI*>();
        GlobalNamespace::PlayerLevelStatsData* playerLevelStatsData = playerData->GetPlayerLevelStatsData(difficultyBeatmap);
        currentScore = playerLevelStatsData->highScore;
        mapID = to_utf8(csstrtostr(playerLevelStatsData->get_levelID()));
        mapType = to_utf8(csstrtostr(playerLevelStatsData->get_beatmapCharacteristic()->get_serializedName()));
        if (playerLevelStatsData->validScore)
        {
            //calculate maximum possilble score
            int currentDifficultyMaxScore = calculateMaxScore(difficultyBeatmap->get_beatmapData()->cuttableNotesCount);

            float maxPP = mapType.compare("Standard") == 0 ? PPCalculator::PP::BeatmapMaxPP(mapID, difficultyBeatmap->get_difficulty()) : -1;
            float truePP = PPCalculator::PP::CalculatePP(maxPP, calculatePercentage(currentDifficultyMaxScore, playerLevelStatsData->highScore)/100);
            
            //calculate actual score percentage
            double currentDifficultyPercentageScore = calculatePercentage(currentDifficultyMaxScore, playerLevelStatsData->highScore);
            currentPercentage = currentDifficultyPercentageScore;

            if (mapType.compare("Standard") != 0) idString = mapType + std::to_string(difficultyBeatmap->get_difficultyRank());
            else idString = std::to_string(difficultyBeatmap->get_difficultyRank());
            ConfigHelper::LoadBeatMapInfo(mapID, idString);

            if (ScoreDetails::config.MenuHighScore)
            { 
                for(int i=0; i<levelStatsComponents->get_Length(); i++) if (!(to_utf8(csstrtostr((*levelStatsComponents)[i]->get_text())).compare(statsButtonText)==0))(*levelStatsComponents)[i]->set_enabled(false);
                buttonHolder->get_gameObject()->SetActive(true);
                scoreText = "Score - " + to_utf8(csstrtostr(ScoreFormatter::Format(playerLevelStatsData->highScore))) + " (<color=#EBCD00>" + Round(currentDifficultyPercentageScore, 2) + "%</color>)" + ((maxPP != -1 && ScoreDetails::config.uiPP) ? " - (" + ppColour + Round(truePP, 2) + "<size=60%>pp</size></color>)" : "");
                maxComboText = "Max Combo - " + (playerLevelStatsData->fullCombo ? "Full Combo" : std::to_string(playerLevelStatsData->maxCombo));
                playCountText = "Play Count - " + std::to_string(playerLevelStatsData->playCount);
                missCountText = "Miss Count - " + (ScoreDetails::config.missCount != -1 ? (ScoreDetails::config.missCount == 0 ? colorNoMiss : colorNegative) + std::to_string(ScoreDetails::config.missCount) : "N/A");
                badCutCountText = "Bad Cut Count - " + (ScoreDetails::config.badCutCount != -1 ? (ScoreDetails::config.badCutCount == 0 ? colorNoMiss : colorNegative) + std::to_string(ScoreDetails::config.badCutCount) : "N/A");
                pauseCountText = "Pause Count - " + (ScoreDetails::config.pauseCount != -1 ? (ScoreDetails::config.pauseCount == 0 ? colorNoMiss : colorNegative) + std::to_string(ScoreDetails::config.pauseCount) : "N/A");
                datePlayedText = "Date Played - " + (ScoreDetails::config.datePlayed.compare("") != 0 ? ("<size=85%><line-height=75%>" + ScoreDetails::config.datePlayed) : "N/A");
                score->SetText(il2cpp_utils::newcsstr(scoreText));
                maxCombo->SetText(il2cpp_utils::newcsstr(maxComboText));
                if (ScoreDetails::config.uiPlayCount) playCount->SetText(il2cpp_utils::newcsstr(playCountText));
                if (ScoreDetails::config.uiMissCount) missCount->SetText(il2cpp_utils::newcsstr(missCountText));
                if (ScoreDetails::config.uiBadCutCount) badCutCount->SetText(il2cpp_utils::newcsstr(badCutCountText));
                if (ScoreDetails::config.uiPauseCount) pauseCountGUI->SetText(il2cpp_utils::newcsstr(pauseCountText));
                if (ScoreDetails::config.uiDatePlayed) datePlayed->SetText(il2cpp_utils::newcsstr(datePlayedText));
            }
            else{
                screen->Hide(true, nullptr);
                buttonHolder->get_gameObject()->SetActive(false);
                for(int i=0; i<levelStatsComponents->get_Length(); i++) (*levelStatsComponents)[i]->set_enabled(true);
            }
        }
        else{
            screen->Hide(true, nullptr);
            buttonHolder->get_gameObject()->SetActive(false);
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
    int maxScore;
    double resultPercentage;
    int resultScore;
    int modifiedScore;
    // Default Rank Text
    std::string rankTextLine1 = to_utf8(csstrtostr(self->rankText->get_text()));
    std::string rankTextLine2 = "";

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
        // Add Percent Difference to 2nd Line if enabled and previous Score exists
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
        //End Preparations for Changes to Rank Text

        //Add ScoreDifference Calculation if enabled
        if (ScoreDetails::config.ScoreDifference && !self->get_practice() && currentScore != 0)
        {
            std::string scoreDifferenceText;
            self->newHighScoreText->SetActive(false);
            scoreDifferenceText = valueDifferenceString((modifiedScore - currentScore));
            self->scoreText->SetText(il2cpp_utils::newcsstr("<line-height=30%><size=60%> " + to_utf8(csstrtostr(ScoreFormatter::Format(modifiedScore))) + "\n<size=40%>" + scoreDifferenceText));
        }
        //End ScoreDifference Calculation
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

MAKE_HOOK_MATCH(test, &ResultsTableCell::SetData, void, ResultsTableCell* self, int order, IConnectedPlayer* connectedPlayer, LevelCompletionResults* levelCompletionResults){
    test(self, order, connectedPlayer, levelCompletionResults);

    if (!self->rankText->get_richText()){
            self->rankText->set_enableWordWrapping(false);
            self->rankText->set_richText(true);
            self->scoreText->set_enableWordWrapping(false);
            self->scoreText->set_richText(true);
            self->scoreText->get_transform()->set_localPosition(self->scoreText->get_transform()->get_localPosition() + UnityEngine::Vector3(-5, 0, 0));
            self->rankText->get_transform()->set_localPosition(self->rankText->get_transform()->get_localPosition() + UnityEngine::Vector3(-2, 0, 0));
        }

    std::string percentageText = Round(calculatePercentage(calculateMaxScore(multiplayerMapData.beatmap->get_beatmapData()->get_cuttableNotesCount()), levelCompletionResults->modifiedScore), 2);
    self->rankText->SetText(il2cpp_utils::newcsstr(percentageText + "<size=75%>%</size>"));
    std::string score = to_utf8(csstrtostr(self->scoreText->get_text()));
    std::string missText = levelCompletionResults->fullCombo ? "FC" : "<color=red>X</color><size=65%> </size>" + std::to_string(levelCompletionResults->missedCount + levelCompletionResults->badCutsCount);
    self->scoreText->SetText(il2cpp_utils::newcsstr(missText + "    " + score));

    getLogger().info("Old Score: %s", std::to_string(multiCurrentScore).c_str());
    getLogger().info("New Score: %s", std::to_string(levelCompletionResults->modifiedScore).c_str());

    if (connectedPlayer->get_isMe() && (levelCompletionResults->modifiedScore - multiCurrentScore > 0) && levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::Cleared){
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
    getLogger().info("current score: %s", std::to_string(multiCurrentScore).c_str());
}

MAKE_HOOK_MATCH(PPTime, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    PPTime(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    PPCalculator::PP::Initialize();
}

MAKE_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &GlobalNamespace::MenuTransitionsHelper::RestartGame, void, GlobalNamespace::MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback)
{
    UnityEngine::GameObject::Destroy(screen);
    UnityEngine::GameObject::Destroy(buttonHolder);
    screen = nullptr;
    buttonHolder = nullptr;
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
    // Install our hooks (none defined yet)
    INSTALL_HOOK(getLogger(), Menu);
    INSTALL_HOOK(getLogger(), Results);
    INSTALL_HOOK(getLogger(), test);
    INSTALL_HOOK(getLogger(), Pause);
    INSTALL_HOOK(getLogger(), PPTime);
    INSTALL_HOOK(getLogger(), MenuTransitionsHelper_StartStandardLevel);
    INSTALL_HOOK(getLogger(), GameplayCoreSceneSetupData_ctor)
    INSTALL_HOOK(getLogger(), MenuTransitionsHelper_RestartGame);
    getLogger().info("Installed all hooks!");
}