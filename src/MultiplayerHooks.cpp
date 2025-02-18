#include "MultiplayerHooks.hpp"

#include "bs-utils/shared/utils.hpp"

#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/IConnectedPlayer.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/MultiplayerScoreDiffText.hpp"
#include "GlobalNamespace/MultiplayerConnectedPlayerSongTimeSyncController.hpp"
#include "GlobalNamespace/MultiplayerLocalActiveClient.hpp"
#include "GlobalNamespace/MultiplayerScoreProvider.hpp"
#include "GlobalNamespace/MultiplayerScoreRingItem.hpp"
#include "GlobalNamespace/MultiplayerScoreRingManager.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/RankModel.hpp"
#include "GlobalNamespace/ResultsTableCell.hpp"
#include "GlobalNamespace/ResultsTableView.hpp"
#include "GlobalNamespace/BeatmapCallbacksController.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/SliderData.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapDataSortedListForTypeAndIds_1.hpp"

#include "System/DateTime.hpp"

#include "TMPro/TextMeshPro.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

#include "UnityEngine/SpriteRenderer.hpp"

#include "Utils/ScoreUtils.hpp"
#include "Utils/MapUtils.hpp"
#include "main.hpp"
#include <type_traits>

using namespace GlobalNamespace;
using namespace ScorePercentage::Utils;
using namespace ScorePercentage::MapUtils;
using namespace UnityEngine;

bool isMapDataValid = false;
std::string nitoID = "QXzRo+cwkPArBeq+0i4yxw";
std::pair<int, int> myIndexScore;
AudioTimeSyncController* myTimeController;
std::map<StringW, std::pair<std::pair<int, int>, MultiplayerConnectedPlayerSongTimeSyncController*>> playerInfos;
float modifierMultiplier;
std::vector<std::pair<int, float>> scoreValues;

template<class T>
requires (std::is_convertible_v<T, BeatmapDataItem*>)
ArrayW<T> GetBeatmapDataItems(IReadonlyBeatmapData* data){
    auto* beatmapDataItems = System::Collections::Generic::List_1<T>::New_ctor(); 
    beatmapDataItems->AddRange(reinterpret_cast<BeatmapData*>(data)->_beatmapDataItemsPerTypeAndId->GetItems<T>(0));
    beatmapDataItems->_items->max_length = beatmapDataItems->_size;
    return beatmapDataItems->_items;
}

void CreateScoreTimeValues(IReadonlyBeatmapData* data){
        ClearVector<std::pair<int, float>>(&scoreValues);
        for (auto& noteData : GetBeatmapDataItems<NoteData*>(data)){
            if (noteData->get_scoringType().value__ != -1 && noteData->get_scoringType().value__ != 0){
                scoreValues.push_back(std::make_pair(noteData->get_scoringType().value__ == 4 ? 85 : 115, noteData->get_time()));
            }
        }
        for (auto& sliderData : GetBeatmapDataItems<SliderData*>(data)){
            if (sliderData->get_sliderType().value__ == 1){
                for (int i = 1; i < sliderData->get_sliceCount(); i++){
                    scoreValues.push_back(std::make_pair(20, LerpU(sliderData->get_time(), sliderData->get_tailTime(), (float)i / (sliderData->get_sliceCount() - 1))));
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
        getLogger().info("Last Index: {}\nMax Score: {}", scoreValues.back().first, maxScore);
    }

void toggleMultiResultsTableFormat(bool value, ResultsTableCell* cell){
    cell->_rankText->set_enableWordWrapping(!value);
    cell->_rankText->set_richText(value);
    cell->_scoreText->set_richText(value);
    cell->_scoreText->set_enableWordWrapping(!value);
    int multiplier = value ? 1 : -1;
    cell->_scoreText->get_transform()->set_localPosition(Vector3::op_Addition(cell->_scoreText->get_transform()->get_localPosition(), Vector3(multiplier * -5, 0, 0)));
    cell->_rankText->get_transform()->set_localPosition(Vector3::op_Addition(cell->_rankText->get_transform()->get_localPosition(), Vector3(multiplier * -2, 0, 0)));
}

MAKE_HOOK_MATCH(Results_SetData, &ResultsTableCell::SetData, void, ResultsTableCell* self, int order, IConnectedPlayer* connectedPlayer, LevelCompletionResults* levelCompletionResults){
    Results_SetData(self, order, connectedPlayer, levelCompletionResults);
    bool passedLevel = levelCompletionResults->levelEndStateType.value__ == 1 ? true : false;
    if (GET_VALUE(multiShowPercentageOnResults)){
        if (!self->_rankText->get_richText()) toggleMultiResultsTableFormat(true, self);
        bool isNoFail = levelCompletionResults->gameplayModifiers->get_noFailOn0Energy() && levelCompletionResults->energy == 0;
        int totalMisses = levelCompletionResults->missedCount + levelCompletionResults->badCutsCount;
        std::string percentageText = Round(CalculatePercentage(mapData.maxScore, levelCompletionResults->modifiedScore), 2);
        std::string score = self->_scoreText->get_text();
        std::string preText = !passedLevel ? "F" + tab : isNoFail ? "NF" + tab : "";
        std::string missText = levelCompletionResults->fullCombo ? "FC" : preText + "<color=red>X</color><size=65%> </size>" + std::to_string(totalMisses);
        self->_rankText->set_text(percentageText + "<size=75%>%</size>");
        self->_scoreText->set_text(missText + tab + score);
        getLogger().info("Index Max Score: %i", myIndexScore.second);
        getLogger().info("Index Score Percentage: %.2f", CalculatePercentage(myIndexScore.second, levelCompletionResults->modifiedScore));
        getLogger().info("True Final Score: %.2f", CalculatePercentage(mapData.maxScore, levelCompletionResults->modifiedScore));
    }
    else{
        if (self->_rankText->get_richText()) toggleMultiResultsTableFormat(false, self);
        if (!passedLevel) self->_rankText->set_text("F");
        else self->_rankText->set_text(RankModel::GetRankName(levelCompletionResults->rank));
    }
    // write new highscore to file
    if (connectedPlayer->get_isMe() && (levelCompletionResults->modifiedScore - mapData.currentScore > 0) && passedLevel && bs_utils::Submission::getEnabled()){
        int misses = levelCompletionResults->missedCount;
        int badCut = levelCompletionResults->badCutsCount;
        std::string currentTime = System::DateTime::get_UtcNow().ToLocalTime().ToString("D");
        ConfigHelper::UpdateBeatMapInfo(mapData.mapID, mapData.idString, misses, badCut, 0, currentTime);
    }
}

MAKE_HOOK_MATCH(ScoreRingManager_UpdateScoreText, &MultiplayerScoreRingManager::UpdateScore, void, MultiplayerScoreRingManager* self, IConnectedPlayer* playerToUpdate){
    if (GET_VALUE(multiLivePercentages)){
        MultiplayerScoreProvider::RankedPlayer* player;
        auto* scoreRingItem = self->GetScoreRingItem(playerToUpdate->get_userId()).ptr();
        bool flag = self->_scoreProvider->TryGetScore(playerToUpdate->get_userId(), player);
        if (!flag || player->get_isFailed()){
            scoreRingItem->SetScore("X"); return;
        }
        if (scoreValues.empty()){
            scoreRingItem->SetScore("0 (0.00%)"); return;
        }
        auto x = playerInfos.find(playerToUpdate->get_userId());
        float currentSongTime;
        std::pair<int, int>* indexScore;
        if (x != playerInfos.end()){
            currentSongTime = x->second.second->get_songTime();
            indexScore = &x->second.first;
        }
        else if (playerToUpdate->get_isMe()){
            currentSongTime = myTimeController->_songTime;
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
        std::string userPercentage = Round(CalculatePercentage(indexScore->second, userScore), 2);
        scoreRingItem->SetScore(std::to_string(userScore) + " (" + userPercentage + "%)");
    }
    else ScoreRingManager_UpdateScoreText(self, playerToUpdate);
}

MAKE_HOOK_MATCH(ScoreDiff_UpdateText, &MultiplayerScoreDiffText::AnimateScoreDiff, void, MultiplayerScoreDiffText* self, int scoreDiff){
    ScoreDiff_UpdateText(self, scoreDiff);
    if (myTimeController != nullptr && GET_VALUE(multiPercentageDifference)){
        if(self->_onPlatformText->get_enableWordWrapping()){
            self->_onPlatformText->set_richText(true);
            self->_onPlatformText->set_enableWordWrapping(false);
            auto* transform = (RectTransform*)(self->_backgroundSpriteRenderer->get_transform().ptr());
            transform->set_localScale({transform->get_localScale().x *2.0f, transform->get_localScale().y, 0.0f});
        }
        std::string baseText = self->_onPlatformText->get_text();
        int maxPossibleScore = myIndexScore.second;
        std::string posneg = (scoreDiff >= 0) ? "+" : "";
        std::string percentageText = " (" + posneg + Round(CalculatePercentage(maxPossibleScore, scoreDiff), 2) + "%)";
        self->_onPlatformText->set_text(baseText + percentageText);
    }
    else if (!GET_VALUE(multiPercentageDifference)){
        if(!self->_onPlatformText->get_enableWordWrapping()){
            self->_onPlatformText->set_richText(false);
            self->_onPlatformText->set_enableWordWrapping(true);
            auto* transform = (RectTransform*)(self->_backgroundSpriteRenderer->get_transform().ptr());
            transform->set_localScale({transform->get_localScale().x /2.0f, transform->get_localScale().y, 0.0f});
        }
    }
}

MAKE_HOOK_MATCH(Local_Start, &MultiplayerLocalActiveClient::Start, void, MultiplayerLocalActiveClient* self){
    Local_Start(self);
    myTimeController = self->_audioTimeSyncController;
    myIndexScore = {0, 0};
}

MAKE_HOOK_MATCH(ConnectedPlayer_Start, &MultiplayerConnectedPlayerSongTimeSyncController::StartSong, void, MultiplayerConnectedPlayerSongTimeSyncController* self, int64_t songStartSyncTime){
    ConnectedPlayer_Start(self, songStartSyncTime);
    playerInfos[self->_connectedPlayer->get_userId()] = {{0, 0}, self};
}

MAKE_HOOK_MATCH(BeatmapData_Init, &BeatmapCallbacksController::_ctor, void, BeatmapCallbacksController* self, BeatmapCallbacksController::InitData* initData)
{
    BeatmapData_Init(self, initData);
    playerInfos.clear();
    CreateScoreTimeValues(initData->beatmapData);
    if (!isMapDataValid) return;
    isMapDataValid = false;
    ScorePercentage::MapUtils::updateMaxScoreFromIReadonlyBeatmapData(initData->beatmapData);
}

void ScorePercentage::MultiplayerHooks::InstallHooks(){
    INSTALL_HOOK(getLogger(), Local_Start);
    INSTALL_HOOK(getLogger(), ConnectedPlayer_Start);
    INSTALL_HOOK(getLogger(), ScoreRingManager_UpdateScoreText);
    INSTALL_HOOK(getLogger(), ScoreDiff_UpdateText);
    INSTALL_HOOK(getLogger(), Results_SetData);
    INSTALL_HOOK(getLogger(), BeatmapData_Init);
}