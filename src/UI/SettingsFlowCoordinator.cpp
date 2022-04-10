#include "UI/SettingsFlowCoordinator.hpp"
#include "UI/ScoreDetailsUIViewController.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/ViewController_AnimationType.hpp"

DEFINE_TYPE(ScoreDetailsUI, SettingsFlowCoordinator);

void ScoreDetailsUI::SettingsFlowCoordinator::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    if (firstActivation) {
        SetTitle("SCORE PERCENTAGE", HMUI::ViewController::AnimationType::Out);

        showBackButton = true;

        settingsViewController = QuestUI::BeatSaberUI::CreateViewController<ScoreDetailsUI::Views::SettingsViewController*>();
        scoreDetailsUIViewController = QuestUI::BeatSaberUI::CreateViewController<ScoreDetailsUI::Views::ScoreDetailsUIViewController*>();
        multiSettingsViewController = QuestUI::BeatSaberUI::CreateViewController<ScoreDetailsUI::Views::MultiplayerSettingsViewController*>();

        ProvideInitialViewControllers(settingsViewController, scoreDetailsUIViewController, multiSettingsViewController, nullptr, nullptr);
    }
}

void ScoreDetailsUI::SettingsFlowCoordinator::BackButtonWasPressed(HMUI::ViewController* topViewController){
    parentFlowCoordinator->DismissFlowCoordinator(this, HMUI::ViewController::AnimationDirection::Horizontal, nullptr, false);
}