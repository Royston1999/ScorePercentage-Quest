#include <iomanip>
#include <cmath>

#include "ScoreDetailsModal.hpp"
#include "ScoreUtils.hpp"
#include "SettingsFlowCoordinator.hpp"
#include "PPCalculator.hpp"
#include "main.hpp"

#include "questui/shared/QuestUI.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "System/Action.hpp"
#include "System/DateTime.hpp"
#include "System/Threading/Tasks/Task_1.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/SpriteRenderer.hpp"

#include "TMPro/TextMeshProUGUI.hpp"
#include "TMPro/TextMeshPro.hpp"

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
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/MultiplayerScoreRingItem.hpp"
#include "GlobalNamespace/MultiplayerScoreRingManager.hpp"
#include "GlobalNamespace/RelativeScoreAndImmediateRankCounter.hpp"
#include "GlobalNamespace/IScoreController.hpp"
#include "GlobalNamespace/MultiplayerLocalActiveClient.hpp"
#include "GlobalNamespace/MultiplayerScoreProvider.hpp"
#include "GlobalNamespace/MultiplayerScoreProvider_RankedPlayer.hpp"
#include "GlobalNamespace/MultiplayerConnectedPlayerScoreDiffText.hpp"
#include "GlobalNamespace/DifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapLevelData.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/MainSettingsModelSO.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/SliderData.hpp"
#include "GlobalNamespace/MultiplayerConnectedPlayerSongTimeSyncController.hpp"
#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/HealthWarningFlowCoordinator.hpp"
#include "GlobalNamespace/HealthWarningFlowCoordinator_InitData.hpp"
#include "GlobalNamespace/GameScenesManager.hpp"
#include "GlobalNamespace/PlayerAgreements.hpp"
#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"

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
BeatmapLevelsModel* model = nullptr;
std::string nitoID = "QXzRo+cwkPArBeq+0i4yxw";
bool hasBeenNitod;
bool hasDiedinMulti;
std::vector<int> routines;
std::vector<std::pair<int, float>> scoreValues;
std::pair<int, int> indexScore;
AudioTimeSyncController* timeController;
custom_types::Helpers::Coroutine FuckYouBeatSaviorData(LevelStatsView* self);
custom_types::Helpers::Coroutine DoNewPercentageStuff(IDifficultyBeatmap* difficultyBeatmap);
bool noException = false;
bool successfullySkipped = false;

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

float LerpUnclamped(float a, float b, float t){
	return a + (b - a) * t;
}

custom_types::Helpers::Coroutine FuckYouBeatSaviorData(LevelStatsView* self)
{
    bool beatBeyondSaving = false;
    for (int i=0; i<3; i++){
        if (i == 2){
            auto beatMySavior = QuestUI::ArrayUtil::First(self->get_transform()->get_parent()->GetComponentsInChildren<Button*>(), [](Button* x) { return x->get_name() == "BeatSaviorDataDetailsButton"; });
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

int FixYourShitBeatGames(IReadonlyBeatmapData* data){
    scoreValues.clear(); std::vector<std::pair<int, float>>().swap(scoreValues);
    auto* notes = List<NoteData*>::New_ctor(); notes->AddRange(data->GetBeatmapDataItems<NoteData*>());
    auto* sliders = List<SliderData*>::New_ctor(); sliders->AddRange(data->GetBeatmapDataItems<SliderData*>());
    auto itr1 = notes->GetEnumerator();
    while (itr1.MoveNext()){
        auto* noteData = itr1.get_Current();
        if (noteData->get_scoringType() != -1 && noteData->get_scoringType() != 0 && noteData->get_scoringType() != 4){
            scoreValues.push_back(std::make_pair(115, noteData->get_time()));
        }
    }
    auto itr2 = sliders->GetEnumerator();
    while (itr2.MoveNext()){
        auto* sliderData = itr2.get_Current();
        if (sliderData->get_sliderType() == 1){
            scoreValues.push_back(std::make_pair(85, sliderData->get_time()));
            for (int i = 1; i < sliderData->get_sliceCount(); i++){
                float t = i / (sliderData->get_sliceCount() - 1);
                scoreValues.push_back(std::make_pair(20, LerpUnclamped(sliderData->get_time(), sliderData->get_tailTime(), t)));
            }
        }
    }
    std::sort(scoreValues.begin(), scoreValues.end(), [](auto &left, auto &right) { return left.second < right.second; });
    int count = 0, multiplier = 0, maxScore = 0;
    for (auto& p : scoreValues){
        count++;
        multiplier = count < 2 ? 1 : count < 6 ? 2 : count < 14 ? 4 : 8;
        maxScore += p.first * multiplier;
    }
    getLogger().info("max score: %i", maxScore);
    return maxScore;
}

custom_types::Helpers::Coroutine DoNewPercentageStuff(IDifficultyBeatmap* difficultyBeatmap)
{
    int crIndex = routines.size() + 1; routines.push_back(crIndex);
    if (scoreDetailsUI != nullptr) scoreDetailsUI->loadingInfo();
    if (model == nullptr) model = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<BeatmapLevelsModel*>());
    auto* envInfo = model->GetLevelPreviewForLevelId(mapData.mapID)->get_environmentInfo();
    auto* result = difficultyBeatmap->GetBeatmapDataAsync(envInfo);
    while (!result->get_IsCompleted()) co_yield nullptr;
    auto* data = result->get_ResultOnSuccess();
    if (routines.empty() || routines.size() != crIndex) co_return;
    routines.clear(); std::vector<int>().swap(routines);
    int maxScore = data != nullptr ? ScoreModel::ComputeMaxMultipliedScoreForBeatmap(data) : 1;
    float currentPercentage = calculatePercentage(maxScore, mapData.currentScore);
    mapData.currentPercentage = currentPercentage; mapData.maxScore = maxScore;
    if (scoreDetailsUI != nullptr) data != nullptr ? scoreDetailsUI->updateInfo() : scoreDetailsUI->loadingFailed();
    co_return;
}

void updateMapData(PlayerLevelStatsData* playerLevelStatsData, IDifficultyBeatmap* difficultyBeatmap){
    int currentScore = playerLevelStatsData->get_highScore();
    std::string mapID = playerLevelStatsData->get_levelID();
    std::string mapType = playerLevelStatsData->get_beatmapCharacteristic()->get_serializedName();
    mapData.mapID = mapID;
    int diff = difficultyBeatmap->get_difficultyRank();
    mapData.currentScore = currentScore;
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(DoNewPercentageStuff(difficultyBeatmap)));
    mapData.diff = difficultyBeatmap->get_difficulty();
    mapData.mapType = mapType;
    mapData.isFC = playerLevelStatsData->get_fullCombo();
    mapData.playCount = playerLevelStatsData->get_playCount();
    mapData.maxCombo = playerLevelStatsData->get_maxCombo();
    mapData.idString = mapType.compare("Standard") != 0 ? mapType + std::to_string(diff) : std::to_string(diff);
}

void toggleMultiResultsTableFormat(bool value, ResultsTableCell* cell){
    cell->dyn__rankText()->set_enableWordWrapping(!value);
    cell->dyn__rankText()->set_richText(value);
    cell->dyn__scoreText()->set_richText(value);
    cell->dyn__scoreText()->set_enableWordWrapping(!value);
    int multiplier = value ? 1 : -1;
    cell->dyn__scoreText()->get_transform()->set_localPosition(cell->dyn__scoreText()->get_transform()->get_localPosition() + UnityEngine::Vector3(multiplier * -5, 0, 0));
    cell->dyn__rankText()->get_transform()->set_localPosition(cell->dyn__rankText()->get_transform()->get_localPosition() + UnityEngine::Vector3(multiplier * -2, 0, 0));
}

void toggleModalVisibility(bool value, LevelStatsView* self){
    scoreDetailsUI->openButton->get_gameObject()->SetActive(value);
    scoreDetailsUI->hasValidScoreData = value;
    if (!value){
        noException = true;
        scoreDetailsUI->modal->Hide(true, nullptr);
    }
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(FuckYouBeatSaviorData(self)));
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
    // throw il2cpp_utils::RunMethodException("sc2ad still crashing my game :(", nullptr);
    if (noException) noException = false;
    if (playerData != nullptr)
    {
        if (scoreDetailsUI == nullptr || modalSettingsChanged) ScorePercentage::initModalPopup(&scoreDetailsUI, self->get_transform());
        auto* playerLevelStatsData = playerData->GetPlayerLevelStatsData(difficultyBeatmap);
        updateMapData(playerLevelStatsData, difficultyBeatmap);
        if (playerLevelStatsData->get_validScore())
        {
            ConfigHelper::LoadBeatMapInfo(mapData.mapID, mapData.idString);
            if (scorePercentageConfig.MenuHighScore) toggleModalVisibility(true, self);
        }
        if (!scorePercentageConfig.MenuHighScore || !playerLevelStatsData->get_validScore()) toggleModalVisibility(false, self);
        if (self->get_isActiveAndEnabled() && scoreDetailsUI->hasValidScoreData && scorePercentageConfig.alwaysOpen && scorePercentageConfig.MenuHighScore) scoreDetailsUI->modal->Show(true, true, nullptr);
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
    bool isParty = flowthingy != nullptr ? flowthingy->get_isActivated() ? true : false : false;

    // funny thing i wonder if anyone notices
    if (firstActivation) CreateText(self->get_transform(), "<size=150%>KNOBHEAD</size>", UnityEngine::Vector2(20, 20));

    // Default Info Texts
    std::string rankText = self->dyn__rankText()->get_text();
    std::string scoreText = self->dyn__scoreText()->get_text();
    std::string missText = self->dyn__goodCutsPercentageText()->get_text();

    bool isValidScore = !(self->get_practice() || mapData.currentScore == 0 || isParty);

    // only update stuff if level was cleared
    if (self->dyn__levelCompletionResults()->dyn_levelEndStateType() == GlobalNamespace::LevelCompletionResults::LevelEndStateType::Cleared)
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
        if ((resultScore - mapData.currentScore) > 0 && !self->get_practice() && !isParty)
        {
            int misses = self->dyn__levelCompletionResults()->dyn_missedCount();
            int badCut = self->dyn__levelCompletionResults()->dyn_badCutsCount();
            std::string currentTime = System::DateTime::get_UtcNow().ToLocalTime().ToString("D");
            ConfigHelper::UpdateBeatMapInfo(mapData.mapID, mapData.idString, misses, badCut, pauseCount, currentTime);
        }
    }
}

MAKE_HOOK_MATCH(MultiplayerResults, &ResultsTableCell::SetData, void, ResultsTableCell* self, int order, IConnectedPlayer* connectedPlayer, LevelCompletionResults* levelCompletionResults){
    MultiplayerResults(self, order, connectedPlayer, levelCompletionResults);
    getLogger().info("estimated percentage: %.2f", calculatePercentage(indexScore.second, levelCompletionResults->dyn_modifiedScore()));
    bool passedLevel = levelCompletionResults->dyn_levelEndStateType() == 1 ? true : false;
    if (scorePercentageConfig.LevelEndRank){
        if (!self->dyn__rankText()->get_richText()) toggleMultiResultsTableFormat(true, self);
        bool isNoFail = levelCompletionResults->dyn_gameplayModifiers()->get_noFailOn0Energy() && levelCompletionResults->dyn_energy() == 0;
        std::string percentageText = Round(calculatePercentage(mapData.maxScore, levelCompletionResults->dyn_modifiedScore()), 2);
        self->dyn__rankText()->SetText(percentageText + "<size=75%>%</size>");
        std::string score = self->dyn__scoreText()->get_text();
        std::string preText = isNoFail ? "NF" + tab : !passedLevel ? "F" + tab : "";
        int totalMisses = levelCompletionResults->dyn_missedCount() + levelCompletionResults->dyn_badCutsCount();
        std::string missText = levelCompletionResults->dyn_fullCombo() ? "FC" : preText + "<color=red>X</color><size=65%> </size>" + std::to_string(totalMisses);
        self->dyn__scoreText()->SetText(missText + tab + score);
    }
    else{
        if (self->dyn__rankText()->get_richText()) toggleMultiResultsTableFormat(false, self);
        if (!passedLevel) self->dyn__rankText()->SetText("F");
        else self->dyn__rankText()->SetText(GlobalNamespace::RankModel::GetRankName(levelCompletionResults->dyn_rank()));
    }
    if (connectedPlayer->get_isMe() && (levelCompletionResults->dyn_modifiedScore() - mapData.currentScore > 0) && passedLevel){
        int misses = levelCompletionResults->dyn_missedCount();
        int badCut = levelCompletionResults->dyn_badCutsCount();
        std::string currentTime = System::DateTime::get_UtcNow().ToLocalTime().ToString("D");
        ConfigHelper::UpdateBeatMapInfo(mapData.mapID, mapData.idString, misses, badCut, pauseCount, currentTime);
    }
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(GameplayCoreSceneSetupData_ctor, "", "GameplayCoreSceneSetupData", ".ctor", void, GameplayCoreSceneSetupData* self, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects, EnvironmentInfoSO* environmentInfo, ColorScheme* colorScheme, MainSettingsModelSO* mainSettingsModel)
{
    GameplayCoreSceneSetupData_ctor(self, difficultyBeatmap, previewBeatmapLevel, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects, environmentInfo, colorScheme, mainSettingsModel);
    auto* playerLevelStatsData = QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<PlayerDataModel*>())->get_playerData()->GetPlayerLevelStatsData(difficultyBeatmap);
    if (mapData.mapID != playerLevelStatsData->get_levelID() || mapData.diff != difficultyBeatmap->get_difficulty() || mapData.mapType != playerLevelStatsData->get_beatmapCharacteristic()->get_serializedName()) updateMapData(playerLevelStatsData, difficultyBeatmap);
    pauseCount = 0;
    noException = true;
    if (scoreDetailsUI != nullptr) scoreDetailsUI->modal->Hide(true, nullptr);
    scoreController = nullptr;
    timeController = nullptr;
    hasBeenNitod = false;
    hasDiedinMulti = false;
}

MAKE_HOOK_MATCH(PPTime, &MainMenuViewController::DidActivate, void, MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    PPTime(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    if (firstActivation) PPCalculator::PP::Initialize();
}

MAKE_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &GlobalNamespace::MenuTransitionsHelper::RestartGame, void, GlobalNamespace::MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback)
{
    scoreDetailsUI = nullptr;
    origRankText = nullptr;
    scoreDiffText = nullptr;
    rankDiffText = nullptr;
    model = nullptr;
    MenuTransitionsHelper_RestartGame(self, finishCallback);
}

MAKE_HOOK_MATCH(ScoreRingManager_UpdateScoreText, &MultiplayerScoreRingManager::UpdateScore, void, MultiplayerScoreRingManager* self, IConnectedPlayer* playerToUpdate){
    if (scorePercentageConfig.LevelEndRank){
        MultiplayerScoreProvider::RankedPlayer* player;
        auto* scoreRingItem = self->GetScoreRingItem(playerToUpdate->get_userId());
        if (!hasBeenNitod && playerToUpdate->get_userId() == nitoID){
            hasBeenNitod = true;
            scoreRingItem->SetName("MUNCHKIN");
        }
        bool flag = self->dyn__scoreProvider()->TryGetScore(playerToUpdate->get_userId(), player);
        if (!flag || player->get_isFailed()){
            scoreRingItem->SetScore("X");
            return;
        }
        else if (timeController != nullptr){
            float timeValue;
            float currentSongTime = timeController->dyn__songTime();
            int multiplier = 0;
            for (int i = indexScore.first; i<scoreValues.size(); i++){
                if (indexScore.first == -1) break;
                timeValue = scoreValues[i].second;
                int index = i + 1;
                if (timeValue < currentSongTime){
                    multiplier = index < 2 ? 1 : index < 6 ? 2 : index < 14 ? 4 : 8;
                    indexScore.second += scoreValues[i].first * multiplier;
                    if (i == scoreValues.size() -1) indexScore.first = -1;
                }
                else {
                    indexScore.first = i;
                    break;
                }
            }
            int userScore = player->get_score();
            std::string userPercentage = Round(calculatePercentage(indexScore.second, userScore), 2);
            scoreRingItem->SetScore(std::to_string(userScore) + " (" + userPercentage + "%)");
        }
    }
    else ScoreRingManager_UpdateScoreText(self, playerToUpdate);
    // if (!hasDiedinMulti && flag && player->get_isMe() && player->get_isFailed()) hasDiedinMulti = true;
}

MAKE_HOOK_MATCH(ScoreDiffText, &MultiplayerConnectedPlayerScoreDiffText::AnimateScoreDiff, void, MultiplayerConnectedPlayerScoreDiffText* self, int scoreDiff){
    ScoreDiffText(self, scoreDiff);
    if (timeController != nullptr && scorePercentageConfig.LevelEndRank){
        if(self->dyn__onPlatformText()->get_enableWordWrapping()){
            self->dyn__onPlatformText()->set_richText(true);
            self->dyn__onPlatformText()->set_enableWordWrapping(false);
            auto* transform = (UnityEngine::RectTransform*)(self->dyn__backgroundSpriteRenderer()->get_transform());
            transform->set_localScale({transform->get_localScale().x *1.7f, transform->get_localScale().y, 0.0f});
        }
        std::string baseText = self->dyn__onPlatformText()->get_text();
        int maxPossibleScore = indexScore.second;
        std::string posneg = (scoreDiff >= 0) ? "+" : "";
        std::string percentageText = " (" + posneg + Round(calculatePercentage(maxPossibleScore, scoreDiff), 2) + "%)";
        self->dyn__onPlatformText()->SetText(baseText + percentageText);
    }
    else if (!scorePercentageConfig.LevelEndRank){
        if(!self->dyn__onPlatformText()->get_enableWordWrapping()){
            self->dyn__onPlatformText()->set_richText(false);
            self->dyn__onPlatformText()->set_enableWordWrapping(true);
            auto* transform = (UnityEngine::RectTransform*)(self->dyn__backgroundSpriteRenderer()->get_transform());
            transform->set_localScale({transform->get_localScale().x /1.7f, transform->get_localScale().y, 0.0f});
        }
    }
}

MAKE_HOOK_MATCH(ScoreStuff, &MultiplayerLocalActiveClient::Start, void, MultiplayerLocalActiveClient* self){
    ScoreStuff(self);
    if (timeController == nullptr){
        timeController = self->dyn__audioTimeSyncController();
        scoreController = self->dyn__scoreController();
        indexScore = std::make_pair(0, 0);
    }
}

MAKE_HOOK_MATCH(Modal_Hide, &HMUI::ModalView::Hide, void, HMUI::ModalView* self, bool animated, System::Action* finishedCallback){
    std::string modalName = self->get_name();
    std::string myName = "ScoreDetailsModal";

    if (scoreDetailsUI != nullptr && modalName.compare(myName) == 0 && !noException && scorePercentageConfig.alwaysOpen){
        return;
    }
    else Modal_Hide(self, animated, finishedCallback);
}

MAKE_HOOK_MATCH(LeaderBoard_Activate, &PlatformLeaderboardViewController::DidActivate, void, PlatformLeaderboardViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    LeaderBoard_Activate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    if (scoreDetailsUI != nullptr && scoreDetailsUI->hasValidScoreData && scorePercentageConfig.alwaysOpen && scorePercentageConfig.MenuHighScore) scoreDetailsUI->modal->Show(true, true, nullptr);
}

MAKE_HOOK_MATCH(LeaderBoard_Deactivate, &SoloFreePlayFlowCoordinator::BackButtonWasPressed, void, SinglePlayerLevelSelectionFlowCoordinator* self, HMUI::ViewController* topViewController){
    noException = topViewController != reinterpret_cast<HMUI::ViewController*>(self->dyn__practiceViewController());
    LeaderBoard_Deactivate(self, topViewController);
    if (scoreDetailsUI != nullptr && scoreDetailsUI->modal->dyn__isShown()){
        scoreDetailsUI->modal->Hide(false, nullptr);
    }
}
// quite a small hook
MAKE_HOOK_MATCH(ScoreModel_MaxScore, &ScoreModel::ComputeMaxMultipliedScoreForBeatmap, int, IReadonlyBeatmapData* beatmapData){
    return FixYourShitBeatGames(beatmapData);
}

MAKE_HOOK_MATCH(HealthSkip, &HealthWarningFlowCoordinator::DidActivate, void, HealthWarningFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    HealthSkip(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    bool eula = self->dyn__playerDataModel()->get_playerData()->get_playerAgreements()->AgreedToEula();
    bool helth = self->dyn__playerDataModel()->get_playerData()->get_playerAgreements()->AgreedToHealthAndSafety();
    if (eula && helth){
        successfullySkipped = true;
        auto* nextScene = self->dyn__initData()->dyn_nextScenesTransitionSetupData();
        self->dyn__gameScenesManager()->ReplaceScenes(nextScene, nullptr, 0.0f, nullptr, nullptr);
    }
}
MAKE_HOOK_MATCH(FuckOff, &HealthWarningFlowCoordinator::HandleHealthWarningViewControllerDidFinish, void, HealthWarningFlowCoordinator* self){
    if (!successfullySkipped) FuckOff(self);
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
    INSTALL_HOOK(getLogger(), Modal_Hide);
    INSTALL_HOOK(getLogger(), LeaderBoard_Activate);
    INSTALL_HOOK(getLogger(), LeaderBoard_Deactivate);
    INSTALL_HOOK_ORIG(getLogger(), ScoreModel_MaxScore);

    INSTALL_HOOK(getLogger(), HealthSkip);
    INSTALL_HOOK(getLogger(), FuckOff);
    getLogger().info("Installed all hooks!");
}