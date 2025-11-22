#include "Lookahead.h"
#include <iostream>
#include <algorithm>

ScoredGuess ScoreGroupingLookaheadStrategy::BestGuess(
        Board& board,
        const std::vector<std::string>& solutionWords) const {

    if (solutionWords.size() == 1) {
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

    ScoredGuess bestGuess;
    float bestScore = std::numeric_limits<float>::max();

    for (const auto& guessWord : guessingWords_) {
        float score;

        if (useLookahead) {
            score = ExpectedGuessesWithLookahead(
                    guessWord, solutionWords, effectiveDepth);
        }
        else {
            score = ExpectedRemainingWords(guessWord, solutionWords);
        }

        if (score < bestScore) {
            bestScore = score;
            bestGuess.first = guessWord;
            bestGuess.second = score;
        }
    }

    return bestGuess;
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

    return topGuessesByScore;
}