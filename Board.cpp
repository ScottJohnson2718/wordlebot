

#include "Board.h"
#include "ScoredWord.h"

Board::Board(int lettersPerWord)
        : n(lettersPerWord)
{
}

void Board::PushScoredGuess(std::string const &guessStr, const ScoredWord& score)
{
    guesses.push_back(guessStr);
    scores.push_back(score);
}

// This takes a guess such as "slate" and a score string such as "sL..E",
// and turns the score string into an actual score the bot can use.
// Then it adds the guess and the score to the board.
void Board::PushScoredGuess(std::string const &guessStr, const std::string& scoreStr)
{
    ScoredWord sw(scoreStr);
    guesses.push_back(guessStr);
    scores.push_back(sw);
}

void Board::Pop()
{
    guesses.pop_back();
    scores.pop_back();
}

// Go through all the user guesses and how they were scored and
// create a query that can be used to see which words meet the conditions.
std::vector<std::string> Board::PruneSearchSpace(const std::vector<std::string>& solutionWords) const
{
    std::vector<std::string> survivors = solutionWords;
    for (size_t i = 0; i < guesses.size(); ++i)
    {
        survivors = ScoreGroup(guesses[i], scores[i], survivors);
    }
    return survivors;
}


