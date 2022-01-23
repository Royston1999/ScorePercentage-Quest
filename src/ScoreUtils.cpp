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
}