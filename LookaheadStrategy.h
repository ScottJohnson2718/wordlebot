
#pragma once

#include <string>
#include <vector>
#include "Board.h"
#include "Strategy.h"

// This strategy picks the best guess or guesses by considering the entire search space of the solution words.
// It can do an enormous amount of searching ahead to determine what the best guess is. This is like a chess bot
// looking moves ahead expect it is simpler.
struct LookaheadStrategy : public Strategy
{
    LookaheadStrategy( Strategy &subStrategy,
            const std::vector<std::string>& guessingWords, size_t maxGuessesReturned);

    ScoredGuess BestGuess(Board& board,
                          const std::vector<std::string>& solutionWords) const;


    std::vector<ScoredGuess> BestGuesses(Board& board,
                                         const std::vector<std::string>& solutionWords) const;



    Strategy& subStrategy_;
    const std::vector<std::string>& guessingWords_;
    size_t maxGuessesReturned_;
};

