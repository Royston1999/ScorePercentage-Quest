#include "ScoreDetailsModal.hpp"
#include "UnityEngine/Resources.hpp"
#include "HMUI/ImageView.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/UI/ColorBlock.hpp"
#include "UnityEngine/Time.hpp"
#include "System/Action.hpp"
#include "custom-types/shared/delegate.hpp"
#include "main.hpp"
#include "Utils/MapUtils.hpp"
#include "Utils/EasyDelegate.hpp"

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
    }
    int x = 25 + (6 * uiText);
    if (modalUI == nullptr) modalUI = (ScorePercentage::ModalPopup*) malloc(sizeof(ScorePercentage::ModalPopup));
    modalUI->modal = CreateModal(parent, UnityEngine::Vector2(60, x), [](HMUI::ModalView *modal) {}, !scorePercentageConfig.alwaysOpen);
    modalSettingsChanged = false;
    
    modalUI->list = CreateVerticalLayoutGroup(modalUI->modal->get_transform());
    modalUI->list->set_spacing(-1.0f);
    modalUI->list->set_padding(RectOffset::New_ctor(7, 0, 10, 1));
    modalUI->list->set_childForceExpandWidth(true);
    modalUI->list->set_childControlWidth(false);
    // 21
    int y = 6 + (3 * uiText);
    modalUI->title = CreateText(modalUI->modal->get_transform(), "<size=150%>SCORE DETAILS</size>", UnityEngine::Vector2(14, y));
    modalUI->score = CreateText(modalUI->list->get_transform(), "", false);
    modalUI->maxCombo = CreateText(modalUI->list->get_transform(), "", false);
    if (scorePercentageConfig.uiPlayCount) modalUI->playCount = CreateText(modalUI->list->get_transform(), "", false);
    if (scorePercentageConfig.uiMissCount) modalUI->missCount = CreateText(modalUI->list->get_transform(), "", false);
    if (scorePercentageConfig.uiBadCutCount) modalUI->badCutCount = CreateText(modalUI->list->get_transform(), "", false);
    if (scorePercentageConfig.uiPauseCount) modalUI->pauseCountGUI = CreateText(modalUI->list->get_transform(), "", false);
    if (scorePercentageConfig.uiDatePlayed) modalUI->datePlayed = CreateText(modalUI->list->get_transform(), "", false);

    modalUI->loadingCircle = Object::Instantiate(
        Resources::FindObjectsOfTypeAll<HMUI::ImageView*>().FirstOrDefault([](auto x){ 
            return x->get_gameObject()->get_name() == "LoadingIndicator"; 
        })->get_gameObject(), modalUI->modal->get_transform(), false);
    modalUI->loadingCircle->AddComponent<LayoutElement*>();
    modalUI->list->get_gameObject()->SetActive(false);
    
    modalUI->openButton = CreateUIButton(parent, "", "PracticeButton", {-47.0f, 0.0f}, {10.0f, 11.0f}, [modalUI](){
        modalUI->updateInfo("Loading...");
        modalUI->modal->Show(true, true, EasyDelegate::MakeDelegate<System::Action*>([modalUI](){
            MapUtils::updateMapData(modalUI->playerData, modalUI->currentMap, true);
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
    imageView->set_material(QuestUI::ArrayUtil::First(Resources::FindObjectsOfTypeAll<Material*>(), [](Material* x) { return x->get_name() == "UINoGlow"; }));
    imageView->set_sprite(QuestUI::BeatSaberUI::Base64ToSprite(Sprites::scoreDetailsButton));
    imageView->set_preserveAspect(true);
    imageView->get_transform()->set_localScale({1.7f, 1.7f, 1.7f});
    auto BG = QuestUI::ArrayUtil::First(modalUI->openButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>(), [](HMUI::ImageView* x) { return x->get_name() == "BG"; });
    BG->skew = 0.0f;

    modalUI->closeButton = CreateUIButton(modalUI->modal->get_transform(), "X", "PracticeButton", UnityEngine::Vector2(25, y + 2.3f), {8.0f, 8.0f}, [modalUI](){
        modalUI->modal->Hide(true, nullptr);
    });
    Object::Destroy(modalUI->closeButton->GetComponentInChildren<LayoutElement*>());
    Object::Destroy(modalUI->closeButton->get_transform()->Find("Underline")->get_gameObject());
    Object::Destroy(modalUI->closeButton->get_transform()->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->get_gameObject());
    modalUI->closeButton->set_name("ScoreDetailsCloseButton");
    auto closeBG = QuestUI::ArrayUtil::First(modalUI->closeButton->get_transform()->GetComponentsInChildren<HMUI::ImageView*>(), [](HMUI::ImageView* x) { return x->get_name() == "BG"; });
    closeBG->skew = 0.0f;
    auto* transform = QuestUI::BeatSaberUI::CreateCanvas()->get_transform();
    auto* closeText = CreateText(transform, "X", false, {0.0f, 1.87f}, {10.0f, 10.0f});
    transform->set_localScale({1.2f, 1.2f, 0.0f});
    closeText->set_alignment(TMPro::TextAlignmentOptions::Bottom);
    transform->SetParent(modalUI->closeButton->get_transform(), false);
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
    bool isValidPP = truePP != -1 && scorePercentageConfig.uiPP;

    std::string highScoreText = ScoreFormatter::Format(mapData.currentScore);

    std::string scoreText = createModalScoreText(highScoreText, currentDifficultyPercentageScore, truePP, isValidPP);
    std::string maxComboText = createComboText(mapData.maxCombo, mapData.isFC);
    std::string playCountText = "Play Count - " + std::to_string(mapData.playCount);
    std::string missCountText = createTextFromBeatmapData(scorePercentageConfig.missCount, "Miss Count - ");
    std::string badCutCountText = createTextFromBeatmapData(scorePercentageConfig.badCutCount, "Bad Cut Count - ");
    std::string pauseCountText = createTextFromBeatmapData(scorePercentageConfig.pauseCount, "Pause Count - ");
    std::string datePlayedText = createDatePlayedText(scorePercentageConfig.datePlayed);
    
    score->SetText(scoreText);
    maxCombo->SetText(maxComboText);
    if (scorePercentageConfig.uiPlayCount) playCount->SetText(playCountText);
    if (scorePercentageConfig.uiMissCount) missCount->SetText(missCountText);
    if (scorePercentageConfig.uiBadCutCount) badCutCount->SetText(badCutCountText);
    if (scorePercentageConfig.uiPauseCount) pauseCountGUI->SetText(pauseCountText);
    if (scorePercentageConfig.uiDatePlayed) datePlayed->SetText(datePlayedText);
}
void ScorePercentage::ModalPopup::setDisplayTexts(std::string text){
    score->SetText(text);
    maxCombo->SetText(text);
    if (scorePercentageConfig.uiPlayCount) playCount->SetText(text);
    if (scorePercentageConfig.uiMissCount) missCount->SetText(text);
    if (scorePercentageConfig.uiBadCutCount) badCutCount->SetText(text);
    if (scorePercentageConfig.uiPauseCount) pauseCountGUI->SetText(text);
    if (scorePercentageConfig.uiDatePlayed) datePlayed->SetText(text);
}