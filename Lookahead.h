#pragma once

#include "Strategy.h"
#include <map>
#include <unordered_map>
#include <limits>
#include <algorithm>

class ScoreGroupingLookaheadStrategy : public Strategy {
private:
    std::vector<std::string> guessingWords_;
    size_t maxGuessesReturned_;
    int lookaheadDepth_;
    bool useAdaptiveLookahead_;
    size_t maxCandidatesInLookahead_;
    
    // Cache for memoization
    mutable std::unordered_map<std::string, float> cache_;
    
public:
    ScoreGroupingLookaheadStrategy(
        const std::vector<std::string>& guessingWords,
        size_t maxGuessesReturned,
        int lookaheadDepth = 1,
        bool useAdaptiveLookahead = true,
        size_t maxCandidatesInLookahead = 100)
        : guessingWords_(guessingWords)
        , maxGuessesReturned_(maxGuessesReturned)
        , lookaheadDepth_(lookaheadDepth)
        , useAdaptiveLookahead_(useAdaptiveLookahead)
        , maxCandidatesInLookahead_(maxCandidatesInLookahead)
    {
    }
    
    ScoredGuess BestGuess(Board& board,
        const std::vector<std::string>& solutionWords) const override;
    
    std::vector<ScoredGuess> BestGuesses(Board& board,
        const std::vector<std::string>& solutionWords) const override;
    
private:
    // Determine lookahead depth based on solution space size
    int GetAdaptiveLookahead(size_t solutionCount) const {
        if (solutionCount <= 10) 
            return 3;   // Deep lookahead when very close
        if (solutionCount <= 30) 
            return 2;   // Medium lookahead
        if (solutionCount <= 100)
            return 1;  // Shallow lookahead
        return 0;                             // No lookahead for large spaces
    }
    
    // Original expected remaining words (no lookahead)
    float ExpectedRemainingWords(
        const std::string& guessWord,
        const std::vector<std::string>& solutionWords) const {
        
        auto scoreGroups = GroupByScore(guessWord, solutionWords);
        
        float expected = 0;
        for (const auto& [score, group] : scoreGroups) {
            float probability = (float)group.size() / solutionWords.size();
            expected += probability * group.size();
        }
        
        return expected;
    }
    
    // Expected number of guesses with lookahead
    float ExpectedGuessesWithLookahead(
        const std::string& guessWord,
        const std::vector<std::string>& solutionWords,
        int depth,
        float alpha = std::numeric_limits<float>::max()) const {
        
        if (depth == 0 || solutionWords.size() <= 1) {
            return ExpectedRemainingWords(guessWord, solutionWords);
        }
        
        // Check cache
        std::string cacheKey = CreateCacheKey(guessWord, solutionWords, depth);
        auto it = cache_.find(cacheKey);
        if (it != cache_.end()) {
            return it->second;
        }
        
        auto scoreGroups = GroupByScore(guessWord, solutionWords);
        
        float totalExpectedGuesses = 0;
        
        for (const auto& [score, remainingWords] : scoreGroups) {
            if (remainingWords.empty()) 
                continue;
            
            float probability = (float)remainingWords.size() / solutionWords.size();
            float contribution;
            
            if (remainingWords.size() == 1) {
                // Solved after this guess
                contribution = probability * 1.0f;
            } else {
                // Need more guesses - find the best next guess for this branch
                float bestNextScore = FindBestNextGuess(remainingWords, depth - 1);
                
                // Add 1 for the current guess, plus expected guesses after
                contribution = probability * (1.0f + bestNextScore);
            }
            
            totalExpectedGuesses += contribution;
            
            // Early pruning if we're already worse than best found
            if (totalExpectedGuesses >= alpha) {
                cache_[cacheKey] = alpha;
                return alpha;
            }
        }
        
        cache_[cacheKey] = totalExpectedGuesses;
        return totalExpectedGuesses;
    }
    
    // Find the best next guess for a given set of remaining words
    float FindBestNextGuess(
        const std::vector<std::string>& remainingWords,
        int depth) const {
        
        if (remainingWords.size() == 1) {
            return 0.0f;  // No more guesses needed
        }
        
        float bestScore = std::numeric_limits<float>::max();
        
        // Choose candidates for next guess
        const auto& nextCandidates = GetCandidatesForLookahead(remainingWords);
        
        for (const auto& nextGuess : nextCandidates) {
            float score = ExpectedGuessesWithLookahead(
                nextGuess, remainingWords, depth, bestScore);
            bestScore = std::min(bestScore, score);
        }
        
        return bestScore;
    }
    
    // Get candidate words to consider in lookahead
    std::vector<std::string> GetCandidatesForLookahead(
        const std::vector<std::string>& solutionWords) const {
        
        // If very few solutions left, just use them
        if (solutionWords.size() <= 6) {
            return solutionWords;
        }
        
        // Otherwise, get top candidates from guessing words
        return GetTopCandidates(solutionWords, maxCandidatesInLookahead_);
    }
    
    // Quick heuristic to get top N candidates without full evaluation
    std::vector<std::string> GetTopCandidates(
        const std::vector<std::string>& solutionWords,
        size_t maxCandidates) const {
        
        std::vector<ScoredGuess> quickScores;
        
        // Sample words from guessing dictionary for quick evaluation
        size_t sampleSize = std::min(500, (int) guessingWords_.size());
        size_t step = std::max(1, (int) (guessingWords_.size() / sampleSize));
        
        for (size_t i = 0; i < guessingWords_.size(); i += step) {
            float score = ExpectedRemainingWords(guessingWords_[i], solutionWords);
            quickScores.push_back(ScoredGuess(guessingWords_[i], score));
        }
        
        // Also include some solution words
        for (const auto& word : solutionWords) {
            if (quickScores.size() < sampleSize) {
                float score = ExpectedRemainingWords(word, solutionWords);
                quickScores.push_back(ScoredGuess(word, score));
            }
        }
        
        std::sort(quickScores.begin(), quickScores.end(),
            [](const ScoredGuess& a, const ScoredGuess& b) {
                return a.second < b.second;
            });
        
        std::vector<std::string> result;
        for (size_t i = 0; i < std::min(maxCandidates, quickScores.size()); ++i) {
            result.push_back(quickScores[i].first);
        }
        
        return result;
    }
    
    // Group solutions by their score against a guess
    std::map<ScoredWord, std::vector<std::string>> GroupByScore(
        const std::string& guessWord,
        const std::vector<std::string>& solutionWords) const {
        
        std::map<ScoredWord, std::vector<std::string>> groups;
        for (const auto& solution : solutionWords) {
            ScoredWord score = Score(solution, guessWord);
            groups[score].push_back(solution);
        }
        return groups;
    }
    
    // Create a cache key for memoization
    std::string CreateCacheKey(
        const std::string& guessWord,
        const std::vector<std::string>& solutionWords,
        int depth) const {
        
        // Simple cache key - could be improved with hash
        std::string key = guessWord + "_" + std::to_string(depth) + "_" + 
                         std::to_string(solutionWords.size());
        
        // Add first and last solution words for better uniqueness
        if (!solutionWords.empty()) {
            key += "_" + solutionWords.front();
            if (solutionWords.size() > 1) {
                key += "_" + solutionWords.back();
            }
        }
        
        return key;
    }
};