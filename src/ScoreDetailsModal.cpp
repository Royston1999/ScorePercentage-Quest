#include "ScoreDetailsModal.hpp"

using namespace QuestUI::BeatSaberUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace GlobalNamespace;
using namespace ScorePercentage::Utils;

void ScorePercentage::initModalPopup(ScorePercentage::ModalPopup** modalUIPointer, Transform* parent){
    auto modalUI = *modalUIPointer;
    int uiText = ScoreDetails::config.uiPlayCount + ScoreDetails::config.uiMissCount + ScoreDetails::config.uiBadCutCount + ScoreDetails::config.uiPauseCount + ScoreDetails::config.uiDatePlayed;
    if (modalUI != nullptr){
        UnityEngine::GameObject::Destroy(modalUI->modal->get_gameObject());
        UnityEngine::GameObject::Destroy(modalUI->openButton->get_gameObject());
        delete modalUI;
    }
    int x = 25 + (6 * uiText);
    modalUI = (ScorePercentage::ModalPopup*) malloc(sizeof(ScorePercentage::ModalPopup));
    modalUI->modal = CreateModal(parent, UnityEngine::Vector2(60, x), [](HMUI::ModalView *modal) {}, true);
    ScoreDetails::modalSettingsChanged = false;
    
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
    modalUI->openButton = CreateUIButton(parent, "VIEW SCORE DETAILS", "PracticeButton", {6.0f, -36.5f}, {77.0f, 10.0f}, [modalUI](){
        if (!modalUI->modal->isShown) modalUI->modal->Show(true, true, nullptr);
        else modalUI->modal->Hide(true, nullptr);
    });
    *modalUIPointer = modalUI;
}

void ScorePercentage::ModalPopup::updateInfo(){
    double currentDifficultyPercentageScore = mapData.currentPercentage;

    bool isStandardLevel = mapData.mapType.compare("Standard") == 0;
    float maxPP = isStandardLevel ? PPCalculator::PP::BeatmapMaxPP(mapData.mapID, mapData.diff) : -1;
    float truePP = maxPP != -1 ? PPCalculator::PP::CalculatePP(maxPP, currentDifficultyPercentageScore/100) : -1.0f;
    bool isValidPP = truePP != -1 && ScoreDetails::config.uiPP;

    std::string highScoreText = to_utf8(csstrtostr(ScoreFormatter::Format(mapData.currentScore)));

    std::string scoreText = createModalScoreText(highScoreText, currentDifficultyPercentageScore, truePP, isValidPP);
    std::string maxComboText = createComboText(mapData.maxCombo, mapData.isFC);
    std::string playCountText = "Play Count - " + std::to_string(mapData.playCount);
    std::string missCountText = createTextFromBeatmapData(ScoreDetails::config.missCount, "Miss Count - ");
    std::string badCutCountText = createTextFromBeatmapData(ScoreDetails::config.badCutCount, "Bad Cut Count - ");
    std::string pauseCountText = createTextFromBeatmapData(ScoreDetails::config.pauseCount, "Pause Count - ");
    std::string datePlayedText = createDatePlayedText(ScoreDetails::config.datePlayed);
    
    score->SetText(il2cpp_utils::newcsstr(scoreText));
    maxCombo->SetText(il2cpp_utils::newcsstr(maxComboText));
    if (ScoreDetails::config.uiPlayCount) playCount->SetText(il2cpp_utils::newcsstr(playCountText));
    if (ScoreDetails::config.uiMissCount) missCount->SetText(il2cpp_utils::newcsstr(missCountText));
    if (ScoreDetails::config.uiBadCutCount) badCutCount->SetText(il2cpp_utils::newcsstr(badCutCountText));
    if (ScoreDetails::config.uiPauseCount) pauseCountGUI->SetText(il2cpp_utils::newcsstr(pauseCountText));
    if (ScoreDetails::config.uiDatePlayed) datePlayed->SetText(il2cpp_utils::newcsstr(datePlayedText));
}