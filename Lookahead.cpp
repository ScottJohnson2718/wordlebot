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

    const auto& candidateWords = (solutionWords.size() <= 6)
                                 ? solutionWords
                                 : guessingWords_;

    for (const auto& guessWord : candidateWords) {
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

    const auto& candidateWords = (solutionWords.size() <= 6)
                                 ? solutionWords
                                 : guessingWords_;

    for (const auto& guessWord : candidateWords) {
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

    for (size_t i = 0; i < std::min(maxGuessesReturned_, scoredGuesses.size()); ++i) {
        topGuessesByScore.push_back(scoredGuesses[i]);
    }

    return topGuessesByScore;
}