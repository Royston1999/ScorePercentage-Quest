#include <tuple>
#include "ScoreDetailsConfig.hpp"
#include "main.hpp"

#define GET(obj, fieldName, method, required) auto itr = obj.FindMember(fieldName.data()); \
if (itr == obj.MemberEnd()) { \
    if (required) { \
    } \
    return std::nullopt; \
} \
return itr->value.method()

using namespace rapidjson;

std::optional<bool> getBool(Value& obj, std::string_view fieldName, bool required) {
    GET(obj, fieldName, GetBool, required);
}

std::optional<bool> setBool(Value& obj, std::string_view fieldName, bool value, bool required) {
    auto itr = obj.FindMember(fieldName.data());
    if (itr == obj.MemberEnd()) {
        if (required) {
        }
    }
    itr->value.SetBool(value);
    return true;
}

void ConfigHelper::AddBeatMap(MemoryPoolAllocator<>& allocator, Value& obj, std::string mapID, std::string diff, int missCount, int badCutCount, int pauseCount, std::string datePlayed) {
    auto itr = obj.FindMember("beatMaps");
    Value v(kObjectType);
    v.AddMember("mapID", mapID, allocator);
    Document::ValueType difficulty(kObjectType);
    Document::ValueType diffArr(kArrayType);
    difficulty.AddMember("diffType", diff, allocator);
    difficulty.AddMember("missCount", missCount, allocator);
    difficulty.AddMember("badCutCount", badCutCount, allocator);
    difficulty.AddMember("pauseCount", pauseCount, allocator);
    difficulty.AddMember("datePlayed", datePlayed, allocator);
    diffArr.PushBack(difficulty, allocator);
    v.AddMember("difficulties", diffArr, allocator);
    itr->value.PushBack(v, allocator);
    getConfig().Write();
}

void ConfigHelper::UpdateBeatMapInfo(std::string mapID, std::string diff, int missCount, int badCutCount, int pauseCount, std::string datePlayed){
    MemoryPoolAllocator<>& allocator = getConfig().config.GetAllocator();
    Value& obj = getConfig().config;
    auto [missCountValue, badCutCountValue, pauseCountValue, datePlayedValue, itr2, mapFound, diffFound] = ConfigHelper::getMapStuff(mapID, diff);
    if (diffFound){
        missCountValue->value.SetInt(missCount);
        badCutCountValue->value.SetInt(badCutCount);
        pauseCountValue->value.SetInt(pauseCount);
        char buffer[30]; int len = sprintf(buffer, "%s", datePlayed.c_str());
        datePlayedValue->value.SetString(buffer, len, allocator);
        getConfig().Write(); 
    }
    else if (!diffFound && mapFound){
        Document::ValueType difficulty(kObjectType);
        difficulty.AddMember("diffType", diff, allocator);
        difficulty.AddMember("missCount", missCount, allocator);
        difficulty.AddMember("badCutCount", badCutCount, allocator);
        difficulty.AddMember("pauseCount", pauseCount, allocator);
        difficulty.AddMember("datePlayed", datePlayed, allocator);
        itr2->value.PushBack(difficulty, allocator);
        getConfig().Write();
    }
    else if (!mapFound) AddBeatMap(allocator, obj, mapID, diff, missCount, badCutCount, pauseCount, datePlayed);
}

void ConfigHelper::LoadBeatMapInfo(std::string mapID, std::string diff){    
    auto [missCountValue, badCutCountValue, pauseCountValue, datePlayedValue, itr2, mapFound, diffFound] = ConfigHelper::getMapStuff(mapID, diff);
    if (diffFound){
        ScoreDetails::config.badCutCount = badCutCountValue->value.GetInt();
        ScoreDetails::config.missCount = missCountValue->value.GetInt();
        ScoreDetails::config.pauseCount = pauseCountValue->value.GetInt();
        ScoreDetails::config.datePlayed = datePlayedValue->value.GetString();
    }
    else{
        ScoreDetails::config.badCutCount = -1;
        ScoreDetails::config.missCount = -1;
        ScoreDetails::config.pauseCount = -1;
        ScoreDetails::config.datePlayed = "";
    }
}

std::tuple<GenericMemberIterator<false, UTF8<>, MemoryPoolAllocator<CrtAllocator>>,
GenericMemberIterator<false, UTF8<>, MemoryPoolAllocator<CrtAllocator>>, 
GenericMemberIterator<false, UTF8<>, MemoryPoolAllocator<CrtAllocator>>, 
GenericMemberIterator<false, UTF8<>, MemoryPoolAllocator<CrtAllocator>>,
GenericMemberIterator<false, UTF8<>, MemoryPoolAllocator<CrtAllocator>>, bool, bool> 
ConfigHelper::getMapStuff(std::string mapID, std::string diff){
    MemoryPoolAllocator<>& allocator = getConfig().config.GetAllocator();
    Value& obj = getConfig().config;
    GenericMemberIterator<false, UTF8<>, MemoryPoolAllocator<CrtAllocator>> missCountValue, badCutCountValue, itr2, pauseCountValue, datePlayedValue;
    auto locateMaps = obj.FindMember("beatMaps");
    auto arr = locateMaps->value.GetArray();
    auto size = arr.Size();
    bool mapFound = false;
    bool diffFound = false;
    for (int i = 0; i < size; i++) {
        auto itr = arr[i].FindMember("mapID");
        std::string value = itr->value.GetString();
        if (value.compare(mapID) == 0){
            mapFound = true;
            itr2 = arr[i].FindMember("difficulties");
            auto arr2 = itr2->value.GetArray();
            auto size2 = arr2.Size();
            for (int i2 = 0; i2 < size2; i2++) {
                auto itr3 = arr2[i2].FindMember("diffType");
                std::string diffValue = itr3->value.GetString();
                if (diffValue.compare(diff) == 0){
                    diffFound = true;
                    missCountValue = arr2[i2].FindMember("missCount");
                    badCutCountValue = arr2[i2].FindMember("badCutCount");
                    pauseCountValue = arr2[i2].FindMember("pauseCount");
                    datePlayedValue = arr2[i2].FindMember("datePlayed");
                    break;
                } 
            }
            break;
        } 
    }
    if (!mapFound) itr2 = obj.FindMember("beatMaps");
    if (!mapFound || !diffFound){
        missCountValue = obj.FindMember("beatMaps"); badCutCountValue = obj.FindMember("beatMaps");
        pauseCountValue = obj.FindMember("beatMaps"); datePlayedValue = obj.FindMember("beatMaps");
    }
    return  std::make_tuple(missCountValue, badCutCountValue, pauseCountValue, datePlayedValue, itr2, mapFound, diffFound);
}

bool ConfigHelper::LoadConfig(ScoreDetailsConfig& con, ConfigDocument& config) {
    if (!config.HasMember("Menu Highscore Percentage")) ConfigHelper::CreateDefaultConfig(config);
    // ConfigHelper::UpdateOldConfig(config);
    con.MenuHighScore = getBool(config, "Menu Highscore Percentage").value_or(false);
    con.LevelEndRank = getBool(config, "Level End Rank Display").value_or(false);
    con.missDifference = getBool(config, "Average Cut Score").value_or(false);
    con.ScoreDifference = getBool(config, "Score Difference").value_or(false);
    con.ScorePercentageDifference = getBool(config, "Score Percentage Difference").value_or(false);
    con.uiPP = getBool(config, "uiPP").value_or(false);
    con.uiPlayCount = getBool(config, "uiPlayCount").value_or(false);
    con.uiMissCount = getBool(config, "uiMissCount").value_or(false);
    con.uiBadCutCount = getBool(config, "uiBadCutCount").value_or(false);
    con.uiPauseCount = getBool(config, "uiPauseCount").value_or(false);
    con.uiDatePlayed = getBool(config, "uiDatePlayed").value_or(false);
    con.missCount = -1;
    con.badCutCount = -1;
    con.pauseCount = -1;
    con.datePlayed = "";
    return true;
}

void ConfigHelper::CreateDefaultConfig(ConfigDocument& config){
    config.SetObject();
    config.RemoveAllMembers();
    config.AddMember("Menu Highscore Percentage", true, config.GetAllocator());
    config.AddMember("Level End Rank Display", true, config.GetAllocator());
    config.AddMember("Average Cut Score", false, config.GetAllocator());
    config.AddMember("Score Difference", true, config.GetAllocator());
    config.AddMember("Score Percentage Difference", true, config.GetAllocator());
    config.AddMember("uiPP", true, config.GetAllocator());
    config.AddMember("uiPlayCount", true, config.GetAllocator());
    config.AddMember("uiMissCount", true, config.GetAllocator());
    config.AddMember("uiBadCutCount", true, config.GetAllocator());
    config.AddMember("uiPauseCount", true, config.GetAllocator());
    config.AddMember("uiDatePlayed", true, config.GetAllocator());
    Document::ValueType beatMapArr(kArrayType);
    config.AddMember("beatMaps", beatMapArr, config.GetAllocator());
    getConfig().Write();
}

void ConfigHelper::UpdateOldConfig(ConfigDocument& config){
    Value& obj = getConfig().config;
    auto locateMaps = obj.FindMember("beatMaps");
    auto arr = locateMaps->value.GetArray();
    auto size = arr.Size();
    for (int i = 0; i < size; i++) {
        auto itr = arr[i].FindMember("difficulties");
        auto arr2 = itr->value.GetArray();
        auto size2 = arr2.Size();
        for (int i2 = 0; i2 < size2; i2++) {
            auto diffType = arr2[i2].FindMember("diffType");
            char buffer[5]; int len = sprintf(buffer, "%s", std::to_string(diffType->value.GetInt()).c_str());
            diffType->value.SetString(buffer, len, config.GetAllocator());
        }
    }
    getConfig().Write();
}