#include <iostream>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>

enum CharScore { NotPresent, Correct, CorrectNotHere };

static std::vector<CharScore> noneFound5 = {NotPresent, NotPresent, NotPresent, NotPresent, NotPresent};

uint32_t ComputeMask( const std::string &word)
{
    uint32_t mask(0);

    for (int i = 0; i < word.size(); ++i) {
        mask |= 1 << ('a' - word[i]);
    }
    return mask;
}

std::vector<CharScore> Score( const std::string &solution, const std::string &guess)
{
    std::vector<CharScore> score;
    uint32_t solutionMask = ComputeMask(solution);

    for (int i = 0; i < solution.size(); ++i)
    {
        if (guess[i] == solution[i])
        {
            score.push_back( Correct );
        }
        else
        {
            uint32_t charMask = 1 << (guess[i] - 'a');

            if (charMask & solutionMask)
            {
                score.push_back(CorrectNotHere);
            }
            else
            {
                score.push_back(NotPresent);
            }
        }
    }
    return score;
}

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
                    // This is a new letter. Pretend it is not there
                    cantContain[i] |= (1 << (guess[i] - 'a'));
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
    std::string BestGuess( const WordQuery& query,
                         const std::vector<std::string> &words,
                         const std::vector<std::string> &remaining,
                         size_t &resultingSearchSpaceSize)
    {
        if (remaining.size() == 1)
        {
            resultingSearchSpaceSize = 1;
            return remaining[0];
        }
        resultingSearchSpaceSize = 1000000;
        std::string bestGuess;

        // Try the remaining words first
        for (size_t dictionaryWordIndex = 0; dictionaryWordIndex < remaining.size(); ++dictionaryWordIndex)
        {
            WordQuery newQuery(query);
            const std::string& dictionaryWord = remaining[dictionaryWordIndex];
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
            newQuery.ScoreCandidate(dictionaryWord);

            size_t searchSpaceSize = SearchSpaceSize(newQuery, remaining);
            if (searchSpaceSize > 0 && searchSpaceSize < resultingSearchSpaceSize)
            {
                resultingSearchSpaceSize = searchSpaceSize;
                bestGuess = dictionaryWord;
            }
        }
        return bestGuess;
    }

    WordQuery GenerateQuery( const Board &board, const std::vector<std::string> &words)
    {
        WordQuery query(board.n);
        for (size_t guessIndex = 0; guessIndex < board.guesses.size(); ++guessIndex)
        {
            std::string guess = board.guesses[guessIndex];
            auto score = board.scores[guessIndex];
            for (int charIndex = 0; charIndex < score.size(); ++charIndex)
            {
                CharScore scoreChar = score[charIndex];
                if (scoreChar == NotPresent)
                {
                    query.SetCantContain(guess[charIndex]);
                }
                else if (scoreChar == CorrectNotHere)
                {
                    query.SetMustContain(guess[charIndex]);
                    query.SetCantContain(charIndex, guess[charIndex]);
                }
                else
                {
                    query.SetCorrect( charIndex, guess[charIndex]);
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
    LoadDictionary("/work/git_repos/wordlebot/words5", words);
    std::cout << "loaded " << words.size() << " words." << std::endl;

    Board board(5);

    for (int guesses = 1; guesses < argc; ++guesses)
    {
        board.PushScoredGuess(argv[guesses], argv[guesses+1]);
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
    std::string best = bot.BestGuess(query, words, remaining, resultingSearchSpaceSize);
    std::cout << "Best guess : " << best << " reduces the search space to " << resultingSearchSpaceSize << std::endl;
    return 0;
}
