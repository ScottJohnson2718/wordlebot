
#include "Strategy.h"

#include <iostream>
#include <map>
#include <unordered_map>


//////////////////

ScoreGroupingStrategy::ScoreGroupingStrategy(
    const std::vector<std::string>& guessingWords,
    size_t maxGuessesReturned)
    : guessingWords_(guessingWords)
    , maxGuessesReturned_(maxGuessesReturned)
{
}

ScoredGuess ScoreGroupingStrategy::BestGuess(Board& board,
    const std::vector<std::string>& solutionWords) const
{
    if (solutionWords.size() == 1)
    {
        return ScoredGuess(solutionWords[0], 1);
    }

    ScoredGuess bestGuess;
    size_t mostGroups = 0;
    size_t largestGroup = 0;
    if (solutionWords.size() > 6)
    {
        for (auto const& guessWord : guessingWords_)
        {
            size_t groupCount = ScoreGroupCount(guessWord, solutionWords, largestGroup);

            if (groupCount > mostGroups)
            {
                mostGroups = groupCount;
                bestGuess.first = guessWord;
                bestGuess.second = ((float)mostGroups) + (float)1.0f / (float)largestGroup;
            }
        }
    }
    else
    {
        for (auto const& guessWord : solutionWords)
        {
            size_t groupCount = ScoreGroupCount(guessWord, solutionWords, largestGroup);

            if (groupCount > mostGroups)
            {
                mostGroups = groupCount;
                bestGuess.first = guessWord;
                bestGuess.second = ((float)mostGroups) + (float)1.0f / (float)largestGroup;
            }
        }
    }

    return bestGuess;
}

std::vector<ScoredGuess> ScoreGroupingStrategy::BestGuesses(Board& board,
    const std::vector<std::string>& solutionWords) const
{
    std::vector<ScoredGuess> scoredGuesses;
    if (solutionWords.size() == 1)
    {
        scoredGuesses.push_back(ScoredGuess(solutionWords[0], 1));
        return scoredGuesses;
    }

    size_t largestGroup = 0;
    for (auto const& guessWord : guessingWords_)
    {
        size_t groupCount = ScoreGroupCount(guessWord, solutionWords, largestGroup);

        float s = (float) groupCount + 1.0f / (float) largestGroup;
        if (std::binary_search(solutionWords.begin(), solutionWords.end(), guessWord))
        {
            // Solution words get a bonus. This must happen before they are sorted and the top ones returned
            s += 1.0;
        }
        scoredGuesses.push_back(ScoredGuess(guessWord, s));
    }

    std::vector< ScoredGuess > topGuessesByScore;
    //if (solutionWords.size() <= mostGroups)
    //{
    //    // The guessing words didn't partition into groups any better than the solutions
    //    // themselves.
    //    for (int i = 0; i < std::min(maxGuessesReturned_, solutionWords.size()); ++i)
    //    {
    //        topGuessesByScore.push_back(ScoredGuess(solutionWords[i], 1.0));
    //    }
    //}
    //else
    {
        // Remove duplicate guesses by string
        RemoveDuplicateGuesses(scoredGuesses);

        // Sort them big to small
        std::sort(scoredGuesses.begin(), scoredGuesses.end(),
            [](const ScoredGuess& a, const ScoredGuess& b)
            {
                return a.second > b.second;
            }
        );

        for (int i = 0; i < std::min(maxGuessesReturned_, scoredGuesses.size()); ++i)
        {
            topGuessesByScore.push_back(scoredGuesses[i]);
        }
    }
    return topGuessesByScore;
}
