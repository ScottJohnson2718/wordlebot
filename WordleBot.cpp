

#include "WordleBot.h"

Bot::Bot(const std::vector<std::string>& guessingWords, const std::vector<std::string>& solutionWords, Strategy &strategy)
        : guessingWords_(guessingWords)
        , solutionWords_(solutionWords)
        , strategy_(strategy)
{
}

int Bot::SolvePuzzle( std::string const &hiddenSolution, const std::string &openingGuess,bool verbose)
{
    Board board(5);

    return SolvePuzzle(board, hiddenSolution, openingGuess, verbose);
}

int Bot::SolvePuzzle(Board& board, std::string const& hiddenSolution, const std::string& openingGuess,
                bool verbose)
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
        if (verbose)
            std::cout << "Remaining word count : " << remaining.size() << std::endl;

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
            std::cerr << "Guessed " << guess.first << " twice in a row." << std::endl;
            return 0;
        }

        if (verbose)
            std::cout << "Best guess : " << guess.first << std::endl;
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
