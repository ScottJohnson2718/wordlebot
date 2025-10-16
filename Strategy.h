
#pragma once

#include "Wordle.h"
#include "ScoredWord.h"
#include "Board.h"

#include <vector>

struct Strategy
{

    // Return the best single guess for the given board
    virtual ScoredGuess BestGuess(Board& board,
                                  const std::vector<std::string>& solutionWords) const = 0;

    // Return a list of the best guesses for the given board.
    // This is useful in interactive mode when the guessed word
    // is not in the dictionary of the Wordle app. In that case,
    // there are alternative words to chose from.
    virtual std::vector<ScoredGuess> BestGuesses(Board& board,
                                                 const std::vector<std::string>& solutionWords) const = 0;
};


////////////////

// Pick the best guess by how the scoring of solution list is partitioned into groups by the guesses.
// This is how the New York Times Wordlebot describes their best solution.

struct ScoreGroupingStrategy : public Strategy
{
    ScoreGroupingStrategy(
        const std::vector<std::string>& guessingWords, size_t maxGuessesReturned);

    ScoredGuess BestGuess(Board& board,
        const std::vector<std::string>& solutionWords) const;

    std::vector<ScoredGuess> BestGuesses(Board& board,
        const std::vector<std::string>& solutionWords) const;

    const std::vector<std::string>& guessingWords_;
    size_t maxGuessesReturned_;
};


