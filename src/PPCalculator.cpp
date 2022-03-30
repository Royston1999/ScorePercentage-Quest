#include "PPCalculator.hpp"

const int PP_CURVE_SIZE = 32;
float ppCurve[PP_CURVE_SIZE][2] = {
    {0, 0},
    {.6f, .25f},
    {.65f, .29f},
    {.7f, .34f},
    {.75f, .40f},
    {.8f, .47f},
    {.825f, .51f},
    {.85f, .57f},
    {.875f, .655f},
    {.9f, .75f},
    {.91f, .79f},
    {.92f, .835f},
    {.93f, 0.885f},
    {.94f, 0.94f},
    {.95f, 1.0f},
    {.955f, 1.045f},
    {.96f, 1.11f},
    {.965f, 1.20f},
    {.97f, 1.31f},
    {.9725f, 1.37f},
    {.975f, 1.45f},
    {.9775f, 1.57f},
    {.98f, 1.71f},
    {.9825f, 1.88f},
    {.985f, 2.1f},
    {.9875f, 2.38f},
    {.99f, 2.73f},
    {.9925f, 3.17f},
    {.995f, 3.76f},
    {.9975f, 4.7f},
    {.999f, 5.8f},
    {1.0f, 7.0f}
};
float ppCurveSlopes[31];
static std::unordered_set<std::string> songsAllowingPositiveModifiers = {
    "2FDDB136BDA7F9E29B4CB6621D6D8E0F8A43B126", // Overkill Nuketime
    "27FCBAB3FB731B16EABA14A5D039EEFFD7BD44C9" // Overkill Kry
};
const std::string PP_DATA_URI = "https://raw.githubusercontent.com/Royston1999/ScorePercentage-Quest/main/raw_pp.json";

void PPCalculator::PP::Initialize() {
    request = UnityEngine::Networking::UnityWebRequest::Get(il2cpp_utils::newcsstr(PP_DATA_URI));
    request->SetRequestHeader(il2cpp_utils::newcsstr("User-Agent"), il2cpp_utils::newcsstr(std::string(ID) + " " + VERSION));
    request->SendWebRequest()->add_completed(il2cpp_utils::MakeDelegate<DownloadCompleteDelegate>(
        classof(DownloadCompleteDelegate), (void*)nullptr, PPCalculator::PP::HandleWebRequestCompleted
    ));

    // precalculate curve slopes
    for (auto i = 0; i < PP_CURVE_SIZE - 1; i++) {
        auto x1 = ppCurve[i][0];
        auto y1 = ppCurve[i][1];
        auto x2 = ppCurve[i+1][0];
        auto y2 = ppCurve[i+1][1];

        auto m = (y2 - y1) / (x2 - x1);
        ppCurveSlopes[i] = m;
    }
}

void PPCalculator::PP::HandleWebRequestCompleted() {
    auto response = to_utf8(csstrtostr(request->get_downloadHandler()->GetText()));
    rapidjson::Document document;
    document.Parse(response.c_str());

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
    if (accuracy >= 1.0) return 7.0f;
    if (accuracy <= 0.0f) return 0.0f;

    int i = 0;
    for (; i < PP_CURVE_SIZE; i++)
        if (i == PP_CURVE_SIZE - 1 || ppCurve[i + 1][0] > accuracy) break;

    auto accuracyFloor = ppCurve[i][0];
    auto ppFloor = ppCurve[i][1];
    return ppCurveSlopes[i] * (accuracy - accuracyFloor) + ppFloor;
}

std::string SongIDToHash(std::string songID) {
    if (!songID.starts_with("custom_level_")) return "";
    for (auto& c: songID) c = toupper(c);
    return songID.substr(13);
}

float PPCalculator::PP::CalculatePP(float maxPP, float accuracy) {
    return maxPP * RatioOfMaxPP(accuracy);
}

bool PPCalculator::PP::SongAllowsPositiveModifiers(std::string songID) {
    return songsAllowingPositiveModifiers.contains(SongIDToHash(songID));
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