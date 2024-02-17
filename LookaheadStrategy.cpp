
#include "LookaheadStrategy.h"

#include "WordleBot.h"
#include "ScoredWord.h"

#include <map>

LookaheadStrategy::LookaheadStrategy( Strategy& subStrategy,
        const std::vector<std::string>& guessingWords,
        size_t maxGuessesReturned)
        : subStrategy_(subStrategy)
        , guessingWords_(guessingWords)
        , maxGuessesReturned_(maxGuessesReturned)
{
}

ScoredGuess LookaheadStrategy::BestGuess(Board& board,
                                      const std::vector<std::string>& solutionWords) const
{
    if (solutionWords.size() == 1)
    {
        return ScoredGuess(solutionWords[0], 1);
    }

    // Find at most the best 100 guesses. We can't fan out into an infinite search now can we?
    EntropyStrategy entropyStrategy(guessingWords_, 100);
    std::vector<ScoredGuess> scoredGuesses = entropyStrategy.BestGuesses(board, solutionWords);
    std::vector<std::string> prunedGuessWords;
    for (auto const& scoredGuess : scoredGuesses) {
        const std::string &guessWord = scoredGuess.first;

        prunedGuessWords.push_back(guessWord);
    }

    ScoredGuess bestGuess;
    size_t minSearchSpace = std::numeric_limits<size_t>::max();

    for (auto const& possibleSolution : solutionWords)
    {
        // Pretend that we guess this word and see how it would go.
        for (auto const& guessWord : prunedGuessWords)
        {
            Bot::SearchResult result;
            auto scoredWord = Score(guessWord, possibleSolution);
            Board delta( board.n);

            delta.PushScoredGuess( guessWord, scoredWord);
            WordQuery query = board.GenerateQuery();

            auto remaining = PruneSearchSpace(query, solutionWords);

            Bot slaveBot( prunedGuessWords, remaining, subStrategy_);

            // Do a full search of the remaining space
            slaveBot.Search(board, remaining, result);

            if (minSearchSpace < result.totalSize)
            {
                minSearchSpace = result.totalSize;
                bestGuess.first = guessWord;
                bestGuess.second = result.totalSize;
            }
        }
    }

    return bestGuess;
}

std::vector<ScoredGuess> LookaheadStrategy::BestGuesses(Board& board,
                                                     const std::vector<std::string>& solutionWords) const
{
    std::vector<ScoredGuess> scoredGuesses;
    if (solutionWords.size() == 1)
    {
        scoredGuesses.push_back(ScoredGuess(solutionWords[0], 1));
        return scoredGuesses;
    }

    // Find at most the best 100 guesses. We can't fan out into an infinite search now can we?
    EntropyStrategy entropyStrategy(guessingWords_, 100);
    scoredGuesses = entropyStrategy.BestGuesses(board, solutionWords);
    std::vector<std::string> prunedGuessWords;
    for (auto const& scoredGuess : scoredGuesses) {
        const std::string &guessWord = scoredGuess.first;

        prunedGuessWords.push_back(guessWord);
    }

    for (auto const& possibleSolution : solutionWords)
    {
        for (auto const& guessWord : prunedGuessWords)
        {
            Bot::SearchResult result;
            auto scoredWord = Score(guessWord, possibleSolution);
            Board delta( board.n);

            delta.PushScoredGuess( guessWord, scoredWord);
            WordQuery query = board.GenerateQuery();

            auto remaining = PruneSearchSpace(query, solutionWords);

            Bot slaveBot( prunedGuessWords, remaining, subStrategy_);

            // Do a full search of the remaining space
            slaveBot.Search(board, solutionWords, result);

            scoredGuesses.push_back( ScoredGuess(guessWord, result.totalSize));
        }
    }

    // Sort them small to big
    std::sort(scoredGuesses.begin(), scoredGuesses.end(),
              [](const ScoredGuess& a, const ScoredGuess& b)
              {
                  return a.second < b.second;
              }
    );

    // Crude. Yikes.
    while (scoredGuesses.size() > maxGuessesReturned_)
    {
        scoredGuesses.pop_back();
    }

    return scoredGuesses;
}

