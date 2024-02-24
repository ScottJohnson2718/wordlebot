#pragma once

#include <string>
#include <stdint.h>
#include <vector>
#include <array>

struct WordQuery
{
    WordQuery(int _n)
            : n(_n)
            , mustContain(0)
            , correct(std::string(n, '.'))
    {
        cantContain.resize(n);
    }

    int n;  // character count per word

    // The letters the word must contain
    uint32_t mustContain;
    // Per character, a mask of the letters it cant contain.
    std::vector<uint32_t> cantContain;

    // List of characters in the correct place.
    // A dot means a blank space
    std::string correct;

    // No character can contain the character ch
    void SetCantContain(char ch)
    {
        ch = tolower(ch);
        for (int i = 0; i < n; ++i)
            cantContain[i] |= (1 << (ch - 'a'));
    }

    // The given character index cannot contain the given character
    void SetCantContain(int charIndex, char ch)
    {
        ch = tolower(ch);
        cantContain[charIndex] |= (1 << (ch - 'a'));
    }

     void SetMustContain( char ch )
    {
        ch = tolower(ch);
        mustContain |= (1 << (ch - 'a'));
    }

    void SetCorrect( int charIndex, char ch)
    {
        ch = tolower(ch);
        correct[charIndex] = ch;
        mustContain |= (1 << (ch - 'a'));
    }

    bool Satisfies(const std::string &word) const
    {
        uint32_t wordMask = 0;

        for (size_t i = 0; i < cantContain.size(); ++i)
        {
            if (correct[i] != '.' && correct[i] != word[i])
            {
                return false;
            }
            uint32_t charMask = 1 << (word[i] - 'a');
            wordMask |= charMask;

            if (cantContain[i] & charMask)
                return false;
        }

        // The word must contain all the characters in the mustContain mask
        if ((mustContain & wordMask) != mustContain)
            return false;

        return true;
    }
};

