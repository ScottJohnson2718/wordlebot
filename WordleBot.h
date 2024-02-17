
#pragma once

#include "Strategy.h"

#include <vector>
#include <string>
#include <iostream>
#include <set>

// The Wordle Bot. It solves Wordle puzzles.
struct Bot
{
    // These are the initial loaded dictionaries
    const std::vector<std::string>& guessingWords_;
    const std::vector<std::string>& solutionWords_;
    Strategy& strategy_;
    bool verbose_;

    Bot(const std::vector<std::string>& guessingWords, const std::vector<std::string>& solutionWords,
        Strategy &strategy, bool verbose = false);

    // Returns the number of guesses it took to solve the puzzle
    int SolvePuzzle( std::string const &hiddenSolution, const std::string &openingGuess);

    // Given the solution, this find out how many guesses it would take this bot (with its strategy) would
    // take to solve the puzzle.
    // The solution words have yet to be pruned for the opening guess
    // Returns the number of guesses it took to solve the puzzle
    int SolvePuzzle(Board& board, std::string const& hiddenSolution, const std::string& openingGuess);

    // No opening guess is given. Bot is on its own. remainingSolutions must be already pruned to match the state
    // of the board.
    int SolvePuzzle( Board& board, const std::string &hiddenSolution, const std::vector<std::string>& remainingSolutions);

    struct SearchResult
    {
        size_t totalSize;
        size_t minDepth;
        size_t maxDepth;

        //std::set<std::string> guessedWords;
    };

    // Visit the entire search space and return statistics of the result of iterating the entire space.
    void SearchRecurse(Board& board,
                             const std::vector<std::string>& remainingSolutions,
                             SearchResult &result);

    void Search(Board& board,
                       const std::vector<std::string>& remainingSolutions,
                       SearchResult &result);

};

