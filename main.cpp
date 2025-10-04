#include <iostream>

#include <string>

//#include "WordQuery.h"
//#include "Entropy.h"
//#include "ScoredWord.h"
#include "Board.h"
#include "Strategy.h"
#include "LookaheadStrategy.h"
#include "WordleBot.h"

int main(int argc, char *argv[])
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
    std::filesystem::path dictPath(".");

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
        if (strcmp(argv[tokenIndex], "--dict") == 0)
        {
            dictPath = argv[tokenIndex + 1];
            tokenIndex += 2;
            continue;
        }
        if (strcmp(argv[tokenIndex], "--score") == 0)
        {
            std::string solution = argv[tokenIndex+1];
            std::string guess = argv[tokenIndex+2];
            auto scoredWord = Score(solution, guess);
            std::cout << "Scoring guess " << guess << " against solution " << solution << std::endl;
            printColored(std::cout, guess, scoredWord);
            std::cout << std::endl;
            return 0;
        }
        if (strcmp(argv[tokenIndex], "--solve") == 0)
        {
            std::string solution = argv[tokenIndex+1];
            std::vector<std::string> guessingWords;
            std::vector<std::string> solutionWords;
            LoadDictionaries(newYorkTimes, 5, dictPath, solutionWords, guessingWords);
            ScoreGroupingStrategy scoreGrouping(guessingWords, 10);
            //LookaheadStrategy lookahead( scoreGrouping, guessingWords, 20);
            Bot bot(guessingWords, solutionWords, scoreGrouping, true);
            bot.SolvePuzzle(solution, "slate");
            std::cout << std::endl;
            return 0;
        }

        std::string guess = argv[tokenIndex];
        std::string score;
        if (tokenIndex + 1 < argc)
        {
            score = argv[tokenIndex + 1];
        }
        else
        {
            std::cout << "Guess " << guess << " needs a score as the next parameter.\n" <<
            "The score is a string that reflects the colored letters returned for the guess\n" <<
            "green - use a lower case letter\n" <<
            "yellow - use an upper case letter\n" <<
            "gray - use a period character\n" <<
            "example : guess 'slate' is scored with green 's' and yellow 'l' and the rest of the letters are gray.\n" <<
            "Score would be 'sL...'" << std::endl;
            exit(1);
        }

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

    bool loaded = LoadDictionaries(newYorkTimes, n, dictPath, solutionWords, guessingWords);
    if (!loaded) {
        std::cerr << "The dictionaries were not found at " << dictPath << std::endl;
        return -1;
    }

    if (newYorkTimes)
        std::cout << "Using New York Times mode" << std::endl;
    else
        std::cout << "using Lion Studio App mode (use --nyt for New York Times)" << std::endl;

    //EntropyStrategy entropyStrategy(guessingWords, 50);
    ScoreGroupingStrategy strategy(guessingWords, 10);
    //LookaheadStrategy strategy(scoreGroupingStrategy,guessingWords, 10);

    WordQuery query = board.GenerateQuery();

    std::vector<std::string>  remaining = PruneSearchSpace(query, solutionWords);
    std::cout << "Remaining word count : " << remaining.size() << std::endl;
    for (int idx = 0; idx < remaining.size() ; ++idx)
    {
        std::cout << remaining[idx] << " ";
        if ((idx % 15) == 0 && (idx > 0))
        {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;

    std::cout << "Best guesses " << std::endl;
    auto bestGuesses = strategy.BestGuesses(board, remaining);
    for (const auto &g : bestGuesses)
    {
        std::cout << g.first << " : " << g.second;

        if (newYorkTimes)
        {
            if (std::binary_search(solutionWords.begin(), solutionWords.end(), g.first))
            {
                std::cout << " <-- solution word";
            }
        }
        std::cout << std::endl;
    }
}

// wordlebot search strategy doesn't favor the solution words as guesses
// Change Search Strategy to return which guess is most likely the solution.
// For small search spaces, have it look ahead and do the whole solution tree and return
// the likelihood of each guess. Try changing SolvePuzzle to return the chance that the given
// guess is the solution. Or make a new function that returns the chance.

// I keep thinking I'm going to write a strategy that looks ahead moves. There are too many guessing words
// for that. I either have to prune the guessing words by entropy (?) or prune the whole dictionary of words.
// I can make a dictionary of guessing words per opening word. So if the opening word is "slate" I can load
// slate.txt and use only the guessing words that are selected for some solution word.

// Since redoing the double letter scoring I have to revisit the WordQuery to see how it is affected.
// The "oozed" test case fails now due to the new scoring as an example.
