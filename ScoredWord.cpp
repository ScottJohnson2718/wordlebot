
#include "ScoredWord.h"

ScoredWord Score(const std::string& solution, const std::string& guess)
{
    ScoredWord score;
    uint32_t solutionMask = ComputeMask(solution);

    for (int i = 0; i < solution.size(); ++i)
    {
        if (guess[i] == solution[i])
        {
            score.Set(i, Correct);
        }
        else
        {
            uint32_t charMask = 1 << (guess[i] - 'a');

            if (charMask & solutionMask)
            {
                score.Set(i, CorrectNotHere);
            }
            else
            {
                score.Set(i, NotPresent);
            }
        }
    }
    return score;
}