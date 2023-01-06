#include "PPCalculator.hpp"
#include "main.hpp"
#include "custom-types/shared/delegate.hpp"

using namespace ScorePercentage;
using namespace UnityEngine;

int PP_CURVE_SIZE = 0;
std::vector<std::pair<float, float>> ppCurve;
std::vector<float> ppCurveSlopes;

const std::string CURVE_DATA_URL = "https://raw.githubusercontent.com/Royston1999/ScorePercentage-Quest/main/curve.json";

void WebUtils::SendWebRequest(std::string URL, callback_ptr callback){
    auto request = UnityEngine::Networking::UnityWebRequest::Get(URL);
    request->SetRequestHeader("User-Agent", std::string(ID) + " " + VERSION);
    request->SendWebRequest()->add_completed(DLCompletedDeleg([=](auto* value){
        callback(request->get_downloadHandler()->GetText());
    }));
}

void PPCalculator::PP::Initialize() {
    // SendWebRequest(CURVE_DATA_URL, PPCalculator::PP::HandleCurveWebRequestCompleted);
    WebUtils::SendWebRequest(CURVE_DATA_URL, [](std::string response){
        rapidjson::Document document;
        document.Parse(response.c_str());
        if (document.Empty()) return;
        auto curveArray = document.FindMember("curve")->value.GetArray();
        PP_CURVE_SIZE = curveArray.Size();
        for (int i=PP_CURVE_SIZE-1; i>=0; i--){
            auto coords = curveArray[i].GetArray();
            ppCurve.push_back(std::make_pair(coords[0].GetFloat(), coords[1].GetFloat()));
        }

        for (auto i = 0; i < PP_CURVE_SIZE - 1; i++) {
            auto x1 = ppCurve[i].first;
            auto y1 = ppCurve[i].second;
            auto x2 = ppCurve[i+1].first;
            auto y2 = ppCurve[i+1].second;

            auto m = (y2 - y1) / (x2 - x1);
            ppCurveSlopes.push_back(m);
        }

        std::string URL = document.FindMember("ppData")->value.GetString();

        WebUtils::SendWebRequest(URL, PPCalculator::PP::HandlePPWebRequestCompleted);

        // WebUtils::SendWebRequest("https://www.example.com", [](std::string response){

        //     // hello this is the code block inside of the lambda that will run once the web request completes.
        //     // the string should be empty if the request fails

        // });

        });
}


void PPCalculator::PP::HandlePPWebRequestCompleted(std::string text) {
    rapidjson::Document document;
    document.Parse(text.c_str());

    for (auto itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
        RawPPData data;
        itr->value.HasMember("_Easy_SoloStandard") ? data._Easy_SoloStandard = itr->value["_Easy_SoloStandard"].GetFloat() : data._Easy_SoloStandard = -1;
        itr->value.HasMember("_Normal_SoloStandard") ? data._Normal_SoloStandard = itr->value["_Normal_SoloStandard"].GetFloat() : data._Normal_SoloStandard = -1;
        itr->value.HasMember("_Hard_SoloStandard") ? data._Hard_SoloStandard = itr->value["_Hard_SoloStandard"].GetFloat() : data._Hard_SoloStandard = -1;
        itr->value.HasMember("_Expert_SoloStandard") ? data._Expert_SoloStandard = itr->value["_Expert_SoloStandard"].GetFloat() : data._Expert_SoloStandard = -1;
        itr->value.HasMember("_ExpertPlus_SoloStandard") ? data._ExpertPlus_SoloStandard = itr->value["_ExpertPlus_SoloStandard"].GetFloat() : data._ExpertPlus_SoloStandard = -1;
        index.insert({std::string(itr->name.GetString()), data});
    }
}

float RatioOfMaxPP(float accuracy) {
    if (accuracy >= ppCurve.back().second) return ppCurve.back().second;
    if (accuracy <= ppCurve[0].second) return ppCurve[0].second;
    if (PP_CURVE_SIZE == 0) return -1.0f;
    int i = 0;
    for (; i < PP_CURVE_SIZE; i++)
        if (i == PP_CURVE_SIZE - 1 || ppCurve[i + 1].first > accuracy) break;

    auto accuracyFloor = ppCurve[i].first;
    auto ppFloor = ppCurve[i].second;
    float ratio = ppCurveSlopes[i] * (accuracy - accuracyFloor) + ppFloor;
    return ratio;
}

std::string SongIDToHash(std::string songID) {
    if (!songID.starts_with("custom_level_")) return "";
    for (auto& c: songID) c = toupper(c);
    return songID.substr(13);
}

float PPCalculator::PP::CalculatePP(float maxPP, float accuracy) {
    return maxPP * RatioOfMaxPP(accuracy);
}

float PPCalculator::PP::BeatmapMaxPP(std::string songID, GlobalNamespace::BeatmapDifficulty difficulty) {
    auto itr = index.find(SongIDToHash(songID));
    if (itr == index.end()) return -1;

    switch ((int)difficulty) {
        case 0:
            return itr->second._Easy_SoloStandard;
        case 1:
            return itr->second._Normal_SoloStandard;
        case 2:
            return itr->second._Hard_SoloStandard;
        case 3:
            return itr->second._Expert_SoloStandard;
        case 4:
            return itr->second._ExpertPlus_SoloStandard;
        default:
            return -1;
    }
}