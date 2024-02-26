//
// Created by Scott Johnson on 1/21/24.
//

#include "WordQuery.h"

LetterCount CountLetters( const std::string &word)
{
    LetterCount c;
    for (int i = 0; i < 26; ++i)
        c[i] = 0;
    for (auto ch : word)
    {
        if (ch == '.')
            continue;
        ch = tolower(ch) - 'a';
        c[ch]++;
    }
    return c;
}