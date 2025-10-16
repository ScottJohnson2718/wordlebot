

#include "WordleBot.h"
#include "ScoredWord.h"

#include <map>

Bot::Bot(const std::vector<std::string>& guessingWords,
         const std::vector<std::string>& solutionWords,
         Strategy &strategy, bool verbose)
        : guessingWords_(guessingWords)
        , solutionWords_(solutionWords)
        , strategy_(strategy)
        , verbose_(verbose)
{
}

int Bot::SolvePuzzle(const std::string& hiddenSolution, const std::string& openingGuess)
{
    Board board;
    return SolvePuzzle(board, hiddenSolution, openingGuess);
}

int Bot::SolvePuzzle( Board& board, const std::string &hiddenSolution, const std::string& openingGuess)
{
    bool first = true;
    if (verbose_)
    {
        std::cout << "Solving for word " << hiddenSolution << std::endl;
    }

    if (!std::binary_search(solutionWords_.begin(), solutionWords_.end(), hiddenSolution))
    {
        std::cout << "Can't solve for " << hiddenSolution << " since it is not in the solution dictionary" << std::endl;
        return 0;
    }

    auto remaining = solutionWords_;

    while (true)
    {
        std::string selectedGuess;

        if (remaining.empty())
        {
            // Either the solution is not in the dictionary or there was a contradiction in the board
            std::cerr << "No solution for word " << hiddenSolution << std::endl;
            return 100000;  // ruin the overall statistics of any strategy that fails
        }

        if (first)
        {
            selectedGuess = openingGuess;
            first = false;
        }
        else if (remaining.size() <= 2)
        {
            selectedGuess = remaining[0];
        }
        else
        {
            ScoredGuess guess = strategy_.BestGuess(board, remaining);
            selectedGuess = guess.first;
        }

        // Eval the new guess and reduce the search space
        ScoredWord score = Score(hiddenSolution, selectedGuess);
        board.PushScoredGuess(selectedGuess, score);

        remaining = PruneSearchSpace(selectedGuess, score, remaining);

        if (verbose_)
            std::cout << "Best guess : " << selectedGuess;

        if (board.guesses.back() == hiddenSolution)
        {

            // Solved
            if (verbose_)
            {
                std::cout << std::endl; // ends Best guess line above
                std::cout << hiddenSolution << " -> ";
                for (size_t i = 0; i < board.guesses.size(); ++i)
                {
                    printColored(std::cout, board.guesses[i], board.scores[i]) << " ";
                }
                std::cout << std::endl;
            }
            break;
        }

        if (verbose_)
            std::cout << " with " << remaining.size() << " remaining" << std::endl;

    }
    return board.guesses.size();
}

// Determine the properties of the subtree of the search space
void Bot::SearchRecurse(Board& board,
                               const std::vector<std::string>& remainingSolutions,
                               SearchResult &result) {

    if (remainingSolutions.size() == 0)
    {
        // Failure. All solutions pruned. The board has a contradiction or the dictionary
        // doesn't have the solution word.
        return;
    }
    // Count the number of visited states in the search tree
    result.totalSize++;

    if (remainingSolutions.size() == 1)
    {
        // Solved. Don't recurse.
        result.maxDepth = std::max(result.maxDepth, board.guesses.size());
        result.minDepth = std::min(result.minDepth, board.guesses.size());
    }
    else
    {
        // use the strategy to pick the best guess.
        auto bestScoredGuess = strategy_.BestGuess(board, remainingSolutions);
        std::string bestGuessWord = bestScoredGuess.first;

        auto wordScores = ScoresByGuess( bestGuessWord, remainingSolutions);

        // Recurse on the scores that are possible from the chosen guess
        // How to define the search space? It can't be every possible guess against
        // every possible solution. That explodes too fast to search ahead several guesses.
        // This seems reasonable. We pick the best guess and iterate across the various
        // scores that could be returned
        for (auto sc : wordScores)
        {
            board.PushScoredGuess(bestGuessWord, sc);

            size_t size = remainingSolutions.size();
            auto pruned = board.PruneSearchSpace( remainingSolutions );
            if (pruned.size() < size)
            {
                SearchRecurse(board, pruned, result);
            }

            board.Pop();
        }
    }
}

// Determine the properties of the subtree of the search space
void Bot::Search(Board& board,
                        const std::vector<std::string>& remainingSolutions,
                        SearchResult &result)
{
    result.maxDepth = 0;
    result.minDepth = std::numeric_limits<size_t>::max();
    result.totalSize = 0;
    //result.guessedWords.clear();
    SearchRecurse(board, remainingSolutions, result);
}