#pragma once

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include <string>
#include <iostream>
#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ScorePercentageConfig,
    CONFIG_VALUE(version, std::string, "version", "3.0.0");
    CONFIG_VALUE(showPercentageOnResults, bool, "Display Rank as Percentage", true);
    CONFIG_VALUE(showPercentageInMenu, bool, "Show Percentage in Main Menu", true);
    CONFIG_VALUE(showMissDifference, bool, "Show Miss Difference", true);
    CONFIG_VALUE(showPercentageDifference, bool, "Show Percentage Difference", true);
    CONFIG_VALUE(showScoreDifference, bool, "Show Score Difference", true);
    CONFIG_VALUE(enablePopup, bool, "Enable Score Details Popup", true);
    CONFIG_VALUE(uiPP, bool, "uiPP", true);
    CONFIG_VALUE(uiPlayCount, bool, "uiPlayCount", true);
    CONFIG_VALUE(uiMissCount, bool, "uiMissCount", true);
    CONFIG_VALUE(uiBadCutCount, bool, "uiBadCutCount", true);
    CONFIG_VALUE(uiPauseCount, bool, "uiPauseCount", true);
    CONFIG_VALUE(uiDatePlayed, bool, "uiDatePlayed", true);
    CONFIG_VALUE(alwaysOpen, bool, "alwaysOpen", false);
    CONFIG_VALUE(multiShowPercentageOnResults, bool, "Display Percentage on Results", true);
    CONFIG_VALUE(multiLivePercentages, bool, "Display Percentages in Level", true);
    CONFIG_VALUE(multiPercentageDifference, bool, "Display Percentage Difference in Level", true);
)

#define GET_VALUE(name) getScorePercentageConfig().name.GetValue()

class ScorePercentageConfig {
public:
    ConfigDocument beatMapData;

    // // solo
    // bool MenuHighScore;
    // bool LevelEndRank;
    // bool missDifference;
    // bool ScoreDifference;
    // bool ScorePercentageDifference;

    // // popup
    // bool uiPP;
    // bool uiPlayCount;
    // bool uiMissCount;
    // bool uiBadCutCount;
    // bool uiPauseCount;
    // bool uiDatePlayed;
    // bool alwaysOpen;

    // // multi
    // bool multiLevelEndRank;
    // bool multiLivePercentages;
    // bool multiPercentageDifference;

    int missCount;
    int badCutCount;
    int pauseCount;
    std::string datePlayed;
};

class ConfigHelper {
public:
    static bool LoadConfig(ScorePercentageConfig& con, ConfigDocument& config);
    static void AddBeatMap(rapidjson::MemoryPoolAllocator<>& allocator, rapidjson::Value& obj, std::string mapID, std::string diff, int missCount, int badCutCount, int pauseCount, std::string datePlayed);
    static void UpdateBeatMapInfo(std::string mapID, std::string diff, int missCount, int badCutCount, int pauseCount, std::string datePlayed);
    static void LoadBeatMapInfo(std::string mapID, std::string diff);
    static void CreateDefaultConfig(ConfigDocument& config);
    static void UpdateOldConfig(ConfigDocument& config);
    static void LoadBeatMapDataFile();
    static void WriteFile();
};