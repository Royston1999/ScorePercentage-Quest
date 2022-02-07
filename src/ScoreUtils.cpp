#include "ScoreUtils.hpp"

namespace ScorePercentage::Utils{
    std::string Round (float val, int precision)
    {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(precision) << val;
        std::string Out = stream.str();
        return Out;
    }

    int calculateMaxScore(int blockCount)
    {
        int maxScore;
        if(blockCount < 14)
        {
            if (blockCount == 1)
                maxScore = 115;
            else if (blockCount < 5)
                maxScore = (blockCount - 1) * 230 + 115;
            else
                maxScore = (blockCount - 5) * 460 + 1035;
        }
        else
            maxScore = (blockCount - 13) * 920 + 4715;
        return maxScore;
    }

    std::string valueDifferenceString(float valueDifference){
        std::string differenceColor, positiveIndicator;
        //Better or same Score
        if (valueDifference >= 0)
        {
            differenceColor = colorPositive;
            positiveIndicator = "+";
        }
        //Worse Score
        else
        {
            differenceColor = colorNegative;
            positiveIndicator = "";
        }
        return differenceColor + positiveIndicator + (ceilf(valueDifference) != valueDifference ? Round(valueDifference, 2) : Round(valueDifference, 0));
    }

    double calculatePercentage(int maxScore, int resultScore)
    {
        double resultPercentage = (double)(100 / (double)maxScore * (double)resultScore);
        return resultPercentage;
    }

    std::string createRankText(std::string rankText, double percentageDifference){
        std::string diffString =  valueDifferenceString(percentageDifference);
        std::string firstLineFormatting = "  <line-height=30%><size=60%>";
        std::string secondLineFormatting = "\n<line-height=30%><size=40%>";
        std::string percentSymbol = "<size=30%>%";
        return firstLineFormatting + rankText + secondLineFormatting + diffString + percentSymbol;
    }
    std::string createScoreText(std::string scoreText, int scoreDifference){
        std::string diffString = valueDifferenceString(scoreDifference);
        std::string firstLineFormatting = "<line-height=30%><size=60%> ";
        std::string secondLineFormatting = "\n<size=40%>";
        return firstLineFormatting + scoreText + secondLineFormatting + diffString;
    }
    std::string createMissText(std::string missText, int missDifference){
        std::string diffString = valueDifferenceString(missDifference);
        std::string lineFormatting = " <size=50%>";
        std::string openParenthesis = "(";
        std::string closeParenthesis = ")";
        return missText + lineFormatting + openParenthesis + diffString + colourEnd + closeParenthesis;
    }
    std::string createPPText(float ppValue){
        std::string header = " - (";
        std::string ppText = Round(ppValue, 2);
        std::string sizeFormatting = "<size=60%>";
        std::string pp = "pp";
        std::string closeParenthesis = ")";
        return header + ppColour + ppText + sizeFormatting + pp + sizeEnd + colourEnd + closeParenthesis;
    }
    std::string createModalScoreText(std::string highScoreText, double currentPercentage, float ppValue, bool isPP){
        std::string header = "Score - ";
        std::string percentageText = Round(currentPercentage, 2);
        std::string openParenthesis = " (";
        std::string closeParenthesis = ")";
        std::string percent = "%";
        std::string ppText = isPP ? createPPText(ppValue) : "";
        return header + highScoreText + openParenthesis + scoreColour + percentageText + percent + colourEnd + closeParenthesis + ppText;
    }
    std::string createComboText(int combo, bool isFC){
        std::string header = "Max Combo - ";
        std::string comboText = isFC ? "Full Combo" : std::to_string(combo);
        return header + comboText;
    }
    std::string createTextFromBeatmapData(int value, std::string header){
        std::string invalidValue = "N/A";
        if (value == -1) return header + invalidValue;
        std::string colourValue = value == 0 ? colorNoMiss : colorNegative;
        std::string valueText = std::to_string(value);
        return header + colourValue + valueText;
    }
    std::string createDatePlayedText(std::string datePlayed){
        std::string header = "Date Played - ";
        std::string invalid = "N/A";
        if (datePlayed.compare("") == 0) return header + invalid;
        std::string formatting = "<size=85%><line-height=75%>";
        return header + formatting + datePlayed;
    }
}