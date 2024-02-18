

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

    if (remaining.empty())
    {
        // Failed. Not in dictionary or query created a contradiction
        return 0;
    }
    else if (remaining.size() == 1)
    {
        // The opening guess and then guessing the remaining one is two guesses
        std::string nextGuess = remaining[0];
        board.PushScoredGuess( nextGuess, Score(hiddenSolution, nextGuess));
        return board.guesses.size();
    }

    return SolvePuzzle(board, hiddenSolution, remaining);
}

int Bot::SolvePuzzle( Board& board, const std::string &hiddenSolution, const std::vector<std::string>& remainingSolutions)
{
    ScoredGuess guess;
    std::vector<std::string> remaining = remainingSolutions;

    while (true)
    {
        if (remaining.empty())
        {
            // Either the solution is not in the dictionary or there was a contradiction in the board
            std::cerr << "No solution for word " << hiddenSolution << std::endl;
            return 0;
        }
        else if (remaining.size() <= 2)
        {
            // Just pick the first one
            guess.first = remaining[0];
            guess.second = 1.0f;
           
            if (remaining.size() > 1) {
                remaining[0] = remaining[1];
                remaining.pop_back();
            }
            if (verbose_)
                std::cout << "Best guess : " << guess.first << " with " << remaining.size() << " left" << std::endl;
            ScoredWord score = Score(hiddenSolution, guess.first);
            board.PushScoredGuess(guess.first, score);
        }
        else
        {
            // Pick a new guess using the strategy
            size_t prevRemainingCount = remaining.size();
            guess = strategy_.BestGuess(board, remaining);

            // Eval the new guess and reduce the search space
            ScoredWord score = Score(hiddenSolution, guess.first);
            board.PushScoredGuess(guess.first, score);
            WordQuery query = board.GenerateQuery();
            remaining = PruneSearchSpace(query, remaining);

            if (verbose_)
                std::cout << "Best guess : " << guess.first << " with " << remaining.size() << " left" << std::endl;

            // The search space has to always get smaller.
            if (remaining.size() >= prevRemainingCount) {
                std::cerr << "Failed to solve when " << hiddenSolution << " was the solution." << std::endl;
                std::cerr << "Guess " << guess.first << " did not reduce the search space from " << remaining.size()
                          << std::endl;
                return 0;
            }
        }
        if (board.guesses.back() == hiddenSolution)
        {
            // Solved
            if (verbose_)
            {
                std::cout << hiddenSolution << " -> ";
                for (auto v: board.guesses)
                {
                    std::cout << v << " ";
                }
                std::cout << std::endl;
            }
            break;
        }
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

        // Note that we picked this word.
        //result.guessedWords.insert(bestScoredGuess.first);

        // This guess separates the remaining solutions into groups according to a common board score
        std::map<ScoredWord, std::shared_ptr<std::vector<std::string>>> groups;
        for (auto const &possibleSolution: remainingSolutions)
        {
            ScoredWord s = Score(possibleSolution, bestGuessWord);
            auto iter = groups.find(s);
            if (iter == groups.end())
            {
                auto strList = std::make_shared<std::vector<std::string>>();
                strList->push_back(possibleSolution);
                groups[s] = strList;
            }
            else
            {
                iter->second->push_back(possibleSolution);
            }
        }

//        int groupIndex  =0;
//        for (auto const &p : groups)
//        {
//            std::cout << groupIndex++ << " : ";
//            for (auto const &str : *p.second)
//            {
//                std::cout << str << " ";
//            }
//            std::cout << std::endl;
//        }

        // Recurse on the groups of solution words
        for (auto &p: groups)
        {
            board.PushScoredGuess(bestGuessWord, p.first);

            SearchRecurse(board, *p.second, result);

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