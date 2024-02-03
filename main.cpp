#include <iostream>

#include <string>

//#include "WordQuery.h"
//#include "Entropy.h"
//#include "ScoredWord.h"
#include "Board.h"
#include "Strategy.h"

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




