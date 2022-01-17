#include "SettingsViewController.hpp"
#include "SettingsFlowCoordinator.hpp"
#include "ScoreDetailsUIViewController.hpp"

#include "GlobalNamespace/OVRPlugin.hpp"
#include "GlobalNamespace/OVRPlugin_SystemHeadset.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/ViewController_AnimationType.hpp"

DEFINE_TYPE(ScoreDetailsUI::Views, SettingsViewController);

void ScoreDetailsUI::Views::SettingsViewController::DidActivate(
    bool firstActivation,
    bool addedToHierarchy,
    bool screenSystemEnabling
) {
    using namespace GlobalNamespace;
    using namespace UnityEngine;
    using namespace QuestUI::BeatSaberUI;
    using namespace UnityEngine::UI;

    if (firstActivation) {
        VerticalLayoutGroup* container = CreateVerticalLayoutGroup(get_rectTransform());

        AddHoverHint(CreateToggle(container->get_transform(), "Display Rank as Percentage", ScoreDetails::config.LevelEndRank, 
        [](bool value) {
            setBool(getConfig().config, "Level End Rank Display", value, false); getConfig().Write();
            ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
        } )->get_gameObject(), "displays your rank as a percentage on the results screen");
        
        AddHoverHint(CreateToggle(container->get_transform(), "Display Percentage Difference", ScoreDetails::config.ScorePercentageDifference, 
        [](bool value) {
            setBool(getConfig().config, "Score Percentage Difference", value, false); getConfig().Write();
            ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
        } )->get_gameObject(), "displays the score difference as a percentage compared to your previous high score");

        AddHoverHint(CreateToggle(container->get_transform(), "Display Score Difference", ScoreDetails::config.ScoreDifference, 
        [](bool value) {
            setBool(getConfig().config, "Score Difference", value, false); getConfig().Write();
            ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
        } )->get_gameObject(), "displays the score difference compared to your previous high score");

        AddHoverHint(CreateToggle(container->get_transform(), "Display Miss Difference", ScoreDetails::config.missDifference, 
        [](bool value) {
            setBool(getConfig().config, "Average Cut Score", value, false); getConfig().Write();
            ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
        } )->get_gameObject(), "displays the miss difference copared to your previous high score");

        graphicsButton = CreateUIViewControllerButton(container->get_transform(), "Advanced Score Details Settings", QuestUI::BeatSaberUI::CreateViewController<ScoreDetailsUI::Views::ScoreDetailsUIViewController*>());
    }
}

UnityEngine::UI::Button* ScoreDetailsUI::Views::SettingsViewController::CreateUIViewControllerButton(
    UnityEngine::Transform* parent,
    std::string title,
    HMUI::ViewController* viewController
) {
    using namespace UnityEngine;

    return QuestUI::BeatSaberUI::CreateUIButton(parent, title, Vector2(), Vector2(5.0f, 1.15f),
        [this, title, viewController]() {
            flowCoordinator->SetTitle(il2cpp_utils::newcsstr(title), ViewController::AnimationType::In);
            flowCoordinator->ReplaceTopViewController(viewController, flowCoordinator, flowCoordinator, nullptr, ViewController::AnimationType::In, ViewController::AnimationDirection::Horizontal);

            reinterpret_cast<ScoreDetailsUI::SettingsFlowCoordinator*>(flowCoordinator)->currentViewController = viewController;
        }
    );
}