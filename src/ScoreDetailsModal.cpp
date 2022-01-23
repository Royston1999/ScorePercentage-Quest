#include "ScoreDetailsModal.hpp"

using namespace QuestUI::BeatSaberUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace GlobalNamespace;
using namespace ScorePercentage::Utils;

DEFINE_TYPE(ScorePercentage, ModalPopup);

ScorePercentage::ModalPopup* ScorePercentage::ModalPopup::CreatePopupModal(UnityEngine::Transform* parent){
    int totalUIText = ScoreDetails::config.uiPlayCount + ScoreDetails::config.uiMissCount + ScoreDetails::config.uiBadCutCount + ScoreDetails::config.uiPauseCount + ScoreDetails::config.uiDatePlayed;
    // 55
    int x = 25 + (6 * totalUIText);
    ScorePercentage::ModalPopup* modal = reinterpret_cast<ScorePercentage::ModalPopup*>(CreateModal(parent->get_transform(), UnityEngine::Vector2(60, x), [](HMUI::ModalView *modal) {}, true));
    modal->initModal(totalUIText);
    ScoreDetails::modalSettingsChanged = false;
    modal->Hide(false, nullptr);
    return modal;
}

void ScorePercentage::ModalPopup::initModal(int uiText){
    list = CreateVerticalLayoutGroup(get_transform());
    list->set_spacing(-1.0f);
    list->set_padding(RectOffset::New_ctor(7, 0, 10, 1));
    list->set_childForceExpandWidth(true);
    list->set_childControlWidth(false);
    // 21
    int y = 6 + (3 * uiText);
    title = CreateText(get_transform(), "<size=150%>SCORE DETAILS</size>", UnityEngine::Vector2(14, y));
    score = CreateText(list->get_transform(), "");
    maxCombo = CreateText(list->get_transform(), "");
    if (ScoreDetails::config.uiPlayCount) playCount = CreateText(list->get_transform(), "");
    if (ScoreDetails::config.uiMissCount) missCount = CreateText(list->get_transform(), "");
    if (ScoreDetails::config.uiBadCutCount) badCutCount = CreateText(list->get_transform(), "");
    if (ScoreDetails::config.uiPauseCount) pauseCountGUI = CreateText(list->get_transform(), "");
    if (ScoreDetails::config.uiDatePlayed) datePlayed = CreateText(list->get_transform(), "");
    buttonHolder = CreateGridLayoutGroup(get_transform()->get_parent());
    buttonHolder->set_cellSize({70.0f, 10.0f});
    openButton = CreateUIButton(buttonHolder->get_transform(), "VIEW SCORE DETAILS", "PracticeButton", {0, 0}, {70.0f, 10.0f}, [this](){
        Show(true, true, nullptr);
    });
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
    if (playCount) playCount->SetText(il2cpp_utils::newcsstr(playCountText));
    if (missCount) missCount->SetText(il2cpp_utils::newcsstr(missCountText));
    if (badCutCount) badCutCount->SetText(il2cpp_utils::newcsstr(badCutCountText));
    if (pauseCountGUI) pauseCountGUI->SetText(il2cpp_utils::newcsstr(pauseCountText));
    if (datePlayed) datePlayed->SetText(il2cpp_utils::newcsstr(datePlayedText));
}