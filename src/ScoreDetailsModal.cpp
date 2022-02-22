#include "ScoreDetailsModal.hpp"
#include "UnityEngine/Resources.hpp"
#include "HMUI/ImageView.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Rect.hpp"
using namespace QuestUI::BeatSaberUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace GlobalNamespace;
using namespace ScorePercentage::Utils;

void ScorePercentage::initModalPopup(ScorePercentage::ModalPopup** modalUIPointer, Transform* parent){
    auto modalUI = *modalUIPointer;
    int uiText = scorePercentageConfig.uiPlayCount + scorePercentageConfig.uiMissCount + scorePercentageConfig.uiBadCutCount + scorePercentageConfig.uiPauseCount + scorePercentageConfig.uiDatePlayed;
    if (modalUI != nullptr){
        UnityEngine::GameObject::Destroy(modalUI->modal->get_gameObject());
        UnityEngine::GameObject::Destroy(modalUI->openButton->get_gameObject());
        delete modalUI;
    }
    int x = 25 + (6 * uiText);
    modalUI = (ScorePercentage::ModalPopup*) malloc(sizeof(ScorePercentage::ModalPopup));
    modalUI->modal = CreateModal(parent, UnityEngine::Vector2(60, x), [](HMUI::ModalView *modal) {}, true);
    modalSettingsChanged = false;
    
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
    if (scorePercentageConfig.uiPlayCount) modalUI->playCount = CreateText(modalUI->list->get_transform(), "");
    if (scorePercentageConfig.uiMissCount) modalUI->missCount = CreateText(modalUI->list->get_transform(), "");
    if (scorePercentageConfig.uiBadCutCount) modalUI->badCutCount = CreateText(modalUI->list->get_transform(), "");
    if (scorePercentageConfig.uiPauseCount) modalUI->pauseCountGUI = CreateText(modalUI->list->get_transform(), "");
    if (scorePercentageConfig.uiDatePlayed) modalUI->datePlayed = CreateText(modalUI->list->get_transform(), "");

    modalUI->onScoreDetails = [modalUI](){
        if (!modalUI->modal->isShown) modalUI->modal->Show(true, true, nullptr);
        else modalUI->modal->Hide(true, nullptr);
    };
    modalUI->openButton = CreateUIButton(parent, "", "PracticeButton", {-47.0f, 10.0f}, {10.0f, 11.0f}, modalUI->onScoreDetails);
    auto contentTransform = modalUI->openButton->get_transform()->Find(il2cpp_utils::newcsstr("Content"));
    Object::Destroy(contentTransform->Find(il2cpp_utils::newcsstr("Text"))->get_gameObject());
    Object::Destroy(contentTransform->GetComponent<LayoutElement*>());
    Object::Destroy(modalUI->openButton->get_transform()->Find(il2cpp_utils::newcsstr("Underline"))->get_gameObject());
    modalUI->openButton->set_name(il2cpp_utils::newcsstr("ScoreDetailsButton"));
    auto iconGameObject = GameObject::New_ctor(il2cpp_utils::newcsstr("Icon"));
    auto imageView = iconGameObject->AddComponent<HMUI::ImageView*>();
    auto iconTransform = imageView->get_rectTransform();
    iconTransform->SetParent(contentTransform, false);
    imageView->set_material(QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<Material*>(), [](Material* x) { return to_utf8(csstrtostr(x->get_name())) == "UINoGlow"; }));
    imageView->set_sprite(QuestUI::BeatSaberUI::Base64ToSprite(Sprites::scoreDetailsButton));
    imageView->set_preserveAspect(true);
    imageView->get_transform()->set_localScale({1.7f, 1.7f, 1.7f});
    auto BG = QuestUI::ArrayUtil::First(modalUI->openButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>(), [](HMUI::ImageView* x) { return to_utf8(csstrtostr(x->get_name())) == "BG"; });
    BG->skew = 0.0f;
    *modalUIPointer = modalUI;
}

void ScorePercentage::ModalPopup::updateInfo(){
    double currentDifficultyPercentageScore = mapData.currentPercentage;

    bool isStandardLevel = mapData.mapType.compare("Standard") == 0;
    float maxPP = isStandardLevel ? PPCalculator::PP::BeatmapMaxPP(mapData.mapID, mapData.diff) : -1;
    float truePP = maxPP != -1 ? PPCalculator::PP::CalculatePP(maxPP, currentDifficultyPercentageScore/100) : -1.0f;
    bool isValidPP = truePP != -1 && scorePercentageConfig.uiPP;

    std::string highScoreText = to_utf8(csstrtostr(ScoreFormatter::Format(mapData.currentScore)));

    std::string scoreText = createModalScoreText(highScoreText, currentDifficultyPercentageScore, truePP, isValidPP);
    std::string maxComboText = createComboText(mapData.maxCombo, mapData.isFC);
    std::string playCountText = "Play Count - " + std::to_string(mapData.playCount);
    std::string missCountText = createTextFromBeatmapData(scorePercentageConfig.missCount, "Miss Count - ");
    std::string badCutCountText = createTextFromBeatmapData(scorePercentageConfig.badCutCount, "Bad Cut Count - ");
    std::string pauseCountText = createTextFromBeatmapData(scorePercentageConfig.pauseCount, "Pause Count - ");
    std::string datePlayedText = createDatePlayedText(scorePercentageConfig.datePlayed);
    
    score->SetText(il2cpp_utils::newcsstr(scoreText));
    maxCombo->SetText(il2cpp_utils::newcsstr(maxComboText));
    if (scorePercentageConfig.uiPlayCount) playCount->SetText(il2cpp_utils::newcsstr(playCountText));
    if (scorePercentageConfig.uiMissCount) missCount->SetText(il2cpp_utils::newcsstr(missCountText));
    if (scorePercentageConfig.uiBadCutCount) badCutCount->SetText(il2cpp_utils::newcsstr(badCutCountText));
    if (scorePercentageConfig.uiPauseCount) pauseCountGUI->SetText(il2cpp_utils::newcsstr(pauseCountText));
    if (scorePercentageConfig.uiDatePlayed) datePlayed->SetText(il2cpp_utils::newcsstr(datePlayedText));
}