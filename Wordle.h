
#pragma once

#include <string>
#include <filesystem>
#include <vector>

using ScoredGuess = std::pair< std::string, float >;
using FrequencyTable = std::array<float, 26>;

struct WordQuery;

enum CharScore { NotPresent = 1, Correct = 2, CorrectNotHere = 3 };

void LoadDictionary( const std::filesystem::path &filename, std::vector<std::string> &words);
std::vector<std::string>  PruneSearchSpace(const WordQuery& query, const std::vector<std::string>& words);

uint32_t ComputeMask( const std::string &word);

void LoadDictionaries(bool newYorkTimes, int n,
                      const std::filesystem::path &dictPath,    // dictionary directory
                      std::vector<std::string>& solutionWords,
                      std::vector<std::string>& guessingWords);

