

#include "Board.h"



Board::Board(int lettersPerWord)
        : n(lettersPerWord)
{
}

void Board::PushScoredGuess(std::string const &guessStr, ScoredWord& score)
{
    guesses.push_back(guessStr);
    scores.push_back(score);
}

// This takes a guess such as "slate" and a score string such as "sL..E",
// and turns the score string into an actual score the bot can use.
// Then it adds the guess and the score to the board.
void Board::PushScoredGuess(std::string const &guessStr, const std::string& scoreStr)
{
    ScoredWord score;
    for (size_t i = 0; i < guessStr.size(); ++i)
    {
        char ch = scoreStr[i];
        if (ch == '.')
        {
            score.Set(i, NotPresent);
        }
        else
        {
            if (ch >= 'A' && ch <= 'Z')
            {
                score.Set(i, CorrectNotHere);
            }
            else if (ch >= 'a' && ch <= 'z')
            {
                score.Set(i,  Correct);
            }
        }
    }
    guesses.push_back(guessStr);
    scores.push_back(score);
}

void Board::Pop()
{
    guesses.pop_back();
    scores.pop_back();
}

// Go through all the user guesses and how they were scored and
// create a query that can be used to see which words meet the conditions.
WordQuery Board::GenerateQuery() const
{
    WordQuery query(n);
    for (size_t guessIndex = 0; guessIndex < guesses.size(); ++guessIndex)
    {
        std::string guess = guesses[guessIndex];
        auto score = scores[guessIndex];
        // Avoid a quirk in wordle where it scores guesses with a contradiction.
        // For instance, "moony" was scored "..oN." which says there is no 'o'
        // but there is an 'o' in the third position

        for (int charIndex = 0; charIndex < n; ++charIndex)
        {
            CharScore scoreChar = score[charIndex];
            if (scoreChar == Correct)
                query.SetCorrect( charIndex, guess[charIndex]);
        }
        uint32_t correctMask = ComputeMask(query.correct);
        for (int charIndex = 0; charIndex < n; ++charIndex)
        {
            CharScore scoreChar = score[charIndex];
            uint32_t charMask = (1 << (guess[charIndex] - 'a'));
            if (scoreChar == NotPresent && ((charMask & correctMask) == 0))
            {
                // Set that the char is not present in the word but only
                // if it is not already a char in the correct list
                query.SetCantContain(guess[charIndex]);
            }
            else if (scoreChar != Correct)
            {
                query.SetMustContain(guess[charIndex]);
                query.SetCantContain(charIndex, guess[charIndex]);
            }
        }
    }
    return query;
}


