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
    static std::string colorPositive = "<color=#00B300>";
    static std::string colorNegative = "<color=#FF0000>";
    static std::string colorNoMiss = "<color=#05BCFF>";
    static std::string ppColour = "<color=#5968BB>";
}
