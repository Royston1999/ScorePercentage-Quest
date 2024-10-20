#include "UI/SettingsViewController.hpp"
#include "bsml/shared/BSML-Lite/Creation/Misc.hpp"
#include "bsml/shared/BSML-Lite/Creation/Layout.hpp"
#include "ScorePercentageConfig.hpp"

DEFINE_TYPE(ScoreDetailsUI::Views, SettingsViewController);

void ScoreDetailsUI::Views::SettingsViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    using namespace BSML::Lite;
    if (firstActivation) {
        auto container = CreateVerticalLayoutGroup(get_rectTransform());

        auto showPerc = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().showPercentageOnResults);
        AddHoverHint(showPerc->get_gameObject(), "displays your rank as a percentage on the results screen");
        auto percDiff = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().showPercentageDifference);
        AddHoverHint(percDiff->get_gameObject(), "displays the score difference as a percentage compared to your previous high score");
        auto scoreDiff = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().showScoreDifference);
        AddHoverHint(scoreDiff->get_gameObject(), "displays the score difference compared to your previous high score");
        auto missDiff = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().showMissDifference);
        AddHoverHint(missDiff->get_gameObject(), "displays the miss difference compared to your previous high score");
        auto percMenu = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().showPercentageInMenu);
        AddHoverHint(percMenu->get_gameObject(), "displays your rank as a percentage in the stats view below the leaderboard");
    }
}