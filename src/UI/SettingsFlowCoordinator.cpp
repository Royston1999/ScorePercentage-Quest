#include "UI/SettingsFlowCoordinator.hpp"
#include "UI/ScoreDetailsUIViewController.hpp"
#include "bsml/shared/Helpers/creation.hpp"

DEFINE_TYPE(ScoreDetailsUI, SettingsFlowCoordinator);

void ScoreDetailsUI::SettingsFlowCoordinator::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    if (!firstActivation) return;
    SetTitle("SCORE PERCENTAGE", HMUI::ViewController::AnimationType::Out);

    _showBackButton = true;

    settingsViewController = BSML::Helpers::CreateViewController<ScoreDetailsUI::Views::SettingsViewController*>();
    scoreDetailsUIViewController = BSML::Helpers::CreateViewController<ScoreDetailsUI::Views::ScoreDetailsUIViewController*>();
    multiSettingsViewController = BSML::Helpers::CreateViewController<ScoreDetailsUI::Views::MultiplayerSettingsViewController*>();

    ProvideInitialViewControllers(settingsViewController, scoreDetailsUIViewController, multiSettingsViewController, nullptr, nullptr);
    
}

void ScoreDetailsUI::SettingsFlowCoordinator::BackButtonWasPressed(HMUI::ViewController* topViewController){
    _parentFlowCoordinator->DismissFlowCoordinator(this, HMUI::ViewController::AnimationDirection::Horizontal, nullptr, false);
}