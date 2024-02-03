
#pragma once

#include "Strategy.h"

#include <vector>
#include <string>
#include <iostream>

// The Wordle Bot. It solves Wordle puzzles.
struct Bot
{
    const std::vector<std::string>& guessingWords_;
    const std::vector<std::string>& solutionWords_;
    Strategy& strategy_;

    Bot(const std::vector<std::string>& guessingWords, const std::vector<std::string>& solutionWords,
        Strategy &strategy);

    int SolvePuzzle( std::string const &hiddenSolution, const std::string &openingGuess,bool verbose = false);

    int SolvePuzzle(Board& board, std::string const& hiddenSolution, const std::string& openingGuess,
                    bool verbose = false);
};

