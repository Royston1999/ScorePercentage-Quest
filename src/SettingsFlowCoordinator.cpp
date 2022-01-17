#include "SettingsFlowCoordinator.hpp"
#include "ScoreDetailsUIViewController.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/ViewController_AnimationType.hpp"

DEFINE_TYPE(ScoreDetailsUI, SettingsFlowCoordinator);

void ScoreDetailsUI::SettingsFlowCoordinator::DidActivate(
    bool firstActivation,
    bool addedToHierarchy,
    bool screenSystemEnabling
) {
    using namespace HMUI;
    
    if (firstActivation) {
        SetTitle(il2cpp_utils::newcsstr(ID), ViewController::AnimationType::Out);

        showBackButton = true;

        settingsViewController = QuestUI::BeatSaberUI::CreateViewController<ScoreDetailsUI::Views::SettingsViewController*>();
        settingsViewController->flowCoordinator = this;

        currentViewController = nullptr;

        ProvideInitialViewControllers(settingsViewController, nullptr, nullptr, nullptr, nullptr);
    }
}

void ScoreDetailsUI::SettingsFlowCoordinator::BackButtonWasPressed(
    HMUI::ViewController* topViewController
) {
    using namespace GlobalNamespace;
    using namespace HMUI;
    using namespace UnityEngine;

    if (currentViewController) {
        SetTitle(il2cpp_utils::newcsstr(ID), ViewController::AnimationType::In);
        ReplaceTopViewController(settingsViewController, this, this, nullptr, ViewController::AnimationType::Out, ViewController::AnimationDirection::Horizontal);

        currentViewController = nullptr;
    } else {

        parentFlowCoordinator->DismissFlowCoordinator(this, ViewController::AnimationDirection::Horizontal, nullptr, false);
    }
}