#include "ScorePercentageConfig.hpp"
#include "main.hpp"

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
    getConfig().Write();
    ConfigHelper::LoadConfig(scorePercentageConfig, getConfig().config);
    return true;
}

void ConfigHelper::AddBeatMap(MemoryPoolAllocator<>& allocator, Value& obj, std::string mapID, std::string diff, int missCount, int badCutCount, int pauseCount, std::string datePlayed) {
    Value v(kObjectType);

    char buffer[60]; int len = sprintf(buffer, "%s", mapID.c_str());
    Document::ValueType string(kStringType);
    string.SetString(buffer, len, allocator);

    char diffType[20]; int len2 = sprintf(diffType, "%s", diff.c_str());
    Document::ValueType string2(kStringType);
    string2.SetString(diffType, len2, allocator);

    Document::ValueType diffArr(kObjectType);
    diffArr.AddMember("missCount", missCount, allocator);
    diffArr.AddMember("badCutCount", badCutCount, allocator);
    diffArr.AddMember("pauseCount", pauseCount, allocator);
    diffArr.AddMember("datePlayed", datePlayed, allocator);
    v.AddMember(string2, diffArr, allocator);
    obj.AddMember(string, v, allocator);
    WriteFile();
}

void ConfigHelper::UpdateBeatMapInfo(std::string mapID, std::string diff, int missCount, int badCutCount, int pauseCount, std::string datePlayed){
    MemoryPoolAllocator<>& allocator = scorePercentageConfig.beatMapData.GetAllocator();
    Value& obj = scorePercentageConfig.beatMapData;
    auto itr = obj.FindMember(mapID);
    if (itr != obj.MemberEnd()){
        auto itr2 = itr->value.GetObject().FindMember(diff);
        if (itr2 != itr->value.GetObject().MemberEnd()){
            itr2->value.GetObject().FindMember("missCount")->value.SetInt(missCount);
            itr2->value.GetObject().FindMember("badCutCount")->value.SetInt(badCutCount);
            itr2->value.GetObject().FindMember("pauseCount")->value.SetInt(pauseCount);
            char buffer[30]; int len = sprintf(buffer, "%s", datePlayed.c_str());
            itr2->value.GetObject().FindMember("datePlayed")->value.SetString(buffer, len, allocator);
            WriteFile();
        }
        else{
            Document::ValueType difficulty(kObjectType);
            difficulty.AddMember("missCount", missCount, allocator);
            difficulty.AddMember("badCutCount", badCutCount, allocator);
            difficulty.AddMember("pauseCount", pauseCount, allocator);
            difficulty.AddMember("datePlayed", datePlayed, allocator);
            char diffType[20]; int len2 = sprintf(diffType, "%s", diff.c_str());
            Document::ValueType string2(kStringType);
            string2.SetString(diffType, len2, allocator);
            itr->value.AddMember(string2, difficulty, allocator);
            WriteFile();
        }
    }
    else AddBeatMap(allocator, obj, mapID, diff, missCount, badCutCount, pauseCount, datePlayed);
}

void ConfigHelper::LoadBeatMapInfo(std::string mapID, std::string diff){    
    MemoryPoolAllocator<>& allocator = scorePercentageConfig.beatMapData.GetAllocator();
    Value& obj = scorePercentageConfig.beatMapData;
    auto itr = obj.FindMember(mapID);
    if (itr != obj.MemberEnd()){
        auto itr2 = itr->value.GetObject().FindMember(diff);
        if (itr2 != itr->value.GetObject().MemberEnd()){
            scorePercentageConfig.badCutCount = itr2->value.GetObject().FindMember("badCutCount")->value.GetInt();
            scorePercentageConfig.missCount = itr2->value.GetObject().FindMember("missCount")->value.GetInt();
            scorePercentageConfig.pauseCount = itr2->value.GetObject().FindMember("pauseCount")->value.GetInt();
            scorePercentageConfig.datePlayed = itr2->value.GetObject().FindMember("datePlayed")->value.GetString();
            return;
        }
    }
    scorePercentageConfig.badCutCount = -1;
    scorePercentageConfig.missCount = -1;
    scorePercentageConfig.pauseCount = -1;
    scorePercentageConfig.datePlayed = "";
}

bool ConfigHelper::LoadConfig(ScorePercentageConfig& con, ConfigDocument& config) {
    if (!config.HasMember("Menu Highscore Percentage")) ConfigHelper::CreateDefaultConfig(config);
    ConfigHelper::LoadBeatMapDataFile();
    if (!config.HasMember("alwaysOpen")) ConfigHelper::UpdateOldConfig(config);
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
    con.alwaysOpen = getBool(config, "alwaysOpen").value_or(false);
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
    getConfig().Write();
    config.AddMember("uiPP", true, config.GetAllocator());
    config.AddMember("uiPlayCount", true, config.GetAllocator());
    config.AddMember("uiMissCount", true, config.GetAllocator());
    config.AddMember("uiBadCutCount", true, config.GetAllocator());
    config.AddMember("uiPauseCount", true, config.GetAllocator());
    getConfig().Write();
    config.AddMember("uiDatePlayed", true, config.GetAllocator());
    config.AddMember("alwaysOpen", false, config.GetAllocator());
    getConfig().Write();
}

void ConfigHelper::UpdateOldConfig(ConfigDocument& config){
    config.AddMember("alwaysOpen", false, config.GetAllocator());
}

// void ConfigHelper::UpdateOldConfig(ConfigDocument& config){
//     MemoryPoolAllocator<>& allocator = scorePercentageConfig.beatMapData.GetAllocator();
//     auto locateMaps = config.FindMember("beatMaps");
//     auto arr = locateMaps->value.GetArray();
//     auto size = arr.Size();
//     for (int i = 0; i < size; i++) {
//         std::string mapID = arr[i].FindMember("mapID")->value.GetString();
//         auto itr = arr[i].FindMember("difficulties");
//         auto arr2 = itr->value.GetArray();
//         auto size2 = arr2.Size();

//         Value v(kObjectType);
//         char buffer[60]; int len = sprintf(buffer, "%s", mapID.c_str());
//         Document::ValueType string(kStringType);
//         string.SetString(buffer, len, allocator);

//         for (int i2 = 0; i2 < size2; i2++) {
//             std::string diff = arr2[i2].FindMember("diffType")->value.GetString();

//             char diffType[20]; int len2 = sprintf(diffType, "%s", diff.c_str());
//             Document::ValueType string2(kStringType);
//             string2.SetString(diffType, len2, allocator);
//             arr2[i2].EraseMember("diffType");
//             v.AddMember(string2, arr2[i2], allocator);
//         }
//         scorePercentageConfig.beatMapData.AddMember(string, v, allocator);
//     }
//     WriteFile();
//     config.EraseMember("beatMaps");
//     getConfig().Write();
// }

void ConfigHelper::LoadBeatMapDataFile(){
    std::string dir = getDataDir(modInfo);
    std::string file = dir + "beatMapData.json";

    if(!direxists(dir)) {
        int made = mkpath(dir);
        if(made < 0) {
            getLogger().info("Failed to make data directory");
            return;
        }
    }
    if(!fileexists(file)) writefile(file, "{}");
    ConfigDocument d;
    if(!parsejsonfile(d, file)) {
        getLogger().info("Failed to read map data");
        return;
    }
    scorePercentageConfig.beatMapData.Swap(d);
}

void ConfigHelper::WriteFile(){
    std::string dir = getDataDir(modInfo);
    std::string file = dir + "beatMapData.json";
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    scorePercentageConfig.beatMapData.Accept(writer);
    std::string s = buffer.GetString();
    writefile(file, s);
}