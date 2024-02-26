#pragma once

#include <string>
#include <stdint.h>
#include <vector>
#include <array>

using LetterCount = std::array<int,26>;

LetterCount CountLetters( const std::string &word);

struct WordQuery
{
    WordQuery(int _n)
            : n(_n)
            , mustContain(0)
            , correct(std::string(n, '.'))
    {
        cantContain.resize(n);
        for (int i = 0; i < 26; ++i)
        {
            minOverall[i] = 0;
            maxOverall[i] = n;
        }
    }

    int n;  // character count per word

    // The letters the word must contain
    uint32_t mustContain;
    // Per character, a mask of the letters it cant contain.
    std::vector<uint32_t> cantContain;

    // Min and max for overall word. This handles the requirement that some letters
    // have a minimum required count and a maximum required count overall in the word.
    LetterCount minOverall;
    LetterCount maxOverall;

    // List of characters in the correct place.
    // A dot means a blank space
    std::string correct;

    // No character can contain the character ch
    void SetCantContain(char ch)
    {
        int charIndex = tolower(ch) - 'a';
        for (int i = 0; i < n; ++i)
            cantContain[i] |= (1 << charIndex);
        maxOverall[charIndex] = 0;
    }

    // The given character index cannot contain the given character
    void SetCantContain(int charIndex, char ch)
    {
        ch = tolower(ch);
        cantContain[charIndex] |= (1 << (ch - 'a'));
    }

     void SetMustContain( char ch )
    {
        int charIndex = tolower(ch) - 'a';
        mustContain |= (1 << charIndex);
        
        minOverall[charIndex] = std::min(minOverall[charIndex], 1);
    }

    void SetCorrect( int charIndex, char ch)
    {
        ch = tolower(ch);
        correct[charIndex] = ch;
        mustContain |= (1 << (ch - 'a'));
        minOverall[ch - 'a'] = std::min(minOverall[ch - 'a'], 1);
    }

    void SetMinimumLetterCount( int count, char ch)
    {
        minOverall[ch] = count;
    }

    void SetMaximumLetterCount( int count, char ch)
    {
        maxOverall[ch] = count;
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

        LetterCount letterCount = CountLetters(word);
        for (int i = 0; i < 26; ++i)
        {
            if (letterCount[i] < minOverall[i])
                return false;
            if (letterCount[i] > maxOverall[i])
                return false;
        }

        return true;
    }
};

