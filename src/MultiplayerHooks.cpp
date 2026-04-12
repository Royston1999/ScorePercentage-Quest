#include "MultiplayerHooks.hpp"

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

// void toggleMultiResultsTableFormat(bool value, ResultsTableCell* cell){
//     cell->_rankText->set_enableWordWrapping(!value);
//     cell->_rankText->set_richText(value);
//     cell->_scoreText->set_richText(value);
//     cell->_scoreText->set_enableWordWrapping(!value);
//     int multiplier = value ? 1 : -1;
//     cell->_scoreText->get_transform()->set_localPosition(Vector3::op_Addition(cell->_scoreText->get_transform()->get_localPosition(), Vector3(multiplier * -5, 0, 0)));
//     cell->_rankText->get_transform()->set_localPosition(Vector3::op_Addition(cell->_rankText->get_transform()->get_localPosition(), Vector3(multiplier * -2, 0, 0)));
// }

// MAKE_HOOK_MATCH(Results_SetData, &ResultsTableCell::SetData, void, ResultsTableCell* self, int order, IConnectedPlayer* connectedPlayer, LevelCompletionResults* levelCompletionResults){
//     Results_SetData(self, order, connectedPlayer, levelCompletionResults);
//     bool passedLevel = levelCompletionResults->levelEndStateType.value__ == 1 ? true : false;
//     if (GET_VALUE(multiShowPercentageOnResults)){
//         if (!self->_rankText->get_richText()) toggleMultiResultsTableFormat(true, self);
//         bool isNoFail = levelCompletionResults->gameplayModifiers->get_noFailOn0Energy() && levelCompletionResults->energy == 0;
//         int totalMisses = levelCompletionResults->missedCount + levelCompletionResults->badCutsCount;
//         std::string percentageText = Round(CalculatePercentage(mapData.maxScore, levelCompletionResults->modifiedScore), 2);
//         std::string score = self->_scoreText->get_text();
//         std::string preText = !passedLevel ? "F" + tab : isNoFail ? "NF" + tab : "";
//         std::string missText = levelCompletionResults->fullCombo ? "FC" : preText + "<color=red>X</color><size=65%> </size>" + std::to_string(totalMisses);
//         self->_rankText->set_text(percentageText + "<size=75%>%</size>");
//         self->_scoreText->set_text(missText + tab + score);
//         getLogger().info("Index Max Score: {}", myIndexScore.second);
//         getLogger().info("Index Score Percentage: {:.2f}", CalculatePercentage(myIndexScore.second, levelCompletionResults->modifiedScore));
//         getLogger().info("True Final Score: {:.2f}", CalculatePercentage(mapData.maxScore, levelCompletionResults->modifiedScore));
//     }
//     else{
//         if (self->_rankText->get_richText()) toggleMultiResultsTableFormat(false, self);
//         if (!passedLevel) self->_rankText->set_text("F");
//         else self->_rankText->set_text(RankModel::GetRankName(levelCompletionResults->rank));
//     }
// }

void ScorePercentage::MultiplayerHooks::InstallHooks(){
    // INSTALL_HOOK(getLogger(), Results_SetData);
}