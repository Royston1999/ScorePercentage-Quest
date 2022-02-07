#pragma once
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace ScorePercentage::Utils{
    std::string Round(float val, int precision);
    int calculateMaxScore(int blockCount);
    std::string valueDifferenceString(float valueDifference);
    double calculatePercentage(int maxScore, int resultScore);
    std::string createRankText(std::string rankText, double percentageDifference);
    std::string createScoreText(std::string scoreText, int scoreDifference);
    std::string createMissText(std::string missText, int missDifference);
    std::string createModalScoreText(std::string highScoreText, double currentPercentage, float ppValue, bool isPP);
    std::string createComboText(int combo, bool isFC);
    std::string createTextFromBeatmapData(int value, std::string header);
    std::string createDatePlayedText(std::string datePlayed);
    static std::string colorPositive = "<color=#00B300>";
    static std::string colorNegative = "<color=#FF0000>";
    static std::string colorNoMiss = "<color=#05BCFF>";
    static std::string ppColour = "<color=#5968BB>";
    static std::string scoreColour = "<color=#EBCD00>";
    static std::string colourEnd = "</color>";
    static std::string sizeEnd = "</size>";
}
