
#pragma once

#include "Wordle.h"

#include <set>

using ScoredGuess = std::pair< std::string, float >;
struct ScoredWord;

std::ostream& print(std::ostream &str, const std::string &guess, const ScoredWord &scored);

// An array of CharScore but put into bits in a uint32_t.
// This was a std::vector< CharScore > but it was made into this after the allocations
// in the std::vector were high in the performance profile.
struct ScoredWord
{
    ScoredWord() = default;

    // This is more of a debug constructor for hard coded test cases
    ScoredWord(const std::string& scoreStr)
    {
        for (int i = 0; i < scoreStr.size(); ++i)
        {
            char ch = scoreStr[i];

            if (ch == '.')
            {
                Set(i, NotPresent);
            }
            else if (islower(ch))
            {
                Set(i, Correct);
            }
            else
            {
                Set(i, CorrectNotHere);
            }
        }
    }
    void Set(int index, CharScore cs)
    {
        v |= cs << (index << 1);
    }

    CharScore Get(int index) const
    {
        index <<= 1;
        return static_cast<CharScore>((v >> index) & 3);
    }

    CharScore operator[](int index) const
    {
        return Get(index);
    }

    bool operator <(const ScoredWord &s) const
    {
        return v < s.v;
    }

    std::string ToString(const std::string &guess) const;
    std::string ToStringColored(const std::string &guess) const;

    bool operator==(const ScoredWord& rhs) const
    {
        return v == rhs.v;
    }


    uint32_t v = 0;
};

struct ScoredWordHash
{
    std::size_t operator()(const ScoredWord& s) const
    {
        return std::hash<uint32_t>()(s.v);
    }
};

struct ScoredWordEqual
{
    bool operator()(const ScoredWord& lhs, const ScoredWord& rhs) const
    {
        return lhs.v == rhs.v;
    }
};

ScoredWord Score(const std::string& solution, const std::string& guess);
std::string ScoreString(const std::string &solution, const std::string &guess);

std::ostream& print(std::ostream &str, const std::string &guess, const ScoredWord &scored);

// Return the number of score groups the given guess breaks the solutions into.
size_t ScoreGroupCount(const std::string& guessWord, const std::vector<std::string>& solutionWords, size_t &largestGroup);
// Return the solution words that all have the same score as the target score
std::vector<std::string> ScoreGroup(const std::string& guessWord, const ScoredWord& target,
    const std::vector < std::string>& solutionWords);

std::set<ScoredWord> ScoresByGuess(const std::string& guessWord,
                                      const std::vector < std::string>& solutionWords );
