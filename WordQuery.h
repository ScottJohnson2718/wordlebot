#pragma once

#include <string>
#include <stdint.h>
#include <vector>

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
    // Per character, the letters it cant contain.
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

    // Pretend like none of the characters in this guess are in the solution
    // The idea is to pretend that we guess the given guess and that the
    // new letters are not found. That would prune the search space a lot.
    // We prune best when we introduce new letters to the query.
    //void ScoreCandidate(const std::string &guess)
    //{
    //    for (size_t i = 0; i < guess.size(); ++i) {
    //        char ch = guess[i];

    //        // Don't consider letters that are already correct
    //        if (guess[i] != correct[i]) {
    //            uint32_t charMask = (1 << (ch - 'a'));

    //            // Don't consider letters that we must have
    //            if ((charMask & mustContain) == 0) {
    //                // This is a new letter. Pretend it is not in the solution
    //                SetCantContain(ch);
    //            }
    //        }
    //    }
    //}

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

