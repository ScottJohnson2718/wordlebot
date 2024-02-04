
#pragma once

#include <vector>
#include <string>
#include <array>

#include "Wordle.h"

struct WordQuery;

std::array<float, 26> charFrequency(const std::vector<std::string> &words);
std::array<float, 26> charFrequency(const std::vector<std::string> &words, const WordQuery &query);

std::array<float, 26> removeKnownChars(const std::array<float, 26> &freq, const std::string &contains);

float entropy( const std::string &word, const std::array<float, 26> &freqs );
