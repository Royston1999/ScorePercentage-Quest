#include "UI/MultiplayerSettingsViewController.hpp"
#include "ScorePercentageConfig.hpp"
#include "Utils/UIUtils.hpp"

DEFINE_TYPE(ScoreDetailsUI::Views, MultiplayerSettingsViewController);

void ScoreDetailsUI::Views::MultiplayerSettingsViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    using namespace BSML::Lite;

    if (firstActivation) {
        auto container = CreateVerticalLayoutGroup(get_rectTransform());
        UIUtils::AddHeader(get_transform(), "Multiplayer Settings", UnityEngine::Color(0.188f, 0.620f, 1.0f, 1.0f));
        auto multiPerc = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().multiShowPercentageOnResults);
        AddHoverHint(multiPerc->get_gameObject(), "displays player ranks as a percentage on the multiplayer results screen");
        auto multiLivePerc = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().multiLivePercentages);
        AddHoverHint(multiLivePerc->get_gameObject(), "displays the live percentages of players next to their score");
        auto multiDiff = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().multiPercentageDifference);
        AddHoverHint(multiDiff->get_gameObject(), "displays how far ahead/behind the player is as a percentage as well as the raw score");
    }
}