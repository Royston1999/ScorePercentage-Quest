#pragma once

#include <string>
namespace ScorePercentage::Utils{
    int calculateMaxScore(int blockCount);
    double CalculatePercentage(int maxScore, int resultScore);
    std::string Round(float val, int precision);
    std::string valueDifferenceString(float valueDifference);
    std::string createScoreText(std::string text);
    inline const std::string positiveColour = "<color=#00B300>";
    inline const std::string negativeColour = "<color=#FF0000>";
    inline const std::string noMissColour = "<color=#05BCFF>";
    inline const std::string ppColour = "<color=#5968BB>";
    inline const std::string scoreColour = "<color=#EBCD00>";
    inline const std::string colourEnd = "</color>";
    inline const std::string sizeEnd = "</size>";
    inline const std::string tab = "    ";
}
