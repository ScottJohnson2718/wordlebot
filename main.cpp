#include <iostream>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <random>
#include <execution>

#include "WordQuery.h"
//#include "WordTree.h"
#include "Entropy.h"

using ScoredGuess = std::pair< std::string, float >;
using FrequencyTable = std::array<float, 26>;

enum CharScore { NotPresent = 1, Correct = 2, CorrectNotHere = 3 };

// An array of CharScore but put into bits in a uint32_t
struct ScoredWord
{
    void Set(int index, CharScore cs)
    {
        v |= cs << (index << 1);
    }

    CharScore Get(int index) const
    {
        index <<= 1;
        return static_cast<CharScore>((v >> index) & 3);
    }

    CharScore operator[](int index) const
    {
        return Get(index);
    }

    uint32_t v = 0;
};

uint32_t ComputeMask( const std::string &word)
{
    uint32_t mask(0);

    for (int i = 0; i < word.size(); ++i)
    {
        // This has to handle a '.' as a char that is skipped
        char ch = tolower(word[i]);
        if (ch >= 'a' && ch <= 'z')
            mask |= 1 << (ch - 'a');
    }
    return mask;
}

ScoredWord Score(const std::string& solution, const std::string& guess)
{
    ScoredWord score;
    uint32_t solutionMask = ComputeMask(solution);

    for (int i = 0; i < solution.size(); ++i)
    {
        if (guess[i] == solution[i])
        {
            score.Set(i, Correct);
        }
        else
        {
            uint32_t charMask = 1 << (guess[i] - 'a');

            if (charMask & solutionMask)
            {
                score.Set(i, CorrectNotHere);
            }
            else
            {
                score.Set(i, NotPresent);
            }
        }
    }
    return score;
}

// This is a Wordle board with the guesses and how
// they were scored.
struct Board
{
    int n;  // number of characters in each guess
    Board() = default;
    Board(int lettersPerWord)
    : n(lettersPerWord)
    {
    }

    void PushScoredGuess(std::string const &guessStr, ScoredWord& score)
    {
        guesses.push_back(guessStr);
        scores.push_back(score);
    }

    // This takes a guess such as "slate" and a score string such as "sL..E",
    // and turns the score string into an actual score the bot can use.
    // Then it adds the guess and the score to the board.
    void PushScoredGuess(std::string const &guessStr, const std::string& scoreStr)
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

    void Pop()
    {
        guesses.pop_back();
        scores.pop_back();
    }

    // Go through all the user guesses and how they were scored and 
    // create a query that can be used to see which words meet the conditions.
    WordQuery GenerateQuery() const
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

    std::vector< std::string> guesses;
    std::vector< ScoredWord > scores;
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

std::vector<std::string>  PruneSearchSpace(const WordQuery& query, const std::vector<std::string>& words)
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

struct Strategy
{

    // Return the best single guess for the given board
    virtual ScoredGuess BestGuess(Board& board,
        const std::vector<std::string>& solutionWords) const = 0;

    // Return a list of the best guesses for the given board.
    // This is useful in interactive mode when the guessed word
    // is not in the dictionary of the Wordle app. In that case,
    // there are alternative words to chose from.
    virtual std::vector<ScoredGuess> BestGuesses(Board& board,
        const std::vector<std::string>& solutionWords) const = 0;
};

// This strategy employs only entropy to chose the next guess given a board.
struct EntropyStrategy : public Strategy
{
    const std::vector<std::string>& guessingWords_;
    size_t maxGuessesReturned_;

    EntropyStrategy(
        const std::vector<std::string>& guessingWords, size_t maxGuessesReturned)
        : guessingWords_(guessingWords)
        , maxGuessesReturned_(maxGuessesReturned)
    {
    }

    virtual ScoredGuess BestGuess(Board& board,
        const std::vector<std::string>& solutionWords) const
    {
        if (solutionWords.size() == 1)
        {
            return ScoredGuess(solutionWords[0], 1);
        }

        ScoredGuess bestGuess;
        bestGuess.second = 0.0f;

        // Compute the frequency table on the remaining words that satisfy the board
        FrequencyTable freqs = charFrequency(solutionWords);

        for (const auto& w : guessingWords_)
        {
            float ent = entropy(w, freqs);
            if (ent > bestGuess.second)
            {
                bestGuess.first = w;
                bestGuess.second = ent;
            }
        }
        for (const auto& w : solutionWords)
        {
            float ent = entropy(w, freqs);
            if (ent > bestGuess.second)
            {
                bestGuess.first = w;
                bestGuess.second = ent;
            }
        }

        return bestGuess;
    }

    std::vector<ScoredGuess> BestGuesses(Board& board,
        const std::vector<std::string>& solutionWords) const
    {
        std::vector<ScoredGuess> guesses;
        if (solutionWords.size() == 1)
        {
            guesses.push_back(ScoredGuess(solutionWords[0], 1));
            return guesses;
        }

        // Compute the frequency table on the remaining words that satisfy the board
        FrequencyTable freqs = charFrequency(solutionWords);

        std::vector< ScoredGuess > scoredGuesses;
        std::vector< ScoredGuess > topGuessesByEntropy;

        for (const auto& w : guessingWords_)
        {
            float ent = entropy(w, freqs);
            scoredGuesses.push_back(ScoredGuess(w, ent));
        }
        for (const auto& w : solutionWords)
        {
            float ent = entropy(w, freqs);
            scoredGuesses.push_back(ScoredGuess(w, ent));
        }

        // Sort by entropy big to small
        std::sort(scoredGuesses.begin(), scoredGuesses.end(),
            [](const ScoredGuess& a, const ScoredGuess& b)
            {
                return a.second > b.second;
            });


        for (int i = 0; i < std::min(maxGuessesReturned_, scoredGuesses.size()); ++i)
        {
            topGuessesByEntropy.push_back(scoredGuesses[i]);
        }

        return topGuessesByEntropy;
    }
};

///////////////

// This strategy picks the word that reduces the total size of the search space remaining
struct SearchStrategy : public Strategy
{
    SearchStrategy(
        const std::vector<std::string>& guessingWords, size_t maxGuessesReturned)
        : guessingWords_(guessingWords)
        , maxGuessesReturned_(maxGuessesReturned)
    {
    }

    ScoredGuess BestGuess(Board& board,
        const std::vector<std::string>& solutionWords) const
    {
        if (solutionWords.size() == 1)
        {
            return ScoredGuess(solutionWords[0], 1);
        }

        // Assumes that the solutionWords have already been pruned by the query
        // that satisies the board. We only need to make a query with the new guess
        // because the query for all the previous guesses has already pruned the solution words.
        Board boardWithJustNewGuess(board.n);

        ScoredGuess bestGuess;
        bestGuess.second = 1e6;

        for (auto const& guessWord : guessingWords_)
        {
            size_t totalSearchSpaceSize = 0;

            // Pretend that each of the remaining words is the solution and
            // see how well the current guess would prune the search space.
            for (auto const& possibleSolution : solutionWords)
            {
                auto score = Score(possibleSolution, guessWord);

                boardWithJustNewGuess.PushScoredGuess(guessWord, score);
                WordQuery query = boardWithJustNewGuess.GenerateQuery();

                size_t searchSpaceSize = SearchSpaceSize(query, solutionWords);
                totalSearchSpaceSize += searchSpaceSize;

                boardWithJustNewGuess.Pop();
            }
            if (totalSearchSpaceSize < bestGuess.second)
            {
                bestGuess.first = guessWord;
                bestGuess.second = totalSearchSpaceSize;
            }
        }

        return bestGuess;
    }

    std::vector<ScoredGuess> BestGuesses(Board& board,
        const std::vector<std::string>& solutionWords) const
    {
        std::vector<ScoredGuess> guesses;
        if (solutionWords.size() == 1)
        {
            guesses.push_back(ScoredGuess(solutionWords[0], 1));
            return guesses;
        }

        std::vector< ScoredGuess > scoredGuesses;

        Board boardWithJustNewGuess(board.n);

        for (auto const& guessWord : guessingWords_)
        {
            size_t totalSearchSpaceSize = 0;

            // Pretend that each of the remaining words is the solution and
            // see how well the current guess would prune the search space.
            for (auto const& possibleSolution : solutionWords)
            {
                auto score = Score(possibleSolution, guessWord);

                boardWithJustNewGuess.PushScoredGuess(guessWord, score);
                WordQuery query = boardWithJustNewGuess.GenerateQuery();

                size_t searchSpaceSize = SearchSpaceSize(query, solutionWords);
                totalSearchSpaceSize += searchSpaceSize;

                boardWithJustNewGuess.Pop();
            }
            scoredGuesses.push_back(ScoredGuess(guessWord, (float)totalSearchSpaceSize));
        }

        // Sort them small to big
        std::sort(scoredGuesses.begin(), scoredGuesses.end(),
            [](const ScoredGuess& a, const ScoredGuess& b)
            {
                return a.second < b.second;
            }
        );
        for (size_t i = 0; i < maxGuessesReturned_ && i < scoredGuesses.size(); ++i)
        {
            guesses.push_back(scoredGuesses[i]);
        }
        return guesses;
    }

    static size_t  SearchSpaceSize(const WordQuery& query, const std::vector<std::string>& words)
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

    const std::vector<std::string>& guessingWords_;
    size_t maxGuessesReturned_;
};

///////////////

// This strategy picks the word that reduces the total size of the search space remaining
struct BlendedStrategy : public Strategy
{
    EntropyStrategy entropyStrategy_;
    SearchStrategy searchStrategy_;

    BlendedStrategy(
        const std::vector<std::string>& guessingWords, size_t maxGuessesReturned)
        : entropyStrategy_(guessingWords, maxGuessesReturned)
        , searchStrategy_(guessingWords, maxGuessesReturned)

    {
    }

    ScoredGuess BestGuess(Board& board,
        const std::vector<std::string>& solutionWords) const
    {
        if (solutionWords.size() > 50)
            return entropyStrategy_.BestGuess(board, solutionWords);

        return searchStrategy_.BestGuess(board, solutionWords);
    }

    std::vector<ScoredGuess> BestGuesses(Board& board,
        const std::vector<std::string>& solutionWords) const
    {
        if (solutionWords.size() > 50)
            return entropyStrategy_.BestGuesses(board, solutionWords);

        return searchStrategy_.BestGuesses(board, solutionWords);
    }
};


// The Wordle Bot. It solves Wordle puzzles.
struct Bot
{
    const std::vector<std::string>& guessingWords_;
    const std::vector<std::string>& solutionWords_;
    Strategy& strategy_;

    Bot(const std::vector<std::string>& guessingWords, const std::vector<std::string>& solutionWords, Strategy &strategy)
        : guessingWords_(guessingWords)
        , solutionWords_(solutionWords)
        , strategy_(strategy)
    {
    }

    int SolvePuzzle( std::string const &hiddenSolution, const std::string &openingGuess,bool verbose = false)
    {
        Board board(5);

        return SolvePuzzle(board, hiddenSolution, openingGuess, verbose);
    }

    int SolvePuzzle(Board& board, std::string const& hiddenSolution, const std::string& openingGuess,
        bool verbose = false)
    {
        if (hiddenSolution == openingGuess)
            return 1;
        
        // Make the opening guess to reduce the search space right away
        auto firstScore = Score(hiddenSolution, openingGuess);
        board.PushScoredGuess( openingGuess, firstScore);
        WordQuery query = board.GenerateQuery();
        auto remaining = PruneSearchSpace(query, solutionWords_);

        bool solved = false;
        int guessCount = 1;
        std::string previousGuess = openingGuess;
        
        while (!solved) {
            
            if (remaining.size() == 1)
            {
                solved = true;
                if (verbose)
                {
                    std::cout << hiddenSolution << " -> ";
                    for (auto v : board.guesses)
                    {
                        std::cout << v << " ";
                    }
                    std::cout << remaining[0] << std::endl;
                }
                guessCount++;
                break;
            }
            //if (verbose)
            //    std::cout << "Remaining word count : " << remaining.size() << std::endl;

            if (remaining.empty())
            {
                std::cerr << "No solution for word " << hiddenSolution << std::endl;
                board.Pop();
                return 0;
            }
            // Pick a new guess
            ScoredGuess guess;
            
            guess = strategy_.BestGuess(board, remaining);
           

            if (guess.first == previousGuess)
            {
                std::cerr << "Failed to solve when " << hiddenSolution << " was the solution." << std::endl;
                return 0;
            }

            previousGuess = guess.first;
            guessCount++;
            if (guessCount > 10)
            {
                // Debug runaway condition
                std::cerr << "Solution word " << hiddenSolution << " guess is " << guess.first << std::endl;
                std::cerr << "previous guess : " << previousGuess << std::endl;
                std::cerr << remaining.size() << " remaining " << std::endl;
            }
            
            ScoredWord score = Score(hiddenSolution, guess.first);
            board.PushScoredGuess(guess.first, score);
            query = board.GenerateQuery();
            remaining = PruneSearchSpace(query, remaining);
        }
        board.Pop();
        return guessCount;
    }

};

float TestWords(std::vector<std::string> &solutionWords, const std::vector < std::string>& guessingWords,
    const std::string &openingGuess, Strategy &strategy)
{
    Bot bot(guessingWords, solutionWords, strategy);

    int guesses = 0;
    std::for_each(std::execution::par_unseq, solutionWords.begin(), solutionWords.end(),
        [&guesses, &bot, &openingGuess](const std::string& word)
        {
            guesses += bot.SolvePuzzle(word, openingGuess, false);
        });
    /*for (auto const& word : solutionWords)
    {
        guesses += bot.SolvePuzzle( word, openingGuess, true );
    }*/
    std::cout << "Total guesses for : "<< openingGuess << " " << guesses << std::endl;
    std::cout << "Ave guesses : " << openingGuess << " " << (double) guesses / (double) solutionWords.size() << std::endl;
    return (double)guesses / (double)solutionWords.size();
}

void LoadDictionaries(bool newYorkTimes, int n, 
    std::vector<std::string>& solutionWords,
    std::vector<std::string>& guessingWords)
{
    if (!newYorkTimes)
    {
        if (n == 5)
        {
            LoadDictionary("words5_long", guessingWords);
            LoadDictionary("words5_short", solutionWords);
            // For Lion Studios, we make one big dictionary and the guessing words and solution
            // words are actually the same list
            std::copy(guessingWords.begin(), guessingWords.end(), std::back_inserter(solutionWords));
            guessingWords = solutionWords;
        }
        else if (n == 6)
        {
            LoadDictionary("words6", guessingWords);
            solutionWords = guessingWords;
        }
    }
    else
    {
        // For new york times, again, we keep the solution words and the guessing words separate. The solutions
        // words is a fairly small list.
        LoadDictionary("words5_long", guessingWords);
        LoadDictionary("words5_short", solutionWords);
    }

}

void InteractiveRound(int argc, char *argv[])
{
    int n = 5;

    // New York Times mode keeps the solution words and the guessing words separate.
    // Wordlebot can converge on a solution quickly with fewer words available as solution words.
    // The Lion Studios app has a larger selection of solution words and using just the short
    // words dictionary misses solution words. In that case we have to use all the strange words
    // in the long dictionary as possible solution words. That is not optimal but this is the
    // the weakness in the dictionaries as used for the Lion Studios app.
    bool newYorkTimes = false;

    Board board;

    for (int tokenIndex = 1; tokenIndex < argc; )
    {
        if (strcmp(argv[tokenIndex], "--nyt") == 0)
        {
            newYorkTimes = true;
            ++tokenIndex;
            n = 5;
            continue;
        }
        if (strcmp(argv[tokenIndex], "--six") == 0)
        {
            newYorkTimes = false;
            n = 6;
            tokenIndex++;
            continue;
        }

        std::string guess = argv[tokenIndex];
        std::string score = argv[tokenIndex+1];

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
        tokenIndex += 2;
    }
    board.n = n;

    // These dictionaries do not share words between them. We can combine them without making duplicates.
    std::vector<std::string> guessingWords;
    std::vector<std::string> solutionWords;

    LoadDictionaries(newYorkTimes, n, solutionWords, guessingWords);

    if (newYorkTimes)
        std::cout << "Using New York Times mode" << std::endl;
    else
        std::cout << "using Lion Studio App mode (use --nyt for New York Times)" << std::endl;

    BlendedStrategy strategy(guessingWords, 10);

    WordQuery query = board.GenerateQuery();

    std::vector<std::string>  remaining = PruneSearchSpace(query, solutionWords);
    std::cout << "Remaining word count : " << remaining.size() << std::endl;
    for (auto const &w : remaining)
    {
        std::cout << w << std::endl;
    }

    auto bestGuesses = strategy.BestGuesses(board, remaining);
    std::cout << "Best guesses " << std::endl;
    for (const auto &g : bestGuesses)
    {
        std::cout << g.first << " : " << g.second << std::endl;
    }
}

void TestEntropy()
{
    std::vector<std::string> solutionWords;
   
    solutionWords.push_back("bikes");
    solutionWords.push_back("hikes");
    solutionWords.push_back("likes");

    auto freqs = charFrequency(solutionWords);

    // The conclusion was that entropy alone does not make the correct choice.
    // The best guess is "bahil" because it differentiates between the choices
    // but it has less entropy than the others because the 'a' counts for zero.
    std::cout << "Entropy for guess bahil: " << entropy("bahil", freqs) << std::endl;
    std::cout << "Entropy for guess bikes: " << entropy("bikes", freqs) << std::endl;
    std::cout << "Entropy for guess hikes: " << entropy("hikes", freqs) << std::endl;
}
void TestOpeningWords()
{
    // These dictionaries do not share words between them. We can combine them without making duplicates.
    std::vector<std::string> guessingWords;
    std::vector<std::string> solutionWords;

    LoadDictionaries(true, 5, solutionWords, guessingWords);
    std::cout << "loaded " << solutionWords.size() << " words." << std::endl;
    std::cout << "loaded " << guessingWords.size() << " words." << std::endl;

    BlendedStrategy strategy(guessingWords, 10);

    TestWords(solutionWords, guessingWords, "slate", strategy);  // 9659
    //TestWords(solutionWords, "stoae");
    //TestWords(solutionWords, "arose");  // 9640
    //TestWords(solutionWords, "limen");  // 9825
    //TestWords(solutionWords, "daisy");  // 9822
}

void TestStrategy()
{
    // These dictionaries do not share words between them. We can combine them without making duplicates.
    std::vector<std::string> guessingWords;
    std::vector<std::string> solutionWords;

    LoadDictionaries(true, 5, solutionWords, guessingWords);
    std::cout << "loaded " << solutionWords.size() << " words." << std::endl;
    std::cout << "loaded " << guessingWords.size() << " words." << std::endl;

    BlendedStrategy blended(guessingWords, 10);
    EntropyStrategy entropy(guessingWords, 10);

    float aveGuessesToSolve;

    aveGuessesToSolve = TestWords(solutionWords, guessingWords, "slate", entropy);
    std::cout << "Ave guesses for entropy only strategy: " << aveGuessesToSolve << std::endl;
    aveGuessesToSolve = TestWords(solutionWords, guessingWords, "slate", blended);
    std::cout << "Ave guesses for blended strategy: " << aveGuessesToSolve << std::endl;
}

// Cable is the solution
// My bot picked arose, glint, then duchy.
// belch is better third guess than mine
//void Cable()
//{
//    std::vector<std::string> words;
//    std::vector<std::string> remaining;
//    LoadDictionary("/Users/scott/git_repos/wordlebot/words5_long", words);
//    std::cout << "loaded " << words.size() << " words." << std::endl;
//    std::string solution = "cable";
//
//    Bot bot;
//    Board board(5);
//    board.PushScoredGuess("arose", Score(solution, "arose"));
//    board.PushScoredGuess("glint", Score(solution, "glint"));
//    auto query = board.GenerateQuery();
//
//    remaining = bot.PruneSearchSpace(query, words);
//
//    std::cout << "Search space size "<< remaining.size() << std::endl;
//
//    auto bestGuesses = bot.BestGuessesWithSearch(board, words, remaining);
//
//    std::cout << "Best guesses " << std::endl;
//    for (const auto &g : bestGuesses)
//    {
//        std::cout << g.first << " : " << g.second << std::endl;
//    }
//   //int guessCount = bot.SolvePuzzle( "cable", "stale", solutionWords, true );
//    //std::cout << "Total guesses for cable with opening : "<< "stale" << " " << guessCount << std::endl;
//}

// Word was votes
// wordlebot guessed " stale tents boric vapid votes
// Could it do better? tents doesn't seem like a good guess to me.
void TestSolve()
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    LoadDictionaries(true, 5, solutionWords, guessingWords);

    BlendedStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy);

    bot.SolvePuzzle("votes", "stale", true);
}

void TestJoker()
{
    std::vector<std::string> solutionWords;
    std::vector<std::string> guessingWords;

    LoadDictionaries(true, 5, solutionWords, guessingWords);

    BlendedStrategy strategy(guessingWords, 10);
    Bot bot(guessingWords, solutionWords, strategy);

    bot.SolvePuzzle("joker", "stale", true);
}

//void TestWordTree()
//{
//    std::vector< std::string> words;
//    words.push_back("slate");
//    words.push_back("daisy");
//    words.push_back("adieu");
//
//    WordTree tree(words);
//    WordQuery query(5);
//    query.SetMustContain('a');
//    query.SetCantContain(0,'d');
//
//    auto w = tree.Satisfies(query);
//    for (auto ww : w)
//    {
//        std::cout << ww << std::endl;
//    }
//}

int main(int argc, char *argv[])
{
    //TestSolve();
    //TestEntropy();
    //TestOpeningWords();
    //TestJoker();
    TestStrategy();
    //TestWordTree();
    //InteractiveRound(argc, argv);
    //Cable();
    return 0;
}


