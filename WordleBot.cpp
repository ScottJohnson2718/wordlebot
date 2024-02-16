

#include "WordleBot.h"

Bot::Bot(const std::vector<std::string>& guessingWords,
         const std::vector<std::string>& solutionWords,
         Strategy &strategy, bool verbose)
        : guessingWords_(guessingWords)
        , solutionWords_(solutionWords)
        , strategy_(strategy)
        , verbose_(verbose)
{
}

int Bot::SolvePuzzle( std::string const &hiddenSolution, const std::string &openingGuess)
{
    Board board(5);

    return SolvePuzzle(board, hiddenSolution, openingGuess);
}

int Bot::SolvePuzzle(Board& board, std::string const& hiddenSolution, const std::string& openingGuess)
{
    if (hiddenSolution == openingGuess)
        return 1;

    // Make the opening guess to reduce the search space right away
    auto firstScore = Score(hiddenSolution, openingGuess);
    board.PushScoredGuess( openingGuess, firstScore);
    WordQuery query = board.GenerateQuery();
    auto remaining = PruneSearchSpace(query, solutionWords_);

    bool solved = false;
    size_t prevRemainingCount = remaining.size();
    ScoredGuess guess;
    guess.first = openingGuess;
    guess.second = 1.0f;    // arbitrary

    while (!solved)
    {
        if (board.guesses.back() == hiddenSolution)
        {
            // Solved!
            solved = true;
            if (verbose_)
            {
                std::cout << hiddenSolution << " -> ";
                for (auto v : board.guesses)
                {
                    std::cout << v << " ";
                }
                std::cout << std::endl;
            }
            break;
        }
        if (remaining.empty())
        {
            std::cerr << "No solution for word " << hiddenSolution << std::endl;
            board.Pop();
            return 0;
        }
        else if (remaining.size() <= 2)
        {
            // Just pick the first one
            solved = false;
            guess.first = remaining[0];
            guess.second = 1.0f;
            prevRemainingCount = remaining.size();
            if (remaining.size() > 1) {
                remaining[0] = remaining[1];
                remaining.pop_back();
            }
            ScoredWord score = Score(hiddenSolution, guess.first);
            board.PushScoredGuess(guess.first, score);
        }
        else
        {
            // Pick a new guess using the strategy
            guess = strategy_.BestGuess(board, remaining);

            // Eval the new guess and reduce the search space
            ScoredWord score = Score(hiddenSolution, guess.first);
            board.PushScoredGuess(guess.first, score);
            query = board.GenerateQuery();
            remaining = PruneSearchSpace(query, remaining);

            // The search space has to always get smaller.
            if (remaining.size() >= prevRemainingCount)
            {
                std::cerr << "Failed to solve when " << hiddenSolution << " was the solution." << std::endl;
                std::cerr << "Guess " << guess.first << " did not reduce the search space from " << remaining.size() << std::endl;
                return 0;
            }
            prevRemainingCount = remaining.size();
        }

        //if (verbose)
        //    std::cout << "Best guess : " << guess.first << std::endl;
    }
    return board.guesses.size();
}


//int Bot::SolvePuzzleRecurse( std::string &hiddienSolution, const std::vector<std::string>& remaining)
//{
//
//}
