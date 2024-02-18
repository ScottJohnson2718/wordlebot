
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
    for (auto const& scoredGuess : scoredGuesses)
    {
        const std::string &guessWord = scoredGuess.first;

        prunedGuessWords.push_back(guessWord);
    }
    scoredGuesses.clear();

    ScoredGuess bestGuess;
    size_t minSearchSpace = std::numeric_limits<size_t>::max();

    for (auto const& guessWord : prunedGuessWords)
    {
        size_t totalSearchSpacePerGuess = 0;

        // This guess separates the remaining solutions into groups according to a common board score
        std::map<ScoredWord, std::shared_ptr<std::vector<std::string>>> groups;
        for (auto const &possibleSolution: solutionWords)
        {
            ScoredWord s = Score(possibleSolution, guessWord);
            auto iter = groups.find(s);
            if (iter == groups.end()) {
                auto strList = std::make_shared<std::vector<std::string>>();
                strList->push_back(possibleSolution);
                groups[s] = strList;
            } else {
                iter->second->push_back(possibleSolution);
            }
        }

        // Process the groups of solution words
        for (auto &p: groups)
        {
            const std::vector<std::string> &remaining = *p.second;

            Bot slaveBot(prunedGuessWords, remaining, subStrategy_);

            // Do a full search of the remaining space
            Bot::SearchResult result;
            slaveBot.Search(board, remaining, result);

            totalSearchSpacePerGuess += result.totalSize;
        }
        if (totalSearchSpacePerGuess < minSearchSpace)
        {
            minSearchSpace = totalSearchSpacePerGuess;
            bestGuess.first = guessWord;
            bestGuess.second = totalSearchSpacePerGuess;
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
    scoredGuesses.clear();

    for (auto const& guessWord : prunedGuessWords)
    {
        size_t totalSearchSpacePerGuess = 0;

        // This guess separates the remaining solutions into groups according to a common board score
        std::map<ScoredWord, std::shared_ptr<std::vector<std::string>>> groups;
        for (auto const &possibleSolution: solutionWords)
        {
            ScoredWord s = Score(possibleSolution, guessWord);
            auto iter = groups.find(s);
            if (iter == groups.end()) {
                auto strList = std::make_shared<std::vector<std::string>>();
                strList->push_back(possibleSolution);
                groups[s] = strList;
            } else {
                iter->second->push_back(possibleSolution);
            }
        }

        // Process the groups of solution words
        for (auto &p: groups)
        {
            const std::vector<std::string> &remaining = *p.second;

            Bot slaveBot(prunedGuessWords, remaining, subStrategy_);

            // Do a full search of the remaining space
            Bot::SearchResult result;
            slaveBot.Search(board, remaining, result);

            totalSearchSpacePerGuess += result.totalSize;
        }
        scoredGuesses.push_back( ScoredGuess(guessWord, (float) totalSearchSpacePerGuess));
    }

    // Remove duplicate guesses by string
    RemoveDuplicateGuesses(scoredGuesses);

    // Sort them small to big
    std::sort(scoredGuesses.begin(), scoredGuesses.end(),
              [](const ScoredGuess& a, const ScoredGuess& b)
              {
                  return a.second < b.second;
              }
    );

    std::vector< ScoredGuess > topGuessesByScore;
    for (int i = 0; i < std::min(maxGuessesReturned_, scoredGuesses.size()); ++i)
    {
        topGuessesByScore.push_back(scoredGuesses[i]);
    }

    return topGuessesByScore;
}

