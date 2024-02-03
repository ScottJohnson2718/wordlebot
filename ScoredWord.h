
#pragma once

#include "Wordle.h"

using ScoredGuess = std::pair< std::string, float >;

// An array of CharScore but put into bits in a uint32_t.
// This was a std::vector< CharScore > but it was made into this after the allocations
// in the std::vector were high in the performance profile.
struct ScoredWord
{
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

    uint32_t v = 0;
};

ScoredWord Score(const std::string& solution, const std::string& guess);
