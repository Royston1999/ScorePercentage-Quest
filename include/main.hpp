#pragma once

#define SCORE_PERCENTAGE_EXPORT __attribute__((visibility("default")))
#ifdef __cplusplus
#define SCORE_PERCENTAGE_EXPORT_FUNC extern "C" SCORE_PERCENTAGE_EXPORT
#else
#define SCORE_PERCENTAGE_EXPORT_FUNC SCORE_PERCENTAGE_EXPORT
#endif

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include "scotland2/shared/loader.hpp"
#include "ScorePercentageConfig.hpp"
#include "ScoreDetailsModal.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "GlobalNamespace/BeatmapKey.hpp"

// Define these functions here so that we can easily read configuration and log information from other files
Configuration& getConfig();
const Paper::ConstLoggerContext<16UL>& getLogger();

struct BeatMapData{
    int currentScore;
    float currentPercentage;
    int maxScore;
    int diff;
    bool isFC;
    int maxCombo;
    int playCount;
    std::string mapType;
    std::string mapID;
    std::string idString;
    GlobalNamespace::BeatmapKey key;
};

extern modloader::ModInfo modInfo;
extern BeatMapData mapData;
extern ScorePercentageConfig scorePercentageConfig;
extern ScorePercentage::ModalPopup* scoreDetailsUI;
extern float modifierMultiplier;
extern bool finishedLoading;