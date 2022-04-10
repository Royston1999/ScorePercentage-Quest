#include "MultiplayerHooks.hpp"

#include "bs-utils/shared/utils.hpp"

#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/IConnectedPlayer.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/MultiplayerConnectedPlayerScoreDiffText.hpp"
#include "GlobalNamespace/MultiplayerConnectedPlayerSongTimeSyncController.hpp"
#include "GlobalNamespace/MultiplayerLocalActiveClient.hpp"
#include "GlobalNamespace/MultiplayerScoreProvider.hpp"
#include "GlobalNamespace/MultiplayerScoreProvider_RankedPlayer.hpp"
#include "GlobalNamespace/MultiplayerScoreRingItem.hpp"
#include "GlobalNamespace/MultiplayerScoreRingManager.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/RankModel.hpp"
#include "GlobalNamespace/ResultsTableCell.hpp"
#include "GlobalNamespace/ResultsTableView.hpp"
#include "GlobalNamespace/BeatmapCallbacksController.hpp"
#include "GlobalNamespace/BeatmapCallbacksController_InitData.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/SliderData.hpp"

#include "System/DateTime.hpp"

#include "TMPro/TextMeshPro.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

#include "UnityEngine/SpriteRenderer.hpp"

#include "Utils/ScoreUtils.hpp"
#include "Utils/MapUtils.hpp"
#include "main.hpp"

using namespace GlobalNamespace;
using namespace ScorePercentage::Utils;
using namespace ScorePercentage::MapUtils;
using namespace UnityEngine;

std::string nitoID = "QXzRo+cwkPArBeq+0i4yxw";
bool hasBeenNitod;
std::pair<int, int> myIndexScore;
AudioTimeSyncController* myTimeController;
std::map<StringW, std::pair<std::pair<int, int>, MultiplayerConnectedPlayerSongTimeSyncController*>> playerInfos;
float modifierMultiplier;
std::vector<std::pair<int, float>> scoreValues;

void CreateScoreTimeValues(IReadonlyBeatmapData* data){
        ClearVector<std::pair<int, float>>(&scoreValues);
        auto* notes = GetBeatmapDataItems<NoteData*>(data);
        auto* sliders = GetBeatmapDataItems<SliderData*>(data);
        for (int i = 0; i < notes->size; i++){
            NoteData* noteData = notes->items[i];
            if (noteData->scoringType != -1 && noteData->scoringType != 0){
                scoreValues.push_back(std::make_pair(noteData->scoringType == 4 ? 85 : 115, noteData->time));
            }
        }
        for (int i = 0; i < sliders->size; i++){
            SliderData* sliderData = sliders->items[i];
            if (sliderData->sliderType == 1){
                for (int i = 1; i < sliderData->sliceCount; i++){
                    scoreValues.push_back(std::make_pair(20, LerpU(sliderData->time, sliderData->tailTime, i / (sliderData->sliceCount - 1))));
                }
            }
        }
        if (scoreValues.empty()) return;
        std::sort(scoreValues.begin(), scoreValues.end(), [](auto &left, auto &right) { return left.second < right.second; });
        int count = 0, multiplier = 0, maxScore = 0;
        for (auto& p : scoreValues){
            count++;
            multiplier = count < 2 ? 1 : count < 6 ? 2 : count < 14 ? 4 : 8;
            maxScore += (int)(p.first * multiplier * modifierMultiplier);
            p.first = maxScore;
        }
        getLogger().info("Last Index: %i\nMax Score: %i", scoreValues.back().first, maxScore);
    }

void toggleMultiResultsTableFormat(bool value, ResultsTableCell* cell){
    cell->dyn__rankText()->set_enableWordWrapping(!value);
    cell->dyn__rankText()->set_richText(value);
    cell->dyn__scoreText()->set_richText(value);
    cell->dyn__scoreText()->set_enableWordWrapping(!value);
    int multiplier = value ? 1 : -1;
    cell->dyn__scoreText()->get_transform()->set_localPosition(cell->dyn__scoreText()->get_transform()->get_localPosition() + Vector3(multiplier * -5, 0, 0));
    cell->dyn__rankText()->get_transform()->set_localPosition(cell->dyn__rankText()->get_transform()->get_localPosition() + Vector3(multiplier * -2, 0, 0));
}

MAKE_HOOK_MATCH(Results_SetData, &ResultsTableCell::SetData, void, ResultsTableCell* self, int order, IConnectedPlayer* connectedPlayer, LevelCompletionResults* levelCompletionResults){
    Results_SetData(self, order, connectedPlayer, levelCompletionResults);
    bool passedLevel = levelCompletionResults->dyn_levelEndStateType() == 1 ? true : false;
    if (scorePercentageConfig.multiLevelEndRank){
        if (!self->dyn__rankText()->get_richText()) toggleMultiResultsTableFormat(true, self);
        bool isNoFail = levelCompletionResults->dyn_gameplayModifiers()->get_noFailOn0Energy() && levelCompletionResults->dyn_energy() == 0;
        std::string percentageText = Round(calculatePercentage(mapData.maxScore, levelCompletionResults->dyn_modifiedScore()), 2);
        self->dyn__rankText()->SetText(percentageText + "<size=75%>%</size>");
        std::string score = self->dyn__scoreText()->get_text();
        std::string preText = !passedLevel ? "F" + tab : isNoFail ? "NF" + tab : "";
        int totalMisses = levelCompletionResults->dyn_missedCount() + levelCompletionResults->dyn_badCutsCount();
        std::string missText = levelCompletionResults->dyn_fullCombo() ? "FC" : preText + "<color=red>X</color><size=65%> </size>" + std::to_string(totalMisses);
        self->dyn__scoreText()->SetText(missText + tab + score);
    }
    else{
        if (self->dyn__rankText()->get_richText()) toggleMultiResultsTableFormat(false, self);
        if (!passedLevel) self->dyn__rankText()->SetText("F");
        else self->dyn__rankText()->SetText(RankModel::GetRankName(levelCompletionResults->dyn_rank()));
    }
    // write new highscore to file
    if (connectedPlayer->get_isMe() && (levelCompletionResults->dyn_modifiedScore() - mapData.currentScore > 0) && passedLevel && bs_utils::Submission::getEnabled()){
        int misses = levelCompletionResults->dyn_missedCount();
        int badCut = levelCompletionResults->dyn_badCutsCount();
        std::string currentTime = System::DateTime::get_UtcNow().ToLocalTime().ToString("D");
        ConfigHelper::UpdateBeatMapInfo(mapData.mapID, mapData.idString, misses, badCut, pauseCount, currentTime);
    }
}

MAKE_HOOK_MATCH(ScoreRingManager_UpdateScoreText, &MultiplayerScoreRingManager::UpdateScore, void, MultiplayerScoreRingManager* self, IConnectedPlayer* playerToUpdate){
    if (scorePercentageConfig.multiLivePercentages){
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
        if (scoreValues.empty()){
            scoreRingItem->SetScore("0 (0.00%)");
            return;
        }
        auto x = playerInfos.find(playerToUpdate->get_userId());
        float currentSongTime;
        std::pair<int, int>* indexScore;
        if (x != playerInfos.end()){
            currentSongTime = x->second.second->songTime;
            indexScore = &x->second.first;
        }
        else if (playerToUpdate->get_isMe()){
            currentSongTime = myTimeController->dyn__songTime();
            indexScore = &myIndexScore;
        }
        else return;
        for (int i = indexScore->first; i<scoreValues.size(); i++){
            float nextNoteTimeValue = scoreValues[i].second;
            float maxScoreAtNextNote = scoreValues[i].first;
            if (nextNoteTimeValue < currentSongTime) indexScore->second = maxScoreAtNextNote;
            else {
                indexScore->first = i; break;
            }
        }
        int userScore = player->get_score();
        std::string userPercentage = Round(calculatePercentage(indexScore->second, userScore), 2);
        scoreRingItem->SetScore(std::to_string(userScore) + " (" + userPercentage + "%)");
    }
    else ScoreRingManager_UpdateScoreText(self, playerToUpdate);
}

MAKE_HOOK_MATCH(ScoreDiff_UpdateText, &MultiplayerConnectedPlayerScoreDiffText::AnimateScoreDiff, void, MultiplayerConnectedPlayerScoreDiffText* self, int scoreDiff){
    ScoreDiff_UpdateText(self, scoreDiff);
    if (myTimeController != nullptr && scorePercentageConfig.multiPercentageDifference){
        if(self->dyn__onPlatformText()->get_enableWordWrapping()){
            self->dyn__onPlatformText()->set_richText(true);
            self->dyn__onPlatformText()->set_enableWordWrapping(false);
            auto* transform = (RectTransform*)(self->dyn__backgroundSpriteRenderer()->get_transform());
            transform->set_localScale({transform->get_localScale().x *2.0f, transform->get_localScale().y, 0.0f});
        }
        std::string baseText = self->dyn__onPlatformText()->get_text();
        int maxPossibleScore = myIndexScore.second;
        std::string posneg = (scoreDiff >= 0) ? "+" : "";
        std::string percentageText = " (" + posneg + Round(calculatePercentage(maxPossibleScore, scoreDiff), 2) + "%)";
        self->dyn__onPlatformText()->SetText(baseText + percentageText);
    }
    else if (!scorePercentageConfig.multiPercentageDifference){
        if(!self->dyn__onPlatformText()->get_enableWordWrapping()){
            self->dyn__onPlatformText()->set_richText(false);
            self->dyn__onPlatformText()->set_enableWordWrapping(true);
            auto* transform = (RectTransform*)(self->dyn__backgroundSpriteRenderer()->get_transform());
            transform->set_localScale({transform->get_localScale().x /2.0f, transform->get_localScale().y, 0.0f});
        }
    }
}

MAKE_HOOK_MATCH(Local_Start, &MultiplayerLocalActiveClient::Start, void, MultiplayerLocalActiveClient* self){
    Local_Start(self);
    myTimeController = self->dyn__audioTimeSyncController();
    myIndexScore = std::make_pair(0, 0);
}

MAKE_HOOK_MATCH(ConnectedPlayer_Start, &MultiplayerConnectedPlayerSongTimeSyncController::StartSong, void, MultiplayerConnectedPlayerSongTimeSyncController* self, float songStartSyncTime){
    ConnectedPlayer_Start(self, songStartSyncTime);
    playerInfos.insert(std::make_pair(self->connectedPlayer->get_userId(), std::make_pair(std::make_pair(0, 0), self)));
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(BeatmapData_Init, "", "BeatmapCallbacksController", ".ctor", void, BeatmapCallbacksController* self, BeatmapCallbacksController::InitData* initData)
{
    BeatmapData_Init(self, initData);
    CreateScoreTimeValues(initData->beatmapData);
}

void ScorePercentage::MultiplayerHooks::InstallHooks(){
    INSTALL_HOOK(getLogger(), Local_Start);
    INSTALL_HOOK(getLogger(), ConnectedPlayer_Start);
    INSTALL_HOOK(getLogger(), ScoreRingManager_UpdateScoreText);
    INSTALL_HOOK(getLogger(), ScoreDiff_UpdateText);
    INSTALL_HOOK(getLogger(), Results_SetData);
    INSTALL_HOOK(getLogger(), BeatmapData_Init);
}