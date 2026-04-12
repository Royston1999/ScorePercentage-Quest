#pragma once

#include "beatsaber-hook/shared/config/config-utils.hpp"
#include <string>
#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ScorePercentageConfig) {
    CONFIG_VALUE(version, std::string, "version", "3.0.0");
    CONFIG_VALUE(showPercentageOnResults, bool, "Display Rank as Percentage", true);
    CONFIG_VALUE(showPercentageInMenu, bool, "Show Percentage in Main Menu", true);
    CONFIG_VALUE(showMissDifference, bool, "Show Miss Difference", true);
    CONFIG_VALUE(showPercentageDifference, bool, "Show Percentage Difference", true);
    CONFIG_VALUE(showScoreDifference, bool, "Show Score Difference", true);
    CONFIG_VALUE(multiShowPercentageOnResults, bool, "Display Percentage on Results", true);
    CONFIG_VALUE(multiLivePercentages, bool, "Display Percentages in Level", true);
    CONFIG_VALUE(multiPercentageDifference, bool, "Display Percentage Difference in Level", true);
};

#define GET_VALUE(name) getScorePercentageConfig().name.GetValue()