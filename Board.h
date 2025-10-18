

#pragma once

#include "Wordle.h"
#include "ScoredWord.h"

#include <vector>
#include <string>

// This is a Wordle board with the guesses and how they were scored.
struct Board
{
    int n{5};  // number of characters in each guess
    Board() = default;
    Board(int lettersPerWord);

    void PushScoredGuess(std::string const &guessStr, const ScoredWord& score);

    // This takes a guess such as "slate" and a score string such as "sL..E",
    // and turns the score string into an actual score the bot can use.
    // Then it adds the guess and the score to the board.
    void PushScoredGuess(std::string const &guessStr, const std::string& scoreStr);

    void Pop();

    std::vector<std::string> PruneSearchSpace(const std::vector<std::string>& solutionWords) const;

    std::vector< std::string> guesses;
    std::vector< ScoredWord > scores;
};




