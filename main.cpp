#include <iostream>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

enum CharScore { NotPresent, Correct, CorrectNotHere };

static std::vector<CharScore> noneFound5 = {NotPresent, NotPresent, NotPresent, NotPresent, NotPresent};

uint32_t ComputeMask( const std::string &word)
{
    uint32_t mask(0);

    for (int i = 0; i < word.size(); ++i)
    {
        if (word[i] >= 'a' && word[i] <= 'z')
            mask |= 1 << (word[i] - 'a');
    }
    return mask;
}

//std::vector<CharScore> Score( const std::string &solution, const std::string &guess)
//{
//    std::vector<CharScore> score;
//    uint32_t solutionMask = ComputeMask(solution);
//
//    for (int i = 0; i < solution.size(); ++i)
//    {
//        if (guess[i] == solution[i])
//        {
//            score.push_back( Correct );
//        }
//        else
//        {
//            uint32_t charMask = 1 << (guess[i] - 'a');
//
//            if (charMask & solutionMask)
//            {
//                score.push_back(CorrectNotHere);
//            }
//            else
//            {
//                score.push_back(NotPresent);
//            }
//        }
//    }
//    return score;
//}

struct Board
{
    int n;

    Board(int lettersPerWord)
    : n(lettersPerWord)
    {
    }

    void PushScoredGuess(std::string const &guessStr, const std::string& scoreStr)
    {
        std::vector<CharScore> score;
        for (size_t i = 0; i < guessStr.size(); ++i)
        {
            char ch = scoreStr[i];
            if (ch == '.')
            {
                score.push_back(NotPresent);
            }
            else
            {
                if (ch >= 'A' && ch <= 'Z')
                {
                    score.push_back( CorrectNotHere);
                }
                else if (ch >= 'a' && ch <= 'z')
                {
                    score.push_back( Correct);
                }
            }
        }
        guesses.push_back(guessStr);
        scores.push_back(score);
    }

    void Pop()
    {
        guesses.pop_back();
        scores.pop_back();
    }

    std::vector< std::string> guesses;
    std::vector< std::vector<CharScore> > scores;
};


struct WordQuery
{
    WordQuery(int _n)
        : n(_n)
        , mustContain(0)
        , correct(std::string(n, '.'))
    {
        cantContain.resize(n);
    }

    int n;  // character count per word

    // The letters the word must contain
    uint32_t mustContain;
    // Per character, the letters it cant contain.
    std::vector<uint32_t> cantContain;

    // List of characters in the correct place.
    // A dot means a blank space
    std::string correct;

    // No character can contain the character ch
    void SetCantContain(char ch)
    {
        ch = tolower(ch);
        for (int i = 0; i < n; ++i)
            cantContain[i] |= (1 << (ch - 'a'));
    }

    // The given character index cannot contain the given character
    void SetCantContain(int charIndex, char ch)
    {
        ch = tolower(ch);
        cantContain[charIndex] |= (1 << (ch - 'a'));
    }

    // Pretend like none of the characters in this guess are in the solution
    // The idea is to pretend that we guess the given guess and that the
    // new letters are not found. That would prune the search space alot.
    // We prune best when we introduce new letters to the query.
    void ScoreCandidate(const std::string &guess)
    {
        for (size_t i = 0; i < guess.size(); ++i) {
            char ch = guess[i];

            // Don't consider letters that are already correct
            if (guess[i] != correct[i]) {
                uint32_t charMask = (1 << (ch - 'a'));

                // Don't consider letters that we must have
                if ((charMask & mustContain) == 0) {
                    // This is a new letter. Pretend it is not in the solution
                    SetCantContain(ch);
                }
            }
        }
    }

    void SetMustContain( char ch )
    {
        ch = tolower(ch);
        mustContain |= (1 << (ch - 'a'));
    }

    void SetCorrect( int charIndex, char ch)
    {
        ch = tolower(ch);
        correct[charIndex] = ch;
    }

    bool Satisfies(const std::string &word) const
    {
        uint32_t wordMask = 0;

        for (size_t i = 0; i < cantContain.size(); ++i)
        {
            if (correct[i] != '.' && correct[i] != word[i])
            {
                return false;
            }
            uint32_t charMask = 1 << (word[i] - 'a');
            wordMask |= charMask;

            if (cantContain[i] & charMask)
                return false;
        }

        // The word must contain all the characters in the mustContain mask
        if ((mustContain & wordMask) != mustContain)
            return false;

        return true;
    }

};

void LoadDictionary( const std::filesystem::path &filename, std::vector<std::string> &words)
{
    std::fstream newfile;
    newfile.open(filename,std::ios::in);
    if (newfile.is_open())
    {
        std::string tp;
        while(getline(newfile, tp))
        {
            words.push_back(tp);
        }
        newfile.close();
    }
}

struct Bot
{
    std::vector<std::string> BestGuesses( const WordQuery& query,
                         const std::vector<std::string> &words,
                         const std::vector<std::string> &remaining,
                         size_t &resultingSearchSpaceSize)
    {
        std::vector<std::string> guesses;
        if (remaining.size() == 1)
        {
            resultingSearchSpaceSize = 1;
            guesses.push_back(remaining[0]);
            return guesses;
        }
        resultingSearchSpaceSize = 1000000;
        std::string bestGuess;

        // Try the remaining words first
        for (size_t remainingWordIndex = 0; remainingWordIndex < remaining.size(); ++remainingWordIndex)
        {
            WordQuery newQuery(query);
            const std::string& dictionaryWord = remaining[remainingWordIndex];
            newQuery.ScoreCandidate(dictionaryWord);

            size_t searchSpaceSize = SearchSpaceSize(newQuery, remaining);
            if (searchSpaceSize > 0 && searchSpaceSize < resultingSearchSpaceSize)
            {
                resultingSearchSpaceSize = searchSpaceSize;
                bestGuess = dictionaryWord;
            }
        }
        // Now see if there are any better in the entire dictionary
        for (size_t dictionaryWordIndex = 0; dictionaryWordIndex < words.size(); ++dictionaryWordIndex)
        {
            WordQuery newQuery(query);
            const std::string& dictionaryWord = words[dictionaryWordIndex];
            if (dictionaryWord == bestGuess)
                continue;

            newQuery.ScoreCandidate(dictionaryWord);

            size_t searchSpaceSize = SearchSpaceSize(newQuery, remaining);
            if (searchSpaceSize > 0 && searchSpaceSize < resultingSearchSpaceSize)
            {
                resultingSearchSpaceSize = searchSpaceSize;
                bestGuess = dictionaryWord;
             }
        }
        guesses.push_back(bestGuess);
        // Find words that are just as good as the bestGuess
        for (size_t dictionaryWordIndex = 0; dictionaryWordIndex < words.size(); ++dictionaryWordIndex)
        {
            WordQuery newQuery(query);
            const std::string& dictionaryWord = words[dictionaryWordIndex];
            if (dictionaryWord == bestGuess)
                continue;

            newQuery.ScoreCandidate(dictionaryWord);

            size_t searchSpaceSize = SearchSpaceSize(newQuery, remaining);
            if (searchSpaceSize > 0 && searchSpaceSize == resultingSearchSpaceSize)
            {
                guesses.push_back(dictionaryWord);
            }
        }
        return guesses;
    }

    WordQuery GenerateQuery( const Board &board, const std::vector<std::string> &words)
    {
        WordQuery query(board.n);
        for (size_t guessIndex = 0; guessIndex < board.guesses.size(); ++guessIndex)
        {
            std::string guess = board.guesses[guessIndex];
            auto score = board.scores[guessIndex];
            // Avoid a quirk in wordle where it scores guesses with a contradiction.
            // For instance, "moony" was scored "..oN." which says there is no 'o'
            // but there is an 'o' in the third position

            for (int charIndex = 0; charIndex < score.size(); ++charIndex)
            {
                CharScore scoreChar = score[charIndex];
                if (scoreChar == Correct)
                    query.SetCorrect( charIndex, guess[charIndex]);
            }
            uint32_t correctMask = ComputeMask(query.correct);
            for (int charIndex = 0; charIndex < score.size(); ++charIndex)
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

    static
    std::vector<std::string>  PruneSearchSpace( const WordQuery& query, const std::vector<std::string> &words)
    {
        std::vector<std::string> remaining;

        // Apply the query to each word
        for (size_t i = 0; i < words.size(); ++i)
        {
            if (query.Satisfies(words[i]))
            {
                remaining.push_back(words[i]);
            }
        }

        return remaining;
    }

    static size_t  SearchSpaceSize( const WordQuery& query, const std::vector<std::string> &words)
    {
        size_t size(0);

        for (const auto& w : words)
        {
            if (query.Satisfies(w))
            {
                size++;
            }
        }

        return size;
    }
};

int main(int argc, char *argv[])
{
    int n = 5;
    std::vector<std::string> words;
    LoadDictionary("/Users/scott/git_repos/wordlebot/words5_long", words);
    std::cout << "loaded " << words.size() << " words." << std::endl;

    Board board(5);

    for (int guesses = 1; guesses < argc; guesses+=2)
    {
        std::string guess = argv[guesses];
        std::string score = argv[guesses+1];

        std::cout << "Guessed : " << guess << " with score " << score << std::endl;

        if (guess.size() != n)
        {
            std::cerr << "Guess " << guess << " needs to have " << n << " characters in it" << std::endl;
            exit(1);
        }

        if (score.size() != n)
        {
            std::cerr << "Score " << score << " needs to have " << n << " characters in it" << std::endl;
            exit(1);
        }

        board.PushScoredGuess(guess, score);
    }
//    board.PushScoredGuess("adieu", "A....");
//    board.PushScoredGuess("story", ".T.R.");
//    board.PushScoredGuess( "chunk", "c....");
//    board.PushScoredGuess( "craft", "cRA.t");
    //board.PushScoredGuess( "brows", ".ro.s");

    Bot bot;
    WordQuery query = bot.GenerateQuery(board, words);
    std::vector<std::string>  remaining = bot.PruneSearchSpace(query, words);
    std::cout << "Remaining word count : " << remaining.size() << std::endl;
    for (auto const &w : remaining)
    {
        std::cout << w << std::endl;
    }

    size_t resultingSearchSpaceSize(0);
    auto guesses = bot.BestGuesses(query, words, remaining, resultingSearchSpaceSize);
    std::cout << "Guesses that reduce the search space to " << resultingSearchSpaceSize << std::endl;
    size_t count = std::min((size_t)5, guesses.size());
    for (size_t i = 0; i < count; ++i) {
            std::cout << guesses[i] << std::endl;
        }


    return 0;
}
