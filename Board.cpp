

#include "Board.h"

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

        // The multiple letter scoring makes this difficult.
        // You'd think you can go through the scoredWord elements once
        // left to right but the query would contain contradictions.
        // A letter may be marked gray NotPresent but then later in 
        // the scored word the same letter is marked 'CorrectNotHere' 
        // For instance, solution = "abbey" guess = "keeps". The score
        // is ".E...". Parsing left to right, the first 'e' in 'keeps' is
        // marked yellow and the second 'e' is marked gray. If you only
        // consider the second gray 'e' then you would mark it as not in the word
        // but it must be in the word.

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
            if (scoreChar == CorrectNotHere && ((charMask & correctMask) == 0))
            {
                // The word must contain this char somewhere
                query.SetMustContain(guess[charIndex]);
                // The word cannot contain this char at the charIndex position
                query.SetCantContain(charIndex, guess[charIndex]);
            }
        }

        for (int charIndex = 0; charIndex < n; ++charIndex)
        {
            CharScore scoreChar = score[charIndex];
            uint32_t charMask = (1 << (guess[charIndex] - 'a'));
            if (scoreChar == NotPresent)
            {
                if (((charMask & correctMask) == 0) &&
                    ((charMask & query.mustContain) == 0))
                {
                    // Set that the char is not present in the word anywhere.
                    query.SetCantContain(guess[charIndex]);
                }
                else
                {
                    // Set that the char can't be at this particular index
                    query.SetCantContain(charIndex, guess[charIndex]);
                }
            } 
               
        }
    }
    return query;
}


