
#include "Strategy.h"

#include <iostream>

// This strategy employs only entropy to chose the next guess given a board.
EntropyStrategy::EntropyStrategy(
        const std::vector<std::string>& guessingWords, size_t maxGuessesReturned)
        : guessingWords_(guessingWords)
        , maxGuessesReturned_(maxGuessesReturned)
{
}

ScoredGuess EntropyStrategy::BestGuess(Board& board,
                              const std::vector<std::string>& solutionWords) const
{
    if (solutionWords.size() == 1)
    {
        return ScoredGuess(solutionWords[0], 1);
    }

    WordQuery query = board.GenerateQuery();
    // Compute the frequency table on the remaining words that satisfy the board
    FrequencyTable freqs = charFrequency(solutionWords, query);

    ScoredGuess bestGuess;
    if (!solutionWords.empty())
    {
        bestGuess.first = solutionWords[0];
        bestGuess.second = entropy(solutionWords[0], freqs);
    }

    for (const auto& w : guessingWords_)
    {
        float ent = entropy(w, freqs);
        if (ent > bestGuess.second)
        {
            bestGuess.first = w;
            bestGuess.second = ent;
        }
    }
    for (const auto& w : solutionWords)
    {
        float ent = entropy(w, freqs);
        if (ent > bestGuess.second)
        {
            bestGuess.first = w;
            bestGuess.second = ent;
        }
    }

    return bestGuess;
}

std::vector<ScoredGuess> EntropyStrategy::BestGuesses(Board& board,
                                     const std::vector<std::string>& solutionWords) const
{
    std::vector<ScoredGuess> guesses;
    if (solutionWords.size() == 1)
    {
        guesses.push_back(ScoredGuess(solutionWords[0], 1));
        return guesses;
    }

    // Compute the frequency table on the remaining words that satisfy the board
    FrequencyTable freqs = charFrequency(solutionWords);

    WordQuery query = board.GenerateQuery();
    freqs = removeKnownChars(freqs, query.correct);

    std::vector< ScoredGuess > scoredGuesses;
    std::vector< ScoredGuess > topGuessesByEntropy;

    for (const auto& w : guessingWords_)
    {
        float ent = entropy(w, freqs);
        scoredGuesses.push_back(ScoredGuess(w, ent));
    }
    for (const auto& w : solutionWords)
    {
        float ent = entropy(w, freqs);
        scoredGuesses.push_back(ScoredGuess(w, ent));
    }

    // Sort by entropy big to small
    std::sort(scoredGuesses.begin(), scoredGuesses.end(),
              [](const ScoredGuess& a, const ScoredGuess& b)
              {
                  return a.second > b.second;
              });


    for (int i = 0; i < std::min(maxGuessesReturned_, scoredGuesses.size()); ++i)
    {
        topGuessesByEntropy.push_back(scoredGuesses[i]);
    }

    return topGuessesByEntropy;
}

///////////////

SearchStrategy::SearchStrategy(
            const std::vector<std::string>& guessingWords, size_t maxGuessesReturned)
            : guessingWords_(guessingWords)
            , maxGuessesReturned_(maxGuessesReturned)
    {
    }

ScoredGuess SearchStrategy::BestGuess(Board& board,
                      const std::vector<std::string>& solutionWords) const
{
    if (solutionWords.size() == 1)
    {
        return ScoredGuess(solutionWords[0], 1);
    }

    // Assumes that the solutionWords have already been pruned by the query
    // that satisies the board. We only need to make a query with the new guess
    // because the query for all the previous guesses has already pruned the solution words.
    Board boardWithJustNewGuess(board.n);

    ScoredGuess bestGuess;
    bestGuess.second = 1e6;

    for (auto const& guessWord : guessingWords_)
    {
        size_t totalSearchSpaceSize = 0;

        // Pretend that each of the remaining words is the solution and
        // see how well the current guess would prune the search space.
        for (auto const& possibleSolution : solutionWords)
        {
            auto score = Score(possibleSolution, guessWord);

            boardWithJustNewGuess.PushScoredGuess(guessWord, score);
            WordQuery query = boardWithJustNewGuess.GenerateQuery();

            size_t searchSpaceSize = SearchSpaceSize(query, solutionWords);
            totalSearchSpaceSize += searchSpaceSize;

            boardWithJustNewGuess.Pop();
        }
        if (totalSearchSpaceSize < bestGuess.second)
        {
            bestGuess.first = guessWord;
            bestGuess.second = totalSearchSpaceSize;
        }
    }

    return bestGuess;
}

std::vector<ScoredGuess> SearchStrategy::BestGuesses(Board& board,
                                     const std::vector<std::string>& solutionWords) const
{
    std::vector<ScoredGuess> guesses;
    if (solutionWords.size() == 1)
    {
        guesses.push_back(ScoredGuess(solutionWords[0], 1));
        return guesses;
    }

    std::vector< ScoredGuess > scoredGuesses;

    Board boardWithJustNewGuess(board.n);

    for (auto const& guessWord : guessingWords_)
    {
        size_t totalSearchSpaceSize = 0;

        // Pretend that each of the remaining words is the solution and
        // see how well the current guess would prune the search space.
        for (auto const& possibleSolution : solutionWords)
        {
            auto score = Score(possibleSolution, guessWord);

            boardWithJustNewGuess.PushScoredGuess(guessWord, score);
            WordQuery query = boardWithJustNewGuess.GenerateQuery();

            size_t searchSpaceSize = SearchSpaceSize(query, solutionWords);
            totalSearchSpaceSize += searchSpaceSize;

            boardWithJustNewGuess.Pop();
        }
        scoredGuesses.push_back(ScoredGuess(guessWord, (float)totalSearchSpaceSize));
    }

    // Sort them small to big
    std::sort(scoredGuesses.begin(), scoredGuesses.end(),
              [](const ScoredGuess& a, const ScoredGuess& b)
              {
                  return a.second < b.second;
              }
    );

    // Prune the guesses to something reasonable. The list might have started at 10,000
    for (size_t i = 0; i < 100 && i < scoredGuesses.size(); ++i)
    {
        guesses.push_back(scoredGuesses[i]);
    }

    // prefer the guesses in the solution list.
    for (auto &g : guesses)
    {
        if (std::binary_search(solutionWords.begin(), solutionWords.end(), g.first))
        {
            std::cout << "Preferring guess " << g.first << std::endl;
            g.second -= 0.5f;
        }
    }

    // Sort them small to big
    std::sort(guesses.begin(), guesses.end(),
              [](const ScoredGuess& a, const ScoredGuess& b)
              {
                  return a.second < b.second;
              }
    );

    // crude, I know
    while (guesses.size() > maxGuessesReturned_)
    {
        guesses.pop_back();
    }

    return guesses;
}

size_t  SearchStrategy::SearchSpaceSize(const WordQuery& query, const std::vector<std::string>& words)
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

///////////////

BlendedStrategy::BlendedStrategy(
        const std::vector<std::string>& guessingWords, size_t maxGuessesReturned)
        : entropyStrategy_(guessingWords, maxGuessesReturned)
        , searchStrategy_(guessingWords, maxGuessesReturned)

{
}

ScoredGuess BlendedStrategy::BestGuess(Board& board,
                      const std::vector<std::string>& solutionWords) const
{
    if (solutionWords.size() > 99)
        return entropyStrategy_.BestGuess(board, solutionWords);

    return searchStrategy_.BestGuess(board, solutionWords);
}

std::vector<ScoredGuess> BlendedStrategy::BestGuesses(Board& board,
                                     const std::vector<std::string>& solutionWords) const
{
    if (solutionWords.size() > 99)
        return entropyStrategy_.BestGuesses(board, solutionWords);

    return searchStrategy_.BestGuesses(board, solutionWords);
}

