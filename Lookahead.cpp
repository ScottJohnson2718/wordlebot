#include "Lookahead.h"
#include <iostream>
#include <algorithm>
#include <thread>

ScoredGuess ScoreGroupingLookaheadStrategy::BestGuess(
        Board& board,
        const std::vector<std::string>& solutionWords) const {

    if (solutionWords.size() == 1)
    {
        return ScoredGuess(solutionWords[0], 1);
    }

    // Clear cache for new evaluation
    cache_.clear();

    // Determine effective lookahead depth
    int effectiveDepth = lookaheadDepth_;
    if (useAdaptiveLookahead_) {
        effectiveDepth = GetAdaptiveLookahead(solutionWords.size());
    }

    // Use lookahead only when solution space is manageable
    bool useLookahead = effectiveDepth > 0 && solutionWords.size() <= 100;

    const auto& candidateWords = (solutionWords.size() <= 6)
                                 ? solutionWords
                                 : guessingWords_;

    // Thread-safe result storage
    std::atomic<float> bestScore(std::numeric_limits<float>::max());
    std::string bestGuessWord;
    std::mutex resultMutex;

    const size_t num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    const size_t total_candidates = candidateWords.size();
    const size_t chunk_size = (total_candidates + num_threads - 1) / num_threads;

    for (size_t t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            const size_t start = t * chunk_size;
            const size_t end = std::min(start + chunk_size, total_candidates);

            float localBestScore = std::numeric_limits<float>::max();
            std::string localBestGuess;

            for (size_t i = start; i < end; ++i) {
                const auto& guessWord = candidateWords[i];
                float score;

                if (useLookahead) {
                    score = ExpectedGuessesWithLookahead(
                            guessWord, solutionWords, effectiveDepth);
                }
                else {
                    score = ExpectedRemainingWords(guessWord, solutionWords);
                }

                if (score < localBestScore) {
                    localBestScore = score;
                    localBestGuess = guessWord;
                }
            }

            // Update global best if local best is better
            std::lock_guard<std::mutex> lock(resultMutex);
            float currentBest = bestScore.load();
            if (localBestScore < currentBest) {
                bestScore.store(localBestScore);
                bestGuessWord = localBestGuess;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return ScoredGuess(bestGuessWord, bestScore.load());
}

std::vector<ScoredGuess> ScoreGroupingLookaheadStrategy::BestGuesses(
        Board& board,
        const std::vector<std::string>& solutionWords) const {

    std::vector<ScoredGuess> scoredGuesses;

    if (solutionWords.size() == 1) {
        scoredGuesses.push_back(ScoredGuess(solutionWords[0], 1));
        return scoredGuesses;
    }

    // Clear cache for new evaluation
    cache_.clear();

    // Determine effective lookahead depth
    int effectiveDepth = lookaheadDepth_;
    if (useAdaptiveLookahead_) {
        effectiveDepth = GetAdaptiveLookahead(solutionWords.size());
    }

    // Use lookahead only when solution space is manageable
    bool useLookahead = effectiveDepth > 0 && solutionWords.size() <= 100;

    for (const auto& guessWord : guessingWords_) {
        float score;

        if (useLookahead) {
            score = ExpectedGuessesWithLookahead(
                    guessWord, solutionWords, effectiveDepth);
        }
        else {
            score = ExpectedRemainingWords(guessWord, solutionWords);
        }

        scoredGuesses.push_back(ScoredGuess(guessWord, score));
    }

    std::vector<ScoredGuess> topGuessesByScore;

    // Remove duplicate guesses by string
    RemoveDuplicateGuesses(scoredGuesses);

    // Sort them small to large (lower expected guesses is better)
    std::sort(scoredGuesses.begin(), scoredGuesses.end(),
              [](const ScoredGuess& a, const ScoredGuess& b) {
                  return a.second < b.second;
              });

    // When few solutions remain, ensure they're included if competitive
    if (solutionWords.size() <= 10) {
        // Find the best score threshold
        float bestScore = scoredGuesses.empty() ? 0.0f : scoredGuesses[0].second;
        float threshold = bestScore + 0.5f; // Include anything within 0.5 of best

        // First, add all solution words that are competitive
        for (const auto& solutionWord : solutionWords) {
            auto it = std::find_if(scoredGuesses.begin(), scoredGuesses.end(),
                                   [&solutionWord](const ScoredGuess& sg) { return sg.first == solutionWord; });
            if (it != scoredGuesses.end() && it->second <= threshold) {
                topGuessesByScore.push_back(*it);
            }
        }

        // Then add other top guesses up to the limit
        for (const auto& guess : scoredGuesses) {
            if (topGuessesByScore.size() >= maxGuessesReturned_) break;

            // Skip if already added (was a solution word)
            bool alreadyAdded = std::find_if(topGuessesByScore.begin(), topGuessesByScore.end(),
                                             [&guess](const ScoredGuess& sg) { return sg.first == guess.first; }) != topGuessesByScore.end();
            if (!alreadyAdded) {
                topGuessesByScore.push_back(guess);
            }
        }
    } else {
        // Normal case: just take top N
        for (size_t i = 0; i < std::min(maxGuessesReturned_, scoredGuesses.size()); ++i) {
            topGuessesByScore.push_back(scoredGuesses[i]);
        }
    }

    // Sort them small to large (lower expected guesses is better)
    std::sort(topGuessesByScore.begin(), topGuessesByScore.end(),
              [](const ScoredGuess& a, const ScoredGuess& b) {
                  return a.second < b.second;
              });


    return topGuessesByScore;
}