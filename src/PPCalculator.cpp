#include "PPCalculator.hpp"
#include "Utils/HttpUtils.hpp"
#include "Utils/TaskCoroutine.hpp"
#include "main.hpp"
#include <unordered_map>

struct PPPoint {
    float percentage;
    float multiplier;
};

int PP_CURVE_SIZE = 0;
std::vector<PPPoint> ppCurve;
std::vector<float> ppCurveSlopes;

const std::string CURVE_DATA_URL = "https://raw.githubusercontent.com/Royston1999/ScorePercentage-Quest/main/curve.json";
const std::unordered_map<std::string, std::string> REQUEST_HEADERS = {{"User-Agent", MOD_ID " " VERSION}};

DECLARE_JSON_STRUCT(PPCurveData) {
    NAMED_VALUE(std::vector<std::vector<float>>, curve, "curve");
    NAMED_VALUE(std::string, ppDataUrl, "ppData");
};

task_coroutine<void> PPCalculator::PP::Initialize() {
    getLogger().info("Initialising Scoresaber PP Info");
    ppCurve.clear(); ppCurveSlopes.clear();
    
    HttpResponse<PPCurveData> curveRes = co_await HttpService::GetAsync<PPCurveData>(CURVE_DATA_URL, REQUEST_HEADERS);
    if (!curveRes.success) co_return;

    for (auto &point : curveRes.content.curve | std::views::reverse) {
        ppCurve.emplace_back(point[0], point[1]);
    }

    PP_CURVE_SIZE = ppCurve.size();
    for (auto i = 0; i < PP_CURVE_SIZE - 1; i++) {
        auto x1 = ppCurve[i].percentage;
        auto y1 = ppCurve[i].multiplier;
        auto x2 = ppCurve[i+1].percentage;
        auto y2 = ppCurve[i+1].multiplier;

        auto m = (y2 - y1) / (x2 - x1);
        ppCurveSlopes.push_back(m);
    }
    auto ppRes = co_await HttpService::GetAsync<StringKeyedMap<RawPPData>>(curveRes.content.ppDataUrl, REQUEST_HEADERS);
    if (!ppRes.success) co_return;
    index = std::move(ppRes.content);
    co_return;
}

float RatioOfMaxPP(float accuracy) {
    if (accuracy >= ppCurve.back().percentage) return ppCurve.back().multiplier;
    if (accuracy <= ppCurve[0].percentage) return ppCurve[0].multiplier;
    if (PP_CURVE_SIZE == 0) return -1.0f;
    int i = 0;
    for (; i < PP_CURVE_SIZE; i++)
        if (i == PP_CURVE_SIZE - 1 || ppCurve[i + 1].percentage > accuracy) break;

    auto accuracyFloor = ppCurve[i].percentage;
    auto ppFloor = ppCurve[i].multiplier;
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