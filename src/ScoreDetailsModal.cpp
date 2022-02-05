#include "ScoreDetailsModal.hpp"

using namespace QuestUI::BeatSaberUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace GlobalNamespace;
using namespace ScorePercentage::Utils;

ScorePercentage::ModalPopup* ScorePercentage::CreatePopupModal(UnityEngine::Transform* parent, int totalUIText){
    // 55
    int x = 25 + (6 * totalUIText);
    ScorePercentage::ModalPopup* modalUI = (ScorePercentage::ModalPopup*) malloc(sizeof(ScorePercentage::ModalPopup));
    modalUI->modal = CreateModal(parent, UnityEngine::Vector2(60, x), [](HMUI::ModalView *modal) {}, true);
    ScoreDetails::modalSettingsChanged = false;
    return modalUI;
}

void ScorePercentage::initModalPopup(ScorePercentage::ModalPopup** modalUIPointer, Transform* parent){
    auto modalUI = *modalUIPointer;
    int uiText = ScoreDetails::config.uiPlayCount + ScoreDetails::config.uiMissCount + ScoreDetails::config.uiBadCutCount + ScoreDetails::config.uiPauseCount + ScoreDetails::config.uiDatePlayed;
    if (modalUI != nullptr){
        UnityEngine::GameObject::Destroy(modalUI->buttonHolder->get_gameObject());
        UnityEngine::GameObject::Destroy(modalUI->modal->get_gameObject());
        delete modalUI;
    }
    modalUI = CreatePopupModal(parent, uiText);
    
    modalUI->list = CreateVerticalLayoutGroup(modalUI->modal->get_transform());
    modalUI->list->set_spacing(-1.0f);
    modalUI->list->set_padding(RectOffset::New_ctor(7, 0, 10, 1));
    modalUI->list->set_childForceExpandWidth(true);
    modalUI->list->set_childControlWidth(false);
    // 21
    int y = 6 + (3 * uiText);
    modalUI->title = CreateText(modalUI->modal->get_transform(), "<size=150%>SCORE DETAILS</size>", UnityEngine::Vector2(14, y));
    modalUI->score = CreateText(modalUI->list->get_transform(), "");
    modalUI->maxCombo = CreateText(modalUI->list->get_transform(), "");
    if (ScoreDetails::config.uiPlayCount) modalUI->playCount = CreateText(modalUI->list->get_transform(), "");
    if (ScoreDetails::config.uiMissCount) modalUI->missCount = CreateText(modalUI->list->get_transform(), "");
    if (ScoreDetails::config.uiBadCutCount) modalUI->badCutCount = CreateText(modalUI->list->get_transform(), "");
    if (ScoreDetails::config.uiPauseCount) modalUI->pauseCountGUI = CreateText(modalUI->list->get_transform(), "");
    if (ScoreDetails::config.uiDatePlayed) modalUI->datePlayed = CreateText(modalUI->list->get_transform(), "");
    modalUI->buttonHolder = CreateGridLayoutGroup(modalUI->modal->get_transform()->get_parent());
    modalUI->buttonHolder->set_cellSize({70.0f, 10.0f});
    modalUI->openButton = CreateUIButton(modalUI->buttonHolder->get_transform(), "VIEW SCORE DETAILS", "PracticeButton", {0, 0}, {70.0f, 10.0f}, [modalUI](){
        modalUI->modal->Show(true, true, nullptr);
    });
    *modalUIPointer = modalUI;
}

void ScorePercentage::ModalPopup::updateInfo(PlayerLevelStatsData* playerLevelStatsData, IDifficultyBeatmap* difficultyBeatmap){
    int currentDifficultyMaxScore = calculateMaxScore(difficultyBeatmap->get_beatmapData()->cuttableNotesCount);

    float maxPP = to_utf8(csstrtostr(playerLevelStatsData->get_beatmapCharacteristic()->get_serializedName())).compare("Standard") == 0 ? PPCalculator::PP::BeatmapMaxPP(to_utf8(csstrtostr(playerLevelStatsData->get_levelID())), difficultyBeatmap->get_difficulty()) : -1;
    float truePP = PPCalculator::PP::CalculatePP(maxPP, calculatePercentage(currentDifficultyMaxScore, playerLevelStatsData->highScore)/100);
    
    //calculate actual score percentage
    double currentDifficultyPercentageScore = calculatePercentage(currentDifficultyMaxScore, playerLevelStatsData->highScore);
    getLogger().info("misses: %s", std::to_string(ScoreDetails::config.missCount).c_str());
    std::string scoreText = "Score - " + to_utf8(csstrtostr(ScoreFormatter::Format(playerLevelStatsData->highScore))) + " (<color=#EBCD00>" + Round(currentDifficultyPercentageScore, 2) + "%</color>)" + ((maxPP != -1 && ScoreDetails::config.uiPP) ? " - (" + ppColour + Round(truePP, 2) + "<size=60%>pp</size></color>)" : "");
    std::string maxComboText = "Max Combo - " + (playerLevelStatsData->fullCombo ? "Full Combo" : std::to_string(playerLevelStatsData->maxCombo));
    std::string playCountText = "Play Count - " + std::to_string(playerLevelStatsData->playCount);
    std::string missCountText = "Miss Count - " + (ScoreDetails::config.missCount != -1 ? (ScoreDetails::config.missCount == 0 ? colorNoMiss : colorNegative) + std::to_string(ScoreDetails::config.missCount) : "N/A");
    std::string badCutCountText = "Bad Cut Count - " + (ScoreDetails::config.badCutCount != -1 ? (ScoreDetails::config.badCutCount == 0 ? colorNoMiss : colorNegative) + std::to_string(ScoreDetails::config.badCutCount) : "N/A");
    std::string pauseCountText = "Pause Count - " + (ScoreDetails::config.pauseCount != -1 ? (ScoreDetails::config.pauseCount == 0 ? colorNoMiss : colorNegative) + std::to_string(ScoreDetails::config.pauseCount) : "N/A");
    std::string datePlayedText = "Date Played - " + (ScoreDetails::config.datePlayed.compare("") != 0 ? ("<size=85%><line-height=75%>" + ScoreDetails::config.datePlayed) : "N/A");
    score->SetText(il2cpp_utils::newcsstr(scoreText));
    maxCombo->SetText(il2cpp_utils::newcsstr(maxComboText));
    if (ScoreDetails::config.uiPlayCount) playCount->SetText(il2cpp_utils::newcsstr(playCountText));
    if (ScoreDetails::config.uiMissCount) missCount->SetText(il2cpp_utils::newcsstr(missCountText));
    if (ScoreDetails::config.uiBadCutCount) badCutCount->SetText(il2cpp_utils::newcsstr(badCutCountText));
    if (ScoreDetails::config.uiPauseCount) pauseCountGUI->SetText(il2cpp_utils::newcsstr(pauseCountText));
    if (ScoreDetails::config.uiDatePlayed) datePlayed->SetText(il2cpp_utils::newcsstr(datePlayedText));
}