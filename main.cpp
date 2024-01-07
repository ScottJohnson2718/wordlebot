#include <iostream>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <random>

using ScoredGuess = std::pair< std::string, size_t >;

enum CharScore { NotPresent, Correct, CorrectNotHere };

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
    // new letters are not found. That would prune the search space a lot.
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

struct Board
{
    int n;

    Board(int lettersPerWord)
    : n(lettersPerWord)
    {
    }

    void PushScoredGuess(std::string const &guessStr, const std::vector<CharScore>& score)
    {
        guesses.push_back(guessStr);
        scores.push_back(score);
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

    std::vector< std::string> guesses;
    std::vector< std::vector<CharScore> > scores;
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

    std::vector<ScoredGuess> BestGuessesWithSearch( Board& board,
                                          const std::vector<std::string> &words,
                                          const std::vector<std::string> &remaining)
    {
        std::vector<ScoredGuess> guesses;
        if (remaining.size() == 1)
        {
            guesses.push_back(ScoredGuess(remaining[0], 1));
            return guesses;
        }
        std::vector< ScoredGuess > totalSearchSpace;

        // Try all the remaining words as guesses
        for (auto const &guessWord : remaining)
        {
            size_t totalSearchSpaceSize = 0;

            // Pretend that each of the remaining words is the solution and
            // see how well the current guess would prune the search space.
            for (auto const &possibleSolution : remaining)
            {
                auto score = Score(possibleSolution, guessWord);

                board.PushScoredGuess(guessWord, score);
                WordQuery query = board.GenerateQuery();

                size_t searchSpaceSize = SearchSpaceSize(query, remaining);
                totalSearchSpaceSize += searchSpaceSize;

                board.Pop();
            }
            totalSearchSpace.push_back( ScoredGuess( guessWord, totalSearchSpaceSize));
        }
        for (auto const &guessWord : words)
        {
            size_t totalSearchSpaceSize = 0;

            // Pretend that each of the remaining words is the solution and
            // see how well the current guess would prune the search space.
            for (auto const &possibleSolution : remaining)
            {
                auto score = Score(possibleSolution, guessWord);

                board.PushScoredGuess(guessWord, score);
                WordQuery query = board.GenerateQuery();

                size_t searchSpaceSize = SearchSpaceSize(query, remaining);
                totalSearchSpaceSize += searchSpaceSize;

                board.Pop();
            }
            totalSearchSpace.push_back( ScoredGuess( guessWord, totalSearchSpaceSize));
        }

        // Sort them small to big
        // Could use nth instead
        std::sort( totalSearchSpace.begin(), totalSearchSpace.end(),
                   []( const ScoredGuess &a, const ScoredGuess &b)
                   {
                        return a.second < b.second;
                   }
                   );
        for (size_t i = 0; i < 20 && i < totalSearchSpace.size(); ++i)
        {
            guesses.push_back( totalSearchSpace[i] );
        }
        return guesses;
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

    int SolvePuzzle( std::string const &hiddenSolution, const std::string &openingGuess,
                     const std::vector<std::string> &words , bool verbose = false)
    {
        Board board(5);

        return SolvePuzzle(board, hiddenSolution, openingGuess, words, verbose);
    }

    int SolvePuzzle( Board &board, std::string const &hiddenSolution, const std::string &openingGuess,
                     const std::vector<std::string> &words , bool verbose = false)
    {
        if (hiddenSolution == openingGuess)
            return 1;
        // Make the opening guess to reduce the search space right away
        auto firstScore = Score(hiddenSolution, openingGuess);

        board.PushScoredGuess( openingGuess, firstScore);

        bool solved = false;
        int guessCount = 1;
        std::string previousGuess = openingGuess;

        while (!solved) {
            WordQuery query = board.GenerateQuery();
            std::vector<std::string> remaining = PruneSearchSpace(query, words);
            if (remaining.size() == 1)
            {
                // You still need one more guess to make the final guess
                guessCount++;
                solved = true;
                if (verbose)
                    std::cout << hiddenSolution << " solved in " << guessCount << " guesses" << std::endl;
                break;
            }
            if (verbose)
                std::cout << "Remaining word count : " << remaining.size() << std::endl;

            if (remaining.empty())
            {
                std::cerr << "No solution for word " << hiddenSolution << std::endl;
                board.Pop();
                return 0;
            }
            auto guesses = BestGuessesWithSearch(board, words, remaining);

            std::string bestGuess;
            if (guesses.size() > 2 && (guesses[0].first != previousGuess))
            {
                // Pick from the list of guesses that most reduce the search space
                bestGuess = guesses[0].first; // primitive algorithm
            }
            else
            {
                // Pick from the remaining list of candidate words
                bestGuess = remaining[0];
            }
            auto score = Score(hiddenSolution, bestGuess);
            board.PushScoredGuess(bestGuess, score);

            previousGuess = bestGuess;
            guessCount++;
            if (guessCount > 10)
            {
                // Debug runaway condition
                std::cerr << "Solution word " << hiddenSolution << " guess is " << bestGuess << std::endl;
                std::cerr << "previous guess : " << previousGuess << std::endl;
                std::cerr << guesses.size() << " guesses, " << remaining.size() << " remaining " << std::endl;
            }
        }
        board.Pop();
        return guessCount;
    }

    // Return the number of guesses it takes to solve the puzzle with
    int ScoreGuess( Board &board, std::string const &hiddenSolution, const std::string &guess,
        const std::vector<std::string> &remaining)
    {
        return SolvePuzzle( board, hiddenSolution, guess, remaining);
    }
};

void TestWords(std::vector<std::string> &solutionWords, const std::string &openingGuess)
{
    Bot bot;

    int guesses = 0;
    for (auto const word : solutionWords)
    {
        guesses += bot.SolvePuzzle( word, openingGuess, solutionWords );
    }
    std::cout << "Total guesses for : "<< openingGuess << " " << guesses << std::endl;
}

void InteractiveRound(int argc, char *argv[])
{
    int n = 5;
    // These dictionaries do not share words between them. We can combine them without making duplicates.
    std::vector<std::string> guessingWords;
    LoadDictionary("/Users/scott/git_repos/wordlebot/words5_long", guessingWords);
    std::cout << "loaded " << guessingWords.size() << " words." << std::endl;

    std::vector<std::string> solutionWords;
    LoadDictionary("/Users/scott/git_repos/wordlebot/words5_short", solutionWords);
    std::cout << "loaded " << solutionWords.size() << " words." << std::endl;


    // New York Times mode keeps the solution words and the guessing words separate.
    // Wordlebot can converge on a solution quickly with fewer words available as solution words.
    // The Lion Studios app has a larger selection of solution words and using just the short
    // words dictionary misses solution words. In that case we have to use all the strange words
    // in the long dictionary as possible solution words. That is not optimal but this is the
    // the weakness in the dictionaries as used for the Lion Studios app.
    bool newYorkTimes = false;

    // Init a board for 5 letter words
    Board board(5);

    for (int tokenIndex = 1; tokenIndex < argc; )
    {
        if (strcmp(argv[tokenIndex], "--nyt") == 0)
        {
            newYorkTimes = true;
            ++tokenIndex;
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

    if (!newYorkTimes)
    {
        // For Lion Studios, we make one big dictionary and the guessing words and solution
        // words are actually the same list
        std::copy(guessingWords.begin(), guessingWords.end(), std::back_inserter(solutionWords));
        guessingWords = solutionWords;
    }
    // For new york times, again, we keep the solution words and the guessing words separate. The solutions
    // words is a fairly small list.

    if (newYorkTimes)
        std::cout << "Using New York Times mode" << std::endl;
    else
        std::cout << "using Lion Studio App mode (use --nyt for New York Times)" << std::endl;

    Bot bot;
    WordQuery query = board.GenerateQuery();
    std::vector<std::string>  remaining = bot.PruneSearchSpace(query, solutionWords);
    std::cout << "Remaining word count : " << remaining.size() << std::endl;
    for (auto const &w : remaining)
    {
        std::cout << w << std::endl;
    }

    auto bestGuesses = bot.BestGuessesWithSearch(board, guessingWords, remaining);
    std::cout << "Best guesses " << std::endl;
    for (const auto &g : bestGuesses)
    {
        std::cout << g.first << " : " << g.second << std::endl;
    }
}

void TestOpeningWords()
{
    std::vector<std::string> solutionWords;
    LoadDictionary("/Users/scott/git_repos/wordlebot/words5_short", solutionWords);
    std::cout << "loaded " << solutionWords.size() << " words." << std::endl;

    TestWords(solutionWords, "stoae");  // 9659
    TestWords(solutionWords, "arose");  // 9640
    TestWords(solutionWords, "limen");  // 9825
    TestWords(solutionWords, "daisy");  // 9822
}

// Cable is the solution
// My bot picked arose, glint, then duchy.
// belch is better third guess than mine
void Cable()
{
    std::vector<std::string> words;
    std::vector<std::string> remaining;
    LoadDictionary("/Users/scott/git_repos/wordlebot/words5_long", words);
    std::cout << "loaded " << words.size() << " words." << std::endl;
    std::string solution = "cable";

    Bot bot;
    Board board(5);
    board.PushScoredGuess("arose", Score(solution, "arose"));
    board.PushScoredGuess("glint", Score(solution, "glint"));
    auto query = board.GenerateQuery();

    remaining = bot.PruneSearchSpace(query, words);

    std::cout << "Search space size "<< remaining.size() << std::endl;

    auto bestGuesses = bot.BestGuessesWithSearch(board, words, remaining);

    std::cout << "Best guesses " << std::endl;
    for (const auto &g : bestGuesses)
    {
        std::cout << g.first << " : " << g.second << std::endl;
    }
   //int guessCount = bot.SolvePuzzle( "cable", "stale", solutionWords, true );
    //std::cout << "Total guesses for cable with opening : "<< "stale" << " " << guessCount << std::endl;
}

int main(int argc, char *argv[])
{
    //TestOpeningWords();
    InteractiveRound(argc, argv);
    //Cable();
    return 0;
}

// Word was votes
// wordlebot guessed " stale tents boric vapid votes
// Could it do better? tents doesn't seem like a good guess to me.
