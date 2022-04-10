#include "UI/MultiplayerSettingsViewController.hpp"
#include "UI/SettingsFlowCoordinator.hpp"
#include "ScorePercentageConfig.hpp"
#include "Utils/UIUtils.hpp"

DEFINE_TYPE(ScoreDetailsUI::Views, MultiplayerSettingsViewController);

void ScoreDetailsUI::Views::MultiplayerSettingsViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    using namespace GlobalNamespace;
    using namespace UnityEngine;
    using namespace QuestUI::BeatSaberUI;
    using namespace UnityEngine::UI;

    if (firstActivation) {
        VerticalLayoutGroup* container = CreateVerticalLayoutGroup(get_rectTransform());
        UIUtils::AddHeader(get_transform(), "Multiplayer Settings", UnityEngine::Color(0.188f, 0.620f, 1.0f, 1.0f));
        AddHoverHint(CreateToggle(container->get_transform(), "Display Percentage on Results", scorePercentageConfig.multiLevelEndRank, 
        [](bool value) {
            setBool(getConfig().config, "multiLevelEndRank", value, false);
        } )->get_gameObject(), "displays player ranks as a percentage on the multiplayer results screen");
        
        AddHoverHint(CreateToggle(container->get_transform(), "Display Percentages in Level", scorePercentageConfig.multiLivePercentages, 
        [](bool value) {
            setBool(getConfig().config, "multiLivePercentages", value, false);
        } )->get_gameObject(), "displays the live percentages of players next to their score");

        AddHoverHint(CreateToggle(container->get_transform(), "Display Percentage Difference in Level", scorePercentageConfig.multiPercentageDifference, 
        [](bool value) {
            setBool(getConfig().config, "multiPercentageDifference", value, false);
        } )->get_gameObject(), "displays how far ahead/behind the player is as a percentage as well as the raw score");
    }
}