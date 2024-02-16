
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
    bool verbose_;

    Bot(const std::vector<std::string>& guessingWords, const std::vector<std::string>& solutionWords,
        Strategy &strategy, bool verbose = false);

    // Returns the number of guesses it took to solve the puzzle
    int SolvePuzzle( std::string const &hiddenSolution, const std::string &openingGuess);

    // The solution words have yet to be pruned for the opening guess
    // Returns the number of guesses it took to solve the puzzle
    int SolvePuzzle(Board& board, std::string const& hiddenSolution, const std::string& openingGuess);

    // No opening guess is given. Bot is on its own.
    //int SolvePuzzle( std::string &hiddenSolution, const std::vector<std::string>& remaining);
};

