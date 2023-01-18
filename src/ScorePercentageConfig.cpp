#include "ScorePercentageConfig.hpp"
#include "main.hpp"

using namespace rapidjson;

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
    ConfigHelper::LoadBeatMapDataFile();
    if (config.HasMember("version") && std::string(config.FindMember("version")->value.GetString()) <= "2.0.0"){
        config.SetObject();
        config.RemoveAllMembers();
    }
    con.missCount = -1;
    con.badCutCount = -1;
    con.pauseCount = -1;
    con.datePlayed = "";
    return true;
}

void ConfigHelper::CreateDefaultConfig(ConfigDocument& config){
    config.SetObject();
    config.RemoveAllMembers();
    config.AddMember("version", "2.0.0", config.GetAllocator());
    config.AddMember("Menu Highscore Percentage", true, config.GetAllocator());
    config.AddMember("Level End Rank Display", true, config.GetAllocator());
    getConfig().Write();
    config.AddMember("Average Cut Score", true, config.GetAllocator());
    config.AddMember("Score Difference", true, config.GetAllocator());
    config.AddMember("Score Percentage Difference", true, config.GetAllocator());
    getConfig().Write();
    config.AddMember("uiPP", true, config.GetAllocator());
    config.AddMember("uiPlayCount", true, config.GetAllocator());
    config.AddMember("uiMissCount", true, config.GetAllocator());
    config.AddMember("uiBadCutCount", true, config.GetAllocator());
    getConfig().Write();
    config.AddMember("uiPauseCount", true, config.GetAllocator());
    config.AddMember("uiDatePlayed", true, config.GetAllocator());
    config.AddMember("alwaysOpen", false, config.GetAllocator());
    getConfig().Write();
    config.AddMember("multiLevelEndRank", true, config.GetAllocator());
    config.AddMember("multiLivePercentages", true, config.GetAllocator());
    config.AddMember("multiPercentageDifference", false, config.GetAllocator());
    getConfig().Write();
    ConfigHelper::LoadConfig(scorePercentageConfig, config);
}

// void ConfigHelper::UpdateOldConfig(ConfigDocument& config){
//     config.SetObject();
//     config.RemoveAllMembers();
//     config.AddMember("version", "2.0.0", config.GetAllocator());
//     config.AddMember("Menu Highscore Percentage", scorePercentageConfig.MenuHighScore, config.GetAllocator());
//     config.AddMember("Level End Rank Display", scorePercentageConfig.LevelEndRank, config.GetAllocator());
//     getConfig().Write();
//     config.AddMember("Average Cut Score", scorePercentageConfig.missDifference, config.GetAllocator());
//     config.AddMember("Score Difference", scorePercentageConfig.ScoreDifference, config.GetAllocator());
//     config.AddMember("Score Percentage Difference", scorePercentageConfig.ScorePercentageDifference, config.GetAllocator());
//     getConfig().Write();
//     config.AddMember("uiPP", scorePercentageConfig.uiPP, config.GetAllocator());
//     config.AddMember("uiPlayCount", scorePercentageConfig.uiPlayCount, config.GetAllocator());
//     config.AddMember("uiMissCount", scorePercentageConfig.uiMissCount, config.GetAllocator());
//     config.AddMember("uiBadCutCount", scorePercentageConfig.uiBadCutCount, config.GetAllocator());
//     getConfig().Write();
//     config.AddMember("uiPauseCount", scorePercentageConfig.uiPauseCount, config.GetAllocator());
//     config.AddMember("uiDatePlayed", scorePercentageConfig.uiDatePlayed, config.GetAllocator());
//     config.AddMember("alwaysOpen", scorePercentageConfig.alwaysOpen, config.GetAllocator());
//     getConfig().Write();
//     config.AddMember("multiLevelEndRank", true, config.GetAllocator());
//     config.AddMember("multiLivePercentages", true, config.GetAllocator());
//     config.AddMember("multiPercentageDifference", false, config.GetAllocator());
//     getConfig().Write();
//     ConfigHelper::LoadConfig(scorePercentageConfig, config);
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