#pragma once

#include <string>
namespace ScorePercentage::Utils{
    int calculateMaxScore(int blockCount);
    double CalculatePercentage(int maxScore, int resultScore);
    std::string Round(float val, int precision);
    std::string valueDifferenceString(float valueDifference);
    std::string createScoreText(std::string text);
    std::string createMissText(std::string missText, int missDifference);
    std::string createModalScoreText(std::string highScoreText, double currentPercentage, float ppValue, bool isPP);
    std::string createComboText(int combo, bool isFC);
    std::string createTextFromBeatmapData(int value, std::string header);
    std::string createDatePlayedText(std::string datePlayed);
    std::string createFixedNumberText(float score, bool isDifferenceText);
    std::string correctText(std::string originalText);
    static const std::string positiveColour = "<color=#00B300>";
    static const std::string negativeColour = "<color=#FF0000>";
    static const std::string noMissColour = "<color=#05BCFF>";
    static const std::string ppColour = "<color=#5968BB>";
    static const std::string scoreColour = "<color=#EBCD00>";
    static const std::string colourEnd = "</color>";
    static const std::string sizeEnd = "</size>";
    static const std::string tab = "    ";
}
