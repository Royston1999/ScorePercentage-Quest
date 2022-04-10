#include "UI/SettingsViewController.hpp"
#include "UI/SettingsFlowCoordinator.hpp"

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

        AddHoverHint(CreateToggle(container->get_transform(), "Display Rank as Percentage", scorePercentageConfig.LevelEndRank, 
        [](bool value) {
            setBool(getConfig().config, "Level End Rank Display", value, false);
        } )->get_gameObject(), "displays your rank as a percentage on the results screen");
        
        AddHoverHint(CreateToggle(container->get_transform(), "Display Percentage Difference", scorePercentageConfig.ScorePercentageDifference, 
        [](bool value) {
            setBool(getConfig().config, "Score Percentage Difference", value, false);
        } )->get_gameObject(), "displays the score difference as a percentage compared to your previous high score");

        AddHoverHint(CreateToggle(container->get_transform(), "Display Score Difference", scorePercentageConfig.ScoreDifference, 
        [](bool value) {
            setBool(getConfig().config, "Score Difference", value, false);
        } )->get_gameObject(), "displays the score difference compared to your previous high score");

        AddHoverHint(CreateToggle(container->get_transform(), "Display Miss Difference", scorePercentageConfig.missDifference, 
        [](bool value) {
            setBool(getConfig().config, "Average Cut Score", value, false);
        } )->get_gameObject(), "displays the miss difference copared to your previous high score");
    }
}