
#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <string>

using ScoredGuess = std::pair< std::string, float >;
using FrequencyTable = std::array<float, 26>;

struct Strategy;

enum CharScore { NotPresent = 1, Correct = 2, CorrectNotHere = 3 };

bool LoadDictionary( const std::filesystem::path &filename, std::vector<std::string> &words);

void RemoveDuplicateGuesses( std::vector<ScoredGuess> &scoredGuesses);

uint32_t ComputeMask( const std::string &word);

bool LoadDictionaries(bool newYorkTimes, int n,
                      const std::filesystem::path &dictPath,    // dictionary directory
                      std::vector<std::string>& solutionWords,
                      std::vector<std::string>& guessingWords);


float TestWords(std::vector<std::string>& solutionWords, const std::vector < std::string>& guessingWords,
    const std::string& openingGuess, Strategy& strategy, bool verbose = true);