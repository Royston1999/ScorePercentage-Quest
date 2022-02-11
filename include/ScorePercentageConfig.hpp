#pragma once

#include <string>
#include <unordered_map>
#include "modloader/shared/modloader.hpp"
// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/byref.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/Color.hpp"
#include "custom-types/shared/macros.hpp"
#include "custom-types/shared/types.hpp"
#include "custom-types/shared/register.hpp"
#include <string>
#include <iostream>

#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "config-utils/shared/config-utils.hpp"
#include "UnityEngine/Color.hpp"

#define GET(obj, fieldName, method, required) auto itr = obj.FindMember(fieldName.data()); \
if (itr == obj.MemberEnd()) { \
    if (required) { \
    } \
    return std::nullopt; \
} \
return itr->value.method()


std::optional<bool> getBool(rapidjson::Value& obj, std::string_view fieldName, bool required = false);
std::optional<bool> setBool(rapidjson::Value& obj, std::string_view fieldName,  bool value, bool required = false);

class ScorePercentageConfig {
public:
    ConfigDocument beatMapData;

    bool MenuHighScore;
    bool LevelEndRank;
    bool missDifference;
    bool ScoreDifference;
    bool ScorePercentageDifference;
    bool uiPP;
    bool uiPlayCount;
    bool uiMissCount;
    bool uiBadCutCount;
    bool uiPauseCount;
    bool uiDatePlayed;

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