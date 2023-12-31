#include <iostream>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>

enum CharScore { NotPresent, Correct, CorrectNotHere };

struct Board
{
    int n;

    Board(int lettersPerWord)
    : n(lettersPerWord)
    {
    }

    void AddScoredGuess(std::string const &guessStr, const std::string scoreStr)
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

    std::vector< std::string> guesses;
    std::vector< std::vector<CharScore> > scores;
};


struct WordQuery
{
    WordQuery(int n)
        : mustContain(0)
        , correct(std::string(n, '.'))
    {
        cantContain.resize(n);
    }

    // The letters the word must contain
    uint32_t mustContain;
    // Per character, the letters it cant contain.
    std::vector<uint32_t> cantContain;

    // List of characters in the correct place.
    // A dot means a blank space
    std::string correct;

    void SetCantContain(int charIndex, char ch)
    {
        ch = tolower(ch);
        cantContain[charIndex] |= (1 << (ch - 'a'));
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

    // Create a mask of the letters of the word
//    static uint32_t WordToMask( const std::string &word )
//    {
//        uint32_t mask = 0;
//        for (int i = 0; i < word.size(); ++i)
//        {
//            char ch = tolower(word[i]);
//            mask |= 1 << (ch - 'a');
//        }
//        return mask;
//    }

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

        if (!(mustContain & wordMask))
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
    //std::string BestGuess( const Server& server, const Words &words);

    std::vector<std::string>  PruneSearchSpace( const Board& board, const std::vector<std::string> &words)
    {
        std::vector<std::string> remaining;
        WordQuery query(board.n);

        // make the query
        for (size_t guessIndex = 0; guessIndex < board.guesses.size(); ++guessIndex)
        {
            std::string guess = board.guesses[guessIndex];
            auto score = board.scores[guessIndex];
            for (int charIndex = 0; charIndex < score.size(); ++charIndex)
            {
                CharScore scoreChar = score[charIndex];
                if (scoreChar == NotPresent)
                {
                    query.SetCantContain(charIndex, guess[charIndex]);
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
};


int main(int argc, char *argv[])
{
    int n = 5;
    std::vector<std::string> words;
    LoadDictionary("/Users/scott/words5", words);
    std::cout << "loaded " << words.size() << " words.";

    Board board(5);

    board.AddScoredGuess("strap", "S....");
    board.AddScoredGuess("limen", "l....");
    board.AddScoredGuess( "dough", ".oUgh");

    Bot bot;
    std::vector<std::string>  remaining = bot.PruneSearchSpace(board, words);
    std::cout << "Remaining word count : " << remaining.size() << std::endl;
    for (auto const &w : remaining)
    {
        std::cout << w << std::endl;
    }

    //std::string best = bot.BestGuess(server.board, remaining);
    //std::cout << "Best guess : " << best << std::endl;
    return 0;
}
