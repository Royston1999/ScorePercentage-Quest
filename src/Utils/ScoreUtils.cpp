#include "Utils/ScoreUtils.hpp"
#include <iomanip>
#include <sstream>
namespace ScorePercentage::Utils{
    std::string Round (float val, int precision)
    {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(precision) << val;
        std::string Out = stream.str();
        return Out;
    }

    std::string valueDifferenceString(float valueDifference){
        std::string differenceColor, positiveIndicator;
        //Better or same Score
        if (valueDifference >= 0)
        {
            differenceColor = positiveColour;
            positiveIndicator = "+";
        }
        //Worse Score
        else
        {
            differenceColor = negativeColour;
            positiveIndicator = "";
        }
        return differenceColor + positiveIndicator + (ceilf(valueDifference) != valueDifference ? Round(valueDifference, 2) : Round(valueDifference, 0));
    }

    double CalculatePercentage(int maxScore, int resultScore)
    {
        if (maxScore == 0) return 0.0f;
        double resultPercentage = (double)(100 / (double)maxScore * (double)resultScore);
        return resultPercentage;
    }

    std::string createScoreText(std::string text){
        std::string firstLineFormatting = "<line-height=10%><size=65%>";
        std::string secondLineFormatting = "\n<alpha=#00>F";
        return firstLineFormatting + text + secondLineFormatting;
    }
}