
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

    std::vector<ScoredGuess> scoredGuesses = subStrategy_.BestGuesses(board, solutionWords);
    std::vector<std::string> prunedGuessWords;
    prunedGuessWords.reserve(scoredGuesses.size());
    for (auto const& scoredGuess : scoredGuesses)
    {
        const std::string& guessWord = scoredGuess.first;

        prunedGuessWords.push_back(guessWord);
    }
    scoredGuesses.clear();

    ScoredGuess bestGuess;
    size_t minSearchSpace = std::numeric_limits<size_t>::max();

    for (auto const& guessWord : prunedGuessWords)
    {
        auto wordScores = ScoresByGuess( guessWord, solutionWords);

        size_t totalSearchSpace = 0;
        for (auto sc : wordScores)
        {
            board.PushScoredGuess(guessWord, sc);

            WordQuery query = board.GenerateQuery();
            auto prunedSolutions = PruneSearchSpace( query, solutionWords );

            Bot::SearchResult result;
            Bot bot( prunedGuessWords, prunedSolutions, subStrategy_);
            bot.Search(board, prunedSolutions, result);

            board.Pop();

            totalSearchSpace += result.totalSize;
        }
        if (totalSearchSpace < minSearchSpace)
        {
            minSearchSpace = totalSearchSpace;
            bestGuess.first = guessWord;
            bestGuess.second = totalSearchSpace;
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

    // The point of this is that we don't do all kinds of searching on dumb guesses.
    scoredGuesses = subStrategy_.BestGuesses(board, solutionWords);
    std::vector<std::string> prunedGuessWords;
    for (auto const& scoredGuess : scoredGuesses)
    {
        const std::string &guessWord = scoredGuess.first;

        prunedGuessWords.push_back(guessWord);
    }
    // Don't prune out the actual guess words.
    std::copy( solutionWords.begin(), solutionWords.end(), std::back_inserter(prunedGuessWords));
    scoredGuesses.clear();

    // let's be reasonable about how many iterations we intend to do
    if (prunedGuessWords.size() * solutionWords.size() > 5000)
    {
        std::cout << "Used substrategy because guesses * solutionWords was " << prunedGuessWords.size() * solutionWords.size() << std::endl;
        return subStrategy_.BestGuesses(board, solutionWords);
    }

    for (auto const& guessWord : prunedGuessWords)
    {
        auto wordScores = ScoresByGuess( guessWord, solutionWords);

        std::cout << "Guess : " << guessWord;
        size_t totalSearchSpace = 0;
        size_t maxDepth = 0;
        // Recurse on the scores that are possible from the chosen guess
        for (auto sc : wordScores)
        {
            std::cout << " " << sc.ToString(guessWord);
            board.PushScoredGuess(guessWord, sc);

            WordQuery query = board.GenerateQuery();
            auto prunedSolutions = PruneSearchSpace( query, solutionWords );

            Bot::SearchResult result;
            Bot bot( prunedGuessWords, prunedSolutions, subStrategy_);
            bot.Search(board, prunedSolutions, result);

            board.Pop();

            if (maxDepth < result.maxDepth)
                maxDepth = result.maxDepth;
            totalSearchSpace += result.totalSize;
        }
        float s = (float) totalSearchSpace - 0.5f *  maxDepth;
        if (std::binary_search(solutionWords.begin(), solutionWords.end(), guessWord))
        {
            // Solution words get a bonus. This must happen before they are sorted and the top ones returned
            s -= 1.0;
        }
        std::cout <<  " " << s << std::endl;
        scoredGuesses.push_back( ScoredGuess( guessWord, s));
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

