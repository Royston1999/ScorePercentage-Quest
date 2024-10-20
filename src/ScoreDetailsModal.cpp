#include "ScoreDetailsModal.hpp"
#include "UnityEngine/Resources.hpp"
#include "HMUI/ImageView.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Rect.hpp"
#include "System/Action.hpp"
#include "main.hpp"
#include "Utils/MapUtils.hpp"
#include "Utils/EasyDelegate.hpp"
#include "bsml/shared/BSML-Lite/Creation/Layout.hpp"

using namespace BSML::Lite;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace GlobalNamespace;
using namespace ScorePercentage::Utils;

void ScorePercentage::initModalPopup(ScorePercentage::ModalPopup** modalUIPointer, Transform* parent){
    auto modalUI = *modalUIPointer;
    int uiText = GET_VALUE(uiPlayCount) + GET_VALUE(uiMissCount) + GET_VALUE(uiBadCutCount) + GET_VALUE(uiPauseCount) + GET_VALUE(uiDatePlayed);
    if (modalUI != nullptr){
        UnityEngine::GameObject::Destroy(modalUI->modal->get_gameObject());
        UnityEngine::GameObject::Destroy(modalUI->openButton->get_gameObject());
    }
    int x = 25 + (6 * uiText);
    if (modalUI == nullptr) modalUI = (ScorePercentage::ModalPopup*) malloc(sizeof(ScorePercentage::ModalPopup));
    modalUI->modal = BSML::Lite::CreateModal(parent, UnityEngine::Vector2(60, x), []() {}, !GET_VALUE(alwaysOpen));
    modalUI->modalSettingsChanged = false;
    
    modalUI->list = CreateVerticalLayoutGroup(modalUI->modal->get_transform());
    modalUI->list->set_spacing(-1.0f);
    modalUI->list->set_padding(RectOffset::New_ctor(-25, 0, 10, 1));
    modalUI->list->set_childForceExpandWidth(true);
    modalUI->list->set_childControlWidth(false);
    // 21
    int y = 6 + (3 * uiText);
    modalUI->title = CreateText(modalUI->modal->get_transform(), "<size=150%>SCORE DETAILS</size>", UnityEngine::Vector2(0, y));
    modalUI->score = CreateText(modalUI->list->get_transform(), "");
    modalUI->maxCombo = CreateText(modalUI->list->get_transform(), "");
    if (GET_VALUE(uiPlayCount)) modalUI->playCount = CreateText(modalUI->list->get_transform(), "");
    if (GET_VALUE(uiMissCount)) modalUI->missCount = CreateText(modalUI->list->get_transform(), "");
    if (GET_VALUE(uiBadCutCount)) modalUI->badCutCount = CreateText(modalUI->list->get_transform(), "");
    if (GET_VALUE(uiPauseCount)) modalUI->pauseCountGUI = CreateText(modalUI->list->get_transform(), "");
    if (GET_VALUE(uiDatePlayed)) modalUI->datePlayed = CreateText(modalUI->list->get_transform(), "");

    modalUI->title->set_alignment(TMPro::TextAlignmentOptions::Center);
    modalUI->score->set_fontStyle(TMPro::FontStyles::Normal);
    modalUI->maxCombo->set_fontStyle(TMPro::FontStyles::Normal);
    if (GET_VALUE(uiPlayCount)) modalUI->playCount->set_fontStyle(TMPro::FontStyles::Normal);
    if (GET_VALUE(uiMissCount)) modalUI->missCount->set_fontStyle(TMPro::FontStyles::Normal);
    if (GET_VALUE(uiBadCutCount)) modalUI->badCutCount->set_fontStyle(TMPro::FontStyles::Normal);
    if (GET_VALUE(uiPauseCount)) modalUI->pauseCountGUI->set_fontStyle(TMPro::FontStyles::Normal);
    if (GET_VALUE(uiDatePlayed)) modalUI->datePlayed->set_fontStyle(TMPro::FontStyles::Normal);

    modalUI->loadingCircle = Object::Instantiate(
        Resources::FindObjectsOfTypeAll<HMUI::ImageView*>().front_or_default([](auto x){ 
            return x->get_gameObject()->get_name() == "LoadingIndicator"; 
        })->get_gameObject(), modalUI->modal->get_transform(), false);
    modalUI->loadingCircle->AddComponent<LayoutElement*>();
    modalUI->list->get_gameObject()->SetActive(false);
    
    modalUI->openButton = CreateUIButton(parent, "", "PracticeButton", {-8.0f, -4.0f}, {10.0f, 11.0f}, [modalUI](){
        modalUI->updateInfo("Loading...");
        modalUI->modal->Show(true, true, EasyDelegate::MakeDelegate<System::Action*>([modalUI](){
            MapUtils::updateMapData(modalUI->currentMap);
        }));
    });
    auto contentTransform = modalUI->openButton->get_transform()->Find("Content");
    Object::Destroy(contentTransform->Find("Text")->get_gameObject());
    Object::Destroy(contentTransform->GetComponent<LayoutElement*>());
    Object::Destroy(modalUI->openButton->get_transform()->Find("Underline")->get_gameObject());
    modalUI->openButton->set_name("ScoreDetailsButton");
    auto iconGameObject = GameObject::New_ctor("Icon");
    auto imageView = iconGameObject->AddComponent<HMUI::ImageView*>();
    auto iconTransform = imageView->get_rectTransform();
    iconTransform->SetParent(contentTransform, false);
    imageView->set_material(Resources::FindObjectsOfTypeAll<Material*>().front_or_default([](Material* x) { return x->get_name() == "UINoGlow"; }));
    imageView->set_sprite(Base64ToSprite(Sprites::scoreDetailsButton));
    imageView->set_preserveAspect(true);
    imageView->get_transform()->set_localScale({1.7f, 1.7f, 1.7f});
    imageView->set_raycastTarget(false);
    auto BG = modalUI->openButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().front_or_default([](HMUI::ImageView* x) { return x->get_name() == "BG"; });
    BG->_skew = 0.0f;

    modalUI->closeButton = CreateUIButton(modalUI->modal->get_transform(), "X", "PracticeButton", UnityEngine::Vector2(55, -5), {16.0f, 16.0f}, [modalUI](){
        modalUI->modal->Hide(true, nullptr);
    });
    Object::Destroy(modalUI->closeButton->GetComponentInChildren<LayoutElement*>());
    Object::Destroy(modalUI->closeButton->get_transform()->Find("Underline")->get_gameObject());
    modalUI->closeButton->set_name("ScoreDetailsCloseButton");
    auto closeBG = modalUI->closeButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>().front_or_default([](HMUI::ImageView* x) { return x->get_name() == "BG"; });
    closeBG->_skew = 0.0f;
    modalUI->closeButton->get_transform()->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_fontStyle(TMPro::FontStyles::Normal);
    modalUI->modal->set_name("ScoreDetailsModal");
    *modalUIPointer = modalUI;
}

void ScorePercentage::ModalPopup::updateInfo(std::string text){
    if (text != "") {
        list->get_gameObject()->set_active(false);
        loadingCircle->set_active(true);
        return setDisplayTexts(text);
    }
    if (!hasValidScoreData || mapData.maxScore == -1) {
        list->get_gameObject()->set_active(true);
        loadingCircle->set_active(false);
        return setDisplayTexts("failed ;(");
    }
    loadingCircle->set_active(false);
    list->get_gameObject()->set_active(true);
    double currentDifficultyPercentageScore = mapData.currentPercentage;

    bool isStandardLevel = mapData.mapType.compare("Standard") == 0;
    float maxPP = isStandardLevel ? PPCalculator::PP::BeatmapMaxPP(mapData.mapID, mapData.diff) : -1;
    float truePP = maxPP != -1 ? PPCalculator::PP::CalculatePP(maxPP, currentDifficultyPercentageScore/100) : -1.0f;
    bool isValidPP = truePP != -1 && GET_VALUE(uiPP);

    std::string highScoreText = ScoreFormatter::Format(mapData.currentScore);

    std::string scoreText = createModalScoreText(highScoreText, currentDifficultyPercentageScore, truePP, isValidPP);
    std::string maxComboText = createComboText(mapData.maxCombo, mapData.isFC);
    std::string playCountText = "Play Count - " + std::to_string(mapData.playCount);
    std::string missCountText = createTextFromBeatmapData(scorePercentageConfig.missCount, "Miss Count - ");
    std::string badCutCountText = createTextFromBeatmapData(scorePercentageConfig.badCutCount, "Bad Cut Count - ");
    std::string pauseCountText = createTextFromBeatmapData(scorePercentageConfig.pauseCount, "Pause Count - ");
    std::string datePlayedText = createDatePlayedText(scorePercentageConfig.datePlayed);
    
    score->set_text(scoreText);
    maxCombo->set_text(maxComboText);
    if (GET_VALUE(uiPlayCount)) playCount->set_text(playCountText);
    if (GET_VALUE(uiMissCount)) missCount->set_text(missCountText);
    if (GET_VALUE(uiBadCutCount)) badCutCount->set_text(badCutCountText);
    if (GET_VALUE(uiPauseCount)) pauseCountGUI->set_text(pauseCountText);
    if (GET_VALUE(uiDatePlayed)) datePlayed->set_text(datePlayedText);
}
void ScorePercentage::ModalPopup::setDisplayTexts(std::string text){
    score->set_text(text);
    maxCombo->set_text(text);
    if (GET_VALUE(uiPlayCount)) playCount->set_text(text);
    if (GET_VALUE(uiMissCount)) missCount->set_text(text);
    if (GET_VALUE(uiBadCutCount)) badCutCount->set_text(text);
    if (GET_VALUE(uiPauseCount)) pauseCountGUI->set_text(text);
    if (GET_VALUE(uiDatePlayed)) datePlayed->set_text(text);
}